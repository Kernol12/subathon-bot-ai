# CORE-001 implementation evidence

Author: `/root/numeric_implementer`; date: 2026-10-31.
Branch: `task/CORE-001-implementation`, based on merged main
`cab329b17ca3edb02e408ad1bc7244a73ae20c3d` (baseline PR12).
Scope: core/src/exact_decimal.cpp and this evidence only. Coordinator owns shared
build/dependency/CI files, progress/HTML, integration and the focused task PR.

## Contract and implementation

Implements [exact-decimal revision 1](../contracts/exact-decimal-v1.md), specification
v0.45 §3.5 / NUM-01/02/04, after the separately reviewed and observed-red
[CORETEST-001 handoff](coretest-001-baseline-handoff.md) was merged.
The implementation validates complete ASCII JSON-number grammar before internal
resource errors, builds exact integer/rational values without floating conversion,
preserves the owned original spelling, and shares the parser for rate/factor domains.
The finalizer uses integer quotient/remainder and parity for signed half-even
rounding, then checks the rounded integer before signed-64 conversion.

Initial source commit `4ab8613` compiled and passed31/32 original numeric cases;
the existing signed-zero factor assertion failed because Boost1.92 division of
zero by100 retained `0/100`, which compared unequal to integer zero. This was an
implementation/dependency interaction, not a defective assertion. Commit `873cc6d`
returns canonical rational zero after all parser grammar/resource checks while
preserving its source spelling; all32 unchanged numeric cases then passed.
Source candidate blob: `e9ea7805c04af1bbd30f38946a40f5f483da7589`.

The original reviewed header, tests and contract are unchanged:

| Artifact | Exact blob |
| --- | --- |
| Public header | `c0e70aeea1f435fd07f9eb83b04f3ef5602e6c69` |
| Original32 numeric tests | `6cf83aeba79bfc4c50e3e11787147dd6bf342c10` |
| Contract revision1 | `59d334d68970d1b1dd81fddcfbb2a8118437ce13` |

## Actual checks on candidate873cc6d

Persistent isolated worktree: `local-evidence/worktrees/core001-implementation`.
Actual local GCC16.2.1, CMake4.4.5, GoogleTest1.18.0 and Boost1.92.0 headers.
No package was installed or downloaded by this author.

| Command | Actual outcome |
| --- | --- |
| `cmake --preset exact-numeric-debug` | exit0; fresh configure with installed dependencies |
| `cmake --build --preset exact-numeric-debug` | exit0; implementation/tests compiled and linked |
| `ctest --preset exact-numeric-debug -L exact-numeric` | exit0; original32/32 pass after parser-zero correction |
| `cmake --preset acceptance-debug` | exit0; fresh credit configuration |
| `cmake --build --preset acceptance-debug` | exit0 |
| `ctest --preset acceptance-debug -L 'acceptance\|regression'` | exit0; credit23/23 pass |
| `ctest --preset exact-numeric-debug` | exit0; full preset35/35, including three infrastructure cases |
| `python3 tools/check_project.py` | exit0;377 tasks/122 decisions and local consistency pass |
| `python3 -m unittest discover -s tests/tooling -q` | exit0;19/19 tooling tests pass |
| `git diff --check` | exit0 |
| `git hash-object` on header/original tests/contract | unchanged exact blobs above |

The retained `intentional-red` CTest metadata belongs to the original baseline;
these observed executions passed. Coordinator owns changing suite/status prose and
activating the stable numeric CI lane. Passing these primitive cases establishes
no application transaction, aggregate-state, refund, modifier engine or UI behavior.

## Additional normalized-zero finding and pending gates

An ignored scratch probe compiled against this candidate used:
`const Exact zero = Exact(0) / (cpp_int(1) << 4096);`.
On Boost1.92 it reported numerator0, denominator4097bits and equality-to-zero false;
the current finalizer throws `length_error`. The contract budgets normalized
components, so mathematical zero should normalize to0/1 and return0.

Coordinator and independent reviewer agreed this needs one meaningful additive
regression rather than changing an existing expectation. Original test author owns
that separate regression. Finalizer changes are paused until its actual test revision
receives separate review and an observed intended red against this candidate.
No regression case, assertion, header or contract was edited by this implementer.

Independent code approval, the additional reviewed/red regression and fix,
coordinator integration, automatic pinned-dependency numeric CI observation and
latest-head PR/merge gates remain pending. Ubuntu/Boost1.83, Windows/release,
all services, platform/renderers, UI/localization, packaging and OBS remain outside
this author's local validation. No credentials, supporter state or live side effects
were used.
