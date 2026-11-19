// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <bitset>
#include <map>
#include <set>
#include <string_view>
#include <subathon/core/rule_definition.hpp>
#include <tuple>
#include <type_traits>

namespace subathon::core {
namespace {
using Error = RuleDefinitionError;
using Kind = RuleTriggerKind;
using Truth = RuleTruth;
using boost::multiprecision::cpp_int;

constexpr std::size_t maximum_text_bytes = 4096;
constexpr std::size_t maximum_total_bytes = 262144;
constexpr std::size_t maximum_entries = 4096;
constexpr std::size_t maximum_condition_depth = 16;
constexpr RateSnapshotLimits rate_limits{};

[[noreturn]] void fail(Error error) { throw RuleDefinitionFailure(error); }

void require_reference(std::string_view text, Error error) {
    if (text.empty())
        fail(error);
}

template <typename Enum> bool in_range(Enum value, Enum first, Enum last) {
    return value >= first && value <= last;
}

template <typename... Visitors> struct Visit : Visitors... {
    using Visitors::operator()...;
};

template <typename Variant, typename Visitor>
decltype(auto) visit_checked(const Variant &value, Error error, Visitor visitor) {
    if (value.valueless_by_exception())
        fail(error);
    return std::visit(visitor, value);
}

// Use the same parameter lists for sizing and immutable action equality.
auto fields(const RuleDeltaAction &action) {
    return std::tie(action.unit, action.target_ref, action.delta);
}
auto fields(const RuleFixedDelta &delta) { return std::tie(delta.original_decimal); }
auto fields(const RuleCalculation &delta) { return std::tie(delta.calculation_version_ref); }
auto fields(const RuleActivateModifier &action) {
    return std::tie(action.definition_version_ref, action.timing);
}
auto fields(const RuleExtendModifier &action) {
    return std::tie(action.modifier_ref, action.duration_ms);
}
auto fields(const RuleTerminateModifier &action) { return std::tie(action.modifier_ref); }
auto fields(const RuleChatAction &action) {
    return std::tie(action.route_ref, action.sender, action.template_version_ref,
                    action.fallback_policy_version_ref);
}
auto fields(const RuleAnnouncement &action) { return std::tie(action.presentation_version_ref); }
auto fields(const RuleAnimation &action) { return std::tie(action.animation_version_ref); }
auto fields(const RuleTimerControl &action) {
    return std::tie(action.timer_ref, action.command_version_ref);
}
auto fields(const RuleNamedEvent &action) {
    return std::tie(action.event_name, action.payload_version_ref);
}
auto fields(const RuleConnectorAction &action) {
    return std::tie(action.connection_ref, action.operation_ref, action.parameters_version_ref);
}

using Delta = decltype(RuleDeltaAction::delta);

class Budget {
  public:
    void entries(std::size_t count) { add(entries_, count, maximum_entries); }

    void measure(const std::string &value) {
        if (value.size() > maximum_text_bytes)
            fail(Error::resource_limit);
        add(bytes_, value.size(), maximum_total_bytes);
    }

    void measure(const std::optional<std::string> &value) {
        if (value)
            measure(*value);
    }

    void measure(const RuleText &fact) { measure(fact.value); }

    void measure(const RuleMoney &fact) {
        measure(fact.value.original_decimal);
        measure(fact.value.currency);
    }

    void measure(const RuleActor &fact) {
        measure(fact.scope.provider);
        measure(fact.scope.source_account);
        measure(fact.actor_id);
    }

    void measure(const Delta &delta) { action(delta); }

    template <typename T>
        requires(std::is_enum_v<T> || std::is_integral_v<T>)
    void measure(const T &) {}

    template <typename Field, typename Value>
        requires std::is_integral_v<Value>
    void measure(const RuleFact<Field, Value> &) {}

    template <typename Variant> void action(const Variant &payload) {
        visit_checked(payload, Error::invalid_action, [&](const auto &typed) {
            std::apply([&](const auto &...field) { (measure(field), ...); }, fields(typed));
        });
    }

    void condition(const RuleCondition &tree) {
        entries(tree.size());
        for (const auto &node : tree) {
            visit_checked(node, Error::invalid_condition,
                          Visit{[&](const RulePredicate &predicate) {
                                    visit_checked(
                                        predicate, Error::invalid_field,
                                        [&](const auto &comparison) {
                                            if constexpr (requires { comparison.operand; })
                                                measure(comparison.operand);
                                        });
                                },
                                [&](const RuleNot &) { entries(1); },
                                [&](const auto &group) { entries(group.children.size()); }});
        }
    }

  private:
    static void add(std::size_t &total, std::size_t count, std::size_t limit) {
        if (count > limit - total)
            fail(Error::resource_limit);
        total += count;
    }

    std::size_t entries_ = 0;
    std::size_t bytes_ = 0;
};

// Preflight the complete DTO without making an owned copy or a proportional scratch index.
void check_configuration_budget(const RuleConfigurationInput &input) {
    Budget budget;
    budget.measure(input.configuration_ref);
    budget.entries(input.rules.size());
    for (const auto &rule : input.rules) {
        budget.measure(rule.rule_ref);
        budget.measure(rule.version_ref);
        budget.measure(rule.name);
        budget.measure(rule.description);
        budget.measure(rule.exclusive_group_ref);
        budget.entries(rule.excluded_rule_refs.size());
        budget.entries(rule.triggers.size());
        budget.entries(rule.actions.size());
        for (const auto &excluded : rule.excluded_rule_refs)
            budget.measure(excluded);
        budget.condition(rule.common_condition);
        for (const auto &trigger : rule.triggers)
            budget.condition(trigger.condition);
        for (const auto &action : rule.actions) {
            budget.measure(action.action_ref);
            budget.measure(action.version_ref);
            budget.action(action.payload);
        }
    }
}

void check_capture_budget(const RuleCapturedInput &input,
                          std::span<const RuleExplicitValuation> valuations) {
    Budget budget;
    budget.entries(input.facts.size());
    budget.entries(valuations.size());
    budget.measure(input.event_ref);
    budget.measure(input.configuration_ref);
    budget.measure(input.arrival_snapshot_ref);
    for (const auto &fact : input.facts)
        visit_checked(fact, Error::invalid_capture,
                      [&](const auto &typed) { budget.measure(typed); });
    for (const auto &valuation : valuations) {
        budget.measure(valuation.resolution_ref);
        budget.measure(valuation.event_ref);
        budget.measure(valuation.original);
        budget.measure(valuation.comparison_currency);
        budget.measure(valuation.numerator);
        budget.measure(valuation.denominator);
    }
}

ExactDecimal decimal(std::string_view text, Error error) {
    try {
        return parse_decimal(text);
    } catch (const std::length_error &) {
        fail(Error::resource_limit);
    } catch (const std::invalid_argument &) {
        fail(error);
    }
}

RateMoney money(const ReceiptMoney &value, Error error) {
    try {
        return make_rate_money(value);
    } catch (const RateSnapshotFailure &failure) {
        fail(failure.code() == RateSnapshotError::resource_limit ? Error::resource_limit : error);
    }
}

void validate_parameter(const std::string &reference) {
    require_reference(reference, Error::invalid_action);
}

void validate_parameter(const std::optional<std::string> &reference) {
    if (reference)
        validate_parameter(*reference);
}

void validate_parameter(RuleDeltaUnit unit) {
    if (!in_range(unit, RuleDeltaUnit::milliseconds, RuleDeltaUnit::whole_count))
        fail(Error::invalid_action);
}

void validate_parameter(ModifierTriggerTiming timing) {
    if (!in_range(timing, ModifierTriggerTiming::include_trigger,
                  ModifierTriggerTiming::subsequent_only))
        fail(Error::invalid_action);
}

void validate_parameter(RuleChatSender sender) {
    if (!in_range(sender, RuleChatSender::configured_default, RuleChatSender::broadcaster))
        fail(Error::invalid_action);
}

void validate_parameter(std::int64_t duration_ms) {
    if (duration_ms <= 0)
        fail(Error::invalid_action);
}

void validate_payload(const RuleDeltaAction &action) {
    validate_parameter(action.unit);
    validate_parameter(action.target_ref);
    std::visit(Visit{[&](const RuleFixedDelta &fixed) {
                         const auto parsed = decimal(fixed.original_decimal, Error::invalid_action);
                         if (action.unit == RuleDeltaUnit::whole_count &&
                             boost::multiprecision::denominator(parsed.value()) != 1)
                             fail(Error::invalid_action);
                     },
                     [](const RuleCalculation &calculation) {
                         validate_parameter(calculation.calculation_version_ref);
                     }},
               action.delta);
}

template <typename Action> void validate_payload(const Action &action) {
    std::apply([](const auto &...field) { (validate_parameter(field), ...); }, fields(action));
}

template <typename Variant> bool same_payload(const Variant &left, const Variant &right);

template <typename Action> bool same_parameters(const Action &left, const Action &right) {
    return fields(left) == fields(right);
}

bool same_parameters(const RuleDeltaAction &left, const RuleDeltaAction &right) {
    return left.unit == right.unit && left.target_ref == right.target_ref &&
           same_payload(left.delta, right.delta);
}

template <typename Variant> bool same_payload(const Variant &left, const Variant &right) {
    return std::visit(
        [&](const auto &payload) {
            const auto *other = std::get_if<std::decay_t<decltype(payload)>>(&right);
            return other && same_parameters(payload, *other);
        },
        left);
}

using TriggerMask = std::uint32_t;

constexpr TriggerMask branch(Kind kind) { return TriggerMask{1} << static_cast<unsigned>(kind); }

constexpr TriggerMask common_fields = ~TriggerMask{0};
constexpr TriggerMask payment = branch(Kind::payment_received) | branch(Kind::payment_confirmed);
constexpr TriggerMask subscription = branch(Kind::subscription) | branch(Kind::gift_bundle);
constexpr TriggerMask chat = branch(Kind::chat_command);

// Each row follows the corresponding public field enum's declaration order.
constexpr auto scopes(RuleTextField) {
    return std::array{common_fields,              // provider
                      common_fields,              // source_account
                      common_fields,              // connection_ref
                      common_fields,              // actor_display_name
                      subscription,               // subscription_tier
                      branch(Kind::subscription), // subscription_classification
                      payment,                    // payment_kind
                      chat,                       // command_ref
                      branch(Kind::schedule),     // schedule_ref
                      branch(Kind::milestone_completion),
                      branch(Kind::modifier_activation) | branch(Kind::modifier_expiry),
                      branch(Kind::named_event)};
}

constexpr auto scopes(RuleCountField) {
    return std::array{branch(Kind::gift_bundle), branch(Kind::cheer)};
}

constexpr auto scopes(RuleIntegerField) {
    return std::array{common_fields, common_fields, common_fields,
                      common_fields, common_fields, branch(Kind::manual_adjustment)};
}

constexpr auto scopes(RuleFlagField) {
    return std::array{common_fields, chat, chat, chat, chat, chat};
}

constexpr auto scopes(RuleMoneyField) {
    return std::array{payment | branch(Kind::refund), branch(Kind::support_aggregate_change),
                      subscription};
}

template <typename Field, typename Value>
TriggerMask fact_scope(const RuleFact<Field, Value> &fact) {
    constexpr auto allowed = scopes(Field{});
    const auto index = static_cast<std::size_t>(fact.field);
    return index < allowed.size() ? allowed[index] : 0;
}

TriggerMask fact_scope(const RuleActor &) { return common_fields; }

TriggerMask fact_scope(PaymentEvidence value) {
    return in_range(value, PaymentEvidence::unconfirmed, PaymentEvidence::not_applicable)
               ? payment | branch(Kind::refund)
               : 0;
}

TriggerMask fact_scope(TimerLifecycle value) {
    return in_range(value, TimerLifecycle::Ready, TimerLifecycle::Finished) ? common_fields : 0;
}

TriggerMask fact_scope(CountdownPhase value) {
    return in_range(value, CountdownPhase::Main, CountdownPhase::Grace) ? common_fields : 0;
}

TriggerMask fact_scope(TimerTransition value) {
    return in_range(value, TimerTransition::MainExpired, TimerTransition::Finished)
               ? branch(Kind::timer_expiry)
               : 0;
}

template <typename Fact> void validate_fact(const Fact &fact, TriggerMask scope, Error error) {
    if ((fact_scope(fact) & scope) != scope)
        fail(error);
    if constexpr (std::is_same_v<Fact, RuleActor>) {
        require_reference(fact.scope.provider, error);
        require_reference(fact.scope.source_account, error);
        require_reference(fact.actor_id, error);
    }
}

bool valid_operator(RuleEquality op) {
    return in_range(op, RuleEquality::equal, RuleEquality::not_equal);
}

bool valid_operator(RuleOrder op) { return in_range(op, RuleOrder::equal, RuleOrder::at_least); }

void validate_predicate(const RulePredicate &predicate, TriggerMask scope) {
    std::visit(
        [&](const auto &comparison) {
            if (!valid_operator(comparison.op))
                fail(Error::invalid_field);
            if constexpr (requires { comparison.operand; }) {
                validate_fact(comparison.operand, scope, Error::invalid_field);
                if constexpr (std::is_same_v<std::decay_t<decltype(comparison.operand)>, RuleMoney>)
                    static_cast<void>(money(comparison.operand.value, Error::invalid_field));
            } else {
                validate_fact(comparison.value, scope, Error::invalid_field);
            }
        },
        predicate);
}

void validate_node(const RuleCondition &tree, std::size_t node, std::size_t depth,
                   TriggerMask scope, std::bitset<maximum_entries> &seen) {
    if (node >= tree.size() || seen.test(node))
        fail(Error::invalid_condition);
    if (depth > maximum_condition_depth)
        fail(Error::resource_limit);
    seen.set(node);
    std::visit(Visit{[&](const RulePredicate &predicate) { validate_predicate(predicate, scope); },
                     [&](const RuleNot &negation) {
                         validate_node(tree, negation.child, depth + 1, scope, seen);
                     },
                     [&](const auto &group) {
                         if (group.children.empty())
                             fail(Error::invalid_condition);
                         for (auto child : group.children)
                             validate_node(tree, child, depth + 1, scope, seen);
                     }},
               tree[node]);
}

void validate_condition(const RuleCondition &tree, TriggerMask scope) {
    if (tree.empty())
        return;
    std::bitset<maximum_entries> seen;
    validate_node(tree, 0, 1, scope, seen);
    if (seen.count() != tree.size())
        fail(Error::invalid_condition);
}

void validate_configuration(const RuleConfigurationInput &input) {
    require_reference(input.configuration_ref, Error::invalid_definition);
    if (input.generation.maximum_depth == 0 || input.generation.maximum_generated_events == 0)
        fail(Error::invalid_generation_limits);

    std::set<std::string_view> rule_refs;
    using ActionKey = std::pair<std::string_view, std::string_view>;
    std::map<ActionKey, const RuleActionPayload *> versions;
    for (const auto &rule : input.rules) {
        require_reference(rule.rule_ref, Error::invalid_definition);
        require_reference(rule.version_ref, Error::invalid_definition);
        require_reference(rule.name, Error::invalid_definition);
        if (!rule_refs.insert(rule.rule_ref).second || rule.triggers.empty())
            fail(Error::invalid_definition);
        if (rule.exclusive_group_ref)
            require_reference(*rule.exclusive_group_ref, Error::invalid_definition);
        for (const auto &excluded : rule.excluded_rule_refs)
            require_reference(excluded, Error::invalid_definition);

        validate_condition(rule.common_condition, common_fields);
        for (const auto &trigger : rule.triggers) {
            if (!in_range(trigger.kind, Kind::subscription, Kind::named_event))
                fail(Error::invalid_definition);
            validate_condition(trigger.condition, branch(trigger.kind));
        }

        std::set<std::string_view> action_refs;
        for (const auto &action : rule.actions) {
            require_reference(action.action_ref, Error::invalid_action);
            require_reference(action.version_ref, Error::invalid_action);
            if (!action_refs.insert(action.action_ref).second)
                fail(Error::invalid_action);
            std::visit([](const auto &payload) { validate_payload(payload); }, action.payload);

            const ActionKey key{action.action_ref, action.version_ref};
            const auto [existing, inserted] = versions.try_emplace(key, &action.payload);
            if (!inserted && !same_payload(*existing->second, action.payload))
                fail(Error::invalid_action);
        }
    }
}

template <typename Fact>
const Fact *find_fact(std::span<const RuleCapturedFact> facts, const Fact &key) {
    for (const auto &entry : facts) {
        const auto *fact = std::get_if<Fact>(&entry);
        if (!fact)
            continue;
        if constexpr (requires { key.field; }) {
            if (fact->field != key.field)
                continue;
        }
        return fact;
    }
    return nullptr;
}

cpp_int valuation_component(std::string_view text) {
    if (text.empty() || text.front() < '1' || text.front() > '9' ||
        !std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; }))
        fail(Error::invalid_valuation);
    cpp_int value = 0;
    for (char digit : text) {
        value *= 10;
        value += static_cast<unsigned>(digit - '0');
        // Stop at the first oversized prefix. The previous prefix was at most 4096 bits.
        if (boost::multiprecision::msb(value) >= rate_limits.potential_component_bits)
            fail(Error::resource_limit);
    }
    return value;
}

Exact valuation_rate(const RuleExplicitValuation &valuation) {
    const auto numerator = valuation_component(valuation.numerator);
    const auto denominator = valuation_component(valuation.denominator);
    Exact rate(numerator);
    rate /= denominator;
    if (boost::multiprecision::denominator(rate) != denominator)
        fail(Error::invalid_valuation);
    return rate;
}

Truth truth(bool matches) { return matches ? Truth::match : Truth::no_match; }

Truth compare_equality(bool equal, RuleEquality op) {
    return truth(op == RuleEquality::equal ? equal : !equal);
}

template <typename Value> Truth compare_order(const Value &left, const Value &right, RuleOrder op) {
    switch (op) {
    case RuleOrder::equal:
        return truth(left == right);
    case RuleOrder::not_equal:
        return truth(left != right);
    case RuleOrder::less:
        return truth(left < right);
    case RuleOrder::at_most:
        return truth(left <= right);
    case RuleOrder::greater:
        return truth(left > right);
    case RuleOrder::at_least:
        return truth(left >= right);
    }
    fail(Error::invalid_field);
}

class CapturedFacts {
  public:
    CapturedFacts(const RuleCapturedInput &input, std::string_view configuration_ref,
                  std::span<const RuleExplicitValuation> valuations)
        : facts_(input.facts) {
        if (input.event_ref.empty() || input.configuration_ref != configuration_ref ||
            !in_range(input.kind, Kind::subscription, Kind::named_event))
            fail(Error::invalid_capture);
        if (static_cast<bool>(input.arrival_rates) != input.arrival_snapshot_ref.has_value())
            fail(Error::invalid_capture);
        if (input.arrival_snapshot_ref &&
            (input.arrival_snapshot_ref->empty() ||
             *input.arrival_snapshot_ref != input.arrival_rates->data().snapshot_ref))
            fail(Error::invalid_capture);

        const auto scope = branch(input.kind);
        for (std::size_t index = 0; index < facts_.size(); ++index) {
            std::visit(
                [&](const auto &fact) {
                    validate_fact(fact, scope, Error::invalid_capture);
                    if (find_fact(facts_.first(index), fact))
                        fail(Error::invalid_capture);
                    if constexpr (std::is_same_v<std::decay_t<decltype(fact)>, RuleMoney>)
                        amounts_[static_cast<std::size_t>(fact.field)] =
                            money(fact.value, Error::invalid_field).data().value;
                },
                facts_[index]);
        }
        for (const auto &valuation : valuations)
            validate_valuation(input.event_ref, valuation);

        // The typed snapshot has already passed its factory's evidence and resource checks.
        if (input.arrival_rates) {
            for (const auto &component : input.arrival_rates->data().components)
                for (const auto &member : component.members)
                    weights_.emplace(member.currency, Weight{&component, &member.weight});
        }
    }

    Truth test(const RulePredicate &predicate) const {
        return std::visit([&](const auto &comparison) { return test_comparison(comparison); },
                          predicate);
    }

  private:
    void validate_valuation(std::string_view event_ref, const RuleExplicitValuation &valuation) {
        if (valuation.resolution_ref.empty() || valuation.event_ref != event_ref)
            fail(Error::invalid_valuation);
        const auto *original = find_fact(facts_, valuation.original);
        if (!original || original->value != valuation.original.value ||
            original->value.currency == valuation.comparison_currency)
            fail(Error::invalid_valuation);
        // Validate the destination through the same currency grammar and bound as money.
        if (valuation.comparison_currency.size() > rate_limits.currency_bytes)
            fail(Error::resource_limit);
        static_cast<void>(money({"0", valuation.comparison_currency}, Error::invalid_valuation));
        const auto key =
            std::pair{valuation.original.field, std::string_view(valuation.comparison_currency)};
        if (valuations_.contains(key))
            fail(Error::invalid_valuation);
        valuations_.emplace(key, valuation_rate(valuation));
    }

    template <typename Enum>
    Truth test_comparison(const RuleEnumComparison<Enum> &comparison) const {
        const auto *fact = find_fact(facts_, comparison.value);
        return fact ? compare_equality(*fact == comparison.value, comparison.op) : Truth::unknown;
    }

    Truth test_comparison(const RuleActorComparison &comparison) const {
        const auto *fact = find_fact(facts_, comparison.operand);
        return fact ? compare_equality(fact->scope == comparison.operand.scope &&
                                           fact->actor_id == comparison.operand.actor_id,
                                       comparison.op)
                    : Truth::unknown;
    }

    template <typename Fact, typename Operator>
    Truth test_comparison(const RuleComparison<Fact, Operator> &comparison) const {
        const auto *fact = find_fact(facts_, comparison.operand);
        if (!fact)
            return Truth::unknown;
        if constexpr (std::is_same_v<Fact, RuleMoney>) {
            return compare_money(*fact, comparison.operand, comparison.op);
        } else if constexpr (std::is_same_v<Operator, RuleEquality>) {
            return compare_equality(fact->value == comparison.operand.value, comparison.op);
        } else {
            return compare_order(fact->value, comparison.operand.value, comparison.op);
        }
    }

    Truth compare_money(const RuleMoney &fact, const RuleMoney &operand, RuleOrder op) const {
        const auto &amount = *amounts_[static_cast<std::size_t>(fact.field)];
        const auto &from = fact.value.currency;
        const auto &to = operand.value.currency;
        if (from == to)
            return compare_order(
                amount, decimal(operand.value.original_decimal, Error::invalid_field).value(), op);

        const auto source = weights_.find(from);
        const auto destination = weights_.find(to);
        Exact rate;
        if (source != weights_.end() && destination != weights_.end() &&
            source->second.component == destination->second.component) {
            rate = *destination->second.value / *source->second.value;
        } else {
            const auto resolution = valuations_.find({fact.field, to});
            if (resolution == valuations_.end())
                return Truth::unknown;
            rate = resolution->second;
        }

        // Each rate component is at most 8192 bits (two 4096-bit weights).
        // A parsed decimal component is below 10^512 (under 2048 bits), so scaling stays within
        // 10240 bits and even a comparison cross-product stays within 12288 bits.
        const Exact converted = amount * rate;
        const auto threshold = decimal(operand.value.original_decimal, Error::invalid_field);
        return compare_order(converted, threshold.value(), op);
    }

    struct Weight {
        const RateComponent *component;
        const Exact *value;
    };

    std::span<const RuleCapturedFact> facts_;
    std::array<std::optional<Exact>, scopes(RuleMoneyField{}).size()> amounts_;
    std::map<std::pair<RuleMoneyField, std::string_view>, Exact> valuations_;
    std::map<std::string_view, Weight> weights_;
};

Truth negate(Truth value) {
    return value == Truth::unknown ? value : truth(value == Truth::no_match);
}

Truth evaluate_node(const RuleCondition &tree, std::size_t node, const CapturedFacts &facts) {
    return std::visit(Visit{[&](const RulePredicate &predicate) { return facts.test(predicate); },
                            [&](const RuleNot &negation) {
                                return negate(evaluate_node(tree, negation.child, facts));
                            },
                            [&](const auto &group) {
                                constexpr bool all =
                                    std::is_same_v<std::decay_t<decltype(group)>, RuleAll>;
                                const auto decisive = all ? Truth::no_match : Truth::match;
                                auto result = negate(decisive);
                                for (auto child : group.children) {
                                    const auto value = evaluate_node(tree, child, facts);
                                    if (value == decisive)
                                        return decisive;
                                    if (value == Truth::unknown)
                                        result = Truth::unknown;
                                }
                                return result;
                            }},
                      tree[node]);
}

Truth evaluate_condition(const RuleCondition &tree, const CapturedFacts &facts) {
    return tree.empty() ? Truth::match : evaluate_node(tree, 0, facts);
}

Truth evaluate_rule(const RuleDefinition &rule, Kind kind, const CapturedFacts &facts) {
    auto alternatives = Truth::no_match;
    for (const auto &trigger : rule.triggers) {
        if (trigger.kind != kind)
            continue;
        const auto value = evaluate_condition(trigger.condition, facts);
        if (value == Truth::match) {
            alternatives = Truth::match;
            break;
        }
        if (value == Truth::unknown)
            alternatives = Truth::unknown;
    }
    if (alternatives == Truth::no_match)
        return Truth::no_match;
    const auto common = evaluate_condition(rule.common_condition, facts);
    if (common == Truth::no_match || alternatives == Truth::match)
        return common;
    return Truth::unknown;
}
} // namespace

RuleConfiguration::RuleConfiguration(const RuleConfigurationInput &input) : data_(input) {}
RuleConfiguration::RuleConfiguration(const RuleConfiguration &) = default;

const RuleConfigurationInput &RuleConfiguration::data() const noexcept { return data_; }

RuleDefinitionFailure::RuleDefinitionFailure(RuleDefinitionError code) noexcept : code_(code) {}

RuleDefinitionError RuleDefinitionFailure::code() const noexcept { return code_; }

const char *RuleDefinitionFailure::what() const noexcept { return "Rule definition refused"; }

RuleConfiguration validate_rule_configuration(const RuleConfigurationInput &input) {
    check_configuration_budget(input);
    validate_configuration(input);
    return RuleConfiguration(input);
}

std::vector<std::optional<RuleTruth>>
evaluate_rule_conditions(const RuleConfigurationInput &configuration,
                         const RuleCapturedInput &input,
                         std::span<const RuleExplicitValuation> valuations) {
    check_configuration_budget(configuration);
    check_capture_budget(input, valuations);
    validate_configuration(configuration);
    const CapturedFacts facts(input, configuration.configuration_ref, valuations);

    std::vector<std::optional<RuleTruth>> results;
    results.reserve(configuration.rules.size());
    for (const auto &rule : configuration.rules) {
        if (rule.enabled)
            results.emplace_back(evaluate_rule(rule, input.kind, facts));
        else
            results.emplace_back(std::nullopt);
    }
    return results;
}
} // namespace subathon::core
