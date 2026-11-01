# CORETEST-005 independent contract/test review

Author: `/root/debt_test_author` (author revision389f8ba).
Reviewer: `/root/debt_test_reviewer`, a different agent/session, 2026-11-01.
Reviewer branch `task/CORETEST-005-debt-review`, isolated worktree
`local-evidence/worktrees/coretest005-review`.
Actual integrated candidate: `e405ae4`, from main1183314 and task claim9ff4af9.
Authority: specification v0.45 §4 Refund overflow, §18 refund-exceeds-time example,
NUM-02/03/04; CORE-005 brief and event/state boundaries revision1.

## Reviewed artifacts

| Artifact | Exact Git blob |
| --- | --- |
| Time-debt contract revision1 | `909f16e7ee6f4e9de6dc31acc728236032d51482` |
| Public interface | `f3cc819c16f3df907c0b68b0738e3aaef4f51950` |
| Two unconditional missing-behavior stubs | `c2d19b7a9f477b1629de00922b473e481b18265b` |
| 32 GoogleTest cases | `c3cd6c65dcdb7e8b20c8c71140256787788e8790` |
| Baseline build/discovery workflow | `e96126f87609bbff877360ac126b1a70f2de7f4a` |

The reviewer read all32 test bodies, contract, interface, stubs, author evidence,
root/core/test CMake changes, preset and new workflow. Existing time-credit and
timer-lifecycle sources/interfaces/acceptance tests have no diff against main.
No production arithmetic or validation is supplied by the new source: each public
function unconditionally throws `TimeDebtNotImplemented`.

## Contract and oracle assessment

The pure `TimeBalance` API separates nonnegative Main and debt without inventing
an Active-only restriction. Opaque references preserve the caller's retained full
record; they are not provider identifiers, deduplication proof or a ledger schema.
The result owns its request and keeps requested, actual Main, debt and unapplied/
capped components distinct. No original award, rate, partial-refund entitlement or
rounding is recomputed. Those accounting responsibilities remain ACCT-004/006.

RequireReview applies only when the reversal exceeds Main. I independently accept
that bounded interpretation from §4's Refund overflow heading, the §18 exceeds-time
scenario and the task's deficit-policy scope. Equality/zero create no deficit hold.
There is no unspecified default policy. A deficit Review holds the whole timer
adjustment; it cannot first subtract available Main. Numeric Applied is explicitly
distinct from contribution application, permission or clearing another review hold.

For additions, current Main at/above cap has zero headroom while debt is still
repaid first. The literal Main100/debt5/request8/cap60 result is Main100/debt0,
debt repaid5, Main added0 and capped3. This preserves existing Main rather than
performing a configuration correction. It fits debt-before-cap scope without
deciding how a cap edit is authorized or accepted. The older narrow time-credit
helper's above-cap rejection remains unchanged; the new API states its different
input scope explicitly.

All expected fields and transition vectors are literal, independently derived
integer or specification results. Fixture helpers only supply opaque strings and
snapshots; they generate no expected accounting result from production code.

| Reviewed group | Coverage and independent oracle |
| --- | --- |
| 14 reversal cases | All three policies within Main, exact exhaustion and zero; Stop100/150 leaves signed-50 unapplied, Carry adds50 to existing7, Review retains the whole-150; both references and selected policy retained on deficit paths; old debt survives policy selection; returned request is owned; invalid signs/balances/references/policy reject, including invalid enum without deficit |
| Signed-boundary paths within that group | MIN with MainMAX leaves Main adjustment-MAX and remainder-1; MIN with Main1 carries exactly debtMAX; MIN with Main0 cannot fit debt and throws; Review can hold fullMIN without constructing its positive magnitude; overflowing existing debt returns no proposal and leaves inputs unchanged |
| 12 addition cases | Debt repayment at full/zero/above cap, partial/exact repayment, debt then Main then capped remainder, uncapped surplus, zero request with positive balances, debt/cap avoiding overflowing hypothetical Main+request, true final Main overflow and invalid inputs |
| 6 composition cases | Only `main_added_ms` reaches the existing CORE-002 reducer: debt-only repayment preserves Grace/confirmation wait; surplus2 rescues Grace while retaining its73000 budget; Main exhaustion selects literal Immediate/Shared Grace/Confirmation transitions, including paused-zero handling |

The conceptual signed conservation equation is documented as exact arithmetic,
not a request to evaluate an overflowing signed expression. The MIN cases cannot
be implemented by unsafe signed negation or silent saturation. Exception choices
are explicit local C++ API decisions; overflow returns no proposal, and the caller
must retain the full request and commit no partial effects/jobs.

Preliminary draft gaps were resolved before this approval: held/limited reversal
paths now assert both provenance references and selected policy, and invalid-enum
coverage includes a within-Main path. The draft above-cap rejection was replaced
before approval with the explicit debt-first/headroom-zero contract and literal
oracles. No reviewed baseline assertion was removed or weakened.

## Actual independent checks

Reviewer environment: Linux7.2.11-arch1-1/x86-64, GNU16.2.1, CMake4.4.5,
Ninja1.13.2, Python3.14.8 and installed GoogleTest1.18.0. Versions were observed
from uname/compiler/CMake/Ninja/Python/pkg-config commands. No dependency was
downloaded or installed by this review.

Commands in the isolated reviewer worktree:

| Command/check | Actual outcome |
| --- | --- |
| `cmake --preset time-debt-debug` | Exit0 |
| `cmake --build --preset time-debt-debug` | Exit0; actual stubs and unchanged timer source compiled/linked |
| `ctest --preset time-debt-debug -L time-debt --show-only` | Exit0; exactly32 cases discovered, no bodies |
| `build/time-debt-debug/tests/core_time_debt_acceptance --gtest_list_tests` | Exit0; exactly32 compiled cases listed, no bodies |
| Python comparison of compiled GoogleTest listing with CTest `--show-only=json-v1` | Exit0; exact set equality,32 unique names in each |
| actionlint1.7.12 on `.github/workflows/time-debt.yml` | Exit0, no findings |
| `python3 tools/check_project.py` | Exit0;377 task records/122 decisions/links/presets/current HTML |
| `python3 tools/progress.py render --check` | Exit0; current snapshot |
| `git diff --check` | Exit0 |

The CMake lane preserves C++20 and existing target options, uses installed or
explicitly selected/pinned GoogleTest, and links existing CORE-002 only for actual
composition. It introduces no Qt, SQL, network or provider dependency in core.
Automatic CI builds/discovers this intentional-red baseline with a nonempty guard;
it does not claim passing behavior. Explicit manual acceptance runs real CTest and
propagates failure. No WILL_FAIL, continue-on-error, skip or failure inversion is
present. Actual GitHub execution remains a publication gate, not an observed result
of this review.

## Verdict and remaining gates

**Approved for test baseline** at actual candidatee405ae4 and the exact blobs above.
No test body, intended red or acceptance run occurred during this review. Integrate
this approval before observing the intended32-case stub-red. CORE-005 implementation
must wait for that observed red, preserve the reviewed artifacts and obtain its own
separate code/config review and actual automatic acceptance CI.

No owner decision is required by these bounded numerical choices. This approval
does not finish refund accounting or a running application. Permission/override
audit, durable full provenance, operation/reversal deduplication, authoritative
state/clock selection, hard-end/lifecycle ordering, post-finish/reopen application,
cap edits, ledger allocation and atomic effects/jobs remain their explicit caller/
downstream gates. Grace is never supplied as debt. Provider/platform/live/UI,
recovery/backup/deployment and full-release validation remain unperformed here.

## Final reviewed/red integration and completion review

Reviewer `/root/debt_test_reviewer`, 2026-11-01; actual final candidate `343bb8b`.
The isolated reviewer branch normally merged the task branch, preserving original
reviewb3413e5 and its coordinator cherry-pickdba4401 rather than resetting history.
The contract/interface/stub/tests/workflow and shared build/preset files have no
diff from the originally approved candidatee405ae4. Exact reviewed blobs above
remain unchanged; no test assertion, fixture or public boundary was weakened.

The retained [red output](coretest-005-red-ctest.txt) contains32 executed CTest
case blocks. I audited every block for its test-body execution and explicit missing
behavior description, including wrong-exception failures in validation cases.
All32 fail for the intended stubs; zero skipped cases or missing dependencies/
compilation/test-not-run failures are used as red. The retained process exit is8.
The [handoff](coretest-005-handoff.md) records reviewb3413e5 integrateddba4401
before that run, with the reviewed source/test artifacts unchanged. This meets the
review-before-red gate; it does not implement or pass debt behavior.

I independently queried actual
[Ubuntu run41291917578](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41291917578):
success at exact head `c2be886c5a29f852a8e196583b534feec6fbc7c4`. I also queried
the commit's check-runs: all11 cited-head checks completed successfully. Retained
actual job logs show GNU13.3.0/Python3.12.3, selected `_deps/googletest-build`
compilation and `Discovered 32 time-debt cases; behavior not run`. The ordinary
baseline job did not execute debt bodies; its explicit manual acceptance step was
outside this automatic run. That limited build/discovery success is correctly
separate from the actual local32-case intended red and from future product
acceptance. It is not a failed-case inversion or a claim of passing debt behavior.

Completion metadata correctly marks CORETEST-005 done with baseline observed_red
and product_acceptance not_run, while CORE-005 remains planned. NUM-03 stays partial
with explicit proportional-entitlement, application/audit/permission/persistence
and release gates. The docs distinguish the new numeric above-cap scope from the
older unchanged credit contract, and preserve actual CORE-002 composition/caller
responsibilities. No full application, provider or platform capability is upgraded.
The coordinator's39-case timer regression pass is attributed to the coordinator;
this final review did not repeat unchanged native suites or debt bodies.

At the final metadata revision, reviewer commands
`python3 tools/check_project.py`, `python3 tools/progress.py render --check` and
`git diff --check` all exited0. Original configure/build/discovery/actionlint
results remain valid because their inputs are unchanged.

**Approved for baseline integration/completion** at343bb8b. When recording this
approval, the coordinator should replace the checkpoint's superseded paragraph
claiming the proposal is still unreviewed (and its old magnitude wording) with the
actual reviewed signed-request boundary, and mark the reviewer role in past tense.
The authoritative state and next-action section already identify the reviewed/red
baseline correctly. Latest-head CI, resolved discussions and current normal merge
rules remain publication gates. CORE-005 must start from the integrated prerequisite,
preserve32 assertions, implement only the reviewed boundary, obtain separate code
review and enable/observe ordinary actual acceptance CI before its own completion.
