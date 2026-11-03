# STORAGE-016 implementation author checkpoint

Author: bundle_implementer,2026-11-03. Fresh isolated `task/STORAGE-016-source`
starts from integrated main017d40f6233e3416b4a785924052a9ab9bbeb83f after normal
PR47 merge13:20:43Z and root claim4622646a6631f5820807dd672d21f785bd9b045f.
Root owns metadata/build/CI/schema order/publication. Runtime source is authored;
final acceptance, independent source/allocation review and ordinary CI are PENDING.
This checkpoint does not claim task completion or a passing complete hold suite.

Authority: approved logical701c710f/physical83e96362/API46c70297/headerfe50fb10,
[baseline handoff](storagetest-016-handoff.md), independently authored265 scenarios
in unchanged test0cce9545 and actual reviewed/red source598. The completed intended
red13:02:29.325–13:02:35.196Z followed OWN selected41640390883 success:265 markers,
70 init/cleanup passes,zero setup/skip. It was NOT replayed as preparation.

## Concrete runtime and preservation

Pure private helper1cf31df imported as476c242, blob a18a0bd7705f8043daac75acd365471cc59e49c5.
It owns option/input/shape/equality/B/I/manifest logic. Actual runtime checkpoint
0fe329cd45ae09b3ff30c593094a076d3e1837a5 has these source blobs:

| Path | Git blob |
| --- | --- |
| sqlite_application_holds.cpp |97c83b746fe1507670b03bf5430c45231facf551 |
| private sqlite_application_bundle_access.hpp |7107d0ee88d4d51be8642b55b1bceadb1758489a |
| existing public bundle.cpp bodies |24759a20a8c9e1de5f577bf36d16f829a171b03f |

The old anonymous Access/strict decoders are moved unchanged into a private header.
A programmatic reverse of ONLY that factoring reconstructs the ENTIRE original
bundle.cpp BYTE-EXACT, including every old query/body/accounting walk. No old public
API, stage equality, field charge or schema/fixture/test assertion changes. Header
fe50, test0cce, fixture63de remain exact; no dependencies or machine paths are added.

HoldAccess uses the actual borrowed session and its own hold-aware guard. Known
local failures establish whole-unit rollback; already-aborted old/writer failures
map without second cleanup; actual WorkFailure uncertainty/capability truth survives.
There is no own transaction/commit/adoption/connection/destructor cleanup/publication.

Six implementations cover exact step6/prefixes, global subject discovery, full
selected finite views/history, actual-kind historical mutation comparison and
atomic detection/subset/resolution/replacement. Unfiltered actual counts/domains/
ordinals precede full vector allocation. All selected-subject anchor/head/history
metadata is streamed64/65 before status/result limits. Returned item projections
are checked before full view/page vector materialization; output bytes/B/I count
actual repeated owning members. Statements/bindings/results/head checks are prepared
before writes and every per-hold head advances LAST. No contribution/application
success namespace is modified by the hold adapter.

Strict old same-session receipt/config/context/support/selected-plan/historical-
snapshot/full-application readers validate referenced data without current-policy
inference. Known context configuration matches source/receipt config; NULL remains
unknown. Optional FailedCapture plan/head stays honest; present prior plan receives
full closure. No dummy bundle, owning legacy repository or public getter is invented.
Timer links/remaining values/decision annotations remain DATA, not authority,
entitlement, producer computation or same-unit COMMIT certificates.

## Actual native results and unresolved gates

Linux x86-64/GNU16.2.1/C++20/Qt6.12.0/Boost1.92/SQLite3.54.0/CMake4.4.5.
`cmake --preset sqlite-hold-debug` succeeds with system discovery. First compile
found mixed signed/unsigned auto deduction in a new reader; corrected explicit
separate deductions. A later local-shadow warning was renamed. Latest build command
`cmake --build --preset sqlite-hold-debug --parallel 2`:3 steps,ZERO warnings/errors.

Actual unchanged `ctest --preset sqlite-hold-debug -L '^sqlite-hold$'` initially
32/35 passed. A real resource_limit on first64 history was fixed by reusing ONE
complete metadata/cause audit for that immutable same-hold page. Every returned
event still owns a copy; SQL budgets are not reset. Latest run:33/35 PASS11.66s,
zero skips; only the two independently identified wrong-cursor baseline failures
remain. Full `ctest --preset sqlite-hold-debug`:262/264 PASS19.57s, only those same
two failures; all229 older registrations pass. No baseline assertion was edited.

Genuine test-input conflict: calls575/1940 request hOld after_position0/page64,
while frozen old_resolution contains only position2 with338 bytes/6 items. Contract
history after0 includes positions1 AND2. Actual GDB at holds.cpp1141 conditional
i==1 observes query hOld/0/64, two fully appended entries at positions1/2 and
has_morefalse; stack reaches test575. Root is handling a separate specification-
based two-cursor correction/review; production will not omit initial history.

The installed clion-debugger SKILL.md was read/applied. Router status had no
sessions; the worktree file had no run points
and IDE launch explicitly failed to create a run configuration. Its one agent
logpoint was removed, leaving four pre-existing user breakpoints unchanged.
Read-only batch GDB fallback inspected actual values/stacks, without source logging
or baseline edits. The first64 completion showed synchronous hold resource_limit,
writer resource_limit/rolled_back/no value before the page-audit optimization.

Two additional actual review findings are held for independent regression/review/
reached-red gates: helpera18 cross-stage body-versus-matrix/order priority, and
narrow supersedes lineage's ancestor head/first-producer/reciprocal scalar closure.
Neither has been privately patched. Root owns new regression/test corrections;
helper author owns its pure fix. Current code remains exact for those red targets.

Raw logs under ignored local-evidence/storage016:

| File | SHA256 |
| --- | --- |
| storage016-build6.txt |fc241a711dc587ee3e20078f5b8b785cccf0470417a632501daf207b781c9286 |
| storage016-tests4.txt |66d4de9167ec30a401e712298c902a293f269dbdcada9bd48414c9447eadc8a7 |
| storage016-full1.txt |d239db9963cefba0b9cbff9f9d4189e8c84d8a0af0301a69bdce57ad3edb7eea |
| storage016-history-gdb3.txt |859d21f3358a502043c2c58dc10845ecd2ce5468781119a4d60f12015c04bca6 |
| storage016-budget-gdb.txt |1cc031c43e672d07fe46c43c499592196f0c2eb89d8ee9c9ffb7926f6f1c6547 |

Project checker PASS396 tasks/122 decisions/links/presets/current HTML and
git diff --check PASS. Final corrected acceptance and different actual source/
allocation/native validation/own promoted CI/latest merge gates remain mandatory.
Service004 and all current-authority/producer/mixed/postfinish/continuation/financial/
recovery/provider/UI/platform/backup/live/release obligations remain separate.
