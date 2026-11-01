// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/time_debt.hpp>

#include <algorithm>
#include <limits>

namespace subathon::core {
namespace {
constexpr auto maximum = std::numeric_limits<std::int64_t>::max();

void validate_balance(const TimeBalance &balance) {
    if (balance.main_remaining_ms < 0 || balance.time_debt_ms < 0)
        throw std::invalid_argument("Main and time debt must be nonnegative");
}

bool valid_policy(RefundDeficitPolicy policy) {
    return policy == RefundDeficitPolicy::StopAtZero ||
           policy == RefundDeficitPolicy::CarryTimeDebt ||
           policy == RefundDeficitPolicy::RequireReview;
}
} // namespace

TimeReversalResult plan_time_reversal(const TimeBalance &balance,
                                      const TimeReversalRequest &request,
                                      RefundDeficitPolicy policy) {
    validate_balance(balance);
    if (request.requested_adjustment_ms > 0 || request.reversal_ref.empty() ||
        request.original_effect_ref.empty() || !valid_policy(policy))
        throw std::invalid_argument("Invalid reversal request or deficit policy");

    const auto main = balance.main_remaining_ms;
    const auto adjustment = request.requested_adjustment_ms;
    // Main is nonnegative, so -Main is representable even for a full-range request.
    if (adjustment >= -main) {
        return {request,
                policy,
                TimeReversalDisposition::Applied,
                {main + adjustment, balance.time_debt_ms},
                adjustment,
                0,
                0};
    }
    if (policy == RefundDeficitPolicy::RequireReview) {
        return {request, policy,    TimeReversalDisposition::RequiresReview, balance, 0,
                0,       adjustment};
    }

    // Adding available Main to a nonpositive request cannot overflow. Keeping the
    // deficit signed lets Stop/Review retain INT64_MIN without unsafe negation.
    const auto deficit = adjustment + main;
    if (policy == RefundDeficitPolicy::StopAtZero) {
        return {request, policy, TimeReversalDisposition::Applied, {0, balance.time_debt_ms}, -main,
                0,       deficit};
    }

    const auto debt_headroom = maximum - balance.time_debt_ms;
    if (deficit < -debt_headroom)
        throw std::overflow_error("Time debt would exceed signed 64-bit range");
    // The headroom check proves deficit >= -INT64_MAX before negation/addition.
    const auto debt_added = -deficit;
    return {request,
            policy,
            TimeReversalDisposition::Applied,
            {0, balance.time_debt_ms + debt_added},
            -main,
            debt_added,
            0};
}

TimeAdditionResult plan_time_addition(const TimeBalance &balance,
                                      const TimeAdditionRequest &request,
                                      std::optional<std::int64_t> maximum_remaining_ms) {
    validate_balance(balance);
    if (request.requested_ms < 0 || request.addition_ref.empty() ||
        (maximum_remaining_ms && *maximum_remaining_ms < 0))
        throw std::invalid_argument("Invalid time addition or Main cap");

    const auto debt_repaid = std::min(balance.time_debt_ms, request.requested_ms);
    const auto remainder = request.requested_ms - debt_repaid;
    std::int64_t main_added;
    if (maximum_remaining_ms) {
        // A lowered cap never reduces existing Main or blocks debt repayment.
        const auto headroom = balance.main_remaining_ms >= *maximum_remaining_ms
                                  ? 0
                                  : *maximum_remaining_ms - balance.main_remaining_ms;
        main_added = std::min(remainder, headroom);
    } else {
        if (remainder > maximum - balance.main_remaining_ms)
            throw std::overflow_error("Main time would exceed signed 64-bit range");
        main_added = remainder;
    }
    return {request,
            {balance.main_remaining_ms + main_added, balance.time_debt_ms - debt_repaid},
            debt_repaid,
            main_added,
            remainder - main_added};
}
} // namespace subathon::core
