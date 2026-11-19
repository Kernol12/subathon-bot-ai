# Coordinator checkpoint

Owner: Kernol12. Issues own brief/status/prerequisites; PR comments own reviews.
Complete specification v0.45, all seven integrations and all three FX providers
remain release scope. This checkpoint grants no new authorization.

## Standing authorization — continuous coordination (2026-11-18)

The owner replaced the former “merge, then STOP” instruction. Continue through
both stages, reviews, observed intended red, implementation, checks, proof and
protected merge without routine permission pauses. Start SESSION-009 #582 now
on current main in a fresh session, tier A both stages. Stage1 includes both
owner criteria against the real API-001 host: ex-HL05 revocation/thread/reentrancy
and ex-HL15 UTCmin/UTCmax. Observed intended red precedes stage2.

After every protected merge, close the issue, batch the checkpoint into the next
task PR or one small docs PR, and claim the next dependency-ready product task
on the board. Prefer release-unblocking work, such as SESSION-003 toward API-010.
Apply the workflow tier; report each merge briefly and continue without awaiting
a reply. The original [#278 handover](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6930279142)
remains historical technical context, not a continuing stop directive.

Only owner-needed stops are: third fix round; contract change beyond the issue
or ADR-0012 assertion weakening; specification conflict or unclear requirement;
unclassifiable failure or product failure persisting after allowed fix rounds;
provider switch, new dependency, any process/tooling change; no dependency-ready
product task; or explicit owner stop. Ask with options and a recommendation,
while continuing independent work not blocked. Pre-build/test CI infrastructure
allows one recorded rerun; routine in-scope choices, self-caught mistakes and
filing planned issues are not stops. #619/#620 remain frozen.

**Standing test-binary admission (owner decisions 2026-11-18).** The existing
procedure applies to every authorized Qt or GoogleTest binary using its own
closure. First record passive ELF/ABI and dependency hashes for the exact reviewed
binary. Run the first real test with bounded loader and plugin logs, reconcile the
loaded objects and their hashes before crediting the result, then check hashes
again. Any mismatch voids the run. Changes to the procedure, tooling, modes or
native-supervisor debug variables require the owner under rule 4(e).

Apply log size caps only to captured log files, including admission logs.
Test processes and their children run without a process file-size limit.
The owner’s BUILD-002 diagnostic reproduced SIGXFSZ at a 1,048,576-byte
SQLite WAL under the inherited cap. The original
`example8_twoEvents` observation remains in the ledger as a harness fault.

Record `readelf` checks of both `.dynsym` and `.symtab` for Drogon
`native_profile` Session symbols. The consumer Session RTTI export check applies
if either table contains a defined or imported Session symbol. Retain all other
checks, including imports, versions, SONAMEs, selected SDK identity and exports,
the SDK's own Session RTTI and marker, hashes and loader reconciliation. External
compiler and system-header pre-build hashes are outside this procedure.

SESSION-009 #582 is Done after [protected merge #703](https://github.com/Kernol12/subathon-bot-ai/pull/703)
at `3f1d35b8`. MILESTONE-003 #232 is Done after
[protected merge #705](https://github.com/Kernol12/subathon-bot-ai/pull/705)
at `5cc1da42`. All seven checks passed on proof head `a7631860`. The final
jobs binary passed 31 cases. The earlier admitted run passed both catalog
controls and 343 affected regressions. The delivery catalog is available at
v11. Host startup still selects v10.

SESSION-011 #591 is Done after [protected merge #704](https://github.com/Kernol12/subathon-bot-ai/pull/704)
at `402056e0`. Stage 1 observed 68 intended NI outcomes and 41 passing
controls. Stage 2 passed all 109 unchanged cases and all six affected regression
selections on the admitted successor. The earlier 104/5 run and its unknown
disconnect wait phase stay separate. All seven checks passed on proof head
`4fd4647c`.

MILESTONETEST-004 #233 is Done after [protected merge #708](https://github.com/Kernol12/subathon-bot-ai/pull/708)
at `7ddcfc82`. On `052b40c4` the admitted baseline reached all nine intended NI
failures and all 86 unchanged controls passed. The single unchanged-source Qt
retry resolved APT setup timeout. All seven checks passed on proof `52493669`.
MILESTONE-004 #234 is Done after [protected merge #712](https://github.com/Kernol12/subathon-bot-ai/pull/712)
at `c92e042a`. On admitted `e7321be9`, all nine frozen timeline cases and
86 controls passed on four binaries. The new hosted timeline step and all
seven checks passed on proof head `a15f07b2`. Under owner revision 2, pause
gates new starts and releases while the active presentation and gap keep
time. Jobs and bonuses require release of their group. Unreleased jobs stay
queued across restart.

### Structure review of `199dc02` (owner decision 2026-11-18)

- Correct the obsolete CMake status banner and README description of application
  accounting and SQL durability in a separate documentation-tier PR, CI only.
  Keep [#707](https://github.com/Kernol12/subathon-bot-ai/pull/707) out of the
  SESSION-011 and MILESTONE-003 review packets and proofs.
- [BUILD-001 #706](https://github.com/Kernol12/subathon-bot-ai/issues/706) is
  Done after [protected merge #713](https://github.com/Kernol12/subathon-bot-ai/pull/713)
  at `e295538f`. The 46 configuration comparisons preserved registrations and
  build targets. All 505 measured cases matched the baseline after fresh
  admission of ten binaries. The development server linked without test
  dependencies, and scaffold linked without Qt. All seven checks passed on
  `8ea780a3`. The existing Qt job linked the tests-disabled development server
  with preinstalled GNU 14.2 and the existing Boost pins. #619/#620 stay frozen.
- [BUILD-002 #710](https://github.com/Kernol12/subathon-bot-ai/issues/710) is
  Done after [protected merge #711](https://github.com/Kernol12/subathon-bot-ai/pull/711)
  at `06241738`. GCC 14.2 reproduced both errors in the isolated functions and
  compiled both fixes. All 410 normal-compiler cases passed on six admitted
  binaries after the owner-authorized harness correction. All seven checks passed
  on `dfecefe9`. BUILD-001 includes this fix before its hosted GCC 14 build.
- [API-008 #292](https://github.com/Kernol12/subathon-bot-ai/issues/292) remains
  planned. Its brief now calls for one contribution operation shared by the dev
  CLI and HTTP host, by default in `subathon_reference_intake`. Keep the existing
  receipt-to-snapshot transaction boundaries, return a typed clock-reservation
  refusal, and test both callers. The CLI builds a synthetic event, calls the
  operation and prints its outcome.
- Move encode/decode code out of `application_service.cpp` or
  `sqlite_application_bundle_access.hpp` only within an authorized task that
  already changes the respective file. Use a separate commit and name it in
  that task's review packet.

## Handover (2026-11-18)

The standing authorization continues for the successor session. This session
leaves the next task unclaimed. Follow the board and issue prerequisites for
current readiness.

- Recommend [MILESTONE-009 #709](https://github.com/Kernol12/subathon-bot-ai/issues/709)
  next. #234 satisfies its prerequisite. It owns durable group order and
  ordinary restart recovery without blind replay. #236 still waits on #709,
  and #294 owns running host integration.
- Other dependency-ready product work on the board includes #177, #197, #199,
  #204, #214, #230, #257, #270, #286 and #406. #286 is a prerequisite for #294.
  #619/#620 remain frozen and #292 remains planned.
- Keep the local evidence under `build/MILESTONE-004/`,
  `build/MILESTONETEST-004/intended-red/`, `build/BUILD-001/coordinator/` and
  `build/BUILD-002/`. Keep the SESSION-011 runtime and regression packets under
  `build/SESSION-011/` and the sealed successor SDK and consumers in
  `~/.t3/worktrees/subathon-bot-ai/t3-0d9c03aa`. Shared dependencies remain in
  `build/deps/`, and the proof validator's schema environment is
  `build/workflow005-schema-venv/`.
- No worktree has active edits for this handover. The clean worktrees at
  `build/BUILD-001/coordinator/worktree` and
  `build/MILESTONETEST-004/coordinator/registration-worktree`, and the
  MILESTONE-004 author worktree `t3-1b6d3578`, can be removed after checking
  ignored files. Keep the baseline and BUILD-001 author worktrees
  `t3-d7d4147d` and `t3-ea41127e` until their ignored build artifacts are
  archived. No owner decision or test run remains pending on this handover.

## Coordination after MILESTONE-009 (2026-11-19)

- MILESTONE-009 #709 is Done after protected merge `9219560c` in
  [#716](https://github.com/Kernol12/subathon-bot-ai/pull/716#issuecomment-7001909142).
  All seven checks passed on proof head `db17f85b`. The admitted Stage 1 run
  observed 16 named NI outcomes, one expected dispatch assertion and one passing
  control. Stage 2 passed 18 restart invocations and 168 regressions across nine
  admitted runners. Live host composition and v12 activation remain #294.
- CORE-004 #177 is Done after protected merge `646fb857` in
  [#717](https://github.com/Kernol12/subathon-bot-ai/pull/717#issuecomment-7002792033).
  All seven checks passed on `cee31651`. Independent review approved and
  25 frozen tests plus 37 regressions passed on separately admitted binaries.
  Owner 10A retained the forward compound-gap fallback. Configured timezone
  integration and data applicability remain #720.
- MODIFIERTEST-001 #214 is Done after protected merge `51d4c5d7` in
  [#718](https://github.com/Kernol12/subathon-bot-ai/pull/718#issuecomment-7003577969).
  All 48 frozen cases reached intended NI with zero skips. The exact admitted
  runner and receipts were retained, and all seven checks passed on `63ec75e4`.
  This is baseline evidence. #215 owns implementation together with #719's
  SQLite baseline.
- MILESTONE-005 #236 is Done after protected merge `400a5c51` in
  [#721](https://github.com/Kernol12/subathon-bot-ai/pull/721#issuecomment-7006091072).
  All seven checks passed on proof head `189ab104`. SQLite v13 preserves the
  v12 prefix and adds exact paused choices, refund holds and terminal Skip.
  All 39 decisions and 378 regressions passed across nine admitted runners.
  Running-host composition remains #294. #719 now has its catalog prerequisite.
  Owner 12B–16A remain
  [recorded](https://github.com/Kernol12/subathon-bot-ai/pull/721#issuecomment-7002464326).
  Owner [17A/18A/19A](https://github.com/Kernol12/subathon-bot-ai/pull/721#issuecomment-7003511184)
  approved numbered timeline correction 1, refund policy on a new downward
  crossing, and terminal Skip with each remaining hold recorded as ended by
  that Skip. Its additive catalog follows v12 and precedes #719.
- #236 corrected one fixture under owner decision 23A. The
  [first run](https://github.com/Kernol12/subathon-bot-ai/pull/721#issuecomment-7004391307)
  had one unexpected fixture stop. The [reviewed rerun](https://github.com/Kernol12/subathon-bot-ai/pull/721#issuecomment-7005001088)
  reached the repaired case's named `observe_refund` NI and all 39 intended
  stops, with zero setup failures or skips.
- The owner stopped #715 after its [one hosted experiment](https://github.com/Kernol12/subathon-bot-ai/issues/715#issuecomment-7004781214).
  The root canary survived the direct `sudo` timeout. The draft #722 was
  closed and #715 returned to Planned, the board's backlog status. It uses
  no parking slot. The retry change was never enabled.
- API-005 #286 is Done after protected merge `80a0bee1` in
  [#723](https://github.com/Kernol12/subathon-bot-ai/pull/723#issuecomment-7007029408).
  The registered suite reached 19 intended NI outcomes and passed all 19
  frozen synthetic cases after implementation. All seven checks passed on
  proof head `00f73263`. AUTH-15 and INTEGRATION-01 remain partial. Actual
  provider authorization, credentials and client onboarding remain separate.
- RATES-007 #406 is Done after protected merge `5cfe1959` in
  [#724](https://github.com/Kernol12/subathon-bot-ai/pull/724). All seven
  required checks passed on proof head `9821f48e`. The admitted core run passed
  24 override and 86 companion cases. Permissioned save and durable revisions
  remain [#727](https://github.com/Kernol12/subathon-bot-ai/issues/727), queued
  after #719 and dependent on #406.
- MODIFIERTEST-007 #719 is Done after protected merge `a15fca82` in
  [#725](https://github.com/Kernol12/subathon-bot-ai/pull/725). The exact
  [admitted run](https://github.com/Kernol12/subathon-bot-ai/pull/725#issuecomment-7010490021)
  had 19 named NI stops, three expected v14 compatibility failures and three
  passing controls. All seven required checks passed on proof head `5e100683`.
  The suite remains intentional red. #215 owns the additive v14 implementation.
- #199 is active on protected `a15fca82`. Its [owner rulings](https://github.com/Kernol12/subathon-bot-ai/pull/726#issuecomment-7008905863)
  cover active mixed-refund timer holds, the explicit Finished disposition,
  and Main-first reversal allocation after rounding cumulative entitlement.
  The [standalone core runner](https://github.com/Kernol12/subathon-bot-ai/pull/726#issuecomment-7011317503)
  reached the expected NI in all 15 methods. The registered run and separate
  writer/service review remain pending.

### Parallel work and standing CI rerun rule

- At most two active tasks in different modules and three parked tasks. When
  three are parked, finish the active tasks, then stop. Answered parked tasks
  get the next free slot before unclaimed tasks. Start nothing that depends
  on an unanswered question or touches files of an active or parked task.
  The coordinator owns shared CMake, CI, acceptance map and checkpoint.
- Serialize local test execution and exact-binary admission. Builds and
  reviews may overlap. At a provider limit, pause and retry the same provider
  on the next active turn. If two tasks park for the same cause, stop new
  claims and report it. The APT setup retry exception below is
  excluded from the same-cause parking rule.
- Main CI stops new work on a build or test failure. A failure in
  **Prepare authenticated Ubuntu package acquisition** before build or test
  does not stop new work. The authorized [main rerun](https://github.com/Kernol12/subathon-bot-ai/pull/714#issuecomment-6993941079)
  passed after the original APT timeout on `c8674545`.
- Until the owner reopens [#715](https://github.com/Kernol12/subathon-bot-ai/issues/715),
  a failure in that exact setup step may be rerun up to twice on fresh
  runners (three attempts total). Record each rerun on the PR. Park after
  the third setup failure.

## Authorized path — complete

1. **API-016 [#616](https://github.com/Kernol12/subathon-bot-ai/issues/616)**:
   Done, PR #664 / `4184c9fc`, proof `progress/proofs/API-016.json`.
2. **APITEST-015 [#613](https://github.com/Kernol12/subathon-bot-ai/issues/613)**:
   Done, PR #667 / `60d7730d`, proof `c8762280`. All 87 Qt selectors and 30 native
   methods reached intended NI under their approved admissions. This baseline
   delivered no host implementation. **APITEST-017 [#672](https://github.com/Kernol12/subathon-bot-ai/issues/672)**:
   Done, PR #690 / `2cd29763`, proof `8429246c`. Native-only historical/current/
   frozen gates, source reviews and latest-head CI passed. HTTP scope remains #689.
3. **API-001 [#278](https://github.com/Kernol12/subathon-bot-ai/issues/278)**:
   Done, PR #700 / `85b04da5`, proof `dd49a921`. Fresh implementation session on
   `2cd29763`; no PR #121 code, approval or CI carried over. Host and transport
   acceptance passed; all seven final proof-head checks passed before protected
   merge. The issue is closed as completed and the board is Done.

API-001 and its checkpoint #701 (`199dc020`) are merged. The owner has now
extended authorization to SESSION-009 and continuous dependency-ready product
selection above. No baseline assertion or release requirement is waived.

## Completed SESSION-009 gates

- Tests recorded 31 Host and 59 Linux intended NI outcomes and two passing
  6000 ms controls across admitted revisions. The original “A ended” result
  stays unclassified and uncredited. The approved second fix changed only four
  recorder wrappers in the test. The implementation used zero fix rounds.
- Independent Stage 2 acceptance passed all 248 cases (33 Host clock, 59 Linux
  clock, 87 Qt Host, 30 native Host, 39 session-resolution) with zero failures
  or skips. Both ex-HL05 and ex-HL15 owner criteria passed. The five binaries
  had independent admissions and matching postseals. The local Linux smoke
  permits unavailable and did not report initialization or acquisition status.
  Supported-platform clock trust, recovery, restore and deployment remain separate.
- Proof `7db535c9` binds both stages. All seven checks passed at that head after
  one unchanged-source Qt/native retry for pre-build APT HTTP 429. The existing
  native job built the two clock consumers and ran its existing gated regressions.
  The two label removals permit ordinary local acceptance. Hosted clock execution
  remains outside the existing job. S9-IR-01 and S9-IR-02 remain non-blocking
  source findings. Admission, commands, raw reports and proof readbacks are
  retained under `build/SESSION-009/` and linked from PR #703.

## API-001 completed gates and limits

- Reviewed source `5544c436`: initial Claude changes requested B1–B3; first-party
  fix round 1 of 2 resolved them with whole-class N1–N5 dispositions. A distinct
  Claude initial reviewed the approved SDK addition; combined
  [Codex verification](https://github.com/Kernol12/subathon-bot-ai/pull/700#issuecomment-6963388379)
  and [fresh Claude closing](https://github.com/Kernol12/subathon-bot-ai/pull/700#issuecomment-6963524189)
  approved the final composition. Corrections 1–11 remain; correction 12 is only
  the approved before-bind no-sharing sentence. No baseline assertion changed.
- Fresh additive SDK seal `c4d6782f3601c590c7c1071f6d59c644779f479857617e43cb49b48e86bc47b4`.
  Existing patch 0004 selects Linux bounded `reUseAddr=true`, `reUsePort=false` BEFORE
  bind; late readback remains. Ordinary Linux/nonLinux behavior, the candidate4
  regression lane and the existing recipe stay unchanged. Old `0b489` SDK and all
  old producer/evidence bytes remain unchanged.
- [Independent host acceptance](https://github.com/Kernol12/subathon-bot-ai/pull/700#issuecomment-6963845337):
  both fresh passive ELF/ABI/plugin and full host admissions passed. L07 all three
  subtests asserted exact exit 1; Qt 87/87 and native 30/30 passed. Actual Qt 33
  loader objects and native 31/33/33 maps matched; serving was checked by the
  unchanged supervisor. No native debug variables or new capture/tooling mode.
- [New-SDK regressions](https://github.com/Kernol12/subathon-bot-ai/pull/700#issuecomment-6964305515):
  freshly admitted foundation37 ×20 = 740 method runs +20 actual LegacyControls,
  including frozen34 ×20; native44 ×20 = 880 method runs. Zero failures/errors/skips.
  N5 had 20 equal-epoch witnesses in 20 attempts, zero misses. No product retry,
  fixture/assertion change or new loaded/historical experiment. All postseals
  matched; own workers reaped. Current passes do not prove the old faulty branch.
- Single proof commit `dd49a921` preserves imported PR121 history without reusing
  it as current evidence. Its tests projection binds six unchanged APITEST-015
  files; current implementation review binds all 14 paths, including SDK/CI.
  Complete publication, historical-policy, repository and real schema checks
  passed. [Final run 44201858136](https://github.com/Kernol12/subathon-bot-ai/actions/runs/44201858136)
  passed all seven checks on that proof head; no unresolved review threads or
  branch-protection bypass. Protected merge `85b04da5` was read back.
- Original L07 branch (shared/served versus listen failure/teardown) stays
  **UNKNOWN**. Earlier 5645 APT-before-build and 8213/a2ad/5152 missing-header
  compile/profile failures remain recorded; none was rerun away. New source
  fixes the host SDK selection. Source-only nonblocking SDK-N1 stale step name,
  SDK-O1 setter/readback observation and API1-C-O1 simultaneous copied-lease
  destruction concern stay in the receipts; no runtime witness or new fix scope.
- Evidence is bounded synthetic Linux development and required Ubuntu CI, not
  full deployment, live providers, security/performance or release verification.
  Raw reports, binaries, producer/admission receipts and seals remain under
  ignored `build/API-001/`; the proof/PR own the authoritative review record.

## Completed baseline evidence

WORKFLOW-008 (#698), PR #699 / `b8e30e86`, resolved the provider-proof gate.
A supported provider differing from a phase default requires an
`owner_authorization` HTTPS URL; without it rejection remains. Actual temporary
Codex initial receipts on #667/#690 retain their owner links and stay valid;
completed reviews are not redone. #619/#620 remain frozen.

APITEST-017 historical `0e774a51` ten NEW peer16 PRODUCT failures satisfy the
owner-corrected attribution; original fa9 trial01 remains uncredited. Exact
`8fbc0ee8` with its own SDK exposed both 1/9-peer suspend/admit failures. Prior
current N1–N5 each passed 20 unloaded/20 loaded runs, and frozen34/native44 each passed 20 all passed.
Proof `8429246c` and PR #690 retain raw limits, setup misses and the one recorded
pre-build APT rerun. Unknown legacy exit90 remains planned, unauthorized #692;
HTTP/f8be1aec scope stays planned, unauthorized #689. Neither blocks API-001.

Board: #278, #613, #672 and #698 are Done. API-001 closure made SESSION-009 #582
Ready because all its native blockers are closed; all other open native dependents
retain an open blocker. **Ready does not authorize work.** Epics keep their status.

## Product queue and frozen process work

- SESSION-011 #591 and MILESTONE-003 #232 are in progress. SESSION-013 [#662](https://github.com/Kernol12/subathon-bot-ai/issues/662) and API-010 [#560](https://github.com/Kernol12/subathon-bot-ai/issues/560) remain unclaimed queue items.
- SESSION-009 closure left #253, #262, #326 and #585 with open prerequisites.
  SESSION-003 #264 still waits on #262, #588 and #593. SESSION-011 precedes #588.
- APITEST-018 [#689](https://github.com/Kernol12/subathon-bot-ai/issues/689): large buffered HTTP input with more than eight connections and its
  bounded observation seam, moved from APITEST-017 by the owner. Planned under
  EPIC-008; not an API-001 prerequisite. Historical `f8be1aec` coverage is deferred here.
- APITEST-019 [#692](https://github.com/Kernol12/subathon-bot-ai/issues/692): unexplained unloaded legacy-control exit90/SDK diagnostic. Planned under
  EPIC-008, not authorized; not a prerequisite of APITEST-017 or API-001.
- API-016 follow-ups: V2, C2, C4, C6, I14, I15, I16, I19, I20, O5; see the handover
  comment. Filed as [#678–#687](https://github.com/Kernol12/subathon-bot-ai/issues/273)
  (API-017 through API-026), planned under EPIC-008; none is authorized for work.
- API-027 [#702](https://github.com/Kernol12/subathon-bot-ai/issues/702): planned under EPIC-008, native parent link recorded. Owns API1-C-O1 simultaneous copied-response-lease destruction from [the #700 closing receipt](https://github.com/Kernol12/subathon-bot-ai/pull/700#issuecomment-6963524189); source-only finding, no runtime reproduction claimed. Schedule like other product tasks.
- Process/tooling #619/#620 is frozen; lifting the freeze is the owner's decision.

The standing authorization permits selection of dependency-ready product work;
existing task-specific holds and the process/tooling freeze remain. No live chat or
payment side effects are authorized. Keep credentials and supporter data out of
source, fixtures, reports and HTML; synthetic or redacted fixtures only.

## ADR-0012 decision for APITEST-015 (2026-11-16)

**Option 1.** The host baseline is revised so that the host runs the path that
already runs in `subathon-dev-server`. Groups A to E are approved exactly as
[itemized](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6907983078) ([decision](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6908067929)). Nothing beyond that list may be removed or replaced.
If a review changes an item of the list, bring only that item to the owner.

Owner amendments, all recorded on #278:

- [O1/O3](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6913620654): the epoch fence is named in two parts: the comparison in
  `resolve_current_account_session` (`server/src/session_resolution.cpp`) for units
  that authenticate, and `current()`/`accepted()` in
  `server/src/sqlite_account_sessions.cpp` for units that insert or replace a session.
  The host has no generation fence. `local-login-authority-v1.md`,
  `session-lifecycle-v1.md` and `guarded-enrollment-recovery-v1.md` get pointer-only
  changes.
- [Two changed list terms and the §5 pointer](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6914837587) (`known_rollback_preserves_source_and_generation`,
  `busy_begin_is_actual_failure`; pointer-only change in §5 of the listener contract).
- [Reply rule, final form](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6916926039): the host withholds a reply whenever a unit's outcome is
  unknown or something was committed without ending normally; otherwise the
  dispatcher may reply. A rollback the writer performs after an error counts as a
  known rollback. A unit body that throws before anything is committed may be
  answered. Reference units report refusals as rollbacks, and those must be answered.
- API-010 (#560), the first replying dispatcher, carries the criterion "a command with
  a unit that ended without a commit never yields a success reply" ([widened](https://github.com/Kernol12/subathon-bot-ai/issues/613#issuecomment-6919654379)).
  #560 and the matching reviewed, published APITEST-015 boundary sentence are updated.
- #278 acceptance criterion: SIGTERM leads to an orderly stop; the host installs its
  own handler, because Drogon's default one terminates the process.

Owner conditions met before any removal: C, #662 (SESSION-013) exists; E, both clock
guards are acceptance criteria of SESSION-009 (#582).

APITEST-015 step-1 instruction (still binding for the rest of the task): no authored
tests, stubs or header changes until API-016 is merged (now met). The gates for the
next step are on [#613](https://github.com/Kernol12/subathon-bot-ai/issues/613#issuecomment-6917314751) and in the handover comment.

## Standing owner rules for this path

**Contract corrections.** Every correction to a task's contract is a numbered item in
the PR description and is named explicitly in each tier A review packet. It goes to
the owner first if it adds more than one SDK function or touches a first-party header
([rule](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6909553282)). Where no contract sentence governs a behaviour that a fix must decide,
bring the owner a numbered correction before the fix. A recorded reason for a
correction states the design reason; do not claim "matches earlier behaviour" without
measuring it.

**Rulings cover one case.** A coordinator or owner ruling applies to the case at
hand; it grants no class of future corrections or permissions.

**Class audit** ([rule](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6912622673)). When a review finding is one case of a class of defect,
the author audits the whole change for that class in the same fix round and reports
the sites checked. A later finding of the same class is a non-blocking follow-up, not
a new round. Put this rule into every review packet and fix assignment.

**Failed CI runs and re-runs** ([rule](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6921048499)). An author's handback lists every failed
CI run on its branch with the cause. No author re-runs a failed required check
without the coordinator's say. The review packets name those runs. Never merge on a
green re-run of a failed check; find the cause first. A change to a frozen test is
the owner's decision. (Coordinator practice, not an owner rule: a setup failure whose
cause lies outside the change, such as an apt mirror timeout on the checkpoint PR
#674, was re-run by the coordinator after recording the cause on the PR.)

**Fix rounds (owner clarification, 2026-11-18).** Each separately reviewed revision,
including a coordinator integration revision, has its own two fix rounds. After a
fix round, verify the changed commit and the dispositions of the findings. Bring
new defects found after a fix round on that same revision to the owner before
folding them into the round. Findings from the first review of a new revision use
the new revision's fix rounds. Ask the owner before a third round. The other stop
rules continue to apply.

**C++ compile gate (owner decision, 2026-11-18).** Compile each C++ source or
test revision before sending it for review. This gate is build-only, with no
test or product execution. Report compile failures to the author as ordinary
findings. They earn no intended-red credit and do not consume a fix round.

**Stop rule of a last pass** (API-016 third pass, [decision](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6924951572)). When the owner
grants a third and last author pass: the author first lists what the fix must cover
and the correction is limited to that list; an independent investigator then repeats
the whole measurement; any further regression means no new author pass, and the
coordinator brings the owner the fallback options instead. The verification and
closing re-check cover everything since the last approved head and name every defect
the measurement found that no frozen test covers.

**Measurement before merge** (API-016 fix round 2, [decision](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6921048499)). A defect that appears only intermittently in CI is
investigated, not re-run away. After a transport fix the investigator measures the
fixed build with the same method and sample size as the investigation, and the
unmodified frozen test runs at least 20 times locally; a non-zero failure rate stops
the task and goes to the owner.

**Provider flags.** If a Codex author run is flagged for cybersecurity risk, save the
uncommitted work as a patch and ask the owner; do not retry or switch providers on
your own. If a Codex verification is flagged, stop and bring it to the owner.

**Roles and providers — default restored (2026-11-17).**
The owner revoked the temporary 2026-11-16 split. Codex authors; Claude provides
initial and closing reviews for tier A and reviews for tier B; Codex verifies.
Investigations and measurements stay with Codex. Completed or running reviews
under the temporary rule remain valid and are not redone. The next new review
uses the default. Every review/investigation is a fresh session that did not
author or fix the work under review. If the required reviewer is unavailable,
leave the gate pending and continue other authorized work; do not substitute.
A provider flag stops for owner. Record actual provider/model in every review
comment; an author never approves its work. Any other provider or model switch
needs the owner. Other standing rules remain unchanged.

**Codex model — owner decision (2026-11-18).** The coordinator and every new
Codex child use `gpt-6.2-sol`, including authors, investigators and verifiers.
The provider stays Codex, so this is within the standing authorization. Existing
running sessions, completed reviews and receipts retain their original models.

## Project board maintenance (2026-11-16)

The "Subathon bot ai development" board follows the issues' native "blocked by"
links. The enabled "Item closed → Status Done" workflow sets Done on close for
issues and pull requests. The coordinator archives board items closed as not
planned without changing the issue, sets Status when claiming or blocking an issue,
and moves issues to Ready when a close unblocks them.

Open non-epic issues are Ready when every native blocker is closed, otherwise
Planned; claimed issues are In progress and issues awaiting the owner are Blocked.
The owner's API-016 follow-ups are explicitly Planned and not authorized for work.
Epics keep their status. Board Ready shows dependency order; it does not authorize
work. The checkpoint's authorized path does.

## Working rules from the owner (2026-11-15)

- `/tmp` is RAM-backed on this machine. Sessions put build directories on disk, in an
  ignored directory of their worktree (`build/TASK-ID/<session>/`) with `TMPDIR` set
  there, and follow the "Temporary files" section of the workflow. Name that
  directory in every assignment and review packet.
- Before the first tests review the author runs only the cases that call a
  not-implemented stub and reports the fixture check in the handoff; it is never red
  evidence. Unreviewed test source stays local.
- Tier A proof record: `progress/proofs/TASK-ID.json` (and its policy entry if the task
  is not enrolled yet), in one commit after the final closing approval, immediately
  before merge. `tools/check_publication.py` requires `origin/main` as an ancestor of
  the head, so merge `main` into the branch before the proof commit.

## Bounded owner authorizations (2026-11-17)

- The coordinator is the sole, serial writer of `.github/workflows/ci.yml` for
  APITEST-017 and APITEST-015. Keep separate commits on their task branches and
  independent review before publication. Reviewed foundation registration is
  37 methods / 297 maximum children / 10158s after N5 correction 1; host registration
  is 87 Qt selectors / 72 native children / 1788s. The workflow edits are separate
  commits on their task branches. Other shared-file conflicts still stop for owner.
- APITEST-015 correction 11 is the THIRD AND LAST contract pass: only the
  phase/thread of `command_admitted` as approved on #613. It is synchronous on Qt
  after retained admission, before entry checks/execution/writer scope, without a
  host lock; a bounded pause leaves IO ingress independent. Any further contract
  gap goes to the owner with fallback options, not a fourth pass. Fresh changed-part
  Codex verification and Claude closing remain required; NI is not runtime proof.
- The owner accepts the APITEST-017 unloaded sample (0/20) as bounded
  non-reproduction. The earlier exit90/SDK diagnostic stays unexplained: one
  failure in 21 unloaded runs across the original and amended consumers. Keep
  the reviewed logging amendment permanently in the frozen foundation test.
  Planned, unauthorized #692 owns that failure; it blocks neither #672 nor API-001.
- The diagnosis-only stage is complete: 20 loaded N5 attempts passed, with no
  acceptance credit; the separate fresh read-only answer was **no deterministic
  overlap using existing gates without timing/load** ([combined report](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6940815717)). The owner then
  resumed acceptance. The later `343414cb` loaded repetition 17 missed its witness
  with `(10,1,0,True)`; the original loaded repetition 12 tuple remains unknown.
  Both are recorded setup misses, not red; do not backfill the earlier tuple/cause.
- **APITEST-017 correction1:** at most three TOTAL fresh-child setup attempts;
  product bodies are never retried. Final optionA permits only a received,
  well-formed unequal-epoch witness to print "scenario not established", pass
  normal stop/reap0/empty-stderr/maps/seals/canaries, then try a fresh child.
  All three misses are a FIXTURE failure; every other failure propagates.
  This supersedes the old9+1 pending101 tuple criterion, whose evidence is retained.
  C017-V1 cleanup-loss finding was resolved and reviewed at `d9875341`
  ([closing](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6943014022)).
- Loaded sampling restarts from the beginning after an approved amendment and
  fresh admission. **No builds or other heavy work anywhere on the machine during
  loaded sampling**; earlier concurrent CPU2/3 build permission is superseded.
  Keep inner setup attempts/misses separate from established product outcomes.
  Any product assertion or exhausted-witness failure stops for owner; no outer retry.
- Historical gates are `0e774a51` and, for suspend/admit, `8fbc0ee8`, as #672 states.
  The owner's earlier `f8be1aec` wording was a copied mistake; it belongs to #689.
  Existing Git/CMake construction is allowed. Anything beyond it (script, route or
  new tooling) comes to owner with options **before authoring**.
- The control-only launcher is ignored evidence, never committed and unused by
  CI. Retain its hash-bound bytes outside #690 to preserve the completed sample's
  evidence. From now on, ANY new tooling, launcher or harness goes to the owner
  BEFORE authoring, even when it serves an approved step.
- The owner authorized the completed fresh N5 scenario/sampling investigation
  and unchanged 20-run historical diagnosis in parallel, with paired delivery.
  These diagnostic results do not resume acceptance or authorize the proposed
  sampled gate. The APITEST-015 existing-facility assessment was read-only; its
  result does not authorize a new capture procedure or identity-only mode.
- **APITEST-017 correction2 / optionA:** existing `boundedEpoch()` only, 16
  established native peers without binary prewarming, seventeenth Upgrade and
  ordinary accept in the same active batch. Active callback origins and same IO0
  supplement equal epochs; one frame each, new peer16 first. No SDK/producer
  change was needed. Foundation pass cannot prove the old faulty branch occurred.
  The historical gate is ten `0e` runs with at least one attributable PRODUCT
  failure, not the rejected 60-sample proposal. Setup max3/product-never-retry holds.
- For APITEST-015, first-real-run retrospective admission was an explicit owner
  exception to prior pre-run receipt/maps sequencing. The owner later authorized
  full Qt selection with logs and full native selection with reused exact first
  admission, unchanged supervisor, no debug variables and before/after seals.
  Any object/hash mismatch would void that run as admission failure, never NI red.
  No staged external capture or identity-only mode is authorized by that exception.
- **APITEST-017 correction3:** optionA's sole source fix1/1 adds only native
  message failure peer/checks-passed diagnostics. Claude diff review approved
  `0059feaf`; no assertion/setup/predicate/timeout/retry change. No source fix
  remains. Original fa9 trial01 stays unattributed/uncredited/no retry.
- **APITEST-017 correction4:** the owner corrected the old peers0–15 criterion:
  NEW peer16 is #616 reproduction2's target. The ten new historical peer16 PRODUCT
  failures satisfy the gate10/10; raw evidence and old reports stay unchanged.
  Continue remaining authorized gates. Any peer16 failure on the current foundation
  is a fixture defect: stop/report, no fix round. All current samples passed.
- Batch checkpoint updates into one documentation PR per stage, not per decision.

## Completed API-001 owner exceptions (2026-11-17/18)

These approvals covered API-001 only, not a general process or tooling change:

- The owner authorized the complete API-001 sequence with the default provider
  split, then one checkpoint PR and stop. Its six stop conditions were third fix
  round, beyond-issue contract/frozen assertion change, specification conflict,
  unclassified or persistent product failure after rounds, provider/tooling/new
  process change, and explicit owner stop. Pre-result own command/path mistakes
  were correctable; pre-build/test CI infrastructure could be recorded and rerun
  once unchanged. These did not permit rerunning product failures away.
- [Rebuilt Qt admission](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6960584143):
  full passive closure/hashes and the first real HL06 with existing loader/plugin
  logs plus reconciliation for EVERY binary with recorded results, including red,
  acceptance, reviewer-relied runs and proof. Intermediate author runs with no
  recorded result needed passive checks only. Any mismatch voids the run, never
  red/pass. No new mode/harness or native supervisor debug variables were approved.
- [Bounded L07 SDK scope](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6962700781):
  existing0004 before-bind Linux constructor selection, manifest hash, ONE listener
  contract sentence and host CI selection of a fresh additive successor, keeping
  candidate4 unchanged. Existing recipe only; no old0b489 mutation, new API/wrapper/
  seam, port probe or broader contract pass. Tier A review, fresh admissions,
  all three L07 exact-exit-1 cases, full Qt87/native30/foundation37/native44 and standing
  repetitions, currentCI/proof/protectedmerge all passed. This bounded tooling
  freeze exception does not lift the freeze for another task.

## What SLICE-001 delivered

`subathon-dev-server` (`server/dev/`, suite `dev-server` in the `qt` job) starts the
Qt loop, opens the single writer, advances the catalog to version 10, initializes one
timer and applies synthetic contributions through API-011's reference path. The
contract is on #642 (revisions 1 to 4). Owner decisions of 2026-11-15: development
session clock, development provisioning and `--controlled-clock` live only in that
executable; `encode_reference_execution_ref` (SRF1) is public in the server library.

It is a development walking skeleton. Limits: no login or session renewal (the
development session lasts 24 hours), no restart of a live clock anchor (an existing
database is read-only), no HTTP, no production host. SESSION-009, API-010, setup and
API-001 keep their ownership and release requirements.

## Completed sessions and retained evidence

The former API-001 sessions are finished. Six finished clean API-001 review/investigation worktrees and their
merged-base branches were removed without force; two private investigator reports
were hash-checked against their retained mainroot copies before removal.

Retain author `t3-a365ff53` / `t3/api001-host-lifecycle`, source/proof `dd49a921`,
and its SDK/source/evidence refs. The protected squash merge does not make those
original commit IDs ancestors of main; do not force-delete evidence-bearing refs.
Retain APITEST-015 `t3-0dc4e70e`, APITEST-017 `t3-aa9893a6`, historical
`t3-415a156d`/`0e` and `t3-24f198e1`/`8f`, and other original SDK/runtime artifacts
until a separate safe evidence handoff. Earlier clean baseline reviewers were
already removed as recorded in their merge receipts. No retained tree is a claim
or permission to resume work.

APITEST-015 final owner corrections 1–6 and 7–11 remain binding: execute may prequeue
units; body forward/finish refuses and ends without reply; forwarding is only from
execute or supplied completion; payload borrows charged host storage; unit captures
keep existing intake rules and the compile-time capture cap was dropped. Correction 11
was THIRD/LAST. Full review/contract details stay on #613, #667 and in its proof.
The independent default writer-budget inspections remain component evidence,
not a default-budget advancement or separate activation proof.

Coordinator owns any future shared CI/proof/checkpoint integration only within
new owner authorization. Build/scratch remain on disk under owned ignored task
roots; verify actual checkout/PR/session before any future authorized task.

## API-011 capability map (the reference path SLICE-001 hosts)

Read `server/include/subathon/server/reference_configuration.hpp`,
`reference_runtime.hpp`, `reference_intake.hpp`, issue #562 and PR653.
Entry points: complete immutable `ReferenceConfigurationArchive`; typed
`encode_reference_state`/`decode_reference_state`; real `SystemReferenceClockSampler`
and `OsReferenceEntropy`; five-port `ReferenceApplicationAdapters` with bounded
reserve/settle/reconcile journal; `ReferenceIntake::make_initial_work`,
`make_receipt_work`, `make_evidence_work`, `make_capture_work`, `make_snapshot_work`.

This is component composition, NOT a running host/login/provisioning/recovery or
public synthetic endpoint. Startup needs actual stored fresh owner/session/graph
and complete configuration; no owner boolean or anchor reconstructed from raw SAS1.
Only normal acknowledged startup publishes the live process anchor. Supported
runtime is Active/Main-positive, no wait/debt/balances/deadline, with pause/recovery
known false ONLY from the live journal. Unknown data never proves empty actions or
provider/payment/activity knowledge. API010/login, recovery/restart, full249 and
host/API001/SLICE001 keep their ownership and complete release requirements.

Four DISTINCT transactions: adopted receipt commit; adopted evidence commit;
caller-owned plan/SRX/index commit; actual application-service commit. No second
writer/timer write or commit after adoption. Snapshot authenticates first, requires
current ordinary timer read USE (no owner shortcut), then publishes coherent normal
committed state. Automation rechecks original-author USE+GRANT and execution USE;
historical disclosure checks EVERY actual hold. Unknown stops; real application
postcommit error needs matching independent committed reconciliation, not replay.

Preserve B04 dual SAS1 echoes: facade uses stored previous basis (or initial token),
old service binds CURRENT head; no SAS1/SAR1/SAA1 byte changes. N11 compares complete
duplicate output including optional/nullness. N12 bounds spent RRF1 even for direct
issue; full domain/purpose/suffix identity, no probability/eviction/reuse shortcut.
Every reserve draws one RCO1 including reuse; absent-basis examples use3draws and
after03, known-rollback retry advanced05/after06. No entropy retry/fallback.

Reviewed tests/fixtures/public semantics remain fixed. Existing Qt CI runs exact
32+51methods/59+78invocations and61+80passes with hooks,0skips/warnings, ordinary
acceptance (no WILL_FAIL/inversion). Existing selector name
`SUBATHON_BUILD_REFERENCE_INTAKE_BASELINE=ON` is retained with `reference-rule`
selection. Use docs/TEST_STRATEGY.md, CI and local CMakeUserPresets for SDK paths;
no personal paths are required here.

Local Qt6.12/GCC16.2/Boost1.92 differs from hosted pinned Boost1.83. ASan/UBSan runs
have leaks disabled; LSan fails under ptrace and external libraries are uninstrumented.
No leak proof, allocator-fault/performance theorem, one aggregate process heap
ceiling, live provider, host/HTTP, platform deployment or recovery verification.

## Settled policy and prerequisites

[Owner decisions249](https://github.com/Kernol12/subathon-bot-ai/issues/249#issuecomment-6879902355): restrictive edits need management USE+GRANT scope;
untouched out-of-scope grants may remain, changed/newly exposed rights and every
shared-role account are checked. Automation needs author USE+GRANT and separate
sender checks; recheck at execution and hold after revocation. Full249 remains open;
identity251, delivery208/232 and UI424/473 retain their scope. Prefix1–10 migration
bytes are frozen; no new migration is assigned.

Completed protected merges: ACCESS-009 (#645), PR648, 1fd11c3b;
ACCESS-010 (#646), PR650, 65936bcb; ACCESS-011 (#649), PR651, d7d64f25;
RULES-006 (#558), PR652, 03e52e70; API-011 (#562), PR653, f7921b95. Their
issues, PRs and proofs own the evidence. Do not reverify pre-MIGRATE-004 records.
