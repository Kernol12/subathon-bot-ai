// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <gtest/gtest.h>
#include <subathon/server/rule_simulator.hpp>

namespace rules_005 {
using namespace subathon::core;
using namespace subathon::core::rule_chain;
using namespace subathon::server;
using namespace subathon::server::rule_simulator;
using namespace std::chrono_literals;

inline constexpr Limits limits{
    40, 4 * 1024 * 1024, 4096, {{64, 1024}, 8, 1024, 4 * 1024 * 1024}, {1 << 20, 1 << 20, 65536},
    {}};
using Kind = RuleTriggerKind;
using Disposition = RuleSelectionDisposition;

inline RuleActionKey key(std::string rule, std::string action, std::size_t position = 0) {
    return {std::move(rule), "v1", std::move(action), "v1", position};
}

inline RuleActionDefinition time(std::string id, std::string amount) {
    return {
        std::move(id), "v1",
        RuleDeltaAction{RuleDeltaUnit::milliseconds, "Main", RuleFixedDelta{std::move(amount)}}};
}

inline RuleActionDefinition score(std::string id, std::string amount) {
    return {std::move(id), "v1",
            RuleDeltaAction{RuleDeltaUnit::micro_points, "T", RuleFixedDelta{std::move(amount)}}};
}

inline RuleActionDefinition emit(std::string id, std::string name) {
    return {std::move(id), "v1", RuleNamedEvent{std::move(name), {}}};
}

inline RuleActionDefinition message() {
    return {"chat", "v1", RuleChatAction{"X", RuleChatSender::bot, "T", {}}};
}

inline RulePredicate text(RuleTextField field, std::string value) {
    return RuleComparison<RuleText, RuleEquality>{{field, std::move(value)}, RuleEquality::equal};
}

inline RuleDefinition rule(std::string ref, Kind trigger, std::vector<RuleActionDefinition> actions,
                           RuleCondition condition = {}, std::int64_t priority = 0,
                           std::optional<std::string> group = {},
                           std::vector<std::string> excludes = {}) {
    return {ref,
            "v1",
            ref,
            {},
            true,
            priority,
            std::move(group),
            std::move(excludes),
            {},
            {{trigger, std::move(condition)}},
            std::move(actions)};
}

inline RuleDefinition named(std::string ref, std::string name,
                            std::vector<RuleActionDefinition> actions) {
    return rule(std::move(ref), Kind::named_event, std::move(actions),
                {text(RuleTextField::event_name, std::move(name))});
}

inline RootBinding root(std::string id = "R") {
    return {{{"I", id}, id, {}, "capture-" + id, {}}, "normalized-" + id};
}

inline EventBinding child(const RootBinding &source, const EventBinding &parent, std::string id,
                          ChainCause cause) {
    return {source.event.chain, id, parent.event, "capture-" + id, std::move(cause)};
}

inline ContributionState contribution(std::optional<ReceiptMoney> money = {},
                                      std::optional<std::string> resolution = {}) {
    const bool monetary = money.has_value();
    const bool missing_rate = monetary && !resolution;
    return {monetary ? PaymentEvidence::confirmed : PaymentEvidence::not_applicable,
            {{monetary, std::move(money), std::move(resolution), missing_rate},
             ContributionReview::not_required,
             {EvidencePolicy::require_evidence_review, "eligible-policy", {}},
             ContributionApplication::not_applied,
             ContributionReversal::none,
             SettlementGate::clear}};
}

inline EventCapture retain(const EventBinding &binding, const RuleConfiguration &config, Kind kind,
                           std::string event_type, std::vector<RuleCapturedFact> facts,
                           std::optional<HistoricalValuation> valuation = {},
                           std::optional<ReceiptMoney> missing_rate_money = {},
                           std::string modifier_context = "modifier-empty",
                           std::uint64_t receipt_id = 1) {
    const auto money = valuation ? std::optional{valuation->original_money} : missing_rate_money;
    auto eligibility =
        contribution(money, valuation ? std::optional{valuation->resolution_ref} : std::nullopt);
    const auto state = valuation ? PlanValuationState::resolved
                       : money   ? PlanValuationState::missing_historical
                                 : PlanValuationState::not_applicable;
    PlanEvent receipt{
        binding.event,
        binding.parent,
        binding.chain.root,
        ReceiptId{receipt_id},
        {{"synthetic", "account-I"}, "normalized", binding.event},
        {std::move(event_type), 1, {}, {}, {}, money, {}, {}},
        {0, "synthetic-connection", config.data().configuration_ref, std::move(modifier_context)},
        ContributionId{receipt_id}};
    return {binding,
            config,
            kind,
            std::move(receipt),
            std::move(eligibility),
            {state, std::move(valuation)},
            std::move(facts),
            std::vector<std::string>(config.data().rules.size(), "author"),
            {},
            {},
            {},
            {},
            {}};
}

inline ScenarioInputs inputs(std::int64_t main, std::optional<std::int64_t> cap = {},
                             std::int64_t debt = 0) {
    ScenarioInputs value;
    value.initial.timer = {TimerLifecycle::Active, CountdownPhase::Main, main, 120'000, false};
    value.initial.time_debt_ms = debt;
    value.initial.main_cap_ms = cap;
    value.initial.state_revision = "S1";
    return value;
}

inline void add_root(ScenarioInputs &in, const RootBinding &binding) {
    in.roots.push_back({binding, VerifiedNew{"admit-" + binding.event.event}});
}

inline EventTurn event(const EventBinding &binding,
                       RuleCompletion completion = RuleCompletion::committed) {
    return {binding, completion, {}, {}, {}, {}};
}

inline void turn(ScenarioInputs &in, std::string ref, TurnInput input, ClockSample sample = {}) {
    in.turns.push_back({std::move(ref), sample, std::move(input)});
}

inline Scenario replay(std::string ref, ScenarioInputs in) {
    auto capture = "scenario-capture-" + ref;
    return {std::move(ref), 1,
            CapturedReplay{std::move(capture), CaptureSelection::exact, std::move(in), {}}};
}

inline void expect_complete(const SimulationResult &result, const Scenario &scenario) {
    EXPECT_TRUE(result.simulated);
    EXPECT_EQ(result.scenario, scenario.ref);
    EXPECT_EQ(result.mode, std::holds_alternative<CapturedReplay>(scenario.input)
                               ? Mode::captured_replay
                               : Mode::hypothetical);
    EXPECT_EQ(result.completion, Completion::complete);
}

inline void expect_actual_time(const ActionTrace &action, const RuleActionKey &expected_key,
                               const RuleTimeObservation &expected) {
    EXPECT_EQ(action.intent.key, expected_key);
    EXPECT_TRUE(action.submitted);
    ASSERT_TRUE(action.accepted);
    const auto &effect = std::get<RuleEffectObservation>(*action.accepted);
    EXPECT_EQ(std::get<RuleTimeObservation>(effect.change), expected);
}

inline void expect_cutoff(const Cutoff &actual, const Cutoff &expected) {
    EXPECT_EQ(actual.chain, expected.chain);
    EXPECT_EQ(actual.root_capture, expected.root_capture);
    EXPECT_EQ(actual.last, expected.last);
    EXPECT_EQ(actual.limits.maximum_depth, expected.limits.maximum_depth);
    EXPECT_EQ(actual.limits.maximum_generated_events, expected.limits.maximum_generated_events);
    EXPECT_EQ(actual.reason, expected.reason);
}
} // namespace rules_005
