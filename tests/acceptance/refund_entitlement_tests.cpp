// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <gtest/gtest.h>
#include <limits>
#include <span>
#include <subathon/core/refund_entitlement.hpp>
#include <subathon/core/time_debt.hpp>

namespace {
using namespace subathon::core;
using Unit = RequestedUnit;

void expect_entitlement(const RefundEntitlement &proposal, const Exact &fraction,
                        const Exact &exact, std::int64_t cumulative, std::int64_t increment,
                        const Exact &residual, bool full) {
    EXPECT_EQ(proposal.cumulative_fraction, fraction);
    EXPECT_EQ(proposal.exact_entitlement, exact);
    EXPECT_EQ(proposal.cumulative_units, cumulative);
    EXPECT_EQ(proposal.requested_increment, increment);
    EXPECT_EQ(proposal.rounding_residual, residual);
    EXPECT_EQ(proposal.full_refund, full);
}

TEST(RefundEntitlement, A01_FiveOfTwentyReversesSeventyFiveSecondsAndTenPoints) {
    const RefundAmounts amounts{20, 0, 5};
    expect_entitlement(calculate_refund_entitlement(amounts, Unit::milliseconds, 300000, 0),
                       Exact(1) / 4, 75000, 75000, -75000, 0, false);
    expect_entitlement(calculate_refund_entitlement(amounts, Unit::micro_points, 40000000, 0),
                       Exact(1) / 4, 10000000, 10000000, -10000000, 0, false);
}

TEST(RefundEntitlement, A02_SubtractsPriorUnitsAfterRoundingCumulativeEntitlement) {
    // At 1/4, 6 units have already rounded from 1.5 to 2. At 1/2 the total is
    // 3, so this refund requests one unit even though a standalone 1/4 rounds to two.
    expect_entitlement(calculate_refund_entitlement({20, 5, 5}, Unit::milliseconds, 6, 2),
                       Exact(1) / 2, 3, 3, -1, 0, false);
}

TEST(RefundEntitlement, A03_OneHundredCentRefundsCrossOnlyTheExpectedThresholds) {
    struct Credit {
        Unit unit;
        std::int64_t units;
        std::span<const int> crossings;
    };
    // Manually derived first cents for each rounded unit. At 50 cents, both
    // 1.5 and 2.5 choose 2. At 10/30/70/90 cents, the even neighbors for
    // five units are respectively 0/2/4/4.
    constexpr std::array time_crossings{17, 50, 84};
    constexpr std::array score_crossings{11, 30, 51, 70, 91};
    for (const auto &[unit, credit, crossings] : {Credit{Unit::milliseconds, 3, time_crossings},
                                                  Credit{Unit::micro_points, 5, score_crossings}}) {
        for (int cent = 1; cent <= 100; ++cent) {
            SCOPED_TRACE(::testing::Message() << "credit " << credit << ", cent " << cent);
            const std::int64_t cumulative =
                std::ranges::upper_bound(crossings, cent) - crossings.begin();
            const std::int64_t increment = std::ranges::binary_search(crossings, cent) ? -1 : 0;
            const RefundAmounts amounts{1, Exact(cent - 1) / 100, Exact(1) / 100};
            expect_entitlement(
                calculate_refund_entitlement(amounts, unit, credit, cumulative + increment),
                Exact(cent) / 100, Exact(credit * cent) / 100, cumulative, increment,
                Exact(credit * cent - 100 * cumulative) / 100, cent == 100);
        }
    }
}

TEST(RefundEntitlement, A04_ExactHalfTiesChooseTheEvenNeighborForBothUnits) {
    for (const auto unit : {Unit::milliseconds, Unit::micro_points}) {
        SCOPED_TRACE(static_cast<int>(unit));
        expect_entitlement(calculate_refund_entitlement({4, 0, 1}, unit, 2, 0), Exact(1) / 4,
                           Exact(1) / 2, 0, 0, Exact(1) / 2, false);
        expect_entitlement(calculate_refund_entitlement({4, 0, 3}, unit, 2, 0), Exact(3) / 4,
                           Exact(3) / 2, 2, -2, -Exact(1) / 2, false);
    }
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    constexpr std::int64_t even_neighbor = 4611686018427387904;
    expect_entitlement(calculate_refund_entitlement({2, 0, 1}, Unit::micro_points, max, 0),
                       Exact(1) / 2, Exact(max) / 2, even_neighbor, -even_neighbor, -Exact(1) / 2,
                       false);
}

TEST(RefundEntitlement, A05_FullRefundClearsExactRemainingCredit) {
    const RefundAmounts amounts{20, 5, 15};
    expect_entitlement(calculate_refund_entitlement(amounts, Unit::milliseconds, 300000, 75000), 1,
                       300000, 300000, -225000, 0, true);
    expect_entitlement(
        calculate_refund_entitlement(amounts, Unit::micro_points, 40000000, 10000000), 1, 40000000,
        40000000, -30000000, 0, true);
}

TEST(RefundEntitlement, A06_WholeCountsWaitForTheExactFullRefund) {
    expect_entitlement(calculate_refund_entitlement({4, 0, 3}, Unit::whole_count, 7, 0),
                       Exact(3) / 4, Exact(21) / 4, 0, 0, Exact(21) / 4, false);
    expect_entitlement(calculate_refund_entitlement({4, 3, 1}, Unit::whole_count, 7, 0), 1, 7, 7,
                       -7, 0, true);
}

TEST(RefundEntitlement, A07_UsesOriginalActualMainCreditAfterTheCap) {
    // 55 min before, 10 min requested, 60 min after; 5 min credited and 5 min capped.
    const TimeAdditionResult original{
        .request = {"original award", 600000},
        .balance_after = {3600000, 0},
        .debt_repaid_ms = 0,
        .main_added_ms = 300000,
        .cap_limited_ms = 300000,
    };
    expect_entitlement(
        calculate_refund_entitlement({20, 0, 10}, Unit::milliseconds, original.main_added_ms, 0),
        Exact(1) / 2, 150000, 150000, -150000, 0, false);
}

TEST(RefundEntitlement, A08_RejectsExcessAndNonpositiveRefundAmounts) {
    for (const RefundAmounts &amounts :
         {RefundAmounts{20, 19, Exact(101) / 100}, RefundAmounts{20, 20, 1},
          RefundAmounts{20, 0, 0}, RefundAmounts{20, 0, -1}, RefundAmounts{0, 0, 1},
          RefundAmounts{20, -1, 1}}) {
        EXPECT_THROW((void)calculate_refund_entitlement(amounts, Unit::milliseconds, 300000, 0),
                     std::invalid_argument);
    }
}

TEST(RefundEntitlement, A09_ExactFullFlagSurvivesEarlierRoundingToFullUnits) {
    const Exact original = parse_decimal("20.00000000000000000002").value();
    const Exact prior = parse_decimal("20.00000000000000000000").value();
    const Exact tiny = parse_decimal("0.00000000000000000001").value();
    constexpr auto unit = Unit::micro_points;
    const Exact denominator(boost::multiprecision::cpp_int("2000000000000000000002"));
    const Exact fraction = 1 - 1 / denominator;
    expect_entitlement(calculate_refund_entitlement({original, prior, tiny}, unit, 1, 1), fraction,
                       fraction, 1, 0, -1 / denominator, false);
    expect_entitlement(calculate_refund_entitlement({original, prior + tiny, tiny}, unit, 1, 1), 1,
                       1, 1, 0, 0, true);
}

TEST(RefundEntitlement, A10_RejectsComponentDataThatCannotYieldAReversal) {
    constexpr auto unit = Unit::milliseconds;
    EXPECT_THROW((void)calculate_refund_entitlement({20, 5, 5}, unit, 10, 6),
                 std::invalid_argument);
    EXPECT_THROW((void)calculate_refund_entitlement({20, 5, 5}, unit, 10, -1),
                 std::invalid_argument);
    EXPECT_THROW((void)calculate_refund_entitlement({20, 0, 5}, unit, -1, 0),
                 std::invalid_argument);
}

TEST(RefundEntitlement, A11_ZeroActualCreditKeepsTheFinancialFractionWithoutAnAdjustment) {
    expect_entitlement(calculate_refund_entitlement({20, 0, 5}, Unit::milliseconds, 0, 0),
                       Exact(1) / 4, 0, 0, 0, 0, false);
    expect_entitlement(calculate_refund_entitlement({20, 5, 15}, Unit::milliseconds, 0, 0), 1, 0, 0,
                       0, 0, true);
}

TEST(RefundEntitlement, A12_RoundsTheCombinedTimeTieBeforeAllocatingMainFirst) {
    // Half of 3 Main + 2 debt is 2.5, which rounds to 2. Rounding each share
    // separately would allocate 2 Main and 1 debt, one too many.
    const auto down = calculate_refund_time_entitlement({2, 0, 1}, {3, 2}, {0, 0});
    expect_entitlement(down.total, Exact(1) / 2, Exact(5) / 2, 2, -2, Exact(1) / 2, false);
    EXPECT_EQ(down.cumulative.main_ms, 2);
    EXPECT_EQ(down.cumulative.debt_repayment_ms, 0);
    EXPECT_EQ(down.requested_main_adjustment_ms, -2);
    EXPECT_EQ(down.requested_debt_restoration_ms, 0);

    // Half of 1 Main + 2 debt is 1.5, which rounds to 2. Consume the original
    // Main credit first, even though half of that component alone rounds to zero.
    const auto up = calculate_refund_time_entitlement({2, 0, 1}, {1, 2}, {0, 0});
    expect_entitlement(up.total, Exact(1) / 2, Exact(3) / 2, 2, -2, -Exact(1) / 2, false);
    EXPECT_EQ(up.cumulative.main_ms, 1);
    EXPECT_EQ(up.cumulative.debt_repayment_ms, 1);
    EXPECT_EQ(up.requested_main_adjustment_ms, -1);
    EXPECT_EQ(up.requested_debt_restoration_ms, 1);
}

TEST(RefundEntitlement, A13_SuccessiveRefundsCrossFromMainIntoDebtRepayment) {
    constexpr RefundTimeCredit original{3, 3};
    const auto first = calculate_refund_time_entitlement({8, 0, 2}, original, {0, 0});
    expect_entitlement(first.total, Exact(1) / 4, Exact(3) / 2, 2, -2, -Exact(1) / 2, false);
    EXPECT_EQ(first.cumulative.main_ms, 2);
    EXPECT_EQ(first.cumulative.debt_repayment_ms, 0);
    EXPECT_EQ(first.requested_main_adjustment_ms, -2);
    EXPECT_EQ(first.requested_debt_restoration_ms, 0);

    const auto second = calculate_refund_time_entitlement({8, 2, 4}, original, {2, 0});
    expect_entitlement(second.total, Exact(3) / 4, Exact(9) / 2, 4, -2, Exact(1) / 2, false);
    EXPECT_EQ(second.cumulative.main_ms, 3);
    EXPECT_EQ(second.cumulative.debt_repayment_ms, 1);
    EXPECT_EQ(second.requested_main_adjustment_ms, -1);
    EXPECT_EQ(second.requested_debt_restoration_ms, 1);

    const auto final = calculate_refund_time_entitlement({8, 6, 2}, original, {3, 1});
    expect_entitlement(final.total, 1, 6, 6, -2, 0, true);
    EXPECT_EQ(final.cumulative.main_ms, 3);
    EXPECT_EQ(final.cumulative.debt_repayment_ms, 3);
    EXPECT_EQ(final.requested_main_adjustment_ms, 0);
    EXPECT_EQ(final.requested_debt_restoration_ms, 2);
}

TEST(RefundEntitlement, A14_FullRefundClearsBothOriginalTimeRemainders) {
    // The accepted half refund allocated 5 of the 6 Main ms and none of the 4 debt ms.
    const auto final = calculate_refund_time_entitlement({20, 10, 10}, {6, 4}, {5, 0});
    expect_entitlement(final.total, 1, 10, 10, -5, 0, true);
    EXPECT_EQ(final.cumulative.main_ms, 6);
    EXPECT_EQ(final.cumulative.debt_repayment_ms, 4);
    EXPECT_EQ(final.requested_main_adjustment_ms, -1);
    EXPECT_EQ(final.requested_debt_restoration_ms, 4);
}

TEST(RefundEntitlement, A15_RejectsImpossibleTimeAllocationsAndOutOfRangeTotal) {
    EXPECT_THROW((void)calculate_refund_time_entitlement({20, 10, 10}, {6, 4}, {4, 1}),
                 std::invalid_argument); // Debt already allocated before Main was exhausted.
    EXPECT_THROW((void)calculate_refund_time_entitlement({20, 5, 5}, {6, 4}, {6, 0}),
                 std::invalid_argument); // Current cumulative Main is only 5.
    EXPECT_THROW((void)calculate_refund_time_entitlement({20, 0, 10}, {6, -1}, {0, 0}),
                 std::invalid_argument);
    EXPECT_THROW((void)calculate_refund_time_entitlement({20, 0, 10}, {6, 4}, {-1, 0}),
                 std::invalid_argument);
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    EXPECT_THROW((void)calculate_refund_time_entitlement({20, 0, 10}, {max, 1}, {0, 0}),
                 std::overflow_error);
}
} // namespace
