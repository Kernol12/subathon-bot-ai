// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_milestone_interruption.hpp>

namespace milestone_006_stage1 {
using namespace subathon::server;
using namespace subathon::server::milestone_interruption;

// Only owner facts cross this seam. The sample belongs to the accepting owner
// turn, including for normal readback. The observation is evidence for a live
// presentation when available. A queued slot can qualify Stop without one.
// The test boundary separates adoption of an owner fact from the scheduling
// walk. A following advance at the same sample is a distinct, eligible turn.
enum class StopProof { normal_commit, normal_readback };
struct CommittedStop {
    StopIntent intent;
    QByteArray operation;
    StopProof proof;
    std::optional<PresentationObservation> presentation;
    std::optional<ScopedCause> added_hold;
};

struct UncertainStop {
    Occurrence target;
    QByteArray operation;
};

struct CommittedResume {
    std::vector<ScopedCause> resolved;
};

using OwnerUpdate = std::variant<CommittedStop, UncertainStop, CommittedResume>;

class TimelineUpdateMissing final : public std::logic_error {
  public:
    using std::logic_error::logic_error;
};

inline void apply_owner_update(MilestoneTimeline &, std::chrono::milliseconds,
                               const OwnerUpdate &update) {
    if (const auto *stop = std::get_if<CommittedStop>(&update)) {
        if (stop->proof == StopProof::normal_readback)
            throw TimelineUpdateMissing("T1: apply normally acknowledged exact Stop readback");
        throw TimelineUpdateMissing("S2-S5/S8: apply normally acknowledged exact Stop");
    }
    if (std::holds_alternative<UncertainStop>(update))
        throw TimelineUpdateMissing("T1: fence scheduling after uncertain Stop completion");
    throw TimelineUpdateMissing("S5: apply exact committed Queue Resume");
}
} // namespace milestone_006_stage1
