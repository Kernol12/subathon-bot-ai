# CORE-002 — Main countdown and ending-mode transitions

Parent: [EPIC-001](../EPIC-001.md). Kind: implementation.
Specification: v0.45; §4, TIMER-01, TIMER-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Core lifecycle reducer and deterministic clock interface; persistence wiring belongs to STORAGE-003.

Primary module paths: core/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Represent ready/active/finished and main/grace separately; default to two-minute grace then finish, with immediate and confirmation modes.
- Rescue from grace only after positive applicable credit; implement fresh-grace default and optional shared budget without consuming grace while main runs.

## Boundary and failure cases

Exercise zero crossings, repeated expirations, exact timestamp boundaries and hard-end precedence during running, pause and grace.

## Separate baseline and implementation tasks

- `CORETEST-002` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `CORE-002` (implementation): depends on that completed baseline and its other
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
