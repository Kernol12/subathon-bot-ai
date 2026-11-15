// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "reference_authority_values.hpp"
#include <array>
#include <subathon/server/reference_configuration.hpp>
#include <tuple>
namespace subathon::server::reference_detail {
using namespace core;
using Error = ReferenceInputError;
using Limits = ReferenceAdapterLimits;
using reference_authority::fields;
[[noreturn]] inline void fail(Error e) { throw ReferenceInputFailure(e); }
[[noreturn]] inline void corrupt() {
    throw ReferenceExecutionFailure(ReferenceAuthorityFailure::invalid_data);
}
inline void require(bool b, Error e) {
    if (!b)
        fail(e);
}
inline void data(bool b) {
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
inline Projection projection(const Limits &l, Error e = Error::input_limit) {
    return {l.value_owned_bytes, l.value_items, l.reference_bytes, l.payload_bytes, 0, 0, e};
}
inline void valid_limits(const Limits &l) {
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
inline void unique(const std::vector<std::string> &v) {
    unique(v, [](const auto &x) -> const auto & { return x; });
}
inline void numeric(const ReferenceRule &r) { require(r.add_ms >= 0, Error::invalid_value); }
inline void numeric(const ReferenceConfiguration &c) {
    numeric(c.rule);
    require((!c.maximum_main_ms || *c.maximum_main_ms >= 0) &&
                c.timer_config.grace_duration_ms >= 0,
            Error::invalid_value);
}
inline void binding(const ReferenceRule &r) {
    require(r.automation.kind == ApplicationPrincipalKind::automation, Error::invalid_binding);
}
inline void arrival_shape(const ReferenceArrivalCapture &a) {
    require(!a.observation.original_payload.isNull(), Error::invalid_shape);
}
inline void facts_shape(const ReferenceConfigurationResolution &r) {
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
inline void facts_numeric(const ReferenceConfigurationResolution &r) {
    if (const auto *f = std::get_if<ReferenceConfigurationFacts>(&r)) {
        numeric(f->projection);
        for (const auto &v : f->enabled_rules)
            numeric(v.rule);
    }
}
inline void facts_binding(const std::string &expected, const ReferenceConfigurationResolution &r) {
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
inline const ReferenceConfigurationFacts *supported(const ReferenceConfigurationResolution &r) {
    return std::get_if<ReferenceConfigurationFacts>(&r);
}
inline std::optional<ReferenceGate> refusal(const ReferenceConfigurationFacts &f) {
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
inline ReferenceUnavailable unavailable(ReferenceGate g, const Limits &l) {
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
inline ReferenceUnavailable copy_refusal(const ReferenceUnavailable &u, const Limits &l) {
    auto p = projection(l, Error::result_limit);
    p(u);
    return u;
}
inline ReferenceAdmittedConfiguration admitted(const ReferenceConfigurationFacts &f,
                                               const Limits &l) {
    auto p = projection(l, Error::result_limit);
    p(f.projection, f.configuration_owner_revision_ref);
    ReferenceAdmittedConfiguration out{f.projection, f.configuration_owner_revision_ref};
    auto &c = out.configuration;
    c.other_rules = c.modifiers = c.milestones = c.outbound_actions = c.generated_events =
        ReferenceFactsState::complete;
    return out;
}
} // namespace subathon::server::reference_detail
namespace subathon::server::reference_detail {
ReferenceExecutionBinding captured_binding(const ApplicationReferenceReadView &,
                                           const OriginalApplicationCommand &,
                                           ReferenceConfigurationSource &,
                                           const ReferenceAdapterLimits &);
AccessDecision disclose(const ApplicationReferenceReadView &, const OriginalApplicationCommand &,
                        ReferenceConfigurationSource &, ReferenceAuthoritySource &,
                        ReferenceAuthorityUse, const ApplicationOperationKey &,
                        const std::optional<std::string> &, const ReferenceAdapterLimits &,
                        const ReferenceAuthorityLimits &, const ApplicationEnvelope &);
} // namespace subathon::server::reference_detail
