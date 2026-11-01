# STORAGE-011 — initial U+FEFF regression authoring

Regression author: `/root/writer_transaction_reviewer`, 2026-11-01. Different
reviewer: `/root/storage_work_code_reviewer`; production implementation/fix remains
separately owned. This author owns only the migration test file and this evidence
in `local-evidence/worktrees/storage011-unicode-regression`, branch
`task/STORAGE-011-unicode-regression`, created at `0118fea` and refreshed normally
through coordinator revision `2fb5f89` for the two new CTest registrations.

Authority: root/tests AGENTS, specification v0.45 §16/DB-01/OPS-02 and approved
[migration contract revision 1](../contracts/migration-catalog-v1.md), blob
`cbdea1a48d2989a9c8bd17edf0539bdf7c1bce61`. Its strict UTF-8/full-Unicode identity
requires preserving valid U+FEFF content rather than stripping it or calling it
invalid. No requirement or existing assertion is changed.

## Actual issue and exact scope

Different code review [evidence](storage-011-code-review.md) identified
`strict_utf8`'s QString::fromUtf8 round trip as rejecting a valid initial U+FEFF.
It affects canonical object fields and actual stored-schema text. The reviewed
production source remains unchanged at blob
`d91c7d6831f656ccba93e077d8bf9f541db65a80`. This author has not corrected it or
executed a new regression body to manufacture a red result before test approval.

Two focused slots are added:

- `canonicalCodecPreservesInitialBom` supplies a valid schema name/table-name
  starting with literal U+FEFF followed by ASCII `a`. It requires exact canonical
  payload bytes and digest rather than normalization/rejection.
- `storedSchemaPreservesInitialBom` independently creates that quoted table and
  row17, plus a matching literal catalog/history. Before calling the UUT it checks
  actual name/table-name QString values, UTF-8 and BLOB bytes. A current inspection
  must then commit normally with version1/applied0, unchanged raw snapshot/data and
  released lock.

The original22 slot bodies, all existing initialization/helpers and all ten old
JSON vectors are unchanged. The corrected test file is blob
`e94befdc01ceff62d2e486e1e5bff556cabe1e55`; original test blob was `a85a6c0576`.
No production/header/contract/fixture/CMake/state file was edited by this author.
The coordinator owns the two CTest-name additions and all shared publication gates.

## Independent literal oracle

U+FEFF is constructed as `QChar(0xfeff)` and concatenated with `a`, avoiding a
decoder that could consume the very initial character under test. Its exact UTF-8
is `efbbbf61` (four bytes), and F(name) is `343aefbbbf61` (ASCII `4:` plus bytes).
The migration ID `bom-name` remains ordinary ASCII; this case does not relax the
separate ASCII-ID rule.

An independent Python standard-library reference framed version1, ID `bom-name`,
two SQL statements `CREATE TABLE "U+FEFF+a"(value INTEGER)` and
`INSERT INTO "U+FEFF+a" VALUES (17)`, the three reserved catalog objects and the
new table object. The complete unsigned-UTF-8-sorted payload is frozen inline:
638 bytes, SHA-256
`baf9a94eeb7cbdd3994a277363ae6dd1cd4b63af76f99b02b48f8f3ac4775b7d`.
Both leading-name fields have the exact F frame; complete payload comparison and
the literal digest reject silent character loss. No expected value came from the
production codec, digest, runner or storage inspection. The coordinator separately
recomputed the reference and reported matching full bytes/hash.

The stored fixture uses literal quoted DDL and literal row insertion. It does not
bind a leading-U+FEFF text value, which the reviewer found can consume that mark in
some Qt/SQLite binding paths. Only the already-literal digest is bound as BLOB.
Raw CAST(name/table-name AS BLOB) assertions require actual `efbbbf61` bytes before
the UUT request, so a setup that silently loses the mark cannot pass as a valid
regression. Production currently can reject the manifest before reaching stored
inspection; after any codec-only correction, the stored case still requires the
stored-text path to preserve the same valid character. No claim is made that this
author observed those future execution paths.

## Actual checks and remaining sequence

Commands/checks in the isolated regression worktree:

| Command/check | Actual result |
| --- | --- |
| `clang-format -i tests/qt/sqlite_migration_tests.cpp` | Completed |
| `cmake --preset sqlite-migrations-debug` | Exit0; GNU16.2.1, configured selected Qt dependency |
| `cmake --build --preset sqlite-migrations-debug` | Exit0; all existing/new test targets linked successfully |
| Compiled `-functions` versus CTest `--show-only=json-v1` | Exact24 migration names/count, 78 total registrations; no new body execution |
| Static original-function comparison | All25 existing multiline functions (including original22) and inline init preserved byte-for-byte; only two new slots |
| Independent inline-hex length/SHA check | 638 bytes and exact literal digest; both BOM name frames present |
| Source/vector blob checks | Source d91c7d and old JSON12280c8f unchanged |
| `python3 tools/check_project.py`, `python3 tools/progress.py render --check`, `git diff --check` | Exit0; 391 records/122 decisions/current HTML and clean whitespace |

The first static audit script assumed 26 multiline functions; actual source has
25 plus inline init. Correcting that count produced the preservation result above;
this script mistake neither executed the UUT nor established feature red.

No new regression body was run, no red/pass result claimed and no production fix
authorized by this author. A different reviewer must approve these actual
assertions first. Root then observes genuine current-source failure after the
successful build, authorizes a separately owned correction, and requires the
unchanged original22 plus these two regressions and all other applicable checks
to pass with code review. Latest-head CI/discussions/rules/normal merge gates and
all migration/application/operational/provider/platform limits remain in force.

## Coordinator observed intended red after actual approval

Actual separate test approvalad4f54d was integrated as8c70e6e BEFORE new body
execution. At8c70e6e the root build was already compiled successfully, with
productionblobd91c7d and testblobe94befdc. Root ran exactly:

```sh
ctest --preset sqlite-migrations-debug -R '^sqlite-migrations\.(canonicalCodecPreservesInitialBom|storedSchemaPreservesInitialBom)$' --output-on-failure
```

Actualexit8, both bodies failed for valid initialU+FEFF rejection with reason2
invalid_manifest: purecodec exception and runner manifest admission. All init/
cleanup passed, zero skips/crashes/dependency/fixture failures. The stored case's
raw-byte and literal fixture guards passed before admission, establishing a real
valid storedschema. [Raw original output](storage-011-unicode-red-ctest.txt).
This is the new narrow regression red; original22 stub-red was not repeated.
Source correction is now authorized within the unchanged contract/assertions.
