// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "event_announcement_producer.hpp"

namespace subathon::server::event_announcement_owner {

// The checked component uses synthetic owner admission during the tests stage.
enum class CheckedManagementOutcome { applied, stale, refused, unavailable };

struct CheckedManagementTurn {
    bool owner_admitted;
    CommandAuthority command;
    CommandAuthority disclosure;
    OwnerTurn turn;
};

struct ObservedAnnouncementManagement {
    std::string owner_reference;
    EventAnnouncementManagement view;
};

class AnnouncementManagementCandidate {
  public:
    AnnouncementManagementCandidate(Producer &, std::string_view) {}

    void advance_and_capture(OwnerTurn) {}
    void invalidate_capture() {}

    ObservedAnnouncementManagement read(CommandAuthority) const {
        return {{}, EventAnnouncementUnavailable{}};
    }

    CheckedManagementOutcome cancel(std::string_view, CancelStuckEventAnnouncement,
                                    CheckedManagementTurn) {
        return CheckedManagementOutcome::unavailable;
    }

    CheckedManagementOutcome resume(std::string_view, ResumeEventAnnouncementQueue,
                                    CheckedManagementTurn) {
        return CheckedManagementOutcome::unavailable;
    }
};

} // namespace subathon::server::event_announcement_owner
