# CORE-001 additive normalized-zero regression

Author: `/root/numeric_test_author`; date: 2026-10-31.
Branch: `task/CORE-001-zero-regression`, from integrated main `cab329b`.
Owned paths: this note and `tests/acceptance/exact_decimal_regression_tests.cpp`.
The coordinator owns the separate regression build target and integrated evidence.

## Requirement and independent oracle

[Exact decimal revision1](../contracts/exact-decimal-v1.md) requires finalizer
resource checks on **normalized** numerator/denominator components. Its numerical
precision authority remains specification v0.45 §3.5, NUM-01/04. Exact zero has
normalized numerator0 and denominator1, irrespective of the size of an intermediate
expression's denominator, and therefore rounds to the literal signed64 result0.

The one added test constructs `Exact(0) / (cpp_int(1) << 4096)` with the selected
Boost primitive and expects `round_effect_units(zero)` to return0. It neither calls
the production parser to construct an expected value nor asserts Boost's raw internal
representation. A Boost version that canonicalizes zero immediately and a version
that retains an intermediate denominator must satisfy the same mathematical oracle.

The coordinator reported that implementation candidate `873cc6d` rejects this exact
zero with length_error on the local Boost1.92 environment. That report motivates the
regression; this author has not independently executed it. This is an implementation
mismatch with the existing normalized-component contract, not a defect or change in
the original32-case baseline. Header, contract and all original assertions are
unchanged. There is no need to weaken the existing nonzero oversized-denominator
rejection or expand resource limits.

## Checks and review/red sequence

Author syntax-checked this test using:
`c++ -std=c++20 -Icore/include -fsyntax-only tests/acceptance/exact_decimal_regression_tests.cpp`.
It exited0 with installed GoogleTest/Boost headers. `git diff --check` exited0.
No test body or production numeric code was run by the author.

Separate reviewer `/root/numeric_reviewer` must inspect the actual committed test
before the coordinator records an observed behavioral red against the existing
implementation candidate. After approved red, the implementation fix must preserve
this assertion and the immutable32-case baseline. Independent code review and the
full relevant acceptance/regression/CI gates still apply before CORE-001 completes.
This supporting regression remains within the focused CORE-001 PR; no duplicate
implementation task or PR is created. No service/provider/platform behavior is claimed.
