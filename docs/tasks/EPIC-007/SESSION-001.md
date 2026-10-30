# SESSION-001 — Password hashing and parameter upgrades

Parent: [EPIC-007](../EPIC-007.md). Kind: implementation.
Specification: v0.45; §10, AUTH-11.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Libsodium Argon2id password API, stored parameters and independent verifier fixtures.

Primary module paths: server/ credential/session services; desktop/ platform stores
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Store salted hashes and parameters, verify credentials and upgrade outdated parameters after successful login.
- Define bounded password/input/cost handling and a separate supported-hardware benchmark procedure.

## Boundary and failure cases

Wrong passwords cannot upgrade credentials; no plaintext password in logs/storage/fixtures and no fake claim of measured cost.

## Separate baseline and implementation tasks

- `SESSIONTEST-001` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `SESSION-001` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest for pure policy; Qt Test/platform-adapter tests with synthetic secrets.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
