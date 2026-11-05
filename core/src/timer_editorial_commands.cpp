// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/timer_editorial_commands.hpp>

#include <algorithm>
#include <limits>
#include <new>
#include <utility>

namespace subathon::core {
namespace {
using Error = TimerEditorialError;
using Disposition = TimerEditorialDisposition;
using Prerequisite = TimerEditorialPrerequisite;
using Values = TimerEditorialValues;
using Batch = TimerEditorialTransitionBatch;
constexpr auto representable = static_cast<std::uint64_t>(std::min<std::uintmax_t>(
    std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::ptrdiff_t>::max()));

[[noreturn]] void fail(Error error) { throw TimerEditorialFailure(error); }

void validate_limits(const TimerEditorialLimits &limits) {
    for (const auto value : {limits.reference_bytes, limits.observation_steps,
                             limits.input_owned_bytes, limits.result_owned_bytes})
        if (!value || value > representable)
            fail(Error::invalid_limits);
    if (limits.observation_steps > (representable - 4) / 6 ||
        limits.observation_steps > representable / 22 ||
        limits.reference_bytes > (representable - 512) / 8)
        fail(Error::invalid_limits);
}

std::uint64_t values_bytes(const Values &values) {
    // State19 + debt8 + optional cap1 + config11; present scalar optionals add8.
    return 39 + (values.maximum_remaining_ms ? 8 : 0) + (values.config.hard_end_utc_ms ? 8 : 0);
}

struct ByteCount {
    std::uint64_t total = 0;
    std::uint64_t ceiling;
    std::uint64_t reference_ceiling;
    Error error;

    void add(std::uint64_t bytes) {
        if (bytes > ceiling - total)
            fail(error);
        total += bytes;
    }
    void reference(const std::string &value) {
        if (value.size() > reference_ceiling || value.size() > std::string{}.max_size())
            fail(error);
        add(value.size());
    }
    void optional_reference(const std::optional<std::string> &value) {
        add(1);
        if (value)
            reference(*value);
    }
    void optional_scalar(const std::optional<std::int64_t> &value) { add(value ? 9 : 1); }
};

std::uint64_t preflight_input(const TimerEditorialInput &input,
                              const TimerEditorialLimits &limits) {
    ByteCount count{0, limits.input_owned_bytes, limits.reference_bytes, Error::input_limit};
    count.reference(input.current.revision);
    count.add(values_bytes(input.current.values));
    count.reference(input.command.operation);
    count.reference(input.command.expected_revision);
    count.optional_reference(input.command.actor);
    count.optional_reference(input.command.reason);
    count.add(1); // variant discriminator, including valueless input
    if (const auto *delta = std::get_if<TimerSignedDelta>(&input.command.payload)) {
        count.add(9 + (delta->negative_policy ? 1 : 0));
    } else if (const auto *cap = std::get_if<TimerCapReplacement>(&input.command.payload)) {
        count.optional_scalar(cap->maximum_remaining_ms);
        count.add(1 + (cap->choice ? 1 : 0));
        count.add(1);
        if (cap->warning_preview) {
            const auto &preview = *cap->warning_preview;
            count.reference(preview.revision);
            count.add(16);
            count.optional_scalar(preview.old_cap_ms);
        }
    } else if (const auto *deadline =
                   std::get_if<TimerDeadlineReplacement>(&input.command.payload)) {
        count.optional_scalar(deadline->hard_end_utc_ms);
    }
    count.add(1); // access state
    count.optional_reference(input.access.permission_revision);
    if (input.observations.size() > limits.observation_steps ||
        input.observations.size() > std::vector<TimerClockStep>{}.max_size())
        fail(Error::input_limit);
    // Validated22*step limit proves this multiplication representable before it occurs.
    count.add(17 * static_cast<std::uint64_t>(input.observations.size()));
    count.add(8); // final UTC
    return count.total;
}

bool nonnegative(const std::optional<std::int64_t> &value) { return !value || *value >= 0; }

bool valid_policy(RefundDeficitPolicy value) {
    return value == RefundDeficitPolicy::StopAtZero ||
           value == RefundDeficitPolicy::CarryTimeDebt ||
           value == RefundDeficitPolicy::RequireReview;
}

void validate_snapshot(const TimerEditorialSnapshot &snapshot) {
    const auto &values = snapshot.values;
    const auto &state = values.timer;
    const auto &config = values.config;
    // Existing core enums have full int range; never narrow them to the new u8 enums.
    if (snapshot.revision.empty() ||
        (state.lifecycle != TimerLifecycle::Ready && state.lifecycle != TimerLifecycle::Active &&
         state.lifecycle != TimerLifecycle::Finished) ||
        (state.phase != CountdownPhase::Main && state.phase != CountdownPhase::Grace) ||
        (config.end_mode != TimerEndMode::Immediate && config.end_mode != TimerEndMode::Grace &&
         config.end_mode != TimerEndMode::Confirmation) ||
        (config.grace_reuse != GraceReuse::Fresh && config.grace_reuse != GraceReuse::Shared) ||
        state.main_remaining_ms < 0 || state.grace_remaining_ms < 0 || values.time_debt_ms < 0 ||
        !nonnegative(values.maximum_remaining_ms) || config.grace_duration_ms < 0 ||
        (state.phase == CountdownPhase::Grace &&
         (state.main_remaining_ms != 0 || state.awaiting_confirmation)) ||
        (state.awaiting_confirmation &&
         (state.lifecycle != TimerLifecycle::Active || state.phase != CountdownPhase::Main ||
          state.main_remaining_ms != 0 || config.end_mode != TimerEndMode::Confirmation)) ||
        (state.lifecycle == TimerLifecycle::Ready &&
         (state.phase != CountdownPhase::Main || state.awaiting_confirmation)))
        fail(Error::invalid_snapshot);
}

void validate_command(const TimerEditorialCommand &command) {
    if (command.operation.empty() || command.expected_revision.empty() ||
        command.payload.valueless_by_exception())
        fail(Error::invalid_command);
    if (const auto *delta = std::get_if<TimerSignedDelta>(&command.payload)) {
        if (delta->negative_policy &&
            (delta->requested_ms >= 0 || !valid_policy(*delta->negative_policy)))
            fail(Error::invalid_command);
    } else if (const auto *cap = std::get_if<TimerCapReplacement>(&command.payload)) {
        if (!nonnegative(cap->maximum_remaining_ms) ||
            (cap->choice && *cap->choice != TimerCapChoice::keep &&
             *cap->choice != TimerCapChoice::reduce && *cap->choice != TimerCapChoice::abort) ||
            (!cap->maximum_remaining_ms &&
             (cap->warning_preview || (cap->choice && *cap->choice != TimerCapChoice::abort))))
            fail(Error::invalid_command);
        if (cap->warning_preview) {
            const auto &preview = *cap->warning_preview;
            if (preview.revision.empty() || preview.revision != command.expected_revision ||
                preview.main_remaining_ms < 0 || !nonnegative(preview.old_cap_ms) ||
                preview.proposed_cap_ms < 0 ||
                preview.proposed_cap_ms != *cap->maximum_remaining_ms ||
                preview.main_remaining_ms <= preview.proposed_cap_ms)
                fail(Error::invalid_command);
        }
    }
}

void validate_access(const TimerEditorialAccess &access) {
    if ((access.state != TimerEditorialAccessState::unknown &&
         access.state != TimerEditorialAccessState::denied &&
         access.state != TimerEditorialAccessState::allowed) ||
        ((access.state == TimerEditorialAccessState::allowed) !=
         access.permission_revision.has_value()) ||
        (access.permission_revision && access.permission_revision->empty()))
        fail(Error::invalid_access);
}

void validate_observations(const TimerEditorialInput &input) {
    if (input.observations.empty() || input.observations.back().utc_now_ms != input.final_utc_ms)
        fail(Error::invalid_observation);
    for (const auto &step : input.observations)
        if (step.elapsed_ms < 0)
            fail(Error::invalid_observation);
}

Batch batch(const TimerAdvance &advance) {
    if (advance.transitions.size() > 4)
        throw std::logic_error("Lifecycle transition contract exceeded its bound");
    Batch result;
    result.count = static_cast<std::uint8_t>(advance.transitions.size());
    std::copy(advance.transitions.begin(), advance.transitions.end(), result.entries.begin());
    return result;
}

// Scalar observation pass: each existing reducer temporary has at most four events.
// Ready is explicitly retained; it is not passed to the already-started reducer.
template <class Record> Values observe(const TimerEditorialInput &input, Record record) {
    auto values = input.current.values;
    for (const auto &step : input.observations) {
        Batch transitions;
        if (values.timer.lifecycle != TimerLifecycle::Ready) {
            const auto advance = advance_timer(values.timer, values.config, step);
            values.timer = advance.state;
            transitions = batch(advance);
        }
        record(transitions);
    }
    return values;
}

void observe_editorial(Values &values, const TimerEditorialInput &input, Batch &transitions) {
    const auto advance = advance_timer(values.timer, values.config,
                                       {0, input.final_utc_ms, input.observations.back().paused});
    values.timer = advance.state;
    transitions = batch(advance);
}

// No owning strings/vectors/input copy: only fixed scalar state and optional audit fields.
struct Computed {
    Disposition disposition = Disposition::accepted;
    Prerequisite prerequisite = Prerequisite::none;
    std::optional<Values> observed;
    std::optional<Values> after;
    Batch editorial;
    std::optional<TimerEditorialAdjustment> adjustment;
    std::optional<TimerCapWarning> warning;
};

void preflight_numeric(const std::string &operation, const TimerEditorialLimits &limits,
                       bool reversal) {
    // Complete simultaneous request/result metadata: addition2L+56; reversal4L+58.
    // Stage0/1 ensure the actual fixed helper work fits the derived8R+512 bound.
    const auto length = static_cast<std::uint64_t>(operation.size());
    const auto bytes = reversal ? 4 * length + 58 : 2 * length + 56;
    if (bytes > 8 * limits.reference_bytes + 512)
        fail(Error::input_limit);
}

void signed_delta(const TimerEditorialInput &input, const TimerSignedDelta &delta,
                  const TimerEditorialLimits &limits, Computed &result) {
    const auto &before = *result.observed;
    const auto request = delta.requested_ms;
    result.adjustment = TimerEditorialAdjustment{request, 0, 0, 0, 0, request};
    if (before.timer.lifecycle != TimerLifecycle::Active) {
        result.disposition = Disposition::prerequisite_not_ready;
        result.prerequisite = before.timer.lifecycle == TimerLifecycle::Ready
                                  ? Prerequisite::first_start_absolute_main
                                  : Prerequisite::finished_policy;
        return;
    }
    if (request < 0 && !delta.negative_policy) {
        result.disposition = Disposition::policy_not_ready;
        return;
    }
    auto after = before;
    const TimeBalance balance{before.timer.main_remaining_ms, before.time_debt_ms};
    try {
        preflight_numeric(input.command.operation, limits, request < 0);
        if (request >= 0) {
            const auto planned = plan_time_addition(balance, {input.command.operation, request},
                                                    before.maximum_remaining_ms);
            after.timer = apply_applicable_time_credit(before.timer, planned.main_added_ms);
            after.time_debt_ms = planned.balance_after.time_debt_ms;
            result.adjustment =
                TimerEditorialAdjustment{request, planned.main_added_ms,  planned.debt_repaid_ms,
                                         0,       planned.cap_limited_ms, 0};
        } else {
            // Both ephemeral identities name this MANUAL command solely for numeric reuse;
            // no financial original effect or entitlement is returned/persisted by this adapter.
            const auto planned = plan_time_reversal(
                balance, {input.command.operation, input.command.operation, request},
                *delta.negative_policy);
            if (planned.disposition == TimeReversalDisposition::RequiresReview) {
                result.disposition = Disposition::timer_review_required;
                return;
            }
            after.timer.main_remaining_ms = planned.balance_after.main_remaining_ms;
            after.time_debt_ms = planned.balance_after.time_debt_ms;
            result.adjustment = TimerEditorialAdjustment{request, planned.main_adjustment_ms,
                                                         0,       planned.debt_added_ms,
                                                         0,       planned.unapplied_adjustment_ms};
            observe_editorial(after, input, result.editorial);
        }
        result.after = after;
    } catch (const std::overflow_error &) {
        result.disposition = Disposition::numeric_overflow_review;
        result.after.reset();
        result.editorial = {};
        result.adjustment = TimerEditorialAdjustment{request, 0, 0, 0, 0, request};
    }
}

void cap_replacement(const TimerEditorialInput &input, const TimerCapReplacement &cap,
                     Computed &result) {
    const auto &before = *result.observed;
    const auto main = before.timer.main_remaining_ms;
    if (cap.maximum_remaining_ms && *cap.maximum_remaining_ms < main)
        result.warning =
            TimerCapWarning{main, before.maximum_remaining_ms, *cap.maximum_remaining_ms,
                            main - *cap.maximum_remaining_ms};
    if (cap.choice == TimerCapChoice::abort) {
        result.disposition = Disposition::aborted;
        return;
    }
    if (result.warning && !cap.choice) {
        result.disposition = Disposition::cap_choice_required;
        return;
    }
    auto after = before;
    after.maximum_remaining_ms = cap.maximum_remaining_ms;
    if (cap.choice == TimerCapChoice::reduce) {
        const auto reduction =
            main > *cap.maximum_remaining_ms ? main - *cap.maximum_remaining_ms : 0;
        const auto adjustment = -reduction; // reduction<=INT64_MAX, never negate INT64_MIN
        after.timer.main_remaining_ms -= reduction;
        result.adjustment = TimerEditorialAdjustment{adjustment, adjustment, 0, 0, 0, 0};
        if (after.timer.lifecycle == TimerLifecycle::Active && after.timer.main_remaining_ms == 0)
            observe_editorial(after, input, result.editorial);
    }
    result.after = after;
}

Computed compute(const TimerEditorialInput &input, const TimerEditorialLimits &limits) {
    Computed result;
    if (input.access.state != TimerEditorialAccessState::allowed) {
        result.disposition = input.access.state == TimerEditorialAccessState::denied
                                 ? Disposition::access_denied
                                 : Disposition::access_unknown;
        return result;
    }
    if (input.current.revision != input.command.expected_revision) {
        result.disposition = Disposition::revision_conflict;
        return result;
    }
    if (const auto *cap = std::get_if<TimerCapReplacement>(&input.command.payload))
        if (cap->warning_preview &&
            cap->warning_preview->old_cap_ms != input.current.values.maximum_remaining_ms)
            fail(Error::invalid_command);
    result.observed = observe(input, [](const Batch &) {});
    if (const auto *delta = std::get_if<TimerSignedDelta>(&input.command.payload)) {
        signed_delta(input, *delta, limits, result);
    } else if (const auto *cap = std::get_if<TimerCapReplacement>(&input.command.payload)) {
        cap_replacement(input, *cap, result);
    } else if (const auto *deadline =
                   std::get_if<TimerDeadlineReplacement>(&input.command.payload)) {
        auto after = *result.observed;
        after.config.hard_end_utc_ms = deadline->hard_end_utc_ms;
        if (after.timer.lifecycle == TimerLifecycle::Active)
            observe_editorial(after, input, result.editorial);
        result.after = after;
    } else {
        const auto &before = *result.observed;
        result.prerequisite = before.timer.lifecycle == TimerLifecycle::Finished
                                  ? Prerequisite::finished_policy
                                  : Prerequisite::first_start_absolute_main;
        if (before.timer.lifecycle == TimerLifecycle::Ready)
            result.disposition = before.config.hard_end_utc_ms &&
                                         *before.config.hard_end_utc_ms <= input.final_utc_ms
                                     ? Disposition::start_deadline_blocked
                                     : Disposition::start_deadline_clear;
        else
            result.disposition = Disposition::prerequisite_not_ready;
    }
    return result;
}

void preflight_result(std::uint64_t input_bytes, const TimerEditorialInput &input,
                      const TimerEditorialLimits &limits, const Computed &computed) {
    ByteCount count{0, limits.result_owned_bytes, limits.reference_bytes, Error::result_limit};
    count.add(input_bytes);
    // Disposition/prerequisite2, observed/after tags2, fixed editorial batch5,
    // adjustment/warning tags2; vector has no header byte charge.
    count.add(11);
    if (computed.observed) {
        count.add(values_bytes(*computed.observed));
        if (input.observations.size() > std::vector<Batch>{}.max_size())
            fail(Error::result_limit);
        count.add(5 * static_cast<std::uint64_t>(input.observations.size()));
    }
    if (computed.after)
        count.add(values_bytes(*computed.after));
    if (computed.adjustment)
        count.add(48);
    if (computed.warning)
        count.add(25 + (computed.warning->old_cap_ms ? 8 : 0));
}

TimerEditorialProposalData materialize(const TimerEditorialInput &input, const Computed &computed) {
    // The first owning DTO and complete input copy follow the full exact projection.
    TimerEditorialProposalData data;
    data.input = input;
    data.disposition = computed.disposition;
    data.prerequisite = computed.prerequisite;
    data.observed_before = computed.observed;
    data.accepted_after = computed.after;
    data.editorial_transitions = computed.editorial;
    data.adjustment = computed.adjustment;
    data.warning = computed.warning;
    if (computed.observed) {
        data.observation_transitions.reserve(input.observations.size());
        (void)observe(input, [&](const Batch &transitions) {
            data.observation_transitions.push_back(transitions);
        });
    }
    return data;
}
} // namespace

TimerEditorialProposal::TimerEditorialProposal(TimerEditorialProposalData data)
    : data_(std::move(data)) {}

const TimerEditorialProposalData &TimerEditorialProposal::data() const noexcept { return data_; }

TimerEditorialFailure::TimerEditorialFailure(TimerEditorialError code) noexcept : code_(code) {}

TimerEditorialError TimerEditorialFailure::code() const noexcept { return code_; }

const char *TimerEditorialFailure::what() const noexcept {
    return "Timer editorial command projection failed";
}

TimerEditorialCommandsNotImplemented::TimerEditorialCommandsNotImplemented()
    : std::logic_error(
          "Timer editorial commands are intentionally unimplemented; review tests first") {}

TimerEditorialProposal project_timer_editorial_command(const TimerEditorialInput &input,
                                                       const TimerEditorialLimits &limits) {
    try {
        validate_limits(limits);
        const auto input_bytes = preflight_input(input, limits);
        validate_snapshot(input.current);
        validate_command(input.command);
        validate_access(input.access);
        validate_observations(input);
        const auto computed = compute(input, limits);
        preflight_result(input_bytes, input, limits, computed);
        return TimerEditorialProposal(materialize(input, computed));
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_failure);
    } catch (const std::length_error &) {
        fail(Error::allocation_failure);
    }
}
} // namespace subathon::core
