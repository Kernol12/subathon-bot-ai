# CORE-005 independent implementation and CI review

Author: `/root/debt_implementer` (source revision `6394cab`).
Reviewer: `/root/debt_code_reviewer`, a different agent/session, 2026-11-01.
Reviewer branch `task/CORE-005-code-review`, isolated worktree
`local-evidence/worktrees/core005-review`.
Actual integrated candidate: `f0c888f24540a4d0b022d95f87ed3c8114bd586b`;
source integrated as `cbfdd85` after task claim `05961a8`, from main `bca06e8`.
Authority: specification v0.45 §4 Refund overflow and NUM-02/03/04,
[time-debt revision 1](../contracts/time-debt-v1.md), CORE-005 brief and the
[event/state boundary](../contracts/event-state-boundaries-v1.md).

## Exact reviewed artifacts and prerequisite

| Artifact | Exact Git blob |
| --- | --- |
| Production proposals | `68141b15a4578cc85b4537a9b1403638c6e445f2` |
| Unchanged public interface | `f3cc819c16f3df907c0b68b0738e3aaef4f51950` |
| Unchanged contract revision 1 | `909f16e7ee6f4e9de6dc31acc728236032d51482` |
| Unchanged 32 GoogleTest cases | `c3cd6c65dcdb7e8b20c8c71140256787788e8790` |
| Promoted acceptance workflow | `792b14b6635fc60c48f573d863ba2a6140079b03` |

The original [baseline review](coretest-005-review.md),
[handoff](coretest-005-handoff.md) and [retained red result](coretest-005-red-ctest.txt)
record independent approval `b3413e5` integrated at `dba4401` before the actual
32-case intended stub failures, exit 8, and final baseline approval `5474fdc`.
PR26 integrated that prerequisite in main `bca06e8`. Existing valid evidence is
reused; this review does not repeat or claim to author the original red run.

I read every production branch and the complete reviewed contract/interface/tests,
plus changed CMake option/preset/labels and workflow. The interface, contract and
32 test bodies have no diff from main `bca06e8`; existing time-credit and timer
sources/interfaces are unchanged. There is no baseline weakening, dependency
substitution, new external dependency or floating-point arithmetic.

## Source assessment beyond existing assertions

Validation precedes arithmetic. Both balances must be nonnegative; reversal and
addition signs, nonempty references, explicit policy membership and an engaged
negative cap are rejected with the specified invalid_argument. Invalid policy is
validated even for a reversal that fits Main or is zero. No default refund policy
is invented.

For a fitting reversal, `-main` is representable because valid Main is between
zero and INT64_MAX; the fit condition proves `main + adjustment` is nonnegative
and no greater than Main. Exact exhaustion and zero therefore apply without a
new hold. All returned aggregates copy the complete request and selected policy,
retain old debt where required and preserve disposition and actual/unapplied
components separately. Standard owned strings introduce no borrowed return state.

For a deficit, RequireReview returns the unchanged balance and full signed request
before constructing any debt magnitude. This holds the whole adjustment, including
INT64_MIN, without subtracting affordable Main first. Stop computes the signed
remainder `adjustment + main`; opposite-sign addition is representable, including
MIN with zero Main. It never negates MIN or silently stores negative Grace/debt.

Carry computes `debt_headroom = MAX - old_debt`, a representable nonnegative value.
The comparison against `-debt_headroom` proves the negative deficit is at least
-MAX before its only negation, and that adding its magnitude to old debt fits.
MIN with Main1/debt0 can produce exactly MAX debt; MIN with Main0 cannot fit and
throws. An overflowing existing-debt case returns no partial proposal. No caller
input is mutated, no authoritative state is written and no effect is published.

For additions, min(debt, request) and subtraction form a nonnegative repayment and
remainder before cap application. Engaged cap zero is handled as a real cap. Main
at or above a cap has headroom zero and is preserved; lower Main has representable
cap-minus-Main headroom and cannot grow beyond that cap. Uncapped remainder is
checked against MAX-minus-Main before addition. Final debt subtraction and capped
remainder subtraction are bounded by their preceding min operations. A hypothetical
Main-plus-original-request overflow is never evaluated to reject a debt/cap-limited
actual result. These range arguments also cover zero, both positive balances,
maximum inputs, and cases not individually enumerated in the 32 assertions.

No function interprets lifecycle, permissions, application status, another review
hold, provider identity or entitlement. Grace is absent from numeric balances.
The six existing composition oracles now reach the unchanged CORE-002 reducer and
pass the actual Main credit only. Signed request/provenance and separate debt/Main
components remain available for later authoritative accounting and atomic commit.

## CI/configuration assessment

The existing `time-debt-build` check name stays stable. All PR/main/manual triggers
now run actual `ctest --preset time-debt-debug -L time-debt` unconditionally. The
former manual acceptance input/guard is removed. The same selected GoogleTest
setup and pinned checkout action remain; contents permission is read-only and
checkout does not persist credentials. The CMake draft label is removed only for
the now implemented, passing suite; original red evidence remains retained.

The registration guard compares compiled GoogleTest names with CTest JSON, checks
nonempty and exact set/count equality, then executes the real bodies. I executed
that exact embedded guard against this candidate: actual 32-case metadata passed;
empty, one-missing and duplicate registrations each failed. Actionlint 1.7.12 passed.
No WILL_FAIL, skipped baseline, failure inversion or continue-on-error is introduced.
This is configuration approval; actual promoted Ubuntu acceptance still must be
observed before merge, and a workflow does not itself configure required checks.

## Actual independent checks

Environment observed in this isolated worktree: Linux 7.2.11-arch1-1/x86-64,
GNU 16.2.1, CMake 4.4.5, Ninja 1.13.2, Python 3.14.8, installed GoogleTest 1.18.0.
No dependency was downloaded or installed for these native checks.

| Command/check | Actual outcome |
| --- | --- |
| `cmake --preset time-debt-debug` | Exit 0 |
| `cmake --build --preset time-debt-debug` | Exit 0, warning-free reviewed source build |
| `ctest --preset time-debt-debug -L time-debt` | Exit 0, all 32 unchanged cases pass |
| `ctest --preset time-debt-debug` | Exit 0, 35/35 including three infrastructure checks |
| Exact workflow registration guard using compiled listing and CTest JSON | Actual 32 pass; empty/missing/duplicate variants reject |
| `/tmp/subathon-actionlint-1.7.12/actionlint .github/workflows/time-debt.yml` | Exit 0, no findings |
| `python3 tools/check_project.py` | Exit 0, 377 records/122 decisions/links/presets/current HTML |
| `python3 tools/progress.py render --check` | Exit 0, current snapshot |
| `git diff --check` | Exit 0 |

## Verdict and remaining gates

**Approved for source/configuration integration** at actual candidate `f0c888f`
and the exact artifacts above. No blocking finding or source/test correction is
requested. This approval is independent of the implementation author and includes
source range/ownership/error reasoning beyond a passing count.

Actual promoted Ubuntu32 acceptance, final task-completion/integration metadata,
latest-head CI, resolved discussions and current normal merge rules remain gates.
Proportional entitlement/reversal allocation, authoritative audit/permission/revision/
ledger/persistence/effects/jobs, real cap edits, lifecycle/hard-end ordering,
post-finish/reopen application, providers/platforms/live checks and full release
remain outside this bounded numeric implementation and are not upgraded by review.

## Final completion, CI evidence and integration review

Reviewer `/root/debt_code_reviewer`, 2026-11-01; actual completion candidate
`f55d99a`. The isolated reviewer branch normally merged the task branch, preserving
original review commit `98f3f04` and its coordinator integration `091eda9`.
Production source, interface, contract, 32 test bodies, workflow and build/preset
files have no diff from the originally approved candidate `f0c888f`. Exact source
`68141b15` and workflow `792b14b6` remain unchanged. No assertion or original
numeric/lifecycle behavior was altered to obtain a passing result.

I independently queried actual
[Ubuntu run 41294422355](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41294422355):
completed successfully at exact head
`de78828efc7388da926b91cfd1ad89d65ed2f2ed`. The commit's check-runs independently
show all 11 cited-head checks completed successfully. I downloaded and read actual
job logs: GNU13.3.0, selected `_deps/googletest-build` compilation, exact
`Discovered all 32 compiled time-debt cases`, followed by the unconditional
`Run reviewed debt acceptance` step and 32/32 passed. This is actual passing
ordinary acceptance for the implemented proposals, distinct from the old
build/discovery-only baseline run. No manual behavior guard or hidden failure
remains. These observations do not certify a different or later PR revision.

The state correctly marks CORE-005 done with passing native/acceptance/regression,
separate code review and observed GitHub CI; application entitlement/persistence/
provider and platform-release fields remain not_run. NUM-03 remains partial and
explicitly excludes proportional entitlement, ledger allocation and authoritative
effects/audit/permissions/persistence. Updated architecture, strategy and CI docs
state pure proposal/CORE-002 composition scope, retain original reviewed stub-red
and distinguish actual Ubuntu acceptance from release or provider verification.
The handoff links the unchanged baseline and the source/review/CI evidence.

The old reviewed/red sequence remains valid: baseline approval `b3413e5` integrated
at `dba4401` before its recorded 32 stub body failures, final baseline approval
`5474fdc`, and prerequisite merge `bca06e8` before this task's author branch.
No additional red or native behavior runs were needed for this metadata review;
my independent 32/35 checks above remain valid because their inputs are unchanged.
At this revision `python3 tools/check_project.py`,
`python3 tools/progress.py render --check` and `git diff --check` each exited 0.

**Approved for final implementation integration/completion** at `f55d99a`.
When recording this approval, the coordinator should remove or clearly mark as
historical the checkpoint's inherited prior-session declarations that the current
root is still a local-only checkpoint, no task claim exists, and CORE-005 is still
unimplemented. The later current-assignment section already correctly records this
task and its completion gates. Mark this review role in past tense after recording
approval so the next resume has one clear current state.

Latest-head CI, resolved discussions, any current required GitHub approvals and
normal merge rules still require a fresh check before merge; this review claims
no merge. All downstream application/accounting/recovery/provider/platform and
full-release gates listed above remain explicit and unresolved by this helper.
