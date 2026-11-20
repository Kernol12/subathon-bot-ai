// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdexcept>

#include <subathon/server/scheduled_pause.hpp>

namespace subathon::server {

std::variant<ScheduledPausePreview, ScheduledPauseRefusal>
project_scheduled_pause_edit(const ScheduledPauseState &, const ScheduledPauseClockInput &,
                             const ScheduledPauseConfiguration &, std::int64_t,
                             ScheduleZoneSource &) {
    throw std::logic_error("SCHEDULE-001 NI: project_scheduled_pause_edit");
}

std::variant<ScheduledPauseCutover, ScheduledPauseCoverageFailure, ScheduledPauseRefusal>
project_scheduled_pause_cutover(const ScheduledPauseState &, const ScheduledPauseClockInput &,
                                const ScheduledPauseConfiguration &, ScheduleZoneSource &) {
    throw std::logic_error("SCHEDULE-001 NI: project_scheduled_pause_cutover");
}

std::variant<ScheduledPauseProgress, ScheduledPauseRefusal>
enter_scheduled_pauses(const ScheduledPauseState &, const ScheduledPauseEntry &,
                       ScheduleZoneSource &) {
    throw std::logic_error("SCHEDULE-001 NI: enter_scheduled_pauses");
}

std::variant<ScheduledPauseProgress, ScheduledPauseRefusal>
progress_scheduled_pauses(const ScheduledPauseState &, const ScheduledPauseClockInput &,
                          ScheduleZoneSource &) {
    throw std::logic_error("SCHEDULE-001 NI: progress_scheduled_pauses");
}

} // namespace subathon::server
