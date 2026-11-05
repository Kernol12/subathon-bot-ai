# ACCESS-002 — focused arithmetic/allocation regression author handoff

2026-11-05, /root/access_implementer, separate
`task/ACCESS-002-regression-author`, `/tmp/subathon-access002-regressions`.
Base4765bdc. Owns only the NEW regression source and this note. The original30
assertions, fixture, public header, implementation and root harness are unchanged.
Different actual test review is required before invoking either new test.

## Public-contract oracles

Authority: specification v0.45 §10/AUTH-03/06 and
[effective-permissions API](../contracts/effective-permissions-api-v1.md) §3/4/5.

Two tests in the intended isolated executable:

- `EffectivePermissionRegression.UnequalLongRatiosAndCancellationStayExact`
- `EffectivePermissionRegression.AllocationFailuresTranslateAndPreflightStaysAllocationFree`

The first has nine literal direction cases. N=10^80, independently constructed
using fixed decimal digits, not SUT parsing/formatting. Since
(N+1)^2 - N(N+2)=1, (N+1)/N exceeds (N+2)/(N+1) by exactly
1/[N(N+1)]; both operands are reduced and the comparison cannot cancel common
numerator/denominator factors. Reversing the pair reverses the direction. The
common-numerator pairs use (N+1)/(N-1) and (N+1)/(N+3); both are reduced because
N±1/N+3 are odd and possible common factors divide2. Equal positive numerator
means smaller positive denominator is larger. The common-denominator pair
(N+1)/N and (N+3)/N is reduced because neither numerator has factor2 or5;
its order follows the numerators. Three negative cases reverse the independently
known orders. Permission is exactly requested<=AtMost(bound), with complete
source match, reason, witness and retained request-value assertions. Expected
results are hard-coded mathematical consequences, never calculated by an
implementation helper or generic comparison algorithm in test setup.

The second uses a strict permit with unequal long ratios and128-byte owned context,
source and grant strings. Fixtures and all expected strings exist before arming.
The standalone executable's replacement global new/new[] and aligned/nothrow/
sized-delete companions use malloc/free; only the current thread's explicitly
armed allocation event can throw the synthetic bad_alloc. The hook fires once
per trial. Direct evaluator invocation, inert optional move/capture and catch-code
capture occur inside the hook scope. Hook is disarmed before GTest/trace/diagnostic
work or caller-owned result copies. Each encountered allocation is injected in
turn until the first full success. Every injection must return the typed fixed
allocation_failure, never raw bad_alloc/another exception/partial decision. An
8192-trial termination guard avoids hanging a broken test; it asserts no fixed
allocation total or internal scratch strategy. At least one injection must occur.
At first success the complete result is retained, the caller fixture is cleared,
and complete request/source/context ownership and permit evidence are checked.

Under an armed first-allocation fault, zero result_items must yield invalid_limits
and an already oversized reference must yield input_limit, both with zero allocator
calls/injection. Those assertions exercise the approved preflight ordering rather
than attempting to infer every internal allocation's purpose. This is fault
simulation, not hardware exhaustion/real-OOM recovery/platform proof.

## Actual commands and execution boundary

```sh
clang-format -i tests/acceptance/effective_permissions_regression_tests.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Icore/include \
  -c tests/acceptance/effective_permissions_regression_tests.cpp \
  -o /tmp/subathon-permission-regressions.o
git diff --check
```

All exit0, no compiler warning. No executable was linked/launched and neither new
feature test/hook body was invoked. Root owns the isolated target/labels/discovery/
ordinary CI and must prevent these replacement allocation functions from being
linked into the original baseline, environment or product executables. Actual
different test/harness review precedes first invocation. Existing reviewed/red
baseline already establishes the production behavior gate; no additional original
stub-red or retroactive new-regression-red claim is made.
