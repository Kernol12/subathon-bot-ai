# CONTRACT-001 — Time-credit contract and test plan

Author: `/root/test_author`, separate test-author agent. Date: 2026-10-30.
Branch: `task/CONTRACT-001-credit-contract`, from `origin/main` at `9982001`.
Authority: specification v0.45; time-credit contract revision 0 → 1.
Coordinator `/root` owns progress JSON, generated HTML and shared build files.
This author owns the contract, its header comments and this evidence; the calculation
stub is unchanged. Separate review and observed red are still required.

## Review of the original six examples

§3.4 requires requested/actual provenance; §4 requires an optional remaining-time
cap; §8 and §18 explicitly distinguish requested ten minutes from actual five at a
55-minute timer and 60-minute cap. The six draft fixtures are within that numeric
scope and their expected values are consistent. The capped example has only five
minutes of headroom. At 20 + 10 the 60-minute cap leaves all ten available; at 60
there is none; absent cap 55 + 10 is 65; exactly five reaches 60; zero changes nothing.
No assertion or fixture in those six is removed or weakened.

## Contract decisions and requirement mapping

- NUM-01/04 and §3.5: preserve signed 64-bit millisecond results and check final
  aggregation. This boundary consumes already-rounded whole milliseconds and
  does not establish half-even arithmetic coverage.
- NUM-02/04 and the §18 overflow scenarios: `std::overflow_error` reports an
  unrepresentable final remaining value; the application later holds the entire
  input for review without effects. No persistence/review behavior is implemented.
- §3.4/4 and §18 cap example: retain requested versus actual credit, including
  cap-limited results near the signed integer maximum.
- §4 debt and NUM-03 signed reversals: negatives are outside this addition-only
  helper and yield `std::invalid_argument`; rejecting them here does not forbid
  signed corrections/refunds elsewhere. A supplied zero cap has zero capacity.
- §17 allows routine interface choices: standard exception classes keep the
  current aggregate API small. Validation errors precede final-sum overflow.

The newly lowered-cap question is deliberately **unresolved at the product level**:
reject `remaining > cap` at this narrow boundary instead of choosing a silent
time-removal or grandfathering policy. A later timer/configuration contract must
define that transition before a service calls this helper for it. Lifecycle,
grace, pause, finish/reopen, debt-first application and hard end are also caller
responsibilities, not guessed capabilities of this numeric helper.

## Independently derived TEST-001 plan

All values below are literal milliseconds, with `M = 9223372036854775807`.
Expected results come from these examples, never from the stub or a second copy
of an implementation algorithm. Original six cases remain, supplemented by:

| Input (remaining, request, cap) | Expected (requested, applied, remaining after) or error | Purpose |
| --- | --- | --- |
| `(0, 1, absent)` | `(1, 1, 1)` | Zero remaining; millisecond precision |
| `(0, 1, 0)` | `(1, 0, 0)` | Zero capacity |
| `(2, 2, 3)` | `(2, 1, 3)` | One-millisecond cap headroom |
| `(M - 1, 1, absent)` | `(1, 1, M)` | Exact representable upper boundary |
| `(M, 0, absent)` | `(0, 0, M)` | Zero request at upper boundary |
| `(M, 1, absent)` | `std::overflow_error` | First unrepresentable final sum |
| `(1, M, absent)` | `std::overflow_error` | Large request must not wrap |
| `(M - 1, M, M)` | `(M, 1, M)` | Cap limits before unsafe addition |
| `(-1, 1, absent)` | `std::invalid_argument` | Negative remaining is not debt |
| `(1, -1, absent)` | `std::invalid_argument` | Negative request is not a refund |
| `(0, 1, -1)` | `std::invalid_argument` | Invalid configured cap |
| `(2, 0, 1)` | `std::invalid_argument` | Above-cap state is explicitly rejected |

Error cases will assert their specific standard exception type, so the current
`FeatureNotImplemented` stub cannot satisfy them. Valid cases will assert all three
result fields. No sleeps, fixtures from real services or duplicated product engines
are needed. This plan adds no test claiming lifecycle/debt/hard-end enforcement.

## Checks and handoff

Commands actually run in this worktree on 2026-10-30 (GNU C++ 16.2.1,
Python 3.14.8), all exited 0:

- `cmake --preset scaffold-debug`: configured without Qt or GoogleTest.
- `cmake --build --preset scaffold-debug`: built the unchanged stub and scaffolding.
- `ctest --preset scaffold-debug`: 3/3 infrastructure tests passed.
- `git diff --check`: no whitespace errors.
- `git diff -- core/src/time_credit.cpp`: empty; explicit stub unchanged.

These infrastructure results do not claim any implemented credit behavior.
This contract task does not claim an acceptance run or independent review. Next: coordinator accepts
CONTRACT-001 and claims TEST-001, then author the linked tests on their own task
branch before handing them to `/root/independent_review`.
