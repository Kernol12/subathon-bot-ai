// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/contribution.hpp>

namespace subathon::core {
ContributionEligibility assess_contribution_eligibility(const ContributionState &) {
    throw ContributionEligibilityNotImplemented{};
}
} // namespace subathon::core
