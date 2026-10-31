# Independent review — CORE-001

Implementation author: `/root/numeric_implementer`.
Independent reviewer: `/root/numeric_reviewer`; date: 2026-10-31.
Baseline: specification v0.45 §3.5/NUM-01/02/04; exact-decimal contract revision1,
32-case reviewed/red baseline integrated in PR12 at main `cab329b`.

## Additive regression review before the finalizer fix

Regression author: `/root/numeric_test_author`.
Reviewed actual commit `5f9b3c017a820c9447e0708b8eadf0fc7278463f`;
[regression test](../../tests/acceptance/exact_decimal_regression_tests.cpp) blob
`347a810b1edba2ffc36d66ba44b700092bb2b381` and
[authoring note](core-001-zero-regression.md).
Verdict for this stage: **approved for additive regression baseline**.
This is not approval of the incomplete implementation candidate.

The existing contract checks normalized rational numerator/denominator components.
`Exact(0) / (cpp_int(1) << 4096)` is mathematically zero, whose normalized components
are0/1; its final integer must be the literal0. The test does not assert dependency
raw representation and does not use the production parser to derive the expected
value. It remains valid whether a Boost release canonicalizes the intermediate zero
immediately or preserves a raw denominator. Requiring0 does not weaken the original
nonzero4097-bit denominator rejection or enlarge any resource bound.

This is a focused implementation regression under the existing revision1 requirement,
not a contract change or correction of a wrong original assertion. The reviewed
32-case baseline/header/contract are unchanged. The separately authored test must
receive an actual intended red against candidate `873cc6d` before its corresponding
finalizer fix; missing dependencies/compile failure do not count.

Independent syntax-only compilation:
`c++ -std=c++20 -Icore/include -fsyntax-only tests/acceptance/exact_decimal_regression_tests.cpp`
exited0 with installed headers; `git diff --check` exited0. No new regression body
was executed by the reviewer before this approval. The coordinator owns the test
target, observed-red evidence and integration. Final code/CI review remains pending.

## Candidate implementation inspection

Inspected author commits `4ab8613` and `873cc6d4fba9663f3d6f8d5834d528442222b44b`,
source blob `e9ea7805c04af1bbd30f38946a40f5f483da7589`. This candidate is not the
final approved implementation: its normalized-zero finalizer handling is pending the
reviewed regression/red/fix sequence above.

The bounded ASCII cursor parser, complete-syntax-before-internal-resource precedence,
exponent accumulator (at most2569 before its over-limit sentinel), <=256-digit
coefficient, <=511 decimal power, exact signed construction and post-validation
canonical parserzero are consistent with revision1. No binary floating point, Qt,
network/provider or SQL dependency enters core. Sign/magnitude final rounding and
post-round checked signed64 conversion preserve the required tie/end-point behavior.
The known raw-zero denominator issue is the remaining implementation mismatch.

Independent fresh candidate checks in isolated `local-evidence/worktrees/core001-review`:

| Command | Actual result |
| --- | --- |
| `cmake --preset exact-numeric-debug` | exit0 |
| `cmake --build --preset exact-numeric-debug` | exit0 |
| `ctest --preset exact-numeric-debug -L exact-numeric` | exit0; original32/32 pass |
| `cmake --preset acceptance-debug` | exit0 |
| `cmake --build --preset acceptance-debug` | exit0 |
| `ctest --preset acceptance-debug -L 'acceptance\|regression'` | exit0; credit23/23 pass |

These are local candidate checks only, with installed Boost1.92.0/GCC16.2.1/
GoogleTest1.18.0. No final code verdict, automatic numeric CI result, Ubuntu/Windows
compatibility, application atomicity or provider/platform/live claim follows yet.


### Regression harness approval

Reviewed coordinator `bd9c43ba0f243d9cc5c64a460b59b8ade727ffa4`, including the
separate regression target/linkage and `exact-numeric;regression;core` discovery
labels. This target runs the exact reviewed test against the intended numeric
library without skipping, failure inversion or special expected-failure behavior.
Independent `cmake --preset exact-numeric-debug` and
`cmake --build --preset exact-numeric-debug` exited0;
`ctest --preset exact-numeric-debug -N -R ExactDecimalRegression` exited0 and
listed exactly the one new case, without executing it. The coordinator may now
record its genuine intended red against candidate `873cc6d` before authorizing the
finalizerzero fix. This harness approval leaves final implementation/workflow
approval and actual CI observation pending.
