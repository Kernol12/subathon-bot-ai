# Timer editorial commands — concrete API proposal v1

CORETEST-008 API document stage, release `ac057b89`. Authority: approved
[logical model](timer-editorial-commands-v1.md), blob
`f4a4c3ceea45a58bc8116f73169d7bac081e9919`, its
[full independent review](../evidence/coretest-008-contract-review.md), blob
`99298e93800c41b3109379a76083759b24808f14`, specification §4/§18/TIMER-01 and
[owner decisions](../evidence/timer-editorial-owner-decisions.md). Existing
[CORE-002](timer-lifecycle-v1.md), [CORE-005](time-debt-v1.md) and the
[writer/execution boundary](event-state-boundaries-v1.md) are unchanged.
This is an API document, not source, a validated factory, tests or executable proof.
Actual different API review precedes any minimal header/source stage.

## 1. Concrete public boundary

Future header `core/include/subathon/core/timer_editorial_commands.hpp`, namespace
`subathon::core`, includes the existing std-only `timer_lifecycle.hpp` and
`time_debt.hpp` plus `<array>`, `<cstdint>`, `<exception>`, `<optional>`,
`<stdexcept>`, `<string>`, `<utility>`, `<variant>` and `<vector>` as needed.
One raw feature entrypoint is proposed. New enums use `std::uint8_t` and consecutive
values starting at zero in listed order; existing Core enums retain their ABI.
Declarations below freeze field order/defaults; they are not an actual header.

```cpp
namespace subathon::core {
enum class TimerEditorialAccessState : std::uint8_t { unknown, denied, allowed };
enum class TimerCapChoice : std::uint8_t { keep, reduce, abort };

struct TimerEditorialValues {
    TimerState timer;
    std::int64_t time_debt_ms = 0;
    std::optional<std::int64_t> maximum_remaining_ms;
    TimerConfig config;
};
struct TimerEditorialSnapshot {
    std::string revision;
    TimerEditorialValues values;
};
struct TimerEditorialAccess {
    TimerEditorialAccessState state = TimerEditorialAccessState::unknown;
    std::optional<std::string> permission_revision;
};
struct TimerSignedDelta {
    std::int64_t requested_ms = 0;
    std::optional<RefundDeficitPolicy> negative_policy;
};
struct TimerCapWarningPreview {
    std::string revision;
    std::int64_t main_remaining_ms = 0;
    std::optional<std::int64_t> old_cap_ms;
    std::int64_t proposed_cap_ms = 0;
};
struct TimerCapReplacement {
    std::optional<std::int64_t> maximum_remaining_ms;
    std::optional<TimerCapChoice> choice;
    std::optional<TimerCapWarningPreview> warning_preview;
};
struct TimerDeadlineReplacement {
    std::optional<std::int64_t> hard_end_utc_ms;
};
struct TimerFirstStartDeadlineCheck {};
using TimerEditorialPayload = std::variant<TimerSignedDelta, TimerCapReplacement,
    TimerDeadlineReplacement, TimerFirstStartDeadlineCheck>;
struct TimerEditorialCommand {
    std::string operation;
    std::string expected_revision;
    std::optional<std::string> actor;
    std::optional<std::string> reason;
    TimerEditorialPayload payload;
};
struct TimerEditorialInput {
    TimerEditorialSnapshot current;
    TimerEditorialCommand command;
    TimerEditorialAccess access;
    std::vector<TimerClockStep> observations;
    std::int64_t final_utc_ms = 0;
};
struct TimerEditorialLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t observation_steps = 4096;
    std::uint64_t input_owned_bytes = 8388608;
    std::uint64_t result_owned_bytes = 16777216;
};

enum class TimerEditorialDisposition : std::uint8_t {
    accepted, access_denied, access_unknown, revision_conflict,
    cap_choice_required, aborted, prerequisite_not_ready, policy_not_ready,
    timer_review_required, numeric_overflow_review,
    start_deadline_blocked, start_deadline_clear
};
enum class TimerEditorialPrerequisite : std::uint8_t {
    none, first_start_absolute_main, finished_policy
};
struct TimerEditorialTransitionBatch {
    std::uint8_t count = 0;
    std::array<TimerTransition, 4> entries{};
};
struct TimerEditorialAdjustment {
    std::int64_t requested_ms = 0;
    std::int64_t main_adjustment_ms = 0;
    std::int64_t debt_repaid_ms = 0;
    std::int64_t debt_added_ms = 0;
    std::int64_t cap_limited_ms = 0;
    std::int64_t unapplied_ms = 0;
};
struct TimerCapWarning {
    std::int64_t main_remaining_ms = 0;
    std::optional<std::int64_t> old_cap_ms;
    std::int64_t proposed_cap_ms = 0;
    std::int64_t reduction_ms = 0;
};
struct TimerEditorialProposalData {
    TimerEditorialInput input;
    TimerEditorialDisposition disposition = TimerEditorialDisposition::prerequisite_not_ready;
    TimerEditorialPrerequisite prerequisite = TimerEditorialPrerequisite::none;
    std::optional<TimerEditorialValues> observed_before;
    std::optional<TimerEditorialValues> accepted_after;
    std::vector<TimerEditorialTransitionBatch> observation_transitions;
    TimerEditorialTransitionBatch editorial_transitions;
    std::optional<TimerEditorialAdjustment> adjustment;
    std::optional<TimerCapWarning> warning;
};

class TimerEditorialProposal;
[[nodiscard]] TimerEditorialProposal project_timer_editorial_command(
    const TimerEditorialInput &input, const TimerEditorialLimits &limits = {});
class TimerEditorialProposal final {
  public:
    TimerEditorialProposal(const TimerEditorialProposal &) = default;
    TimerEditorialProposal(TimerEditorialProposal &&) noexcept = default;
    TimerEditorialProposal &operator=(const TimerEditorialProposal &) = default;
    TimerEditorialProposal &operator=(TimerEditorialProposal &&) noexcept = default;
    [[nodiscard]] const TimerEditorialProposalData &data() const noexcept;
  private:
    explicit TimerEditorialProposal(TimerEditorialProposalData data);
    TimerEditorialProposalData data_;
    friend TimerEditorialProposal project_timer_editorial_command(
        const TimerEditorialInput &, const TimerEditorialLimits &);
};
enum class TimerEditorialError : std::uint8_t {
    invalid_limits, input_limit, invalid_snapshot, invalid_command,
    invalid_access, invalid_observation, result_limit, allocation_failure
};
class TimerEditorialFailure final : public std::exception {
  public:
    explicit TimerEditorialFailure(TimerEditorialError code) noexcept;
    [[nodiscard]] TimerEditorialError code() const noexcept;
    [[nodiscard]] const char *what() const noexcept override;
  private:
    TimerEditorialError code_;
};
class TimerEditorialCommandsNotImplemented final : public std::logic_error {
  public:
    TimerEditorialCommandsNotImplemented();
};
} // namespace subathon::core
```

Only the evaluator constructs a proposal. All raw aggregates can be constructed
from literal data; no SUT factory, prior query or validity/permission certificate
is needed. The command variant index is its sole payload discriminator. A valueless
variant is invalid_command, not an empty/default operation. Its default is the
literal signed zero delta; empty required references still make a default input
invalid. No second operation is encoded by changing a lifecycle flag.

`data()` borrows immutable storage until its object is assigned, moved from or
destroyed. Results own all input strings/steps/settings, original and observed
facts, choices and after-values, surviving input destruction. Public copy/assignment
may allocate and throw std::bad_alloc outside evaluation's selected budgets/error
mapping. Moved-from values are destructible/assignable, without promised proposal
contents. Failure construction/code/what are inert and allocation-free; what() is
always `"Timer editorial command projection failed"`, with no input diagnostics.
The marker text is exactly
`"Timer editorial commands are intentionally unimplemented; review tests first"`.

## 2. Raw facts and valid shapes

All duration/adjustment/debt/cap values are already finalized signed64 milliseconds;
UTC fields are signed64 instants. No parser, float, rounding, timezone conversion,
clock read or restart reconstruction occurs. Current cap, Main/Grace/debt, configured
Grace duration and elapsed steps must be nonnegative; UTC may be negative or at
signed64 extrema. An enabled cap below existing Main is a VALID current state.
Never impose Main<=cap as a state invariant or reject INT64_MIN delta data.

Validate every existing enum in its declared alternatives, including cast values.
Use CORE-002's coherence rules for Active/Finished; Ready requires Main phase and
no confirmation wait. Grace requires Main0 and no wait. Wait requires Active/Main0
and Confirmation config. Ready/Finished retained fields are not silently reset.
The input has one unchanged old config/cap for every supplied step; configuration
mutations need separate host-sequenced operations. Each step has nonnegative
elapsed, with a constant known pause interpretation. observations is nonempty and
its final element's utc_now_ms equals final_utc_ms. No monotonic UTC ordering or
UTC-difference-as-elapsed rule is invented. The authoritative loader must stop an
unresolved clock/pause interval before invoking this complete-data projection.

Current revision, operation and expected_revision are required nonempty opaque
byte strings. An allowed access fact requires present nonempty permission_revision;
denied/unknown require it absent. actor/reason are honestly optional and may be
present-empty, distinct from absence. All bytes are preserved, including NUL/high
bytes; no token format or display label grants authority. Required-reference emptiness
is a typed error in the category owning that field, not a default identity.

For a negative delta, absent negative_policy is honest unresolved policy, producing
policy_not_ready after observation/lifecycle admission. No deficit default is chosen.
A present policy must be one of the three existing RefundDeficitPolicy values.
For a nonnegative delta it must be absent. Zero retains its command identity and
needs current access/revision and the same lifecycle admission as another delta.

Cap payloads are absent cap or nonnegative cap. A present choice must be in the
three alternatives. With absent cap, warning_preview must be absent and choice
must be absent or Abort; Keep/Reduce cannot describe removing a limit. With a finite
cap, Keep/Reduce may remain meaningful after elapsed time removed the excess:
Reduce never adds Main. Abort cancels the cap attempt even if a warning is no longer
needed. A present preview has a nonempty revision equal to command.expected_revision,
nonnegative Main/old/proposed caps, proposed cap equal to the command's finite cap,
and preview Main>proposed cap. Its old_cap must equal current cap AFTER revision
match; a stale expected revision is handled as conflict before this dependent check.
Preview Main need not equal observed-before Main. The preview is retained as history,
not substituted for current state or evidence that a warning was shown.

The first-Start variant checks ONLY the resolved deadline admission on Ready.
It is not Start, absolute-Main setting, setup validation or reopening. All ordinary
cap/deadline and local Ready/Finished delta boundaries are exactly the logical
matrix; no product ban is inferred from CORE-002's Ready precondition.

## 3. Stages and outcome/error priority

Finish each stage over the complete input before advancing. For several categories
in one stage, their listed order wins, independent of where the malformed value
appears. Reference/resource checks are internal diagnostics, not a public transport
error-disclosure policy.

| Stage | Required action / first applicable outcome |
| --- | --- |
| 0 | Positive/representable limits and checked derived bounds (§5): invalid_limits |
| 1 | Borrowed complete input B and string/step ceilings: input_limit, before copies |
| 2 | Complete structural/domain validation: invalid_snapshot, invalid_command, invalid_access, invalid_observation, in that priority order |
| 3 | Access denied → access_denied; unknown → access_unknown. Do not observe or edit |
| 4 | Expected/current revision unequal → revision_conflict. Do not observe or edit |
| 5 | Validate preview old-cap/current-cap consistency: invalid_command |
| 6 | Observe all steps under OLD config. Ready retains state/no transitions; Active/Finished use existing CORE-002. Retain every batch and observed_before |
| 7 | Resolve the chosen payload as §4. No earlier refusal is converted to a zero edit |
| 8 | Exact complete result B/container projection: result_limit, then materialize the owning proposal |

All typed errors throw TimerEditorialFailure with the respective code, leaving
input untouched. Snapshot/current revision invalidity belongs to invalid_snapshot;
command/expected/preview/variant/policy shape to invalid_command; access tag/revision
to invalid_access; missing/bad steps/final UTC binding to invalid_observation.
Oversized strings/step count or input count arithmetic overflow is input_limit
regardless of semantic defects. Invalid_limits outranks every input defect.
No access/revision refusal hides malformed input. The later code must translate
its own std::bad_alloc/std::length_error into allocation_failure without allocating
error text. Caller construction/copies, bad borrowed lifetimes/data races, hardware
failure and process termination remain outside that guarantee.

Only numeric overflow from an otherwise valid CORE-005/credit calculation becomes
a returned numeric_overflow_review; invalid fields are not silently reclassified
as review. All arithmetic effects of that command are zero, full request remains
unapplied, accepted_after is absent, and observed_before/old transitions remain
separate diagnostic facts. There is no partially repaid debt/Main/state result.

## 4. Payload resolution and complete audit data

Statuses describe PURE proposals, never applied/durable state. The result echoes
the entire input unchanged once. Observed/accepted values contain no new revision.
The host owns revision issuance, deduplication, actor authentication, fresh current
rights and actual atomic CAS/commit. An allowed access tag/permission reference or
checked revision is not a capability. Current whole-command permission must cover
the actual Reduce or delta; the server must recheck actual proposal intent/rights
at its authoritative execution/commit boundary, not rely on a stale preview grant.

Access/refusal/conflict results have no observed_before, accepted_after, adjustment
or warning; observation_transitions is empty, editorial_transitions.count=0 and
prerequisite=none. An absent observed-before means this entry did not evaluate those
intervals, not that elapsed was0 or the deadline inactive; current authoritative
clock progression remains a separate host duty. After observation,
observation_transitions has exactly one batch
per input step, including zero-count batches for unchanged Ready/Finished observations.
Each batch count is 0..4. Only entries[0..count) are events; unused entries are the
default TimerTransition::MainExpired and carry no event meaning. Flatten used
observation batches in input order, then the editorial batch, preserving chronology.
Batch association identifies its input segment, not an invented exact event UTC.

### Signed delta

If observed lifecycle is Ready or Finished, return prerequisite_not_ready with
first_start_absolute_main or finished_policy respectively; no accepted_after.
Adjustment retains full request, actual components0, unapplied=request. These are
local prerequisite outcomes, not an Active-only product release. CORE-007/API-007
remain mandatory before enabling those other paths. If Active and negative policy
is absent, return policy_not_ready with the same full-unapplied audit.

For an admitted nonnegative delta, reuse CORE-005 plan_time_addition over observed
Main/debt/current cap, then apply ONLY main_added_ms to the observed Active timer
through CORE-002 apply_applicable_time_credit. Do not install the numeric result's
Main and add it again. Retain debt repayment and cap-limited remainder separately.
Above-cap Main is permitted: it has zero headroom, still repays debt, and remains
above cap unless real countdown/correction changes it. Never invoke the older
above-cap-rejecting time-credit-v0 helper on this legitimate state.

For a negative delta, use the explicitly selected CORE-005 policy. RequiresReview
returns timer_review_required, no accepted_after, actual0 and full request unapplied.
For an applied subtraction, install CORE-005's resulting Main/debt into the observed
timer values, then run the existing zero-elapsed CORE-002 observation at final UTC/final pause to compose actual ending
behavior. Grace, wait, pauses and retained Fresh/Shared behavior follow that existing
contract, without negative Grace or a new phase machine. Positive credit emits no
new TimerTransition itself; its editorial batch has count 0. accepted_after is present
if and only if status is accepted, including a zero/no-op command.

Private CORE-005 numeric adapters use the EXISTING command.operation as a manual
caller-record identity. An ephemeral TimeAdditionRequest uses it as addition_ref;
a TimeReversalRequest uses it in both opaque reference slots solely to reuse the
numeric calculation over this caller-retained manual command. It is NOT a claim
that a financial original effect exists. Import numeric components only; never
return/persist that request as a contribution reversal or manufacture a financial
original_effect_ref. Public audit retains the typed manual command/actor/reason,
not a financial entitlement certificate. This adapter distinction needs actual
source review; no existing CORE-005 contract/header or accounting path is changed.

Adjustment has six exact scalar components. For nonnegative requests:
`requested = main_adjustment + debt_repaid + cap_limited + unapplied`.
For nonpositive requests:
`requested = main_adjustment - debt_added + unapplied`.
These are mathematical identities, not permission to add/negate signed64 intermediates.
All debt/cap components are nonnegative, Main actual has request sign, and unapplied
has request sign or zero. For accepted nonnegative deltas, unapplied is0 and the
remaining uncredited amount is specifically cap_limited; for accepted negative
deltas debt_repaid/cap_limited are0. Prerequisite/policy/review/overflow cases have
actual components0 and unapplied=request. Adjustment is absent for admission/access/
revision refusals before observation, ordinary cap-only/Keep, choice-required/Abort,
deadline edits and first-Start checks; explicit Reduce records its derived request.
Prerequisite is none except the specifically listed Ready/Finished/Start cases.
Signed minimum is retained without signed abs/negation.

### Cap and deadline replacement

If a finite cap is below observed Main, warning records observed Main, current old
cap, requested cap and exact positive reduction. Missing choice returns
cap_choice_required with no accepted_after/adjustment. Keep accepts cap and retains
all timer/debt fields. Reduce accepts cap plus actual Main reduction together,
with adjustment requested/main = cap-observed Main, all other components0. If no
excess remains, Reduce has requested/main0 and cannot add time. An absent choice
when no warning is required is a cap-only accepted edit, adjustment absent.
Abort always returns aborted with no accepted_after/adjustment; a current warning,
if applicable, remains recorded. Choice data/preview never proves actual UI display.

Reduce on Active composes the zero-elapsed existing lifecycle reducer when Main
becomes0; on Ready/Finished retain lifecycle with no editorial transitions.
It changes no debt, clears no pause, gives no financial entitlement and cannot
partially accept only the cap. At worst its requested reduction is -INT64_MAX,
which fits; no fictitious deficit policy is selected for this within-Main reduction.

Deadline replacement accepts on Ready/Active/Finished, preserving unrelated values.
Only an observed Active state is zero-observed under the new deadline at final UTC.
At/before-now gives HardEndReached,Finished, clears wait and preserves the existing
pre-call remaining-field convention. Ready/Finished stay so with count0; an old
observation that already finished cannot be undone by moving/disabling the deadline.
Identical settings still retain command identity; Finished does not finish twice.
Cap/deadline commands do not alter end mode, Grace configuration or history.

### First-Start check and host prerequisites

If observed Ready, enabled hard deadline<=final UTC gives start_deadline_blocked;
future/disabled gives start_deadline_clear. Both retain Ready unchanged, no
accepted_after/adjustment/editorial transitions and prerequisite=first_start_absolute_main.
Other observed lifecycles yield prerequisite_not_ready: first_start_absolute_main
for Active, finished_policy for Finished. Neither result is an actual Start.

No refusal/choice/abort/prerequisite/numeric-review result authorizes a clock-only
write. observed_before/batches are computation DATA; the host needs its separately
reviewed clock/command transaction contract for any independent clock commit and
must continue required deadline/checkpoint progression. Mixed effects/global review/
authorized continuation, post-finish input/action settings, explicit reopen/absolute
Main/grace/debt choices and hold/replay handling remain their owning mandatory gates.
There are no SQL/writer handles, fresh revision tokens or commit flags in this API.

## 5. Finite accounting and before-copy checks

B is semantic owned bytes, excluding allocator metadata/capacity and sizeof padding:
strings contribute payload size; int64/u64 contribute 8; enums/bool/u8 contribute 1;
optional contributes 1 plus its contained B when present; a variant contributes 1 plus
its active payload B; vector B is the sum; struct B sums declared fields. Fixed
transition array always contributes 4 plus its batch's one-byte count, even when
unused. The wrapper adds no extra field. No allocation/RSS equivalence is claimed.

Every nonnumeric string, including optional actor/reason/preview/access, is within
reference_bytes. Full input B<=input_owned_bytes; 1<=observations.size<=observation_steps.
For a valueless variant, borrowed preflight counts its one discriminator byte and
no active payload; stage2 then rejects it without visiting a nonexistent alternative.
Perform the complete borrowed preflight before any owned input/result clone, helper
reference copy or vector growth. All limits are positive, <=SIZE_MAX/PTRDIFF_MAX.
Checked representability of `6*observation_steps+4`, `22*observation_steps` and
`8*reference_bytes+512` is required at options validation. No Main+Grace sum or UTC
subtraction is used to validate clocks; do not sum all elapsed into an overflowing
signed accumulator when per-step progression suffices.

The only variable-size containers are the input steps and output observation batches.
A batch has at most 4 used events; total used transitions<=4*(N+1). Counting copied
steps, batch records and fixed slots gives result item bound <=6*N+4, default 24580,
within the logical default 32768. The derived bound replaces a redundant independent
item-limit field; wider valid step profiles derive their own checked larger bound.
Input step B=17N; observed batch B=5N. The four selected limits therefore bound all
variable strings/containers; no generic policy work counter or new SDK is needed.

Before constructing the complete returned DTO, project its ACTUAL complete B,
including copied input, optionals/after-values, preview, adjustment/warning and both
transition traces; require <=result_owned_bytes and representable container sizes.
No oversized proposal is first built then measured, no losing fields are omitted,
and a refusal still needs its full retained input/audit. A pure count/scalar pass
before materialization, possibly reevaluating deterministic existing helpers, is
allowed; tests must not demand an internal invocation/allocation count. Each bounded
CORE-002 temporary vector has at most 4 elements. Preflight CORE-005's fixed request/
result reference copies from known operation length before invoking it, retain only
numeric scalars for the final projection and release its temporary owning metadata.
Any extra whole DTO copy has its own complete prior projection. Library internal
capacity/overhead needs source audit; this document claims no physical heap bound.
Allocation/length failures map to the fixed typed failure, not a truncated proposal.
Structural/resource/allocation errors return no partially owned trace; input remains
caller-owned so the host can preserve/retry its original observations through its
separate clock path. A numeric overflow/review or any after-observation disposition
instead returns the complete bounded observed-before/trace as specified, not a
thrown primitive exception that silently loses those required facts. No failure
establishes that wall time stopped or a hard end was postponed.

## 6. Literal setup and stage gates

All 44 approved ED rows directly construct this raw input: original snapshot,
explicit command variant/policy/choice, allowed access with permission revision,
explicit clock steps and final UTC. First-Start ED28/29 use the check variant, not
a prior successful Start. Settings/deltas never require a SUT factory/certificate.
ED15/16 use conflicting revisions or denied/unknown access. No expected result may
be generated by invoking these helpers during fixture setup.

For a minimal literal byte setup, use one-byte refs current r / expected r /
operation o / permission p, absent actor/reason/preview/cap/deadline, one
elapsed0/UTC500/unpaused step and final UTC500. Explicit Active/Main1/Grace73,
Immediate/configured120/Fresh, debt0, signed delta0/policy absent and allowed access
is valid. TimerState B19, TimerConfig B11 and TimerEditorialValues B39 follow the
fixed fields, not sizeof.

| Independent literal | Required exact semantic B / outcome |
| --- | --- |
| That active zero-delta input | Input82; accepted result224, including input, both values, one batch, editorial batch and six scalar adjustment components |
| Same current values, denied access with permission absent | Input81; access_denied result92, no observed/accepted values or adjustment, no observation batches |
| Active/Main100/Grace73/debt5/cap120, same refs/clock, cap60/Reduce, preview absent | Input93; accepted result284, including warning33 plus presence tag, derived requested/actual -40 and unchanged debt |

Useful independent facts for later reviewed tests include:

- ED17 Keep Main100/debt5/cap60 then ED42 delta8: repay5, Main0, cap-limited3,
  Main100/debt0; ED44 elapsed45 then delta12: observed55, repay5/Main5/cap2, Main60.
- ED18 cap60 from Main100 gives exact requested/actual -40; ED21 observed90 gives
  -30, not replayed -40. If observation leaves Main<=60, Reduce cannot add Main.
- Main=INT64_MAX, cap0 Reduce gives -9223372036854775807, not INT64_MIN; delta
  INT64_MIN remains valid and 0/debt0 plus its full magnitude cannot fit debt.
- Default N=4096 permits at most 16388 used transitions, batch bytes 20480 and copied
  step bytes 69632; fixed-slot/container item bound 24580. All products are checked.

After actual API review, the future module marker is
TimerEditorialCommandsNotImplemented. The sole feature's entire minimal body is:

```cpp
throw TimerEditorialCommandsNotImplemented{};
```

No validation, currentness check, primitive call, allocation/copy helper or success
precedes it. Only inert types/getters/fixed exceptions may be supplied at that stage.
Build/discovery compiles/links/lists without executing a feature or marker in global
setup, configuration or build. Separate source/harness and test reviews, selected
green environment and FIRST plain intended behavioral red precede production
CORE-008. Host authentication/permissions/queue/CAS, UI warning, API-007/CORE-007/
ACCT-005/006 integration and release/platform verification remain explicit. Root
owns shared schema/spec/state/build/CI/PR52 and subsequent stage releases.
