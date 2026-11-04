// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/effective_permissions.hpp>

#include <utility>

namespace subathon::core {

EffectivePermissionDecision::EffectivePermissionDecision(EffectivePermissionDecisionData data)
    : data_(std::move(data)) {}

const EffectivePermissionDecisionData &EffectivePermissionDecision::data() const noexcept {
    return data_;
}

EffectivePermissionFailure::EffectivePermissionFailure(EffectivePermissionError code) noexcept
    : code_(code) {}

EffectivePermissionError EffectivePermissionFailure::code() const noexcept { return code_; }

const char *EffectivePermissionFailure::what() const noexcept {
    return "Effective permission evaluation failed";
}

EffectivePermissionsNotImplemented::EffectivePermissionsNotImplemented()
    : std::logic_error(
          "Effective permission behavior is intentionally unimplemented; review tests first") {}

// Baseline stub: every raw input reaches the distinct marker without validation.
EffectivePermissionDecision evaluate_effective_permission(const EffectivePermissionInput &,
                                                          const EffectivePermissionLimits &) {
    throw EffectivePermissionsNotImplemented{};
}

} // namespace subathon::core
