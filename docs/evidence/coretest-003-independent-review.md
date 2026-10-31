# CORETEST-003 — bounded independent draft review

Author: `/root/pause_test_author`; authored evidence commit
`bb34d1ede52e13aa43d5fb797d7c0085caa5d877`.
Reviewer: `/root/pause_resume_reviewer`, a separate agent/session, 2026-10-31.
Review worktree: `local-evidence/worktrees/coretest003-review`, branch
`task/CORETEST-003-pause-review`, input commit
`cf07abc13477edbc2d8b92cca44badbf641b88aa`.
Ownership: this evidence file only; coordinator retains shared state/build/publication.

## Verdict and authority

**The known 30 cases are valid as a bounded partial review. The full baseline remains
blocked and is not approved for a red run, implementation or merge by this review.**
No incorrect oracle or blocking defect was found within those 30 cases. The two
unanswered owner questions below still prevent complete baseline acceptance. This
review does not certify missing interactions, observed red, working pause behavior,
timer integration, platforms or provider reliability.

Read authority: root/core/tests AGENTS, FIRST_AGENT_PROMPT, architecture, workflow,
test strategy, progress state, CORE-003 brief, authoring evidence and policy-question
record; specification v0.45 §4 and §18, TIMER-03/05; timer-lifecycle revision1,
pause-policy proposed revision1, and root/core/tests CMake plus presets and the
GoogleTest dependency loader. Existing timer lifecycle behavior is unchanged.

## Exact reviewed artifacts

| Artifact | Git blob |
| --- | --- |
| Proposed pause contract | `46b6c641c7391a6a0c81a3472d3f03a9190f10fb` |
| Public header | `c91b7da3fa1c98a1b2e47ab389f48f74901c2a47` |
| Explicit stub source | `dee2a91c31b524c082192aba70e7e0c0efa27501` |
| All 30 test cases | `af1275f7e893345edd3820edd5382af5c718d965` |
| Root CMake | `3ed088bd1415b39772a3790f6cc2325d10d7ee8f` |
| Core CMake | `6beeec4620e7d11f577ae7f5ea51145278ed4fe8` |
| Tests CMake | `cc855942456883dbd51534bce0d8ed3c3a90a5db` |
| Presets | `7be5314269f18503d2cb828e1f8c3a38380f50e1` |

## Independent oracle and boundary review

Reviewed every test body and all five fields checked by `expect_snapshot`. Expected
sets and durations are explicit constants, not results produced by either stub or
another production algorithm. The following numbering follows source order.

| Cases | Independently derived result and assessment |
| --- | --- |
| 1–2, default offline/client | A continuous 29999ms interval is below 30000ms; one further millisecond reaches expiry. Pending0 is distinct from absent. Before expiry the reason set is empty; expiry replaces only its own pending value with its reason. Both defaults and disabled Unknown timeout are checked. |
| 3–4, unequal delays | With10/20 thresholds, accrued9+1 activates offline while client retains10, then another10 activates client. With20/10, client activates first while offline retains10 then19 then expires. An existing reason does not stop observation of the other pending condition. |
| 5–6, zero configuration | Nonnegative local configuration includes0: elapsed0 meets that threshold. Offline0 does not bypass the independent client30000 threshold; client remains pending0. |
| 7–9, flapping/repetition | Confirmed Online or client reconnect cancels its own unexpired pending counter; the next disconnect starts fresh. Repeated unchanged elapsed0 cannot reset12000;12000+17999=29999, then1 expires. An active reason remains a single set member through a max-duration observation. Manual survives client flapping. |
| 10–13, reason ownership | Online removes only StreamOffline; reconnect removes only client disconnect. Manual, Schedule, RecoveryReview and other signal holds survive. External setters change only their selected reason, repeated operations are idempotent, and pending5/7 remain intact. Case11 keeps Unknown throughout, so it does not choose Unknown recovery policy. |
| 14–17, default Unknown | Arbitrarily long Unknown raises notice without a new hold/timeout. It cannot erase confirmed Offline or external reasons, and offline delay0 cannot turn Unknown into confirmed Offline. Online clears the notice in a state with no optional Unknown hold/pending value. These cases avoid interrupted offline pending state. |
| 18–20, optional Unknown | Continuous Unknown0/9/1 with timeout10 adds only StreamStatusUnknown at expiry; repeated active Unknown is stable. An enabled timeout adds alongside all five existing reasons; timeout0 activates immediately while preserving Manual/Offline. No known-status recovery is asserted. |
| 21–23, signed64 limits | For delay INT64_MAX, pending MAX-1 plus1 expires exactly. A MAX interval from pending MAX-1 also expires; no sum exceeding signed64 is needed. Both disconnect counters and Unknown timeout are covered. No UTC arithmetic or floating point enters the oracle. |
| 24–25, invalid arguments | Negative config values, elapsed-1 and elapsed INT64_MIN reject with invalid_argument, consistent with explicit local preconditions. The unconditional logic_error stub is not a matching exception. |
| 26–29, invalid snapshots | Negative pending, own active-plus-pending, pending at/above its configured threshold, pending under delay0, Unknown pending with disabled timeout or missing notice reject. Structural checks apply to both entry points; threshold/config checks apply only to observe, which has the configuration. Validation happens even if the supplied signal might otherwise recover or replace the invalid field. |
| 30, automatic ownership | External setters reject adding or removing any of the three signal-owned reasons. This protects independent signal authority rather than introducing a global resume command. |

All tested durations/invalid-snapshot conventions are explicit local API choices in
the proposed contract, rather than invented provider behavior. Const-reference
inputs and returned owned values provide a pure proposal boundary; the interface
has no UI, Qt/network, persistence, credentials or authoritative-state mutation.
Both production functions still unconditionally throw PausePolicyNotImplemented.
No actual algorithm, validation path or timer pause effect has been established.

## Segmentation, harness and coverage limits

The header/contract require constant normalized signal segments and explicit
nonnegative monotonic elapsed duration. A caller must split at signal changes and
delay expiry: the final paused set cannot retroactively freeze the elapsed pending
interval. This agrees with timer-lifecycle's constant-pause interval contract.
The 30 cases exercise final policy snapshots only; they do not prove countdown
consumption across such a boundary. Timer/service integration remains a separate
gate, including preserving main/grace phase and hard-end precedence.

Static harness inspection confirms optional OFF-by-default pause build selection,
its own library/executable, correct GoogleTest main linkage and discovery, and
pause-policy/acceptance/intentional-red labels. The preset enables that slice without
enabling the other optional feature lanes. No WILL_FAIL, skip, assertion weakening
or failure inversion appears in the reviewed harness/tests. Dependency fetching
requires explicit opt-in. The intentional-red lane must stay separate from passing
feature CI until implemented; no CI execution or branch enforcement is certified here.

These are focused examples rather than exhaustive validation permutations. They
provide meaningful boundary/ownership coverage for the known contract; they do not
authorize delay reconfiguration during an episode, invalid out-of-domain enum values,
schedule/recovery algorithms, provider interpretation, or production integration.

## Blocking owner decisions and next gates

The [recorded questions](pause-signal-policy-questions.md) remain unanswered:

1. Unknown interrupts an unexpired Offline delay: cancel, freeze or continue?
2. An optional Unknown pause has activated: does any known status clear it or only
   Online, and does a later Unknown episode start a fresh complete timeout?

No recommendation is treated as an owner answer. Existing cases select neither
policy, and omitted combined cases remain required release scope. Record the owner
decisions, revise contract/examples and add affected tests, then independently review
the complete exact revision before the first intended stub-red body run. Successful
compilation and listing alone are not red evidence. CORETEST-003 remains incomplete;
CORE-003 production implementation and subsequent code-review/CI gates remain closed.

## Actual review commands and evidence

This session performed read-only `cat`, `head`, `sed`, `rg`, `git status --short`,
`git log -4 --oneline`, `git rev-parse HEAD`, `git rev-parse bb34d1e`,
`git ls-files -s` for the eight artifacts above, and `date -u +%FT%TZ`.
All completed with exit0 on this Linux worktree; timestamp observed
2026-10-31T20:10:19Z. Artifact hashes match the authoring evidence where recorded.
The coordinator/author already reported successful compile and discovery of30;
those commands were not repeated here and are not independent runtime evidence.
**No test body was run in this review.** No configure/build/test, provider call,
network operation or live/platform verification is claimed. Final whitespace and
evidence-only status checks are recorded in the review commit/handoff.

## Full 37-case independent review addendum — 2026-10-31

Reviewer: `/root/pause_resume_reviewer`, independent of original author
`/root/pause_test_author` and owner-edge author `/root/pause_owner_case_author`.
This explicitly resumed review preserves the preceding partial review as history.
Actual input commit: `2e86c42294c7ecc5f08149958dfb8d15ecbc50af`, incorporating
coordinator commit001d316 and the owner-edge author candidate3c6d242.
Ownership remains this evidence file only.

**Verdict: approved for the complete bounded CORETEST-003 test baseline.**
All37 actual test bodies, their public contract/header, explicit stubs and optional
harness were examined. No blocking findings or incorrect expected results remain.
The coordinator may now build and record the intended stub-red run of this exact
approved revision. This is test/contract approval, not observed red, implementation
approval or PR merge authorization. CORETEST-003 still needs actual intended-red
evidence; CORE-003 implementation remains behind the completed test-first gate.

### Exact approved revision

| Artifact | Git blob |
| --- | --- |
| Proposed revision1 contract | `a8174be71e7f4cf453a55df14f57126b1b6e61d8` |
| All37 tests | `646d83f4e38722239bb8b590a9f368141bed4246` |
| Public header, unchanged | `c91b7da3fa1c98a1b2e47ab389f48f74901c2a47` |
| Unconditional stubs, unchanged | `dee2a91c31b524c082192aba70e7e0c0efa27501` |
| Root CMake | `5616e5da8454ec933de8e124ea2cfd2792209a04` |
| Core CMake | `6beeec4620e7d11f577ae7f5ea51145278ed4fe8` |
| Tests CMake | `65a66e8db62fa49a4316ac7d5786c7f68afec266` |
| Presets | `e0553c2b3f1cf0c1b3c3ce39c63dd0b126d11b60` |
| Specification v0.45, including owner clarifications | `3e008efaef3181dd8b06a038f6309347ec31619d` |
| Owner decision/recovery-composition record | `581592286b05564a1e8c62ad8658fa598bf05eb9` |

### Complete oracle and contract assessment

Re-read all original30 bodies and checked them against the updated §4, §18,
TIMER-03/05, contract, CORE-003 brief and authoring evidence. Their independent
arithmetic/set-membership analysis in the preceding table remains valid. Defaults,
custom/zero thresholds, flapping, reason ownership, known recovery, Unknown notice
and retained established Offline, validation and signed64 exact/overshoot boundaries
still agree. Both stubs are unconditional; no expected value was obtained from them.

Independently checked the seven added bodies in source order:

| Case | Independent oracle and result |
| --- | --- |
| 31 UnknownCancelsPendingOfflineEvenAcrossALongUnknownInterval | Pending29999 is canceled at Unknown0. Unknown for MAX cannot activate Offline with default Unknown timeout disabled. Offline0/29999/1 then accrues an entirely new30000 delay. Correct. |
| 32 UnknownCancellationRestartsTheFullCustomOfflineDelay | Under Offline delay7, prior6 is discarded. Unknown3 creates only Unknown pending3 under timeout20. Known Offline ends that episode and accrues fresh6; another1 expires Offline. Correct for configured rather than hardcoded default delay. |
| 33 EitherKnownStatusCancelsPendingUnknownAndNextEpisodeStartsFresh | Both known statuses clear Unknown pending9 and notice. Offline independently begins pending0, whereas Online leaves no pending condition. Subsequent Unknown cancels that Offline pending and starts fresh9, then1 activates the timeout10 hold. Correct for both known branches. The conditional expected snapshot uses the explicit known input, not production output. |
| 34 KnownOfflineClearsActivatedUnknownWhileRetainingAllUnrecoveredHolds | Unknown10 adds to all five existing holds. Known Offline clears Unknown alone, retaining established Offline/client/external holds. New Unknown9 is still pending, then1 reactivates at10. Correct; this distinguishes a fresh timeout from immediate reactivation using stale accrued time. |
| 35 KnownOnlineComposesOfflineAndUnknownRecoveryWithoutGlobalResume | Known Online clears Unknown and independently recovers Offline. Disconnected client and Manual/Schedule/RecoveryReview stay. Correct composition of two reason-specific rules. |
| 36 KnownOfflineAndConnectedClientEachClearTheirOwnReason | Known Offline clears Unknown; connected client clears its disconnect hold. Manual and established Offline remain. Correct composition without global resume. |
| 37 ZeroUnknownTimeoutRearmsImmediatelyAfterEitherKnownStatus | Timeout0 first activates at elapsed0. Each known status clears that hold/notice; Offline additionally has pending0. New Unknown0 cancels Offline pending and immediately activates a new Unknown hold. Correct zero-threshold boundary on both known branches. |

The decision record now explicitly resolves both earlier blocking owner questions.
The contract describes cancellation/rearming and independent recovery composition
without interpreting known Online as retaining an already recovered Offline hold.
The specification's broad retention sentence is read with its existing automatic
signal-recovery rule and the linked explicit composition clarification; these do not
grant a global resume. No product-policy uncertainty remains for this bounded API.

Public values remain owned, standard-library-only pure proposals. Complete snapshot
validation precedes observation recovery; negative/coherence/threshold assertions
remain valid even where Unknown would otherwise cancel a pending field. Signed64
limits require bounded comparisons, not overflowing addition. Original and added
cases use constant normalized signal segments and explicit monotonic durations;
elapsed-zero change boundaries are valid. No wall-clock/provider facts are inferred.

Re-inspected root/core/tests CMake and all preset entries: the pause lane remains
optional, OFF by default, isolated under pause-policy-debug, correctly linked and
discovered with pause-policy/acceptance/intentional-red labels. No skipping,
WILL_FAIL, failure inversion or assertion weakening appears. Other contribution-lane
metadata changed since the earlier review; it does not change this pause harness.
The existing header/stub hashes match the previously examined revision exactly.

The 37 cases are sufficient focused acceptance examples for this bounded policy
contract. They do not prove timer consumption across expiry: the caller must still
segment at both signal changes and delay expiry, rather than apply a final paused
set retroactively. Countdown integration, phase/hard-end behavior, configured delay
changes during an episode, schedules/recovery algorithms, authoritative Qt/writer
integration, provider behavior and live/platform checks remain separate gates.

### Actual commands and remaining sequence

Read-only commands completed with exit0: `git status --short`, `git rev-parse HEAD`,
`git log -4 --oneline`, `cat` of the actual updated contract/tests/header/stub,
brief/decision record and root/core/tests CMake/presets, `rg` on specification policy
references, and `tail` on authoring evidence. An independent Python script loaded
the original test blob through `git show`, extracted complete brace-balanced TEST
bodies and asserted all30 original bodies byte-for-byte equal in the37-case candidate;
it passed. That script also ran `git hash-object` on each artifact in the table,
producing the exact hashes recorded above. It executes no C++ test body and does
not compute any expected policy result.

No configure/build/discovery was repeated by this reviewer. The owner's case-author
evidence reports exit0 configure/build and `ctest --preset pause-policy-debug -N -L
pause-policy` listing37, which is discovery only. **No test body or red run occurred
in this review.** Next: coordinator captures a successful exact-revision build and
actual intended failure from PausePolicyNotImplemented, then records baseline status
and remaining PR/CI gates. No implementation, provider, platform or timer-integration
verification is claimed by this approval.

## Final baseline integration review — 2026-10-31

Reviewer `/root/pause_resume_reviewer`; actual candidate
`4a832623bd98ad4e588da3ac47bbee25ef58b44c`, incorporating coordinatorb50d8d8.
**Approved for integration of CORETEST-003's reviewed/red baseline.** No blocking
findings. This verdict covers authored/reviewed tests and intended red, not working
pause behavior. Latest PR CI, review-thread resolution, repository merge rules and
the merge itself remain coordinator gates; no future CI outcome is claimed.

Inspected the baseline handoff and actual967-line CTest log. Independent Python
inspection split all37 started test sections, verified37 GoogleTest RUN entries and
the explicit stub diagnostic in every section, and found no Not Run, skipped or
missing-executable result. There are61 stub-message diagnostics because grouped
validation assertions make multiple calls; this is37 failed cases, not61 cases.
Normal cases report the thrown missing-behavior exception; validation cases report
the same PausePolicyNotImplemented instead of invalid_argument. The final summary
is37/37 failed, consistent with the coordinator's recorded actual exit8 after full
approval was integrated atf39ccc8. Raw-log blob:
`4e49ada47aa51287395fe973a79eb65c47b8019b`. The reviewer inspected the retained
execution evidence and did not repeat the native/red run. Configure/build exit0 and
stable native regression results are attributed to the coordinator's handoff, not
claimed as this reviewer's executions.

Compared current artifact hashes against approved86c0bbd: all37 tests
`646d83f4e38722239bb8b590a9f368141bed4246`, header
`c91b7da3fa1c98a1b2e47ab389f48f74901c2a47`, stubs
`dee2a91c31b524c082192aba70e7e0c0efa27501`, and all four approved root/core/tests
CMake/preset blobs are unchanged. Contract now has blob
`e2eae912418b58f512dfe9505efd75e3453c4636`; its diff changes the proposed/review/red
status prose only, leaving the reviewed behavioral rules intact. CORE-003 brief
changes likewise record completed baseline gates and outstanding implementation/CI.

Checked authoritative progress: CORETEST-003 is done with authored/native-build/
test-review passed, baseline observed_red and product_acceptance not_run; CORE-003
remains planned and unimplemented. This correctly distinguishes baseline completion
from feature acceptance. TIMER-03/05 map entries remain partial, and the scenario
is observed_red with the exact review/artifact references. TEST_STRATEGY keeps the
optional intentional-red lane separate from stable feature CI and requires CORE-003
to implement/pass, obtain separate code review and add/observe automatic pause CI.
The explicit resume and remaining PR20 publication gates are retained in task notes.
Full product scope and separate timer/Qt/provider/platform gates remain explicit.

Independent commands actually run on this Linux review worktree:

| Command | Result |
| --- | --- |
| `python3 tools/check_project.py` | Exit0;377 tasks,122 decisions, links/presets/current HTML pass. Its informational19 draft-credit count is not pause execution evidence. |
| `python3 -m unittest discover -s tests/tooling -v` | Exit0;19/19 pass. |
| `python3 tools/progress.py render --check` | Exit0; snapshot current. |
| `node tools/check_progress_ui.mjs` | Exit0; DOM simulation passes; actual browser/OBS not run. |
| Python raw-log/body-count/hash/status inspection | Exit0; results recorded above. No C++ body execution. |
| `git diff --check` | Exit0 before this evidence append; repeated before commit. |

Read-only file/status/diff searches also inspected the actual handoff, log, task
records, contract/brief changes, strategy and `tests/acceptance-map.json`. Initial
searches for a differently named acceptance map/handoff returned absent paths; the
actual map and authoritative state records were then located and inspected.
Only this review evidence is appended by this reviewer. Required next work is
coordinator publication/CI/merge validation, followed by the separately authorized
dependency-ready task loop; this review neither starts nor approves implementation.
