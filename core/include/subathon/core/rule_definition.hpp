// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <span>
#include <string>
#include <subathon/core/contribution.hpp>
#include <subathon/core/modifier_activation.hpp>
#include <subathon/core/rate_snapshot.hpp>
#include <subathon/core/timer_lifecycle.hpp>
#include <variant>
#include <vector>

namespace subathon::core {
// RULES-001 draft revision 1. See #204 and owner decisions 47A–50A.
enum class RuleTriggerKind {
    subscription, gift_bundle, cheer, payment_received, payment_confirmed, refund,
    support_aggregate_change, chat_command, manual_adjustment, schedule, timer_expiry,
    milestone_completion, modifier_activation, modifier_expiry, named_event
};
enum class RuleTruth { no_match, match, unknown };
enum class RuleEquality { equal, not_equal };
enum class RuleOrder { equal, not_equal, less, at_most, greater, at_least };

// Common fields: provider, source account, connection, scoped actor identity
// (including explicit user selection), display name, source/arrival time, and
// captured timer/score context. Missing common facts are valid for any kind.
// Branch-local fields:
// subscription -> tier/classification and subscription_valuation;
// gift_bundle -> tier, gift_count and subscription_valuation; cheer -> bits;
// payment_received/confirmed -> payment_kind, payment_evidence and source_amount;
// refund -> payment_evidence and captured source_amount with its original sign;
// support_aggregate_change -> aggregate_amount. Aggregate and subscription
// values carry no individual payment or cash evidence.
// chat_command -> command_ref and the five independent chat category flags;
// manual_adjustment -> adjustment_ms; schedule -> schedule_ref;
// timer_expiry -> timer_transition; milestone_completion -> milestone_ref;
// modifier_activation/expiry -> modifier_ref; named_event -> event_name.
enum class RuleTextField {
    provider, source_account, connection_ref, actor_display_name,
    subscription_tier, subscription_classification, payment_kind, command_ref, schedule_ref,
    milestone_ref, modifier_ref, event_name
};
enum class RuleCountField { gift_count, bits };
enum class RuleIntegerField {
    source_utc_ms, arrival_utc_ms, main_remaining_ms, grace_remaining_ms, score_micro_points,
    adjustment_ms
};
enum class RuleFlagField {
    paused, chat_broadcaster, chat_moderator, chat_vip, chat_subscriber, chat_unsubscribed
};
enum class RuleMoneyField { source_amount, aggregate_amount, subscription_valuation };

template <typename Field, typename Value> struct RuleFact {
    Field field;
    Value value;
};
template <typename Fact, typename Operator> struct RuleComparison {
    Fact operand;
    Operator op;
};
using RuleText = RuleFact<RuleTextField, std::string>;
using RuleCount = RuleFact<RuleCountField, std::uint64_t>;
using RuleInteger = RuleFact<RuleIntegerField, std::int64_t>;
using RuleFlag = RuleFact<RuleFlagField, bool>;
using RuleMoney = RuleFact<RuleMoneyField, ReceiptMoney>;
struct RuleActor {
    ReceiptOrigin scope;
    std::string actor_id;
};
struct RuleActorComparison {
    RuleActor operand;
    RuleEquality op;
};

template <typename Enum> struct RuleEnumComparison {
    Enum value;
    RuleEquality op;
};
using RulePredicate =
    std::variant<RuleComparison<RuleText, RuleEquality>, RuleComparison<RuleCount, RuleOrder>,
                 RuleComparison<RuleInteger, RuleOrder>, RuleComparison<RuleFlag, RuleEquality>,
                 RuleComparison<RuleMoney, RuleOrder>, RuleActorComparison,
                 RuleEnumComparison<PaymentEvidence>,
                 RuleEnumComparison<TimerLifecycle>, RuleEnumComparison<CountdownPhase>,
                 RuleEnumComparison<TimerTransition>>;
using RuleCapturedFact =
    std::variant<RuleText, RuleCount, RuleInteger, RuleFlag, RuleMoney, RuleActor,
                 PaymentEvidence, TimerLifecycle, CountdownPhase, TimerTransition>;

struct RuleAll {
    std::vector<std::size_t> children;
};
struct RuleAny {
    std::vector<std::size_t> children;
};
struct RuleNot {
    std::size_t child;
};
using RuleConditionNode = std::variant<RulePredicate, RuleAll, RuleAny, RuleNot>;
// Empty is unconditional. Otherwise node 0 roots one tree, with ordered nonempty
// all/any and unary not. Reject invalid indices, cycles, aliases and unreachable
// nodes. Root depth is 1. Common trees use only common fields. Each branch tree
// can use common fields and fields for its own kind, irrespective of nesting.
using RuleCondition = std::vector<RuleConditionNode>;
struct RuleTrigger {
    RuleTriggerKind kind;
    RuleCondition condition;
};

// Action definitions own parameters and/or retain exact immutable parameter
// versions. A ref never means "latest". Planning verifies resolution, category,
// capability and current authorization before any effect, job or generated event.
enum class RuleDeltaUnit { milliseconds, micro_points, whole_count };
// Signed bounded JSON-number spelling (parse_decimal), including zero. Fractions
// are valid for milliseconds and micro_points. whole_count requires an exact
// integer value. Rounding and credited range checks belong to final effects.
struct RuleFixedDelta {
    std::string original_decimal;
};
struct RuleCalculation {
    std::string calculation_version_ref;
};
struct RuleDeltaAction {
    RuleDeltaUnit unit;
    std::string target_ref;
    std::variant<RuleFixedDelta, RuleCalculation> delta;
};
struct RuleActivateModifier {
    std::string definition_version_ref;
    ModifierTriggerTiming timing; // Explicit include_trigger or subsequent_only.
};
struct RuleExtendModifier {
    std::string modifier_ref;
    std::int64_t duration_ms; // Positive extension.
};
struct RuleTerminateModifier {
    std::string modifier_ref;
};
enum class RuleChatSender { configured_default, bot, broadcaster };
struct RuleChatAction {
    std::string route_ref;
    RuleChatSender sender;
    std::string template_version_ref;
    std::optional<std::string> fallback_policy_version_ref;
};
struct RuleAnnouncement {
    std::string presentation_version_ref;
};
struct RuleAnimation {
    std::string animation_version_ref;
};
struct RuleTimerControl {
    std::string timer_ref;
    std::string command_version_ref;
};
struct RuleNamedEvent {
    std::string event_name;
    std::optional<std::string> payload_version_ref;
};
struct RuleConnectorAction {
    std::string connection_ref;
    std::string operation_ref;
    std::string parameters_version_ref;
};
using RuleActionPayload =
    std::variant<RuleDeltaAction, RuleActivateModifier, RuleExtendModifier, RuleTerminateModifier,
                 RuleChatAction, RuleAnnouncement, RuleAnimation, RuleTimerControl, RuleNamedEvent,
                 RuleConnectorAction>;
struct RuleActionDefinition {
    std::string action_ref;
    std::string version_ref;
    RuleActionPayload payload;
};
struct RuleDefinition {
    std::string rule_ref;
    std::string version_ref;
    std::string name;
    std::string description;
    bool enabled = true;
    std::int64_t priority = 0;
    std::optional<std::string> exclusive_group_ref;
    std::vector<std::string> excluded_rule_refs;
    RuleCondition common_condition;
    std::vector<RuleTrigger> triggers;
    std::vector<RuleActionDefinition> actions; // User order, preserved exactly.
};

// #210 enforces these per-root limits.
struct RuleGenerationLimits {
    std::uint32_t maximum_depth = 32;
    std::uint32_t maximum_generated_events = 256;
};
struct RuleConfigurationInput {
    std::string configuration_ref;
    RuleGenerationLimits generation;
    std::vector<RuleDefinition> rules; // User-controlled tie-break order, not sorted here.
};
class RuleConfiguration final {
  public:
    RuleConfiguration(const RuleConfiguration &);
    [[nodiscard]] const RuleConfigurationInput &data() const noexcept;

  private:
    explicit RuleConfiguration(const RuleConfigurationInput &);
    friend RuleConfiguration validate_rule_configuration(const RuleConfigurationInput &);
    const RuleConfigurationInput data_;
};

// The caller projects retained, already attributed facts. Omit unknown or
// unavailable values, including unknown payment evidence. Absent never means
// zero, empty, false, or confirmed. Present not_applicable is distinct. Keep
// actor identifiers scoped to their captured origin. A repeated field is invalid.
// Chat category flags are independent; unsubscribed is never inferred.
// The captured snapshot ref and data are both absent or both present and equal.
// A partially supplied snapshot is invalid_capture.
struct RuleCapturedInput {
    std::string event_ref;
    std::string configuration_ref;
    RuleTriggerKind kind;
    std::span<const RuleCapturedFact> facts;
    const EffectiveRateSnapshot *arrival_rates = nullptr;
    std::optional<std::string> arrival_snapshot_ref;
};
// Separate validated resolution projected by the caller from retained provenance
// for this event and field. One per field and destination. Original spelling and
// currency must equal the captured fact exactly. Canonical reduced positive rate
// is destination units per source unit, subject to the existing 4096-bit bound.
// These strings convey identity and value, not authority to record a valuation.
struct RuleExplicitValuation {
    std::string resolution_ref;
    std::string event_ref;
    RuleMoney original;
    std::string comparison_currency;
    std::string numerator;
    std::string denominator;
};

enum class RuleDefinitionError {
    resource_limit, invalid_generation_limits, invalid_definition, invalid_condition,
    invalid_field, invalid_action, invalid_capture, invalid_valuation, invalid_exclusion
};
class RuleDefinitionFailure final : public std::exception {
  public:
    explicit RuleDefinitionFailure(RuleDefinitionError code) noexcept;
    [[nodiscard]] RuleDefinitionError code() const noexcept;
    [[nodiscard]] const char *what() const noexcept override;

  private:
    RuleDefinitionError code_;
};

// Draft revision and examples: https://github.com/Kernol12/subathon-bot-ai/pull/735
// Reject zero limits, empty identity/name, duplicate rule refs and action refs
// within each rule, and rules without triggers. Reject undeclared enum values,
// valueless variants and unknown as a payment-evidence operand. Validate
// disabled definitions, every branch and every action. Required reference
// strings and present optional references are nonempty. Compare text bytes
// exactly, and preserve signed deltas, exact spellings and order.
// Exclusion graph checks and precedence belong to #206. Empty configuration,
// action list and description are representable.
// Across rules, the same (action_ref, version_ref) must have identical typed
// payloads, or validation gives invalid_action. Different immutable versions
// of the same action_ref may be selected by different rules.
//
// Engineering ceilings (separate from generation): 4096 bytes per text field,
// 262144 aggregate string bytes and 4096 aggregate list entries per configuration
// or captured input (including separate valuations), 16 levels per expression.
// Count every represented list entry, including edges. Snapshot validation has
// its own existing limits. Reuse rate currency grammar and exact decimal bounds.
// Check total input sizes and structure before owning a result or evaluating.
// Both entry points signal refusals through RuleDefinitionFailure. Translate
// helper bounds to resource_limit, grammar errors in money and thresholds to
// invalid_field, an invalid fixed delta to invalid_action, malformed snapshot
// evidence to invalid_capture, and invalid explicit evidence to invalid_valuation.
// std::bad_alloc propagates.
[[nodiscard]] RuleConfiguration validate_rule_configuration(const RuleConfigurationInput &input);

// Same validation as above, with complete captured input and pair evidence
// validation before predicate evaluation. Return one result per rule in input
// order; disabled rules yield nullopt. A mismatched configuration_ref gives
// invalid_capture. Same-kind alternatives are ORed, then AND
// with the common condition; absent kind yields no_match. In all, false dominates;
// in any, true dominates; not preserves unknown. Only definite match qualifies.
//
// Exact captured amount times the rate (identity 1 for equal currencies, else
// destination weight divided by source weight in the same snapshot component)
// is compared with the exact threshold in its explicit currency. A missing
// foreign pair stays unknown except for an explicit valuation of that exact input
// and pair. Validate all supplied valuations. Refuse a same-currency valuation
// as invalid_valuation. An otherwise valid valuation for a pair already present
// in the snapshot is unused. Never round or consult display currency/current rates.
[[nodiscard]] std::vector<std::optional<RuleTruth>>
evaluate_rule_conditions(const RuleConfigurationInput &configuration,
                         const RuleCapturedInput &input,
                         std::span<const RuleExplicitValuation> valuations = {});
} // namespace subathon::core
