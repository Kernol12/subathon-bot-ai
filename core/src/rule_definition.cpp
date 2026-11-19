// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdexcept>
#include <subathon/core/rule_definition.hpp>

namespace subathon::core {
RuleConfiguration::RuleConfiguration(const RuleConfigurationInput &input) : data_(input) {}
RuleConfiguration::RuleConfiguration(const RuleConfiguration &) = default;

const RuleConfigurationInput &RuleConfiguration::data() const noexcept { return data_; }

RuleDefinitionFailure::RuleDefinitionFailure(RuleDefinitionError code) noexcept : code_(code) {}

RuleDefinitionError RuleDefinitionFailure::code() const noexcept { return code_; }

const char *RuleDefinitionFailure::what() const noexcept { return "Rule definition refused"; }

RuleConfiguration validate_rule_configuration(const RuleConfigurationInput &) {
    throw std::logic_error("RULES-001 validate_rule_configuration not implemented");
}

std::vector<std::optional<RuleTruth>>
evaluate_rule_conditions(const RuleConfigurationInput &, const RuleCapturedInput &,
                         std::span<const RuleExplicitValuation>) {
    throw std::logic_error("RULES-001 evaluate_rule_conditions not implemented");
}
} // namespace subathon::core
