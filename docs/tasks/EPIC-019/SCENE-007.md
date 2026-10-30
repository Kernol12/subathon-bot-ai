# SCENE-007 — Permissioned live publishing restore and compatibility checks

Parent: [EPIC-019](../EPIC-019.md). Kind: implementation.
Specification: v0.45; §11, UI-04.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Server publish/restore commands and native compatibility preview.

Primary module paths: schemas/ scene, server/ asset/publication services and desktop/ editor
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Publish a validated draft as a new live revision with permission/revision checks and support restoring an earlier revision.
- Flag unsupported features before publishing to both outputs; scenes intended for both must use their shared supported feature set.

## Boundary and failure cases

No partial publication on invalid assets/conflicts; restoring a revision must not mutate history or replay actions.

## Separate baseline and implementation tasks

- `SCENETEST-007` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `SCENE-007` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: Schema/Qt Test for documents/assets; Qt Quick Test for editor interactions.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
