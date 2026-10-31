// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/pause_policy.hpp>

namespace subathon::core {
PauseState set_pause_reason(const PauseState &, PauseReason, bool) {
    throw PausePolicyNotImplemented{};
}

PauseState observe_pause_signals(const PauseState &, const PauseConfig &,
                                 const PauseObservation &) {
    throw PausePolicyNotImplemented{};
}
} // namespace subathon::core
