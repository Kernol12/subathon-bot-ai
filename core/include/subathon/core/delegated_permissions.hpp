// SPDX-License-Identifier: GPL-3.0-or-later
// ACCESS-003 proposed public contract v1; tests-stage declarations only.
#pragma once

#include <subathon/core/effective_permissions.hpp>

namespace subathon::core {

enum class PermissionDelegationContext : std::uint8_t {
    role_edit,
    invitation,
    individual_override,
    automation_authorization
};

// One complete capability, not a concrete invocation or a sample of its range.
// Shapes, exact rationals, resource sets and quantity modes follow the existing
// effective-permission contract. Only allow capabilities are represented here.
struct DelegatedPermissionGrant {
    std::string operation;
    std::vector<PermissionResourceConstraint> resources;
    std::vector<PermissionQuantityConstraint> quantities;
};

struct PermissionDelegationInput {
    std::string actor;
    std::string snapshot;
    PermissionCatalog catalog;
    PermissionOwnerFacts owner;
    // Current owner-configured authority to GRANT, never the actor's use policy.
    // Role membership and individual precedence have the existing API semantics.
    PermissionPolicyFacts delegation;
    PermissionDelegationContext context = PermissionDelegationContext::role_edit;
    // Account/role/invitation/automation reference, according to context. Equal
    // actor and target references do not provide a self-edit exception.
    std::string target;
    std::vector<DelegatedPermissionGrant> proposed;
};

struct PermissionDelegationDecision {
    std::string actor;
    std::string snapshot;
    std::string catalog_revision;
    std::string target;
    // Input order. Permit iff EVERY request admitted by that proposed capability
    // is permitted by the delegation policy, using complete-grant matching and
    // individual/role precedence. Finite endpoint sampling is insufficient.
    std::vector<PermissionOutcome> grants;
    PermissionOutcome outcome = PermissionOutcome::deny; // All grants must fit.
};

// Pure, supplied-fact calculation. No mutation, session authentication, currentness
// proof, owner-configuration write, or executable authorization token is produced.
// Recompute from current authoritative facts at saving/execution, never reuse a
// preview. Twitch identity/category facts deliberately have no authority input.
// Invalid/unknown facts and malformed capabilities fail with the existing
// EffectivePermissionFailure vocabulary; they are not ordinary denials. Selected
// EffectivePermissionLimits bound the combined input/result, including proposals.
//
// The caller must separately authorize the management operation using USE rights.
// It must derive ALL newly exposed capabilities (including removal of restrictive
// overrides and affected role members), not just submitted allow rows. This API
// does not establish closure of that server-side change set or commit an edit.
// Concrete before/after effective previews use evaluate_effective_permission with
// the target account's before/after policy and retain its source explanations.
[[nodiscard]] PermissionDelegationDecision
evaluate_permission_delegation(const PermissionDelegationInput &input,
                               const EffectivePermissionLimits &limits = {});

class PermissionDelegationNotImplemented final : public std::logic_error {
  public:
    PermissionDelegationNotImplemented()
        : std::logic_error(
              "Delegation behavior is intentionally unimplemented; review tests first") {}
};
} // namespace subathon::core
