# STORAGETEST-001 — authored Qt SQL writer baseline

Author: `/root/storage_test_author`, 2026-11-01.
Branch: `task/STORAGETEST-001-writer-author`, based on coordinator claim `dfd383c`
and integrated main `ba4e94c`. This note records authoring and compile/discovery only;
it is not an independent review, observed red, implementation or passing acceptance.

## Scope and contract

[Revision 1](../contracts/sqlite-writer-v1.md) refines specification v0.45 §2,
DB-01 and the existing [owner/transaction boundary](../contracts/event-state-boundaries-v1.md).
The writer stays on the authoritative Qt command/state thread, with owned queued
SQL batches, private Qt SQL handles, process-local writer ownership, WAL/FULL,
bounded admission, per-batch transaction and drain-before-close behavior. Separate
reader connections use actual SQLite read-only mode as well as query_only.

No product application schema/migration, repository adapter, timer/accounting
application, recovery/backup, provider, deployment or API-thread bridge is supplied.
The writer and reader operations explicitly return
`feature_not_implemented`; they perform no SQL, queue or lifecycle implementation.
The helper does not create another authoritative thread.

Trusted SQL limits, supported leading commands and whole-batch ownership are local
interface choices. Semicolons in SQL text are rejected to avoid a partial SQL
parser/preparation-order dependency; data containing semicolons uses bindings.
Query complexity and a hard execution deadline remain outside this low-level slice.
Every bound/result value carries a base8 charge plus string/blob payload, so empty
or null bindings cannot bypass count bounding. This is a deterministic payload
budget, not a measurement of QVariant heap overhead.

## Authored assertions

The 22 Qt Test slots use disposable SQLite files, real QSQLITE and literal expected
rows/PRAGMA values. Fixture SQL only creates synthetic tables and independent
observations; it does not implement admission, transaction or writer policy.

| Cases | Independent oracle / requirement |
| --- | --- |
| opensWalFullOnOwnerThread | Actual writer SQL results are journal_mode wal, synchronous2/FULL, foreign_keys1, busy_timeout0; asynchronous owner-thread callback |
| commitsFifoBatchesBeforeCompletion | Separate fixture connection sees sums10 then30, in FIFO completion order, no pre-dispatch writes |
| rejectsDuplicateWriterUntilShutdown | Equivalent canonical path cannot acquire another writer; lease released after drain |
| readerCannotWriteEvenAfterQueryOnlyOff | INSERT fails both before and after disabling query_only; persistent count remains0 |
| readerIsOwnedByItsOpeningThread, writerRejectsForeignThreadBeforeAdmission | Foreign methods return wrong_thread; original owner remains usable; rejected callback never runs |
| validatesLimitsAndBatchBeforeAdmission | Invalid paths/zero limit, absent completion, empty/oversized batch, unsupported bound type and valid SQL at101 bytes rejected; exactSQL100/value16 admitted; multibyte value18 rejected |
| pendingCapacityRejectsQueuedOverflow, completionCanEnqueueAfterCommit | Capacity2 accepts two, rejects third with no callback/write; capacity1 callback can enqueue after completed slot is freed |
| rollsBackWholeBatchOnSqlFailure | Row2 inserted before duplicate key does not persist; original sum10 remains; later valid batch yields40 |
| busyBeginFailsWithoutReplay | External BEGIN IMMEDIATE stays held until busy completion; release does not replay rejected row40; fresh row50 commits |
| readerSnapshotDoesNotBlockWalCommit | Actual read transaction sees10 while writer commits another row; post-COMMIT read sees30 |
| shutdownDrainsAcceptedWorkAndRejectsNewWork, shutdownIsIdempotentAndReleasesConnection | Two accepted batches precede shutdown result, new admission rejected, no dropped/duplicate completions; actual connection-name inventory restored |
| closedLifecycleIsExplicit, readerMissingFileDoesNotCreateDatabase, failedOpenDoesNotPublishAWriter | Closed/terminal writer states, null completion, reusable reader close, missing-file no-create and no leaked partial writer connection |
| rejectsTransactionControlStatements | COMMIT/BEGIN/ROLLBACK/SAVEPOINT/END, mutating PRAGMA, ATTACH and semicolon sequence rejected before any first statement/callback |
| completionExceptionDoesNotUndoCommitOrStopQueue | Synthetic post-commit exception contained; next queued batch completes; sum30/count2 remain |
| boundsResultRowsAndRollsBack, boundsResultBytesAndRollsBack | Excess actual rows or UTF-8 result budget rolls back preceding insert and exposes no partial rows |
| failedCommitRollsBack | Deferred foreign-key failure at COMMIT leaves no child; subsequent valid parent/child batch proves connection transaction reset |

QSQLITE absence is an environment failure, never a skip or valid red. Qt SQL/thread/
live-handle warnings are failures via QTest's warning matcher. No sleeps, clock
advancement, provider endpoints, sockets or supporter data are involved. QTRY checks
wait for deterministic Qt event-loop completions; SQL lock remains held until the
busy completion has actually arrived.

## Actual compilation and discovery

Environment: GNU C++16.2.1; CMake4.4.5; system Qt Core/Test/Sql6.12.0; Ninja.
Coordinator owns root CMake/presets and will record their integrated commands.
Author used a disposable local CMake harness outside tracked source, with C++20,
AUTOMOC and `find_package(Qt6 6.12 REQUIRED COMPONENTS Core Test Sql)`. It builds
only the real authored stub source and test executable with the required Qt targets.

Commands from the author worktree:

```sh
cmake -S /tmp/subathon-storagetest001-author-harness -B build/storagetest001-author-compile -G Ninja -DSUBATHON_SOURCE="$PWD"
cmake --build build/storagetest001-author-compile
build/storagetest001-author-compile/server_sqlite_writer_acceptance -functions
git diff --check
```

All exited0. Configure/build succeeded and `-functions` discovered exactly22 behavior
slots without running them. The compile harness had no CTest invocation. No intended
red execution has run; it must follow a separate review of the actual candidate.

## Remaining gates and ownership

Coordinator `/root` owns CMake/presets/workflows, progress/state.json, PROGRESS.html,
acceptance map, checkpoint and PR integration. Author owns only the contract,
minimal public header/explicit source stubs, Qt tests and this note. No shared
progress/build/workflow file was edited here; support branch is not pushed.

Independent review must verify the exact contract/test/interface/stub revision,
then the coordinator can run and retain the intended behavioral red. STORAGE-001
cannot start production behavior before that baseline gate is complete.

Known coverage limits: active-transaction capacity counting is a contract/code-review
obligation because this owner-thread synchronous-SQL API has no clean reentrant
test hook inside SQL execution. Queue capacity before dispatch and after completion
is directly tested. Rollback failure/indeterminate durable commit, abrupt crashes,
power loss, schema/repository app-level atomicity, pathological SQL runtime, actual
shutdown deadlines, deployments and backup/recovery remain separate explicit gates.
