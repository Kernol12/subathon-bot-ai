# Handoff — STORAGETEST-015

Coordinator `/root`,2026-11-02, branch
`task/STORAGETEST-015-captured-plans`,
[draft PR39](https://github.com/Kernol12/subathon-bot-ai/pull/39).
**Reviewed tests and first intended red are complete; implementation is absent.**
Publication/merge remain separate gates below.

## What changed and why

Versioned [logical/API contract](../contracts/sqlite-application-plans-v1.md) and
[physical proposal](../contracts/sqlite-application-plans-schema-v1.md) define
caller-owned staged immutable revisions, honest selected-versus-complete history,
full uint64 and canonical arbitrary-precision rational persistence, all valuation/
selection/provenance fields, typed actual targets and explicit rollback truth.
Specification v0.45 §§3/6/7, NUM-01, DB-01; [brief](../tasks/EPIC-009/STORAGE-015.md).

Actual minimal header/stubs,29 real-SQL feature cases, independently derived JSON,
four old-writer-only diagnostics, dedicated profile/library and CI are integrated.
Exact migration4 proposal is five DDL/full30 objects/unchanged20+13+7 prefixes,
payload29015/SHA bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50.
Expected data never came from a future producer. Exact is canonical rational VALUE;
source decimal/rate spellings stay separate unchanged strings. Sparse selectedMAX
is deliberately INVALID aggregate history, never fake valid exhaustion/import.

## Actual ordered reviews and observations

| Gate | Actual evidence |
| --- | --- |
| Full logical/physical contract | different a2926ee integratedc61b990 |
| Minimal API/stub/build | different4282cc4 integrated2836aac |
| Independent fixture/old-writer helper | author6c276fc/root6b4a2a2; different53c0f621/rootdab24ab |
| Full authored feature/harness | author2771c9bb/rootc1cde5b; different8b537cdb/rootbcd08e6 |
| Selected official NEW-green/build/discovery | actual41494786280 SUCCESS at approved clean aa6907b, AFTER full approval |
| FIRST intended29-case feature red | SAME aa6907b,2026-11-02T17:52:55.646037Z, exit8/all29 marker failures/all29 init+cleanupPASS/0skip/crash/setup failure/0.57s |

[Actual reviews](storagetest-015-review.md), [author matrix](storagetest-015-author.md),
[independent literal derivation](storagetest-015-fixtures.md),
[build/probe evidence](storagetest-015-probe.md), [CI](storagetest-015-ci.md),
[original red](storagetest-015-red-ctest.txt). Preserve that original red; do not repeat.

Frozen baseline artifacts:

| Artifact | Blob |
| --- | --- |
| Feature cases | `0f8e625ff7c238776602fd1be102959f8c43b073` |
| Literal JSON | `64eb8b4e0d709bc34ab6d33a4860a62883f06c6b` |
| Writer-only green | `a8d63d6a220992fcb71273d89a3aa32609870051` |
| Header | `4634d7b57dccc5c559f2ab05a968833b4b925feb` |
| Nine-throw source | `e989d70f5021a9285db0dbc79c32d1153b4e7700` |
| Logical/API | `3af256cdf82a4737c2fc71a67c88b8b332d6ba71` |
| Physical | `99bf45b46f6c9d361b899fdb975dc2db51da4554` |
| Qt registration | `1ef6dfccf4e6bd4b369aa2886716eeb899a76811` |
| SQL workflow | `4d8c5f8d3a79d40233d7e7851b791535f2729d5b` |
| Original red | `ae3e0e25956b16b0d3bc26065c71247263bca421` |

## Commands and actual results

Configure/build `sqlite-plan-debug` passed on host GNU16.2.1/CMake4.4.5/Qt6.12/
Boost1.92; actual incremental20 steps0warnings. Exact173 registration, compiled
feature29/green4 lists match. Root/reviewer inspected `-LE '^sqlite-plan$'` JSON
before running only144 old+green tests:PASS3.51s/3.57s. Tooling unittest19,
actionlint1.7.12, project391/122, rendered HTML and diff checks pass.

Actual official job uses GNU13.3/pinnedBoost1.83/Qt6.12/SQLite3.53.4, both archive
hashes OK,82-step build/no warnings, old135/required old diagnostics PASS, newgreen
four slots/6PASS0skip39ms, exact29 discovery/no feature call; all11 source-head
checksSUCCESS. Only then the exact first-red command above ran once. No historical
red or already completed source task was repeated.

Ordinary CI compiles/discovers the new feature baseline and automatically runs
all stable SQL suites and required new diagnostics. Intentional red remains explicit
manual dispatch; no skipped assertion, WILL_FAIL or continue-on-error hides it.
After STORAGE-015 passes, promote its29 cases to unconditional automatic execution
with actual source/config review and observed new CI before merging implementation.

## Remaining gates and next task

Final metadata/CI/red attribution review and latest-head CI, discussions, required
GitHub approvals, current rules and mergeability remain. PR39 stays draft until
those applicable gates pass. No owner access or decision is needed. Merge only
normally at the exact eligible head, refresh main, reconcile actual outcome.

Then select separate dependency-ready STORAGE-015 from integrated prerequisites;
claim a new branch before edits. Preserve all frozen assertions/fixtures/API and
original reds. Implement only the reviewed persistence behavior, pass29 and relevant
regressions, obtain different actual source/config review and automatic CI promotion.
No production implementation has been authorized by merely passing old diagnostics.

All prior source/fixture/assertion/review evidence remains unchanged. Semantic
reference/permission/evaluator, actual effects/jobs/full app transactions, all seven
integrations/three FX providers, backup/update/recovery/power-loss, renderers/UI/
locales/themes/packaging/platform/live/OBS remain required and incomplete. No live
credential/supporter/chat/payment/deployment action occurred. State/HTML record
baseline completion only. [Coordinator checkpoint](coordinator-handoff.md).


## Actual merge outcome

PR39 normally merged at a7a1844d3f20495e4edcf179a4a48caf22a1123b,
2026-11-02T18:07:39Z, exact finald8/all11SUCCESS. Actual full8b and final3dec
reviews, readyMERGEABLECLEAN, zero discussions, current requiredGHapprovals0/rules
were checked. No force/admin/bypass. Root main refreshed clean; its tree equals
final reviewed d8. Postmerge nine workflows are observed SUCCESS; SQL41497596911
is still building, explicitly not yet passed. Separate STORAGE-015 is now claimed
from that integrated main; no baseline/red is repeated. Later checkpoint records
actual remaining postmain result and production gates.


Postmerge reconciliation: all10 actual main a7 workflows subsequently completed
SUCCESS, including SQL41497596911. The earlier pending statement is historical;
no unchanged feature red was repeated. Separate source15 author/reviewer roles
are explicitly resumed in fresh isolated trees, with all frozen artifacts retained.
