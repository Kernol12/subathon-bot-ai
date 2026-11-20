// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/chat_commands.hpp>

#include "permission_engine.hpp"

#include <algorithm>
#include <functional>
#include <list>
#include <stdexcept>
#include <string_view>
#include <subathon/core/permission_graph_validation.hpp>
#include <type_traits>
#include <utility>

namespace subathon::core {
namespace {
namespace permission = permission_engine;
using Error = ChatCommandError;
using Disposition = ChatCommandDisposition;
using Fact = TwitchChatFact;
using Route = ChatManagementRoute;
using Index = std::size_t;
using Tick = std::uint64_t;

[[noreturn]] void fail(Error error) { throw ChatCommandFailure(error); }

template <class T> bool valid_tag(T value, T last) { return value <= last; }

template <class F> auto allocation_guard(F operation) {
    try {
        return operation();
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_failure);
    } catch (const std::length_error &) {
        fail(Error::allocation_failure);
    }
}

template <class T> const T *find_key(const std::vector<T> &values, std::string_view key, auto get) {
    const auto found = std::ranges::find(values, key, get);
    return found == values.end() ? nullptr : &*found;
}

template <class T> bool unique(const std::vector<T> &values, auto get) {
    for (auto it = values.begin(); it != values.end(); ++it)
        if (std::ranges::find(values.begin(), it, std::invoke(get, *it), get) != it)
            return false;
    return true;
}

constexpr auto identity = std::identity{};
constexpr auto slot_key = [](const auto &value) -> const auto & { return value.slot; };

struct NumberView {
    std::string_view numerator;
    std::string_view denominator = "1";

    PermissionRational own() const { return {std::string(numerator), std::string(denominator)}; }
};

NumberView view(const PermissionRational &number) { return {number.numerator, number.denominator}; }

// Charge fields as declared, before inspecting semantic tags or copying any payload.
// Permission projections also enforce the per-reference and per-component bounds.
class Budget {
  public:
    Budget(Tick bytes, Tick items, Error error, const EffectivePermissionLimits *limits = nullptr)
        : bytes_left_(bytes), items_left_(items), error_(error), permission_(limits) {}

    void bytes(Tick count) {
        if (count > bytes_left_)
            fail(error_);
        bytes_left_ -= count;
    }

    template <class T> void items(Index count) {
        if (count > std::vector<T>{}.max_size() || count > items_left_)
            fail(error_);
        items_left_ -= count;
    }

    template <class... T> void operator()(const T &...fields) { (field(fields), ...); }
    bool empty_reference() const { return empty_reference_; }

  private:
    void field(std::string_view value) {
        if (value.size() > std::string{}.max_size() ||
            (permission_ && value.size() > permission_->reference_bytes))
            fail(error_);
        empty_reference_ |= permission_ && value.empty();
        bytes(value.size());
    }
    void field(NumberView value) {
        for (auto component : {value.numerator, value.denominator}) {
            if (component.size() > std::string{}.max_size() ||
                (permission_ && component.size() > permission_->component_bytes))
                fail(error_);
            bytes(component.size());
        }
    }
    void field(const PermissionRational &v) { field(view(v)); }
    void field(Tick) { bytes(8); }
    void field(bool) { bytes(1); }
    void field(std::uint8_t) { bytes(1); }
    template <class T>
        requires std::is_enum_v<T>
    void field(T) {
        bytes(1);
    }
    template <class T> void field(const std::optional<T> &v) {
        bytes(1);
        if (v)
            field(*v);
    }
    template <class... T> void field(const std::variant<T...> &v) {
        bytes(1);
        if (!v.valueless_by_exception())
            std::visit([this](const auto &term) { field(term); }, v);
    }
    template <class T, Index N> void field(const std::array<T, N> &v) {
        for (const auto &entry : v)
            field(entry);
    }
    template <class T> void field(const std::vector<T> &v) {
        items<T>(v.size());
        for (const auto &entry : v)
            field(entry);
    }

    void field(const ReceiptOrigin &v) { (*this)(v.provider, v.source_account); }
    void field(const ReceiptIdentity &v) { (*this)(v.origin, v.identifier_namespace, v.opaque_id); }
    void field(const ChatExplicitUser &v) { field(v.twitch_user); }
    void field(const ChatArgumentDefinition &v) { (*this)(v.name, v.type); }
    void field(const ChatArgumentReference &v) { field(v.name); }
    void field(const ChatActionResource &v) { (*this)(v.slot, v.value); }
    void field(const ChatActionQuantity &v) { (*this)(v.slot, v.unit, v.value); }
    void field(const ChatCommandAction &v) {
        (*this)(v.operation, v.required, v.resources, v.quantities);
    }
    void field(const ChatResponseTemplate &v) { (*this)(v.sender, v.parts); }
    void field(const ChatCommandEnvelope &v) {
        (*this)(v.message, v.twitch_user, v.categories, v.alias, v.argument_tokens);
    }
    void field(const ChatCommandAuthority &v) {
        (*this)(v.source, v.link_state, v.link, v.snapshot, v.catalog, v.owner,
                v.category_grants_state, v.category_grants);
    }
    void field(const ChatLinkedAccount &v) { (*this)(v.twitch_user, v.account, v.use); }
    void field(const ChatCategoryGrant &v) { (*this)(v.category, v.grant); }
    void field(const DelegatedPermissionGrant &v) {
        (*this)(v.operation, v.resources, v.quantities);
    }
    void field(const PermissionCatalog &v) { (*this)(v.state, v.revision, v.operations); }
    void field(const PermissionOperationDefinition &v) {
        (*this)(v.operation, v.classification, v.resource_slots, v.quantity_slots,
                v.helper_target_slot);
    }
    void field(const PermissionQuantityDefinition &v) {
        (*this)(v.slot, v.unit, v.kind, v.minimum, v.maximum, v.allowed_modes);
    }
    void field(const PermissionOwnerFacts &v) { (*this)(v.state, v.account); }
    void field(const PermissionPolicyFacts &v) { (*this)(v.state, v.assigned_roles, v.entries); }
    void field(const PermissionPolicyEntry &v) {
        (*this)(v.origin, v.source, v.role, v.operation, v.effect, v.grant, v.resources,
                v.quantities);
    }
    void field(const PermissionResourceConstraint &v) { (*this)(v.slot, v.mode, v.allowed); }
    void field(const PermissionQuantityConstraint &v) {
        (*this)(v.slot, v.unit, v.mode, v.lower, v.upper);
    }

    Tick bytes_left_;
    Tick items_left_;
    Error error_;
    const EffectivePermissionLimits *permission_;
    bool empty_reference_ = false;
};

Budget permission_input_budget(const EffectivePermissionLimits &limits) {
    return {limits.input_owned_bytes, limits.input_items, Error::input_limit, &limits};
}

bool valid_reference(std::string_view value, const ChatCommandLimits &limits) {
    return !value.empty() && value.size() <= limits.permission.reference_bytes;
}

bool required_reference(std::string_view value, const ChatCommandLimits &limits) {
    if (value.size() > limits.permission.reference_bytes)
        fail(Error::input_limit);
    return !value.empty();
}

bool valid_alias(std::string_view value) {
    return !value.empty() &&
           std::ranges::all_of(value, [](unsigned char c) { return c >= 0x21 && c <= 0x7e; });
}

// Iterative traversal keeps both preflight and ownership safe for caller-configured depths.
void walk_eligibility(const ChatEligibility &root, Index depth, auto enter, auto leave) {
    struct Frame {
        const ChatEligibility *node;
        Index next_child;
    };
    std::vector<Frame> stack;
    const Index maximum = std::min(depth, stack.max_size());
    auto descend = [&](const ChatEligibility &node) {
        if (stack.size() == maximum)
            fail(Error::input_limit);
        enter(node);
        if (stack.size() == stack.capacity()) {
            const Index size = stack.size();
            stack.reserve(size + std::min(maximum - size, std::max(Index{1}, size)));
        }
        stack.push_back({&node, 0});
    };
    descend(root);
    while (!stack.empty()) {
        auto &frame = stack.back();
        if (frame.next_child < frame.node->children.size()) {
            descend(frame.node->children[frame.next_child++]);
        } else {
            leave(*frame.node);
            stack.pop_back();
        }
    }
}

void preflight_revision(const ChatCommandRevision &revision, const ChatCommandLimits &limits) {
    Budget budget(limits.input_bytes, limits.input_items, Error::input_limit);
    budget(revision.source, revision.revision);
    budget.items<ChatCommandDefinition>(revision.commands.size());
    for (const auto &command : revision.commands) {
        budget(command.command, command.aliases, command.arguments, command.eligibility.has_value(),
               command.route, command.actions, command.response, command.global_cooldown_ms,
               command.user_cooldown_ms);
        if (command.eligibility)
            walk_eligibility(
                *command.eligibility, static_cast<Index>(limits.eligibility_depth),
                [&](const ChatEligibility &node) {
                    budget.items<ChatEligibility>(1); // The root is also an item.
                    budget(node.term);
                },
                [](const ChatEligibility &) {});
    }
}

bool valid_number(NumberView number, const ChatCommandLimits &limits) {
    const Tick maximum = limits.permission.component_bytes;
    if (number.numerator.size() > maximum || number.denominator.size() > maximum)
        fail(Error::input_limit);
    try {
        permission::validate_number(number.own());
        return true;
    } catch (const EffectivePermissionFailure &failure) {
        if (failure.code() != EffectivePermissionError::invalid_number)
            throw;
        return false;
    }
}

bool numeric(ChatArgumentType type) {
    return type == ChatArgumentType::integer || type == ChatArgumentType::rational;
}

const ChatArgumentDefinition *argument(const ChatCommandDefinition &command,
                                       const ChatArgumentReference &reference) {
    return find_key(command.arguments, reference.name, &ChatArgumentDefinition::name);
}

void validate_definition(const ChatCommandDefinition &command, const ChatCommandLimits &limits) {
    auto reference = [&](std::string_view text) { return required_reference(text, limits); };
    if (!reference(command.command) || command.aliases.empty() ||
        !valid_tag(command.route, Route::both) || (command.actions.empty() && !command.response) ||
        (!command.actions.empty() &&
         std::ranges::none_of(command.actions, &ChatCommandAction::required)) ||
        !unique(command.arguments, &ChatArgumentDefinition::name))
        fail(Error::invalid_revision);

    for (const auto &alias : command.aliases)
        if (!valid_alias(alias))
            fail(Error::invalid_revision);
    for (const auto &arg : command.arguments)
        if (!reference(arg.name) || !valid_tag(arg.type, ChatArgumentType::rational))
            fail(Error::invalid_revision);

    for (const auto &action : command.actions) {
        if (!reference(action.operation) || !unique(action.resources, slot_key) ||
            !unique(action.quantities, slot_key))
            fail(Error::invalid_revision);
        for (const auto &resource : action.resources) {
            if (!reference(resource.slot) || resource.value.valueless_by_exception())
                fail(Error::invalid_revision);
            if (const auto *literal = std::get_if<std::string>(&resource.value)) {
                if (!reference(*literal))
                    fail(Error::invalid_revision);
            } else {
                const auto *arg =
                    argument(command, std::get<ChatArgumentReference>(resource.value));
                if (!arg || arg->type != ChatArgumentType::resource)
                    fail(Error::invalid_revision);
            }
        }
        for (const auto &quantity : action.quantities) {
            if (!reference(quantity.slot) || !reference(quantity.unit) ||
                quantity.value.valueless_by_exception())
                fail(Error::invalid_revision);
            if (const auto *literal = std::get_if<PermissionRational>(&quantity.value)) {
                if (!valid_number(view(*literal), limits))
                    fail(Error::invalid_revision);
            } else {
                const auto *arg =
                    argument(command, std::get<ChatArgumentReference>(quantity.value));
                if (!arg || !numeric(arg->type))
                    fail(Error::invalid_revision);
            }
        }
    }

    if (command.response) {
        if (!valid_tag(command.response->sender, ChatResponseSender::broadcaster))
            fail(Error::invalid_revision);
        for (const auto &part : command.response->parts) {
            const auto *ref = std::get_if<ChatArgumentReference>(&part);
            if (part.valueless_by_exception() || (ref && !argument(command, *ref)))
                fail(Error::invalid_revision);
        }
    }
}

struct EligibilityInstruction {
    decltype(ChatEligibility::term) term;
    Index children;
};
struct ConfiguredCommand {
    ChatCommandDefinition definition;
    std::vector<EligibilityInstruction> eligibility;
    std::optional<Tick> global_start;
};

Index validate_eligibility(const ChatEligibility &root, const ChatCommandLimits &limits) {
    Index count = 0;
    walk_eligibility(
        root, static_cast<Index>(limits.eligibility_depth),
        [&](const ChatEligibility &node) {
            ++count;
            if (const auto *group = std::get_if<ChatEligibilityGroup>(&node.term)) {
                if (!valid_tag(*group, ChatEligibilityGroup::any) || node.children.empty())
                    fail(Error::invalid_revision);
                return;
            }
            if (!node.children.empty())
                fail(Error::invalid_revision);
            if (const auto *category = std::get_if<TwitchChatCategory>(&node.term)) {
                if (valid_tag(*category, TwitchChatCategory::unsubscribed_viewer))
                    return;
            }
            if (const auto *user = std::get_if<ChatExplicitUser>(&node.term)) {
                if (required_reference(user->twitch_user, limits))
                    return;
            }
            fail(Error::invalid_revision);
        },
        [](const ChatEligibility &) {});
    return count;
}

std::vector<ConfiguredCommand> configure(const ChatCommandRevision &revision,
                                         const ChatCommandLimits &limits) {
    preflight_revision(revision, limits);
    if (revision.source.provider != "twitch" ||
        !required_reference(revision.source.provider, limits) ||
        !required_reference(revision.source.source_account, limits) ||
        !required_reference(revision.revision, limits) ||
        !unique(revision.commands, &ChatCommandDefinition::command))
        fail(Error::invalid_revision);

    for (auto it = revision.commands.begin(); it != revision.commands.end(); ++it) {
        validate_definition(*it, limits);
        if (!unique(it->aliases, identity))
            fail(Error::invalid_revision);
        for (const auto &alias : it->aliases)
            for (auto prior = revision.commands.begin(); prior != it; ++prior)
                if (std::ranges::find(prior->aliases, alias) != prior->aliases.end())
                    fail(Error::invalid_revision);
    }

    std::vector<ConfiguredCommand> commands;
    if (revision.commands.size() > commands.max_size())
        fail(Error::input_limit);
    commands.reserve(revision.commands.size());
    for (const auto &c : revision.commands) {
        std::vector<EligibilityInstruction> program;
        if (c.eligibility) {
            const auto count = validate_eligibility(*c.eligibility, limits);
            if (count > program.max_size())
                fail(Error::input_limit);
            program.reserve(count);
            walk_eligibility(
                *c.eligibility, static_cast<Index>(limits.eligibility_depth),
                [](const ChatEligibility &) {},
                [&](const ChatEligibility &node) {
                    program.push_back({node.term, node.children.size()});
                });
        }
        // Copy the configuration once. The tree is owned in flat postorder.
        commands.push_back({{c.command,
                             c.aliases,
                             c.arguments,
                             {},
                             c.route,
                             c.actions,
                             c.response,
                             c.global_cooldown_ms,
                             c.user_cooldown_ms},
                            std::move(program),
                            {}});
    }
    return commands;
}

Fact category_fact(const ChatCommandEnvelope &envelope, TwitchChatCategory category) {
    return envelope.categories[static_cast<Index>(category)];
}

Fact eligibility(const ConfiguredCommand &command, const ChatCommandEnvelope &envelope) {
    if (command.eligibility.empty())
        return Fact::present;
    std::vector<Fact> values;
    if (command.eligibility.size() > values.max_size())
        fail(Error::input_limit);
    values.reserve(command.eligibility.size());
    for (const auto &instruction : command.eligibility) {
        if (const auto *category = std::get_if<TwitchChatCategory>(&instruction.term)) {
            values.push_back(category_fact(envelope, *category));
        } else if (const auto *user = std::get_if<ChatExplicitUser>(&instruction.term)) {
            values.push_back(user->twitch_user == envelope.twitch_user ? Fact::present
                                                                       : Fact::absent);
        } else {
            const bool all =
                std::get<ChatEligibilityGroup>(instruction.term) == ChatEligibilityGroup::all;
            const Fact decisive = all ? Fact::absent : Fact::present;
            Fact result = all ? Fact::present : Fact::absent;
            const Index children_start = values.size() - instruction.children;
            for (Index i = children_start; i < values.size(); ++i) {
                if (values[i] == decisive) {
                    result = decisive;
                    break;
                }
                if (values[i] == Fact::unknown)
                    result = Fact::unknown;
            }
            values.resize(children_start);
            values.push_back(result);
        }
    }
    return values.front();
}

struct Argument {
    std::string_view text;
    NumberView number;
};
using Arguments = std::vector<Argument>;

bool bind_arguments(const ChatCommandDefinition &command, const ChatCommandEnvelope &envelope,
                    const ChatCommandLimits &limits, Arguments &arguments) {
    if (command.arguments.size() != envelope.argument_tokens.size())
        return false;
    if (command.arguments.size() > arguments.max_size())
        fail(Error::input_limit);
    arguments.reserve(command.arguments.size());
    for (Index i = 0; i < command.arguments.size(); ++i) {
        const auto type = command.arguments[i].type;
        const std::string_view token = envelope.argument_tokens[i];
        Argument bound{token, {}};
        if (type == ChatArgumentType::resource && !required_reference(token, limits))
            return false;
        if (numeric(type)) {
            const auto slash = token.find('/');
            if (slash == std::string_view::npos) {
                bound.number = {token, "1"};
            } else {
                if (type != ChatArgumentType::rational)
                    return false;
                bound.number = {token.substr(0, slash), token.substr(slash + 1)};
            }
            if (!valid_number(bound.number, limits))
                return false;
        }
        arguments.push_back(bound);
    }
    return true;
}

const Argument &bound_argument(const ChatCommandDefinition &command, const Arguments &arguments,
                               const ChatArgumentReference &ref) {
    return arguments[static_cast<Index>(argument(command, ref) - command.arguments.data())];
}

std::string_view resource_value(const ChatCommandDefinition &command, const Arguments &arguments,
                                const ChatActionResource &resource) {
    if (const auto *literal = std::get_if<std::string>(&resource.value))
        return *literal;
    return bound_argument(command, arguments, std::get<ChatArgumentReference>(resource.value)).text;
}

NumberView quantity_value(const ChatCommandDefinition &command, const Arguments &arguments,
                          const ChatActionQuantity &quantity) {
    if (const auto *literal = std::get_if<PermissionRational>(&quantity.value))
        return view(*literal);
    return bound_argument(command, arguments, std::get<ChatArgumentReference>(quantity.value))
        .number;
}

struct RequestDraft {
    const ChatCommandDefinition &command;
    const Arguments &arguments;
    const ChatCommandAction &action;

    void project(Budget &budget) const {
        budget(action.operation);
        budget.items<PermissionResourceBinding>(action.resources.size());
        for (const auto &binding : action.resources)
            budget(binding.slot, resource_value(command, arguments, binding));
        budget.items<PermissionQuantityBinding>(action.quantities.size());
        for (const auto &binding : action.quantities)
            budget(binding.slot, binding.unit, quantity_value(command, arguments, binding));
    }

    PermissionRequest own() const {
        PermissionRequest request{action.operation, {}, {}};
        request.resources.reserve(action.resources.size());
        request.quantities.reserve(action.quantities.size());
        for (const auto &binding : action.resources)
            request.resources.push_back(
                {binding.slot, std::string(resource_value(command, arguments, binding))});
        for (const auto &binding : action.quantities)
            request.quantities.push_back(
                {binding.slot, binding.unit, quantity_value(command, arguments, binding).own()});
        return request;
    }
};

bool in_domain(const PermissionRational &value, const PermissionQuantityDefinition &domain) {
    return (domain.kind != PermissionNumberKind::integer || value.denominator == "1") &&
           (!domain.minimum || permission::compare(value, *domain.minimum) >= 0) &&
           (!domain.maximum || permission::compare(value, *domain.maximum) <= 0);
}

const PermissionOperationDefinition *operation(const PermissionCatalog &catalog,
                                               std::string_view name) {
    return find_key(catalog.operations, name, &PermissionOperationDefinition::operation);
}

template <class Binding>
bool same_slots(const PermissionOperationDefinition &operation,
                const std::vector<Binding> &resources, const auto &quantities) {
    if (operation.resource_slots.size() != resources.size() ||
        operation.quantity_slots.size() != quantities.size())
        return false;
    for (const auto &binding : resources)
        if (!find_key(operation.resource_slots, binding.slot, identity))
            return false;
    for (const auto &binding : quantities) {
        const auto *slot = find_key(operation.quantity_slots, binding.slot, slot_key);
        if (!slot || slot->unit != binding.unit)
            return false;
    }
    return true;
}

// The catalog and every action shape are checked before either value-domain pass.
std::optional<Disposition> action_domains(const ChatCommandDefinition &command,
                                          const Arguments &arguments,
                                          const PermissionCatalog &catalog) {
    for (const auto &action : command.actions) {
        const auto *op = operation(catalog, action.operation);
        if (!op || !same_slots(*op, action.resources, action.quantities))
            return Disposition::authority_unavailable;
    }
    for (const auto &action : command.actions) {
        const auto &slots = operation(catalog, action.operation)->quantity_slots;
        for (const auto &binding : action.quantities)
            if (const auto *literal = std::get_if<PermissionRational>(&binding.value))
                if (!in_domain(*literal, *find_key(slots, binding.slot, slot_key)))
                    return Disposition::authority_unavailable;
    }
    for (const auto &action : command.actions) {
        const auto &slots = operation(catalog, action.operation)->quantity_slots;
        for (const auto &binding : action.quantities)
            if (std::holds_alternative<ChatArgumentReference>(binding.value) &&
                !in_domain(quantity_value(command, arguments, binding).own(),
                           *find_key(slots, binding.slot, slot_key)))
                return Disposition::invalid_arguments;
    }
    return {};
}

bool permission_available(auto check) {
    try {
        check();
        return true;
    } catch (const EffectivePermissionFailure &failure) {
        using PermissionError = EffectivePermissionError;
        switch (failure.code()) {
        case PermissionError::invalid_limits:
            fail(Error::invalid_limits);
        case PermissionError::input_limit:
            fail(Error::input_limit);
        case PermissionError::result_limit:
            fail(Error::result_limit);
        case PermissionError::allocation_failure:
            fail(Error::allocation_failure);
        default:
            return false;
        }
    }
}

enum class Authority { deny, unavailable, permit, veto };

Authority linked_authority(const ChatCommandAuthority &authority, const PermissionRequest &request,
                           const PermissionOperationDefinition &op,
                           const EffectivePermissionLimits &limits, bool empty_reference) {
    const auto &link = *authority.link;
    const permission::Input input{link.account,
                                  authority.snapshot,
                                  authority.catalog,
                                  authority.owner,
                                  {link.use.state, link.use.assigned_roles, link.use.entries},
                                  request};
    Authority result = Authority::unavailable;
    (void)permission_available([&] {
        // Borrow the existing USE engine's input rather than copying catalog/policy.
        // The catalog, request and exact numbers in them already passed validation.
        permission::validate_owner_tags(input.owner);
        permission::validate_policy_tags(input.policy);
        if (empty_reference || input.owner.state == PermissionOwnerState::unknown ||
            input.policy.state == PermissionFactsState::unknown)
            return;
        permission::validate_policy_shape(input);
        permission::validate_individual_scopes(input.policy);
        for (const auto &row : input.policy.entries)
            for (const auto &quantity : row.quantities)
                for (const auto *end : {&quantity.lower, &quantity.upper})
                    if (*end)
                        permission::validate_number(**end);
        permission::validate_domains(input, op);
        const auto decision = permission::resolve(input, op);
        // Charge the full effective result even though chat needs only its disposition.
        permission::Count projection{0,
                                     0,
                                     limits.result_owned_bytes,
                                     limits.result_items,
                                     limits.reference_bytes,
                                     limits.component_bytes,
                                     EffectivePermissionError::result_limit};
        permission::project_result(input, decision, projection);
        if (decision.outcome == PermissionOutcome::permit)
            result = Authority::permit;
        else if (decision.reason == PermissionReason::individual_deny ||
                 decision.reason == PermissionReason::role_deny)
            result = Authority::veto;
        else
            result = Authority::deny;
    });
    return result;
}

bool valid_grant(const ChatCategoryGrant &row, const PermissionCatalog &catalog) {
    const auto &grant = row.grant;
    const auto *op = operation(catalog, grant.operation);
    if (!valid_tag(row.category, TwitchChatCategory::unsubscribed_viewer) || !op ||
        !unique(grant.resources, slot_key) || !unique(grant.quantities, slot_key) ||
        !same_slots(*op, grant.resources, grant.quantities))
        return false;
    for (const auto &resource : grant.resources) {
        if (!valid_tag(resource.mode, PermissionResourceMode::allowed_set) ||
            (resource.mode == PermissionResourceMode::any) != resource.allowed.empty() ||
            !unique(resource.allowed, identity))
            return false;
    }
    using Mode = PermissionQuantityMode;
    for (const auto &quantity : grant.quantities) {
        const auto &slot = *find_key(op->quantity_slots, quantity.slot, slot_key);
        if (!valid_tag(quantity.mode, Mode::magnitude_at_most) ||
            !(slot.allowed_modes & (1U << static_cast<unsigned>(quantity.mode))) ||
            quantity.lower.has_value() != (quantity.mode == Mode::closed_interval) ||
            quantity.upper.has_value() != (quantity.mode != Mode::any))
            return false;
        for (const auto *end : {&quantity.lower, &quantity.upper})
            if (*end) {
                permission::validate_number(**end);
                if (slot.kind == PermissionNumberKind::integer && (**end).denominator != "1")
                    return false;
            }
        if (quantity.mode == Mode::closed_interval &&
            permission::compare(*quantity.lower, *quantity.upper) > 0)
            return false;
        if (quantity.mode == Mode::magnitude_at_most && permission::sign(*quantity.upper) < 0)
            return false;
    }
    return true;
}

bool quantity_fits(const PermissionQuantityConstraint &scope, const PermissionRational &value) {
    using Mode = PermissionQuantityMode;
    switch (scope.mode) {
    case Mode::any:
        return true;
    case Mode::at_most:
        return permission::compare(value, *scope.upper) <= 0;
    case Mode::closed_interval:
        return permission::compare(value, *scope.lower) >= 0 &&
               permission::compare(value, *scope.upper) <= 0;
    case Mode::magnitude_at_most:
        return permission::compare(value, *scope.upper, true) <= 0;
    }
    return false;
}

bool covers(const DelegatedPermissionGrant &grant, const PermissionRequest &request) {
    if (grant.operation != request.operation)
        return false;
    for (const auto &scope : grant.resources)
        if (!permission::resource_contains(
                scope, find_key(request.resources, scope.slot, slot_key)->resource))
            return false;
    for (const auto &scope : grant.quantities)
        if (!quantity_fits(scope, find_key(request.quantities, scope.slot, slot_key)->value))
            return false;
    return true;
}

Authority category_authority(const ChatCommandAuthority &authority,
                             const ChatCommandEnvelope &envelope,
                             const PermissionRequest &request) {
    auto result = Authority::deny;
    for (const auto &row : authority.category_grants) {
        const auto fact = category_fact(envelope, row.category);
        if (fact != Fact::absent && covers(row.grant, request)) {
            if (fact == Fact::present)
                return Authority::permit;
            result = Authority::unavailable;
        }
    }
    return result;
}

using PermittedActions = std::vector<Index>;

std::optional<Disposition>
authorize(const ChatCommandDefinition &command, const Arguments &arguments,
          const ChatCommandEnvelope &envelope, const ChatCommandAuthority &authority,
          const EffectivePermissionLimits &limits, PermittedActions &permitted) {
    if (authority.source != envelope.message.origin ||
        (command.route != Route::delegated_category && authority.link &&
         authority.link->twitch_user != envelope.twitch_user))
        fail(Error::invalid_envelope);

    auto catalog_budget = permission_input_budget(limits);
    catalog_budget(authority.catalog);
    if (catalog_budget.empty_reference() || !permission_available([&] {
            internal::validate_permission_graph_catalog(authority.catalog);
        }))
        return Disposition::authority_unavailable;
    if (const auto invalid = action_domains(command, arguments, authority.catalog))
        return invalid;

    const bool linked_selected = command.route != Route::delegated_category;
    const bool try_link =
        linked_selected && authority.link_state == PermissionFactsState::complete && authority.link;
    const bool try_category = command.route != Route::linked_account &&
                              authority.category_grants_state == PermissionFactsState::complete;
    // A complete null link is known absent. Unknown or malformed selected link
    // facts remain unavailable and have no veto.
    const auto base_link =
        !linked_selected ||
                (authority.link_state == PermissionFactsState::complete && !authority.link)
            ? Authority::deny
            : Authority::unavailable;
    const auto base_category =
        command.route == Route::linked_account ? Authority::deny : Authority::unavailable;

    if (command.actions.size() > permitted.max_size())
        fail(Error::input_limit);
    permitted.reserve(command.actions.size());
    bool denied = false;
    bool unavailable = false;
    std::optional<bool> category_valid;
    for (Index i = 0; i < command.actions.size(); ++i) {
        const auto &action = command.actions[i];
        const auto &op = *operation(authority.catalog, action.operation);
        if (op.classification != PermissionOperationClass::ordinary) {
            denied |= action.required;
            continue;
        }
        auto linked = base_link;
        auto category = base_category;
        if (try_link || try_category) {
            const RequestDraft draft{command, arguments, action};
            bool link_has_empty_reference = false;
            if (try_link) {
                auto budget = catalog_budget;
                budget(authority.link->account, authority.snapshot, authority.owner,
                       authority.link->use);
                draft.project(budget);
                link_has_empty_reference = budget.empty_reference();
            }
            if (try_category) {
                auto budget = catalog_budget;
                budget(authority.snapshot, authority.category_grants_state,
                       authority.category_grants);
                draft.project(budget);
                // Validate the supplied complete set once. No row can be used until
                // the selected policy has passed exact shape and number validation.
                if (!category_valid) {
                    category_valid = !budget.empty_reference() && permission_available([&] {
                        for (const auto &row : authority.category_grants)
                            if (!valid_grant(row, authority.catalog))
                                permission::fail(EffectivePermissionError::invalid_policy);
                    });
                }
            }
            const auto request = draft.own();
            if (try_link)
                linked = linked_authority(authority, request, op, limits, link_has_empty_reference);
            if (category_valid.value_or(false))
                category = category_authority(authority, envelope, request);
        }
        if (linked == Authority::veto && command.route == Route::both) {
            denied = true;
        } else if (linked == Authority::permit || category == Authority::permit) {
            permitted.push_back(i);
        } else if (action.required) {
            if (linked == Authority::unavailable || category == Authority::unavailable)
                unavailable = true;
            else
                denied = true;
        }
    }
    if (denied)
        return Disposition::required_action_denied;
    if (unavailable)
        return Disposition::authority_unavailable;
    return {};
}

// Text projection and rendering visit the same chunks, including every repetition.
void response_chunks(const ChatCommandDefinition &command, const Arguments &arguments, auto emit) {
    for (const auto &part : command.response->parts) {
        if (const auto *literal = std::get_if<std::string>(&part)) {
            emit(std::string_view(*literal));
        } else {
            const auto &arg =
                bound_argument(command, arguments, std::get<ChatArgumentReference>(part));
            if (arg.number.numerator.empty()) {
                emit(arg.text);
            } else {
                emit(arg.number.numerator);
                if (arg.number.denominator != "1") {
                    emit(std::string_view("/"));
                    emit(arg.number.denominator);
                }
            }
        }
    }
}

ChatCommandResult make_result(const ReceiptIdentity &message, const std::string &revision,
                              Disposition disposition, const ChatCommandDefinition *command,
                              bool replay, const ChatCommandLimits &limits,
                              const Arguments &arguments = {},
                              const PermittedActions &actions = {}) {
    Budget budget(limits.result_bytes, limits.result_items, Error::result_limit);
    const bool proposed = !replay && disposition == Disposition::admitted;
    budget(message, revision, disposition, command != nullptr, replay, proposed);
    if (command)
        budget(command->command);
    Index text_size = 0;
    if (proposed) {
        budget.items<PermissionRequest>(actions.size());
        for (const Index index : actions)
            RequestDraft{*command, arguments, command->actions[index]}.project(budget);
        budget(command->response.has_value());
        if (command->response) {
            budget(command->response->sender);
            response_chunks(*command, arguments, [&](std::string_view chunk) {
                budget(chunk);
                if (chunk.size() > std::string{}.max_size() - text_size)
                    fail(Error::result_limit);
                text_size += chunk.size();
            });
        }
    }

    ChatCommandResult result{message, revision, {disposition, {}}, replay, {}};
    if (command)
        result.terminal.command = command->command;
    if (proposed) {
        auto &proposal = result.proposal.emplace();
        proposal.actions.reserve(actions.size());
        for (const Index index : actions)
            proposal.actions.push_back(
                RequestDraft{*command, arguments, command->actions[index]}.own());
        if (command->response) {
            auto &response = proposal.response.emplace(command->response->sender, std::string{});
            response.text.reserve(text_size);
            response_chunks(*command, arguments,
                            [&](std::string_view chunk) { response.text.append(chunk); });
        }
    }
    return result;
}

struct TerminalRecord {
    ReceiptIdentity message;
    std::optional<Index> command;
    Disposition disposition;
};

struct UserWindow {
    Index command;
    std::string sender;
    Tick start;
};

void validate_limits(const ChatCommandLimits &limits) {
    (void)permission_available([&] { permission::validate_limits(limits.permission); });
    constexpr auto max = static_cast<Tick>(std::numeric_limits<std::ptrdiff_t>::max());
    for (const auto value :
         {limits.input_bytes, limits.input_items, limits.result_bytes, limits.result_items,
          limits.eligibility_depth, limits.terminal_entries, limits.active_user_entries})
        if (value == 0 || value > max)
            fail(Error::invalid_limits);

    // Account for one staged record of each kind during publication.
    const Tick refs = limits.permission.reference_bytes;
    if (refs > max / 4 || limits.terminal_entries > std::list<TerminalRecord>{}.max_size() ||
        limits.active_user_entries > std::list<UserWindow>{}.max_size())
        fail(Error::invalid_limits);
    const auto terminals = limits.terminal_entries + 1;
    const auto users = limits.active_user_entries + 1;
    if (terminals > (max - limits.input_bytes) / (4 * refs))
        fail(Error::invalid_limits);
    const auto terminal_bytes = terminals * (4 * refs);
    if (users > (max - limits.input_bytes - terminal_bytes) / refs)
        fail(Error::invalid_limits);
}

bool active(Tick now, Tick start, Tick duration) noexcept { return now - start < duration; }
} // namespace

ChatCommandFailure::ChatCommandFailure(ChatCommandError code) noexcept : code_(code) {}

ChatCommandError ChatCommandFailure::code() const noexcept { return code_; }

const char *ChatCommandFailure::what() const noexcept { return "Chat command evaluation failed"; }

struct ChatCommandEvaluator::State {
    ChatCommandLimits limits;
    ReceiptOrigin source;
    std::string revision;
    std::vector<ConfiguredCommand> commands;
    std::optional<Tick> last_sample;
    std::list<TerminalRecord> terminals;
    std::list<UserWindow> users;

    State(const ChatCommandRevision &input, const ChatCommandLimits &configured_limits,
          std::vector<ConfiguredCommand> configured_commands)
        : limits(configured_limits), source(input.source), revision(input.revision),
          commands(std::move(configured_commands)) {}

    bool active_user(const UserWindow &user, Tick now) const noexcept {
        return active(now, user.start, commands[user.command].definition.user_cooldown_ms);
    }

    const ChatCommandDefinition *definition(std::optional<Index> command) const {
        return command ? &commands[*command].definition : nullptr;
    }

    Disposition decide(Index index, const ChatCommandEnvelope &envelope,
                       const ChatCommandAuthority &authority, Tick now, Arguments &arguments,
                       PermittedActions &permitted) const {
        const auto &configured = commands[index];
        const auto &command = configured.definition;
        if (!bind_arguments(command, envelope, limits, arguments))
            return Disposition::invalid_arguments;
        const auto eligible = eligibility(configured, envelope);
        if (eligible != Fact::present)
            return eligible == Fact::absent ? Disposition::ineligible
                                            : Disposition::eligibility_unavailable;
        if (!command.actions.empty())
            if (const auto refused = authorize(command, arguments, envelope, authority,
                                               limits.permission, permitted))
                return *refused;

        const bool global = configured.global_start &&
                            active(now, *configured.global_start, command.global_cooldown_ms);
        const bool user = command.user_cooldown_ms > 0 &&
                          std::ranges::any_of(users, [&](const UserWindow &entry) {
                              return entry.command == index &&
                                     entry.sender == envelope.twitch_user &&
                                     active_user(entry, now);
                          });
        if (global && user)
            return Disposition::both_cooldowns;
        if (global)
            return Disposition::global_cooldown;
        if (user)
            return Disposition::user_cooldown;
        return Disposition::admitted;
    }

    ChatCommandResult evaluate(const ChatCommandEnvelope &envelope,
                               const ChatCommandAuthority &authority, Tick now) {
        const auto &key = envelope.message;
        const auto reference = [&](std::string_view value) {
            return valid_reference(value, limits);
        };
        if (key.origin != source || !reference(key.origin.provider) ||
            !reference(key.origin.source_account) || !reference(key.identifier_namespace) ||
            !reference(key.opaque_id))
            fail(Error::invalid_identity);
        const auto retained = std::ranges::find(terminals, key, &TerminalRecord::message);
        if (retained != terminals.end())
            return make_result(key, revision, retained->disposition, definition(retained->command),
                               true, limits);

        Budget input(limits.input_bytes, limits.input_items, Error::input_limit);
        input(envelope, authority, now);
        if (envelope.twitch_user.size() > limits.permission.reference_bytes)
            fail(Error::input_limit);
        if (envelope.twitch_user.empty() || !valid_alias(envelope.alias) ||
            !std::ranges::all_of(envelope.categories,
                                 [](Fact fact) { return valid_tag(fact, Fact::present); }))
            fail(Error::invalid_envelope);
        if (last_sample && now < *last_sample)
            fail(Error::clock_regressed);

        const auto resolved = std::ranges::find_if(commands, [&](const ConfiguredCommand &command) {
            return std::ranges::find(command.definition.aliases, envelope.alias) !=
                   command.definition.aliases.end();
        });
        std::optional<Index> index;
        Arguments arguments;
        PermittedActions permitted;
        auto disposition = Disposition::unrecognized_alias;
        if (resolved != commands.end()) {
            index = static_cast<Index>(resolved - commands.begin());
            disposition = decide(*index, envelope, authority, now, arguments, permitted);
        }

        const bool admitted = disposition == Disposition::admitted;
        const bool start_user = admitted && resolved->definition.user_cooldown_ms > 0;
        if (start_user) {
            Tick running = 0;
            for (const auto &user : users)
                if (active_user(user, now))
                    ++running;
            if (running >= limits.active_user_entries)
                fail(Error::user_cooldown_capacity);
        }
        auto result = make_result(key, revision, disposition, definition(index), false, limits,
                                  arguments, permitted);

        // Prepare all allocations off-ledger. The publication below uses only
        // node transfers, erasure and scalar assignments.
        std::list<TerminalRecord> pending_terminal;
        pending_terminal.emplace_back(key, index, disposition);
        std::list<UserWindow> pending_user;
        if (start_user)
            pending_user.emplace_back(*index, envelope.twitch_user, now);

        if (terminals.size() == limits.terminal_entries)
            terminals.pop_front();
        terminals.splice(terminals.end(), pending_terminal);
        if (start_user) {
            users.remove_if(
                [&](const UserWindow &user) noexcept { return !active_user(user, now); });
            users.splice(users.end(), pending_user);
        }
        if (admitted && resolved->definition.global_cooldown_ms > 0)
            resolved->global_start = now;
        last_sample = now;

        static_assert(std::is_nothrow_move_constructible_v<ChatCommandResult>);
        return result;
    }
};

ChatCommandEvaluator::ChatCommandEvaluator(const ChatCommandRevision &revision,
                                           const ChatCommandLimits &limits)
    : state_(allocation_guard([&] {
          validate_limits(limits);
          auto commands = configure(revision, limits);
          return std::make_unique<State>(revision, limits, std::move(commands));
      })) {}

ChatCommandEvaluator::~ChatCommandEvaluator() = default;

ChatCommandResult ChatCommandEvaluator::evaluate(const ChatCommandEnvelope &envelope,
                                                 const ChatCommandAuthority &authority,
                                                 std::uint64_t monotonic_ms) {
    return allocation_guard([&] { return state_->evaluate(envelope, authority, monotonic_ms); });
}

} // namespace subathon::core
