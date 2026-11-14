// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/delegated_permissions.hpp>

namespace subathon::core {
PermissionDelegationDecision evaluate_permission_delegation(const PermissionDelegationInput &,
                                                            const EffectivePermissionLimits &) {
    throw PermissionDelegationNotImplemented{};
}
PermissionEditDecision preview_permission_edit(const PermissionEditInput &,
                                               const EffectivePermissionLimits &) {
    throw PermissionDelegationNotImplemented{};
}
PermissionEditDecision revalidate_permission_edit(const PermissionEditInput &,
                                                  const EffectivePermissionLimits &) {
    throw PermissionDelegationNotImplemented{};
}
PermissionAutomationDecision authorize_permission_automation(const PermissionAutomationInput &,
                                                             const EffectivePermissionLimits &) {
    throw PermissionDelegationNotImplemented{};
}
PermissionAutomationDecision revalidate_permission_automation(const PermissionAutomationInput &,
                                                              const EffectivePermissionLimits &) {
    throw PermissionDelegationNotImplemented{};
}
} // namespace subathon::core
