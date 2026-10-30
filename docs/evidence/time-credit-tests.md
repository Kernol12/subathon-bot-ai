# TEST-001 — Authored time-credit acceptance baseline

Author: `/root/test_author`, separate test-author agent. Date: 2026-10-30.
Branch: `task/TEST-001-credit-tests`, from main `9982001` plus prerequisite
CONTRACT-001 (`f527eea`, equivalent to author commit `ed96a88`).
Authority: specification v0.45; [time-credit contract revision 1](../contracts/time-credit-v0.md).
Coordinator `/root` centrally claimed the task after accepting CONTRACT-001 and owns
progress JSON/HTML, build files and integration. Author owns this note and acceptance
test source only. Formal reviewer is `/root/reviewer`; the earlier contract handoff's
`/root/independent_review` was a prospective role name, not a separate review claim.

## Scope and oracle review

The original six case names, input fixtures and assertions are retained. Three
existing valid cases gain a requested-amount assertion; all valid examples now
check requested, applied and remaining values. Thirteen additional cases cover
numeric boundaries and the specified error types. No production or stub behavior
is changed. There are 19 registered test cases in this source.

The literal oracles are reviewed arithmetic examples, not values produced by a
reference implementation or `calculate_time_credit`. `minute = 60000`; the type
limit `maximum_ms` is exactly 9223372036854775807 (here called M). No test computes
an overflowing signed input expression.

| Test name suffix | Input (remaining, request, cap), milliseconds | Expected (requested, applied, remaining after) / error | Specification / contract link |
| --- | --- | --- | --- |
| CapRecordsOnlyActuallyAddedTime | `(3300000, 600000, 3600000)` | `(600000, 300000, 3600000)` | §3.4/4/8/18 explicit capped example; revision 1 original example 1 |
| AvailableHeadroomAcceptsTheWholeAward | `(1200000, 600000, 3600000)` | `(600000, 600000, 1800000)` | §4 cap; original example 2 |
| FullTimerAcceptsNoAdditionalTime | `(3600000, 600000, 3600000)` | `(600000, 0, 3600000)` | §3.4 requested/actual; §4 cap; original example 3 |
| NoCapAcceptsTheWholeAward | `(3300000, 600000, absent)` | `(600000, 600000, 3900000)` | §4 optional cap; original example 4 |
| ExactBoundaryDoesNotDiscardTime | `(3300000, 300000, 3600000)` | `(300000, 300000, 3600000)` | §4 cap; original example 5 |
| ZeroAwardLeavesTheTimerUnchanged | `(3300000, 0, 3600000)` | `(0, 0, 3300000)` | §3.4 actual effect; original example 6 |
| ZeroRemainingAcceptsOneMillisecond | `(0, 1, absent)` | `(1, 1, 1)` | NUM-01/04, §3.5 millisecond output; revision 1 numeric zero |
| ZeroCapHasNoCapacity | `(0, 1, 0)` | `(1, 0, 0)` | §4 cap; revision 1 zero-cap API decision |
| OneMillisecondHeadroomClipsTheAward | `(2, 2, 3)` | `(2, 1, 3)` | §3.4/4 actual effect; NUM-01 precision |
| UncappedCreditCanReachTheLargestRepresentableTime | `(M-1, 1, absent)` | `(1, 1, M)` | NUM-04 signed 64-bit final aggregation |
| ZeroAwardAtLargestRepresentableTimeIsValid | `(M, 0, absent)` | `(0, 0, M)` | NUM-04 exact boundary, revision 1 zero award |
| OneMillisecondBeyondTheRangeRequiresReview | `(M, 1, absent)` | `std::overflow_error` | NUM-02/04, §18 out-of-range scenario; revision 1 error signal |
| LargeUncappedRequestMustNotWrap | `(1, M, absent)` | `std::overflow_error` | NUM-02/04, §18 out-of-range scenario; revision 1 error signal |
| CapLimitsLargeRequestBeforeUnsafeAddition | `(M-1, M, M)` | `(M, 1, M)` | §4 cap; NUM-04 checked actual final aggregation |
| NegativeRemainingIsNotDebtAtThisBoundary | `(-1, 1, absent)` | `std::invalid_argument` | §4 separate debt; revision 1 positive-credit precondition |
| NegativeRequestIsNotARefundAtThisBoundary | `(1, -1, absent)` | `std::invalid_argument` | NUM-02/03 signed corrections elsewhere; revision 1 precondition |
| NegativeCapIsRejected | `(0, 1, -1)` | `std::invalid_argument` | Revision 1 invalid-cap API decision; §17 public-contract preparation |
| ExistingTimeAboveCapIsRejectedEvenForZeroAward | `(2, 0, 1)` | `std::invalid_argument` | Revision 1 restricted helper domain; lowered-cap product policy unresolved |
| InvalidInputIsRejectedBeforeConsideringOverflow | `(M, 1, -1)` | `std::invalid_argument` | Revision 1 validation-before-overflow ordering |

The extra validation-order case makes an explicit contract rule executable; unlike
the negative-cap-only example, its naive uncapped sum is also unrepresentable.
Every error assertion requires a specific standard exception type and rejects
the current `FeatureNotImplemented` logic_error subtype. Exceptions report the
numeric helper outcome only: these tests do not claim that an application review
hold, persisted effect, lifecycle transition, debt payment, refund or hard-end
enforcement exists. Those remain separate release behaviors and contracts.

## Verification and next gates

Actual commands run in this worktree on 2026-10-30:

- `cmake --preset acceptance-debug`: exit 0 with installed GoogleTest; no download.
- `cmake --build --preset acceptance-debug`: exit 0. The first build emitted
  `nodiscard` warnings inside new exception assertions. Explicitly discarding the
  unexpected successful return fixes that warning without changing assertions;
  the subsequent build exited 0 without warnings.
- `clang-format -i tests/acceptance/time_credit_tests.cpp`: exit 0 using repository style.
- `ctest --preset acceptance-debug -N -L acceptance`: exit 0, lists all 19 cases;
  `-N` discovers cases only and does **not** run them.
- `git diff --check`: exit 0, no whitespace errors.
- A one-off Python source check against main `9982001` confirmed all 15 original
  assertion lines retained, 19 test declarations and an unchanged production stub.

Separate REVIEW-001 must
review this exact source/contract baseline; only afterward may TEST-002 record an
intended red acceptance run. Compilation or missing dependencies do not establish
that baseline. No feature implementation, independent approval or observed red
is claimed by this authoring note.
