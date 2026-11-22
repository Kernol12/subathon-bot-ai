// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/rule_chain.hpp>

namespace subathon::core::rule_chain {

struct Chain::State {};
struct Unit::State {};
Chain::Chain() = default;
Chain::Chain(Chain &&) noexcept = default;
Chain &Chain::operator=(Chain &&) noexcept = default;
Chain::~Chain() = default;
Unit::Unit() = default;
Unit::Unit(Unit &&) noexcept = default;
Unit &Unit::operator=(Unit &&) noexcept = default;
Unit::~Unit() = default;

PolicyCheck check_generation_policy(const RuleConfiguration &, const ChainProfile &) {
    throw RuleChainNotImplemented("check_generation_policy");
}
OpenResult open_chain(const RootBinding &, const ChainProfile &, const OwnerSelection &,
                      OwnerView) {
    throw RuleChainNotImplemented("open_chain");
}
BeginResult begin_event(Chain &, const EventBinding &, std::string) {
    throw RuleChainNotImplemented("begin_event");
}
BeginResult begin_occurrences(Chain &, const std::string &, std::string) {
    throw RuleChainNotImplemented("begin_occurrences");
}
RuleActionPlan plan_event(const Unit &, const RuleActionMapper &) {
    throw RuleChainNotImplemented("plan_event");
}
ChildLookup lookup_child(const Chain &, const EventBinding &) {
    throw RuleChainNotImplemented("lookup_child");
}
ChildDecision offer_named_child(Unit &, const EventBinding &, const RuleActionKey &,
                                const RuleNamedEvent &) {
    throw RuleChainNotImplemented("offer_named_child");
}
NamedConfirmation confirm_named_child(Unit &, const AdmittedEvent &, const NamedOutcome &) {
    throw RuleChainNotImplemented("confirm_named_child");
}
RuleActionSubmit chain_action_submit(Unit &, NamedProposal, NamedSubmit, RuleActionSubmit) {
    throw RuleChainNotImplemented("chain_action_submit");
}
ChildDecision offer_occurrence_child(Unit &, const MilestoneOccurrence &,
                                     const std::optional<EventBinding> &, OccurrenceState) {
    throw RuleChainNotImplemented("offer_occurrence_child");
}
UnitResult finish_unit(Unit &&, const std::optional<RuleStagedPass> &, RuleCompletion) {
    throw RuleChainNotImplemented("finish_unit");
}
UnitResult reconcile_unit(Chain &, const std::string &, const UnitReadback &) {
    throw RuleChainNotImplemented("reconcile_unit");
}

RuleChainNotImplemented::RuleChainNotImplemented(const char *entry)
    : std::logic_error(std::string("RULES-004 intentionally unimplemented: ") + entry) {}

} // namespace subathon::core::rule_chain
