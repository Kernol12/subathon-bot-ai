# TESTCLEAN-004 — Share Qt test support and deduplicate writer plumbing

Module: Test infrastructure. Kind: cleanup. Specification: v0.45; §17.
Contract revision: current SQLite writer/repository contracts. Parent: EPIC-028.
Depends on: TESTCLEAN-003. Shared-file ownership: `tests/qt`, its support helpers, and
SQLite writer/repository suites.

## Scope

Move repeated assertion and completion-wait helpers into `tests/qt/support`; prefer
`QVERIFY`/`QCOMPARE` so failures report source and data-row context. Consolidate copies
of shared writer-session tests (lost acknowledgements, second begin, foreign/stale
capabilities, budgets and input-error priority) into writer-work coverage. Keep the
repository-specific unknown-outcome/physical-row case for each repository and keep
repository behavior distinct from shared writer mechanics.

## Acceptance criteria

- Common helpers have one small maintained implementation and retain condition-based
  waits without sleeps.
- Writer mechanics remain directly covered once at the writer layer; each repository
  retains only necessary repository-specific behavior and unknown-outcome coverage.
- Remove no unique public behavior, rollback, idempotency, exact-accounting or ownership
  assertion merely because it resembles another test.
- Run focused Qt writer and repository suites; preserve meaningful diagnostics.
- A separate reviewer checks the deleted cases against retained coverage and approves
  the exact baseline change.
