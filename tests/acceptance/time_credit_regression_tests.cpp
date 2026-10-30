// SPDX-License-Identifier: GPL-3.0-or-later
// Contract revision 1: validation order and signed 64-bit boundary regressions.
#include <gtest/gtest.h>
#include <limits>
#include <subathon/core/time_credit.hpp>
namespace {
using namespace subathon::core;
constexpr Milliseconds minimum_ms = std::numeric_limits<Milliseconds>::min();
constexpr Milliseconds maximum_ms = std::numeric_limits<Milliseconds>::max();

TEST(TimeCreditRegression, SignedMinimumInputsAreRejectedBeforeArithmetic) {
    EXPECT_THROW((void)calculate_time_credit({minimum_ms, maximum_ms, maximum_ms}),
                 std::invalid_argument);
    EXPECT_THROW((void)calculate_time_credit({0, minimum_ms, 0}), std::invalid_argument);
    EXPECT_THROW((void)calculate_time_credit({maximum_ms, maximum_ms, minimum_ms}),
                 std::invalid_argument);
}
TEST(TimeCreditRegression, AboveCapInputIsRejectedBeforeAnOverflowingAward) {
    EXPECT_THROW((void)calculate_time_credit({maximum_ms, maximum_ms, maximum_ms - 1}),
                 std::invalid_argument);
}
TEST(TimeCreditRegression, FullLargestCapPreservesTheLargestRequest) {
    const auto result = calculate_time_credit({maximum_ms, maximum_ms, maximum_ms});
    EXPECT_EQ(result.requested_ms, maximum_ms);
    EXPECT_EQ(result.applied_ms, 0);
    EXPECT_EQ(result.remaining_after_ms, maximum_ms);
}
TEST(TimeCreditRegression, LargestUncappedRequestCanBeCreditedFromZero) {
    const auto result = calculate_time_credit({0, maximum_ms, std::nullopt});
    EXPECT_EQ(result.requested_ms, maximum_ms);
    EXPECT_EQ(result.applied_ms, maximum_ms);
    EXPECT_EQ(result.remaining_after_ms, maximum_ms);
}
} // namespace
