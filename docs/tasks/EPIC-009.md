# EPIC-009 — Qt SQL migrations, backups, recovery and rollback

Module: Operations. Kind: epic. Specification: v0.45; §16.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `STORAGETEST-001` → [STORAGE-001](EPIC-009/STORAGE-001.md) | Qt SQL ownership and single SQLite writer |
| [CONTRACT-004](CONTRACT-004.md) | Writer-owned synchronous repository transaction contract |
| `STORAGETEST-010` → [STORAGE-010](EPIC-009/STORAGE-010.md) | Writer-owned dynamic repository transactions |
| `STORAGETEST-011` → [STORAGE-011](EPIC-009/STORAGE-011.md) | Version catalog and transactional migration harness |
| `STORAGETEST-012` → [STORAGE-012](EPIC-009/STORAGE-012.md) | Immutable configuration/context retention |
| `STORAGETEST-013` → [STORAGE-013](EPIC-009/STORAGE-013.md) | Identified-receipt SQL adapter |
| `STORAGETEST-014` → [STORAGE-014](EPIC-009/STORAGE-014.md) | Contribution/evidence SQL adapter |
| `STORAGETEST-015` → [STORAGE-015](EPIC-009/STORAGE-015.md) | Captured-plan/history persistence |
| [STORAGECONTRACT-001](STORAGECONTRACT-001.md) | Internal application-bundle contract |
| `STORAGETEST-002` → [STORAGE-002](EPIC-009/STORAGE-002.md) | Real application-bundle repositories and cross-record integration |
| `STORAGETEST-003` → [STORAGE-003](EPIC-009/STORAGE-003.md) | Durable clock checkpoints and restart integration |
| `STORAGETEST-004` → [STORAGE-004](EPIC-009/STORAGE-004.md) | Consistent database configuration and asset snapshot |
| `STORAGETEST-005` → [STORAGE-005](EPIC-009/STORAGE-005.md) | Versioned encrypted backup envelope and recovery keys |
| `STORAGETEST-006` → [STORAGE-006](EPIC-009/STORAGE-006.md) | Automatic manual and pre-upgrade backup retention |
| `STORAGETEST-007` → [STORAGE-007](EPIC-009/STORAGE-007.md) | Stopped-server staged restore and recovery holds |
| `STORAGETEST-008` → [STORAGE-008](EPIC-009/STORAGE-008.md) | Explicit upgrade and matching-state rollback |
| `STORAGETEST-009` → [STORAGE-009](EPIC-009/STORAGE-009.md) | Backup recipient-key rotation and old-archive recovery |

## Completion and integration

The epic depends on every listed baseline/implementation/verification child plus its
retained foundation prerequisites in progress/state.json. Marking it done requires all
of those dependencies done and a closure evidence note. Planned tasks are not authored
tests, reviewed contracts, implementations or verified capabilities.

Claim a dependency-ready child on its own branch, read its brief and scoped instructions,
and record shared-file ownership. Keep separate independent test/code review roles and
the existing [test strategy](../TEST_STRATEGY.md). No assertion weakening or simulated
provider/platform support is permitted. Missing live/platform evidence stays an explicit
gate; it does not remove a release requirement. Integrate focused PRs, update task evidence
and regenerate PROGRESS.html before handoff.

The [storage split decision](../decisions/0002-storage-executable-slices.md) maps the
original STORAGE-002 scope to mandatory prerequisites; no requirement or real
reviewed/red gate is removed. All added tasks are planning only.
