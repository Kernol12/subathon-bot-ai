// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace subathon::core {
// Numeric Main balance only. Grace and lifecycle are separate caller-owned state.
struct TimeBalance {
    std::int64_t main_remaining_ms{};
    std::int64_t time_debt_ms{};
};

enum class RefundDeficitPolicy { StopAtZero, CarryTimeDebt, RequireReview };
enum class TimeReversalDisposition { Applied, RequiresReview };

// Opaque references identify retained caller records; no original award is recomputed.
struct TimeReversalRequest {
    std::string reversal_ref;
    std::string original_effect_ref;
    std::int64_t requested_adjustment_ms{}; // Nonpositive, including INT64_MIN.
};

struct TimeReversalResult {
    TimeReversalRequest request;
    RefundDeficitPolicy policy;
    TimeReversalDisposition disposition;
    TimeBalance balance_after;
    std::int64_t main_adjustment_ms{};      // Nonpositive actual Main change.
    std::int64_t debt_added_ms{};           // Nonnegative additional debt.
    std::int64_t unapplied_adjustment_ms{}; // Nonpositive clamped/held remainder.
};

struct TimeAdditionRequest {
    std::string addition_ref;
    std::int64_t requested_ms{}; // Nonnegative already-eligible whole milliseconds.
};

struct TimeAdditionResult {
    TimeAdditionRequest request;
    TimeBalance balance_after;
    std::int64_t debt_repaid_ms{};
    std::int64_t main_added_ms{};
    std::int64_t cap_limited_ms{};
};

class TimeDebtNotImplemented final : public std::logic_error {
  public:
    TimeDebtNotImplemented()
        : std::logic_error("Time debt is intentionally unimplemented; review tests first") {}
};

// Pure revision-1 proposals: docs/contracts/time-debt-v1.md. Invalid input throws
// invalid_argument; unrepresentable final balances throw overflow_error, with no
// proposal. The caller preserves the full request and commits no partial effects.
[[nodiscard]] TimeReversalResult plan_time_reversal(const TimeBalance &balance,
                                                    const TimeReversalRequest &request,
                                                    RefundDeficitPolicy policy);
[[nodiscard]] TimeAdditionResult
plan_time_addition(const TimeBalance &balance, const TimeAdditionRequest &request,
                   std::optional<std::int64_t> maximum_remaining_ms = std::nullopt);
} // namespace subathon::core
