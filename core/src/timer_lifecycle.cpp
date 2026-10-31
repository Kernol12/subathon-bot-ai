// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/timer_lifecycle.hpp>

#include <limits>

namespace subathon::core {
namespace {
// All proposal boundaries reject incoherent snapshots rather than repairing them.
void validate_state(const TimerState &state) {
    if (state.main_remaining_ms < 0 || state.grace_remaining_ms < 0) {
        throw std::invalid_argument("Timer remaining durations must be nonnegative");
    }
    if (state.phase == CountdownPhase::Grace && state.main_remaining_ms != 0) {
        throw std::invalid_argument("Grace requires zero Main remaining");
    }
    if (state.awaiting_confirmation &&
        (state.lifecycle != TimerLifecycle::Active || state.phase != CountdownPhase::Main ||
         state.main_remaining_ms != 0)) {
        throw std::invalid_argument("Confirmation wait requires Active Main at zero");
    }
}
} // namespace

TimerAdvance advance_timer(const TimerState &state, const TimerConfig &config,
                           const TimerClockStep &step) {
    validate_state(state);
    if (config.grace_duration_ms < 0 || step.elapsed_ms < 0) {
        throw std::invalid_argument("Grace and elapsed durations must be nonnegative");
    }
    if (state.awaiting_confirmation && config.end_mode != TimerEndMode::Confirmation) {
        throw std::invalid_argument("Confirmation wait requires Confirmation mode");
    }
    if (state.lifecycle == TimerLifecycle::Ready) {
        throw std::invalid_argument("Timer advance requires an already-started state");
    }

    TimerAdvance result{state, {}};
    auto &next = result.state;
    auto &transitions = result.transitions;
    if (next.lifecycle == TimerLifecycle::Finished) {
        return result;
    }
    // UTC decides only deadline priority; retain the pre-call countdown fields.
    if (config.hard_end_utc_ms && step.utc_now_ms >= *config.hard_end_utc_ms) {
        next.lifecycle = TimerLifecycle::Finished;
        next.awaiting_confirmation = false;
        transitions = {TimerTransition::HardEndReached, TimerTransition::Finished};
        return result;
    }
    if (next.awaiting_confirmation) {
        return result;
    }

    // A paused interval consumes nothing, but an already-zero phase still expires.
    auto elapsed_ms = step.paused ? std::int64_t{0} : step.elapsed_ms;
    if (next.phase == CountdownPhase::Main) {
        if (elapsed_ms < next.main_remaining_ms) {
            next.main_remaining_ms -= elapsed_ms;
            return result;
        }
        elapsed_ms -= next.main_remaining_ms;
        next.main_remaining_ms = 0;
        transitions.push_back(TimerTransition::MainExpired);
        switch (config.end_mode) {
        case TimerEndMode::Immediate:
            next.lifecycle = TimerLifecycle::Finished;
            transitions.push_back(TimerTransition::Finished);
            return result;
        case TimerEndMode::Confirmation:
            next.awaiting_confirmation = true;
            transitions.push_back(TimerTransition::ConfirmationRequired);
            return result;
        case TimerEndMode::Grace:
            next.phase = CountdownPhase::Grace;
            if (config.grace_reuse == GraceReuse::Fresh) {
                next.grace_remaining_ms = config.grace_duration_ms;
            }
            transitions.push_back(TimerTransition::GraceStarted);
            break;
        }
    }

    // Carry only Main's unconsumed overshoot; never sum Main and Grace durations.
    if (elapsed_ms < next.grace_remaining_ms) {
        next.grace_remaining_ms -= elapsed_ms;
        return result;
    }
    next.grace_remaining_ms = 0;
    next.lifecycle = TimerLifecycle::Finished;
    transitions.push_back(TimerTransition::GraceExpired);
    transitions.push_back(TimerTransition::Finished);
    return result;
}

TimerState apply_applicable_time_credit(const TimerState &state, std::int64_t actual_ms) {
    validate_state(state);
    if (actual_ms < 0 || state.lifecycle != TimerLifecycle::Active) {
        throw std::invalid_argument(
            "Applicable credit requires an Active timer and nonnegative time");
    }
    if (actual_ms == 0) {
        return state;
    }
    if (actual_ms > std::numeric_limits<std::int64_t>::max() - state.main_remaining_ms) {
        throw std::overflow_error("Applicable credit exceeds Main remaining range");
    }
    auto next = state;
    next.main_remaining_ms += actual_ms;
    next.phase = CountdownPhase::Main;
    next.awaiting_confirmation = false;
    return next;
}

TimerState confirm_timer_finish(const TimerState &state) {
    validate_state(state);
    if (!state.awaiting_confirmation) {
        throw std::invalid_argument("Finish confirmation requires an awaiting timer");
    }
    auto next = state;
    next.lifecycle = TimerLifecycle::Finished;
    next.awaiting_confirmation = false;
    return next;
}
} // namespace subathon::core
