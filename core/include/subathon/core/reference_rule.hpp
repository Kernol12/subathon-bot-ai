// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/core/application_plan.hpp>

namespace subathon::core {
// RULES-006 public-header contract revision 1. Pure configured-integer derivation;
// no admission, currentness, authority, event parsing, valuation or actual credit.
struct ReferenceRuleInput {
    std::string rule_ref, rule_version_ref, action_ref, action_version_ref, timer_ref;
    bool enabled = true;
    std::string trigger_type, observed_type, action_kind, action_unit;
    std::int64_t add_ms = 0;
};
struct ReferenceRuleLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t value_owned_bytes = 1048576;
    std::uint64_t value_items = 4096;
};
enum class ReferenceDerivationError : std::uint8_t {
    invalid_limits,
    input_limit,
    invalid_reference,
    invalid_value,
    result_limit,
    allocation_failure
};
class ReferenceDerivationFailure final : public std::exception {
  public:
    explicit ReferenceDerivationFailure(ReferenceDerivationError code) noexcept : code_(code) {}
    ReferenceDerivationError code() const noexcept { return code_; }
    const char *what() const noexcept override { return "Reference rule derivation failed"; }

  private:
    ReferenceDerivationError code_;
};
enum class ReferenceDerivationState : std::uint8_t { matched, disabled, nonmatch, unsupported };
struct ReferenceRuleDerivation {
    ReferenceDerivationState state = ReferenceDerivationState::unsupported;
    RuleSelection selection;
    std::optional<RequestedEffect> request;
};
class ReferenceRuleNotImplemented final : public std::logic_error {
  public:
    ReferenceRuleNotImplemented()
        : std::logic_error("Reference rule intentionally unimplemented; review tests first") {}
};
// Borrowed input: B=sum all string bytes + bool1 + i64=8; I=0. Every string is
// required and <=reference_bytes, including unsupported/disabled/nonmatch inputs.
// Positive limits must fit size_t, ptrdiff_t and relevant string/container max_size.
// Priority: invalid limits -> whole per-field/B/I preflight -> required refs ->
// negative amount -> supported profile -> disabled -> nonmatch -> result projection.
// Profile: trigger synthetic.reference.support.v1, action time_add, unit ms. Any
// nonempty observed type is valid; its mismatch produces no request. Unsupported
// profile produces no request. Selection always retains rule/version; selected
// iff matched. Explanations: reference.single-rule.{match,disabled,nonmatch,unsupported}.v1.
// Matched request has exact add_ms/1, integer add_ms (including0 and INT64_MAX),
// milliseconds, no historical valuation, copied action/version/rule/timer,
// reference.integer-ms.v1 and reference.original-time.v1. No rounding/history is
// fabricated; exact minus integer residual is0. No caller-resolved award input.
// Output B=sum declared fields recursively: enum1, selection bool1, optional1,
// int64=8, RequestedUnit1, historical bool1; Exact canonical numerator/denominator
// byte lengths. I=0. Input AND complete output independently fit value budgets.
// Project output before any owned output/Exact construction; bounded decimal
// workspace <=20 numerator bytes +1 denominator byte (backend capacity is separate).
// Borrowed scans O(B), fixed field count, no variable owning scratch; output string
// sizes/all explicit reserve requests <=projected B and each container max_size.
// Borrowed input capacities are ignored. Fixed result objects/standard-library SSO,
// spare capacity and allocator metadata are outside semantic B; no caller capacity
// is cloned. Own bad_alloc/length_error -> allocation_failure; caller allocations
// excluded. No input mutation. These costs require later source audit.
[[nodiscard]] ReferenceRuleDerivation derive_reference_rule(const ReferenceRuleInput &,
                                                            const ReferenceRuleLimits & = {});
} // namespace subathon::core
