# STORAGE-016 — durable application-review hold repository

Coordinator/integrator and independent native validator `/root`,2026-11-03,
`task/STORAGE-016-durable-holds`, [draft PR48](https://github.com/Kernol12/subathon-bot-ai/pull/48).
Runtime author `/root/bundle_implementer`; pure-value author `/root/bundle_validation`;
different source reviewer `/root/bundle_reviewer`.

Actual native and official acceptance PASS; final full independent source/config
review is pending. Task remains in_review, not done or merged. Completion metadata
and latest CI/current rules/discussions/readiness/normal merge remain separate.

## Scope and contracts

The six hold functions now provide the exact additive pure manifest, narrow view,
complete active-subject discovery, bounded history, detection and resolution on
the caller's active writer session. Immutable causes, source/plan/state provenance,
history/versions, retries, independent active holds, complete-set CAS and atomic
replacement remain distinct. A staged/read result is provisional until the actual
caller commits and its work body completes normally. Known local failures roll
back the whole caller unit; uncertain/capability failures preserve writer truth.

Authority: specv0.45 §§3–5/NUM-02/04/DB-01; logical701c710f, physical83e96362,
API46c70297. Migration6 `durable-application-holds-v1` remains the exact12 approved
DDL statements/full89 objects/F89890/SHA
9f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a;
all five older prefixes remain exact. No new dependency, public type/signature,
wire schema or policy interpretation.

Runtime CPP8d82aa2268b39f19b8ed640c03383c99f640d6e3 and final pure helper
13828cc3c72548791dc4299b7fd4c813c36b59f3 use strict selected-source closure,
unfiltered actual counts before vector materialization,64/65 metadata buffers,
constant-space ancestry and exact returned B/I accounting. Existing bundle reader
factoring into privateAccess7107d0ee preserves old behavior/body/queries/accounting;
old bundle valuesb0ca921c is unchanged. Actual allocation ordering still requires
the separate final source audit; passing tests alone do not measure allocations.

## Reviewed baseline and genuine corrections

Original [baseline handoff](storagetest-016-handoff.md) and actual separate
[final attribution](storagetest-016-final-review.md) retain the first valid revised
red on SAME CLEAN598 AFTER its own selected41640390883:35slots/265unique named
markers/70initcleanupPASS/0setup/skip. Earlier261markers+4setup-error attempt is
preserved as invalid; no replay or assertion weakening.

Root-authored [baseline revision](storage-016-baseline-revision.md) and different
[revision review](storage-016-baseline-revision-review.md) document two genuine
history query cursor0→1 corrections, matching the unchanged frozen position2-only
338B6I oracle. Three additive priority/lineage regressions independently approved
1650658 reached actual behavior red on CLEAN68dc779 before corrections. A fourth
token-phase regression approved001896f reached public red on CLEANf67ae73 before
the final bounded helper correction. Both raw outputs are retained. No original
row/assertion/oracle/fixture was removed or relaxed. Final testd740a175 has35slots/
269scenarios; frozen JSON63de09c5/helper7ca946f2/environmenta17312fe remain unchanged.

## Actual checks and CI coverage

[Independent native validation](storage-016-validation.md) pins the exact final
code/config/test artifacts. Root final3-step build0warnings; actual full264 CTest
exit0/PASS19.97s, including35hold slots/269scenarios.261Qtprocesses sum1059PASS/
0FAIL/0skip/blacklist on Arch/GNU16.2.1/Boost1.92/Qt6.12/SQLite3.54. Author checks
and frozen accounting/manifest diagnostics are separately documented in
[runtime author](storage-016-author.md) and [pure author](storage-016-values-author.md).

[Own promoted official CI41658920776](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41658920776)
at7a3dcfcfe292b77f29ba648809c7eb0a03b46913 SUCCESS14:57:58–15:00:42Z:
fresh110/0warnings/GNU13.3/pinnedBoost108300/verifiedQt6.12/SQLite3.53.4;
mandatory new3fixture guard5PASS0skip181ms/exact35compiled/**UNCONDITIONAL35
PASS26.41s**/all old stable suites/all11 same-head checksSUCCESS.
[CI evidence](storage-016-ci.md) records exact raw/hash and synthetic PR checkout
tree equality. New suite executes automatically; old tests/pins/permissions/check
names remain fixed. No failure inversion/disabled tests/manual switch/hiding.

Project consistency/progress rendering and source/metadata whitespace checks pass.
Final source review, completion metadata and latest-head CI are still required.

## Remaining gates and next task

No service authority/evaluator, current permission/policy/segmented-clock/action
producer, financial entitlement/refund allocation, job delivery, backup/recovery,
UI/renderers/locales/themes, live provider/platform/package or release capability
follows from hold persistence. DB-01/NUM-02/NUM-04 stay partial. All seven
integrations and all three exchange-rate providers remain required.

Changed paths: new hold runtime/private values/private shared access, factored old
bundleCPP, comment-only public header, four additive test rows/two history inputs,
new hold CI promotion/build wording, evidence/progress/HTML. All prior worktrees,
dirty verification copies and valid reviews remain preserved. No owner action.

After the actual final gates and normal merge, refresh main and reclaim
ACCTTEST-004. Its next gate is a concrete service API/complete-command codec
document under logical6d0cd67f/ADR0003, independently reviewed BEFORE minimal API/
stubs, independent real-SQL fixtures/tests, review and its OWN valid service red.
Older component acceptance is not service acceptance. Do not stop at this PR.

Final source audit found a retained-source query exception mapping gap after the
above269-row acceptance. New test522faee/0e172a16 adds one270th row, under actual
independent review before red/source correction. Known writer result failures
inside the retained primitive must map to Hold result_limit after already-known
rollback; capability/protocol/unknown errors must remain original writer truth.
The269-row pass is preserved, but current270 acceptance/sourceapproval is pending.
