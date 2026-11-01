# STORAGE-001 independent code review

Author: `/root/storage_implementer`, separate session.
Reviewer: `/root/storage_code_reviewer`, 2026-11-01.
Reviewed integrated candidate: `850eef45df3397923dbc96749603ea01122faf9d`
(author source commit `f2a6379`, root CI promotion `fdc90c8`).
Isolated review branch: `task/STORAGE-001-code-review`.

Verdict: **approved for integration**, subject to actual automatic Ubuntu acceptance,
latest-head checks and normal PR gates. No source changes or baseline corrections
were requested. This is bounded writer/reader approval, not release verification.

## Exact baseline and source

| Artifact | Git blob |
| --- | --- |
| Implementation | `a8e5c8e80ebb9bde8321d01f7fbc1b1d826c2811` |
| Public interface, unchanged | `11d204aab3340bc670df08716ae1c80cae0c337f` |
| Contract revision1, unchanged | `4d280461966f3fd9fcb13ab6d054c88228ca05b8` |
| 22 Qt cases, unchanged | `f8418e83dbd88d74505c047e6b3e2c30304ade69` |
| Promoted automatic Qt workflow | `504240ef34542bb7c4529221af40e411776b1681` |

Read root/server/tests instructions, workflow/test strategy, architecture, the
STORAGE-001 brief, specification v0.45 §2/DB-01, event/state revision1, the full
writer contract/header/test source, original baseline reviews and actual red
handoff. PR24 supplied the integrated reviewed/red prerequisite; no initial setup
or test authoring was restarted. Original red at6647490 and its exact artifacts
remain preserved. This reviewer did not author implementation or baseline tests.

## Examined behavior beyond the assertions

The writer reuses the construction thread's Qt queue and creates no separate
thread. Public operations reject a foreign thread before accessing Qt SQL; on-owner
checks also reject a moved QObject. Construction/destruction on the owner remains
an explicit caller responsibility, with destructor assertions. Connections stay
private, and every QSqlQuery is operation-local. Query objects have left scope
before connection close/reset/removal, including error and partial-open paths.

Canonical paths resolve existing symlinks or an existing canonical parent for new
files. A mutex protects the process-local lease set across different owner threads.
Partial open, failed configuration, fatal close, graceful shutdown and destruction
release the lease. This supplements SQLite locks; it does not claim hostile external
filesystem changes or arbitrary other processes are prevented from opening SQLite.

Admission validates the entire trusted batch before queueing and stores owned value
copies. The command allow list and semicolon policy preserve internal transaction
ownership. Positive limits and subtraction of per-value charges avoid summing past
signed bounds. Every value has a base charge, including null/empty values, and UTF-8
or blob payload consumes the remaining budget. Result rows/bytes share one budget
across the whole transaction; fetch errors and exceeded bounds expose no partial
results. Exact numeric meaning remains with callers; no money/score arithmetic or
new REAL representation policy was introduced.

The active flag remains true while the front batch has left the queue and through
its complete BEGIN/statements/COMMIT or rollback path. Admission counts active plus
queued work. The slot is freed only after transaction completion, before the user
callback. One queued dispatch handles one complete batch. A next dispatch is queued
before user completion, with no Impl/member access afterward; callback reentrancy
or destruction cannot leave the dispatcher touching a deleted Impl. Shutdown moves
its completion list to local storage before callbacks for the same reason. Callback
exceptions are contained after the transaction outcome is fixed.

BEGIN IMMEDIATE, statement and COMMIT errors propagate without replay. Primary
busy/locked codes are obtained from the native numeric error, including extended
codes' low byte. A failed statement/COMMIT rolls back; failure to establish rollback
closes admission and connection conservatively. Already admitted work still drains
through dispatch and receives sql_failure rather than being silently dropped.
Further enqueue/open is terminally rejected. Known successful rollback allows the
remaining queue to continue. This is not recovery of an uncertain durable commit.

WAL/FULL, foreign_keys and zero busy timeout are actually configured and checked on
the writer connection. Reader connections use the real QSQLITE_OPEN_READONLY flag
plus query_only, not a toggleable PRAGMA alone. They retain their own construction
thread and do not grant a raw writer handle. Graceful shutdown rejects new work
immediately, drains accepted work, closes/releases the connection/lease and posts
completion asynchronously; repeated completion requests remain exactly once.

Framework assumptions were checked against primary documentation:
[Qt SQL connection ownership/removal](https://doc.qt.io/qt-6/qsqldatabase.html),
[Qt queued invocation](https://doc.qt.io/qt-6/qmetaobject.html),
[SQLite result codes](https://www.sqlite.org/rescode.html), and
[read-only SQLite open](https://www.sqlite.org/c3ref/open.html).
These sources support the implementation mechanisms, not full product correctness.

## Actual independent commands

Run from the isolated reviewer worktree on Linux x86-64, GCC16.2.1, Qt Core/Sql/
Test6.12.0, CMake4.4.5 and Ninja1.13.2:

| Command | Actual outcome |
| --- | --- |
| `cmake --preset sqlite-writer-debug` | Exit0 |
| `cmake --build --preset sqlite-writer-debug` | Exit0; compiled actual implementation and unchanged Qt cases |
| `ctest --preset sqlite-writer-debug` | Exit0;25/25: all22 real disposable-file Qt/SQLite cases and3 infrastructure checks |
| `git diff --exit-code c2ca25a -- server/include/subathon/server/sqlite_writer.hpp docs/contracts/sqlite-writer-v1.md tests/qt/sqlite_writer_tests.cpp` | Exit0; approved public boundary/oracles unchanged |
| `actionlint .github/workflows/sqlite-writer.yml` (official1.7.12, disposable tool path) | Exit0; no findings |
| `git diff --check` | Exit0 |

No test assertion/fixture was weakened and no baseline was skipped. The full preset
already executes the22 writer cases, so a redundant second native run was not used
as additional evidence. No source/test files were edited by this reviewer.

## CI and remaining gates

The reviewed workflow now runs actual writer acceptance unconditionally for PRs,
main pushes and manual dispatch. It retains the already observed pinned Qtbase/
ICU archives, action pin, version/plugin check, complete registration guard and Qt
probe. Manual-only acceptance gating was removed; there is no failure inversion or
continue-on-error. The existing job name is preserved. Only descriptive CMake/preset
text and the historical draft label changed; case enumeration/commands/timeouts are
unchanged. Actual promoted Ubuntu execution must still be observed before merge.

Private-handle ownership, active capacity and callback lifetime were specifically
inspected beyond direct tests; they are not described as separately runtime-tested
fault injections. Rollback-failure/uncertain durable commit, abrupt crash/power loss,
pathological trusted SQL runtime, process shutdown deadlines, actual application
schema/migration/repositories/effects/jobs/aggregates, backup/recovery and transport
integration remain later gates. No Windows, deployment, provider, credential, OBS or
supported-product release claim follows from this Linux local writer evidence.

## Final completion and integration review

Reviewer `/root/storage_code_reviewer`, 2026-11-01. Reviewed root completion
candidate `9bb63be` after preserving local review history with a normal merge.
Verdict: **approved for integration**. No source/baseline correction is required.

Compared the complete implementation, public header, contract, all22 cases,
CMake targets/preset/registration and workflow against approved850eef4 with
`git diff --exit-code`: exit0, no changes. Sourcea8e5c8e and workflow504240ef
remain the actual reviewed artifacts. Examined the author implementation evidence,
completion state/HTML, DB-01 map, architecture/test strategy, third-party inventory,
CI/handoff and coordinator checkpoint. Their bounded claims agree with the code
and retained red/pass evidence. STORAGE-001 may be done while publication/merge
checks remain separate; no repository/application schema, crash/backup/recovery,
provider, deployment or release/platform gate is upgraded. DB-01 remains partial.

Independently read actual GitHub run41289246502 and full relevant logs, rather
than accepting only the coordinator summary. It succeeded at head
`ed5da87514c423e8771e0b26f2b3cc3814755f10`: both archive SHA256 checksOK,
GNU13.3.0, environment probe1/1, complete22 registration, then unconditional
`Run reviewed writer acceptance` with22/22 real file cases passing. The acceptance
step was executed successfully, not conditional/skipped. A separate exact-commit
check-runs query confirmed all10checks ated5da875 completed successfully.
[Actual run](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41289246502).

Commands actually run after refresh:

| Command | Result |
| --- | --- |
| `python3 tools/check_project.py` | Exit0;377tasks/122decisions/links/presets/currentHTML |
| `python3 tools/progress.py render --check` | Exit0 |
| Exact implementation/baseline/build/workflow comparison850eef4..9bb63be | Exit0, unchanged |
| `git diff --check` | Exit0 |
| GitHub run/result/log and exact-commit check-runs inspection | Actual acceptance and all10checks confirmed above |

No native suite was redundantly repeated after metadata-only changes; independent
25/25 execution remains recorded above. The actual CI result is attributed to its
observeded5da875 revision, with latest-head checks explicitly remaining a merge
gate after final review commits. Resolve discussions and recheck current normal
rules/latest revision before merge; this review does not claim PR25 has merged.

## Additional ownership finding — approval held

Reviewer `/root/storage_code_reviewer`, 2026-11-01, before PR25 merge.
The earlier ownership inspection did not cover Qt values made with fromRawData.
**The approval of sourcea8e5c8e is held for the queue-ownership gap below.** Existing
22 reviewed tests and their recorded passes remain valid; this finding does not
weaken or replace them and is not a reviewed new regression baseline.

Coordinator raised whether `queue_.push_back({batch, ...})` truly owns all accepted
SQL/QString/QByteArray content. A temporary native probe against the actual
reviewed static library on Qt6.12.0 confirms that raw backing survives implicit
value copies. It enqueued only `SELECT 1, ?, ?` with raw-backed textabc and blob
hex616263. After every caller-side Qt value left scope, backing arrays remained
alive and were deliberately changed before Qt dispatch. Admission and SQL execution
succeeded, but results were literal9, textzbc and blobhex7a6263. Copied SQL and
QVariant QString/QByteArray values each retained the original raw pointer. Callback
was asynchronous and both callback/event-loop completed successfully. No dangling
pointer was used and no application state was touched; the probe used a disposable
file and read-only SQL.

The public header promises owned statements; CONTRACT-002 excludes borrowed mutable
queue inputs. The writer contract says enqueue copies the batch, without an explicit
precondition requiring the caller to normalize every raw-backed Qt value before
admission. Qt's own raw-data contract requires backing lifetime/nonmutation through
all unmodified copies; ordinary copying does not establish the advertised ownership
snapshot. Explicitly narrowing callers to pre-owned backing would need a reviewed
contract clarification. The preferred correction is to make accepted SQL text and
QString/QByteArray bindings own their data at admission while preserving their
values, null/type semantics and existing budgets.

Primary documentation checked:
[QString raw data](https://doc.qt.io/qt-6/qstring.html#fromRawData) and
[QByteArray raw data](https://doc.qt.io/qt-6/qbytearray.html#fromRawData).
The probe deliberately checks API normalization; it is not a claim that mutating
raw backing while Qt still has borrowed copies satisfies Qt's own precondition.

Diagnostic files: `/tmp/subathon-raw-ownership-review-probe.cpp` and corresponding
binary/output text. Compiled with C++20/-fPIC/-g/-O0 against this review worktree's
actual `libsubathon_sqlite_writer.a` and installed Qt6Core/Sql pkg-config flags;
compile and probe exited0. An initial standalone link without-fPIC failed and ran
no probe; it is not counted as runtime or red evidence. The debugger skill's
required router was unavailable, so its activation gate required this native-probe
fallback. No production/header/test file was edited.

Verdict for this newly discovered path: **request changes before integration**.
Separately author/review a meaningful ownership regression, record intended failure
against the unchanged implementation, then review the targeted fix and new actual
CI results. No future fix or regression is approved by this diagnostic note.

## Independent ownership regression baseline review

Reviewer `/root/storage_code_reviewer`, 2026-11-01; regression author is the
separate `/root/storage_test_author`. Reviewed integrated candidate `b874f31`,
including corrected author8fa7808. Verdict: **approved for the three-case regression
baseline only**. Sourcea8e5c8e remains unmodified and its ownership approval remains
held until actual regression red, targeted correction and separate fix review.

Exact new test blob: `34c25c2786db42f6a4483fc874da9d4d3fc525a3`.
Exact grouped-discovery workflow: `d99df8ad627e68b492009ccba5091bab4e093001`.
The original22/header/contract/source compare unchanged against approved850eef4.

Examined every new body and independently checked literal oracles:

- Raw-backed SQL `SELECT 1` must still execute as1 after the caller's backing array
  becomes `SELECT 9` following admission.
- Raw-backed QString bindingabc must persist asabc, independently read through
  the fixture connection, after caller storage becomeszbc.
- Raw-backed QByteArray binding must persist one row of SQLite typeblob and literal
  hex616263 after caller storage becomes7a6263.

These expected values come from the existing owned admission contract, not the
implementation under test. The synthetic fixture only creates/observes SQLite
state; it supplies no snapshot or queue policy. Arrays remain alive throughout
completion, mutation is on the same thread before event dispatch, and no dangling
memory or guessed sleep is involved. The initial candidate's mutation while a
caller Qt view was still live was corrected before approval: every caller view and
batch now leaves scope first, then backing changes, then QTRY dispatches. Before
admission, constData identity proves the input really uses borrowed raw storage.
This avoids a separate misuse of a still-live caller view while preserving all
original output assertions. No bodies ran during that review correction.

The separate executable is correctly linked with the existing writer and Qt Test.
Each of the three slots is independently registered with sqlite-writer/regression
labels and a finite timeout. Original22 enumeration, commands and oracles remain
unchanged. The updated CI guard groups CTest commands by executable, requires both
acceptance/regression executables, then requires exact nonempty function-set and
count equality for each. It rejects omission or duplicate registration rather
than silently checking only the first binary. Ordinary acceptance still executes
both through the shared sqlite-writer label, with no conditional/inversion.

Actual commands from the isolated reviewer worktree:

| Command | Outcome |
| --- | --- |
| `cmake --preset sqlite-writer-debug` | Exit0 |
| `cmake --build --preset sqlite-writer-debug` | Exit0 |
| `ctest --preset sqlite-writer-debug -R '^sqlite-writer-regression\.' --show-only` | Exit0;exactly3registered |
| CTest JSON discovery plus exact per-binary `-functions` comparison | Exit0;22original+3regression,2executables |
| `actionlint .github/workflows/sqlite-writer.yml` (official1.7.12) | Exit0 |
| Exact original22/header/contract/source comparison against850eef4 | Exit0, unchanged |
| `git diff --check` | Exit0 |

No new test body or intended-red suite was run by this reviewer. This committed
approval permits the coordinator to run the actual three-case intended red against
unfixeda8e5c8e before any production fix. It approves neither future source changes
nor passing behavior; original22 evidence and all later application/recovery/live/
platform gates remain intact.

## Targeted ownership fix — fresh code approval

Reviewer `/root/storage_code_reviewer`, 2026-11-01. Reviewed integrated candidate
`9d96363`, author fixbe533bd, exact source
`829d9c66e47168384510e8ed2ff436e272686414` and grouped-discovery workflow
`d99df8ad627e68b492009ccba5091bab4e093001`.
Verdict: **approved for integration of the corrected source**, subject to actual
updated Ubuntu acceptance and latest-head normal PR gates. This fresh approval
resolves the ownership hold9623943 for source829d9c66; it does not retroactively
approve the deficient raw-backed behavior in sourcea8e5c8e.

Verified the required sequence in actual committed evidence: regression approval
94820fa was integratedf79d002, then coordinator observed all3 regression bodies
failing against unfixeda8e5c8e and committed actual output1499d57, before fix9d96363.
The retained log shows SQL9 instead of1, textzbc instead ofabc and blob7A6263
instead of616263, each after successful fixture/init and with zero skips; CTest
exited8. This is behavioral red, not a compilation/dependency failure. The original
22 cases/header/contract and corrected new3testblob34c25c27 remain unchanged.

Examined the complete small source delta and surrounding admission/dispatch code.
Validation and queue-capacity decisions are unchanged and precede the owned copy.
A copied batch detaches its Qt containers as its statements/bindings are rewritten.
Each SQL string and non-null QString/QByteArray binding gets explicit data/size
construction, which copies borrowed backing instead of only sharing it. Native
UTF-16 units and arbitrary blob bytes are retained without encoding/normalization;
QVariant metatypes and untouched scalar/null variants remain unchanged. The helper
preserves null buffers separately from non-null empty ones. Snapshotting completes
before the queued batch is published and successful admission returns; subsequent
caller backing mutation cannot alter admitted SQL or payload values. No caller
precondition was narrowed and no public interface/product semantics changed.

The resource policy still measures the same values and budgets; the fix does not
bypass validation or touch transaction/rollback/lease/thread/callback logic. The
previous detailed review of those unchanged paths remains applicable. The three
new regression cases directly cover the defect's distinct SQL/text/blob routes,
while the original22 continue to verify the surrounding behavior unchanged.

Actual independent reviewer commands after refreshing the isolated worktree:

| Command | Result |
| --- | --- |
| `cmake --preset sqlite-writer-debug` | Exit0 |
| `cmake --build --preset sqlite-writer-debug` | Exit0 |
| `ctest --preset sqlite-writer-debug` | Exit0;28/28 including22original+3ownership+3infrastructure |
| CTest JSON versus per-binary `-functions` guard | Exit0;2executables,25realcases |
| `actionlint .github/workflows/sqlite-writer.yml` (official1.7.12) | Exit0 |
| Original/new tests, header, contract and configuration comparisonb874f31..9d96363 | Exit0, unchanged |
| `git diff --check` | Exit0 |

No source/test edits were made by this reviewer and no redundant second full native
run was used as extra evidence. Updated automatic Ubuntu execution must include all
25 real cases before merge; the earlier22-only run cannot satisfy this corrected
revision's CI gate. Application schema/migrations/aggregates/jobs, abrupt crash/
backup/recovery, hard SQL/shutdown deadlines, provider/platform and full release
gates remain exactly as previously recorded.

## Final corrected completion and actual CI review

Reviewer `/root/storage_code_reviewer`, 2026-11-01. Reviewed final completion
candidate `8befd04` after a history-preserving refresh of the isolated reviewer
branch. Verdict: **approved for integration**, retaining the fresh corrected-source
approvalc029a53 and all latest-head normal merge gates.

The implementation, original22 cases, new3 ownership cases, public header,
contract, CMake registration and workflow compare unchanged from corrected
candidate9d96363. Exact source829d9c66, regression34c25c27 and workflowd99df8ad
remain the reviewed artifacts. Examined corrected implementation evidence, handoff,
CI notes, state/HTML, acceptance map, test strategy and coordinator checkpoint.
The done/approved record cites the reviewed3-regression/red/fix sequence and fresh
source approval; it does not reuse old22-only CI as proof of the corrected revision.
DB-01 remains partial and app schema/repositories/transactions, migration, crash/
backup/recovery, provider/platform/deployment and full release gates remain explicit.

Independently inspected actual GitHub run41290532883 and logs. It succeeded at
`883d0ab684168bf0beff3ea9948a8c3ee7b11ade`: official Qt/ICU SHA256 checksOK,
GNU13.3.0, Qt environment probe1/1, exact25 compiled registrations, followed by
unconditional actual25/25 passing writer cases. All three ownership regressions
executed and passed. The run's acceptance step is success, not skipped or inverted.
An exact-commit check-runs query independently confirmed all10checks at883d0ab
completed successfully.
[Actual corrected run](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41290532883).

`python3 tools/check_project.py`, `python3 tools/progress.py render --check`, exact
source/test/config comparison and `git diff --check` each exited0. No native suite
was redundantly rerun for these metadata-only changes; independent28/28 execution
against the unchanged corrected source remains recorded above. No source/test file
was edited. The cited CI revision is distinct from final review commits; inspect
all latest-head checks, discussions and current normal rules before merge. This
review does not claim PR25 is already merged.
