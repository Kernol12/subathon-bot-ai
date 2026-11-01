# STORAGE-011 implementation evidence

Author: `/root/debt_code_reviewer`, explicitly reassigned as implementer,
2026-11-01. Branch `task/STORAGE-011-implementation`, isolated worktree
`local-evidence/worktrees/storage011-implementation`, from freshly merged
main `ef29c87` / PR31. Authority: specification v0.45 §16, DB-01/OPS-02,
[task brief](../tasks/EPIC-009/STORAGE-011.md) and
[migration catalog revision1](../contracts/migration-catalog-v1.md).

## Scope and exact artifacts

Owned paths are only `server/src/sqlite_migrations.cpp` and this evidence. Root
owns shared build/presets/workflow/state/map/HTML/checkpoint. The shared branch
was normally merged through `66be523` before compilation; that coordinator change
promotes unchanged migration cases to ordinary CI after the mandatory green probe.

Actual source candidate: `9eb1fef2aec74dc2a841ed68cec0e5c84ed4840d`.
Source blob: `d91c7d6831f656ccba93e077d8bf9f541db65a80`.
The explicit stub source was replaced in its existing file; no file/interface/test
removal or new dependency was needed.

The [baseline handoff](storagetest-011-handoff.md),
[independent actual review](storagetest-011-review.md),
[original intended-red output](storagetest-011-red-ctest.txt) and
[official-Qt prerequisite evidence](storagetest-011-ci.md) establish the completed
test-first gate. Its original red was not repeated. Exact preserved artifacts:

| Artifact | Git blob |
| --- | --- |
| Migration contract | `cbdea1a48d2989a9c8bd17edf0539bdf7c1bce61` |
| Public migration header | `02f83ce592b110176c1dac46f3e5ea4d93529fcd` |
| New22 Qt cases | `a85a6c057637f43201dfaca308fbec27748ce2dd` |
| Separate green7 diagnostic probe | `832fe61656451390ff465d4411c7d0900c741bd6` |
| Frozen independent vectors | `12280c8f403fd3902edaeff88e9a236f9c5b613b` |
| Existing writer/work implementation | `023451987e40a9330fef91c6c0955f0e527f14e6` |

An actual `git diff ef29c87 -- server/include tests/qt/sqlite_writer_tests.cpp
 tests/qt/sqlite_writer_ownership_tests.cpp tests/qt/sqlite_writer_work_tests.cpp
 tests/qt/sqlite_migration_tests.cpp tests/qt/sqlite_migration_environment.cpp
 tests/fixtures docs/contracts server/src/sqlite_writer.cpp` is empty. Existing
writer headers, hook, contracts and all49 assertions remain unchanged.

## Implemented behavior

Pure encoding validates UTF-16 scalar structure, rejects embedded NUL/unpaired
surrogates, measures exact UTF-8 lengths without allocating encoded text, and
checks the complete F-framed size before allocating the encoded fields, sort keys
or output payload. Schema tuples sort by unsigned UTF-8 field bytes with distinct
NULL/present markers; NULL and present empty SQL retain different frames. Existing
Qt Core SHA-256 hashes that exact payload. Pure codec boundaries remain separate
from runner acceptance: an encodable object need not be a valid catalog schema.

Pure request preflight checks the mode, format, fixed identity, positive limits,
consecutive 1..N versions, unique printable ASCII IDs with the effective hard128
cap, ordered unique supported starting versions including0/N, exact manifest byte
accounting, duplicate projection tuples and required unchanged reserved catalog
objects. Fixed identity is independent of the configurable migration-ID limit.
The wrapper checks request validity before completion presence, then delegates
unchanged admission to `enqueue_work`; rejected requests run no SQL/probe/completion.
Accepted work captures ordinary owned request values and precomputed content hashes.

Each request executes through exactly one existing work context/session and one
real BEGIN IMMEDIATE. The first inspection is the explicit `pragma_encoding()`
function call, before any raw-text/schema/catalog observations. Catalog reads and
writes explicitly qualify main. Exact schema projection includes both reserved
DDL strings, their generated NULL-SQL UNIQUE index and every other main object.
Stored header/history validation checks all rows, actual SQLite typeof values,
integer ranges/cardinality, raw TEXT bytes, printable unique IDs, the full prefix
and every known content hash. It refuses partial/foreign/future/unsupported or
inconsistent state without conversion, repair, truncation or inferred initialization.

Strict fixed function calls independently check full integrity and foreign keys;
ordinary shadow tables fail inspection. Final verification repeats encoding,
schema, header, history and both integrity checks. Advancement bootstraps only a
strictly empty store, executes every missing version, verifies each intermediate
projection before its history append, then performs the real synchronous commit.
Bootstrap, DDL/data and the entire appended history prefix are one transaction.
Inspection never applies DDL/history changes and reports fresh/upgrade-required/
current accurately; already-current advancement verifies without replay.

A private per-request bounded failure/report record composes with the unchanged
`SqliteWorkResult`. Custom refusals and cancellation establish rollback through
noexcept session cleanup before throwing. Cancellation callbacks run only at the
specified cooperative checkpoints; callback exceptions record callback_failure.
The first migration/framework reason remains authoritative; unknown outcome always
overrides it. Busy, SQL and resource errors retain the exact underlying work
category. No failed or uncertain completion exposes a normal report/value.
Completion state owns its data and accesses no writer after invoking user code,
including throwing/destructive completions.

Every diagnostic, schema/catalog query/write and migration statement consumes the
existing cumulative session budgets. No second connection, direct Qt SQL/query
object, native handle, new thread, nested event wait, retry, hidden budget increase
or split transaction was introduced. Raw-backed/borrowed mutable request data
remain outside the trusted caller's ownership precondition.

## Actual verification

Linux x86-64, GNU16.2.1, Qt6.12.0 with SQLite3.54.0, CMake4.4.5,
Ninja1.13.2, Python3.14.8. Synthetic disposable files only.

- `cmake --preset sqlite-migrations-debug`: exit0.
- `cmake --build --preset sqlite-migrations-debug`: exit0; actual migration,
  writer, work, receipt-service composition and diagnostic executables compiled.
- `ctest --preset sqlite-migrations-debug -L sqlite-migrations --output-on-failure`:
  exit0, all22 bodies passed on the first compiling implementation. A final source
  refinement then moved strict UTF-8 sizing and complete-frame overflow checking
  before encoded-field allocation; the final full run includes all22 again.
- `ctest --preset sqlite-migrations-debug --output-on-failure`: exit0 at the
  candidate source above, 76/76 pass: new22, old49, infrastructure5. The last group
  includes the separate seven-slot diagnostic environment executable.
- `ctest --preset sqlite-migrations-debug -R '^scaffold.sqlite_migration_environment$' -V`:
  exit0, actual Qt6.12.0/SQLite3.54.0, all7 cases plus init/cleanup passed,
  zero failures/skips. This records the host runtime, not official bundled CI.
- CTest JSON discovery plus Python comparisons with each compiled executable's
  `-functions`: exact nonempty22 migration,24 work and25 old writer slots match;
  full count76, and exactly7 unique environment slots. No missing or duplicated
  registration was accepted.
- `clang-format --dry-run --Werror server/src/sqlite_migrations.cpp`: exit0.
- `python3 tools/check_project.py`: exit0,391 task records/122 decisions and links/presets/current HTML valid.
- `python3 tools/progress.py render --check`: exit0, root-owned snapshot current.
- `git diff --check`: exit0.

Independent source/config review, actual selected Ubuntu/official-Qt CI and final
latest-head metadata/discussion/merge gates remain required. Local passing results
do not independently finish the task. Root owns those gates and durable progress.
OPS-02 actual operator initiation, restorable backup, maintenance exclusion and
compatibility/update orchestration remain trusted caller/integration requirements;
no invented request field proves them. No domain migration/store schema, app bundle,
recovery authorization, disk/power-loss, platform or live-provider claim follows.
