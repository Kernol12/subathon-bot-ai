# STREAMLABS-003 — Gap catch-up first import and capability reporting

Parent: [EPIC-013](../EPIC-013.md). Kind: implementation.
Specification: v0.45; §14, INTAKE-01, INTAKE-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Persistent live/history boundary, supported recovery and health/limitations model.

Primary module paths: integrations/ Streamlabs
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Use baseline/new-support defaults and reviewed explicit history import with common catch-up policy.
- Report refund/history evidence and unrecoverable gaps, and enforce source authority when Twitch activity is mirrored.

## Boundary and failure cases

Test disconnect during paging, duplicate pages and invalid credentials; unknown refund support remains unknown.

## Separate baseline and implementation tasks

- `STREAMLABSTEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `STREAMLABS-003` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: CTest adapter tests with fake Socket.IO/REST and overlap fixtures.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
