// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>
#include <limits>
#include <subathon/core/reference_rule.hpp>
using namespace subathon::core;
namespace {
ReferenceRuleInput input() {
    return {"R",
            "r",
            "a",
            "b",
            "T",
            true,
            "synthetic.reference.support.v1",
            "synthetic.reference.support.v1",
            "time_add",
            "ms",
            600000};
}
void request(const ReferenceRuleDerivation &r, std::int64_t amount) {
    ASSERT_EQ(r.state, ReferenceDerivationState::matched);
    EXPECT_TRUE(r.selection.selected);
    EXPECT_EQ(r.selection.rule_ref, "R");
    EXPECT_EQ(r.selection.version_ref, "r");
    EXPECT_EQ(r.selection.explanation, "reference.single-rule.match.v1");
    ASSERT_TRUE(r.request);
    const auto &q = *r.request;
    EXPECT_EQ(q.action_ref, "a");
    EXPECT_EQ(q.action_version_ref, "b");
    EXPECT_EQ(q.rule_ref, "R");
    EXPECT_EQ(q.target_ref, "T");
    EXPECT_EQ(q.unit, RequestedUnit::milliseconds);
    EXPECT_FALSE(q.requires_historical_valuation);
    EXPECT_EQ(q.requested_units, amount);
    EXPECT_EQ(numerator(q.exact_requested_units), amount);
    EXPECT_EQ(denominator(q.exact_requested_units), 1);
    EXPECT_EQ(q.exact_requested_units - Exact{q.requested_units}, Exact{0});
    EXPECT_EQ(q.calculation_explanation, "reference.integer-ms.v1");
    EXPECT_EQ(q.reversal_behavior_ref, "reference.original-time.v1");
}
template <class F> void fails(F f, ReferenceDerivationError expected) {
    try {
        f();
        FAIL() << "Expected typed derivation refusal";
    } catch (const ReferenceDerivationFailure &e) {
        EXPECT_EQ(e.code(), expected);
    }
    // NI is intentionally not accepted or caught.
}
TEST(ReferenceRule, CapturedIntegerHasExactRequestAndOriginalProvenance) {
    request(derive_reference_rule(input()), 600000);
}
TEST(ReferenceRule, ZeroIsARealRequest) {
    auto i = input();
    i.add_ms = 0;
    request(derive_reference_rule(i), 0);
}
TEST(ReferenceRule, Signed64MaximumNeedsNoNarrowingOrOverflowingTestLiteral) {
    auto i = input();
    i.add_ms = std::numeric_limits<std::int64_t>::max();
    request(derive_reference_rule(i), INT64_C(9223372036854775807));
}
TEST(ReferenceRule, DisabledAndNonmatchDoNotInventAnAward) {
    auto i = input();
    i.enabled = false;
    const auto a = derive_reference_rule(i);
    EXPECT_EQ(a.state, ReferenceDerivationState::disabled);
    EXPECT_FALSE(a.request);
    EXPECT_FALSE(a.selection.selected);
    EXPECT_EQ(a.selection.rule_ref, "R");
    EXPECT_EQ(a.selection.version_ref, "r");
    EXPECT_EQ(a.selection.explanation, "reference.single-rule.disabled.v1");
    i.enabled = true;
    i.observed_type = "another-known-trigger";
    const auto b = derive_reference_rule(i);
    EXPECT_EQ(b.state, ReferenceDerivationState::nonmatch);
    EXPECT_FALSE(b.request);
    EXPECT_FALSE(b.selection.selected);
    EXPECT_EQ(b.selection.explanation, "reference.single-rule.nonmatch.v1");
}
TEST(ReferenceRule, UnsupportedTriggerActionAndUnitNeverResolveCallerAwards) {
    for (int field = 0; field < 3; ++field) {
        auto i = input();
        (field == 0 ? i.trigger_type : field == 1 ? i.action_kind : i.action_unit) = "other";
        const auto r = derive_reference_rule(i);
        EXPECT_EQ(r.state, ReferenceDerivationState::unsupported);
        EXPECT_FALSE(r.request);
        EXPECT_FALSE(r.selection.selected);
        EXPECT_EQ(r.selection.explanation, "reference.single-rule.unsupported.v1");
    }
}
TEST(ReferenceRule, NegativeIsMalformedEvenIfDisabledOrUnsupported) {
    auto i = input();
    i.add_ms = -1;
    i.enabled = false;
    i.action_unit = "other";
    fails([&] { (void)derive_reference_rule(i); }, ReferenceDerivationError::invalid_value);
}
TEST(ReferenceRule, WholeInputPreflightAndRequiredReferencesPrecedeProfileRefusal) {
    auto i = input();
    i.enabled = false;
    i.action_unit = "other";
    i.timer_ref.clear();
    fails([&] { (void)derive_reference_rule(i); }, ReferenceDerivationError::invalid_reference);
    i.timer_ref = std::string(4097, 'x');
    fails([&] { (void)derive_reference_rule(i); }, ReferenceDerivationError::input_limit);
}
TEST(ReferenceRule, ExactFitAndOneLessInputBudgetOnNonmatch) {
    // Declaration arithmetic: five1-byte refs + bool1 + two30-byte trigger types
    // + time_add8 + ms2 + i64=8 =84. Change observed to a different30-byte value.
    auto i = input();
    i.observed_type = std::string(30, 'x');
    ReferenceRuleLimits l;
    l.value_owned_bytes = 84;
    EXPECT_EQ(derive_reference_rule(i, l).state, ReferenceDerivationState::nonmatch);
    l.value_owned_bytes = 83;
    fails([&] { (void)derive_reference_rule(i, l); }, ReferenceDerivationError::input_limit);
}
TEST(ReferenceRule, CompleteOutputExactFitAndOneLess) {
    // Independently summed declared output fields, no evaluate/encode oracle.
    ReferenceRuleLimits l;
    l.value_owned_bytes = 105;
    request(derive_reference_rule(input(), l), 600000);
    l.value_owned_bytes = 104;
    fails([&] { (void)derive_reference_rule(input(), l); }, ReferenceDerivationError::result_limit);
}
TEST(ReferenceRule, ReferenceBoundaryAndInvalidLimitsPrecedeBorrowedScan) {
    auto i = input();
    i.timer_ref = std::string(4096, 'x');
    EXPECT_TRUE(derive_reference_rule(i).request);
    i.timer_ref.push_back('x');
    fails([&] { (void)derive_reference_rule(i); }, ReferenceDerivationError::input_limit);
    ReferenceRuleLimits l;
    l.value_items = 0;
    fails([&] { (void)derive_reference_rule(i, l); }, ReferenceDerivationError::invalid_limits);
}
TEST(ReferenceRule, ResultsOwnTheirCapturedVersion) {
    auto i = input();
    const auto r1 = derive_reference_rule(i);
    i.rule_version_ref = "r2";
    i.add_ms = 900000;
    const auto r2 = derive_reference_rule(i);
    request(r1, 600000);
    EXPECT_EQ(r2.selection.version_ref, "r2");
    ASSERT_TRUE(r2.request);
    EXPECT_EQ(r2.request->requested_units, 900000);
}
} // namespace
