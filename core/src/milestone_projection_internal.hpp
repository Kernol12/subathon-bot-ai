// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/core/milestone_progression.hpp>

namespace subathon::core::milestone_detail {
MilestoneProgressionData project(const MilestoneProgressionInput &,
                                 const MilestoneProgressionLimits &, bool complete_reachable);
}
