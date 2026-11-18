// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/recurring_schedule.hpp>

namespace subathon::core {

std::vector<std::int64_t> preview_schedule_occurrences(const RecurringSchedule &,
                                                       const ScheduleZoneRules &,
                                                       SchedulePreviewRange) {
    throw RecurringScheduleNotImplemented{};
}

PauseState reconcile_scheduled_pause(const PauseState &, std::span<const ScheduledPauseWindow>,
                                     std::int64_t) {
    throw RecurringScheduleNotImplemented{};
}

} // namespace subathon::core
