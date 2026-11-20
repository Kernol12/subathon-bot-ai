// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>
#include <limits>
#include <subathon/core/rule_selection.hpp>
#include <utility>

namespace {
using namespace subathon::core;
using Disposition = RuleSelectionDisposition;
using Kind = RuleTriggerKind;
using Error = RuleDefinitionError;

constexpr RuleSelectionDecision selected{Disposition::selected, {}};
constexpr RuleSelectionDecision disabled{Disposition::disabled, {}};
constexpr RuleSelectionDecision no_match{Disposition::condition_no_match, {}};
constexpr RuleSelectionDecision unknown{Disposition::condition_unknown, {}};

RuleSelectionDecision excluded(std::size_t blocker) { return {Disposition::excluded, blocker}; }

RuleSelectionDecision group_loser(std::size_t blocker) {
    return {Disposition::exclusive_group, blocker};
}

RuleDefinition rule(std::string ref, std::int64_t priority, std::optional<std::string> group = {},
                    std::vector<std::string> exclusions = {}, Kind kind = Kind::named_event) {
    return {.rule_ref = std::move(ref),
            .version_ref = "v1",
            .name = "Fixture rule",
            .description = {},
            .priority = priority,
            .exclusive_group_ref = std::move(group),
            .excluded_rule_refs = std::move(exclusions),
            .common_condition = {},
            .triggers = {{kind, {}}},
            .actions = {}};
}

RuleConfiguration configuration(std::vector<RuleDefinition> rules,
                                std::string ref = "configuration-v1") {
    return validate_rule_configuration({std::move(ref), {}, std::move(rules)});
}

RuleConditionNode text_matches(RuleTextField field, std::string expected) {
    return RulePredicate{
        RuleComparison<RuleText, RuleEquality>{{field, std::move(expected)}, RuleEquality::equal}};
}

RuleConditionNode gift_minimum(std::uint64_t minimum) {
    return RulePredicate{RuleComparison<RuleCount, RuleOrder>{{RuleCountField::gift_count, minimum},
                                                              RuleOrder::at_least}};
}

RuleCapturedInput captured(std::span<const RuleCapturedFact> facts = {},
                           Kind kind = Kind::named_event, std::string config = "configuration-v1") {
    return {"selected-event", std::move(config), kind, facts, nullptr, {}};
}

void expect_selection(const RuleConfiguration &rules, const RuleCapturedInput &event,
                      const std::vector<std::size_t> &chosen,
                      const std::vector<RuleSelectionDecision> &decisions,
                      const std::vector<RulePriorityTie> &warnings = {}) {
    const auto result = select_rules(rules, event);
    EXPECT_EQ(result.configuration_ref, event.configuration_ref);
    EXPECT_EQ(result.event_ref, event.event_ref);
    EXPECT_EQ(result.selected_rule_indices, chosen);
    EXPECT_EQ(result.decisions, decisions);
    EXPECT_EQ(result.priority_ties, warnings);
    EXPECT_EQ(validate_rule_selection(rules), warnings);
}

template <class Operation> void expect_error(Error code, Operation run) {
    try {
        static_cast<void>(run());
        ADD_FAILURE() << "Expected a typed rule definition failure";
    } catch (const RuleDefinitionFailure &error) {
        EXPECT_EQ(error.code(), code);
    }
}

TEST(RuleSelectionAcceptance, GiftBonusPreservesIndependentCredit) {
    auto normal = rule("Normal", -1, {}, {}, Kind::gift_bundle);
    auto bonus25 = rule("Bonus25", 25, "Gifts", {}, Kind::gift_bundle);
    auto bonus50 = rule("Bonus50", 50, "Gifts", {}, Kind::gift_bundle);
    bonus25.triggers.front().condition = {gift_minimum(25)};
    bonus50.triggers.front().condition = {gift_minimum(50)};
    const std::vector<RuleCapturedFact> facts{RuleCount{RuleCountField::gift_count, 50}};
    expect_selection(configuration({normal, bonus25, bonus50}), captured(facts, Kind::gift_bundle),
                     {2, 0}, {selected, group_loser(2), selected});
}

TEST(RuleSelectionAcceptance, TiedGroupUsesCapturedOrderAndVersion) {
    const auto z = rule("Z", 0, "Gifts");
    const auto a = rule("A", 0, "Gifts");
    const auto independent = rule("Independent", 0);
    const auto v1 = configuration({z, a, independent});
    const auto v2 = configuration({a, z, independent}, "configuration-v2");
    const auto v1_event = captured();
    const auto v2_event = captured({}, Kind::named_event, "configuration-v2");
    expect_selection(v1, v1_event, {0, 2}, {selected, group_loser(0), selected}, {{{0, 1}}});
    expect_selection(v2, v2_event, {0, 2}, {selected, group_loser(0), selected}, {{{0, 1}}});
    ASSERT_EQ(v1.data().rules[0].rule_ref, "Z");
    ASSERT_EQ(v2.data().rules[0].rule_ref, "A");
    expect_selection(v1, v1_event, {0, 2}, {selected, group_loser(0), selected}, {{{0, 1}}});
    expect_error(Error::invalid_capture, [&] { return select_rules(v2, v1_event); });
}

TEST(RuleSelectionAcceptance, SignedPriorityExtremesRankWithoutSuppressing) {
    constexpr auto min = std::numeric_limits<std::int64_t>::min();
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    expect_selection(configuration({rule("Low", min), rule("Middle", 0), rule("High", max)}),
                     captured(), {2, 1, 0}, {selected, selected, selected});
}

TEST(RuleSelectionAcceptance, OnlySelectedRulesApplyDirectionalExclusions) {
    auto a = rule("A", 30, {}, {"B"});
    const auto b = rule("B", 20, {}, {"C"});
    const auto c = rule("C", 10);
    expect_selection(configuration({a, b, c}), captured(), {0, 2},
                     {selected, excluded(0), selected});
    a.common_condition = {text_matches(RuleTextField::provider, "other-provider")};
    const std::vector<RuleCapturedFact> facts{
        RuleText{RuleTextField::provider, "fixture-provider"}};
    expect_selection(configuration({a, b, c}), captured(facts), {1},
                     {no_match, selected, excluded(1)});
}

TEST(RuleSelectionAcceptance, ExcludedTopMatchLeavesGroupOpen) {
    const auto outside = rule("Outside", 40, {}, {"High"});
    expect_selection(configuration({outside, rule("High", 30, "Gifts"), rule("Low", 20, "Gifts")}),
                     captured(), {0, 2}, {selected, excluded(0), selected});
}

TEST(RuleSelectionAcceptance, GroupLoserAppliesNoExclusions) {
    const auto winner = rule("Winner", 30, "Gifts");
    const auto loser = rule("Loser", 20, "Gifts", {"Independent"});
    expect_selection(configuration({winner, loser, rule("Independent", 10)}), captured(), {0, 2},
                     {selected, group_loser(0), selected});
}

TEST(RuleSelectionAcceptance, UnknownNonmatchingAndDisabledRulesRetainTheirReasons) {
    auto unknown_rule = rule("Unknown", 30, "Gifts", {"Match"});
    unknown_rule.common_condition = {text_matches(RuleTextField::actor_display_name, "missing")};
    auto false_rule = rule("NoMatch", 30, "Gifts", {"Match"});
    false_rule.common_condition = {text_matches(RuleTextField::provider, "other-provider")};
    auto disabled_rule = rule("Disabled", 30, "Gifts", {"Match"});
    disabled_rule.enabled = false;
    const auto match = rule("Match", 10, "Gifts");
    const std::vector<RuleCapturedFact> facts{
        RuleText{RuleTextField::provider, "fixture-provider"}};
    expect_selection(configuration({unknown_rule, false_rule, disabled_rule, match}),
                     captured(facts), {3}, {unknown, no_match, disabled, selected}, {{{0, 1}}});
    const auto outside = rule("Outside", 40, {}, {"Unknown", "NoMatch", "Disabled"});
    expect_selection(configuration({outside, unknown_rule, false_rule, disabled_rule, match}),
                     captured(facts), {0, 4}, {selected, unknown, no_match, disabled, selected},
                     {{{1, 2}}});
}

TEST(RuleSelectionAcceptance, RecordsHighestRankedBlockerAndItsMechanism) {
    auto a = rule("A", 30, "Gifts");
    const auto b = rule("B", 20, {}, {"C"});
    const auto c = rule("C", 10, "Gifts");
    expect_selection(configuration({a, b, c}), captured(), {0, 1},
                     {selected, selected, group_loser(0)});
    a.excluded_rule_refs = {"C"};
    expect_selection(configuration({a, b, c}), captured(), {0, 1},
                     {selected, selected, excluded(0)});
}

TEST(RuleSelectionAcceptance, CapturedTieOrderDeterminesExclusionDirection) {
    const auto z = rule("Z", 0, {}, {"A", "A"});
    const auto a = rule("A", 0, "Gifts");
    const auto forward = configuration({z, a});
    EXPECT_TRUE(validate_rule_selection(forward).empty());
    expect_selection(forward, captured(), {0}, {selected, excluded(0)});
    const auto backward = configuration({a, z}, "configuration-v2");
    expect_error(Error::invalid_exclusion, [&] { return validate_rule_selection(backward); });
    expect_error(Error::invalid_exclusion, [&] {
        return select_rules(backward, captured({}, Kind::named_event, "configuration-v2"));
    });
}

TEST(RuleSelectionAcceptance, RefusesEachInvalidConfiguredEdgeBeforeEnabling) {
    struct Graph {
        const char *name;
        std::vector<RuleDefinition> rules;
    };
    auto disabled_source = rule("Low", 1, {}, {"High"});
    disabled_source.enabled = false;
    auto disabled_target = rule("High", 2);
    disabled_target.enabled = false;
    const std::vector<Graph> graphs{
        {"missing", {rule("A", 2, {}, {"missing"}), rule("B", 1)}},
        {"self", {rule("A", 2, {}, {"A"})}},
        {"backward", {rule("Low", 1, {}, {"High"}), rule("High", 2)}},
        {"cycle", {rule("A", 3, {}, {"B"}), rule("B", 2, {}, {"C"}), rule("C", 1, {}, {"A"})}},
        {"disabled source", {disabled_source, rule("High", 2)}},
        {"disabled target", {rule("Low", 1, {}, {"High"}), disabled_target}},
    };
    for (const auto &[name, definitions] : graphs) {
        SCOPED_TRACE(name);
        const auto graph = configuration(definitions);
        expect_error(Error::invalid_exclusion, [&] { return validate_rule_selection(graph); });
        expect_error(Error::invalid_exclusion, [&] { return select_rules(graph, captured()); });
    }
}

} // namespace
