# MILESTONE-001 — Sequential achievement and deficit calculation

Parent: [EPIC-005](../EPIC-005.md). Kind: implementation.
Specification: v0.45; §8, MILE-01.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Pure score/threshold progression model separate from action delivery and UI presentation.

Primary module paths: core/ progression and server/ delivery services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Record every crossed target immediately in ascending order; score 800 below completed target 1000 retains completion and deficit 200.
- Progress to the next 2000 target requires 1200 points and displays zero progress below the previous completed threshold.

## Boundary and failure cases

Enforce ascending targets and checked score arithmetic; refund/deficit recovery never automatically repeats an achievement.

## Separate baseline and implementation tasks

- `MILESTONETEST-001` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `MILESTONE-001` (implementation): depends on that completed baseline and its other
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
