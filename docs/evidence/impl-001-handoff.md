# IMPL-001 — numeric time-credit implementation handoff

Coordinator: `/root`, 2026-10-30. Specification v0.45, time-credit contract revision 1.
Branch `task/IMPL-001-credit-coordination` starts from refreshed main `45a38d8`.
Implementation author: `/root/credit_implementer`; independent reviewer:
`/root/credit_reviewer`. Publication revision is the containing Git commit.

## Scope, isolation and unchanged baseline

This implements only `calculate_time_credit`: validate the restricted nonnegative
input domain, preserve requested credit, apply at most available cap headroom,
and signal an unrepresentable final sum without evaluating an overflowing signed
intermediate. No floating point or new dependency is needed for checked integer
addition. No refund, persistence, UI, lifecycle, debt, rounding or application
transaction behavior is included. Those remain later release work; the helper
cannot enforce eligibility or current authoritative state absent from its inputs.

All writer branches/worktrees were created from current main `45a38d8`:

| Role | Branch / worktree | Ownership |
| --- | --- | --- |
| Coordinator `/root` | `task/IMPL-001-credit-coordination`, `/tmp/subathon-impl001-coordinator` | progress JSON/HTML, current-status documentation, root build metadata/workflow, acceptance-map integration and publication |
| Implementer `/root/credit_implementer` | `task/IMPL-001-credit-implementation`, `/tmp/subathon-impl001-implementation` | numeric source, additive regression source/tests CMake, implementation evidence |
| Independent reviewer `/root/credit_reviewer` | `task/IMPL-001-credit-code-review`, `/tmp/subathon-impl001-review` | separate test/code review evidence and independent replay |

TEST-002 was done before claim `a442b92`; its [handoff](time-credit-baseline-handoff.md)
records all 19 reviewed cases failing on the explicit stub at `f30e293` after
successful compilation. The original acceptance source, fixture and public header
remain byte-identical to the approved baseline:

| Path | Git blob |
| --- | --- |
| tests/acceptance/time_credit_tests.cpp | `055dec680f6ded7c457d14daf088115a0d73fa9f` |
| tests/fixtures/time-credit.json | `a1ab9d7b56e9eeaa1e429c25da5a522786320df3` |
| core/include/subathon/core/time_credit.hpp | `564d3c379fafcaa9862aab55e2ae7e15f86cd75a` |

The contract's numeric/error policies remain revision 1. Only status/evidence prose
was updated after TEST-002. The public `FeatureNotImplemented` type remains for
header compatibility but is no longer thrown. The historical CTest `draft` label
is retained; execution and review evidence identify the passing numeric slice.

## Authorship and independent gates

Author test-only commit `d3d1256` (integrated `04a8742`) adds four distinct
signed-limit/validation regressions. The independent reviewer approved the exact
new source before red and before implementation (durable review `d5c7202`).
[The raw regression red run](impl-001-regression-red-ctest.txt) records actual exit 8,
four executed stub failures; it is separate from dependency/build errors.

Author implementation `29325b5` (integrated `4d50676`) supplies the numeric helper,
source blob `d87cc073ce470fe6b864ef16c75d1ca6aa8d0145`. See
[author evidence](impl-001-implementation.md) for independently derived test oracles,
exact commands, red history and author results (19/19 acceptance, 4/4 regression,
26/26 full preset). [Independent code review](impl-001-code-review.md), original
reviewer commits `d5c7202` and `46de20f` (integrated `3b02bb8` and `7be1300`),
approves the exact implementation with no blocking findings. The separate reviewer
clean-configured/built and independently passed acceptance 19/19, regression 4/4
and the full preset 26/26. No implementation author self-certifies review.

## Coordinator environment and validation

Linux 7.2.11-arch1-1 x86-64, GCC 16.2.1 (20260810), CMake 4.4.5,
Ninja 1.13.2, Python 3.14.8, installed GoogleTest 1.18.0. No new dependency.
Commands below were run in the coordinator worktree after integrating the source,
test and review commits, with current-status documentation present. All exited 0.
No live services are exercised.

| Command | Observation |
| --- | --- |
| `cmake --preset acceptance-debug` | Installed GoogleTest; no fetch |
| `cmake --build --preset acceptance-debug` | Clean configure/build, no warning output |
| `ctest --preset acceptance-debug -L acceptance` | 19/19 original acceptance cases pass |
| `ctest --preset acceptance-debug -L regression` | 4/4 additive regressions pass |
| `ctest --preset acceptance-debug` | 26/26 including three infrastructure checks |
| `cmake --preset scaffold-debug` | Configured |
| `cmake --build --preset scaffold-debug` | Built |
| `ctest --preset scaffold-debug` | 3/3 infrastructure checks pass |
| `python3 -m unittest discover -s tests/tooling -v` | 19/19 pass after reviewed fixture correction |
| `python3 tools/check_project.py` | 375 task records, 122 decisions, links/presets/current HTML pass |
| `python3 tools/progress.py render --check` | Snapshot current |
| `node tools/check_progress_ui.mjs` | Offline controller DOM simulation passes; no real browser/OBS |
| `git diff --check` | No whitespace errors |
| `git diff --exit-code 45a38d8 -- tests/acceptance/time_credit_tests.cpp tests/fixtures/time-credit.json core/include/subathon/core/time_credit.hpp` | Empty diff; all three approved blobs rechecked with `git hash-object` |

The original stub red and new regression red logs remain intact. No test outcome
is inverted, no assertion weakened, and no skipped or missing-dependency case is
counted as passing. Qt-specific, live-provider and platform checks were not rerun: no
Qt, transport, UI, database or provider path changed. Prior Qt/CLion evidence remains
historical environment evidence, separate from this numeric task.

## Remaining gates and next context

All seven integrations, all three rate providers, Windows/Linux client support,
Wayland/OBS, packaging, authoritative timer/transaction state and live/provider
verification remain in release scope. This assignment changes no such capability.
SPIKE-001's bounded feasibility evidence remains separate and approves no production
reuse. Later tasks must read the revision-1 caller responsibilities and TEST-002
handoff before building lifecycle, debt, hard-end, persistence or reversal contracts.

The final record rerun (with IMPL-001 done) passed Python 19/19, full acceptance
preset 26/26, scaffold 3/3, project consistency, current HTML, DOM simulation and
whitespace checks. The production validator, public header, original 19 acceptance
cases and fixture are unchanged.

Changed paths include the numeric source, four additive regression cases and their
CMake target, implementation/red/review/handoff evidence, current-status docs and
contract status prose, acceptance-map scenario evidence, CMake/preset/workflow names,
progress/state.json and generated PROGRESS.html. The manual acceptance workflow now
executes both acceptance and regression labels without hiding failures.

Publication/CI/merge result: pending. IMPL-001 local acceptance and separate code
review gates are satisfied; progress JSON/HTML records the numeric task only.
Final publication still requires latest-revision CI and repository merge gates.

The first final-record tooling rerun after marking IMPL-001 done exited 1:
`test_completed_implementation_requires_passing_acceptance` mutated live IMPL-001
without clearing its newly passing acceptance result, so its expected ValueError
was absent (17/18 tooling cases passed). This is a test-fixture defect, not a C++
acceptance failure or a reason to falsify progress. The task returned to in_review
while a separate author/reviewer corrected the fixture with a specification-linked
reason. No validator or product assertion was changed. Separate author correction
`3864b15` replaces the borrowed live task with synthetic IMPL-999, retains missing
acceptance rejection, adds failed-acceptance rejection and accepts the complementary
passing case with separate review. [Fixture correction evidence](impl-001-progress-fixture-correction.md)
records the specification-based reason and independent author reproduction.
Independent correction approval `1453195` confirms the exact fixture with
Python 19/19, completed-state injection 2/2 and full CTest 26/26 replay. The
validator and all product baseline blobs remain unchanged. Final integrated
checks were repeated after the correction and honest completion record.
