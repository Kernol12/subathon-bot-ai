// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "contribution_review_access.hpp"
#include "contribution_review_hold_history.hpp"
#include "contribution_review_wire.hpp"
#include "sqlite_application_bundle_values.hpp"
#include "sqlite_terminal_partition.hpp"

#include <QCryptographicHash>
#include <limits>
#include <subathon/server/sqlite_financial_refunds.hpp>

namespace subathon::server::review_detail {

using V = ReviewVerb;
using S = ReviewStatus;
using D = ReviewDisposition;
using Kind = ApplicationComponentKind;
using Unit = core::RequestedUnit;
using Deficit = core::RefundDeficitPolicy;

inline QVariant id(std::uint64_t value) { return encode_plan_revision_uint64(value); }
inline bool same(const ApplicationOperationKey &a, const ApplicationOperationKey &b) {
    return a.operation_scope == b.operation_scope && a.operation_ref == b.operation_ref;
}
inline bool same(const ApplicationPrincipalBinding &a, const ApplicationPrincipalBinding &b) {
    return a.kind == b.kind && a.installation_ref == b.installation_ref &&
           a.principal_ref == b.principal_ref;
}
inline bool same(const ApplicationHoldVersionLocator &a, const ApplicationHoldVersionLocator &b) {
    return a.hold_ref == b.hold_ref && a.version_ref == b.version_ref;
}
inline std::string hash(QByteArrayView data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex().toStdString();
}

inline std::string distinct_id(const ReviewCommand &c, int ordinal, std::string_view lane) {
    Encode out(envelope_limit);
    out.b(c.principal.installation_ref);
    out.b(c.key.operation_scope);
    out.b(c.key.operation_ref);
    out.u32(size_t(ordinal));
    out.b(lane);
    return "review-" + hash(out.data);
}

inline std::string digest_intent(const ReviewEntryIntent &intent, bool discover_choices) {
    Encode out;
    selector_fields(out, intent.selector);
    out.tag(intent.verb, V::retain_held);
    if (discover_choices) {
        auto copy = intent;
        copy.milestones.reset();
        options(out, copy);
    } else {
        options(out, intent);
    }
    return hash(out.data);
}

struct Basis {
    std::string root;
    std::string selection;
    std::string intent;
    bool discover_choices{};
    std::string live;

    QByteArray encode() const {
        Encode w;
        w.raw("SRB1");
        w.ref(root);
        w.ref(selection);
        w.ref(intent);
        w.flag(discover_choices);
        w.ref(live);
        return std::move(w.data);
    }

    static Basis decode(const QByteArray &doc) {
        Decode r(doc);
        r.magic("SRB1");
        Basis b;
        r.ref(b.root);
        r.ref(b.selection);
        r.ref(b.intent);
        r.flag(b.discover_choices);
        r.ref(b.live);
        r.end();
        require(b.selection.size() == 64 && b.intent.size() == 64 && b.live.size() == 64);
        return b;
    }
};

inline std::string selection_digest(const ApplicationPrincipalBinding &principal,
                                    std::span<const std::string> intents) {
    Encode out(envelope_limit);
    principal_fields(out, principal);
    out.u32(intents.size());
    for (const auto &intent : intents)
        out.b(intent);
    return hash(out.data);
}

struct Authority {
    ContributionReviewFacts &facts;
    const ApplicationPrincipalBinding &principal;
    std::string_view proof;
    std::string_view actor;
    const ApplicationOperationKey *key;
    std::span<const ReviewSelector> selection;

    AccessDecision check(const SqliteWorkSession &session, ReviewAuthorityUse use,
                         const ReviewEntryIntent *entry = nullptr,
                         const ReviewOutcomeFacts *committed = nullptr,
                         const std::vector<ApplicationEffect> *effects = nullptr,
                         detail::ReviewReadCost *cost = nullptr) const {
        const auto before = cost ? detail::ContributionReviewWriterAccess::remaining(session)
                                 : detail::ReviewReadCost{};
        auto decision = detail::ContributionReviewAccess::call(session, [&](const auto &view) {
            return facts.check(view,
                               {use, principal, proof, actor, key, entry, committed, selection,
                                effects ? std::optional{std::span(*effects)} : std::nullopt});
        });
        if (cost)
            *cost += before - detail::ContributionReviewWriterAccess::remaining(session);
        return decision;
    }

    std::optional<ReviewRefusal> scope(const SqliteWorkSession &session,
                                       detail::ReviewReadCost *cost = nullptr) const {
        for (auto use : {ReviewAuthorityUse::session, ReviewAuthorityUse::batch_key}) {
            const auto answer = check(session, use, nullptr, nullptr, nullptr, cost);
            if (std::holds_alternative<ApplicationAccessDenied>(answer))
                return ReviewRefusal::denied;
            if (!std::holds_alternative<ApplicationAccessAllowed>(answer))
                return ReviewRefusal::unavailable;
        }
        return {};
    }

    std::optional<ReviewPolicyFacts> policy(const SqliteWorkSession &session,
                                            const ReviewSelector &selector) const {
        return detail::ContributionReviewAccess::call(
            session, [&](const auto &view) { return facts.policy(view, selector); });
    }
};

inline std::vector<ReviewSelector> selectors(const auto &entries, auto select) {
    std::vector<ReviewSelector> out;
    out.reserve(entries.size());
    for (const auto &entry : entries)
        out.push_back(select(entry));
    return out;
}

inline bool allowed(const AccessDecision &a) {
    const auto *grant = std::get_if<ApplicationAccessAllowed>(&a);
    return grant && !grant->permission_revision_ref.empty();
}

inline std::string text_cell(const QVariant &value, qsizetype limit = 4096) {
    require(value.metaType().id() == QMetaType::QByteArray && !value.isNull());
    const auto raw = value.toByteArray();
    require(raw.size() > 0 && raw.size() <= limit);
    auto text = raw.toStdString();
    require(utf8(text));
    return text;
}

inline qint64 integer(const QVariant &value) {
    require(value.metaType().id() == QMetaType::Int ||
            value.metaType().id() == QMetaType::LongLong);
    return value.toLongLong();
}

struct ConflictingAnchor {};

inline bool anchor(const SqliteWorkSession &s, const ReviewSelector &sel) {
    const auto rows = s.execute(
        {"SELECT c.contribution_id,c.original_receipt_id,o.contribution_id,o.kind "
         "FROM main.subathon_contributions c LEFT JOIN main.subathon_contribution_observations o "
         "ON o.receipt_id=c.original_receipt_id "
         "WHERE c.contribution_id=? OR c.original_receipt_id=? LIMIT 3",
         {id(sel.cid.value), id(sel.receipt.value)}});
    if (rows.empty())
        return false;
    if (rows.size() != 1 || rows[0][0] != id(sel.cid.value) || rows[0][1] != id(sel.receipt.value))
        throw ConflictingAnchor{};
    require(rows[0][2] == id(sel.cid.value) && integer(rows[0][3]) == 0);
    return true;
}

inline std::optional<ApplicationPlanKey> plan_key(const SqliteWorkSession &s,
                                                  const ReviewSelector &sel) {
    auto rows = s.execute(
        {"SELECT CASE WHEN length(plan_ref)<=4096 THEN plan_ref END, head_revision, "
         "typeof(plan_ref)='blob' AND length(plan_ref)>4096 "
         "FROM main.subathon_plan_headers WHERE contribution_id=? AND receipt_id=? LIMIT 2",
         {id(sel.cid.value), id(sel.receipt.value)}});
    require(rows.size() <= 1);
    if (rows.empty())
        return {};
    if (integer(rows[0][2]))
        throw Oversize{};
    return ApplicationPlanKey{text_cell(rows[0][0]),
                              decode_plan_revision_uint64(rows[0][1].toByteArray())};
}

inline bool claimed(const SqliteWorkSession &s, const ReviewSelector &sel) {
    auto rows = s.execute({"SELECT EXISTS(SELECT 1 FROM main.subathon_app_claims "
                           "WHERE subject_kind=0 AND subject_id=?), "
                           "EXISTS(SELECT 1 FROM main.subathon_app_operations WHERE "
                           "operation_kind=0 AND receipt_id=?)",
                           {id(sel.cid.value), id(sel.receipt.value)}});
    require(rows.size() == 1);
    return integer(rows[0][0]) || integer(rows[0][1]);
}

struct Decision {
    std::string ref;
    PausedAcceptanceMode mode;
};

struct Row {
    std::optional<ApplicationBundleInputs> inputs;
    std::optional<ReviewPolicyFacts> policy;
    std::optional<Decision> pending;
    bool rejected{};
    bool credited{};
    bool can_decide{};
    bool can_apply{};
    bool bad_choices{};
    bool disclose{};
    bool authorized{};
    std::string permission_revision;
    std::vector<core::ContributionBlocker> blockers;
    std::vector<ApplicationHoldView> ordinary;
    ActiveHoldReadCost hold_read;
    detail::ReviewReadCost authority_read;
    std::vector<ApplicationHoldVersionLocator> mixed;
    std::vector<ApplicationHoldResolutionEntry> closures;
    std::vector<ApplicationEffect> effects;
    std::vector<MilestoneOccurrenceKey> occurrences;
    std::vector<CapturedMilestoneGroup> groups;
    std::vector<ApplicationJobIntent> jobs;
    std::vector<core::TimerTransition> transitions;
    std::optional<ReviewTimerSelector> deficit;
    std::optional<ApplicationHoldTimerEvidence> detected;
    std::optional<ApplicationHoldFailureEvidence> overflow;
    QByteArray partition;
    ApplicationStateSnapshot before;
    ApplicationStateSnapshot after;
};

inline void bounded_shape(const SqliteWorkSession &s, const ReviewEntryIntent &i,
                          const std::optional<ReviewPolicyFacts> &policy) {
    const auto cid = id(i.selector.cid.value);
    const auto receipt = id(i.selector.receipt.value);
    const auto rows =
        s.execute({"SELECT "
                   "(SELECT count(*) FROM (SELECT 1 FROM main.subathon_plan_requests r "
                   " JOIN main.subathon_plan_headers h ON r.plan_ref=h.plan_ref AND "
                   "r.revision=h.head_revision"
                   " WHERE h.contribution_id=? AND h.receipt_id=? LIMIT 33)),"
                   "(SELECT count(*) FROM (SELECT 1 FROM main.subathon_app_balances b "
                   " JOIN main.subathon_app_head h USING(token) LIMIT 33)),"
                   "(SELECT count(*) FROM (SELECT 1 FROM main.subathon_hold_causes "
                   " WHERE subject_kind=0 AND subject_id=? LIMIT 9)),"
                   "(SELECT count(*) FROM (SELECT 1 FROM main.subathon_financial_refunds f "
                   " WHERE f.subject=? AND f.hold_ref IS NOT NULL AND NOT EXISTS "
                   " (SELECT 1 FROM main.subathon_financial_resolutions r WHERE "
                   "r.hold_ref=f.hold_ref) LIMIT 9))",
                   {cid, receipt, cid, cid}});
    require(rows.size() == 1 && rows.front().size() == 4);
    const auto request_count =
        i.revision ? i.revision->captured.requested_effects.size() : size_t(integer(rows[0][0]));
    const auto balance_count = size_t(integer(rows[0][1]));
    const auto ordinary = size_t(integer(rows[0][2]));
    const auto financial = size_t(integer(rows[0][3]));
    if (request_count > 32 || balance_count > 32 || ordinary > 8 || financial > 8 ||
        i.ordinary_closures.size() > 8)
        throw Oversize{};
    if (!policy)
        return;
    const auto &p = *policy;
    const bool normal = i.mode == PausedAcceptanceMode::normal;
    const auto alerts = normal ? p.ordinary_alerts.size() : 0;
    if (p.internal_jobs.size() > 32 || alerts > 32 || p.milestones.size() > 32 ||
        p.signed_original.size() > 32)
        throw Oversize{};
    // The bundle store charges all physical rows again during guarded readback.
    // Reserve both traversals as well as the worst branch for the admitted shape.
    const auto fanout = 160 + 6 * request_count + 4 * balance_count + 12 * p.milestones.size() +
                        6 * (alerts + p.internal_jobs.size()) + 48 * ordinary + 64 * financial +
                        32 * i.ordinary_closures.size();
    if (fanout > 512)
        throw Oversize{};
    Encode job_bytes;
    for (const auto &job : p.internal_jobs)
        application_result_wire::put_job(job_bytes, job);
    if (normal)
        for (const auto &job : p.ordinary_alerts)
            application_result_wire::put_job(job_bytes, job);
}

inline std::int64_t checked_add(std::int64_t prior, std::int64_t delta) {
    if ((delta > 0 && prior > std::numeric_limits<std::int64_t>::max() - delta) ||
        (delta < 0 && prior < std::numeric_limits<std::int64_t>::min() - delta))
        throw std::overflow_error("review balance");
    return prior + delta;
}

inline std::optional<Deficit> signed_policy(const ReviewPolicyFacts &p, std::int64_t ordinal) {
    const auto it = std::ranges::find(p.signed_original, ordinal,
                                      &ApplicationSignedCorrectionPolicy::request_ordinal);
    if (it == p.signed_original.end())
        return {};
    require(std::ranges::count(p.signed_original, ordinal,
                               &ApplicationSignedCorrectionPolicy::request_ordinal) == 1);
    return it->deficit_policy;
}

inline bool calculate(Row &out, const ReviewEntryIntent &entry,
                      const core::CapturedPlanRevision &plan) {
    if (!out.policy || out.before.timer.lifecycle == core::TimerLifecycle::Ready)
        return false;
    const auto &p = *out.policy;
    const auto clock = core::advance_timer(out.before.timer, p.timer, p.clock);
    out.before.timer = clock.state;
    out.after = out.before;
    out.transitions = clock.transitions;
    const bool finished = clock.state.lifecycle == core::TimerLifecycle::Finished;
    if (finished && !p.finished_siblings)
        return false;
    std::size_t seen_deficits{};
    const auto &requests = plan.captured.requested_effects;
    for (size_t i = 0; i < requests.size(); ++i) {
        const auto &r = requests[i];
        if (finished &&
            (r.unit == Unit::milliseconds ||
             std::ranges::find(*p.finished_siblings, r.unit) == p.finished_siblings->end())) {
            out.partition.append(r.unit == Unit::milliseconds ? '\1' : '\2');
            continue;
        }
        out.partition.append('\0');
        ApplicationEffect effect;
        effect.request_ordinal = std::int64_t(i + 1);
        effect.unit = r.unit;
        effect.exact_requested_units = r.exact_requested_units;
        effect.requested_units = r.requested_units;
        effect.rounding_residual = r.exact_requested_units - core::Exact(r.requested_units);
        effect.application_explanation = r.calculation_explanation;
        auto &timer = out.after.timer;
        auto &debt = out.after.time_debt_ms;
        auto fail_numeric = [&](core::Exact attempted) {
            out.overflow = ApplicationHoldFailureEvidence{
                ApplicationHoldFailureStage::aggregate_after_finalization,
                effect.request_ordinal,
                r.target_ref,
                std::move(attempted),
                {"subathon.internal.contribution-review.v1", "whole-input-overflow"}};
            return false;
        };
        if (r.unit != Unit::milliseconds) {
            const bool score = r.unit == Unit::micro_points;
            const auto kind =
                score ? ApplicationBalanceKind::score : ApplicationBalanceKind::whole_count;
            auto target = std::ranges::find_if(out.after.balances, [&](const auto &balance) {
                return balance.kind == kind && balance.target_ref == r.target_ref;
            });
            if (target == out.after.balances.end())
                return false;
            try {
                target->units = checked_add(target->units, r.requested_units);
            } catch (const std::overflow_error &) {
                return fail_numeric(core::Exact(target->units) + r.requested_units);
            }
            effect.components = {
                {score ? Kind::score : Kind::whole_count, r.requested_units, r.target_ref}};
        } else if (r.requested_units >= 0) {
            core::TimeAdditionResult credit;
            try {
                credit = core::plan_time_addition({timer.main_remaining_ms, debt},
                                                  {"review", r.requested_units}, p.cap);
            } catch (const std::overflow_error &) {
                return fail_numeric(
                    core::Exact(timer.main_remaining_ms) +
                    std::max(core::Exact(0), core::Exact(r.requested_units) - debt));
            }
            timer = core::apply_applicable_time_credit(timer, credit.main_added_ms);
            debt = credit.balance_after.time_debt_ms;
            effect.arithmetic_role = ApplicationArithmeticRole::time_addition;
            effect.maximum_main_ms = p.cap;
            effect.components = {{Kind::main_delta, credit.main_added_ms, {}},
                                 {Kind::debt_delta, -credit.debt_repaid_ms, {}},
                                 {Kind::cap_limited_time, credit.cap_limited_ms, {}}};
        } else {
            auto policy = signed_policy(p, effect.request_ordinal);
            if (!policy)
                return false;
            const bool continuation = entry.verb == V::continue_timer;
            const bool shortfall = -core::Exact(r.requested_units) > timer.main_remaining_ms;
            if (continuation && shortfall) {
                const ReviewDeficitChoice *choice = nullptr;
                if (entry.deficits && seen_deficits < entry.deficits->size())
                    choice = &entry.deficits->at(seen_deficits);
                if (!choice || choice->ordinal != effect.request_ordinal ||
                    choice->signed_request != r.requested_units) {
                    out.deficit = ReviewTimerSelector{entry.timer->locator, effect.request_ordinal,
                                                      r.requested_units};
                    out.bad_choices = true;
                    return false;
                }
                policy = choice->policy;
                ++seen_deficits;
            }
            core::TimeReversalResult debit;
            try {
                debit =
                    core::plan_time_reversal({timer.main_remaining_ms, debt},
                                             {"review", "captured", r.requested_units}, *policy);
            } catch (const std::overflow_error &) {
                return fail_numeric(core::Exact(debt) +
                                    std::max(core::Exact(0), -core::Exact(r.requested_units) -
                                                                 timer.main_remaining_ms));
            }
            if (debit.disposition == core::TimeReversalDisposition::RequiresReview) {
                out.detected = ApplicationHoldTimerEvidence{
                    effect.request_ordinal,
                    Kind::unapplied_time,
                    r.requested_units,
                    *policy,
                    debit.disposition,
                    r.requested_units,
                    ApplicationHoldComposition::defer_whole_input,
                    {"subathon.internal.contribution-review.v1", "whole-input"},
                    {}};
                return false;
            }
            timer.main_remaining_ms = debit.balance_after.main_remaining_ms;
            // A negative request settles against Main in sequence. Observe the
            // configured post-effect transition when Main reaches zero.
            const auto advance =
                core::advance_timer(timer, p.timer, {0, p.clock.utc_now_ms, p.clock.paused});
            timer = advance.state;
            out.transitions.insert(out.transitions.end(), advance.transitions.begin(),
                                   advance.transitions.end());
            debt = debit.balance_after.time_debt_ms;
            effect.arithmetic_role = ApplicationArithmeticRole::time_reversal;
            effect.deficit_policy = *policy;
            effect.deficit_disposition = debit.disposition;
            effect.components = {{Kind::main_delta, debit.main_adjustment_ms, {}},
                                 {Kind::debt_delta, debit.debt_added_ms, {}},
                                 {Kind::unapplied_time, debit.unapplied_adjustment_ms, {}}};
        }
        out.effects.push_back(std::move(effect));
    }
    if (entry.verb == V::continue_timer &&
        (!entry.deficits || seen_deficits != entry.deficits->size())) {
        out.bad_choices = true;
        return false;
    }
    return true;
}

inline std::optional<std::int64_t> score(const ApplicationStateSnapshot &state) {
    const auto it = std::ranges::find_if(state.balances, [](const auto &b) {
        return b.kind == ApplicationBalanceKind::score && b.target_ref == "score";
    });
    if (it == state.balances.end())
        return {};
    return it->units;
}

inline void milestones(const SqliteWorkSession &s, Row &row, const ReviewEntryIntent &i,
                       std::string_view installation, const std::set<std::string> &projected) {
    if (row.policy->milestones.empty())
        return;
    const auto before = score(row.before);
    const auto after = score(row.after);
    if (!before || !after) {
        row.can_apply = false;
        return;
    }
    std::set<std::string> seen;
    std::optional<std::int64_t> previous;
    for (const auto &target : row.policy->milestones) {
        require(!target.milestone_ref.empty() &&
                (!previous || *previous < target.target_micro_points) &&
                seen.insert(target.milestone_ref).second);
        previous = target.target_micro_points;
        if (*before >= target.target_micro_points || *after < target.target_micro_points ||
            projected.contains(target.milestone_ref))
            continue;
        const auto existing =
            s.execute({"SELECT 1 FROM main.subathon_milestone_groups WHERE milestone_ref=? LIMIT 1",
                       {bytes(target.milestone_ref)}});
        if (!existing.empty())
            continue;
        Encode occurrence;
        occurrence.b(installation);
        selector_fields(occurrence, i.selector);
        occurrence.b(row.policy->revision);
        occurrence.b(target.milestone_ref);
        occurrence.i64(target.target_micro_points);
        occurrence.u32(row.occurrences.size());
        auto &key = row.occurrences.emplace_back("review-occurrence-" + hash(occurrence.data));
        row.groups.push_back({{{target.milestone_ref, row.policy->revision,
                                target.target_micro_points, *after, key.ref},
                               {},
                               {},
                               key},
                              {}});
    }
}

inline bool exact_choices(const Row &row, const ReviewEntryIntent &i) {
    return i.milestones && i.milestones->size() == row.occurrences.size() &&
           std::ranges::equal(*i.milestones, row.occurrences, {},
                              &MilestoneAcceptanceChoice::occurrence);
}

inline void collect_holds(const SqliteWorkSession &s, Row &row, const ReviewEntryIntent &i,
                          bool meter) {
    auto ordinary = meter ? read_active_holds(s, {i.selector.cid}, row.hold_read)
                          : SqliteApplicationHoldStore(s).active({i.selector.cid});
    if (ordinary.holds.size() > 8)
        throw Oversize{};
    row.ordinary = std::move(ordinary.holds);
    const auto financial = SqliteFinancialRefundStore(s).active({i.selector.cid});
    if (financial.size() > 8)
        throw Oversize{};
    for (const auto &hold : row.ordinary)
        row.mixed.push_back(hold.current_version.locator);
    for (const auto &hold : financial)
        row.mixed.push_back(hold.locator);
    std::ranges::sort(row.mixed, {}, &ApplicationHoldVersionLocator::hold_ref);
    require(std::ranges::adjacent_find(row.mixed, {}, &ApplicationHoldVersionLocator::hold_ref) ==
            row.mixed.end());
}

inline const ApplicationHoldView *timer_hold(const Row &row, const ReviewEntryIntent &i) {
    if (!i.timer)
        return nullptr;
    const auto it = std::ranges::find_if(row.ordinary, [&](const auto &hold) {
        const auto &timer = hold.cause.timer;
        return timer && same(hold.current_version.locator, i.timer->locator) &&
               timer->request_ordinal == i.timer->ordinal &&
               timer->unapplied_units == i.timer->signed_request;
    });
    return it == row.ordinary.end() ? nullptr : &*it;
}

inline bool usable_revision(const Row &row, const ReviewEntryIntent &i) {
    if (!i.revision)
        return true;
    const auto &revision = *i.revision;
    const auto &head = row.inputs->selected_plan;
    const auto &v = row.inputs->current_contribution->contribution.state.conditions.valuation;
    return revision.parent_revision == head.revision && revision.revision > head.revision &&
           revision.revision - head.revision == 1 &&
           revision.captured.event == head.captured.event &&
           revision.captured.contribution_state.conditions.valuation == v &&
           revision.override_decision &&
           revision.override_decision->authority == core::OverrideAuthority::authorized;
}

inline bool complete_closures(Row &row, const ReviewEntryIntent &i) {
    row.closures = i.ordinary_closures;
    if (i.timer && std::ranges::none_of(row.closures, [&](const auto &closure) {
            return closure.hold_ref == i.timer->locator.hold_ref;
        })) {
        row.closures.push_back({i.timer->locator.hold_ref,
                                i.timer->locator.version_ref,
                                {},
                                {"subathon.internal.contribution-review.v1", "close"},
                                {}});
        std::ranges::sort(row.closures, {}, &ApplicationHoldResolutionEntry::hold_ref);
    }
    return row.closures.size() == row.ordinary.size() &&
           std::ranges::equal(row.closures, row.ordinary, [](const auto &entry, const auto &hold) {
               return entry.hold_ref == hold.current_version.locator.hold_ref &&
                      entry.expected_version_ref == hold.current_version.locator.version_ref;
           });
}

inline void plan_row(Row &row, const SqliteWorkSession &s, const ReviewEntryIntent &i,
                     std::string_view installation, const std::set<std::string> &projected) {
    using R = core::ContributionReview;
    auto state = row.inputs->current_contribution->contribution.state;
    const auto review = state.conditions.review;
    const auto *hold = timer_hold(row, i);
    const bool whole =
        hold && hold->cause.timer->composition == ApplicationHoldComposition::defer_whole_input;
    const bool financial = row.mixed.size() != row.ordinary.size();
    if (financial && !row.credited)
        return;
    switch (i.verb) {
    case V::accept:
    case V::reject:
        row.can_decide = !row.credited && !row.rejected && review == R::pending;
        break;
    case V::apply_accepted:
        row.can_decide = !row.credited && !row.rejected && review == R::accepted && row.pending &&
                         i.pending_decision_ref == row.pending->ref;
        break;
    case V::continue_timer:
        row.can_decide =
            !row.credited && !row.rejected && whole &&
            (review == R::not_required || (review == R::accepted && row.pending &&
                                           i.pending_decision_ref == row.pending->ref)) &&
            hold->cause.plan_evidence.plan &&
            hold->cause.plan_evidence.plan->plan_ref == row.inputs->plan.plan_ref &&
            hold->cause.plan_evidence.plan->revision == row.inputs->plan.revision &&
            complete_closures(row, i);
        break;
    case V::reject_held:
        row.can_decide =
            hold &&
            (whole ? !row.credited && !row.rejected && complete_closures(row, i) : row.credited);
        if (row.can_decide && !whole) {
            row.closures = i.ordinary_closures;
            if (row.closures.empty())
                row.closures.push_back({i.timer->locator.hold_ref,
                                        i.timer->locator.version_ref,
                                        {},
                                        {"subathon.internal.contribution-review.v1", "reject"},
                                        {}});
            row.can_decide =
                row.closures.size() == 1 &&
                row.closures.front().hold_ref == i.timer->locator.hold_ref &&
                row.closures.front().expected_version_ref == i.timer->locator.version_ref;
        }
        return;
    case V::retain_held:
        row.can_decide = hold && (whole ? !row.credited : row.credited);
        return;
    }
    if (!row.can_decide)
        return;
    if (i.verb == V::accept || i.verb == V::reject)
        state.conditions.review = R::accepted;
    const auto eligible = assess_contribution_eligibility(state);
    row.blockers.assign(eligible.blockers.begin(), eligible.blockers.end());
    if (!row.blockers.empty() || !usable_revision(row, i))
        return;
    if ((!row.ordinary.empty() || !i.ordinary_closures.empty()) && !complete_closures(row, i))
        return;
    const auto &plan = i.revision ? *i.revision : row.inputs->selected_plan;
    if (plan.captured.valuation.state == core::PlanValuationState::missing_historical)
        return;
    row.can_apply = calculate(row, i, plan);
    if (!row.can_apply) {
        row.effects.clear();
        return;
    }
    milestones(s, row, i, installation, projected);
    if (row.can_apply && i.verb != V::reject) {
        row.jobs = row.policy->internal_jobs;
        if (i.mode == PausedAcceptanceMode::normal)
            row.jobs.insert(row.jobs.end(), row.policy->ordinary_alerts.begin(),
                            row.policy->ordinary_alerts.end());
        const bool terminal = std::ranges::any_of(row.partition, [](char tag) { return tag != 0; });
        // SAR2 links require actual physical source ordinals. Policy may supply
        // independent source-less jobs only when that provenance can be staged.
        for (const auto &job : row.jobs) {
            if (job.group_ref || job.group_ordinal || (terminal && !job.source_request_ordinal) ||
                (job.source_request_ordinal &&
                 (*job.source_request_ordinal < 1 ||
                  *job.source_request_ordinal > row.partition.size() ||
                  row.partition.at(qsizetype(*job.source_request_ordinal - 1)) != 0))) {
                row.can_apply = false;
                row.effects.clear();
                return;
            }
        }
    }
}

} // namespace subathon::server::review_detail
