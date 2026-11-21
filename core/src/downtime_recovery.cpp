// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/downtime_recovery.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace subathon::core {
namespace {

using Instant = std::int64_t;
using Window = ScheduledPauseWindow;
using Deadline = RecoveryDeadlineStatus;
using ScheduleUnavailable = RecoveryScheduleUnavailable;

constexpr auto min_ms = std::numeric_limits<Instant>::min();
constexpr auto max_ms = std::numeric_limits<Instant>::max();
constexpr auto day_ms = std::chrono::milliseconds{std::chrono::days{1}}.count();
constexpr auto padding_ms = 2 * day_ms;
constexpr auto slice_ms = std::chrono::milliseconds{schedule_preview_max_span}.count();
constexpr auto review = PauseReason::RecoveryReview;
constexpr auto schedule = PauseReason::Schedule;

struct InputLimit {};

// Charge logical field widths (including presence/variant tags), string payloads
// and each element of every supplied span, even when arguments share storage.
class InputBudget {
  public:
    void bytes(std::size_t count) {
        if (count > remaining_) {
            throw InputLimit{};
        }
        remaining_ -= count;
    }

    template <typename... Strings> void references(const Strings &...values) {
        (reference(values), ...);
    }

    void optional_reference(const std::optional<std::string> &value) {
        bytes(1);
        if (value) {
            reference(*value);
        }
    }

    void entries(std::size_t count) {
        if (count > entries_remaining_) {
            throw InputLimit{};
        }
        entries_remaining_ -= count;
    }

    void rows(std::size_t count, std::size_t width) {
        if (count > remaining_ / width) {
            throw InputLimit{};
        }
        bytes(count * width);
    }

  private:
    void reference(const std::string &value) {
        if (value.size() > recovery_max_reference_bytes) {
            throw InputLimit{};
        }
        bytes(value.size());
    }

    std::size_t remaining_ = recovery_max_input_bytes;
    std::size_t entries_remaining_ = recovery_max_schedule_entries;
};

template <typename T> std::size_t optional_instant_bytes(const std::optional<T> &instant) {
    return instant ? 9 : 1;
}

void charge(InputBudget &budget, const TimerConfig &config) {
    budget.bytes(10 + optional_instant_bytes(config.hard_end_utc_ms));
}

void charge(InputBudget &budget, const PauseState &pause) {
    budget.rows(pause.reasons.size(), 1);
    budget.bytes(1 + optional_instant_bytes(pause.stream_offline_pending_ms) +
                 optional_instant_bytes(pause.client_disconnect_pending_ms) +
                 optional_instant_bytes(pause.stream_unknown_pending_ms));
}

void charge(InputBudget &budget, const RecoveryUtcObservation &observation) {
    budget.references(observation.observation_ref);
    budget.bytes(optional_instant_bytes(observation.utc));
}

void charge(InputBudget &budget, const DowntimeRecoveryInput &input) {
    const auto &basis = input.basis;
    budget.references(basis.timer_lineage, basis.checkpoint_ref, basis.configuration_revision,
                      basis.facts_bundle_ref);
    // Timer state (19), downtime policy (1), gap and schedule discriminants (2).
    budget.bytes(22);
    charge(budget, input.config);
    charge(budget, input.pause);
    charge(budget, input.checkpoint);
    charge(budget, input.restart);
    if (const auto *gap = std::get_if<RecoveryContinuousGap>(&input.gap)) {
        budget.references(gap->evidence_ref);
        budget.bytes(16);
    } else {
        budget.bytes(1);
    }

    const auto *coverage = std::get_if<RecoveryScheduleCoverage>(&input.schedule);
    if (!coverage) {
        budget.bytes(1);
        return;
    }
    budget.entries(coverage->schedules.size());
    budget.entries(coverage->retained.size());
    budget.references(coverage->facts_ref, coverage->delivery_revision);
    budget.bytes(1);
    if (coverage->zone) {
        budget.bytes(20); // Two endpoint instants and the initial offset.
        budget.rows(coverage->zone->transitions.size(), 12);
    }
    for (const auto &rule : coverage->schedules) {
        budget.entries(rule.delivered.size());
        budget.references(rule.schedule_ref);
        budget.bytes(25); // Local time, weekdays, gap/fold policies, and duration.
        budget.rows(rule.delivered.size(), 9);
    }
    for (const auto &row : coverage->retained) {
        budget.references(row.schedule_ref);
        budget.bytes(25 + optional_instant_bytes(row.stopped_at));
    }
}

void charge(InputBudget &budget, const RecoveryReviewSnapshot &snapshot) {
    budget.references(snapshot.episode_ref, snapshot.timer_lineage, snapshot.facts_bundle_ref,
                      snapshot.current.revision);
    budget.bytes(27 + optional_instant_bytes(snapshot.current.values.maximum_remaining_ms));
    charge(budget, snapshot.current.values.config);
    charge(budget, snapshot.pause);
    charge(budget, snapshot.observed_at);
}

void charge(InputBudget &budget, const RecoveryReviewCommand &command) {
    budget.references(command.operation_ref, command.expected_episode_ref,
                      command.expected_revision, command.expected_facts_bundle_ref);
    budget.optional_reference(command.reason);
    budget.bytes(command.choice ? 2 : 1);
    budget.bytes(1);
    if (command.correction) {
        budget.bytes(command.correction->negative_policy ? 10 : 9);
    }
}

void charge(InputBudget &budget, const RecoveryReviewAccess &access) {
    budget.bytes(1);
    if (const auto *authorization = std::get_if<RecoveryReviewAuthorization>(&access)) {
        charge(budget, authorization->command);
        budget.references(authorization->revision, authorization->observation_ref,
                          authorization->actor_ref, authorization->permission_revision);
        budget.bytes(1);
        if (authorization->correction_access) {
            budget.bytes(1);
            budget.optional_reference(authorization->correction_access->permission_revision);
        }
    } else {
        budget.bytes(1);
    }
}

void require(bool condition) {
    if (!condition) {
        throw std::invalid_argument("Invalid downtime recovery input");
    }
}

template <typename Enum, typename... Choices> bool one_of(Enum value, Choices... choices) {
    return ((value == choices) || ...);
}

bool valid_timer(const TimerState &timer, const TimerConfig &config) {
    return one_of(timer.lifecycle, TimerLifecycle::Ready, TimerLifecycle::Active,
                  TimerLifecycle::Finished) &&
           one_of(timer.phase, CountdownPhase::Main, CountdownPhase::Grace) &&
           one_of(config.end_mode, TimerEndMode::Immediate, TimerEndMode::Grace,
                  TimerEndMode::Confirmation) &&
           one_of(config.grace_reuse, GraceReuse::Fresh, GraceReuse::Shared) &&
           timer.main_remaining_ms >= 0 && timer.grace_remaining_ms >= 0 &&
           config.grace_duration_ms >= 0 &&
           (timer.phase == CountdownPhase::Main ||
            (timer.lifecycle != TimerLifecycle::Ready && timer.main_remaining_ms == 0)) &&
           (!timer.awaiting_confirmation ||
            (timer.lifecycle == TimerLifecycle::Active && timer.phase == CountdownPhase::Main &&
             timer.main_remaining_ms == 0 && config.end_mode == TimerEndMode::Confirmation));
}

void validate_pause(const PauseState &pause) {
    for (const auto reason : pause.reasons) {
        require(one_of(reason, PauseReason::Manual, schedule, PauseReason::StreamOffline,
                       PauseReason::DesignatedClientDisconnected, review,
                       PauseReason::StreamStatusUnknown));
    }
    // Use the pause owner's validation without advancing any pending signal.
    (void)set_pause_reason(pause, review, pause.reasons.contains(review));
}

// Subtract in unsigned space after checking order, including spans crossing zero.
std::uint64_t distance(Instant from, Instant to) {
    return static_cast<std::uint64_t>(to) - static_cast<std::uint64_t>(from);
}

void validate_clock(const DowntimeRecoveryInput &input) {
    require(!input.checkpoint.observation_ref.empty() && !input.restart.observation_ref.empty());
    if (const auto *gap = std::get_if<RecoveryContinuousGap>(&input.gap)) {
        require(!gap->evidence_ref.empty() && input.checkpoint.utc == gap->checkpoint &&
                input.restart.utc == gap->restart && gap->checkpoint.ms <= gap->restart.ms &&
                distance(gap->checkpoint.ms, gap->restart.ms) <=
                    static_cast<std::uint64_t>(max_ms));
    } else {
        const auto *unavailable = std::get_if<RecoveryGapUnavailable>(&input.gap);
        require(unavailable && one_of(*unavailable, RecoveryGapUnavailable::Unknown,
                                      RecoveryGapUnavailable::Discontinuous,
                                      RecoveryGapUnavailable::Unrepresentable));
    }
}

// CORE-004 also checks these shape invariants when previewing. Check the
// supplied shape even when insufficient UTC coverage prevents any preview.
void validate_zone(const ScheduleZoneRules &zone) {
    require(zone.coverage_start_utc_ms <= zone.coverage_end_utc_ms &&
            zone.transitions.size() <= schedule_zone_max_transitions);
    std::array<Window, schedule_zone_max_transitions + 1> local{};
    std::size_t count = 0;
    auto first = zone.coverage_start_utc_ms;
    auto offset_seconds = zone.offset_at_start_seconds;
    const auto append = [&](Instant last) {
        require(offset_seconds >= -86'400 && offset_seconds <= 86'400);
        const auto offset = std::chrono::milliseconds{std::chrono::seconds{offset_seconds}}.count();
        const auto clipped_first = offset < 0 ? std::max(first, min_ms - offset) : first;
        const auto clipped_last = offset > 0 ? std::min(last, max_ms - offset) : last;
        if (clipped_first <= clipped_last) {
            local[count++] = {clipped_first + offset, clipped_last + offset};
        }
    };
    for (const auto &transition : zone.transitions) {
        require(first < transition.at_utc_ms && transition.at_utc_ms <= zone.coverage_end_utc_ms);
        append(transition.at_utc_ms - 1);
        first = transition.at_utc_ms;
        offset_seconds = transition.offset_after_seconds;
    }
    append(zone.coverage_end_utc_ms);

    const std::span intervals{local.data(), count};
    for (const auto &interval : intervals) {
        require(std::ranges::count_if(intervals, [&](const auto &other) {
                    return other.start_utc_ms <= interval.start_utc_ms &&
                           interval.start_utc_ms <= other.end_utc_ms;
                }) <= 2);
    }
}

using NominalKey = std::pair<ScheduleLocalTime, ScheduleFoldRun>;

NominalKey nominal_key(const ScheduleOccurrenceId &id) {
    require(one_of(id.run, ScheduleFoldRun::First, ScheduleFoldRun::Second));
    return {id.nominal_local_start, id.run};
}

Instant effective_end(const RecoveryRetainedPause &row) {
    // Validation establishes that an explicit stop is inside the original bounds.
    return row.stopped_at ? row.stopped_at->ms : row.window.end_utc_ms;
}

void validate_schedule(const DowntimeRecoveryInput &input) {
    const auto *coverage = std::get_if<RecoveryScheduleCoverage>(&input.schedule);
    if (!coverage) {
        const auto *unavailable = std::get_if<ScheduleUnavailable>(&input.schedule);
        require(unavailable &&
                one_of(*unavailable, ScheduleUnavailable::Coverage,
                       ScheduleUnavailable::DeliveryContext, ScheduleUnavailable::Source,
                       ScheduleUnavailable::ReconstructionLimit));
        return;
    }
    require(input.checkpoint.utc && input.restart.utc &&
            input.checkpoint.utc->ms <= input.restart.utc->ms && !coverage->facts_ref.empty() &&
            !coverage->delivery_revision.empty());
    if (coverage->zone) {
        validate_zone(*coverage->zone);
    }

    std::map<std::string_view, std::set<NominalKey>> enabled;
    for (const auto &rule : coverage->schedules) {
        require(!rule.schedule_ref.empty() && rule.duration.count() > 0 &&
                rule.recurrence.local_time >= std::chrono::milliseconds::zero() &&
                rule.recurrence.local_time < std::chrono::days{1} &&
                one_of(rule.recurrence.gap_policy, ScheduleGapPolicy::ShiftForward,
                       ScheduleGapPolicy::Skip) &&
                one_of(rule.recurrence.fold_policy, ScheduleFoldPolicy::RunOnce,
                       ScheduleFoldPolicy::RunTwice));
        auto [position, inserted] = enabled.try_emplace(rule.schedule_ref);
        require(inserted);
        for (const auto &id : rule.delivered) {
            require(position->second.insert(nominal_key(id)).second);
        }
    }

    const auto checkpoint = input.checkpoint.utc->ms;
    std::set<std::pair<std::string_view, NominalKey>> retained;
    bool checkpoint_paused = false;
    for (const auto &row : coverage->retained) {
        const auto [start, end] = row.window;
        const auto stop = effective_end(row);
        require(!row.schedule_ref.empty() && start <= checkpoint && start < end && start <= stop &&
                stop <= end);
        const auto key = nominal_key(row.occurrence);
        require(retained.emplace(row.schedule_ref, key).second);
        const auto rule = enabled.find(row.schedule_ref);
        require(rule == enabled.end() || rule->second.contains(key));
        checkpoint_paused |= checkpoint < stop;
    }
    require(input.timer.lifecycle != TimerLifecycle::Active ||
            checkpoint_paused == input.pause.reasons.contains(schedule));
}

void validate(const DowntimeRecoveryInput &input) {
    const auto &basis = input.basis;
    require(!basis.timer_lineage.empty() && !basis.checkpoint_ref.empty() &&
            !basis.configuration_revision.empty() && !basis.facts_bundle_ref.empty() &&
            one_of(input.policy, DowntimePolicy::Freeze, DowntimePolicy::CountElapsed,
                   DowntimePolicy::Review) &&
            valid_timer(input.timer, input.config));
    validate_pause(input.pause);
    validate_clock(input);
    validate_schedule(input);
}

void validate(const RecoveryReviewSnapshot &snapshot) {
    const auto &values = snapshot.current.values;
    require(!snapshot.episode_ref.empty() && !snapshot.timer_lineage.empty() &&
            !snapshot.facts_bundle_ref.empty() && !snapshot.current.revision.empty() &&
            !snapshot.observed_at.observation_ref.empty() &&
            valid_timer(values.timer, values.config) && values.time_debt_ms >= 0 &&
            values.maximum_remaining_ms.value_or(0) >= 0);
    validate_pause(snapshot.pause);
}

void validate(const RecoveryReviewCommand &command) {
    require(!command.operation_ref.empty() && !command.expected_episode_ref.empty() &&
            !command.expected_revision.empty() && !command.expected_facts_bundle_ref.empty());
    require(!command.choice || one_of(*command.choice, RecoveryOfflineChoice::Freeze,
                                      RecoveryOfflineChoice::CountElapsed));
    if (command.correction && command.correction->negative_policy) {
        require(command.correction->requested_ms < 0 &&
                one_of(*command.correction->negative_policy, RefundDeficitPolicy::StopAtZero,
                       RefundDeficitPolicy::CarryTimeDebt, RefundDeficitPolicy::RequireReview));
    }
}

void validate(const RecoveryReviewAccess &access) {
    if (const auto *authorization = std::get_if<RecoveryReviewAuthorization>(&access)) {
        validate(authorization->command);
        require(!authorization->revision.empty() && !authorization->observation_ref.empty() &&
                !authorization->actor_ref.empty() && !authorization->permission_revision.empty());
        if (const auto &editorial = authorization->correction_access) {
            require(one_of(editorial->state, TimerEditorialAccessState::allowed,
                           TimerEditorialAccessState::denied, TimerEditorialAccessState::unknown) &&
                    ((editorial->state == TimerEditorialAccessState::allowed) ==
                     editorial->permission_revision.has_value()) &&
                    (!editorial->permission_revision || !editorial->permission_revision->empty()));
        }
    } else {
        const auto *refusal = std::get_if<RecoveryReviewAccessRefusal>(&access);
        require(refusal && one_of(*refusal, RecoveryReviewAccessRefusal::Denied,
                                  RecoveryReviewAccessRefusal::Unknown));
    }
}

using Windows = std::vector<Window>;

void merge_windows(Windows &windows) {
    std::ranges::sort(windows, {}, &Window::start_utc_ms);
    std::size_t count = 0;
    for (const auto window : windows) {
        if (count != 0 && window.start_utc_ms <= windows[count - 1].end_utc_ms) {
            auto &end = windows[count - 1].end_utc_ms;
            end = std::max(end, window.end_utc_ms);
        } else {
            windows[count++] = window;
        }
    }
    windows.resize(count);
}

std::optional<ScheduleUnavailable> preview_windows(const RecoveryScheduleCoverage &coverage,
                                                   Instant checkpoint, Instant restart,
                                                   Windows &windows) {
    if (coverage.schedules.empty()) {
        return {};
    }
    // Check the whole range before previewing any slice, with unsigned span
    // arithmetic even when clock continuity was unavailable.
    if (!coverage.zone || checkpoint < min_ms + padding_ms || restart > max_ms - padding_ms ||
        coverage.zone->coverage_start_utc_ms > checkpoint - padding_ms ||
        coverage.zone->coverage_end_utc_ms < restart + padding_ms) {
        return ScheduleUnavailable::Coverage;
    }
    const auto &zone = *coverage.zone;
    for (const auto &rule : coverage.schedules) {
        const auto duration = rule.duration.count();
        if (restart > max_ms - duration || zone.coverage_end_utc_ms < restart + duration) {
            return ScheduleUnavailable::Coverage;
        }
    }
    const auto span = distance(checkpoint, restart);
    const auto slices = span / slice_ms + (span % slice_ms != 0);
    if (slices > recovery_max_schedule_previews / coverage.schedules.size()) {
        return ScheduleUnavailable::ReconstructionLimit;
    }

    for (auto after = checkpoint; after < restart;) {
        const auto through = distance(after, restart) > static_cast<std::uint64_t>(slice_ms)
                                 ? after + slice_ms
                                 : restart;
        for (const auto &rule : coverage.schedules) {
            try {
                const auto selected = preview_scheduled_pauses(
                    rule.recurrence, zone, {after, through}, rule.duration, rule.delivered);
                if (selected.size() > recovery_max_schedule_windows - windows.size()) {
                    return ScheduleUnavailable::ReconstructionLimit;
                }
                for (const auto &pause : selected) {
                    windows.push_back({pause.start.start_utc_ms, pause.end_utc_ms});
                }
            } catch (const std::invalid_argument &) {
                // The rule and zone shape were checked above. CORE-004 can
                // still find an unrepresentable projected local end.
                return ScheduleUnavailable::Coverage;
            }
        }
        after = through;
    }
    return {};
}

std::variant<Windows, ScheduleUnavailable>
reconstruct_schedule(const DowntimeRecoveryInput &input) {
    if (const auto *unavailable = std::get_if<ScheduleUnavailable>(&input.schedule)) {
        return *unavailable;
    }
    const auto &coverage = std::get<RecoveryScheduleCoverage>(input.schedule);
    Windows windows;
    windows.reserve(coverage.retained.size());
    for (const auto &row : coverage.retained) {
        const auto end = effective_end(row);
        if (row.window.start_utc_ms < end) {
            windows.push_back({row.window.start_utc_ms, end});
        }
    }
    if (const auto unavailable =
            preview_windows(coverage, input.checkpoint.utc->ms, input.restart.utc->ms, windows)) {
        return *unavailable;
    }
    merge_windows(windows);
    return windows;
}

Deadline deadline_status(const TimerConfig &config, const RecoveryUtcObservation &observation) {
    if (!config.hard_end_utc_ms) {
        return Deadline::Disabled;
    }
    if (!observation.utc) {
        return Deadline::Unknown;
    }
    return observation.utc->ms >= *config.hard_end_utc_ms ? Deadline::Reached
                                                          : Deadline::NotReached;
}

void advance(RecoveryTimerProposal &proposal, const TimerConfig &config, Instant utc,
             Instant elapsed, RecoveryTransitionBasis basis) {
    const auto next =
        advance_timer(proposal.timer, config, {elapsed, utc, !proposal.pause.reasons.empty()});
    if (next.transitions.size() > 4 - proposal.transitions.size()) {
        throw std::logic_error("Recovery lifecycle trace exceeds four entries");
    }
    proposal.timer = next.state;
    if (next.progressed_ms > 0) {
        // Count's positive cuts are bounded by a phase, schedule or deadline edge.
        proposal.progression.push_back({{utc - elapsed}, next.progressed_ms});
    }
    for (const auto transition : next.transitions) {
        proposal.transitions.push_back({transition, {utc}, basis});
    }
}

RecoveryTimerProposal observe_hard_end(const TimerState &timer, const PauseState &pause,
                                       const TimerConfig &config,
                                       const RecoveryUtcObservation &observation,
                                       Deadline deadline) {
    RecoveryTimerProposal result{timer, pause, {}};
    if (timer.lifecycle == TimerLifecycle::Active && deadline == Deadline::Reached) {
        advance(result, config, observation.utc->ms, 0, RecoveryTransitionBasis::ObservedUtc);
    }
    return result;
}

// Cut at the next phase boundary before passing elapsed to CORE-002, so every
// transition in its returned batch occurred at the same UTC millisecond.
void advance_interval(RecoveryTimerProposal &proposal, const TimerConfig &config, Instant from,
                      Instant to) {
    while (from < to && proposal.timer.lifecycle == TimerLifecycle::Active &&
           !proposal.timer.awaiting_confirmation && proposal.pause.reasons.empty()) {
        const auto remaining = proposal.timer.phase == CountdownPhase::Main
                                   ? proposal.timer.main_remaining_ms
                                   : proposal.timer.grace_remaining_ms;
        const auto elapsed = std::min(to - from, remaining);
        from += elapsed;
        advance(proposal, config, from, elapsed, RecoveryTransitionBasis::ReconstructedUtc);
    }
}

RecoveryTimerProposal count_elapsed(const DowntimeRecoveryInput &input, const Windows &windows) {
    RecoveryTimerProposal result{input.timer, input.pause, {}};
    if (result.timer.lifecycle != TimerLifecycle::Active) {
        return result;
    }
    auto cursor = input.checkpoint.utc->ms;
    const auto restart = input.restart.utc->ms;
    const auto deadline = input.config.hard_end_utc_ms;
    const bool reached_at_checkpoint = deadline && *deadline <= cursor;
    advance(result, input.config, cursor, 0,
            reached_at_checkpoint ? RecoveryTransitionBasis::ObservedUtc
                                  : RecoveryTransitionBasis::ReconstructedUtc);

    const bool reached_offline = deadline && cursor < *deadline && *deadline <= restart;
    // A later deadline wins a tied natural expiry. Process edges through D-1,
    // then observe D with no debit and without applying its schedule edge.
    const auto through = reached_offline ? *deadline - 1 : restart;
    const auto edge = [&](Instant at, bool paused) {
        if (at > through || result.timer.lifecycle != TimerLifecycle::Active) {
            return;
        }
        advance_interval(result, input.config, cursor, at);
        cursor = at;
        if (result.timer.lifecycle == TimerLifecycle::Active) {
            result.pause = set_pause_reason(result.pause, schedule, paused);
        }
    };
    for (const auto &[start, end] : windows) {
        if (result.timer.lifecycle != TimerLifecycle::Active || start > through) {
            break;
        }
        if (start > cursor) {
            edge(start, true);
        }
        if (end > cursor) {
            edge(end, false);
        }
    }
    advance_interval(result, input.config, cursor, through);
    if (reached_offline && result.timer.lifecycle == TimerLifecycle::Active) {
        advance(result, input.config, *deadline, 0,
                *deadline == restart ? RecoveryTransitionBasis::ObservedUtc
                                     : RecoveryTransitionBasis::ReconstructedUtc);
    }
    return result;
}

DowntimeRecoveryPreview make_preview(const DowntimeRecoveryInput &input) {
    const auto deadline = deadline_status(input.config, input.restart);
    auto freeze = observe_hard_end(input.timer, input.pause, input.config, input.restart, deadline);
    const auto reconstructed = reconstruct_schedule(input);
    const auto *gap = std::get_if<RecoveryGapUnavailable>(&input.gap);
    DowntimeRecoveryPreview result{input.basis,
                                   input.policy,
                                   std::move(freeze),
                                   {},
                                   {},
                                   deadline,
                                   gap ? std::optional{*gap} : std::nullopt,
                                   {}};
    if (const auto *windows = std::get_if<Windows>(&reconstructed)) {
        if (result.freeze.timer.lifecycle == TimerLifecycle::Active) {
            result.freeze.pause =
                reconcile_scheduled_pause(input.pause, *windows, input.restart.utc->ms);
        }
        if (!gap) {
            result.count_elapsed = count_elapsed(input, *windows);
        }
    } else {
        result.schedule_unavailable = std::get<ScheduleUnavailable>(reconstructed);
    }
    if (input.policy == DowntimePolicy::Review ||
        (input.policy == DowntimePolicy::CountElapsed && !result.count_elapsed)) {
        result.review_hold = result.freeze;
        result.review_hold->pause = set_pause_reason(result.freeze.pause, review, true);
    }
    return result;
}

bool same_correction(const std::optional<TimerSignedDelta> &left,
                     const std::optional<TimerSignedDelta> &right) {
    return left.has_value() == right.has_value() &&
           (!left || (left->requested_ms == right->requested_ms &&
                      left->negative_policy == right->negative_policy));
}

bool bound_to_review(const DowntimeRecoveryInput &input, const RecoveryReviewSnapshot &snapshot,
                     const RecoveryReviewCommand &command,
                     const RecoveryReviewAuthorization &authorization) {
    const auto &echo = authorization.command;
    return command.operation_ref == echo.operation_ref &&
           command.expected_episode_ref == echo.expected_episode_ref &&
           command.expected_revision == echo.expected_revision &&
           command.expected_facts_bundle_ref == echo.expected_facts_bundle_ref &&
           command.choice == echo.choice && command.reason == echo.reason &&
           same_correction(command.correction, echo.correction) &&
           authorization.revision == snapshot.current.revision &&
           authorization.observation_ref == snapshot.observed_at.observation_ref &&
           command.expected_episode_ref == snapshot.episode_ref &&
           command.expected_revision == snapshot.current.revision &&
           command.expected_facts_bundle_ref == snapshot.facts_bundle_ref &&
           snapshot.facts_bundle_ref == input.basis.facts_bundle_ref &&
           snapshot.timer_lineage == input.basis.timer_lineage;
}

bool consistent_hold(const TimerState &current, const TimerState &original) {
    // An authoritative finish after the provisional hold is terminal, including
    // when the current clock can no longer establish its original deadline.
    return current.lifecycle == TimerLifecycle::Finished ||
           (current.lifecycle == original.lifecycle && current.phase == original.phase &&
            current.main_remaining_ms == original.main_remaining_ms &&
            current.grace_remaining_ms == original.grace_remaining_ms &&
            current.awaiting_confirmation == original.awaiting_confirmation);
}

} // namespace

std::variant<DowntimeRecoveryPreview, RecoveryPreviewRefusal>
preview_downtime_recovery(const DowntimeRecoveryInput &input) {
    try {
        InputBudget budget;
        charge(budget, input);
        validate(input);
        if (input.pause.reasons.contains(review)) {
            return RecoveryPreviewRefusal::ReviewAlreadyPending;
        }
        return make_preview(input);
    } catch (const InputLimit &) {
        return RecoveryPreviewRefusal::InputLimit;
    } catch (const std::invalid_argument &) {
        return RecoveryPreviewRefusal::InvalidInput;
    }
}

std::variant<RecoveryReviewWait, RecoveryWaitRefusal>
observe_recovery_review_wait(const RecoveryReviewSnapshot &snapshot) {
    try {
        InputBudget budget;
        charge(budget, snapshot);
        validate(snapshot);
        if (!snapshot.pause.reasons.contains(review)) {
            return RecoveryWaitRefusal::NoReviewPending;
        }
        const auto &values = snapshot.current.values;
        const auto deadline = deadline_status(values.config, snapshot.observed_at);
        return RecoveryReviewWait{observe_hard_end(values.timer, snapshot.pause, values.config,
                                                   snapshot.observed_at, deadline),
                                  deadline};
    } catch (const InputLimit &) {
        return RecoveryWaitRefusal::InputLimit;
    } catch (const std::invalid_argument &) {
        return RecoveryWaitRefusal::InvalidInput;
    }
}

std::variant<RecoveryReviewResolution, RecoveryReviewRefusal, RecoveryCorrectionRefusal>
resolve_recovery_review(const DowntimeRecoveryInput &input, const RecoveryReviewSnapshot &snapshot,
                        const RecoveryReviewCommand &command, const RecoveryReviewAccess &access) {
    using Refusal = RecoveryReviewRefusal;
    try {
        InputBudget budget;
        charge(budget, input);
        charge(budget, snapshot);
        charge(budget, command);
        charge(budget, access);
        validate(input);
        validate(snapshot);
        validate(command);
        validate(access);

        if (const auto *refusal = std::get_if<RecoveryReviewAccessRefusal>(&access)) {
            return *refusal == RecoveryReviewAccessRefusal::Denied ? Refusal::AccessDenied
                                                                   : Refusal::AccessUnknown;
        }
        const auto &authorization = std::get<RecoveryReviewAuthorization>(access);
        if (!bound_to_review(input, snapshot, command, authorization) ||
            input.pause.reasons.contains(review)) {
            return Refusal::StaleReview;
        }
        if (!snapshot.pause.reasons.contains(review)) {
            return Refusal::NoReviewPending;
        }
        auto preview = make_preview(input);
        auto values = snapshot.current.values;
        if (!preview.review_hold || !consistent_hold(values.timer, preview.review_hold->timer)) {
            return Refusal::StaleReview;
        }
        if (values.timer.lifecycle == TimerLifecycle::Active) {
            const auto deadline = deadline_status(values.config, snapshot.observed_at);
            if (deadline == Deadline::Reached) {
                return Refusal::WaitObservationRequired;
            }
            if (deadline == Deadline::Unknown) {
                return Refusal::DeadlineUnknown;
            }
        }

        if (!command.choice ||
            (*command.choice == RecoveryOfflineChoice::CountElapsed && !preview.count_elapsed)) {
            return Refusal::ChoiceUnavailable;
        }
        auto selected = *command.choice == RecoveryOfflineChoice::Freeze
                            ? std::move(preview.freeze)
                            : std::move(*preview.count_elapsed);
        if (values.timer.lifecycle == TimerLifecycle::Active) {
            // The captured end mode can produce Confirmation under a current
            // configuration that no longer permits it. Preserve the hold.
            if (!valid_timer(selected.timer, values.config)) {
                return Refusal::ChoiceUnavailable;
            }
            values.timer = selected.timer;
        }

        auto pause = set_pause_reason(snapshot.pause, review, false);
        std::optional<TimerEditorialAdjustment> adjustment;
        TimerEditorialTransitionBatch transitions;
        if (command.correction) {
            if (!snapshot.observed_at.utc || !authorization.correction_access ||
                values.timer.lifecycle != TimerLifecycle::Active) {
                return Refusal::CorrectionUnavailable;
            }
            const auto utc = snapshot.observed_at.utc->ms;
            const auto correction = project_timer_editorial_command({
                {snapshot.current.revision, values},
                {command.operation_ref, command.expected_revision, authorization.actor_ref,
                 command.reason, *command.correction},
                *authorization.correction_access,
                {{0, utc, !pause.reasons.empty()}},
                utc,
            });
            const auto &editorial = correction.data();
            if (editorial.disposition != TimerEditorialDisposition::accepted) {
                return RecoveryCorrectionRefusal{editorial.disposition};
            }
            values = *editorial.accepted_after;
            adjustment = editorial.adjustment;
            transitions = editorial.editorial_transitions;
        }
        return RecoveryReviewResolution{
            command, authorization,    input.basis, snapshot.observed_at, std::move(selected),
            values,  std::move(pause), adjustment,  transitions};
    } catch (const InputLimit &) {
        return Refusal::InputLimit;
    } catch (const std::invalid_argument &) {
        return Refusal::InvalidInput;
    }
}

} // namespace subathon::core
