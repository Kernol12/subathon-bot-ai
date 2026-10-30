// SPDX-License-Identifier: GPL-3.0-or-later
// Contract revision 1; independent review/red evidence is tracked separately.
// Oracles/specification mapping: docs/evidence/time-credit-tests.md.
#include <gtest/gtest.h>
#include <limits>
#include <subathon/core/time_credit.hpp>
namespace {
using namespace subathon::core;
constexpr Milliseconds minute = 60'000;
constexpr Milliseconds maximum_ms = std::numeric_limits<Milliseconds>::max();
TEST(TimeCreditAcceptance, CapRecordsOnlyActuallyAddedTime) {
    const auto result = calculate_time_credit({55 * minute, 10 * minute, 60 * minute});
    EXPECT_EQ(result.requested_ms, 10 * minute);
    EXPECT_EQ(result.applied_ms, 5 * minute);
    EXPECT_EQ(result.remaining_after_ms, 60 * minute);
}
TEST(TimeCreditAcceptance, AvailableHeadroomAcceptsTheWholeAward) {
    const auto result = calculate_time_credit({20 * minute, 10 * minute, 60 * minute});
    EXPECT_EQ(result.requested_ms, 10 * minute);
    EXPECT_EQ(result.applied_ms, 10 * minute);
    EXPECT_EQ(result.remaining_after_ms, 30 * minute);
}
TEST(TimeCreditAcceptance, FullTimerAcceptsNoAdditionalTime) {
    const auto result = calculate_time_credit({60 * minute, 10 * minute, 60 * minute});
    EXPECT_EQ(result.requested_ms, 10 * minute);
    EXPECT_EQ(result.applied_ms, 0);
    EXPECT_EQ(result.remaining_after_ms, 60 * minute);
}
TEST(TimeCreditAcceptance, NoCapAcceptsTheWholeAward) {
    const auto result = calculate_time_credit({55 * minute, 10 * minute, std::nullopt});
    EXPECT_EQ(result.requested_ms, 10 * minute);
    EXPECT_EQ(result.applied_ms, 10 * minute);
    EXPECT_EQ(result.remaining_after_ms, 65 * minute);
}
TEST(TimeCreditAcceptance, ExactBoundaryDoesNotDiscardTime) {
    const auto result = calculate_time_credit({55 * minute, 5 * minute, 60 * minute});
    EXPECT_EQ(result.requested_ms, 5 * minute);
    EXPECT_EQ(result.applied_ms, 5 * minute);
    EXPECT_EQ(result.remaining_after_ms, 60 * minute);
}
TEST(TimeCreditAcceptance, ZeroAwardLeavesTheTimerUnchanged) {
    const auto result = calculate_time_credit({55 * minute, 0, 60 * minute});
    EXPECT_EQ(result.requested_ms, 0);
    EXPECT_EQ(result.applied_ms, 0);
    EXPECT_EQ(result.remaining_after_ms, 55 * minute);
}

// Numeric zero is covered without asserting eligibility or a lifecycle transition.
TEST(TimeCreditAcceptance, ZeroRemainingAcceptsOneMillisecond) {
    const auto result = calculate_time_credit({0, 1, std::nullopt});
    EXPECT_EQ(result.requested_ms, 1);
    EXPECT_EQ(result.applied_ms, 1);
    EXPECT_EQ(result.remaining_after_ms, 1);
}
TEST(TimeCreditAcceptance, ZeroCapHasNoCapacity) {
    const auto result = calculate_time_credit({0, 1, 0});
    EXPECT_EQ(result.requested_ms, 1);
    EXPECT_EQ(result.applied_ms, 0);
    EXPECT_EQ(result.remaining_after_ms, 0);
}
TEST(TimeCreditAcceptance, OneMillisecondHeadroomClipsTheAward) {
    const auto result = calculate_time_credit({2, 2, 3});
    EXPECT_EQ(result.requested_ms, 2);
    EXPECT_EQ(result.applied_ms, 1);
    EXPECT_EQ(result.remaining_after_ms, 3);
}
TEST(TimeCreditAcceptance, UncappedCreditCanReachTheLargestRepresentableTime) {
    const auto result = calculate_time_credit({maximum_ms - 1, 1, std::nullopt});
    EXPECT_EQ(result.requested_ms, 1);
    EXPECT_EQ(result.applied_ms, 1);
    EXPECT_EQ(result.remaining_after_ms, maximum_ms);
}
TEST(TimeCreditAcceptance, ZeroAwardAtLargestRepresentableTimeIsValid) {
    const auto result = calculate_time_credit({maximum_ms, 0, std::nullopt});
    EXPECT_EQ(result.requested_ms, 0);
    EXPECT_EQ(result.applied_ms, 0);
    EXPECT_EQ(result.remaining_after_ms, maximum_ms);
}
TEST(TimeCreditAcceptance, OneMillisecondBeyondTheRangeRequiresReview) {
    EXPECT_THROW(static_cast<void>(calculate_time_credit({maximum_ms, 1, std::nullopt})),
                 std::overflow_error);
}
TEST(TimeCreditAcceptance, LargeUncappedRequestMustNotWrap) {
    EXPECT_THROW(static_cast<void>(calculate_time_credit({1, maximum_ms, std::nullopt})),
                 std::overflow_error);
}
TEST(TimeCreditAcceptance, CapLimitsLargeRequestBeforeUnsafeAddition) {
    const auto result = calculate_time_credit({maximum_ms - 1, maximum_ms, maximum_ms});
    EXPECT_EQ(result.requested_ms, maximum_ms);
    EXPECT_EQ(result.applied_ms, 1);
    EXPECT_EQ(result.remaining_after_ms, maximum_ms);
}
TEST(TimeCreditAcceptance, NegativeRemainingIsNotDebtAtThisBoundary) {
    EXPECT_THROW(static_cast<void>(calculate_time_credit({-1, 1, std::nullopt})),
                 std::invalid_argument);
}
TEST(TimeCreditAcceptance, NegativeRequestIsNotARefundAtThisBoundary) {
    EXPECT_THROW(static_cast<void>(calculate_time_credit({1, -1, std::nullopt})),
                 std::invalid_argument);
}
TEST(TimeCreditAcceptance, NegativeCapIsRejected) {
    EXPECT_THROW(static_cast<void>(calculate_time_credit({0, 1, -1})), std::invalid_argument);
}
TEST(TimeCreditAcceptance, ExistingTimeAboveCapIsRejectedEvenForZeroAward) {
    EXPECT_THROW(static_cast<void>(calculate_time_credit({2, 0, 1})), std::invalid_argument);
}
TEST(TimeCreditAcceptance, InvalidInputIsRejectedBeforeConsideringOverflow) {
    EXPECT_THROW(static_cast<void>(calculate_time_credit({maximum_ms, 1, -1})),
                 std::invalid_argument);
}
} // namespace
