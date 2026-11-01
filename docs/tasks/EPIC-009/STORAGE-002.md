# STORAGE-002 — Application bundle repositories and transactional storage integration

Parent: [EPIC-009](../EPIC-009.md). Kind: implementation. Specification v0.45 §3,
§16, DB-01, OPS-02. Status/owners/dependencies: progress/state.json.

## Preserved package scope and executable remainder

The original migration/receipt/contribution/effect/state/job/configuration package
was too broad for one baseline. All requirements remain mandatory, now sequenced:

| Original responsibility | Required bounded prerequisite |
| --- | --- |
| Controlled dynamic repository transactions | CONTRACT-004; STORAGETEST-010 → STORAGE-010 |
| Versioned migration catalog, failed/interrupted migration and integrity | STORAGETEST-011 → STORAGE-011 |
| Immutable configuration/arrival-context versions | STORAGETEST-012 → STORAGE-012 |
| Receipt originals/attempts/scoped uniqueness | STORAGETEST-013 → STORAGE-013 |
| Contribution evidence/receipt linkage | STORAGETEST-014 → STORAGE-014 |
| Captured plan/provenance history | STORAGETEST-015 → STORAGE-015 |
| Internal application effect/state/job/result/audit contract | STORAGECONTRACT-001 |
| Atomic application-bundle repositories and cross-record integration | This task and its own real STORAGETEST-002 baseline |

The executable remainder implements the independently reviewed internal application
bundle contract after those prerequisites integrate. It does not finalize unknown
wire, recovery-checkpoint, delivery-policy or provider schemas. Keep downstream
ACCT-004 and other STORAGE-002 dependencies unchanged; children never depend on
this future consumer, and this package is not a status-only aggregate.

## Acceptance and failure criteria

- Persist original money/rates losslessly, arbitrary-precision rational components,
  full identifier/revision ranges and signed64 actual effects, without implicit REAL
  conversion or unsigned-to-signed narrowing. Preserve all resolvable provenance.
- Use reviewed versioned migrations/integrity fixtures supplied by the prerequisite
  slices. Test their cross-record compatibility, uniqueness and missing references;
  preserve their failure/interruption evidence rather than inventing aggregate red.
- Commit application state, effects, jobs/groups, retrievable operation result and
  audit atomically under the controlled writer; uniqueness protects durable identity.
- CONTRACT-002 B08 is a primary literal oracle: a previously committed receipt stays
  pending when application fails; no prepared effect/state/job/application-success
  status/result survives. A permitted retry must recheck identity and apply once.
- Verify failure at intermediate writes and commit, duplicate operations, missing
  references and reopen of actual disposable files. Never publish speculative state
  or external delivery before a known commit; uncertain outcomes are not blind retries.

## Real baseline and implementation gates

STORAGETEST-002 remains kind test_baseline. Author actual tests for genuinely missing
application-bundle behavior, independently review them and record intended red from
explicit stubs after successful compilation. It cannot finish by aggregating passing
child suites or renaming planned work observed_red. STORAGE-002 remains kind
implementation and waits for that gate plus all prerequisites, passing relevant
acceptance/cross-record checks and separate actual code review.

Primary paths: server/ persistence, versioned migrations and scoped Qt Test/CTest.
Read root/server/tests AGENTS.md; integrator owns shared schemas/migration order,
CMake/presets/progress/HTML. Synthetic/redacted disposable data only. Extend/observe
ordinary stable CI, record commands/environment/revisions and publish focused PRs.
No full application/provider/platform/backup/recovery release claim follows.
