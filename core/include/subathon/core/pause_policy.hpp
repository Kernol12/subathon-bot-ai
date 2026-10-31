// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>

namespace subathon::core {
enum class PauseReason {
    Manual,
    Schedule,
    StreamOffline,
    DesignatedClientDisconnected,
    RecoveryReview,
    StreamStatusUnknown
};
using PauseReasons = std::set<PauseReason>;
enum class StreamStatus { Online, Offline, Unknown };

struct PauseConfig {
    std::int64_t stream_offline_delay_ms{30'000};
    std::int64_t client_disconnect_delay_ms{30'000};
    std::optional<std::int64_t> stream_unknown_timeout_ms{};
};

// Pending durations never coexist with their own active reason. Empty reasons means
// this policy imposes no countdown pause; lifecycle and phase remain caller-owned.
struct PauseState {
    PauseReasons reasons{};
    std::optional<std::int64_t> stream_offline_pending_ms{};
    std::optional<std::int64_t> client_disconnect_pending_ms{};
    std::optional<std::int64_t> stream_unknown_pending_ms{};
    bool stream_unknown_notice{};
};

// One supplied, constant signal segment; elapsed_ms is monotonic duration, not UTC.
// Callers split at signal changes and must not retroactively pause an entire segment
// merely because its delay expires at the segment's end.
struct PauseObservation {
    std::int64_t elapsed_ms{};
    StreamStatus stream_status{StreamStatus::Unknown};
    bool designated_client_connected{};
};

class PausePolicyNotImplemented final : public std::logic_error {
  public:
    PausePolicyNotImplemented()
        : std::logic_error("Pause policy is intentionally unimplemented; review tests first") {}
};

// Pure proposals only. Manual/Schedule/RecoveryReview are the externally managed
// reasons accepted here; automatic signal reasons belong to observe_pause_signals.
[[nodiscard]] PauseState set_pause_reason(const PauseState &state, PauseReason reason, bool active);
[[nodiscard]] PauseState observe_pause_signals(const PauseState &state, const PauseConfig &config,
                                               const PauseObservation &observation);
} // namespace subathon::core
