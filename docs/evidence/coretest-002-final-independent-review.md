# CORETEST-002 final independent test and contract review

Date: 2026-10-31. Reviewer: `/root/timer_final_reviewer`, separate from original
author `/root/numeric_test_author`, correction author `/root/timer_test_fixes` and
confirmation-policy author `/root/timer_policy_author`.

Verdict: **approved for test baseline** at the exact artifacts below. All 39 actual
test bodies, their helpers, public header, three unconditional stubs, final contract,
task brief and optional CMake/preset lane were examined independently. No blocking
findings remain. This approves the next intended stub-red run; it is not evidence
that any timer behavior passes or that CORE-002 implementation may skip that run.

## Actual revision, authority and ownership

Reviewed author candidate `160c7e6e3ffa3894c53df924df7323f24ebda9dd` as integrated
at review checkout `3a15c8132b6894278b0dab6af57b9c131beed236`, branch
`task/CORETEST-002-timer-review`, isolated worktree
`local-evidence/worktrees/coretest002-review`. This reviewer owns only this evidence
file; the coordinator owns central status/progress, publication and the observed red.

Authority read: root, core and tests AGENTS instructions; coordinator instructions;
architecture, workflow, test strategy and first-assignment guidance;
specification v0.45 §4/§18 and TIMER-01/02; event/state boundaries revision 1;
the final timer contract, CORE-002 brief, owner clarification, authoring history and
[prior partial review](coretest-002-independent-review.md). The previous review's
two coverage findings had been closed separately. Its unresolved product decision
is now settled by the explicit [owner clarification](timer-confirmation-credit-decision.md)
and the updated specification; no policy was inferred from a request to resume work.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| `tests/acceptance/timer_lifecycle_tests.cpp` (39 cases) | `bf8c1f46d2fe8e6fdbd1ce2ed5841e679dc4a1a7` |
| `docs/contracts/timer-lifecycle-v1.md` revision 1 | `09c7c77d2e42b6cc5971f8bff8962c0486a84082` |
| `core/include/subathon/core/timer_lifecycle.hpp` | `69cad7023b9928371877d45955b800aed9996cd5` |
| `core/src/timer_lifecycle.cpp` | `5584f845faef1dff68377363ec04c7b2effc4cae` |
| `docs/specification.md` | `78d487f4a7983b5c33260eb4b4ab2dff7e520181` |
| `docs/contracts/event-state-boundaries-v1.md` | `8b60da43f1362c46c54730e8ca029b858964ea59` |
| `docs/evidence/timer-confirmation-credit-decision.md` | `6e6c87dd3524f10b83d6d4567c39e3755d6a2fe5` |
| `docs/tasks/EPIC-001/CORE-002.md` | `be531e574ba2be496d535a6c3a236f55744e90d0` |
| Root `CMakeLists.txt` | `b3d20800ddc7ddb8ea8c0923fa5d4e32e1a594f5` |
| `core/CMakeLists.txt` | `f00a81d0f1f9f47c7366d8db991c638eecd13ef6` |
| `tests/CMakeLists.txt` | `e8af22137e354c3b871b129a76461a10727af94a` |
| `CMakePresets.json` | `fdfff4729dd4455474b560246193fac0b8b8453e` |
| `cmake/GoogleTestDependency.cmake` | `ecedcf7a9f7084d9af7d5f12b9cca2612283fb39` |
| `cmake/ProjectOptions.cmake` | `2490bc69f92623415b2bc2f28f7d586ebe3e26cc` |

## Independent oracle and coverage review

Case numbers below are the observed CTest discovery order, matched to every actual
body in the reviewed test blob. Expected values come from the cited requirements,
explicit snapshot conventions, integer subtraction and command ordering. The helpers
only construct synthetic states; no expected value is calculated by the reducer or
another production algorithm.

| Cases examined | Oracle and finding |
| --- | --- |
| 1 `DefaultExactMainExpiryStartsSeparateTwoMinuteGrace`; 2 `MainCountdownBeforeZeroPreservesGraceBudget`; 3 `MainOvershootImmediatelyConsumesGrace` | Defaults are Grace/Fresh/120000ms. From Main1000, elapsed999 leaves Main1 and budget120000; elapsed1000 enters Grace120000 with MainExpired → GraceStarted; elapsed1001 leaves Grace119999. Matches §4/TIMER-01/02. |
| 4 `OneStepCanCrossMainAndAllGraceWithoutLosingTransitions`; 5 `GraceStaysActiveUntilItsExactLastMillisecond`; 6 `ExactGraceExpiryFinishesAndNeverMakesGraceNegative` | Main1000 plus Grace120000 expires at elapsed121000 with exactly MainExpired → GraceStarted → GraceExpired → Finished. Grace1000 minus999 remains active at1; Grace1 minus1 finishes at0. Ordered proposals preserve crossed phases. |
| 7 `AlreadyZeroClocksResolveWithoutConsumingPausedTime`; 8 `ConfiguredGraceDurationReceivesTheExactOvershoot`; 9 `ConfiguredZeroGraceExpiresWithoutAnArtificialDelay` | Already-zero expiry resolves while paused without consuming elapsed or new Grace. Configured30000 minus500 overshoot yields29500. Configured0 enters/expires Grace immediately, preserving the four-event trace. These follow the explicit zero-boundary representation. |
| 10 `ImmediateModeFinishesAtMainZeroWithoutGrace`; 11 `ConfirmationModeWaitsWithoutConsumingGrace`; 12 `PreauthorizedConfirmationFinishesOnlyAwaitingState` | Immediate finishes Main at0. Confirmation consumes no Grace despite overshoot, emits MainExpired → ConfirmationRequired once and waits across another500000ms. An authorized confirmation finishes only the waiting snapshot and clears its flag; ordinary Main and Grace reject. Permission checking itself is external. |
| 13 `PositiveActualCreditResumesConfirmationWaitAndRejectsStaleConfirm`; 14 `ZeroActualCreditRetainsConfirmationWaitAndBothClocks` | Owner-selected policy: Active/Main0/wait/Grace73000 plus actual5000 becomes Active/Main5000/no-wait/Grace73000. A subsequent stale confirmation rejects its waiting-state precondition. Actual0 preserves lifecycle, phase, Main0, Grace73000 and wait. These are actual amounts already resolved by the caller, not an assumption that a requested award survives debt/caps/review. |
| 15 `ResumedConfirmationWaitDoesNotOverrideAnExternalPause`; 16 `HardEndObservationPreventsCreditFromResumingPausedConfirmationWait` | After actual5000 resumes Main, a paused elapsed1000 retains Main5000/Grace73000 and produces no transitions. At enabled UTC1000, paused waiting Main0 finishes first, clears wait, retains Grace73000 and emits exactly HardEndReached → Finished; subsequent actual5000 rejects. This tests the bounded pause observation and advance-before-credit contract, not pause-reason storage. |
| 17 `PositiveApplicableCreditRescuesGraceAndPreservesItsBudget`; 18 `ZeroActualCreditDoesNotRescueGrace`; 19 `FreshModeRestartsFullGraceOnRepeatedMainExpiry`; 20 `SharedModeReusesOnlyUnusedGraceAfterRescue` | Grace90000 plus actual5000 returns to Main5000 retaining90000; zero stays in Grace. Next Main expiry resets Fresh to120000 but reuses Shared90000. Grace is not consumed while Main runs. |
| 21 `ExhaustedSharedBudgetExpiresAtMainZeroWithoutAnArtificialDelay`; 22 `SharedGraceConsumesOnlyTheOvershootAfterMain` | Shared retained0 with configured120000 and exact Main expiry finishes with the four-event trace, without reloading the configured budget or requiring another tick. Rescued Main5000 advances4000 to Main1000/budget90000, then1500 to Grace89500. Prior coverage finding remains closed. |
| 23 `PausedMainPreservesBothClocksAndPhase`; 24 `PausedGracePreservesSharedBudget`; 25 `AcceptedRescueDoesNotOverrideAnExternalPause` | A paused500000ms consumes neither Main1000 nor Grace90000 and preserves phase. Already-eligible Grace rescue switches to Main5000 but the next paused observation still preserves it and retained Grace90000. No clearing of independent reasons is represented or claimed here. |
| 26 `HardEndIsNotReachedOneUtcMillisecondEarly`; 27 `ExactHardEndFinishesBeforeCountdownProjection`; 28 `HardEndFinishesPausedGraceWithoutPostponement`; 29 `HardEndFinishesConfirmationWaitWithoutConfirmation` | Enabled deadline1000 does not finish at UTC999 and does finish at1000 in Main, paused Grace and confirmation wait. Exact hard-end observation has the declared priority over countdown projection and emits HardEndReached → Finished. Retained pre-call fields are explicitly snapshot conventions, not invented values at an interpolated historical instant. |
| 30 `UtcObservationDoesNotBecomeCountdownElapsed`; 31 `FullSigned64MainDurationDoesNotOverflowPhaseArithmetic`; 32 `VeryLargeOvershootFinishesWithoutNegativeDurations` | With no deadline, UTC INT64_MAX does not replace elapsed100: Main1000 becomes900. Main INT64_MAX minus equal elapsed enters untouched Grace120000, avoiding a Main+Grace sum. Main1000 with elapsed INT64_MAX crosses both phases to0/0 and finishes without wrap. |
| 33 `ReadyRepresentationRequiresASeparateStartBoundary`; 34 `FinishedStateNeverAdvancesOrAcceptsAnImplicitReopen`; 35 `ExpiryObservationPrecedesSameInstantRescueCommand`; 36 `HardEndObservationPrecedesSameInstantCreditCommand` | Ready is represented but rejected by this already-started boundary. Finished retains its snapshot and yields no transitions, rejecting credit/confirmation. Observed Grace expiry or hard end wins before same-instant credit; advancing Grace1 by0 still permits rescue. First-start, explicit reopening and actual application admission remain separate contracts, not removed scope. |
| 37 `NegativeClockDurationsAreRejected`; 38 `IncoherentPhaseAndConfirmationWaitSnapshotsAreRejected`; 39 `ApplicableCreditRejectsNegativeAndOverflowingAdjustments` | Negative elapsed/Main/Grace/configured Grace and negative credit reject. Grace with positive Main, wait with positive Main, and wait in Grace reject at all three public boundaries; a valid wait rejects advance under Immediate/Grace config. Main INT64_MAX plus1 rejects overflow; Main1000 plus5000 becomes6000. The malformed-state coverage correction is preserved. |

Read-only comparison against correction author `ef82bbcf12b9ee8074ab27c18bc41f986a786c8e`
confirmed all 35 prior test bodies are retained byte-for-byte, with exactly four
additions. There is no assertion deletion, weakening, skip, expected-value generation
from the implementation, fixture substitution or production implementation.

## Contract, boundaries and harness findings

The public layouts separate lifecycle from phase and retain one Grace budget rather
than duplicate mutable budgets. The confirmation flag is restricted to Active/Main0.
Const input snapshots/configuration and returned value proposals keep authoritative
writes outside core. Credit has no clock/configuration access: callers must advance
first and resolve actual eligibility/caps/debt against that current state. This is
explicit in the contract and header, and the sequence tests exercise it. The three
source functions unconditionally throw `TimerLifecycleNotImplemented`; no existing
algorithm is being approved indirectly.

Elapsed duration and UTC instant remain separate inputs. A pause observation describes
one already-segmented interval, not arbitrary historical signal changes. Hard-end
priority and retained snapshot fields are clearly bounded conventions. The ordered
trace is an uncommitted domain proposal, not an audit/event transaction or a fabricated
UTC chronology. This is consistent with event/state boundaries revision 1.

No remaining blocker or requirement conflict was found for this slice. Validation
tests are focused examples rather than exhaustive permutations. The absent pause-set,
schedule/DST, clock-discontinuity/recovery, start/reopen, signed-adjustment and
configuration-edit algorithms have explicit downstream tasks; this approval does not
certify those behaviors. Persistence/Qt ownership, source admission, permissions,
deduplication, atomic actual effects and event ancestry remain application/storage
gates. A paused-observation test cannot establish correct retention of a real pause
reason set, and none is claimed.

The optional root flag builds `subathon_timer_lifecycle` from the reviewed stub and
links its acceptance executable to that target and `GTest::gtest_main`. All39 cases
are discovered with `timer-lifecycle;acceptance;intentional-red`. The preset enables
that lane without enabling or altering stable feature suites. Installed GoogleTest
is selected here, fetching remains OFF, and no dependency pin was changed. There is
no failure inversion, skip, WILL_FAIL or continue-on-error.

CI assessment: current workflows execute scaffold, credit, numeric and receipt lanes;
the intentional-red timer lane remains separate. CORE-002 must make this suite pass,
add it to automatic relevant PR/main CI and observe the workflow before implementation
completion. No new workflow or branch-protection configuration was changed or verified
by this review.

## Commands actually run and evidence limits

Environment: Linux x86_64, GCC16.2.1 (`/usr/bin/c++`), CMake4.4.5, installed
GoogleTest1.18.0 (`/usr/lib/cmake/GTest`), Python3.14.8. Reviewer commands ran in
the isolated review worktree above; no timer test body was executed.

| Command / inspection | Observed result |
| --- | --- |
| `git status --short`, `git log -4 --oneline`, `git rev-parse HEAD`, `git branch --show-current`, `git rev-parse 160c7e6` | exit0; clean candidate checkout and exact revisions recorded above |
| `cat`, `nl -ba`, `sed` and `rg` reads of the named instructions, specification, contracts, all tests/header/stubs, task/evidence and build/workflow files | exit0 for reviewed files; initial attempted `docs/tasks/CORETEST-002.md` read reported no such file, then the actual state-linked `docs/tasks/EPIC-001/CORE-002.md` was read successfully |
| `git hash-object` over all artifacts in the table | exit0; exact blobs recorded above |
| `git diff ef82bbcf12b9ee8074ab27c18bc41f986a786c8e HEAD -- tests/acceptance/timer_lifecycle_tests.cpp docs/contracts/timer-lifecycle-v1.md` | exit0; four additive cases and explicit owner-policy contract refinement |
| Read-only `python3` comparison using `git show ef82bbcf12b9ee8074ab27c18bc41f986a786c8e:tests/acceptance/timer_lifecycle_tests.cpp` and the actual file; matched each full TEST body | exit0; all35 prior bodies identical, exactly4 additions, total39 |
| `cmake --preset timer-lifecycle-debug` | exit0; existing isolated build configured/generated |
| `cmake --build --preset timer-lifecycle-debug` | exit0; incremental test compilation and linking completed (2 steps); existing stub target reused |
| `ctest --preset timer-lifecycle-debug -L timer-lifecycle -N` | exit0; exactly39 cases listed; discovery only |
| `cmake --version`, `c++ -dumpfullversion`, `pkg-config --modversion gtest`, `uname -s -m`, `python3 --version`; relevant CMake cache inspection | exit0; environment above, GoogleTest fetch OFF |
| `git diff --check` | exit0 before evidence authoring; repeated before commit |

The coordinator must now run and retain the actual
`ctest --preset timer-lifecycle-debug -L timer-lifecycle` result against these exact
reviewed behavioral artifacts. Intended failure must be attributable to the explicit
missing behavior, after successful compilation, with no missing dependencies or
skips counted as red. Only that observation completes CORETEST-002's remaining
behavioral gate; publication/CI/merge bookkeeping remains coordinator-owned.
CORE-002 then needs implementation, passing relevant checks and separate code review.
No SQL, Qt runtime, provider, renderer, supported-platform, packaging or live/OBS
verification is established here. All release scope remains required.

## Final red-evidence and status integration review

Same independent reviewer, 2026-10-31. Verdict: **approved for baseline integration**
of coordinator candidate `6e51d161c2a3d5b82459869f1b4d09bc3a85d3b7`, inspected in
review checkout `ba7a903e4ee5c3efb3cc313dfb9a4614df0e6e2b`. Approval of the actual
39-case baseline above was integrated as `cd59601fff4149d7c83ab4c4c442c0daf19a5de7`
before the coordinator's timer run. The raw log and handoff were added afterwards;
this review does not claim to have personally rerun that test-body command.

Inspected the full integration delta from `2fb544f`, including the raw red output,
baseline handoff, contract/brief status prose, test strategy, acceptance map, task
state, generated progress and coordinator checkpoint. A read-only comparison
confirmed the tests, header, stubs and root/core/tests/preset build harness remain
byte-for-byte identical to the approved artifacts. The timer contract changes only
its opening status paragraph; none of its behavior, examples or validation changed.
The completed-status contract blob is `cfee0767cc0829106057b2cf631a03e2e59cca30`,
and the completed-status brief is `cb6577defbaf1e00e5a6bb4b9da3f5be62c3864b`.

The [raw log](coretest-002-red-ctest.txt), blob
`9166d4df19df61c844ff9183f9dad52ac0ea006c`, contains all 39 reviewed case names
in order, their individual RUN records, failed CTest outcomes and the explicit
unimplemented diagnostic in every case. Validation cases show the actual
`TimerLifecycleNotImplemented` type instead of the required domain exception, as
expected from the unconditional stubs. The footer records 0% passing and 39 failures;
no skipped/not-run cases or missing-executable failure appears. The
[baseline handoff](coretest-002-baseline-handoff.md), blob
`b71ae7d1a5fedfdede0cb01d754f59500e3b8306`, records the exact command, observed
exit8, successful prior compilation, run revision and environment. This is valid
intended missing-behavior red evidence, following independent approval.

CORETEST-002 is done as a test-baseline task, with observed_red and product acceptance
not_run. CORE-002 remains planned. TIMER-01/02 remain partial and the bounded scenario
is observed_red; neither the map nor HTML upgrades timer behavior to passing.
Generated completed-task count rises to20 while verified implementation tasks remain3.
Historical author/partial-review records and explicit downstream scope are preserved.
No blocking integration finding remains. A minor snapshot timestamp observation was
sent to the coordinator: updated_at still preceded the new history entries; a final
publication refresh can update that display stamp without changing this review scope.

Commands actually run for this integration review, all successful unless noted:

| Command / inspection | Observed result |
| --- | --- |
| `git status --short`, `git log -6 --oneline`, `git diff --stat 2fb544f HEAD`, targeted full diffs and reads of the integration paths, `git rev-parse HEAD 6e51d16 cd59601`, artifact `git hash-object` | exit0; candidate/revisions and bounded delta above. A search for optional CLI usage strings in the DOM script had no matches (exit1); the script was then read and run successfully. |
| Read-only Python validation of every raw-log case block against the actual 39 TEST names; absence of skip/not-run/missing-executable diagnostics; byte comparison of tests/header/stubs/harness against `2fb544f` | exit0; all checks satisfied |
| `git diff --quiet 2fb544f HEAD -- core tests/acceptance CMakeLists.txt CMakePresets.json cmake` | exit0; no executable source or build-harness delta |
| `python3 tools/check_project.py` | exit0;377 task records,122 decisions, links/presets/current HTML pass |
| `python3 tools/progress.py render --check` | exit0; generated snapshot matches current state |
| `python3 -m unittest discover -s tests/tooling -v` | exit0;19/19 tooling tests pass |
| `node tools/check_progress_ui.mjs` | exit0; DOM simulation passes; actual browser/OBS explicitly not run |
| `git diff --check` | exit0 before append; repeated with staged evidence before commit |

No redundant native rebuild or timer-body rerun was performed: the approved executable
artifacts and harness are unchanged, the reviewer already built them successfully,
and the coordinator has retained the actual intended red. Stable native suite counts
in the handoff are coordinator evidence, not newly executed reviewer commands.
Latest-revision PR CI, publication and merge eligibility remain coordinator gates.
Implementation, automatic passing timer CI, separate code review and all downstream
release/platform/live gates remain required.
