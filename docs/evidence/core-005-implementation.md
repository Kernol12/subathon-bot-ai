# CORE-005 — checked deficit and time-debt proposals

Implementer `/root/debt_implementer`, 2026-11-01, isolated branch
`task/CORE-005-implementation`, based on coordinator claim05961a8 and integrated
mainbca06e8. Source candidate `6394cab0032b1f737fc6f6c6016d63a9403d0659`, blob
`68141b15a4578cc85b4537a9b1403638c6e445f2`.

## Authority and preserved baseline

Specification v0.45 §4/NUM-03 and [time-debt revision1](../contracts/time-debt-v1.md).
The merged [baseline handoff](coretest-005-handoff.md) records independent
reviewb3413e5/final5474fdc and actual32-case intended stub-red atdba4401 before
production implementation. This author did not repeat or claim to author that red.

These reviewed artifacts remain unchanged, checked with `git hash-object`:

| Artifact | Exact blob |
| --- | --- |
| Contract revision1 | `909f16e7ee6f4e9de6dc31acc728236032d51482` |
| Public interface | `f3cc819c16f3df907c0b68b0738e3aaef4f51950` |
| 32 reviewed cases | `c3cd6c65dcdb7e8b20c8c71140256787788e8790` |

Only `core/src/time_debt.cpp` supplies production behavior. No test, fixture,
header, contract, older numeric/lifecycle source, build file or dependency changed.

## Implementation and arithmetic ownership

Both entry points validate their local inputs before returning a proposal.
Negative balances, invalid request signs/references, invalid deficit enums and
negative supplied caps reject with `std::invalid_argument`. No input is mutated.
Returned requests and references are owned copies of the complete supplied values.

For reversals, compare the nonpositive signed request with `-Main`, whose value is
always representable because Main is nonnegative signed64. A fitting request applies
fully under every policy; equality/zero create no deficit hold. RequireReview on a
deficit returns both balances unchanged and the entire signed request unapplied.

For Stop/Carry deficits, `request + Main` adds opposite-signed values and remains
nonpositive, including INT64_MIN when Main is zero. Stop retains that signed
remainder directly. Carry first compares it with negative available debt headroom;
only after proving its negation and final debt fit does it negate/add. Thus MIN can
stop or be held unchanged, and MIN with Main1 can create exactly debtMAX, without
signed overflow, saturation, magnitude conversion or a partial proposal.

For additions, repay `min(existing debt, requested)` before considering Main.
Remaining request and reduced debt are safe nonnegative subtractions. A supplied
cap provides nonreducing headroom: at/above cap means zero Main credit, with debt
repayment retained and the remainder explicitly capped. Without a cap, compare the
remainder against `MAX - Main` before addition. Every returned balance/component is
representable, and the requested amount is partitioned into debt, Main and capped
components without a floating-point calculation or an overflowing hypothetical sum.

The implementation uses only existing C++ standard facilities. No external library
or new dependency is needed for these checked fixed-width operations. The older
time-credit helper and its narrower above-cap precondition remain unchanged.

## Actual checks

Environment observed with uname/compiler/CMake/Ninja/Python/pkg-config commands:
Linux7.2.11-arch1-1/x86-64, GNU16.2.1, CMake4.4.5, Ninja1.13.2,
Python3.14.8, GoogleTest1.18.0 and clang-format23.1.4.

Commands in the isolated implementer worktree:

| Command | Actual result |
| --- | --- |
| `cmake --preset time-debt-debug` | Exit0 |
| `cmake --build --preset time-debt-debug` | Exit0, warning-free |
| `ctest --preset time-debt-debug -L time-debt` | Exit0,32/32 reviewed cases pass |
| `ctest --preset time-debt-debug` | Exit0,35/35 including32 cases and3 infrastructure checks |
| `clang-format --dry-run --Werror core/src/time_debt.cpp` | Final exit0 |
| `git diff --check` | Exit0 |

The first format dry-run identified aggregate wrapping; the repository formatter
was applied to the owned source only. Rebuild and fullpreset35/35 were then observed
again on the final formatted source. No behavioral assertion was changed or skipped.

## Handoff and remaining gates

The coordinator owns CI promotion, build/label/progress/map/checkpoint integration,
publication and merge. Separate actual-source/config approval and an observed
ordinary Ubuntu32-case acceptance run are still required. This support branch is
not pushed; no task completion, PR or merge is claimed by the implementer.

This is a pure proposal library. Grace/lifecycle/hard-end composition remains
CORE-002/caller-owned; no timer state machine is duplicated. Proportional refund
entitlement, original award/rate/reversal allocation, audit/permission/review
resolution, deduplication, atomic state/effects/jobs, persistence, actual services,
platform/deployment/recovery and full release verification remain their explicit
downstream gates. All seven integrations and three rate providers stay in scope.
