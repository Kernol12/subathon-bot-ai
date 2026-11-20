// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdexcept>
#include <subathon/core/rule_selection.hpp>

namespace subathon::core {

std::vector<RulePriorityTie> validate_rule_selection(const RuleConfiguration &) {
    throw std::logic_error("RULES-002 NI: validate_rule_selection");
}

RuleSelection select_rules(const RuleConfiguration &, const RuleCapturedInput &,
                           std::span<const RuleExplicitValuation>) {
    throw std::logic_error("RULES-002 NI: select_rules");
}

} // namespace subathon::core
