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

Pending exact implementation revision and independent build/test execution.
