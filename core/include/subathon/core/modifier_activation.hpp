// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <subathon/core/exact_decimal.hpp>
#include <vector>

namespace subathon::core {
// MODIFIER-001 draft revision 1 with correction 1, specification v0.45 §7 and MOD-03.
enum class ModifierTargetKind { added_time, milestone_score, counter };
struct ModifierTarget {
    ModifierTargetKind kind;
    std::string target_ref;
    // An absent qualifier matches every eligible contribution. Predicate evaluation is upstream.
    std::optional<std::string> qualifier_ref;
    std::string factor_decimal;
    bool operator==(const ModifierTarget &) const = default;
};

enum class ModifierReactivation { extend, restart, replace, ignore };
struct ModifierDefinition {
    std::string definition_ref;
    std::string revision_ref;
    std::int64_t duration_ms;
    std::vector<ModifierTarget> targets;
    ModifierReactivation reactivation = ModifierReactivation::extend;
    bool operator==(const ModifierDefinition &) const = default;
};

enum class ModifierActivationKind { moderator, chat_command, milestone, rule, schedule };
struct ModifierActivationSource {
    ModifierActivationKind kind;
    std::string source_ref;
    bool operator==(const ModifierActivationSource &) const = default;
};

enum class ModifierTriggerTiming { unspecified, include_trigger, subsequent_only };
struct ModifierTriggeringContribution {
    std::string event_ref;
    ModifierTriggerTiming timing = ModifierTriggerTiming::unspecified;
    bool operator==(const ModifierTriggeringContribution &) const = default;
};

struct ModifierActivation {
    std::string new_instance_ref;
    std::string new_revision_ref;
    ModifierActivationSource source;
    std::optional<ModifierTriggeringContribution> trigger;
};

struct ModifierInstance {
    std::string instance_ref;
    std::string revision_ref;
    std::optional<std::string> parent_revision_ref;
    ModifierDefinition captured_definition;
    std::int64_t remaining_ms;
    ModifierActivationSource source;
    std::optional<ModifierTriggeringContribution> trigger;
    bool operator==(const ModifierInstance &) const = default;
};

struct ModifierActivationProposal {
    std::optional<ModifierInstance> revision_to_append;
    std::optional<std::string> superseded_instance_ref;
    std::optional<ModifierInstance> for_trigger;
};

enum class ModifierActivationError { invalid_factor, duration_overflow, missing_trigger_timing };
class ModifierActivationFailure final : public std::invalid_argument {
  public:
    explicit ModifierActivationFailure(ModifierActivationError code)
        : std::invalid_argument("Invalid modifier activation"), code_(code) {}
    [[nodiscard]] ModifierActivationError code() const noexcept { return code_; }

  private:
    ModifierActivationError code_;
};

class ModifierActivationNotImplemented final : public std::logic_error {
  public:
    ModifierActivationNotImplemented()
        : std::logic_error(
              "Modifier activation is intentionally unimplemented; review tests first") {}
};

// The caller supplies fresh refs and the current live revision, if any, for the same definition.
// With no active instance, every policy creates the initial instance under the selected trigger
// timing. The policies below apply only when reactivating an active instance.
// Reactivation uses the incoming policy and duration. Extend adds that duration to remaining time;
// Restart resets remaining time to that duration. Both keep the instance's captured definition.
// Replace captures new settings on a new instance and names the instance to supersede. Ignore
// appends no revision. A configured factor must be finite and nonnegative.
// Subsequent inputs see the appended revision, or the existing instance on Ignore. For a
// triggering input, subsequent_only preserves the pre-activation view. Without a trigger
// there is no for_trigger view. The caller owns authority, clock sampling and atomic commit.
[[nodiscard]] ModifierActivationProposal
propose_modifier_activation(const ModifierDefinition &definition,
                            const std::optional<ModifierInstance> &active,
                            const ModifierActivation &activation);

struct ModifierTargetSelection {
    ModifierTarget target;
    std::string definition_revision_ref;
    std::string instance_revision_ref;
    Exact factor;
    bool operator==(const ModifierTargetSelection &) const = default;
};

// Project an already selected current or retained arrival revision. Only supplied matching
// qualifier refs qualify. The projection owns its provenance and exact factors. Combination,
// requested/actual effects and historical accounting remain with their owning consumers.
[[nodiscard]] std::vector<ModifierTargetSelection>
select_modifier_targets(const ModifierInstance &instance,
                        std::span<const std::string_view> matched_qualifiers);
} // namespace subathon::core
