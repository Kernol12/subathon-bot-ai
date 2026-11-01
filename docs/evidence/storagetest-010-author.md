# STORAGETEST-010 author evidence

Author: `/root/storage_decomposition`, 2026-11-01. Branch
`task/STORAGETEST-010-author`, isolated `storagetest010-author` worktree.
Authority: specification v0.45 §2/§3.4, DB-01/SERVER-01;
[STORAGE-010 brief](../tasks/EPIC-009/STORAGE-010.md) and
[writer transaction contract revision1](../contracts/writer-transactions-v1.md).

## Scope and status

Authored a concrete additive API and 24 Qt Test cases; successfully compiled and
listed them. Independent review, approved observed-red execution and production
implementation remain separate gates. No new work test body has been executed by
this author. Compilation/discovery are not an observed-red baseline or product
acceptance. `STORAGE-010` remains unimplemented.

Owned paths are additive declarations in `server/include/subathon/server/sqlite_writer.hpp`,
new `server/src/sqlite_writer_work_stub.cpp`, new `tests/qt/sqlite_writer_work_tests.cpp`,
this evidence, and the explicitly approved narrow new hook header
`server/include/subathon/server/testing/sqlite_writer_work_faults.hpp`.
The coordinator owns CMake/presets/workflow/progress/HTML/checkpoint. Incorporated
its shared harness by fast-forward to `eea1e10` before the first API commit
`6532c07ddb2d38658bf789bb27c2c1752bfaa932`.

Existing `sqlite_writer.cpp`, both old test files, old25 assertions and contracts
were not changed. The old targets also compiled with the additive header. No
transaction, queue, lifetime or SQL algorithm is supplied by the new stub: work
admission/fault installation explicitly return `feature_not_implemented`; work
operations throw that error and noexcept cleanup returns that error.

## Concrete protocol and error mapping

`enqueue_work(Work, WorkCompletion)` accepts a trusted owning callback returning
`QVariant`. Completion separates `TransactionOutcome`, `WorkError` and an optional
owning logical value. A normal value is permitted only with confirmed commit and
normal body return. Synchronous `begin/execute/commit/rollback` expose no Qt SQL
handle. Copyable context/session tickets hold only weak state; default, expired or
revoked throwing operations report `stale_session`; same-body terminal session
operations report `session_closed`, a concrete protocol-error category. A second
begin reports `protocol_error`, including after busy/commit/rollback. Sticky first
framework failure wins over subsequent protocol/body errors; uncertainty wins
over known outcomes. `cleanup()` is noexcept: stale/inactive no-op returns `none`,
live foreign returns `wrong_thread` without mutation, active unknown rollback
returns `uncertain_outcome` and fail-closes the writer.

SQL text length, statement-count and bound-byte overages use `resource_limit`;
result row/value-byte overages use `result_limit`; prohibited SQL and unsupported
binding QVariant types use `invalid_statement`. SQL execution/known failed commit
use `sql_failure`; the initial busy begin uses `busy`. The existing receipt port's
known-rollback error category is only used for known uncommitted storage failures;
unknown durability crosses the real service as distinct `SqliteWorkFailure`.

Old queued batches behind unknown work retain their existing closed-dispatch
`StorageError::sql_failure` and empty rows; new queued work gets
`not_started/writer_unavailable`, new admission gets `shutting_down`. This matches
the existing closed-connection path without adding a new old-batch outcome claim.
No assertion requires the process-local path lease to remain held after uncertain
closure; later recovery authorization is outside this mechanism slice.

The hook is a one-shot, owner/idle writer installation. It is consumed only after
the targeted real successful work SQL COMMIT/ROLLBACK, before its acknowledgement
reaches normal classification. Old batch controls, failed controls and inactive
cleanup do not consume it. It cannot supply a computed result or fake SQL executor.
The implementation reviewer must verify its actual placement below classification.
Physical rows are independently checked: lost commit acknowledgement persists30,
lost rollback acknowledgement persists0, queued old/new work does not execute and
there is no replay. This is deterministic acknowledgement-loss simulation, not
disk/power-loss proof.

## Literal oracle coverage

| Contract example | Compiled cases and key boundary |
| --- | --- |
| WT01 | `mixedBatchAndWorkAreFifo`: old batch10/work20/later read30, completion1/2/3, body observes prior completion only |
| WT02 | `dynamicReadDecideWriteIsAtomic`: real read drives second write; independent rows0 before and2 after commit |
| WT03 | `serviceCommitIsVisibleBeforeBodyReturns`: real ReceiptIntake returns only after primitive session commit; independent sum30 inside body before completion |
| WT04 | `returnWithoutCommitRollsBack` (active return and explicit rollback); `exceptionBeforeCommitRollsBackAndQueueContinues` (no replay, following healthy work) |
| WT05 | `caughtSqlErrorStaysStickyAndCannotCommit`: duplicate key rolls back earlier write; commit/second begin cannot rescue; later body error cannot replace first SQL failure |
| WT06 | `busyBeginDoesNotReplay`: completion while external lock held; second begin is protocol error; releasing lock never replays item |
| WT07 | `failedCommitRollsBackBeforeThrowing`: deferred foreign key fails real commit, then later old batch succeeds |
| WT08 | `exceptionAfterCommitPreservesCommittedOutcome`: committed row persists exactly once, work failure has no success payload, later batch runs |
| WT09 | `lostCommitAcknowledgmentFailsClosed`: physical sum30, ordered queued old/new failure/no SQL, closed admission/reopen |
| WT10 | `lostRollbackAcknowledgmentFailsClosed`: physical rows0, uncertainty, ordered queued old/new failure/no SQL |
| WT11 | `retainedCapabilitiesCannotReachLaterWork`; `capabilitiesAfterWriterDestructionAreRevoked`: ownable copied tickets, no dangling-reference calls, no effect on later owner work or connection lifetime |
| WT12 | `foreignCapabilitiesDoNotPoisonOwner`: begin/execute/commit/rollback/cleanup rejected on foreign thread, owner commits30 |
| WT13 | `secondBeginAndSqlControlAreRejected`: second begin while active/after commit/after rollback, prohibited controls/ATTACH/DETACH/VACUUM/PRAGMAs/semicolons |
| WT14 | `cumulativeBudgetsHaveExactBoundaries`: exact and over boundaries after speculative write for statement count, per-statement UTF8, cumulative bindings, result rows/bytes, scalar/null charges; unsupported QVariant |
| WT15 | `activeCapacityLastsUntilBodyReturns`: capacity2 active plus queued, both pre/postcommit admissions full, completion admission succeeds |
| WT16 | `shutdownDrainsWorkAndBatches`: active commit, accepted mixed FIFO drain, new rejection, path released before shutdown callback |
| WT17 | `throwingOrDestroyingCompletionIsContained`: exception containment and completion destruction after fixed terminal result |
| WT18 | `ownedRequestAndResultOutliveCallerAndSession`: explicit owning captures, discarded caller request, result text/blob valid after session/writer closure |
| WT19 | `unknownOutcomeIsNotKnownRepositoryFailure`: real ReceiptIntake plus primitive repository, known SQL rollback category and distinct uncertain-commit category |
| WT20 | `returnWithoutBeginHasNoSuccessPayload`: normal no-BEGIN return suppressed; exception before-BEGIN remains not_started/work_exception |
| Additional admission | `admissionErrorsDoNotInvokeWork`: closed/missing callable/wrong thread/full/shutdown, no rejected work/completion call |
| Additional cleanup | `cleanupIsNonthrowingAndDoesNotRepeatRollback`: noexcept/default/stale semantics, committed cleanup cannot consume pending rollback-ack hook, active cleanup uncertainty dominates normal return |

Independent zero-wait BEGIN IMMEDIATE followed by ROLLBACK is attempted inside
the body's catch for duplicate-key, deferred-commit, later-call budget and known
service SQL failures. This proves rollback released the writer transaction before
the synchronous failure was exposed; external row invisibility alone is weaker.
Resource oracles use literal8+payload charging: UTF8 é costs10, blob7 costs15,
null/scalar costs8; exact10+15=25 versus26, and SQL text48 versus49 UTF8 bytes.
No expected output is generated by production algorithms.

The synthetic primitive receipt adapter maps create/append to generic test rows101
and102 and delegates every physical transaction operation to the session. The
actual service supplies its call order; this adapter does not implement real
receipt row encoding, schema migration, money/ID serialization, receipt dedup or
provider semantics. Domain IDs, lossless decimal/money preservation and final
row/wire formats remain downstream contract/storage work.

## Commands and CI assessment

Environment: Linux x86-64, GCC16.2.1, CMake4.4.5, Ninja1.13.2, Qt6.12.0 from
`/usr/lib/cmake/Qt6`. No new dependency or personal preset.

Commands actually run in this worktree:

```sh
clang-format -i server/include/subathon/server/sqlite_writer.hpp server/include/subathon/server/testing/sqlite_writer_work_faults.hpp server/src/sqlite_writer_work_stub.cpp tests/qt/sqlite_writer_work_tests.cpp
cmake --preset sqlite-work-debug
cmake --build --preset sqlite-work-debug
build/sqlite-work-debug/tests/qt/server_sqlite_work_acceptance -functions
ctest --preset sqlite-work-debug -N
git diff --check
python3 tools/check_project.py
python3 tools/progress.py render --check
```

Configure/build passed, including actual `subathon_receipt_intake`, new target and
the old writer/ownership targets. Discovery printed the exact24 named slots and
CTest listed53 cases:24 new work,25 unchanged writer and4 scaffold.
An additional Python comparison of the compiled `-functions` names against
`ctest --preset sqlite-work-debug --show-only=json-v1` passed exact nonempty24 name
equality. `git diff e3f24bd -- server/src/sqlite_writer.cpp
tests/qt/sqlite_writer_tests.cpp tests/qt/sqlite_writer_ownership_tests.cpp` was
empty. Diff whitespace, project391 records/122 decisions/local links/presets/HTML
and current progress render checks passed. This author did
not run work acceptance or claim any feature test red. Coordinator-owned harness
and CI assessment are in [CI evidence](storagetest-010-ci.md); ordinary jobs preserve
old acceptance and compile/discover new cases, explicit reviewed dispatch observes
new red. STORAGE-010 must promote unchanged reviewed work acceptance to automatic
execution when implemented.

The full candidate still needs independent assertion/hook review before baseline
execution. No hard arbitrary-SQL deadline, arbitrary captured-pointee ownership,
live provider behavior, application recovery or platform release gate is claimed.
