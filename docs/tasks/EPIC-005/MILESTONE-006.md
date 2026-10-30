# MILESTONE-006 — Replace unstarted actions and interrupt groups

Parent: [EPIC-005](../EPIC-005.md). Kind: implementation.
Specification: v0.45; §8, MILE-04, MILE-05.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Authorized replacement/stop commands and cancellation/stop requests to supported playback adapters.

Primary module paths: core/ progression and server/ delivery services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Queued definitions remain captured; explicit previewed replacement affects only actions not yet started.
- Stop cancels unstarted actions and requests playback stop; Continue next/Hold(default) does not roll back sent messages/internal effects.

## Boundary and failure cases

Retain original started/completed definitions and independent review holds; reset/replay remains separately authorized.

## Separate baseline and implementation tasks

- `MILESTONETEST-006` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `MILESTONE-006` (implementation): depends on that completed baseline and its other
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
