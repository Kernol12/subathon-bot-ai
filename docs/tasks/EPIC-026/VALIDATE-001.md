# VALIDATE-001 — Maintain complete requirement and integration evidence matrix

Parent: [EPIC-026](../EPIC-026.md). Kind: test_draft.
Specification: v0.45; §19, DEV-04, TEST-05, TEST-01, TEST-02, TEST-04, DEV-01, DEV-03.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Acceptance-map/planning matrix and evidence ownership; no product implementation.

Primary module paths: tests/ integration/platform/performance evidence
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Map every specification decision/scenario to test owner, planned suite and explicit live/platform gate without labeling planned work verified.
- Track versioned fixtures/environments, independent review and exceptions; identify missing coverage before feature integration.

## Boundary and failure cases

Task counts and green infrastructure cannot become completion percentages or blanket supported-capability claims.

## Test preparation gate

Author specification-derived oracles/fixtures without a production implementation.
Arrange a separate reviewer and record identity/revision before dependent acceptance
baselines use the artifacts. Authored test data alone does not establish observed red,
passing product behavior or platform compatibility.

## Checks and handoff

Primary home: Existing relevant acceptance suites plus explicitly recorded platform/load runs.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
