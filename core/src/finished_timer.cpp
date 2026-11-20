// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/finished_timer.hpp>

namespace subathon::core {

FinishedInputResult project_finished_input(const FinishedSnapshot &, const FinishedSupport &,
                                           const FinishedLimits &) {
    throw FinishedTimerNotImplemented{};
}

FinishedCommandResult project_finished_command(const FinishedSnapshot &, const FinishedCommand &,
                                               const FinishedAccess &,
                                               const std::optional<ReopenPreview> &,
                                               const FinishedLimits &) {
    throw FinishedTimerNotImplemented{};
}

} // namespace subathon::core
