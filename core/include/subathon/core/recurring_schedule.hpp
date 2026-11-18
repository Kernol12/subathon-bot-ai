// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include <subathon/core/pause_policy.hpp>

namespace subathon::core {

enum class ScheduleGapPolicy { ShiftForward, Skip };
enum class ScheduleFoldPolicy { RunOnce, RunTwice };

struct RecurringSchedule {
    std::chrono::milliseconds local_time{};
    // Monday through Sunday, all enabled by default.
    std::array<bool, 7> weekdays{true, true, true, true, true, true, true};
    ScheduleGapPolicy gap_policy{ScheduleGapPolicy::ShiftForward};
    ScheduleFoldPolicy fold_policy{ScheduleFoldPolicy::RunOnce};
};

struct ScheduleZoneTransition {
    std::int64_t at_utc_ms{};
    std::int32_t offset_after_seconds{};
};

// Complete offset facts for the caller-selected subathon timezone over the closed
// UTC coverage interval. Rows are strictly ordered, after the coverage start and
// through its end. Offsets are seconds east of UTC, with magnitude at most 24 hours.
struct ScheduleZoneRules {
    std::int64_t coverage_start_utc_ms{};
    std::int64_t coverage_end_utc_ms{};
    std::int32_t offset_at_start_seconds{};
    std::span<const ScheduleZoneTransition> transitions{};
};

inline constexpr std::chrono::days schedule_preview_max_span{366};
inline constexpr std::size_t schedule_zone_max_transitions{64};

struct SchedulePreviewRange {
    std::int64_t after_utc_ms{};
    std::int64_t through_utc_ms{};
};

class RecurringScheduleNotImplemented final : public std::logic_error {
  public:
    RecurringScheduleNotImplemented()
        : std::logic_error(
              "Recurring schedule is intentionally unimplemented; review tests first") {}
};

// Preview in UTC order over (after, through], using local weekdays before any
// gap shift. RunOnce selects the same fold candidate regardless of preview range.
// Preview is a pure query. When previewing a timezone edit, use the authoritative
// observation as the exclusive lower bound.
// Throws std::invalid_argument for invalid local times, malformed or incomplete
// zone facts, reversed/excessive ranges, or unrepresentable range padding. Zone
// coverage must extend 48 hours to either side of the range to compare all
// candidate instants, even when a gap or fold straddles a range boundary.
[[nodiscard]] std::vector<std::int64_t>
preview_schedule_occurrences(const RecurringSchedule &schedule, const ScheduleZoneRules &zone,
                             SchedulePreviewRange range);

struct ScheduledPauseWindow {
    std::int64_t start_utc_ms{};
    std::int64_t end_utc_ms{};
};

// Reconcile the Schedule reason from current-configuration UTC windows. A window
// is active over [start, end); overlapping and duplicate windows have set
// semantics. Preserve all other PauseState fields. Throws std::invalid_argument
// if any window has start >= end. Callers supply the complete relevant windows,
// own clock segmentation, and decide how configured pause ends are produced.
[[nodiscard]] PauseState reconcile_scheduled_pause(const PauseState &state,
                                                   std::span<const ScheduledPauseWindow> windows,
                                                   std::int64_t now_utc_ms);

} // namespace subathon::core
