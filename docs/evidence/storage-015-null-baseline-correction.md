# STORAGE-015 baseline defect correction — SQL NULL oracle

Author: `/root/plan_fixture_author`, 2026-11-02. **Authored correction candidate;
separate actual review pending.** No implementation, acceptance pass or revised
red run is claimed here.

The coordinator delegated this narrow correction on the fresh clean isolated
`storage015-null-baseline` worktree / `task/STORAGE-015-null-baseline` branch at
`1859ef24`. Ownership is limited to the projection/assertions inside
`revisionsAppendContiguouslyWithoutRewritingInitial` and this evidence file.
Production, interfaces, contracts, fixtures, other tests/assertions, shared build,
CI and progress metadata are unchanged.

## Defect and unchanged requirement

The implementation author's changed-source run, read from
`/tmp/storage015-initial-full173-LastTest.log`, reports 28/29 new cases and172/173
full cases passing. The sole failure is the raw nullable `override_authority`
comparison at the old test's line1003: actual Qt QVariant is printed as
QString/empty, while the expected value is default `QVariant{}` / Invalid.
These are attributed run results, not commands repeated by this author. The log's
pretty-print alone cannot establish whether the actual SQL value was NULL or TEXT.

Specification v0.45 §§3.2–3.5/6/7 and DB-01, the
[logical contract](../contracts/sqlite-application-plans-v1.md) and the exact
[physical role CHECK](../contracts/sqlite-application-plans-schema-v1.md) require
initial revision1 with no parent/override and later authorized2 audit. The initial
SQL column must be **NULL**; no contract requires the QSQLITE result's QVariant
metatype to be Invalid. Later authority remains SQL INTEGER with exact value2.
`initialRoundTripPreservesCompleteOwnedSnapshot` already asserts the initial
SQL `typeof(override_authority) == 'null'`. No requirement or release scope changes.

## Independent runtime observation

A disposable scratch C++ program used only Qt Core/SQL and an in-memory QSQLITE
connection. No project/plan header, fixture, codec, factory, store, feature body or
historical stub/red was used. It executed:

```sql
CREATE TABLE probe(ordinal INTEGER PRIMARY KEY, override_authority INTEGER)
INSERT INTO probe VALUES (1,NULL),(2,2)
SELECT override_authority,typeof(override_authority) FROM probe ORDER BY ordinal
```

Actual environment: Linux x86-64, GNU16.2.1, Qt/QSQLITE6.12.0, SQLite3.54.0.
The exact standalone compiler invocation used C++20, `-Wall -Wextra -Wpedantic
-Wconversion -Wshadow -Werror -fPIC`, Qt pkg-config defines/system include paths,
`/tmp/storage015-null-variant-probe.cpp -lQt6Sql -lQt6Core`, and output
`/tmp/storage015-null-variant-probe`. Compilation exited0 without warnings;
executing that binary exited0 and printed:

```text
Initial: Qt type=QString valid=1 null=1 equalsInvalid=0 SQL type=null
Later: Qt type=qlonglong null=0 value=2 SQL type=integer
Qt 6.12.0 SQLite 3.54.0; no plan API or fixture used
```

The program explicitly requires initial `raw_null.isNull()` true,
`raw_null == QVariant{}` false and SQL type `null`. It separately requires later
`raw_authority == QVariant(qint64(2))` and SQL type `integer`. Thus typed NULL and
Invalid are observably unequal even when the required SQL NULL semantics hold;
empty TEXT would fail the SQL-type and null checks.

Scratch artifacts and SHA256:

| Artifact | SHA256 |
| --- | --- |
| `/tmp/storage015-null-variant-probe.cpp` | `a96168023a63ad5278add864963e0d250162044824b206e13e20924b2707d66d` |
| `/tmp/storage015-null-variant-probe-build.log` | `01274d8399f2428f61e994508b1b31da877bd02fb66ca574a55e050bb5b0bf0a` |
| `/tmp/storage015-null-variant-probe.log` | `ce170e79658a159665304c46c86a0121b16f31cfe147feef247c900fcbbe757b` |

## Minimal correction and preservation

Old complete feature-test blob:
`0f8e625ff7c238776602fd1be102959f8c43b073`.
Corrected complete feature-test blob:
`a805674e014c0c7ae325474460bc64d6b10de770`.

The same original six data columns are still selected, with one added SQL
`typeof(override_authority)` column. Explicit 3-row/7-column checks protect indexing.
Initial raw authority is asserted `isNull()`. Both later raw authority values are
still compared exactly against `QVariant(qint64(2))`. Only after all three raw
values are checked is that column removed from the local comparison copy; the
remaining projection asserts SQL types `null/integer/integer` and every original
revision/parent/actor/reason/decision byte literal unchanged. No NULL is defaulted
to zero, cast, coerced to empty text or replaced in production/fixture data.

All complete-history, saved-input, source-mutation and immutable-original-row
assertions before the projection remain byte-exact. A static before/after check
also proved every byte outside the single delegated slot is unchanged. This
corrects an unsupported Qt-metatype oracle while **adding** explicit SQL-class/
null observations; it does not weaken the storage requirement to make code pass.

Actual static commands passed:

```sh
clang-format --dry-run --Werror --lines=1003:1025 tests/qt/sqlite_application_plans_tests.cpp
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

No plan feature suite, stub suite, old red or original red was run by this author.
The original baseline red Git blob
`ae3e0e25956b16b0d3bc26065c71247263bca421` remains immutable historical evidence;
this is a separately linked corrected baseline. The prior full review
`8b537cdbe3f0d55d56034a013f6e4cbaebc7baee` remains valid
for untouched assertions, and cannot approve this new correction without actual
separate review. The coordinator must integrate only after that review, then the
implementation author reruns changed-source29/full173 and proceeds through source/
CI/final gates. Full product, semantic/provider/FX/UI/platform/recovery/live gates
remain open; no owner decision/access is needed for this defect correction.
