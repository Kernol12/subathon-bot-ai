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
