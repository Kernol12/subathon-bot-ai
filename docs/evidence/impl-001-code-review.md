# IMPL-001 — independent code and regression review

Date: 2026-10-30. Implementation/test author: `/root/credit_implementer`.
Independent reviewer: `/root/credit_reviewer`, separate agent with no production
or test authorship. Review branch `task/IMPL-001-credit-code-review`, isolated
worktree `/tmp/subathon-impl001-review`, created from refreshed main `45a38d8`.
Coordinator `/root` owns progress JSON/HTML and shared integration records.
Authority: specification v0.45 §3.4/3.5, §4, §18; time-credit contract revision 1;
reviewed TEST-002 baseline and its recorded intended red at `f30e293`.

## Additive regression baseline review before implementation

**Verdict: approved for regression baseline.** On 2026-10-30 the reviewer inspected
authored commit `d3d1256` while the production stub remained unchanged. Approval
was sent directly to the implementer before its intended-red execution and before
production implementation. Reviewer prerequisite cherry-pick: `9655bda`.

| Path | Reviewed Git blob |
| --- | --- |
| tests/acceptance/time_credit_regression_tests.cpp | `86a5c2a716204af292a8c72600a3ce6b27629097` |
| tests/CMakeLists.txt | `a68401aa94d3a6ff7994d722caf80627f19fcf35` |

Let M=9223372036854775807 and m=-9223372036854775808. The four additive cases
exercise existing revision-1 semantics, with independently derived literal oracles:

- Each of `(m,M,M)`, `(0,m,0)` and `(M,M,m)` contains an invalid negative input,
  requiring `std::invalid_argument` before any arithmetic. In particular m cannot
  be safely negated or subtracted speculatively.
- `(M,M,M-1)` violates remaining-at-or-below-cap and must throw
  `std::invalid_argument` before considering the unrepresentable naive sum.
- `(M,M,M)` has no headroom: result `(requested=M, applied=0, remaining=M)`.
- `(0,M,absent)` fits exactly: result `(requested=M, applied=M, remaining=M)`.

No production/reference algorithm generates expected values. The original 19
acceptance cases are unmodified and still registered under their original labels;
the new target uses separate `regression;core` labels without skips or expected
failure inversion. No public contract, fixture or production change is introduced
by this test-only commit. Tests are synthetic and have no external side effects.

Commands actually run for this pre-implementation review: `git show d3d1256 --`,
`git show d3d1256:tests/acceptance/time_credit_regression_tests.cpp`, and
`git hash-object` for both reviewed paths (all exit 0). This is source/oracle
review only; no acceptance run or implementation approval is claimed here.

## Implementation review

**Verdict: approved for integration. No blocking findings.** The reviewer inspected
implementation author commit `29325b5` (reviewer prerequisite cherry-pick `e68aa0a`)
and the exact source blob `d87cc073ce470fe6b864ef16c75d1ca6aa8d0145`.
This approves only the revision-1 numeric planning helper, not a complete timer or
application transaction. Final coordinator integration and latest-revision CI/PR
merge gates remain separate.

The implementation validates negatives and the above-cap state with comparisons
before any arithmetic. For valid inputs, `remaining` is in [0,M]; when capped,
`cap` is also in [remaining,M]. Therefore `cap-remaining` is in [0,M], so the
headroom subtraction cannot overflow. Clipping the request to that headroom
precedes any addition and allows representable actual credit even when the naive
requested sum exceeds M. Without a cap the applied request is unchanged.

The subsequent `M-remaining` subtraction is representable for every validated
remaining value. The checked comparison rejects exactly the positive sums outside
signed 64-bit range, then the final addition is safe. There is no negation of
INT64_MIN, unsafe signed intermediate, floating point, saturation or truncation.
The contract explicitly permits checked arithmetic for these already-integral
milliseconds; broader exact money/rate/rounding work retains its separate selected
Boost.Multiprecision requirement. No additional dependency is necessary here.

Requested credit is copied exactly from the input, independently of applied credit;
only actual applied credit contributes to remaining-after. The function uses no
mutable external state, provider/network/UI framework, persistence, debt or refund
policy. Exceptions return no proposal. A later authoritative caller must still
implement eligibility, review holds and transactional effects; this review claims
none of those behaviors.

The reviewer compared source against main `45a38d8` and confirmed the 19 original
acceptance cases, fixture and public header are byte-identical. New regression
source and tests CMake match the pre-implementation reviewed blobs. The public
unused stub exception type remains for compatibility; production no longer throws
it. Author red evidence was inspected: four regression failures on the explicit
stub, not compilation/dependency/skipped-test failures. It is attributed author
execution rather than an independent reviewer red replay.

| Preserved baseline | Git blob |
| --- | --- |
| tests/acceptance/time_credit_tests.cpp | `055dec680f6ded7c457d14daf088115a0d73fa9f` |
| tests/fixtures/time-credit.json | `a1ab9d7b56e9eeaa1e429c25da5a522786320df3` |
| core/include/subathon/core/time_credit.hpp | `564d3c379fafcaa9862aab55e2ae7e15f86cd75a` |

### Independent commands and results

Executed in the reviewer worktree on Linux 7.2.11-arch1-1 x86-64, GCC 16.2.1
(20260810), CMake 4.4.5, Ninja 1.13.2, Python 3.14.8, installed GoogleTest 1.18.0.
No dependency downloads or external services were used.

| Command | Actual result |
| --- | --- |
| `cmake --preset acceptance-debug` | Exit 0; clean reviewer build configured |
| `cmake --build --preset acceptance-debug` | Exit 0; compiled and linked without warnings |
| `ctest --preset acceptance-debug -L acceptance` | Exit 0; all 19 original reviewed cases passed |
| `ctest --preset acceptance-debug -L regression` | Exit 0; all four separately reviewed regression cases passed |
| `ctest --preset acceptance-debug` | Exit 0; 26/26 passed including three infrastructure/tooling checks |
| `git diff 45a38d8 -- core/src/time_credit.cpp tests/CMakeLists.txt tests/acceptance/time_credit_regression_tests.cpp` | Exit 0; inspected full implementation/test changes |
| `git diff --exit-code 45a38d8 -- tests/acceptance/time_credit_tests.cpp tests/fixtures/time-credit.json core/include/subathon/core/time_credit.hpp` | Exit 0; empty, preserved reviewed artifacts |
| `git hash-object` for implementation/tests/header/fixture/tests CMake | Exit 0; exact blobs above confirmed |
| `clang-format --dry-run --Werror core/src/time_credit.cpp tests/acceptance/time_credit_regression_tests.cpp` | Exit 0 |
| `git diff --check` | Exit 0 |

Existing CTest `draft` metadata is historical; the review and TEST-002 handoff
identify the approved baseline. Passing this small numeric suite establishes no
live provider, Windows, Wayland/OBS, packaging, persistence, complete timer,
refund, or UI verification. All integrations and exchange providers remain in
release scope. Coordinator progress/current-status changes receive a separate
integration assessment after they are available.

## Separate tooling-fixture correction review

**Verdict: approved for integration.** Author `/root/credit_implementer` supplied
correction `3864b15`; independent reviewer `/root/credit_reviewer` inspected its
exact test blob `f606dad3592b2bc2ae329b249f81d3b3c131daf9` and
[specification-linked rationale](impl-001-progress-fixture-correction.md).
Reviewer prerequisite cherry-pick: `423c71c`. The author and coordinator report
an original 17/18 tooling result because live IMPL-001 gained genuine passing
acceptance. The old negative fixture borrowed that task without clearing its
verification. This was a fixture assumption; the completion validator correctly
accepted the completed evidence.

The revised negative test creates synthetic IMPL-999 with explicit separate
review and valid evidence, then still requires the `passing acceptance` rejection
for both missing and failed acceptance. Its new companion accepts explicit
passing acceptance plus separate review. Clearing activity only removes references
to replaced live tasks. No validator policy or assertion is weakened. The existing
self-review rejection remains and explicitly supplies its own acceptance field.
This follows specification TEST-04/05 and preserves the completion evidence gate
while removing dependency on whether real IMPL-001 is finished.

Independent reviewer executions, all exit 0:

- `python3 -m unittest discover -s tests/tooling -v`: 19/19 pass.
- The author's documented in-memory `setUp` injection of live IMPL-001 acceptance
  `passed`, executing both completion tests: 2/2 pass, confirming isolation.
- `ctest --preset acceptance-debug`: 26/26 pass including original19, regression4
  and three infrastructure/tooling checks.
- `git diff --exit-code 29325b5 -- core/src/time_credit.cpp tests/acceptance/time_credit_tests.cpp tests/acceptance/time_credit_regression_tests.cpp tests/fixtures/time-credit.json core/include/subathon/core/time_credit.hpp tools/progress.py`:
  empty, confirming validator and product/reviewed test baseline unchanged.
- `git hash-object tests/tooling/test_progress.py` and `git diff --check`:
  exact corrected blob confirmed, no whitespace errors.

This separate approval covers the genuine test defect and records the revised
baseline. Coordinator must still replay final integrated checks with the honestly
recorded task status and regenerate progress HTML before publication.

## Final independent integration assessment

**Verdict: approved for integration at coordinator revision `b046e9e`. No blocking
findings.** The reviewer inspected the full diff `45a38d8..b046e9e`, final
handoff, progress JSON/HTML, acceptance-map scenario, current-status documentation,
contract status prose, CMake option/preset display names and manual workflow.
The exact implementation and test/fixture/header/validator blobs remain those
already approved above. Numeric/error contract requirements are unchanged;
revision-1 edits update status and evidence links only.

The map labels only the numeric capped-credit proposal passed and leaves full
specification decisions pending. Progress records IMPL-001's passing acceptance,
regression, independent review and integrated checks; TEST-002's observed-red
history remains. The corrected tooling fixture is exactly author `3864b15`.
Handoff retains the original failed tooling attempt and separate correction
review, identifies author/reviewer and isolated worktrees, and records exact
coordinator checks. No refund, persistence, UI, debt/lifecycle or live/platform
claim is introduced. Qt/CLion evidence is clearly historical and publication,
latest-revision CI and repository merge gates are explicitly pending.

The manual acceptance workflow now runs `-L "acceptance|regression"` with real
failure propagation, covering all 23 product cases. CMake changes at root are
text/display labels only; regression registration remains the reviewed tests
CMake. Historical `draft` test metadata does not mask outcomes. Unchanged original
19-case source and fixture were checked with empty diffs against both the author
implementation and refreshed main.

Actual final assessment commands, all exit 0:

- `git diff --stat 45a38d8 b046e9e`, scoped full `git diff` for integration files,
  `git show b046e9e:docs/evidence/impl-001-handoff.md` and progress-state diff:
  inspected the exact final artifacts.
- `git diff --exit-code 29325b5 b046e9e -- core/src/time_credit.cpp tests/acceptance/time_credit_tests.cpp tests/acceptance/time_credit_regression_tests.cpp tests/fixtures/time-credit.json core/include/subathon/core/time_credit.hpp tools/progress.py`:
  empty; reviewed production and baseline/validator unchanged.
- `git diff --exit-code 3864b15 b046e9e -- tests/tooling/test_progress.py`:
  empty; reviewed fixture correction unchanged.
- `git diff --check 45a38d8 b046e9e`: no whitespace errors.
- In the clean coordinator checkout of `b046e9e`, `python3 tools/check_project.py`,
  `python3 tools/progress.py render --check`, `node tools/check_progress_ui.mjs`
  and `python3 -m unittest discover -s tests/tooling -v`: project375/decisions122,
  current HTML, offline DOM simulation and Python19/19 all passed. `git status
  --short` was empty. No actual browser/OBS run is claimed.

The coordinator's final full CTest26/26 and scaffold3/3 are recorded coordinator
executions. Independent reviewer source-identical clean C++ build/run and
post-correction full26/26 replay are separately recorded above; no redundant
C++ rebuild is needed for subsequent documentation/status/display-label changes.
This assessment is separate agent review, not GitHub account approval or evidence
that unperformed CI/merge gates have passed.
