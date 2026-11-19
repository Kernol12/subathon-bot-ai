// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <stdexcept>
#include <subathon/core/application_plan.hpp>
#include <subathon/core/exact_decimal.hpp>

namespace subathon::core {
struct RefundAmounts {
    // Exact values in the original payment currency. The caller verifies currency,
    // correlation, identity and accepted history before proposing a new refund.
    Exact original;
    Exact previously_accepted;
    Exact incoming;
};

struct RefundEntitlement {
    Exact cumulative_fraction;
    Exact exact_entitlement;          // Original actual credit times the cumulative fraction.
    std::int64_t cumulative_units;    // Positive magnitude, or zero.
    std::int64_t requested_increment; // Signed adjustment, nonpositive.
    Exact rounding_residual;          // Exact entitlement minus cumulative_units.
    bool full_refund;
};

class RefundEntitlementNotImplemented final : public std::logic_error {
  public:
    RefundEntitlementNotImplemented()
        : std::logic_error(
              "Refund entitlement is intentionally unimplemented; review tests first") {}
};

// ACCT-006 revision 2 draft. The caller projects a selected original credited
// component and its committed cumulative requested reversal units. Use actual
// credit after caps. For time that originally repaid debt, use the allocation
// boundary below. Prior units come from reversal progress, independent of actual
// timer subtraction. Keep the original basis identity and provenance with the caller.
//
// Round the cumulative proportional time/score entitlement half to even in the
// credited unit, then subtract the previous cumulative units to obtain the new
// signed adjustment. Whole counts have cumulative units zero until full refund.
// A full refund clears exactly the original credit less those previous units.
//
// Invalid amounts (nonpositive original/incoming, negative prior monetary total,
// or total exceeding original) or invalid unit/component data (negative actual
// credit or prior units outside [0, new cumulative]) throw std::invalid_argument.
// Partial time/score final conversion follows round_effect_units. A proposal grants no
// authority and expresses no actual timer subtraction, debt, or hold outcome.
[[nodiscard]] RefundEntitlement
calculate_refund_entitlement(const RefundAmounts &amounts, RequestedUnit unit,
                             std::int64_t original_actual_credit_units,
                             std::int64_t previous_cumulative_units);

struct RefundTimeCredit {
    std::int64_t main_ms;
    std::int64_t debt_repayment_ms;
};

struct RefundTimeEntitlement {
    RefundEntitlement total;
    RefundTimeCredit cumulative;                // Nonnegative reversal magnitudes, Main first.
    std::int64_t requested_main_adjustment_ms;  // Nonpositive.
    std::int64_t requested_debt_restoration_ms; // Nonnegative; restores repaid debt.
};

// Use original actual Main added after caps plus actual debt repaid as one time
// basis. Keep the exact cumulative fraction, entitlement and rounding residual
// on the total. Round that total once; allocate the rounded magnitude to original
// Main first, then to original debt repayment. Subtract each prior cumulative
// allocation from its new allocation to obtain the two requests. Full refund
// clears both original amounts less their respective prior allocations.
//
// Previous allocations are committed request progress, including held or unapplied
// requests. They must fit the new split and can include debt repayment only once
// all original Main was allocated. Invalid amounts, negative components or invalid
// prior allocations throw std::invalid_argument. An original sum outside signed
// 64-bit range throws std::overflow_error. Neither request changes a live balance
// or selects a deficit, lifecycle or hold disposition.
[[nodiscard]] RefundTimeEntitlement
calculate_refund_time_entitlement(const RefundAmounts &amounts, RefundTimeCredit original,
                                  RefundTimeCredit previous_cumulative);
} // namespace subathon::core
