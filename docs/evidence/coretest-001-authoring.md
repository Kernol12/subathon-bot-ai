# CORETEST-001 authoring handoff

Author: `/root/numeric_test_author`; date: 2026-10-31.
Branch: `task/CORETEST-001-author-resume`; based on current main
`fcdcd476fd28db1930e17e14ca0e9a912bed4798` (CONTRACT-002 PR11).
Actual authored baseline commit: `6e3038343bfb8cd32c87abca8057884ee148f0e1`.
Coordinator owns progress/state.json, PROGRESS.html, acceptance map and task PR.

## Changes and scope

[Exact decimal revision1](../contracts/exact-decimal-v1.md) refines specification
v0.45 §3.5/NUM-01/02/04 and the reviewed record/clock/writer boundary. It defines
owned source spelling plus exact rational values, complete JSON-number grammar,
positive-rate/nonnegative-factor validation and final signed64 half-even conversion
of already-scaled units. Limits/error precedence are explicit local engineering
choices for independent review, not provider capabilities.

Thirty-one deterministic GoogleTests use manually derived literal fractions/integers
and exact powers, independent of production calculations. They cover owned precision,
syntax/nonfinite input,512-byte/256-mantissa-digit/absolute-exponent256 boundaries,
positive rates/nonnegative factors including signed zero, both signs near half ties,
signed64 endpoints, exact unit scaling/no intermediate rounding, and4096/4097-bit
normalized components. The contract links these examples to their requirements.

The source contains only four unconditional `ExactNumericNotImplemented` throws.
No parser, validator or rounding algorithm is implemented. A separate explicit
`exact-numeric-debug` CMake lane builds this new target and its tests. Existing
scaffold/credit presets, source, assertions and fixtures are unchanged. No Qt/SQL,
provider, aggregate-state, refund, expression or review-transaction behavior is added.
Application callers still own durable review/atomic rejection of numeric failures.

## Exact reviewed-candidate artifacts

| Artifact | Git blob at6e30383 |
| --- | --- |
| `core/include/subathon/core/exact_decimal.hpp` | `c0e70aeea1f435fd07f9eb83b04f3ef5602e6c69` |
| `core/src/exact_decimal.cpp` explicit stubs | `9662e541cef66b975db1a9b6cb33cf421d5fb1f1` |
| `tests/acceptance/exact_decimal_tests.cpp` | `25504ae6d6d5f8b883281fd7c8de36d80b25ba4a` |
| `docs/contracts/exact-decimal-v1.md` | `42db740d530d362cf5821ffdcbd27524bbef05a4` |

These are authored candidates, not self-approved test gates. Separate reviewer
`/root/numeric_reviewer` receives the actual commit before any approval/red run.

## Commands actually run

Environment was checked afresh in the persistent isolated author worktree under
`local-evidence/worktrees/coretest001-author` (ignored local workspace): Linux
`7.2.11-arch1-1 x86_64`, GCC16.2.1, CMake4.4.5, Ninja1.13.2, Python3.14.8,
GoogleTest1.18.0 and installed Boost1.92.0. No package installation/download occurred.

| Command | Actual result |
| --- | --- |
| `uname -srmo` | exit0; Linux/x86-64 above |
| `c++ --version`, `cmake --version`, `ninja --version`, `python3 --version` | exit0; versions above |
| `pkg-config --modversion gtest` | exit0;1.18.0 |
| `rg '#define BOOST_LIB_VERSION' /usr/include/boost/version.hpp` | exit0; `1_92` |
| `cmake --preset exact-numeric-debug` | exit0; local installed dependencies discovered |
| `cmake --build --preset exact-numeric-debug` | exit0; all10 compile/link/discovery steps complete |
| `python3 tools/check_project.py` | exit0;377 tasks/122 decisions, links/presets/current HTML pass |
| `python3 tools/progress.py render --check` | exit0; current snapshot |
| `python3 -m unittest discover -s tests/tooling -q` | exit0;19/19 tooling tests pass |
| `git diff --check` | exit0 |

Build-time GoogleTest discovery lists case names; no numeric test bodies were run.
No CTest numeric red/pass result is claimed. After independent approval, the
coordinator must run `ctest --preset exact-numeric-debug -L exact-numeric` and record
actual behavioral failures from the explicit stub. Missing dependencies, failed
compilation or skipped tests cannot substitute for that intended red.

## CI assessment and next gates

This intentional-red suite is separate from green scaffold/credit CI, with no
failure inversion or skipped/disabled assertions. No workflow changes were made.
CORE-001 must enable stable numeric acceptance on automatic PR/main CI and observe
real dependency/build/test results before implementation completion. Only installed
Boost1.92 compilation is established here; proposed pinned Ubuntu Boost1.83 and
Windows/release compatibility are unverified.

Next: independent contract/test/harness review, any separately reviewed correction,
observed intended red, coordinator evidence/state/map/HTML integration and focused
baseline PR gates. Only after the reviewed/red baseline integrates may CORE-001
implement these functions. All aggregate/transaction, domain, provider/platform,
renderer/UI/localization, packaging and live/OBS release gates remain separate.
