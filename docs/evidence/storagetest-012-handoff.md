# STORAGETEST-012 — reviewed and observed-red retention baseline

Coordinator `/root`, 2026-11-01, task branch
`task/STORAGETEST-012-retained-context` from main29f3818 (merged PR32).
[Draft PR33](https://github.com/Kernol12/subathon-bot-ai/pull/33). Baseline gates
complete; final metadata/CI attribution review/latest-head merge gates remain.
STORAGE-012 production is still unimplemented.

## Contract, ownership and exact artifacts

Specification v0.45 §§3/6/7, DB-01; [brief](../tasks/EPIC-009/STORAGE-012.md),
[contract](../contracts/retained-context-v1.md), [author](storagetest-012-author.md),
[independent review](storagetest-012-review.md), [host probes](storagetest-012-probe.md),
[CI](storagetest-012-ci.md).

| Artifact | Git blob |
| --- | --- |
| Contract | 5b0ee84acbbf1a3ce8dbc57f99c110f3073d353b |
| Public API | 55c82815986052b996a46c408ab616e68815a4f2 |
| Explicit three-body stub | b27eb48867df8a6dbc2c0c92da3eba0a80b11d76 |
| Feature16 | c0e54003802f63a6f0393250f3bcca58cea8ee04 |
| Green fixture3 | aaa1644771decdb722b1105d220bac5313d54c6e |
| Independent frozen vectors | 0f76fd2c1bb1766c46a47ddcd38f46c18d4704a8 |

Contract3cca59f/root8813dcc plus root migration/brief choices791cacf approved
40d0dc6/rooteddbe2b. API/stub/precision6bfbb90/roota6a4402 and minimalbuild
e5532d1 approved46fc8ee/root2d0289b. Actual newtests7d89c4c/root320ae61 were
authored uncompiled as authorized. Fixture1eda/root21f explicitly disablesFK
only for independent negative setup, preserving product writer ON. Different
actual review found incomplete post-retain stage proof and nontext context/link/
independent-limit coverage. Author strengthened e76/root180 and648/rootab4,
retaining all assertions. Timeout helper70/root306 preserves failure/done state
and removes a nodiscard warning. Findings are preserved3b914ec/root02dc87b.

Actual fulltest/interface/fixture/harness approval672f93c integrated8d561a6
approves exact27fea1d artifacts above BEFORE any new feature-body execution.
It explicitly required selected officialQt currentgreen3 success before red.
The reviewer independently built/discovered16/3/95 and ran only green3, with no
new feature-body call. No approval transfers to future implementation.

## Observed build, selected fixture and intended red sequence

Root configure/build sqlite-retained-debug exit0, final build zero warnings.
Exact compiled16/registered16 and green3 match;95 total registrations. Local
green3 (5 including init/cleanup,0skip) and old78 separately pass. No full95
passing claim; new16 deliberately remain stubs.

[Actual automatic run41318070740](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41318070740)
on cb7db2db511d54422eb5961168cd64df1e758ed4 passed official Qt/ICU hashes,
GNU13.3, Qt6.12/SQLite3.53.4, builds, old25/work24/migration24/green7, exact
newgreen3 execution (5 including init/cleanup,0skip) and compilednew16 discovery.
Manual feature-body step was not executed. Root read actual metadata/logs BEFORE
first local feature-body run. All11 PR checks atcb7db2d succeeded; no transfer
to future metadata heads. Host uses Qt6.12/SQLite3.54.0/GNU16.2.1.

Only then, at cb7db2d, after validating exact artifact hashes and fullapproval
ancestry, root ran:

```sh
ctest --preset sqlite-retained-debug -L sqlite-retained --output-on-failure
```

Actual exit8, all16 cases fail explicitly with feature_not_implemented=0; all16
init and16 cleanup pass, zero skips/crashes/dependency/setup failures, total0.09s.
The SQL-failure stage guard also reports the explicit stub rather than falsely
accepting an earlier failure. [Raw first red](storagetest-012-red-ctest.txt).
Do not repeat this completed red or treat it as implemented retention.

The full independent migration payload is2000 bytes, SHA256
0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e.
Author/root/reviewer separately reproduce exact F framing, unsigned-byte object
sort, seven-object projection and12 opaque byte vectors without UUT-derived
expected results. Root hash checks preserve all old implementations/interfaces/
73 cases/green7/fixtures. Project391/currentHTML/render/format/actionlint/diff pass.
Python tooling19 and DOM simulation also pass; actual browser not run. Final
metadata/latest-head publication gates remain.

## Scope and next handoff

Author `/root/writer_transaction_reviewer` owns new contract/API/stubs/test/probe/
vectors and evidence in storagetest012-author. Different reviewer
`/root/storage_work_code_reviewer` owns only review evidence in storagetest012-review.
Root owns actual migration order/DDL acceptance, shared build/CI/state/map/HTML/
checkpoint. Separate branches/worktrees and historical reviews remain preserved.

Exact version identity is kind+opaque byte reference, document is required present
BLOB (empty valid/null input invalid), context link is optional typed configuration
reference. Caller owns real transaction/commit/publication; facade owns only weak
revocable session/limits. Validation/error/known-cleanup/uncertainty bounds are
explicit; no global audit, latest-settings/evaluator/wire schema is supplied.
Application migration1 retain-config-context-v1 contains two retained tables,
two generated indexes plus three catalog objects, starts0/1. Actual typed uint64
fields remain explicit lossless SQLcodec/zero/range gates in013/014/015; full-range
opaque-byte preservation here does not certify those converters.

Next final metadata/red/CI-attribution review, latest-head checks/discussions/
current rules and normal PR33 merge. Refresh main, claim STORAGE-012 separately,
then implement ONLY the stub source and own evidence on an isolated branch,
preserving all reviewed assertions/API/vectors. Different source review, new16/
old73/green diagnostics passing and unconditional automatic new16 CI are required.
Read-only implementation prep is complete; no implementation branch/claim/edit
has been made. No owner question. All domain semantics/application bundle,
backup/update/recovery, seven integrations/three FX providers, renderers/UI/
locales/themes, packaging/platform/live/OBS gates retain full release scope.
