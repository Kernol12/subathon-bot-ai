# Milestone progression C++ boundary — proposal v1

MILESTONETEST-001 concrete API document stage. This refines the independently
approved [logical model](milestone-progression-v1.md), Git blob
`0115a5cc3cdc4fb1ebb8c9739716a27cb3379933`, without changing its MP01–22 examples.
Authority remains specification v0.45 §8/MILE-01/NUM-04,
[MILESTONE-001](../tasks/EPIC-005/MILESTONE-001.md),
[record boundaries](event-state-boundaries-v1.md) and
[exact decimal revision 1](exact-decimal-v1.md).

This proposes exact declarations, errors and budgets. It creates no header/source,
stub, schema, fixture, test or implemented feature. Different actual API review
precedes those stages; reviewed tests and an intended behavioral red precede
production MILESTONE-001.

## 1. One raw-input operation

Future header: `core/include/subathon/core/milestone_progression.hpp`, namespace
`subathon::core`, C++20, standard library plus the existing selected Boost `Exact`.
There is no Qt, SQL, network/provider, clock, authorization or UI dependency.

```cpp
[[nodiscard]] MilestoneProgressionResult advance_milestone_progression(
    const MilestoneProgressionInput &input,
    const MilestoneProgressionLimits &limits = {});
```

This is the ONLY feature entrypoint. It validates raw owned input, derives the full
new completion suffix and returns its complete owned prefix/projection together.
An ordinary preview uses that same proposal without committing it; no second
factory or validating projection query is needed. Literal valid input can reach
this operation's future stub directly, without a prior feature call or a test-only
production constructor. No caller-provided output struct is a validation certificate.

The input is borrowed synchronously and remains unchanged on success/failure.
The caller keeps it alive and unmodified for that call, including all referenced
vector/string storage.
No callback, clock, mutable global/cache or external lookup supplies data during
the call. Results own every retained field independent of caller buffers, later
calls and input lifetime. This operation grants no permission or durable commit.

## 2. Exact aggregate types and field order

All structs below are ordinary owned aggregates. Fields/defaults/order are exact;
unspecified string/vector members default to empty. Default raw values are not an
accepted track until the feature validates them. References are nonempty opaque
bytes bounded below, including non-ASCII/NUL bytes; no Unicode/case normalization
or C-string truncation is allowed. IDs are compared by exact bytes, not display text.

```cpp
struct MilestoneTarget {
    std::string milestone_ref;
    std::int64_t target_micro_points{0};
};
struct MilestoneCompletion {
    std::string milestone_ref;
    std::string definition_revision_ref;
    std::int64_t target_micro_points{0};
    std::int64_t score_micro_points{0};
    std::string completion_context_ref;
};
struct MilestoneProgressionInput {
    std::string track_ref;
    std::string definition_revision_ref;
    std::string progression_context_ref;
    std::int64_t score_micro_points{0};
    std::vector<MilestoneTarget> targets;
    std::vector<MilestoneCompletion> completed_prefix;
};
struct MilestoneProgressionLimits {
    std::size_t reference_bytes{256};
    std::size_t targets{4096};
    std::size_t completed_prefix{4096};
    std::size_t items{16384};
    std::size_t owned_bytes{8388608};
};
enum class MilestoneTrackState {
    empty, before_first, between_targets, all_completed
};
struct MilestoneDeficit {
    MilestoneTarget current_target;
    std::uint64_t deficit_micro_points{0};
};
struct MilestoneInterval {
    std::uint64_t earned_micro_points{0};
    std::uint64_t span_micro_points{0};
    Exact fraction{0};
};
struct MilestoneProjection {
    MilestoneTrackState state{MilestoneTrackState::empty};
    std::vector<MilestoneDeficit> completed_deficits;
    std::optional<MilestoneTarget> previous;
    std::optional<MilestoneTarget> next;
    std::uint64_t track_deficit_micro_points{0};
    std::optional<std::uint64_t> remaining_micro_points;
    std::optional<MilestoneInterval> interval;
};
struct MilestoneProgressionData {
    std::string track_ref;
    std::string definition_revision_ref;
    std::string progression_context_ref;
    std::int64_t score_micro_points{0};
    std::vector<MilestoneCompletion> completed_prefix;
    std::uint64_t newly_completed_begin{0};
    std::uint64_t newly_completed_count{0};
    MilestoneProjection projection;
};
```

`MilestoneProgressionResult final` owns exactly one `MilestoneProgressionData` and
exposes `const MilestoneProgressionData &data() const noexcept`. Copy/move
constructors and assignments are public and defaulted. There is no default or
public data-taking constructor, mutable accessor or implicit conversion. Its
data-taking constructor is private to `advance_milestone_progression`.

Data aggregates are inspectable/copyable and deliberately not accepted as trusted
input certificates. A `data()` borrow is valid while that owner remains alive and
has not been assigned to or moved from. Its vector/string/Exact sub-borrows follow
the same lifetime. No result borrows an input object. Callers synchronize assignment/
destruction against readers; there is no hidden shared mutable state.

## 3. Returned invariants and arithmetic

Let n be target count and k the supplied completed-prefix length. The operation
finds k′ by starting at k and consuming every consecutive target `<= score`, stopping
at the first higher one. The output prefix has k′ records. Its first k records
remain byte/value identical, including their captured definition/target/score/
context. Each appended record owns its current milestone ID, current definition
revision, current target, the SAME supplied actual post-effect score, and incoming
progression context as its completion context. There are no synthesized timestamps,
threshold score samples, target sorts or action-delivery fields.

`newly_completed_begin = k`, `newly_completed_count = k′−k`; their sum equals output
prefix size. A zero-length suffix may begin at the end. The suffix is identified
inside the result's owned prefix, not stored in a second vector or borrowed from
the input. Prior completion/context evidence cannot authorize an omitted/reset
prefix, action replay, state write or provider fact.

Projection describes the POST-APPEND prefix, exactly logical §4:

- completed_deficits has k′ entries in prefix order. Each owns the corresponding
  CURRENT MilestoneTarget and `max(current_target−score, 0)`. Captured completion
  targets are never substituted for current configured targets.
- previous is current target k′−1 when k′>0; next is current target k′ when k′<n.
  track_deficit_micro_points is the last completed deficit or 0, not a sum.
- state is empty for n=0; before_first for n>0/k′=0; between_targets for 0<k′<n;
  all_completed for n>0/k′=n. Empty is not a completed-track celebration.
- remaining is `next.target−score` when next exists, otherwise absent. It is
  strictly positive after progression; exact-threshold completion has already
  moved next forward. No absent next is manufactured as zero.
- interval exists ONLY when previous and next both exist. Its span is `next−previous`,
  earned is `max(score−previous, 0)` and fraction is exact reduced earned/span.
  `span>0`, `0<=earned<span`, `0<=fraction<1`. Deficit/boundary progress is exactly 0.
  Before the first completion, interval is absent, not an invented zero-origin
  fraction. All-completed has no interval/remaining, even when deficit is positive.

Every score/target/captured numeric field accepts the complete signed64 domain,
including zero and INT64_MIN. A scalar's prior checked construction and correct
micro-point unit belong to the caller: the function cannot recover a value already
wrapped/truncated before it was passed. It accepts an already authoritative post-
effect score, not a delta, requested score, decimal parser input or rounded display.
No `round_effect_units` call, score aggregation or second rounding is performed.

Derived nonnegative differences span 0..UINT64_MAX. Widen BEFORE subtraction or use
checked unsigned arithmetic; never negate INT64_MIN in signed64. A selected Boost
signed integer temporary needs at most 65 bits; numerator/denominator and all stored
distance outputs need at most 64. A valid input never fails because a projection
does not fit signed64. To construct the fraction, normalize bounded earned/span
with an unsigned gcd; zero normalizes to 0/1. Its exact output size can be measured
from those bounded integers before materializing the returned Exact. There is no
unbounded caller-constructed Exact input or arbitrary-size arithmetic chain.

Several actual ordered score observations may be composed using each returned
prefix as the next raw prefix, with all proposals committed together by the owning
application. This API neither chooses an input's observation sequence nor proves
atomic commit. Silent/manual/edit/reset/paused/post-finish policy, action choices,
queue/presentation and full archival history remain their owning tasks.

## 4. Complete logical bytes/items and allocation boundary

Every limit must be positive and at most its declaration default, in declaration
order. Lower ceilings are allowed; zero or a larger value is invalid_options.
Empty target/prefix vectors remain valid when their structural rules allow them.
The completed_prefix ceiling applies independently to input and COMPLETE output,
so advancement exceeding a smaller selected ceiling is a whole-proposal resource
failure. No fitting suffix or partial result is returned.

Let `s(x)` be string byte length and `E(q)` the lengths of q's canonical signed
numerator decimal spelling plus positive denominator decimal spelling. No plus/
leading zero is emitted; zero is 0/1. Vector length contributes collection items,
not byte overhead. Each explicit signed/unsigned64 field contributes 8 bytes, state
tag 1, absent optional 0, present optional its actual value; structs/wrappers sum
their fields, without C++ padding/pointers/capacity/control blocks.

The COMPLETE field walks are:

~~~text
T(target) = s(milestone_ref) + 8
C(completion) = s(milestone_ref) + s(definition_revision_ref)
                + 8 + 8 + s(completion_context_ref)
D(deficit) = T(current_target) + 8
I(interval) = 8 + 8 + E(fraction)
P(projection) = 1 + sum D(completed_deficits)
                + optional T(previous) + optional T(next)
                + 8 + optional 8(remaining) + optional I(interval)
B(input) = s(track_ref) + s(definition_revision_ref) + s(progression_context_ref)
           + 8 + sum T(targets) + sum C(completed_prefix)
B(result) = s(track_ref) + s(definition_revision_ref) + s(progression_context_ref)
            + 8 + sum C(completed_prefix) + 8 + 8 + P(projection)
Items(input) = target count + supplied completed-prefix count
Items(result) = output completed-prefix count + completed-deficit count
~~~

Measure input/result independently against selected owned_bytes/items. Every real
copy counts: current IDs/targets in deficit entries and optional previous/next,
and repeated revision/context references in newly appended records. The suffix
begin/count add 16 bytes but no second record allocation. No full copied target
list or retained raw-input copy is hidden inside the result.

Counts and string lengths are inspected before dynamic input copying/hash indexing.
Scratch indexes use at most 4096 borrowed string_views/indices into the input, with
no owning string keys or speculative completed-history copies. Fixed-size numeric
temporaries stay within the bit bounds above; bounded decimal digit counting for
Exact byte projection needs no allocated canonical string. Project the COMPLETE
result's record/deficit counts and bytes before reserving/growing/copying ANY output
vector/string or constructing its returned Exact. Do not append/copy first and
discover that the full result exceeds its selected budget afterward.

Already-constructed caller strings/vectors and their earlier arithmetic/allocation
cannot be retroactively bounded. Explicit caller copies of returned results/data
also allocate outside a feature call. These semantic budgets are not a global
allocator/CPU quota or a promise that allocation cannot fail.

## 5. Exact failures and validation order

```cpp
enum class MilestoneProgressionError {
    invalid_options, resource_limit, invalid_reference, duplicate_milestone,
    nonascending_targets, invalid_completed_prefix
};
class MilestoneProgressionFailure final : public std::invalid_argument {
  public:
    explicit MilestoneProgressionFailure(MilestoneProgressionError code);
    [[nodiscard]] MilestoneProgressionError code() const noexcept;
};
```

`what()` is exactly `Invalid milestone progression input`; `code()` owns the supplied
enum. No raw reference, private input, external error or traceback is included.
Constructing an exception manually proves no feature outcome. Feature failures use
only the declared categories; allocation failure propagates `std::bad_alloc`.
Unexpected programming errors are not translated into a fake valid/resource result.
No expected valid numeric extreme requires `overflow_error` or saturation.

The feature's normative first-failure order is:

1. Validate all limits in declared field order: invalid_options.
2. Preflight input target count, prefix count and combined items, then every reference
   byte ceiling in declared depth-first input order, then total input bytes, with
   checked size arithmetic: resource_limit. An excessive later field precedes an
   earlier empty-field failure. No output/input copy or hash key allocation yet.
3. Check every required reference is nonempty, in the same field order:
   invalid_reference. This includes all captured prefix references. Nothing parses
   a context/provider/configuration reference as authority or retrieves its target.
4. Reject duplicate current milestone IDs in target order: duplicate_milestone.
   Do not sort/merge targets. Repeated context references across different completed
   milestones are allowed, including several rows created by one score observation.
5. Compare adjacent current target values for strict increase: nonascending_targets.
   Use comparisons, not potentially overflowing differences.
6. Reject prefix length greater than target count or an identity not equal to the
   corresponding first-k target identity: invalid_completed_prefix. Do not require
   captured revision/target to equal current definitions, historical target order,
   or captured score to meet target. Those would reject legitimate corrections/
   manual completion. Duplicate/out-of-order prefix identities fail here.
7. Derive k′ and bounded projection arithmetic without output ownership growth;
   preflight output prefix ceiling, complete result items and bytes: resource_limit.
   Then materialize the complete result. No partial prefix or earlier returned
   result/input is mutated if allocation or validation fails.

All signed64 bit patterns are representable typed numeric inputs. Conversion from
untrusted/out-of-range numbers to these fields must already have been checked by
their owning boundary; a cast cannot serve as validation. None of these failures
persists a NUM-04 hold, clears another gate or changes authoritative state.

## 6. Literal API boundary examples

Use valid nonempty track/current/context references and literal completion records;
no feature call is needed to arrange input. Logical MP01–22 remain the independent
semantic expectations. These extra byte/shape examples pin the concrete field walk.
All numeric units here are MICRO-POINTS, and unmentioned limits retain defaults.

| Raw input | Required result |
| --- | --- |
| track=`t`, current revision=`v`, context=`c`, score 0, empty vectors | Input bytes 11/items 0. Result prefix empty, new begin/count 0, projection empty: result bytes 36/items 0 |
| Same with owned_bytes=36 | Accepted; no next/remaining/interval and track deficit 0 |
| Same with owned_bytes=35 | resource_limit before complete output copy |
| Same with limits.targets=0, track_ref empty | invalid_options before invalid_reference |
| Empty track_ref plus a 257-byte context | resource_limit before invalid_reference |
| Targets A=0/B=0 and prefix B | nonascending_targets before invalid_completed_prefix |
| Targets A=0/A=1 and prefix B | duplicate_milestone before target/prefix semantics |
| Targets A=0/B=1, empty prefix, score 1, completed_prefix limit 1 | resource_limit for complete output count; neither A nor B escapes as a partial result |
| Target A=0, actual input score −1, valid nonempty prefix row A with captured revision=`old`, target 9, score −3 | Historical row retained; all_completed, current previous 0, deficit 1, no next/interval; no captured-score sufficiency test |
| track=`t`, current revision=`v`, context=`c`, score 1, targets A=0/B=3; prefix A with captured revision=`old`, target/score 0 and completion context=`z` | Input bytes 50/items 3; result bytes 118/items 2, unchanged prefix, new begin 1/count 0, remaining 2 and interval earned 1/span 3/fraction 1/3 |
| Same two-target example, owned_bytes=117 | resource_limit before complete output copy |

The empty-result 36 bytes are context 3 + score 8 + suffix scalars 16 + projection
state 1 + track deficit 8. These are manual field arithmetic, not implementation-
generated expected output. Missing optionals/vectors add no hidden fields.
The two-target example has input `11 + 18 + 21 = 50`; its result is context/score
11 + prefix 21 + suffix scalars 16 + projection 70 = 118. The projection's 70 is
state 1 + deficit 17 + previous 9 + next 9 + track deficit 8 + remaining 8 + interval 18;
interval 18 includes its two 8-byte distances and the Exact value 1/3's two
canonical digits.

## 7. Future immediate stub and proof partition

The minimal future header/source may define the above types, const accessor and
exception constructors only. There is ONE immediate feature stub with the full
signature; its first statement must be `throw MilestoneProgressionNotImplemented{};`.
No option/reference/prefix validation, successful value, call to a parser/helper,
Boost arithmetic, action/state access or canned projection precedes that throw.

`MilestoneProgressionNotImplemented final : public std::logic_error` has a
no-argument constructor with exact text:

```text
Milestone progression is intentionally unimplemented; review tests first
```

This marker is distinct from existing numeric/application/rate markers and from
MilestoneProgressionFailure. Inert aggregate/accessor/exception construction is not
the feature implementation. Configure/build/discovery may compile/link types and
verify selected C++20/Boost/GoogleTest, but must NOT execute this entrypoint or its
marker. No build-time generated fixture/expected result or discovery data provider
may call it. A selected green foundation may use existing std/Boost primitives with
independent literal integer/rational facts; no new feature call supplies setup.

After actual API/minimal-source/harness review, independent tests construct raw
valid input and literal expected histories/projections/fractions, review them, then
observe intended failure from this named immediate marker. Missing dependencies,
compile/setup failures, skipped cases, catching the marker as success or reversing
exits are not red. Only the later production task implements this same reviewed
boundary and retains its assertions, with separate source audit, relevant regression
acceptance and ordinary selected CI. Manual/edit/reset/queue/action/atomic-state
integration, UI and platform gates remain required separate work.
