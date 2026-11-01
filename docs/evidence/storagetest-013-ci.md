# STORAGETEST-013 actual CI evidence

Coordinator /root, 2026-11-01. This record distinguishes compiled interface,
selected fixture support and future intentional-red behavior.

## Actual API stage

[Automatic PR run41329002993](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41329002993)
at actual head `a152a82ed66413c73e99a5fbe443eabdaf569528` completed SUCCESS,
event pull_request. Root read actual GitHub metadata and full executed logs using
`gh run view 41329002993 --json databaseId,headSha,status,conclusion,event,jobs`
and `gh run view 41329002993 --log`. Ignored copies are under
`local-evidence/coordinator/storagetest013-api-ci-a152a82.{json,log}`.

Observed on Ubuntu24.04: both pinned official Qt/ICU archive SHA256 checks OK,
CMake GNU13.3.0, actual selected Qt6.12.0/SQLite3.53.4. The changed workflow uses
sqlite-receipt-debug and actually compiled new sqlite_receipts.cpp at13/48 and
linked libsubathon_sqlite_receipts.a at35/48. It ran all old25writer,24work,
24migration and16retention cases successfully. Existing migration diagnostic7
methods report9passes including init/cleanup/0skip; existing retention diagnostic3
report5passes including init/cleanup/0skip. Discovery matches each compiled suite.
All11 PR checks at this exact head were separately observed SUCCESS.

This is interface/stub compilation plus stable foundation evidence. Four named
SqliteReceiptsNotImplemented throws remain; no new receipt fixture/case/body is
authored or run at this stage. It is not receipt acceptance, new fixture proof,
intended red, production implementation or full-release verification.

## Remaining CI and baseline gates

Different actual API/stub/precision/build/workflow review precedes new test and
literal-fixture authoring. Those then need separate actual full review, successful
compilation, complete compiled/CTest registration, required green receipt-fixture
diagnostics in this selected official environment, and only then the first genuine
intended missing-behavior red. Intentional-red feature bodies stay separately
invoked and visible; never invert/hide failure in ordinary passing CI. STORAGE-013
implementation remains a later focused task after the baseline merges normally.
No platform/live/provider, recovery, power-loss or deployment proof follows.
