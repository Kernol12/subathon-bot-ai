// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/schedule_command.hpp>

namespace subathon::server {

// Stage 1 link seam for the opt-in tests.
std::variant<ScheduleIssuedRef, ScheduleRefusal>
issue_schedule_preview(const ScheduleCommandRef &, SchedulePorts &) {
    return ScheduleRefusal::not_admitted;
}

std::variant<SchedulePublication, ScheduleRefusal>
present_schedule_preview(const ScheduleCommandRef &, const ScheduleIssuedRef &, SchedulePorts &) {
    return ScheduleRefusal::not_admitted;
}

ScheduleApply apply_schedule_save(const ScheduleCommandRef &, SchedulePorts &) {
    return ScheduleRefusal::not_admitted;
}

ScheduleApply apply_schedule_entry(ScheduleCommandKind, const ScheduleCommandRef &, SchedulePorts &) {
    return ScheduleRefusal::not_admitted;
}

std::optional<ScheduledPauseCoverageNotice> disclose_schedule_notice(const ScheduleContext &,
                                                                     SchedulePorts &) {
    return {};
}

} // namespace subathon::server
