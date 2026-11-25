// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/recurring_schedule.hpp>

#include <algorithm>
#include <limits>
#include <set>
#include <tuple>
#include <utility>

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

enum class SegmentKind { inverse, gap };

// An end event is placed one millisecond past the closed interval, when that
// millisecond is representable. Events at the same instant take effect together.
struct LocalEvent {
    std::int64_t at;
    std::int64_t offset;
    std::size_t segment;
    SegmentKind kind;
    bool begins;
};

auto delivered_order(const ScheduleOccurrenceId &id) {
    return std::pair{id.nominal_local_start, id.run};
}

} // namespace

ScheduleTimeline::ScheduleTimeline(ScheduleZoneRules zone) : rules_(zone) {
    if (zone.transitions.size() > max_transitions ||
        zone.coverage_start_utc_ms > zone.coverage_end_utc_ms) {
        throw std::invalid_argument("Invalid schedule zone coverage");
    }

    std::vector<LocalEvent> events;
    events.reserve(4 * zone.transitions.size() + 2);
    const auto append = [&](std::int64_t first, std::int64_t last, std::int64_t offset,
                            std::size_t segment, SegmentKind kind) {
        events.push_back({first, offset, segment, kind, true});
        if (last < max_ms) {
            events.push_back({last + 1, offset, segment, kind, false});
        }
    };
    const auto inverse = [&](std::int64_t first, std::int64_t last, std::int64_t offset,
                             std::size_t segment) {
        first = std::max(first, min_ms - std::min(offset, std::int64_t{0}));
        last = std::min(last, max_ms - std::max(offset, std::int64_t{0}));
        if (first <= last) {
            append(first + offset, last + offset, offset, segment, SegmentKind::inverse);
        }
    };

    auto segment_start = zone.coverage_start_utc_ms;
    auto offset = validated_offset_ms(zone.offset_at_start_seconds);
    for (std::size_t i = 0; i < zone.transitions.size(); ++i) {
        const auto [change, new_offset] = zone.transitions[i];
        if (change <= segment_start || change > zone.coverage_end_utc_ms) {
            throw std::invalid_argument(
                "Schedule zone transitions are unordered or out of coverage");
        }
        const auto next_offset = validated_offset_ms(new_offset);
        inverse(segment_start, change - 1, offset, i);
        if (next_offset > offset) {
            // Intersect [change + offset, change + next_offset - 1] with
            // representable local times. The padding bounds gap inverses at query time.
            const auto last = change - 1;
            if (change <= max_ms - std::max(offset, std::int64_t{0}) &&
                last >= min_ms - std::min(next_offset, std::int64_t{0})) {
                const auto first_local =
                    change < min_ms - std::min(offset, std::int64_t{0}) ? min_ms : change + offset;
                const auto last_local = last > max_ms - std::max(next_offset, std::int64_t{0})
                                            ? max_ms
                                            : last + next_offset;
                append(first_local, last_local, offset, i, SegmentKind::gap);
            }
        }
        segment_start = change;
        offset = next_offset;
    }
    inverse(segment_start, zone.coverage_end_utc_ms, offset, zone.transitions.size());

    // Segment order is UTC order for inverses and crossing order for gaps.
    using Active = std::set<std::pair<std::size_t, std::int64_t>>;
    Active inverses;
    Active gaps;
    std::ranges::sort(events, {}, &LocalEvent::at);
    bands_.reserve(events.size());
    for (std::size_t first = 0; first < events.size();) {
        const auto at = events[first].at;
        do {
            const auto &event = events[first++];
            auto &active = event.kind == SegmentKind::inverse ? inverses : gaps;
            const auto entry = std::pair{event.segment, event.offset};
            if (event.begins) {
                active.insert(entry);
            } else {
                active.erase(entry);
            }
        } while (first < events.size() && events[first].at == at);

        if (inverses.size() > 2) {
            throw std::invalid_argument(
                "Schedule zone maps a local time to more than two instants");
        }
        LocalBand band{.first = at, .gap_offset = {}};
        for (const auto &entry : inverses) {
            band.offsets[band.inverse_count++] = entry.second;
        }
        if (!gaps.empty()) {
            band.gap_offset = gaps.rbegin()->second;
        }
        bands_.push_back(band);
    }
}

std::optional<ScheduleLocalTime> ScheduleTimeline::local_at(std::int64_t utc_ms) const {
    if (utc_ms < rules_.coverage_start_utc_ms || utc_ms > rules_.coverage_end_utc_ms) {
        return {};
    }
    const auto next = std::ranges::upper_bound(rules_.transitions, utc_ms, {},
                                               &ScheduleZoneTransition::at_utc_ms);
    const auto offset =
        offset_ms(next == rules_.transitions.begin() ? rules_.offset_at_start_seconds
                                                     : (next - 1)->offset_after_seconds);
    if (utc_ms < min_ms - std::min(offset, std::int64_t{0}) ||
        utc_ms > max_ms - std::max(offset, std::int64_t{0})) {
        return {};
    }
    return ScheduleLocalTime{milliseconds{utc_ms + offset}};
}

std::vector<ScheduleOccurrence>
ScheduleTimeline::starts(const RecurringSchedule &schedule, SchedulePreviewRange range,
                         std::span<const ScheduleOccurrenceId> delivered) const {
    validate_preview(schedule, rules_, range);

    std::vector<ScheduleOccurrenceId> sorted_deliveries;
    if (!std::ranges::is_sorted(delivered, {}, delivered_order)) {
        sorted_deliveries.assign(delivered.begin(), delivered.end());
        std::ranges::sort(sorted_deliveries, {}, delivered_order);
        delivered = sorted_deliveries;
    }
    // A nominal slot uses an offset within one day of its UTC start, including
    // when a gap uses the pre-transition offset. The 48-hour padding leaves
    // room for both rounding to a whole day and inverse offset arithmetic.
    const auto after_nominal_ms = range.after_utc_ms - day_ms;
    const auto through_nominal_ms = range.through_utc_ms + day_ms;
    const auto first_day = floor<WideDays>(milliseconds{after_nominal_ms});
    const auto last_day = floor<WideDays>(milliseconds{through_nominal_ms});

    std::vector<ScheduleOccurrence> occurrences;
    // At most 369 dates and two starts per date, including both shoulders.
    occurrences.reserve(2 * static_cast<std::size_t>((last_day - first_day).count() + 1));
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

        const auto next = std::ranges::upper_bound(bands_, nominal_ms, {}, &LocalBand::first);
        if (next == bands_.begin()) {
            continue;
        }
        const auto &band = *(next - 1);
        auto offsets = band.offsets;
        auto runs = band.inverse_count;
        if (runs == 0 && schedule.gap_policy == ScheduleGapPolicy::ShiftForward &&
            band.gap_offset) {
            offsets[runs++] = *band.gap_offset;
        }

        const bool two_runs = runs == 2 && schedule.fold_policy == ScheduleFoldPolicy::RunTwice;
        if (runs == 2 && !two_runs) {
            runs = 1;
        }
        for (std::size_t run = 0; run < runs; ++run) {
            const auto start = nominal_ms - offsets[run];
            const ScheduleOccurrenceId id{nominal, run == 0 ? ScheduleFoldRun::First
                                                            : ScheduleFoldRun::Second};
            const auto previous = std::ranges::lower_bound(
                delivered, delivered_order({nominal, two_runs ? id.run : ScheduleFoldRun::First}),
                {}, delivered_order);
            const bool was_delivered = previous != delivered.end() &&
                                       previous->nominal_local_start == nominal &&
                                       (!two_runs || previous->run == id.run);
            if (start > range.after_utc_ms && start <= range.through_utc_ms && !was_delivered) {
                // The range padding also makes every in-range UTC start representable locally.
                occurrences.push_back({id, start, local_at(start).value()});
            }
        }
    }

    // A gap shift can carry a slot into a later nominal day's UTC ordering.
    std::ranges::sort(occurrences, {}, [](const ScheduleOccurrence &occurrence) {
        return std::tuple{occurrence.start_utc_ms, occurrence.id.nominal_local_start,
                          occurrence.id.run};
    });
    return occurrences;
}

namespace {

ScheduleTimeline legacy_timeline(const ScheduleZoneRules &zone) {
    if (zone.transitions.size() > schedule_zone_max_transitions) {
        throw std::invalid_argument("Too many schedule zone transitions");
    }
    return ScheduleTimeline{zone};
}

} // namespace

std::vector<ScheduleOccurrence>
preview_schedule_occurrences(const RecurringSchedule &schedule, const ScheduleZoneRules &zone,
                             SchedulePreviewRange range,
                             std::span<const ScheduleOccurrenceId> delivered) {
    return legacy_timeline(zone).starts(schedule, range, delivered);
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

    const auto timeline = legacy_timeline(zone);
    const auto starts = timeline.starts(schedule, range, delivered);
    std::vector<ScheduledPauseOccurrence> pauses;
    pauses.reserve(starts.size());
    for (const auto &start : starts) {
        const auto end = checked_add(start.start_utc_ms, duration.count());
        const auto local_end = timeline.local_at(end);
        if (!local_end) {
            throw std::invalid_argument("Schedule time is out of range");
        }
        pauses.push_back({start, end, *local_end});
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
