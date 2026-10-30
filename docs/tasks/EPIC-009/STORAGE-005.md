# STORAGE-005 — Versioned encrypted backup envelope and recovery keys

Parent: [EPIC-009](../EPIC-009.md). Kind: implementation.
Specification: v0.45; §16, CRYPTO-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Libsodium secretstream backup format and owner-controlled recipient-key provisioning/export.

Primary module paths: server/ persistence, migrations and console operations
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Use fresh per-backup encryption key sealed to owner public key; unattended server retains public key only.
- Authenticate metadata/finalization, validate integrity and document that sealed boxes alone do not authenticate the sender.

## Boundary and failure cases

Test wrong private key, truncation, tampering and missing final tag with synthetic files; account recovery codes cannot decrypt backups.

## Separate baseline and implementation tasks

- `STORAGETEST-005` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `STORAGE-005` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: Qt Test/CTest with disposable SQLite/filesystem and deterministic fault injection.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
