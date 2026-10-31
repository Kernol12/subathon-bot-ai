# CORE-002 bounded timer implementation

Date: 2026-10-31. Implementer: `/root/timer_implementer`, separate from the
baseline authors/reviewers. Branch: `task/CORE-002-implementation`; isolated
worktree: `local-evidence/worktrees/core002-implementation`. Started from claim
`04cd539079eb0be50e62878d57a2930e06ce456b`, based on integrated main `419d855`.
Authority: specification v0.45 §4/§18, TIMER-01/02 and
[timer lifecycle revision 1](../contracts/timer-lifecycle-v1.md).

## Baseline and scope

Before implementation, read the root/core instructions, first-assignment guidance,
architecture/workflow/test strategy, specification, event/state boundaries revision 1,
timer contract/task brief and the exact
[reviewed/red baseline](coretest-002-baseline-handoff.md) with its
[final independent review](coretest-002-final-independent-review.md). The baseline
records independent approval `2fb544f`, integration at `cd59601`, and the subsequent
actual 39-case `TimerLifecycleNotImplemented` failure. Its final integration review
is `26e40f9`. No new intended-red run was needed for the unchanged baseline.

Owned changes are only `core/src/timer_lifecycle.cpp` and this evidence file.
The coordinator owns build/preset labels, automatic timer CI, task/progress state,
acceptance map, architecture/status updates and PR integration. No baseline test,
header, contract, build file or shared state was edited. No test defect or required
baseline change was identified.

| Artifact | Git blob after implementation |
| --- | --- |
| Pure reducer source | `61384a59309767a8d2cb86ad08667b09e32086c7` |
| Unchanged 39 reviewed test cases | `bf8c1f46d2fe8e6fdbd1ce2ed5841e679dc4a1a7` |
| Unchanged public header | `69cad7023b9928371877d45955b800aed9996cd5` |
| Unchanged completed-baseline contract revision 1 | `cfee0767cc0829106057b2cf631a03e2e59cca30` |

## Result and implementation decisions

All three pure value proposals are implemented with the standard library only.
Shared snapshot validation rejects negative clocks, Grace with nonzero Main and
incoherent confirmation waits. Advance additionally validates elapsed/configured
grace duration, Confirmation mode for a waiting snapshot, and the already-started
lifecycle precondition before producing a proposal.

A Finished snapshot is retained without transitions. For Active snapshots, an
observed enabled hard end finishes before countdown progression, clears the wait
and retains pre-call clock fields. UTC is compared directly to the deadline and
never used as elapsed duration. Paused intervals use zero consumable elapsed, so
already-zero phases resolve without consuming paused time. Main consumption then
Grace consumption uses bounded subtraction, never a Main-plus-Grace sum. Fresh
reloads configured Grace at each Main expiry; Shared retains its unused budget.
Overshoot crosses both phases in one call and retains the ordered four-transition
trace. Immediate and Confirmation endings return without consuming Grace.

Positive already-resolved actual credit resumes Main from Grace or confirmation
wait while preserving the retained Grace budget. Zero preserves the entire
snapshot. A checked Main addition rejects signed64 overflow. Ready/Finished credit,
negative credit and stale/invalid confirmation reject; ordinary commands cannot
start or reopen a timer. All input values remain const and proposals are local.

## Commands actually run

Environment: Linux x86_64, `/usr/bin/c++` GCC 16.2.1 (20260810), CMake 4.4.5,
Ninja 1.13.2, installed GoogleTest 1.18.0, Python 3.14.8 and clang-format 23.1.4.
The numeric regression lane found installed Boost 1.92.0. No dependency was added
or fetched. Every command below ran in the isolated implementation worktree and
exited 0. Configure/build/test commands were sequential within each lane.

| Commands | Observed result |
| --- | --- |
| `cmake --version`; `c++ --version`; `ninja --version`; `uname -s -m`; `pkg-config --modversion gtest`; `python3 --version`; `clang-format --version` | Actual versions above |
| `clang-format -i core/src/timer_lifecycle.cpp` | Repository formatting applied before build |
| `cmake --preset timer-lifecycle-debug`; `cmake --build --preset timer-lifecycle-debug`; `ctest --preset timer-lifecycle-debug -L timer-lifecycle` | Successful configure/build; all 39 reviewed bodies passed, no skips or failure inversion |
| `cmake --preset scaffold-debug`; `cmake --build --preset scaffold-debug`; `ctest --preset scaffold-debug` | 3/3 passed, including Python tooling and project consistency |
| `cmake --preset acceptance-debug`; `cmake --build --preset acceptance-debug`; `ctest --preset acceptance-debug -L 'acceptance\|regression'` | 23/23 existing credit acceptance/regression cases passed |
| `cmake --preset exact-numeric-debug`; `cmake --build --preset exact-numeric-debug`; `ctest --preset exact-numeric-debug -L exact-numeric` | 33/33 existing exact-numeric acceptance/regression cases passed |
| `cmake --preset receipt-intake-debug`; `cmake --build --preset receipt-intake-debug`; `ctest --preset receipt-intake-debug -L receipt-intake` | 15/15 existing receipt simulation cases passed |
| `clang-format --dry-run --Werror core/src/timer_lifecycle.cpp`; `git diff --check`; `git hash-object` over the four artifacts above; `git diff --stat`; source diff inspection | Formatting/whitespace checks passed; exact artifacts recorded above |

The timer preset/test labels still say `intentional-red` at this implementer-owned
revision. The actual run passed all 39 bodies; coordinator-owned metadata/CI changes
must represent that passing behavior without inverting or skipping any result.

## Remaining integration gates and limits

This is deterministic pure-domain simulation evidence. Independent code review of
the actual implementation, automatic timer PR/main CI and latest-revision CI/PR
integration remain coordinator gates before CORE-002 completion. The implementer
does not self-certify either independent review gate or update task completion.

There is no Qt queue/writer, durable transaction/event ancestry, real pause-reason
set, start/reopen command, recovery, clock-discontinuity/schedule/DST, provider,
renderer, packaging or live/platform verification here. All existing downstream
tasks and full release scope remain required. No clock read, sleep, network,
credential access or live contribution/chat effect occurs in the reducer or tests.
