# STORAGE-012 — immutable retained documents implementation handoff

Coordinator `/root`, 2026-11-01. Branch task/STORAGE-012-retained-context, claim
49719cc from refreshed mainae57f2b after PR33 merged. Source and local acceptance
are independently approved; actual automatic selected-Qt CI passes. Final metadata/
publication review and exact-latest-head gates remain.
[Brief](../tasks/EPIC-009/STORAGE-012.md), [contract](../contracts/retained-context-v1.md),
[baseline](storagetest-012-handoff.md), [implementation](storage-012-implementation.md),
[code/config review](storage-012-code-review.md), [CI assessment](storage-012-ci.md).

The three former stubs now return the approved pure manifest and implement
synchronous find/retain through the caller's existing session. Exact BLOB refs/
documents and typed known links preserve all bytes/presence; present-empty loaded
payload is owned/non-null and can be retained idempotently. Pure validation precedes
SQL; selected types/lengths/known target are validated before immutable comparison.
Conflict/missing makes no own writes or cleanup. Exceptional selected data/field
limits clean up before known local failure; uncertain cleanup retains underlying
SqliteWorkFailure. No connection/native handle/thread, autonomous transaction,
retry, core port or input-budget adjustment was added.

Sourcea114225/c96dae9 integrated41ffb5a; authorb8 integrated4b3d116. Exact frozen
API55/contract5b/testc0/envAAA/JSON0f/old023+4ee remain. Author/root/different
reviewer new16/full95 pass, no warnings. Actual different code/config approval
8c4261d integratedefdb987 includes sharedcaeb60d unconditional new16 CI promotion.
Original fulltest approval/red/selected-fixture gate remains intact, not repeated.
Project391/currentHTML/render/format/actionlint/whitespace, Python tooling19 and
DOM simulation pass; actual browser/platform verification remains unperformed.

Factory is exactly migration1 retain-config-context-v1, starts0/1, approvedtwoDDL
and7objects, independent2000-byte payload/SHA0906f121... verified by frozen test.
No mutable settings pointer/evaluator, document/wire schema, numeric config ID or
production fixture loading. Full-range opaque bytes do not certify actual typed
ReceiptId/ContributionId/CapturedPlanRevision SQL codecs; mandatory013/014/015
gates remain. Caller owns startup inspection, actual commit/publication and any
operator/backup/maintenance policy. No app bundle/recovery/power-loss/platform/
provider/live behavior is upgraded. Synthetic disposable databases only.

Source implementer /root/debt_code_reviewer owns only source/implementation evidence
in storage012-implementation; reviewer /root/storage_work_code_reviewer owns only
new code-review evidence in storage012-review. Both isolated branches completed
clean. Root owns shared build/CI/state/map/HTML/checkpoint/schema order.

Automatic selected officialQt new16/old73/green diagnostics now pass below.
Next final CI/metadata review and latest-head discussion/rules merge gates, then
refresh main and select dependency-ready STORAGETEST-013.
No owner question; full agreed product remains incomplete.

## Actual selected official-Qt automatic acceptance

[PR34](https://github.com/Kernol12/subathon-bot-ai/pull/34),
[run41322157820](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41322157820)
on2aadef323a5e1e47452a78a5005db04f835c9d28 completed SUCCESS as an automatic
pull_request run. Root read actual metadata/logs: hashes/GNU13.3/Qt6.12SQLite3.53.4,
old25/work24/migration24,green7(9 including init/cleanup,0skip),green3(5 including
init/cleanup,0skip),exact16discovery and real unconditional16/16 pass. All11 PR
checks atthathead SUCCESS/MERGEABLE/CLEAN; no transfer to later metadata heads.
Rootsourcec96 and all frozen artifacts match actual source/configapproval8c.
Component done, finalmetadata/CI attribution review/latesthead merge stillpending.
