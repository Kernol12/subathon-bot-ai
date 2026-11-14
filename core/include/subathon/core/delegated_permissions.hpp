// SPDX-License-Identifier: GPL-3.0-or-later
// ACCESS-003 proposed public contract v2; tests-stage declarations only.
#pragma once

#include <subathon/core/effective_permissions.hpp>

namespace subathon::core {

enum class PermissionDelegationContext : std::uint8_t {
    role_edit,
    invitation,
    individual_override,
    // Reserved audit label only: automation channel/lifetime await owner Q3/Q4.
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
    // Only the owner configures these facts through a separate protected path.
    // An administrator cannot delegate GRANT authority. Role membership and
    // individual precedence reuse the existing API semantics (design choice).
    PermissionPolicyFacts delegation;
    PermissionDelegationContext context = PermissionDelegationContext::role_edit;
    // Opaque audit reference only; context/target do not select authority or
    // constrain resource slots. A helper reset target is its catalog-named slot.
    // Equal actor/target does not prove self-edit impact closure.
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
    // individual/role precedence. Separate complete witnesses may cover different
    // requests in one proposal (design choice); never mix tuple dimensions.
    // Finite endpoint sampling is insufficient.
    std::vector<PermissionOutcome> grants;
    PermissionOutcome outcome = PermissionOutcome::deny; // All grants must fit.
};

// Pure, supplied-fact calculation. No mutation, session authentication, currentness
// proof, owner-configuration write, or executable authorization token is produced.
// Recompute from current authoritative facts at saving/execution, never reuse a
// preview. Twitch identity/category facts deliberately have no authority input.
// Invalid/unknown facts and malformed capabilities use EffectivePermissionFailure.
// Validation follows effective-permissions-api-v1 stages 0-9, replacing request
// shape/domain validation by validation of EVERY proposed complete tuple (errors
// invalid_policy). Unknown delegation always fails facts_unavailable: even the
// owner has no implicit GRANT bypass. Protected access-management/recovery proposals
// are ordinary denials, regardless of supplied allows or actor ownership. A helper
// reset proposal containing the current owner in its catalog-named target slot
// (including Any) is denied in full, even for an owner actor. Other helper targets
// use normal containment. No ownership or GRANT configuration can be edited here.
//
// Exact semantic B/I accounting (not heap/RSS), for EVERY declared field:
// string B=size/I=0; enum B=1/I=0; optional B=1+present payload B,
// I=present payload I; vector B=sum(element B), I=size+sum(element I).
// Structs sum fields; rational sums numerator/denominator strings, no extra tag.
// Thus input B/I = actor + snapshot + catalog + owner + delegation + context
// + target + proposed, each using those recursive formulas and the field types in
// effective_permissions.hpp. Field expansions (both B and I use these sums):
// catalog=state+revision+operations; operation=operation+classification+
// resource_slots+quantity_slots+helper_target_slot; quantity definition=slot+unit+
// kind+minimum+maximum+allowed_modes; owner=state+account;
// delegation=state+assigned_roles+entries; entry=origin+source+role+operation+
// effect+grant+resources+quantities; proposal=operation+resources+quantities;
// resource constraint=slot+mode+allowed; quantity constraint=slot+unit+mode+
// lower+upper. Vector element counts include allowed members, slot definitions,
// assigned roles, rows, constraints and proposals; optionals add no item alone.
// All references <=reference_bytes, each numeric component <=component_bytes.
// Result B = actor.size + snapshot.size + catalog.revision.size + target.size
// + proposed.size (one outcome byte each) + 1 (aggregate outcome); I=proposed.size.
// Limits themselves are uncharged. Stage 1 borrows/preflights the WHOLE input
// against input_owned_bytes/input_items before copying, sorting or numeric work.
// Stage 8 projects the WHOLE result against result_owned_bytes/result_items and
// destination max_size before materializing; no partial permit or truncation.
// Overflow yields input_limit/result_limit respectively. Allocation failures map
// to allocation_failure. Reference/numeric validation and numeric scratch bounds
// reuse effective-permissions-api-v1 section 5, including its stage-0 checks.
//
// Additional checked symbolic work bound: L=input_items; stage 0 requires L^2,
// L^4, 128*L and 16*component_bytes+128*L representable in SIZE_MAX and
// PTRDIFF_MAX (else invalid_limits).
// Build shared partitions per operation from ALL same-operation policy/proposal
// tuples: each resource dimension uses distinct explicit references plus ONE other
// class. Each quantity dimension uses distinct finite policy/proposal endpoints
// (magnitude bounds contribute both signs) AND catalog domain endpoints. Partition
// into endpoint singletons and open gaps/tails, intersect with catalog domain and
// each proposal separately, discarding empty regions. Integer dimensions retain only integer
// points/nonempty integer gaps (ceil/floor exactly); rational gaps stay dense.
// A zero-dimension product has one cell. Checked sum of Cartesian cell counts over
// ALL proposals must be <=L^2; overflow/excess is input_limit, before cell storage
// or enumeration. This projection follows semantic validation, before stage 8.
// Stream cells, never materialize the product. Scratch is at most 16*input I
// eight-byte borrowed index/counter records (128*input I bytes), including signed
// endpoint views and traversal stacks; no owned endpoint strings/ratios cache.
// Numeric scratch is separately bounded by 16*component_bytes as in the API.
// At most L^2 cells * input I rows * input I slots <=L^4 tuple-slot comparisons;
// sorting/validation/projection use O(input I^2) comparisons. Numeric comparisons
// retain the bounded exact-integer strategy of the existing API, one at a time.
// No endpoint sampling shortcut: a nonempty gap is a distinct cell. Source review
// must verify these work/allocation bounds; output tests alone cannot prove them.
//
// This is a GRANT containment primitive, not an automation authorization decision.
// The automation audit label neither selects USE/GRANT nor resolves Q3/Q4. Explicit
// action/sender containment is necessary coverage only; automation remains OPEN.
//
// The caller must separately authorize the management operation using USE rights.
// It must derive ALL newly exposed capabilities (including removal of restrictive
// overrides and affected role members), not just submitted allow rows. This API
// does not establish closure of that server-side change set or commit an edit.
// F1 remains required within ACCESS-003: this primitive alone is insufficient.
// Restrictive edits and untouched out-of-scope rows await owner Q1/Q2. Effective
// preview/source explanations (including denied proposals), real execution-time
// revalidation and identity-link invariance remain OPEN integration acceptance;
// calling the existing evaluator alone does not establish any of those behaviors.
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
