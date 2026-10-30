# MILESTONE-003 — Durable delivery-job states and uncertain outcomes

Parent: [EPIC-005](../EPIC-005.md). Kind: implementation.
Specification: v0.45; §3, §8.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Generic persisted outbox/job executor with fake external adapters; connector send implementations remain provider tasks.

Primary module paths: core/ progression and server/ delivery services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Persist queued/sending/delivered/failed/outcome-unknown states and captured sender/action definitions.
- Retry only under a reviewed provider duplicate-safety or known-undelivered policy; uncertain performed actions require review after restart.

## Boundary and failure cases

Crash before send, after send and before acknowledgment must not cause blind replay; default tests send no live messages.

## Separate baseline and implementation tasks

- `MILESTONETEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `MILESTONE-003` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest for progression; Qt Test/fake adapters for queue/restart behavior.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
