// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/milestone_timeline.hpp>

#include <algorithm>
#include <iterator>
#include <list>
#include <type_traits>
#include <utility>

namespace subathon::server {
namespace {

using Milliseconds = std::chrono::milliseconds;
using TickDistance = std::make_unsigned_t<Milliseconds::rep>;

// For ordered samples the unsigned difference is exact, even across the full
// signed range. Test the duration and gap separately so their sum needn't fit.
TickDistance ticks_since(Milliseconds now, Milliseconds started_at) {
    return static_cast<TickDistance>(now.count()) - static_cast<TickDistance>(started_at.count());
}

void check_timing(Milliseconds duration, Milliseconds gap) {
    if (duration <= Milliseconds::zero() || gap < Milliseconds::zero())
        throw std::invalid_argument("Invalid milestone timing");
}

struct PresentationWindow {
    std::string milestone_ref;
    Milliseconds started_at;
    Milliseconds duration;
    Milliseconds gap;
    std::optional<MilestoneOccurrenceKey> occurrence;
    std::optional<MilestoneEditKey> edit;
    std::optional<MilestoneApplicationOrigin> application;
};

bool matches(const MilestoneTimelineOccurrence &target,
             const std::optional<MilestoneOccurrenceKey> &occurrence,
             const std::optional<MilestoneEditKey> &edit,
             const std::optional<MilestoneApplicationOrigin> &application) {
    // Legacy committed groups can carry only a globally unique occurrence.
    return occurrence == target.occurrence && target.edit == edit &&
           (!application || application == target.application);
}

struct StopKey {
    MilestoneTimelineOccurrence target;
    std::string operation;
    bool operator==(const StopKey &) const = default;
};

} // namespace

struct MilestoneTimeline::State {
    State(MilestoneTimelineSettings configured, MilestoneGroupRelease &port)
        : settings(configured), release(port) {}

    void check_sample(Milliseconds now) const {
        if (last_sample && now < *last_sample)
            throw std::invalid_argument("Regressing milestone timeline sample");
    }

    void append(std::vector<CommittedMilestoneGroup> groups) {
        for (const auto &group : groups)
            check_timing(group.timing.presentation.value_or(settings.presentation),
                         group.timing.gap.value_or(settings.gap));

        // Build the complete batch before appending any of it.
        std::list<CommittedMilestoneGroup> batch(std::make_move_iterator(groups.begin()),
                                                 std::make_move_iterator(groups.end()));
        pending.splice(pending.end(), batch);
    }

    void advance_to(Milliseconds now) {
        last_sample = now;
        if (window) {
            const auto elapsed = ticks_since(now, window->started_at);
            const auto duration = static_cast<TickDistance>(window->duration.count());
            const auto gap = static_cast<TickDistance>(window->gap.count());
            if (elapsed < duration || elapsed - duration < gap)
                return;
            window.reset();
        }

        if (queue_paused || (settings.follow_countdown_pause && countdown_paused) ||
            release_uncertain || !uncertain_stops.empty() || !interruption_holds.empty())
            return;

        while (!pending.empty()) {
            const auto &head = pending.front();
            if (!head.occurrence) {
                if (release.review_held(head.achievement))
                    return;
                break;
            }
            const auto disposition = release.occurrence_disposition(*head.occurrence);
            const auto edit =
                head.edit_membership ? std::optional{head.edit_membership->origin} : std::nullopt;
            // The head port reads the current independent holds on every turn.
            if (disposition == MilestoneHeadDisposition::ready) {
                const auto stopped = std::ranges::any_of(stops, [&](const auto &entry) {
                    return matches(entry.target, head.occurrence, edit, head.application_origin);
                });
                if (!stopped)
                    break;
            } else if (disposition != MilestoneHeadDisposition::skipped) {
                return;
            }
            pending.pop_front();
        }
        if (pending.empty())
            return;
        const auto &head = pending.front();

        // Start on the actual owner turn, with a full presentation for this group.
        PresentationWindow next{head.achievement.milestone_ref,
                                now,
                                head.timing.presentation.value_or(settings.presentation),
                                head.timing.gap.value_or(settings.gap),
                                head.occurrence,
                                head.edit_membership ? std::optional{head.edit_membership->origin}
                                                     : std::nullopt,
                                head.application_origin};
        auto captured = head;
        // A throwing port may already have scheduled work. Keep the head and
        // block further starts in this lifetime rather than retry or pass it.
        release_uncertain = true;
        release.release(std::move(captured));
        release_uncertain = false;
        window.emplace(std::move(next));
        pending.pop_front();
    }

    MilestoneTimelineSettings settings;
    MilestoneGroupRelease &release;
    std::list<CommittedMilestoneGroup> pending;
    std::optional<PresentationWindow> window;
    std::optional<Milliseconds> last_sample;
    bool queue_paused{false};
    bool countdown_paused{false};
    bool release_uncertain{false};
    std::vector<StopKey> stops;
    std::vector<StopKey> uncertain_stops;
    std::vector<MilestoneTimelineCause> interruption_holds;
    std::vector<MilestoneTimelineCause> resolved_interruption_holds;
};

MilestoneTimelineNotImplemented::MilestoneTimelineNotImplemented(const char *entry)
    : std::logic_error(std::string("MILESTONETEST-004 milestone timeline NI: ") + entry) {}

MilestoneTimeline::MilestoneTimeline(MilestoneTimelineSettings settings,
                                     MilestoneGroupRelease &release) {
    check_timing(settings.presentation, settings.gap);
    state_ = std::make_unique<State>(settings, release);
}
MilestoneTimeline::~MilestoneTimeline() = default;

void MilestoneTimeline::accept_committed(std::vector<CommittedMilestoneGroup> groups,
                                         Milliseconds now) {
    state_->check_sample(now);
    state_->append(std::move(groups));
    state_->advance_to(now);
}

void MilestoneTimeline::restore_committed(std::vector<CommittedMilestoneGroup> groups) {
    if (state_->last_sample || !state_->pending.empty())
        throw std::logic_error("Milestone queue restoration requires a fresh timeline");
    state_->append(std::move(groups));
}

void MilestoneTimeline::advance(Milliseconds now) {
    state_->check_sample(now);
    state_->advance_to(now);
}

void MilestoneTimeline::adopt_stop(Milliseconds now, const MilestoneTimelineOccurrence &target,
                                   const std::string &operation,
                                   std::optional<MilestoneTimelineCause> hold) {
    state_->check_sample(now);
    StopKey key{target, operation};
    const bool first_seen = std::ranges::find(state_->stops, key) == state_->stops.end();
    if (first_seen) {
        state_->stops.reserve(state_->stops.size() + 1);
        if (hold &&
            std::ranges::find(state_->interruption_holds, *hold) ==
                state_->interruption_holds.end() &&
            std::ranges::find(state_->resolved_interruption_holds, *hold) ==
                state_->resolved_interruption_holds.end()) {
            state_->interruption_holds.push_back(std::move(*hold));
        }
        if (auto &window = state_->window;
            window && matches(target, window->occurrence, window->edit, window->application) &&
            ticks_since(now, window->started_at) <
                static_cast<TickDistance>(window->duration.count())) {
            window->started_at = now;
            window->duration = Milliseconds::zero();
        }
    }
    std::erase(state_->uncertain_stops, key);
    if (first_seen)
        state_->stops.push_back(std::move(key));
    state_->last_sample = now;
}

void MilestoneTimeline::observe_stop_completion(Milliseconds now,
                                                const MilestoneTimelineOccurrence &target,
                                                const std::string &operation, bool uncertain,
                                                bool reconciled) {
    state_->check_sample(now);
    const StopKey key{target, operation};
    if (reconciled)
        std::erase(state_->uncertain_stops, key);
    if (uncertain &&
        std::ranges::find(state_->stops, key) == state_->stops.end() &&
        std::ranges::find(state_->uncertain_stops, key) == state_->uncertain_stops.end())
        state_->uncertain_stops.push_back(key);
    state_->last_sample = now;
}

void MilestoneTimeline::adopt_resume(Milliseconds now,
                                     const std::vector<MilestoneTimelineCause> &resolved) {
    state_->check_sample(now);
    // Resume can reconcile before an earlier Stop acknowledgement. An
    // already resolved cause must never be reintroduced by Stop readback.
    for (const auto &cause : resolved)
        if (std::ranges::find(state_->resolved_interruption_holds, cause) ==
            state_->resolved_interruption_holds.end())
            state_->resolved_interruption_holds.push_back(cause);
    std::erase_if(state_->interruption_holds, [&](const auto &hold) {
        return std::ranges::find(resolved, hold) != resolved.end();
    });
    state_->last_sample = now;
}

void MilestoneTimeline::adopt_ordered_stop(std::chrono::milliseconds now,
                                           const MilestoneTimelineOccurrence &target,
                                           const std::string &operation,
                                           std::vector<MilestoneTimelineCause> active_holds) {
    adopt_stop(now, target, operation, {});
    std::erase_if(active_holds, [&](const auto &hold) {
        return std::ranges::find(state_->resolved_interruption_holds, hold) !=
               state_->resolved_interruption_holds.end();
    });
    std::erase_if(state_->interruption_holds,
                  [&](const auto &hold) { return hold.track == target.track; });
    state_->interruption_holds.insert(state_->interruption_holds.end(),
                                      std::make_move_iterator(active_holds.begin()),
                                      std::make_move_iterator(active_holds.end()));
}

void MilestoneTimeline::set_queue_paused(bool paused, Milliseconds now) {
    state_->check_sample(now);
    state_->queue_paused = paused;
    state_->advance_to(now);
}

void MilestoneTimeline::set_countdown_paused(bool paused, Milliseconds now) {
    state_->check_sample(now);
    state_->countdown_paused = paused;
    state_->advance_to(now);
}

std::optional<ActiveMilestonePresentation> MilestoneTimeline::presentation() const {
    const auto &window = state_->window;
    if (!window)
        return std::nullopt;
    const auto elapsed = ticks_since(*state_->last_sample, window->started_at);
    if (elapsed >= static_cast<TickDistance>(window->duration.count()))
        return std::nullopt;
    return ActiveMilestonePresentation{window->milestone_ref,
                                       Milliseconds{static_cast<Milliseconds::rep>(elapsed)},
                                       window->duration};
}

} // namespace subathon::server
