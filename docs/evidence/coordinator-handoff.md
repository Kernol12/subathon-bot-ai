# Coordinator checkpoint — STORAGE-012 implementation

Coordinator `/root`, 2026-11-01. Continue the full specification v0.45 standing
loop under AGENTS.md and COORDINATOR_PROMPT.md. No owner decision or access
is needed. Full product remains incomplete.

## Integrated tasks and actual main

Last observed main is `ae57f2bef8ca90f2c2fd150a6c4ae6e03ee77365` (PR33).
Postmerge checks have started; inspect actual results before claiming them. Recent completed PRs:

| Task | Merged PR | Durable evidence |
| --- | --- | --- |
| CORE-005 | [27](https://github.com/Kernol12/subathon-bot-ai/pull/27), 1259cfb | [Handoff](core-005-handoff.md) |
| CONTRACT-004 | [28](https://github.com/Kernol12/subathon-bot-ai/pull/28), e374788 | [Handoff](contract-004-handoff.md) |
| STORAGETEST-010 | [29](https://github.com/Kernol12/subathon-bot-ai/pull/29), c60e1ed | [Handoff](storagetest-010-handoff.md) |
| STORAGE-010 | [30](https://github.com/Kernol12/subathon-bot-ai/pull/30), 88e86c1 | [Handoff](storage-010-handoff.md), source023451 |
| STORAGETEST-011 | [31](https://github.com/Kernol12/subathon-bot-ai/pull/31), ef29c87 | [Handoff](storagetest-011-handoff.md) |
| STORAGE-011 | [32](https://github.com/Kernol12/subathon-bot-ai/pull/32), 29f3818 | [Handoff](storage-011-handoff.md) |
| STORAGETEST-012 | [33](https://github.com/Kernol12/subathon-bot-ai/pull/33), ae57f2b | [Handoff](storagetest-012-handoff.md) |

Earlier core/accounting/writer contracts, reviews and red evidence remain valid
in their linked task records. Do not repeat completed preparation or original red.
Preserve existing refs, reviews and local-only historical checkpoints.

## Latest completed mechanism

STORAGE-011 PR32 actually merged29f3818 at2026-11-01T13:32:15Z after exactfinal
headc31edca all11 checks SUCCESS, no threads/currentnormalrules, source/config
approval854cace and finalmetadata/CI-attribution approvala70e495. Source4ee2365,
teste94befdc (original22 bodies intact plus two focusedUnicode regressions),
header02f83ce/contractcbdea1a/tenvectors12280c8f/green7832fe616 unchanged.
Author/root/reviewer full78 pass; actual officialUbuntu41310940283 atda7cbde
passes pinnedhashes/GNU13.3/Qt6.12SQLite3.53.4/old49/green7/exact24/real24.
Finalhead writer-run41311584939 also succeeded, all11 checks confirmed before
normal merge. [Implementation](storage-011-handoff.md), [review](storage-011-code-review.md),
[CI](storage-011-ci.md). Original reviewed22 stub-red plus actual independently
reviewed U+FEFF2 regression-red-before-fix retained; never repeat completedred.
No domain schema/realadapter/application bundle/backup/updateauthority/recovery
or platform/provider upgrade. Preserve clean completed source/reviewer/regression
worktrees and all historical branches/reviews.

## Latest integrated baseline and preserved gates

PR33 STORAGETEST-012 actually mergedae57f2b at2026-11-01T15:08:19Z after
exactfinal8353b00/all11SUCCESS, actualfullreview672f93c/root8d561a6 and final
metadata/red/CI-attribution review9f55963/root8353b00, no discussions/currentrules.
[Baseline handoff](storagetest-012-handoff.md), [review](storagetest-012-review.md),
[CI](storagetest-012-ci.md), [original first red](storagetest-012-red-ctest.txt).

Exactbaseline contract5b0ee84acbbf1a3ce8dbc57f99c110f3073d353b, header
55c82815986052b996a46c408ab616e68815a4f2, explicitstubb27eb48867df8a6dbc2c0c92da3eba0a80b11d76,
featurec0e54003802f63a6f0393250f3bcca58cea8ee04, greenenv
aaa1644771decdb722b1105d220bac5313d54c6e, JSON0f76fd2c1bb1766c46a47ddcd38f46c18d4704a8.
Independent literal payload2000/SHA0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e
and12opaquevectors verified. Both oraclefindings fixedseparately/allassertions retained.

Fullactualapproval8d ancestorcb7, then actualselectedofficialQt41318070740 atcb7
passed hashes/GNU13.3/Qt6.12SQLite3.53.4/builds/old73+green7/currentgreen3(5incl
initcleanup0skip)/exact16discovery; manualnewbody notexecuted. Onlythen rootfirst
new16bodyrunatcb7 exit8/all16explicitfeature_not_implemented=0/16init+16cleanup
PASS/0skip/crash/dependency/setupfail,total0.09s. Do NOT repeat completedred.
Author/root/reviewer builds/95registration/16+3discovery/localgreen3+old78 pass.
Finalhead8353b00 writerCI41319443691 and all11 checks passed before merge.

## Active task, scope and ownership

STORAGE-012 claimed on task/STORAGE-012-retained-context from freshlyrefreshed
mainae57f2b. Dependencies STORAGE011/CONTRACT002/STORAGETEST012 done/integrated.
Only production source server/src/sqlite_retained_context.cpp plus own new
implementation evidence is assigned to implementer. Preserve API55/contract5b/
featurec0/envAAA/JSON0f, oldwriter023/migration4ee and all73+green7+newgreen3.

Scope: approved literal migration1 retain-config-context-v1/twoDDL/sevenobjects/
starts0,1; synchronous exactBLOBfind/retain, typedoptionalconfiglink and namespaces,
idempotence/conflict/missing, positiveinputprecedence, ownednonnull-emptyresult,
selected-rowtypes/limits/knowncleanup/uncertainty through existingcaller-owned
SqliteWorkSession only. No privateSQLhandle/newconnection/thread/autoBEGIN/COMMIT/
retry/numericconfigID/coreport/evaluator/provider/wire behavior. No fixtureloading
as productiondata. Actual typed uint64 SQLcodec gates remain013/014/015.

Implementer /root/debt_code_reviewer is explicitly reassigned from completed
readonlyprep to NEW isolated local-evidence/worktrees/storage012-implementation,
branch task/STORAGE-012-implementation, owning only newmodulesource/own evidence.
Different code reviewer /root/storage_work_code_reviewer will use NEW isolated
storage012-review/task/STORAGE-012-review, owns only new code-review evidence.
Root owns schema/migrationorder/sharedCMake/presets/CI/state/map/HTML/checkpoint.
New worktrees/assignments must be created/sent after this claim commit; verifyactual
onresume. No prior review approves an implementation that does not yet exist.

Next source implementation preserving frozenbaseline→actualnew16/full95 pass→
differentactualsource/configreview→removeintentionalredguard/promotenew16 to
automaticunconditionalCI and observeactualselectedUbuntu execution→done evidence/
finalmetadata/latestheadmergegates. Root can prepare CIpromotion but must not claim
passing retention from prior baseline/compiling stubs. No ownerquestion.

## Resume audit and release scope

Last full audit55 existing worktrees clean at snapshot,16 absent historical/tmp
references preserved/no pruning. Old author/reviewer/prep tasks completed clean,
no rootnativecommand after merge/fetch/claim. Inspect actualagents/files/processes/
GitHub onresume, preserve unfinishedwork/validreviews/normaladd-add cf147 history.
Root original baseline red isdurable, no need rerun.

Ruleset26170263 currently requiresPR/resolvedthreads,0GitHubapprovals,no bypass/
force/delete,no required-status-check rule. Written allapplicablelatestheadCI
remainsmandatory; repositoryreview doesnotreplace anyrequiredGitHubapproval.
Mainae57 postmerge checks pending observation.

Full product remainsincomplete: domain/applicationbundle/backup/update/recovery,
7integrations/3FXproviders/renderers/UI/locales/themes/packaging/platform/live/OBS
all remain required. No credentials/supporterdata/chat/payment/deploy sideeffects.
