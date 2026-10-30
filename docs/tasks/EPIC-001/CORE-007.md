# CORE-007 — Finished-input and audited reopening policies

Parent: [EPIC-001](../EPIC-001.md). Kind: implementation.
Specification: v0.45; §4, TIMER-06, TIMER-10.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Core finished-state/reopen commands using permission decisions and pending-group choices supplied by services.

Primary module paths: core/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Continue recording finished inputs with explicit post-finish progression/action settings; ordinary support never reopens.
- Reopen preserves history, score, achievements and debt; require resolving a passed deadline, explicit grace-budget choice and Resume/Keep deferred(default)/Skip for pending groups.

## Boundary and failure cases

Reject unauthorized or stale reopen/correction commands; preserve independent review holds and never replay already-performed actions.

## Separate baseline and implementation tasks

- `CORETEST-007` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `CORE-007` (implementation): depends on that completed baseline and its other
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
