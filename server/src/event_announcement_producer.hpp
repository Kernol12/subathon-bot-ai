// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <span>
#include <string_view>
#include <subathon/server/event_announcement.hpp>

namespace subathon::server::event_announcement_owner {

// Private Stage 1 entry points. Facts represent observations of the serialized
// owner, scoped to its installation. Tests supply synthetic facts at this cut.
enum class ApplicationCompletion { staged, rolled_back, unknown, abnormal, committed };
enum class Cause {
    event_announcement,
    silent_acceptance,
    plan,
    simulation,
    queued_job,
    delivered_job
};
enum class ActionAuthority {
    current,
    use_withdrawn,
    grant_withdrawn,
    execution_gate_unavailable,
    stale_checked_snapshot,
    definition_missing
};

enum class DurationDeclaration { omitted, explicit_null, out_of_range };
struct AuthoredVersion {
    std::string_view version_ref;
    std::variant<std::chrono::milliseconds, DurationDeclaration> duration;
};

struct Admission {
    EventAnnouncementAction action;
    std::optional<AuthoredVersion> version;
    Cause cause;
    ApplicationCompletion completion;
    ActionAuthority authority;
};

struct StartFact {
    EventAnnouncementAction action;
    ActionAuthority authority;
};

struct ClockCut {
    std::string_view lifetime;
    std::optional<std::chrono::milliseconds> elapsed_sample;
};

struct OwnerTurn {
    ClockCut clock;
    std::span<const StartFact> start_facts;
};

enum class Disclosure { allowed, denied };
enum class SelectionEvidence { complete, unavailable };
enum class CommandAuthority { current, withdrawn, unavailable };

struct ManagementCommit {
    CommandAuthority authority;
    ApplicationCompletion completion;
    OwnerTurn turn;
};

enum class RecoveryDisposition { never_shown, shown, uncertain };
struct RetainedAnnouncement {
    EventAnnouncementAction action;
    std::chrono::milliseconds duration;
    RecoveryDisposition disposition;
};

class Producer {
  public:
    explicit Producer(std::string_view lifetime);

    // Facts are ordered as received from the owner. A single batch preserves
    // the selected order of actions in one application pass.
    void admit(std::span<const Admission> arrivals);
    void advance(OwnerTurn turn);
    void restart(std::string_view new_lifetime, std::span<const RetainedAnnouncement> arrivals);
    void resolve(const EventAnnouncementAction &action, RecoveryDisposition evidence);
    void cancel(CancelStuckEventAnnouncement request, ManagementCommit commit);
    void resume(ResumeEventAnnouncementQueue request, ManagementCommit commit);

    [[nodiscard]] CurrentEventAnnouncement
    current(ClockCut cut, Disclosure disclosure,
            SelectionEvidence selection = SelectionEvidence::complete) const;
    [[nodiscard]] EventAnnouncementManagement management(Disclosure disclosure) const;
};

} // namespace subathon::server::event_announcement_owner
