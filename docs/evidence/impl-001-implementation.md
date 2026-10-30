# IMPL-001 — narrow time-credit implementation evidence

Author/implementer: `/root/credit_implementer`, separate from reviewer
`/root/credit_reviewer`. Date: 2026-10-30. Branch:
`task/IMPL-001-credit-implementation`, worktree `/tmp/subathon-impl001-implementation`,
from refreshed main `45a38d8`. Coordinator `/root` owns progress JSON/HTML,
current-status documentation and publication. This author owns the numeric source,
additive regression source/tests CMake and this evidence only.

Authority: specification v0.45 §3.4/3.5, §4, §18 and NUM-02/04;
[time-credit contract revision 1](../contracts/time-credit-v0.md).
[TEST-002 handoff](time-credit-baseline-handoff.md) records independently reviewed
19-case acceptance and actual intended red before IMPL-001. This assignment adds
no refunds, persistence, lifecycle/debt, UI or provider behavior.

## Preserved baseline

The original reviewed 19 acceptance cases, fixtures and public header remain
byte-identical to main. Their blobs are:

| Path | Blob |
| --- | --- |
| tests/acceptance/time_credit_tests.cpp | `055dec680f6ded7c457d14daf088115a0d73fa9f` |
| tests/fixtures/time-credit.json | `a1ab9d7b56e9eeaa1e429c25da5a522786320df3` |
| core/include/subathon/core/time_credit.hpp | `564d3c379fafcaa9862aab55e2ae7e15f86cd75a` |

No assertions are removed, skipped or weakened. The original `draft` acceptance
label remains historical metadata; independent review and observed-red evidence
identify the actual approved baseline. The unused public `FeatureNotImplemented`
class is retained to preserve the reviewed public header.

## Additive regression baseline

Commit `d3d1256` adds four cases in a separate executable using the same installed
GoogleTest and acceptance preset, labeled `regression;core`. These implement
existing revision-1 requirements rather than adding new product behavior. Expected
values below are independently derived from the contract; M is INT64_MAX and L is
INT64_MIN.

| Case | Literal input/oracle | Purpose / requirement |
| --- | --- | --- |
| SignedMinimumInputsAreRejectedBeforeArithmetic | `(L,M,M)`, `(0,L,0)`, `(M,M,L)` each throw `invalid_argument` | Revision-1 negatives/validation-first rule, including values that cannot safely be negated or subtracted before validation |
| AboveCapInputIsRejectedBeforeAnOverflowingAward | `(M,M,M-1)` throws `invalid_argument` | Revision-1 above-cap validation precedes final-overflow handling |
| FullLargestCapPreservesTheLargestRequest | `(M,M,M)` returns `(M,0,M)` | §3.4/4 requested versus actual credit: a full cap permits no additional time even for the largest request |
| LargestUncappedRequestCanBeCreditedFromZero | `(0,M,absent)` returns `(M,M,M)` | NUM-04/revision 1 representable exact boundary, no cap |

Regression test blob: `86a5c2a716204af292a8c72600a3ce6b27629097`.
Tests CMake blob at authoring: `a68401aa94d3a6ff7994d722caf80627f19fcf35`.
`/root/credit_reviewer` independently approved these exact authored blobs before
red execution and before replacement of the production stub; the coordinator and
reviewer received the approval through the agent handoff. The reviewer records
the durable verdict separately from this author's evidence.

At authoring commit `d3d1256`, after approval, `ctest --preset acceptance-debug
-L regression` actually exited 8: all four cases executed and failed because of
the explicit `FeatureNotImplemented` stub. Two valid-result cases threw the stub
exception; the two error cases rejected that wrong exception type (the signed-min
case has three exception assertions). The complete combined stdout/stderr is
retained in [the red log](impl-001-regression-red-ctest.txt). The subprocess wrapper
captured the command and propagated its exit 8. There were no build/dependency
failures, skips, expected-failure wrappers or CTest failure masking.

## Implementation

The helper validates all negatives and the above-cap condition before doing
arithmetic. For valid inputs it clips the applied request to cap headroom first,
then checks available signed-64-bit range by subtraction before addition. The
subtractions are safe because validation establishes `0 <= remaining <= cap`
where supplied, and `remaining <= INT64_MAX` by its type. This implements the
contract's checked-arithmetic option for already-integral milliseconds without
new dependencies, floating point, saturation or an overflowing intermediate.
Requested credit is preserved exactly in the returned proposal. Only the caller
will commit actual effects or hold input for review in later application work.

Production source blob after formatting:
`d87cc073ce470fe6b864ef16c75d1ca6aa8d0145`.
The four regression cases remain byte-identical to the reviewed/red baseline.

## Environment and actual commands

Linux 7.2.11-arch1-1 x86-64, GCC 16.2.1 (20260810), CMake 4.4.5,
Ninja 1.13.2, Python 3.14.8, installed GoogleTest 1.18.0. No dependency downloads
or additions were needed. Commands below run in the implementation worktree.

| Command | Actual outcome |
| --- | --- |
| `clang-format -i tests/acceptance/time_credit_regression_tests.cpp` | Exit 0; repository style |
| `cmake --preset acceptance-debug` | Exit 0; installed dependencies configured |
| `cmake --build --preset acceptance-debug` | Exit 0; original acceptance and additive regression binaries linked without warnings |
| `ctest --preset acceptance-debug -N -L 'acceptance\|regression'` | Exit 0; discovers 19 original acceptance plus four regression cases; executes none |
| `git diff 45a38d8 -- core/src/time_credit.cpp core/include/subathon/core/time_credit.hpp tests/acceptance/time_credit_tests.cpp tests/fixtures/time-credit.json` | Exit 0, empty before implementation |
| `git diff --check` | Exit 0 |
| `ctest --preset acceptance-debug -L regression` before implementation | Exit 8; 4/4 intended stub failures, raw log linked above |
| `clang-format -i core/src/time_credit.cpp` | Exit 0; repository style |
| `cmake --build --preset acceptance-debug` after implementation | Exit 0; rebuilt source and relinked all dependent binaries without warnings |
| `ctest --preset acceptance-debug -L acceptance` after implementation | Exit 0; 19/19 original reviewed acceptance cases pass |
| `ctest --preset acceptance-debug -L regression` after implementation | Exit 0; 4/4 independently reviewed regression cases pass |
| `ctest --preset acceptance-debug` after implementation | Exit 0; 26/26 (19 acceptance, four regression, three scaffold/tooling checks) pass |
| `git hash-object` on original tests, fixture, header and regression source | Exit 0; preserved baseline blobs listed above |

Independent final code review and coordinator integration/regression checks remain
required before completion. No live provider,
Windows, Wayland/OBS, packaging or full timer/application verification is claimed.
