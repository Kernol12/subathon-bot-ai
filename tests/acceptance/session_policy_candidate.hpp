// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "session_policy.hpp"

namespace subathon::server::session_policy_candidate {
using session_policy::Attempt;
using session_policy::Budgets;
using session_policy::Candidate;
using session_policy::Drain;
using session_policy::Identity;
using session_policy::Owner;
using session_policy::Pin;
using session_policy::Policy;
using session_policy::Readiness;
using session_policy::ReadinessProducer;
using session_policy::ReadinessState;
using session_policy::release;
using session_policy::Route;
using session_policy::Snapshot;

struct Facts : session_policy::Facts {
    Facts() {
        installation = 1;
        origin = "https://timer.example";
        routes = {{"/account", "POST", AccountTransportPurpose::protected_access}};
        peers = {"127.0.0.1"};
        limits = AccountTransportLimits{};
        foundation = native_abi = native_artifact = profile = 1;
        listener = edge_artifact = edge_config = logging = 1;
        isolation = forwarding_grammar = normalization = 1;
        configuration = "configuration";
        address = "127.0.0.1";
        mode = Mode::proxied;
        forwarded_metadata = true;
    }
};
} // namespace subathon::server::session_policy_candidate
