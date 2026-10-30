# STORAGE-003 — Durable clock checkpoints and restart integration

Parent: [EPIC-009](../EPIC-009.md). Kind: implementation.
Specification: v0.45; §4, TIMER-08, TIMER-09.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Authoritative clock checkpoint persistence and restart/recovery-review service.

Primary module paths: server/ persistence, migrations and console operations
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Save state with every timer-changing operation and advancing-clock checkpoints every second by default with configurable interval.
- Restore from the last durable checkpoint using configured downtime/recovery policy and hold unresolved support.

## Boundary and failure cases

Kill/restart between writes; verify abrupt Freeze semantics and hard-end precedence without conflating millisecond effect precision and checkpoint cadence.

## Separate baseline and implementation tasks

- `STORAGETEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `STORAGE-003` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: Qt Test/CTest with disposable SQLite/filesystem and deterministic fault injection.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
