// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/rule_actions.hpp>

namespace subathon::core {

RuleActionPlan plan_rule_actions(const RuleConfiguration &, const RuleCapturedInput &,
                                 const RuleEventLineage &, std::span<const std::string>,
                                 std::span<const RuleExplicitValuation>, const RuleActionMapper &) {
    throw RuleActionsNotImplemented{};
}

std::optional<RuleStop> disclose_rule_stop(const RuleStop &, RuleStopDisclosure) {
    throw RuleActionsNotImplemented{};
}

RuleSimulation simulate_rule_actions(const RuleActionPlan &, const RuleActionChecks &,
                                     const RuleActionProjector &) {
    throw RuleActionsNotImplemented{};
}

RuleDeliveryTrace observe_rule_delivery(const RuleDeliveryTrace &,
                                        const std::optional<RuleDeliveryObservation> &) {
    throw RuleActionsNotImplemented{};
}

RulePass consume_rule_actions(const RuleActionPlan &, const RuleActionChecks &,
                              const RuleActionSubmit &) {
    throw RuleActionsNotImplemented{};
}

std::optional<RuleCommittedPass> publish_rule_pass(const RuleStagedPass &, RuleCompletion) {
    throw RuleActionsNotImplemented{};
}

} // namespace subathon::core
