// SPDX-License-Identifier: GPL-3.0-or-later
#include "event_announcement_producer.hpp"

namespace subathon::server::event_announcement_owner {

// Stage 1 has no producer transitions or authoritative selection.
Producer::Producer(std::string_view) {}
void Producer::admit(std::span<const Admission>) {}
void Producer::advance(OwnerTurn) {}
void Producer::restart(std::string_view, std::span<const RetainedAnnouncement>) {}
void Producer::resolve(const EventAnnouncementAction &, RecoveryDisposition) {}
void Producer::cancel(CancelStuckEventAnnouncement, ManagementCommit) {}
void Producer::resume(ResumeEventAnnouncementQueue, ManagementCommit) {}

CurrentEventAnnouncement Producer::current(ClockCut, Disclosure disclosure,
                                           SelectionEvidence) const {
    if (disclosure == Disclosure::denied) {
        return EventAnnouncementUndisclosed{};
    }
    return EventAnnouncementUnavailable{};
}

EventAnnouncementManagement Producer::management(Disclosure disclosure) const {
    if (disclosure == Disclosure::denied) {
        return EventAnnouncementUndisclosed{};
    }
    return EventAnnouncementUnavailable{};
}

} // namespace subathon::server::event_announcement_owner
