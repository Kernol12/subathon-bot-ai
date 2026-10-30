# RULES-004 — Bounded generated-event processing and parentage

Parent: [EPIC-003](../EPIC-003.md). Kind: implementation.
Specification: v0.45; §6, RULE-03.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Root/parent event tracing, duplicate rule processing prevention and per-chain budgets.

Primary module paths: core/ automation; simulator adapters in server/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Default to depth 32 and 256 generated events per root with configurable validated limits.
- Stop further generation and audit/notify on a limit while preserving recorded effects and allowing other server work.

## Boundary and failure cases

Cover cycles, wide fan-out, replayed events and distinct milestone events in one chain; never use an unbounded recursion budget.

## Separate baseline and implementation tasks

- `RULESTEST-004` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `RULES-004` (implementation): depends on that completed baseline and its other
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
