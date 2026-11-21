// SPDX-License-Identifier: GPL-3.0-or-later
#include "permission_engine.hpp"

#include <algorithm>
#include <subathon/core/rule_actions.hpp>
#include <tuple>
#include <type_traits>

namespace subathon::core {
namespace {
using Refusal = RuleActionRefusal;
using Channel = RuleAuthorityChannel;
using PermissionError = EffectivePermissionError;
namespace permission = permission_engine;

constexpr EffectivePermissionLimits limits{};

bool reference(std::string_view value) {
    return !value.empty() && value.size() <= limits.reference_bytes;
}

bool action_identity(const RuleActionKey &key) {
    return reference(key.rule_ref) && reference(key.rule_version) && reference(key.action_ref) &&
           reference(key.action_version);
}

bool valid_lineage(const RuleEventLineage &event) {
    if (!reference(event.event_ref) || !reference(event.root_ref))
        return false;
    if (!event.parent_ref)
        return event.root_ref == event.event_ref && !event.cause;
    return reference(*event.parent_ref) && *event.parent_ref != event.event_ref &&
           event.root_ref != event.event_ref && (!event.cause || action_identity(*event.cause));
}

template <class T> auto immutable_parameters(const T &action) {
    if constexpr (std::is_same_v<T, RuleActivateModifier>)
        return std::tie(action.definition_version_ref, action.timing);
    else if constexpr (std::is_same_v<T, RuleExtendModifier>)
        return std::tie(action.modifier_ref, action.duration_ms);
    else if constexpr (std::is_same_v<T, RuleTerminateModifier>)
        return std::tie(action.modifier_ref);
    else if constexpr (std::is_same_v<T, RuleTimerControl>)
        return std::tie(action.timer_ref, action.command_version_ref);
    else if constexpr (std::is_same_v<T, RuleAnnouncement>)
        return std::tie(action.presentation_version_ref);
    else if constexpr (std::is_same_v<T, RuleAnimation>)
        return std::tie(action.animation_version_ref);
    else if constexpr (std::is_same_v<T, RuleNamedEvent>)
        return std::tie(action.event_name, action.payload_version_ref);
    else
        return std::tie(action.connection_ref, action.operation_ref, action.parameters_version_ref);
}

bool same_delta(const RuleDeltaAction &captured, const RuleDeltaAction &mapped) {
    if (captured.unit != mapped.unit || captured.target_ref != mapped.target_ref)
        return false;
    if (const auto *fixed = std::get_if<RuleFixedDelta>(&captured.delta)) {
        const auto *value = std::get_if<RuleFixedDelta>(&mapped.delta);
        return value && value->original_decimal == fixed->original_decimal;
    }
    const auto *calculation = std::get_if<RuleCalculation>(&captured.delta);
    const auto *value = std::get_if<RuleCalculation>(&mapped.delta);
    return calculation && value &&
           value->calculation_version_ref == calculation->calculation_version_ref;
}

bool same_payload(const RuleActionPayload &captured, const RuleActionPayload &mapped) {
    if (captured.valueless_by_exception())
        return false;
    return std::visit(
        [&](const auto &action) {
            using Action = std::decay_t<decltype(action)>;
            const auto *bound = std::get_if<Action>(&mapped);
            if (!bound)
                return false;
            if constexpr (std::is_same_v<Action, RuleDeltaAction>) {
                return same_delta(action, *bound);
            } else if constexpr (std::is_same_v<Action, RuleChatAction>) {
                const bool resolved_default = action.sender == RuleChatSender::configured_default &&
                                              (bound->sender == RuleChatSender::bot ||
                                               bound->sender == RuleChatSender::broadcaster);
                return (action.sender == bound->sender || resolved_default) &&
                       action.route_ref == bound->route_ref &&
                       action.template_version_ref == bound->template_version_ref &&
                       action.fallback_policy_version_ref == bound->fallback_policy_version_ref;
            } else {
                return immutable_parameters(action) == immutable_parameters(*bound);
            }
        },
        captured);
}

bool unique_slots(const auto &bindings) {
    for (auto it = bindings.begin(); it != bindings.end(); ++it) {
        if (std::find_if(bindings.begin(), it,
                         [&](const auto &other) { return other.slot == it->slot; }) != it)
            return false;
    }
    return true;
}

// The mapper owns operation-specific meaning and resolves the concrete sender.
// Here we check the retained payload and the shape/encoding of the exact request.
bool valid_binding(const RuleActionPayload &captured, const RuleBoundAction &bound) {
    const auto &request = bound.request;
    try {
        permission::Count budget{0,
                                 0,
                                 limits.input_owned_bytes,
                                 limits.input_items,
                                 limits.reference_bytes,
                                 limits.component_bytes,
                                 PermissionError::input_limit};
        budget.request(request);
        if (budget.empty_reference || !same_payload(captured, bound.payload) ||
            !unique_slots(request.resources) || !unique_slots(request.quantities))
            return false;
        if (std::holds_alternative<RuleChatAction>(captured) && request.resources.empty())
            return false;
        if ((std::holds_alternative<RuleDeltaAction>(captured) ||
             std::holds_alternative<RuleExtendModifier>(captured)) &&
            request.quantities.empty())
            return false;
        for (const auto &quantity : request.quantities)
            permission::validate_number(quantity.value);
        return true;
    } catch (const EffectivePermissionFailure &) {
        return false;
    }
}

DelegatedPermissionGrant singleton(const PermissionRequest &request) {
    DelegatedPermissionGrant point{request.operation, {}, {}};
    point.resources.reserve(request.resources.size());
    point.quantities.reserve(request.quantities.size());
    for (const auto &resource : request.resources)
        point.resources.push_back(
            {resource.slot, PermissionResourceMode::allowed_set, {resource.resource}});
    for (const auto &quantity : request.quantities)
        point.quantities.push_back({quantity.slot, quantity.unit,
                                    PermissionQuantityMode::closed_interval, quantity.value,
                                    quantity.value});
    return point;
}

const PermissionOperationDefinition &validate_point(const std::string &author,
                                                    const RuleActionAuthorityFacts &facts,
                                                    const PermissionRequest &request) {
    const std::vector<std::string> no_roles;
    const permission::Policy no_policy{PermissionFactsState::complete, no_roles, {}};
    const permission::Input input{author,      facts.snapshot, facts.catalog,
                                  facts.owner, no_policy,      request};
    if (permission::preflight_input(input, limits))
        permission::fail(PermissionError::invalid_reference);
    permission::validate_catalog_tags(facts.catalog);
    permission::validate_owner_tags(facts.owner);
    if (facts.catalog.state == PermissionFactsState::unknown ||
        facts.owner.state == PermissionOwnerState::unknown)
        permission::fail(PermissionError::facts_unavailable);
    permission::validate_catalog_shape(facts.catalog);
    const auto op = std::ranges::find(facts.catalog.operations, request.operation,
                                      &PermissionOperationDefinition::operation);
    permission::validate_request_shape(op == facts.catalog.operations.end() ? nullptr : &*op,
                                       request);
    for (const auto &operation : facts.catalog.operations)
        for (const auto &quantity : operation.quantity_slots) {
            if (quantity.minimum)
                permission::validate_number(*quantity.minimum);
            if (quantity.maximum)
                permission::validate_number(*quantity.maximum);
        }
    // #249 capability containment intersects with the catalog domain. A concrete
    // out-of-domain request must fail instead of being contained as an empty set.
    permission::validate_domains(input, *op);
    return *op;
}

bool covers_resources(const PermissionPolicyEntry &row, const PermissionRequest &request) {
    return row.operation == request.operation &&
           std::ranges::all_of(request.resources, [&](const auto &resource) {
               const auto slot = std::ranges::find(row.resources, resource.slot,
                                                   &PermissionResourceConstraint::slot);
               return slot != row.resources.end() &&
                      permission::resource_contains(*slot, resource.resource);
           });
}

// Called only on validated facts. Follow individual precedence and report a
// bound only when one effective row supplies one unambiguous quantity constraint.
std::optional<PermissionRational> denial_limit(const PermissionPolicyFacts &policy,
                                               const PermissionRequest &request) {
    const bool individual = std::ranges::any_of(policy.entries, [&](const auto &row) {
        return row.origin == PermissionOrigin::individual && covers_resources(row, request);
    });
    const PermissionPolicyEntry *applicable = nullptr;
    for (const auto &row : policy.entries) {
        if ((row.origin == PermissionOrigin::individual) != individual ||
            !covers_resources(row, request))
            continue;
        if (applicable)
            return {};
        applicable = &row;
    }
    if (!applicable || applicable->effect != PermissionEffect::allow)
        return {};
    const PermissionQuantityConstraint *constraint = nullptr;
    for (const auto &quantity : applicable->quantities) {
        if (quantity.mode == PermissionQuantityMode::any)
            continue;
        if (constraint)
            return {};
        constraint = &quantity;
    }
    return constraint ? constraint->upper : std::nullopt;
}

RuleStop stop_at(const RuleActionIntent &intent, Refusal reason,
                 std::optional<RuleStopDetail> detail = {}) {
    return {intent.key.position, reason, std::move(detail)};
}

// The checked snapshot is passed to the owner for an atomic currentness check.
using ActionCheck = std::variant<std::string, RuleStop>;

ActionCheck check_action(const RuleSelectedActions &rule, const RuleActionIntent &intent,
                         const RuleActionChecks &checks) {
    if (const auto *refusal = std::get_if<Refusal>(&intent.binding))
        return stop_at(intent, *refusal);
    const auto *bound = std::get_if<RuleBoundAction>(&intent.binding);
    if (!bound || !valid_binding(intent.captured_payload, *bound))
        return stop_at(intent, Refusal::invalid_binding);
    if (!reference(rule.original_author) || !checks.authority)
        return stop_at(intent, Refusal::authority_unavailable);

    std::optional<Channel> unavailable_channel;
    std::string snapshot;
    try {
        auto facts = checks.authority(rule.original_author, intent.key, *bound);
        if (!facts || !reference(facts->snapshot))
            return stop_at(intent, Refusal::authority_unavailable);

        const bool ordinary =
            validate_point(rule.original_author, *facts, bound->request).classification ==
            PermissionOperationClass::ordinary;
        if (facts->use.state == PermissionFactsState::unknown &&
            facts->grant.state == PermissionFactsState::complete)
            unavailable_channel = Channel::use;
        if (facts->grant.state == PermissionFactsState::unknown &&
            facts->use.state == PermissionFactsState::complete)
            unavailable_channel = Channel::grant;

        const PermissionAutomationInput input{
            rule.original_author,       std::move(facts->snapshot), std::move(facts->catalog),
            std::move(facts->owner),    std::move(facts->use),      std::move(facts->grant),
            {singleton(bound->request)}};
        const auto author_decision = revalidate_permission_automation(input).actions.front();
        if (author_decision.disposition != PermissionAutomationDisposition::authorized) {
            const bool use_denied = author_decision.use != PermissionOutcome::permit;
            RuleStopDetail detail{};
            detail.author_decision = author_decision;
            detail.channel = use_denied ? Channel::use : Channel::grant;
            if (ordinary)
                detail.limit =
                    denial_limit(use_denied ? input.use : input.delegation, bound->request);
            return stop_at(intent, Refusal::authorization_hold, std::move(detail));
        }
        snapshot = input.snapshot;
    } catch (const EffectivePermissionFailure &failure) {
        if (failure.code() == PermissionError::invalid_request)
            return stop_at(intent, Refusal::invalid_binding);
        RuleStopDetail detail{};
        detail.authority_error = failure.code();
        if (failure.code() == PermissionError::facts_unavailable)
            detail.channel = unavailable_channel;
        return stop_at(intent, Refusal::authority_unavailable, std::move(detail));
    }

    const auto execution =
        checks.execution ? checks.execution(*bound, snapshot) : RuleGate::unavailable;
    if (execution != RuleGate::permit) {
        RuleStopDetail detail{};
        detail.channel = Channel::execution;
        return stop_at(intent,
                       execution == RuleGate::deny ? Refusal::authorization_hold
                                                   : Refusal::authority_unavailable,
                       std::move(detail));
    }
    if (!checks.valid_target)
        return stop_at(intent, Refusal::unresolved_input);
    if (!checks.valid_target(*bound))
        return stop_at(intent, Refusal::invalid_target);
    const auto capability =
        checks.capability ? checks.capability(*bound) : RuleCapability::unavailable;
    if (capability != RuleCapability::supported)
        return stop_at(intent, capability == RuleCapability::unsupported
                                   ? Refusal::unsupported_capability
                                   : Refusal::capability_unavailable);
    return snapshot;
}

void validate_plan(const RuleActionPlan &plan) {
    if (!valid_lineage(plan.event) || plan.event.event_ref != plan.selection.event_ref ||
        !reference(plan.selection.configuration_ref) ||
        plan.selected.size() != plan.selection.selected_rule_indices.size())
        throw RuleDefinitionFailure{RuleDefinitionError::invalid_capture};

    std::vector<bool> seen(plan.selection.decisions.size());
    for (std::size_t i = 0; i < plan.selected.size(); ++i) {
        const auto &rule = plan.selected[i];
        if (rule.captured_index != plan.selection.selected_rule_indices[i] ||
            rule.captured_index >= plan.selection.decisions.size() || seen[rule.captured_index] ||
            plan.selection.decisions[rule.captured_index].disposition !=
                RuleSelectionDisposition::selected)
            throw RuleDefinitionFailure{RuleDefinitionError::invalid_capture};
        seen[rule.captured_index] = true;

        for (std::size_t position = 0; position < rule.intents.size(); ++position) {
            const auto &key = rule.intents[position].key;
            const auto &first = rule.intents.front().key;
            if (!action_identity(key) || key.position != position ||
                key.rule_ref != first.rule_ref || key.rule_version != first.rule_version)
                throw RuleDefinitionFailure{RuleDefinitionError::invalid_capture};
        }
    }
}

bool valid_delivery(const RuleDeliveryObservation &observation) {
    if (!reference(observation.job_ref) || observation.revision < 1 ||
        (observation.evidence_ref && !reference(*observation.evidence_ref)))
        return false;
    switch (observation.state) {
    case RuleDeliveryState::queued:
        return !observation.attempt &&
               observation.evidence_ref.has_value() == (observation.revision != 1);
    case RuleDeliveryState::sending:
        return observation.revision >= 2 && observation.attempt == observation.revision &&
               !observation.evidence_ref;
    case RuleDeliveryState::delivered:
    case RuleDeliveryState::failed:
    case RuleDeliveryState::outcome_unknown:
        return observation.revision >= 3 && observation.attempt == observation.revision - 1 &&
               observation.evidence_ref.has_value();
    }
    return false;
}

bool initial_queue(const RuleDeliveryObservation &observation) {
    return observation.state == RuleDeliveryState::queued && observation.revision == 1 &&
           valid_delivery(observation);
}

RuleOwnerObservation accepted_observation(const RuleEventLineage &event, const RuleActionKey &key,
                                          RuleOwnerObservation observation) {
    // Retain the owner's acceptance even if there is no usable observation.
    if (const auto *child = std::get_if<RuleEventLineage>(&observation)) {
        if (!valid_lineage(*child) || child->parent_ref != event.event_ref ||
            child->root_ref != event.root_ref || child->cause != key)
            return std::monostate{};
    } else if (const auto *job = std::get_if<RuleDeliveryObservation>(&observation)) {
        if (!initial_queue(*job))
            return std::monostate{};
    } else if (const auto *effect = std::get_if<RuleEffectObservation>(&observation)) {
        if (!reference(effect->effect_ref) || effect->change.valueless_by_exception())
            return std::monostate{};
    }
    if (observation.valueless_by_exception())
        return std::monostate{};
    return observation;
}
} // namespace

RuleActionPlan plan_rule_actions(const RuleConfiguration &configuration,
                                 const RuleCapturedInput &capture, const RuleEventLineage &event,
                                 std::span<const std::string> original_authors,
                                 std::span<const RuleExplicitValuation> valuations,
                                 const RuleActionMapper &mapper) {
    auto selection = select_rules(configuration, capture, valuations);
    const auto &rules = configuration.data().rules;
    if (!valid_lineage(event) || event.event_ref != selection.event_ref ||
        original_authors.size() != rules.size())
        throw RuleDefinitionFailure{RuleDefinitionError::invalid_capture};

    RuleActionPlan plan{event, std::move(selection), {}};
    plan.selected.reserve(plan.selection.selected_rule_indices.size());
    for (const auto index : plan.selection.selected_rule_indices) {
        const auto &rule = rules[index];
        RuleSelectedActions selected{index, original_authors[index], {}};
        selected.intents.reserve(rule.actions.size());
        for (std::size_t position = 0; position < rule.actions.size(); ++position) {
            const auto &action = rule.actions[position];
            RuleActionKey key{rule.rule_ref, rule.version_ref, action.action_ref,
                              action.version_ref, position};
            auto binding = mapper ? mapper(key, action.payload, capture)
                                  : RuleActionBinding{Refusal::unresolved_input};
            if (const auto *bound = std::get_if<RuleBoundAction>(&binding);
                binding.valueless_by_exception() ||
                (bound && !valid_binding(action.payload, *bound)))
                binding = Refusal::invalid_binding;
            selected.intents.push_back({std::move(key), action.payload, std::move(binding)});
        }
        plan.selected.push_back(std::move(selected));
    }
    return plan;
}

std::optional<RuleStop> disclose_rule_stop(const RuleStop &stop, RuleStopDisclosure disclosure) {
    switch (disclosure) {
    case RuleStopDisclosure::stop_only:
        return RuleStop{stop.position, stop.refusal, {}};
    case RuleStopDisclosure::detail:
        return stop;
    case RuleStopDisclosure::none:
        return std::nullopt;
    }
    return std::nullopt;
}

RuleSimulation simulate_rule_actions(const RuleActionPlan &plan, const RuleActionChecks &checks,
                                     const RuleActionProjector &project) {
    validate_plan(plan);
    RuleSimulation simulation;
    for (const auto &rule : plan.selected) {
        for (const auto &intent : rule.intents) {
            auto checked = check_action(rule, intent, checks);
            if (auto *stop = std::get_if<RuleStop>(&checked)) {
                simulation.stops.emplace_back(rule.captured_index, std::move(*stop));
                break;
            }
            if (!project) {
                simulation.stops.emplace_back(rule.captured_index,
                                              stop_at(intent, Refusal::unresolved_input));
                break;
            }
            simulation.projected.emplace_back(intent.key,
                                              project(std::get<RuleBoundAction>(intent.binding)));
        }
    }
    return simulation;
}

RuleDeliveryTrace observe_rule_delivery(const RuleDeliveryTrace &trace,
                                        const std::optional<RuleDeliveryObservation> &observed) {
    if (!observed || !initial_queue(trace.queued) || !valid_delivery(trace.latest) ||
        trace.queued.job_ref != trace.latest.job_ref || !valid_delivery(*observed) ||
        observed->job_ref != trace.queued.job_ref || observed->revision <= trace.latest.revision ||
        trace.latest.state == RuleDeliveryState::delivered ||
        trace.latest.state == RuleDeliveryState::outcome_unknown)
        return trace;

    // Observations may skip revisions. Check a predecessor only when it was observed.
    if (observed->revision - trace.latest.revision == 1) {
        const auto expected =
            observed->state == RuleDeliveryState::queued    ? RuleDeliveryState::failed
            : observed->state == RuleDeliveryState::sending ? RuleDeliveryState::queued
                                                            : RuleDeliveryState::sending;
        if (trace.latest.state != expected)
            return trace;
    }
    return {trace.queued, *observed};
}

RulePass consume_rule_actions(const RuleActionPlan &plan, const RuleActionChecks &checks,
                              const RuleActionSubmit &submit) {
    validate_plan(plan);
    RuleStagedPass staged;
    staged.rules.reserve(plan.selected.size());
    for (const auto &rule : plan.selected) {
        RuleConsumedActions consumed{rule.captured_index, {}, {}};
        for (const auto &intent : rule.intents) {
            auto checked = check_action(rule, intent, checks);
            if (auto *stop = std::get_if<RuleStop>(&checked)) {
                consumed.stop = std::move(*stop);
                break;
            }
            if (!submit) {
                consumed.stop = stop_at(intent, Refusal::capability_unavailable);
                break;
            }

            auto result = submit(plan.event, intent.key, std::get<RuleBoundAction>(intent.binding),
                                 std::get<std::string>(checked));
            if (const auto *failure = std::get_if<RulePassFailure>(&result))
                return *failure;
            if (auto *refused = std::get_if<RuleOwnerRefusal>(&result)) {
                RuleStopDetail detail{};
                detail.opaque_reason = std::move(refused->opaque_reason);
                consumed.stop = stop_at(intent, Refusal::handler_refusal, std::move(detail));
                break;
            }
            auto &acceptance = std::get<RuleOwnerAcceptance>(result);
            consumed.accepted.push_back(
                {intent.key,
                 accepted_observation(plan.event, intent.key, std::move(acceptance.observed))});
        }
        staged.rules.push_back(std::move(consumed));
    }
    return staged;
}

std::optional<RuleCommittedPass> publish_rule_pass(const RuleStagedPass &staged,
                                                   RuleCompletion completion) {
    if (completion != RuleCompletion::committed)
        return std::nullopt;
    return RuleCommittedPass{staged.rules};
}

} // namespace subathon::core
