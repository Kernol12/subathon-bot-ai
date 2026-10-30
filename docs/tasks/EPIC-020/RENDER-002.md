# RENDER-002 — Native QML scene elements and synchronized bindings

Parent: [EPIC-020](../EPIC-020.md). Kind: implementation.
Specification: v0.45; §11, UI-01, UI-02, UI-04.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Desktop-only renderer for timers/text/progress/images/shapes/groups/recent support/announcements.

Primary module paths: desktop/ native output, web/overlay/ and tests/ reference scenes
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Interpret supported scene bindings/visibility/geometry/progress using server state snapshots and shared fixtures.
- Use default aspect-fit with crop/stretch options and show current server timeline after reconnect.

## Boundary and failure cases

Do not share a browser rendering engine; unknown data and unsupported elements are explicit instead of silently reinterpreted.

## Separate baseline and implementation tasks

- `RENDERTEST-002` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `RENDER-002` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: Qt Quick Test for native; Vitest/Playwright for browser; actual OBS checks separately.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
