// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/core/milestone_progression.hpp>
#include <vector>

namespace subathon::server {

// Valid timing inputs: positive presentation durations and nonnegative gaps.
// A zero gap adds no delay. This applies to overrides when present.
struct MilestoneTimelineSettings {
    std::chrono::milliseconds presentation{10'000};
    std::chrono::milliseconds gap{1'000};
    bool follow_countdown_pause{false};
};

struct MilestoneTimingOverride {
    std::optional<std::chrono::milliseconds> presentation;
    std::optional<std::chrono::milliseconds> gap; // Gap after this presentation.
};

enum class MilestoneActionKind { automation, bonus };

struct MilestoneAction {
    MilestoneActionKind kind;
    std::string action_ref;
    std::string version_ref;

    bool operator==(const MilestoneAction &) const = default;
};

struct MilestoneOccurrenceKey {
    std::string ref;

    bool operator==(const MilestoneOccurrenceKey &) const = default;
};

struct CommittedMilestoneGroup {
    core::MilestoneCompletion achievement;
    std::vector<MilestoneAction> actions;
    MilestoneTimingOverride timing;
    std::optional<MilestoneOccurrenceKey> occurrence{};
};

struct ActiveMilestonePresentation {
    std::string milestone_ref;
    std::chrono::milliseconds elapsed;
    std::chrono::milliseconds duration;

    bool operator==(const ActiveMilestonePresentation &) const = default;
};

class MilestoneGroupRelease {
  public:
    virtual ~MilestoneGroupRelease() = default;

    // Read the existing decision for the head immediately before starting it.
    // An already held head neither presents nor releases and blocks later groups.
    [[nodiscard]] virtual bool review_held(const core::MilestoneCompletion &achievement) const = 0;

    // Schedule the complete captured group at presentation start, on the owner.
    // Authorized release of this group is required before dispatching its jobs
    // or applying its captured bonuses. Existing authority and transaction
    // checks still apply. Return promptly with no reentry.
    virtual void release(CommittedMilestoneGroup group) = 0;
};

class MilestoneTimelineNotImplemented final : public std::logic_error {
  public:
    explicit MilestoneTimelineNotImplemented(const char *entry);
};

// Revision 2. The caller serializes operations on the authoritative server
// thread and supplies nondecreasing elapsed samples in one server lifetime.
// Regressing samples and invalid timings are outside the valid caller contract.
// The release port must outlive the timeline. Outputs consume owned snapshots
// via the server read path and never register with or acknowledge the queue.
class MilestoneTimeline final {
  public:
    MilestoneTimeline(MilestoneTimelineSettings settings, MilestoneGroupRelease &release);
    ~MilestoneTimeline();

    // Append new, committed groups in threshold order without moving existing
    // presentation or gap boundaries. This entry carries no commit authority
    // and does not alter achievement, score or job history.
    void accept_committed(std::vector<CommittedMilestoneGroup> groups,
                          std::chrono::milliseconds now);
    // Install the committed pending suffix on a fresh timeline without
    // advancing it. The first eligible owner turn starts at elapsed zero.
    void restore_committed(std::vector<CommittedMilestoneGroup> groups);
    void advance(std::chrono::milliseconds now);

    // Manual pause and an optionally linked countdown pause gate new starts.
    // Active presentation and gap time keep running. Subject to holds, the
    // head starts when both pause reasons permit it and the gap has elapsed.
    void set_queue_paused(bool paused, std::chrono::milliseconds now);
    void set_countdown_paused(bool paused, std::chrono::milliseconds now);

    // Owned view at the last server sample. No active presentation during a gap.
    [[nodiscard]] std::optional<ActiveMilestonePresentation> presentation() const;

  private:
    struct State;
    std::unique_ptr<State> state_;
};

} // namespace subathon::server
