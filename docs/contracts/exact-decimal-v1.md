# Exact decimal and final effect conversion — revision 1

Task: CORETEST-001 / CORE-001. Authority: specification v0.45
[§3.5](../specification.md#35-numerical-precision-and-validation), NUM-01/02/04 and
§18 precision/overflow scenarios. This bounded contract refines the reviewed
[internal boundary contract](event-state-boundaries-v1.md), not its ownership rules.
The existing time-credit revision 1 is unchanged.

Status: CORETEST-001 completed independent review and observed stub-red before
CORE-001 implemented this interface. [Implementation evidence](../evidence/core-001-handoff.md)
records passing checks and separate code review. Revision1 API, limits, errors and
expected examples below are unchanged; only status prose is updated.

## Public boundary and scope

`core/include/subathon/core/exact_decimal.hpp` supplies these core-only operations:

- `parse_decimal(string_view)` returns an owned `ExactDecimal`: the unmodified
  original spelling plus its exact `boost::multiprecision::cpp_rational` value.
  Construction is private to the parser so those two representations agree.
- `parse_positive_rate(string_view)` uses the same parser and additionally requires
  an exact value greater than zero.
- `parse_nonnegative_factor(string_view)` uses the same parser and additionally
  requires an exact value greater than or equal to zero. Signed zero is zero.
- `round_effect_units(const Exact&)` converts an exact, already-scaled effect to
  signed 64-bit units using round-half-to-even, then checks the rounded range.
  Units are milliseconds for time or millionths of a point for score. Callers
  perform exact scaling before this operation; display precision is not an input.

No Qt, provider, SQL, clock or UI dependency enters this boundary. Original currency,
provider evidence and the association of a decimal with money/rates remain owned by
calling domain records. These functions do not implement expression evaluation,
modifier group combination/clamping, aggregate-state checks, refunds, persistence
or contribution review/application. Those requirements remain in their bounded
rule/modifier/accounting baselines. In particular, an exception does not itself
persist a review hold: the application caller must retain the input/provenance and
commit no partial effects or jobs on numeric failure (§3.5 and CONTRACT-002).

## Decimal grammar, preservation and local limits

The following grammar, bounds and exception classes are explicit local engineering
choices under the specification's parser/resource implementation work. They are
not new provider capability promises or previously agreed numerical maxima.

Accept only a complete ASCII JSON-number spelling inside the supplied string:
`-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?`.
Do not accept whitespace, a leading plus, leading integer zeros, separators,
hexadecimal notation, fractions such as `1/2`, locale-specific forms, NaN/Infinity,
embedded NUL or an otherwise valid prefix followed by junk. No locale or binary
floating-point conversion participates. Provider formats outside this internal
boundary require their own reviewed adapter handling with original provenance;
unsupported input is not silently truncated or reinterpreted.

The returned `original()` owns every input byte, including trailing zeros, `-0`,
exponent case/sign/leading zeros and original decimal precision. `value()` is an
exact rational; numerically equal literals may retain different original strings.
Mutating/destroying the caller's buffer cannot change the returned record.

| Resource | Accepted maximum | Rejection |
| --- | --- | --- |
| Entire original string | 512 bytes | 513 bytes or more |
| Mantissa digit count | 256 digits across integer and fraction, including zeros | 257 digits or more |
| Absolute explicit exponent value | 256; leading exponent zeros are allowed | Magnitude 257 or more, even for a zero mantissa |
| Finalizer normalized numerator magnitude / denominator | Each at most 4096 bits | Either has 4097 bits or more |

A 512-byte literal `1e` followed by 510 zero digits is accepted: its mantissa has
one digit and its exponent value is zero. Exponent *digit count* has no separate
budget beyond total bytes. Parser exponent scanning must remain bounded and cannot
first overflow a built-in integer. The mantissa/exponent limits also bound decimal
power construction; no arbitrary-size input or exponent is accepted.

For the finalizer, bit count means the bits of the absolute normalized numerator
and positive normalized denominator; zero numerator has zero bits. For example,
`2^4095 / (2^4095+1)` has 4096-bit coprime components and is accepted, while
`1 / 2^4096` exceeds the denominator budget even though it would round to zero.
The caller has already constructed the `Exact` argument: this operation cannot
retroactively bound that construction or arbitrary prior arithmetic. Broader
calculation execution/resource budgets remain a separate rule-evaluation contract.

## Validation, errors and final rounding

1. Reject excessive input bytes with `std::length_error` before syntax checking.
2. Validate the complete bounded grammar; malformed input throws
   `std::invalid_argument`. For syntactically valid input, excessive mantissa digits
   or exponent magnitude throw `std::length_error` before value/domain acceptance.
3. Rates require `>0` and factors require `>=0`; domain violations throw
   `std::invalid_argument`. Validation never silently substitutes zero, clamps a
   negative input factor or treats an invalid rate as missing evidence.
4. The finalizer rejects excessive normalized component bits with `std::length_error`
   before rounding/range conversion. For accepted components, round the exact value
   to its nearest integer; an exact half chooses the even neighbor for either sign.
5. Only the rounded integer must fit `[-9223372036854775808,9223372036854775807]`.
   Otherwise throw `std::overflow_error`, returning no effect. Do not wrap, saturate
   or reject merely because the unrounded rational lies fractionally outside that
   interval while its rounded integer fits. Exception message text is not contractual.

Resource/domain failure signals cannot authorize partial state application. This
primitive's exception API is a C++ engineering choice; API error mapping and durable
review/atomic aggregate checks remain downstream work, not passing behavior here.

## Independently derived examples and executable oracles

These fractions/expected integers are manually derived from decimal place value,
unit scaling and nearest-even rounding. Expected results never call the production
parser/finalizer to calculate another expected result. Tests use Boost's exact
integer/rational constructors only to represent literal fractions and large powers.

| Input / operation | Required result |
| --- | --- |
| `1.2300` | Original unchanged; exact `123/100` |
| `1.2300E-02` | Original unchanged; exact `123/10000` |
| `0.10000000000000000001` | Exact `10000000000000000001/100000000000000000000` |
| `-0.000e+03` | Original unchanged; numerical zero |
| Rate `0`, `-0.00` or `-1` | Invalid argument; no accepted rate |
| Factor `-0.00`, `0.5` | Zero and `1/2` respectively; negative nonzero factor rejected |
| `0.5,1.5,2.5,3.5` effect units | `0,2,2,4` |
| `-0.5,-1.5,-2.5,-3.5` effect units | `0,-2,-2,-4` |
| `0.499,0.501,-0.499,-0.501` | `0,1,0,-1` |
| `9223372036854775807.49` effect units | `INT64_MAX` |
| `9223372036854775807.5` effect units | Overflow: upper tie chooses even `9223372036854775808` |
| `-9223372036854775808.5` effect units | `INT64_MIN`: lower endpoint is even |
| `-9223372036854775808.5001` effect units | Overflow |
| `0.0005,0.0015` seconds, scaled exactly by 1000 | `0,2` milliseconds |
| `0.0000005,0.0000015` points, scaled exactly by 1000000 | `0,2` micro-points |
| Exact `(1/3)*3`, finalized once | `1`; rounding the intermediate third would be wrong |
| 256 nines | Exact `10^256-1`; 257 mantissa digits rejected |
| `1e256`, `1e-256` | Exact `10^256`, `1/10^256`; magnitude257 rejected |
| `0e257` | Resource rejection, not silent zero |
| `2^4095/(2^4095+1)` | Accepted 4096-bit components; rounded result1 |
| `2^4096` or `1/2^4096` | Component resource rejection before conversion |

Primary executable home: `tests/acceptance/exact_decimal_tests.cpp`, currently
32 GoogleTest cases. The suite covers lexical/owned-value preservation, limits and
one-beyond limits, rate/factor domains, both signs around half ties, signed endpoints,
exact scaling/no intermediate rounding and normalized-component limits. Literal
expected values and resource-bound powers are independent of the implementation.
No service atomicity, aggregate-state limit, API, live provider or platform behavior
is claimed by these primitive tests.

## Baseline harness, CI and revision policy

The following harness/CI paragraphs record historical baseline authoring. Current
CORE-001 removes the historical intentional-red label, adds the reviewed regression
and runs33 real cases automatically; [CI evidence](../evidence/core-001-ci.md)
records actual version/pass results. They do not change the revision1 behavior.

The explicit `exact-numeric-debug` preset enables
`SUBATHON_BUILD_EXACT_NUMERIC_TESTS=ON`, builds `subathon_exact_numeric` and its
GoogleTest executable, and discovers cases with labels
`exact-numeric;acceptance;intentional-red`. Default scaffold/credit presets do not
enable this lane. Configure discovers installed Boost via `find_package(Boost CONFIG
REQUIRED)` / `Boost::headers`; no implicit download or package installation occurs.
The locally observed Boost is1.92.0, GoogleTest1.18.0, GCC16.2.1; this records
compilation only until an approved red run is performed.

This is a new intentional-red baseline, not stable passing product coverage. The
existing green credit/scaffold workflows continue unchanged; the red suite is
explicitly separate and must not use failure inversion, skipped tests or
`continue-on-error`. CORE-001 must add the implemented numeric suite to automatic
PR/main CI and observe the workflow before claiming implementation complete.
Boost1.83/Ubuntu and Windows/release compatibility remain unverified here.

Revision1 changes no existing public time-credit interface or persisted schema.
The new numeric API has no implemented callers. Independent review must approve
this exact contract/test revision before the intended red run; CORE-001 waits for
that reviewed/red baseline and preserves its assertions. Revised requirements or
real test defects need written specification reasoning and separate review.
