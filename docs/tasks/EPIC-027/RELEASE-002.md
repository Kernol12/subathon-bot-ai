# RELEASE-002 — Operational log retention and redacted diagnostic export

Parent: [EPIC-027](../EPIC-027.md). Kind: implementation.
Specification: v0.45; §16, OPS-03.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Server diagnostic schema/log rotation/export with permissioned versions/connection-health data.

Primary module paths: release manifests, server/ diagnostics and docs/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Default local operational-log retention to 14 days independently from retained accounting/audit.
- Exclude secrets and supporter details by default; additional event data requires explicit authorized selection.

## Boundary and failure cases

Test raw tokens/headers, user-controlled messages, identifiers and revoked export permission; ordinary diagnostics are not encrypted full backups.

## Separate baseline and implementation tasks

- `RELEASETEST-002` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `RELEASE-002` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: Manifest/redaction checks with synthetic data and final artifact/document audit.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
