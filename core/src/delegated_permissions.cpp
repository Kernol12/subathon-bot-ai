// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/delegated_permissions.hpp>

namespace subathon::core {
PermissionDelegationDecision evaluate_permission_delegation(const PermissionDelegationInput &,
                                                            const EffectivePermissionLimits &) {
    throw PermissionDelegationNotImplemented{};
}
} // namespace subathon::core
