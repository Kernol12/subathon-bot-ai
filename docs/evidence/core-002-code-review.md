# CORE-002 independent implementation and CI review

Date: 2026-10-31. Implementer: `/root/timer_implementer`; CI/configuration author:
coordinator `/root`; independent reviewer: `/root/timer_final_reviewer`.

Verdict: **approved for bounded code and CI configuration** at the exact revisions
below. No blocking findings. The reviewer independently ran all 39 unchanged timer
cases and relevant stable suites successfully. Hosted workflow execution,
latest-revision PR checks, final status integration and merge eligibility remain
coordinator gates; this is not a claim of observed GitHub CI success.

## Actual revision, authority and ownership

Reviewed integrated candidate `f2e5a4622cfaa89c83148bbb63fd4464e057c509`, including
implementer commit `c40eea24e6000fe66667a3a45a11a928e353cc8e` and coordinator
CI commit `1a355878a932c4bfc1298d05efb56de0ef759beb`. Review branch:
`task/CORE-002-code-review`; isolated worktree:
`local-evidence/worktrees/core002-review`; integrated base main `419d855` (PR14).
This reviewer owns only this file and changed no source, baseline, contract,
configuration, shared status or generated progress.

Authority: specification v0.45 §4/§18, TIMER-01/02; timer lifecycle revision 1;
event/state boundaries revision 1; root/core/tests instructions; architecture,
workflow, test strategy, first-assignment preparation and CORE-002 brief. The
[39-case baseline](coretest-002-baseline-handoff.md) and
[independent test/integration review](coretest-002-final-independent-review.md)
establish approval before the actual stub-red at `cd59601`. The owner-selected
confirmation-credit policy is recorded in both the specification and
[decision](timer-confirmation-credit-decision.md). Those completed preparation
gates were preserved, not repeated or inferred from the passing implementation.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| `core/src/timer_lifecycle.cpp` | `61384a59309767a8d2cb86ad08667b09e32086c7` |
| Unchanged public header | `69cad7023b9928371877d45955b800aed9996cd5` |
| Unchanged 39-case tests | `bf8c1f46d2fe8e6fdbd1ce2ed5841e679dc4a1a7` |
| Unchanged timer contract revision 1 | `cfee0767cc0829106057b2cf631a03e2e59cca30` |
| `.github/workflows/timer-lifecycle.yml` | `5c34da5f67921214a28ce5d41eff3b23bc90b9b3` |
| Root `CMakeLists.txt` | `e79fe4da9c76577ca65d70334f2ebf7510c148b3` |
| `core/CMakeLists.txt` | `346731380931324ef20b2272f21373c00cc6e9b7` |
| `tests/CMakeLists.txt` | `091b5bab9be4e90aff6c0f69eff8f74f63044e6c` |
| `CMakePresets.json` | `58011859775817a027f3f4b216f65e30eb5b38cb` |
| Unchanged GoogleTest dependency selection | `ecedcf7a9f7084d9af7d5f12b9cca2612283fb39` |
| Implementer evidence | `ca9e120b14b58f8f70186d568723658f1c69b43f` |

`git diff --quiet 419d855 HEAD -- core/include/subathon/core/timer_lifecycle.hpp
tests/acceptance/timer_lifecycle_tests.cpp docs/contracts/timer-lifecycle-v1.md`
exited0: the actual reviewed public boundary, assertions and normative contract
remain unchanged. No fixture, expected value or baseline assertion was weakened.

## Source review beyond the 39 assertions

Shared validation runs at every public proposal boundary. It rejects negative Main
and Grace, Grace with nonzero Main, and a confirmation flag outside Active/Main0.
Advance additionally rejects negative elapsed/configured Grace, waiting snapshots
under a non-Confirmation mode, and Ready. Validation precedes early returns and
arithmetic, including paused/Finished paths; invalid input is not silently repaired.
Enum representations outside the declared alternatives remain outside the admitted
domain, as the contract states; no new invalid-enum capability is claimed.

For valid Finished snapshots, advance copies the snapshot and returns no transitions,
before applying any deadline or elapsed projection. Active snapshots compare the
separate signed64 UTC instant directly to the optional deadline; no timestamp
subtraction or conversion into elapsed duration occurs. A reached deadline clears
the wait, retains the pre-call fields and produces exactly HardEndReached → Finished
before normal progression, including pause and confirmation wait. There are no
fabricated historical transition instants or wall/monotonic interpolation.

The pause observation makes consumable elapsed zero. Positive clocks therefore stay
unchanged while paused, while already-zero Main/Grace still resolve their existing
expiry. Main consumption either subtracts a smaller nonnegative elapsed value or
subtracts Main from an elapsed value known to be at least Main. Immediate finishes
and Confirmation waits without consuming Grace; both return before the Grace path.
Grace mode selects Fresh's configured budget or retains Shared's remaining budget,
then consumes only remaining overshoot. Zero Fresh/Shared budget expires without
another tick. Ordered proposals retain the complete four-transition path when one
step crosses Main and all Grace. Waiting observations return without repeated
MainExpired/ConfirmationRequired proposals.

Every countdown subtraction is bounded by a prior comparison of validated
nonnegative signed64 values. No Main+Grace sum is formed, so the maximum Main,
Grace, configured duration and elapsed values do not introduce signed overflow.
Credit first rejects negative amounts and non-Active lifecycle, then preserves the
whole snapshot for zero. Its overflow condition compares actual against
INT64_MAX minus validated Main before addition; neither that difference nor the
accepted sum can overflow. A positive amount switches to Main and clears waiting
without resetting or consuming Grace. In Grace and confirmation wait, the validated
Main is zero, so the resulting Main equals the actual amount exactly.

Confirmation accepts only the coherent waiting state: shared validation establishes
Active/Main0 and the explicit flag is then required. Positive-credit resumption
clears that flag, so stale confirmation rejects. Ready and Finished cannot accept
credit or confirmation, and ordinary commands cannot reopen. The credit helper has
no UTC/configuration parameter: observing expiry/hard end first and resolving actual
eligibility/caps/debt remain explicit caller duties, exercised by the sequence tests.
The same boundary does not fabricate input-review or permission decisions.

All state/configuration/clock inputs are const; mutations and vector proposals are
local values returned to the caller. There is no Qt, SQL, network/provider code,
system-clock access, sleep, persistence, global mutable state or authoritative write.
The code cannot clear a real pause-reason set because it owns none. The accepted
credit plus next paused observation correctly preserves the bounded independent
pause contract. No new dependency or algorithm outside the assigned slice was added.
No required test/contract correction or uncovered implementation defect was found.

## CI and harness review

The new workflow runs on every PR targeting main, every main push and manual dispatch,
without path filters. It grants only contents:read, disables checkout credential
persistence, pins checkout to the existing repository SHA
`3d3c42e5aac5ba805825da76410c181273ba90b1`, uses ubuntu-24.04 and a15-minute timeout.
Its configure/build/CTest steps invoke the same timer preset and exact
`-L timer-lifecycle` suite used locally. Explicit GoogleTest fetch permission follows
existing stable lanes; dependency selection still prefers an installed compatible
package and otherwise fetches pinned commit
`063de7e9578f82b369302001269680b4b1553359`. No dependency pin was altered.

CTest still discovers the same39 test bodies against the real timer library and
GoogleTest main. The label change removes intentional-red and adds core; selection
by timer-lifecycle remains identical. Root option/preset descriptions and the core
target comment now describe implemented behavior; no target or assertion is disabled.
There is no WILL_FAIL, skipping, result inversion or continue-on-error. Existing
scaffold, credit, numeric and receipt lanes remain unchanged. The automatic timer
configuration is appropriate for this now-passing deterministic suite, but its actual
hosted execution and any required-check/ruleset configuration are not verified here.

## Reviewer commands and observed results

Environment: Linux x86_64; GCC16.2.1; CMake4.4.5; Ninja1.13.2; Python3.14.8;
installed GoogleTest1.18.0; clang-format23.1.4. The numeric lane selected installed
Boost1.92.0. Local GoogleTest fetch was OFF; these runs do not reproduce or claim
the separate Ubuntu/pinned-fetch environment.

All configure/build/CTest commands below ran sequentially within each lane against
the exact integrated candidate, with exit0 and no skipped test bodies.

| Commands | Observed result |
| --- | --- |
| `cmake --preset timer-lifecycle-debug`; `cmake --build --preset timer-lifecycle-debug`; `ctest --preset timer-lifecycle-debug -L timer-lifecycle` | 39/39 pass; labels acceptance/core/timer-lifecycle |
| `cmake --preset scaffold-debug`; `cmake --build --preset scaffold-debug`; `ctest --preset scaffold-debug` | 3/3 pass |
| `cmake --preset acceptance-debug`; `cmake --build --preset acceptance-debug`; `ctest --preset acceptance-debug -L 'acceptance\|regression'` | 23/23 pass |
| `cmake --preset exact-numeric-debug`; `cmake --build --preset exact-numeric-debug`; `ctest --preset exact-numeric-debug -L exact-numeric` | 33/33 pass |
| `cmake --preset receipt-intake-debug`; `cmake --build --preset receipt-intake-debug`; `ctest --preset receipt-intake-debug -L receipt-intake` | 15/15 simulation cases pass |
| `python3 -m unittest discover -s tests/tooling -v` | 19/19 pass |
| `node tools/check_progress_ui.mjs` | DOM simulation passes; actual browser/OBS explicitly not run |
| `python3 tools/check_project.py`; `python3 tools/progress.py render --check` | 377 task records,122 decisions, links/presets/current HTML pass |
| `clang-format --dry-run --Werror core/src/timer_lifecycle.cpp`; `git diff --check` | Formatting and whitespace pass; staged evidence checked before commit |
| `git status --short`, `git log -6 --oneline`, revision/blob queries, full source/config diff and named-file reads; compiler/tool version and relevant CMake cache inspection | Actual revisions/artifacts/environment recorded above; candidate clean before evidence authoring |

No live contribution/chat effect, credential, provider endpoint, real database or
external service was used. Passing tests establish this bounded pure-domain behavior,
not correct Qt queue/writer ownership, atomic accounting/event commits, real pause
reason storage, first-start/reopen, signed corrections/configuration changes,
recovery/clock-discontinuity/schedule/DST, renderers, supported-platform packages or
live/OBS behavior. Those downstream tasks and all release scope remain required.
