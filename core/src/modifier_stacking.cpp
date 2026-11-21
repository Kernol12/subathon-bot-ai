// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/modifier_stacking.hpp>

namespace subathon::core {
ModifierCombination combine_modifiers(std::span<const QualifiedModifierOccurrence>,
                                      const ModifierStackPolicy &,
                                      std::span<const ModifierTargetKey>,
                                      const ModifierStackLimits &) {
    throw ModifierCombinationNotImplemented{};
}
} // namespace subathon::core
