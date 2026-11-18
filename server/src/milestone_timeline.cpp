// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/milestone_timeline.hpp>

namespace subathon::server {

struct MilestoneTimeline::State {};

MilestoneTimelineNotImplemented::MilestoneTimelineNotImplemented(const char *entry)
    : std::logic_error(std::string("MILESTONETEST-004 milestone timeline NI: ") + entry) {}

MilestoneTimeline::MilestoneTimeline(MilestoneTimelineSettings, MilestoneGroupRelease &) {}
MilestoneTimeline::~MilestoneTimeline() = default;

void MilestoneTimeline::accept_committed(std::vector<CommittedMilestoneGroup>,
                                         std::chrono::milliseconds) {
    throw MilestoneTimelineNotImplemented("accept_committed");
}

void MilestoneTimeline::advance(std::chrono::milliseconds) {
    throw MilestoneTimelineNotImplemented("advance");
}

void MilestoneTimeline::set_queue_paused(bool, std::chrono::milliseconds) {
    throw MilestoneTimelineNotImplemented("set_queue_paused");
}

void MilestoneTimeline::set_countdown_paused(bool, std::chrono::milliseconds) {
    throw MilestoneTimelineNotImplemented("set_countdown_paused");
}

std::optional<ActiveMilestonePresentation> MilestoneTimeline::presentation() const {
    throw MilestoneTimelineNotImplemented("presentation");
}

} // namespace subathon::server
