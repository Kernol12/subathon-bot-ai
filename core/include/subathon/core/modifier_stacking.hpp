// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <subathon/core/modifier_activation.hpp>
#include <variant>
#include <vector>

namespace subathon::core {
// MODIFIER-003, frozen revision 3. Keys include kind even when references coincide.
struct ModifierTargetKey {
    ModifierTargetKind kind;
    std::string ref;
    bool operator==(const ModifierTargetKey &) const = default;
};

struct ModifierOccurrenceKey {
    std::string instance_ref;
    std::string instance_revision_ref;
    std::string definition_revision_ref;
    std::size_t target_ordinal; // One-based position in the captured definition.
    bool operator==(const ModifierOccurrenceKey &) const = default;
};

struct QualifiedModifierOccurrence {
    ModifierOccurrenceKey key;
    ModifierTargetKey target;
    std::string original_factor_decimal;
    Exact factor;
    bool operator==(const QualifiedModifierOccurrence &) const = default;
};

enum class ModifierStackOperator { multiply, additive, highest, lowest, priority };

struct ModifierPriority {
    std::int64_t rank; // Larger wins.
    std::size_t order; // Earlier wins among equal ranks.
    bool operator==(const ModifierPriority &) const = default;
};

struct ModifierMemberPolicy {
    ModifierOccurrenceKey occurrence;
    std::optional<ModifierPriority> priority;
};

struct ModifierGroupPolicy {
    std::string group_ref;
    std::optional<ModifierStackOperator> operation;
    std::optional<ModifierPriority> priority;  // Group ranking within its target.
    std::vector<ModifierMemberPolicy> members; // Immutable captured order.
};

struct ModifierTargetPolicy {
    ModifierTargetKey target;
    std::optional<ModifierStackOperator> operation;
    std::vector<ModifierGroupPolicy> groups; // Immutable captured order.
};

struct ModifierStackPolicy {
    std::string policy_ref;
    std::string configuration_ref;
    std::vector<ModifierTargetPolicy> targets;
};

inline constexpr unsigned modifier_component_bits = 4096;
inline constexpr unsigned modifier_temporary_bits = 8193;

struct ModifierStackLimits {
    // The owning caller supplies the effective aggregate and work budgets.
    std::size_t input_bytes;
    std::size_t output_bytes;
    std::size_t operations;
    std::size_t occurrences = 1024;
    std::size_t policy_entries = 4096;
    std::size_t targets = 1024;
    std::size_t requests = 1024;
    std::size_t materialized_rows = 4096; // Remaining full-find rows, after unrelated rows.
    std::size_t reference_bytes = 4096;
    std::size_t value_bytes = 262144;
    std::size_t exact_component_bytes = 16384;
};

struct ModifierReduction {
    ModifierStackOperator operation;
    Exact raw;
    Exact factor;
};

struct ModifierMemberTrace {
    QualifiedModifierOccurrence occurrence;
    std::optional<ModifierPriority> priority;
    bool winner; // Selection operators record every equal extremum.
};

struct ModifierGroupTrace {
    std::string group_ref;
    std::optional<ModifierPriority> priority;
    ModifierReduction reduction;
    std::vector<ModifierMemberTrace> members;
    bool winner; // Decision at the target level.
};

struct ModifierTargetTrace {
    ModifierTargetKey target;
    std::string policy_ref;
    ModifierReduction reduction;
    std::vector<ModifierGroupTrace> groups;
};

enum class ModifierStackFailure { unresolved, invalid, resource_limit, numeric_overflow };

class ModifierCombinationNotImplemented final : public std::logic_error {
  public:
    ModifierCombinationNotImplemented()
        : std::logic_error(
              "Modifier combination is intentionally unimplemented; review tests first") {}
};

using ModifierCombination = std::variant<ModifierStackFailure, std::vector<ModifierTargetTrace>>;

// Pure two-level calculation. Requested targets with no operands have identity. Qualified
// occurrences also get reduced and validated when no action uses their target. Policy order
// drives folds and traces. In a resolved policy, omitted operators mean Multiply. Preflight
// the complete shape and effective bounds. A failure carries no factor or trace.
[[nodiscard]] ModifierCombination
combine_modifiers(std::span<const QualifiedModifierOccurrence> occurrences,
                  const ModifierStackPolicy &policy, std::span<const ModifierTargetKey> targets,
                  const ModifierStackLimits &limits);
} // namespace subathon::core
