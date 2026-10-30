# RATES-007 — Consistent manual inverse cross-rate and pin overrides

Parent: [EPIC-017](../EPIC-017.md). Kind: implementation.
Specification: v0.45; §9, FX-06.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Permissioned effective-rate editing/preview and provenance revisions.

Primary module paths: core/ exact currency values and server/ rate/provider services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Manual pair sets inverse and derived cross-rates; reject contradictory pins before save and preview affected rates.
- Temporary override expires on relevant successful update; pins survive updates and reporting-currency changes.

## Boundary and failure cases

Cover inconsistent cycles, stale previews, zero/negative values and unrelated pair updates without rewriting fetched observations.

## Separate baseline and implementation tasks

- `RATESTEST-007` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `RATES-007` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest for exact rates; Qt Test/fake HTTP for scheduling/cache/adapters.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
