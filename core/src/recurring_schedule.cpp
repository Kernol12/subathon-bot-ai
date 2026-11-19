// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/recurring_schedule.hpp>

#include <algorithm>
#include <limits>
#include <optional>

namespace subathon::core {
namespace {

using namespace std::chrono;
using WideDays = duration<std::int64_t, days::period>;

constexpr auto day_ms = milliseconds{days{1}}.count();
constexpr auto padding_ms = 2 * day_ms;
constexpr auto max_span_ms = milliseconds{schedule_preview_max_span}.count();
constexpr auto min_ms = std::numeric_limits<std::int64_t>::min();
constexpr auto max_ms = std::numeric_limits<std::int64_t>::max();

std::int64_t checked_add(std::int64_t time, std::int64_t delta) {
    if ((delta > 0 && time > max_ms - delta) || (delta < 0 && time < min_ms - delta)) {
        throw std::invalid_argument("Schedule time is out of range");
    }
    return time + delta;
}

std::int64_t offset_ms(std::int32_t offset_seconds) {
    return milliseconds{seconds{offset_seconds}}.count();
}

std::int64_t validated_offset_ms(std::int32_t offset_seconds) {
    const auto offset = offset_ms(offset_seconds);
    if (offset < -day_ms || offset > day_ms) {
        throw std::invalid_argument("Schedule zone offset exceeds 24 hours");
    }
    return offset;
}

void validate_preview(const RecurringSchedule &schedule, const ScheduleZoneRules &zone,
                      SchedulePreviewRange range) {
    if (schedule.local_time < milliseconds::zero() || schedule.local_time >= days{1}) {
        throw std::invalid_argument("Schedule time must be within a local day");
    }
    if ((schedule.gap_policy != ScheduleGapPolicy::ShiftForward &&
         schedule.gap_policy != ScheduleGapPolicy::Skip) ||
        (schedule.fold_policy != ScheduleFoldPolicy::RunOnce &&
         schedule.fold_policy != ScheduleFoldPolicy::RunTwice)) {
        throw std::invalid_argument("Invalid schedule gap or fold policy");
    }

    if (range.through_utc_ms < range.after_utc_ms) {
        throw std::invalid_argument("Schedule preview range is reversed");
    }
    // After checking order, unsigned subtraction gives the exact nonnegative span.
    const auto span_ms = static_cast<std::uint64_t>(range.through_utc_ms) -
                         static_cast<std::uint64_t>(range.after_utc_ms);
    if (span_ms > static_cast<std::uint64_t>(max_span_ms)) {
        throw std::invalid_argument("Schedule preview range exceeds 366 days");
    }
    const auto padded_start = checked_add(range.after_utc_ms, -padding_ms);
    const auto padded_end = checked_add(range.through_utc_ms, padding_ms);
    if (zone.coverage_start_utc_ms > padded_start || zone.coverage_end_utc_ms < padded_end) {
        throw std::invalid_argument("Schedule preview requires 48 hours of zone padding");
    }
}

struct LocalInterval {
    std::int64_t first_ms;
    std::int64_t last_ms;
    std::int64_t offset_ms;

    bool contains(std::int64_t local_ms) const {
        return first_ms <= local_ms && local_ms <= last_ms;
    }
};

using LocalIntervals = std::array<LocalInterval, schedule_zone_max_transitions + 1>;

// Project each constant-offset segment in UTC order. Clip to representable local
// times so coverage itself can extend to either extreme of the UTC representation.
std::size_t build_local_intervals(const ScheduleZoneRules &zone, LocalIntervals &intervals) {
    if (zone.transitions.size() > schedule_zone_max_transitions) {
        throw std::invalid_argument("Too many schedule zone transitions");
    }

    std::size_t count = 0;
    const auto append = [&](std::int64_t first, std::int64_t last, std::int64_t offset) {
        if (offset < 0) {
            first = std::max(first, min_ms - offset);
        } else {
            last = std::min(last, max_ms - offset);
        }
        if (first <= last) {
            intervals[count++] = {first + offset, last + offset, offset};
        }
    };

    auto segment_start = zone.coverage_start_utc_ms;
    auto offset = validated_offset_ms(zone.offset_at_start_seconds);
    for (const auto &transition : zone.transitions) {
        if (transition.at_utc_ms <= segment_start ||
            transition.at_utc_ms > zone.coverage_end_utc_ms) {
            throw std::invalid_argument(
                "Schedule zone transitions are unordered or out of coverage");
        }
        append(segment_start, transition.at_utc_ms - 1, offset);
        segment_start = transition.at_utc_ms;
        offset = validated_offset_ms(transition.offset_after_seconds);
    }
    append(segment_start, zone.coverage_end_utc_ms, offset);
    return count;
}

void validate_local_overlaps(std::span<const LocalInterval> intervals) {
    // Any triple overlap includes the first millisecond of at least one interval.
    for (const auto &interval : intervals) {
        const auto mappings = std::ranges::count_if(
            intervals, [&](const auto &other) { return other.contains(interval.first_ms); });
        if (mappings > 2) {
            throw std::invalid_argument(
                "Schedule zone maps a local time to more than two instants");
        }
    }
}

ScheduleLocalTime project_local(const ScheduleZoneRules &zone, std::int64_t utc_ms) {
    const auto next =
        std::ranges::upper_bound(zone.transitions, utc_ms, {}, &ScheduleZoneTransition::at_utc_ms);
    const auto offset = next == zone.transitions.begin() ? zone.offset_at_start_seconds
                                                         : (next - 1)->offset_after_seconds;
    return ScheduleLocalTime{milliseconds{checked_add(utc_ms, offset_ms(offset))}};
}

// A nominal time in a forward gap is at or after the transition under the old
// offset and strictly before it under the new offset. Resolve against the old
// offset. If gaps overlap, the last crossing keeps the resolved start forward:
// a later fall below the nominal time would require another crossing back up.
std::optional<std::int64_t> shifted_gap_start(const ScheduleZoneRules &zone,
                                              std::int64_t nominal_ms) {
    std::optional<std::int64_t> shifted;
    auto before = offset_ms(zone.offset_at_start_seconds);
    for (const auto &transition : zone.transitions) {
        const auto after = offset_ms(transition.offset_after_seconds);
        if (nominal_ms - before >= transition.at_utc_ms &&
            nominal_ms - after < transition.at_utc_ms) {
            shifted = nominal_ms - before;
        }
        before = after;
    }
    return shifted;
}

bool was_delivered(std::span<const ScheduleOccurrenceId> delivered, ScheduleOccurrenceId id,
                   bool two_runs) {
    return std::ranges::any_of(delivered, [&](const auto &previous) {
        return previous.nominal_local_start == id.nominal_local_start &&
               (!two_runs || previous.run == id.run);
    });
}

} // namespace

std::vector<ScheduleOccurrence>
preview_schedule_occurrences(const RecurringSchedule &schedule, const ScheduleZoneRules &zone,
                             SchedulePreviewRange range,
                             std::span<const ScheduleOccurrenceId> delivered) {
    validate_preview(schedule, zone, range);
    LocalIntervals storage{};
    const auto count = build_local_intervals(zone, storage);
    const std::span intervals{storage.data(), count};
    validate_local_overlaps(intervals);

    // A nominal slot uses an offset within one day of its UTC start, including
    // when a gap uses the pre-transition offset. The 48-hour padding leaves
    // room for both rounding to a whole day and inverse offset arithmetic.
    const auto after_nominal_ms = range.after_utc_ms - day_ms;
    const auto through_nominal_ms = range.through_utc_ms + day_ms;
    const auto first_day = floor<WideDays>(milliseconds{after_nominal_ms});
    const auto last_day = floor<WideDays>(milliseconds{through_nominal_ms});

    std::vector<ScheduleOccurrence> occurrences;
    for (auto day = first_day; day <= last_day; ++day) {
        // Reduce by whole weeks before constructing a chrono weekday so large
        // millisecond instants need no conversion to a bounded calendar year.
        const auto weekday_index = weekday{local_days{days{day.count() % 7}}}.iso_encoding() - 1;
        if (!schedule.weekdays[weekday_index]) {
            continue;
        }
        const auto nominal = ScheduleLocalTime{day + schedule.local_time};
        const auto nominal_ms = nominal.time_since_epoch().count();
        if (nominal_ms <= after_nominal_ms || nominal_ms > through_nominal_ms) {
            continue;
        }

        std::array<std::int64_t, 2> starts{};
        std::size_t runs = 0;
        for (const auto &interval : intervals) {
            if (interval.contains(nominal_ms)) {
                starts[runs++] = nominal_ms - interval.offset_ms;
            }
        }
        if (runs == 0 && schedule.gap_policy == ScheduleGapPolicy::ShiftForward) {
            if (const auto shifted = shifted_gap_start(zone, nominal_ms)) {
                starts[runs++] = *shifted;
            }
        }

        const bool two_runs = runs == 2 && schedule.fold_policy == ScheduleFoldPolicy::RunTwice;
        if (runs == 2 && !two_runs) {
            runs = 1;
        }
        for (std::size_t run = 0; run < runs; ++run) {
            const auto start = starts[run];
            const ScheduleOccurrenceId id{nominal, run == 0 ? ScheduleFoldRun::First
                                                            : ScheduleFoldRun::Second};
            if (start > range.after_utc_ms && start <= range.through_utc_ms &&
                !was_delivered(delivered, id, two_runs)) {
                occurrences.push_back({id, start, project_local(zone, start)});
            }
        }
    }

    // A gap shift can carry a slot into a later nominal day's UTC ordering.
    std::ranges::stable_sort(occurrences, {}, &ScheduleOccurrence::start_utc_ms);
    return occurrences;
}

std::vector<ScheduledPauseOccurrence>
preview_scheduled_pauses(const RecurringSchedule &schedule, const ScheduleZoneRules &zone,
                         SchedulePreviewRange range, std::chrono::milliseconds duration,
                         std::span<const ScheduleOccurrenceId> delivered) {
    if (duration <= milliseconds::zero()) {
        throw std::invalid_argument("Scheduled pause duration must be positive");
    }
    const auto latest_end = checked_add(range.through_utc_ms, duration.count());
    if (zone.coverage_end_utc_ms < latest_end) {
        throw std::invalid_argument("Schedule zone coverage ends before the latest pause end");
    }

    const auto starts = preview_schedule_occurrences(schedule, zone, range, delivered);
    std::vector<ScheduledPauseOccurrence> pauses;
    pauses.reserve(starts.size());
    for (const auto &start : starts) {
        const auto end = checked_add(start.start_utc_ms, duration.count());
        pauses.push_back({start, end, project_local(zone, end)});
    }
    return pauses;
}

PauseState reconcile_scheduled_pause(const PauseState &state,
                                     std::span<const ScheduledPauseWindow> windows,
                                     std::int64_t now_utc_ms) {
    bool active = false;
    for (const auto &window : windows) {
        if (window.start_utc_ms >= window.end_utc_ms) {
            throw std::invalid_argument("Scheduled pause end must follow its start");
        }
        active |= window.start_utc_ms <= now_utc_ms && now_utc_ms < window.end_utc_ms;
    }
    return set_pause_reason(state, PauseReason::Schedule, active);
}

} // namespace subathon::core
