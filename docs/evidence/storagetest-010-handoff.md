# STORAGETEST-010 — writer transaction-work baseline

Coordinator `/root`, test author `/root/storage_decomposition`, independent reviewer
`/root/writer_transaction_reviewer`, 2026-11-01. Task branch
`task/STORAGETEST-010-writer-work` from merged main e374788 (PR28).
Specification v0.45 §2/§3, DB-01/SERVER-01;
[writer transaction work revision1](../contracts/writer-transactions-v1.md).

This task refines an additive C++ capability boundary, authors real Qt SQL cases,
obtains a different agent's actual test/interface/harness review and then observes
intended missing-behavior red. It does not implement STORAGE-010. Existing writer
source and all25 acceptance/regression assertions remain unchanged. Generic
counters/deferred constraints are disposable fixtures, not application schemas.

Author works in `local-evidence/worktrees/storagetest010-author`, branch
`task/STORAGETEST-010-author`; owns header additions, explicit new work stubs,
new Qt tests, the narrowly scoped testing fault seam and author evidence. Root owns
shared CMake/presets/workflow/map/state/HTML/checkpoint. Reviewer uses
`local-evidence/worktrees/storagetest010-review`, branch
`task/STORAGETEST-010-review`, and owns only independent review evidence.

Root shared harness d680dd4 adds `sqlite-work-debug` and a separate `sqlite-work`
label. It preserves the stable writer job and old25 automatic acceptance, builds/
lists proposed new24 cases, and only runs known-red bodies by explicit manual
input. [CI assessment](storagetest-010-ci.md). Existing ReceiptIntake is linked for
actual synchronous service-port composition using a synthetic primitive repository.
No final receipt/contribution adapter or domain schema is implied.

Author6532c07/API then76d3a00/tests integrate as be925db/9a9f29c. Exact artifacts:

| Artifact | Git blob |
| --- | --- |
| Contract revision1 | `4ba20506db9a858450099b82888a22c8f6dbd761` |
| Public header | `afc9f483da7d8e61a5e73715ca33819216e9e668` |
| Explicit work stubs | `b47f131af895311cef790c64ac0187532994a819` |
| Control-ack test seam | `a2cd7b8900848d8e7b7091b676aeaf16754efe0e` |
| 24 Qt slots | `eb9e3b600788f9473c5be7abdda86440cd73a3f4` |

[Author evidence](storagetest-010-author.md) maps WT01–WT20 to concrete literal
oracles, fixed error categories and honest limits. Independent BEGIN IMMEDIATE/
ROLLBACK probes inside caught failures establish synchronous write-lock release;
row invisibility alone was insufficient. Queued old batches preserve sql_failure/
no rows; uncertainty remains distinct from known repository rollback.

Author and root `cmake --preset sqlite-work-debug`, build and exact nonempty24
compiled-vs-CTest discovery pass (53 total). Root CTest `-L sqlite-writer` passes25,
`-L scaffold` passes4 including actual Qt probe; old source/tests/contract diff is
empty. Linux x86-64, GNU16.2.1, Qt6.12.0, CMake4.4.5, Ninja1.13.2, Python3.14.8.
Project391/tooling19/DOM/actionlint/diff checks pass. These do not establish new
work behavior. Independent actual test/interface/harness approval96c1481 of d09d330 was integrated
at75c07fc BEFORE the first new behavior run. [Review](storagetest-010-review.md).
Author fixes0c6fc8e integratedd09d330 add uncertainty-over-first-SQL-error and
healthy-work-after-validation-exception variants without changing24 slots. Final
reviewed artifacts are the table above; oldsource/tests/contract remain unchanged.

At75c07fc root ran `ctest --preset sqlite-work-debug -L sqlite-work` after successful
build: actual exit8, all24 cases fail on explicit feature_not_implemented value1,
all fixture/init setup succeeds, zero skips/compile/dependency failures. No failure
inversion or assertion weakening. [Full output](storagetest-010-red-ctest.txt).

Actual corrected Ubuntu [run41299021575](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41299021575)
at d09d330 passed selected Qt/ICU hashes, GNU13.3, probe1, old25 real acceptance
and exact24 new discovery. All11 PR checks there succeeded; no new body execution
is inferred from those green checks. Earlier41298835177 at a6499f4 is historical.

STORAGETEST-010 is done as an independently reviewed, observed-red baseline only.
[PR29](https://github.com/Kernol12/subathon-bot-ai/pull/29) remains draft pending
final completion-metadata/red/CI attribution review, latest-head CI/discussions and
normal merge gates. No owner decision/access or running root command remains.

Later STORAGE-010 must preserve the independently reviewed baseline, implement only
this additive mechanism, obtain separate code review, pass real acceptance and
promote unchanged cases to automatic CI. Migration/config/receipt/contribution/plan/
application bundle/recovery/platform/live gates remain separate and required. All
seven integrations and three rate providers remain full intended release scope.
