# STORAGETEST-001 independent baseline review

Author: `/root/storage_test_author`, separate session.
Reviewer: `/root/storage_test_reviewer`, 2026-11-01.
Reviewed integrated candidate: `48db98c`, including author `819edbb` and root
build/CI integration. Review branch: `task/STORAGETEST-001-writer-review`.

Verdict: **approved for test baseline**. No blocking findings remain. This permits
the coordinator to observe the intended stub-red after this approval is committed;
it does not approve implementation or claim that behavior has run.

## Exact artifacts and authority

| Artifact | Git blob |
| --- | --- |
| SQLite writer revision 1 contract | `4d280461966f3fd9fcb13ab6d054c88228ca05b8` |
| Public writer/reader interface | `11d204aab3340bc670df08716ae1c80cae0c337f` |
| Explicit missing-behavior stubs | `f0edd7e6c504c152f6412b186b80c5d9485e1ba3` |
| All 22 Qt Test slots | `f8418e83dbd88d74505c047e6b3e2c30304ade69` |
| Qt baseline workflow | `9144d8b51056ae25172d7f03f346a726604087af` |

Read root/server/tests AGENTS, workflow, test strategy, STORAGE-001 brief,
specification v0.45 §2/DB-01, event-state boundaries revision 1, and the bounded
SPIKE-001 handoff. Examined the actual contract, interface, stubs, every test body,
author evidence, CMake targets/presets and CI evidence/workflow at the candidate.

The contract appropriately refines the accepted one-writer boundary. It keeps SQL
on the existing authoritative Qt command/state thread and exposes owned batches
and owned result rows, with no raw SQL handles or independent writer thread.
Process-local path ownership supplements SQLite locking; it does not prohibit
external processes. Separate readers use actual SQLite read-only mode, rather than
treating the toggleable query_only setting as a permanent write boundary.

Trusted SQL limits, restricted leading statements, semicolon rejection, zero busy
timeout and terminal writer shutdown are explicit local engineering choices. They
do not alter contribution semantics or claim final migration/schema policy.
The payload budget charges every value a nonzero base plus string/blob bytes;
it does not pretend to measure QVariant heap allocation or bound arbitrary SQL
execution time. Deferred foreign keys are a fixture for COMMIT failure, not a
chosen product schema.

Primary framework assumptions were checked against current official documentation:
[Qt connection ownership/lifetime](https://doc.qt.io/qt-6/qsqldatabase.html),
[QSQLITE options](https://doc.qt.io/qt-6/sql-driver.html),
[SQLite WAL](https://sqlite.org/wal.html),
[PRAGMA configuration](https://sqlite.org/pragma.html), and
[deferred foreign keys](https://www.sqlite.org/foreignkeys.html).
These sources support the test mechanisms; they do not establish product behavior.

## Independent examination of all test oracles

| Case | Examined expected result and basis |
| --- | --- |
| opensWalFullOnOwnerThread | Results from the writer connection itself must be wal, FULL=2, foreign_keys=1 and busy_timeout=0. Callback is asynchronous on the owner. DB-01 and the slice's lock policy. |
| commitsFifoBatchesBeforeCompletion | No pre-dispatch write; literal independent sums 10 then 30 in completion order 1,2. Mutating/clearing the caller's admitted batch cannot change row1/amount10. Owned queue and commit-before-publication boundary. |
| rejectsDuplicateWriterUntilShutdown | Equivalent `/./` path cannot acquire a second process-local writer; a different object can acquire it after actual shutdown completion. |
| readerCannotWriteEvenAfterQueryOnlyOff | Two INSERT attempts fail, including after query_only is disabled; the independent persistent count remains zero. Proves the actual read-only open requirement. |
| readerIsOwnedByItsOpeningThread | Foreign read/close reject with wrong_thread; original owner remains able to read and close. |
| writerRejectsForeignThreadBeforeAdmission | Foreign enqueue/shutdown/open reject; their callbacks never run, and the owner can still commit only row2/amount20. |
| validatesLimitsAndBatchBeforeAdmission | Zero capacity, invalid paths, missing callback, empty/multiple statements, unsupported type, valid SQL101 bytes and UTF-8 value18 budget reject. The same valid SQL at exactly100 bytes and value16 succeeds, distinguishing resource checks from invalid SQL. |
| pendingCapacityRejectsQueuedOverflow | Capacity2 accepts two queued batches and rejects row3 without completion or persistence; literal count2/sum30. |
| rollsBackWholeBatchOnSqlFailure | Row2 inserted before a duplicate key is rolled back; original count1/sum10 remains. A later valid row3 commits, yielding40. |
| busyBeginFailsWithoutReplay | An independent BEGIN IMMEDIATE lock stays held until the writer reports busy with no results. After release, only a newly admitted row5/amount50 persists; row4 is not replayed. |
| readerSnapshotDoesNotBlockWalCommit | A real read transaction retains sum10 while the writer commits another row. After reader COMMIT, a fresh query sees30. DB-01 WAL concurrency. |
| shutdownDrainsAcceptedWorkAndRejectsNewWork | Accepted batches complete1,2 before shutdown3, while row3 admission rejects. Actual persistent count2/sum30 confirms drain. |
| shutdownIsIdempotentAndReleasesConnection | Each shutdown callback runs once, connection inventory returns to its pre-writer set, and reopening the terminal object rejects. |
| closedLifecycleIsExplicit | Unopened enqueue returns not_open; null shutdown is invalid without terminal transition; unopened shutdown completes asynchronously and becomes terminal. Reader close is idempotent/reusable and repeat-open is explicit. |
| readerMissingFileDoesNotCreateDatabase | Missing-file read-only open fails without creating a file, then valid open remains possible. |
| failedOpenDoesNotPublishAWriter | Opening a directory fails without leaking a registered connection or preventing a later valid open. |
| completionCanEnqueueAfterCommit | At capacity1, a completed slot is freed before its callback admits another batch; both succeed and independent sum is30. |
| rejectsTransactionControlStatements | Transaction commands, mutating PRAGMA, ATTACH and a semicolon sequence reject the entire batch before a preceding INSERT can run. No callback or row appears. |
| completionExceptionDoesNotUndoCommitOrStopQueue | A synthetic post-commit callback exception is contained; the next batch completes once and count2/sum30 remain committed. |
| boundsResultRowsAndRollsBack | Two actual result rows exceed limit1 and roll back the prior INSERT, exposing no partial rows. |
| boundsResultBytesAndRollsBack | UTF-8 result value18 exceeds limit16 and rolls back the prior INSERT, exposing no partial rows. |
| failedCommitRollsBack | Deferred missing-parent failure occurs at COMMIT, leaves no child, and a subsequent valid parent/child batch commits child2. This checks rollback/reset beyond a statement error. |

The fixture creates disposable tables and reads literal persisted results; it never
implements queue admission, transaction execution or the expected writer decisions.
Expected results are not generated by the implementation under test. All storage
operations in the candidate remain unconditional feature_not_implemented returns.
No skipped test, compile failure, missing driver or inverted failure can establish red.

Before approval, author refinements resolved an SQL-byte oracle that was also
invalid SQL, clarified otherwise unspecified closed/open/shutdown lifecycle cases,
added actual busy_timeout=0 observation, and directly checked owned caller data.
These were changes to an unapproved draft; no reviewed baseline was weakened.

## Harness, CI and coverage limits

The default-off flag adds one Qt Core/Sql library and one Qt Test executable.
CTest registers each actual slot in a separate process with a 15-second failure
timeout and explicit draft/acceptance labels. Existing suites and assertions are
unchanged. No timeout is success, and no WILL_FAIL/continue-on-error/skipping is used.
Warnings about SQL/thread/live-handle misuse are test failures. QTRY waits for
observable event-loop completion; no sleep-based ordering determines an oracle.

The workflow builds/discovers all slots and runs the existing Qt event-loop probe
on normal PRs, while manual acceptance propagates the actual intentional failure.
The JSON/function-set guard rejects missing, mismatched or duplicate registrations.
It does not run or claim passing writer behavior. Qt archives are checksum-pinned
and the selected minimum remains 6.12. The separate CI investigator's approval and
actual archive consumer evidence are in [the CI assessment](storagetest-001-ci.md).
Ubuntu workflow execution remains a coordinator gate before merge. STORAGE-001
must promote these unchanged reviewed cases to automatic passing acceptance.

Coverage is a focused boundary baseline, not an exhaustive proof. Counting the
active synchronous transaction within outstanding capacity, exact handle ownership,
all invalid-limit variants, all binding/result types, reader row limits, all SQL
prefix forms, thread lifecycle and cleanup paths still require careful implementation
review. The current public interface has no clean test hook inside active SQL;
adding a production hook solely to test that invariant is unnecessary. Rollback
failure/uncertain durable COMMIT, crash/power-loss recovery, migrations, app-level
atomicity, process deadlines, pathological SQL performance and backup/restore stay
explicit downstream gates. No API bridge, provider, deployment, Windows or release
support follows from this baseline. All seven integrations and three rate providers
remain required.

## Commands actually run by this reviewer

Reviewer worktree, GNU16.2.1, CMake4.4.5, Python3.14.8,
installed Qt Core/Sql/Test6.12.0:

| Command | Observed result |
| --- | --- |
| `cmake --preset sqlite-writer-debug` | exit0, Qt dependencies configured |
| `cmake --build --preset sqlite-writer-debug` | exit0, all 12 build steps completed |
| `ctest --preset sqlite-writer-debug -N -L sqlite-writer` | exit0, exactly22 cases listed |
| CTest JSON/compiled `-functions` exact-set/count comparison | exit0, all22 slots registered once |
| `pkg-config --modversion Qt6Core Qt6Sql Qt6Test` | all6.12.0 |
| `git diff --check` | exit0 |
| `git ls-files -s` for baseline artifacts | exact blobs above confirmed |

No test bodies ran in this review. Compilation and discovery are not observed red.
The coordinator must integrate this committed approval, run the actual22 cases,
retain genuine missing-behavior failures, and review final evidence/status before
baseline merge. STORAGE-001 remains unimplemented pending that gate.

## Final baseline integration review — 2026-11-01

Reviewer `/root/storage_test_reviewer` examined integrated task candidate `da98505`.
Verdict: **approved for baseline integration**; no blocking source/test/evidence
findings. The original review commit was cherry-picked by the integrator, so this
reviewer preserved its original local history with an ordinary support-branch merge
after an ff-only update correctly refused divergent commit identities.

The approved contract, public interface, explicit stubs, all22 assertions and all
CMake/preset build registrations have no diff from `48db98c`. The retained red log
contains22 distinct failed behavior slots, each reporting enum1 from an unconditional
feature_not_implemented stub against required success/validation/error results.
All22 initTestCase and cleanupTestCase entries pass, every summary reports zero
skips, and the combined CTest summary is22 failed with actual recorded process exit8.
Reaching those slot assertions also requires their real disposable QSQLITE fixtures
to initialize successfully. Approval integration `6647490` precedes this observed
run. No repeated behavioral execution was needed for this evidence review.

Reviewed handoff, architecture/test strategy, acceptance-map DB-01 partial status,
task state and generated progress. STORAGETEST-001 remains in_review pending normal
publication closeout; its baseline review/intended-red fields are accurate and
STORAGE-001 remains planned. No writer behavior is relabeled implemented/passing.
The coordinator will replace obsolete next-action checkpoint prose and record actual
CI/completion metadata before final publication. All downstream release gates remain.

The only executable integration delta is workflow prefix initialization: unavailable
job-level runner-context expansion is replaced by a prior step writing QT_PREFIX
through GITHUB_ENV. This does not change archive pins, build commands, test discovery
or intentional-red separation. Corrected workflow blob
`398733ebeb7bd2c83d4396def554979f01ef4fd1` has separate actionlint investigation/
approval in the CI assessment. This reviewer also inspected its actual diff.

This reviewer independently queried
[run41288299328](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41288299328)
at exact head `da98505a61dae406d5d5706eac481ed9b8c3d764`: completed success.
Actual logs confirm both archive SHA checks passed, GNU13.3.0/Python3.12.3 configured,
the target built, the Qt event-loop probe passed1/1 and the guard discovered all22
compiled slots. Writer behavior was deliberately not run by that PR baseline job;
its green result is build/environment/registration evidence only.

Actual final reviewer checks: red-log count/result audit and reviewed-artifact diff
comparison passed; `python3 tools/check_project.py` exit0 (377 tasks,122 decisions,
links/presets/current HTML), `python3 tools/progress.py render --check` exit0 and
`git diff --check` exit0. GitHub run status/log inspection was read-only. No native
test bodies were rerun and no assertions/source/build/progress files were changed.

Latest-revision CI after further metadata commits, final status/publication records,
resolved discussions, repository rules/mergeability and merge remain coordinator
gates. This review approves the bounded reviewed/red baseline only. Production
implementation still starts from its integrated prerequisite and needs unchanged
acceptance passes, separate code review and automatic behavior CI.

## Completion metadata review — 2026-11-01

Reviewer `/root/storage_test_reviewer` examined completion candidate `ca05b1d`.
Verdict: **approved for final baseline publication**, subject to the coordinator's
latest-head CI/discussion/rules/merge checks. This narrow pass changes no behavior
approval or remaining release gate.

Compared with previously examined `da98505`, contract/interface/stubs/tests,
CMake/presets and corrected workflow are unchanged. Only task evidence, completion
state, generated HTML, checkpoint and the already-examined review are added.
STORAGETEST-001 done accurately means authored tests, separate test review and
actual intended red; product_acceptance remains not_run and STORAGE-001 remains
planned. The baseline=observed_red field now uses the existing validator's required
name; no tooling gate or validator was changed. Native/test-review/GitHub fields
link actual prior evidence rather than implying new runs.

CI outcome is attributed to actual run41288299328 at da98505, with build/probe/
registration scope and latest-head CI distinguished. The checkpoint's next action
now closes PR24 publication gates, refreshes main after merge and starts STORAGE-001
from the integrated baseline. Completed authoring/setup/red work is not restarted.
Its preserved startup audit and prior notes are historical observations; live status
must still be inspected on resumption, as the checkpoint instructs.

Actual reviewer checks: `python3 tools/check_project.py`,
`python3 tools/progress.py render --check`, `git diff --check`, and exact
reviewed-source/build/workflow diff comparison all exited0. No native tests or
behavioral red were repeated. Only this review note was edited by the reviewer.
