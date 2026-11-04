// SPDX-License-Identifier: GPL-3.0-or-later
// Approved effective-permissions API v1 (dda2793c); raw facts confer no authority.
#pragma once

#include <cstdint>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace subathon::core {

enum class PermissionFactsState : std::uint8_t { unknown, complete };
enum class PermissionOwnerState : std::uint8_t { unknown, absent, present };
enum class PermissionOperationClass : std::uint8_t {
    ordinary,
    protected_access_management,
    protected_recovery,
    helper_password_reset
};
enum class PermissionNumberKind : std::uint8_t { rational, integer };
enum class PermissionResourceMode : std::uint8_t { any, allowed_set };
enum class PermissionQuantityMode : std::uint8_t {
    any,
    at_most,
    closed_interval,
    magnitude_at_most
};
enum class PermissionOrigin : std::uint8_t { individual, role };
enum class PermissionEffect : std::uint8_t { allow, deny };

struct PermissionRational {
    std::string numerator = "0";
    std::string denominator = "1";
};
struct PermissionQuantityDefinition {
    std::string slot;
    std::string unit;
    PermissionNumberKind kind = PermissionNumberKind::rational;
    std::optional<PermissionRational> minimum;
    std::optional<PermissionRational> maximum;
    std::uint8_t allowed_modes = 15;
};
struct PermissionOperationDefinition {
    std::string operation;
    PermissionOperationClass classification = PermissionOperationClass::ordinary;
    std::vector<std::string> resource_slots;
    std::vector<PermissionQuantityDefinition> quantity_slots;
    std::optional<std::string> helper_target_slot;
};
struct PermissionCatalog {
    PermissionFactsState state = PermissionFactsState::unknown;
    std::string revision;
    std::vector<PermissionOperationDefinition> operations;
};
struct PermissionOwnerFacts {
    PermissionOwnerState state = PermissionOwnerState::unknown;
    std::optional<std::string> account;
};
struct PermissionResourceBinding {
    std::string slot;
    std::string resource;
};
struct PermissionQuantityBinding {
    std::string slot;
    std::string unit;
    PermissionRational value;
};
struct PermissionRequest {
    std::string operation;
    std::vector<PermissionResourceBinding> resources;
    std::vector<PermissionQuantityBinding> quantities;
};
struct PermissionResourceConstraint {
    std::string slot;
    PermissionResourceMode mode = PermissionResourceMode::any;
    std::vector<std::string> allowed;
};
struct PermissionQuantityConstraint {
    std::string slot;
    std::string unit;
    PermissionQuantityMode mode = PermissionQuantityMode::any;
    std::optional<PermissionRational> lower;
    std::optional<PermissionRational> upper;
};
struct PermissionPolicyEntry {
    PermissionOrigin origin = PermissionOrigin::individual;
    std::string source;
    std::optional<std::string> role;
    std::string operation;
    PermissionEffect effect = PermissionEffect::allow;
    std::optional<std::string> grant;
    std::vector<PermissionResourceConstraint> resources;
    std::vector<PermissionQuantityConstraint> quantities;
};
struct PermissionPolicyFacts {
    PermissionFactsState state = PermissionFactsState::unknown;
    std::vector<std::string> assigned_roles;
    std::vector<PermissionPolicyEntry> entries;
};
struct EffectivePermissionInput {
    std::string actor;
    std::string snapshot;
    PermissionCatalog catalog;
    PermissionOwnerFacts owner;
    PermissionPolicyFacts policy;
    PermissionRequest request;
};
struct EffectivePermissionLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t component_bytes = 16384;
    std::uint64_t input_items = 4096;
    std::uint64_t input_owned_bytes = 16777216;
    std::uint64_t result_items = 4096;
    std::uint64_t result_owned_bytes = 33554432;
};

enum class PermissionOutcome : std::uint8_t { deny, permit };
enum class PermissionReason : std::uint8_t {
    default_deny,
    individual_allow,
    individual_deny,
    individual_limit,
    role_allow,
    role_deny,
    protected_owner,
    protected_owner_target
};
enum class PermissionSourceKind : std::uint8_t { protected_owner, individual, role };
enum class PermissionMatch : std::uint8_t {
    other_operation,
    resource_mismatch,
    quantity_mismatch,
    complete
};
enum class PermissionSourceDisposition : std::uint8_t {
    not_applicable,
    suppressed,
    quantity_limit,
    permit_witness,
    fitting_allow,
    decisive_deny
};
struct PermissionSourceExplanation {
    PermissionSourceKind kind = PermissionSourceKind::individual;
    std::string source;
    std::optional<std::string> grant;
    std::optional<std::string> role;
    std::string operation;
    PermissionEffect effect = PermissionEffect::allow;
    PermissionMatch match = PermissionMatch::other_operation;
    PermissionSourceDisposition disposition = PermissionSourceDisposition::not_applicable;
    std::vector<std::string> failed_resources;
    std::vector<std::string> failed_quantities;
};
struct EffectivePermissionDecisionData {
    std::string actor;
    std::string snapshot;
    std::string catalog_revision;
    PermissionOwnerFacts owner;
    PermissionRequest request;
    PermissionOutcome outcome = PermissionOutcome::deny;
    PermissionReason reason = PermissionReason::default_deny;
    std::optional<std::uint64_t> witness_row;
    std::vector<PermissionSourceExplanation> sources;
};

class EffectivePermissionDecision;
[[nodiscard]] EffectivePermissionDecision
evaluate_effective_permission(const EffectivePermissionInput &input,
                              const EffectivePermissionLimits &limits = {});

class EffectivePermissionDecision final {
  public:
    EffectivePermissionDecision(const EffectivePermissionDecision &) = default;
    EffectivePermissionDecision(EffectivePermissionDecision &&) noexcept = default;
    EffectivePermissionDecision &operator=(const EffectivePermissionDecision &) = default;
    EffectivePermissionDecision &operator=(EffectivePermissionDecision &&) noexcept = default;
    [[nodiscard]] const EffectivePermissionDecisionData &data() const noexcept;

  private:
    explicit EffectivePermissionDecision(EffectivePermissionDecisionData data);
    EffectivePermissionDecisionData data_;
    friend EffectivePermissionDecision
    evaluate_effective_permission(const EffectivePermissionInput &,
                                  const EffectivePermissionLimits &);
};

enum class EffectivePermissionError : std::uint8_t {
    invalid_limits,
    input_limit,
    invalid_reference,
    invalid_catalog,
    invalid_facts,
    invalid_request,
    invalid_policy,
    contradictory_individual,
    invalid_number,
    facts_unavailable,
    result_limit,
    allocation_failure
};
class EffectivePermissionFailure final : public std::exception {
  public:
    explicit EffectivePermissionFailure(EffectivePermissionError code) noexcept;
    [[nodiscard]] EffectivePermissionError code() const noexcept;
    [[nodiscard]] const char *what() const noexcept override;

  private:
    EffectivePermissionError code_;
};
class EffectivePermissionsNotImplemented final : public std::logic_error {
  public:
    EffectivePermissionsNotImplemented();
};
} // namespace subathon::core
