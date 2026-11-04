// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/milestone_progression.hpp>

namespace subathon::core {
MilestoneProgressionResult advance_milestone_progression(const MilestoneProgressionInput &,
                                                         const MilestoneProgressionLimits &) {
    throw MilestoneProgressionNotImplemented{};
}
} // namespace subathon::core
