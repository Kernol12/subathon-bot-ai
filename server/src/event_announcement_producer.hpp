// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <set>
#include <span>
#include <string_view>
#include <subathon/server/event_announcement.hpp>

namespace subathon::server::event_announcement_owner {

// Private producer entry points. Facts represent observations of the serialized
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

class AnnouncementManagement;

class Producer {
  public:
    explicit Producer(std::string_view lifetime);
    Producer(const Producer &) = delete;
    Producer &operator=(const Producer &) = delete;

    // The owner supplies validated installation-scoped identities in arrival
    // order. A single batch preserves the selected order in one application pass.
    void admit(std::span<const Admission> arrivals);
    void advance(OwnerTurn turn);
    // The owner supplies a fresh lifetime and retained evidence in original
    // arrival order. The lifetime is an opaque, non-sensitive namespace.
    void restart(std::string_view new_lifetime, std::span<const RetainedAnnouncement> arrivals);
    void resolve(const EventAnnouncementAction &action, RecoveryDisposition evidence);
    void cancel(CancelStuckEventAnnouncement request, ManagementCommit commit);
    void resume(ResumeEventAnnouncementQueue request, ManagementCommit commit);

    [[nodiscard]] CurrentEventAnnouncement
    current(ClockCut cut, Disclosure disclosure,
            SelectionEvidence selection = SelectionEvidence::complete) const;
    [[nodiscard]] EventAnnouncementManagement management(Disclosure disclosure) const;

  private:
    friend class AnnouncementManagement;

    // The first successful exact admission binds version and lineage to this
    // key. A version or duration that cannot be resolved consumes no key.
    struct SubmissionLess {
        bool operator()(const EventAnnouncementAction &left,
                        const EventAnnouncementAction &right) const;
    };

    struct Waiting {
        EventAnnouncementAction action;
        std::chrono::milliseconds duration;
        RecoveryDisposition disposition;
        bool start_authorized{false};

        [[nodiscard]] bool stuck() const;
    };

    struct Window {
        EventAnnouncementOccurrence occurrence;
        std::chrono::milliseconds start;
        std::chrono::milliseconds duration;
    };

    [[nodiscard]] bool cancel_waiting_head(EventAnnouncementAfterCancel);
    [[nodiscard]] bool matches_resume(const ResumeEventAnnouncementQueue &) const;
    void refresh_authority(std::span<const StartFact> facts);
    [[nodiscard]] bool owns(ClockCut cut) const;
    [[nodiscard]] std::optional<std::chrono::milliseconds> qualified_sample(ClockCut cut) const;
    [[nodiscard]] std::optional<std::string> issue_reference();

    std::string lifetime_;
    std::set<std::string, std::less<>> lifetimes_;
    std::optional<std::chrono::milliseconds> last_sample_;
    std::set<EventAnnouncementAction, SubmissionLess> submissions_;
    std::list<Waiting> waiting_;
    std::optional<Window> active_;
    std::optional<std::string> hold_;
    std::optional<ObservedAnnouncementManagement> management_capture_;
    std::uint64_t reference_sequence_{0};
};

// The owner constructs this component with its admitted opaque reference.
// Use only on the serialized owner thread, and keep the producer alive for
// the binding's lifetime. Its capture represents that owner's observation cut.
class AnnouncementManagement {
  public:
    AnnouncementManagement(Producer &producer, std::string_view owner_reference)
        : producer_(producer), owner_reference_(owner_reference) {}

    // An empty owner reference invalidates the capture without advancing.
    void advance_and_capture(OwnerTurn);
    void invalidate_capture();
    [[nodiscard]] ObservedAnnouncementManagement read(CommandAuthority) const;
    [[nodiscard]] CheckedManagementOutcome cancel(std::string_view, CancelStuckEventAnnouncement,
                                                  CheckedManagementTurn);
    [[nodiscard]] CheckedManagementOutcome resume(std::string_view, ResumeEventAnnouncementQueue,
                                                  CheckedManagementTurn);

  private:
    [[nodiscard]] std::optional<CheckedManagementOutcome> refuse(CheckedManagementTurn) const;

    Producer &producer_;
    std::string owner_reference_;
};

} // namespace subathon::server::event_announcement_owner
