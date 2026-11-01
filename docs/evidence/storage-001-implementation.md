# STORAGE-001 — Qt SQL writer implementation

Implementer: `/root/storage_implementer`, 2026-11-01.
Branch: `task/STORAGE-001-implementation`, based on claimc76bfb0 and merged
mainc2ca25a. Source revisionf2a6379, blob
`a8e5c8e80ebb9bde8321d01f7fbc1b1d826c2811`.
Only `server/src/sqlite_writer.cpp` and this evidence note are owned here.
Coordinator retains build/workflow/progress/map/checkpoint and PR ownership.

## Reviewed prerequisite and preserved contract

Authority: specification v0.45 §2/DB-01, event/state boundaries revision1 and
[writer revision1](../contracts/sqlite-writer-v1.md).
[Baseline handoff](storagetest-001-handoff.md) retains exact contract4d280461,
interface11d204aa, testsf8418e83 and the22 intended stub failures at6647490,
observed after independent review3e7c8c7. PR24 merged as c2ca25a before this task.
No baseline assertion, fixture, public interface or contract was changed here.

## Implementation and ownership decisions

The writer connection and queued work stay on the constructing authoritative Qt
thread. Each admitted owned batch gets a separate Qt queued dispatch, one
BEGIN IMMEDIATE/ordered-statement/COMMIT transaction, and one completion after the
result is fixed. The active batch remains in admission accounting through commit
or rollback; its slot is freed before the completion may enqueue more work.
No second thread or event loop is created.

Private connection RAII closes/removes named connections after operation-local
queries are destroyed. Method guards reject foreign threads before SQL handle
access; destructor ownership remains the caller's contract and is asserted.
Canonical existing paths resolve aliases; new files resolve their existing parent.
A mutex protects the process-local canonical-path writer lease across owners.
Failed open/shutdown/destruction releases that lease after connection closure.

Writer open configures/verifies actual WAL, synchronous FULL, foreign_keys ON and
busy_timeout0. Batch admission checks the reviewed trusted-command allow list,
semicolon rejection, positive limits, statement/SQL/value budgets and supported
bound variants. Counters subtract from remaining capacity to avoid overflow.
Rows and value bytes are accumulated across the entire transaction, with no partial
result or preceding write exposed after SQL/COMMIT/result-bound failure.

SQLite native primary/extended busy/locked codes produce `busy`, without replay.
Known transaction failures roll back; failed rollback closes admission and its
connection. Already-admitted remaining batches still complete with errors rather
than disappearing. Unknown durable commit resolution remains a later application/
recovery responsibility. No arbitrary trusted SQL execution deadline is claimed.

Reader connections are separate, private and opened with actual
QSQLITE_OPEN_READONLY plus query_only. The latter can be disabled diagnostically
without changing the real file access flag. Reader snapshots/transactions use the
reader's owning thread and return owned rows. No authoritative writer handle escapes.

Shutdown closes admission immediately, drains accepted work FIFO, closes/removes
connections and then invokes owned shutdown completions asynchronously. Repeat
shutdown is safe and terminal. User callback exceptions are contained after the
transaction result is fixed. Subsequent dispatch is scheduled before invoking a
write completion, with no implementation/member access after user code; callbacks
can reenter or destroy the writer without a later dangling implementation access.

## Actual commands and results

Local Linux x86-64; GNU16.2.1; Qt Core/Sql/Test6.12.0; CMake4.4.5;
Ninja1.13.2; Python3.14.8. Real disposable QSQLITE files; no provider endpoints,
credentials, live contribution data, application state or extra dependencies.

| Command | Actual result |
| --- | --- |
| `clang-format -i server/src/sqlite_writer.cpp` | Exit0, repository style |
| `cmake --preset sqlite-writer-debug` | Exit0; real Qt dependencies found |
| `cmake --build --preset sqlite-writer-debug` | Exit0; no compile warnings |
| `ctest --preset sqlite-writer-debug -L sqlite-writer` | Exit0; all22 unchanged cases pass,0skips |
| `ctest --preset sqlite-writer-debug` | Exit0;25/25 including scaffold/tooling/current project checks |
| `git diff --check` | Exit0 |
| `pkg-config --modversion Qt6Core Qt6Sql Qt6Test` | Each6.12.0 |

The first compiled implementation passed all22; no test or fixture was modified
and no additional regression case was authored or claimed. Active-transaction
accounting, private handle/thread discipline and fatal rollback handling still
require the separate implementation review beyond directly exercised assertions.

## Remaining gates

Independent code/configuration review, coordinator integration, automatic acceptance
promotion and actual Ubuntu CI are pending. This note is not an approval or task
completion. The coordinator must preserve the baseline and observe all latest PR
gates before merge.

No application schema/migration/repository adapter, authoritative accounting/effect/
job aggregate transaction, API-thread bridge, hard query/shutdown deadline, abrupt
crash/power-loss durability, backup/recovery, Windows or release/provider/platform
verification is supplied. The low-level trusted-SQL boundary does not define product
input limits or permit a caller to replace exact decimal/accounting contracts with
floating point. Full release scope and downstream gates remain unchanged.

## Reviewed ownership regression and focused correction

The initial sourcef2a6379/blob a8e5c8e passed the original22 cases but did not
fully own accepted raw-backed Qt data: implicit-sharing copies can retain
QString::fromRawData/QByteArray::fromRawData storage. The coordinator held merge
approval and commissioned three separate tests of the existing owned-admission
guarantee. This corrects the earlier implementation's ownership gap; no contract
or product scope changed.

New tests blob`34c25c2786db42f6a4483fc874da9d4d3fc525a3` were independently approved
in94820fa, integratedf79d002 before root ran their bodies. Actual sourcea8e5c8e
failed all3 for the intended value differences (SQL9 instead of1, textzbc instead
ofabc, blob7A6263 instead of616263), with successful fixtures,0skips and exit8.
The approved/regression-red gate was committed1499d57 before the production fix.
[Regression authoring](storage-001-ownership-tests.md),
[independent review](storage-001-code-review.md) and
[actual red](storage-001-ownership-red.txt) retain that sequence. The original
22-case stub-red remains unchanged.

Narrow source correctionbe533bd, blob
`829d9c66e47168384510e8ed2ff436e272686414`, snapshots each admitted SQL QString
and QString/QByteArray binding through their copying data/size constructors.
Null buffers/variants remain null, non-null empty buffers remain distinct, other
scalar values and QVariant types remain unchanged. Existing validation and payload/
row/count limits run before the snapshot; transactions, scheduling and callbacks
are unchanged. No test/interface/contract/shared build file was edited here.

After the correction, actual commands on the same GNU16.2.1/Qt6.12.0 environment:

| Command | Actual result |
| --- | --- |
| `clang-format -i server/src/sqlite_writer.cpp` | Exit0 |
| `cmake --preset sqlite-writer-debug` | Exit0 |
| `cmake --build --preset sqlite-writer-debug` | Exit0 |
| `ctest --preset sqlite-writer-debug -L sqlite-writer` | Exit0; original22 + new3 =25/25,0skips |
| `ctest --preset sqlite-writer-debug` | Exit0;28/28 including3 infrastructure checks |
| `git diff --check` | Exit0 |

Unchanged exact artifacts were rechecked: original22 test blobf8418e83, new3 blob
34c25c27, public header11d204aa and contract4d280461. The implementer owns only
this source and append-only evidence. Independent corrected-source approval,
coordinator integration, actual Ubuntu automatic25-case acceptance and latest-head
PR gates remain required; no application/recovery/provider/platform claim is added.
