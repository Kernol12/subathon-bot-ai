# Coordinator checkpoint — STORAGETEST-016 minimal API authoring

Continue [standing coordination](../COORDINATOR_PROMPT.md) under AGENTS. Full
product is incomplete; all seven integrations/three FX, UI/renderers/locales/themes,
platforms/packaging/security/backup/live and release scope remain mandatory.
No owner decision/access currently needed. Inspect actual state on resumption.

## Actual integrated main and completed task PRs

Fetched/local main **aad1ad11dd2f826d11d81897319671aa10017b8c**.

- [PR43](https://github.com/Kernol12/subathon-bot-ai/pull/43), STORAGE002,
  exact34ed→8c7f71f05:49:50Z; all11final/all10postmainSUCCESS.
  [Handoff](storage-002-handoff.md), [review](storage-002-code-review.md), [CI](storage-002-ci.md).
- [PR44](https://github.com/Kernol12/subathon-bot-ai/pull/44), ACCTCONTRACT001
  DOCUMENTONLY, dd5e→21c087106:23:15Z; all11final/all10postmainSUCCESS.
  [Handoff](acctcontract-001-handoff.md), [ADR0003](../decisions/0003-application-service-prerequisites.md).
- [PR45](https://github.com/Kernol12/subathon-bot-ai/pull/45), STORAGETEST017
  testbaseline,48b36a0→a40b42f08:35:37Z; all11final/all10postmainSUCCESS.
  [Handoff](storagetest-017-handoff.md), [fullreviewf5c6](storagetest-017-review.md),
  [finala729+eb563](storagetest-017-final-review.md). Original sameclean-c24
  FIRST18/35marker red90f3c739 is preserved; never replay it as a new baseline.
- [PR46](https://github.com/Kernol12/subathon-bot-ai/pull/46), STORAGE017,
  exact8120c231b9bdb1b974d1335f9c56ea449f0f1cf6→aad1ad1 **08:57:48Z**.
  All11latestSUCCESS inclSQL41606867976/readyCLEANMERGEABLE/0threads/current
  rules26170263formalGHapprovals0/rootclean/criticalblobsunchanged before normal
  exact-head merge. No bypass/force/delete. Mainffclean/treeequals812.
  All10postmainaadSUCCESS observed inclSQL41607196319.
  [Handoff](storage-017-handoff.md), [sourceapproval11012da](storage-017-code-review.md),
  [validation](storage-017-validation.md), [CI](storage-017-ci.md).

Source017 runtime834336d3/header8614(values/APIunchanged except1statuscomment)/
helperb0ca, feature0e4f/18slots35scenarios, JSONbea127/helper67d4, oldschema66.
Root/reviewer/validator18/full228PASS0warnings/Qt skips;225Qt715PASS. Ownsource
official41605888462 at680SUCCESS/fresh100/GNU13.3/Boost108300/Qt6.12SQLite3.53.4/
new3fixture5PASS61ms/exact18/mandatory18PASS1.29/oldstable/all11. Initialf1
completion omitted canonical acceptance; render correctly failed but rootmistakenly
published before noticing. Corrected812 mapped ALREADY passing evidence, regenerated
HTML and passed freshall11 before merge; no validator/assertion/source weakening.
DB01/NUM04 remainpartial; hold16/service004 and full release remain incomplete.

## Active branch, PR and reviewed document gates

Root `task/STORAGETEST-016-durable-holds` created from actualaad BEFORE edits/claim.
[Draft PR47](https://github.com/Kernol12/subathon-bot-ai/pull/47). Last published
phase commit **e6b6900831b33d82b2d1de89493633a8d3cb221a**; current local
prepared build changes are described below. STORAGETEST016 in_progress/test_baseline;
STORAGE016 implementation planned. [Brief](../tasks/EPIC-009/STORAGE-016.md).

- Concrete [contract701c710f](../contracts/sqlite-application-holds-v1.md),413lines,
  authorc14f, actual [DOCreviewc0ed4b6](storagetest-016-contract-review.md)
  rootc3c5568 after five findings corrected0ad/root1c8ef. Global typedsubject,
  immutable cause/sharedrawmutationref/perholdversions/multiple independentholds,
  completeactive gate/history/CAS resolution/replacement,16literalHexamples.
  CapturedRevision versus FailedCapture handles conversion before a valid finalized
  int64 plan, with required alternative source/provenance and explicit unknowns.
  Service004 selected-plan protocol unchanged; producer/semanticcodec and mixed/
  postfinish/continuation policy remain separate owning-task gates.
- Physical [contract83e96362](../contracts/sqlite-application-holds-schema-v1.md),
 495lines, author082659a9, actual [physicalDOCreview696d7f9](storagetest-016-physical-review.md)
  root568b3cf after three findings fixedab0/root42b9. Step6ID
  durable-application-holds-v1,12exactstatements/10newtables+11NULLautoindexes+
 2queryindexes=23delta/full89. EVERYprefix1–5 payloadhex/hash/projection is exact.
  FinalF89890/SHA**9f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a**.
  Old972/e094/F89806 andearlier89396 are superseded proposal drafts.
  SQLNULLlinkguard, actualcounts before fullcollection allocation and H11kind0
  oldprocessedBesidealias fixed/reviewed. ScratchSQLite3.54 FK/CHECK/catalog/
  create-append-replace-resolve geometry only; NO port/native/fixture/red proof.

## Actual roles and owned paths

Root owns shared state/HTML/map, schemaorder/build/pins/workflows/integration.

- `/root/bundle_validation` ACTIVE minimal API author in NEW
  `local-evidence/worktrees/storagetest016-api-author`,
  `task/STORAGETEST-016-api-author`, fromjustverifiedcurrentmainaad, ffapprovede6.
  Own ONLY `docs/contracts/sqlite-application-holds-api-v1.md`,
  `server/include/subathon/server/sqlite_application_holds.hpp`,
  `server/src/sqlite_application_holds.cpp`, `docs/evidence/storagetest-016-api-author.md`.
  No actual candidate yet. Exact finite owned types/results/equality/errorpriority/
  limits/byte+item formulas in DOC/header; EVERYnewstore/catalog entrypoint must
  IMMEDIATELY UNCONDITIONALLY throw unique named marker. Constructor/copy inert.
  No validation/query/SQL/manifest data/persistence/cleanup implementation now.
- `/root/bundle_reviewer` API read-only prep COMPLETE, awaiting actual candidate,
  no verdict. Reuses clean `local-evidence/worktrees/storagetest016-review`,
  `task/STORAGETEST-016-review`; owns next ONLY
  `docs/evidence/storagetest-016-api-review.md`. Prior actualc0/696 preserved.
- `/root/bundle_implementer` physical author COMPLETE cleanab0, no currentwriting
  assignment. Tree `local-evidence/worktrees/storagetest016-physical-author`,
  `task/STORAGETEST-016-physical-author`. Preserve initialNULL-hole witness/raw
  diagnostics. Future independentfixture work ONLY after explicitassignment following
  actualAPI/configreview+OWNAPI selectedofficialstablegreen.

Read-only type/reader map COMPLETE: reuse ApplicationPlanKey/OperationKey/Envelope/
State/Subject/IDs/Exact. Current-state/current-plan getters cannot validate arbitrary
historical observations or selected-body+complete metadata-only planhistory. Future
same-session private SQL/decoder factoring needed; no inventedgetter, dummybundle,
owning legacyadoption or separate reader connection.

## Prepared UNCOMMITTED shared build changes / next actions

Root currently has FOUR prepared config files (no newAPI code present yet):
`CMakeLists.txt`, `server/CMakeLists.txt`, `CMakePresets.json`,
`.github/workflows/sqlite-writer.yml`. NewHOLDflag→ORIGINAL→BUNDLE; new static
subathon_sqlite_application_holds from newCPP linksoldbundle+QtCore; preset20
sqlite-hold-debug inherits19 with matchingbuild/test; workflow ONLYpreset/builddir
token substitution, alloldsteps/pins/guards/counts/checkname untouched.
`cmake --list-presets`20/project396/122/diffPASS. Configure/build20 NOTattempted
until actualAPIheader/CPP exists. Do not publish incomplete configs ahead of source.
Authors may copy these as uncommitted verification copies, never commit sharedfiles.

1. Integrate actual minimalAPI/header/APIDOC/namedstub candidate; verify no new
   behavior/SQL/global initializer and all old source/fixtures unchanged. Compile
   target/newprofile20 and exact228OLD discovery; NOnewholdentrypoint invocation.
2. Different actual full API/config/byteformula review, root/reviewer existing228
   stable checks and candidate's OWN selectedofficialoldstable CI before independent
   frozen newfoundation author. Existingdoc/017CI is not that future API proof.
3. Independent raw literalstep6/foundation uses oldwriter only, actualfullreview+
   OWNNEWselectedQtgreen before featuretestauthor. Then actualtestsource/fullreview/
   compiler/exactdiscovery/sameapprovedheadownselectedgreen→FIRSTnamedholdstubred.
   No missing dependency/compile/skip is red. Production16 waits integratedbaseline.
4. Service004 waits integrated16 and its OWNreviewed API/tests/red; don'taggregate
   228passes or prototypeSQL as hold/service acceptance. Keep focusedPR/latestCI/
   independentreviews/currentrules/discussions/normalexactmerge gates.

## Preservation and recovery

All previousbranches/worktrees/reviews preserved; no reset/prune/discard. Old dirty
verification trees: storage002-review3configs, storagetest017-review4configs,
storage017-review5configs, storage017-validation6importedcopies. They are not
unfinishedsource/test changes. New16review clean, physicalauthorclean. Last full
inventory112refs/96existing/16absent BEFOREphysical/API trees were added; inspect
actual inventory/processes/agents/PRstate on resumption. No oldsource/native process
assumed running. Usefollowup_task to resume a completedauthor after findings;
send_message alone does not trigger an idle turn.
Current mainrules26170263 formalGHapproval0/threadresolutiontrue/no requiredstatus
rule; coordinator explicitly checks latestall11. No owneraccess needed, liveprovider
effects or unsupported platform/release claim.

## Actual minimal API candidate compiled (no invocation)

APIac0610f importedb6b6d11: DOC421acf29012f31345cc41cbee239bcf95000ad62,
headerfe50fb10e6f5a7198df27358666734ad02fb47d1, CPP5b10c0e0b4e7057af4042ae510af851efcd24efa.
Author77cddef imported0f0e8f1/MD5217ec35. Sixmethods/factory immediately throw
unique marker; constructoronlystores actualrevocableticket/limits. No SQL/validators/
catalogdata/behavior/globalinitializer, no newholdcall ever. Rootfresh102build
ZEROwarnings/exact228OLD discovery/formatPASS; authorold228PASS8.00s0Qt skips.
ActualAPI/fullformula/configreviewACTIVE, ownMDawaited; authorCOMPLETE,
4sharedverificationcopiespreserveduncommittedinAPItree. Root nowfreezes4actual
configs5d76/b55b/053b/88f8 plusmetadata beforeownold228run+publicationCI. No
fixtures/tests/red yet. [API compiler evidence](storagetest-016-api-ci.md).

Actual rootold228 at CLEAN8dbde1e PASS7.90s/exit0/225Qt715PASS0fail0skip;
raw09be2439/LastTest16e765 preserved. No newholdbody/factory/marker invocation.
API reviewer actualfullaudit/nativeold228PASS7.88/no blockers reported; WRITTEN
approval notyetimported, ownAPIselectedofficialCI notyetobserved, fixturesunassigned.

## Latest actual API gate before independent fixtures

Actual [API/configapprovalcfe443a8](storagetest-016-api-review.md) imported8c665f2
accepts exact421/fe50/5b10/5217/5d76/b55b/053b/88f8. Ownreviewerfresh102/0warn/
old228PASS7.88/225Qt715PASS0skip/exactold228; rootold228PASS7.90 at8db. All
B/I/formulas/richliteral math verified, SIXfunctions stillunconditional namedstubs.
No holdentrypoint/factory/marker call. RootpublishingcompleteAPIcandidate for OWN
selectedofficialoldstableCI now; fixtureauthor MUSTWAIT actualsuccess. All earlier
awaitingreview paragraphs historical, APIapproval COMPLETE. Fourrootconfigs now
COMMITTED8db, rootclean beforemetadata. APIauthorCOMPLETE77cddef with4shared
verificationcopiespreserved, reviewerCOMPLETEcfe with4copiespreserved. Independent
fixtureauthor NOTassigned yet. No holdschemaimplementation/fixture/test/red.
