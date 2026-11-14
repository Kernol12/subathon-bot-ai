# STORAGE-007 — Stopped-server staged restore and recovery holds

Parent: [EPIC-009](../EPIC-009.md). Kind: implementation.
Specification: v0.45; §16, OPS-04.

Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Console restore staging/validation/replacement and guarded restart of restored installation.

Primary module paths: server/ persistence, migrations and console operations
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Decrypt/stage/validate schema/assets/integrity, preserve current installation before replacement and require stopped application.
- On restart enter Recovery review, revoke restored sessions, hold outbound jobs and enforce hard end without replaying actions.
- At EVERY supported restore crash/reopen point, the selected installation path must not be acceptable as an ordinary store to a host lacking a restore consumer. Keep the original ordinary installation intact or ensure guarded restart/refusal before restored credential resolution, accepted login, issuance or publication. An actual acknowledged installation-epoch fence must invalidate restored sessions before restored access; unknown fence outcomes stay held.
- Extend the real API-001 guarded restart startup to consume the chosen restore protocol. A fixture flag, plausible epoch reread or a replacement-only unit test cannot close this integration gate.

## Boundary and failure cases

Test interrupted replacement, invalid/wrong-key archives, missing credential recovery material and prior-state rollback with disposable installations.

The shared STORAGETEST-007 baseline must enumerate the chosen protocol's actual
crash points before/after staging, destructive replacement/installation and epoch
fence acknowledgment, then reopen through real guarded host startup. For each
point prove either the untouched original ordinary installation or refusal/Recovery
with no restored credential acceptance/publication or outbound replay. Include a
host that lacks the restore consumer: a valid-looking old schema8 snapshot must
never become an ordinary acceptable selected store after a supported restore
crash. Prove final recovery still enters review, holds outbound work, enforces hard
end and invalidates old sessions before guarded restored access.

Possible approaches include staging epoch/Recovery/outbound-fenced state before
atomic installation, or durable intent combined with a future catalog that old
hosts refuse. These are examples ONLY; no marker/schema/filesystem/atomic-install
mechanism is selected or certified sufficient here. The task's own reviewed
contract/baseline must choose and prove its crash-safety and compatibility protocol.

The [APITEST-001 consumer proposal](../../contracts/api-host-lifecycle-boundary-v1.md)
§1 permits ordinary current-schema startup now via exact acknowledged prefix1–8,
version8 inspection plus the configured installation read/normal COMMIT on the
same live writer. No supported restore producer exists at that schema. Coherent
out-of-band copies/VM rollback remain the clock contract §1 trust limit; malformed
current facts and known unresolved fences still refuse. This forward obligation
belongs to this existing STORAGE-007/STORAGETEST-007 scope. Do not make API-001 wait
for it: STORAGE-007 already depends on SESSION-009, which depends on API-001. No
new task/prerequisite, dependency change, executable authorization or scope waiver
is created by this brief correction.

## Separate baseline and implementation tasks

- `STORAGETEST-007` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `STORAGE-007` (implementation): requires that completed baseline.
  Preserve reviewed assertions, implement only this slice,
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

## ADR-0010 additive session prerequisites

[Session executable decomposition](../../decisions/0010-session-executable-prerequisites.md)
preserves all original scope and test-first/release gates. The authoritative task
graph adds the relevant supplier/lifecycle/store/local-login/clock/ingress prerequisites.
These components do not complete this task's own full acceptance or real integration.
