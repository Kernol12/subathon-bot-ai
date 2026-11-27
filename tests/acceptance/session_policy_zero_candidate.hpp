// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "session_policy_candidate.hpp"

namespace subathon::server::session_policy_candidate {
struct ExplicitEmptyAffectedChannels {};

inline std::optional<Candidate> validate_exact(Policy &policy, Attempt attempt, const Facts &facts,
                                               ExplicitEmptyAffectedChannels) {
    // Before the new input is implemented, the policy can represent only a legacy empty span.
    return policy.validate(attempt, facts, std::span<const Identity>{});
}
} // namespace subathon::server::session_policy_candidate
