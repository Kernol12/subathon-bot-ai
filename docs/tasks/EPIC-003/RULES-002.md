# RULES-002 — Exclusive groups and directional exclusions

Parent: [EPIC-003](../EPIC-003.md). Kind: implementation.
Specification: v0.45; §6, RULE-01, RULE-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Deterministic candidate ranking and selection independent of action execution.

Primary module paths: core/ automation; simulator adapters in server/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Highest priority wins within a group; ties use explicit user order and generate a warning/preview.
- Only selected rules apply lower-ranked exclusions; A excluding B and B excluding C with A selected leaves C eligible.

## Boundary and failure cases

Reject backward/contradictory exclusions before enabling; unrelated matching rules still run despite priorities.

## Separate baseline and implementation tasks

- `RULESTEST-002` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `RULES-002` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest via CTest with bounded fixtures and fake actions.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
