// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/milestone_target_edits.hpp>

namespace subathon::core {

MilestoneEditPlan plan_milestone_target_edits(const MilestoneProgressionInput &,
                                              std::span<const MilestoneTarget>, MilestoneEditPolicy,
                                              const MilestoneProgressionLimits &) {
    throw std::logic_error("MILESTONE-002 NI: plan_milestone_target_edits");
}

} // namespace subathon::core
