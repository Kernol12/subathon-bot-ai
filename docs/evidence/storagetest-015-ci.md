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
