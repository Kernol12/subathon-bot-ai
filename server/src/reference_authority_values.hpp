// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "permission_graph_codec.hpp"
#include <subathon/server/sqlite_reference_authority.hpp>
namespace subathon::server::reference_authority {
using namespace core;
using F = ReferenceAuthorityFailure;
struct Refusal {
    F code;
};
[[noreturn]] inline void fail(F f) { throw Refusal{f}; }
inline void data(bool b) {
    if (!b)
        fail(F::invalid_data);
}
inline void space(bool b) {
    if (!b)
        fail(F::resource_limit);
}
inline void add(std::uint64_t &n, std::uint64_t v, std::uint64_t cap) {
    space(n <= cap && v <= cap - n);
    n += v;
}
using permission_graph::fields;
#define RA_FIELDS(Type, ...)                                                                       \
    template <class P, class V>                                                                    \
        requires std::is_same_v<std::remove_cvref_t<V>, Type>                                      \
    void fields(P &p, V &v) {                                                                      \
        p(__VA_ARGS__);                                                                            \
    }
RA_FIELDS(ReceiptOrigin, v.provider, v.source_account)
RA_FIELDS(ReceiptIdentity, v.origin, v.identifier_namespace, v.opaque_id)
RA_FIELDS(ApplicationPrincipalBinding, v.kind, v.installation_ref, v.principal_ref)
RA_FIELDS(ApplicationPlanKey, v.plan_ref, v.revision)
RA_FIELDS(ApplicationOperationKey, v.operation_scope, v.operation_ref)
RA_FIELDS(ReferenceRule, v.rule_ref, v.rule_version_ref, v.action_ref, v.action_version_ref,
          v.timer_ref, v.add_ms, v.automation, v.automation_capability_ref)
RA_FIELDS(ReferenceExecutionBinding, v.execution_context_ref, v.arrival_context_ref,
          v.installation_ref, v.source, v.receipt_id.value, v.contribution_id.value, v.plan, v.rule)
RA_FIELDS(ReferenceAuthorityRequest, v.use, v.principal, v.captured, v.requested_operation,
          v.historical_operation, v.historical_hold)
RA_FIELDS(ReferenceAuthorityBinding, v.installation, v.capability, v.principal, v.rule,
          v.rule_version, v.action, v.action_version, v.timer, v.origin, v.identifier_namespace,
          v.requested_ms)
RA_FIELDS(ReferenceAuthorityRecord, v.binding, v.original_author, v.execution_account,
          v.created_revision, v.state_revision, v.head_revision, v.state, v.actor, v.session,
          v.session_revision, v.authenticated_utc_ms)
RA_FIELDS(PermissionPolicyFacts, v.state, v.assigned_roles, v.entries)
RA_FIELDS(DelegatedPermissionGrant, v.operation, v.resources, v.quantities)
RA_FIELDS(PermissionAutomationInput, v.author, v.snapshot, v.catalog, v.owner, v.use, v.delegation,
          v.actions)
RA_FIELDS(EffectivePermissionInput, v.actor, v.snapshot, v.catalog, v.owner, v.policy, v.request)
RA_FIELDS(ReferenceAuthorityFacts, v.binding, v.registry, v.permission_revision_ref,
          v.automation_revision_ref, v.original_author_input, v.permission_input)
#undef RA_FIELDS
// Allocation-free declaration-order semantic B/I traversal, also usable for
// borrowed policy expansions before constructing their owning output vectors.
struct Projection {
    std::uint64_t cap, item_cap, reference, bytes = 0, items = 0;
    bool empty = false;
    std::uint64_t component_bytes = UINT64_MAX;
    void fixed(std::uint64_t n) { add(bytes, n, cap); }
    void elements(std::uint64_t n) { add(items, n, item_cap); }
    void one(std::string_view s) {
        space(s.size() <= reference && s.size() <= std::string{}.max_size());
        empty |= s.empty();
        fixed(s.size());
    }
    void one(const std::string &s) { one(std::string_view(s)); }
    void component(const std::string &s) {
        space(s.size() <= component_bytes);
        fixed(s.size());
    }
    template <class T>
        requires(std::is_integral_v<T> || std::is_enum_v<T>)
    void one(T) {
        fixed(std::is_enum_v<T> ? 1 : sizeof(T));
    }
    template <class T> void one(const std::optional<T> &v) {
        fixed(1);
        if (v)
            one(*v);
    }
    template <class T> void one(const std::vector<T> &v) {
        space(v.size() <= v.max_size());
        elements(v.size());
        for (const auto &x : v)
            one(x);
    }
    template <class T>
        requires(!std::is_integral_v<T> && !std::is_enum_v<T>)
    void one(const T &v) {
        fields(*this, v);
    }
    template <class... T> void operator()(const T &...v) { (one(v), ...); }
};
inline void valid_limits(const ReferenceAuthorityLimits &l) {
    const auto max =
        std::min({std::uint64_t(PTRDIFF_MAX), std::uint64_t(SIZE_MAX),
                  std::uint64_t(QByteArray::maxSize()), std::uint64_t(std::string{}.max_size())});
    auto positive = [&](std::uint64_t n) {
        if (!n || n >= max)
            throw ReferenceAuthorityInputFailure(F::invalid_data);
    };
    for (auto n : {l.reference_bytes,
                   l.input_bytes,
                   l.registry_selected_bytes,
                   l.mutation_bytes,
                   l.result_bytes,
                   l.result_items,
                   l.graph.reference_bytes,
                   l.graph.component_bytes,
                   l.graph.rows,
                   l.graph.items,
                   l.graph.selected_bytes,
                   l.graph.mutation_bytes,
                   l.accounts.reference_bytes,
                   l.accounts.selected_bytes,
                   l.accounts.mutation_bytes,
                   l.accounts.page_rows,
                   l.sessions.reference_bytes,
                   l.sessions.owned_bytes,
                   l.permissions.reference_bytes,
                   l.permissions.component_bytes,
                   l.permissions.input_items,
                   l.permissions.input_owned_bytes,
                   l.permissions.result_items,
                   l.permissions.result_owned_bytes})
        positive(n);
    for (auto n : {l.captures.reference_bytes, l.captures.document_bytes, l.captures.owned_bytes}) {
        if (n <= 0)
            throw ReferenceAuthorityInputFailure(F::invalid_data);
        positive(std::uint64_t(n));
    }
    auto invalid = [&](bool b) {
        if (b)
            throw ReferenceAuthorityInputFailure(F::invalid_data);
    };
    invalid(l.graph.selected_bytes > (max - l.graph.mutation_bytes) / 3);
    invalid(l.accounts.selected_bytes > (max - l.accounts.mutation_bytes) / 3);
    invalid(l.accounts.page_rows >= std::vector<SessionRecordFacts>{}.max_size());
    invalid(l.sessions.owned_bytes > SIZE_MAX / 4);
    const auto &p = l.permissions;
    invalid(p.component_bytes > max / 16 || p.input_items > (max - 16 * p.component_bytes) / 128);
    std::uint64_t power = p.input_items;
    for (int i = 1; i < 4; ++i) {
        invalid(power > max / p.input_items);
        power *= p.input_items;
    }
}
template <class... T> void preflight(const ReferenceAuthorityLimits &l, const T &...v) {
    try {
        Projection p{l.input_bytes, UINT64_MAX, l.reference_bytes};
        p(v...);
        if (p.empty)
            throw ReferenceAuthorityInputFailure(F::invalid_data);
    } catch (const Refusal &e) {
        throw ReferenceAuthorityInputFailure(e.code);
    }
}
inline void request_shape(const ReferenceAuthorityRequest &r) {
    if (unsigned(r.use) > 2 || r.principal.kind != ApplicationPrincipalKind::automation ||
        r.captured.rule.automation.kind != ApplicationPrincipalKind::automation ||
        r.captured.rule.add_ms < 0 || !r.captured.receipt_id.value || !r.captured.plan.revision ||
        (r.use == ReferenceAuthorityUse::execute_time_add &&
         (r.historical_operation || r.historical_hold)) ||
        (r.use != ReferenceAuthorityUse::execute_time_add && !r.historical_operation) ||
        (r.use == ReferenceAuthorityUse::disclose_application && r.historical_hold) ||
        (r.use == ReferenceAuthorityUse::disclose_holds && !r.historical_hold))
        throw ReferenceAuthorityInputFailure(F::invalid_data);
}
inline F graph_failure(PermissionGraphFailure f) {
    switch (f) {
    case PermissionGraphFailure::missing:
        return F::missing;
    case PermissionGraphFailure::resource_limit:
        return F::resource_limit;
    default:
        return F::invalid_data;
    } // exact compiled catalog is mandatory
}
template <class R, class Fn> R guarded(Fn fn) {
    try {
        return fn();
    } catch (const Refusal &e) {
        return e.code;
    } catch (const std::bad_alloc &) {
        return F::resource_limit;
    } catch (const std::length_error &) {
        return F::resource_limit;
    } catch (const EffectivePermissionFailure &e) {
        switch (e.code()) {
        case EffectivePermissionError::input_limit:
        case EffectivePermissionError::result_limit:
        case EffectivePermissionError::allocation_failure:
            return F::resource_limit;
        case EffectivePermissionError::facts_unavailable:
            return F::unavailable;
        default:
            return F::invalid_data;
        }
    }
}
} // namespace subathon::server::reference_authority
