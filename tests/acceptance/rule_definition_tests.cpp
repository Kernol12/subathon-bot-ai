// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/rate_snapshots_vectors.hpp"
#include <gtest/gtest.h>
#include <iterator>
#include <limits>
#include <source_location>
#include <subathon/core/rule_definition.hpp>
#include <type_traits>
#include <utility>

namespace {
using namespace subathon::core;
using Error = RuleDefinitionError;
using Kind = RuleTriggerKind;
using Truth = RuleTruth;
using Results = std::vector<std::optional<Truth>>;

static_assert(!std::is_constructible_v<RuleConfiguration, RuleConfigurationInput>);
static_assert(std::is_copy_constructible_v<RuleConfiguration>);
static_assert(std::is_same_v<decltype(std::declval<RuleConfiguration &>().data()),
                             const RuleConfigurationInput &>);

RuleDefinition rule(std::string ref = "rule", Kind kind = Kind::named_event) {
    return {.rule_ref = std::move(ref),
            .version_ref = "v1",
            .name = "Rule",
            .description = {},
            .exclusive_group_ref = std::nullopt,
            .excluded_rule_refs = {},
            .common_condition = {},
            .triggers = {{kind, {}}},
            .actions = {}};
}

RuleConfigurationInput configuration(Kind kind = Kind::named_event, RuleCondition condition = {}) {
    RuleConfigurationInput input{"config-v1", {}, {rule("rule", kind)}};
    input.rules.front().triggers.front().condition = std::move(condition);
    return input;
}

template <typename Fact, typename Operator> RuleConditionNode leaf(Fact fact, Operator op) {
    return RulePredicate{RuleComparison<Fact, Operator>{std::move(fact), op}};
}

RuleConditionNode provider() {
    return leaf(RuleText{RuleTextField::provider, "Twitch"}, RuleEquality::equal);
}

RuleConditionNode gifts(std::uint64_t minimum) {
    return leaf(RuleCount{RuleCountField::gift_count, minimum}, RuleOrder::at_least);
}

RuleConditionNode money(RuleOrder op, std::string decimal, std::string currency) {
    return leaf(RuleMoney{RuleMoneyField::source_amount, {std::move(decimal), std::move(currency)}},
                op);
}

RuleCapturedInput capture(Kind kind, std::span<const RuleCapturedFact> facts = {},
                          const EffectiveRateSnapshot *rates = nullptr) {
    return {"event", "config-v1", kind,
            facts,   rates,       rates ? std::optional{rates->data().snapshot_ref} : std::nullopt};
}

// The positive and refusal helpers both fail at the feature call on the NI marker.
// Nonfatal expectations keep subsequent independent rows reachable.
void matches(const RuleConfigurationInput &input, const RuleCapturedInput &captured,
             const Results &expected, std::span<const RuleExplicitValuation> valuations = {},
             std::source_location where = std::source_location::current()) {
    SCOPED_TRACE(testing::Message() << where.file_name() << ':' << where.line());
    EXPECT_NO_THROW(EXPECT_EQ(evaluate_rule_conditions(input, captured, valuations), expected));
}

void accepts(const RuleConfigurationInput &input,
             std::source_location where = std::source_location::current()) {
    SCOPED_TRACE(testing::Message() << where.file_name() << ':' << where.line());
    EXPECT_NO_THROW(static_cast<void>(validate_rule_configuration(input)));
}

template <typename Call> void rejects(Error expected, Call call, std::source_location where) {
    SCOPED_TRACE(testing::Message() << where.file_name() << ':' << where.line());
    EXPECT_THROW(
        {
            try {
                static_cast<void>(call());
            } catch (const RuleDefinitionFailure &error) {
                EXPECT_EQ(error.code(), expected);
                throw;
            }
        },
        RuleDefinitionFailure);
}

void rejects_definition(const RuleConfigurationInput &input, Error expected,
                        std::source_location where = std::source_location::current()) {
    rejects(expected, [&] { return validate_rule_configuration(input); }, where);
}

void rejects_evaluation(const RuleConfigurationInput &input, const RuleCapturedInput &captured,
                        Error expected, std::span<const RuleExplicitValuation> valuations = {},
                        std::source_location where = std::source_location::current()) {
    rejects(expected, [&] { return evaluate_rule_conditions(input, captured, valuations); }, where);
}

RuleActionDefinition chat(std::string action_ref = "chat", std::string version_ref = "A-v1") {
    return {std::move(action_ref), std::move(version_ref),
            RuleChatAction{"route", RuleChatSender::bot, "template-v1", std::nullopt}};
}

RuleActionDefinition delta(RuleDeltaUnit unit, std::string original) {
    return {"delta", "B-v1", RuleDeltaAction{unit, "target", RuleFixedDelta{std::move(original)}}};
}

RuleExplicitValuation valuation() {
    return {"resolution", "event", {RuleMoneyField::source_amount, {"12.000", "USD"}},
            "EUR",        "3",     "4"};
}

EffectiveRateSnapshot arrival_snapshot() {
    // EUR is the anchor. The third quote is retained surplus evidence, with no
    // selected constraint to JPY. USD -> GBP crosses EUR at 2 / (6/5).
    return make_effective_rate_snapshot(
        {"arrival",
         std::nullopt,
         std::nullopt,
         {{{"obs", "rates", "cfg", "request", "evidence", std::nullopt, std::nullopt, {}, {}},
           {{"EUR", "USD", "1.20"}, {"EUR", "GBP", "2"}, {"USD", "JPY", "100"}}}},
         {},
         {FetchedRateConstraint{"obs", 1}, FetchedRateConstraint{"obs", 2}},
         {{{{"EUR", "1", "1"}, {"GBP", "2", "1"}, {"USD", "6", "5"}}}}});
}

TEST(RuleDefinitionAcceptance, CapturesTwoVersionsAndOwnsCopiesInUserOrder) {
    const auto owned_v1 = [] {
        auto v1 = configuration();
        auto &first = v1.rules.front();
        first.name = "Original name";
        first.description = "Original description";
        first.priority = -11;
        first.exclusive_group_ref = "group";
        first.excluded_rule_refs = {"other"};
        first.actions = {chat(), delta(RuleDeltaUnit::milliseconds, "-1.2500")};
        v1.rules.push_back(rule("other"));
        v1.rules.back().enabled = false;
        v1.rules.back().priority = 900;

        const auto captured_v1 = validate_rule_configuration(v1);
        auto v2 = v1;
        v2.configuration_ref = "config-v2";
        v2.generation.maximum_generated_events = 64;
        v2.rules.front().version_ref = "v2";
        auto &actions = v2.rules.front().actions;
        std::swap(actions[0], actions[1]);
        actions[1].version_ref = "A-v2";
        std::get<RuleChatAction>(actions[1].payload).template_version_ref = "template-v2";

        const auto captured_v2 = validate_rule_configuration(v2);
        const auto &later = captured_v2.data();
        EXPECT_EQ(later.configuration_ref, "config-v2");
        EXPECT_EQ(later.generation.maximum_depth, 32U);
        EXPECT_EQ(later.generation.maximum_generated_events, 64U);
        EXPECT_EQ(later.rules.at(0).version_ref, "v2");
        const auto &later_actions = later.rules.at(0).actions;
        EXPECT_EQ(later_actions.at(0).action_ref, "delta");
        EXPECT_EQ(later_actions.at(0).version_ref, "B-v1");
        EXPECT_EQ(later_actions.at(1).action_ref, "chat");
        EXPECT_EQ(later_actions.at(1).version_ref, "A-v2");
        EXPECT_EQ(std::get<RuleChatAction>(later_actions.at(1).payload).template_version_ref,
                  "template-v2");

        v1.rules.front().name.clear();
        std::get<RuleChatAction>(v1.rules.front().actions[0].payload).template_version_ref =
            "caller mutation";
        v1.rules.clear();
        return RuleConfiguration(captured_v1);
    }();

    const auto &original = owned_v1.data();
    EXPECT_EQ(original.configuration_ref, "config-v1");
    EXPECT_EQ(original.generation.maximum_depth, 32U);
    EXPECT_EQ(original.generation.maximum_generated_events, 256U);
    ASSERT_EQ(original.rules.size(), 2U);
    const auto &first = original.rules.front();
    EXPECT_EQ(first.rule_ref, "rule");
    EXPECT_EQ(first.version_ref, "v1");
    EXPECT_EQ(first.name, "Original name");
    EXPECT_EQ(first.description, "Original description");
    EXPECT_TRUE(first.enabled);
    EXPECT_EQ(first.priority, -11);
    EXPECT_EQ(first.exclusive_group_ref, "group");
    EXPECT_EQ(first.excluded_rule_refs, (std::vector<std::string>{"other"}));
    EXPECT_EQ(original.rules.back().rule_ref, "other");
    EXPECT_FALSE(original.rules.back().enabled);
    EXPECT_EQ(original.rules.back().priority, 900);
    ASSERT_EQ(first.actions.size(), 2U);
    EXPECT_EQ(first.actions[0].action_ref, "chat");
    EXPECT_EQ(first.actions[0].version_ref, "A-v1");
    EXPECT_EQ(std::get<RuleChatAction>(first.actions[0].payload).template_version_ref,
              "template-v1");
    EXPECT_EQ(first.actions[1].action_ref, "delta");
    EXPECT_EQ(first.actions[1].version_ref, "B-v1");
    EXPECT_EQ(std::get<RuleFixedDelta>(std::get<RuleDeltaAction>(first.actions[1].payload).delta)
                  .original_decimal,
              "-1.2500");
}

TEST(RuleDefinitionAcceptance, UsesCapturedConfigurationRefAndReturnsRuleOrder) {
    auto old = configuration(Kind::named_event);
    old.rules.push_back(rule("disabled"));
    old.rules.back().enabled = false;
    old.rules.back().priority = 100;
    old.rules.push_back(rule("wrong-kind", Kind::schedule));
    old.rules.push_back(rule("matches-too"));
    const auto captured = capture(Kind::named_event);
    matches(old, captured, {Truth::match, std::nullopt, Truth::no_match, Truth::match});

    auto later = old;
    later.configuration_ref = "config-v2";
    later.rules.front().triggers.front().kind = Kind::schedule;
    rejects_evaluation(later, captured, Error::invalid_capture);
    auto later_capture = captured;
    later_capture.configuration_ref = "config-v2";
    matches(later, later_capture, {Truth::no_match, std::nullopt, Truth::no_match, Truth::match});
    matches(old, captured, {Truth::match, std::nullopt, Truth::no_match, Truth::match});
}

TEST(RuleDefinitionAcceptance, CompleteActionIdentityRequiresIdenticalTypedPayload) {
    auto input = configuration();
    input.rules.push_back(rule("second"));
    input.rules[0].actions = {chat()};
    input.rules[1].actions = {chat()};

    accepts(input);

    auto &second = input.rules[1].actions.front();
    std::get<RuleChatAction>(second.payload).template_version_ref = "template-v2";
    rejects_definition(input, Error::invalid_action);

    second.version_ref = "A-v2";
    accepts(input);

    second.action_ref = "independent-chat";
    second.version_ref = "A-v1";
    accepts(input);

    second = chat();
    second.payload = RuleAnnouncement{"template-v1"};
    rejects_definition(input, Error::invalid_action);
}

TEST(RuleDefinitionAcceptance, ValidatesTypedActionParameters) {
    const RuleActionPayload valid[] = {
        RuleDeltaAction{RuleDeltaUnit::micro_points, "score", RuleCalculation{"calc-v1"}},
        RuleActivateModifier{"modifier-v1", ModifierTriggerTiming::include_trigger},
        RuleActivateModifier{"modifier-v1", ModifierTriggerTiming::subsequent_only},
        RuleExtendModifier{"modifier", 1},
        RuleTerminateModifier{"modifier"},
        RuleChatAction{"route", RuleChatSender::configured_default, "template-v1", "fallback-v1"},
        RuleAnnouncement{"presentation-v1"},
        RuleAnimation{"animation-v1"},
        RuleTimerControl{"timer", "command-v1"},
        RuleNamedEvent{"event-name", std::nullopt},
        RuleConnectorAction{"connection", "operation", "parameters-v1"},
    };
    auto input = configuration();
    for (const auto &payload : valid) {
        SCOPED_TRACE(payload.index());
        input.rules.front().actions = {{"action", "v1", payload}};
        accepts(input);
    }

    const RuleActionPayload invalid[] = {
        RuleDeltaAction{RuleDeltaUnit::micro_points, "score", RuleCalculation{""}},
        RuleActivateModifier{"modifier-v1", ModifierTriggerTiming::unspecified},
        RuleExtendModifier{"modifier", 0},
        RuleTerminateModifier{""},
        RuleChatAction{"route", static_cast<RuleChatSender>(99), "template-v1", std::nullopt},
        RuleChatAction{"route", RuleChatSender::configured_default, "template-v1", ""},
        RuleAnnouncement{""},
        RuleAnimation{""},
        RuleTimerControl{"timer", ""},
        RuleNamedEvent{"event-name", ""},
        RuleConnectorAction{"connection", "", "parameters-v1"},
    };
    input.rules.front().enabled = false;
    for (const auto &payload : invalid) {
        SCOPED_TRACE(payload.index());
        input.rules.front().actions = {{"action", "v1", payload}};
        rejects_definition(input, Error::invalid_action);
    }
}

TEST(RuleDefinitionAcceptance, FixedDeltasPreserveSignedExactSpellingAndRequireIntegralCounts) {
    struct Row {
        RuleDeltaUnit unit;
        const char *spelling;
    };
    const Row valid[] = {
        {RuleDeltaUnit::whole_count, "1.0"},     {RuleDeltaUnit::whole_count, "1e2"},
        {RuleDeltaUnit::whole_count, "-2.00"},   {RuleDeltaUnit::whole_count, "-0.000"},
        {RuleDeltaUnit::milliseconds, "-0.125"}, {RuleDeltaUnit::micro_points, "1e-2"},
        {RuleDeltaUnit::milliseconds, "1e256"}};
    auto input = configuration();
    for (const auto &row : valid) {
        SCOPED_TRACE(row.spelling);
        input.rules.front().actions = {delta(row.unit, row.spelling)};
        EXPECT_NO_THROW({
            const auto owned = validate_rule_configuration(input);
            const auto &action =
                std::get<RuleDeltaAction>(owned.data().rules.at(0).actions.at(0).payload);
            EXPECT_EQ(action.unit, row.unit);
            EXPECT_EQ(std::get<RuleFixedDelta>(action.delta).original_decimal, row.spelling);
        });
    }

    for (const char *spelling : {"1e-1", "1.25", "+1", "1/2", "1.0junk"}) {
        SCOPED_TRACE(spelling);
        input.rules.front().actions = {delta(RuleDeltaUnit::whole_count, spelling)};
        rejects_definition(input, Error::invalid_action);
    }
    input.rules.front().actions = {delta(RuleDeltaUnit::whole_count, "1e257")};
    rejects_definition(input, Error::resource_limit);
}

TEST(RuleDefinitionAcceptance, ValidatesDefinitionIdentityAndGenerationLimits) {
    accepts({"empty-config", {}, {}});
    auto input = configuration();
    input.generation.maximum_depth = 0;
    rejects_definition(input, Error::invalid_generation_limits);

    input = configuration();
    input.generation.maximum_generated_events = 0;
    rejects_definition(input, Error::invalid_generation_limits);

    input = configuration();
    input.configuration_ref.clear();
    rejects_definition(input, Error::invalid_definition);

    input = configuration();
    input.rules.push_back(input.rules.front());
    rejects_definition(input, Error::invalid_definition);

    input = configuration();
    input.rules.front().name.clear();
    rejects_definition(input, Error::invalid_definition);

    input = configuration();
    input.rules.front().triggers.clear();
    rejects_definition(input, Error::invalid_definition);

    input = configuration();
    input.rules.front().actions = {chat(), chat("chat", "A-v2")};
    rejects_definition(input, Error::invalid_action);
}

TEST(RuleConditionAcceptance, SubscriptionOrGiftUsesTypedBranchFacts) {
    auto input = configuration(Kind::subscription);
    input.rules.front().common_condition = {provider()};
    input.rules.front().triggers.push_back({Kind::gift_bundle, {gifts(25)}});
    const RuleCapturedFact twitch[] = {RuleText{RuleTextField::provider, "Twitch"}};
    const RuleCapturedFact gift25[] = {twitch[0], RuleCount{RuleCountField::gift_count, 25}};
    const RuleCapturedFact gift24[] = {twitch[0], RuleCount{RuleCountField::gift_count, 24}};
    matches(input, capture(Kind::subscription, twitch), {Truth::match});
    matches(input, capture(Kind::gift_bundle, gift25), {Truth::match});
    matches(input, capture(Kind::gift_bundle, gift24), {Truth::no_match});
    matches(input, capture(Kind::gift_bundle, twitch), {Truth::unknown});

    auto invalid_common = input;
    invalid_common.rules.front().common_condition = {RuleAny{{1, 2}}, provider(), gifts(25)};
    rejects_evaluation(invalid_common, capture(Kind::subscription, twitch), Error::invalid_field);

    input.rules.front().triggers.front().condition = {gifts(25)};
    rejects_evaluation(input, capture(Kind::gift_bundle, gift25), Error::invalid_field);
}

TEST(RuleConditionAcceptance, SameKindAlternativesYieldOneResultAndShareCommonCondition) {
    auto input = configuration(Kind::gift_bundle, {gifts(25)});
    input.rules.front().triggers.push_back({Kind::gift_bundle, {gifts(50)}});
    input.rules.front().common_condition = {provider()};
    const auto gift50 = RuleCount{RuleCountField::gift_count, 50};
    const RuleCapturedFact twitch[] = {gift50, RuleText{RuleTextField::provider, "Twitch"}};
    const RuleCapturedFact other[] = {gift50, RuleText{RuleTextField::provider, "twitch"}};
    const RuleCapturedFact unknown[] = {gift50};
    matches(input, capture(Kind::gift_bundle, twitch), {Truth::match});
    matches(input, capture(Kind::gift_bundle, other), {Truth::no_match});
    matches(input, capture(Kind::gift_bundle, unknown), {Truth::unknown});
}

TEST(RuleConditionAcceptance, AppliesKleeneLogicWithEitherChildOrder) {
    const auto paused = leaf(RuleFlag{RuleFlagField::paused, true}, RuleEquality::equal);
    struct Row {
        RuleCondition condition;
        Truth expected;
    };
    const Row rows[] = {{{RuleAll{{1, 2}}, paused, provider()}, Truth::no_match},
                        {{RuleAll{{2, 1}}, paused, provider()}, Truth::no_match},
                        {{RuleAny{{1, 2}}, paused, provider()}, Truth::unknown},
                        {{RuleNot{1}, provider()}, Truth::unknown},
                        {{paused}, Truth::no_match}};
    const RuleCapturedFact off[] = {RuleFlag{RuleFlagField::paused, false}};
    for (const auto &row : rows) {
        matches(configuration(Kind::named_event, row.condition), capture(Kind::named_event, off),
                {row.expected});
    }

    const RuleCapturedFact on[] = {RuleFlag{RuleFlagField::paused, true}};
    matches(configuration(Kind::named_event, {RuleAny{{1, 2}}, paused, provider()}),
            capture(Kind::named_event, on), {Truth::match});
    matches(configuration(Kind::named_event, {RuleAny{{2, 1}}, paused, provider()}),
            capture(Kind::named_event, on), {Truth::match});
    matches(configuration(Kind::named_event, {RuleAll{{1, 2}}, paused, provider()}),
            capture(Kind::named_event, on), {Truth::unknown});
    matches(configuration(Kind::named_event, {paused}), capture(Kind::named_event),
            {Truth::unknown});
}

TEST(RuleConditionAcceptance, DoesNotInferSubscriptionDetailsChatFlagsOrPaymentEvidence) {
    const auto subscription = configuration(
        Kind::subscription,
        {leaf(RuleText{RuleTextField::subscription_classification, "Prime"}, RuleEquality::equal)});
    matches(subscription, capture(Kind::subscription), {Truth::unknown});

    const auto chat_input = configuration(
        Kind::chat_command,
        {leaf(RuleFlag{RuleFlagField::chat_unsubscribed, true}, RuleEquality::equal)});
    const RuleCapturedFact nonsubscriber[] = {RuleFlag{RuleFlagField::chat_subscriber, false}};
    matches(chat_input, capture(Kind::chat_command, nonsubscriber), {Truth::unknown});

    const auto payment = configuration(Kind::payment_received,
                                       {RulePredicate{RuleEnumComparison<PaymentEvidence>{
                                           PaymentEvidence::not_applicable, RuleEquality::equal}}});
    const RuleCapturedFact not_applicable[] = {PaymentEvidence::not_applicable};
    matches(payment, capture(Kind::payment_received), {Truth::unknown});
    matches(payment, capture(Kind::payment_received, not_applicable), {Truth::match});

    const auto unknown_operand =
        configuration(Kind::payment_received, {RulePredicate{RuleEnumComparison<PaymentEvidence>{
                                                  PaymentEvidence::unknown, RuleEquality::equal}}});
    rejects_definition(unknown_operand, Error::invalid_field);
}

TEST(RuleConditionAcceptance, ComparesOpaqueTextScopedActorsAndFullRangeIntegers) {
    const auto opaque = std::string("A\0\xff", 3);
    const auto text = configuration(
        Kind::named_event,
        {leaf(RuleText{RuleTextField::actor_display_name, opaque}, RuleEquality::equal)});
    const RuleCapturedFact exact[] = {RuleText{RuleTextField::actor_display_name, opaque}};
    const RuleCapturedFact prefix[] = {RuleText{RuleTextField::actor_display_name, "A"}};
    matches(text, capture(Kind::named_event, exact), {Truth::match});
    matches(text, capture(Kind::named_event, prefix), {Truth::no_match});

    const RuleActor actor{{"Twitch", "account-A"}, "user"};
    const auto actor_input = configuration(
        Kind::named_event, {RulePredicate{RuleActorComparison{actor, RuleEquality::equal}}});
    const RuleCapturedFact same_actor[] = {actor};
    const RuleCapturedFact other_account[] = {RuleActor{{"Twitch", "account-B"}, "user"}};
    matches(actor_input, capture(Kind::named_event, same_actor), {Truth::match});
    matches(actor_input, capture(Kind::named_event, other_account), {Truth::no_match});

    const auto max_count = std::numeric_limits<std::uint64_t>::max();
    const auto counts = configuration(Kind::gift_bundle, {gifts(max_count)});
    const RuleCapturedFact counts_at[] = {RuleCount{RuleCountField::gift_count, max_count}};
    const RuleCapturedFact counts_below[] = {RuleCount{RuleCountField::gift_count, max_count - 1}};
    matches(counts, capture(Kind::gift_bundle, counts_at), {Truth::match});
    matches(counts, capture(Kind::gift_bundle, counts_below), {Truth::no_match});

    const auto minimum = std::numeric_limits<std::int64_t>::min();
    const auto maximum = std::numeric_limits<std::int64_t>::max();
    const auto integers = configuration(
        Kind::manual_adjustment,
        {leaf(RuleInteger{RuleIntegerField::adjustment_ms, maximum}, RuleOrder::less)});
    const RuleCapturedFact min_value[] = {RuleInteger{RuleIntegerField::adjustment_ms, minimum}};
    const RuleCapturedFact max_value[] = {RuleInteger{RuleIntegerField::adjustment_ms, maximum}};
    matches(integers, capture(Kind::manual_adjustment, min_value), {Truth::match});
    matches(integers, capture(Kind::manual_adjustment, max_value), {Truth::no_match});
}

TEST(RuleConditionAcceptance, RejectsMalformedTreesEvenInDisabledRules) {
    const RuleCondition malformed[] = {{RuleAll{{}}},
                                       {RuleAny{{}}},
                                       {RuleNot{1}},
                                       {RuleNot{0}},
                                       {RuleAll{{1, 1}}, provider()},
                                       {provider(), provider()}};
    for (std::size_t row = 0; row < std::size(malformed); ++row) {
        SCOPED_TRACE(row);
        auto input = configuration(Kind::named_event, malformed[row]);
        input.rules.front().enabled = false;
        rejects_definition(input, Error::invalid_condition);
    }

    const auto invalid_op = configuration(
        Kind::gift_bundle,
        {leaf(RuleCount{RuleCountField::gift_count, 25}, static_cast<RuleOrder>(99))});
    rejects_definition(invalid_op, Error::invalid_field);
}

TEST(RuleConditionAcceptance, ValidatesAllFactsAndSnapshotBindingBeforeEvaluation) {
    const auto input = configuration(Kind::subscription);
    const RuleCapturedFact wrong_scope[] = {RuleCount{RuleCountField::gift_count, 1}};
    rejects_evaluation(input, capture(Kind::subscription, wrong_scope), Error::invalid_capture);

    const RuleCapturedFact duplicate[] = {RuleText{RuleTextField::provider, "Twitch"},
                                          RuleText{RuleTextField::provider, "Twitch"}};
    rejects_evaluation(input, capture(Kind::subscription, duplicate), Error::invalid_capture);

    const auto rates = arrival_snapshot();
    const auto good = capture(Kind::subscription, {}, &rates);
    matches(input, good, {Truth::match});

    auto wrong = good;
    wrong.arrival_snapshot_ref = "other-snapshot";
    rejects_evaluation(input, wrong, Error::invalid_capture);
    wrong = good;
    wrong.arrival_snapshot_ref.reset();
    rejects_evaluation(input, wrong, Error::invalid_capture);
    wrong = good;
    wrong.arrival_rates = nullptr;
    rejects_evaluation(input, wrong, Error::invalid_capture);
}

TEST(RuleMoneyAcceptance, UsesExactInverseBeforeRoundingAndKeepsArrivalRates) {
    const auto rates = arrival_snapshot();
    const RuleCapturedFact at[] = {RuleMoney{RuleMoneyField::source_amount, {"12.000", "USD"}}};
    const RuleCapturedFact below[] = {RuleMoney{RuleMoneyField::source_amount, {"11.999", "USD"}}};
    const auto captured = capture(Kind::payment_received, at, &rates);

    // 12 * 5/6 = 10, and 11.999 * 5/6 = 11999/1200 < 12000/1200.
    const auto equal =
        configuration(Kind::payment_received, {money(RuleOrder::equal, "10.00", "EUR")});
    matches(equal, captured, {Truth::match});
    const auto at_least =
        configuration(Kind::payment_received, {money(RuleOrder::at_least, "10.00", "EUR")});
    matches(at_least, captured, {Truth::match});
    const auto greater =
        configuration(Kind::payment_received, {money(RuleOrder::greater, "10.00", "EUR")});
    matches(greater, captured, {Truth::no_match});
    matches(at_least, capture(Kind::payment_received, below, &rates), {Truth::no_match});

    const auto later = make_effective_rate_snapshot(
        {"later",
         std::nullopt,
         std::nullopt,
         {{{"later-obs", "rates", "cfg", "request", "evidence", std::nullopt, std::nullopt, {}, {}},
           {{"USD", "EUR", "0.5"}}}},
         {},
         {FetchedRateConstraint{"later-obs", 1}},
         {{{{"EUR", "1", "1"}, {"USD", "2", "1"}}}}});
    // The separately captured later event would see exactly 6 EUR.
    matches(at_least, capture(Kind::payment_received, at, &later), {Truth::no_match});
    matches(at_least, captured, {Truth::match});
}

TEST(RuleMoneyAcceptance, UsesDirectAndCrossRatesButAnUnselectedQuoteIsNotAPair) {
    const auto rates = arrival_snapshot();
    const RuleCapturedFact eur[] = {RuleMoney{RuleMoneyField::source_amount, {"10", "EUR"}}};
    const RuleCapturedFact usd[] = {RuleMoney{RuleMoneyField::source_amount, {"12.000", "USD"}}};

    // 10 EUR * 6/5 = 12 USD; 12 USD * (2 / (6/5)) = 20 GBP.
    matches(configuration(Kind::payment_received, {money(RuleOrder::equal, "12", "USD")}),
            capture(Kind::payment_received, eur, &rates), {Truth::match});
    matches(configuration(Kind::payment_received, {money(RuleOrder::equal, "20", "GBP")}),
            capture(Kind::payment_received, usd, &rates), {Truth::match});
    matches(configuration(Kind::payment_received, {money(RuleOrder::equal, "1200", "JPY")}),
            capture(Kind::payment_received, usd, &rates), {Truth::unknown});
}

TEST(RuleMoneyAcceptance, ComparesWithWideCrossPairWithoutFinalEffectBounds) {
    const auto rates = make_effective_rate_snapshot(subathon::test::rates::large_snapshot());
    const RuleCapturedFact facts[] = {RuleMoney{RuleMoneyField::source_amount, {"1e-256", "K"}}};
    const auto input =
        configuration(Kind::payment_received, {money(RuleOrder::greater, "1e256", "F")});
    // K -> F = 3^2584 * 5^1764 > 2^(2584+3528) > 10^512.
    // Multiplying by 10^-256 still leaves a value strictly above 10^256.
    matches(input, capture(Kind::payment_received, facts, &rates), {Truth::match});
}

TEST(RuleMoneyAcceptance, SameCurrencyZeroIsKnownWithoutRatesAndMissingMoneyIsUnknown) {
    const RuleCapturedFact zero[] = {RuleMoney{RuleMoneyField::source_amount, {"0", "JPY"}}};
    const auto input = configuration(Kind::payment_received, {money(RuleOrder::equal, "0", "JPY")});
    matches(input, capture(Kind::payment_received, zero), {Truth::match});
    matches(input, capture(Kind::payment_received), {Truth::unknown});

    const RuleCapturedFact negative[] = {
        RuleMoney{RuleMoneyField::source_amount, {"-2.50", "JPY"}}};
    const auto refund = configuration(Kind::refund, {money(RuleOrder::less, "0", "JPY")});
    matches(refund, capture(Kind::refund, negative), {Truth::match});
}

TEST(RuleMoneyAcceptance, ExplicitValuationResolvesOnlyMissingPairForExactInput) {
    const auto empty = make_effective_rate_snapshot(subathon::test::rates::empty_snapshot());
    const auto arrival = arrival_snapshot();
    const auto input =
        configuration(Kind::payment_received, {money(RuleOrder::at_least, "10.00", "EUR")});
    const RuleCapturedFact facts[] = {RuleMoney{RuleMoneyField::source_amount, {"12.000", "USD"}}};
    const auto missing_pair = capture(Kind::payment_received, facts, &empty);
    matches(input, missing_pair, {Truth::unknown});

    const RuleExplicitValuation resolution[] = {valuation()};
    // Explicit 12 * 3/4 = 9 < 10. A matching arrival pair still yields 10.
    matches(input, missing_pair, {Truth::no_match}, resolution);
    matches(configuration(Kind::payment_received, {money(RuleOrder::equal, "9", "EUR")}),
            missing_pair, {Truth::match}, resolution);
    matches(input, capture(Kind::payment_received, facts, &arrival), {Truth::match}, resolution);

    RuleExplicitValuation independent[] = {valuation()};
    independent[0].comparison_currency = "GBP";
    matches(input, missing_pair, {Truth::unknown}, independent);
}

TEST(RuleMoneyAcceptance, RefusesEachInvalidValuationIndependently) {
    const auto input =
        configuration(Kind::payment_received, {money(RuleOrder::at_least, "10.00", "EUR")});
    const RuleCapturedFact facts[] = {RuleMoney{RuleMoneyField::source_amount, {"12.000", "USD"}}};
    const auto captured = capture(Kind::payment_received, facts);
    struct Row {
        const char *name;
        void (*alter)(RuleExplicitValuation &);
        Error expected = Error::invalid_valuation;
    };
    const Row rows[] = {
        {"wrong event", [](auto &value) { value.event_ref = "another-event"; }},
        {"wrong field",
         [](auto &value) { value.original.field = RuleMoneyField::aggregate_amount; }},
        {"different spelling of same value",
         [](auto &value) { value.original.value.original_decimal = "12"; }},
        {"wrong original currency", [](auto &value) { value.original.value.currency = "GBP"; }},
        {"same currency", [](auto &value) { value.comparison_currency = "USD"; }},
        {"zero numerator", [](auto &value) { value.numerator = "0"; }},
        {"zero denominator", [](auto &value) { value.denominator = "0"; }},
        {"noncanonical", [](auto &value) { value.numerator = "03"; }},
        {"unreduced", [](auto &value) { value.denominator = "6"; }},
        {"over 4096 bits", [](auto &value) { value.numerator = std::string(1234, '9'); },
         Error::resource_limit},
    };
    for (const auto &row : rows) {
        SCOPED_TRACE(row.name);
        RuleExplicitValuation bad[] = {valuation()};
        row.alter(bad[0]);
        rejects_evaluation(input, captured, row.expected, bad);
    }

    RuleExplicitValuation duplicate_pair[] = {valuation(), valuation()};
    duplicate_pair[1].resolution_ref = "other-resolution";
    rejects_evaluation(input, captured, Error::invalid_valuation, duplicate_pair);
}

TEST(RuleMoneyAcceptance, ValidatesSurplusValuationsEvenWhenTheirRateCannotAffectResult) {
    const auto rates = arrival_snapshot();
    const auto input =
        configuration(Kind::payment_received, {money(RuleOrder::at_least, "10.00", "EUR")});
    const RuleCapturedFact facts[] = {RuleMoney{RuleMoneyField::source_amount, {"12.000", "USD"}}};
    const auto captured = capture(Kind::payment_received, facts, &rates);

    RuleExplicitValuation surplus[] = {valuation()};
    surplus[0].comparison_currency = "JPY";
    matches(input, captured, {Truth::match}, surplus);
    surplus[0].denominator = "0";
    rejects_evaluation(input, captured, Error::invalid_valuation, surplus);

    surplus[0] = valuation();
    surplus[0].event_ref = "different-event";
    rejects_evaluation(input, captured, Error::invalid_valuation, surplus);
}

TEST(RuleMoneyAcceptance, MapsMoneyGrammarAndHelperResourceRefusals) {
    const auto input = configuration(Kind::payment_received, {money(RuleOrder::equal, "1", "USD")});
    const RuleCapturedFact valid[] = {RuleMoney{RuleMoneyField::source_amount, {"1", "USD"}}};
    matches(input, capture(Kind::payment_received, valid), {Truth::match});

    const auto invalid_threshold =
        configuration(Kind::payment_received, {money(RuleOrder::equal, "01", "USD")});
    rejects_definition(invalid_threshold, Error::invalid_field);

    const RuleCapturedFact malformed[] = {RuleMoney{RuleMoneyField::source_amount, {"1 ", "USD"}}};
    rejects_evaluation(input, capture(Kind::payment_received, malformed), Error::invalid_field);

    const auto missing_currency =
        configuration(Kind::payment_received, {money(RuleOrder::equal, "1", "")});
    rejects_definition(missing_currency, Error::invalid_field);

    const RuleCapturedFact currency_bound[] = {
        RuleMoney{RuleMoneyField::source_amount, {"1", std::string(65, 'U')}}};
    rejects_evaluation(input, capture(Kind::payment_received, currency_bound),
                       Error::resource_limit);

    // This valid 513-byte JSON number exceeds the helper's 512-byte input limit.
    const auto too_long = configuration(
        Kind::payment_received, {money(RuleOrder::equal, "1e" + std::string(511, '0'), "USD")});
    rejects_definition(too_long, Error::resource_limit);
}

TEST(RuleBoundsAcceptance, ChecksDepthOfUnvisitedBranchAtSixteenAndSeventeen) {
    auto input = configuration(Kind::subscription);
    RuleCondition chain;
    for (std::size_t node = 1; node <= 15; ++node) {
        chain.push_back(RuleNot{node});
    }
    chain.push_back(provider());
    input.rules.front().triggers.push_back({Kind::gift_bundle, chain});
    const auto captured = capture(Kind::subscription);
    matches(input, captured, {Truth::match});

    auto &gift = input.rules.front().triggers.back().condition;
    gift.back() = RuleNot{16};
    gift.push_back(provider());
    rejects_evaluation(input, captured, Error::resource_limit);
}

TEST(RuleBoundsAcceptance, CountsNodesAndEdgesInConfigurationListBudget) {
    auto input = configuration();
    auto &first = input.rules.front();
    RuleAll root;
    for (std::size_t index = 1; index <= 2046; ++index) {
        root.children.push_back(index);
    }
    first.common_condition.push_back(std::move(root));
    first.common_condition.resize(2047, provider());
    first.actions = {chat()};
    // 1 rule + 1 trigger + 1 action + 2047 nodes + 2046 edges = 4096.
    accepts(input);
    first.actions.push_back(chat("extra"));
    rejects_definition(input, Error::resource_limit);
}

TEST(RuleBoundsAcceptance, ChecksPerFieldAndAggregateConfigurationBytes) {
    auto input = configuration();
    input.rules.front().description = std::string(4096, 'x');
    accepts(input);
    input.rules.front().description.push_back('x');
    rejects_definition(input, Error::resource_limit);

    // c + 64*(3-byte ref + v1 + Rule) = 577 identity/name bytes.
    // Descriptions account for the remaining 262144 - 577 bytes.
    RuleConfigurationInput full{"c", {}, {}};
    for (int suffix = 100; suffix < 164; ++suffix) {
        auto row = rule(std::to_string(suffix));
        row.description = std::string(4096, 'x');
        full.rules.push_back(std::move(row));
    }
    full.rules.back().description.resize(4096 - 577);
    accepts(full);
    full.rules.back().description.push_back('x');
    rejects_definition(full, Error::resource_limit);
}

TEST(RuleBoundsAcceptance, CountsCapturedFactsAndAllSeparateValuationsTogether) {
    const auto input = configuration(Kind::payment_received);
    const RuleCapturedFact facts[] = {RuleMoney{RuleMoneyField::source_amount, {"12.000", "USD"}}};
    const auto captured = capture(Kind::payment_received, facts);
    std::vector<RuleExplicitValuation> vals;
    for (int index = 0; index < 4096; ++index) {
        auto value = valuation();
        value.resolution_ref = "v" + std::to_string(index);
        value.comparison_currency = "X" + std::to_string(index);
        vals.push_back(std::move(value));
    }

    // 1 fact + 4095 distinct valuation pairs, then one more valuation.
    matches(input, captured, {Truth::match}, std::span{vals}.first(4095));
    rejects_evaluation(input, captured, Error::resource_limit, vals);
}

TEST(RuleBoundsAcceptance, ChecksPerFieldAndAggregateCapturedBytes) {
    const auto input = configuration(Kind::payment_received);
    const RuleCapturedFact facts[] = {RuleMoney{RuleMoneyField::source_amount, {"12.000", "USD"}}};
    auto captured = capture(Kind::payment_received, facts);
    captured.event_ref = std::string(4096, 'e');
    matches(input, captured, {Truth::match});
    captured.event_ref.push_back('e');
    rejects_evaluation(input, captured, Error::resource_limit);
    captured.event_ref.pop_back();

    std::vector<RuleExplicitValuation> vals;
    for (int suffix = 100; suffix < 162; ++suffix) {
        auto value = valuation();
        value.event_ref = captured.event_ref;
        value.resolution_ref = "r" + std::to_string(suffix);
        value.comparison_currency = "X" + std::to_string(suffix);
        vals.push_back(std::move(value));
    }
    // Capture bytes: 4096 + 9 + 6 + 3 = 4114. Each of 62 valuations
    // adds 4096 + 4 + 6 + 3 + 4 + 1 + 1 = 4115. Pad the first by 2900.
    vals.front().resolution_ref.append(2900, 'x');
    matches(input, captured, {Truth::match}, vals);
    vals.front().resolution_ref.push_back('x');
    rejects_evaluation(input, captured, Error::resource_limit, vals);
}
} // namespace
