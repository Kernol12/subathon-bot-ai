// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/core/reference_rule.hpp>
#include <subathon/server/sqlite_reference_authority.hpp>

namespace subathon::server {
// RULES-006 public-header contract revision 3, CONTRACT-014 sections1/4.
// Imported capture/authority types and their limits are not redefined here.
enum class ReferenceGate : std::uint8_t { not_ready, unsupported_configuration, denied, conflict };
struct ReferenceUnavailable {
    ReferenceGate gate = ReferenceGate::not_ready;
    std::string reason_ref;
};
struct ReferenceCategoryFacts {
    ReferenceFactsState state = ReferenceFactsState::unknown;
    std::vector<std::string> enabled_refs;
};
struct ReferenceEnabledRuleFacts {
    ReferenceRule rule;
    std::vector<std::string> trigger_types, action_kinds, action_units;
    std::vector<std::string> conditions, exclusions, execution_limits;
};
struct ReferenceConfigurationFacts {
    std::string configuration_ref, configuration_owner_revision_ref;
    ReferenceConfiguration projection;
    ReferenceFactsState rules_state = ReferenceFactsState::unknown;
    std::vector<ReferenceEnabledRuleFacts> enabled_rules;
    ReferenceCategoryFacts modifiers, milestones, outbound_actions, generated_events;
};
struct ReferenceAdmittedConfiguration {
    ReferenceConfiguration configuration;
    std::string configuration_owner_revision_ref;
};
struct ReferenceRuleEvaluation {
    core::RuleSelection selection;
    core::RequestedEffect request;
};
using ReferenceConfigurationResolution =
    std::variant<ReferenceConfigurationFacts, ReferenceUnavailable>;
using ReferenceAdmissionResolution =
    std::variant<ReferenceAdmittedConfiguration, ReferenceUnavailable>;
using ReferenceRuleResolution = std::variant<ReferenceRuleEvaluation, ReferenceUnavailable>;
// Trusted queue-owned dependency. #562 implements actual complete immutable version
// loading. A supplied DATA fixture or SRC1 decoder is not a production loader.
class ReferenceConfigurationSource {
  public:
    virtual ~ReferenceConfigurationSource() = default;
    virtual ReferenceConfigurationResolution snapshot(const std::string &configuration_ref) = 0;
    virtual ReferenceConfigurationResolution load(const ApplicationReferenceReadView &,
                                                  const std::string &configuration_ref) = 0;
};
class ReferenceApplicationNotImplemented final : public std::logic_error {
  public:
    ReferenceApplicationNotImplemented()
        : std::logic_error(
              "Reference application intentionally unimplemented; review tests first") {}
};
// Healthy semantic consumer failure: DOES NOT claim the reader already rolled back.
// Existing service catch-all must perform its one whole-unit cleanup. Distinct from
// original SqliteWorkFailure and callback lifetime/thread failures, which propagate.
class ReferenceExecutionFailure final : public std::exception {
  public:
    explicit ReferenceExecutionFailure(ReferenceAuthorityFailure code) noexcept : code_(code) {}
    ReferenceAuthorityFailure code() const noexcept { return code_; }
    const char *what() const noexcept override { return "Reference execution facts rejected"; }

  private:
    ReferenceAuthorityFailure code_;
};
// Fully borrowed complete input contract: positive representable AdapterLimits;
// B/I rules from CONTRACT-014 section2 (all strings payload bytes; enums/bools1,
// i64/u64=8, optional1+payload; vector size contributes I recursively, structs sum
// EVERY field/copy). All input, intermediate and output values independently fit
// value_owned_bytes/value_items. Per-field reference/payload/document limits also
// apply. Variants charge only the held alternative, with no wrapper/tag bytes.
// Admission charges expected_configuration_ref PLUS resolution; evaluation charges
// arrival PLUS resolution. Authority result charges the held Facts/failure only.
// Limits and borrowed dependency objects are excluded from value argument charges.
// Check every losing or unsupported member and source result BEFORE
// cloning. Limits must fit SIZE_MAX/PTRDIFF_MAX/qsizetype/qint64/container max_size;
// framed lengths fit U32. Capacity is separately bounded by relevant max_size;
// never reserve caller capacity or expand count*size unchecked. Imported authority,
// service and permission inputs retain their OWN complete preflight/work bounds.
// Priority: limits -> whole input B/I/per-field -> all tags -> required refs ->
// conditional shapes/duplicates -> numeric validity -> bindings -> profile refusal
// -> whole output projection -> allocation. Own allocation errors map to existing
// ReferenceInputFailure(allocation_failure). No eager clone/sort before validation.
// Pairwise borrowed duplicate checks O(I^2 * reference_bytes), validation O(B+I),
// fixed-depth stack, no variable owning cache. Not a constant-work/64-item promise.
// Result B/I follows declared fields, including canonical Exact strings. Existing
// capture codecs retain their depth3/64-index-record/512-byte scratch contract.
// Admission checks ALL facts, even later malformed members after unsupported ones.
// Unknown tags require empty collections; projection complete contradicting unknown
// facts is invalid_binding. Duplicate refs are invalid_shape. Expected/facts/
// projection refs match, owner revision nonempty, sole rule equals projection.
// Valid complete zero/multiple rules, nonempty categories, shape other than singleton
// synthetic.reference.support.v1/time_add/ms, conditions/exclusions/limits, deadline
// or non-synthetic origin -> unsupported_configuration/reference.configuration-unsupported.v1.
// Unknown complete-category knowledge -> not_ready/reference.configuration-unavailable.v1.
// Only not_ready/unsupported source gates are valid; preserve exact source refusal.
// Negative add/cap, invalid TimerConfig/tags remain input failures BEFORE refusal.
[[nodiscard]] ReferenceAdmissionResolution
admit_reference_configuration(const std::string &expected_configuration_ref,
                              const ReferenceConfigurationResolution &,
                              const ReferenceAdapterLimits & = {});
// Whole arrival preflight/admission first; admitted config must equal EVERY retained
// arrival config field. Never replace captured r1 by current r2. Payload is audit
// only (nonnull, possibly empty); fixed synthetic observed type, not parsed awards.
// Successful request/provenance is exactly derive_reference_rule's matched result.
[[nodiscard]] ReferenceRuleResolution
evaluate_reference_rule(const ReferenceArrivalCapture &, const ReferenceConfigurationResolution &,
                        const ReferenceAdapterLimits & = {});
// Small EXECUTION consumer only; full ExecutionAccess/disclosure/runtime/policy/
// clock/actions/entropy adapters remain #562. Borrow dependencies synchronously,
// no caching, writes, nested work, connection/session/SQL/network or public permits.
// Command service preflight uses ApplicationServiceLimits defaults additionally.
// Caller authority/nested limits are validated before source-error translation or
// callback access; their ReferenceAuthorityInputFailure is preserved unchanged.
// The temporary validating ApplicationEnvelope counts format_ref + document bytes
// against value_owned_bytes (I=0), format_ref against reference_bytes and document
// against document_bytes. Project all three BEFORE encoder allocation; service
// defaults apply additionally. An overage returns Unknown before source callbacks.
// Decode exactly one SRF1, read SRX1/SRA1/SRC1 with exact key/link/format bindings,
// load complete captured config through trusted source, admit and derive full amount.
// Validate all command/principal/installation/source/receipt/CID(including0)/plan/
// rule/action/both versions/timer/capability/amount duplications before authority.
// Ask source anew for execute_time_add and THIS operation, then validate returned
// echo, authorized registry, immutable binding, exact author/subject mapping,
// nonzero ordered created<=state<=head revisions, nonempty revision provenance,
// matching snapshots/catalog/owner. Distinct identities are allowed, not required.
// For authorized facts, automation_revision_ref must match the registry's canonical
// RAA1 fingerprint (reference-authority-v1); both snapshots equal permission revision.
// Known pending/revoked registry states deny before fingerprint/policy re-evaluation.
// Exact compiled reference catalog and whole concrete RSS1 scope/amount/1 required;
// never trust source success, union/intersect policies or use cap-limited credit.
// Re-run revalidate_permission_automation (author USE AND GRANT) and independent
// evaluate_effective_permission (execution subject USE), using authority.permissions.
// All missing/unavailable/resource_limit -> Unknown; denied/inactive -> Denied;
// known contradictory/malformed facts, empty/disagreeing revisions and impossible
// resolve failures (stale, collision, out-of-range) ->
// ReferenceExecutionFailure(invalid_data), never a fake RetentionFailure. Valid raw
// policy non-permit -> Denied; unknown policy -> Unknown; invalid policy -> invalid_data.
// Allowed returns EXACT freshly supplied permission_revision_ref. No old permit input.
// Oversized source facts or nested resource refusal -> Unknown, before copies.
// Source RefInput/known-format semantic failures -> execution invalid_data; source
// original SqliteWorkFailure/ApplicationReferenceViewFailure propagate unchanged.
// Nonallow returns before service protected lookup/policy/clock; whole-unit cleanup
// and publication are owned by existing service, not this helper. Each source result
// is preflighted against authority.result_bytes/items and nested limits BEFORE copy,
// expansion, exact numeric work or re-evaluation. Checked RSS1 length16+three fields
// fits authority.permissions.reference_bytes BEFORE construction. No unsound work
// bound: nested permission evaluators retain their documented L^4 symbolic bound.
[[nodiscard]] AccessDecision check_reference_execution(const ApplicationReferenceReadView &,
                                                       const OriginalApplicationCommand &,
                                                       ReferenceConfigurationSource &,
                                                       ReferenceAuthoritySource &,
                                                       const ReferenceAdapterLimits & = {},
                                                       const ReferenceAuthorityLimits & = {});
} // namespace subathon::server
