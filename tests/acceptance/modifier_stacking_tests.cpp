// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <gtest/gtest.h>
#include <limits>
#include <subathon/server/modifier_stacking.hpp>
#include <tuple>

namespace {
using namespace std::string_view_literals;
using namespace subathon::core;
using namespace subathon::server;
using Op = ModifierStackOperator;
using Failure = ModifierStackFailure;

const ModifierTargetKey time_key{ModifierTargetKind::added_time, "timer.main"};
const ModifierTargetKey score_key{ModifierTargetKind::milestone_score, "score.main"};
const ModifierTargetKey count_key{ModifierTargetKind::counter, "counter.synthetic"};
const ModifierTargetKey gifts_key{ModifierTargetKind::counter, "counter.gifts"};
const ModifierTargetKey all_key{ModifierTargetKind::counter, "counter.all"};
constexpr std::array payment{"payment"sv};

// Values from #215's sqlite_modifier_activations_fixture.hpp. The pure runner
// uses the same retained shapes and the shipped Replace producer, without SQL.
const ModifierInstance old_instance{"I",
                                    "j1",
                                    "j0",
                                    {"D",
                                     "z-old",
                                     480000,
                                     {{time_key.kind, time_key.ref, "payment", "2.00"},
                                      {count_key.kind, count_key.ref, {}, "1.250"}}},
                                    180000,
                                    {ModifierActivationKind::moderator, "operator-A"},
                                    {}};
const ModifierDefinition replacement{"D",
                                     "a-new",
                                     600000,
                                     {{time_key.kind, time_key.ref, "payment", "0.50"}},
                                     ModifierReactivation::replace};
const ModifierActivation replace_activation{
    "J",
    "k0",
    {ModifierActivationKind::rule, "rule-B"},
    ModifierTriggeringContribution{"event-B", ModifierTriggerTiming::subsequent_only}};

ModifierInstance new_instance() {
    return propose_modifier_activation(replacement, old_instance, replace_activation)
        .revision_to_append.value();
}

ModifierInstance extra(std::string_view name, std::string factor,
                       const ModifierTargetKey &key = time_key,
                       std::optional<std::string> qualifier = {}) {
    const auto id = "extra/" + std::string(name);
    return {id, id + "/r1",
            {}, {id, id + "/v1", 1, {{key.kind, key.ref, std::move(qualifier), std::move(factor)}}},
            1,  {ModifierActivationKind::moderator, "operator-A"},
            {}};
}

ModifierOccurrenceKey occurrence(const ModifierInstance &instance, std::size_t ordinal = 1) {
    return {instance.instance_ref, instance.revision_ref, instance.captured_definition.revision_ref,
            ordinal};
}

ModifierMemberPolicy member(const ModifierInstance &instance, std::size_t ordinal = 1,
                            std::optional<ModifierPriority> priority = {}) {
    return {occurrence(instance, ordinal), priority};
}

ModifierGroupPolicy group(std::string ref, std::vector<ModifierMemberPolicy> members,
                          std::optional<Op> operation = {},
                          std::optional<ModifierPriority> priority = {}) {
    return {std::move(ref), operation, priority, std::move(members)};
}

QualifiedModifierOccurrence projected(const ModifierInstance &instance, std::size_t ordinal = 1) {
    const auto &row = instance.captured_definition.targets.at(ordinal - 1);
    const auto selected = select_modifier_targets(instance, payment);
    const auto found = std::ranges::find(selected, row, &ModifierTargetSelection::target);
    if (found == selected.end())
        throw std::logic_error("Shipped projection did not select the fixture row");
    return {occurrence(instance, ordinal),
            {row.kind, row.target_ref},
            row.factor_decimal,
            found->factor};
}

ModifierStackPolicy policy(std::vector<ModifierTargetPolicy> targets,
                           std::string config = "cfg-arrival") {
    return {"stacking-policy", std::move(config), std::move(targets)};
}

ModifierBaseEffect base(ModifierTargetKey key, Exact units, std::size_t ordinal = 1) {
    return {ordinal,        "action", "action/v1",      "rule",
            std::move(key), true,     std::move(units), "reverse-actual"};
}

SelectedModifierRevision selected(const ModifierInstance &instance) {
    return {instance.instance_ref, instance.revision_ref, instance.captured_definition.revision_ref,
            &instance, &instance.captured_definition};
}

struct Arrival {
    ReceiptArrival receipt{1000, "connection-A", "cfg-arrival", "modifier-context-A"};
    ModifierTraceProfile admission{"admitted-producer", "stacking-v1"};
    CapturedModifierContext context{
        receipt.modifier_context_ref, receipt.configuration_ref, "stacking-policy", admission, {}};

    ModifierStackCapture capture(const ModifierStackPolicy &p,
                                 std::span<const SelectedModifierRevision> eligible,
                                 std::span<const std::string_view> qualifiers = payment) const {
        return {"arrival-A", receipt, {}, &context, &admission, &p, eligible, qualifiers, {}, {}};
    }
};

using TargetTraces = std::vector<ModifierTargetTrace>;

constexpr ModifierStackLimits fixture_limits{
    .input_bytes = 4 * 1024 * 1024, .output_bytes = 4 * 1024 * 1024, .operations = 65536};

ModifierCombination combine(std::span<const QualifiedModifierOccurrence> operands,
                            const ModifierStackPolicy &p,
                            std::span<const ModifierTargetKey> targets) {
    return combine_modifiers(operands, p, targets, fixture_limits);
}

ModifierStackEvaluation evaluate(const ModifierStackCapture &capture,
                                 std::span<const ModifierBaseEffect> bases,
                                 const ModifierStackLimits &limits = fixture_limits) {
    return evaluate_captured_modifiers(capture, bases, limits);
}

template <class Outcome> void expect_refusal(const Outcome &outcome, Failure failure) {
    const auto *refusal = std::get_if<Failure>(&outcome);
    ASSERT_NE(refusal, nullptr) << "Refusal must carry no partial result";
    EXPECT_EQ(*refusal, failure);
}

const std::array operations{Op::multiply, Op::additive, Op::highest, Op::lowest, Op::priority};
// Pairwise r3 oracles, 2.00 and 3.000 at base 100. Default multiplication is omitted.
const std::array group_units{600, 400, 300, 200, 200};
const std::array target_units{600, 400, 300, 200, 300};

class FrozenOperators : public testing::TestWithParam<std::tuple<unsigned, bool>> {};

TEST_P(FrozenOperators, TwoAndThreeWithIsolatedCounter) {
    const auto [index, at_target] = GetParam();
    const std::optional<Op> operation = index ? std::optional{operations.at(index)} : std::nullopt;
    const auto three = extra("time-three", "3.000");
    const std::array eligible{selected(three),
                              selected(old_instance)}; // Arrival order is irrelevant.
    const auto groups =
        at_target ? std::vector{group("G", {member(old_instance)}, {}, ModifierPriority{1, 1}),
                                group("H", {member(three)}, {}, ModifierPriority{2, 2})}
                  : std::vector{group("G",
                                      {member(old_instance, 1, ModifierPriority{9, 2}),
                                       member(three, 1, ModifierPriority{8, 1})},
                                      operation)};
    const auto p = policy({{time_key, at_target ? operation : std::nullopt, groups},
                           {count_key, {}, {group("count", {member(old_instance, 2)})}}});
    const Arrival arrival;
    const std::array bases{base(time_key, 100), base(count_key, 4, 2)};
    const auto outcome = evaluate(arrival.capture(p, eligible), bases);
    const auto &proposal = std::get<ModifierStackProposal>(outcome);
    ASSERT_EQ(proposal.effects.size(), 2U);
    const int expected = (at_target ? target_units : group_units).at(index);
    EXPECT_EQ(proposal.effects[0].exact_units, Exact(expected));
    EXPECT_EQ(proposal.effects[0].rounded_units, expected);
    EXPECT_EQ(proposal.effects[1].exact_units, Exact(5));
    EXPECT_FALSE(proposal.effects[1].rounded_units);
    ASSERT_EQ(proposal.targets.size(), 2U);
    const auto time = std::ranges::find(proposal.targets, time_key, &ModifierTargetTrace::target);
    ASSERT_NE(time, proposal.targets.end());
    EXPECT_EQ(time->reduction.factor, Exact(expected / 100));
    EXPECT_EQ(time->reduction.operation, at_target ? operations.at(index) : Op::multiply);
    ASSERT_EQ(time->groups.size(), groups.size());
    EXPECT_EQ(time->groups.front().reduction.operation,
              at_target ? Op::multiply : operations.at(index));
}

INSTANTIATE_TEST_SUITE_P(EachLevel, FrozenOperators,
                         testing::Combine(testing::Range(0U, 5U), testing::Bool()));

class OperatorComposition : public testing::TestWithParam<std::tuple<unsigned, unsigned>> {};

// G reduces 2,3; H is always 1/2. Column priority chooses H by target rank.
// All 25 pairs exercise the pure seam with the operated group and a second group.
TEST_P(OperatorComposition, GroupResultIsOneTargetOperand) {
    const auto [inner, outer] = GetParam();
    const auto three = extra("time-three", "3.000");
    const auto half = new_instance();
    const std::array operands{projected(three), projected(half), projected(old_instance)};
    const auto p = policy({{time_key,
                            operations.at(outer),
                            {group("G",
                                   {member(old_instance, 1, ModifierPriority{9, 2}),
                                    member(three, 1, ModifierPriority{8, 1})},
                                   operations.at(inner), ModifierPriority{1, 1}),
                             group("H", {member(half)}, {}, ModifierPriority{2, 2})}}});
    const std::array targets{time_key};
    const auto outcome = combine(operands, p, targets);
    const auto &traces = std::get<TargetTraces>(outcome);
    ASSERT_EQ(traces.size(), 1U);
    ASSERT_EQ(traces.front().groups.size(), 2U);
    constexpr std::array group_factors{6, 4, 3, 2, 2};
    constexpr int target_numerators[][5]{
        {6, 11, 12, 1, 1}, {4, 7, 8, 1, 1}, {3, 5, 6, 1, 1}, {2, 3, 4, 1, 1}, {2, 3, 4, 1, 1}};
    EXPECT_EQ(traces[0].groups[0].reduction.factor, Exact(group_factors.at(inner)));
    EXPECT_EQ(traces[0].groups[1].reduction.factor, Exact(1) / 2);
    EXPECT_EQ(traces[0].reduction.factor, Exact(target_numerators[inner][outer]) / 2);
}

INSTANTIATE_TEST_SUITE_P(AllPairs, OperatorComposition,
                         testing::Combine(testing::Range(0U, 5U), testing::Range(0U, 5U)));

TEST(ModifierCombination, PriorityUsesIndependentSignedRanksAndExplicitTieOrder) {
    constexpr auto low = std::numeric_limits<std::int64_t>::min();
    constexpr auto high = std::numeric_limits<std::int64_t>::max();
    const auto three = extra("time-three", "3.000");
    const auto half = new_instance();
    const std::array operands{projected(half), projected(old_instance), projected(three)};
    auto p = policy({{time_key,
                      Op::priority,
                      {group("G",
                             {member(three, 1, ModifierPriority{low, 2}),
                              member(old_instance, 1, ModifierPriority{low, 1})},
                             Op::priority, ModifierPriority{low, 2}),
                       group("H", {member(half, 1, ModifierPriority{high, 1})}, Op::priority,
                             ModifierPriority{low, 1})}}});
    const std::array targets{time_key};

    const auto tied = std::get<TargetTraces>(combine(operands, p, targets));
    const auto &g = tied.at(0).groups.at(0);
    const auto &h = tied.at(0).groups.at(1);
    EXPECT_EQ(g.reduction.factor, Exact(2));
    EXPECT_EQ(g.members.at(0).priority, (ModifierPriority{low, 2}));
    EXPECT_FALSE(g.members.at(0).winner);
    EXPECT_TRUE(g.members.at(1).winner);
    EXPECT_EQ(h.members.at(0).priority, (ModifierPriority{high, 1}));
    EXPECT_FALSE(g.winner);
    EXPECT_TRUE(h.winner);
    EXPECT_EQ(tied.at(0).reduction.factor, Exact(1) / 2);

    p.targets[0].groups[0].priority->rank = high;
    const auto retargeted = std::get<TargetTraces>(combine(operands, p, targets));
    EXPECT_EQ(retargeted.at(0).reduction.factor, Exact(2));
    EXPECT_EQ(retargeted.at(0).groups.at(0).priority, (ModifierPriority{high, 2}));
    EXPECT_TRUE(retargeted.at(0).groups.at(0).winner);
    EXPECT_FALSE(retargeted.at(0).groups.at(1).winner);
    EXPECT_EQ(tied.at(0).groups.at(0).priority, (ModifierPriority{low, 2}));
    EXPECT_EQ(tied.at(0).reduction.factor, Exact(1) / 2);
}

TEST(ModifierCombination, MissingOrIndistinguishablePriorityRefusesAtEitherLevel) {
    const auto three = extra("time-three", "3.000");
    const std::array operands{projected(old_instance), projected(three)};
    const std::array targets{time_key};
    constexpr ModifierPriority rank{5, 1};
    for (const auto second : {std::optional<ModifierPriority>{}, std::optional{rank}}) {
        const auto member_policy =
            policy({{time_key,
                     {},
                     {group("G", {member(old_instance, 1, rank), member(three, 1, second)},
                            Op::priority)}}});
        const auto group_policy = policy({{time_key,
                                           Op::priority,
                                           {group("G", {member(old_instance)}, {}, rank),
                                            group("H", {member(three)}, {}, second)}}});
        expect_refusal(combine(operands, member_policy, targets), Failure::invalid);
        expect_refusal(combine(operands, group_policy, targets), Failure::invalid);
    }
}

TEST(ModifierCombination, EqualExtremaKeepEveryCoWinnerInCaptureOrderAndExactSpelling) {
    const auto equal = extra("time-equal", "2e0");
    const std::array operands{projected(old_instance), projected(equal)};
    const std::array targets{time_key};
    for (const auto op : {Op::highest, Op::lowest}) {
        SCOPED_TRACE(static_cast<int>(op));
        const auto p =
            policy({{time_key, {}, {group("G", {member(equal), member(old_instance)}, op)}}});
        const auto result = std::get<TargetTraces>(combine(operands, p, targets));
        EXPECT_EQ(result.at(0).reduction.factor, Exact(2));
        const auto &members = result.at(0).groups.at(0).members;
        ASSERT_EQ(members.size(), 2U);
        EXPECT_EQ(members[0].occurrence, operands[1]);
        EXPECT_EQ(members[0].occurrence.original_factor_decimal, "2e0");
        EXPECT_EQ(members[1].occurrence, operands[0]);
        EXPECT_EQ(members[1].occurrence.original_factor_decimal, "2.00");
        EXPECT_TRUE(members[0].winner);
        EXPECT_TRUE(members[1].winner);

        const auto split = policy(
            {{time_key, op, {group("H", {member(equal)}), group("G", {member(old_instance)})}}});
        const auto outer = std::get<TargetTraces>(combine(operands, split, targets));
        EXPECT_EQ(outer.at(0).reduction.factor, Exact(2));
        const auto &groups = outer.at(0).groups;
        ASSERT_EQ(groups.size(), 2U);
        EXPECT_EQ(groups[0].group_ref, "H");
        EXPECT_EQ(groups[1].group_ref, "G");
        EXPECT_EQ(groups[0].members.at(0).occurrence, operands[1]);
        EXPECT_EQ(groups[1].members.at(0).occurrence, operands[0]);
        EXPECT_TRUE(groups[0].winner);
        EXPECT_TRUE(groups[1].winner);
    }
}

TEST(ModifierCombination, AdditiveClampsOnlyAtTheEndOfEachReduction) {
    const auto half = new_instance();
    const auto zero = extra("clamp-zero", "0");
    const auto three = extra("clamp-three", "3");
    const std::array operands{projected(half), projected(zero), projected(three)};
    const std::array targets{time_key};
    const auto p = policy(
        {{time_key,
          Op::additive,
          {group("G", {member(half), member(zero)}, Op::additive), group("H", {member(three)})}}});
    const auto inner = std::get<TargetTraces>(combine(operands, p, targets));
    const auto &zero_group = inner.at(0).groups.at(0);
    EXPECT_EQ(zero_group.reduction.raw, Exact(-1) / 2);
    EXPECT_EQ(zero_group.reduction.factor, Exact(0));
    EXPECT_EQ(zero_group.members.at(0).occurrence.original_factor_decimal, "0.50");
    EXPECT_EQ(zero_group.members.at(1).occurrence.original_factor_decimal, "0");
    EXPECT_EQ(inner.at(0).reduction.raw, Exact(2));
    EXPECT_EQ(inner.at(0).reduction.factor, Exact(2));

    const auto split = policy(
        {{time_key,
          Op::additive,
          {group("G", {member(half)}), group("H", {member(zero)}), group("K", {member(three)})}}});
    const auto full = std::get<TargetTraces>(combine(operands, split, targets));
    EXPECT_EQ(full.at(0).reduction.raw, Exact(3) / 2);
    EXPECT_EQ(full.at(0).reduction.factor, Exact(3) / 2);

    const auto one_group = policy(
        {{time_key, {}, {group("G", {member(half), member(zero), member(three)}, Op::additive)}}});
    const auto combined = std::get<TargetTraces>(combine(operands, one_group, targets));
    EXPECT_EQ(combined.at(0).groups.at(0).reduction.raw, Exact(3) / 2);
    EXPECT_EQ(combined.at(0).reduction.factor, Exact(3) / 2);

    const auto shortened = std::span{operands}.first(2);
    const auto outer = std::get<TargetTraces>(combine(shortened, split, targets));
    EXPECT_EQ(outer.at(0).reduction.raw, Exact(-1) / 2);
    EXPECT_EQ(outer.at(0).reduction.factor, Exact(0));
    EXPECT_EQ(outer.at(0).groups.size(), 2U);
}

TEST(ModifierCombination, ZeroRemainsAnOperandAtBothLevelsIncludingWinningPriority) {
    const auto half = new_instance();
    const auto zero = extra("clamp-zero", "0");
    const std::array operands{projected(half), projected(zero)};
    const std::array targets{time_key};
    constexpr std::array choices{Op::multiply, Op::highest, Op::lowest, Op::priority};
    for (const auto op : choices) {
        SCOPED_TRACE(static_cast<int>(op));
        const auto p = policy({{time_key,
                                {},
                                {group("G",
                                       {member(half, 1, ModifierPriority{1, 1}),
                                        member(zero, 1, ModifierPriority{2, 2})},
                                       op)}}});
        const auto split = policy({{time_key,
                                    op,
                                    {group("G", {member(half)}, {}, ModifierPriority{1, 1}),
                                     group("H", {member(zero)}, {}, ModifierPriority{2, 2})}}});
        const Exact expected = op == Op::highest ? Exact(1) / 2 : Exact(0);
        const auto inner = std::get<TargetTraces>(combine(operands, p, targets));
        const auto outer = std::get<TargetTraces>(combine(operands, split, targets));
        EXPECT_EQ(inner.at(0).reduction.factor, expected);
        EXPECT_EQ(outer.at(0).reduction.factor, expected);
        ASSERT_EQ(inner.at(0).groups.at(0).members.size(), 2U);
        ASSERT_EQ(outer.at(0).groups.size(), 2U);
        if (op == Op::priority || op == Op::lowest) {
            EXPECT_TRUE(inner.at(0).groups.at(0).members.at(1).winner);
            EXPECT_TRUE(outer.at(0).groups.at(1).winner);
        }
    }
}

std::vector<ModifierInstance> powers(const ModifierInstance &half, unsigned count) {
    std::vector instances{half};
    for (const auto &[name, factor] :
         {std::pair{"time-big-", "1e256"}, std::pair{"time-small-", "1e-256"}})
        for (unsigned index = 1; index <= count; ++index)
            instances.push_back(extra(name + std::to_string(index), factor));
    return instances;
}

ModifierStackPolicy power_policy(std::span<const ModifierInstance> instances, bool split) {
    std::vector<ModifierGroupPolicy> groups;
    if (split) {
        for (const auto &instance : instances.subspan(1))
            groups.push_back(group(instance.instance_ref, {member(instance)}));
    } else {
        auto large = group("G", {});
        auto small = group("H", {});
        for (const auto &instance : instances.subspan(1, (instances.size() - 1) / 2))
            large.members.push_back(member(instance));
        for (const auto &instance : instances.subspan((instances.size() + 1) / 2))
            small.members.push_back(member(instance));
        groups = {std::move(large), std::move(small)};
    }
    groups.push_back(group("K", {member(instances.front())}));
    return policy({{time_key, {}, std::move(groups)}});
}

std::vector<SelectedModifierRevision> selected_all(std::span<const ModifierInstance> instances) {
    std::vector<SelectedModifierRevision> revisions;
    for (const auto &instance : instances)
        revisions.push_back(selected(instance));
    return revisions;
}

class ProductBound : public testing::TestWithParam<bool> {};

TEST_P(ProductBound, FourthTermFitsAndFifthRefusesBeforeCancellation) {
    const auto half = new_instance();
    const auto four = powers(half, 4);
    const auto five = powers(half, 5);
    const Arrival arrival;
    const std::array bases{base(time_key, 3)};
    const auto p = power_policy(four, GetParam());
    const auto four_selected = selected_all(four);
    const auto good =
        std::get<ModifierStackProposal>(evaluate(arrival.capture(p, four_selected), bases));
    EXPECT_EQ(good.targets.at(0).reduction.factor, Exact(1) / 2);
    EXPECT_EQ(good.effects.at(0).exact_units, Exact(3) / 2);
    EXPECT_EQ(good.effects.at(0).rounded_units, 2);
    if (!GetParam()) {
        const Exact ten_to_1024(boost::multiprecision::cpp_int("1" + std::string(1024, '0')));
        EXPECT_EQ(good.targets.at(0).groups.at(0).reduction.factor, ten_to_1024);
        EXPECT_EQ(good.targets.at(0).groups.at(1).reduction.factor, Exact(1) / ten_to_1024);
    }

    const auto cancellation = power_policy(five, GetParam());
    const auto five_selected = selected_all(five);
    // 10^1280 needs 4253 bits. Cancellation after the fifth term cannot make it fit.
    expect_refusal(evaluate(arrival.capture(cancellation, five_selected), bases),
                   Failure::resource_limit);
    const std::array zero_base{base(time_key, 0)};
    expect_refusal(evaluate(arrival.capture(cancellation, five_selected), zero_base),
                   Failure::resource_limit);
}

INSTANTIATE_TEST_SUITE_P(GroupAndTarget, ProductBound, testing::Bool());

TEST(ModifierCombination, BoundAppliesToLosingGroup) {
    const auto five = powers(new_instance(), 5);
    auto p = power_policy(five, false);
    p.targets[0].operation = Op::priority;
    auto &groups = p.targets[0].groups;
    groups[0].priority = ModifierPriority{1, 1};
    groups[1].priority = ModifierPriority{2, 2};
    groups[2].priority = ModifierPriority{3, 3}; // K would win with a half.
    std::vector<QualifiedModifierOccurrence> operands;
    for (const auto &instance : five)
        operands.push_back(projected(instance));
    const std::array targets{time_key};
    expect_refusal(combine(operands, p, targets), Failure::resource_limit);
}

TEST(CapturedModifiers, SignedFinalRoundingAndScalingRefuseWholeUnitOnOverflow) {
    const auto four = powers(new_instance(), 4);
    const auto p = power_policy(four, false);
    const auto revisions = selected_all(four);
    const Arrival arrival;
    const auto capture = arrival.capture(p, revisions);
    const Exact twice_max_plus_one(boost::multiprecision::cpp_int("18446744073709551615"));
    constexpr auto min = std::numeric_limits<std::int64_t>::min();
    const std::array valid_bases{base(time_key, -twice_max_plus_one)};
    const auto valid = std::get<ModifierStackProposal>(evaluate(capture, valid_bases));
    EXPECT_EQ(valid.effects.at(0).exact_units, -twice_max_plus_one / 2);
    EXPECT_EQ(valid.effects.at(0).rounded_units, min);
    EXPECT_EQ(round_effect_units(valid.effects.at(0).exact_units), min);

    const std::array overflow_bases{base(time_key, 3), base(time_key, twice_max_plus_one, 2)};
    EXPECT_THROW((void)round_effect_units(twice_max_plus_one / 2), std::overflow_error);
    expect_refusal(evaluate(capture, overflow_bases), Failure::numeric_overflow);

    // The factor and base fit separately. Scaling 10^1024 by 10^256 does not fit.
    const auto large = std::span{four}.subspan(1, 4);
    auto large_group = group("G", {});
    for (const auto &instance : large)
        large_group.members.push_back(member(instance));
    const auto scaling = policy({{time_key, {}, {large_group}}});
    const auto large_revisions = selected_all(large);
    const std::array large_bases{base(time_key, parse_decimal("1e256").value())};
    expect_refusal(evaluate(arrival.capture(scaling, large_revisions), large_bases),
                   Failure::resource_limit);

    const auto zero = extra("zero", "0");
    const std::array zero_selected{selected(zero)};
    const auto zero_policy = policy({{time_key, {}, {group("zero", {member(zero)})}}});
    const Exact out_of_bounds(boost::multiprecision::cpp_int(1) << 4096);
    const std::array oversized_bases{base(time_key, out_of_bounds)};
    expect_refusal(evaluate(arrival.capture(zero_policy, zero_selected), oversized_bases),
                   Failure::resource_limit);
}

TEST(CapturedModifiers, QualifierProjectionKeepsDistinctTargetsAndHalfEvenRequests) {
    // The starting definition and activation inputs are the named #215 projection fixture.
    const ModifierDefinition definition{
        "modifier.support",
        "definition.v2",
        600000,
        {{time_key.kind, time_key.ref, "qualifier.payment", "0.125"},
         {score_key.kind, score_key.ref, "qualifier.cheer", "3"},
         {gifts_key.kind, gifts_key.ref, "qualifier.gift", "1.25"},
         {all_key.kind, all_key.ref, {}, "0"}}};
    const auto instance =
        propose_modifier_activation(definition, {},
                                    {"instance.new",
                                     "activation.revision",
                                     {ModifierActivationKind::rule, "rule.support"},
                                     {}})
            .revision_to_append.value();
    constexpr std::array ordinary{"qualifier.payment"sv, "qualifier.gift"sv};
    constexpr std::array cheer{"qualifier.payment"sv, "qualifier.gift"sv, "qualifier.cheer"sv};
    const auto projected_rows = select_modifier_targets(instance, ordinary);
    ASSERT_EQ(projected_rows.size(), 3U);
    const std::array expected_sources{definition.targets[0], definition.targets[2],
                                      definition.targets[3]};
    EXPECT_TRUE(std::ranges::is_permutation(projected_rows, expected_sources,
                                            std::ranges::equal_to{},
                                            &ModifierTargetSelection::target));

    const auto score_half = extra("score-half", "0.50", score_key);
    for (const bool with_cheer : {false, true}) {
        SCOPED_TRACE(with_cheer);
        auto score_group = group("score", {member(instance, 2)});
        if (with_cheer)
            score_group.members.push_back(member(score_half));
        const auto p = policy({{time_key, {}, {group("time", {member(instance)})}},
                               {score_key, {}, {score_group}},
                               {gifts_key, {}, {group("gifts", {member(instance, 3)})}},
                               {all_key, {}, {group("all", {member(instance, 4)})}}});
        const std::array revisions{selected(instance), selected(score_half)};
        const auto eligible = std::span{revisions}.first(with_cheer ? 2 : 1);
        const auto matches = with_cheer ? std::span<const std::string_view>{cheer}
                                        : std::span<const std::string_view>{ordinary};
        const std::array bases{base(time_key, 12), base(gifts_key, 4, 2), base(all_key, 9, 3),
                               base(score_key, 3, 4)};
        const Arrival arrival;
        const auto result =
            std::get<ModifierStackProposal>(evaluate(arrival.capture(p, eligible, matches), bases));
        ASSERT_EQ(result.effects.size(), bases.size());
        for (std::size_t i = 0; i < bases.size(); ++i)
            EXPECT_EQ(result.effects[i].base, bases[i]);
        EXPECT_EQ(result.effects[0].exact_units, Exact(3) / 2);
        EXPECT_EQ(result.effects[0].rounded_units, 2);
        EXPECT_EQ(round_effect_units(result.effects[0].exact_units), 2);
        EXPECT_EQ(result.effects[1].exact_units, Exact(5));
        EXPECT_FALSE(result.effects[1].rounded_units);
        EXPECT_EQ(result.effects[2].exact_units, Exact(0));
        EXPECT_FALSE(result.effects[2].rounded_units);
        EXPECT_EQ(result.effects[3].exact_units, with_cheer ? Exact(9) / 2 : Exact(3));
        EXPECT_EQ(result.effects[3].rounded_units, with_cheer ? 4 : 3);
        EXPECT_EQ(round_effect_units(result.effects[3].exact_units), with_cheer ? 4 : 3);
        const auto score =
            std::ranges::find(result.targets, score_key, &ModifierTargetTrace::target);
        ASSERT_NE(score, result.targets.end());
        EXPECT_EQ(score->reduction.factor, with_cheer ? Exact(3) / 2 : Exact(1));
        EXPECT_EQ(score->groups.size(), with_cheer ? 1U : 0U);
    }
}

TEST(CapturedModifiers, SameReferenceDifferentKindAndFractionalCountRemainIndependent) {
    const ModifierTargetKey time_with_count_ref{ModifierTargetKind::added_time, count_key.ref};
    const auto p = policy({{count_key, {}, {group("count", {member(old_instance, 2)})}}});
    const std::array eligible{selected(old_instance)};
    const std::array bases{base(count_key, 1), base(time_with_count_ref, -7, 2)};
    const Arrival arrival;
    const std::array lower_case_miss{"Payment"sv};
    const auto proposal = std::get<ModifierStackProposal>(
        evaluate(arrival.capture(p, eligible, lower_case_miss), bases));
    ASSERT_EQ(proposal.effects.size(), 2U);
    EXPECT_EQ(proposal.effects[0].exact_units, Exact(5) / 4);
    EXPECT_EQ(proposal.effects[0].base.target, count_key);
    EXPECT_FALSE(proposal.effects[0].rounded_units);
    EXPECT_EQ(proposal.effects[1].exact_units, Exact(-7));
    EXPECT_EQ(proposal.effects[1].rounded_units, -7);
    ASSERT_EQ(proposal.selection.size(), 1U);
    EXPECT_EQ(proposal.selection[0].original_factor_decimal, "1.250");
    EXPECT_EQ(proposal.links, (std::vector{ModifierTraceLink{1, occurrence(old_instance, 2)}}));
}

TEST(CapturedModifiers, KnownEmptyAndZeroHaveDifferentTraceAndUnresolvedHasNoIdentity) {
    const Arrival arrival;
    const auto p = policy({});
    const std::array bases{base(time_key, 7)};
    auto capture = arrival.capture(p, {});
    const auto result = std::get<ModifierStackProposal>(evaluate(capture, bases));
    EXPECT_EQ(result.effects.at(0).exact_units, Exact(7));
    EXPECT_EQ(result.targets.at(0).reduction.factor, Exact(1));
    EXPECT_TRUE(result.targets.at(0).groups.empty());
    EXPECT_TRUE(result.selection.empty());
    EXPECT_TRUE(result.links.empty());

    auto unresolved = capture;
    unresolved.eligible.reset();
    expect_refusal(evaluate(unresolved, bases), Failure::unresolved);
    unresolved = capture;
    unresolved.policy = nullptr;
    expect_refusal(evaluate(unresolved, bases), Failure::unresolved);
    unresolved = capture;
    unresolved.context = nullptr;
    expect_refusal(evaluate(unresolved, bases), Failure::unresolved);

    const auto zero = extra("zero", "0");
    const auto zero_policy = policy({{time_key, {}, {group("zero", {member(zero)})}}});
    const std::array eligible{selected(zero)};
    const auto selected_zero =
        std::get<ModifierStackProposal>(evaluate(arrival.capture(zero_policy, eligible), bases));
    EXPECT_EQ(selected_zero.effects.at(0).exact_units, Exact(0));
    EXPECT_EQ(selected_zero.effects.at(0).rounded_units, 0);
    EXPECT_EQ(selected_zero.links, (std::vector{ModifierTraceLink{1, occurrence(zero)}}));
    const auto no_actions =
        std::get<ModifierStackProposal>(evaluate(arrival.capture(zero_policy, eligible), {}));
    EXPECT_TRUE(no_actions.effects.empty());

    expect_refusal(evaluate(arrival.capture(p, eligible), bases), Failure::invalid);
}

TEST(CapturedModifiers, ValidateEvenAnUnmatchedFactorBeforeFiltering) {
    auto invalid = new_instance();
    invalid.captured_definition.targets.push_back(
        {score_key.kind, score_key.ref, "never-matched", "-1"});
    EXPECT_THROW((void)select_modifier_targets(invalid, payment), ModifierActivationFailure);
    const auto p = policy({{time_key, {}, {group("time", {member(invalid)})}}});
    const auto zero = extra("zero", "0", count_key);
    const std::array eligible{selected(zero), selected(invalid)};
    const auto p_with_zero =
        policy({p.targets.front(), {count_key, {}, {group("zero", {member(zero)})}}});
    const std::array bases{base(count_key, 4), base(time_key, 0, 2)};
    const Arrival arrival;
    expect_refusal(evaluate(arrival.capture(p_with_zero, eligible), bases), Failure::invalid);
}

TEST(CapturedModifiers, PendingArrivalKeepsItsSelectionAndPolicyAfterSubsequentReplace) {
    const auto activation =
        propose_modifier_activation(replacement, old_instance, replace_activation);
    ASSERT_EQ(activation.for_trigger, old_instance);
    ASSERT_TRUE(activation.revision_to_append);
    ASSERT_EQ(activation.superseded_instance_ref, "I");
    const auto &current = *activation.revision_to_append;
    ASSERT_EQ(current.revision_ref, "k0");
    ASSERT_EQ(current.captured_definition.revision_ref, "a-new");
    const auto pending_three = extra("pending-three", "3.00", time_key, "payment");
    const std::array pending_revisions{selected(*activation.for_trigger), selected(pending_three)};
    const std::array current_revisions{selected(current), selected(pending_three)};
    Arrival pending;
    pending.receipt = {1002, "pending-A", "cfg-pending-A", "ctx-pending-A"};
    pending.context.configuration_ref = pending.receipt.configuration_ref;
    pending.context.document_ref = pending.receipt.modifier_context_ref;
    const auto pending_policy =
        policy({{time_key, {}, {group("time", {member(old_instance), member(pending_three)})}},
                {count_key, {}, {group("count", {member(old_instance, 2)})}}},
               "cfg-pending-A");
    const std::array pending_bases{base(time_key, 60000), base(count_key, 4, 2)};
    auto capture = pending.capture(pending_policy, pending_revisions);
    capture.original_arrival_ref = "pending-A";

    Arrival fresh;
    fresh.receipt = {1003, "new-B", "cfg-new-B", "ctx-new-B"};
    fresh.context.configuration_ref = fresh.receipt.configuration_ref;
    fresh.context.document_ref = fresh.receipt.modifier_context_ref;
    const auto fresh_policy = policy(
        {{time_key, {}, {group("time", {member(current), member(pending_three)}, Op::lowest)}}},
        "cfg-new-B");
    const std::array fresh_bases{base(time_key, 60000)};
    auto fresh_capture = fresh.capture(fresh_policy, current_revisions);
    fresh_capture.original_arrival_ref = "new-B";

    const auto later = std::get<ModifierStackProposal>(evaluate(fresh_capture, fresh_bases));
    ASSERT_EQ(later.effects.size(), 1U);
    EXPECT_EQ(later.effects[0].exact_units, Exact(30000));
    EXPECT_EQ(later.effects[0].rounded_units, 30000);
    EXPECT_EQ(later.context_ref, "ctx-new-B");

    const auto original = std::get<ModifierStackProposal>(evaluate(capture, pending_bases));
    ASSERT_EQ(original.effects.size(), 2U);
    EXPECT_EQ(original.effects[0].exact_units, Exact(360000));
    EXPECT_EQ(original.effects[0].rounded_units, 360000);
    EXPECT_EQ(original.effects[1].exact_units, Exact(5));
    EXPECT_FALSE(original.effects[1].rounded_units);
    EXPECT_EQ(original.context_ref, "ctx-pending-A");
    EXPECT_NE(
        std::ranges::find(original.links, occurrence(old_instance), &ModifierTraceLink::occurrence),
        original.links.end());
    EXPECT_EQ(
        std::ranges::find(original.links, occurrence(current), &ModifierTraceLink::occurrence),
        original.links.end());

    auto corrupted = capture;
    corrupted.policy = &fresh_policy;
    expect_refusal(evaluate(corrupted, pending_bases), Failure::invalid);
    corrupted = capture;
    corrupted.policy = nullptr;
    expect_refusal(evaluate(corrupted, pending_bases), Failure::unresolved);

    auto wrong = pending_revisions;
    corrupted = capture;
    corrupted.eligible = wrong;
    wrong[0].definition_revision_ref = "a-new";
    expect_refusal(evaluate(corrupted, pending_bases), Failure::invalid);
    wrong[0].definition_revision_ref = "z-old";
    auto different_definition = old_instance.captured_definition;
    different_definition.targets[0].factor_decimal = "3.00";
    wrong[0].definition = &different_definition;
    expect_refusal(evaluate(corrupted, pending_bases), Failure::invalid);
    wrong[0].instance = &current;
    expect_refusal(evaluate(corrupted, pending_bases), Failure::invalid);
    wrong[0].instance = nullptr;
    expect_refusal(evaluate(corrupted, pending_bases), Failure::unresolved);
}

TEST(CapturedModifiers, IndependentLaterAwardRoundsBeforeTheApplicationCap) {
    const auto current = new_instance();
    const std::array eligible{selected(current)};
    const auto p = policy({{time_key, {}, {group("time", {member(current)})}}});
    const std::array bases{base(time_key, 60001)};
    const Arrival arrival;
    const auto result =
        std::get<ModifierStackProposal>(evaluate(arrival.capture(p, eligible), bases));
    ASSERT_EQ(result.effects.size(), 1U);
    EXPECT_EQ(result.effects[0].exact_units, Exact(60001) / 2);
    EXPECT_EQ(result.effects[0].rounded_units, 30000);
    EXPECT_EQ(round_effect_units(result.effects[0].exact_units), 30000);
    EXPECT_EQ(result.effects[0].exact_units - result.effects[0].rounded_units.value(),
              Exact(1) / 2);
}

TEST(CapturedModifiers, RecalculationBindsDecisionProfileAndOriginalOrdinals) {
    // Original plan-A/1's arrival is from the retained fixture. The retained
    // plan and its opaque explanation belong to the historical reader.
    ReceiptArrival original_arrival{1000, "arrival-A", "cfg-arrival-A", "modifier-context-A"};
    const auto original_binding = original_arrival;

    // Target 1 is skipped. Targets 2 and 4 have identical values and qualifiers.
    auto instance = extra("repeated", "2e0", time_key, "payment");
    const auto repeated = instance.captured_definition.targets.front();
    instance.captured_definition.targets = {{time_key.kind, time_key.ref, "skip", "3"},
                                            repeated,
                                            {time_key.kind, time_key.ref, "skip", "0"},
                                            repeated};
    ASSERT_EQ(select_modifier_targets(instance, payment).size(), 2U);
    const std::array eligible{selected(instance)};
    const auto p =
        policy({{time_key, {}, {group("G", {member(instance, 2), member(instance, 4)})}}},
               "cfg-recalculation");
    const ModifierTraceProfile admitted{"admitted-producer", "stacking-v1"};
    const PlanOverrideDecision decision{OverrideAuthority::authorized, "operator-A",
                                        "synthetic override", "decision/plan-A/2"};
    const CapturedModifierContext document{
        decision.decision_ref, p.configuration_ref, p.policy_ref, admitted,
        ModifierRecalculationBinding{"plan-A", 2, "arrival-A", decision.decision_ref}};
    const std::vector<ModifierSelection> rows(2, {instance.captured_definition.revision_ref,
                                                  instance.revision_ref, time_key.ref, "G",
                                                  "multiply", "multiply", "2e0"});
    const std::array links{ModifierTraceLink{1, occurrence(instance, 2)},
                           ModifierTraceLink{2, occurrence(instance, 4)}};
    const ModifierStackCapture capture{
        "arrival-A", original_arrival, ModifierRecalculation{"plan-A", 2, decision},
        &document,   &admitted,        &p,
        eligible,    payment,          rows,
        links};
    const std::array bases{base(time_key, 60000)};
    const auto result = std::get<ModifierStackProposal>(evaluate(capture, bases));
    EXPECT_EQ(result.context_ref, decision.decision_ref);
    EXPECT_EQ(result.profile, admitted);
    EXPECT_EQ(result.selection, rows);
    EXPECT_EQ(result.links, (std::vector{links[0], links[1]}));
    EXPECT_EQ(result.targets.at(0).policy_ref, p.policy_ref);
    EXPECT_EQ(result.effects.at(0).exact_units, Exact(240000));
    EXPECT_EQ(result.effects.at(0).rounded_units, 240000);

    auto corrupted = capture;
    auto wrong = document;
    corrupted.context = &wrong;
    wrong.document_ref = original_arrival.modifier_context_ref;
    expect_refusal(evaluate(corrupted, bases), Failure::invalid);
    wrong = document;
    wrong.configuration_ref = original_arrival.configuration_ref;
    expect_refusal(evaluate(corrupted, bases), Failure::invalid);
    wrong = document;
    wrong.policy_ref = "another-policy";
    expect_refusal(evaluate(corrupted, bases), Failure::invalid);
    wrong = document;
    wrong.recalculation->revision = 1;
    expect_refusal(evaluate(corrupted, bases), Failure::invalid);
    wrong = document;
    wrong.recalculation->original_arrival_ref = "another-arrival";
    expect_refusal(evaluate(corrupted, bases), Failure::invalid);
    wrong = document;
    wrong.recalculation->plan_ref = "another-plan";
    expect_refusal(evaluate(corrupted, bases), Failure::invalid);
    wrong = document;
    wrong.recalculation->decision_ref = "another-decision";
    expect_refusal(evaluate(corrupted, bases), Failure::invalid);
    wrong = document;
    wrong.profile.producer_ref = "legacy-producer"; // The profile text alone cannot select v1.
    expect_refusal(evaluate(corrupted, bases), Failure::invalid);

    auto denied = decision;
    denied.authority = OverrideAuthority::denied;
    corrupted = capture;
    corrupted.recalculation = ModifierRecalculation{"plan-A", 2, denied};
    expect_refusal(evaluate(corrupted, bases), Failure::invalid);

    corrupted = capture;
    auto wrong_rows = rows;
    wrong_rows[0].group_operator = "lowest";
    corrupted.recorded_selection = wrong_rows;
    expect_refusal(evaluate(corrupted, bases), Failure::invalid);
    corrupted = capture;
    auto wrong_links = links;
    wrong_links[0].occurrence.target_ordinal = wrong_links[0].plan_row_ordinal;
    corrupted.recorded_links = wrong_links;
    expect_refusal(evaluate(corrupted, bases), Failure::invalid);

    EXPECT_EQ(original_arrival, original_binding);
}

TEST(CapturedModifiers, EffectiveOwningLimitsApplyToTheEntireProposal) {
    const auto half = new_instance();
    const std::array eligible{selected(half)};
    const auto p = policy({{time_key, {}, {group("time", {member(half)})}}});
    const Arrival arrival;
    const auto capture = arrival.capture(p, eligible);
    const std::array bases{base(time_key, 3), base(time_key, 7, 2)};
    const auto full = std::get<ModifierStackProposal>(evaluate(capture, bases));
    EXPECT_EQ(full.effects.at(0).exact_units, Exact(3) / 2);
    EXPECT_EQ(full.effects.at(1).exact_units, Exact(7) / 2);

    using Limit = std::size_t ModifierStackLimits::*;
    const std::array<std::tuple<const char *, Limit, std::size_t>, 9> limits{
        {{"occurrences", &ModifierStackLimits::occurrences, 0},
         {"policy entries", &ModifierStackLimits::policy_entries, 0},
         {"targets", &ModifierStackLimits::targets, 0},
         {"requests", &ModifierStackLimits::requests, 1},
         {"references", &ModifierStackLimits::reference_bytes, 4},
         {"trace value", &ModifierStackLimits::value_bytes, 1},
         {"input", &ModifierStackLimits::input_bytes, 1},
         {"output", &ModifierStackLimits::output_bytes, 1},
         {"operations", &ModifierStackLimits::operations, 0}}};
    for (const auto &[name, field, value] : limits) {
        SCOPED_TRACE(name);
        auto budget = fixture_limits;
        budget.*field = value;
        expect_refusal(evaluate(capture, bases, budget), Failure::resource_limit);
    }

    auto rows = fixture_limits;
    rows.materialized_rows = 2; // One selection plus one request; unrelated rows already charged.
    const auto one_base = std::span{bases}.first(1);
    EXPECT_EQ(std::get<ModifierStackProposal>(evaluate(capture, one_base, rows)).effects.size(),
              1U);
    expect_refusal(evaluate(capture, bases, rows), Failure::resource_limit);

    const auto nine = extra("nine", "9");
    const std::array nine_selected{selected(nine)};
    const auto nine_policy = policy({{time_key, {}, {group("nine", {member(nine)})}}});
    const std::array exact_bases{base(time_key, 9)};
    const auto nine_capture = arrival.capture(nine_policy, nine_selected);
    auto exact = fixture_limits;
    exact.exact_component_bytes = 2;
    EXPECT_EQ(std::get<ModifierStackProposal>(evaluate(nine_capture, exact_bases, exact))
                  .effects.at(0)
                  .exact_units,
              Exact(81));
    exact.exact_component_bytes = 1;
    expect_refusal(evaluate(nine_capture, exact_bases, exact), Failure::resource_limit);
}
} // namespace
