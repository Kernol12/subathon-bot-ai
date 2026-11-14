// SPDX-License-Identifier: GPL-3.0-or-later
// ACCESS-003 proposed public contract v3; tests-stage declarations only.
#pragma once

#include <subathon/core/effective_permissions.hpp>

namespace subathon::core {

enum class PermissionDelegationContext : std::uint8_t {
    role_edit,
    invitation,
    individual_override,
    // Audit label only; automation uses the dual-channel API below.
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
// This is a GRANT containment primitive. The edit and automation APIs below own
// impact closure, management USE and dual-channel automation respectively.
[[nodiscard]] PermissionDelegationDecision
evaluate_permission_delegation(const PermissionDelegationInput &input,
                               const EffectivePermissionLimits &limits = {});

// Complete editable graph, not caller-selected affected accounts. Role entries
// must have role origin and the enclosing role identity; account entries must be
// individual. All source/grant/shape rules of the effective API apply after binding.
// Duplicate role/account identities or unresolved memberships are invalid_policy.
// Unknown state has no payload and fails facts_unavailable. Both snapshots include
// ALL accounts, role definitions, memberships and overrides, even unrelated ones.
struct PermissionEditableRole {
    std::string role;
    std::vector<PermissionPolicyEntry> entries;
};
struct PermissionEditableAccount {
    std::string account;
    std::vector<std::string> assigned_roles;
    std::vector<PermissionPolicyEntry> individual_entries;
};
struct PermissionEditableState {
    PermissionFactsState state = PermissionFactsState::unknown;
    std::vector<PermissionEditableRole> roles;
    std::vector<PermissionEditableAccount> accounts;
};
struct PermissionEditProbe {
    std::string account;
    PermissionRequest request;
};
struct PermissionEditInput {
    std::string actor;
    std::string snapshot;
    PermissionCatalog catalog;
    PermissionOwnerFacts owner;
    PermissionPolicyFacts delegation; // Current owner-configured actor GRANT scope.
    // Trusted server mapping of the actual edit to its management USE operation.
    PermissionRequest management;
    PermissionEditableState before;
    PermissionEditableState after;
    std::vector<PermissionEditProbe> probes;
};
enum class PermissionEditSide : std::uint8_t { before, after };
struct PermissionChangedCapabilityDecision {
    std::string source;
    std::optional<std::string> grant;
    std::optional<std::string> role;
    PermissionEditSide side = PermissionEditSide::before;
    PermissionOutcome outcome = PermissionOutcome::deny;
};
struct PermissionAffectedAccountDecision {
    std::string account;
    PermissionOutcome newly_exposed = PermissionOutcome::deny;
};
struct PermissionEditProbeDecision {
    EffectivePermissionDecisionData before;
    EffectivePermissionDecisionData after;
};
struct PermissionEditDecision {
    std::string actor;
    std::string snapshot;
    std::string catalog_revision;
    PermissionOutcome outcome = PermissionOutcome::deny;
    EffectivePermissionDecisionData management;
    std::vector<PermissionChangedCapabilityDecision> changed;
    std::vector<PermissionAffectedAccountDecision> affected;
    std::vector<PermissionEditProbeDecision> previews;
};

// Owner Q1/Q2, issue #249, 2026-11-14: management USE AND delegation scope for
// restrictive edits too; untouched out-of-scope rows may remain. This API derives
// impact itself. Bind each account's individual entries plus assigned role rows.
// Actor must exist in before; management USE is evaluated from BEFORE, never from
// proposed self-grants. New/deleted accounts have an empty policy on the missing
// side (known absent, not unknown). Probes must name an account in either snapshot.
//
// Compare entries by (origin,source,grant,role), sets/slot vectors semantically,
// ignoring vector order. Every added/removed/modified row must fit GRANT in FULL
// on each existing side, treating its tuple as an allow capability even for denies.
// A modified row checks both old and new tuples; a genuinely untouched row does
// not need scope. Membership addition/removal similarly checks every tuple of the
// assigned/removed role on the corresponding side. Collapse duplicate row+side
// checks, sort by (source,grant,role,side), unsigned bytes, absent first. No need
// for management USE of each granted action: management USE and GRANT are distinct.
//
// Affected accounts are the union of changed/new/deleted account records and ALL
// before/after members of changed roles, including actor/self. For EACH account,
// symbolically derive effective(after) minus effective(before), with the existing
// individual-before-quantity and role-deny semantics. Every newly permitted request
// must fit GRANT. This includes removing/narrowing denies, override changes, role
// membership/invitation changes, and role edits which expose role fallback.
// Never ask a caller for a precomputed capability list or sampled requests.
// outcome permits iff management, every changed tuple and every exposed region
// permit (empty exposed region fits). affected sorted by account, previews in probe
// order. Previews retain FULL effective evaluator source explanations on both
// sides, including denials; they do not imply a save is authorized.
//
// preview and revalidation BOTH compute all of the above from raw facts; neither
// accepts an earlier decision as authority. Revalidation consumes newly loaded
// current before/after proposal, actor membership, owner, catalog and delegation.
// It performs no authentication, revision CAS, transaction or durable edit. The
// server must load a complete graph and map the management intent, then recheck
// session/current revisions atomically with commit. That producer/enforcement gate
// remains within ACCESS-003 integration scope, not proven by supplied-fact tests.
[[nodiscard]] PermissionEditDecision
preview_permission_edit(const PermissionEditInput &, const EffectivePermissionLimits & = {});
[[nodiscard]] PermissionEditDecision
revalidate_permission_edit(const PermissionEditInput &, const EffectivePermissionLimits & = {});

struct PermissionAutomationInput {
    std::string author;
    std::string snapshot;
    PermissionCatalog catalog;
    PermissionOwnerFacts owner;
    PermissionPolicyFacts use;
    PermissionPolicyFacts delegation;
    std::vector<DelegatedPermissionGrant> actions;
};
enum class PermissionAutomationDisposition : std::uint8_t { authorized, hold };
struct PermissionAutomationActionDecision {
    PermissionOutcome use = PermissionOutcome::deny;
    PermissionOutcome grant = PermissionOutcome::deny;
    PermissionAutomationDisposition disposition = PermissionAutomationDisposition::hold;
};
struct PermissionAutomationDecision {
    std::string author;
    std::string snapshot;
    std::string catalog_revision;
    std::vector<PermissionAutomationActionDecision> actions;
};
// Owner Q3/Q4: each entire action capability must fit BOTH current author USE and
// GRANT, using whole-tuple containment in each channel. Sender is a separately
// constrained chat resource; bot never implies broadcaster. An action is authorized
// iff both fit, otherwise HOLD it. No implicit fallback or stored permit input.
// USE containment preserves effective-permission protected guards; GRANT preserves
// the nondelegable guards above. Unknown/invalid facts throw, never authorize.
// At execution revalidation uses the ORIGINAL author and current raw policies,
// membership, catalog and owner. No decision is a stored execution capability.
// Persisting original authorship, loading current facts and actually holding queued
// actions are server producer/enforcement requirements, not a pure-core claim.
[[nodiscard]] PermissionAutomationDecision
authorize_permission_automation(const PermissionAutomationInput &,
                                const EffectivePermissionLimits & = {});
[[nodiscard]] PermissionAutomationDecision
revalidate_permission_automation(const PermissionAutomationInput &,
                                 const EffectivePermissionLimits & = {});

// All additional APIs use the exact recursive B/I formulas above, applied to EVERY
// declared input/result field, including nested management/preview decision fields
// (u64 witness index B=8,I=0). Graph state/roles/accounts, memberships, both policy
// channels, probes and every output check/account/source/failed-key count. No hidden
// wrapper charge. Whole borrowed input preflight and whole result projection use
// the same six limits, failure ordering and before-copy rules. Graph validation
// occurs at policy stage; all opposing individual scopes are checked on BOTH sides.
//
// Symbolic partitions include ALL same-operation endpoints/resource identities in
// before/after graphs, actor policies, actions, probes and management requests
// (concrete values add singleton boundaries). Count all nonempty traversal cells
// across management/probe singletons, changed-tuple checks, each affected-account
// diff and each automation channel/action; require their checked sum <=L^2 before enumeration
// (input_limit). Derive/stream graph bindings from borrowed indices: no expanded account*role
// policy copies. Total cell*row*slot work <=L^4; all other scans/sorts/projection
// O(input I^2). Same 128*input I scratch bound and existing exact numeric workspace.
// Project complete nested explanations before output growth; result overflow is
// result_limit, never omitted accounts, sources, checks or actions. Source review
// must audit these bounds in implementation. No tests-stage behavior is supplied.

class PermissionDelegationNotImplemented final : public std::logic_error {
  public:
    PermissionDelegationNotImplemented()
        : std::logic_error(
              "Delegation behavior is intentionally unimplemented; review tests first") {}
};
} // namespace subathon::core
