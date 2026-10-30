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
| `STORAGETEST-002` → [STORAGE-002](EPIC-009/STORAGE-002.md) | Versioned schema migrations and transaction repositories |
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
