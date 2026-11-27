// SPDX-License-Identifier: GPL-3.0-or-later
#include "event_announcement_producer.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <tuple>
#include <type_traits>
#include <utility>

namespace subathon::server::event_announcement_owner {
namespace {

using Milliseconds = std::chrono::milliseconds;
using TickDistance = std::make_unsigned_t<Milliseconds::rep>;

// Once samples are ordered, unsigned subtraction represents their exact
// distance even when the signed difference would overflow.
TickDistance ticks_since(Milliseconds now, Milliseconds start) {
    return static_cast<TickDistance>(now.count()) - static_cast<TickDistance>(start.count());
}

bool valid_duration(Milliseconds duration) {
    return duration > Milliseconds::zero() &&
           duration.count() <= std::numeric_limits<std::int64_t>::max();
}

std::optional<Milliseconds> resolve_duration(const Admission &arrival) {
    if (arrival.action.presentation_version_ref.empty() || !arrival.version ||
        arrival.version->version_ref != arrival.action.presentation_version_ref) {
        return std::nullopt;
    }
    if (const auto *duration = std::get_if<Milliseconds>(&arrival.version->duration)) {
        return valid_duration(*duration) ? std::optional{*duration} : std::nullopt;
    }
    if (std::get<DurationDeclaration>(arrival.version->duration) == DurationDeclaration::omitted) {
        return Milliseconds{10000};
    }
    return std::nullopt;
}

bool authorized(ManagementCommit commit) {
    return commit.authority == CommandAuthority::current &&
           commit.completion == ApplicationCompletion::committed;
}

} // namespace

bool Producer::SubmissionLess::operator()(const EventAnnouncementAction &left,
                                          const EventAnnouncementAction &right) const {
    const auto key = [](const EventAnnouncementAction &entry) {
        const auto &action = entry.action;
        return std::tie(entry.event.event_ref, action.rule_ref, action.rule_version,
                        action.action_ref, action.action_version, action.position);
    };
    return key(left) < key(right);
}

bool Producer::Waiting::stuck() const {
    return !start_authorized || disposition != RecoveryDisposition::never_shown ||
           !valid_duration(duration);
}

Producer::Producer(std::string_view lifetime) : lifetime_(lifetime), lifetimes_{lifetime_} {}

void Producer::admit(std::span<const Admission> arrivals) {
    for (const auto &arrival : arrivals) {
        if (arrival.cause != Cause::event_announcement ||
            arrival.completion != ApplicationCompletion::committed ||
            arrival.authority != ActionAuthority::current ||
            submissions_.contains(arrival.action)) {
            continue;
        }
        const auto duration = resolve_duration(arrival);
        if (!duration) {
            continue;
        }

        std::list<Waiting> admitted;
        admitted.push_back({arrival.action, *duration, RecoveryDisposition::never_shown});
        submissions_.insert(arrival.action);
        waiting_.splice(waiting_.end(), admitted);
        management_capture_.reset();
    }
}

void Producer::refresh_authority(std::span<const StartFact> facts) {
    management_capture_.reset();
    for (auto &item : waiting_) {
        item.start_authorized = false;
        for (const auto &fact : facts) {
            if (fact.action == item.action) {
                item.start_authorized = fact.authority == ActionAuthority::current;
                if (!item.start_authorized) {
                    break;
                }
            }
        }
    }
}

bool Producer::owns(ClockCut cut) const { return !lifetime_.empty() && cut.lifetime == lifetime_; }

std::optional<Milliseconds> Producer::qualified_sample(ClockCut cut) const {
    if (!owns(cut) || !cut.elapsed_sample ||
        (last_sample_ && *cut.elapsed_sample < *last_sample_)) {
        return std::nullopt;
    }
    return cut.elapsed_sample;
}

std::optional<std::string> Producer::issue_reference() {
    if (reference_sequence_ == std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
    }
    // A newly constructed owner can use a fresh lifetime's namespace.
    auto reference = std::to_string(reference_sequence_ + 1) + ":" + lifetime_;
    ++reference_sequence_;
    return reference;
}

void Producer::advance(OwnerTurn turn) {
    if (!owns(turn.clock)) {
        return;
    }
    refresh_authority(turn.start_facts);

    const auto sample = qualified_sample(turn.clock);
    if (!sample) {
        return;
    }
    last_sample_ = sample;
    if (active_) {
        if (ticks_since(*sample, active_->start) <
            static_cast<TickDistance>(active_->duration.count())) {
            return;
        }
        active_.reset();
    }

    if (hold_ || waiting_.empty() || waiting_.front().stuck()) {
        return;
    }
    auto reference = issue_reference();
    if (!reference) {
        return;
    }
    auto &head = waiting_.front();
    active_.emplace(
        Window{{std::move(*reference), std::move(head.action)}, *sample, head.duration});
    waiting_.pop_front();
}

void Producer::restart(std::string_view new_lifetime,
                       std::span<const RetainedAnnouncement> arrivals) {
    if (new_lifetime.empty() || lifetimes_.contains(new_lifetime)) {
        return;
    }

    // Reconcile the ordered recovery history with the exact positions already
    // known to this owner. An omitted in-memory waiting item remains unresolved.
    std::string lifetime{new_lifetime};
    auto submissions = submissions_;
    auto waiting = waiting_;
    std::map<EventAnnouncementAction, std::list<Waiting>::iterator, SubmissionLess> previous;
    for (auto it = waiting.begin(); it != waiting.end(); ++it) {
        previous.emplace(it->action, it);
        it->disposition = RecoveryDisposition::uncertain;
        it->start_authorized = false;
    }

    struct Evidence {
        const RetainedAnnouncement *first;
        RecoveryDisposition disposition;
    };
    std::map<EventAnnouncementAction, Evidence, SubmissionLess> evidence;
    for (const auto &row : arrivals) {
        const auto [it, inserted] =
            evidence.try_emplace(row.action, Evidence{&row, row.disposition});
        if (inserted) {
            continue;
        }
        auto &retained = it->second;
        if (row.action == retained.first->action && row.disposition == RecoveryDisposition::shown) {
            retained.disposition = RecoveryDisposition::shown;
        } else if (retained.disposition != RecoveryDisposition::shown &&
                   (row.action != retained.first->action ||
                    row.duration != retained.first->duration ||
                    row.disposition != retained.disposition)) {
            retained.disposition = RecoveryDisposition::uncertain;
        }
    }

    std::list<Waiting> imported;
    for (const auto &row : arrivals) {
        const auto &retained = evidence.find(row.action)->second;
        if (retained.first != &row) {
            continue;
        }
        if (const auto old = previous.find(row.action); old != previous.end()) {
            const auto item = old->second;
            waiting.splice(item, imported);
            if (row.action != item->action) {
                continue;
            }
            if (retained.disposition == RecoveryDisposition::shown) {
                waiting.erase(item);
            } else if (row.duration == item->duration) {
                item->disposition = retained.disposition;
            }
        } else if (submissions.insert(row.action).second &&
                   retained.disposition != RecoveryDisposition::shown) {
            imported.push_back({row.action, row.duration, retained.disposition});
        }
    }
    waiting.splice(waiting.end(), imported);

    lifetimes_.insert(lifetime);
    lifetime_.swap(lifetime);
    submissions_.swap(submissions);
    waiting_.swap(waiting);
    last_sample_.reset();
    active_.reset();
    management_capture_.reset();
}

void Producer::resolve(const EventAnnouncementAction &action, RecoveryDisposition evidence) {
    const auto item = std::ranges::find(waiting_, action, &Waiting::action);
    if (item == waiting_.end()) {
        return;
    }
    if (evidence == RecoveryDisposition::shown) {
        management_capture_.reset();
        waiting_.erase(item);
    } else if (evidence == RecoveryDisposition::never_shown && item->disposition != evidence) {
        management_capture_.reset();
        item->disposition = evidence;
    }
}

void Producer::cancel(CancelStuckEventAnnouncement request, ManagementCommit commit) {
    if (!authorized(commit) || !owns(commit.turn.clock) || waiting_.empty() ||
        waiting_.front().action != request.expected_head) {
        return;
    }

    refresh_authority(commit.turn.start_facts);
    if (!waiting_.front().stuck()) {
        return;
    }

    (void)cancel_waiting_head(request.after_cancel);
}

bool Producer::cancel_waiting_head(EventAnnouncementAfterCancel choice) {
    if (choice == EventAnnouncementAfterCancel::continue_queue) {
        hold_.reset();
    } else if (!hold_) {
        auto reference = issue_reference();
        if (!reference) {
            return false;
        }
        hold_ = std::move(reference);
    }
    waiting_.pop_front();
    management_capture_.reset();
    return true;
}

bool Producer::matches_resume(const ResumeEventAnnouncementQueue &request) const {
    return hold_ == request.expected_hold_ref &&
           request.expected_next_head.has_value() != waiting_.empty() &&
           (!request.expected_next_head || *request.expected_next_head == waiting_.front().action);
}

void Producer::resume(ResumeEventAnnouncementQueue request, ManagementCommit commit) {
    if (!authorized(commit) || !owns(commit.turn.clock) || !matches_resume(request)) {
        return;
    }
    hold_.reset();
    advance(commit.turn);
}

CurrentEventAnnouncement Producer::current(ClockCut cut, Disclosure disclosure,
                                           SelectionEvidence selection) const {
    if (disclosure == Disclosure::denied) {
        return EventAnnouncementUndisclosed{};
    }
    if (selection != SelectionEvidence::complete || !owns(cut)) {
        return EventAnnouncementUnavailable{};
    }
    if (!active_) {
        return NoCurrentEventAnnouncement{};
    }
    const auto sample = qualified_sample(cut);
    if (!sample) {
        return EventAnnouncementUnavailable{};
    }
    const auto elapsed = ticks_since(*sample, active_->start);
    if (elapsed >= static_cast<TickDistance>(active_->duration.count())) {
        return NoCurrentEventAnnouncement{};
    }
    return ActiveEventAnnouncement{active_->occurrence,
                                   Milliseconds{static_cast<Milliseconds::rep>(elapsed)},
                                   active_->duration};
}

EventAnnouncementManagement Producer::management(Disclosure disclosure) const {
    if (disclosure == Disclosure::denied) {
        return EventAnnouncementUndisclosed{};
    }
    const auto *head = waiting_.empty() ? nullptr : &waiting_.front();
    if (hold_) {
        HeldEventAnnouncementQueue view{*hold_, std::nullopt};
        if (head) {
            view.next_head = EventAnnouncementQueueHead{head->action, head->stuck()};
        }
        return view;
    }
    if (head && head->stuck()) {
        return StuckEventAnnouncement{head->action};
    }
    return NoStuckEventAnnouncement{};
}

void AnnouncementManagement::advance_and_capture(OwnerTurn turn) {
    invalidate_capture();
    if (owner_reference_.empty() || !producer_.owns(turn.clock)) {
        return;
    }
    producer_.advance(turn);
    producer_.management_capture_ = producer_.management(Disclosure::allowed);
}

void AnnouncementManagement::invalidate_capture() { producer_.management_capture_.reset(); }

ObservedAnnouncementManagement AnnouncementManagement::read(CommandAuthority disclosure) const {
    if (disclosure != CommandAuthority::current) {
        return {{}, EventAnnouncementUndisclosed{}};
    }
    if (owner_reference_.empty() || !producer_.management_capture_) {
        return {{}, EventAnnouncementUnavailable{}};
    }
    return {owner_reference_, *producer_.management_capture_};
}

std::optional<CheckedManagementOutcome>
AnnouncementManagement::refuse(CheckedManagementTurn admission) const {
    using Outcome = CheckedManagementOutcome;
    if (!admission.owner_admitted || owner_reference_.empty() ||
        !producer_.owns(admission.turn.clock) ||
        admission.command == CommandAuthority::unavailable ||
        admission.disclosure == CommandAuthority::unavailable) {
        return Outcome::unavailable;
    }
    if (admission.command != CommandAuthority::current ||
        admission.disclosure != CommandAuthority::current) {
        return Outcome::refused;
    }
    return std::nullopt;
}

CheckedManagementOutcome AnnouncementManagement::cancel(std::string_view observed_owner,
                                                        CancelStuckEventAnnouncement request,
                                                        CheckedManagementTurn admission) {
    using Outcome = CheckedManagementOutcome;
    if (const auto refusal = refuse(admission)) {
        return *refusal;
    }
    const auto choice = request.after_cancel;
    if (observed_owner.empty() || (choice != EventAnnouncementAfterCancel::hold &&
                                   choice != EventAnnouncementAfterCancel::continue_queue)) {
        return Outcome::refused;
    }
    if (observed_owner != owner_reference_ || producer_.waiting_.empty() ||
        producer_.waiting_.front().action != request.expected_head) {
        return Outcome::stale;
    }
    if (std::ranges::find(admission.turn.start_facts, request.expected_head, &StartFact::action) ==
        admission.turn.start_facts.end()) {
        return Outcome::unavailable;
    }
    producer_.refresh_authority(admission.turn.start_facts);
    if (!producer_.waiting_.front().stuck()) {
        return Outcome::stale;
    }
    if (!producer_.cancel_waiting_head(choice)) {
        return Outcome::unavailable;
    }
    if (choice == EventAnnouncementAfterCancel::continue_queue) {
        producer_.advance(admission.turn);
    }
    return Outcome::applied;
}

CheckedManagementOutcome AnnouncementManagement::resume(std::string_view observed_owner,
                                                        ResumeEventAnnouncementQueue request,
                                                        CheckedManagementTurn admission) {
    using Outcome = CheckedManagementOutcome;
    if (const auto refusal = refuse(admission)) {
        return *refusal;
    }
    if (observed_owner.empty() || request.expected_hold_ref.empty()) {
        return Outcome::refused;
    }
    if (observed_owner != owner_reference_ || !producer_.matches_resume(request)) {
        return Outcome::stale;
    }
    producer_.hold_.reset();
    producer_.advance(admission.turn);
    return Outcome::applied;
}

} // namespace subathon::server::event_announcement_owner
