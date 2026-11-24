// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <subathon/core/rule_actions.hpp>
#include <variant>

namespace subathon::server {

// Internal exact keys. Projection authorizes any fields it makes available.
struct EventAnnouncementAction {
    core::RuleEventLineage event;
    core::RuleActionKey action;
    std::string presentation_version_ref;

    bool operator==(const EventAnnouncementAction &) const = default;
};

// Issued by the authoritative owner for one activation. The reference is
// opaque. It must not be recycled for another activation of the same version.
struct EventAnnouncementOccurrence {
    std::string occurrence_ref;
    EventAnnouncementAction source;

    bool operator==(const EventAnnouncementOccurrence &) const = default;
};

// Owned value from one admitted selection and qualified owner-clock cut.
// Consumer resumes this exact occurrence at elapsed, never at a local origin.
struct ActiveEventAnnouncement {
    EventAnnouncementOccurrence occurrence;
    std::chrono::milliseconds elapsed;
    std::chrono::milliseconds duration;

    bool operator==(const ActiveEventAnnouncement &) const = default;
};

struct NoCurrentEventAnnouncement {};
struct EventAnnouncementUnavailable {};
// The projection returns this whenever disclosure is denied for the view,
// independently of whether an occurrence is current.
struct EventAnnouncementUndisclosed {};
using CurrentEventAnnouncement =
    std::variant<ActiveEventAnnouncement, NoCurrentEventAnnouncement,
                 EventAnnouncementUnavailable, EventAnnouncementUndisclosed>;

// Separate, authorized management projection for the exact waiting head.
struct StuckEventAnnouncement {
    EventAnnouncementAction head;
};

struct EventAnnouncementQueueHead {
    EventAnnouncementAction action;
    bool stuck;

    bool operator==(const EventAnnouncementQueueHead &) const = default;
};

// Operator Hold has an opaque owner-issued reference unique to this Hold.
struct HeldEventAnnouncementQueue {
    std::string hold_ref;
    std::optional<EventAnnouncementQueueHead> next_head;

    bool operator==(const HeldEventAnnouncementQueue &) const = default;
};

struct NoStuckEventAnnouncement {};
using EventAnnouncementManagement =
    std::variant<StuckEventAnnouncement, HeldEventAnnouncementQueue,
                 NoStuckEventAnnouncement, EventAnnouncementUnavailable,
                 EventAnnouncementUndisclosed>;

enum class EventAnnouncementAfterCancel { hold, continue_queue };

// The command owner checks current authority and the exact stuck head in
// the admitted queue state before committing this request, including during
// operator Hold. Hold keeps an existing Hold. Continue clears it.
struct CancelStuckEventAnnouncement {
    EventAnnouncementAction expected_head;
    EventAnnouncementAfterCancel after_cancel{EventAnnouncementAfterCancel::hold};
};

// Resume requires current authority, the same Hold, and the same next head
// or empty queue. Other arrivals behind that head leave the request valid.
struct ResumeEventAnnouncementQueue {
    std::string expected_hold_ref;
    std::optional<EventAnnouncementAction> expected_next_head;
};

// The command owner binds admission, arrival order, immutable timing and
// recovery evidence. These read values cannot admit or start an announcement.
// The scene selects exactly one of the event and milestone owner views.
} // namespace subathon::server
