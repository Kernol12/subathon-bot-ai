// SPDX-License-Identifier: GPL-3.0-or-later
// DRAFT acceptance baseline: requires independent specification review.
#include <gtest/gtest.h>
#include <subathon/core/time_credit.hpp>
namespace {
using namespace subathon::core;
constexpr Milliseconds minute = 60'000;
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
    EXPECT_EQ(result.applied_ms, 0);
    EXPECT_EQ(result.remaining_after_ms, 60 * minute);
}
TEST(TimeCreditAcceptance, NoCapAcceptsTheWholeAward) {
    const auto result = calculate_time_credit({55 * minute, 10 * minute, std::nullopt});
    EXPECT_EQ(result.applied_ms, 10 * minute);
    EXPECT_EQ(result.remaining_after_ms, 65 * minute);
}
TEST(TimeCreditAcceptance, ExactBoundaryDoesNotDiscardTime) {
    const auto result = calculate_time_credit({55 * minute, 5 * minute, 60 * minute});
    EXPECT_EQ(result.applied_ms, 5 * minute);
    EXPECT_EQ(result.remaining_after_ms, 60 * minute);
}
TEST(TimeCreditAcceptance, ZeroAwardLeavesTheTimerUnchanged) {
    const auto result = calculate_time_credit({55 * minute, 0, 60 * minute});
    EXPECT_EQ(result.requested_ms, 0);
    EXPECT_EQ(result.applied_ms, 0);
    EXPECT_EQ(result.remaining_after_ms, 55 * minute);
}
} // namespace
