# RENDER-001 — Author shared scene reference fixtures and conformance oracles

Parent: [EPIC-020](../EPIC-020.md). Kind: test_draft.
Specification: v0.45; §11, UI-02, UI-04, MEDIA-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Tests reference scenes/expected bindings, geometry, visibility, progress and animation timelines; no renderer algorithm.

Primary module paths: desktop/ native output, web/overlay/ and tests/ reference scenes
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Derive reference expectations independently from specification, including normal/deficit/final progress and both phase/modifier states.
- Arrange separate fixture/oracle review before renderer baselines rely on them; allow minor font raster differences only.

## Boundary and failure cases

Include unsupported-feature rejection, resizing and skipped-frame timing; authored fixtures alone are not native/browser/OBS verification.

## Test preparation gate

Author specification-derived oracles/fixtures without a production implementation.
Arrange a separate reviewer and record identity/revision before dependent acceptance
baselines use the artifacts. Authored test data alone does not establish observed red,
passing product behavior or platform compatibility.

## Checks and handoff

Primary home: Qt Quick Test for native; Vitest/Playwright for browser; actual OBS checks separately.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
