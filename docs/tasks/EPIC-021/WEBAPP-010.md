# WEBAPP-010 — Browser rate configuration and historical conversion review

Parent: [EPIC-021](../EPIC-021.md). Kind: implementation.
Specification: v0.45; §9, §15.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Browser provider/usage/freshness/override and reporting-currency panels.

Primary module paths: web/management/ and server/ static-asset boundary
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Show fetch/publication ages, source/fallback/next sync and authoritative versus local usage estimates.
- Preview consistent inverse/cross-rate/pin changes, historical conversion and partial/unconverted totals.

## Boundary and failure cases

Display precision/reference allowances never change accounting/provider limits; reject stale/unauthorized override commits.

## Separate baseline and implementation tasks

- `WEBAPPTEST-010` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `WEBAPP-010` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: Vitest for units/components; Playwright for complete browser workflows.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
