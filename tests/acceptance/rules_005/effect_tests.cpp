// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

#include <limits>

namespace {
using namespace rules_005;

const ModifierTargetKey main_lane{ModifierTargetKind::added_time, "Main"};

SelectedModifiers donation_modifiers(const std::string &cfg, const std::string &instance_revision,
                                     const std::string &definition_revision, std::string factor) {
    ModifierDefinition definition{"D",
                                  definition_revision,
                                  100,
                                  {{main_lane.kind, main_lane.ref, "payment", std::move(factor)}}};
    auto instance = propose_modifier_activation(
                        definition, {},
                        {"I", instance_revision, {ModifierActivationKind::moderator, "author"}, {}})
                        .revision_to_append.value();
    const ModifierTraceProfile profile{"modifier-handoff", "admitted-v1"};
    return {{"donation-context", cfg, "donation-stack", profile, {}},
            profile,
            {"donation-stack",
             cfg,
             {{main_lane,
               ModifierStackOperator::multiply,
               {{"G",
                 ModifierStackOperator::multiply,
                 {},
                 {{{"I", instance_revision, definition_revision, 1}, {}}}}}}}},
            std::vector{std::move(instance)},
            {"payment"}};
}

HistoricalValuation donation_valuation() {
    return {
        "identity-valuation", {"20", "EUR"}, "EUR", "1", {}, "identity-snapshot", {}, {}, {}, {}};
}

ScenarioInputs donation(const RootBinding &r, bool alternate) {
    const std::string cfg = alternate ? "hypothetical-donation" : "captured-donation";
    const auto configuration = validate_rule_configuration(
        {cfg, {}, {rule("award", Kind::payment_confirmed, {time("donation-time", "300000")})}});
    auto in = inputs(3'300'000, 3'600'000);
    add_root(in, r);
    auto capture = retain(
        r.event, configuration, Kind::payment_confirmed, "payment_confirmed",
        {RuleMoney{RuleMoneyField::source_amount, {"20", "EUR"}}, PaymentEvidence::confirmed},
        donation_valuation(), {}, "donation-context");
    capture.receipt.facts.payment_reference =
        ReceiptLinkReference{"payment", "payment-" + r.event.event};
    capture.rates.emplace(
        make_effective_rate_snapshot({"identity-snapshot", {}, {}, {}, {}, {}, {}}));
    capture.modifiers = alternate ? donation_modifiers(cfg, "I2", "D2", "1")
                                  : donation_modifiers(cfg, "I1", "D1", "2.0");
    auto head = capture.modifiers->eligible->front();
    // The later owner sample is after expiry of this exact arrival revision.
    in.initial.current_modifier =
        ModifierClockValue{std::move(head), 0, ModifierClockMode::RealElapsed};
    in.captures.push_back(std::move(capture));
    turn(in, "apply-captured-donation", event(r.event), {101, 101ms});
    return in;
}

void expect_donation(const SimulationResult &result, bool alternate) {
    ASSERT_FALSE(result.trace.empty());
    const auto &trace = result.trace.front();
    ASSERT_TRUE(trace.selection);
    EXPECT_EQ(trace.selection->selection.configuration_ref,
              alternate ? "hypothetical-donation" : "captured-donation");
    ASSERT_TRUE(trace.plan);
    EXPECT_EQ(trace.plan->inspected.captured.valuation.historical, donation_valuation());
    ASSERT_TRUE(trace.modifiers);
    const auto &proposal = std::get<ModifierStackProposal>(*trace.modifiers);
    ASSERT_EQ(proposal.selection.size(), 1U);
    EXPECT_EQ(proposal.selection.front().instance_revision_ref, alternate ? "I2" : "I1");
    EXPECT_EQ(proposal.selection.front().definition_version_ref, alternate ? "D2" : "D1");
    EXPECT_EQ(proposal.selection.front().original_factor_decimal, alternate ? "1" : "2.0");
    ASSERT_EQ(proposal.targets.size(), 1U);
    const auto &lane = proposal.targets.front();
    EXPECT_EQ(lane.target, main_lane);
    EXPECT_EQ(lane.reduction.factor, Exact(alternate ? 1 : 2));
    ASSERT_EQ(lane.groups.size(), 1U);
    EXPECT_EQ(lane.groups.front().group_ref, "G");
    EXPECT_EQ(lane.groups.front().reduction.operation, ModifierStackOperator::multiply);
    ASSERT_EQ(lane.groups.front().members.size(), 1U);
    EXPECT_EQ(lane.groups.front().members.front().occurrence.key,
              (ModifierOccurrenceKey{"I", alternate ? "I2" : "I1", alternate ? "D2" : "D1", 1}));
    ASSERT_EQ(proposal.effects.size(), 1U);
    const auto request = alternate ? 300'000 : 600'000;
    EXPECT_EQ(proposal.effects.front().base.exact_units, Exact(300'000));
    EXPECT_EQ(proposal.effects.front().exact_units, Exact(request));
    EXPECT_EQ(proposal.effects.front().rounded_units, request);
    ASSERT_EQ(trace.actions.size(), 1U);
    expect_actual_time(trace.actions.front(), key("award", "donation-time"),
                       {request, 300'000, 0, alternate ? 0 : 300'000, 3'600'000});
    EXPECT_EQ(trace.state.timer.main_remaining_ms, 3'600'000);
}

TEST(Rules005, S2CapturedFactorsCapAndHypothetical) {
    const auto r = root("donation");
    const auto original = replay("S2", donation(r, false));
    auto replay_with_current = original;
    const auto live = donation_modifiers("unrelated-current", "current-I", "current-D", "1");
    replay_with_current.current.configuration.emplace(validate_rule_configuration(
        {"unrelated-current",
         {},
         {rule("award", Kind::payment_confirmed, {time("donation-time", "300000")})}}));
    replay_with_current.current.modifier =
        ModifierClockValue{live.eligible->front(), 100, ModifierClockMode::RealElapsed};
    const Scenario alternate{
        "S2-hypothetical", 1,
        Hypothetical{"hypothetical-capture", donation(root("alternate"), true)}};

    for (const auto *scenario :
         {&original, static_cast<const Scenario *>(&replay_with_current), &alternate, &original}) {
        SCOPED_TRACE(scenario->ref);
        const auto result = inspect_simulation(*scenario, limits);
        expect_complete(result, *scenario);
        ASSERT_EQ(result.trace.size(), 1U);
        expect_donation(result, scenario == &alternate);
        EXPECT_EQ(result.final_state.timer.main_remaining_ms, 3'600'000);
    }
    EXPECT_EQ(std::get<CapturedReplay>(original.input).inputs.initial.timer.main_remaining_ms,
              3'300'000);

    auto invalid = original;
    std::get<CapturedReplay>(invalid.input).attempted_manual_modifiers =
        donation_modifiers("captured-donation", "I2", "D2", "1");
    const auto result = inspect_simulation(invalid, limits);
    EXPECT_EQ(result.completion, Completion::invalid_input);
    EXPECT_TRUE(result.simulated);
    EXPECT_EQ(result.owner_callbacks, 0U);
    EXPECT_TRUE(result.trace.empty());
    EXPECT_EQ(result.final_state.timer.main_remaining_ms, 3'300'000);
}

ScenarioInputs ordered_refusal(const RootBinding &r, RuleCompletion completion) {
    const auto cfg = validate_rule_configuration(
        {"ordered-config",
         {},
         {rule("H", Kind::named_event, {time("first", "100"), message(), time("tail", "200")}, {},
               10),
          rule("L", Kind::named_event, {score("score", "7")})}});
    auto in = inputs(1000);
    add_root(in, r);
    in.captures.push_back(retain(r.event, cfg, Kind::named_event, "named_event",
                                 {RuleText{RuleTextField::event_name, "ordered"}}));
    auto apply = event(r.event, completion);
    apply.chat = ChatObservation{key("H", "chat", 1), "Y", RuleGate::deny, {}, {}};
    apply.effect_refs = {{key("H", "first"), "first"}, {key("L", "score"), "score"}};
    turn(in, "apply", std::move(apply));
    return in;
}

RuleStop execution_stop() {
    RuleStopDetail detail{};
    detail.channel = RuleAuthorityChannel::execution;
    return {1, RuleActionRefusal::authorization_hold, detail};
}

ModeledHistory ordered_committed(const RootBinding &r) {
    const RuleCommittedPass pass{
        {{0,
          {{key("H", "first"),
            RuleEffectObservation{"first", RuleTimeObservation{100, 100, 0, 0, 1100}}}},
          execution_stop()},
         {1,
          {{key("L", "score"),
            RuleEffectObservation{
                "score", RuleQuantityObservation{"micro_points", {"7", "1"}, {"7", "1"}}}}},
          {}}}};
    return {
        r,
        "admit-" + r.event.event,
        limits.chain,
        {{r.event, 0, 0}},
        {{{"apply", r.event.chain, r.event.event, UnitPurpose::event_pass}, pass, {}, {}, {}, {}}},
        {},
        {}};
}

void expect_ordered_stop(const TurnTrace &trace) {
    ASSERT_EQ(trace.actions.size(), 4U);
    const auto &message = trace.actions[1];
    EXPECT_EQ(message.intent.key, key("H", "chat", 1));
    EXPECT_EQ(message.message, (MessageIntent{"X", "Y", "T", {}}));
    ASSERT_TRUE(message.stop);
    EXPECT_EQ(message.stop->position, 1U);
    EXPECT_EQ(message.stop->refusal, RuleActionRefusal::authorization_hold);
    ASSERT_TRUE(message.stop->detail);
    EXPECT_EQ(message.stop->detail->channel, RuleAuthorityChannel::execution);
    EXPECT_EQ(message.gates, (std::vector<CheckedGate>{{Gate::use, RuleGate::permit},
                                                       {Gate::grant, RuleGate::permit},
                                                       {Gate::execution, RuleGate::deny}}));
    EXPECT_FALSE(message.submitted);
    EXPECT_FALSE(message.accepted);
    EXPECT_FALSE(trace.delivery);
    const auto &tail = trace.actions[2];
    EXPECT_EQ(tail.intent.key, key("H", "tail", 2));
    EXPECT_EQ(
        std::get<RuleFixedDelta>(std::get<RuleDeltaAction>(tail.intent.captured_payload).delta)
            .original_decimal,
        "200");
    EXPECT_TRUE(tail.gates.empty());
    EXPECT_FALSE(tail.stop);
    EXPECT_FALSE(tail.proposed);
    EXPECT_FALSE(tail.submitted);
    EXPECT_FALSE(tail.accepted);
    EXPECT_EQ(trace.actions[3].intent.key, key("L", "score"));
    EXPECT_TRUE(trace.actions[3].submitted);
}

TEST(Rules005, S7OrderedRefusalRollbackAndReconciliation) {
    const auto r = root("ordered");
    for (const auto outcome :
         {RuleCompletion::committed, RuleCompletion::rolled_back, RuleCompletion::outcome_unknown,
          RuleCompletion::abnormal_completion}) {
        SCOPED_TRACE(static_cast<int>(outcome));
        const auto scenario = replay("one-pass", ordered_refusal(r, outcome));
        const auto result = inspect_simulation(scenario, limits);
        EXPECT_TRUE(result.simulated);
        ASSERT_EQ(result.trace.size(), 1U);
        const auto &trace = result.trace.front();
        expect_ordered_stop(trace);
        EXPECT_EQ(trace.completion, outcome);
        const bool committed = outcome == RuleCompletion::committed;
        EXPECT_EQ(result.completion, outcome == RuleCompletion::outcome_unknown ||
                                             outcome == RuleCompletion::abnormal_completion
                                         ? Completion::unit_hold
                                         : Completion::complete);
        EXPECT_EQ(trace.publication.has_value(), committed);
        EXPECT_EQ(result.final_state.timer.main_remaining_ms, committed ? 1100 : 1000);
        EXPECT_EQ(result.final_state.score_micro_points, committed ? 7 : 0);
        if (committed) {
            expect_actual_time(trace.actions.front(), key("H", "first"), {100, 100, 0, 0, 1100});
            ASSERT_TRUE(trace.actions[3].accepted);
            EXPECT_EQ(std::get<RuleQuantityObservation>(
                          std::get<RuleEffectObservation>(*trace.actions[3].accepted).change)
                          .actual.numerator,
                      "7");
        } else {
            for (const auto &action : trace.actions)
                EXPECT_FALSE(action.accepted);
        }
    }

    auto in = ordered_refusal(r, RuleCompletion::outcome_unknown);
    const auto history = ordered_committed(r);
    const auto identity = history.publications.front().identity;
    turn(in, "while-held", event(r.event));
    turn(in, "exact-readback", UnitReconciliation{r.event.chain, identity, history});
    turn(in, "same-event-again", event(r.event));
    const auto scenario = replay("ordered-readback", std::move(in));
    const auto result = inspect_simulation(scenario, limits);
    expect_complete(result, scenario);
    ASSERT_EQ(result.trace.size(), 4U);
    EXPECT_FALSE(result.trace[0].publication);
    EXPECT_EQ(result.trace[0].state.timer.main_remaining_ms, 1000);
    EXPECT_EQ(result.trace[0].state.score_micro_points, 0);
    EXPECT_EQ(result.trace[1].begin_disposition, BeginDisposition::in_flight_hold);
    EXPECT_TRUE(result.trace[1].actions.empty());
    ASSERT_TRUE(result.trace[2].publication);
    EXPECT_EQ(result.trace[2].publication->identity, identity);
    EXPECT_TRUE(result.trace[2].actions.empty());
    EXPECT_EQ(result.trace[2].state.timer.main_remaining_ms, 1100);
    EXPECT_EQ(result.trace[2].state.score_micro_points, 7);
    EXPECT_EQ(result.trace[3].begin_disposition, BeginDisposition::duplicate_event);
    EXPECT_FALSE(result.trace[3].publication);
    EXPECT_TRUE(result.trace[3].actions.empty());
    EXPECT_EQ(result.final_state.timer.main_remaining_ms, 1100);
    EXPECT_EQ(result.final_state.score_micro_points, 7);
}

TEST(Rules005, ElapsedTimePrecedesRuleEffects) {
    const auto r = root("timed-effect");
    auto in = ordered_refusal(r, RuleCompletion::committed);
    in.turns.front().sample = {500, 500ms};
    turn(in, "status-only",
         ConnectorReport{{},
                         ConnectorInstallation::validated,
                         ConnectorConsent::granted,
                         ConnectorConnection::connected,
                         ConnectorGap::none},
         {550, 550ms});
    turn(in, "after-effect", TimeCut{}, {600, 600ms});
    const auto scenario = replay("timer-cut-before-effect", std::move(in));

    const auto result = inspect_simulation(scenario, limits);
    expect_complete(result, scenario);
    ASSERT_EQ(result.trace.size(), 3U);
    const auto &effect = result.trace[0];
    ASSERT_TRUE(effect.timer_cut);
    EXPECT_EQ(effect.timer_cut->progressed_ms, 500);
    ASSERT_FALSE(effect.actions.empty());
    expect_actual_time(effect.actions.front(), key("H", "first"), {100, 100, 0, 0, 600});
    EXPECT_EQ(effect.state.timer.main_remaining_ms, 600);
    EXPECT_TRUE(result.trace[1].connector);
    EXPECT_FALSE(result.trace[1].timer_cut);
    ASSERT_TRUE(result.trace[2].timer_cut);
    EXPECT_EQ(result.trace[2].timer_cut->progressed_ms, 100);
    EXPECT_EQ(result.final_state.timer.main_remaining_ms, 500);
}

TEST(Rules005, PauseControlAppliesAtTheObservedSample) {
    auto in = inputs(1000);
    turn(in, "pause", PauseControl{PauseReason::Manual, true}, {500, 500ms});
    turn(in, "paused-cut", TimeCut{}, {600, 600ms});
    turn(in, "resume", PauseControl{PauseReason::Manual, false}, {700, 700ms});
    turn(in, "running-cut", TimeCut{}, {800, 800ms});
    const auto scenario = replay("timer-cut-before-pause", std::move(in));

    const auto result = inspect_simulation(scenario, limits);
    expect_complete(result, scenario);
    ASSERT_EQ(result.trace.size(), 4U);
    ASSERT_TRUE(result.trace[0].timer_cut);
    EXPECT_EQ(result.trace[0].timer_cut->progressed_ms, 500);
    EXPECT_EQ(result.trace[0].state.timer.main_remaining_ms, 500);
    EXPECT_EQ(result.trace[0].state.pause.reasons, PauseReasons{PauseReason::Manual});
    ASSERT_TRUE(result.trace[1].timer_cut);
    EXPECT_EQ(result.trace[1].timer_cut->progressed_ms, 0);
    EXPECT_EQ(result.trace[2].state.timer.main_remaining_ms, 500);
    EXPECT_TRUE(result.trace[2].state.pause.reasons.empty());
    ASSERT_TRUE(result.trace[3].timer_cut);
    EXPECT_EQ(result.trace[3].timer_cut->progressed_ms, 100);
    EXPECT_EQ(result.final_state.timer.main_remaining_ms, 400);
}

TEST(Rules005, HeldTimerCutKeepsElapsedForAdmittedCut) {
    const auto r = root("held-timer");
    auto in = ordered_refusal(r, RuleCompletion::outcome_unknown);
    const auto history = ordered_committed(r);
    const auto identity = history.publications.front().identity;
    turn(in, "held-cut", TimeCut{}, {500, 500ms});
    turn(in, "exact-readback", UnitReconciliation{r.event.chain, identity, history}, {550, 550ms});
    turn(in, "admitted-cut", TimeCut{}, {600, 600ms});
    const auto scenario = replay("held-timer-cut", std::move(in));

    const auto result = inspect_simulation(scenario, limits);
    expect_complete(result, scenario);
    ASSERT_EQ(result.trace.size(), 4U);
    EXPECT_TRUE(result.trace[1].clock_held);
    EXPECT_FALSE(result.trace[1].timer_cut);
    EXPECT_EQ(result.trace[1].state.timer.main_remaining_ms, 1000);
    ASSERT_TRUE(result.trace[2].publication);
    EXPECT_EQ(result.trace[2].publication->identity, identity);
    EXPECT_EQ(result.trace[2].state.timer.main_remaining_ms, 1100);
    ASSERT_TRUE(result.trace[3].timer_cut);
    EXPECT_EQ(result.trace[3].timer_cut->progressed_ms, 600);
    EXPECT_EQ(result.final_state.timer.main_remaining_ms, 500);
}

const RuleDeliveryObservation queued{"J", RuleDeliveryState::queued, 1, {}, {}};
const RuleDeliveryObservation sending{"J", RuleDeliveryState::sending, 2, 2, {}};
const RuleDeliveryObservation delivered{"J", RuleDeliveryState::delivered, 3, 2, "evidence-J"};

Scenario messaging(RuleCompletion completion = RuleCompletion::committed, bool other_job = false) {
    const auto cfg = validate_rule_configuration(
        {"message-config", {}, {named("message-rule", "message", {message()})}});
    const auto r = root("message-event");
    auto in = inputs(1000);
    add_root(in, r);
    in.captures.push_back(retain(r.event, cfg, Kind::named_event, "named_event",
                                 {RuleText{RuleTextField::event_name, "message"}}));
    auto apply = event(r.event, completion);
    apply.chat =
        ChatObservation{key("message-rule", "chat"), "Y", RuleGate::permit, "hello", queued};
    turn(in, "queue-intent", std::move(apply));
    turn(in, "observe-attempt", DeliveryTurn{"J", sending});
    auto final = delivered;
    if (other_job)
        final.job_ref = "K";
    turn(in, "observe-result", DeliveryTurn{"J", std::move(final)});
    return replay(other_job                                 ? "other-job"
                  : completion == RuleCompletion::committed ? "message-delivered"
                                                            : "queue-rollback",
                  std::move(in));
}

TEST(Rules005, S8QueueSendingAndDeliveryAreSeparate) {
    for (const auto &scenario : {messaging(), messaging(RuleCompletion::committed, true),
                                 messaging(RuleCompletion::rolled_back)}) {
        SCOPED_TRACE(scenario.ref);
        const auto result = inspect_simulation(scenario, limits);
        expect_complete(result, scenario);
        ASSERT_EQ(result.trace.size(), 3U);
        const auto &event = result.trace[0];
        ASSERT_EQ(event.actions.size(), 1U);
        EXPECT_EQ(event.actions.front().message, (MessageIntent{"X", "Y", "T", "hello"}));
        if (scenario.ref == "queue-rollback") {
            EXPECT_FALSE(event.publication);
            EXPECT_FALSE(event.actions.front().accepted);
            for (const auto &entry : result.trace)
                EXPECT_FALSE(entry.delivery);
            continue;
        }
        const std::vector expected{queued, sending,
                                   scenario.ref == "other-job" ? sending : delivered};
        for (std::size_t i = 0; i < expected.size(); ++i) {
            ASSERT_TRUE(result.trace[i].delivery);
            EXPECT_EQ(result.trace[i].delivery->queued, queued);
            EXPECT_EQ(result.trace[i].delivery->latest, expected[i]);
        }
    }
}

TEST(Rules005, S9RefundUsesActualCappedCredit) {
    const auto original = root("original-donation");
    auto in = donation(original, false);
    const auto refund = root("refund-receipt");
    add_root(in, refund);
    const auto config = validate_rule_configuration({"refund-config", {}, {}});
    auto retained = retain(
        refund.event, config, Kind::refund, "refund",
        {PaymentEvidence::confirmed, RuleMoney{RuleMoneyField::source_amount, {"-5", "EUR"}}},
        HistoricalValuation{
            "refund-identity", {"-5", "EUR"}, "EUR", "1", {}, "identity-snapshot", {}, {}, {}, {}},
        {}, "modifier-empty", 2);
    retained.receipt.facts.payment_reference = in.captures.front().receipt.facts.payment_reference;
    in.captures.push_back(retained);
    turn(in, "qualified-refund",
         RefundTurn{refund.event,
                    key("award", "donation-time"),
                    original.event.event,
                    {Exact(20), Exact(0), Exact(5)},
                    {0, 0},
                    RefundDeficitPolicy::StopAtZero,
                    {TimerLifecycle::Active, CountdownPhase::Main, 100'000, 120'000, false}},
         {102, 102ms});
    const auto scenario = replay("S9", std::move(in));
    const auto result = inspect_simulation(scenario, limits);
    expect_complete(result, scenario);
    ASSERT_EQ(result.trace.size(), 2U);
    expect_donation(result, false);
    const auto &trace = result.trace.back();
    EXPECT_EQ(trace.refund_receipt, retained.receipt);
    ASSERT_TRUE(trace.refund);
    EXPECT_EQ(trace.refund->total.cumulative_fraction, Exact(1) / 4);
    EXPECT_EQ(trace.refund->total.requested_increment, -75'000);
    EXPECT_EQ(trace.refund->requested_main_adjustment_ms, -75'000);
    EXPECT_EQ(trace.refund->requested_debt_restoration_ms, 0);
    ASSERT_TRUE(trace.reversal);
    EXPECT_EQ(trace.reversal->policy, RefundDeficitPolicy::StopAtZero);
    EXPECT_EQ(trace.reversal->disposition, TimeReversalDisposition::Applied);
    EXPECT_EQ(trace.reversal->main_adjustment_ms, -75'000);
    EXPECT_EQ(trace.reversal->debt_added_ms, 0);
    EXPECT_EQ(trace.reversal->unapplied_adjustment_ms, 0);
    EXPECT_EQ(result.final_state.timer.main_remaining_ms, 25'000);
    EXPECT_EQ(result.final_state.time_debt_ms, 0);
}

TEST(Rules005, S10BoundsAndUnavailableHistory) {
    auto bounded = limits;
    const auto scenario = messaging();
    bounded.turns = 3;
    const auto exact = inspect_simulation(scenario, bounded);
    expect_complete(exact, scenario);
    ASSERT_EQ(exact.trace.size(), 3U);
    ASSERT_TRUE(exact.trace.back().delivery);
    EXPECT_EQ(exact.trace.back().delivery->latest, delivered);
    bounded.turns = 2;
    const auto held = inspect_simulation(scenario, bounded);
    EXPECT_EQ(held.completion, Completion::resource_hold);
    EXPECT_TRUE(held.simulated);
    EXPECT_EQ(held.owner_callbacks, 0U);
    EXPECT_TRUE(held.trace.empty());
    EXPECT_EQ(held.final_state.timer.main_remaining_ms, 1000);

    auto in = inputs(1000);
    const auto r = root("unavailable");
    in.roots.push_back({r, Unavailable{}});
    const auto cfg = validate_rule_configuration(
        {"available-capture",
         {},
         {named("available-rule", "available", {time("would-apply", "17")})}});
    in.captures.push_back(retain(r.event, cfg, Kind::named_event, "named_event",
                                 {RuleText{RuleTextField::event_name, "available"}}));
    turn(in, "attempt-unavailable-root", event(r.event));
    const auto missing = inspect_simulation(replay("missing-history", std::move(in)), limits);
    EXPECT_EQ(missing.completion, Completion::unavailable);
    ASSERT_EQ(missing.trace.size(), 1U);
    const auto &attempt = missing.trace.front();
    EXPECT_EQ(attempt.open_refusal, OpenRefusal::history_unavailable);
    EXPECT_FALSE(attempt.committed_children);
    EXPECT_FALSE(attempt.publication);
    EXPECT_FALSE(attempt.cutoff_notice_ref);
    EXPECT_TRUE(attempt.actions.empty());
    EXPECT_EQ(missing.final_state.timer.main_remaining_ms, 1000);
}

TEST(Rules005, S11DebtCapAndCheckedOverflowRollback) {
    const auto cfg = validate_rule_configuration(
        {"debt-config", {}, {named("debt", "debt", {time("pay-debt", "20")})}});
    const auto r = root("debt");
    auto in = inputs(100, 105, 7);
    add_root(in, r);
    in.captures.push_back(retain(r.event, cfg, Kind::named_event, "named_event",
                                 {RuleText{RuleTextField::event_name, "debt"}}));
    turn(in, "pay-debt", event(r.event));
    const auto scenario = replay("S11-debt", std::move(in));
    const auto result = inspect_simulation(scenario, limits);
    expect_complete(result, scenario);
    ASSERT_EQ(result.trace.size(), 1U);
    ASSERT_EQ(result.trace.front().actions.size(), 1U);
    expect_actual_time(result.trace.front().actions.front(), key("debt", "pay-debt"),
                       {20, 5, -7, 8, 105});
    EXPECT_EQ(result.final_state.timer.main_remaining_ms, 105);
    EXPECT_EQ(result.final_state.time_debt_ms, 0);

    constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
    const auto overflow_config = validate_rule_configuration(
        {"overflow-config",
         {},
         {named("overflow", "overflow", {time("stage-one", "1"), time("try-two", "2")})}});
    auto uncapped = inputs(maximum - 2);
    const auto overflowing = root("overflow");
    add_root(uncapped, overflowing);
    uncapped.captures.push_back(retain(overflowing.event, overflow_config, Kind::named_event,
                                       "named_event",
                                       {RuleText{RuleTextField::event_name, "overflow"}}));
    turn(uncapped, "overflow-unit", event(overflowing.event));
    const auto overflow_case = replay("S11-overflow", std::move(uncapped));
    const auto held = inspect_simulation(overflow_case, limits);
    expect_complete(held, overflow_case);
    ASSERT_EQ(held.trace.size(), 1U);
    const auto &trace = held.trace.front();
    EXPECT_EQ(trace.pass_failure, RulePassFailure::numeric_hold);
    ASSERT_EQ(trace.actions.size(), 2U);
    EXPECT_EQ(trace.actions[0].intent.key, key("overflow", "stage-one"));
    EXPECT_EQ(trace.actions[1].intent.key, key("overflow", "try-two", 1));
    EXPECT_TRUE(trace.actions[0].submitted);
    EXPECT_TRUE(trace.actions[1].submitted);
    ASSERT_TRUE(trace.actions[0].proposed);
    EXPECT_EQ(std::get<RuleTimeObservation>(*trace.actions[0].proposed),
              (RuleTimeObservation{1, 1, 0, 0, maximum - 1}));
    EXPECT_FALSE(trace.actions[1].proposed);
    for (const auto &action : trace.actions)
        EXPECT_FALSE(action.accepted);
    EXPECT_FALSE(trace.publication);
    EXPECT_FALSE(trace.delivery);
    EXPECT_FALSE(trace.cutoff_notice_ref);
    EXPECT_EQ(held.final_state.timer.main_remaining_ms, maximum - 2);
}
} // namespace
