// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <string_view>
#include <subathon/core/application_plan.hpp>
#include <subathon/core/modifier_activation.hpp>
#include <vector>

namespace {
using namespace std::string_view_literals;
using namespace subathon::core;

constexpr std::array payment_matches{"qualifier.payment"sv};
constexpr auto max_ms = std::numeric_limits<std::int64_t>::max();

ModifierDefinition original_definition() {
    return {"modifier.support",
            "definition.v1",
            480000,
            {{ModifierTargetKind::added_time, "timer.main", "qualifier.payment", "2.00"}}};
}

ModifierDefinition updated_definition() {
    return {"modifier.support",
            "definition.v2",
            600000,
            {{ModifierTargetKind::added_time, "timer.main", "qualifier.payment", "0.50"}}};
}

ModifierInstance prior_instance() {
    return {"instance.old", "instance.old.v1",
            std::nullopt,   original_definition(),
            180000,         {ModifierActivationKind::moderator, "moderator.original"},
            std::nullopt};
}

ModifierActivation activation() {
    return {"instance.new",
            "activation.revision",
            {ModifierActivationKind::rule, "rule.support"},
            std::nullopt};
}

ModifierInstance expected_initial(const ModifierDefinition &definition,
                                  const ModifierActivation &request) {
    return {"instance.new", "activation.revision", std::nullopt,   definition,
            600000,         request.source,        request.trigger};
}

void expect_rejected(const ModifierDefinition &definition,
                     const std::optional<ModifierInstance> &active,
                     const ModifierActivation &request, ModifierActivationError error) {
    const auto before = active;
    try {
        (void)propose_modifier_activation(definition, active, request);
        ADD_FAILURE() << "Expected a typed rejection with no activation proposal";
    } catch (const ModifierActivationFailure &failure) {
        EXPECT_EQ(failure.code(), error);
    }
    EXPECT_EQ(active, before);
}

TEST(ModifierActivation, NewInstancePinsDefinitionIdentityAndRevision) {
    const auto definition = updated_definition();
    const auto request = activation();

    const auto result = propose_modifier_activation(definition, std::nullopt, request);

    ASSERT_TRUE(result.revision_to_append);
    EXPECT_EQ(*result.revision_to_append, expected_initial(definition, request));
    EXPECT_EQ(result.superseded_instance_ref, std::nullopt);
    EXPECT_EQ(result.for_trigger, std::nullopt);
}

TEST(ModifierActivation, DefaultExtendAddsTenMinutesToThreeAndKeepsCapturedDefinition) {
    const std::optional active{prior_instance()};
    const auto request = activation();

    const auto result = propose_modifier_activation(updated_definition(), active, request);

    ASSERT_TRUE(result.revision_to_append);
    const ModifierInstance expected{
        "instance.old", "activation.revision", "instance.old.v1", original_definition(),
        780000,         request.source,        std::nullopt};
    EXPECT_EQ(*result.revision_to_append, expected);
    EXPECT_EQ(result.superseded_instance_ref, std::nullopt);
    EXPECT_EQ(active, prior_instance());
}

TEST(ModifierActivation, RestartUsesFullConfiguredDurationAndKeepsCapturedSettings) {
    auto definition = updated_definition();
    definition.reactivation = ModifierReactivation::restart;
    const std::optional active{prior_instance()};
    const auto request = activation();

    const auto result = propose_modifier_activation(definition, active, request);

    ASSERT_TRUE(result.revision_to_append);
    const ModifierInstance expected{
        "instance.old", "activation.revision", "instance.old.v1", original_definition(),
        600000,         request.source,        std::nullopt};
    EXPECT_EQ(*result.revision_to_append, expected);
    EXPECT_EQ(result.superseded_instance_ref, std::nullopt);
    EXPECT_EQ(active, prior_instance());
}

TEST(ModifierActivation, ReplaceSupersedesTheActiveInstanceAndPinsNewSettings) {
    auto definition = updated_definition();
    definition.reactivation = ModifierReactivation::replace;
    const std::optional active{prior_instance()};
    const auto request = activation();

    const auto result = propose_modifier_activation(definition, active, request);

    ASSERT_TRUE(result.revision_to_append);
    EXPECT_EQ(*result.revision_to_append, expected_initial(definition, request));
    EXPECT_EQ(result.superseded_instance_ref, "instance.old");
    EXPECT_EQ(active, prior_instance());
}

TEST(ModifierActivation, IgnoreCreatesNoRevisionOrSupersessionAndKeepsTheExistingTriggerView) {
    auto definition = updated_definition();
    definition.reactivation = ModifierReactivation::ignore;
    const std::optional active{prior_instance()};
    auto request = activation();
    request.trigger = {"event.trigger", ModifierTriggerTiming::include_trigger};

    const auto result = propose_modifier_activation(definition, active, request);

    EXPECT_EQ(result.revision_to_append, std::nullopt);
    EXPECT_EQ(result.superseded_instance_ref, std::nullopt);
    EXPECT_EQ(result.for_trigger, prior_instance());
    EXPECT_EQ(active, prior_instance());
}

TEST(ModifierActivation, IncludeTriggerExposesTheNewRevisionToBothInputs) {
    const auto definition = updated_definition();
    auto request = activation();
    request.trigger = {"event.trigger", ModifierTriggerTiming::include_trigger};

    const auto result = propose_modifier_activation(definition, std::nullopt, request);

    const auto expected = expected_initial(definition, request);
    EXPECT_EQ(result.for_trigger, expected);
    EXPECT_EQ(result.revision_to_append, expected);
}

TEST(ModifierActivation, SubsequentOnlyWithNoPriorInstanceLeavesTheTriggerUnmodified) {
    const auto definition = updated_definition();
    auto request = activation();
    request.trigger = {"event.trigger", ModifierTriggerTiming::subsequent_only};

    const auto result = propose_modifier_activation(definition, std::nullopt, request);

    EXPECT_EQ(result.for_trigger, std::nullopt);
    EXPECT_EQ(result.revision_to_append, expected_initial(definition, request));
}

TEST(ModifierActivation, SubsequentOnlyReplacementPreservesThePreActivationTriggerView) {
    auto definition = updated_definition();
    definition.reactivation = ModifierReactivation::replace;
    auto request = activation();
    request.trigger = {"event.trigger", ModifierTriggerTiming::subsequent_only};

    const auto result = propose_modifier_activation(definition, prior_instance(), request);

    EXPECT_EQ(result.for_trigger, prior_instance());
    EXPECT_EQ(result.revision_to_append, expected_initial(definition, request));
    EXPECT_EQ(result.superseded_instance_ref, "instance.old");
}

TEST(ModifierActivation, IncludingAReplacementExposesOnlyTheNewRevisionToTheTrigger) {
    auto definition = updated_definition();
    definition.reactivation = ModifierReactivation::replace;
    auto request = activation();
    request.trigger = {"event.trigger", ModifierTriggerTiming::include_trigger};

    const auto result = propose_modifier_activation(definition, prior_instance(), request);
    const auto expected = expected_initial(definition, request);
    EXPECT_EQ(result.for_trigger, expected);
    EXPECT_EQ(result.revision_to_append, expected);
    EXPECT_EQ(result.superseded_instance_ref, "instance.old");
}

TEST(ModifierActivation, TriggeringContributionRequiresAnExplicitTimingChoice) {
    auto request = activation();
    request.trigger = {"event.trigger"};
    expect_rejected(updated_definition(), std::nullopt, request,
                    ModifierActivationError::missing_trigger_timing);
}

struct SourceExample {
    const char *name;
    ModifierActivationKind kind;
    const char *ref;
};

class ModifierActivationSources : public testing::TestWithParam<SourceExample> {};

TEST_P(ModifierActivationSources, CapturesTheSuppliedKindAndSourceIdentity) {
    const auto &example = GetParam();
    auto request = activation();
    request.source = {example.kind, example.ref};

    const auto result = propose_modifier_activation(updated_definition(), std::nullopt, request);

    ASSERT_TRUE(result.revision_to_append);
    EXPECT_EQ(result.revision_to_append->source.kind, example.kind);
    EXPECT_EQ(result.revision_to_append->source.source_ref, example.ref);
}

INSTANTIATE_TEST_SUITE_P(
    Sources, ModifierActivationSources,
    testing::Values(SourceExample{"Moderator", ModifierActivationKind::moderator, "actor.17"},
                    SourceExample{"ChatCommand", ModifierActivationKind::chat_command, "chat.4"},
                    SourceExample{"Milestone", ModifierActivationKind::milestone, "source.29"},
                    SourceExample{"Rule", ModifierActivationKind::rule, "rule.8"},
                    SourceExample{"Schedule", ModifierActivationKind::schedule, "source.51"}),
    [](const auto &example_info) { return example_info.param.name; });

struct InvalidFactorExample {
    const char *name;
    const char *factor;
};

class ModifierInvalidFactors : public testing::TestWithParam<InvalidFactorExample> {};

TEST_P(ModifierInvalidFactors, RejectsInvalidFactorInAnyConfiguredTarget) {
    auto definition = updated_definition();
    definition.reactivation = ModifierReactivation::replace;
    definition.targets.push_back(
        {ModifierTargetKind::counter, "counter.gifts", "qualifier.gift", GetParam().factor});

    expect_rejected(definition, prior_instance(), activation(),
                    ModifierActivationError::invalid_factor);
}

INSTANTIATE_TEST_SUITE_P(Factors, ModifierInvalidFactors,
                         testing::Values(InvalidFactorExample{"Negative", "-0.5"},
                                         InvalidFactorExample{"NaN", "NaN"},
                                         InvalidFactorExample{"PositiveInfinity", "Infinity"},
                                         InvalidFactorExample{"NegativeInfinity", "-Infinity"}),
                         [](const auto &example_info) { return example_info.param.name; });

TEST(ModifierActivation, ExtendAcceptsTheExactLimit) {
    auto active = prior_instance();
    active.captured_definition.duration_ms = max_ms;
    active.remaining_ms = max_ms - 600000;

    const auto result = propose_modifier_activation(updated_definition(), active, activation());

    ASSERT_TRUE(result.revision_to_append);
    EXPECT_EQ(result.revision_to_append->remaining_ms, max_ms);
}

TEST(ModifierActivation, ExtendRejectsOneMillisecondBeyondTheLimit) {
    auto active = prior_instance();
    active.captured_definition.duration_ms = max_ms;
    active.remaining_ms = max_ms - 599999;

    expect_rejected(updated_definition(), active, activation(),
                    ModifierActivationError::duration_overflow);
}

TEST(ModifierActivation, TargetQualifiersSelectIndependentlyAndRetainZeroAndExactFractions) {
    auto definition = updated_definition();
    definition.targets = {
        {ModifierTargetKind::added_time, "timer.main", "qualifier.payment", "0.125"},
        {ModifierTargetKind::milestone_score, "score.main", "qualifier.cheer", "3"},
        {ModifierTargetKind::counter, "counter.gifts", "qualifier.gift", "1.25"},
        {ModifierTargetKind::counter, "counter.all", std::nullopt, "0"}};
    constexpr std::array matched{"qualifier.payment"sv, "qualifier.gift"sv};

    const auto result = propose_modifier_activation(definition, std::nullopt, activation());
    ASSERT_TRUE(result.revision_to_append);
    const auto selected = select_modifier_targets(*result.revision_to_append, matched);

    const std::array expected{
        ModifierTargetSelection{
            {ModifierTargetKind::added_time, "timer.main", "qualifier.payment", "0.125"},
            "definition.v2",
            "activation.revision",
            Exact(1) / 8},
        ModifierTargetSelection{
            {ModifierTargetKind::counter, "counter.gifts", "qualifier.gift", "1.25"},
            "definition.v2",
            "activation.revision",
            Exact(5) / 4},
        ModifierTargetSelection{{ModifierTargetKind::counter, "counter.all", std::nullopt, "0"},
                                "definition.v2",
                                "activation.revision",
                                Exact(0)}};
    EXPECT_TRUE(
        std::is_permutation(selected.begin(), selected.end(), expected.begin(), expected.end()));
}

TEST(ModifierActivation, NoMatchDoesNotSelectAnIndependentlyQualifiedTarget) {
    const auto selected = select_modifier_targets(prior_instance(), {});

    EXPECT_TRUE(selected.empty());
}

TEST(ModifierActivation, ScoreOnlyMatchDoesNotLeakToTimeOrAnotherCounter) {
    auto active = prior_instance();
    active.captured_definition.targets.push_back(
        {ModifierTargetKind::milestone_score, "score.main", "qualifier.cheer", "0.50"});
    active.captured_definition.targets.push_back(
        {ModifierTargetKind::counter, "counter.gifts", "qualifier.gift", "3"});
    constexpr std::array matched{"qualifier.cheer"sv};

    const auto selected = select_modifier_targets(active, matched);

    const std::vector<ModifierTargetSelection> expected{
        {{ModifierTargetKind::milestone_score, "score.main", "qualifier.cheer", "0.50"},
         "definition.v1",
         "instance.old.v1",
         Exact(1) / 2}};
    EXPECT_EQ(selected, expected);
}

class NoRules final : public RulePlanSource {
    std::vector<RuleSelection> capture(const PlanEvent &) override { return {}; }
};

// One already resolved target feeds the existing capture boundary in this example.
class CapturedModifier final : public ModifierPlanSource {
  public:
    void set(const ModifierTargetSelection &selected) {
        selection_ = {selected.definition_revision_ref,
                      selected.instance_revision_ref,
                      selected.target.target_ref,
                      "group.one",
                      "multiply",
                      "multiply",
                      selected.target.factor_decimal};
    }

  private:
    std::vector<ModifierSelection> capture(const PlanEvent &) override { return {selection_}; }
    ModifierSelection selection_;
};

PlanCapture capture_input(const ModifierTargetSelection &selected, std::uint64_t receipt_id,
                          std::string_view event, std::string_view context) {
    PlanCapture capture{};
    capture.event.event_ref = event;
    capture.event.root_event_ref = event;
    capture.event.receipt_id = ReceiptId{receipt_id};
    capture.event.arrival.modifier_context_ref = context;
    capture.valuation.state = PlanValuationState::not_applicable;
    const Exact exact_ms = Exact(60000) * selected.factor;
    capture.requested_effects.push_back(
        {"action.time", "action.time.v1", "rule.support", selected.target.target_ref,
         RequestedUnit::milliseconds, false, exact_ms, round_effect_units(exact_ms),
         "60 seconds scaled by one captured factor", "reverse-actual"});
    return capture;
}

TEST(ModifierActivation, ReplacementDoesNotRewriteThePendingPlanOrItsHistoricalSelection) {
    const auto active = prior_instance();
    const auto selected_before = select_modifier_targets(active, payment_matches);
    ASSERT_EQ(selected_before.size(), 1U);
    const ModifierSelection v1{"definition.v1", "instance.old.v1", "timer.main", "group.one",
                               "multiply",      "multiply",        "2.00"};
    CapturedModifier modifiers;
    modifiers.set(selected_before.front());
    NoRules rules;
    const auto pending = capture_application_plan(
        "plan.pending", capture_input(selected_before.front(), 1, "event.pending", "arrival.v1"),
        rules, modifiers);

    auto definition = updated_definition();
    definition.reactivation = ModifierReactivation::replace;
    const auto result = propose_modifier_activation(definition, active, activation());
    ASSERT_TRUE(result.revision_to_append);
    const auto selected_after =
        select_modifier_targets(*result.revision_to_append, payment_matches);
    ASSERT_EQ(selected_after.size(), 1U);
    modifiers.set(selected_after.front());
    const auto later = capture_application_plan(
        "plan.later", capture_input(selected_after.front(), 2, "event.later", "arrival.v2"), rules,
        modifiers);

    const ModifierSelection v2{
        "definition.v2", "activation.revision", "timer.main", "group.one", "multiply", "multiply",
        "0.50"};
    ASSERT_EQ(pending.revisions.size(), 1U);
    EXPECT_EQ(pending.revisions.front().modifiers, std::vector{v1});
    EXPECT_EQ(pending.revisions.front().captured.event.arrival.modifier_context_ref, "arrival.v1");
    ASSERT_EQ(pending.revisions.front().captured.requested_effects.size(), 1U);
    EXPECT_EQ(pending.revisions.front().captured.requested_effects.front().exact_requested_units,
              Exact(120000));
    EXPECT_EQ(pending.revisions.front().captured.requested_effects.front().requested_units, 120000);
    ASSERT_EQ(later.revisions.size(), 1U);
    EXPECT_EQ(later.revisions.front().modifiers, std::vector{v2});
    EXPECT_EQ(later.revisions.front().captured.event.arrival.modifier_context_ref, "arrival.v2");
    ASSERT_EQ(later.revisions.front().captured.requested_effects.size(), 1U);
    EXPECT_EQ(later.revisions.front().captured.requested_effects.front().exact_requested_units,
              Exact(30000));
    EXPECT_EQ(later.revisions.front().captured.requested_effects.front().requested_units, 30000);
    const auto historical = select_modifier_targets(active, payment_matches);
    const std::vector<ModifierTargetSelection> expected_historical{
        {{ModifierTargetKind::added_time, "timer.main", "qualifier.payment", "2.00"},
         "definition.v1",
         "instance.old.v1",
         Exact(2)}};
    EXPECT_EQ(historical, expected_historical);
}
} // namespace
