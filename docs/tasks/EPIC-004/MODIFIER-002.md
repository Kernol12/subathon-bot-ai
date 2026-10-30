# MODIFIER-002 — Independent modifier clocks and lifecycle behavior

Parent: [EPIC-004](../EPIC-004.md). Kind: implementation.
Specification: v0.45; §7, MOD-02, TIMER-09.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Active-subathon and real-elapsed clock evaluation using reviewed core clock contracts.

Primary module paths: core/ modifier model and server/ persistence adapters
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Default active clock runs during main/grace and freezes on pause/finish; real-elapsed remains available.
- Apply downtime policy and Recovery review distinctions; post-finish effects require explicit enablement and hiding an overlay does not disable a modifier.

## Boundary and failure cases

Test expiry during pause/recovery/hard end and activation/expiry event boundaries with deterministic clocks.

## Separate baseline and implementation tasks

- `MODIFIERTEST-002` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `MODIFIER-002` (implementation): depends on that completed baseline and its other
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
