// SPDX-License-Identifier: GPL-3.0-or-later
#include "reference_authority_values.hpp"
#include "reference_capture_codec_p.hpp"
#include <QCryptographicHash>
#include <array>
#include <charconv>
#include <subathon/server/reference_application.hpp>
#include <tuple>

namespace subathon::server {
namespace {
using namespace core;
using Error = ReferenceInputError;
using Limits = ReferenceAdapterLimits;
using reference_authority::fields;
[[noreturn]] void fail(Error e) { throw ReferenceInputFailure(e); }
[[noreturn]] void corrupt() {
    throw ReferenceExecutionFailure(ReferenceAuthorityFailure::invalid_data);
}
void require(bool b, Error e) {
    if (!b)
        fail(e);
}
void data(bool b) {
    if (!b)
        corrupt();
}
#define FIELDS(Type, ...)                                                                          \
    template <class P> void fields(P &p, const Type &v) { p(__VA_ARGS__); }
FIELDS(TimerConfig, v.end_mode, v.grace_duration_ms, v.grace_reuse, v.hard_end_utc_ms)
FIELDS(ReferenceConfiguration, v.configuration_ref, v.policy_revision_ref, v.synthetic_origin,
       v.receipt_namespace, v.rule, v.maximum_main_ms, v.timer_config, v.other_rules, v.modifiers,
       v.milestones, v.outbound_actions, v.generated_events)
FIELDS(ReferenceSyntheticInput, v.connection_ref, v.event_id, v.source_utc_ms, v.original_payload)
FIELDS(ReferenceArrivalCapture, v.arrival_context_ref, v.event_ref, v.server_arrival_utc_ms,
       v.configuration, v.observation)
FIELDS(ReferenceEnabledRuleFacts, v.rule, v.trigger_types, v.action_kinds, v.action_units,
       v.conditions, v.exclusions, v.execution_limits)
FIELDS(ReferenceCategoryFacts, v.state, v.enabled_refs)
FIELDS(ReferenceConfigurationFacts, v.configuration_ref, v.configuration_owner_revision_ref,
       v.projection, v.rules_state, v.enabled_rules, v.modifiers, v.milestones, v.outbound_actions,
       v.generated_events)
FIELDS(ReferenceUnavailable, v.gate, v.reason_ref)
FIELDS(ReferenceAdmittedConfiguration, v.configuration, v.configuration_owner_revision_ref)
#undef FIELDS

// Fixed-depth declaration traversal; variants charge only their held alternative.
// No dynamic indices, caller capacities, sorting or owned validation cache.
struct Projection {
    std::uint64_t cap, item_cap, reference, payload, bytes = 0, items = 0;
    Error error = Error::input_limit;
    std::uint64_t component_bytes = UINT64_MAX;
    void fixed(std::uint64_t n) {
        require(bytes <= cap && n <= cap - bytes, error);
        bytes += n;
    }
    void component(const std::string &s) {
        require(s.size() <= component_bytes && s.size() < std::string{}.max_size(), error);
        fixed(s.size());
    }
    void one(std::string_view s) {
        require(s.size() <= reference && s.size() <= UINT32_MAX &&
                    s.size() < std::string{}.max_size(),
                error);
        fixed(s.size());
    }
    void one(const std::string &s) { one(std::string_view(s)); }
    void one(const QByteArray &s) {
        require(std::uint64_t(s.size()) <= payload && std::uint64_t(s.size()) <= UINT32_MAX &&
                    s.size() < QByteArray::maxSize(),
                error);
        fixed(std::uint64_t(s.size()));
    }
    template <class T>
        requires(std::is_enum_v<T> || std::is_integral_v<T>)
    void one(T) {
        fixed(std::is_enum_v<T> ? 1 : sizeof(T));
    }
    template <class T> void one(const std::optional<T> &v) {
        fixed(1);
        if (v)
            one(*v);
    }
    template <class T> void one(const std::vector<T> &v) {
        require(v.size() <= v.max_size() && v.capacity() <= v.max_size() && items <= item_cap &&
                    v.size() <= item_cap - items,
                error);
        items += v.size();
        for (const auto &x : v)
            one(x);
    }
    template <class... T> void one(const std::variant<T...> &v) {
        require(!v.valueless_by_exception(), Error::invalid_shape);
        std::visit([&](const auto &x) { one(x); }, v);
    }
    template <class T>
        requires(!std::is_enum_v<T> && !std::is_integral_v<T>)
    void one(const T &v) {
        fields(*this, v);
    }
    template <class... T> void operator()(const T &...v) { (one(v), ...); }
};
Projection projection(const Limits &l, Error e = Error::input_limit) {
    return {l.value_owned_bytes, l.value_items, l.reference_bytes, l.payload_bytes, 0, 0, e};
}
void valid_limits(const Limits &l) {
    const auto max = std::min({std::uint64_t(SIZE_MAX), std::uint64_t(PTRDIFF_MAX),
                               std::uint64_t(QByteArray::maxSize()), std::uint64_t(INT64_MAX),
                               std::uint64_t(std::string{}.max_size())});
    for (auto n : {l.reference_bytes, l.payload_bytes, l.document_bytes, l.value_owned_bytes,
                   l.value_items, l.clock_journal_entries})
        require(n && n < max, Error::invalid_limits);
    require(l.value_items < std::vector<ReferenceEnabledRuleFacts>{}.max_size() &&
                l.clock_journal_entries < std::vector<TimerClockStep>{}.max_size(),
            Error::invalid_limits);
}
// Separate whole-tree passes preserve tag -> required-reference priority globally.
struct Semantics {
    bool tag_pass;
    void one(const std::string &s) {
        if (!tag_pass)
            require(!s.empty(), Error::invalid_reference);
    }
    void one(const QByteArray &) {}
    template <class T>
        requires std::is_integral_v<T>
    void one(T) {}
    template <class T>
        requires std::is_enum_v<T>
    void one(T v) {
        if (!tag_pass)
            return;
        int maximum = 1;
        if constexpr (std::is_same_v<T, TimerEndMode>)
            maximum = 2;
        if constexpr (std::is_same_v<T, ReferenceGate>)
            maximum = 3;
        require(static_cast<std::underlying_type_t<T>>(v) >= 0 &&
                    static_cast<std::uint64_t>(v) <= std::uint64_t(maximum),
                Error::invalid_tag);
    }
    template <class T> void one(const std::optional<T> &v) {
        if (v)
            one(*v);
    }
    template <class T> void one(const std::vector<T> &v) {
        for (const auto &x : v)
            one(x);
    }
    template <class... T> void one(const std::variant<T...> &v) {
        std::visit([&](const auto &x) { one(x); }, v);
    }
    template <class T>
        requires(!std::is_enum_v<T> && !std::is_integral_v<T>)
    void one(const T &v) {
        fields(*this, v);
    }
    template <class... T> void operator()(const T &...v) { (one(v), ...); }
};
// Compare declared fields without serializing or allocating either side.
template <class T> bool same(const T &a, const T &b);
template <class T> bool same(const std::optional<T> &a, const std::optional<T> &b) {
    return bool(a) == bool(b) && (!a || same(*a, *b));
}
template <class T> bool same(const std::vector<T> &a, const std::vector<T> &b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (!same(a[i], b[i]))
            return false;
    return true;
}
template <class T> bool same(const T &a, const T &b) {
    if constexpr (std::is_enum_v<T> || std::is_integral_v<T> || std::is_same_v<T, std::string>) {
        return a == b;
    } else {
        bool equal = true;
        auto left = [&](const auto &...x) {
            auto right = [&](const auto &...y) {
                auto tx = std::tie(x...);
                auto ty = std::tie(y...);
                [&]<std::size_t... I>(std::index_sequence<I...>) {
                    equal = (same(std::get<I>(tx), std::get<I>(ty)) && ...);
                }(std::index_sequence_for<decltype(x)...>{});
            };
            fields(right, b);
        };
        fields(left, a);
        return equal;
    }
}
template <class T, class Key> void unique(const std::vector<T> &v, Key key) {
    for (std::size_t i = 0; i < v.size(); ++i)
        for (std::size_t j = 0; j < i; ++j)
            require(key(v[i]) != key(v[j]), Error::invalid_shape);
}
void unique(const std::vector<std::string> &v) {
    unique(v, [](const auto &x) -> const auto & { return x; });
}
void numeric(const ReferenceRule &r) { require(r.add_ms >= 0, Error::invalid_value); }
void numeric(const ReferenceConfiguration &c) {
    numeric(c.rule);
    require((!c.maximum_main_ms || *c.maximum_main_ms >= 0) &&
                c.timer_config.grace_duration_ms >= 0,
            Error::invalid_value);
}
void binding(const ReferenceRule &r) {
    require(r.automation.kind == ApplicationPrincipalKind::automation, Error::invalid_binding);
}
void arrival_shape(const ReferenceArrivalCapture &a) {
    require(!a.observation.original_payload.isNull(), Error::invalid_shape);
}
void facts_shape(const ReferenceConfigurationResolution &r) {
    if (const auto *u = std::get_if<ReferenceUnavailable>(&r)) {
        require(u->gate == ReferenceGate::not_ready ||
                    u->gate == ReferenceGate::unsupported_configuration,
                Error::invalid_shape);
        return;
    }
    const auto &f = std::get<ReferenceConfigurationFacts>(r);
    require(f.rules_state != ReferenceFactsState::unknown || f.enabled_rules.empty(),
            Error::invalid_shape);
    unique(f.enabled_rules, [](const auto &v) -> const auto & { return v.rule.rule_ref; });
    for (const auto &v : f.enabled_rules) {
        unique(v.trigger_types);
        unique(v.action_kinds);
        unique(v.action_units);
        unique(v.conditions);
        unique(v.exclusions);
        unique(v.execution_limits);
    }
    for (auto c : {&f.modifiers, &f.milestones, &f.outbound_actions, &f.generated_events}) {
        require(c->state != ReferenceFactsState::unknown || c->enabled_refs.empty(),
                Error::invalid_shape);
        unique(c->enabled_refs);
    }
}
void facts_numeric(const ReferenceConfigurationResolution &r) {
    if (const auto *f = std::get_if<ReferenceConfigurationFacts>(&r)) {
        numeric(f->projection);
        for (const auto &v : f->enabled_rules)
            numeric(v.rule);
    }
}
void facts_binding(const std::string &expected, const ReferenceConfigurationResolution &r) {
    if (const auto *f = std::get_if<ReferenceConfigurationFacts>(&r)) {
        const auto &c = f->projection;
        require(f->configuration_ref == expected && c.configuration_ref == expected,
                Error::invalid_binding);
        binding(c.rule);
        for (const auto &v : f->enabled_rules)
            binding(v.rule);
        if (f->enabled_rules.size() == 1)
            require(same(c.rule, f->enabled_rules[0].rule), Error::invalid_binding);
        const std::array pairs{std::pair{c.other_rules, f->rules_state},
                               std::pair{c.modifiers, f->modifiers.state},
                               std::pair{c.milestones, f->milestones.state},
                               std::pair{c.outbound_actions, f->outbound_actions.state},
                               std::pair{c.generated_events, f->generated_events.state}};
        for (auto [p, s] : pairs)
            require(p != ReferenceFactsState::complete || s == ReferenceFactsState::complete,
                    Error::invalid_binding);
    }
}
const ReferenceConfigurationFacts *supported(const ReferenceConfigurationResolution &r) {
    return std::get_if<ReferenceConfigurationFacts>(&r);
}
std::optional<ReferenceGate> refusal(const ReferenceConfigurationFacts &f) {
    for (auto s : {f.rules_state, f.modifiers.state, f.milestones.state, f.outbound_actions.state,
                   f.generated_events.state})
        if (s == ReferenceFactsState::unknown)
            return ReferenceGate::not_ready;
    if (f.enabled_rules.size() != 1 || !f.modifiers.enabled_refs.empty() ||
        !f.milestones.enabled_refs.empty() || !f.outbound_actions.enabled_refs.empty() ||
        !f.generated_events.enabled_refs.empty() || f.projection.timer_config.hard_end_utc_ms ||
        f.projection.synthetic_origin.provider != "synthetic.reference.v1")
        return ReferenceGate::unsupported_configuration;
    const auto &v = f.enabled_rules[0];
    const auto singleton = [](const auto &x, std::string_view expected) {
        return x.size() == 1 && x[0] == expected;
    };
    if (!singleton(v.trigger_types, "synthetic.reference.support.v1") ||
        !singleton(v.action_kinds, "time_add") || !singleton(v.action_units, "ms") ||
        !v.conditions.empty() || !v.exclusions.empty() || !v.execution_limits.empty())
        return ReferenceGate::unsupported_configuration;
    return std::nullopt;
}
ReferenceUnavailable unavailable(ReferenceGate g, const Limits &l) {
    const std::string_view reason = g == ReferenceGate::not_ready
                                        ? "reference.configuration-unavailable.v1"
                                        : "reference.configuration-unsupported.v1";
    auto p = projection(l, Error::result_limit);
    p.fixed(1);
    p(reason);
    return {g, std::string(reason)};
}
template <class F> auto allocate(F f) {
    try {
        return f();
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_failure);
    } catch (const std::length_error &) {
        fail(Error::allocation_failure);
    }
}
ReferenceUnavailable copy_refusal(const ReferenceUnavailable &u, const Limits &l) {
    auto p = projection(l, Error::result_limit);
    p(u);
    return u;
}
ReferenceAdmittedConfiguration admitted(const ReferenceConfigurationFacts &f, const Limits &l) {
    auto p = projection(l, Error::result_limit);
    p(f.projection, f.configuration_owner_revision_ref);
    ReferenceAdmittedConfiguration out{f.projection, f.configuration_owner_revision_ref};
    auto &c = out.configuration;
    c.other_rules = c.modifiers = c.milestones = c.outbound_actions = c.generated_events =
        ReferenceFactsState::complete;
    return out;
}
ReferenceRuleEvaluation derive(const ReferenceRule &r, const Limits &l) {
    // Preproject the adapter's intermediate core input before its owned strings.
    auto p = projection(l);
    p(r.rule_ref, r.rule_version_ref, r.action_ref, r.action_version_ref, r.timer_ref);
    p.fixed(9);
    p(std::string_view("synthetic.reference.support.v1"),
      std::string_view("synthetic.reference.support.v1"), std::string_view("time_add"),
      std::string_view("ms"));
    ReferenceRuleInput in{r.rule_ref,
                          r.rule_version_ref,
                          r.action_ref,
                          r.action_version_ref,
                          r.timer_ref,
                          true,
                          "synthetic.reference.support.v1",
                          "synthetic.reference.support.v1",
                          "time_add",
                          "ms",
                          r.add_ms};
    try {
        auto out =
            derive_reference_rule(in, {l.reference_bytes, l.value_owned_bytes, l.value_items});
        return {std::move(out.selection), std::move(*out.request)};
    } catch (const ReferenceDerivationFailure &e) {
        switch (e.code()) {
        case ReferenceDerivationError::invalid_limits:
            fail(Error::invalid_limits);
        case ReferenceDerivationError::input_limit:
            fail(Error::input_limit);
        case ReferenceDerivationError::invalid_reference:
            fail(Error::invalid_reference);
        case ReferenceDerivationError::invalid_value:
            fail(Error::invalid_value);
        case ReferenceDerivationError::result_limit:
            fail(Error::result_limit);
        case ReferenceDerivationError::allocation_failure:
            fail(Error::allocation_failure);
        }
        throw;
    }
}
} // namespace
ReferenceAdmissionResolution
admit_reference_configuration(const std::string &expected,
                              const ReferenceConfigurationResolution &r, const Limits &l) {
    valid_limits(l);
    auto p = projection(l);
    p(expected, r);
    Semantics tags{true};
    tags(expected, r);
    Semantics refs{false};
    refs(expected, r);
    facts_shape(r);
    facts_numeric(r);
    facts_binding(expected, r);
    return allocate([&]() -> ReferenceAdmissionResolution {
        const auto *f = supported(r);
        if (!f)
            return copy_refusal(std::get<ReferenceUnavailable>(r), l);
        if (auto g = refusal(*f))
            return unavailable(*g, l);
        return admitted(*f, l);
    });
}
ReferenceRuleResolution evaluate_reference_rule(const ReferenceArrivalCapture &a,
                                                const ReferenceConfigurationResolution &r,
                                                const Limits &l) {
    valid_limits(l);
    auto p = projection(l);
    p(a, r);
    Semantics tags{true};
    tags(a, r);
    Semantics refs{false};
    refs(a, r);
    arrival_shape(a);
    facts_shape(r);
    numeric(a.configuration);
    facts_numeric(r);
    binding(a.configuration.rule);
    facts_binding(a.configuration.configuration_ref, r);
    return allocate([&]() -> ReferenceRuleResolution {
        const auto *f = supported(r);
        if (!f)
            return copy_refusal(std::get<ReferenceUnavailable>(r), l);
        if (auto g = refusal(*f))
            return unavailable(*g, l);
        const auto config = admitted(*f, l);
        require(same(a.configuration, config.configuration), Error::invalid_binding);
        return derive(a.configuration.rule, l);
    });
}
namespace {
using AuthorityFailure = ReferenceAuthorityFailure;
AccessDecision authority_failure(AuthorityFailure f) {
    switch (f) {
    case AuthorityFailure::denied:
        return ApplicationAccessDenied{};
    case AuthorityFailure::missing:
    case AuthorityFailure::unavailable:
    case AuthorityFailure::resource_limit:
        return ApplicationAccessUnknown{};
    default:
        corrupt();
    }
}
void authority_limits(const ReferenceAuthorityLimits &l) {
    reference_authority::valid_limits(l);
    // The imported validator checks arithmetic (including the permission L^4
    // bound). Check the independently configurable container ceilings as well.
    const auto invalid = [](bool b) {
        if (b)
            throw ReferenceAuthorityInputFailure(AuthorityFailure::invalid_data);
    };
    invalid(l.graph.rows >= std::vector<StoredAccountFacts>{}.max_size());
    invalid(l.graph.items >= std::vector<PermissionPolicyEntry>{}.max_size());
    invalid(l.result_items >= std::vector<PermissionPolicyEntry>{}.max_size());
    invalid(l.permissions.result_items >= std::vector<PermissionSourceExplanation>{}.max_size());
}
void command_preflight(const OriginalApplicationCommand &c, const Limits &l) {
    // The owning service codec applies its default conditional/shape/wire checks
    // after this complete borrowed pass, before any command envelope is built.
    auto p = projection(l);
    p(c.operation, c.subject_kind, c.receipt_id.value);
    p.fixed(1);
    if (c.contribution_id)
        p(c.contribution_id->value);
    p(c.plan, c.principal, c.purpose, c.preview_mode, c.preview_state_token, c.actor_ref,
      c.decision_ref);
    if (c.reason) {
        p.fixed(1);
        require(c.reason->size() <= l.document_bytes, Error::input_limit);
        p.fixed(c.reason->size());
    } else
        p.fixed(1);
    require(c.choices.size() <= c.choices.max_size() &&
                c.choices.capacity() <= c.choices.max_size() && c.choices.size() <= p.item_cap,
            Error::input_limit);
    p.items = c.choices.size();
    for (const auto &v : c.choices) {
        p(v.format_ref);
        require(std::uint64_t(v.document.size()) <= l.document_bytes, Error::input_limit);
        p.fixed(std::uint64_t(v.document.size()));
    }
}
void command_envelope_preflight(const OriginalApplicationCommand &c, const Limits &l) {
    // Size the service's SAC1 intermediate without building a command or buffer.
    // The imported codec still validates meanings and its own defaults afterward.
    auto value = projection(l, Error::result_limit);
    value(std::string_view("subathon.internal.application-command.v1"));
    const auto body_cap = std::min(l.document_bytes, std::uint64_t{UINT32_MAX});
    std::uint64_t body = 0;
    const auto fixed = [&](std::uint64_t n) {
        require(body <= body_cap && n <= body_cap - body, Error::result_limit);
        body += n;
        value.fixed(n);
    };
    const auto blob = [&](std::uint64_t n) {
        require(n <= UINT32_MAX, Error::result_limit);
        fixed(4);
        fixed(n);
    };
    fixed(4); // SAC1
    blob(c.operation.operation_scope.size());
    blob(c.operation.operation_ref.size());
    fixed(1 + 8 + 1); // subject, receipt, contribution presence
    if (c.contribution_id)
        fixed(8);
    blob(c.plan.plan_ref.size());
    fixed(8 + 1); // plan revision, principal kind
    blob(c.principal.installation_ref.size());
    blob(c.principal.principal_ref.size());
    fixed(1 + 1); // purpose, preview mode
    for (const auto *v : {&c.preview_state_token, &c.actor_ref, &c.decision_ref, &c.reason}) {
        fixed(1);
        if (*v)
            blob((*v)->size());
    }
    require(c.choices.size() <= UINT32_MAX, Error::result_limit);
    fixed(4);
    for (const auto &v : c.choices) {
        blob(v.format_ref.size());
        blob(std::uint64_t(v.document.size()));
    }
    // The complete envelope has no vector items: choices are framed body bytes.
}
std::optional<RetainedDocument> read(const ApplicationReferenceReadView &v, RetainedKind kind,
                                     const std::string &ref, const Limits &l) {
    auto p = projection(l);
    p(kind, ref); // before constructing the owning key
    auto key = allocate([&] { return RetainedKey{kind, ref}; });
    auto out = v.find_retained(key); // Original reader/work/view exceptions escape.
    if (out) {
        auto result = projection(l, Error::result_limit);
        result(out->key.kind, out->key.reference);
        require(std::uint64_t(out->document.size()) <= l.document_bytes &&
                    std::uint64_t(out->document.size()) <= UINT32_MAX,
                Error::result_limit);
        result.fixed(std::uint64_t(out->document.size()));
        result.fixed(1);
        if (out->configuration)
            result(out->configuration->reference);
        data(out->key == key && !out->document.isNull());
    }
    return out;
}
std::string_view bytes(const QByteArray &b) { return {b.constData(), std::size_t(b.size())}; }
std::string execution_ref(const OriginalApplicationCommand &c, const Limits &l) {
    data(c.principal.kind == ApplicationPrincipalKind::automation &&
         c.subject_kind == ApplicationSubjectKind::contribution && c.contribution_id &&
         c.purpose == OriginalApplicationPurpose::captured_original && c.choices.size() == 1);
    const auto &choice = c.choices[0];
    data(choice.format_ref == "subathon.reference.execution-ref.v1");
    const auto b = bytes(choice.document);
    data(!choice.document.isNull() && b.size() >= 8 && b.substr(0, 4) == "SRF1");
    std::uint32_t length = 0;
    for (unsigned char x : b.substr(4, 4))
        length = (length << 8) | x;
    data(b.size() - 8 == length && length != 0);
    auto p = projection(l);
    p(b.substr(8));
    return std::string(b.substr(8));
}
void match_command(const OriginalApplicationCommand &c, const ReferenceExecutionBinding &x) {
    data(same(c.principal, x.rule.automation) &&
         c.principal.installation_ref == x.installation_ref && c.receipt_id == x.receipt_id &&
         c.contribution_id && *c.contribution_id == x.contribution_id && same(c.plan, x.plan));
}
// Full, raw catalog equality against the selected compiled reference profile.
// Constants are borrowed; no temporary owning catalog precedes nested preflight.
bool reference_catalog(const PermissionCatalog &c) {
    constexpr std::array<std::string_view, 10> names{
        "reference.time-add.v1",  "reference.application-read.v1",
        "reference.hold-read.v1", "roles.edit",
        "members.edit",           "overrides.edit",
        "access.manage",          "owner.recover",
        "helper.reset",           "timer.snapshot.read"};
    if (c.state != PermissionFactsState::complete || c.revision != "permission-reference-v1" ||
        c.operations.size() != names.size())
        return false;
    for (std::size_t i = 0; i < names.size(); ++i) {
        const auto &o = c.operations[i];
        const auto classification = i == 6   ? PermissionOperationClass::protected_access_management
                                    : i == 7 ? PermissionOperationClass::protected_recovery
                                    : i == 8 ? PermissionOperationClass::helper_password_reset
                                             : PermissionOperationClass::ordinary;
        if (o.operation != names[i] || o.classification != classification)
            return false;
        if (i < 3) {
            if (o.resource_slots.size() != 3 || o.resource_slots[0] != "installation" ||
                o.resource_slots[1] != "timer" || o.resource_slots[2] != "source")
                return false;
        } else if (i == 6 || i == 7) {
            if (!o.resource_slots.empty())
                return false;
        } else {
            const auto slot = i == 3 ? "role" : i == 8 ? "target" : i == 9 ? "timer" : "account";
            if (o.resource_slots.size() != 1 || o.resource_slots[0] != slot)
                return false;
        }
        if (i == 8) {
            if (!o.helper_target_slot || *o.helper_target_slot != "target")
                return false;
        } else if (o.helper_target_slot)
            return false;
        if (i != 0) {
            if (!o.quantity_slots.empty())
                return false;
        } else {
            if (o.quantity_slots.size() != 1)
                return false;
            const auto &q = o.quantity_slots[0];
            if (q.slot != "adjustment" || q.unit != "ms" ||
                q.kind != PermissionNumberKind::integer || q.allowed_modes != 15 || !q.minimum ||
                q.minimum->numerator != "0" || q.minimum->denominator != "1" || !q.maximum ||
                q.maximum->numerator != "9223372036854775807" || q.maximum->denominator != "1")
                return false;
        }
    }
    return true;
}
void registry_binding(const ReferenceAuthorityRecord &r, const ReferenceAuthorityRequest &q) {
    const auto &b = r.binding;
    const auto &x = q.captured;
    const auto &rule = x.rule;
    data(b.installation == x.installation_ref && b.capability == rule.automation_capability_ref &&
         b.principal == q.principal.principal_ref && b.rule == rule.rule_ref &&
         b.rule_version == rule.rule_version_ref && b.action == rule.action_ref &&
         b.action_version == rule.action_version_ref && b.timer == rule.timer_ref &&
         b.origin == x.source.origin && b.identifier_namespace == x.source.identifier_namespace &&
         b.requested_ms == rule.add_ms && b.requested_ms >= 0 && !r.original_author.empty() &&
         !r.execution_account.empty() && !r.actor.empty() && !r.session.empty() &&
         !r.session_revision.empty() && r.created_revision != 0 &&
         r.created_revision <= r.state_revision && r.state_revision <= r.head_revision &&
         unsigned(r.state) <= unsigned(ReferenceAuthorityState::revoked));
}
void fingerprint(const ReferenceAuthorityRecord &r, std::string_view expected) {
    data(expected.size() == 36 && expected.substr(0, 4) == "RAA1");
    QCryptographicHash hash(QCryptographicHash::Sha256);
    auto raw = [&](std::string_view b) {
        hash.addData(QByteArrayView(b.data(), qsizetype(b.size())));
    };
    auto integer = [&](std::uint64_t value, unsigned width) {
        std::array<char, 8> b{};
        for (unsigned i = 0; i < width; ++i)
            b[width - i - 1] = char((value >> (8 * i)) & 255);
        raw({b.data(), width});
    };
    auto field = [&](std::string_view b) {
        data(b.size() <= UINT32_MAX);
        integer(b.size(), 4);
        raw(b);
    };
    raw("RAA1");
    field(r.binding.installation);
    integer(r.head_revision, 8);
    field(r.binding.capability);
    integer(r.created_revision, 8);
    integer(r.state_revision, 8);
    integer(unsigned(r.state), 1);
    field(r.original_author);
    field(r.execution_account);
    auto digest = hash.resultView();
    data(expected.substr(4) == std::string_view(digest.data(), std::size_t(digest.size())));
}
std::uint64_t source_size(const ReferenceAuthorityBinding &b, const ReferenceAuthorityLimits &l) {
    const auto ceiling = std::min(l.reference_bytes, l.permissions.reference_bytes);
    require(ceiling >= 16, Error::result_limit);
    std::uint64_t n = 16;
    for (std::string_view s :
         {std::string_view(b.origin.provider), std::string_view(b.origin.source_account),
          std::string_view(b.identifier_namespace)}) {
        require(s.size() <= UINT32_MAX && s.size() <= ceiling - n, Error::result_limit);
        n += s.size();
    }
    require(n < std::string{}.max_size(), Error::result_limit);
    return n;
}
bool source_scope(std::string_view actual, const ReferenceAuthorityBinding &b, std::uint64_t n) {
    if (actual.size() != n || actual.substr(0, 4) != "RSS1")
        return false;
    std::size_t at = 4;
    for (std::string_view s :
         {std::string_view(b.origin.provider), std::string_view(b.origin.source_account),
          std::string_view(b.identifier_namespace)}) {
        std::uint32_t length = 0;
        for (unsigned char x : actual.substr(at, 4))
            length = (length << 8) | x;
        at += 4;
        if (length != s.size() || actual.substr(at, s.size()) != s)
            return false;
        at += s.size();
    }
    return true;
}
void action_scope(const ReferenceAuthorityFacts &f, const ReferenceAuthorityLimits &l) {
    const auto &b = f.registry.binding;
    const auto n = source_size(b, l); // checked RSS1 before any construction/numeric work
    std::array<char, 20> decimal{};
    const auto end = std::to_chars(decimal.data(), decimal.data() + decimal.size(), b.requested_ms);
    const std::string_view amount(decimal.data(), std::size_t(end.ptr - decimal.data()));
    require(amount.size() <= l.permissions.component_bytes, Error::result_limit);
    const auto rational = [&](const PermissionRational &r) {
        return r.numerator == amount && r.denominator == "1";
    };
    const auto resource = [&](std::string_view slot, std::string_view value) {
        if (slot == "installation")
            return value == b.installation;
        if (slot == "timer")
            return value == b.timer;
        return slot == "source" && source_scope(value, b, n);
    };
    const auto &request = f.permission_input.request;
    data(request.operation == "reference.time-add.v1" && request.resources.size() == 3 &&
         request.quantities.size() == 1);
    for (std::size_t i = 0; i < request.resources.size(); ++i) {
        const auto &r = request.resources[i];
        data(resource(r.slot, r.resource));
        for (std::size_t j = 0; j < i; ++j)
            data(r.slot != request.resources[j].slot);
    }
    const auto &q = request.quantities[0];
    data(q.slot == "adjustment" && q.unit == "ms" && rational(q.value));
    data(f.original_author_input.actions.size() == 1);
    const auto &a = f.original_author_input.actions[0];
    data(a.operation == request.operation && a.resources.size() == 3 && a.quantities.size() == 1);
    for (std::size_t i = 0; i < a.resources.size(); ++i) {
        const auto &r = a.resources[i];
        data(r.mode == PermissionResourceMode::allowed_set && r.allowed.size() == 1 &&
             resource(r.slot, r.allowed[0]));
        for (std::size_t j = 0; j < i; ++j)
            data(r.slot != a.resources[j].slot);
    }
    const auto &quantity = a.quantities[0];
    data(quantity.slot == "adjustment" && quantity.unit == "ms" &&
         quantity.mode == PermissionQuantityMode::closed_interval && quantity.lower &&
         quantity.upper && rational(*quantity.lower) && rational(*quantity.upper));
}
void authority_preflight(const ReferenceAuthorityResolution &r, const ReferenceAuthorityLimits &l) {
    Projection all{l.result_bytes,
                   l.result_items,
                   l.reference_bytes,
                   l.result_bytes,
                   0,
                   0,
                   Error::result_limit,
                   l.permissions.component_bytes};
    all(r.value);
    if (const auto *f = std::get_if<ReferenceAuthorityFacts>(&r.value)) {
        // Each imported calculator retains its own WHOLE argument charge, not
        // merely the selected operation/row. This happens before fingerprint work.
        Projection nested{l.permissions.input_owned_bytes,
                          l.permissions.input_items,
                          l.permissions.reference_bytes,
                          0,
                          0,
                          0,
                          Error::result_limit,
                          l.permissions.component_bytes};
        nested(f->original_author_input);
        nested.bytes = nested.items = 0;
        nested(f->permission_input);
    }
}
AccessDecision consume(const ReferenceAuthorityResolution &r, const ReferenceAuthorityRequest &q,
                       const ReferenceAuthorityLimits &l) {
    authority_preflight(r, l);
    if (const auto *f = std::get_if<AuthorityFailure>(&r.value))
        return authority_failure(*f);
    const auto &f = std::get<ReferenceAuthorityFacts>(r.value);
    data(same(f.binding, q));
    registry_binding(f.registry, q);
    if (f.registry.state != ReferenceAuthorityState::authorized)
        return ApplicationAccessDenied{};
    const auto &a = f.original_author_input;
    const auto &e = f.permission_input;
    data(!f.permission_revision_ref.empty() && !f.automation_revision_ref.empty() &&
         f.permission_revision_ref.size() == 36 &&
         f.permission_revision_ref.compare(0, 4, "RAR1") == 0 &&
         a.snapshot == f.permission_revision_ref && e.snapshot == f.permission_revision_ref &&
         a.author == f.registry.original_author && e.actor == f.registry.execution_account &&
         a.owner.state == PermissionOwnerState::present && a.owner.account &&
         !a.owner.account->empty() && same(a.owner, e.owner) && reference_catalog(a.catalog) &&
         reference_catalog(e.catalog));
    // Preflight the full RSS1/composite and complete singleton before hash or kernels.
    action_scope(f, l);
    return allocate([&]() -> AccessDecision {
        fingerprint(f.registry, f.automation_revision_ref);
        const auto author = revalidate_permission_automation(a, l.permissions);
        // Evaluate both independently. A denied first channel does not hide an
        // invalid/unknown second raw policy.
        const auto subject = evaluate_effective_permission(e, l.permissions);
        data(author.actions.size() == 1);
        if (author.actions[0].disposition != PermissionAutomationDisposition::authorized ||
            subject.data().outcome != PermissionOutcome::permit)
            return ApplicationAccessDenied{};
        return ApplicationAccessAllowed{f.permission_revision_ref};
    });
}
AccessDecision execute(const ApplicationReferenceReadView &view,
                       const OriginalApplicationCommand &c, ReferenceConfigurationSource &configs,
                       ReferenceAuthoritySource &authority, const Limits &l,
                       const ReferenceAuthorityLimits &al) {
    command_preflight(c, l);
    command_envelope_preflight(c, l);
    auto ref = allocate([&] {
        (void)encode_original_application_command(c); // imported complete service validation
        return execution_ref(c, l);
    });
    auto execution_document = read(view, RetainedKind::arrival_context, ref, l);
    if (!execution_document)
        return ApplicationAccessUnknown{};
    data(execution_document->configuration &&
         !execution_document->configuration->reference.empty());
    const auto x = allocate([&] {
        return reference_capture_detail::decode_execution_body(
            bytes(execution_document->document), l, reference_capture_detail::BodyBounds::retained);
    });
    data(x.execution_context_ref == ref);
    match_command(c, x);
    auto arrival_document = read(view, RetainedKind::arrival_context, x.arrival_context_ref, l);
    if (!arrival_document)
        return ApplicationAccessUnknown{};
    data(arrival_document->configuration == execution_document->configuration);
    const auto a = allocate([&] {
        return reference_capture_detail::decode_arrival_body(
            bytes(arrival_document->document), l, reference_capture_detail::BodyBounds::retained);
    });
    const auto &configuration_ref = execution_document->configuration->reference;
    auto configuration_document = read(view, RetainedKind::configuration, configuration_ref, l);
    if (!configuration_document)
        return ApplicationAccessUnknown{};
    data(!configuration_document->configuration);
    const auto cfg = allocate([&] {
        return reference_capture_detail::decode_configuration_body(
            bytes(configuration_document->document), l,
            reference_capture_detail::BodyBounds::retained);
    });
    data(a.arrival_context_ref == x.arrival_context_ref &&
         cfg.configuration_ref == configuration_ref && same(cfg, a.configuration) &&
         same(cfg.rule, x.rule) && cfg.synthetic_origin == x.source.origin &&
         cfg.receipt_namespace == x.source.identifier_namespace &&
         a.observation.event_id == x.source.opaque_id);
    auto complete = configs.load(view, configuration_ref); // trusted producer, no snapshot/cache
    const auto evaluated = evaluate_reference_rule(a, complete, l);
    if (std::holds_alternative<ReferenceUnavailable>(evaluated))
        return ApplicationAccessUnknown{};
    data(std::get<ReferenceRuleEvaluation>(evaluated).request.requested_units == x.rule.add_ms);
    // Project the complete authority request before cloning any capture or command fields.
    Projection p{al.input_bytes, UINT64_MAX, al.reference_bytes, 0};
    p(ReferenceAuthorityUse::execute_time_add, c.principal, x, c.operation);
    p.fixed(2); // absent historical operation/hold
    auto q = allocate([&] {
        return ReferenceAuthorityRequest{ReferenceAuthorityUse::execute_time_add,
                                         c.principal,
                                         x,
                                         c.operation,
                                         std::nullopt,
                                         std::nullopt};
    });
    reference_authority::request_shape(q);
    auto fresh = authority.resolve(view, q); // a NEW full request on EVERY attempt
    return consume(fresh, q, al);
}
} // namespace
AccessDecision check_reference_execution(const ApplicationReferenceReadView &view,
                                         const OriginalApplicationCommand &c,
                                         ReferenceConfigurationSource &configs,
                                         ReferenceAuthoritySource &authority, const Limits &l,
                                         const ReferenceAuthorityLimits &al) {
    valid_limits(l);      // Caller options retain the declared input-error boundary.
    authority_limits(al); // Preserve caller errors; only source failures translate below.
    try {
        return execute(view, c, configs, authority, l, al);
    } catch (const ReferenceInputFailure &e) {
        switch (e.code()) {
        case Error::input_limit:
        case Error::result_limit:
            return ApplicationAccessUnknown{};
        case Error::allocation_failure:
            throw;
        default:
            corrupt();
        }
    } catch (const ReferenceAuthorityInputFailure &e) {
        return authority_failure(e.code());
    } catch (const EffectivePermissionFailure &e) {
        switch (e.code()) {
        case EffectivePermissionError::input_limit:
        case EffectivePermissionError::result_limit:
        case EffectivePermissionError::allocation_failure:
        case EffectivePermissionError::facts_unavailable:
            return ApplicationAccessUnknown{};
        default:
            corrupt();
        }
    } catch (const ApplicationServiceInputFailure &e) {
        switch (e.error()) {
        case ApplicationServiceInputError::collection_limit:
        case ApplicationServiceInputError::owned_byte_limit:
        case ApplicationServiceInputError::result_limit:
            return ApplicationAccessUnknown{};
        default:
            corrupt();
        }
    }
    // In particular: no SqliteWorkFailure, RetentionFailure or view exception catch.
    // The actual reader/service owns rollback and preserves its original outcome.
}
} // namespace subathon::server
