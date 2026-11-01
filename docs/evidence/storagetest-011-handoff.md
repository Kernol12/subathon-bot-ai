# STORAGETEST-011 — independently reviewed migration baseline and observed red

Coordinator `/root`, author `/root/writer_transaction_reviewer`, different reviewer
`/root/storage_work_code_reviewer`, 2026-11-01. Branch
`task/STORAGETEST-011-migration-catalog` from merged PR30/main88e86c1.
Specification v0.45 §16, DB-01/OPS-02; [contractrevision1](../contracts/migration-catalog-v1.md).
[PR31](https://github.com/Kernol12/subathon-bot-ai/pull/31) remains draft pending
publication gates. No owner question. STORAGE-011 is still unimplemented.

## Exact completed test-first gate

Author6a/cd25/05c contract corrections, MC15amendment6eb and precision7ef received
separate actual approvals1bfdfa8/ecff4f5/112bf38. [Review history](storagetest-011-review.md)
preserves initiald600 requestchanges, real shadowing and encoding findings.
Actual full authorfa16183 integratedb67e81b; reserved catalog DDL variantcae0267
integrated4fa008f. Different full test/interface/fixture/harness approval01f0975
integratedcb3c1b2 BEFORE any new migration case ran. [Author](storagetest-011-author.md).

| Artifact | Exact approved Git blob |
| --- | --- |
| Contract | cbdea1a48d2989a9c8bd17edf0539bdf7c1bce61 |
| Public migration header | 02f83ce592b110176c1dac46f3e5ea4d93529fcd |
| Explicit stubs | 6c5073d4a97eedbdd34130ce48be51cf0aa4537a |
| New22 Qt cases | a85a6c057637f43201dfaca308fbec27748ce2dd |
| Green7 diagnostic probe | 832fe61656451390ff465d4411c7d0900c741bd6 |
| Ten frozen vectors | 12280c8f403fd3902edaeff88e9a236f9c5b613b |

Root atcb3c1b2 then ran `ctest --preset sqlite-migrations-debug -L sqlite-migrations`:
actualexit8,22/22 failures on explicit codec/admission feature_not_implemented,
all case initialization passed, zero skips/crashes/compile/dependency failures.
[Full raw output](storagetest-011-red-ctest.txt). No failure inversion/assertion
weakening. All root/author/reviewer builds/22+7/76discovery and green existing
acceptance pass; old source/header/hook/49 remain byte-identical to88e86c1.

The selected officialQt prerequisite actually ran before red: Ubuntu41307014192
at4fa008f passed hashes/GNU13.3/loop1/realold49/exact7greenprobe with Qt6.12 and
SQLite3.53.4 (9 Qt passes inclinit/cleanup,0skips)/exact22discovery. All11checks
there success. [CI evidence](storagetest-011-ci.md) attributes exact runs; later
heads need fresh checks. LocalLinux GNU16.2.1/Qt6.12/SQLite3.54, CMake4.4.5,
Ninja1.13.2/Python3.14.8; rootgreen54 old49+infra5 pass.

## Bounded coverage and remaining gates

The22 cases map MC01–18 plus ownership/admission/lifetime guards: full catalog
identity/schema/history/content, UTF8/raw bytes, strictfunction diagnostics,
fresh/prior/future/refusal, wholeprefix atomicity, cancellation, busy/no replay,
lateDDL resource failure and real physical ackloss. Exact reserved-DDL change is
isolated with otherwisevalidrows/history/integrity, no early unrelated refusal.
All10 payload/length/SHA vectors and base1127 bytes independently rederived; full
m1 is636/SHA3d1e0f. Expected data never comes from the new UUT. Strict UTF8 format
and explicitinspect/advance preserve fullUnicode and OPS02 caller prerequisites.
No writer API/allowlist/production source expansion or new dependency is needed.

Root owns shared CMake/presets/workflow/map/state/HTML/checkpoint. Author completed
clean in storagetest011-author; different reviewer owns only011-review evidence in
storagetest011-review. Final completionmetadata/red/actualCI attribution review,
latest-head checks/discussions/currentnormalrules remain before PR31merge. No
root process remains running. Preserve all branches, requestchanges and valid
approvals; never repeat the original red as a new verification stage.

After merge refresh main, claim STORAGE-011 separately, implement only new migration
source against these unchanged artifacts, obtain different code review and pass
realnew22/old49/green7/full76 while enabling ordinary automatic new acceptance.
Actual backup/update authority/domain stores/appbundle/recovery/powerloss/platform/
liveprovider and all seven integrations/three rate providers/full release scope
remain mandatory separate gates.

Final completion/red/CI attribution atbd81512 independently approvede1be8ae.
Reviewer parsed all22 explicit failures and init/cleanup/exit8, verified prior
approval ancestry and independently fetched actual41307014192 metadata/logs/exact
4fa11checks. No native/red rerun. Publication-head CI/discussions/rules remain.
