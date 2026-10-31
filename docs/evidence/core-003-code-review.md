# CORE-003 independent code and CI review

Implementer: `/root/pause_policy_implementer`, author commitc428c45.
Reviewer: `/root/pause_resume_reviewer`, separate agent/session, 2026-10-31.
Actual candidate: `07174f6b869706e0ba214e037bf3c5e0614f9659`, branch
`task/CORE-003-code-review`, based on main01058e2 and coordinator CIe5ee766.
Reviewer owns this evidence file only.

**Verdict: code and workflow approved for the bounded CORE-003 implementation.**
No blocking code or workflow findings. Coordinator final documentation/status,
actual hosted CI for the latest revision, review discussions and repository merge
rules remain gates. This is not a claim that hosted CI has run or passed.

## Exact artifacts and authority

| Artifact | Git blob |
| --- | --- |
| Implementation | `d080ac757bedb1e621420c31aca691a6d25b4052` |
| Unchanged public header | `c91b7da3fa1c98a1b2e47ab389f48f74901c2a47` |
| Unchanged37 acceptance cases | `646d83f4e38722239bb8b590a9f368141bed4246` |
| New pause CI workflow | `79e4dfe102a624bac3c6d8646683b6b0c3874542` |
| Root CMake | `4ba5785aab409908f2c1cd5b30268d1b3be22826` |
| Tests CMake | `2f09cf53e7de9b452c3e3cc5ab18f28a96e4b80e` |
| Presets | `d185c1411987d2cceb09940a092cdac8b8386b68` |

Authority: root/core/tests instructions, architecture/workflow/test strategy, current
task state and CORE-003 brief, specification v0.45 §4/§18 TIMER-03/05, pause-policy
revision1 and owner decision record. Reviewed implementation evidence, the approved
37-case baseline (86c0bbd/final565be7c), and retained red handoff. Implementation
follows that separately reviewed and observed-red gate. Baseline assertions, header
and behavioral contract were preserved; no new dependency or policy is introduced.

## Full source assessment

Read every helper and both public entry points, beyond the37 passing assertions.
`validate_state` rejects negative pending values, own-active-plus-pending and Unknown
pending without notice. Observation then validates both configured delays, enabled
Unknown timeout and its pending threshold, rejects disabled-timeout pending, and
rejects negative elapsed before copying/proposing recovery. Thus an invalid snapshot
cannot be silently repaired by Online/Unknown/client recovery. The setter intentionally
has no configuration and checks only structural invariants, then permits exactly
Manual/Schedule/RecoveryReview. Automatic reasons cannot be overwritten through it.

After validation, accumulated pending lies in [0,delay), or absent means0. Therefore
`delay - accumulated` is representable and nonnegative. Addition occurs only when
elapsed is strictly smaller than that remaining amount, so the result stays below
delay and cannot overflow signed64. Exact threshold, zero and maximum duration
all activate correctly; active reasons return before accumulating further time.
Neither wall time nor floating-point arithmetic appears.

Unknown sets notice and resets only pending Offline, preserving established Offline.
Enabled Unknown advances only its own pending/reason. Either known status clears
Unknown notice/reason/pending; Online separately clears Offline, while Offline
advances a fresh or existing Offline episode. Client connection processing runs
independently afterwards and changes only its own signal reason. Other external
holds survive; pending signals advance despite existing countdown holds. Idempotent
sets and absent active counters prevent duplication/stale timeout reuse. This matches
both owner choices and independent recovery composition without global resume.

Const inputs, owned local copies and return-by-value proposals preserve caller state
on validation/allocation exceptions; there are no clock reads, global state, Qt,
provider/network, persistence or authoritative writes. Invalid out-of-domain enum
values and changing episode configuration remain outside the admitted contract;
the code's fallback branch is not a newly supported provider-status policy.
The interface remains a final segment snapshot: timer callers still must split at
signal changes and expiry, rather than retroactively pause a pending interval.
No timer integration, countdown phase/hard-end wiring or live behavior is certified.

## CI and publication assessment

The new workflow mirrors the selected timer lane: PRs targeting main, pushes to main
and manual dispatch, no path filters; Ubuntu24.04,15-minute timeout, contents-read
permissions, pinned checkout with persisted credentials disabled. Configure uses the
same pause-policy preset and explicit opt-in to the existing pinned GoogleTest
fetch; build and CTest use that preset and pause-policy label. Existing dependency
loader first accepts installed/local GoogleTest and otherwise fetches the pinned
commit. No skip, WILL_FAIL, continue-on-error or failure inversion is present.
Replacing intentional-red with core labels and changing display/option wording
does not change cases, assertions or execution selection. Local results below justify
this becoming a normal passing lane. Repository required-check enforcement and actual
hosted runner/dependency execution remain coordinator checks.

Publication finding (status documentation): the current contract/CORE-003 brief
still describe unconditional stubs and pending implementation; some historical
sentences still say awaiting full review. Update these status-only passages in the
coordinator's final integration record, retaining baseline history and behavior.
This does not block the reviewed source/workflow approval, but must be resolved
before declaring final implementation documentation current.

## Independent executions

Linux review worktree `local-evidence/worktrees/core003-review`; GNU16.2.1,
Python3.14.8, CMake4.4.5, Ninja1.13.2, installed GoogleTest1.18.0. No download/live
provider operation. All commands below exit0:

| Commands | Actual result |
| --- | --- |
| `cmake --preset pause-policy-debug`; `cmake --build --preset pause-policy-debug`; `ctest --preset pause-policy-debug -L pause-policy` | Clean configure/build,37/37 pass; labels pause-policy/acceptance/core. |
| `ctest --preset pause-policy-debug -L scaffold` | 3/3 pass, including19 Python tooling tests and project consistency. |
| `cmake --preset timer-lifecycle-debug`; `cmake --build --preset timer-lifecycle-debug`; `ctest --preset timer-lifecycle-debug -L timer-lifecycle` | Clean configure/build,39/39 adjacent timer cases pass. This is separate-suite regression evidence, not timer/pause integration. |
| `python3 tools/progress.py render --check` | Snapshot current. |
| `node tools/check_progress_ui.mjs` | DOM simulation passes; actual browser/OBS not run. |
| `clang-format --dry-run --Werror core/src/pause_policy.cpp` | Pass. |
| `git diff --check` | Pass before evidence write; repeated before commit. |

Read-only git/file inspections established exact hashes and reviewed diffs. An initial
lookup for core-003-authoring.md found no file; actual author evidence is
core-003-implementation.md and was read. Other stable native suites were reported
passing by the author/coordinator; this reviewer did not repeat those unrelated lanes.
No new assertions were needed to correct a discovered defect; no source/test/build/
contract/state edits were made. Remaining release/platform/provider, schedule/recovery,
delay reconfiguration and authoritative-service gates remain explicit and unchanged.
