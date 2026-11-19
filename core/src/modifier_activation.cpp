// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/modifier_activation.hpp>

#include <algorithm>
#include <limits>

namespace subathon::core {
namespace {

ExactDecimal modifier_factor(std::string_view spelling) {
    try {
        return parse_nonnegative_factor(spelling);
    } catch (const std::invalid_argument &) {
        throw ModifierActivationFailure(ModifierActivationError::invalid_factor);
    } catch (const std::length_error &) {
        throw ModifierActivationFailure(ModifierActivationError::invalid_factor);
    }
}

std::int64_t extend_remaining(std::int64_t remaining, std::int64_t duration) {
    using Limits = std::numeric_limits<std::int64_t>;
    if ((duration > 0 && remaining > Limits::max() - duration) ||
        (duration < 0 && remaining < Limits::min() - duration))
        throw ModifierActivationFailure(ModifierActivationError::duration_overflow);
    return remaining + duration;
}

} // namespace

ModifierActivationProposal
propose_modifier_activation(const ModifierDefinition &definition,
                            const std::optional<ModifierInstance> &active,
                            const ModifierActivation &activation) {
    if (activation.trigger &&
        activation.trigger->timing != ModifierTriggerTiming::include_trigger &&
        activation.trigger->timing != ModifierTriggerTiming::subsequent_only)
        throw ModifierActivationFailure(ModifierActivationError::missing_trigger_timing);

    for (const auto &target : definition.targets)
        (void)modifier_factor(target.factor_decimal);

    ModifierActivationProposal result;
    if (active && definition.reactivation == ModifierReactivation::ignore) {
        if (activation.trigger)
            result.for_trigger = active;
        return result;
    }

    if (active && definition.reactivation != ModifierReactivation::replace) {
        const auto remaining = definition.reactivation == ModifierReactivation::extend
                                   ? extend_remaining(active->remaining_ms, definition.duration_ms)
                                   : definition.duration_ms;
        result.revision_to_append = ModifierInstance{active->instance_ref,
                                                     activation.new_revision_ref,
                                                     active->revision_ref,
                                                     active->captured_definition,
                                                     remaining,
                                                     activation.source,
                                                     activation.trigger};
    } else {
        result.revision_to_append = ModifierInstance{activation.new_instance_ref,
                                                     activation.new_revision_ref,
                                                     {},
                                                     definition,
                                                     definition.duration_ms,
                                                     activation.source,
                                                     activation.trigger};
        if (active)
            result.superseded_instance_ref = active->instance_ref;
    }

    if (activation.trigger)
        result.for_trigger = activation.trigger->timing == ModifierTriggerTiming::include_trigger
                                 ? result.revision_to_append
                                 : active;
    return result;
}

std::vector<ModifierTargetSelection>
select_modifier_targets(const ModifierInstance &instance,
                        std::span<const std::string_view> matched_qualifiers) {
    std::vector<ModifierTargetSelection> selected;
    for (const auto &target : instance.captured_definition.targets) {
        const auto factor = modifier_factor(target.factor_decimal);
        if (target.qualifier_ref && std::ranges::find(matched_qualifiers, *target.qualifier_ref) ==
                                        matched_qualifiers.end())
            continue;
        selected.push_back({target, instance.captured_definition.revision_ref,
                            instance.revision_ref, factor.value()});
    }
    return selected;
}
} // namespace subathon::core
