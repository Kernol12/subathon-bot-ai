// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <limits>
#include <subathon/core/rule_chain.hpp>
#include <tuple>

namespace subathon::core::rule_chain::evidence {

// Scalar fields are inline. These projections enumerate fields which need
// recursive size accounting or exact comparison in a retained publication.
#define CHAIN_FIELDS(Type, ...)                                                                    \
    inline auto fields(const Type &v) { return std::tie(__VA_ARGS__); }

CHAIN_FIELDS(ChainKey, v.installation, v.root)
CHAIN_FIELDS(RuleGenerationLimits, v.maximum_depth, v.maximum_generated_events)
CHAIN_FIELDS(ChainProfile, v.maximum, v.active_roots, v.pending_events, v.owned_bytes)
CHAIN_FIELDS(RuleActionKey, v.rule_ref, v.rule_version, v.action_ref, v.action_version, v.position)
CHAIN_FIELDS(NamedAction, v.key)
CHAIN_FIELDS(MilestoneOccurrence, v.source, v.occurrence, v.ordinal)
CHAIN_FIELDS(EventBinding, v.chain, v.event, v.parent, v.capture, v.cause)
CHAIN_FIELDS(RootBinding, v.event, v.normalized_occurrence)
CHAIN_FIELDS(AdmittedEvent, v.binding, v.depth, v.ordinal)
CHAIN_FIELDS(Cutoff, v.chain, v.root_capture, v.last, v.limits, v.reason)
CHAIN_FIELDS(UnitIdentity, v.unit, v.chain, v.anchor, v.purpose)
CHAIN_FIELDS(BlockedUnit, v.identity, v.reason)
CHAIN_FIELDS(GenerationSuppressed, v.stop)
CHAIN_FIELDS(OccurrenceDisposition, v.occurrence, v.parent, v.disposition, v.suppressed_binding)
CHAIN_FIELDS(NamedSuppression, v.action, v.stop)
CHAIN_FIELDS(Publication, v.identity, v.pass, v.children, v.occurrences, v.named_suppressions,
             v.cutoff)
CHAIN_FIELDS(ModeledHistory, v.root, v.admission, v.admitted_profile, v.events, v.publications,
             v.cutoff, v.blocked)
CHAIN_FIELDS(RuleCommittedPass, v.rules)
CHAIN_FIELDS(RuleStagedPass, v.rules)
CHAIN_FIELDS(RuleConsumedActions, v.captured_index, v.accepted, v.stop)
CHAIN_FIELDS(RuleAcceptedAction, v.key, v.observed)
CHAIN_FIELDS(RuleEventLineage, v.event_ref, v.parent_ref, v.root_ref, v.cause)
CHAIN_FIELDS(RuleEffectObservation, v.effect_ref, v.change)
CHAIN_FIELDS(RuleQuantityObservation, v.unit, v.requested, v.actual)
CHAIN_FIELDS(RuleDeliveryObservation, v.job_ref, v.evidence_ref)
CHAIN_FIELDS(PermissionRational, v.numerator, v.denominator)
CHAIN_FIELDS(RuleStop, v.position, v.refusal, v.detail)
CHAIN_FIELDS(RuleStopDetail, v.author_decision, v.channel, v.limit, v.authority_error,
             v.opaque_reason)
CHAIN_FIELDS(PermissionAutomationActionDecision, v.use, v.grant, v.disposition)

CHAIN_FIELDS(CaptureView, v.input, v.authors, v.valuations)
CHAIN_FIELDS(RuleConfigurationInput, v.configuration_ref, v.rules)
CHAIN_FIELDS(RuleDefinition, v.rule_ref, v.version_ref, v.name, v.description,
             v.exclusive_group_ref, v.excluded_rule_refs, v.common_condition, v.triggers, v.actions)
CHAIN_FIELDS(RuleAll, v.children)
CHAIN_FIELDS(RuleAny, v.children)
CHAIN_FIELDS(RuleTrigger, v.condition)
CHAIN_FIELDS(RuleActionDefinition, v.action_ref, v.version_ref, v.payload)
CHAIN_FIELDS(RuleDeltaAction, v.target_ref, v.delta)
CHAIN_FIELDS(RuleFixedDelta, v.original_decimal)
CHAIN_FIELDS(RuleCalculation, v.calculation_version_ref)
CHAIN_FIELDS(RuleActivateModifier, v.definition_version_ref)
CHAIN_FIELDS(RuleExtendModifier, v.modifier_ref)
CHAIN_FIELDS(RuleTerminateModifier, v.modifier_ref)
CHAIN_FIELDS(RuleChatAction, v.route_ref, v.template_version_ref, v.fallback_policy_version_ref)
CHAIN_FIELDS(RuleAnnouncement, v.presentation_version_ref)
CHAIN_FIELDS(RuleAnimation, v.animation_version_ref)
CHAIN_FIELDS(RuleTimerControl, v.timer_ref, v.command_version_ref)
CHAIN_FIELDS(RuleNamedEvent, v.event_name, v.payload_version_ref)
CHAIN_FIELDS(RuleConnectorAction, v.connection_ref, v.operation_ref, v.parameters_version_ref)
CHAIN_FIELDS(RuleCapturedInput, v.event_ref, v.configuration_ref, v.facts, v.arrival_rates,
             v.arrival_snapshot_ref)
CHAIN_FIELDS(RuleExplicitValuation, v.resolution_ref, v.event_ref, v.original,
             v.comparison_currency, v.numerator, v.denominator)
CHAIN_FIELDS(RuleActor, v.scope, v.actor_id)
CHAIN_FIELDS(RuleActorComparison, v.operand)
CHAIN_FIELDS(ReceiptOrigin, v.provider, v.source_account)
CHAIN_FIELDS(ReceiptMoney, v.original_decimal, v.currency)
CHAIN_FIELDS(EffectiveRateSnapshotData, v.snapshot_ref, v.parent_snapshot_ref, v.sources,
             v.overrides, v.constraints, v.components)
CHAIN_FIELDS(FetchedRateObservation, v.data())
CHAIN_FIELDS(FetchedRateObservationData, v.context, v.quotes)
CHAIN_FIELDS(RateObservationContext, v.observation_ref, v.provider_ref, v.configuration_ref,
             v.request_ref, v.source_evidence_ref, v.provider_snapshot_ref, v.source_rate_time,
             v.publication_time)
CHAIN_FIELDS(RateTimeEvidence, v.original_text)
CHAIN_FIELDS(RateQuote, v.from_currency, v.to_currency, v.rate)
CHAIN_FIELDS(ManualRateOverrideData, v.override_ref, v.from_currency, v.to_currency, v.rate,
             v.decision_ref, v.actor_ref, v.reason)
CHAIN_FIELDS(FetchedRateConstraint, v.observation_ref)
CHAIN_FIELDS(ManualRateConstraint, v.override_ref)
CHAIN_FIELDS(RateComponent, v.members)
CHAIN_FIELDS(RatePotential, v.currency, v.weight)
CHAIN_FIELDS(ExactDecimal, v.original(), v.value())
#undef CHAIN_FIELDS

template <class Field, class Value> auto fields(const RuleFact<Field, Value> &fact) {
    return std::tie(fact.value);
}
template <class Fact, class Operator>
auto fields(const RuleComparison<Fact, Operator> &comparison) {
    return std::tie(comparison.operand);
}

template <class T> bool exact(const T &, const T &);
template <class T> bool exact(const std::optional<T> &, const std::optional<T> &);
template <class T> bool exact(const std::vector<T> &, const std::vector<T> &);
template <class... T> bool exact(const std::variant<T...> &, const std::variant<T...> &);

template <class T> bool exact(const std::optional<T> &a, const std::optional<T> &b) {
    return a.has_value() == b.has_value() && (!a || exact(*a, *b));
}
template <class T> bool exact(const std::vector<T> &a, const std::vector<T> &b) {
    return std::ranges::equal(a, b,
                              [](const T &left, const T &right) { return exact(left, right); });
}
template <class... T> bool exact(const std::variant<T...> &a, const std::variant<T...> &b) {
    return !a.valueless_by_exception() && a.index() == b.index() &&
           std::visit(
               [&](const auto &value) {
                   return exact(value, std::get<std::decay_t<decltype(value)>>(b));
               },
               a);
}
template <class T, std::size_t... I>
bool exact_fields(const T &a, const T &b, std::index_sequence<I...>) {
    return (exact(std::get<I>(a), std::get<I>(b)) && ...);
}
template <class T> bool exact(const T &a, const T &b) {
    if constexpr (requires { a == b; })
        return a == b;
    else if constexpr (requires { fields(a); })
        return exact_fields(fields(a), fields(b),
                            std::make_index_sequence<std::tuple_size_v<decltype(fields(a))>>{});
    else {
        static_assert(std::is_empty_v<T>);
        return true;
    }
}

// Count represented storage once per entry, including strings and the
// backing of borrowed capture projections. Subtraction makes overflow a hold.
class Bytes {
  public:
    explicit Bytes(std::size_t limit) : remaining_(limit) {}

    bool take(std::size_t bytes) {
        if (bytes > remaining_) {
            valid_ = false;
            return false;
        }
        remaining_ -= bytes;
        return valid_;
    }

    template <class T> bool entry(const T &value) { return take(sizeof(T)) && dynamic(value); }

    template <class... T> bool entries(const T &...values) { return (entry(values) && ...); }

    template <class... T> bool payloads(const T &...values) { return (dynamic(values) && ...); }

    std::size_t remaining() const { return remaining_; }

  private:
    std::size_t remaining_;
    bool valid_ = true;

    bool dynamic(const std::string &value) { return take(value.size()); }

    bool dynamic(const Exact &value) {
        // The two integer backends are read in place, without copying the rational.
        const auto charge = [this](const auto &numerator, const auto &denominator) {
            return take(numerator.size() * sizeof(*numerator.limbs())) &&
                   take(denominator.size() * sizeof(*denominator.limbs()));
        };
        return [&]<class Backend>(const Backend &fraction) {
            if constexpr (requires {
                              fraction.num();
                              fraction.denom();
                          })
                return charge(fraction.num(), fraction.denom());
            else
                return charge(fraction.data().numerator().backend(),
                              fraction.data().denominator().backend());
        }(value.backend());
    }

    bool dynamic(const EffectiveRateSnapshot *value) { return !value || entry(value->data()); }

    template <class T> bool dynamic(const std::optional<T> &value) {
        return !value || dynamic(*value);
    }

    template <class... T> bool dynamic(const std::variant<T...> &value) {
        return !value.valueless_by_exception() &&
               std::visit([this](const auto &item) { return dynamic(item); }, value);
    }

    template <class T> bool range(std::span<const T> values) {
        if (values.size() > remaining_ / sizeof(T))
            return valid_ = false;
        return take(values.size() * sizeof(T)) &&
               std::ranges::all_of(values, [this](const auto &item) { return dynamic(item); });
    }

    template <class T> bool dynamic(std::span<const T> values) { return range(values); }
    template <class T> bool dynamic(const std::vector<T> &values) {
        return range(std::span{values});
    }

    template <class T> bool dynamic(const T &value) {
        if constexpr (requires { fields(value); })
            return std::apply([this](const auto &...parts) { return (dynamic(parts) && ...); },
                              fields(value));
        else
            return true;
    }
};

} // namespace subathon::core::rule_chain::evidence
