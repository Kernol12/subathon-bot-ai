// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/reference_intake_fixture.hpp"

#include "fixture.hpp"
#include <subathon/core/rate_snapshot.hpp>
#include <subathon/core/refund_entitlement.hpp>

namespace {
using namespace modifier_005_fixture;

template <class Test> void scenario(Test test) {
    try {
        test();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

void refusal(const ModifierCaptureSelection &result, Failure expected) {
    require(std::get_if<Failure>(&result) && std::get<Failure>(result) == expected,
            "Exact typed selection refusal carries no selected value");
}

struct ReviewFacts final : ContributionReviewFacts {
    AccessDecision check(const ApplicationReferenceReadView &view,
                         const ReviewAuthorityQuery &query) override {
        if (query.principal.kind != ApplicationPrincipalKind::account ||
            query.principal.installation_ref != "install" ||
            query.principal.principal_ref != "operator" ||
            query.session_proof != "fixture-review-A" ||
            (!query.actor.empty() && query.actor != "operator") ||
            (query.entry && query.entry->selector != ReviewSelector{{0}, {1}}) ||
            (query.committed && query.committed->verified != ReviewSelector{{0}, {1}}))
            return ApplicationAccessDenied{};
        const auto config = view.find_retained({RetainedKind::configuration, "cfg-apply-A"});
        require(config && config->document == "cfg-apply-A-data",
                "Scoped Review authority inspects actual application configuration");
        return ApplicationAccessAllowed{"fixture-review-A"};
    }

    std::optional<ReviewPolicyFacts> policy(const ApplicationReferenceReadView &,
                                            const ReviewSelector &selector) override {
        require(selector == ReviewSelector{{0}, {1}}, "Current policy for selected original");
        return ReviewPolicyFacts{"apply-policy-A",
                                 3600000,
                                 {core::TimerEndMode::Grace, 120000, core::GraceReuse::Fresh, {}},
                                 {0, 700000, false},
                                 {},
                                 {},
                                 {},
                                 {},
                                 {},
                                 false};
    }
};

void actual(const ApplicationEffect &e) {
    using Component = ApplicationComponentKind;
    require(
        e.request_ordinal == 1 && e.arithmetic_role == ApplicationArithmeticRole::time_addition &&
            e.unit == core::RequestedUnit::milliseconds && e.exact_requested_units == 120000 &&
            e.requested_units == 120000 && e.rounding_residual == 0 &&
            e.maximum_main_ms == 3600000 && e.components.size() == 3 &&
            e.components[0].kind == Component::main_delta && e.components[0].units == 60000 &&
            e.components[1].kind == Component::debt_delta && e.components[1].units == 0 &&
            e.components[2].kind == Component::cap_limited_time && e.components[2].units == 60000 &&
            std::ranges::all_of(e.components, [](const auto &p) { return !p.target_ref; }),
        "120000 exact and rounded; 60000 Main, zero debt, 60000 cap tail");
}

bool main_state(const core::TimerState &state, std::int64_t expected) {
    return state.lifecycle == core::TimerLifecycle::Active &&
           state.phase == core::CountdownPhase::Main && state.main_remaining_ms == expected &&
           state.grace_remaining_ms == 120000 && !state.awaiting_confirmation;
}

core::CapturedPlanRevision delayed_review(Case &c) {
    c.config("cfg-A");
    c.initial_main();
    c.live_original();
    const auto input = arrival_a();
    const auto r = c.original(input, award());
    const auto [cid, state] = c.support(r);
    const auto plan =
        initial_plan(c, selected(c.select(r, input.event, cid), plan_event(input, r, cid)), state);
    c.replace();
    c.expire();
    c.read([](const auto &s) {
        SqliteModifierActivationStore store(s);
        const auto old = store.instance_revision("j1");
        const auto current = store.instance_revision("k1");
        require(old && old->lifetime == ModifierInstanceLifetime::replaced &&
                    old->successor_instance_ref == "J" &&
                    old->instance == activation::checkpointed_instance && current &&
                    current->lifetime == ModifierInstanceLifetime::expired &&
                    current->instance == activation::k1 &&
                    store.current_definition("D")->definition == activation::new_definition &&
                    !store.live_instance("D"),
                "Fresh owner sees replacement and expiry, but exact old revision survives");
        return true;
    });
    const auto &recorded_rows = plan.revisions[0].modifiers;
    const auto value = selected(c.select(r, input.event, cid, {}, {}, std::span(recorded_rows)),
                                plan_event(input, r, cid));
    expect_original(value);
    require(value.recorded_selection == recorded_rows &&
                value.recorded_links ==
                    std::vector<ModifierTraceLink>{{1, original_main}, {2, original_counter}} &&
                original_proposal(value).selection == recorded_rows,
            "Delayed selection independently derives links for the stored original rows");
    const auto original_clock = core::ModifierClockValue{activation::checkpointed_instance, 180000};
    const auto frozen =
        project_modifier_binding("cfg-A", activation::old_definition, 1, original_clock, {});
    require(frozen.active == true && frozen.remaining_ms == 180000 && frozen.factor == 2 &&
                frozen.factor_decimal == "2.00",
            "Historical projection requires the qualified arrival view");
    const auto terminal = project_modifier_binding("cfg-apply-A", activation::new_definition, 1,
                                                   {activation::k1, 0}, {});
    require(terminal.active == false && terminal.remaining_ms == 0 &&
                terminal.factor_decimal == "0.50",
            "Separate qualified terminal view supplies zero remaining and inactive");

    ReviewFacts facts;
    ContributionReview service(c.db.writer(), facts);
    ReviewEntryIntent intent;
    intent.selector = {cid, r.id};
    intent.milestones = std::vector<MilestoneAcceptanceChoice>{};
    const auto preview = review::await<ContributionReview::PreviewResult>([&](auto done) {
        service.preview({review::principal, {intent}}, "fixture-review-A", done);
    });
    const auto &rows = std::get<std::vector<ReviewPreviewRow>>(preview);
    require(rows.size() == 1 && rows[0].ordinal == 1 && rows[0].facts &&
                rows[0].facts->selector == intent.selector && rows[0].facts->may_decide &&
                rows[0].facts->may_apply && !rows[0].facts->basis.isEmpty() &&
                rows[0].facts->prospective_effects.size() == 1,
            "Shipped Review previews exact selected original and current capped application");
    actual(rows[0].facts->prospective_effects[0]);
    auto result = review::Case::execute(
        service,
        {{"review", "modifier-arrival-A"},
         review::principal,
         "operator",
         {{intent, rows[0].facts->basis, "decision-A", "Accept deferred support"}}},
        "fixture-review-A");
    const auto &reply = review::Case::reply(result);
    review::expected_row(reply, 1, ReviewDisposition::committed, ReviewStatus::applied);
    require(!reply.stop && reply.rows.size() == 1 && reply.rows[0].outcome->facts &&
                reply.rows[0].outcome->facts->verified == intent.selector &&
                reply.rows[0].outcome->facts->bundle &&
                finance::same(*reply.rows[0].outcome->facts->bundle, bundle_key),
            "Acknowledged Review returns verified receipt 1/CID 0 and independent bundle key");
    c.db.reopen();
    c.read([&](const auto &s) {
        SqliteApplicationBundleStore store(s);
        const auto readback = store.lookup(bundle_key);
        const auto current = store.read_state();
        require(
            readback && current && readback->intent.receipt_id == r.id &&
                readback->intent.contribution_id == cid &&
                readback->intent.plan.plan_ref == "plan-A" && readback->intent.plan.revision == 1 &&
                readback->audit.actor_ref == "operator" &&
                readback->audit.decision_ref == "decision-A" &&
                readback->audit.reason == "Accept deferred support" &&
                readback->persisted_head.token == "main-before-A" &&
                main_state(readback->advanced_before.timer, 3540000) &&
                main_state(readback->after.timer, 3600000) &&
                readback->after.configuration_ref == "cfg-apply-A" &&
                readback->after.time_debt_ms == 0 && readback->after.balances.empty() &&
                current->token == readback->after.token && main_state(current->timer, 3600000) &&
                readback->jobs.empty() && readback->effects.size() == 1,
            "Committed operation binds original plan, accepted decision and exact state");
        actual(readback->effects[0]);
        const auto archived = SqliteApplicationPlanStore(s).find_revision("plan-A", 1);
        require(archived && archived->revision == plan.revisions[0] &&
                    SqliteRetainedContextStore(s).find(context_key("ctx-A"))->configuration ==
                        ConfigurationReference{"cfg-A"},
                "Committed Review preserves the full request, original receipt and linked context");
        return true;
    });
    return plan.revisions[0];
}

struct OverrideCase {
    Case c;
    ModifierArrivalInput input = arrival_a();
    ContributionReceipt receipt;
    core::ContributionId cid{0};
    core::ApplicationPlan plan;

    OverrideCase() {
        c.config("cfg-A");
        c.initial_main();
        c.live_original();
        receipt = c.original(input, award());
        auto [id, state] = c.support(receipt);
        cid = id;
        plan = initial_plan(
            c, selected(c.select(receipt, input.event, cid), plan_event(input, receipt, cid)),
            state);
        c.replace();
        c.config("cfg-override-A");
    }

    core::CapturedPlanRevision candidate(const core::PlanOverrideDecision &decision) const {
        // An independently calculated candidate submitted to the seam. The
        // readback must reselect its own retained document and run #219 again.
        auto capture = plan.revisions[0].captured;
        capture.requested_effects[0].exact_requested_units = 30000;
        capture.requested_effects[0].requested_units = 30000;
        const std::vector<core::ModifierSelection> rows{
            {"a-new", "k0", "timer.main", "g-override", "multiply", "multiply", "0.50"}};
        RuleSource rules;
        ModifierSource mods(capture.event, rows);
        return core::revise_application_plan(plan, 1, capture, decision, rules, mods).revisions[1];
    }

    void stage(const core::PlanOverrideDecision &authority,
               const std::optional<QualifiedModifierCut> &cut,
               const core::CapturedPlanRevision &next, std::optional<Failure> expected,
               bool cancel = false) {
        auto out = run(c.db, [&](auto ctx) {
            auto s = ctx.begin();
            require(retain_modifier_recalculation(s, {"plan-A", 1}, authority, cut, bases, next,
                                                  limits) == expected,
                    "Exact override append or typed refusal");
            if (expected && *expected != Failure::conflict) {
                require(!SqliteRetainedContextStore(s).find(context_key(authority.decision_ref)),
                        "Refused override stages no context before the freshness/authority fence");
            }
            if (expected || cancel) {
                if (cancel) {
                    require(
                        SqliteRetainedContextStore(s).find(context_key(authority.decision_ref)) &&
                            SqliteApplicationPlanStore(s).find_revision("plan-A", 2),
                        "Both parts are provisional before caller rolls back");
                }
                s.rollback();
                return;
            }
            s.commit();
        });
        if (out.second)
            std::rethrow_exception(out.second);
        if (expected || cancel)
            require(out.first.transaction == TransactionOutcome::rolled_back &&
                        out.first.error == WorkError::not_committed && !out.first.value,
                    "Caller rollback publishes neither tentative value");
        else
            finance::normal(out.first);
    }

    void verify_original() {
        const auto event = plan.revisions[0].captured.event;
        auto value = selected(c.select(receipt, input.event, cid), event);
        expect_original(value);
        original_proposal(value);
        const auto persisted = c.read(
            [](const auto &s) { return SqliteApplicationPlanStore(s).find_revision("plan-A", 1); });
        require(persisted && persisted->revision == plan.revisions[0],
                "Immutable revision one retains the 120000 request and original observation");
        c.no_application();
    }
};

core::ModifierClockValue recovery_unknown() {
    const core::DowntimeRecoveryInput input{
        .basis = {"lineage-A", "checkpoint-A", "configuration-A", "facts-A"},
        .policy = core::DowntimePolicy::CountElapsed,
        .timer = finance::timer(finance::Life::Active, 30),
        .config = {core::TimerEndMode::Grace, 120000, core::GraceReuse::Fresh, 1007},
        .pause = {},
        .checkpoint = {"offline-checkpoint", core::RecoveryUtcInstant{1000}},
        .restart = {"restart", core::RecoveryUtcInstant{1008}},
        .gap = core::RecoveryGapUnavailable::Discontinuous,
        .schedule = core::RecoveryScheduleUnavailable::Source};
    const auto preview =
        std::get<core::DowntimeRecoveryPreview>(core::preview_downtime_recovery(input));
    require(preview.review_hold && !preview.count_elapsed, "R7 owner has genuine recovery hold");
    const core::ModifierDefinition definition{
        "definition-real", "def-1", 30, {{Kind::counter, "counter-E", {}, "2"}}};
    const auto activation = core::propose_modifier_activation(
        definition, {},
        {"instance-real", "revision-1", {core::ModifierActivationKind::rule, "rule-seed"}, {}});
    const core::ModifierClockValue checkpoint{activation.revision_to_append.value(), 30,
                                              core::ModifierClockMode::RealElapsed};
    const auto restart = core::project_modifier_restart(input, preview, checkpoint);
    require(!restart.value.remaining_ms && restart.value.revision.remaining_ms == 30,
            "R7 discontinuous restart preserves revision but lacks qualified remaining");
    const core::RecoveryReviewSnapshot pending{
        "episode-A",
        input.basis.timer_lineage,
        input.basis.facts_bundle_ref,
        {"finished-review-manual-revision", {preview.review_hold->timer, 0, {}, input.config}},
        core::set_pause_reason(preview.review_hold->pause, core::PauseReason::Manual, true),
        input.restart};
    const core::RecoveryReviewCommand cmd{"explicit-freeze-op",
                                          pending.episode_ref,
                                          pending.current.revision,
                                          pending.facts_bundle_ref,
                                          core::RecoveryOfflineChoice::Freeze,
                                          {},
                                          "synthetic explicit choice"};
    const core::RecoveryReviewAuthorization access{
        cmd,       pending.current.revision, pending.observed_at.observation_ref,
        "owner-A", "permission-A",           {}};
    const auto resolved = core::resolve_recovery_review(input, pending, cmd, access);
    require(std::holds_alternative<core::RecoveryReviewResolution>(resolved),
            "Qualified Freeze is resolved by the real recovery owner");
    const auto modifier = core::project_modifier_review_resolution(input, pending, resolved,
                                                                   checkpoint, restart.value);
    require(!modifier.value.remaining_ms && modifier.value.revision == checkpoint.revision &&
                modifier.value.mode == checkpoint.mode,
            "Qualified timer Freeze does not supply the real-elapsed gap");
    return modifier.value;
}

const QByteArray canonical =
    QByteArray::fromHex("4d43413100"
                        "0000000143000000014b00000001430000000145000000014500"
                        "00000001500000000153000000014e0000000149"
                        "0000000000000000000000015101");
const ModifierArrivalInput unresolved_c{
    {{"P", "S"}, "N", "I"}, {0, "Q", "K", "C"}, {"E", "E", {}}, {}};

} // namespace

class ModifierCaptureTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { qt_test_support::fail_on_sql_warnings(); }
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void a1_delayedReviewUsesOriginalAndCurrentCap() {
        scenario([] {
            Case c;
            delayed_review(c);
        });
    }

    void a2_reversalBasisUsesActualOriginalMain() {
        scenario([] {
            Case c;
            const auto archived = delayed_review(c);
            c.read([&](const auto &s) {
                const auto result = SqliteApplicationBundleStore(s).read_reversal_basis(
                    {bundle_key, 1, ApplicationComponentKind::main_delta});
                require(result.disposition == SqliteBundleBasisDisposition::ready && result.basis,
                        "Shipped exact Main component selector returns ready basis");
                const auto &b = *result.basis;
                require(finance::same(b.original.operation, bundle_key) &&
                            b.original.request_ordinal == 1 &&
                            b.original.component_kind == ApplicationComponentKind::main_delta &&
                            b.selected_plan == archived &&
                            b.selected_plan.captured.event.receipt_id.value == 1 &&
                            b.selected_plan.captured.event.contribution_id ==
                                core::ContributionId{0} &&
                            b.contribution_original_receipt_id == core::ReceiptId{1} &&
                            b.plan.plan_ref == "plan-A" && b.plan.revision == 1 &&
                            b.application_configuration_ref == "cfg-apply-A" &&
                            b.actual_credit_units == 60000 && b.progress.position == 0 &&
                            b.progress.cumulative == 0 && b.progress.cumulative_fraction == 0 &&
                            !b.progress.full_refund && !b.progress.last && !b.target,
                        "Basis keeps original receipt/CID/plan, actual credit and zero progress");
                actual(b.effect);
                const auto full = core::calculate_refund_entitlement(
                    {core::Exact{1}, core::Exact{0}, core::Exact{1}}, b.effect.unit,
                    b.actual_credit_units, b.progress.cumulative);
                require(full.full_refund && full.cumulative_fraction == 1 &&
                            full.exact_entitlement == 60000 && full.rounding_residual == 0 &&
                            full.requested_increment == -60000,
                        "Full-entitlement arithmetic excludes the 60000 cap-limited tail");
                return true;
            });
            require(c.db.rows("SELECT operation_ref FROM main.subathon_app_operations").size() == 1,
                    "Basis read and entitlement proposal stage no reversal");
        });
    }

    void a3_selectedDecisionAndRealStaleHead() {
        scenario([] {
            OverrideCase x;
            const auto next = x.candidate(authorized);
            x.stage(authorized, override_cut(), next, {});
            x.c.db.reopen();
            const auto role1 = x.c.retained("override-A-2");
            require(role1 && role1->configuration == ConfigurationReference{"cfg-override-A"} &&
                        role1->document.startsWith(QByteArray("MCA1\1", 5)),
                    "Authorized role 1 uses the decision key and its own typed configuration");
            auto stored = x.c.read(
                [](const auto &s) { return SqliteApplicationPlanStore(s).find("plan-A"); });
            require(stored && stored->revisions.size() == 2 && stored->revisions[1] == next &&
                        next.revision == 2 && next.parent_revision == 1 &&
                        next.override_decision == authorized &&
                        next.captured.event == x.plan.revisions[0].captured.event,
                    "Real head 2 binds actor, reason, decision and original immutable event");
            const ModifierDecisionMode mode{"plan-A", 2, authorized};
            const auto value = selected(
                x.c.select(x.receipt, x.input.event, x.cid, mode, {}, std::span(next.modifiers)),
                x.plan.revisions[0].captured.event);
            require(value.context.document_ref == "override-A-2" &&
                        value.context.configuration_ref == "cfg-override-A" &&
                        value.context.policy_ref == "policy-override" &&
                        value.context.profile == profile &&
                        value.context.recalculation ==
                            ModifierRecalculationBinding{"plan-A", 2, "ctx-A", "override-A-2"} &&
                        value.eligible.size() == 1 &&
                        value.eligible[0].observed == override_cut().eligible[0] &&
                        value.eligible[0].exact_instance == activation::k0 &&
                        value.eligible[0].exact_definition == activation::new_definition,
                    "Decision-bound view selects J/k0 at 600000, leaving original arrival intact");
            const std::vector<ModifierTraceLink> links{{1, replacement_main}};
            const auto p = proposal(evaluate(value, bases, {}, {}, mode), 30000);
            require(
                value.recorded_selection == next.modifiers && value.recorded_links == links &&
                    p.selection == next.modifiers && p.links == links &&
                    request(p.effects[0]) == next.captured.requested_effects[0],
                "Exact role-1 calculation independently confirms 30000 and recorded provenance");
            auto altered = mode;
            altered.decision.reason = "Different reason";
            refusal(x.c.select(x.receipt, x.input.event, x.cid, altered), Failure::invalid);
            auto stale = authorized;
            stale.decision_ref = "override-stale";
            x.stage(stale, override_cut(), x.candidate(stale), Failure::stale_head);
            require(!x.c.retained("override-stale"),
                    "Real stale head never retains new decision key");
            require(x.c.read([](const auto &s) {
                return SqliteApplicationPlanStore(s).find("plan-A");
            }) == stored,
                    "Read-only exact retry sees only the two durable revisions");
            x.verify_original();
        });
    }

    void a3_refusalsAndCallerRollback_data() {
        QTest::addColumn<int>("branch");
        QTest::newRow("denied") << 0;
        QTest::newRow("unknown-authority") << 1;
        QTest::newRow("missing-cut") << 2;
        QTest::newRow("conflicting-key") << 3;
        QTest::newRow("caller-rollback") << 4;
        QTest::newRow("unbound-request") << 5;
    }

    void a3_refusalsAndCallerRollback() {
        QFETCH(int, branch);
        scenario([&] {
            OverrideCase x;
            auto cut = std::optional{override_cut()};
            auto authority = authorized;
            auto expected = std::optional<Failure>{};
            std::optional<RetainedDocument> conflicting;
            if (branch < 2) {
                authority.authority = branch == 0 ? core::OverrideAuthority::denied
                                                  : core::OverrideAuthority::unknown;
                expected = Failure::denied;
            } else if (branch == 2) {
                cut.reset();
                expected = Failure::unresolved;
            } else if (branch == 3) {
                conflicting = document("override-A-2", "cfg-A", "OTHER");
                x.c.db.write([&](const auto &s) {
                    require(SqliteRetainedContextStore(s).retain(*conflicting) ==
                                RetainDisposition::inserted,
                            "Unrelated role/producer occupies the global decision key");
                });
                expected = Failure::conflict;
            }
            auto next = x.candidate(authorized);
            if (branch == 5) {
                // Otherwise valid revision shape, but it asserted the original
                // request rather than the amount from the override's admitted base.
                next.captured.requested_effects = x.plan.revisions[0].captured.requested_effects;
                expected = Failure::invalid;
            }
            x.stage(authority, cut, next, expected, branch == 4);
            x.c.db.reopen();
            require(x.c.retained("override-A-2") == conflicting && !x.c.read([](const auto &s) {
                return SqliteApplicationPlanStore(s).find_revision("plan-A", 2);
            }),
                    "No revision or new context survives refusal or caller rollback");
            x.verify_original();
        });
    }

    void a3_lostOverrideAcknowledgementUsesReadback() {
        scenario([] {
            OverrideCase x;
            const auto next = x.candidate(authorized);
            using namespace subathon::server::testing;
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        x.c.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Fault targets the real caller commit after both override writes");
            const auto out = run(x.c.db, [&](auto ctx) {
                auto s = ctx.begin();
                require(!retain_modifier_recalculation(s, {"plan-A", 1}, authorized, override_cut(),
                                                       bases, next, limits),
                        "Role 1 and successor are staged before caller commit");
                s.commit();
            });
            require(out.first.transaction == TransactionOutcome::outcome_unknown &&
                        out.first.error == WorkError::uncertain_outcome && !out.first.value,
                    "Unknown acknowledgement publishes no successful override");
            x.c.db.reopen();
            const auto persistent = x.c.read(
                [](const auto &s) { return SqliteApplicationPlanStore(s).find("plan-A"); });
            require(persistent && persistent->revisions.size() == 2 &&
                        persistent->revisions[1] == next && x.c.retained("override-A-2"),
                    "Fresh authorized readback proves exact decision and only one append");
            const ModifierDecisionMode mode{"plan-A", 2, authorized};
            const auto value =
                selected(x.c.select(x.receipt, x.input.event, x.cid, mode), next.captured.event);
            const auto p = proposal(evaluate(value, bases, {}, {}, mode), 30000);
            require(p.context_ref == "override-A-2" && p.selection == next.modifiers,
                    "Readback reconciles exact selected calculation without resubmitting");
            x.verify_original();
        });
    }

    void a4_distinctRetryAndConflictKeepOriginal() {
        scenario([] {
            Case c;
            c.config("cfg-A");
            c.initial_main();
            c.live_original();
            const auto orig = arrival_a();
            const auto r = c.original(orig, award());
            c.replace();
            c.expire();
            c.config("cfg-attempt-A");
            auto attempt = orig;
            attempt.arrival = {701001, "conn-retry-A", "cfg-attempt-A", "ctx-retry-A"};
            attempt.event = {"event-retry-A", "event-retry-A", {}};
            attempt.cut = QualifiedModifierCut{{"lineage-A", "clock-retry-A", 701001},
                                               0,
                                               profile,
                                               {"policy-attempt-A", "cfg-attempt-A", {}},
                                               {},
                                               {},
                                               {}};
            const auto retry_value = prepare(attempt);
            const auto retry = c.intake(attempt, award(), retry_value);
            require(retry.receipt_id == r.id && retry.disposition == ReceiptDisposition::retry,
                    "Real repeat with new arrival but exact facts selects receipt 1");
            const auto retry_arrival = attempt.arrival;
            attempt.arrival = {701002, "conn-conflict-A", "cfg-attempt-A", "ctx-conflict-A"};
            attempt.event = {"event-conflict-A", "event-conflict-A", {}};
            attempt.cut->observation = {"lineage-A", "clock-conflict-A", 701002};
            const auto conflict_value = prepare(attempt);
            const auto conflict = c.intake(attempt, award(2), conflict_value);
            require(conflict.receipt_id == r.id &&
                        conflict.disposition == ReceiptDisposition::conflict,
                    "Version change classifies conflict at the same durable receipt");
            c.db.reopen();
            const auto stored = c.receipt(orig.identity);
            const std::vector<core::ReceiptAttempt> attempts{
                {orig.arrival, award(), ReceiptDisposition::created},
                {retry_arrival, award(), ReceiptDisposition::retry},
                {attempt.arrival, award(2), ReceiptDisposition::conflict}};
            require(stored && stored->id == r.id && stored->identity == r.identity &&
                        stored->original_facts == award() &&
                        stored->original_arrival == orig.arrival && stored->attempts == attempts &&
                        c.retained("ctx-retry-A") == retry_value.retained &&
                        c.retained("ctx-conflict-A") == conflict_value.retained,
                    "Three ordered FK-linked attempts have their own exact facts and arrivals");
            const auto [cid, state] = c.support(r);
            const auto expected = plan_event(orig, r, cid);
            const auto plan =
                initial_plan(c, selected(c.select(r, orig.event, cid), expected), state);
            refusal(c.select(r, attempt.event, cid), Failure::invalid);
            auto counterfeit = r;
            counterfeit.arrival = retry_arrival;
            refusal(c.select(counterfeit, orig.event, cid), Failure::invalid);
            const auto again = selected(c.select(r, orig.event, cid), expected);
            require(
                original_proposal(again).effects[0].rounded_units == 120000 &&
                    plan.revisions[0].captured.requested_effects[0].requested_units == 120000 &&
                    c.db.rows("SELECT contribution_id FROM main.subathon_contributions").size() ==
                        1 &&
                    c.db.rows("SELECT plan_ref FROM main.subathon_plan_headers").size() == 1,
                "Only original event-A creates one CID/plan with its original factor");
            c.no_application();
        });
    }

    void a5_recoveryAndLaterRateKeepOriginalUnresolved() {
        scenario([] {
            Case c;
            c.config("cfg-U");
            const auto clock = recovery_unknown();
            const ModifierArrivalInput input{{{"synthetic", "account-U"}, "event-v1", "receipt-U"},
                                             {1008, "conn-U", "cfg-U", "ctx-U"},
                                             {"event-U", "event-U", {}},
                                             {}};
            auto facts = award();
            facts.money = core::ReceiptMoney{"20.0000", "EUR"};
            facts.payment_reference = core::ReceiptLinkReference{"payment-v1", "payment-U"};
            const auto r = c.original(input, facts);
            const auto [cid, state] = c.support(r, "support-U", "proof-U");
            const auto current = core::make_fetched_rate_observation({{"current-quote",
                                                                       "fixture-feed",
                                                                       "cfg-U",
                                                                       "rate-request",
                                                                       "rate-proof",
                                                                       {},
                                                                       700000,
                                                                       {{}, 699999},
                                                                       {}},
                                                                      {{"EUR", "CHF", "1.20"}}});
            require(current.data().quotes.size() == 1 &&
                        current.data().quotes[0].rate.value() == core::Exact{6} / 5 &&
                        current.data().quotes[0].rate.original() == "1.20" &&
                        current.data().context.fetched_at_utc_ms >
                            input.arrival.server_timestamp_utc_ms,
                    "Independent rate owner has a later exact current quote");
            c.db.reopen();
            require(c.receipt(input.identity)->original_facts.money == facts.money &&
                        state.conditions.valuation.monetary &&
                        state.conditions.valuation.missing_rate &&
                        !state.conditions.valuation.resolution_ref &&
                        state.conditions.valuation.original_money == facts.money,
                    "Shipped owner retains original exact monetary facts with missing valuation");
            const auto retained = c.retained("ctx-U");
            require(retained && retained->document.endsWith('\1'),
                    "Role-0 value explicitly records unresolved eligibility");
            refusal(c.select(r, input.event, cid), Failure::unresolved);
            const auto view =
                project_modifier_binding("cfg-U", clock.revision.captured_definition, 1, clock, {});
            require(!view.active && !view.remaining_ms && !view.name && view.factor == 2,
                    "Recovery supplies no active or remaining fact");
            require(
                c.db.rows("SELECT plan_ref FROM main.subathon_plan_headers").isEmpty(),
                "No modifier-dependent 60000 base is scaled or retained without original evidence");
            c.no_application();
        });
    }

    void a5_missingExactHistoryHasNoLogicalFallback() {
        scenario([] {
            Case c;
            c.config("cfg-A");
            c.live_original();
            const auto input = arrival_a();
            const auto r = c.original(input, award());
            const auto [cid, state] = c.support(r);
            c.replace();
            bool called = false;
            refusal(c.select(r, input.event, cid, {},
                             [&](const auto &, auto revision) {
                                 require(revision == "j1",
                                         "Only exact original instance revision can be resolved");
                                 called = true;
                                 return std::nullopt;
                             }),
                    Failure::unresolved);
            require(called, "Specific immutable revision read was unavailable");
            expect_original(selected(c.select(r, input.event, cid), plan_event(input, r, cid)));
            require(c.db.rows("SELECT plan_ref FROM main.subathon_plan_headers").isEmpty(),
                    "Unresolved read does not alter valid retained state");
            c.no_application();
        });
    }

    void a6_triggerCut_data() {
        QTest::addColumn<int>("branch");
        QTest::newRow("include") << 0;
        QTest::newRow("subsequent") << 1;
        QTest::newRow("wrong-event") << 2;
        QTest::newRow("contradictory-prior-view") << 3;
        QTest::newRow("wrong-cut") << 4;
    }

    void a6_triggerCut() {
        QFETCH(int, branch);
        scenario([&] {
            Case c;
            c.config("cfg-A");
            c.append_old();
            using Timing = core::ModifierTriggerTiming;
            const bool include = branch == 0 || branch == 2 || branch == 4;
            const core::ModifierTriggeringContribution trigger{
                "event-T", include ? Timing::include_trigger : Timing::subsequent_only};
            const auto activated = c.read([&](const auto &s) {
                SqliteModifierActivationStore owner(s);
                require(!owner.live_instance("D"), "No instance before ordered activation");
                return owner.activate(
                    "D", "z-old", {},
                    {"I", "j0", {core::ModifierActivationKind::rule, "rule-A"}, trigger});
            });
            require(activated.revision_to_append &&
                        activated.revision_to_append->instance_ref == "I" &&
                        activated.revision_to_append->revision_ref == "j0" &&
                        activated.revision_to_append->trigger == trigger &&
                        (include ? activated.for_trigger == activated.revision_to_append
                                 : !activated.for_trigger),
                    "Owning activation supplies the ordered trigger view");
            const auto live = c.read(
                [](const auto &s) { return SqliteModifierActivationStore(s).live_instance("D"); });
            require(live && live->instance == activated.revision_to_append &&
                        live->instance.remaining_ms == 480000,
                    "Independently qualifies the subsequent new head");
            const std::vector<core::ModifierTargetPolicy> targets{
                target(main_target, "g-main", {"I", "j0", "z-old", 1}),
                target(counter_target, "g-counter", {"I", "j0", "z-old", 2})};
            const std::vector<ModifierArrivalInstance> post{{{"I", "j0"}, "z-old", 480000}};
            const core::ModifierClockObservation clock{"lineage-A", "clock-T", 2000};
            auto input = arrival_a();
            input.identity.opaque_id = "receipt-T";
            input.arrival = {2000, "conn-T", "cfg-A", "ctx-T"};
            const std::string issued = branch == 2 ? "other-event" : "event-T";
            input.event = {issued, issued, {}};
            const bool claims_post_view = include || branch == 3;
            input.cut = QualifiedModifierCut{
                clock,
                1,
                profile,
                {"policy-T", "cfg-A", claims_post_view ? targets : decltype(targets){}},
                {"payment"},
                claims_post_view ? post : decltype(post){},
                ModifierTriggerCut{trigger, clock, 1, activated}};
            if (branch == 4)
                --input.cut->trigger->turn;
            if (branch >= 2) {
                const auto refused = prepare_modifier_arrival(input, limits);
                require(std::get_if<Failure>(&refused) &&
                            std::get<Failure>(refused) == Failure::invalid &&
                            !c.receipt(input.identity) && !c.retained("ctx-T"),
                        "Wrong triggering event or contradictory prior view fails qualification");
                c.no_application();
                return;
            }
            const auto r = c.original(input, award());
            const auto p =
                proposal(evaluate(selected(c.select(r, input.event), plan_event(input, r, {}))),
                         include ? 120000 : 60000);
            const std::vector<ModifierTraceLink> trigger_links{{1, {"I", "j0", "z-old", 1}},
                                                               {2, {"I", "j0", "z-old", 2}}};
            if (include)
                require(p.links == trigger_links && p.targets.size() == 2 &&
                            p.selection.size() == 2 && p.targets[1].target == counter_target &&
                            p.selection[1].original_factor_decimal == "1.250",
                        "Include selects both exact target occurrences from its new trigger view");
            else
                require(p.links.empty() && p.selection.empty(),
                        "Qualified prior empty view has identity and no occurrence");
            if (!include) {
                input.identity.opaque_id = "receipt-U";
                input.arrival = {2000, "conn-U", "cfg-A", "ctx-U"};
                input.event = {"event-U", "event-U", {}};
                input.cut = QualifiedModifierCut{
                    clock, 2, profile, {"policy-T", "cfg-A", targets}, {"payment"}, post, {}};
                const auto next = c.intake(input, award(), prepare(input));
                require(next.receipt_id.value == 2 &&
                            next.disposition == ReceiptDisposition::created,
                        "Later ordered action at the same UTC has its own event and receipt");
                const ContributionReceipt later{next.receipt_id, input.identity, award(),
                                                input.arrival};
                const auto fresh = proposal(
                    evaluate(selected(c.select(later, input.event), plan_event(input, later, {}))),
                    120000);
                require(
                    fresh.links.size() == 2 && fresh.targets.size() == 2 &&
                        fresh.targets[1].target == counter_target &&
                        fresh.selection[1].original_factor_decimal == "1.250",
                    "Later independent source sees qualified new head and unused counter trace");
            }
            c.no_application();
        });
    }

    void a6_exactNameAndQualifiedClock() {
        scenario([] {
            Case c;
            c.config("cfg-A");
            c.live_original();
            const auto [instance, definition] = c.read([](const auto &s) {
                SqliteModifierActivationStore owner(s);
                return std::pair{owner.instance_revision("j1").value().instance,
                                 owner.definition_revision("z-old").value().definition};
            });
            const ModifierBindingName name{"cfg-A", "D", "z-old", "Double time"};
            const core::ModifierClockValue known{instance, 180000};
            const auto expect_known = [&](const core::ModifierClockValue &view,
                                          const std::optional<ModifierBindingName> &record,
                                          std::optional<std::string> expected) {
                const auto binding = project_modifier_binding("cfg-A", definition, 1, view, record);
                require(binding.name == expected && binding.factor == 2 &&
                            binding.factor_decimal == "2.00" && binding.remaining_ms == 180000 &&
                            binding.active == true,
                        "Exact version name availability is separate from numeric and clock facts");
            };
            expect_known(known, name, "Double time");
            expect_known(known, {}, {});
            expect_known(known, ModifierBindingName{"cfg-A", "D", "a-new", "Replacement"}, {});
            expect_known(known, ModifierBindingName{"other-cfg", "D", "z-old", "Other config"}, {});
            expect_known(known, ModifierBindingName{"cfg-A", "E", "z-old", "Other definition"}, {});
            const auto timer = finance::timer(finance::Life::Active, 3540000);
            const core::TimerClockStep step{50, 1050, true};
            const auto paused = core::project_modifier_cut(
                {"lineage-A", "clock-A", 1000}, {"lineage-A", "clock-paused-A", 1050}, timer, step,
                core::advance_timer(timer, {}, step), known);
            require(paused.head && paused.head->remaining_ms == 180000,
                    "Existing owner supplies known-active paused clock without consuming time");
            expect_known(paused.head.value(), name, "Double time");
            // A viewer can hide this binding locally. Visibility is absent from
            // the projection inputs, so that same view retains its active facts.
            const auto unknown =
                project_modifier_binding("cfg-A", definition, 1, {instance, {}}, name);
            require(unknown.name == name.name && unknown.factor == 2 &&
                        unknown.factor_decimal == "2.00" && !unknown.remaining_ms &&
                        !unknown.active,
                    "Unavailable recovery evidence does not turn saved positive time into active");
            const auto counter = project_modifier_binding("cfg-A", definition, 2, known, name);
            require(counter.name == name.name && counter.factor == core::Exact{5} / 4 &&
                        counter.factor_decimal == "1.250",
                    "Exact ordinal selects spelling and rational for a different target");
        });
    }

    void a7_canonical65BytesAndExactReRetention() {
        scenario([] {
            Case c;
            c.config("K");
            c.config("other-K");
            require(canonical.size() == 65, "Independent frozen role-0 unresolved byte length");
            const auto prepared = prepare(unresolved_c);
            require(prepared.retained == document("C", "K", canonical),
                    "Exact frozen big-endian framing, refs, absent parent and unresolved byte");
            const auto r = c.intake(unresolved_c, award(), prepared);
            require(r.receipt_id.value == 1 && r.disposition == ReceiptDisposition::created,
                    "First receipt atomically selects canonical role-0 context");
            c.db.reopen();
            c.db.write([&](const auto &s) {
                require(
                    retain_modifier_arrival(s, prepared) == RetainDisposition::already_present &&
                        SqliteRetainedContextStore(s).retain(document("C", "other-K", canonical)) ==
                            RetainDisposition::conflict,
                    "Exact re-retention is idempotent; raw retarget of same key conflicts");
            });
            require(c.retained("C") == prepared.retained &&
                        c.receipt(unresolved_c.identity)->attempts.size() == 1,
                    "Re-retention adds no delivery or attempt");
            refusal(c.select({r.receipt_id, unresolved_c.identity, award(), unresolved_c.arrival},
                             unresolved_c.event),
                    Failure::unresolved);
        });
    }

    void a7_localRetainedShape_data() {
        QTest::addColumn<int>("branch");
        QTest::newRow("typed-link-mismatch") << 0;
        QTest::newRow("over-limit-length") << 1;
        QTest::newRow("unsupported-role") << 2;
        QTest::newRow("bounded-truncation") << 3;
        QTest::newRow("trailing-byte") << 4;
        QTest::newRow("unsupported-magic") << 5;
        QTest::newRow("wrong-selected-role") << 6;
    }

    void a7_localRetainedShape() {
        QFETCH(int, branch);
        scenario([&] {
            Case c;
            c.config("K");
            c.config("other-K");
            QByteArray bytes = canonical;
            auto expected = Failure::invalid;
            if (branch == 1 || branch == 5) {
                bytes.replace(5, 4, QByteArray::fromHex("ffffffff"));
                expected = Failure::resource_limit;
            }
            if (branch == 2)
                bytes[4] = '\2';
            if (branch == 3)
                bytes.truncate(9);
            if (branch == 4)
                bytes.append('\0');
            if (branch == 5) {
                bytes.replace(0, 4, "OLD!");
                expected = Failure::unresolved;
            }
            const auto raw = document("C", branch == 0 ? "other-K" : "K", bytes);
            c.db.write([&](const auto &s) {
                require(SqliteRetainedContextStore(s).retain(raw) == RetainDisposition::inserted,
                        "Generic owner retains selected opaque local bytes without type knowledge");
            });
            c.db.reopen();
            const auto stored = c.retained("C");
            require(stored == raw, "Typed decoder sees one exact bounded retained record");
            require(inspect_modifier_record_shape(*stored, branch == 6, limits) == expected,
                    "Exact typed shape refusal for selected metadata and byte mutation");
            require(c.retained("C") == raw, "Opaque store still preserves exact selected bytes");
        });
    }

    void a7_existingReferenceArrivalRemainsKnownEmptyInItsOwner() {
        scenario([] {
            namespace ref = reference_intake_fixture;
            Case c;
            const auto config_doc = QByteArray::fromHex(ref::literals()["configurations"]
                                                            .toObject()["standard"]
                                                            .toObject()["document_hex"]
                                                            .toString()
                                                            .toLatin1());
            const auto anchor = ref::committed();
            c.db.write([&](const auto &s) {
                require(SqliteRetainedContextStore(s).retain(
                            {{RetainedKind::configuration, "D"}, config_doc, {}}) ==
                                RetainDisposition::inserted &&
                            SqliteApplicationBundleStore(s).retain_initial_state(anchor.snapshot) ==
                                SqliteBundleInitialDisposition::inserted,
                        "Prior owner starts from exact independent SRC1/SAS1 fixture values");
            });
            ReferenceConfigurationArchive configuration(ref::versions());
            ref::Sampler sampler;
            ref::Entropy entropy;
            ref::Clock clock;
            SqliteReferenceAuthoritySource authority;
            ReferenceApplicationAdapters runtime(
                configuration, authority, sampler, entropy,
                std::optional<ReferenceClockAnchor>{{anchor, {100, 1000}}});
            ReferenceIntake intake(configuration, sampler, entropy, clock, ref::connector(),
                                   &runtime);
            ref::Completion<ReferenceReceiptCompletion> first;
            first.run(c.db.writer(),
                      intake.make_receipt_work("D", {"Q", "reference-A7", {}, QByteArray("", 0)},
                                               first.callback()));
            finance::normal(first.result->work);
            require(first.result->receipt && first.result->receipt->receipt_id.value == 1 &&
                        first.result->receipt->disposition == ReceiptDisposition::created &&
                        first.result->original &&
                        first.result->original->configuration.modifiers ==
                            ReferenceFactsState::complete,
                    "Prior owner creates selected known-empty qualified SRA1");
            ref::Completion<ReferenceReceiptCompletion> retry;
            retry.run(c.db.writer(),
                      intake.make_receipt_work("D", {"Q", "reference-A7", {}, QByteArray("", 0)},
                                               retry.callback()));
            finance::normal(retry.result->work);
            const auto &old = *first.result->original;
            require(retry.result->receipt &&
                        retry.result->receipt->disposition == ReceiptDisposition::retry &&
                        retry.result->original &&
                        retry.result->original->arrival_context_ref == old.arrival_context_ref &&
                        retry.result->original->event_ref == old.event_ref &&
                        retry.result->original->configuration.modifiers ==
                            ReferenceFactsState::complete,
                    "Existing reference reader preserves its exact original and known-empty role");
            const auto sra1 = c.retained(old.arrival_context_ref);
            require(sra1 && sra1->document.startsWith("SRA1"), "Prior path really retained SRA1");
            const core::ReceiptIdentity identity{
                ref::connector().origin, ref::connector().identifier_namespace, "reference-A7"};
            const auto original = c.receipt(identity).value();
            const ContributionReceipt r{original.id, identity, original.original_facts,
                                        original.original_arrival};
            refusal(c.select(r, {old.event_ref, old.event_ref, {}}), Failure::unresolved);
            require(c.retained(old.arrival_context_ref) == sra1, "General refusal preserves SRA1");
        });
    }

    void a7_adoptedFailureAndLostAcknowledgement() {
        scenario([] {
            Case c;
            c.config("K");
            const auto prepared = prepare(unresolved_c);
            const auto intake = [&](auto ctx, const core::ReceiptArrival &arrival) {
                auto s = ctx.begin();
                require(retain_modifier_arrival(s, prepared) == RetainDisposition::inserted,
                        "Actual context staged before receipt adoption");
                SqliteReceiptRepository owner(adopt_active_receipt_session, s);
                return ReceiptIntake(owner).ingest({unresolved_c.identity, award(), arrival});
            };
            auto missing = unresolved_c.arrival;
            missing.configuration_ref = "missing-K";
            const auto rolled = run(c.db, [&](auto ctx) { (void)intake(ctx, missing); });
            require(rolled.second && rolled.first.transaction == TransactionOutcome::rolled_back &&
                        !rolled.first.value && !c.retained("C") &&
                        !c.receipt(unresolved_c.identity),
                    "Adopted receipt failure rolls back caller-staged context and receipt");
            try {
                std::rethrow_exception(rolled.second);
            } catch (const SqliteReceiptStorageFailure &e) {
                require(e.reason() == SqliteReceiptStorageReason::missing_reference,
                        "Exact receipt owner FK-related missing reference");
            }

            using namespace subathon::server::testing;
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        c.db.writer(), WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Existing writer control loses the acknowledgement of a real commit");
            std::optional<core::ReceiptIntakeResult> tentative;
            const auto uncertain =
                run(c.db, [&](auto ctx) { tentative = intake(ctx, unresolved_c.arrival); });
            require(
                !tentative && uncertain.first.transaction == TransactionOutcome::outcome_unknown &&
                    uncertain.first.error == WorkError::uncertain_outcome && !uncertain.first.value,
                "Adopted work cannot publish success without commit acknowledgement");
            c.db.reopen();
            const auto known = c.receipt(unresolved_c.identity);
            require(known && known->id.value == 1 &&
                        known->original_arrival == unresolved_c.arrival &&
                        known->original_facts == award() && known->attempts.size() == 1 &&
                        c.retained("C") == prepared.retained,
                    "Separate committed readback reconciles exactly one original and linked value");
            refusal(c.select({known->id, known->identity, known->original_facts,
                              known->original_arrival},
                             unresolved_c.event),
                    Failure::unresolved);
        });
    }

    void a8_priorityZeroIsPresentAndWinsTie() {
        scenario([] {
            Case c;
            c.config("cfg-A");
            c.live_original();
            const core::ModifierDefinition e{"E",
                                             "e-time",
                                             60000,
                                             {{Kind::added_time, "timer.main", {}, "0.50"}},
                                             core::ModifierReactivation::extend};
            c.db.write([&](const auto &s) {
                SqliteModifierActivationStore owner(s);
                require(!owner.current_definition("E") && !owner.live_instance("E") &&
                            owner.append_definition(e, {}).position == 1,
                        "Separate E has its own fresh initial definition");
            });
            c.db.write([&](const auto &s) {
                const auto activation = SqliteModifierActivationStore(s).activate(
                    "E", "e-time", {},
                    {"K", "ke0", {core::ModifierActivationKind::moderator, "operator-E"}, {}});
                require(activation.revision_to_append &&
                            activation.revision_to_append->remaining_ms == 60000 &&
                            !activation.for_trigger,
                        "Independent live K/ke0 has no trigger");
            });
            require(c.read([](const auto &s) {
                         return SqliteModifierActivationStore(s).live_instance("D");
                     })->instance == activation::checkpointed_instance,
                    "D/I/j1 stays live while E/K/ke0 becomes active");
            const core::ModifierOccurrenceKey fresh{"K", "ke0", "e-time", 1};
            auto input = arrival_a();
            auto &cut = *input.cut;
            cut.policy.policy_ref = "policy-P";
            cut.eligible.push_back({{"K", "ke0"}, "e-time", 60000});
            cut.policy.targets[0].groups = {{"g-priority",
                                             Operator::priority,
                                             {},
                                             {{original_main, core::ModifierPriority{7, 0}},
                                              {fresh, core::ModifierPriority{7, 1}}}}};
            const auto r = c.original(input, award());
            const auto stored = c.retained("ctx-A");
            // B(I) B(j1) B(z-old) ordinal1 presence1 rank7 order0.
            const auto first =
                QByteArray::fromHex("0000000149000000026a31000000057a2d6f6c640000000000000001"
                                    "0100000000000000070000000000000000");
            require(stored && stored->document.contains(first),
                    "MCA1 encodes explicit order zero with present priority byte");
            const auto value = selected(c.select(r, input.event), plan_event(input, r, {}));
            require(value.eligible.size() == 2 && value.eligible[0].observed == cut.eligible[0] &&
                        value.eligible[0].exact_instance == activation::checkpointed_instance &&
                        value.eligible[1].observed == cut.eligible[1] &&
                        value.eligible[1].exact_definition == e,
                    "Selected view owns both exact live revisions in captured order");
            const auto result = proposal(evaluate(value), 120000);
            require(result.links == std::vector<ModifierTraceLink>{{1, original_main},
                                                                   {2, fresh},
                                                                   {3, original_counter}} &&
                        result.targets.size() == 2 && result.targets[1].target == counter_target &&
                        result.targets[1].reduction.factor == core::Exact{5} / 4,
                    "Both priority members and valid unused original counter are traced");
            const auto &priority = result.targets[0].groups[0];
            require(priority.members.size() == 2 && priority.reduction.factor == 2 &&
                        priority.members[0].priority == core::ModifierPriority{7, 0} &&
                        priority.members[0].winner &&
                        priority.members[1].priority == core::ModifierPriority{7, 1} &&
                        !priority.members[1].winner,
                    "Equal ranks prefer earlier order zero, distinct from an absent priority");
            c.no_application();
        });
    }

    void recordedEqualFactorsKeepDistinctExactOrdinals() {
        scenario([] {
            Case c;
            c.config("cfg-A");
            const core::ModifierDefinition d{"D",
                                             "same-rows",
                                             180000,
                                             {{Kind::added_time, "timer.main", "payment", "2.00"},
                                              {Kind::added_time, "timer.main", "payment", "2.00"}},
                                             core::ModifierReactivation::extend};
            c.db.write([&](const auto &s) {
                SqliteModifierActivationStore owner(s);
                require(
                    owner.append_definition(d, {}).position == 1 &&
                        owner.activate("D", "same-rows", {}, {"I", "j0", activation::moderator, {}})
                            .revision_to_append.has_value(),
                    "Real owner retains two equal factors on distinct definition ordinals");
            });
            const core::ModifierOccurrenceKey first{"I", "j0", "same-rows", 1};
            const core::ModifierOccurrenceKey second{"I", "j0", "same-rows", 2};
            auto input = arrival_a();
            input.cut->eligible = {{{"I", "j0"}, "same-rows", 180000}};
            // Policy order is intentionally opposite definition order. The
            // corresponding plan rows have identical bytes.
            input.cut->policy.targets = {
                {main_target,
                 Operator::multiply,
                 {{"g-main", Operator::multiply, {}, {{second, {}}, {first, {}}}}}}};
            const auto r = c.original(input, award());
            const auto [cid, state] = c.support(r);
            const auto event = plan_event(input, r, cid);
            const auto before = selected(c.select(r, input.event, cid), event);
            const auto p = proposal(evaluate(before), 240000);
            require(p.selection.size() == 2 && p.selection[0] == p.selection[1],
                    "Identical recorded spelling and refs cannot identify an ordinal");
            RuleSource rules;
            ModifierSource mods(event, p.selection);
            const auto plan =
                core::capture_application_plan("plan-A",
                                               {event,
                                                state,
                                                {core::PlanValuationState::not_applicable, {}},
                                                {request(p.effects[0])}},
                                               rules, mods);
            c.db.write([&](const auto &s) {
                require(SqliteApplicationPlanStore(s).retain_initial("plan-A", plan.revisions[0]) ==
                            SqlitePlanInitialDisposition::inserted,
                        "Real plan owner retains both rows");
            });
            c.db.reopen();
            const auto stored = c.read([](const auto &s) {
                return SqliteApplicationPlanStore(s).find_revision("plan-A", 1);
            });
            require(stored && stored->revision == plan.revisions[0], "Exact persisted equal rows");
            const auto &rows = stored->revision.modifiers;
            const auto value =
                selected(c.select(r, input.event, cid, {}, {}, std::span(rows)), event);
            const std::vector<ModifierTraceLink> links{{1, second}, {2, first}};
            require(value.recorded_selection == rows && value.recorded_links == links &&
                        proposal(evaluate(value), 240000).links == links,
                    "Derive ordinal links from ordered policy through the exact revisions");
            c.no_application();
        });
    }
};

QTEST_GUILESS_MAIN(ModifierCaptureTests)
#include "capture_tests.moc"
