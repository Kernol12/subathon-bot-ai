# CORETEST-002 authored timer baseline draft

Author: `/root/numeric_test_author`; date: 2026-10-31.
Branch: `task/CORETEST-002-timer-author`, from integrated main `806f69f`
(CORE-001 PR13). Persistent isolated author worktree:
`local-evidence/worktrees/coretest002-author`.

## Scope, ownership and current gate

The [timer lifecycle revision1 draft](../contracts/timer-lifecycle-v1.md) refines
specification v0.45 §4/TIMER-01/02 with pure clock-step and command proposals.
It separates lifecycle/phase, retains shared Grace without a duplicate budget field,
uses separate elapsed/UTC clock domains, preserves ordered Main/Grace transitions
across overshoot and gives hard end deterministic observation priority even paused.
The source contains only three unconditional `TimerLifecycleNotImplemented` throws.

Thirty-three focused tests derive durations and ordered traces manually, including
default/custom/zero Grace, Main and Grace exact expiry/overshoot, Fresh/Shared rescue,
pauses, confirmation wait/authorized confirmation, hard-end ordering, domain errors,
large signed64 durations and no ordinary implicit reopening. No production algorithm,
clock access, SQL, provider, review transaction or permission service was implemented.

**This remains a draft:** the owner question about positive credit while awaiting
confirmation is pending. No expected result for that case has been invented. The
contract/test baseline cannot be finalized or claimed ready until the decision is
recorded, the actual final revision is independently reviewed and intended red is
observed. First-start/passed-deadline behavior remains explicitly assigned to
API-007/CORE-008; Ready rejection is a bounded API precondition, not product policy.

Author owns the header/stub/tests, contract, CORE-002 brief and this evidence only.
Coordinator owns state/HTML/map/PR and all root/shared build/preset files. Author
consumed coordinator harness commit `2c44567` as `39463cb` without modifying it.
Initial header/stub/30-case checkpoint is `68e0119`; the final draft adds three
zero/custom-Grace boundary cases and the contract/brief/evidence.

## Draft artifact identities

| Artifact | Authored Git blob |
| --- | --- |
| `core/include/subathon/core/timer_lifecycle.hpp` | `69cad7023b9928371877d45955b800aed9996cd5` |
| `core/src/timer_lifecycle.cpp` explicit stubs | `5584f845faef1dff68377363ec04c7b2effc4cae` |
| `tests/acceptance/timer_lifecycle_tests.cpp` | `5fdcde0c4a5fecb6d3b7ea7b63ecbb89a9517f1f` |
| `docs/contracts/timer-lifecycle-v1.md` draft | `b88657ce48447e7d509d683a0b5c89cea567ac00` |

These identities record an authored draft; they are not self-approved review evidence.
A policy answer or review correction changes the affected artifact and requires
review of its actual final revision before red/implementation.

## Commands actually run

| Command | Actual result |
| --- | --- |
| `c++ -std=c++20 -Icore/include -fsyntax-only core/src/timer_lifecycle.cpp tests/acceptance/timer_lifecycle_tests.cpp` | exit0; before the three additional cases |
| `c++ -std=c++20 -Icore/include -fsyntax-only tests/acceptance/timer_lifecycle_tests.cpp` | exit0; zero-boundary additions |
| `cmake --preset timer-lifecycle-debug` | exit0; GCC16.2.1/Python3.14.8 and installed GoogleTest discovered |
| `cmake --build --preset timer-lifecycle-debug` | exit0;10 compile/link/discovery steps, all33 authored cases compile |
| `python3 tools/check_project.py` | exit0;377 task records,122 decisions, links/presets/current HTML pass |
| `python3 tools/progress.py render --check` | exit0; current coordinator snapshot |
| `git diff --check` | exit0 |
| `c++ -dumpfullversion`, `pkg-config --modversion gtest` | exit0;16.2.1 and1.18.0 |

No timer test body was executed. Build-time discovery lists case names and is not a
red/pass result. No missing dependency, failed compilation or skipped assertion is
counted as a valid red baseline. Coordinator must perform the actual reviewed
`ctest --preset timer-lifecycle-debug -L timer-lifecycle` run after approval.

## CI assessment and handoff

The coordinator's optional timer lane keeps this draft intentional-red suite separate
from the stable scaffold/credit/numeric workflows. No author workflow changes or
failure inversion occurred. CORE-002 must add stable passing lifecycle acceptance to
automatic PR/main CI and observe it before implementation completion. This draft
compilation establishes no server/runtime, platform/renderer/UI, package or live/OBS
verification. Next: resolve owner policy, review actual final contract/tests/harness,
observe intended stub-red, update central evidence and publish/merge the focused
baseline only after applicable gates. All release scope remains in its task records.

## Additive response to the partial independent review

Author: `/root/timer_test_fixes`; date: 2026-10-31. Branch:
`task/CORETEST-002-review-fixes`, isolated worktree
`local-evidence/worktrees/coretest002-fixes`, based on `6c52b3c` (the preserved
33-case author draft plus coordinator harness). The reviewer is the separate
`/root/baseline_reviewer`; its partial request-changes record is preserved in
commit `a99616a`, `docs/evidence/coretest-002-independent-review.md`.

This response adds exactly two draft cases, bringing the authored total to 35.
All existing tests and assertions are preserved; no production, header, contract,
brief, build or shared-progress changes are included.

- `IncoherentPhaseAndConfirmationWaitSnapshotsAreRejected` exercises the local
  invariants already declared in revision 1's validation section: Grace with
  positive Main, a confirmation-wait flag with positive Main, and a wait flag in
  Grace reject at advance, credit and confirmation boundaries. Credit uses zero
  against these malformed snapshots, without choosing any behavior for valid
  confirmation-wait credit. A valid waiting snapshot rejects advance under each
  declared non-Confirmation end mode. Each rejection expects invalid_argument.
- `ExhaustedSharedBudgetExpiresAtMainZeroWithoutAnArtificialDelay` derives from
  §4/TIMER-02 and the contract's retained-budget/zero-boundary rule. Main 1000 with
  retained Shared Grace 0 and configured positive Grace 120000 receives elapsed 1000;
  it finishes immediately with Main 0/Grace 0 and exactly MainExpired → GraceStarted
  → GraceExpired → Finished. No elapsed overshoot or artificial grace tick is used.

The revised test blob is `7a802f2151da1d62b85f27c357528f63426cf2d0`.
The contract remains `b88657ce48447e7d509d683a0b5c89cea567ac00`, header remains
`69cad7023b9928371877d45955b800aed9996cd5`, and source remains the three explicit
unimplemented stubs at `5584f845faef1dff68377363ec04c7b2effc4cae`.

Commands actually run in this worktree, all exit 0:

| Command | Actual result |
| --- | --- |
| `cmake --preset timer-lifecycle-debug` | configured the isolated draft lane |
| `cmake --build --preset timer-lifecycle-debug` | 10 compile/link/discovery steps; all 35 cases compile |
| `ctest --preset timer-lifecycle-debug -L timer-lifecycle -N` | discovered exactly 35 cases; no bodies executed |
| `git diff --check` | passed |
| `cmake --version`, `c++ -dumpfullversion`, `pkg-config --modversion gtest`, `uname -s -m` | CMake 4.4.5, GCC 16.2.1, GoogleTest 1.18.0, Linux x86_64; configure reports Python 3.14.8 |

`git hash-object` recorded the exact artifact identities above. The separate
reviewer must review the additions; this author provides no independent approval.
No timer test body or intended-red run was performed. The owner decision on
positive credit while awaiting confirmation remains unanswered and blocking full
baseline approval; first-start/deadline policy remains the API-007/CORE-008 gate.
CI assessment is unchanged: the optional intentional-red lane is suitable for
draft compilation/discovery, and CORE-002 still requires passing automatic
lifecycle CI before implementation completion. The coordinator owns central
state/HTML, final review integration, PR publication and the approved red run.

## Confirmation-wait credit clarification: additive author handoff

Author: `/root/timer_policy_author`; date: 2026-10-31. Branch:
`task/CORETEST-002-confirmation-credit`, isolated worktree
`local-evidence/worktrees/coretest002-confirmation`, based on coordinator commit
`21c27bf` (seeded from refreshed main `8ad458e`). The coordinator retains ownership
of shared build/presets, progress/state, generated HTML, review/red integration and
PR publication. This author owns only the timer test file, draft lifecycle contract,
CORE-002 brief and this appended evidence section.

The [owner decision](timer-confirmation-credit-decision.md), now recorded in
specification v0.45 §4/§18, chooses automatic Main resumption for positive actual
time during confirmation wait. It supersedes the pending-policy statements in the
historical author sections above. Revision 1 has never been finalized or implemented;
this refinement changes no public signature, C++ layout, schema or persisted format
and requires no migration. Independent full review and intended stub-red remain
pending. This author provides no independent approval.

Exactly four focused cases are added, bringing the authored total from 35 to 39.
Their explicit snapshots and expectations are manually derived from the recorded
policy and existing observation/validation rules:

- `PositiveActualCreditResumesConfirmationWaitAndRejectsStaleConfirm`: waiting
  Active/Main0 with retained Grace73000 receives actual5000, becoming
  Active/Main5000/Grace73000 with the wait cleared. A stale confirmation rejects
  under its existing waiting-state precondition.
- `ZeroActualCreditRetainsConfirmationWaitAndBothClocks`: the same waiting state
  receives actual0 and retains Active/Main0/Grace73000 and its wait. These inputs
  are actual amounts; eligibility, input review, debt and cap calculations remain
  the caller's responsibility, so a requested positive award need not resume Main.
- `ResumedConfirmationWaitDoesNotOverrideAnExternalPause`: actual5000 resumes
  Main; the next paused elapsed1000 observation retains Main5000/Grace73000,
  clears no external pause reason and produces no transition proposals.
- `HardEndObservationPreventsCreditFromResumingPausedConfirmationWait`: the caller
  first advances waiting/paused Main0/Grace73000 at exact enabled UTC deadline1000.
  The result is Finished with the wait cleared and retained remaining fields,
  emitting exactly HardEndReached → Finished. Subsequent actual5000 rejects,
  so ordinary credit cannot reopen it or override hard-end precedence.

All 35 previous test bodies remain byte-for-byte identical to the seed revision.
The header remains `69cad7023b9928371877d45955b800aed9996cd5`; the source remains
`5584f845faef1dff68377363ec04c7b2effc4cae`, containing three unconditional
`TimerLifecycleNotImplemented` throws. No production behavior, external pause
management, cap/debt algorithm, clock access or hard-end configuration change was
implemented. Root/shared build files, central state/HTML and the specification were
not edited by this author.

Commands actually run in the isolated worktree:

| Command | Actual result |
| --- | --- |
| `cmake --preset timer-lifecycle-debug` | exit0; GCC16.2.1, Python3.14.8; configured the existing optional lane |
| `cmake --build --preset timer-lifecycle-debug` | exit0; all10 compile/link/discovery steps complete |
| `ctest --preset timer-lifecycle-debug -L timer-lifecycle -N` | exit0; exactly39 cases listed, including the four additions; no test bodies executed |
| `python3 tools/check_project.py` | exit0;377 task records,122 decisions, local links/presets/current HTML pass; no timer bodies run |
| `python3 tools/progress.py render --check` | exit0; current coordinator snapshot; no generated/shared file edited |
| `git diff --check` | exit0 before this evidence append; rechecked before commit |
| `git hash-object tests/acceptance/timer_lifecycle_tests.cpp docs/contracts/timer-lifecycle-v1.md docs/tasks/EPIC-001/CORE-002.md core/include/subathon/core/timer_lifecycle.hpp core/src/timer_lifecycle.cpp` | exit0; exact artifacts recorded below |
| `cmake --version`, `c++ -dumpfullversion`, `pkg-config --modversion gtest`, `uname -s -m` | exit0; CMake4.4.5, GCC16.2.1, GoogleTest1.18.0, Linux x86_64 |

| Authored artifact | Exact Git blob |
| --- | --- |
| `tests/acceptance/timer_lifecycle_tests.cpp` (39 cases) | `bf8c1f46d2fe8e6fdbd1ce2ed5841e679dc4a1a7` |
| `docs/contracts/timer-lifecycle-v1.md` (draft revision1) | `09c7c77d2e42b6cc5971f8bff8962c0486a84082` |
| `docs/tasks/EPIC-001/CORE-002.md` | `be531e574ba2be496d535a6c3a236f55744e90d0` |

A read-only Python comparison against `HEAD` also confirmed exactly39 cases,
byte-for-byte retention of all35 old test bodies, and unchanged header/stub,
root/tests CMake, presets, state, HTML and specification. Compilation/discovery
establishes neither a red baseline nor passing timer behavior. No timer test body
or production test was executed. After separate review of the actual final
contract/tests/harness, the coordinator must observe and record the genuine
`TimerLifecycleNotImplemented` red run before CORE-002 implementation.

CI assessment is unchanged: this authored intentional-red lane remains optional;
CORE-002 must introduce and observe passing automatic lifecycle CI before
implementation completion. No SQL/Qt/provider/platform/live verification is claimed.
