// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/refund_entitlement.hpp>

#include <algorithm>
#include <limits>

namespace subathon::core {
RefundEntitlement calculate_refund_entitlement(const RefundAmounts &amounts, RequestedUnit unit,
                                               std::int64_t original_actual_credit_units,
                                               std::int64_t previous_cumulative_units) {
    if (amounts.original <= 0 || amounts.incoming <= 0 || amounts.previously_accepted < 0 ||
        original_actual_credit_units < 0 || previous_cumulative_units < 0) {
        throw std::invalid_argument("Invalid refund amounts or component progress");
    }
    const Exact accepted = amounts.previously_accepted + amounts.incoming;
    if (accepted > amounts.original) {
        throw std::invalid_argument("Refund exceeds original payment");
    }
    const bool full = accepted == amounts.original;
    const Exact fraction = accepted / amounts.original;
    const Exact exact = fraction * original_actual_credit_units;
    std::int64_t cumulative;
    switch (unit) {
    case RequestedUnit::milliseconds:
    case RequestedUnit::micro_points:
        cumulative = full ? original_actual_credit_units : round_effect_units(exact);
        break;
    case RequestedUnit::whole_count:
        cumulative = full ? original_actual_credit_units : 0;
        break;
    default:
        throw std::invalid_argument("Unknown refund unit");
    }
    if (previous_cumulative_units > cumulative) {
        throw std::invalid_argument("Prior refund progress exceeds cumulative entitlement");
    }
    return {fraction,           exact, cumulative, previous_cumulative_units - cumulative,
            exact - cumulative, full};
}

RefundTimeEntitlement calculate_refund_time_entitlement(const RefundAmounts &amounts,
                                                        RefundTimeCredit original,
                                                        RefundTimeCredit previous) {
    if (original.main_ms < 0 || original.debt_repayment_ms < 0 || previous.main_ms < 0 ||
        previous.debt_repayment_ms < 0 || previous.main_ms > original.main_ms ||
        previous.debt_repayment_ms > original.debt_repayment_ms ||
        (previous.debt_repayment_ms != 0 && previous.main_ms != original.main_ms)) {
        throw std::invalid_argument("Invalid Main-first refund allocation");
    }
    if (original.debt_repayment_ms > std::numeric_limits<std::int64_t>::max() - original.main_ms) {
        throw std::overflow_error("Original time credit exceeds signed 64-bit range");
    }
    auto total = calculate_refund_entitlement(amounts, RequestedUnit::milliseconds,
                                              original.main_ms + original.debt_repayment_ms,
                                              previous.main_ms + previous.debt_repayment_ms);
    const auto main = std::min(original.main_ms, total.cumulative_units);
    const auto restoration = total.cumulative_units - main;
    return {std::move(total),
            {main, restoration},
            previous.main_ms - main,
            restoration - previous.debt_repayment_ms};
}
} // namespace subathon::core
