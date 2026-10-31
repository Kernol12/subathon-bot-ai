// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace subathon::core {
enum class TimerLifecycle { Ready, Active, Finished };
enum class CountdownPhase { Main, Grace };
enum class TimerEndMode { Immediate, Grace, Confirmation };
enum class GraceReuse { Fresh, Shared };

struct TimerConfig {
    TimerEndMode end_mode{TimerEndMode::Grace};
    std::int64_t grace_duration_ms{120'000};
    GraceReuse grace_reuse{GraceReuse::Fresh};
    std::optional<std::int64_t> hard_end_utc_ms;
};

struct TimerState {
    TimerLifecycle lifecycle{TimerLifecycle::Ready};
    CountdownPhase phase{CountdownPhase::Main};
    std::int64_t main_remaining_ms{};
    // Retained during Main; reused on the next expiration only in Shared mode.
    std::int64_t grace_remaining_ms{120'000};
    bool awaiting_confirmation{};
};

// One already-segmented interval with an unchanged pause observation. elapsed_ms is
// nonnegative monotonic progression; utc_now_ms is a separate wall-clock instant.
struct TimerClockStep {
    std::int64_t elapsed_ms{};
    std::int64_t utc_now_ms{};
    bool paused{};
};

enum class TimerTransition {
    MainExpired,
    GraceStarted,
    GraceExpired,
    ConfirmationRequired,
    HardEndReached,
    Finished
};

struct TimerAdvance {
    TimerState state;
    // Ordered domain proposals, without invented UTC/monotonic event timestamps.
    std::vector<TimerTransition> transitions;
};

class TimerLifecycleNotImplemented final : public std::logic_error {
  public:
    TimerLifecycleNotImplemented()
        : std::logic_error("Timer lifecycle is intentionally unimplemented; review tests first") {}
};

// Pure proposals; callers own permission, eligibility, caps/debt and atomic commit.
// Advance to a command observation before submitting credit or confirmation.
// advance_timer accepts already-started Active/Finished states; Ready is not a start command.
[[nodiscard]] TimerAdvance advance_timer(const TimerState &state, const TimerConfig &config,
                                         const TimerClockStep &step);
[[nodiscard]] TimerState apply_applicable_time_credit(const TimerState &state,
                                                      std::int64_t actual_ms);
[[nodiscard]] TimerState confirm_timer_finish(const TimerState &state);
} // namespace subathon::core
