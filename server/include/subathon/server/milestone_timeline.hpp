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

struct CommittedMilestoneGroup {
    core::MilestoneCompletion achievement;
    std::vector<MilestoneAction> actions;
    MilestoneTimingOverride timing;
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
    // A held head and all groups behind it wait, regardless of elapsed time.
    [[nodiscard]] virtual bool review_held(const core::MilestoneCompletion &achievement) const = 0;

    // Schedule the complete captured group at presentation start, on the owner.
    // Return promptly with no reentry. Delivery and effects use their own paths.
    virtual void release(CommittedMilestoneGroup group) = 0;
};

class MilestoneTimelineNotImplemented final : public std::logic_error {
  public:
    explicit MilestoneTimelineNotImplemented(const char *entry);
};

// Revision 1. The caller serializes operations on the authoritative server
// thread and supplies nondecreasing elapsed samples in one server lifetime.
// The release port must outlive the timeline. Outputs consume owned snapshots
// via the server read path and never register with or acknowledge the queue.
class MilestoneTimeline final {
  public:
    MilestoneTimeline(MilestoneTimelineSettings settings, MilestoneGroupRelease &release);
    ~MilestoneTimeline();

    // Supply only new, committed groups in threshold order. This entry carries
    // no commit authority and does not alter achievement, score or job history.
    void accept_committed(std::vector<CommittedMilestoneGroup> groups,
                          std::chrono::milliseconds now);
    void advance(std::chrono::milliseconds now);
    void set_queue_paused(bool paused, std::chrono::milliseconds now);
    void set_countdown_paused(bool paused, std::chrono::milliseconds now);

    // Owned view at the last server sample. No active presentation during a gap.
    [[nodiscard]] std::optional<ActiveMilestonePresentation> presentation() const;

  private:
    struct State;
    std::unique_ptr<State> state_;
};

} // namespace subathon::server
