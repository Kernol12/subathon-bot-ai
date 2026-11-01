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

At this draft stage author artifacts, successful compilation/discovery, independent
actual baseline approval, observed red and actual changed Ubuntu CI are pending.
Root scaffold configure/build/3 infrastructure cases, tooling19, DOM simulation
and workflow actionlint pass; these do not establish
new executable behavior. No build/test process was left running by root. Next:
integrate actual authored revision, complete independent review, run genuine stub
cases only after approval, retain exact artifacts/commands/output and verify latest
PR CI/normal merge gates. No owner decision/access is needed.

Later STORAGE-010 must preserve the independently reviewed baseline, implement only
this additive mechanism, obtain separate code review, pass real acceptance and
promote unchanged cases to automatic CI. Migration/config/receipt/contribution/plan/
application bundle/recovery/platform/live gates remain separate and required. All
seven integrations and three rate providers remain full intended release scope.
