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

inline void apply_owner_update(MilestoneTimeline &timeline, std::chrono::milliseconds now,
                               const OwnerUpdate &update) {
    adopt_milestone_timeline_update(
        timeline, now, std::visit([](const auto &fact) -> TimelineUpdate {
            using Fact = std::decay_t<decltype(fact)>;
            if constexpr (std::is_same_v<Fact, CommittedStop>)
                return AcknowledgedStop{fact.intent.target, fact.operation, fact.added_hold,
                                        static_cast<MilestoneStopCompletionSource>(fact.proof),
                                        fact.presentation};
            else if constexpr (std::is_same_v<Fact, UncertainStop>)
                return PendingStopReadback{fact.target, fact.operation};
            else
                return AcknowledgedResume{fact.resolved};
        }, update));
}
} // namespace milestone_006_stage1
