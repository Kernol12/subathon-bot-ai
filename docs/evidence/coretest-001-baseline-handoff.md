# CORETEST-001 reviewed/red baseline handoff

Coordinator: `/root`, 2026-10-31, branch `task/CORETEST-001-exact-baseline`.
Base main `fcdcd476fd28db1930e17e14ca0e9a912bed4798`; draft
[PR #12](https://github.com/Kernol12/subathon-bot-ai/pull/12).
Specification v0.45 §3.5/NUM-01/02/04; exact-decimal contract revision1.

## Sequence and exact artifacts

Author `/root/numeric_test_author` committed contract/interface/stubs/harness/tests
at `6e30383`, author evidence at `45fa0c0`, and the reviewer-requested additive
error-precedence case at `b3b7ab9`. Independent `/root/numeric_reviewer` approved the
final 32-case revision in `79e0f0d`; see [review](coretest-001-independent-review.md)
and [author handoff](coretest-001-authoring.md). The additive test preserves all
prior assertions and makes the contract's complete-grammar-before-resource policy
executable; it changes no requirement or implementation.

| Artifact | Exact final baseline blob |
| --- | --- |
| Contract revision1 | `59d334d68970d1b1dd81fddcfbb2a8118437ce13` |
| Acceptance tests | `6cf83aeba79bfc4c50e3e11787147dd6bf342c10` |
| Public header | `c0e70aeea1f435fd07f9eb83b04f3ef5602e6c69` |
| Explicit stub source | `9662e541cef66b975db1a9b6cb33cf421d5fb1f1` |

The coordinator integrated the reviewed artifacts at `9e8db8a` before running
numeric test bodies. No production algorithm was written. Prior credit source,
acceptance/regression tests and fixtures remain unchanged.

## Actual intended red

Environment: Linux x86-64; GCC16.2.1, CMake4.4.5, Python3.14.8,
installed Boost1.92.0 and GoogleTest1.18.0. No dependency installation/download.
Author and reviewer independently configured and compiled the baseline before
review approval; reviewer discovery used `ctest -N`, not test-body execution.

Coordinator commands in the main workspace:

| Command | Actual result |
| --- | --- |
| `cmake --preset exact-numeric-debug` | exit0; installed Boost/GTest found |
| `cmake --build --preset exact-numeric-debug` | exit0; compile/link/discovery complete; rebuilt final additive case successfully |
| `ctest --preset exact-numeric-debug -L exact-numeric` | exit8;32/32 actual failures from `ExactNumericNotImplemented`; no skips/build/dependency failures |

Full [CTest red output](coretest-001-red-ctest.txt) is retained. Valid-result cases
throw the explicit missing-behavior exception, while error cases receive that same
wrong exception type instead of their specified validation/resource/overflow error.
These are intended behavioral failures, not passing acceptance. The Python capture
wrapper returned CTest's actual exit8; no failure inversion or continuation-to-green
was used. The baseline task can complete with reviewed tests plus this observed red;
CORE-001 is still unimplemented.

## Integration, CI and next gates

The numeric preset is explicitly separate while red. Existing scaffold and credit
CI stay enabled and must pass on the final PR revision. CORE-001 must enable the
stable numeric suite in automatic PR/main CI, pin the selected Boost dependency
setup and observe actual workflow results before completion. Ubuntu Boost1.83,
Windows/release builds, application-level numeric review/atomic aggregate checks
and all provider/platform/live/OBS behavior remain unverified.

Coordinator integration commands/results are recorded in the task note/checkpoint
and final PR. Next: finish tooling/credit/scaffold regression checks and final
integration review; publish ready PR12 and merge only after latest CI/discussions/
repository gates. Start CORE-001 from merged main, preserve reviewed assertions,
implement only these numeric primitives, obtain separate code review and passing
numeric/credit checks. No permission is needed for routine authorized task work.
