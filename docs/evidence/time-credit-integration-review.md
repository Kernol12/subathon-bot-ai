# Independent integration review — initial test preparation

Date: 2026-10-30. Artifact authors: `/root/test_author` and coordinator `/root`.
Independent reviewer: `/root/reviewer`; separate from both authors.
Specification v0.45, sections 17/18 and decisions TEST-04/05; time-credit contract
revision 1. This follows [REVIEW-001](time-credit-independent-review.md).

**Verdict: approved for integration of test-preparation artifacts only.** The
19-case C++ baseline remains unchanged and its captured intended failures support
the handoff. The legitimate tooling fixture correction is separately approved
below. This is not approval of product behavior or permission to bypass required
final checks, CI, review discussions or branch protection. The coordinator still
must record final integrated checks and publication status accurately.

## Reviewed integration scope

Read-only inspection used the coordinator worktree on
`task/TEST-002-reviewed-credit-baseline`, with observed-red execution commit
`f30e293` and the current documentation/progress changes. The reviewer edited
only their own `/tmp/subathon-review` worktree.

Reviewed paths: README.md, docs/ARCHITECTURE.md, docs/CLION_SETUP.md,
docs/TEST_STRATEGY.md, tests/acceptance-map.json, progress/state.json and generated
PROGRESS.html; plus docs/evidence/time-credit-baseline-handoff.md and
docs/evidence/time-credit-red-ctest.txt. The contract, test source, header and
production stub were compared with the exact approved Git blobs in REVIEW-001.
The test blob is still `055dec680f6ded7c457d14daf088115a0d73fa9f`; the unchanged
stub is `b491b86ec83cde41463ef5d7a61b92d78fa9012e`.

No drift, removed assertion, test skipping or changed production/test-runner
configuration was found. `core/src/time_credit.cpp`, root CMakeLists.txt,
tests/CMakeLists.txt and tools/progress.py have no diff against main `9982001`.
The acceptance map marks the narrow scenario observed_red and keeps full
decisions pending; it explicitly excludes persisted effects/refund verification.
README/setup/architecture/strategy describe reviewed preparation and unimplemented
behavior. Shared state leaves IMPL-001 unstarted and real SPIKE-001 blocked/partial.

## Red evidence assessment

The reviewer inspected the captured CTest output rather than rerunning acceptance:

- All 19 named cases ran and failed; the summary reports zero percent passed and
  19 failures out of 19, without skipped/disabled/missing-executable indications.
- Twelve valid-result cases report the explicit unimplemented exception.
- Seven error cases report `subathon::core::FeatureNotImplemented` where the
  reviewed assertions require `std::invalid_argument` or `std::overflow_error`.
- Successful configuration/build and CTest exit 8 are recorded separately in the
  coordinator handoff. The captured log is consistent with missing stub behavior,
  rather than compilation or dependency failure. The reviewer did not independently
  observe that process exit or replay the command.

A one-off independent parser confirmed those 12+7 reasons and the summary. Its
first diagnostic attempt expected a different GoogleTest wording and failed its
own matcher; inspecting the actual `Actual: it throws ...FeatureNotImplemented`
text and correcting the matcher produced the confirmed result. No artifact or
test was modified for that diagnostic correction.

This evidence is an intended red baseline, not passing product acceptance. The
handoff preserves future lifecycle/debt/hard-end, lowered-cap policy, transaction,
refund, rounding, provider and platform gates. The separate feasibility draft PR
does not establish actual Drogon startup, callbacks or shutdown.

## Separate tooling fixture correction review

Author `/root/test_author` supplied commit `42229b3`; the reviewer examined
equivalent prerequisite `c841753` in their own worktree. Exact reviewed tooling
source blob: `970c2c5dc738dacb384c46cf42e3df0a87439d92`.
Changed paths are only tests/tooling/test_progress.py and
[progress-fixture-correction.md](progress-fixture-correction.md).

The original `test_cannot_start_implementation_before_red_baseline` depended on
live IMPL-001/TEST-002 remaining incomplete forever. TEST-04 and section 17 permit
implementation after reviewed intended red; validating an in-progress IMPL-001
therefore correctly succeeds when TEST-002 is genuinely done. The original test
then expected an error for a valid state. It was a fixture defect, not a reason to
roll back evidence or weaken the validator.

The corrected negative case explicitly constructs planned synthetic TEST-999 and
in-progress IMPL-999 depending on it, retaining the original
`assertRaisesRegex(ValueError, "dependency")`. The complementary positive case
uses a done synthetic baseline with observed_red, evidence and distinct approved
author/reviewer fields and requires validation to succeed. Unrelated activity is
cleared to keep the synthetic task set self-contained. The evidence path and
review identities are structural test fixtures, not real verification claims.

This preserves the prohibition before the gate and verifies admission after it.
No production validator, C++ test, original C++ oracle or product behavior changed.
The written specification-based reason and this separate review satisfy TEST-05's
legitimate-test-correction requirement. No blocking correction request remains.

## Checks actually performed by this reviewer

Read-only checks in the coordinator worktree:

| Check | Observed result |
| --- | --- |
| Baseline Git blob comparison and `git diff 9982001` for stub/build/validator paths | Match; no production/test-runner changes |
| Corrected one-off red-log parser | 12 explicit exceptions + 7 wrong-exception assertions; 19 executed |
| `python3 tools/check_project.py` | Exit 0; 375 task records, 122 decisions, links/current HTML |
| `python3 tools/progress.py validate` | Exit 0; 375 records valid |
| `python3 tools/progress.py render --check` | Exit 0; snapshot current at inspection |
| `git diff --check` | Exit 0 |

Checks in the reviewer's own worktree after bringing in the exact fixture change:

| Check | Observed result |
| --- | --- |
| `python3 -m unittest discover -s tests/tooling -v` | Exit 0; 18/18 passed |
| Original test from `3f7c0d3`, coordinator JSON with TEST-002 set done in memory and coordinator evidence root | Exit 1; `AssertionError: ValueError not raised`, reproducing the fixture defect |
| Both corrected gate cases with the same completed-baseline JSON input | Exit 0; 2/2 passed |
| `git diff 9982001 -- tools/progress.py` | Empty |
| `git hash-object tests/tooling/test_progress.py` | Reviewed blob above |

No coordinator file was edited by the reviewer. The in-memory reproduction did
not alter progress. After integrating correction `4d7951f`, the coordinator
reported actual final passes against completed TEST-002 state: scaffold 3/3,
Qt probe 4/4, Python tooling 18/18, project consistency, render check, DOM
controller and diff check. The reviewer did not replay those final coordinator
commands. The coordinator must record their results and GitHub CI/publication
status; the first failed final-check attempt remains distinct from successful
reruns. This review approves the artifacts and correction, not an unperformed
command or automatic merge.
