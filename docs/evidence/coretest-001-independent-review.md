# Independent review — CORETEST-001

Author: `/root/numeric_test_author`; reviewer: `/root/numeric_reviewer`.
Date: 2026-10-31. Verdict: **approved for test baseline**.

Reviewed author commits `6e3038343bfb8cd32c87abca8057884ee148f0e1`
(contract/API/stubs/harness/tests), `45fa0c0` (author evidence) and
`b3b7ab9369f1baaf119e834bc255dd8ec41a1605` (additive review correction).
These were integrated only into the isolated reviewer branch
`task/CORETEST-001-review-resume`; the coordinator retains shared progress/HTML.
The base is integrated main `fcdcd476fd28db1930e17e14ca0e9a912bed4798`.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| [Exact-decimal contract revision 1](../contracts/exact-decimal-v1.md) | `59d334d68970d1b1dd81fddcfbb2a8118437ce13` |
| [32 acceptance cases](../../tests/acceptance/exact_decimal_tests.cpp) | `6cf83aeba79bfc4c50e3e11787147dd6bf342c10` |
| [Public interface](../../core/include/subathon/core/exact_decimal.hpp) | `c0e70aeea1f435fd07f9eb83b04f3ef5602e6c69` |
| [Explicit unimplemented source](../../core/src/exact_decimal.cpp) | `9662e541cef66b975db1a9b6cb33cf421d5fb1f1` |

## Specification and scope assessment

Read the entire contract, interface, four unconditional stubs, 32 acceptance cases,
CMake changes, task brief and authoring evidence against specification v0.45
§3.5, NUM-01/02/04, the §18 precision/overflow scenarios and the reviewed internal
boundary contract revision 1. No blocking requirement conflict remains.

The decimal grammar, 512-byte/256-mantissa-digit/absolute-exponent-256 limits,
4096-bit normalized finalizer component limits and exception API are identified as
local engineering choices. They do not invent provider capabilities or change the
agreed exact accounting. The API owns original spelling alongside an exact rational;
private construction prevents callers from fabricating an inconsistent pair. Tests
cover trailing zeros, exponent spelling, signed zero, non-binary-representable decimal
precision and caller-buffer mutation. Generic signed decimals, strictly positive rates
and nonnegative factors are separate, with non-finite spellings rejected.

The finalizer receives already-scaled exact milliseconds or micro-points and checks
signed-64 range after nearest-even rounding. It cannot persist a review hold or
transactionally apply effects. The contract explicitly leaves those responsibilities,
aggregate checks, modifier group clamping, refund policy and broader calculation
resource budgets in their downstream baselines. This does not shrink release scope.

## Oracle and boundary assessment

Expected fractions and integers are independently derived from decimal place value,
unit scaling and parity, not from the implementation under test. Boost constructors
and powers represent the literal reference values; the production parser/finalizer
never computes another expected result. Manually checked every case, including:

- `1.2300` is `123/100`; `1.2300E-02` is `123/10000` while original spellings remain.
- Positive half ties `1/2,3/2,5/2,7/2` yield `0,2,2,4`; the negative cases yield
  `0,-2,-2,-4`. Both sides of the half remain distinct for each sign.
- Signed-64 maximum is odd, so maximum plus one half rounds beyond range. Minimum
  is even, so minimum minus one half rounds to minimum; subtracting `5001/10000`
  instead must overflow. Exact endpoints and out-of-range integers are covered.
- Scaling `0.0005/0.0015` seconds or `0.0000005/0.0000015` points produces exact
  half/one-and-a-half final units, yielding `0/2`. Finalizing `(1/3)*3` yields one.
- 256 nines equal `10^256-1`; exponent magnitudes 256 and 257 form the accepted and
  rejected boundary. Excessive exponent is rejected even for a zero mantissa.
- `1e` plus 510 zeros is exactly 512 bytes and exponent zero; one more byte exceeds
  the first budget. Every declared resource has an accepted boundary and rejection
  immediately beyond it.
- `2^4095/(2^4095+1)` has coprime 4096-bit components and rounds to one. A
  4097-bit numerator fails before range conversion; a 4097-bit denominator fails
  even when the result would otherwise round to zero.

One review finding requested an additive oracle for complete-grammar precedence over
internal mantissa/exponent limits within the byte budget. Author `b3b7ab9` added
`CompleteSyntaxPrecedesInternalResourceLimits`: 257 digits plus junk and `1e257?`
throw `invalid_argument`; the existing malformed 513-byte `length_error` assertion
is unchanged. No existing expectation was deleted, skipped or weakened. This checks
the explicit contract policy and is not a product requirement change.

## Independent compilation and checks

In persistent isolated worktree `local-evidence/worktrees/coretest001-review`:

| Command | Actual result |
| --- | --- |
| `cmake --preset exact-numeric-debug` | exit 0; fresh configuration, GCC 16.2.1, Python 3.14.8, installed Boost config discovered |
| `cmake --build --preset exact-numeric-debug` | exit 0; complete baseline built; amended test rebuilt and linked successfully |
| `ctest --preset exact-numeric-debug -N -L exact-numeric` | exit 0; final 32 cases discovered; listing only, no test bodies executed |
| `python3 tools/check_project.py` | exit 0; 377 records, 122 decisions, links/presets/current HTML pass |
| `python3 tools/progress.py render --check` | exit 0; current snapshot |
| `python3 -m unittest discover -s tests/tooling -q` | exit 0; 19/19 tooling cases pass |
| `git diff --check` | exit 0 |
| `git hash-object docs/contracts/exact-decimal-v1.md tests/acceptance/exact_decimal_tests.cpp core/include/subathon/core/exact_decimal.hpp core/src/exact_decimal.cpp` | exit 0; exact blobs above |

Actual compiler is GCC 16.2.1, CMake 4.4.5 and GoogleTest pkg-config 1.18.0;
CMake cache resolves Boost and headers configs to 1.92.0. No dependency was fetched
or installed. All four numeric entry points still throw `ExactNumericNotImplemented`
unconditionally; discovery/compilation is not a passing behavior result. The existing
credit sources, assertions and fixtures are unchanged.

## CI and remaining gates

The new optional exact-numeric lane is explicitly separate from the existing passing
credit/scaffold presets and workflows. Its cases are intentionally unimplemented and
carry `exact-numeric;acceptance;intentional-red` labels; there is no skip, failure
inversion or blanket continue-on-error. It is appropriate to record the genuine red
run separately rather than make intentional failures a green merge gate. CORE-001
must add the implemented suite to automatic PR/main CI, observe dependency/build/test
results and assess required-check configuration before implementation completion.

This approval permits the coordinator to run the exact reviewed baseline and record
intended failures from the explicit stubs. **No numeric test body was run by this
review before approval.** CORETEST-001 still requires observed intended red, integrated
checks/evidence/state, latest PR CI and merge. CORE-001 must wait for the reviewed/red
baseline, preserve these assertions and obtain separate implementation review/pass
checks. This review is neither a production code approval nor a GitHub review that
could replace required repository approvals.

Ubuntu/Boost 1.83 compilation, Windows, service atomicity, live providers, renderers,
UI/localization, packaging and OBS/platform verification remain unperformed here.
No credentials or supporter data were used.
