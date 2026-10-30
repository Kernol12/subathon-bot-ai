# BETTERPLACE-002 — Separate aggregate observations and private donation fields

Parent: [EPIC-011](../EPIC-011.md). Kind: implementation.
Specification: v0.45; §14, §13, VIEW-03.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Public donation/aggregate normalization and evidence/privacy labels.

Primary module paths: integrations/ betterplace
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Keep aggregate total changes separate from individual donations and expose explicit aggregate rule triggers.
- Preserve hidden names/amounts and unknown identities; evidence policy cannot relabel an aggregate delta as one confirmed payment.

## Boundary and failure cases

Test hidden/anonymous records, same totals across polls and corrections; do not infer supporter information from totals.

## Separate baseline and implementation tasks

- `BETTERPLACETEST-002` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `BETTERPLACE-002` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: CTest adapter tests with fake HTTP/polling and privacy fixtures.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
