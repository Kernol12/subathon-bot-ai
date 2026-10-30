# ACCT-006 — Cumulative partial and full reversal entitlements

Parent: [EPIC-002](../EPIC-002.md). Kind: implementation.
Specification: v0.45; §3, §8, NUM-03.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Exact reversal entitlement calculation from original actual effects and prior reversals; no current-rule recomputation.

Primary module paths: core/ and server/ application services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- For a 20-euro contribution credited 5 minutes/40 points, a 5-euro refund entitles 75 seconds/10 points.
- Compute cumulative entitlement minus prior reversals; full refund clears exact remaining credit and whole counts reverse only on full refund.

## Boundary and failure cases

Cover many small refunds, rounding ties, excess/repeated refunds and original cap-limited awards; original rates/provenance remain intact.

## Separate baseline and implementation tasks

- `ACCTTEST-006` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `ACCT-006` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest for accounting; Qt Test for writer/service integration.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
