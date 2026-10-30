# ACCT-004 — Atomic effect application and operation idempotency

Parent: [EPIC-002](../EPIC-002.md). Kind: implementation.
Specification: v0.45; §3, §5.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Application transaction service over the controlled writer and deterministic planning interfaces.

Primary module paths: core/ and server/ application services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Commit application status, actual effects, state and delivery-job records atomically; failed transactions leave no partial award.
- Duplicate operations and concurrent acceptance yield one application with existing result/conflict for the other caller.

## Boundary and failure cases

Inject commit failures/restarts and cap changes after preview; retain requested/applied amounts and publish only committed outcomes.

## Separate baseline and implementation tasks

- `ACCTTEST-004` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `ACCT-004` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest for accounting; Qt Test for writer/service integration.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
