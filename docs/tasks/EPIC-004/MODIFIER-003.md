# MODIFIER-003 — Per-group and per-target stacking operators

Parent: [EPIC-004](../EPIC-004.md). Kind: implementation.
Specification: v0.45; §7, MOD-01, NUM-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Pure combination of qualified modifiers with captured groups, ordering and target-specific outputs.

Primary module paths: core/ modifier model and server/ persistence adapters
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Support multiply(default), additive bonuses, highest, lowest and priority selection at group and target levels.
- Clamp negative final additive factors to zero; retain exact calculation details before final effect rounding and cap application.

## Boundary and failure cases

Cover ties, factor zero/0.5, several groups/targets and large rational products with bounded resource use.

## Separate baseline and implementation tasks

- `MODIFIERTEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `MODIFIER-003` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest for arithmetic/clocks; Qt Test for SQL/recovery wiring.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
