# SCENE-003 — Binding visibility and lifecycle preview editors

Parent: [EPIC-019](../EPIC-019.md). Kind: implementation.
Specification: v0.45; §11, UI-04.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Desktop binding/formatting/condition controls and fake state preview models.

Primary module paths: schemas/ scene, server/ asset/publication services and desktop/ editor
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Provide selectable fields/formatting and all/any/not visibility for phase, modifier, milestone and progress states.
- Preview deficits, grace/pause/finish, multiple milestone crossings and long names/descriptions without changing live state.

## Boundary and failure cases

Unknown/unsupported fields must be flagged; custom content stays authored and locale formatting cannot change accounting.

## Separate baseline and implementation tasks

- `SCENETEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `SCENE-003` (implementation): depends on that completed baseline and its other
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
