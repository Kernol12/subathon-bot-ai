// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/time_debt.hpp>

namespace subathon::core {
// Explicit baseline stubs: no validation, policy, balance or lifecycle behavior.
TimeReversalResult plan_time_reversal(const TimeBalance &, const TimeReversalRequest &,
                                      RefundDeficitPolicy) {
    throw TimeDebtNotImplemented{};
}

TimeAdditionResult plan_time_addition(const TimeBalance &, const TimeAdditionRequest &,
                                      std::optional<std::int64_t>) {
    throw TimeDebtNotImplemented{};
}
} // namespace subathon::core
