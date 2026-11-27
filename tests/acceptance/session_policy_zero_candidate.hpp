// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "session_policy_candidate.hpp"

namespace subathon::server::session_policy_candidate {
struct ExplicitEmptyAffectedChannels {};

inline std::optional<Candidate> validate_exact(Policy &policy, Attempt attempt, const Facts &facts,
                                               ExplicitEmptyAffectedChannels) {
    return policy.validate_exact(attempt, facts, session_policy::ExactAffectedChannels{});
}
} // namespace subathon::server::session_policy_candidate
