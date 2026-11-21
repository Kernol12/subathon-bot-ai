// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/modifier_stacking.hpp>

namespace subathon::server {
ModifierStackEvaluation evaluate_captured_modifiers(const ModifierStackCapture &,
                                                    std::span<const ModifierBaseEffect>,
                                                    const core::ModifierStackLimits &) {
    throw ModifierHandoffNotImplemented{};
}
} // namespace subathon::server
