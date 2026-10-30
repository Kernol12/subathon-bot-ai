# DESKTOP-002 — Live timer grace pause and reopening controls

Parent: [EPIC-018](../EPIC-018.md). Kind: implementation.
Specification: v0.45; §4, §10.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Permission-aware native timer/status panels and command previews.

Primary module paths: desktop/ C++ models and QML management UI
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Animate snapshots locally while server owns transitions; expose independent reasons, debt, hard end and recovery review.
- Provide authorized timer changes/reopening choices with operation IDs/revision checks and current effective permissions.

## Boundary and failure cases

Handle permission revocation, stale previews and reconnect without replaying commands or clearing unrelated holds.

## Separate baseline and implementation tasks

- `DESKTOPTEST-002` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `DESKTOP-002` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: Qt Test for client models; Qt Quick Test for QML workflows.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
