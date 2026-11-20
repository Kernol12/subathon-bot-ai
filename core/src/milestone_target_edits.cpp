// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/milestone_target_edits.hpp>

#include "milestone_projection_internal.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <string_view>

namespace subathon::core {

MilestoneEditPlan plan_milestone_target_edits(const MilestoneProgressionInput &current,
                                              std::span<const MilestoneTarget> patch,
                                              MilestoneEditPolicy policy,
                                              const MilestoneProgressionLimits &limits) {
    using Status = MilestoneEditStatus;
    MilestoneEditPlan plan{};
    plan.status = Status::invalid_patch;
    if (patch.size() > current.targets.size() || patch.size() > limits.targets)
        return plan;
    std::size_t patch_bytes = 0;
    for (const auto &entry : patch) {
        if (entry.milestone_ref.empty() || entry.milestone_ref.size() > limits.reference_bytes ||
            entry.milestone_ref.size() > limits.owned_bytes - patch_bytes)
            return plan;
        patch_bytes += entry.milestone_ref.size();
    }
    try {
        plan.before = milestone_detail::project(current, limits, false).projection;
    } catch (const MilestoneProgressionFailure &) {
        plan.status = Status::unavailable;
        return plan;
    }
    std::map<std::string_view, std::int64_t> changes;
    for (const auto &entry : patch)
        if (!changes.emplace(entry.milestone_ref, entry.target_micro_points).second)
            return plan;
    auto proposed = current;
    const auto prefix = current.completed_prefix.size();
    bool correction = false;
    bool unfinished = false;
    for (std::size_t i = 0; i < proposed.targets.size(); ++i) {
        auto &target = proposed.targets[i];
        if (const auto change = changes.find(target.milestone_ref); change != changes.end()) {
            if (target.target_micro_points != change->second) {
                (i < prefix ? correction : unfinished) = true;
                target.target_micro_points = change->second;
            }
            changes.erase(change);
        }
        if (i > 0 && proposed.targets[i - 1].target_micro_points >= target.target_micro_points)
            return plan;
    }
    if (!changes.empty())
        return plan;
    if (!correction && !unfinished) {
        plan.status = Status::no_change;
        return plan;
    }
    const auto reachable_end = [&](const auto &targets) {
        const auto first = targets.begin() + static_cast<std::ptrdiff_t>(prefix);
        return std::upper_bound(first, targets.end(), current.score_micro_points,
                                [](auto score, const auto &target) {
                                    return score < target.target_micro_points;
                                }) -
               targets.begin();
    };
    const auto old_end = std::size_t(reachable_end(current.targets));
    const auto new_end = std::size_t(reachable_end(proposed.targets));
    const bool advance = policy.progression_enabled && old_end == prefix;
    try {
        plan.after = milestone_detail::project(proposed, limits, advance).projection;
    } catch (const MilestoneProgressionFailure &) {
        return plan;
    }
    plan.targets = std::move(proposed.targets);
    plan.suppressed = !policy.progression_enabled && new_end > prefix;
    plan.automation_gated = advance && new_end > prefix && !policy.automation_enabled;
    if ((correction && !policy.correction_grant) || (unfinished && !policy.unfinished_grant))
        plan.status = Status::denied;
    else if (!policy.progression_enabled && new_end > old_end)
        plan.status = Status::finished_progression_disabled;
    else if (policy.progression_enabled && old_end > prefix)
        plan.status = Status::old_reachable_backlog;
    else if (advance && (policy.durable_queue_tail < 0 ||
                         new_end - prefix > std::uint64_t(std::numeric_limits<std::int64_t>::max() -
                                                          policy.durable_queue_tail)))
        plan.status = Status::order_exhausted;
    else {
        plan.status = Status::eligible;
        if (advance)
            plan.newly_completed.assign(plan.targets.begin() + std::ptrdiff_t(prefix),
                                        plan.targets.begin() + std::ptrdiff_t(new_end));
    }
    return plan;
}

} // namespace subathon::core
