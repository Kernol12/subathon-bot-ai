# Coordinator checkpoint — STORAGE-017 component complete, final PR46 merge gates

Continue [standing coordination](../COORDINATOR_PROMPT.md). Full product remains
incomplete; all seven integrations/threeFX, both renderers/platform/UI/operational
release gates remain mandatory. No owner decision/access needed now.

## Actual integrated work

Fetched/local main **a40b42fc6d6e2a419f90f3abfd246c29dfd8173c**.

- [PR43](https://github.com/Kernol12/subathon-bot-ai/pull/43) STORAGE002 merged
  34ed520→8c7f71f05:49:50Z; all11final/all10postmainSUCCESS.
  [Handoff](storage-002-handoff.md), [review](storage-002-code-review.md), [CI](storage-002-ci.md).
- [PR44](https://github.com/Kernol12/subathon-bot-ai/pull/44) ACCTCONTRACT001
  DOCUMENTONLY, dd5e6f1→21c087106:23:15Z; all11final/all10postmainSUCCESS.
  [Handoff](acctcontract-001-handoff.md), [ADR0003](../decisions/0003-application-service-prerequisites.md).
- [PR45](https://github.com/Kernol12/subathon-bot-ai/pull/45) STORAGETEST017
  testbaselineONLY, exact48b36a0d03c052d6611e877e8447e507c6a400a2→a40b42f
  **2026-11-03T08:35:37Z**. All11finalSUCCESS inclSQL41604081877; ready/CLEAN/
  MERGEABLE/zero unresolvedthreads/currentrules26170263formalGHapprovals0/
  rootclean/criticalreviewedblobsunchanged. Normal exact-head merge, no bypass/
  force/deletion. Mainffclean, merge tree equals final48. Postmain all10SUCCESS observed, includingSQL41604430192.
  [Handoff](storagetest-017-handoff.md), [fullreviewf5c6](storagetest-017-review.md),
  [final attributiona729+eb563](storagetest-017-final-review.md), [CI](storagetest-017-ci.md).

## Accepted baseline — preserve exactly

Concrete contractbad19c47, API headercf721e7/source130e3db named immediate stub.
New types+const original-claim query only; oldheader8e24/sourcea905 restored by
removing additions. Independent JSONbea127/helper67d4/environment7dad, full66
coherent catalog/trueMAX allocators/A/B body161/160 plus1tag; signed ORIGINAL C
Receipt8 is NOT a real linked confirmation. Old nonempty20 fixture unchanged.
Final feature CPP **0e4f4f006d2a0e161857e38255cac487f7de83a9**,18slots/35scenarios.
All three actual fullreview findings fixed before execution. QtCMakec35ccf28/
workflow7fec9ade/presets323e/rootCMake4a404 approved; full228=prior210+new18.

Complete approved clean c24fb6eb4d7c4bb666b1dba3ff077a54014dd865 got OWNselected
official41602589680 SUCCESS08:20:48–08:22:39Z/fresh100/zero/GNU13.3/Boost108300/
Qt6.12SQLite3.53.4/requirednew3green5PASS0skip88ms/compiled18/oldstable/all11.
ONLY THEN FIRSTsamecleanhead red08:24:35.551–08:24:36.110Z, exit8/0.55s/all18fail/
35EXACTnamedmarkers/36initcleanupPASS/0skip/crash/setupfailure.
[Originalraw](storagetest-017-red-ctest.txt) SHA90f3c739a735c13ca4eddecf0055ce7322cf1247e6dd52bc030e0956b1ee19b0.
Markers prove setup reached first new calls; downstream assertions remain unexecuted
until implementation passes. Preserve original red; DO NOT REPLAY as a new baseline.

## Active implementation / ownership

STORAGE017 DONE component only, UNMERGED; [draft PR46](https://github.com/Kernol12/subathon-bot-ai/pull/46). Root `task/STORAGE-017-original-claims` created from
actual refresheda40 BEFORE edits/claim. [Brief](../tasks/EPIC-009/STORAGE-017.md).
Only implement approved const query; no schema/header/old equality/fixture/test
weakening. Options→source pure preflight, global taggedsubject first then ORIGINAL
receiptsource, no scopefilter/candidateadmission, FULL selectedoldlookupclosure,
exactbody+1tag, provisionalowneddata, realwholeunitrollback/originalunknown and
cumulativewriterbudget remain required.

Fresh isolated worktrees/branches from integrateda40:

- `/root/bundle_implementer` explicitly reassigned runtime source author.
  `local-evidence/worktrees/storage017-source`, `task/STORAGE-017-source`.
  Own ONLY `server/src/sqlite_application_bundle.cpp` and
  `docs/evidence/storage-017-author.md`. COMPLETE d675eb9+a9d184d, clean source
  tree; runtime834336d3/author492add53 imported34f7dd5/7de22a6.
- `/root/bundle_reviewer` independent source/config reviewer.
  `local-evidence/worktrees/storage017-review`, `task/STORAGE-017-review`.
  Own ONLY `docs/evidence/storage-017-code-review.md`; actual source/harness review COMPLETE11012da, imported634d45e.
  Own18/full228PASS0skip/fresh1000warnings and actualofficialCI inspected.
- `/root/bundle_validation` independent native validator.
  `local-evidence/worktrees/storage017-validation`, `task/STORAGE-017-validation`.
  Own ONLY `docs/evidence/storage-017-validation.md`; actual source834 native validation COMPLETEbcffc51/93b7001/9455421,
  imported4c1d/c5b/554. New18/full228PASS0skip/100+26zero warnings. No red replay.
- Root owns state/HTML/map/build/presets/workflow/schema order and actual merge.

Source author read-only preparation recommends one existing Access+guarded path,
global strict key projections and existing lookup full closure; extract the exact
existing recorded-body byte walk for oldlookup+wrapper reuse, preserving charges/
order and checking one extra enum byte only after selected identity. That preparation preceded actual source d675eb9. Root [CI/native evidence](storage-017-ci.md):
initialnew18PASS0.81s, promoted candidate26buildsteps/0warnings/full228PASS7.88s/
225Qt totals715PASS0fail0skip, compiled18=registered18/full228. Baseline tests/
fixtures/helper unchanged. Header8614 removes ONLY obsolete1stubstatuscomment;
rootCMakea713/preset0acd descriptions, QtCMake687a removes newdraft/comment,
workflowc5d1 promotes mandatoryordinary18 after oldstable/new3green/exactguard.
Different source/config review11012da and independent validation COMPLETE.
Actual ownpromoted official41605888462 at680 SUCCESS08:47:22–08:48:51Z/fresh100/
zero/GNU13.3/Boost108300/Qt6.12SQLite3.53.4/new3guard5PASS61ms/exact18/
UNCONDITIONAL18PASS1.29s/oldstable/all11. [Handoff](storage-017-handoff.md).

## Next concrete actions / preservation

1. Final completion metadata now ready; preserve actual source834/header861/
   fiveconfigpins and baseline0e4/raw90f. Publish final candidate; observe ITS OWN
   latestall11CI, actualready/CLEAN/MERGEABLE/zero unresolvedthreads/currentrules/
   cleanroot and normalexacthead PR46merge only. No futureheadCI preapproval.
2. Refresh main after integration; record postmainCI separately. Claim next ready
   STORAGETEST016 from actualmain before edits, concrete hold contract first then
   differentactualdocreview before schema/API/frozenfixtures/tests/red.
3. Hold016 remains separately required; service004 waits BOTH integrated ports
   and its own reviewed baseline/red. Read-only016prep identifies globalgating/
   detectionvsheldidentity/multipleholds/versiontransitions/continuation questions;
   no implicit unresolvedpolicy enablement.

Previous branches/worktrees and valid reviews preserved. Earlier storage002-review
retains3 root-config verificationcopies; storagetest017-review retains4. No unfinished
old source/test work, no prune/reset/discard. Latest preclaim inventory107refs/
91existing/16absent; three new trees added here. Root completion CI/state/map/checkpoint/handoff updates are being frozen for
publication. Source/reviewer/validator agents COMPLETE; native work finished. Inspect actual processes
and worktrees on resumption. All prior dirty verification copies preserved.
Current ruleset has no required-status-check rule; coordinator explicitly verifies
all11 checks. No future source/query/service/platform/live evidence claimed.

Preserve five uncommitted shared verification copies in new storage017-review
and six imported verification copies in storage017-validation, plus earlier dirty
review trees. No unfinished source or test edits; no reset/prune/discard.
