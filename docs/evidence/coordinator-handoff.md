# Coordinator checkpoint — ACCTTEST-004 independent fixtures

Continue [standing coordination](../COORDINATOR_PROMPT.md) under AGENTS. Product
incomplete. No owner decision or access needed. Do not stop at a completed PR or
repeat completed preparation/red. Inspect actual branches, agents and CI on resume.

## Actual integrated main

Main **efe1c7e87ae2a7e8a92c3f5e7ede88e68e72b089**, tree
45216582d5b715ae63b220d5df1aca2c17f417b4, equals exact final PR48 head
5fa01364fa2636c6b68de4281816057c5030604f. PR48 normally MERGED15:31:10Z after
all11 latest successful incl SQL41664119920, ready/CLEAN/MERGEABLE/current
rules26170263 formal0/thread0/rootclean/14 reviewed artifacts unchanged. No
bypass/force/delete. Postmain efe ALL10 workflows actually observedSUCCESS,
including SQL41664725048; ignored storage016-post-main-runs.json records them.

Recent completed task PRs:

- [PR43](https://github.com/Kernol12/subathon-bot-ai/pull/43) STORAGE002 →8c7f71f,
  [handoff](storage-002-handoff.md).
- [PR44](https://github.com/Kernol12/subathon-bot-ai/pull/44) ACCTCONTRACT001 →21c0871,
  logical6d0cd67f/ADR0003e971a47e, [handoff](acctcontract-001-handoff.md).
- [PR45](https://github.com/Kernol12/subathon-bot-ai/pull/45) STORAGETEST017 →a40b42f.
- [PR46](https://github.com/Kernol12/subathon-bot-ai/pull/46) STORAGE017 →aad1ad1,
  [handoff](storage-017-handoff.md).
- [PR47](https://github.com/Kernol12/subathon-bot-ai/pull/47) STORAGETEST016 →017d40f,
  [baseline](storagetest-016-handoff.md). First VALID reviewed265-marker red598
  after own selected41640390883 preserved; initial261+4 setup failures invalid
  and retained. Do NOT replay the completed first red.
- [PR48](https://github.com/Kernol12/subathon-bot-ai/pull/48) STORAGE016 →efe1c7e,
  [handoff](storage-016-handoff.md), [source approval](storage-016-code-review.md),
  [validation](storage-016-validation.md), [CI](storage-016-ci.md).
  Final270 rows/full264/1060QtPASS0skip0warnings; own corrected official41662457471
  plus finalhead all11 passed. Two genuine history inputs and five additive
  reached-red regressions separately reviewed; all old fixtures/assertions fixed.

## Active primary task and current artifacts

Root branch `task/ACCTTEST-004-service-baseline`, created from refreshed efe BEFORE
claim5236d88. [Draft PR49](https://github.com/Kernol12/subathon-bot-ai/pull/49), last
published complete API head **8d742239616aec1b8e9be65e972a42b4fcf2ae7a**; root now
publishes review/gate/fixture-ownership metadata. Inspect actual HEAD/remote before
acting. ACCTTEST004 in_progress; ACCT004 production PLANNED. No service feature
suite, FIRST service red or implemented service behavior exists.

Approved concrete DOCUMENT:

- docs/contracts/application-service-api-v1.md **8fa0c76ad90ad046712463dfc95206e2a99b4e40**;
  document author note **608acc129c48f8ee5ed97fd09db1ade541511c56**.
- Full actual approval b098c7ef imported99d3ef6,
  [review](accttest-004-api-review.md)615b64db.
- Separate bounded codec/accounting/arithmetic approval7b501fd3 imported950db76,
  [review](accttest-004-codec-review.md)c9133b0e. Original changes-requested f8fc91e
  retained. Actual fixes6160/ee056 cover historical hold retry/no false current
  blocker, all prepared-pair lifetime, per-method codec budgets/decode errors,
  running per-occurrence60→60, finite token-free proposal/jobs→refs→codecs order,
  echoed ref binding, local-view versus actual writer errors and missing callback.
  All literals/CS arithmetic independently checked. DOCUMENT approval only.

Approved MINIMAL interface/stubs:

- header **03a747a70ab7decd49227396466ff12a32c31864**;
  source **ebbea2388b7f4fb6474940b53cc14b95853969cb**.
- Header author fe816735 importedc0359cb,
  [note](accttest-004-header-author.md)a863a1d9. Five entry definitions immediately
  throw named markers (four families); ctor inert; finite owned DTO/ports/private
  weak read-view/matching real Work+Completion/Response metatype. NO validators,
  codecs, byte helpers, policy/numeric/queue/SQL/commit/cleanup/publication logic.
- Different actual header/stub/build approvalbeb928cf importeda5af686, plus completed
  official API audit3e62e3be importedf02054f;
  [review](accttest-004-header-review.md)7ee952d1. Independent fresh34-step archive
  build0warnings/exact264OLD discovery/no new entry call. Five dirty verification
  config copies intentionally remain in its isolated tree.
- Root harness5df5977: rootCMake166cb1f8/core1f40148a/serverf07cd0d0/
  presetsaeeca7a7/workflowe507c083. Profile21 `application-service-debug` inherits
  hold profile; existing numeric libraries without unrelated GTest flags/newdep.
  Selected job ONLY49 preset-reference substitutions, all old commands/guards/
  requiredhold35/pins/checkname/permissions preserved. No service test registered.

## Actual API environment gate completed BEFORE fixtures

[API CI evidence](accttest-004-header-ci.md): root fresh118-build ZEROwarnings,
OLD264 registrations/no service case, OLD264PASS20.47s/261Qt1060PASS0skip. No new
service/codec/view/stub/factory/marker called, not service acceptance or red.

OWN complete APIhead8d74223 [SQL41673443963](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41673443963)
SUCCESS16:21:58–16:24:36Z/fresh1180warnings/GNU13.3/Boost108300/officialQt6.12/
SQLite3.53.4/new interface explicitly compiled+archived/old stable suites/
hold3guard5PASS0skip179ms/exact35/hold35PASS26.73s/all11SUCCESS. Raw
local-evidence/accttest004-api-official-ci.txt SHA
8cd6edea68ab01f2409d11c9e9621be1d66f4f9be18fb068ed13b260e96ac4e8.
APIverified synthetic568fc520 parents efe+8d742, tree df709b23 equals candidate.
Different reviewer independently inspected completed raw/run/checkout. Prior
DOC-only CI was not substituted. API review+ownenvironment gates both PASSED.

## Current FEATURE TEST authoring phase — actual assignments

All document/header/fixture review and OWNfixture selected gates above are COMPLETE.
Final approved fixture JSON0ae4957c/helper069bf7b9/env5a36a407/author7d4dafac,
[full review](accttest-004-fixture-review.md)15f3149e imported90c45c3 +completedCI
addendum2688c1b3 imported295a8fc (currentreview2fd6a15f). Independent150frames,
all30cases/B-I/arithmetic/source32/3arrangements full89/FK0/integrity audited.
OneAS19success100→90→+10→100 preservesALLprior29objects/everyotherJSONvalue.
Source/header03a747/ebbea and contract862c419/author78f36dd remain frozen.

OWN COMPLETEfixturehead **8dfcf5563013f1b06108de68bed4b0766e62aa24**
[selected41685005509](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41685005509)
SUCCESS17:31:06–17:33:56Z/fresh1220warnings/GNU13.3/Boost108300/Qt6.12SQLite3.53.4/
oldstable/exact6mandatorycompiledguard/37QtPASS0skip1785ms/1.80s/all11SUCCESS.
Raw local-evidence/accttest004-fixture-official-ci.txt SHA
**d46a7df4f8a6ec9c61f84ca217a73ba714f1692b9fb6f0d0334e7985df0896c2**;
APIverified synthetic930cfce8 parents efe+8df/equalcandidate treeae9e17a4.
[Fixture CI](accttest-004-fixture-ci.md) records root/reviewer native37PASS0skip,
initialauthorsetupfailures/corrections, semanticpreservation and no newmethodcall.
Oldfull265/1096QtPASS21.55s appliedpre-AS19candidate; updatedfocused37 passes,
allold264 unchanged. No old baseline replay or service acceptance claim.

Only AFTER actual ownfixturegreen root recorded feature release **798d350** and
created NEW isolated tree from that approvedtaskrevision:

- `/root/bundle_implementer` ACTIVE featureTEST author in
  `local-evidence/worktrees/accttest004-feature-tests`,
  `task/ACCTTEST-004-feature-tests` at798d350 beforeedits.
  Own ONLY `tests/qt/application_service_tests.cpp` and
  `docs/evidence/accttest-004-test-author.md`.
  CurrentPLAN35 meaningful publicQt slots with data rows; actual source/N/rows not
  yet committed/verified. Four EXACT newmarkerfamilies must propagatethroughbody
  exception_ptr/workerthread to namedQtFAIL, never blanketoldlogicerror or marker
  as expected success. Real WorkResult plusMATCHING service completion observed,
  no forged committed result. No new codec in fixture/setup/expected generation.
  Compile/static/Qt-functions discovery ONLY; author mayNOT execute any newtest/
  init/body/stub/marker before actual full review+completefeaturehead own green.
- `/root/bundle_reviewer` test-review PREP COMPLETE/IDLE. Same preserved
  `local-evidence/worktrees/accttest004-service-api-review`,
  `task/ACCTTEST-004-service-api-review`; SIX verification config copies dirty
  (root/core/server CMake,presets,workflow,QtCMake), all preserved.
  MUST use followup_task on ACTUAL complete CPP/authornote/rootregistration.
  Own NEW `docs/evidence/accttest-004-test-review.md`, no prospectiveverdict or
  source/data edit. Completed fixture-CI audit2688 already integrated295a8fc.
- `/root/bundle_validation` fixture author COMPLETE/clean05f3ddd in
  `local-evidence/worktrees/accttest004-service-fixtures`,
  `task/ACCTTEST-004-service-fixtures`. Own4fixturepaths remain frozen after review;
  new genuine data gaps need explicit spec reason/separateactualreview/currentgreen.
  Earlier codec-review and pure-values trees/commits preserved independently.
- Root owns ALL sharedCMake/presets/workflows/state/HTML/maps/schema/integration.
  Featuretarget and explicitN registrations/compiled-discovery guard WAITactualCPP;
  currentmandatoryfoundation6 executes onlyOLDprimitives, excludesservicearchive.

## Next concrete actions and non-negotiable first-red gate

Receive actual authored testCPP checkpoint; wire root target/explicit compiledN
registrations with intentional draft-red label, OLDstable/foundation stillmandatory.
NO featurebody execution in CI or author diagnostics before fulltestapproval.
Reviewer independently reads ACTUALsource/oracles/helpers/harness/authornote and
links fullspec AS01–21/CS01–18. Preserveproofpartition: service tests pre-read15/
plan reassessment, clockcallbackbeforepausedrefusal/no writes/visibleunpausedhardend;
oldstore proves staleEXPECTED/current mismatches, NOT legalmidbodyrace; oldcore
proves pausedtraces; FUTUREactualimplementation sourceaudit checksfullcurrentcopy/
directstage/stalemaps/reducerbeforeadmission. No mutablephasehook/fakeportsSQL,
privateproposalclaim, waiver or silent release reduction.

Only AFTER actual fulltestreview plus COMPLETEapprovedfeaturehead OWNselected
foundationgreen/compiled exactN does ROOT invoke FIRSTservice missingfeature red on
SAMECLEANhead. Record per-invocation LastTest/counts, not duplicated verbose stdout;
no compiler/missingdependency/skip/setupassertion is validred. Everynewrow should
reach a namednewstub after validoldsetup. Genuine baseline defects require written
spec reason/differentactualreview/revisedbaseline evidence, no weakening.
Then baseline completion/latestCI/normalmerge gates; ACCT004 production waits
integrated reviewed/red baseline and its OWN separate source/acceptance review.

## Continuity

Currentrootbranch `task/ACCTTEST-004-service-baseline`, draftPR49, lastpublished
fixturehead8df; root publishing gate/review/test-rolemetadata now. Inspect actual
HEAD/remote/CI before acting. Taskin_progress; testsource/firstred/implementation
NOTcomplete. Nativecommands done, no debugsession/newservicebody/marker evercalled.
Old authorAPI/source/logical/dirtyverificationtrees/validreviews preserved; now
125registered109existing16absent after new featuretesttree. Never reset/prune/discard.
Use followup_task forcompletedagents; do not assume priorrolesstillrun. Current
sourceauthor has separate cleanAPI/doctree ataba965; writes only NEWtesttree now.

No owner decision/access needed. All seven integrations/three FX, real authority/
policy/clock/action adapters, bothrenderers/locales/themes, financial/refund/review/
continuation workflows, security/recovery/backup/platform/packaging/live/fullrelease
remain mandatory/incomplete. A merged component PR is a checkpoint, not completion.
