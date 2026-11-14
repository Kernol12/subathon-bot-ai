# Rate snapshot C++ boundary — proposal v1

RATESTEST-001 concrete API document stage. This refines the independently approved
[logical model](rate-snapshots-v1.md), Git blob
`76d2d9b3e542c56b7475e99defd717d9902885a5`, without changing its requirements or
RS01–17 examples. Authority remains specification v0.45 §§3.5, 9, 15, NUM-04/FX-06,
[RATES-001](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-017/RATES-001.md),
[exact decimal revision 1](exact-decimal-v1.md) and
[record ownership](event-state-boundaries-v1.md).

This is a proposed public boundary, not a header, stub, test baseline or feature.
Different actual API review is required before minimal interfaces/build harnesses;
reviewed independent tests and an intended behavioral red are required before
production implementation. RATES-001 remains planned.

## 1. Public operations and ownership

The future header is `core/include/subathon/core/rate_snapshot.hpp`, namespace
`subathon::core`, C++20, using existing `Exact`, `ExactDecimal` and `ReceiptMoney`.
Only selected Boost and the standard library are required. There is no Qt,
network, provider, SQL, clock, filesystem or UI access.

The four feature entrypoints, including argument order and defaults, are:

```cpp
[[nodiscard]] RateMoney make_rate_money(
    const ReceiptMoney &input, const RateSnapshotLimits &limits = {});
[[nodiscard]] FetchedRateObservation make_fetched_rate_observation(
    const FetchedRateObservationInput &input,
    const RateSnapshotLimits &limits = {});
[[nodiscard]] EffectiveRateSnapshot make_effective_rate_snapshot(
    const EffectiveRateSnapshotInput &input,
    const RateSnapshotLimits &limits = {});
[[nodiscard]] RatePairResult quote_effective_rate(
    const EffectiveRateSnapshotInput &input, std::string_view from_currency,
    std::string_view to_currency, const RateSnapshotLimits &limits = {});
```

Every input is borrowed only for the synchronous call. It remains unchanged on
success or failure. Each returned value owns its complete data, independent of
input lifetime, mutation and later calls. There are no callbacks, external reads,
hidden mutable caches or borrowed strings in a result.

Each result class is `final`, has public defaulted copy/move constructors and
assignments, and exposes only `const DataType &data() const noexcept`. It has no
default constructor or public data-taking constructor; construction is private to
the corresponding entrypoint. The exact pairs are `RateMoney/RateMoneyData`,
`FetchedRateObservation/FetchedRateObservationData`,
`EffectiveRateSnapshot/EffectiveRateSnapshotData`, and
`RatePairResult/RatePairResultData`. Each class owns exactly one `DataType` member.
No mutating accessor or conversion to a mutable reference is provided.

The data structs below are ordinary owned aggregates so tests/callers can inspect
or copy them. Constructing such a struct proves nothing. No public feature accepts
an output data struct as a validated certificate. The raw-input query independently
validates the complete candidate under its supplied limits on EVERY call, including
identity/missing queries. A literal valid query can therefore reach its own future
stub without first calling a factory. This is a normal input boundary, not a
test-only constructor or bypass.

A borrowed `data()` reference remains valid while its owning result is alive and
has not been assigned to or moved from. Destroying or changing another result/input
cannot invalidate it. Caller copies of data/results allocate outside a feature
call's budget. Read-only use of an unchanged object needs no hidden shared state;
callers must synchronize assignment/destruction against readers as usual in C++.

## 2. Owned input and data types

All `string` names below mean `std::string`; all `vector<T>`/`optional<T>` mean
their standard-library types. Fields appear in the declared order. Strings contain
opaque bytes, including non-ASCII and embedded NUL where the logical model permits
opaque data. No encoding normalization, locale comparison or C-string truncation
is permitted. Decimal/integer lexical fields have their stricter ASCII grammar.

```cpp
struct RateTimeEvidence {
    std::optional<std::string> original_text;
    std::optional<std::int64_t> utc_ms;
};
struct RateObservationContext {
    std::string observation_ref;
    std::string provider_ref;
    std::string configuration_ref;
    std::string request_ref;
    std::string source_evidence_ref;
    std::optional<std::string> provider_snapshot_ref;
    std::optional<std::int64_t> fetched_at_utc_ms;
    RateTimeEvidence source_rate_time;
    RateTimeEvidence publication_time;
};
struct RateQuoteInput {
    std::string from_currency;
    std::string to_currency;
    std::string original_decimal;
};
struct FetchedRateObservationInput {
    RateObservationContext context;
    std::vector<RateQuoteInput> quotes;
};
struct RateQuote {
    std::string from_currency;
    std::string to_currency;
    ExactDecimal rate;
};
struct FetchedRateObservationData {
    RateObservationContext context;
    std::vector<RateQuote> quotes;
};
struct RateMoneyData {
    ReceiptMoney original;
    Exact value;
};
```

`make_rate_money` preserves BOTH original `ReceiptMoney` strings and adds the
`parse_decimal` exact value; negative amounts and signed zero are data. Every quote
uses the existing parser's complete grammar/resource checks and requires `>0`.
`ExactDecimal` ties its original spelling to that exact parsed value. An observation
retains the supplied quote order, including inconsistent cycles and positive self
quotes; it rejects duplicate directed keys. Its stable ordinal is index+1, not an
extra stored field. No source date is parsed or timestamp inferred here.

```cpp
enum class RateOverrideKind { temporary, pinned };
struct ManualRateOverrideInput {
    std::string override_ref;
    RateOverrideKind kind{};
    std::string from_currency;
    std::string to_currency;
    std::string original_decimal;
    std::string decision_ref;
    std::optional<std::string> actor_ref;
    std::string reason;
    std::optional<std::int64_t> recorded_at_utc_ms;
};
struct ManualRateOverrideData {
    std::string override_ref;
    RateOverrideKind kind{};
    std::string from_currency;
    std::string to_currency;
    ExactDecimal rate;
    std::string decision_ref;
    std::optional<std::string> actor_ref;
    std::string reason;
    std::optional<std::int64_t> recorded_at_utc_ms;
};
struct FetchedRateConstraint {
    std::string observation_ref;
    std::uint64_t quote_ordinal{};
};
struct ManualRateConstraint {
    std::string override_ref;
};
using RateConstraint = std::variant<FetchedRateConstraint, ManualRateConstraint>;
struct RatePotentialInput {
    std::string currency;
    std::string numerator;
    std::string denominator;
};
struct RateComponentInput {
    std::vector<RatePotentialInput> members;
};
struct RatePotential {
    std::string currency;
    Exact weight;
};
struct RateComponent {
    std::vector<RatePotential> members;
};
struct EffectiveRateSnapshotInput {
    std::string snapshot_ref;
    std::optional<std::string> parent_snapshot_ref;
    std::optional<std::int64_t> constructed_at_utc_ms;
    std::vector<FetchedRateObservationInput> sources;
    std::vector<ManualRateOverrideInput> overrides;
    std::vector<RateConstraint> constraints;
    std::vector<RateComponentInput> components;
};
struct EffectiveRateSnapshotData {
    std::string snapshot_ref;
    std::optional<std::string> parent_snapshot_ref;
    std::optional<std::int64_t> constructed_at_utc_ms;
    std::vector<FetchedRateObservation> sources;
    std::vector<ManualRateOverrideData> overrides;
    std::vector<RateConstraint> constraints;
    std::vector<RateComponent> components;
};
```

Raw potential components are canonical positive integers: complete ASCII
`[1-9][0-9]*`, at most 1234 bytes each, with no sign, leading zero, whitespace,
separator, fraction or exponent. Each parsed integer must have at most 4096 bits
(or the selected smaller potential limit), and numerator/denominator must be
coprime. `6/5` is supplied as two strings `"6"`, `"5"`, not one decimal/fraction
field. Reject unreduced `12/10`; do not repair it. This lexical boundary bounds
integer construction before rational construction: 1234 decimal digits can need
4100 bits, so the byte guard alone is insufficient. Anchor weight is exactly
canonical `"1"/"1"`. Outputs contain the corresponding normalized `Exact` only;
there is no second stored potential spelling because the canonical pair is unique.

This choice does not require inspecting/copying an unbounded caller-created
`Exact` through Boost backend internals. Input strings/vectors and any prior caller
arithmetic/serialization are already constructed outside this API. The API bounds
its own parsing, copies and new arithmetic; it cannot retroactively bound caller
construction. Selected Boost represents integer/rational values; no additional
numeric library or floating-point conversion is introduced.

Required/optional identity rules, distinct override endpoints, selected-source
inclusion, exactly-once override constraints, positive canonical components and
edge/connectivity invariants are exactly logical §§2–3. `RateConstraint` must hold
one of its two alternatives; `valueless_by_exception()` is invalid. Ordinal 0 and
out-of-range ordinals do not resolve. Enum values outside the two declared values
are rejected before narrowing. Reference presence never authenticates permission.
There is no parent lookup, pin-removal authorization, successful-update decision or
fetched/manual conflict solver in any entrypoint.

Source/override/constraint input vectors may be in any order. Accepted outputs sort
sources by `context.observation_ref`, overrides by `override_ref`, and constraints
by variant (fetched first), then reference, then numeric fetched ordinal. All string
ordering compares unsigned bytes lexicographically, with shorter prefix first.
Quote order stays unchanged. Component/member input order must ALREADY be canonical
as logical §3 prescribes: strictly increasing anchors/members, smallest member
first with weight 1, no currency shared across components. Reject noncanonical
components instead of reordering/rebasing/solving them. Output retains that order.
Equal ordering keys are duplicates, not an instruction to choose a winner.

## 3. Pair result and exact arithmetic

```cpp
enum class RatePairKind { identity, available, missing };
struct RatePairProvenance {
    std::string snapshot_ref;
    std::string from_currency;
    std::string to_currency;
};
struct RatePairResultData {
    RatePairKind kind{};
    std::optional<Exact> rate;
    std::optional<RatePairProvenance> provenance;
};
```

After complete candidate validation, equal valid query keys yield `identity`,
rate exactly 1 and absent provenance, even when the key is unrepresented. This is
arithmetic identity, not a supported-provider/currency claim. Distinct keys in one
component yield `available`, exact positive `w(to)/w(from)` and the three owned
provenance strings. Otherwise return `missing` with both optionals absent. Missing
is never an exception for a valid candidate, a numeric zero, an approximate rate,
a fabricated source or a network fallback. No empty optional is an accepted rate.

Do not build an all-pairs matrix or multiply an unbounded path. A pair numerator
and denominator are products of two potential integer components: each unreduced
product and final normalized component is bounded by 8192 bits. Cross-cancel before
products where useful; a product of two 4096-bit positive integers can require 8192
bits. Intermediate exact equality against a fetched/manual rate is also bounded:
use checked component-bit sums before multiplication, with a fixed 16384-bit
ceiling for every temporary integer. Existing decimal bounds put its components
well below 4096 bits, so comparison of a ratio and a parsed rate fits this ceiling.
Bounded gcd/cross-cancellation and exact integer comparison need no epsilon.
Check multiply/add size arithmetic without overflow before allocating/growing.

The configurable potential ceiling cannot exceed 4096; the derived 8192 and
temporary 16384 ceilings are fixed and do not enlarge any selected input limit.
No amount conversion, finalizer, rounding, division to a finite decimal, or
crediting is performed. CORE-001's 4096-bit finalizer remains separate. The exact
inverse of 1.20 is 5/6; the current finite-decimal `HistoricalValuation` field cannot
carry it. Required [RATES-009](https://github.com/Kernol12/subathon-bot-ai/issues/553) owns that versioned
valuation/command/codec/durable bridge and its independent test/red/migration gates.
This API neither rounds it nor reports that valid available ratio as missing.

## 4. Limits and complete owned-value measurement

`RateSnapshotLimits` is an aggregate with these `std::size_t` fields/defaults:

| Field | Default and maximum |
| --- | ---: |
| currency_bytes |64|
| reference_bytes |256|
| raw_time_bytes |512|
| reason_bytes |4096|
| observations |16|
| quotes_per_observation |4096|
| total_quotes |16384|
| overrides |1024|
| constraints |4096|
| currencies |256|
| items |32768|
| owned_bytes |8388608|
| potential_component_bits |4096|

Every field must be positive and no greater than its maximum, including fields
unused by the selected operation. Callers may lower ceilings but cannot turn off
checks. Existing decimal 512-byte/256-digit/exponent-256, potential-integer 1234-byte,
pair 8192-bit and temporary 16384-bit limits are fixed, not additional options.
An empty collection is allowed subject to the logical invariants. `currencies`
counts effective component MEMBERS, not keys appearing in unselected source rows.
Observation factory quote counts obey both per-observation and total ceilings.

Use the following recursive metric on the COMPLETE raw input and independently on
the COMPLETE returned owned value. Either exceeding selected `owned_bytes/items`
is a resource failure. Raw-input counts do not pretend parsed exact values already
exist; output counts include them. Nested wrappers add no hidden duplicate fields.

| Kind | Logical bytes / recursive items |
| --- | --- |
| string | Its exact byte length /0 |
| signed/unsigned 64-bit integer |8 /0 |
| enum or variant alternative tag |1 /0 |
| optional | Absent 0, present child's bytes/items; no presence tag |
| Exact | Signed canonical numerator decimal bytes plus positive denominator decimal bytes /0 |
| ExactDecimal | `original()` string plus `value()` Exact /0 |
| vector | Sum of children bytes; its length plus all child recursive items |
| ordinary aggregate/result wrapper | Sum of actual declared fields, without C++ padding/control blocks |

Thus context counts all its declared strings/times; a quote counts two currencies
and its lexical input or parsed rate; an override also counts its kind; fetched
constraint counts tag 1 + reference + ordinal 8, manual constraint tag 1 + reference; a
component counts its member vector; money counts original strings plus its output
Exact; pair counts its kind, optional Exact and optional provenance strings. A
snapshot counts its three scalar fields plus source/override/constraint/component
vectors recursively. The result's source wrapper owns context and parsed quotes
once; a constraint charges its own reference, not its separately owned target
again. No capacities, allocator overhead, pointer sizes or prior lifetime copies
enter this semantic metric. A zero Exact has numerator spelling `0` and denominator
`1`; positive/negative numerators have no leading plus/zeros and include a minus
only when negative.

Perform checked complete projections before constructing/copying the corresponding
output collection/value, including pair disposition/provenance, rather than copy
then reject. Bounded individual decimal/integer parsing temporaries may be needed
to measure exact canonical output sizes; release each before advancing or retain
only within an already projected complete destination. Such a CORE parse temporary
is independently bounded by its fixed lexical/numeric policy and is not an extra
returned-data field or a reason to charge phantom copies to `owned_bytes`.
Index/graph scratch uses borrowed references or indices into bounded input, with
at most 65536 index/reference records; any integer scratch obeys the temporary bit
ceiling. Do not make unmeasured whole-input copies, output-sized speculative matrices, or allocate
according to an unvalidated declared ordinal/count. Logical byte budgets are not
a global allocator/CPU quota or a promise that allocation cannot fail.

## 5. Validation priority and failures

```cpp
enum class RateSnapshotError {
    invalid_options, invalid_reference, invalid_currency, invalid_decimal,
    nonpositive_rate, invalid_override, invalid_potential, duplicate_identity,
    duplicate_quote, duplicate_constraint, unresolved_basis,
    noncanonical_component, unbacked_component, contradictory_constraint,
    resource_limit
};
class RateSnapshotFailure final : public std::invalid_argument {
  public:
    explicit RateSnapshotFailure(RateSnapshotError code);
    [[nodiscard]] RateSnapshotError code() const noexcept;
};
```

`what()` is exactly `Invalid rate snapshot input`; `code()` retains the supplied
enum. No input bytes, credentials, arbitrary diagnostics or external errors are
included. Public construction of an exception authenticates nothing. Feature
functions throw this type for the following deterministic validation failures.
Allocation failures propagate `std::bad_alloc`; no result/state update occurs.
Do not convert allocation failure into a fake malformed rate or missing pair.
Exception construction itself can allocate under normal standard-library rules.

Validation order is normative. Within a phase, visit fields/vectors in their
declared/input order above; after a canonical sort, use the declared output order.
Reject the first failing check, without partial output or input mutation:

1. Check every option's range, in the limits-table order: `invalid_options`.
2. Preflight all input vector ceilings, aggregate quote/member counts, recursive
   item totals, per-field byte ceilings and total raw logical bytes, with checked
   arithmetic, BEFORE copying strings, decimal/integer construction or graph
   arithmetic: `resource_limit`. A query includes its from/to per-field currency
   limits in this phase; its two borrowed arguments are not fields of the snapshot
   and are not added to the snapshot's logical bytes. Walk vectors first (including
   recursive lengths), then strings in declared depth-first order. Required-empty
   checks wait for phase 3, so an excessive later input cannot be hidden by an early
   empty reference. Optionally present raw time text may be empty.
3. Check required/optional identity nonemptiness (`invalid_reference`), currency
   nonemptiness (`invalid_currency`), and each decimal with CORE's complete grammar,
   then mantissa/exponent resource policy. Map CORE `invalid_argument` syntax to
   `invalid_decimal`, `length_error` to `resource_limit`, and parsed rate `<=0` to
   `nonpositive_rate`; money has no positive-domain requirement. Do not classify
   by exception text. Validate override enum/distinct endpoints/nonempty reason
   (`invalid_override`). Validate potential integer grammar, positive values,
   component bit limits and gcd 1: bad grammar/domain/reduction is
   `invalid_potential`, excessive bits is `resource_limit`. Check each integer's
   bit bound before constructing the rational or doing gcd/multiplication. Query
   key nonemptiness is checked after the candidate's lexical fields in this phase.
   A valueless constraint variant is `unresolved_basis` when visited, before any
   constraint comparison or sorting; never access its nonexistent alternative.
4. Reject duplicate source IDs, duplicate override IDs (`duplicate_identity`),
   duplicate directed quote keys within each observation (`duplicate_quote`), then
   duplicate active references (`duplicate_constraint`), in that category order.
   Observation and override identities occupy distinct typed namespaces. Check
   quote duplicates by source input order; source/override duplicate selection is
   deterministic input order. No dictionary insertion overwrites evidence.
5. Check component shape/order/anchor/shared-member/positive invariants
   (`noncanonical_component`), then active reference resolution,
   ordinals, distinct endpoints and same-component membership (`unresolved_basis`).
   In canonical constraint order, verify exact edge equality
   (`contradictory_constraint`). Then check every supplied override is referenced
   exactly once and every included source has an active fetched reference
   (`unresolved_basis`). Finally check each component's undirected connectivity
   (`unbacked_component`). Empty components are noncanonical; a singleton needs no
   edge. Empty whole tables are allowed and do not establish sync success.
6. Project the complete parsed/sorted output against its byte/item limits before
   ownership growth: `resource_limit`. Factory returns that owned result. Query
   validates the same complete snapshot output projection without materializing
   a throwaway snapshot, determines identity/available/missing, then separately
   projects and owns its complete pair result. Potential/ratio/temporary bit checks
   apply before each new arithmetic operation in the preceding phases too.

The observation factory performs the applicable phases only; snapshot-only graph,
override and output ordering checks do not apply. The money factory validates its
two fields and exact value only. Arithmetic/input errors never persist a review
hold, mutate a cache pointer, certify upstream evidence, authorize a pin, or apply
part of an award. Durability/global uniqueness/reference resolution, temporal pin
preservation and permission remain caller/adjacent-operation gates.

Typed signed timestamps admit every `int64_t` value; time text is opaque. There is
no date parser or additional invalid-time category pretending to validate source
meaning. Strings exceeding their byte bounds are resource failures. Decimal/weight
domains, rather than time or optional absence, determine numeric validity.

## 6. Literal API examples

These are manual expected values refining RS01–17, not feature executions or source
fixtures. Tests must spell all required context fields and inputs independently.

Use snapshot `S1`, absent parent/construction time, and observation context
`observation_ref=O1`, `provider_ref=provider-A`, `configuration_ref=config-1`,
`request_ref=request-1`, `source_evidence_ref=evidence-1`. All its optional fields
are absent. Its ordered quotes are USD→EUR `1.2000`, then USD→GBP `0.6000`.
Select fetched constraints `(O1,1)` and `(O1,2)`, no overrides, and one component:

| member in order | input numerator | input denominator | output weight |
| --- | --- | --- | --- |
| EUR | `1` | `1` |1|
| GBP | `1` | `2` |1/2|
| USD | `5` | `6` |5/6|

`make_effective_rate_snapshot` owns those exact quote spellings and unknown fields,
with canonical source/constraint/member order. Query EUR→USD returns `available`,
rate 5/6, provenance `(S1,EUR,USD)`; GBP→USD returns 5/3. USD→CAD is `missing` with
both optionals absent. CAD→CAD is `identity`, rate 1, no provenance. None of these
requires or proves provider-CAD coverage. Changing the caller's `O1` quote bytes,
time optionals or potential strings after factory return changes no returned data.

| Direct input | Required outcome |
| --- | --- |
| Money `original_decimal=1.234500`, `currency=USD` | Original bytes unchanged, value 2469/2000 |
| Money `original_decimal=1`, `currency=U`, `owned_bytes=4` | Accepted; raw bytes 2, complete output bytes 4 |
| Same money, `owned_bytes=3` | `resource_limit` before complete output copy |
| Same valid money, `observations=0` | `invalid_options` even though money uses no observations |
| O1 source time text present `2026-11-04`, instant absent, fetch instant 0 | Exact raw date and known epoch fetch retained; no publication/rate instant inferred |
| Above component USD numerator `10`, denominator `12` | `invalid_potential`, not silently reduced |
| Above component USD numerator `5`, denominator `7` | `contradictory_constraint`; O1 values remain unchanged |
| Above source repeated fetched `(O1,1)` | `duplicate_constraint` |
| Above source selected `(O1,0)` instead of `(O1,1)` | `unresolved_basis` |
| Above valid snapshot with query empty from key | `invalid_currency`, not missing |
| Above contradictory snapshot with query CAD→CAD | `contradictory_constraint`, no identity shortcut |
| Empty snapshot `snapshot_ref=S`, optionals absent, collections empty; query u→u, `owned_bytes=2` | Snapshot bytes 1 fits, pair bytes 3 does not: `resource_limit` |
| Valid singleton `X:1/1` with no edges | Accepted, identity only |
| Two-member component `X:1/1,Y:2/1` with no edges | `unbacked_component` |
| Valid integer strings for numerator/denominator `2^4096` (written as its complete decimal expansion) | `resource_limit` before rational construction, despite at most 1234 digits |

All omitted limits retain defaults. No expected fraction is calculated by calling
one of these four functions or by rendering an actual result into an oracle.

## 7. Minimal stub, harness and remaining gates

The future minimal source stage may define only these types/getters/exception
constructors and four immediate feature stubs, each with its complete signature
and an unconditional first statement `throw RateSnapshotNotImplemented{};`.
`RateSnapshotNotImplemented final : public std::logic_error` has a no-argument
constructor whose exact message is:

```text
Rate snapshots are intentionally unimplemented; review tests first
```

No feature stub validates arguments/options, constructs a successful result, calls
another factory/query/parser, or performs arithmetic before throwing. This marker
is distinct from existing exact/application feature markers. Inline data access
and ordinary DTO copying are inert interfaces, not model implementation.

Build/configure/discovery must compile/link declarations and verify selected C++20/
Boost/GoogleTest dependencies without executing a feature entrypoint. A green
foundation may exercise selected Boost directly with independent literal integer/
rational facts, never this new stub as setup. Header existence, failed compilation,
dependency absence, skipped tests, catching the marker as success or reversing
test exit status cannot stand in for intended behavioral red.

After actual API/source/harness review, baseline tests must construct their inputs
and expected exact fractions from independent literals (RS01–17 and boundary/error
cases), not generate expected snapshots using the feature under test. Factories
and the raw-input query can each reach their own marker directly with valid setup.
The first observed baseline must fail at required behavior with that named marker;
production implementation then preserves those reviewed assertions. Actual
numerical resources/output ownership/error order/immutability, regression checks,
different code review and selected automatic CI remain future gates.

The model certifies internal consistency of supplied data only. All three providers,
their actual payload/plan/metadata capabilities, cache/scheduler/auth/HTTP, authorized
pin editing, history/reporting UI, exact valuation bridge and Linux/Windows release
verification remain required separate work. This document claims none of them.
