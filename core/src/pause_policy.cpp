// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/pause_policy.hpp>

namespace subathon::core {
namespace {
void validate_pending(const PauseState &state, const std::optional<std::int64_t> &pending,
                      PauseReason reason) {
    if (pending && (*pending < 0 || state.reasons.contains(reason))) {
        throw std::invalid_argument("Pending pause duration is negative or already active");
    }
}

void validate_state(const PauseState &state) {
    validate_pending(state, state.stream_offline_pending_ms, PauseReason::StreamOffline);
    validate_pending(state, state.client_disconnect_pending_ms,
                     PauseReason::DesignatedClientDisconnected);
    validate_pending(state, state.stream_unknown_pending_ms, PauseReason::StreamStatusUnknown);
    if (state.stream_unknown_pending_ms && !state.stream_unknown_notice) {
        throw std::invalid_argument("Pending unknown timeout requires an uncertainty notice");
    }
}

void validate_delay(std::int64_t delay, const std::optional<std::int64_t> &pending) {
    if (delay < 0 || (pending && *pending >= delay)) {
        throw std::invalid_argument("Invalid pause delay or expired pending duration");
    }
}

void clear_signal(PauseState &state, std::optional<std::int64_t> &pending, PauseReason reason) {
    state.reasons.erase(reason);
    pending.reset();
}

// Compare the remaining delay before addition so even signed64 maximum durations are safe.
void advance_signal(PauseState &state, std::optional<std::int64_t> &pending, PauseReason reason,
                    std::int64_t delay, std::int64_t elapsed) {
    if (state.reasons.contains(reason)) {
        return;
    }
    const auto accumulated = pending.value_or(0);
    if (elapsed >= delay - accumulated) {
        state.reasons.insert(reason);
        pending.reset();
    } else {
        pending = accumulated + elapsed;
    }
}
} // namespace

PauseState set_pause_reason(const PauseState &state, PauseReason reason, bool active) {
    validate_state(state);
    switch (reason) {
    case PauseReason::Manual:
    case PauseReason::Schedule:
    case PauseReason::RecoveryReview:
        break;
    default:
        throw std::invalid_argument("Only externally managed pause reasons can be set");
    }
    auto proposal = state;
    if (active) {
        proposal.reasons.insert(reason);
    } else {
        proposal.reasons.erase(reason);
    }
    return proposal;
}

PauseState observe_pause_signals(const PauseState &state, const PauseConfig &config,
                                 const PauseObservation &observation) {
    validate_state(state);
    validate_delay(config.stream_offline_delay_ms, state.stream_offline_pending_ms);
    validate_delay(config.client_disconnect_delay_ms, state.client_disconnect_pending_ms);
    if (config.stream_unknown_timeout_ms) {
        validate_delay(*config.stream_unknown_timeout_ms, state.stream_unknown_pending_ms);
    } else if (state.stream_unknown_pending_ms) {
        throw std::invalid_argument("Pending unknown timeout requires an enabled timeout");
    }
    if (observation.elapsed_ms < 0) {
        throw std::invalid_argument("Observation elapsed duration must be nonnegative");
    }

    auto proposal = state;
    if (observation.stream_status == StreamStatus::Unknown) {
        proposal.stream_unknown_notice = true;
        proposal.stream_offline_pending_ms.reset();
        if (config.stream_unknown_timeout_ms) {
            advance_signal(proposal, proposal.stream_unknown_pending_ms,
                           PauseReason::StreamStatusUnknown, *config.stream_unknown_timeout_ms,
                           observation.elapsed_ms);
        }
    } else {
        proposal.stream_unknown_notice = false;
        clear_signal(proposal, proposal.stream_unknown_pending_ms,
                     PauseReason::StreamStatusUnknown);
        if (observation.stream_status == StreamStatus::Online) {
            clear_signal(proposal, proposal.stream_offline_pending_ms, PauseReason::StreamOffline);
        } else {
            advance_signal(proposal, proposal.stream_offline_pending_ms, PauseReason::StreamOffline,
                           config.stream_offline_delay_ms, observation.elapsed_ms);
        }
    }

    if (observation.designated_client_connected) {
        clear_signal(proposal, proposal.client_disconnect_pending_ms,
                     PauseReason::DesignatedClientDisconnected);
    } else {
        advance_signal(proposal, proposal.client_disconnect_pending_ms,
                       PauseReason::DesignatedClientDisconnected, config.client_disconnect_delay_ms,
                       observation.elapsed_ms);
    }
    return proposal;
}
} // namespace subathon::core
