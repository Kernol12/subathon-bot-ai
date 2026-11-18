// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/milestone_timeline.hpp>

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
};

} // namespace

struct MilestoneTimeline::State {
    State(MilestoneTimelineSettings configured, MilestoneGroupRelease &port)
        : settings(configured), release(port) {}

    void check_sample(Milliseconds now) const {
        if (last_sample && now < *last_sample)
            throw std::invalid_argument("Regressing milestone timeline sample");
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
            release_uncertain || pending.empty())
            return;

        const auto &head = pending.front();
        if (release.review_held(head.achievement))
            return;

        // Start on the actual owner turn, with a full presentation for this group.
        PresentationWindow next{head.achievement.milestone_ref, now,
                                head.timing.presentation.value_or(settings.presentation),
                                head.timing.gap.value_or(settings.gap)};
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
    for (const auto &group : groups)
        check_timing(group.timing.presentation.value_or(state_->settings.presentation),
                     group.timing.gap.value_or(state_->settings.gap));

    // Build the complete batch before appending any of it to the running queue.
    std::list<CommittedMilestoneGroup> batch(std::make_move_iterator(groups.begin()),
                                             std::make_move_iterator(groups.end()));
    state_->pending.splice(state_->pending.end(), batch);
    state_->advance_to(now);
}

void MilestoneTimeline::advance(Milliseconds now) {
    state_->check_sample(now);
    state_->advance_to(now);
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
