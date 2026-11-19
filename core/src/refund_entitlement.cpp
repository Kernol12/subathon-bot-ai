// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/refund_entitlement.hpp>

namespace subathon::core {
RefundEntitlement calculate_refund_entitlement(const RefundAmounts &, RequestedUnit, std::int64_t,
                                               std::int64_t) {
    throw RefundEntitlementNotImplemented{};
}

RefundTimeEntitlement calculate_refund_time_entitlement(const RefundAmounts &, RefundTimeCredit,
                                                        RefundTimeCredit) {
    throw RefundEntitlementNotImplemented{};
}
} // namespace subathon::core
