// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "contribution_review_journal.hpp"

#include <QPointer>

namespace subathon::server::review_detail {

inline std::string head_token(const SqliteWorkSession &s) {
    const auto rows = s.execute({"SELECT token FROM main.subathon_app_head WHERE singleton=1", {}});
    require(rows.size() == 1);
    return text_cell(rows[0][0]);
}

inline std::string live_digest(const Row &row, const ApplicationStateSnapshot &state) {
    Encode w;
    w.flag(bool(row.inputs));
    w.b(row.permission_revision);
    if (!row.inputs)
        return hash(w.data);
    const auto &input = *row.inputs;
    w.ref(input.plan.plan_ref);
    plan_fields(w, input.selected_plan);
    state_fields(w, input.current_contribution->contribution.state);
    w.ref(state.configuration_ref);
    w.tag(state.timer.lifecycle, core::TimerLifecycle::Finished);
    w.tag(state.timer.phase, core::CountdownPhase::Grace);
    w.i64(state.timer.main_remaining_ms);
    w.i64(state.timer.grace_remaining_ms);
    w.flag(state.timer.awaiting_confirmation);
    w.i64(state.time_debt_ms);
    w.vec(state.balances, [&](const auto &b) {
        w.u8(unsigned(b.kind));
        w.ref(b.target_ref);
        w.i64(b.units);
    });
    w.vec(row.mixed, [&](const auto &h) {
        w.ref(h.hold_ref);
        w.ref(h.version_ref);
    });
    w.opt(row.pending, [&](const auto &d) {
        w.ref(d.ref);
        w.u8(unsigned(d.mode));
    });
    w.flag(row.rejected);
    w.flag(row.credited);
    w.opt(row.policy, [&](const auto &p) {
        w.ref(p.revision);
        w.opt(p.cap, [&](auto c) { w.i64(c); });
        w.tag(p.timer.end_mode, core::TimerEndMode::Confirmation);
        w.i64(p.timer.grace_duration_ms);
        w.tag(p.timer.grace_reuse, core::GraceReuse::Shared);
        w.opt(p.timer.hard_end_utc_ms, [&](auto end) { w.i64(end); });
        w.flag(p.clock.paused);
        w.opt(p.finished_siblings, [&](const auto &list) {
            w.vec(list, [&](auto unit) { w.tag(unit, Unit::whole_count); });
        });
        w.vec(p.milestones, [&](const auto &target) {
            w.ref(target.milestone_ref);
            w.i64(target.target_micro_points);
        });
        w.vec(p.signed_original, [&](const auto &policy) {
            w.i64(policy.request_ordinal);
            w.tag(policy.deficit_policy, Deficit::RequireReview);
        });
        w.flag(p.may_apply_through_financial_hold);
    });
    w.flag(row.can_decide);
    w.flag(row.can_apply);
    w.flag(row.bad_choices);
    w.tag(row.before.timer.lifecycle, core::TimerLifecycle::Finished);
    w.tag(row.before.timer.phase, core::CountdownPhase::Grace);
    w.flag(row.before.timer.awaiting_confirmation);
    w.b(row.partition);
    w.vec(row.occurrences, [&](const auto &o) { w.ref(o.ref); });
    w.vec(row.blockers, [&](const auto b) { w.u8(unsigned(b)); });
    w.vec(row.jobs, [&](const auto &j) { application_result_wire::put_job(w, j); });
    w.opt(row.deficit, [&](const auto &d) { timer_fields(w, d); });
    w.opt(row.detected, [&](const auto &d) {
        w.i64(d.request_ordinal);
        w.i64(d.request_units);
    });
    w.opt(row.overflow, [&](const auto &failure) {
        w.opt(failure.failing_ordinal, [&](auto n) { w.i64(n); });
        w.opt(failure.attempted_units, [&](const auto &n) { w.exact(n); });
    });
    return hash(w.data);
}

inline Row read_row(const SqliteWorkSession &s, const Authority &auth, const ReviewEntryIntent &i,
                    const std::optional<ApplicationStateSnapshot> &projected = {},
                    const std::set<std::string> &completed = {}, bool preview = false) {
    Row row;
    const auto preliminary = auth.check(s, ReviewAuthorityUse::execute_row, &i);
    if (!allowed(preliminary))
        return row;
    row.authorized = true;
    row.permission_revision =
        std::get<ApplicationAccessAllowed>(preliminary).permission_revision_ref;
    row.disclose = allowed(auth.check(s, ReviewAuthorityUse::disclose_row, &i));
    if (preview && !row.disclose)
        return row;
    if (!anchor(s, i.selector))
        return row;
    const auto key = plan_key(s, i.selector);
    if (!key)
        return row;
    row.policy = auth.policy(s, i.selector);
    bounded_shape(s, i, row.policy);
    auto result =
        SqliteApplicationBundleStore(s).read_inputs(*key, ApplicationSubjectKind::contribution);
    require(result.inputs && result.inputs->current_contribution &&
            result.inputs->current_contribution->contribution.id == i.selector.cid &&
            result.inputs->current_contribution->contribution.original_receipt_id ==
                i.selector.receipt);
    row.inputs = std::move(result.inputs);
    row.pending = decision(s, i.selector, row.rejected);
    row.credited = claimed(s, i.selector);
    collect_holds(s, row, i);
    row.before = projected ? *projected : row.inputs->current_state;
    try {
        plan_row(row, s, i, auth.principal.installation_ref, completed);
    } catch (const std::overflow_error &) {
        row.can_apply = false;
        row.effects.clear();
    }
    {
        const std::vector<ApplicationEffect> empty;
        const auto *proposed = row.can_apply                        ? &row.effects
                               : row.can_decide && !row.bad_choices ? &empty
                                                                    : nullptr;
        const auto execution =
            auth.check(s, ReviewAuthorityUse::execute_row, &i, nullptr, proposed);
        row.authorized = allowed(execution);
        if (row.authorized)
            row.permission_revision =
                std::get<ApplicationAccessAllowed>(execution).permission_revision_ref;
        row.disclose = row.authorized && allowed(auth.check(s, ReviewAuthorityUse::disclose_row, &i,
                                                            nullptr, proposed));
    }
    return row;
}

inline std::vector<ReviewPreviewRow> project(const SqliteWorkSession &s, const Authority &auth,
                                             const ReviewPreviewRequest &request) {
    std::vector<std::string> intents;
    for (const auto &entry : request.entries)
        intents.push_back(digest_intent(entry, !entry.milestones));
    const auto selection = selection_digest(request.principal, intents);
    std::optional<ApplicationStateSnapshot> projected;
    std::set<std::string> completed;
    std::vector<ReviewPreviewRow> result;
    const auto root = head_token(s);
    for (const auto &entry : request.entries) {
        const auto n = result.size();
        auto row = read_row(s, auth, entry, projected, completed, true);
        if (!row.authorized || !row.disclose) {
            result.push_back({int(n + 1), {}});
            continue;
        }
        const auto live = row.inputs
                              ? live_digest(row, projected ? *projected : row.inputs->current_state)
                              : live_digest(row, {});
        const Basis basis{root, selection, intents[n], !entry.milestones, live};
        auto basis_bytes = basis.encode();
        Encode sizes;
        sizes.b(basis_bytes);
        selector_fields(sizes, entry.selector);
        sizes.u8(row.can_decide);
        sizes.u8(row.can_apply);
        sizes.vec(row.blockers, [&](auto b) { sizes.u8(unsigned(b)); });
        sizes.vec(row.effects,
                  [&](const auto &effect) { application_result_wire::put_effect(sizes, effect); });
        sizes.vec(row.occurrences, [&](const auto &o) { sizes.ref(o.ref); });
        sizes.opt(row.deficit, [&](const auto &t) { timer_fields(sizes, t); });
        result.push_back(
            {int(n + 1), ReviewPreviewFacts{entry.selector, std::move(basis_bytes), row.can_decide,
                                            row.can_apply && entry.verb != V::reject, row.blockers,
                                            row.effects, row.occurrences, row.deficit}});
        // Absent milestone choices may be filled after this preview. The
        // predecessor's effects remain prospective and must match on stage.
        if (row.can_apply && entry.verb != V::reject &&
            (!entry.milestones || exact_choices(row, entry))) {
            projected = row.after;
            for (const auto &group : row.groups)
                completed.insert(group.group.achievement.milestone_ref);
        }
    }
    return result;
}

inline void patch_review(const SqliteWorkSession &s, const Row &r, core::ContributionReview value) {
    const auto &record = r.inputs->current_contribution->contribution;
    // The owning read compared all current contribution dimensions in this
    // BEGIN IMMEDIATE. Between that read and this patch no foreign writer runs.
    one_change(s, {"UPDATE main.subathon_contributions SET review=? WHERE contribution_id=? "
                   "AND original_receipt_id=? AND review=? RETURNING 1",
                   {int(value), id(record.id.value), id(record.original_receipt_id.value),
                    int(record.state.conditions.review)}});
}

inline void resolve(const SqliteWorkSession &s, const ReviewCommand &c, int ordinal, Row &row,
                    StoredOutcome &audit) {
    const auto &entry = c.entries.at(size_t(ordinal - 1));
    auto &facts = *audit.outcome.facts;
    std::vector<ApplicationHoldVersionLocator> ordinary;
    for (const auto &h : row.ordinary)
        ordinary.push_back(h.current_version.locator);
    auto &closures = row.closures;
    for (size_t n = 0; n < closures.size(); ++n) {
        auto &closure = closures[n];
        if (closure.proposed_version_ref.empty())
            closure.proposed_version_ref = distinct_id(c, ordinal, "closed-" + std::to_string(n));
        audit.closed.push_back({closure.hold_ref, closure.proposed_version_ref});
    }
    std::optional<ApplicationHoldApplicationLink> link;
    if (facts.bundle)
        link = ApplicationHoldApplicationLink{ApplicationHoldApplicationLinkKind::whole_input,
                                              *facts.bundle,
                                              entry.intent.selector.receipt,
                                              {audit.plan, audit.selected_revision},
                                              {}};
    const ApplicationHoldResolutionBatch batch{distinct_id(c, ordinal, "hold-resolution"),
                                               {entry.intent.selector.cid},
                                               link ? row.mixed : ordinary,
                                               closures,
                                               c.actor,
                                               entry.decision_ref,
                                               entry.reason,
                                               {command_format, entry_document(entry)},
                                               link};
    if (link) {
        const auto result = stage_financial_aware_linked_resolution(s, batch);
        require(result.disposition == FinancialDisposition::ordinary_resolved);
    } else {
        const auto result = SqliteApplicationHoldStore(s).stage_resolution(batch);
        require(result.disposition == SqliteHoldMutationDisposition::recorded);
    }
}

inline void detect(const SqliteWorkSession &s, const ReviewCommand &c, int ordinal, const Row &row,
                   StoredOutcome &audit) {
    const auto &entry = c.entries.at(size_t(ordinal - 1));
    const auto &plan = entry.intent.revision ? *entry.intent.revision : row.inputs->selected_plan;
    const auto &event = plan.captured.event;
    const auto &policy = *row.policy;
    const auto h = distinct_id(c, ordinal, "hold");
    const auto v = distinct_id(c, ordinal, "held");
    const ApplicationEnvelope options{command_format, entry_document(entry)};
    const ApplicationHoldObservation observation{row.inputs->current_state.token,
                                                 options,
                                                 policy.revision,
                                                 row.permission_revision,
                                                 c.actor,
                                                 entry.decision_ref,
                                                 policy.clock.utc_now_ms,
                                                 policy.clock.elapsed_ms};
    ApplicationHoldPlanEvidence evidence;
    evidence.plan = ApplicationPlanKey{audit.plan, audit.selected_revision};
    for (const auto &q : plan.captured.requested_effects)
        evidence.occurrences.push_back({std::int64_t(evidence.occurrences.size() + 1), q.action_ref,
                                        q.action_version_ref, q.rule_ref, q.target_ref, q.unit,
                                        q.exact_requested_units, q.requested_units,
                                        q.calculation_explanation});
    const ApplicationHoldDetection detection{
        {h,
         {{entry.intent.selector.cid},
          event.receipt_id,
          event.event_ref,
          event.parent_event_ref,
          event.root_event_ref,
          event.arrival.configuration_ref,
          event.arrival.modifier_context_ref},
         {{"review-hold", distinct_id(c, ordinal, "hold-command")}, options},
         observation,
         std::move(evidence),
         row.detected ? ApplicationHoldCauseKind::timer_deficit
                      : ApplicationHoldCauseKind::numeric_overflow,
         row.overflow,
         row.detected,
         {},
         {}},
        {distinct_id(c, ordinal, "hold-detection"), {}, v, observation, options}};
    const auto result = SqliteApplicationHoldStore(s).record_detection(detection);
    require(result.disposition == SqliteHoldMutationDisposition::recorded);
    audit.created = ApplicationHoldVersionLocator{h, v};
    if (row.detected)
        audit.outcome.facts->timer =
            ReviewTimerSelector{{h, v}, row.detected->request_ordinal, row.detected->request_units};
}

inline void stage_application(const SqliteWorkSession &s, const ReviewCommand &c, int ordinal,
                              Row &row, StoredOutcome &data) {
    const auto &entry = c.entries.at(size_t(ordinal - 1));
    const auto &selected = entry.intent.selector;
    const auto key = application_key(c, ordinal);
    const auto &head = row.inputs->current_state;
    const auto &contribution = row.inputs->current_contribution->contribution;
    auto expected = contribution.state;
    if (entry.intent.verb == V::accept)
        expected.conditions.review = core::ContributionReview::accepted;
    const bool same_timer = bundle_detail::same_state(head, row.before);
    if (!same_timer)
        row.before.token = distinct_id(c, ordinal, "observed-state");
    row.after.token = distinct_id(c, ordinal, "result-state");
    const ApplicationEnvelope intent{command_format, entry_document(entry)};
    Encode doc(262144);
    const auto terminal_count =
        size_t(std::ranges::count_if(row.partition, [](char tag) { return tag != 0; }));
    doc.raw(terminal_count ? QByteArrayView("SAR2") : QByteArrayView("SAR1"));
    application_result_wire::put_result_fields(doc, intent, head.token, row.before.token,
                                               row.after.token, false, row.effects, row.jobs,
                                               row.transitions);
    if (terminal_count) {
        doc.u32(size_t(row.partition.size()));
        doc.u32(terminal_count);
        doc.raw(row.partition);
    }
    PreparedApplicationBundle bundle{
        {key,
         ApplicationOperationKind::original,
         ApplicationSubjectKind::contribution,
         selected.receipt,
         selected.cid,
         {data.plan, data.selected_revision},
         head.token,
         expected,
         intent,
         {}},
        row.before,
        row.after,
        same_timer ? "" : "review-clock",
        std::move(row.effects),
        std::move(row.jobs),
        std::move(row.transitions),
        {},
        {terminal_count ? terminal_partition::result_format
                        : "subathon.internal.application-result.v1",
         std::move(doc.data)},
        {c.actor, entry.decision_ref, entry.reason, {command_format, outcome_doc(data)}}};
    const SqliteApplicationBundleStore store(s);
    require(store.stage(bundle).disposition == SqliteBundleStageDisposition::staged);
    SqliteMilestoneGroupStore(s).stage(key, row.groups);
    if (!row.groups.empty())
        SqliteMilestoneGroupDecisions(s).record_acceptance(
            key, {entry.intent.mode, {c.actor, entry.decision_ref}, *entry.intent.milestones});
    const auto committed = store.lookup(key);
    require(committed && same(committed->intent.operation, key) &&
            committed->after.token == row.after.token &&
            committed->result.document == bundle.result.document);
}

inline StoredOutcome stage_row(const SqliteWorkSession &s, const ReviewCommand &c, int ordinal,
                               Row &row, const std::optional<Basis> &basis, bool selection_matches,
                               std::string_view previous_state) {
    const auto &entry = c.entries.at(size_t(ordinal - 1));
    const auto &intent = entry.intent;
    const auto verb = intent.verb;
    StoredOutcome data;
    if (!row.authorized) {
        data.outcome.status = S::denied;
        return data;
    }
    if (!row.inputs)
        return data;
    const auto &input = *row.inputs;
    data.outcome.facts = ReviewOutcomeFacts{intent.selector, {}, row.blockers, {}};
    data.decision_ref = entry.decision_ref;
    data.plan = input.plan.plan_ref;
    data.original_revision = input.plan.revision;
    data.selected_revision = input.plan.revision;
    data.live = live_digest(row, input.current_state);
    data.active = row.mixed;
    if (!basis || !selection_matches || basis->live != data.live ||
        input.current_state.token != (previous_state.empty() ? basis->root : previous_state)) {
        data.outcome.status = S::stale;
        return data;
    }

    const bool apply = row.can_apply && verb != V::reject;
    std::set<std::string_view> occurrences;
    if (intent.milestones && !std::ranges::all_of(*intent.milestones, [&](const auto &choice) {
            return occurrences.insert(choice.occurrence.ref).second;
        }))
        return data;
    if (!row.can_decide || row.bad_choices || (apply && !exact_choices(row, intent)) ||
        (!apply && (verb == V::apply_accepted || verb == V::continue_timer)) ||
        (intent.revision &&
         (!usable_revision(row, intent) ||
          intent.revision->override_decision->actor_ref != c.actor ||
          intent.revision->override_decision->decision_ref != entry.decision_ref ||
          intent.revision->override_decision->reason != entry.reason)))
        return data;

    using R = core::ContributionReview;
    if (verb == V::retain_held) {
        data.outcome.status = S::retained;
        data.outcome.facts->timer = intent.timer;
        return data;
    }

    if (verb == V::reject || verb == V::reject_held) {
        data.outcome.status = S::rejected;
        data.outcome.facts->timer = intent.timer;
        if (!row.credited)
            patch_review(s, row, R::rejected);
        if (verb == V::reject_held)
            resolve(s, c, ordinal, row, data);
        return data;
    }
    if (intent.revision) {
        require(SqliteApplicationPlanStore(s).append(data.plan, data.original_revision,
                                                     *intent.revision) ==
                SqlitePlanAppendDisposition::appended);
        data.selected_revision = intent.revision->revision;
    }
    if (verb == V::accept)
        patch_review(s, row, R::accepted);
    if (!apply) {
        data.outcome.status = S::accepted_pending;
        if (row.detected || row.overflow)
            detect(s, c, ordinal, row, data);
        return data;
    }
    data.outcome.status = verb == V::continue_timer ? S::timer_resolved : S::applied;
    data.outcome.facts->bundle = application_key(c, ordinal);
    data.outcome.facts->timer = intent.timer;
    // Reserve produced closure identities before encoding the bundle audit.
    for (size_t n = 0; n < row.closures.size(); ++n) {
        auto &closure = row.closures[n];
        if (closure.proposed_version_ref.empty())
            closure.proposed_version_ref = distinct_id(c, ordinal, "closed-" + std::to_string(n));
        data.closed.push_back({closure.hold_ref, closure.proposed_version_ref});
    }
    stage_application(s, c, ordinal, row, data);
    if (!row.closures.empty()) {
        data.closed.clear();
        resolve(s, c, ordinal, row, data);
    }
    return data;
}

} // namespace subathon::server::review_detail
