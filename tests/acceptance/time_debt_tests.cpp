// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/time_debt.hpp>
#include <subathon/core/timer_lifecycle.hpp>

#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <vector>

using namespace subathon::core;

namespace {
constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
constexpr auto minimum = std::numeric_limits<std::int64_t>::min();

TimeReversalRequest reversal(std::int64_t adjustment) {
    return {"reversal-1", "original-effect-1", adjustment};
}
TimeAdditionRequest addition(std::int64_t amount) { return {"addition-1", amount}; }
void expect_balance(const TimeBalance &balance, std::int64_t main, std::int64_t debt) {
    EXPECT_EQ(balance.main_remaining_ms, main);
    EXPECT_EQ(balance.time_debt_ms, debt);
}
TimerState main_timer(std::int64_t remaining) {
    return {TimerLifecycle::Active, CountdownPhase::Main, remaining, 73'000, false};
}
TimerState grace_timer() {
    return {TimerLifecycle::Active, CountdownPhase::Grace, 0, 73'000, false};
}
} // namespace

// §4 Refund overflow: the selected policy acts only on a reversal deficit.
TEST(TimeDebtReversal, EveryPolicyAppliesAReversalWithinMain) {
    for (const auto policy : {RefundDeficitPolicy::StopAtZero, RefundDeficitPolicy::CarryTimeDebt,
                              RefundDeficitPolicy::RequireReview}) {
        const auto result = plan_time_reversal({100, 7}, reversal(-40), policy);
        expect_balance(result.balance_after, 60, 7);
        EXPECT_EQ(result.policy, policy);
        EXPECT_EQ(result.disposition, TimeReversalDisposition::Applied);
        EXPECT_EQ(result.request.requested_adjustment_ms, -40);
        EXPECT_EQ(result.main_adjustment_ms, -40);
        EXPECT_EQ(result.debt_added_ms, 0);
        EXPECT_EQ(result.unapplied_adjustment_ms, 0);
    }
}

TEST(TimeDebtReversal, ExactExhaustionIsNotAReviewDeficit) {
    for (const auto policy : {RefundDeficitPolicy::StopAtZero, RefundDeficitPolicy::CarryTimeDebt,
                              RefundDeficitPolicy::RequireReview}) {
        const auto result = plan_time_reversal({100, 7}, reversal(-100), policy);
        expect_balance(result.balance_after, 0, 7);
        EXPECT_EQ(result.disposition, TimeReversalDisposition::Applied);
        EXPECT_EQ(result.main_adjustment_ms, -100);
        EXPECT_EQ(result.debt_added_ms, 0);
        EXPECT_EQ(result.unapplied_adjustment_ms, 0);
    }
}

TEST(TimeDebtReversal, ZeroAdjustmentRetainsTheRecordWithoutAReviewHold) {
    for (const auto policy : {RefundDeficitPolicy::StopAtZero, RefundDeficitPolicy::CarryTimeDebt,
                              RefundDeficitPolicy::RequireReview}) {
        const auto result = plan_time_reversal({0, 7}, reversal(0), policy);
        expect_balance(result.balance_after, 0, 7);
        EXPECT_EQ(result.disposition, TimeReversalDisposition::Applied);
        EXPECT_EQ(result.request.reversal_ref, "reversal-1");
        EXPECT_EQ(result.request.original_effect_ref, "original-effect-1");
        EXPECT_EQ(result.request.requested_adjustment_ms, 0);
        EXPECT_EQ(result.main_adjustment_ms, 0);
        EXPECT_EQ(result.debt_added_ms, 0);
        EXPECT_EQ(result.unapplied_adjustment_ms, 0);
    }
}

TEST(TimeDebtReversal, StopAtZeroReportsFullRequestedAndUnappliedAmounts) {
    const auto result =
        plan_time_reversal({100, 7}, reversal(-150), RefundDeficitPolicy::StopAtZero);
    expect_balance(result.balance_after, 0, 7);
    EXPECT_EQ(result.disposition, TimeReversalDisposition::Applied);
    EXPECT_EQ(result.request.reversal_ref, "reversal-1");
    EXPECT_EQ(result.request.original_effect_ref, "original-effect-1");
    EXPECT_EQ(result.request.requested_adjustment_ms, -150);
    EXPECT_EQ(result.policy, RefundDeficitPolicy::StopAtZero);
    EXPECT_EQ(result.main_adjustment_ms, -100);
    EXPECT_EQ(result.debt_added_ms, 0);
    EXPECT_EQ(result.unapplied_adjustment_ms, -50);
}

TEST(TimeDebtReversal, CarryTimeDebtAddsOnlyTheExcessToExistingDebt) {
    const auto result =
        plan_time_reversal({100, 7}, reversal(-150), RefundDeficitPolicy::CarryTimeDebt);
    expect_balance(result.balance_after, 0, 57);
    EXPECT_EQ(result.disposition, TimeReversalDisposition::Applied);
    EXPECT_EQ(result.request.reversal_ref, "reversal-1");
    EXPECT_EQ(result.request.original_effect_ref, "original-effect-1");
    EXPECT_EQ(result.request.requested_adjustment_ms, -150);
    EXPECT_EQ(result.policy, RefundDeficitPolicy::CarryTimeDebt);
    EXPECT_EQ(result.main_adjustment_ms, -100);
    EXPECT_EQ(result.debt_added_ms, 50);
    EXPECT_EQ(result.unapplied_adjustment_ms, 0);
}

TEST(TimeDebtReversal, RequireReviewHoldsTheWholeAdjustmentWithoutPartialSubtraction) {
    const auto result =
        plan_time_reversal({100, 7}, reversal(-150), RefundDeficitPolicy::RequireReview);
    expect_balance(result.balance_after, 100, 7);
    EXPECT_EQ(result.disposition, TimeReversalDisposition::RequiresReview);
    EXPECT_EQ(result.request.reversal_ref, "reversal-1");
    EXPECT_EQ(result.request.original_effect_ref, "original-effect-1");
    EXPECT_EQ(result.request.requested_adjustment_ms, -150);
    EXPECT_EQ(result.policy, RefundDeficitPolicy::RequireReview);
    EXPECT_EQ(result.main_adjustment_ms, 0);
    EXPECT_EQ(result.debt_added_ms, 0);
    EXPECT_EQ(result.unapplied_adjustment_ms, -150);
}

TEST(TimeDebtReversal, SelectingAnotherPolicyDoesNotClearOutstandingDebt) {
    const auto stopped = plan_time_reversal({5, 20}, reversal(-2), RefundDeficitPolicy::StopAtZero);
    expect_balance(stopped.balance_after, 3, 20);
    const auto held = plan_time_reversal({5, 20}, reversal(-8), RefundDeficitPolicy::RequireReview);
    expect_balance(held.balance_after, 5, 20);
    EXPECT_EQ(held.disposition, TimeReversalDisposition::RequiresReview);
}

// NUM-02/04: signed extrema are data, not a license to negate/wrap/saturate.
TEST(TimeDebtReversal, MinimumSignedRequestCanStopAtZeroWithoutNegation) {
    const auto result =
        plan_time_reversal({maximum, 7}, reversal(minimum), RefundDeficitPolicy::StopAtZero);
    expect_balance(result.balance_after, 0, 7);
    EXPECT_EQ(result.request.requested_adjustment_ms, minimum);
    EXPECT_EQ(result.main_adjustment_ms, -9'223'372'036'854'775'807LL);
    EXPECT_EQ(result.debt_added_ms, 0);
    EXPECT_EQ(result.unapplied_adjustment_ms, -1);
}

TEST(TimeDebtReversal, MinimumSignedRequestCanProduceAnExactlyMaximumDebt) {
    const auto result =
        plan_time_reversal({1, 0}, reversal(minimum), RefundDeficitPolicy::CarryTimeDebt);
    expect_balance(result.balance_after, 0, maximum);
    EXPECT_EQ(result.main_adjustment_ms, -1);
    EXPECT_EQ(result.debt_added_ms, maximum);
    EXPECT_EQ(result.unapplied_adjustment_ms, 0);
}

TEST(TimeDebtReversal, MinimumSignedRequestWithNoMainCannotWrapTheDebt) {
    EXPECT_THROW(
        (void)plan_time_reversal({0, 0}, reversal(minimum), RefundDeficitPolicy::CarryTimeDebt),
        std::overflow_error);
}

TEST(TimeDebtReversal, ReviewCanHoldMinimumSignedDataWithoutCreatingOverflowingDebt) {
    const auto result =
        plan_time_reversal({0, maximum}, reversal(minimum), RefundDeficitPolicy::RequireReview);
    expect_balance(result.balance_after, 0, maximum);
    EXPECT_EQ(result.disposition, TimeReversalDisposition::RequiresReview);
    EXPECT_EQ(result.request.requested_adjustment_ms, minimum);
    EXPECT_EQ(result.main_adjustment_ms, 0);
    EXPECT_EQ(result.debt_added_ms, 0);
    EXPECT_EQ(result.unapplied_adjustment_ms, minimum);
}

TEST(TimeDebtReversal, DebtAdditionOverflowReturnsNoPartialProposal) {
    const TimeBalance balance{2, maximum};
    const auto request = reversal(-3);
    EXPECT_THROW((void)plan_time_reversal(balance, request, RefundDeficitPolicy::CarryTimeDebt),
                 std::overflow_error);
    expect_balance(balance, 2, maximum);
    EXPECT_EQ(request.requested_adjustment_ms, -3);
}

TEST(TimeDebtReversal, ReturnedRequestOwnsBothReferencesAndFullSignedAdjustment) {
    auto request = reversal(-150);
    const auto result = plan_time_reversal({100, 7}, request, RefundDeficitPolicy::StopAtZero);
    request.reversal_ref = "changed-reversal";
    request.original_effect_ref.clear();
    request.requested_adjustment_ms = 0;
    EXPECT_EQ(result.request.reversal_ref, "reversal-1");
    EXPECT_EQ(result.request.original_effect_ref, "original-effect-1");
    EXPECT_EQ(result.request.requested_adjustment_ms, -150);
    EXPECT_EQ(result.policy, RefundDeficitPolicy::StopAtZero);
}

TEST(TimeDebtReversal, InvalidBalanceSignReferencesAndPolicyRejectBeforeProposal) {
    EXPECT_THROW((void)plan_time_reversal({-1, 0}, reversal(-1), RefundDeficitPolicy::StopAtZero),
                 std::invalid_argument);
    EXPECT_THROW(
        (void)plan_time_reversal({0, -1}, reversal(-1), RefundDeficitPolicy::CarryTimeDebt),
        std::invalid_argument);
    EXPECT_THROW((void)plan_time_reversal({0, 0}, reversal(1), RefundDeficitPolicy::RequireReview),
                 std::invalid_argument);
    EXPECT_THROW(
        (void)plan_time_reversal({0, 0}, {"", "effect-1", -1}, RefundDeficitPolicy::StopAtZero),
        std::invalid_argument);
    EXPECT_THROW(
        (void)plan_time_reversal({0, 0}, {"reversal-1", "", -1}, RefundDeficitPolicy::StopAtZero),
        std::invalid_argument);
    EXPECT_THROW(
        (void)plan_time_reversal({0, 0}, reversal(-1), static_cast<RefundDeficitPolicy>(99)),
        std::invalid_argument);
    EXPECT_THROW(
        (void)plan_time_reversal({100, 0}, reversal(-1), static_cast<RefundDeficitPolicy>(99)),
        std::invalid_argument);
}

// §4: future additions pay separate debt before any Main cap calculation.
TEST(TimeDebtAddition, WithoutDebtTheCapLimitsActualMainAndRetainsTheRequest) {
    const auto result = plan_time_addition({55, 0}, addition(10), 60);
    expect_balance(result.balance_after, 60, 0);
    EXPECT_EQ(result.request.addition_ref, "addition-1");
    EXPECT_EQ(result.request.requested_ms, 10);
    EXPECT_EQ(result.debt_repaid_ms, 0);
    EXPECT_EQ(result.main_added_ms, 5);
    EXPECT_EQ(result.cap_limited_ms, 5);
}

TEST(TimeDebtAddition, AFullMainCapStillPermitsDebtRepayment) {
    const auto result = plan_time_addition({60, 7}, addition(5), 60);
    expect_balance(result.balance_after, 60, 2);
    EXPECT_EQ(result.request.addition_ref, "addition-1");
    EXPECT_EQ(result.request.requested_ms, 5);
    EXPECT_EQ(result.debt_repaid_ms, 5);
    EXPECT_EQ(result.main_added_ms, 0);
    EXPECT_EQ(result.cap_limited_ms, 0);
}

TEST(TimeDebtAddition, AboveCapMainIsRetainedWhileDebtRepaymentStillPrecedesCap) {
    const auto result = plan_time_addition({100, 5}, addition(8), 60);
    expect_balance(result.balance_after, 100, 0);
    EXPECT_EQ(result.debt_repaid_ms, 5);
    EXPECT_EQ(result.main_added_ms, 0);
    EXPECT_EQ(result.cap_limited_ms, 3);
}

TEST(TimeDebtAddition, PartialRepaymentLeavesMainUntouched) {
    const auto result = plan_time_addition({5, 10}, addition(3), 5);
    expect_balance(result.balance_after, 5, 7);
    EXPECT_EQ(result.debt_repaid_ms, 3);
    EXPECT_EQ(result.main_added_ms, 0);
    EXPECT_EQ(result.cap_limited_ms, 0);
}

TEST(TimeDebtAddition, ExactRepaymentAtZeroCapDoesNotCreateMainCredit) {
    const auto result = plan_time_addition({0, 10}, addition(10), 0);
    expect_balance(result.balance_after, 0, 0);
    EXPECT_EQ(result.request.addition_ref, "addition-1");
    EXPECT_EQ(result.request.requested_ms, 10);
    EXPECT_EQ(result.debt_repaid_ms, 10);
    EXPECT_EQ(result.main_added_ms, 0);
    EXPECT_EQ(result.cap_limited_ms, 0);
}

TEST(TimeDebtAddition, DebtRepaymentPrecedesCapLimitingOfTheRemainder) {
    const auto result = plan_time_addition({55, 2}, addition(10), 60);
    expect_balance(result.balance_after, 60, 0);
    EXPECT_EQ(result.request.requested_ms, 10);
    EXPECT_EQ(result.debt_repaid_ms, 2);
    EXPECT_EQ(result.main_added_ms, 5);
    EXPECT_EQ(result.cap_limited_ms, 3);
}

TEST(TimeDebtAddition, UncappedSurplusAfterDebtBecomesMainTime) {
    const auto result = plan_time_addition({4, 3}, addition(10));
    expect_balance(result.balance_after, 11, 0);
    EXPECT_EQ(result.debt_repaid_ms, 3);
    EXPECT_EQ(result.main_added_ms, 7);
    EXPECT_EQ(result.cap_limited_ms, 0);
}

TEST(TimeDebtAddition, ZeroRequestPreservesBothPositiveBalances) {
    const auto result = plan_time_addition({100, 3}, addition(0), 60);
    expect_balance(result.balance_after, 100, 3);
    EXPECT_EQ(result.request.requested_ms, 0);
    EXPECT_EQ(result.debt_repaid_ms, 0);
    EXPECT_EQ(result.main_added_ms, 0);
    EXPECT_EQ(result.cap_limited_ms, 0);
}

TEST(TimeDebtAddition, DebtFirstAvoidsAnOverflowingHypotheticalMainPlusRequest) {
    const auto result = plan_time_addition({maximum, maximum}, addition(maximum));
    expect_balance(result.balance_after, maximum, 0);
    EXPECT_EQ(result.debt_repaid_ms, maximum);
    EXPECT_EQ(result.main_added_ms, 0);
    EXPECT_EQ(result.cap_limited_ms, 0);
}

TEST(TimeDebtAddition, ActualCapHeadroomAvoidsOverflowOfAnUncappedHypothesis) {
    const auto result =
        plan_time_addition({9'223'372'036'854'775'806LL, 0}, addition(maximum), maximum);
    expect_balance(result.balance_after, maximum, 0);
    EXPECT_EQ(result.debt_repaid_ms, 0);
    EXPECT_EQ(result.main_added_ms, 1);
    EXPECT_EQ(result.cap_limited_ms, 9'223'372'036'854'775'806LL);
}

TEST(TimeDebtAddition, UnrepresentableActualMainReturnsNoPartialProposal) {
    const TimeBalance balance{maximum, 0};
    const auto request = addition(1);
    EXPECT_THROW((void)plan_time_addition(balance, request), std::overflow_error);
    expect_balance(balance, maximum, 0);
    EXPECT_EQ(request.requested_ms, 1);
}

TEST(TimeDebtAddition, InvalidInputsRejectBeforeDebtApplication) {
    EXPECT_THROW((void)plan_time_addition({-1, 0}, addition(1)), std::invalid_argument);
    EXPECT_THROW((void)plan_time_addition({0, -1}, addition(1)), std::invalid_argument);
    EXPECT_THROW((void)plan_time_addition({0, 0}, addition(-1)), std::invalid_argument);
    EXPECT_THROW((void)plan_time_addition({0, 0}, {"", 1}), std::invalid_argument);
    EXPECT_THROW((void)plan_time_addition({0, 0}, addition(0), -1), std::invalid_argument);
}

// Compose with the existing reviewed reducer. Expected snapshots/transitions are
// literal requirements; no implementation computes a test's expected value.
TEST(TimeDebtComposition, DebtOnlyCreditCannotRescueGraceOrConsumeItsBudget) {
    const auto timer = grace_timer();
    const auto proposal = plan_time_addition({0, 5}, addition(5), 0);
    const auto next = apply_applicable_time_credit(timer, proposal.main_added_ms);
    expect_balance(proposal.balance_after, 0, 0);
    EXPECT_EQ(next.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(next.phase, CountdownPhase::Grace);
    EXPECT_EQ(next.main_remaining_ms, 0);
    EXPECT_EQ(next.grace_remaining_ms, 73'000);
}

TEST(TimeDebtComposition, OnlyTheSurplusAfterDebtRescuesGraceAndRetainsItsBudget) {
    const auto timer = grace_timer();
    const auto proposal = plan_time_addition({0, 5}, addition(7));
    const auto next = apply_applicable_time_credit(timer, proposal.main_added_ms);
    expect_balance(proposal.balance_after, 2, 0);
    EXPECT_EQ(next.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(next.phase, CountdownPhase::Main);
    EXPECT_EQ(next.main_remaining_ms, 2);
    EXPECT_EQ(next.grace_remaining_ms, 73'000);
}

TEST(TimeDebtComposition, ExactDebtRepaymentCannotClearConfirmationWait) {
    auto timer = main_timer(0);
    timer.awaiting_confirmation = true;
    const auto proposal = plan_time_addition({0, 5}, addition(5));
    const auto next = apply_applicable_time_credit(timer, proposal.main_added_ms);
    EXPECT_EQ(next.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(next.phase, CountdownPhase::Main);
    EXPECT_EQ(next.main_remaining_ms, 0);
    EXPECT_TRUE(next.awaiting_confirmation);
    EXPECT_EQ(next.grace_remaining_ms, 73'000);
}

TEST(TimeDebtComposition, ExhaustedMainUsesTheExistingImmediateEndPolicyEvenPaused) {
    auto timer = main_timer(100);
    const auto proposal =
        plan_time_reversal({100, 0}, reversal(-150), RefundDeficitPolicy::StopAtZero);
    timer.main_remaining_ms = proposal.balance_after.main_remaining_ms;
    TimerConfig config;
    config.end_mode = TimerEndMode::Immediate;
    const auto next = advance_timer(timer, config, {0, 1'000, true});
    EXPECT_EQ(next.state.lifecycle, TimerLifecycle::Finished);
    EXPECT_EQ(next.state.main_remaining_ms, 0);
    EXPECT_EQ(next.state.grace_remaining_ms, 73'000);
    EXPECT_EQ(next.transitions, (std::vector<TimerTransition>{TimerTransition::MainExpired,
                                                              TimerTransition::Finished}));
}

TEST(TimeDebtComposition, ExhaustedMainUsesSharedGraceWithoutEncodingDebtAsGrace) {
    auto timer = main_timer(100);
    const auto proposal =
        plan_time_reversal({100, 7}, reversal(-150), RefundDeficitPolicy::CarryTimeDebt);
    timer.main_remaining_ms = proposal.balance_after.main_remaining_ms;
    TimerConfig config;
    config.grace_reuse = GraceReuse::Shared;
    const auto next = advance_timer(timer, config, {0, 1'000, false});
    expect_balance(proposal.balance_after, 0, 57);
    EXPECT_EQ(next.state.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(next.state.phase, CountdownPhase::Grace);
    EXPECT_EQ(next.state.main_remaining_ms, 0);
    EXPECT_EQ(next.state.grace_remaining_ms, 73'000);
    EXPECT_EQ(next.transitions, (std::vector<TimerTransition>{TimerTransition::MainExpired,
                                                              TimerTransition::GraceStarted}));
}

TEST(TimeDebtComposition, ExhaustedMainUsesTheExistingConfirmationEndPolicy) {
    auto timer = main_timer(100);
    const auto proposal =
        plan_time_reversal({100, 0}, reversal(-150), RefundDeficitPolicy::StopAtZero);
    timer.main_remaining_ms = proposal.balance_after.main_remaining_ms;
    TimerConfig config;
    config.end_mode = TimerEndMode::Confirmation;
    const auto next = advance_timer(timer, config, {0, 1'000, false});
    EXPECT_EQ(next.state.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(next.state.phase, CountdownPhase::Main);
    EXPECT_EQ(next.state.main_remaining_ms, 0);
    EXPECT_TRUE(next.state.awaiting_confirmation);
    EXPECT_EQ(next.state.grace_remaining_ms, 73'000);
    EXPECT_EQ(next.transitions,
              (std::vector<TimerTransition>{TimerTransition::MainExpired,
                                            TimerTransition::ConfirmationRequired}));
}
