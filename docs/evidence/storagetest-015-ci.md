# STORAGETEST-015 CI observations

Coordinator `/root`, 2026-11-02. These observations distinguish the minimal API
build from the later required authored-baseline/green/first-red gates.

## Actual minimal API/build revision

Published exact head `f24ac08ac182b5c749b407d7323334b452912bee` ran the changed
SQL workflow blob `81ba8204cfc8d10abec8ec3f8b7a87c710d132ac`:
[run 41488687802](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41488687802),
2026-11-02T17:10:56Z–17:12:11Z, actual conclusion **SUCCESS**. All eleven latest
PR checks at this head completed SUCCESS, including both scaffold contexts.

Actual `gh run view 41488687802 --json headSha,conclusion,status,startedAt,updatedAt,jobs`
and `gh run view 41488687802 --log` were inspected and cached in ignored
`local-evidence/coordinator/storagetest015-interface-ci-f24ac08.{json,log}`.
The actual log establishes:

- Both selected Qt6.12.0 archive SHA256 checks returned OK. Project compiler was
  GNU13.3.0 on Ubuntu24.04. The existing diagnostic reported Qt6.12.0 and bundled
  SQLite3.53.4; Qt's GCC11 build banner is not the project compiler version.
- Installed package was `libboost1.83-dev=1.83.0-2.1ubuntu3`; selected compiler
  header macro was `BOOST_VERSION 108300` and CMake selected Boost1.83.0 using the
  pinned config directories. No GoogleTest fetch was enabled by the SQL profile.
- Configure used `sqlite-plan-debug`; all74 build steps completed without warning
  lines, including the actual new `sqlite_application_plans.cpp` object and static
  library. This proves that the concrete stub/API and shared dependency wiring
  compile on the selected official Qt job, not that their behavior works.
- All135 previous SQL acceptance cases passed automatically: writer/ownership25,
  work24, migrations24, retention16, receipts22 and contributions24. Exact compiled
  registration checks and existing required migration7/retention3/receipt3/
  contribution3 environment slots passed; their verbose totals include init and
  cleanup with zero failed/skipped. The Qt event-loop probe also passed.

At this head there is **no new plan test, literal fixture or plan environment
suite**. No new plan method or old missing-behavior red was invoked. Thus this run
does not satisfy the future selected official NEW plan-fixture green gate, full
test-baseline approval or first plan red. Actual different API/config approval
still precedes new test/fixture authoring. STORAGE-015 implementation remains
planned; complete application, semantic authority/evaluation, providers/FX,
operational recovery and platform/UI/live release gates remain open.

## Actual approved baseline official NEW-green gate

Full actual baseline/harness approval `8b537cdb` was integrated as `bcd08e6`
BEFORE publishing `aa6907bcea8338c4f6a418de8b5c16b3253d8f1e`. Actual
[run41494786280](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41494786280)
at that same head ran2026-11-02T17:49:36Z–17:51:13Z and concluded **SUCCESS**.
All11 same-head PR checks completed SUCCESS. Root captured and inspected actual
run JSON/full log and check list under ignored
`local-evidence/coordinator/storagetest015-approved-{ci.json,ci.log,all-checks.json}`.

Both selected archives checked OK; project compiler GNU13.3.0, configured pinned
Boost1.83.0/package1.83.0-2.1ubuntu3/header108300, Qt6.12.0 and bundled SQLite3.53.4.
Actual fresh build **82 steps**, including both new Qt source objects and
executables, no build warnings. All135 previous automatic SQL cases/required old
green7/3/3/3 still pass. Actual NEW helper discovery is4; verbose new helper reports
**6 passed,0 failed,0 skipped,39ms**, including init/cleanup, actual selected runtime
and explicitly INVALID sparse count3/headMAX. Actual feature discovery reports
all29 compiled cases; behavior was not run. New Qt fixture/runtime proof therefore
belongs to the actual final reviewed source/configuration, unlike the earlier f24
minimal-interface run. The feature step remains an explicit manual baseline gate.

Only after observing this gate did root run FIRST actual feature red at SAME
clean `aa6907b`,2026-11-02T17:52:55.646037Z. Actual command
`ctest --preset sqlite-plan-debug -L '^sqlite-plan$'` exited **8**: all29 cases fail
on explicit new missing-behavior markers, all29 init+cleanup pass, zero skipped/
crash/setup failures, CTest0.57s. Six cases print the marker's direct what-message;
23 print the typed `SqliteApplicationPlansNotImplemented` wrapper marker. Root's
first scratch count recognized only the six direct message spellings; inspection
confirmed all29 are the same actual intended marker, without rerunning anything.
[Original immutable red](storagetest-015-red-ctest.txt), blob
`ae3e0e25956b16b0d3bc26065c71247263bca421`, SHA256
`79ddd03f9f37e9657740691b5f8bbb1b7dceff93b2de62499b345f14a8b16f41`.

Root's capture script checked clean exact head, integrated full-review ancestry,
nine frozen artifact hashes, actual same-head official success/required step
results and all11 checks before invoking CTest once. It preserved the actual
CTest exit8 and original raw output. No failure inversion or passing product
claim follows. Header/source remain explicit stubs; STORAGE-015 needs separate
implementation/acceptance/source review and automatic CI promotion. Final metadata
and CI/red attribution review, future latest-head CI/discussions/rules/merge still
remain PR gates. This baseline does not establish any platform/live release gate.
