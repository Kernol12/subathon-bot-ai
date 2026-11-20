// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <subathon/core/rule_definition.hpp>
#include <vector>

namespace subathon::core {
// RULES-002 contract revision 1. All indices address the captured rules vector.
struct RulePriorityTie {
    // At least two enabled members with the same group and priority, in user order.
    // The first member identifies the group and priority.
    std::vector<std::size_t> rule_indices;
    bool operator==(const RulePriorityTie &) const = default;
};

enum class RuleSelectionDisposition {
    selected,
    disabled,
    condition_no_match,
    condition_unknown,
    excluded,
    exclusive_group
};

struct RuleSelectionDecision {
    RuleSelectionDisposition disposition;
    // Present only for excluded and exclusive_group. This is the highest-ranked
    // selected rule that blocks the match. If it blocks by both mechanisms,
    // the disposition is excluded.
    std::optional<std::size_t> blocking_rule_index;
    bool operator==(const RuleSelectionDecision &) const = default;
};

struct RuleSelection {
    std::string configuration_ref;
    std::string event_ref;
    std::vector<RuleSelectionDecision> decisions;   // One per rule, in captured user order.
    std::vector<std::size_t> selected_rule_indices; // In descending rank.
    std::vector<RulePriorityTie> priority_ties;
};

// Higher signed priority ranks first. Equal priorities rank by earlier index in
// the captured user order, including across groups. Compare reference bytes exactly.
// Before enabling a configuration, check every stored exclusion, including those
// on or targeting disabled rules. Each target must resolve in this configuration
// and rank strictly below its source. This rejects self-edges and every cycle.
// Repeated references to the same target have the effect of a single edge.
// Invalid edges raise RuleDefinitionFailure with invalid_exclusion.
//
// Warnings group enabled exclusive-group members by exact group and numeric priority,
// independent of event eligibility. Warning records follow the rank of their first
// member. A warning describes a potential tie. Selection identifies the actual winner.
[[nodiscard]] std::vector<RulePriorityTie>
validate_rule_selection(const RuleConfiguration &configuration);

// Validate the captured configuration's graph as above and reuse RULES-001 capture,
// valuation and condition semantics. Only enabled rules with a definite match compete.
// Visit those matches in rank order. A match is selected when no higher-ranked selected
// rule explicitly excludes it and no selected rule claims its exclusive group.
// Only selected rules claim groups and apply exclusions. The selection and each rule's
// captured group identify the actual group winner in the preview. Ungrouped matches
// remain independent unless explicitly excluded.
//
// Return the captured configuration and event refs with the same priority warnings
// as validate_rule_selection. Keep the result with that configuration. Rule/action
// versions and action order remain in the captured configuration. Selection grants
// no execution authority and performs no actions. Refusals return no partial result.
[[nodiscard]] RuleSelection select_rules(const RuleConfiguration &configuration,
                                         const RuleCapturedInput &input,
                                         std::span<const RuleExplicitValuation> valuations = {});
} // namespace subathon::core
