# ACCT-005 — Individual batch and silent contribution review

Parent: [EPIC-002](../EPIC-002.md). Kind: implementation.
Specification: v0.45; §5.

Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Review decision service with milestone/delivery interfaces; review UI belongs to desktop/web tasks.

Primary module paths: core/ and server/ application services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Preview accept/reject individually and in batches with actor/options/audit and arrival-context preservation.
- Silent acceptance suppresses ordinary alerts but retains internal progression and explicit milestone Run/Defer/Skip choice.

## Boundary and failure cases

Recheck permissions/revisions at execution, handle concurrent reviewers and partial batch eligibility, and never silently bypass another hold.

## Separate baseline and implementation tasks

- `ACCTTEST-005` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `ACCT-005` (implementation): requires that completed baseline.
  Preserve reviewed assertions, implement only this slice,
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

## Explicit mixed-deficit policy gate

[Application-service logical preparation](../../contracts/application-service-v1.md)
retains CORE-005/AT09's timer-only RequireReview meaning. It does not select whole-
input deferral versus a complete atomic result containing timer-held/unapplied and
other actual effects. Signed manual command policy (CORE-008), original review/
global-status and authorized continuation (ACCT-005), and financial reversal policy
(ACCT-006) must settle their respective mapping through actual reviewed contracts/
examples before enabling those paths. Preserve existing applied components and
exact held remaining amount; resolution/apply is one unit, never automatic replay.
Unknown policy stays not-ready, not an invented default. Post-finish mixed time/
score likewise needs explicit reviewed policy and complete retained representation;
never disguise lifecycle denial as cap limitation or invent unsupported components.
This is an explicit required follow-up, not a behavior/support claim or scope waiver.
