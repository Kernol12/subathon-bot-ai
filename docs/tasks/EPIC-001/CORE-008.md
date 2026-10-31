# CORE-008 — Manual adjustments cap edits and hard-deadline commands

Parent: [EPIC-001](../EPIC-001.md). Kind: implementation.
Specification: v0.45; §4, TIMER-01.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Pure timer command/revision model for manual/editorial changes; server API supplies current permission decisions.

Primary module paths: core/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Record requested/actual signed corrections and validate cap/deadline changes through reviewed contracts without overflow or hidden history rewrite.
- Enabled hard deadline remains authoritative during pause/grace; only an authorized explicit change/disable can alter it.

## Boundary and failure cases

Before code, resolve above-new-cap and clock-edit boundary questions against specification/CONTRACT-001 rather than guessing; test competing revisions and exact deadline boundaries.

## Separate baseline and implementation tasks

- `CORETEST-008` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `CORE-008` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest via CTest; controllable clock and exact reference vectors.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.

## First-start boundary follow-up from CORETEST-002

The bounded CORE-002 countdown reducer admits already-started Active/Finished states;
it does not choose whether a hard deadline affects Ready before first start or how
first start validates an already-passed deadline. Resolve those first-start/deadline
semantics with API-007 before claiming complete lifecycle command support. This is
an explicit contract/acceptance gate, not implemented or verified behavior.
