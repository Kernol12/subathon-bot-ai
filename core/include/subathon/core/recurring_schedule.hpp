// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

#include <subathon/core/pause_policy.hpp>

namespace subathon::core {

enum class ScheduleGapPolicy { ShiftForward, Skip };
enum class ScheduleFoldPolicy { RunOnce, RunTwice };
enum class ScheduleFoldRun { First, Second };

using ScheduleLocalTime = std::chrono::local_time<std::chrono::milliseconds>;

struct RecurringSchedule {
    // Milliseconds since local midnight in [0, 24h).
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
// through its end, with the new offset in force at the transition instant.
// A local time can map to at most two UTC instants. Offsets are seconds east of
// UTC, with magnitude at most 24 hours.
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

// Scoped to one caller-owned schedule identity. The nominal local date and time
// are taken before any gap shift. First denotes the sole or earlier run, and
// Second denotes the later RunTwice run. Neither key contains a zone or offset.
struct ScheduleOccurrenceId {
    ScheduleLocalTime nominal_local_start{};
    ScheduleFoldRun run{ScheduleFoldRun::First};

    bool operator==(const ScheduleOccurrenceId &) const = default;
};

struct ScheduleOccurrence {
    ScheduleOccurrenceId id{};
    std::int64_t start_utc_ms{};
    ScheduleLocalTime local_start{};

    bool operator==(const ScheduleOccurrence &) const = default;
};

struct ScheduledPauseOccurrence {
    ScheduleOccurrence start{};
    std::int64_t end_utc_ms{};
    ScheduleLocalTime local_end{};

    bool operator==(const ScheduledPauseOccurrence &) const = default;
};

// Indexed offset facts used by the bounded forecast and the legacy solvers.
// The transition storage must stay alive and immutable while the timeline is used.
class ScheduleTimeline {
  public:
    static constexpr std::size_t max_transitions = 4096;

    // Throws std::invalid_argument for malformed facts or a third local inverse.
    explicit ScheduleTimeline(ScheduleZoneRules);

    // Uses the same range and identity rules as preview_schedule_occurrences.
    [[nodiscard]] std::vector<ScheduleOccurrence>
    starts(const RecurringSchedule &, SchedulePreviewRange,
           std::span<const ScheduleOccurrenceId> delivered = {}) const;
    [[nodiscard]] std::optional<ScheduleLocalTime> local_at(std::int64_t utc_ms) const;

  private:
    struct LocalBand {
        std::int64_t first;
        std::array<std::int64_t, 2> offsets{};
        std::size_t inverse_count{};
        std::optional<std::int64_t> gap_offset;
    };

    ScheduleZoneRules rules_;
    std::vector<LocalBand> bands_;
};

class RecurringScheduleNotImplemented final : public std::logic_error {
  public:
    RecurringScheduleNotImplemented()
        : std::logic_error(
              "Recurring schedule is intentionally unimplemented; review tests first") {}
};

// Preview in UTC order over (after, through], using local weekdays before any
// gap shift. RunOnce selects the earlier UTC instant, even if only the later
// instant is within range. local_start is the actual local start after resolution.
// Preview is a pure query. Use the authoritative observation as the exclusive
// lower bound for timezone edits and pass previously delivered IDs for this
// schedule. For a slot with one run, any delivered ID for the nominal slot
// suppresses it. For a RunTwice slot with two runs, suppress each by its own ID.
// Assign run identities before filtering by range or delivery.
// Throws std::invalid_argument for invalid local times, malformed or incomplete
// zone facts, reversed/excessive ranges, or unrepresentable range padding or
// results. Zone coverage must extend 48 hours to either side of the range to
// compare all candidates, even when a gap or fold straddles a range boundary.
[[nodiscard]] std::vector<ScheduleOccurrence>
preview_schedule_occurrences(const RecurringSchedule &schedule, const ScheduleZoneRules &zone,
                             SchedulePreviewRange range,
                             std::span<const ScheduleOccurrenceId> delivered = {});

// Select starts as in preview_schedule_occurrences. Add a positive elapsed
// duration to each resolved UTC start and project the end into the same zone
// using the offset in force at that end. Ends may extend past the preview range.
// Also throws std::invalid_argument for a nonpositive duration, an unrepresentable
// through_utc_ms + duration, or zone coverage ending before that instant.
[[nodiscard]] std::vector<ScheduledPauseOccurrence>
preview_scheduled_pauses(const RecurringSchedule &schedule, const ScheduleZoneRules &zone,
                         SchedulePreviewRange range, std::chrono::milliseconds duration,
                         std::span<const ScheduleOccurrenceId> delivered = {});

struct ScheduledPauseWindow {
    std::int64_t start_utc_ms{};
    std::int64_t end_utc_ms{};
};

// Reconcile the Schedule reason from current-configuration UTC windows. A window
// is active over [start, end); overlapping and duplicate windows have set
// semantics. Preserve all other PauseState fields. Throws std::invalid_argument
// if any window has start >= end. Callers supply the complete relevant windows
// and own clock segmentation.
[[nodiscard]] PauseState reconcile_scheduled_pause(const PauseState &state,
                                                   std::span<const ScheduledPauseWindow> windows,
                                                   std::int64_t now_utc_ms);

} // namespace subathon::core
