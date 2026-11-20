// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <span>
#include <subathon/core/milestone_progression.hpp>

namespace subathon::core {

enum class MilestoneEditStatus {
    eligible,
    no_change,
    invalid_patch,
    denied,
    unavailable,
    stale,
    key_conflict,
    finished_progression_disabled,
    old_reachable_backlog,
    order_exhausted,
    invalid_choices
};

// The command service derives these facts from current, scoped authority.
struct MilestoneEditPolicy {
    bool unfinished_grant{};
    bool correction_grant{};
    bool progression_enabled{};
    bool automation_enabled{};
    std::int64_t durable_queue_tail{};
};

struct MilestoneEditPlan {
    MilestoneEditStatus status;
    std::vector<MilestoneTarget> targets;
    MilestoneProjection before;
    MilestoneProjection after;
    std::vector<MilestoneTarget> newly_completed;
    bool suppressed{};
    bool automation_gated{};
};

[[nodiscard]] MilestoneEditPlan
plan_milestone_target_edits(const MilestoneProgressionInput &current,
                            std::span<const MilestoneTarget> patch, MilestoneEditPolicy policy,
                            const MilestoneProgressionLimits &limits = {});

} // namespace subathon::core
