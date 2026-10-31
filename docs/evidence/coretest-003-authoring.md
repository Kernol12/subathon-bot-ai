# CORETEST-003 — pause-policy draft authoring

Author: `/root/pause_test_author`, 2026-10-31. Task branch:
`task/CORETEST-003-pause-author`; seed claim425447c from integrated main0700d5d.
Coordinator-owned harness ec9d8a9 was merged into this isolated author worktree.
Authority: specification v0.45 §4/TIMER-03/05, §18 pause/disconnect/unknown cases,
event-state boundaries revision1 and the unchanged timer lifecycle revision1.

## State and owned artifacts

Authored draft: [pause-policy proposed revision1](../contracts/pause-policy-v1.md),
`core/include/subathon/core/pause_policy.hpp`, unconditional
`core/src/pause_policy.cpp` stubs, 30 GoogleTests in
`tests/acceptance/pause_policy_tests.cpp`, and the CORE-003 brief.
The first interface/contract/stub proposal was committed as4f24779078c6019db8de41b9fbcac7073db59a61.
Header empty optional defaults were then made explicit without changing the API.

This is authored and compile/discovery-checked, not independently reviewed, observed
red, implemented, timer-integrated or live/platform-verified. No test body has been
run. Every call still unconditionally throws `PausePolicyNotImplemented`; no delay,
validation or reason-set algorithm has been implemented. Existing code and reviewed
assertions remain untouched. The coordinator owns CMake/presets, shared progress,
HTML/map, CI, integration and publication; this note does not self-certify those gates.

## Manually derived oracles

The test helper compares all five public snapshot fields independently; it derives
no expected value from production code. Every expected duration and set is written
explicitly from arithmetic and §4. Synthetic normalized signals have no provider
capability assumptions, actual sessions, supporter records or external side effects.

| Draft case numbers / names | Authority and independent expectation |
| --- | --- |
| 1 OfflineDefaultDelay…, 2 DesignatedClient… | TIMER-03: default30000ms; 0 then29999 remains pending without a reason; final1 activates exactly and removes only its pending counter. |
| 3 CustomOfflineAndClient…, 4 EarlierClientExpiry… | Delays10/20 and20/10: first threshold activates only that reason; second counter keeps its10 or19 accrued milliseconds, then expires independently. Pending timers keep observing elapsed even when another reason is active. |
| 5 ZeroDelays…, 6 ZeroOfflineDelay… | Nonnegative local configuration convention: delay0 activates at elapsed0; the independent client30000 delay still has pending0. |
| 7 OfflineRecoveryBeforeExpiry…, 8 ClientFlapping… | A known recovered signal cancels its pending delay. Next episode begins fresh: offline29999 remains pending, client1 after recovery remains1; Manual persists. No Unknown edge is involved. |
| 9 RepeatedOfflineSignal… | Repeated elapsed0 retains12000, next17999 produces29999, then1 activates. A further INT64_MAX interval cannot duplicate the reason or overflow an active counter. |
| 10 OfflineRecoveryRetains…, 11 ClientRecoveryRetains… | §4/§18 independent reasons: remove only recovered signal hold. Manual, Schedule, RecoveryReview and the other active signal reasons stay. Unknown is still Unknown in11, so no unresolved recovery edge is selected. |
| 12 ExternalPauseReasons…, 13 RepeatedExternalSet… | Set membership, not a global paused flag: clear RecoveryReview, Manual and Schedule individually; StreamOffline remains. Repeated Manual set/clear preserves both pending5/7 values and Schedule. |
| 14 UnknownDefaultRaises…, 15 UnknownDefaultRetains…, 16 UnknownIsNotOffline… | TIMER-05: uncertainty notice is separate; disabled timeout and arbitrarily long Unknown adds no reason or pending timeout. Confirmed-offline/external holds remain. Even offline delay0 cannot reinterpret Unknown as Offline. |
| 17 KnownOnlineClearsNotice… | Without optional Unknown timeout/pause, known Online removes the uncertainty notice; no unresolved timeout-recovery policy is asserted. |
| 18 OptionalUnknownTimeout…, 19 UnknownTimeoutAddsAlongside…, 20 ZeroUnknownTimeout… | TIMER-05 optional separate reason: timeout10 has pending0 then9; final1 adds only Unknown. Other five reasons remain, including confirmed Offline. Timeout0 activates at elapsed0; repeated Unknown after activation remains stable. |
| 21 LargestSignedDisconnect…, 22 OversizedElapsed…, 23 LargestUnknownTimeout… | Local signed64 conventions: INT64_MAX-1 remains pending under max delay; +1 expires exactly. Adding a max interval to an already max-1 pending duration must activate without signed overflow; no huge active counter is required. |
| 24 NegativeConfiguration…, 25 NegativeElapsed… | Local preconditions: negative delay/timeout or elapsed (including INT64_MIN) rejects with invalid_argument rather than wrapping, repairing or proposing state. |
| 26 NegativePending…, 27 AnActiveReasonCannotAlso…, 28 PendingAtOrBeyond… | Public snapshot coherence: pending values nonnegative and below threshold, and not simultaneous with their own active reason. Both entry points reject structural corruption; only observe has config to validate thresholds. |
| 29 UnknownPendingRequires… | Local snapshot coherence: pending Unknown requires enabled timeout and notice. A pending Unknown without notice rejects through either entry point. |
| 30 ExternalReasonCommands… | Local ownership: manual/schedule/recovery setter cannot add or remove any automatic signal reason. Signal observations retain that responsibility. |

## Unresolved product-policy gates

Repository search found no authority choosing either of these combined behaviors:

- An Offline delay is pending, then the stream becomes Unknown: cancel/freeze/continue
  the offline pending duration are distinct product choices.
- An optional Unknown pause has activated, then status becomes known: which statuses
  clear that hold, and does a subsequent Unknown episode receive a fresh timeout?

The coordinator independently confirmed the gap and requested owner clarification;
see [the durable policy questions](pause-signal-policy-questions.md). No response is
assumed here. No authored test crosses these unresolved edges, and no skipped test
pretends to cover them. Final baseline approval, intended red and CORE-003 production
implementation require the recorded answer, affected contract/examples/cases and an
actual separate review of that exact revision. The intended product scope stays intact.

## Actual commands and environment

Worktree: `local-evidence/worktrees/coretest003-author`. Linux host, GCC16.2.1
(20260810), CMake4.4.5, Ninja1.13.2, GoogleTest1.18.0 (`pkg-config --modversion gtest`),
clang-format23.1.4. Date observed:2026-10-31T19:52:47Z. No network/dependency install,
Qt runtime, UTC arithmetic, clock sleep, persistence or provider call was needed.

| Command | Observed outcome |
| --- | --- |
| `clang-format -i core/include/subathon/core/pause_policy.hpp core/src/pause_policy.cpp tests/acceptance/pause_policy_tests.cpp` | Exit0; applied repository LLVM/4-space/100-column style. |
| `cmake --preset pause-policy-debug` | Exit0; GNU16.2.1/Python3.14.8 detected; generated the isolated optional pause build. |
| `cmake --build --preset pause-policy-debug` (first draft) | Exit0, but aggregate missing-initializer, range-copy and discarded-nodiscard warnings appeared. No test body ran; build discovery lists names only. |
| `cmake --build --preset pause-policy-debug` (after explicit optional defaults, reference loops and `(void)` in throw assertions) | Exit0; four targets rebuilt/linked, no warnings in output. Both production entry points remain unconditional stubs. |
| `ctest --preset pause-policy-debug -N -L pause-policy` | Exit0; lists all30 draft cases. `-N` only discovers, so this is not red evidence. |
| `git diff --check` | Exit0 before this evidence edit; final author commit records the follow-up check. |

No plain `ctest -L pause-policy` or direct test execution occurred. Scaffold/stable
suite checks, tooling, progress rendering and CI remain coordinator-owned integration
work; no unrun checks are claimed. The intentionally red lane remains optional and
separate from passing automatic acceptance suites until reviewed implementation.

## Exact draft artifacts and next handoff

After author formatting/compile/discovery, before this evidence text:

| Artifact | Git blob |
| --- | --- |
| Header | `c91b7da3fa1c98a1b2e47ab389f48f74901c2a47` |
| Unconditional stub source | `dee2a91c31b524c082192aba70e7e0c0efa27501` |
| Draft30 tests | `af1275f7e893345edd3820edd5382af5c718d965` |

The coordinator records the final authored commit/contract blob and independent
review separately. Next: publish this useful draft, obtain the missing owner policy,
author the combined cases, have `/root/timer_final_reviewer` review actual artifacts,
then record a successful build followed by a genuine intended stub-red run. That
does not finish CORE-003; separate implementation/review/stable CI still follow.

## Owner-edge authoring addendum — 2026-10-31

Author: `/root/pause_owner_case_author`; coordinated assignment on
`task/CORETEST-003-owner-cases`, worktree `local-evidence/worktrees/coretest003-decisions`,
starting commit `5d88a75ba83accd0f7f26802c1b7e1fe8b0f041f` (current-main5201edd plus
actual draft PR20 artifacts). The earlier unresolved-policy section is historical;
both owner answers are now recorded in
[pause-signal-policy-questions.md](pause-signal-policy-questions.md).

Only the tests, proposed contract, CORE-003 brief and this appended evidence changed.
No API/layout or production implementation changed. Coordinator retains build files,
state/HTML, acceptance map, specification, decision record and review evidence.
The independent partial review005b6f4 covered the original30; it does not approve
the seven additions or authorize a body run. Full independent review remains next.

Manually derived additions (source order31–37):

| Case | Oracle and authority |
| --- | --- |
| 31 UnknownCancelsPendingOffline… | Owner answer1: pending29999 disappears on Unknown0; even INT64_MAX Unknown cannot activate Offline. Later Offline0/29999/1 starts the full default30000 delay and expires exactly. |
| 32 UnknownCancellationRestarts… | Owner answer1 applies to configured delay7: prior6 is discarded, Unknown3 has only its own pending3; later Offline6 remains pending and final1 expires. Known Offline also clears Unknown pending/notice. |
| 33 EitherKnownStatusCancelsPendingUnknown… | Owner answer2: each known status clears Unknown pending9/notice. Offline starts its own pending0; the next Unknown cancels that and must accrue9+1 fresh for timeout10. |
| 34 KnownOfflineClearsActivatedUnknown… | Owner answer2 and independent holds: Unknown10 adds its hold alongside five existing holds. Known Offline removes only Unknown, then next Unknown9 remains pending and final1 reactivates; established Offline/external/client holds persist throughout. |
| 35 KnownOnlineComposes… | Owner answer2 plus existing TIMER-03: known Online clears Unknown and Offline independently; disconnected client and Manual/Schedule/RecoveryReview remain. |
| 36 KnownOfflineAndConnectedClient… | Owner answer2 plus existing TIMER-03: known Offline clears Unknown and connected client clears its reason, retaining Manual and established Offline. No global resume. |
| 37 ZeroUnknownTimeoutRearms… | Owner answer2 with existing nonnegative configuration: each known status clears activated Unknown; next Unknown0 immediately activates again under timeout0. Offline pending0 is canceled by that Unknown. |

No expected result comes from implementation output. An exact Python extraction of
all `TEST(PausePolicyAcceptance, …)` bodies compared the original tests blob
`af1275f7e893345edd3820edd5382af5c718d965` with the candidate: all30 original bodies
remain byte-for-byte unchanged; seven are appended (37 total). No baseline fixture
or assertion was weakened. Synthetic observations assert no provider capability.

Actual commands in this worktree, Linux/GNU16.2.1/Python3.14.8:

| Command | Outcome |
| --- | --- |
| `clang-format -i tests/acceptance/pause_policy_tests.cpp` | Exit0; repository formatting. |
| `cmake --preset pause-policy-debug` | Exit0; isolated build generated. |
| `cmake --build --preset pause-policy-debug` | Exit0; ten build/link steps, no warnings in output; discovery only, no test bodies. |
| `ctest --preset pause-policy-debug -N -L pause-policy` | Exit0; all37 cases listed, no body execution. |
| Python original30-body comparison and `git hash-object` | Exit0; unchanged originals/header/stub confirmed and exact candidate blobs below. |
| `git diff --check` | Exit0 before this append; repeated for final candidate. |

| Candidate artifact | Git blob |
| --- | --- |
| Header, unchanged | `c91b7da3fa1c98a1b2e47ab389f48f74901c2a47` |
| Unconditional stub, unchanged | `dee2a91c31b524c082192aba70e7e0c0efa27501` |
| Draft37 tests | `646d83f4e38722239bb8b590a9f368141bed4246` |
| Proposed revision1 contract | `a8174be71e7f4cf453a55df14f57126b1b6e61d8` |

No test body/red run, implementation, CI, timer integration or live/platform behavior
is claimed. Next: independent reviewer examines the complete actual candidate, then
the coordinator may run and record intended stub-red only after full approval.
CORE-003 production implementation remains gated separately.
