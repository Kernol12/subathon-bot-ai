// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <map>
#include <set>
#include <tuple>
#include <type_traits>

namespace subathon::server::rule_simulator {
namespace {
namespace sizing {
using namespace core;
using namespace core::rule_chain;

// Inline storage is charged by the enclosing value. List entries and dynamic
// backing are charged where represented, including duplicated trace evidence.
#define F(Type, ...)                                                                               \
    inline auto fields(const Type &v) { return std::tie(__VA_ARGS__); }
F(Scenario, v.ref, v.input, v.current)
F(CapturedReplay, v.capture, v.inputs, v.attempted_manual_modifiers)
F(Hypothetical, v.capture, v.inputs)
F(CurrentSources, v.configuration, v.modifier, v.rates)
F(ScenarioInputs, v.initial, v.roots, v.captures, v.milestone_captures, v.turns)
F(ScenarioState, v.pause, v.milestone_track, v.state_revision, v.current_modifier)
F(PauseState, v.reasons)
F(EventCapture, v.binding, v.configuration, v.receipt, v.eligibility, v.valuation, v.facts,
  v.authors, v.rates, v.explicit_valuations, v.modifiers)
F(RootSelection, v.root, v.selection)
F(OwnerTurn, v.ref, v.input)
F(EventTurn, v.event, v.prepared, v.chat, v.named, v.effect_refs)
F(PreparedPlan, v.selected_history, v.preview)
F(ChatObservation, v.action, v.sender, v.text, v.queue_acceptance)
F(RefundTurn, v.event, v.original_action, v.original_event, v.amounts)
F(RefundAmounts, v.original, v.previously_accepted, v.incoming)
F(TimelineTurn, v.head)
F(GroupObservation, v.head)
F(GroupReadback, v.attempt_turn, v.attempted, v.committed_pending_suffix)
F(MilestoneCapture, v.occurrence, v.actions, v.released_event)
F(UnitReconciliation, v.chain, v.unit, v.selected)
F(RestartTurn, v.basis, v.checkpoint, v.restart, v.gap, v.schedule, v.retained_chain)
F(EmptyRecoverySchedule, v.facts_ref, v.delivery_revision)
F(RecoveryBasis, v.timer_lineage, v.checkpoint_ref, v.configuration_revision, v.facts_bundle_ref)
F(RecoveryUtcObservation, v.observation_ref)
F(RecoveryContinuousGap, v.evidence_ref)
F(ConnectorReport, v.capabilities)
F(ConnectorCapabilities, v.events, v.features)
F(ConnectorEventCapability, v.fields)
F(DeliveryTurn, v.selected_job, v.observation)

F(ChainKey, v.installation, v.root)
F(EventBinding, v.chain, v.event, v.parent, v.capture, v.cause)
F(NamedAction, v.key)
F(MilestoneOccurrence, v.source, v.occurrence)
F(RootBinding, v.event, v.normalized_occurrence)
F(VerifiedNew, v.proposed_admission)
F(ModeledHistory, v.root, v.admission, v.events, v.publications, v.cutoff, v.blocked)
F(AdmittedEvent, v.binding)
F(Cutoff, v.chain, v.root_capture, v.last)
F(BlockedUnit, v.identity)
F(UnitIdentity, v.unit, v.chain, v.anchor)
F(Publication, v.identity, v.pass, v.children, v.occurrences, v.named_suppressions, v.cutoff)
F(OccurrenceDisposition, v.occurrence, v.parent, v.disposition, v.suppressed_binding)
F(GenerationSuppressed, v.stop)
F(NamedSuppression, v.action, v.stop)
F(RuleCommittedPass, v.rules)
F(RuleStagedPass, v.rules)
F(RuleConsumedActions, v.accepted, v.stop)
F(RuleAcceptedAction, v.key, v.observed)
F(RuleEventLineage, v.event_ref, v.parent_ref, v.root_ref, v.cause)
F(RuleEffectObservation, v.effect_ref, v.change)
F(RuleQuantityObservation, v.unit, v.requested, v.actual)
F(RuleDeliveryObservation, v.job_ref, v.evidence_ref)
F(RuleStop, v.detail)
F(RuleStopDetail, v.limit, v.opaque_reason)
F(PermissionRational, v.numerator, v.denominator)

F(RuleConfiguration, v.data())
F(RuleConfigurationInput, v.configuration_ref, v.rules)
F(RuleDefinition, v.rule_ref, v.version_ref, v.name, v.description, v.exclusive_group_ref,
  v.excluded_rule_refs, v.common_condition, v.triggers, v.actions)
F(RuleTrigger, v.condition)
F(RuleAll, v.children)
F(RuleAny, v.children)
F(RuleActor, v.scope, v.actor_id)
F(RuleActorComparison, v.operand)
F(RuleActionDefinition, v.action_ref, v.version_ref, v.payload)
F(RuleDeltaAction, v.target_ref, v.delta)
F(RuleFixedDelta, v.original_decimal)
F(RuleCalculation, v.calculation_version_ref)
F(RuleActivateModifier, v.definition_version_ref)
F(RuleExtendModifier, v.modifier_ref)
F(RuleTerminateModifier, v.modifier_ref)
F(RuleChatAction, v.route_ref, v.template_version_ref, v.fallback_policy_version_ref)
F(RuleAnnouncement, v.presentation_version_ref)
F(RuleAnimation, v.animation_version_ref)
F(RuleTimerControl, v.timer_ref, v.command_version_ref)
F(RuleNamedEvent, v.event_name, v.payload_version_ref)
F(RuleConnectorAction, v.connection_ref, v.operation_ref, v.parameters_version_ref)
F(RuleExplicitValuation, v.resolution_ref, v.event_ref, v.original, v.comparison_currency,
  v.numerator, v.denominator)

F(EffectiveRateSnapshot, v.data())
F(EffectiveRateSnapshotData, v.snapshot_ref, v.parent_snapshot_ref, v.sources, v.overrides,
  v.constraints, v.components)
F(FetchedRateObservation, v.data())
F(FetchedRateObservationData, v.context, v.quotes)
F(RateObservationContext, v.observation_ref, v.provider_ref, v.configuration_ref, v.request_ref,
  v.source_evidence_ref, v.provider_snapshot_ref, v.source_rate_time, v.publication_time)
F(RateTimeEvidence, v.original_text)
F(RateQuote, v.from_currency, v.to_currency, v.rate)
F(ExactDecimal, v.original(), v.value())
F(ManualRateOverrideData, v.override_ref, v.from_currency, v.to_currency, v.rate, v.decision_ref,
  v.actor_ref, v.reason)
F(FetchedRateConstraint, v.observation_ref)
F(ManualRateConstraint, v.override_ref)
F(RateComponent, v.members)
F(RatePotential, v.currency, v.weight)

F(PlanEvent, v.event_ref, v.parent_event_ref, v.root_event_ref, v.identity, v.facts, v.arrival)
F(ReceiptMoney, v.original_decimal, v.currency)
F(ReceiptOrigin, v.provider, v.source_account)
F(ReceiptIdentity, v.origin, v.identifier_namespace, v.opaque_id)
F(ReceiptLinkReference, v.identifier_namespace, v.opaque_id)
F(ReceiptFacts, v.event_type, v.actor_id, v.actor_display_name, v.money, v.payment_reference,
  v.activity_reference)
F(ReceiptArrival, v.connection_ref, v.configuration_ref, v.modifier_context_ref)
F(ContributionState, v.conditions)
F(ContributionConditions, v.valuation, v.evidence_admission)
F(ContributionValuation, v.original_money, v.resolution_ref)
F(EvidenceAdmission, v.policy_ref, v.review_decision)
F(EvidenceReviewDecision, v.decision_ref, v.actor_ref)
F(PlanValuation, v.historical)
F(HistoricalValuation, v.resolution_ref, v.original_money, v.comparison_currency,
  v.effective_rate_decimal, v.fetched_rate_decimal, v.snapshot_ref, v.source_ref,
  v.manual_decision_ref)
F(PlanCapture, v.event, v.contribution_state, v.valuation, v.requested_effects)
F(ApplicationPlan, v.plan_ref, v.revisions)
F(CapturedPlanRevision, v.captured, v.rules, v.modifiers, v.override_decision)
F(PlanOverrideDecision, v.actor_ref, v.reason, v.decision_ref)
F(PlanPreview, v.authoritative_state_revision)
F(RequestedEffect, v.action_ref, v.action_version_ref, v.rule_ref, v.target_ref,
  v.exact_requested_units, v.calculation_explanation, v.reversal_behavior_ref)
F(RuleSelection, v.rule_ref, v.version_ref, v.explanation)
F(ModifierSelection, v.definition_version_ref, v.instance_revision_ref, v.target_ref, v.group_ref,
  v.group_operator, v.target_operator, v.original_factor_decimal)

F(ModifierClockValue, v.revision)
F(ModifierInstance, v.instance_ref, v.revision_ref, v.parent_revision_ref, v.captured_definition,
  v.source, v.trigger)
F(ModifierDefinition, v.definition_ref, v.revision_ref, v.targets)
F(ModifierTarget, v.target_ref, v.qualifier_ref, v.factor_decimal)
F(ModifierActivationSource, v.source_ref)
F(ModifierTriggeringContribution, v.event_ref)
F(SelectedModifiers, v.context, v.admitted_profile, v.policy, v.eligible, v.matched_qualifiers)
F(CapturedModifierContext, v.document_ref, v.configuration_ref, v.policy_ref, v.profile,
  v.recalculation)
F(ModifierTraceProfile, v.producer_ref, v.profile_ref)
F(ModifierRecalculationBinding, v.plan_ref, v.original_arrival_ref, v.decision_ref)
F(ModifierStackPolicy, v.policy_ref, v.configuration_ref, v.targets)
F(ModifierTargetPolicy, v.target, v.groups)
F(ModifierTargetKey, v.ref)
F(ModifierGroupPolicy, v.group_ref, v.members)
F(ModifierMemberPolicy, v.occurrence)
F(ModifierOccurrenceKey, v.instance_ref, v.instance_revision_ref, v.definition_revision_ref)
F(ModifierStackProposal, v.context_ref, v.profile, v.selection, v.links, v.targets, v.effects)
F(ModifierTraceLink, v.occurrence)
F(ModifierTargetTrace, v.target, v.policy_ref, v.reduction, v.groups)
F(ModifierReduction, v.raw, v.factor)
F(ModifierGroupTrace, v.group_ref, v.reduction, v.members)
F(ModifierMemberTrace, v.occurrence)
F(QualifiedModifierOccurrence, v.key, v.target, v.original_factor_decimal, v.factor)
F(ModifierScaledEffect, v.base, v.exact_units)
F(ModifierBaseEffect, v.action_ref, v.action_version_ref, v.rule_ref, v.target, v.exact_units,
  v.reversal_behavior_ref)

F(MilestoneProgressionInput, v.track_ref, v.definition_revision_ref, v.progression_context_ref,
  v.targets, v.completed_prefix)
F(MilestoneTarget, v.milestone_ref)
F(MilestoneCompletion, v.milestone_ref, v.definition_revision_ref, v.completion_context_ref)
F(MilestoneProgressionData, v.track_ref, v.definition_revision_ref, v.progression_context_ref,
  v.completed_prefix, v.projection)
F(MilestoneProjection, v.completed_deficits, v.previous, v.next, v.interval)
F(MilestoneDeficit, v.current_target)
F(MilestoneInterval, v.fraction)
F(CommittedMilestoneGroup, v.achievement, v.actions, v.occurrence, v.edit_membership)
F(MilestoneOccurrenceKey, v.ref)
F(MilestoneAction, v.action_ref, v.version_ref)
F(MilestoneEditMembership, v.origin, v.jobs)
F(MilestoneEditKey, v.track, v.operation_ref)
F(MilestoneTrackKey, v.installation_ref, v.track_ref)
F(MilestoneEditJobKey, v.operation, v.occurrence)
F(ActiveMilestonePresentation, v.milestone_ref)
F(GroupReleaseTrace, v.occurrence)

F(TurnTrace, v.ref, v.state, v.configuration, v.selection, v.actions, v.quote, v.plan, v.modifiers,
  v.admitted, v.retained_cutoff, v.publication, v.cutoff_audit_ref, v.cutoff_notice_ref,
  v.progression, v.presentation, v.group_head, v.releases, v.delivery, v.refund_receipt, v.refund,
  v.reversal, v.timer_cut, v.connector, v.recovery)
F(ActionTrace, v.intent, v.gates, v.stop, v.proposed, v.accepted, v.message)
F(RuleActionPlan, v.event, v.selection, v.selected)
F(RuleSelectionResult, v.configuration_ref, v.event_ref, v.decisions, v.selected_rule_indices,
  v.priority_ties)
F(RulePriorityTie, v.rule_indices)
F(RuleSelectedActions, v.original_author, v.intents)
F(RuleActionIntent, v.key, v.captured_payload, v.binding)
F(RuleActionKey, v.rule_ref, v.rule_version, v.action_ref, v.action_version)
F(RuleBoundAction, v.request, v.payload)
F(PermissionRequest, v.operation, v.resources, v.quantities)
F(PermissionResourceBinding, v.slot, v.resource)
F(PermissionQuantityBinding, v.slot, v.unit, v.value)
F(RuleProjectedDelivery, v.route_ref, v.sender_ref)
F(RuleProjectedChild, v.name)
F(MessageIntent, v.route, v.sender, v.template_version, v.synthetic_text)
F(QuoteTrace, v.provenance, v.rate, v.converted)
F(PlanTrace, v.inspected, v.head_requests)
F(RuleDeliveryTrace, v.queued, v.latest)
F(RefundTimeEntitlement, v.total)
F(RefundEntitlement, v.cumulative_fraction, v.exact_entitlement, v.rounding_residual)
F(TimeReversalResult, v.request)
F(TimeReversalRequest, v.reversal_ref, v.original_effect_ref)
F(TimerAdvance, v.transitions)
F(ConnectorStatus, v.setup)
F(ConnectorSetupGuidance, v.callback_url)
F(DowntimeRecoveryPreview, v.basis, v.freeze, v.count_elapsed, v.review_hold)
F(RecoveryTimerProposal, v.pause, v.transitions, v.progression)
#undef F

template <class Field, class Value> auto fields(const RuleFact<Field, Value> &v) {
    return std::tie(v.value);
}
template <class Fact, class Operator> auto fields(const RuleComparison<Fact, Operator> &v) {
    return std::tie(v.operand);
}
template <class A, class B> auto fields(const std::pair<A, B> &v) {
    return std::tie(v.first, v.second);
}
} // namespace sizing

struct BudgetExceeded {};

struct Budget {
    std::size_t bytes;
    std::size_t rows;

    void charge(std::size_t total_bytes, std::size_t total_rows) {
        if (total_bytes > bytes || total_rows > rows)
            throw BudgetExceeded{};
        bytes -= total_bytes;
        rows -= total_rows;
    }

    void take(std::size_t size, std::size_t count = 1) {
        if (count > bytes / size)
            throw BudgetExceeded{};
        charge(size * count, count);
    }

    template <class T> void entry(const T &value) {
        take(sizeof(T));
        dynamic(value);
    }

  private:
    void dynamic(const std::string &value) { take_payload(1, value.size()); }

    void take_payload(std::size_t size, std::size_t count) {
        if (count > bytes / size)
            throw BudgetExceeded{};
        bytes -= count * size;
    }

    void dynamic(const core::Exact &value) {
        const auto charge = [this](const auto &num, const auto &denom) {
            take_payload(sizeof(*num.limbs()), num.size());
            take_payload(sizeof(*denom.limbs()), denom.size());
        };
        [&]<class Backend>(const Backend &fraction) {
            if constexpr (requires {
                              fraction.num();
                              fraction.denom();
                          })
                charge(fraction.num(), fraction.denom());
            else
                charge(fraction.data().numerator().backend(),
                       fraction.data().denominator().backend());
        }(value.backend());
    }

    template <class Range> void sequence(const Range &values) {
        take(sizeof(typename Range::value_type), values.size());
        for (const auto &value : values)
            dynamic(value);
    }

    template <class T> void dynamic(const std::vector<T> &values) { sequence(values); }
    template <class... T> void dynamic(const std::set<T...> &values) { sequence(values); }
    template <class... T> void dynamic(const std::map<T...> &values) { sequence(values); }
    template <class T> void dynamic(const std::optional<T> &value) {
        if (value)
            dynamic(*value);
    }
    template <class... T> void dynamic(const std::variant<T...> &value) {
        std::visit([this](const auto &selected) { dynamic(selected); }, value);
    }
    template <class T> void dynamic(const T &value) {
        if constexpr (requires { sizing::fields(value); })
            std::apply([this](const auto &...part) { (dynamic(part), ...); },
                       sizing::fields(value));
        else
            static_assert(std::is_trivially_copyable_v<T>, "Missing owned-storage projection");
    }
};
} // namespace
} // namespace subathon::server::rule_simulator
