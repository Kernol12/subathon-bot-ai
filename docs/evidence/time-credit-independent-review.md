# REVIEW-001 — independent time-credit baseline review

Date: 2026-10-30. Author identity: `/root/test_author`.
Reviewer identity: `/root/reviewer`, a separate agent/session with no authorship
of the contract or acceptance tests. Coordinator `/root` centrally claimed
REVIEW-001 after TEST-001 authoring and owns progress JSON/HTML and integration.
Review worktree: `task/REVIEW-001-credit-review`, based on current main `9982001`.

**Verdict: approved for test baseline.** This approves the narrow numeric contract
and its 19 cases for TEST-002's intended-red execution. It is not approval of an
implemented feature, a red-run claim, or permission to bypass the remaining
implementation/code-review gates. No blocking change request remains.

## Exact baseline and scope

Specification v0.45, sections 3.4/3.5, 4, 8, 17/18 and decisions NUM-01 through
NUM-04, TEST-04/05. Contract revision 1 preserves the original six examples and
adds explicit positive-credit validation and checked final-range semantics.

Authored commits are contract `ed96a88` (integrated equivalent `f527eea`), tests
`24ea9ba` and comment-only follow-up `3f7c0d3`. The reviewer inspected equivalent
prerequisite commits `17dcf12`, `ef92876`, `9bf843a` in this worktree. Git blob IDs
identify the exact reviewed content independently of cherry-pick commit IDs:

| Path | Git blob |
| --- | --- |
| docs/contracts/time-credit-v0.md | `7d7234d2b8219c13c2b3f3fb41c60e36638575d1` |
| tests/acceptance/time_credit_tests.cpp | `055dec680f6ded7c457d14daf088115a0d73fa9f` |
| core/include/subathon/core/time_credit.hpp | `564d3c379fafcaa9862aab55e2ae7e15f86cd75a` |
| core/src/time_credit.cpp | `b491b86ec83cde41463ef5d7a61b92d78fa9012e` |

The source stub is byte-identical to main `9982001` and still throws
`FeatureNotImplemented`. The public input/result aggregates are unchanged.
Header changes document the contract and do not implement validation/calculation.

## Independent oracle and boundary assessment

The reviewer derived the following results directly from the specification and
contract, without a production or reference implementation. Let M be the signed
64-bit maximum, 9223372036854775807 milliseconds.

- Original six cases: 55 minutes leaves five minutes of capacity beneath 60, so
  a ten-minute request applies five; 20 plus ten is 30 with full credit; at 60 the
  capacity is zero; without a cap 55 plus ten is 65; 55 plus five exactly reaches
  60; a zero request preserves 55. Requested values remain ten/ten/ten/ten/five/
  zero respectively. These match all original fixtures and assertions.
- `ZeroRemainingAcceptsOneMillisecond` yields requested/applied/remaining
  `(1,1,1)`. This is numeric scope only, without a lifecycle transition assertion.
- `ZeroCapHasNoCapacity` yields `(1,0,0)`; zero configured capacity is explicit.
- `OneMillisecondHeadroomClipsTheAward` yields `(2,1,3)` from remaining two,
  request two, cap three; it checks exact millisecond clipping.
- `UncappedCreditCanReachTheLargestRepresentableTime` yields `(1,1,M)`;
  `ZeroAwardAtLargestRepresentableTimeIsValid` yields `(0,0,M)`.
- `OneMillisecondBeyondTheRangeRequiresReview` and
  `LargeUncappedRequestMustNotWrap` both require `std::overflow_error`: M plus
  one is unrepresentable regardless of operand order. They assert the helper's
  error signal, not an implemented application review hold.
- `CapLimitsLargeRequestBeforeUnsafeAddition` yields `(M,1,M)` because M minus
  one has one millisecond of headroom. The unrepresentable naive uncapped sum
  must not preempt a representable cap-limited actual result.
- `NegativeRemainingIsNotDebtAtThisBoundary`,
  `NegativeRequestIsNotARefundAtThisBoundary`, `NegativeCapIsRejected`, and
  `ExistingTimeAboveCapIsRejectedEvenForZeroAward` require
  `std::invalid_argument` under the explicit restricted-helper preconditions.
- `InvalidInputIsRejectedBeforeConsideringOverflow` also requires
  `std::invalid_argument`: invalid cap validation precedes final aggregation.

All 12 valid cases assert requested, applied and remaining fields. The seven
error cases assert specific standard exception types. `FeatureNotImplemented`
is a different `logic_error` subtype from `invalid_argument` and is not an
`overflow_error`, so the stub cannot accidentally satisfy those error tests.
Inputs use safe integer expressions; no test constructs an overflowing signed
sum or derives its expected result by invoking the function under test.

A source comparison by original test name confirmed all six original fixtures
and all 15 original assertion lines retained. Three original cases gain requested
provenance assertions. There are no skips, expected-failure wrappers, weakened
assertions or implementation-generated fixtures.

## Policy and architectural limits

Rejecting negatives here does not reject signed product corrections/refunds or
replace the separate debt policy. The above-cap input rejection is explicitly a
restricted-helper decision; what lowering a configured cap does to existing time
remains unresolved in the later timer/configuration contract. No silent removal
or grandfathering policy is introduced.

Lifecycle, pause/grace, debt-first additions, hard-end precedence, eligibility,
transactional requested/actual effect provenance, refunds and persisted review
holds remain future service/transaction behavior. The helper cannot enforce
state absent from its inputs. Half-even rounding and lossless money/rate handling
also require separate tests because these inputs are already integer milliseconds.
These exclusions preserve release scope and must accompany implementation handoff.

## Commands actually run

In this reviewer worktree, using GNU C++ 16.2.1 and installed GoogleTest:

| Command | Exit / observation |
| --- | --- |
| `cmake --preset acceptance-debug` | 0; configured with installed dependencies |
| `cmake --build --preset acceptance-debug` | 0; compiled and linked without warning output |
| `ctest --preset acceptance-debug -N -L acceptance` | 0; lists all 19 cases, executes none |
| `git diff 9982001 -- tests/acceptance/time_credit_tests.cpp core/include/subathon/core/time_credit.hpp core/src/time_credit.cpp docs/contracts/time-credit-v0.md` | 0; inspected scope and unchanged production stub |
| One-off Python comparison against `git show 9982001:tests/acceptance/time_credit_tests.cpp` | 0; retained six original bodies/15 assertion lines and 19 declarations |
| `git hash-object` for the four baseline paths above | 0; blob identities recorded above |
| `git diff --check` | 0; no whitespace errors |

The source comparison checked every original line within its original named test
body and separately asserted an empty diff for the production stub. A build and
discovery check after the final comment-only baseline commit also exited 0.
No acceptance behavior was executed by this reviewer; the coordinator must run
the reviewed baseline next and record the actual failures. A successful build
or test listing is not an observed-red result.

## Separate evidence-document assessments

The reviewer read the coordinator's BOOT-002 environment note and found no gate
objection to its reported selected IDE native configuration execution plus
IDE-terminal scaffold configure/build/test with the matching `/usr/bin/c++`
toolchain. This was a document review, not independent replay or inspection of
those IDE tool outputs. Graphical profile settings and product platforms were
appropriately left outside the claims.

The reviewer also read SPIKE-001 commit `c7e0f3e` and its isolated probe. No
blocking ownership/shutdown issue was found in the bounded synthetic path:
admission and posting share a lock, producers join before the drain barrier is
posted, queued SQL executes on its owner and resources outlive joined producers.
The evidence explicitly leaves real Drogon startup/request/response/shutdown
behavior unverified. The reviewer did not rebuild/run that probe and does not
approve SPIKE-001 completion or reuse as production code.

Remaining external/platform gates: real provider checks, actual Drogon bridge,
Windows, Wayland/OBS, packaging and combined application behavior are unverified.
All integrations, exchange providers and specified platforms remain in scope.
Next: coordinator integrates this review, records TEST-002's actual intended red,
updates progress centrally and hands off to a later implementation agent.
