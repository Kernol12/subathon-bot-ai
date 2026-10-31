# ACCT-003 captured application-plan implementation

Implementer `/root/application_plan_implementer`, 2026-10-31, local support branch
`task/ACCT-003-implementation`, based on coordinator claim `8b8e80e` from
main `8ed9d45`. Ownership: only `core/src/application_plan.cpp` and this evidence.
Specification v0.45 §§3/6/7; application-plans revision1 is the behavioral contract.

## Baseline and change

Read root/core/tests instructions, FIRST_AGENT_PROMPT, architecture/workflow/test
strategy, CLion setup, task brief, specification sections and the complete contract,
baseline handoff and independent review. The reviewed test gate was approved in
`36ac654` with final integration review `9aea69e`; the handoff retains the actual
22 intended stub failures after successful build at `a524447`. Coordinator supplied
PR22 merge `8ed9d45`, final `c66870e` with all eight CI checks passing.

The implementation captures owned revision1 values and ordered source results;
appends authorized, audited revisions while preserving original history and whole
event identity/facts/arrival/ancestry/contribution link; and checks preview freshness
with plan staleness preceding state staleness. Shared local validation rejects
malformed captures and histories, including later parent/audit/event inconsistencies.
Missing historical valuation never manufactures a rate or dependent zero request.
Independent holds, exact fractions, signed requested amounts and original decimal
spellings remain supplied values. Source calls receive the owned captured event.
No matching, numeric parsing/rounding or modifier-combination algorithm is duplicated.

Stale heads and incomplete/unauthorized decisions fail before source calls. Revision
increment is checked against uint64 maximum. As recorded by baseline review, runtime
exhaustion requires an unrealizably large valid contiguous vector; no runtime overflow
coverage is claimed. Exceptions leave the caller's original value untouched. Returned
copies still require caller writer ownership and atomic expected-head enforcement.

## Actual verification

Linux x86_64; GNU C++16.2.1; CMake4.4.5; Python3.14.8; GoogleTest1.18.0;
installed Boost1.92.0 headers, reusing selected Boost>=1.83. Commands ran in the
isolated implementation worktree with disposable build outputs and synthetic fixtures.

| Command | Observed result |
| --- | --- |
| `clang-format -i core/src/application_plan.cpp` | exit0 |
| `cmake --preset application-plan-debug` | exit0 |
| `cmake --build --preset application-plan-debug` | exit0;10 steps, no diagnostics |
| `ctest --preset application-plan-debug -L application-plan` | exit0;22/22 unchanged cases pass |
| `ctest --preset application-plan-debug` | exit0;25/25, including scaffold infrastructure |
| `cmake --preset exact-numeric-debug` | exit0 |
| `cmake --build --preset exact-numeric-debug` | exit0 |
| `ctest --preset exact-numeric-debug -L exact-numeric` | exit0;33/33 pass, including numeric regression |
| `git diff --check` | exit0 |
| `git hash-object core/src/application_plan.cpp core/include/subathon/core/application_plan.hpp tests/acceptance/application_plan_tests.cpp` | blobs below |

Implementation source blob: `42b47e4ae4c733ba8e0d8f743c32faa15376d425`.
Unchanged reviewed interface: `501112d64959d20f682420d33a260e7745f5b378`.
Unchanged reviewed22 cases: `f93287fa2c7c3ef4753e6bbbedd1d2a54f4529b9`.
No baseline assertion, fixture, interface, contract, shared build or state was edited.

## Remaining gates and limits

Separate code review and coordinator integration, stable application-plan labels,
pinned automatic CI observation, progress/evidence updates and PR remain required.
This support branch remains local; coordinator owns publication and status. The
local lane still displays its historical intentional-red label until coordinator
updates it; its actual body result above is passing, with no failure inversion.

These are bounded pure core simulation checks. Current preview freshness grants no
permission/eligibility, capacity or effects authority. The authorized enum records
an already evaluated upstream precondition rather than implementing authentication
or permissions. Historical reference durability, actual evaluator correctness,
current caps/lifecycle/debt, atomic application/effects/jobs, SQL/Qt writer,
concurrent persistence, provider capabilities, live services and platform/release
checks remain downstream. No network, credentials, clocks or live effects were used.
All seven integrations and three rate providers remain required release scope.
