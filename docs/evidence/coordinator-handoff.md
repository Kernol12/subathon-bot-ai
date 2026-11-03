# Coordinator checkpoint — corrected hold baseline awaiting fresh CI and valid red

Continue [standing coordination](../COORDINATOR_PROMPT.md) under AGENTS. Full
product is incomplete. No owner decision/access currently needed. Do not stop at
this PR; implementation016 and service004 follow their own gates.

## Main and preserved integrated work

Last observed main **aad1ad11dd2f826d11d81897319671aa10017b8c**; all ten postmerge
workflows successful, including SQL41607196319. Completed normal task merges:

- [PR43](https://github.com/Kernol12/subathon-bot-ai/pull/43), STORAGE-002:
 34ed520→8c7f71f,05:49:50Z. [Handoff](storage-002-handoff.md).
- [PR44](https://github.com/Kernol12/subathon-bot-ai/pull/44), ACCTCONTRACT-001:
 dd5e6f1→21c0871,06:23:15Z. [Handoff](acctcontract-001-handoff.md),
 [ADR0003](../decisions/0003-application-service-prerequisites.md).
- [PR45](https://github.com/Kernol12/subathon-bot-ai/pull/45), STORAGETEST-017:
 48b36a0→a40b42f,08:35:37Z. [Handoff](storagetest-017-handoff.md).
 Original first-red90f3c739 preserved; do not replay it as a new first red.
- [PR46](https://github.com/Kernol12/subathon-bot-ai/pull/46), STORAGE-017:
 8120c231→aad1ad1,08:57:48Z, all11 final/all10 postmain successful.
 [Handoff](storage-017-handoff.md), [source review](storage-017-code-review.md),
 [validation](storage-017-validation.md), [CI](storage-017-ci.md).
 Runtime834336d3/test0e4f4f00/fixturebea1273c/helper67d4 remain unchanged.
 The intermediate missing canonical acceptance metadata was corrected using
 already-passing evidence and fresh checks before merge; no tests/validator weakened.

## Active branch and actual incomplete execution

Root `task/STORAGETEST-016-durable-holds` from actual main before claim.
[Draft PR47](https://github.com/Kernol12/subathon-bot-ai/pull/47), last published
**af310e7d68dfe6c239719bcdf69569a06c4ba526**, all11 checks successful. Complete
source-specific SQL41637913237 succeeded12:42:01–12:44:17Z: fresh110/zero warnings/
GNU13.3/Boost108300/Qt6.12/SQLite3.53.4/new3 diagnostic5PASS0skip186ms/exact35
compiled/old stable. Raw SHA256c565c760b6935299e68d13d074f363a6495ca08e429d12c0e019a78645f3367c.

FIRST execution at SAME CLEAN af310e7,12:45:01.970–12:45:07.795Z, actual CTestexit8,
was NOT a valid complete intended red. Authoritative LastTest:35 summaries,
70 init/cleanup PASS,265 scenario FAIL,0skip;261 unique HOLD-MISSING plus FOUR
HOLD-ASSERT setup errors. Verbose/outputOnFailure repeats stdout, so the initial
recorder522 markers/70 summaries double-counted output and correctly rejected the
run. [Preserved interpretation/logs](storagetest-016-initial-attempt.md): raw
4c2dd57f6f6480f3c6bd21bf957e737daedd5de281a81180d61af49ea7907be0, LastTest
6d4398039fafa4bdf0823d058a3667f5685e440b785245cdc809e7909dfa19fa.
Do not claim complete observed_red or start production based on that attempt.

Two genuine setup defects affected3 timer kind1 rows and1 wrong-context row:
nonexistent operations.effect_count (actual column on results), and swapped
retained-context document/config positional values. Tests now join the actual
operation/results key and use named context columns; all assertions/counts remain.
Old261 reached markers and all valid prior reviews are preserved.

## Current corrected candidate and completed reviews

Current corrected source da588312→root45b3de6:
**0cce9545c3a3851b25df9131e6e35e97366aacee**. Final author3aa286→roote567141:
**1a65cc9e6f61a7c6a69f012a1b596c4165f89537**.
Actual separate correction approval743b87fe→root7607f8b accepts these Git objects,
native OLD-library Qt6.12/SQLite3.54 exact-query diagnostic, genuine source mismatch
and FK0; no feature replay. [Correction review](storagetest-016-setup-correction-review.md).
Root corrected compile3steps/zero warnings/exact35 registered/discovery only.
[Current CI evidence](storagetest-016-ci.md), [author](storagetest-016-author.md).
Current state: STORAGETEST-016 in_progress/baseline failed until valid revised red;
corrected test review passed; new complete corrected-head official Qt not_run.
STORAGE-016 planned. Source/header still six unconditional unimplemented stubs.

Preserved prerequisite approvals (do not repeat):

- Logical701c710f/c0ed4b6; physical83e96362/696d7f9. Step6 twelve statements/full89/
 F89890/SHA9f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a,
 every old prefix exact. [Logical](storagetest-016-contract-review.md),
 [physical](storagetest-016-physical-review.md).
- API originally421acf29/cfe443a8; current prose46c70297 adds ONLY18 category
 clarification lines, separately approved35b1092. Headerfe50fb10/CPP5b10c0e0 unchanged.
 [API](storagetest-016-api-review.md), [categories](storagetest-016-error-category-review.md).
 Own API-source SQL41623088230 at10258a11 passed BEFORE independent fixtures.
- Frozen JSON63de09c5/helper7ca946f2/environmenta17312fe, approvede624614+d66f183.
 Own foundation SQL41631527683 ata5e5998 passed BEFORE test authoring.
 [Fixture review](storagetest-016-fixture-review.md), [CI](storagetest-016-fixture-ci.md).
- Full test approvaladb0404+41aa858 pins complete25da01e and old81644120/fc167da2;
 remains valid for unchanged tests, with separately approved actual SQL revision
 above. [Full review](storagetest-016-review.md). Earlier genuine UPDATE-only trigger
 defect fixed with4 additive INSERT-trigger lines, all assertions preserved.
- Shared Qt CMake**ea1e3cabc0de800d05ee8c8aa8b08f116c233cd4**, workflow
 **b9529c189b33399bd895ca7fdfcee8dd57c1330a** unchanged through setup correction.
 35 enabled draft-red cases, global264=unchanged229+35; no WILL_FAIL/disabled/hiding.
 Workflow runs old stable suites and required new3 foundation, then discovery-only35.

## Roles and preservation

- Root owns shared state/HTML/maps/schema order/build/presets/workflow/integration.
 Local new initial-attempt logs/CI metadata/checkpoint are being committed before
 publishing a complete corrected candidate. No new feature execution since initial.
- `/root/bundle_validation`: feature/source correction author COMPLETE, clean
 `local-evidence/worktrees/storagetest016-feature-author`,
 `task/STORAGETEST-016-feature-author`, latest3aa286. Owns ONLY testCPP/authorMD.
 No feature body was run by author; native setup diagnostic uses old library only.
 Prior API tree/four config copies remain preserved.
- `/root/bundle_reviewer`: correction review COMPLETE743b87fe. Existing
 `local-evidence/worktrees/storagetest016-review`, `task/STORAGETEST-016-review`;
 six verification copies dirty (root/server CMake,presets,workflow,Qt CMake,API doc).
 All independent review commits preserved; no baseline replay by reviewer.
- `/root/bundle_implementer`: fixture author COMPLETE2508658, clean
 `local-evidence/worktrees/storagetest016-fixtures`, `task/STORAGETEST-016-fixtures`.

Last inventory116 registered worktrees/100 existing/16 absent, all preserved.
Older dirty verification trees storage002-review3, storagetest017-review4,
storage017-review5, storage017-validation6 remain untouched. No reset/prune/discard.
Native root corrected build/discovery completed; inspect actual processes/agents
on resume, never assume running. Use followup_task for completed agents.

## Next concrete actions

1. Publish the complete corrected candidate with new approval and preserved initial
 evidence. Observe its OWN selected official job/new3 green/exact35 compiled/all11;
 prior af310 CI does not satisfy the changed-source gate.
2. Keep that new HEAD/worktree CLEAN after CI. Save raw CI/rollup under ignored
 local-evidence; do not write tracked outcome metadata before execution. Record
 revised full `ctest --preset sqlite-hold-debug -L '^sqlite-hold$' --verbose` ONLY
 after actual corrected-source approval plus own CI. Preserve initial artifacts;
 use distinct REVISED paths. Parse canonical LastTest, not repeated stdout.
 Valid revised red requires35 failed slots/265 unique exact named missing rows,
 70 init-cleanup PASS/0skip/0HOLD-ASSERT/setup failures. No valid full red exists yet.
3. Retain exact head/timestamps/exit/log+LastTest hashes; request separate final
 attribution without replay. Update state/map/HTML/handoff and latest CI before
 normal exact-head merge. Recheck current rules, required approvals and threads.
4. Only then STORAGE-016 from integrated main, separate implementation/source review/
 acceptance/promoted ordinary CI. Service004 still waits its own reviewed/red gates.

No owner access or policy decision needed for these engineering corrections. Current
rules previously26170263 formal approval0; recheck actual rules before merge, manually
require all11 latest checks. All seven integrations/three FX, both renderers,
locales/themes/platforms/packaging/security/recovery/live/release remain incomplete.
