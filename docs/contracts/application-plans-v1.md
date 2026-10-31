# Captured application plans — revision 1

Authority: specification v0.45 §§3.1–5, 6, 7; NUM-01/02/04, MOD-04 and
[event/state boundary](event-state-boundaries-v1.md) B03/B04. ACCTTEST-003 supplies22 independently reviewed cases with observed intended stub-red;
[baseline handoff](../evidence/accttest-003-baseline-handoff.md). ACCT-003 now implements the pure transformations with22 passing unchanged cases,
separate code review and observed pinned automatic CI; [handoff](../evidence/acct-003-handoff.md).

## Scope

`core/application_plan.hpp` captures owned versioned values and inspects preview
freshness. No repository/service is needed for this pure transformation slice.
The authoritative caller retains returned histories using the controlled writer.
ACCT-004 owns atomic application, operation deduplication, current eligibility,
current caps/lifecycle/debt, effects/state/jobs and crash-safe persistence. API-008
owns rule matching, exclusions, modifier combination and numerical evaluation.
This interface deliberately accepts already resolved requested amounts; it neither
recalculates them nor proves their numerical correctness. Exact-decimal revision 1
owns parsing/final rounding. All seven integrations and three rate providers remain
in release scope; no provider behavior is established by these synthetic values.

## Capture

`capture_application_plan` returns one revision numbered 1, without parent or
override decision. Preserve the supplied plan reference, event/receipt identity,
source facts and unknown fields, ancestry, optional contribution link, original
arrival/configuration/modifier-context references, independent contribution state,
valuation and resolved requested effects exactly. Call each replaceable rule and
modifier source with that captured event; retain its ordered results, including
excluded rules/explanations, definition/instance versions, factor spellings,
group/target operators. Sources resolve retained context, never mutable current
configuration. Their algorithms and storage are separate contracts.

Values are owned copies. Changes to caller buffers, source results, current
configuration, modifier expiry or later contribution review cannot rewrite an
existing plan. Every referenced configuration/rule/modifier/quote version must
remain resolvable in durable history; this slice retains references and owned
calculation details but does not implement that historical store.

Historical valuation explicitly distinguishes not-applicable, missing-historical
and resolved. Missing-historical has no quote and no valuation-dependent requested effects;
it must not silently use a current quote. Resolved carries a quote; not-applicable
has no quote. The quote preserves original money, effective/fetched rate spellings,
snapshot/comparison currency, source and timestamps where known, and manual
provenance where present. Absent metadata remains absent. A resolved quote is not
financial confirmation, reviewer acceptance or permission to apply.

For local API shape errors throw `std::invalid_argument`: empty plan/event/root
references, zero receipt ID, inconsistent valuation-state/quote presence, valuation-dependent
requests with missing-historical valuation, or quote original money differing from
the event's original money. References are opaque; a nonempty reference proves no
provider fact or authorization. Validation of upstream rate syntax, numerical
results, rule outputs and referenced history is a caller/source precondition.
Each request explicitly records whether it requires historical valuation. Missing
historical valuation permits independent requests marked false; it never authorizes
the application of the contribution. Mixed-action planning and final application
eligibility remain separate evaluator/application responsibilities.

`RequestedEffect` preserves exact already-scaled rational units, the already
resolved signed64 requested amount, action/rule/version/target, explanation and
reversal behavior. Signed corrections and zero are retained. No actual effect,
cap-limited credit, timer remaining state or delivery execution is captured here.
For 60 seconds ×2 the request is120000ms; later caps may yield a smaller actual
amount. Display precision cannot rewrite stored source spelling or exact values.

## Revisions and previews

`revise_application_plan` returns a copy with one appended revision. Its parent is
the previous head and its number is head+1. All original revisions remain byte/value
identical, including original event, rates and requests. The replacement must retain
the same event/receipt identity, original facts/arrival, ancestry and contribution
link; an override changes calculation context via the explicit replacement source
results, valuation and requests, not historical arrival. Contribution state may
reflect later acceptance without changing the original revision. A missing quote
can thus become an explicitly recorded historical/manual resolution.

The caller has already evaluated permission grants/constraints against this exact
operation. `OverrideAuthority::authorized` records that precondition; this function
does not authenticate a caller or turn actor/decision strings into permission.
Unknown/denied authority throws `std::invalid_argument`, even with nonempty strings.
An authorized decision requires nonempty actor, reason and decision references.
Expected revision unequal to the current head throws `std::runtime_error` before
source resolution; invalid replacement/history shapes throw invalid_argument.
Revision-number exhaustion throws `std::overflow_error`. All failures leave the
original untouched; no source invocation is needed on authority/conflict failure.
The caller must atomically enforce the checked revision when persisting the result;
a pure function cannot arbitrate concurrent writers.

A nonempty sequential history starts at1, has no initial parent/decision and each
later revision has its immediate parent and authorized audit decision. Public
functions reject malformed histories with invalid_argument rather than indexing
an empty history or accepting a forged revision chain. These structural checks
cannot prove the truth of externally supplied permission evidence.

`check_plan_preview` checks a structurally valid plan. Return stale_plan when its
head differs from preview.plan_revision; otherwise stale_state when the supplied
current authoritative state revision differs from the preview token; otherwise
current. Plan staleness takes precedence. Empty state tokens are invalid_argument.
This status is only freshness within the supplied tokens. A current token never
authorizes effects or freezes capacity. Current-state revalidation/application and
permission checks remain mandatory at commit (ACCT-004/API-002/ACCT-005).

## Baseline and compatibility

No existing interface, schema or fixture changes. The original functions were unconditional
`ApplicationPlanNotImplemented` stubs; full review36ac654 preceded observed22-case
intended red ata524447. ACCT-003 then implemented the pure transformations with
22 passing preserved cases and separate code review. Coordinator owns CMake/presets/progress/CI and observed red. The explicit lane
is application-plan-debug with application-plan/acceptance/core
labels and now runs in stable automatic PR/main CI using the selected pinned Boost
setup. No assertion was skipped or inverted. SQLite durability, Qt writer,
permission engine, full evaluator, API and live/platform checks remain separate.
