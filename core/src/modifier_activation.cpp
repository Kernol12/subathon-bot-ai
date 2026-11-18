// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/modifier_activation.hpp>

namespace subathon::core {
ModifierActivationProposal propose_modifier_activation(const ModifierDefinition &,
                                                       const std::optional<ModifierInstance> &,
                                                       const ModifierActivation &) {
    throw ModifierActivationNotImplemented{};
}

std::vector<ModifierTargetSelection> select_modifier_targets(const ModifierInstance &,
                                                             std::span<const std::string_view>) {
    throw ModifierActivationNotImplemented{};
}
} // namespace subathon::core
