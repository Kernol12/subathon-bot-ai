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

The TESTCLEAN-003 read-only audit found 13 Qt classes installing
`QTest::failOnWarning` only in `initTestCase`. The selected Qt 6.12 cleanup clears
patterns after each test function, including that initialization stage. This is an
existing ineffective per-test guard, not a registration regression. Review and move
that policy into a shared per-test initialization hook; do not claim active warning
enforcement from the old placement. See the [Qt API contract](https://doc.qt.io/qt-6/qtest.html#failOnWarning)
and [selected Qt source](https://raw.githubusercontent.com/qt/qtbase/v6.12.0/src/testlib/qtestcase.cpp).

## Acceptance criteria

- Common helpers have one small maintained implementation and retain condition-based
  waits without sleeps.
- Writer mechanics remain directly covered once at the writer layer; each repository
  retains only necessary repository-specific behavior and unknown-outcome coverage.
- Remove no unique public behavior, rollback, idempotency, exact-accounting or ownership
  assertion merely because it resembles another test.
- Shared warning policy is installed for each actual test function under the selected
  Qt lifecycle. Review expected-warning handling explicitly; do not suppress unknown
  warnings to make suites pass. Preserve assertions and distinguish observed output
  from an actively enforced guard.
- Run focused Qt writer and repository suites; preserve meaningful diagnostics.
- A separate reviewer checks the deleted cases against retained coverage and approves
  the exact baseline change.
