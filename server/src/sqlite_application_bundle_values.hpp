// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <subathon/server/sqlite_application_bundle.hpp>
#include <subathon/server/sqlite_application_plans.hpp>
#include <tuple>

namespace subathon::server::bundle_detail {
using Input = SqliteBundleInputError;
using core::Exact;
[[noreturn]] inline void invalid(Input error) { throw SqliteBundleInputFailure(error); }
template <class E> inline bool enum_in(E value, int maximum) {
    return int(value) >= 0 && int(value) <= maximum;
}
inline bool bounded(const std::string &value, qsizetype limit, bool required = false) {
    return (!required || !value.empty()) && value.size() <= size_t(limit);
}
inline bool raw_less(const std::string &a, const std::string &b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                        [](unsigned char x, unsigned char y) { return x < y; });
}
inline bool same_envelope(const ApplicationEnvelope &a, const ApplicationEnvelope &b) {
    return a.format_ref == b.format_ref && a.document == b.document &&
           a.document.isNull() == b.document.isNull();
}
inline bool same_balance(const ApplicationBalance &a, const ApplicationBalance &b) {
    return a.kind == b.kind && a.target_ref == b.target_ref && a.units == b.units;
}
inline bool same_state(const ApplicationStateSnapshot &a, const ApplicationStateSnapshot &b) {
    return a.token == b.token && same_envelope(a.envelope, b.envelope) &&
           a.configuration_ref == b.configuration_ref &&
           std::tie(a.timer.lifecycle, a.timer.phase, a.timer.main_remaining_ms,
                    a.timer.grace_remaining_ms, a.timer.awaiting_confirmation, a.time_debt_ms) ==
               std::tie(b.timer.lifecycle, b.timer.phase, b.timer.main_remaining_ms,
                        b.timer.grace_remaining_ms, b.timer.awaiting_confirmation,
                        b.time_debt_ms) &&
           a.balances.size() == b.balances.size() &&
           std::equal(a.balances.begin(), a.balances.end(), b.balances.begin(), same_balance);
}
inline bool same_key(const ApplicationOperationKey &a, const ApplicationOperationKey &b) {
    return a.operation_scope == b.operation_scope && a.operation_ref == b.operation_ref;
}
inline bool same_intent(const ApplicationIntent &a, const ApplicationIntent &b) {
    return same_key(a.operation, b.operation) && a.kind == b.kind &&
           a.subject_kind == b.subject_kind && a.receipt_id == b.receipt_id &&
           a.contribution_id == b.contribution_id && a.plan.plan_ref == b.plan.plan_ref &&
           a.plan.revision == b.plan.revision && a.expected_state_token == b.expected_state_token &&
           a.expected_contribution_state == b.expected_contribution_state &&
           same_envelope(a.choices, b.choices) && a.reversal_ref == b.reversal_ref;
}
inline void validate_options(const SqliteBundleLimits &l) {
    const auto count = [](qsizetype n) {
        return n > 0 && n < std::numeric_limits<qsizetype>::max() &&
               std::uintmax_t(n) < std::uintmax_t(std::numeric_limits<qint64>::max()) &&
               std::uintmax_t(n) < std::numeric_limits<size_t>::max();
    };
    if (l.reference_bytes <= 0 || l.value_bytes <= 0 || l.component_bytes <= 0 ||
        l.owned_bytes <= 0 || !count(l.balances_per_state) || !count(l.plan_items_per_vector) ||
        !count(l.effects_per_bundle) || !count(l.jobs_per_bundle) ||
        !count(l.transitions_per_bundle) || !count(l.links_per_bundle) || !count(l.input_items) ||
        !count(l.result_items) ||
        std::uintmax_t(l.balances_per_state) >= std::vector<ApplicationBalance>{}.max_size() ||
        std::uintmax_t(l.plan_items_per_vector) >= std::vector<core::RuleSelection>{}.max_size() ||
        std::uintmax_t(l.plan_items_per_vector) >=
            std::vector<core::ModifierSelection>{}.max_size() ||
        std::uintmax_t(l.plan_items_per_vector) >=
            std::vector<core::RequestedEffect>{}.max_size() ||
        std::uintmax_t(l.effects_per_bundle) >= std::vector<ApplicationEffect>{}.max_size() ||
        std::uintmax_t(l.jobs_per_bundle) >= std::vector<ApplicationJobIntent>{}.max_size() ||
        std::uintmax_t(l.transitions_per_bundle) >=
            std::vector<core::TimerTransition>{}.max_size() ||
        std::uintmax_t(l.links_per_bundle) >= std::vector<ApplicationReversalLink>{}.max_size() ||
        std::uintmax_t(l.links_per_bundle) >=
            std::vector<ApplicationReversalOccurrence>{}.max_size())
        invalid(Input::invalid_options);
}
inline void validate_key(const ApplicationOperationKey &key, const SqliteBundleLimits &l) {
    validate_options(l);
    if (!bounded(key.operation_scope, l.reference_bytes, true) ||
        !bounded(key.operation_ref, l.reference_bytes, true))
        invalid(Input::invalid_key);
}
inline void validate_plan_key(const ApplicationPlanKey &key, const SqliteBundleLimits &l) {
    validate_options(l);
    if (!bounded(key.plan_ref, l.reference_bytes, true))
        invalid(Input::invalid_key);
    if (key.revision == 0)
        invalid(Input::invalid_source);
}
inline void validate_original(const ApplicationOriginalComponent &original,
                              const SqliteBundleLimits &l) {
    validate_key(original.operation, l);
    if (original.request_ordinal < 1 ||
        (original.component_kind != ApplicationComponentKind::main_delta &&
         original.component_kind != ApplicationComponentKind::score &&
         original.component_kind != ApplicationComponentKind::whole_count))
        invalid(Input::invalid_reversal);
}
inline bool valid_envelope(const ApplicationEnvelope &v, const SqliteBundleLimits &l) {
    return bounded(v.format_ref, l.reference_bytes, true) && !v.document.isNull() &&
           v.document.size() <= l.value_bytes;
}
inline void validate_state_shape(const ApplicationStateSnapshot &s, const SqliteBundleLimits &l) {
    if (!bounded(s.token, l.reference_bytes, true) || !valid_envelope(s.envelope, l) ||
        !bounded(s.configuration_ref, l.reference_bytes, true) || !enum_in(s.timer.lifecycle, 2) ||
        !enum_in(s.timer.phase, 1) || s.timer.main_remaining_ms < 0 ||
        s.timer.grace_remaining_ms < 0 || s.time_debt_ms < 0 ||
        (s.timer.phase == core::CountdownPhase::Grace &&
         (s.timer.main_remaining_ms != 0 || s.timer.awaiting_confirmation)) ||
        (s.timer.awaiting_confirmation &&
         (s.timer.lifecycle != core::TimerLifecycle::Active ||
          s.timer.phase != core::CountdownPhase::Main || s.timer.main_remaining_ms != 0)))
        invalid(Input::invalid_state);
    const ApplicationBalance *previous = nullptr;
    for (const auto &b : s.balances) {
        if ((b.kind != ApplicationBalanceKind::score &&
             b.kind != ApplicationBalanceKind::whole_count) ||
            !bounded(b.target_ref, l.reference_bytes, true) ||
            (previous &&
             (int(b.kind) < int(previous->kind) ||
              (b.kind == previous->kind && !raw_less(previous->target_ref, b.target_ref)))))
            invalid(Input::invalid_state);
        previous = &b;
    }
}
inline void validate_contribution(const core::ContributionState &s, const SqliteBundleLimits &l) {
    const auto &c = s.conditions;
    const auto &v = c.valuation;
    const auto &a = c.evidence_admission;
    if (!enum_in(s.payment_evidence, 3) || !enum_in(c.review, 3) || !enum_in(a.policy, 2) ||
        !enum_in(c.application, 1) || !enum_in(c.reversal, 2) || !enum_in(c.settlement, 1) ||
        (v.original_money && (!bounded(v.original_money->original_decimal, l.value_bytes) ||
                              !bounded(v.original_money->currency, l.value_bytes))) ||
        (v.resolution_ref && !bounded(*v.resolution_ref, l.reference_bytes)) ||
        !bounded(a.policy_ref, l.reference_bytes) ||
        (a.review_decision && (!enum_in(a.review_decision->choice, 1) ||
                               !bounded(a.review_decision->decision_ref, l.reference_bytes) ||
                               !bounded(a.review_decision->actor_ref, l.reference_bytes))))
        invalid(Input::invalid_contribution);
}
inline void validate_exact(const Exact &value, const SqliteBundleLimits &l) {
    const auto numerator = boost::multiprecision::numerator(value).str();
    const auto denominator = boost::multiprecision::denominator(value).str();
    if (numerator.size() > size_t(l.component_bytes) ||
        denominator.size() > size_t(l.component_bytes))
        invalid(Input::exact_component_limit);
}
// All count checks precede inspecting element bodies. Totals reserve against
// overflow before addition; a maximum of three components belongs to each effect.
inline size_t add_count(size_t sum, size_t n, size_t limit) {
    if (sum > limit || n > limit - sum)
        invalid(Input::collection_limit);
    return sum + n;
}
inline void validate_bundle_counts(const PreparedApplicationBundle &b,
                                   const SqliteBundleLimits &l) {
    if (b.advanced_before.balances.size() > size_t(l.balances_per_state) ||
        b.after.balances.size() > size_t(l.balances_per_state) ||
        b.effects.size() > size_t(l.effects_per_bundle) ||
        b.jobs.size() > size_t(l.jobs_per_bundle) ||
        b.transitions.size() > size_t(l.transitions_per_bundle) ||
        b.reversals.size() > size_t(l.links_per_bundle))
        invalid(Input::collection_limit);
    size_t count = 0;
    const auto add = [&](size_t n) { count = add_count(count, n, size_t(l.input_items)); };
    add(b.advanced_before.balances.size());
    add(b.after.balances.size());
    add(b.effects.size());
    for (const auto &effect : b.effects) {
        if (effect.components.size() > 3)
            invalid(Input::collection_limit);
        add(effect.components.size());
    }
    add(b.jobs.size());
    add(b.transitions.size());
    add(b.reversals.size());
}
inline void validate_effect(const ApplicationEffect &e, size_t position,
                            const ApplicationStateSnapshot &before, const SqliteBundleLimits &l) {
    using CK = ApplicationComponentKind;
    using Role = ApplicationArithmeticRole;
    if (e.request_ordinal != std::int64_t(position) || !enum_in(e.arithmetic_role, 2) ||
        !enum_in(e.unit, 2) || !bounded(e.application_explanation, l.value_bytes) ||
        (e.arithmetic_role == Role::direct &&
         (e.unit == core::RequestedUnit::milliseconds || e.maximum_main_ms || e.deficit_policy ||
          e.deficit_disposition)) ||
        (e.arithmetic_role == Role::time_addition &&
         (e.unit != core::RequestedUnit::milliseconds || e.requested_units < 0 ||
          (e.maximum_main_ms && *e.maximum_main_ms < 0) || e.deficit_policy ||
          e.deficit_disposition)) ||
        (e.arithmetic_role == Role::time_reversal &&
         (e.unit != core::RequestedUnit::milliseconds || e.requested_units > 0 ||
          e.maximum_main_ms || !e.deficit_policy || !e.deficit_disposition ||
          !enum_in(*e.deficit_policy, 2) || !enum_in(*e.deficit_disposition, 1))))
        invalid(Input::invalid_effect);
    std::array<CK, 3> expected{CK::main_delta, CK::debt_delta, CK::cap_limited_time};
    size_t number = 3;
    if (e.arithmetic_role == Role::direct) {
        expected[0] = e.unit == core::RequestedUnit::micro_points ? CK::score : CK::whole_count;
        number = 1;
    } else if (e.arithmetic_role == Role::time_reversal) {
        expected[2] = CK::unapplied_time;
    }
    if (e.components.size() != number)
        invalid(Input::invalid_component);
    for (size_t i = 0; i < number; ++i) {
        const auto &c = e.components[i];
        if (c.kind != expected[i] ||
            ((c.kind == CK::score || c.kind == CK::whole_count)
                 ? (!c.target_ref || !bounded(*c.target_ref, l.reference_bytes, true))
                 : c.target_ref.has_value()))
            invalid(Input::invalid_component);
        if (c.target_ref &&
            std::none_of(before.balances.begin(), before.balances.end(), [&](const auto &v) {
                return int(v.kind) == int(c.kind) && v.target_ref == *c.target_ref;
            }))
            invalid(Input::invalid_component);
    }
    validate_exact(e.exact_requested_units, l);
    validate_exact(e.rounding_residual, l);
    if (e.rounding_residual != e.exact_requested_units - Exact(e.requested_units))
        invalid(Input::invalid_effect);
    if (e.arithmetic_role == Role::direct) {
        if (e.components[0].units != e.requested_units)
            invalid(Input::invalid_component);
    } else {
        const auto main = e.components[0].units;
        const auto debt = e.components[1].units;
        const auto tail = e.components[2].units;
        if ((e.arithmetic_role == Role::time_addition && (main < 0 || debt > 0 || tail < 0)) ||
            (e.arithmetic_role == Role::time_reversal && (main > 0 || debt < 0 || tail > 0)) ||
            Exact(main) - Exact(debt) + Exact(tail) != Exact(e.requested_units))
            invalid(Input::invalid_component);
    }
}
inline void validate_deltas(const PreparedApplicationBundle &b) {
    const auto &before = b.advanced_before;
    const auto &after = b.after;
    if (before.configuration_ref != after.configuration_ref ||
        before.envelope.format_ref != after.envelope.format_ref ||
        before.balances.size() != after.balances.size())
        invalid(Input::invalid_state);
    Exact main = before.timer.main_remaining_ms;
    Exact debt = before.time_debt_ms;
    for (const auto &e : b.effects)
        for (const auto &c : e.components) {
            if (c.kind == ApplicationComponentKind::main_delta)
                main += c.units;
            if (c.kind == ApplicationComponentKind::debt_delta)
                debt += c.units;
        }
    if (main != after.timer.main_remaining_ms || debt != after.time_debt_ms)
        invalid(Input::invalid_state);
    for (size_t i = 0; i < before.balances.size(); ++i) {
        const auto &x = before.balances[i];
        const auto &y = after.balances[i];
        if (x.kind != y.kind || x.target_ref != y.target_ref)
            invalid(Input::invalid_state);
        Exact units = x.units;
        for (const auto &e : b.effects)
            for (const auto &c : e.components)
                if (int(c.kind) == int(x.kind) && c.target_ref && *c.target_ref == x.target_ref)
                    units += c.units;
        if (units != y.units)
            invalid(Input::invalid_state);
    }
}
inline void validate_jobs(const PreparedApplicationBundle &b, const SqliteBundleLimits &l) {
    std::set<std::string> keys;
    std::map<std::string, std::set<std::int64_t>> groups;
    for (const auto &j : b.jobs) {
        if (!bounded(j.job_ref, l.reference_bytes, true) || !keys.insert(j.job_ref).second ||
            !valid_envelope(j.envelope, l) ||
            (j.source_request_ordinal &&
             (*j.source_request_ordinal < 1 ||
              std::uintmax_t(*j.source_request_ordinal) > b.effects.size())) ||
            j.group_ref.has_value() != j.group_ordinal.has_value() ||
            (j.group_ref &&
             (!bounded(*j.group_ref, l.reference_bytes, true) || *j.group_ordinal < 1)))
            invalid(Input::invalid_job);
        if (j.group_ref && !groups[*j.group_ref].insert(*j.group_ordinal).second)
            invalid(Input::invalid_job);
    }
    for (const auto &[group, positions] : groups) {
        (void)group;
        std::int64_t expected = 1;
        for (auto position : positions)
            if (position != expected++)
                invalid(Input::invalid_job);
    }
}
inline bool original_less(const ApplicationOriginalComponent &a,
                          const ApplicationOriginalComponent &b) {
    if (a.operation.operation_scope != b.operation.operation_scope)
        return raw_less(a.operation.operation_scope, b.operation.operation_scope);
    if (a.operation.operation_ref != b.operation.operation_ref)
        return raw_less(a.operation.operation_ref, b.operation.operation_ref);
    return std::tie(a.request_ordinal, a.component_kind) <
           std::tie(b.request_ordinal, b.component_kind);
}
inline void validate_reversals(const PreparedApplicationBundle &b, const SqliteBundleLimits &l) {
    if (b.intent.kind == ApplicationOperationKind::original && !b.reversals.empty())
        invalid(Input::invalid_reversal);
    const ApplicationReversalLink *previous = nullptr;
    for (const auto &r : b.reversals) {
        if (!bounded(r.original.operation.operation_scope, l.reference_bytes, true) ||
            !bounded(r.original.operation.operation_ref, l.reference_bytes, true) ||
            r.original.request_ordinal < 1 ||
            (r.original.component_kind != ApplicationComponentKind::main_delta &&
             r.original.component_kind != ApplicationComponentKind::score &&
             r.original.component_kind != ApplicationComponentKind::whole_count) ||
            r.request_ordinal < 1 || std::uintmax_t(r.request_ordinal) > b.effects.size() ||
            r.expected_prior_position < 0 || r.expected_prior_cumulative < 0 ||
            r.cumulative < r.expected_prior_cumulative || r.requested_increment > 0 ||
            (previous && (r.request_ordinal < previous->request_ordinal ||
                          (r.request_ordinal == previous->request_ordinal &&
                           !original_less(previous->original, r.original)))))
            invalid(Input::invalid_reversal);
        validate_exact(r.cumulative_fraction, l);
        validate_exact(r.exact_entitlement, l);
        validate_exact(r.rounding_residual, l);
        if (r.cumulative_fraction < 0 || r.cumulative_fraction > 1 || r.exact_entitlement < 0 ||
            (r.full_refund && r.cumulative_fraction != 1) ||
            r.rounding_residual != r.exact_entitlement - Exact(r.cumulative) ||
            Exact(r.requested_increment) !=
                Exact(r.expected_prior_cumulative) - Exact(r.cumulative))
            invalid(Input::invalid_reversal);
        previous = &r;
    }
    if (b.intent.kind == ApplicationOperationKind::reversal)
        for (const auto &e : b.effects) {
            Exact sum = 0;
            bool present = false;
            for (const auto &r : b.reversals)
                if (r.request_ordinal == e.request_ordinal) {
                    sum += r.requested_increment;
                    present = true;
                }
            if (!present || sum != Exact(e.requested_units))
                invalid(Input::invalid_reversal);
        }
}
// Logical owned-byte accounting is independent of SQL binding reuse. Optional
// absence contributes nothing; every scalar is its declared 8 or 1 byte weight.
class ByteCount {
  public:
    explicit ByteCount(size_t limit = std::numeric_limits<size_t>::max()) : limit_(limit) {}
    void add(size_t n) {
        if (total_ > limit_ || n > limit_ - total_)
            invalid(Input::owned_byte_limit);
        total_ += n;
    }
    [[nodiscard]] size_t total() const { return total_; }

  private:
    size_t limit_;
    size_t total_{0};
};
inline void count_bytes(ByteCount &n, const std::string &v) { n.add(v.size()); }
inline void count_bytes(ByteCount &n, const QByteArray &v) { n.add(size_t(v.size())); }
inline void count_bytes(ByteCount &n, const Exact &v) {
    n.add(boost::multiprecision::numerator(v).str().size());
    n.add(boost::multiprecision::denominator(v).str().size());
}
template <class T> inline void count_bytes(ByteCount &n, const std::optional<T> &v) {
    if (v)
        count_bytes(n, *v);
}
inline void count_bytes(ByteCount &n, const ApplicationEnvelope &v) {
    count_bytes(n, v.format_ref);
    count_bytes(n, v.document);
}
inline void count_bytes(ByteCount &n, const ApplicationOperationKey &v) {
    count_bytes(n, v.operation_scope);
    count_bytes(n, v.operation_ref);
}
inline void count_bytes(ByteCount &n, const ApplicationPlanKey &v) {
    count_bytes(n, v.plan_ref);
    n.add(8);
}
inline void count_bytes(ByteCount &n, const ApplicationBalance &v) {
    n.add(9);
    count_bytes(n, v.target_ref);
}
inline void count_bytes(ByteCount &n, const ApplicationStateSnapshot &v) {
    count_bytes(n, v.token);
    count_bytes(n, v.envelope);
    count_bytes(n, v.configuration_ref);
    n.add(27);
    for (const auto &balance : v.balances)
        count_bytes(n, balance);
}
inline void count_bytes(ByteCount &n, const core::ContributionConditions &c) {
    const auto &a = c.evidence_admission;
    n.add(7); // monetary, missing-rate, review, policy, application, reversal, settlement.
    if (c.valuation.original_money) {
        count_bytes(n, c.valuation.original_money->original_decimal);
        count_bytes(n, c.valuation.original_money->currency);
    }
    count_bytes(n, c.valuation.resolution_ref);
    count_bytes(n, a.policy_ref);
    if (a.review_decision) {
        n.add(1);
        count_bytes(n, a.review_decision->decision_ref);
        count_bytes(n, a.review_decision->actor_ref);
    }
}
inline void count_bytes(ByteCount &n, const core::ContributionState &v) {
    n.add(1);
    count_bytes(n, v.conditions);
}
inline void count_bytes(ByteCount &n, const ApplicationIntent &v) {
    count_bytes(n, v.operation);
    n.add(18); // kind, subject, receipt, selected revision.
    if (v.contribution_id)
        n.add(8);
    count_bytes(n, v.plan.plan_ref);
    count_bytes(n, v.expected_state_token);
    if (v.expected_contribution_state)
        count_bytes(n, *v.expected_contribution_state);
    count_bytes(n, v.choices);
    count_bytes(n, v.reversal_ref);
}
inline void count_bytes(ByteCount &n, const ApplicationEffect &v) {
    n.add(18); // request ordinal, role, unit, requested units.
    count_bytes(n, v.exact_requested_units);
    count_bytes(n, v.rounding_residual);
    if (v.maximum_main_ms)
        n.add(8);
    if (v.deficit_policy)
        n.add(1);
    if (v.deficit_disposition)
        n.add(1);
    count_bytes(n, v.application_explanation);
    for (const auto &component : v.components) {
        n.add(9);
        count_bytes(n, component.target_ref);
    }
}
inline void count_bytes(ByteCount &n, const ApplicationJobIntent &v) {
    count_bytes(n, v.job_ref);
    if (v.source_request_ordinal)
        n.add(8);
    count_bytes(n, v.group_ref);
    if (v.group_ordinal)
        n.add(8);
    count_bytes(n, v.envelope);
}
inline void count_bytes(ByteCount &n, const ApplicationAudit &v) {
    count_bytes(n, v.actor_ref);
    count_bytes(n, v.decision_ref);
    count_bytes(n, v.reason);
    count_bytes(n, v.envelope);
}
inline void count_bytes(ByteCount &n, const ApplicationOriginalComponent &v) {
    count_bytes(n, v.operation);
    n.add(9);
}
inline void count_bytes(ByteCount &n, const ApplicationReversalLink &v) {
    count_bytes(n, v.original);
    n.add(41); // current ordinal, four signed progress/increment fields, full flag.
    count_bytes(n, v.cumulative_fraction);
    count_bytes(n, v.exact_entitlement);
    count_bytes(n, v.rounding_residual);
}
inline void validate_initial(const ApplicationStateSnapshot &s, const SqliteBundleLimits &l) {
    validate_options(l);
    if (s.balances.size() > size_t(l.balances_per_state) ||
        s.balances.size() > size_t(l.input_items))
        invalid(Input::collection_limit);
    validate_state_shape(s, l);
    ByteCount bytes(size_t(l.owned_bytes));
    count_bytes(bytes, s);
}
inline void validate_bundle(const PreparedApplicationBundle &b, const SqliteBundleLimits &l) {
    validate_key(b.intent.operation, l);
    validate_plan_key(b.intent.plan, l);
    const auto &i = b.intent;
    if (!enum_in(i.kind, 1) || !enum_in(i.subject_kind, 1) || i.receipt_id.value == 0 ||
        (i.subject_kind == ApplicationSubjectKind::contribution) != i.contribution_id.has_value() ||
        (i.kind == ApplicationOperationKind::reversal) != i.reversal_ref.has_value() ||
        (i.reversal_ref && !bounded(*i.reversal_ref, l.reference_bytes, true)))
        invalid(Input::invalid_source);
    validate_bundle_counts(b, l);
    if (!bounded(i.expected_state_token, l.reference_bytes, true))
        invalid(Input::invalid_state);
    validate_state_shape(b.advanced_before, l);
    validate_state_shape(b.after, l);
    if (b.after.token == b.advanced_before.token || b.after.token == i.expected_state_token ||
        !bounded(b.advance_explanation, l.value_bytes) ||
        (b.advanced_before.token != i.expected_state_token && b.advance_explanation.empty()))
        invalid(Input::invalid_state);
    if (i.contribution_id.has_value() != i.expected_contribution_state.has_value())
        invalid(Input::invalid_contribution);
    if (i.expected_contribution_state)
        validate_contribution(*i.expected_contribution_state, l);
    if (!valid_envelope(i.choices, l))
        invalid(Input::invalid_contribution);
    for (size_t index = 0; index < b.effects.size(); ++index)
        validate_effect(b.effects[index], index + 1, b.advanced_before, l);
    validate_deltas(b);
    validate_jobs(b, l);
    for (auto transition : b.transitions)
        if (!enum_in(transition, 5))
            invalid(Input::invalid_transition);
    if (!valid_envelope(b.result, l))
        invalid(Input::invalid_result);
    if (!valid_envelope(b.audit.envelope, l) ||
        (b.audit.actor_ref && !bounded(*b.audit.actor_ref, l.reference_bytes)) ||
        (b.audit.decision_ref && !bounded(*b.audit.decision_ref, l.reference_bytes)) ||
        (b.audit.reason && !bounded(*b.audit.reason, l.value_bytes)))
        invalid(Input::invalid_audit);
    validate_reversals(b, l);
    ByteCount bytes(size_t(l.owned_bytes));
    count_bytes(bytes, i);
    count_bytes(bytes, b.advanced_before);
    count_bytes(bytes, b.after);
    count_bytes(bytes, b.advance_explanation);
    for (const auto &effect : b.effects)
        count_bytes(bytes, effect);
    for (const auto &job : b.jobs)
        count_bytes(bytes, job);
    bytes.add(b.transitions.size());
    for (const auto &reversal : b.reversals)
        count_bytes(bytes, reversal);
    count_bytes(bytes, b.result);
    count_bytes(bytes, b.audit);
}
inline void count_bytes(ByteCount &n, const ApplicationReversalOccurrence &v) {
    count_bytes(n, v.original);
    n.add(41);
    count_bytes(n, v.cumulative_fraction);
    count_bytes(n, v.exact_entitlement);
    count_bytes(n, v.rounding_residual);
}
inline void count_bytes(ByteCount &n, const core::ReceiptIdentity &v) {
    count_bytes(n, v.origin.provider);
    count_bytes(n, v.origin.source_account);
    count_bytes(n, v.identifier_namespace);
    count_bytes(n, v.opaque_id);
}
inline void count_bytes(ByteCount &n, const core::ReceiptFacts &v) {
    count_bytes(n, v.event_type);
    n.add(4);
    if (v.source_timestamp_utc_ms)
        n.add(8);
    count_bytes(n, v.actor_id);
    count_bytes(n, v.actor_display_name);
    if (v.money) {
        count_bytes(n, v.money->original_decimal);
        count_bytes(n, v.money->currency);
    }
    if (v.payment_reference) {
        count_bytes(n, v.payment_reference->identifier_namespace);
        count_bytes(n, v.payment_reference->opaque_id);
    }
    if (v.activity_reference) {
        count_bytes(n, v.activity_reference->identifier_namespace);
        count_bytes(n, v.activity_reference->opaque_id);
    }
}
inline void count_bytes(ByteCount &n, const core::ReceiptArrival &v) {
    n.add(8);
    count_bytes(n, v.connection_ref);
    count_bytes(n, v.configuration_ref);
    count_bytes(n, v.modifier_context_ref);
}
inline void count_bytes(ByteCount &n, const core::CapturedPlanRevision &v) {
    n.add(8);
    if (v.parent_revision)
        n.add(8);
    const auto &event = v.captured.event;
    count_bytes(n, event.event_ref);
    count_bytes(n, event.parent_event_ref);
    count_bytes(n, event.root_event_ref);
    n.add(8);
    count_bytes(n, event.identity);
    count_bytes(n, event.facts);
    count_bytes(n, event.arrival);
    if (event.contribution_id)
        n.add(8);
    count_bytes(n, v.captured.contribution_state);
    n.add(1);
    if (v.captured.valuation.historical) {
        const auto &h = *v.captured.valuation.historical;
        count_bytes(n, h.resolution_ref);
        count_bytes(n, h.original_money.original_decimal);
        count_bytes(n, h.original_money.currency);
        count_bytes(n, h.comparison_currency);
        count_bytes(n, h.effective_rate_decimal);
        count_bytes(n, h.fetched_rate_decimal);
        count_bytes(n, h.snapshot_ref);
        count_bytes(n, h.source_ref);
        if (h.rate_timestamp_utc_ms)
            n.add(8);
        if (h.fetch_timestamp_utc_ms)
            n.add(8);
        count_bytes(n, h.manual_decision_ref);
    }
    for (const auto &r : v.captured.requested_effects) {
        count_bytes(n, r.action_ref);
        count_bytes(n, r.action_version_ref);
        count_bytes(n, r.rule_ref);
        count_bytes(n, r.target_ref);
        n.add(10);
        count_bytes(n, r.exact_requested_units);
        count_bytes(n, r.calculation_explanation);
        count_bytes(n, r.reversal_behavior_ref);
    }
    for (const auto &r : v.rules) {
        count_bytes(n, r.rule_ref);
        count_bytes(n, r.version_ref);
        n.add(1);
        count_bytes(n, r.explanation);
    }
    for (const auto &m : v.modifiers) {
        count_bytes(n, m.definition_version_ref);
        count_bytes(n, m.instance_revision_ref);
        count_bytes(n, m.target_ref);
        count_bytes(n, m.group_ref);
        count_bytes(n, m.group_operator);
        count_bytes(n, m.target_operator);
        count_bytes(n, m.original_factor_decimal);
    }
    if (v.override_decision) {
        n.add(1);
        count_bytes(n, v.override_decision->actor_ref);
        count_bytes(n, v.override_decision->reason);
        count_bytes(n, v.override_decision->decision_ref);
    }
}
inline void count_bytes(ByteCount &n, const VerifiedActivityIdentity &v) {
    count_bytes(n, v.origin.provider);
    count_bytes(n, v.origin.source_account);
    count_bytes(n, v.identifier_namespace);
    count_bytes(n, v.opaque_id);
}
inline void count_bytes(ByteCount &n, const ApplicationContributionInput &v) {
    const auto &c = v.contribution;
    n.add(16);
    if (c.activity_identity)
        count_bytes(n, *c.activity_identity);
    count_bytes(n, c.original_facts);
    count_bytes(n, c.original_arrival);
    count_bytes(n, c.state);
    const auto &a = v.support_anchor;
    if (a.contribution_id)
        n.add(8);
    const auto &o = a.observation;
    n.add(10); // receipt ID, observation kind and evidence.
    count_bytes(n, o.receipt.identity);
    count_bytes(n, o.receipt.facts);
    count_bytes(n, o.receipt.arrival);
    if (o.verified_activity) {
        count_bytes(n, o.verified_activity->identity);
        count_bytes(n, o.verified_activity->verification_ref);
    }
    if (o.support_conditions)
        count_bytes(n, *o.support_conditions);
}
template <class T> inline size_t logical_bytes(const T &v) {
    try {
        ByteCount count;
        count_bytes(count, v);
        return count.total();
    } catch (const SqliteBundleInputFailure &) {
        throw std::length_error("SQLite bundle logical byte count overflow");
    }
}

// Frozen step5 DDL is literal contract data, independent of test fixtures.
inline MigrationManifest make_manifest() {
    auto manifest = application_plan_repository_manifest();
    MigrationStep step;
    step.version = 5;
    step.id = "atomic-application-bundles-v1";
    const std::array<QString, 14> sql{
        "CREATE TABLE subathon_app_states(token BLOB NOT NULL CHECK(typeof(token)='blob' AND "
        "length(token)>0), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND "
        "length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), "
        "configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND "
        "length(configuration_ref)>0), lifecycle INTEGER NOT NULL "
        "CHECK(typeof(lifecycle)='integer' AND lifecycle>=0 AND lifecycle<=2), phase INTEGER NOT "
        "NULL CHECK(typeof(phase)='integer' AND phase>=0 AND phase<=1), main_remaining_ms INTEGER "
        "NOT NULL CHECK(typeof(main_remaining_ms)='integer' AND main_remaining_ms>=0), "
        "grace_remaining_ms INTEGER NOT NULL CHECK(typeof(grace_remaining_ms)='integer' AND "
        "grace_remaining_ms>=0), awaiting_confirmation INTEGER NOT NULL "
        "CHECK(typeof(awaiting_confirmation)='integer' AND awaiting_confirmation>=0 AND "
        "awaiting_confirmation<=1), time_debt_ms INTEGER NOT NULL "
        "CHECK(typeof(time_debt_ms)='integer' AND time_debt_ms>=0), balance_count INTEGER NOT NULL "
        "CHECK(typeof(balance_count)='integer' AND balance_count>=0), PRIMARY KEY(token), FOREIGN "
        "KEY(configuration_ref) REFERENCES subathon_retained_configurations(reference), "
        "CHECK(phase<>1 OR (main_remaining_ms=0 AND awaiting_confirmation=0)), "
        "CHECK(awaiting_confirmation=0 OR (lifecycle=1 AND phase=0 AND main_remaining_ms=0)))",
        "CREATE TABLE subathon_app_balances(token BLOB NOT NULL CHECK(typeof(token)='blob' AND "
        "length(token)>0), balance_kind INTEGER NOT NULL CHECK(typeof(balance_kind)='integer' AND "
        "balance_kind>=2 AND balance_kind<=3), target_ref BLOB NOT NULL "
        "CHECK(typeof(target_ref)='blob' AND length(target_ref)>0), units INTEGER NOT NULL "
        "CHECK(typeof(units)='integer'), PRIMARY KEY(token,balance_kind,target_ref), FOREIGN "
        "KEY(token) REFERENCES subathon_app_states(token))",
        "CREATE TABLE subathon_app_head(singleton INTEGER PRIMARY KEY CHECK(singleton=1), token "
        "BLOB NOT NULL CHECK(typeof(token)='blob' AND length(token)>0), FOREIGN KEY(token) "
        "REFERENCES subathon_app_states(token))",
        "CREATE TABLE subathon_app_operations(operation_scope BLOB NOT NULL "
        "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB "
        "NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), operation_kind "
        "INTEGER NOT NULL CHECK(typeof(operation_kind)='integer' AND operation_kind>=0 AND "
        "operation_kind<=1), subject_kind INTEGER NOT NULL CHECK(typeof(subject_kind)='integer' "
        "AND subject_kind>=0 AND subject_kind<=1), receipt_id BLOB NOT NULL "
        "CHECK(typeof(receipt_id)='blob' AND length(receipt_id)=8 AND "
        "receipt_id<>X'0000000000000000'), contribution_id BLOB CHECK(contribution_id IS NULL OR "
        "(typeof(contribution_id)='blob' AND length(contribution_id)=8)), plan_ref BLOB NOT NULL "
        "CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0), plan_revision BLOB NOT NULL "
        "CHECK(typeof(plan_revision)='blob' AND length(plan_revision)=8 AND "
        "plan_revision<>X'0000000000000000'), head_token BLOB NOT NULL "
        "CHECK(typeof(head_token)='blob' AND length(head_token)>0), before_token BLOB NOT NULL "
        "CHECK(typeof(before_token)='blob' AND length(before_token)>0), after_token BLOB NOT NULL "
        "CHECK(typeof(after_token)='blob' AND length(after_token)>0), advance_explanation BLOB NOT "
        "NULL CHECK(typeof(advance_explanation)='blob'), choices_format_ref BLOB NOT NULL "
        "CHECK(typeof(choices_format_ref)='blob' AND length(choices_format_ref)>0), "
        "choices_document BLOB NOT NULL CHECK(typeof(choices_document)='blob'), reversal_ref BLOB "
        "CHECK(reversal_ref IS NULL OR (typeof(reversal_ref)='blob' AND length(reversal_ref)>0)), "
        "PRIMARY KEY(operation_scope,operation_ref), UNIQUE(after_token), "
        "UNIQUE(operation_kind,receipt_id), UNIQUE(operation_scope,reversal_ref), "
        "UNIQUE(operation_scope,operation_ref,plan_ref,plan_revision), "
        "UNIQUE(operation_scope,operation_ref,before_token), FOREIGN KEY(receipt_id) REFERENCES "
        "subathon_receipts(receipt_id), FOREIGN KEY(contribution_id) REFERENCES "
        "subathon_contributions(contribution_id), FOREIGN KEY(plan_ref,plan_revision) REFERENCES "
        "subathon_plan_revisions(plan_ref,revision), FOREIGN KEY(head_token) REFERENCES "
        "subathon_app_states(token), FOREIGN KEY(before_token) REFERENCES "
        "subathon_app_states(token), FOREIGN KEY(after_token) REFERENCES "
        "subathon_app_states(token), CHECK((subject_kind=0 AND contribution_id IS NOT NULL) OR "
        "(subject_kind=1 AND contribution_id IS NULL)), CHECK((operation_kind=0 AND reversal_ref "
        "IS NULL) OR (operation_kind=1 AND reversal_ref IS NOT NULL)), "
        "CHECK(before_token<>after_token AND head_token<>after_token))",
        "CREATE TABLE subathon_app_contributions(operation_scope BLOB NOT NULL "
        "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB "
        "NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), contribution_id "
        "BLOB NOT NULL CHECK(typeof(contribution_id)='blob' AND length(contribution_id)=8), "
        "payment_evidence INTEGER NOT NULL CHECK(typeof(payment_evidence)='integer' AND "
        "payment_evidence>=0 AND payment_evidence<=3), monetary INTEGER NOT NULL "
        "CHECK(typeof(monetary)='integer' AND monetary>=0 AND monetary<=1), original_decimal BLOB "
        "CHECK(original_decimal IS NULL OR (typeof(original_decimal)='blob')), currency BLOB "
        "CHECK(currency IS NULL OR (typeof(currency)='blob')), resolution_ref BLOB "
        "CHECK(resolution_ref IS NULL OR (typeof(resolution_ref)='blob')), missing_rate INTEGER "
        "NOT NULL CHECK(typeof(missing_rate)='integer' AND missing_rate>=0 AND missing_rate<=1), "
        "review INTEGER NOT NULL CHECK(typeof(review)='integer' AND review>=0 AND review<=3), "
        "evidence_policy INTEGER NOT NULL CHECK(typeof(evidence_policy)='integer' AND "
        "evidence_policy>=0 AND evidence_policy<=2), policy_ref BLOB NOT NULL "
        "CHECK(typeof(policy_ref)='blob'), decision_choice INTEGER CHECK(decision_choice IS NULL "
        "OR (typeof(decision_choice)='integer' AND decision_choice>=0 AND decision_choice<=1)), "
        "decision_ref BLOB CHECK(decision_ref IS NULL OR (typeof(decision_ref)='blob')), "
        "decision_actor_ref BLOB CHECK(decision_actor_ref IS NULL OR "
        "(typeof(decision_actor_ref)='blob')), application INTEGER NOT NULL "
        "CHECK(typeof(application)='integer' AND application>=0 AND application<=1), reversal "
        "INTEGER NOT NULL CHECK(typeof(reversal)='integer' AND reversal>=0 AND reversal<=2), "
        "settlement INTEGER NOT NULL CHECK(typeof(settlement)='integer' AND settlement>=0 AND "
        "settlement<=1), application_after INTEGER NOT NULL "
        "CHECK(typeof(application_after)='integer' AND application_after>=0 AND "
        "application_after<=1), PRIMARY KEY(operation_scope,operation_ref), FOREIGN "
        "KEY(operation_scope,operation_ref) REFERENCES "
        "subathon_app_operations(operation_scope,operation_ref), FOREIGN KEY(contribution_id) "
        "REFERENCES subathon_contributions(contribution_id), CHECK((original_decimal IS NULL AND "
        "currency IS NULL) OR (typeof(original_decimal)='blob' AND typeof(currency)='blob')), "
        "CHECK((decision_choice IS NULL AND decision_ref IS NULL AND decision_actor_ref IS NULL) "
        "OR (typeof(decision_choice)='integer' AND decision_choice IN (0,1) AND "
        "typeof(decision_ref)='blob' AND typeof(decision_actor_ref)='blob')))",
        "CREATE TABLE subathon_app_claims(subject_kind INTEGER NOT NULL "
        "CHECK(typeof(subject_kind)='integer' AND subject_kind>=0 AND subject_kind<=1), subject_id "
        "BLOB NOT NULL CHECK(typeof(subject_id)='blob' AND length(subject_id)=8), operation_scope "
        "BLOB NOT NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), "
        "operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND "
        "length(operation_ref)>0), PRIMARY KEY(subject_kind,subject_id), "
        "UNIQUE(operation_scope,operation_ref), FOREIGN KEY(operation_scope,operation_ref) "
        "REFERENCES subathon_app_operations(operation_scope,operation_ref), CHECK(subject_kind=0 "
        "OR subject_id<>X'0000000000000000'))",
        "CREATE TABLE subathon_app_effects(operation_scope BLOB NOT NULL "
        "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB "
        "NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), request_ordinal "
        "INTEGER NOT NULL CHECK(typeof(request_ordinal)='integer' AND request_ordinal>=1), "
        "plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0), "
        "plan_revision BLOB NOT NULL CHECK(typeof(plan_revision)='blob' AND "
        "length(plan_revision)=8 AND plan_revision<>X'0000000000000000'), arithmetic_role INTEGER "
        "NOT NULL CHECK(typeof(arithmetic_role)='integer' AND arithmetic_role>=0 AND "
        "arithmetic_role<=2), unit INTEGER NOT NULL CHECK(typeof(unit)='integer' AND unit>=0 AND "
        "unit<=2), exact_numerator BLOB NOT NULL CHECK(typeof(exact_numerator)='blob' AND "
        "length(exact_numerator)>0), exact_denominator BLOB NOT NULL "
        "CHECK(typeof(exact_denominator)='blob' AND length(exact_denominator)>0), requested_units "
        "INTEGER NOT NULL CHECK(typeof(requested_units)='integer'), residual_numerator BLOB NOT "
        "NULL CHECK(typeof(residual_numerator)='blob' AND length(residual_numerator)>0), "
        "residual_denominator BLOB NOT NULL CHECK(typeof(residual_denominator)='blob' AND "
        "length(residual_denominator)>0), maximum_main_ms INTEGER CHECK(maximum_main_ms IS NULL OR "
        "(typeof(maximum_main_ms)='integer' AND maximum_main_ms>=0)), deficit_policy INTEGER "
        "CHECK(deficit_policy IS NULL OR (typeof(deficit_policy)='integer' AND deficit_policy>=0 "
        "AND deficit_policy<=2)), deficit_disposition INTEGER CHECK(deficit_disposition IS NULL OR "
        "(typeof(deficit_disposition)='integer' AND deficit_disposition>=0 AND "
        "deficit_disposition<=1)), application_explanation BLOB NOT NULL "
        "CHECK(typeof(application_explanation)='blob'), PRIMARY "
        "KEY(operation_scope,operation_ref,request_ordinal), FOREIGN "
        "KEY(operation_scope,operation_ref,plan_ref,plan_revision) REFERENCES "
        "subathon_app_operations(operation_scope,operation_ref,plan_ref,plan_revision), FOREIGN "
        "KEY(plan_ref,plan_revision,request_ordinal) REFERENCES "
        "subathon_plan_requests(plan_ref,revision,ordinal), CHECK((arithmetic_role=0 AND unit IN "
        "(1,2) AND maximum_main_ms IS NULL AND deficit_policy IS NULL AND deficit_disposition IS "
        "NULL) OR (arithmetic_role=1 AND unit=0 AND deficit_policy IS NULL AND deficit_disposition "
        "IS NULL) OR (arithmetic_role=2 AND unit=0 AND maximum_main_ms IS NULL AND deficit_policy "
        "IS NOT NULL AND deficit_disposition IS NOT NULL)))",
        "CREATE TABLE subathon_app_components(operation_scope BLOB NOT NULL "
        "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB "
        "NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), request_ordinal "
        "INTEGER NOT NULL CHECK(typeof(request_ordinal)='integer' AND request_ordinal>=1), "
        "component_kind INTEGER NOT NULL CHECK(typeof(component_kind)='integer' AND "
        "component_kind>=0 AND component_kind<=5), units INTEGER NOT NULL "
        "CHECK(typeof(units)='integer'), before_token BLOB NOT NULL "
        "CHECK(typeof(before_token)='blob' AND length(before_token)>0), target_ref BLOB "
        "CHECK(target_ref IS NULL OR (typeof(target_ref)='blob' AND length(target_ref)>0)), "
        "PRIMARY KEY(operation_scope,operation_ref,request_ordinal,component_kind), FOREIGN "
        "KEY(operation_scope,operation_ref,request_ordinal) REFERENCES "
        "subathon_app_effects(operation_scope,operation_ref,request_ordinal), FOREIGN "
        "KEY(operation_scope,operation_ref,before_token) REFERENCES "
        "subathon_app_operations(operation_scope,operation_ref,before_token), FOREIGN "
        "KEY(before_token,component_kind,target_ref) REFERENCES "
        "subathon_app_balances(token,balance_kind,target_ref), CHECK((component_kind IN (2,3) AND "
        "target_ref IS NOT NULL) OR (component_kind NOT IN (2,3) AND target_ref IS NULL)))",
        "CREATE TABLE subathon_app_transitions(operation_scope BLOB NOT NULL "
        "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB "
        "NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), ordinal INTEGER "
        "NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>=1), transition INTEGER NOT NULL "
        "CHECK(typeof(transition)='integer' AND transition>=0 AND transition<=5), PRIMARY "
        "KEY(operation_scope,operation_ref,ordinal), FOREIGN KEY(operation_scope,operation_ref) "
        "REFERENCES subathon_app_operations(operation_scope,operation_ref))",
        "CREATE TABLE subathon_app_jobs(operation_scope BLOB NOT NULL "
        "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB "
        "NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), ordinal INTEGER "
        "NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>=1), job_ref BLOB NOT NULL "
        "CHECK(typeof(job_ref)='blob' AND length(job_ref)>0), source_request_ordinal INTEGER "
        "CHECK(source_request_ordinal IS NULL OR (typeof(source_request_ordinal)='integer' AND "
        "source_request_ordinal>=1)), group_ref BLOB CHECK(group_ref IS NULL OR "
        "(typeof(group_ref)='blob' AND length(group_ref)>0)), group_ordinal INTEGER "
        "CHECK(group_ordinal IS NULL OR (typeof(group_ordinal)='integer' AND group_ordinal>=1)), "
        "format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), "
        "document BLOB NOT NULL CHECK(typeof(document)='blob'), PRIMARY "
        "KEY(operation_scope,operation_ref,ordinal), "
        "UNIQUE(operation_scope,operation_ref,job_ref), "
        "UNIQUE(operation_scope,operation_ref,group_ref,group_ordinal), FOREIGN "
        "KEY(operation_scope,operation_ref) REFERENCES "
        "subathon_app_operations(operation_scope,operation_ref), FOREIGN "
        "KEY(operation_scope,operation_ref,source_request_ordinal) REFERENCES "
        "subathon_app_effects(operation_scope,operation_ref,request_ordinal), CHECK((group_ref IS "
        "NULL AND group_ordinal IS NULL) OR (group_ref IS NOT NULL AND group_ordinal IS NOT "
        "NULL)))",
        "CREATE TABLE subathon_app_results(operation_scope BLOB NOT NULL "
        "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB "
        "NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), format_ref BLOB "
        "NOT NULL CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT "
        "NULL CHECK(typeof(document)='blob'), effect_count INTEGER NOT NULL "
        "CHECK(typeof(effect_count)='integer' AND effect_count>=0), component_count INTEGER NOT "
        "NULL CHECK(typeof(component_count)='integer' AND component_count>=0), job_count INTEGER "
        "NOT NULL CHECK(typeof(job_count)='integer' AND job_count>=0), transition_count INTEGER "
        "NOT NULL CHECK(typeof(transition_count)='integer' AND transition_count>=0), "
        "reversal_count INTEGER NOT NULL CHECK(typeof(reversal_count)='integer' AND "
        "reversal_count>=0), PRIMARY KEY(operation_scope,operation_ref), FOREIGN "
        "KEY(operation_scope,operation_ref) REFERENCES "
        "subathon_app_operations(operation_scope,operation_ref))",
        "CREATE TABLE subathon_app_audit(operation_scope BLOB NOT NULL "
        "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB "
        "NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), actor_ref BLOB "
        "CHECK(actor_ref IS NULL OR (typeof(actor_ref)='blob')), decision_ref BLOB "
        "CHECK(decision_ref IS NULL OR (typeof(decision_ref)='blob')), reason BLOB CHECK(reason IS "
        "NULL OR (typeof(reason)='blob')), format_ref BLOB NOT NULL "
        "CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT NULL "
        "CHECK(typeof(document)='blob'), PRIMARY KEY(operation_scope,operation_ref), FOREIGN "
        "KEY(operation_scope,operation_ref) REFERENCES "
        "subathon_app_operations(operation_scope,operation_ref))",
        "CREATE TABLE subathon_app_reversals(operation_scope BLOB NOT NULL "
        "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB "
        "NOT NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), request_ordinal "
        "INTEGER NOT NULL CHECK(typeof(request_ordinal)='integer' AND request_ordinal>=1), "
        "original_scope BLOB NOT NULL CHECK(typeof(original_scope)='blob' AND "
        "length(original_scope)>0), original_operation BLOB NOT NULL "
        "CHECK(typeof(original_operation)='blob' AND length(original_operation)>0), "
        "original_request_ordinal INTEGER NOT NULL "
        "CHECK(typeof(original_request_ordinal)='integer' AND original_request_ordinal>=1), "
        "original_component_kind INTEGER NOT NULL CHECK(typeof(original_component_kind)='integer' "
        "AND original_component_kind>=0 AND original_component_kind<=3), basis_position INTEGER "
        "NOT NULL CHECK(typeof(basis_position)='integer' AND basis_position>=1), "
        "fraction_numerator BLOB NOT NULL CHECK(typeof(fraction_numerator)='blob' AND "
        "length(fraction_numerator)>0), fraction_denominator BLOB NOT NULL "
        "CHECK(typeof(fraction_denominator)='blob' AND length(fraction_denominator)>0), "
        "entitlement_numerator BLOB NOT NULL CHECK(typeof(entitlement_numerator)='blob' AND "
        "length(entitlement_numerator)>0), entitlement_denominator BLOB NOT NULL "
        "CHECK(typeof(entitlement_denominator)='blob' AND length(entitlement_denominator)>0), "
        "residual_numerator BLOB NOT NULL CHECK(typeof(residual_numerator)='blob' AND "
        "length(residual_numerator)>0), residual_denominator BLOB NOT NULL "
        "CHECK(typeof(residual_denominator)='blob' AND length(residual_denominator)>0), "
        "previous_cumulative INTEGER NOT NULL CHECK(typeof(previous_cumulative)='integer'), "
        "cumulative INTEGER NOT NULL CHECK(typeof(cumulative)='integer'), requested_increment "
        "INTEGER NOT NULL CHECK(typeof(requested_increment)='integer'), full_refund INTEGER NOT "
        "NULL CHECK(typeof(full_refund)='integer' AND full_refund>=0 AND full_refund<=1), PRIMARY "
        "KEY(operation_scope,operation_ref,request_ordinal,original_scope,original_operation,"
        "original_request_ordinal,original_component_kind), "
        "UNIQUE(original_scope,original_operation,original_request_ordinal,original_component_kind,"
        "basis_position), FOREIGN KEY(operation_scope,operation_ref,request_ordinal) REFERENCES "
        "subathon_app_effects(operation_scope,operation_ref,request_ordinal), FOREIGN "
        "KEY(original_scope,original_operation,original_request_ordinal,original_component_kind) "
        "REFERENCES "
        "subathon_app_components(operation_scope,operation_ref,request_ordinal,component_kind), "
        "CHECK(original_component_kind IN (0,2,3)))",
        "CREATE TABLE subathon_app_reversal_heads(original_scope BLOB NOT NULL "
        "CHECK(typeof(original_scope)='blob' AND length(original_scope)>0), original_operation "
        "BLOB NOT NULL CHECK(typeof(original_operation)='blob' AND length(original_operation)>0), "
        "original_request_ordinal INTEGER NOT NULL "
        "CHECK(typeof(original_request_ordinal)='integer' AND original_request_ordinal>=1), "
        "original_component_kind INTEGER NOT NULL CHECK(typeof(original_component_kind)='integer' "
        "AND original_component_kind>=0 AND original_component_kind<=3), basis_position INTEGER "
        "NOT NULL CHECK(typeof(basis_position)='integer' AND basis_position>=1), cumulative "
        "INTEGER NOT NULL CHECK(typeof(cumulative)='integer'), last_scope BLOB NOT NULL "
        "CHECK(typeof(last_scope)='blob' AND length(last_scope)>0), last_operation BLOB NOT NULL "
        "CHECK(typeof(last_operation)='blob' AND length(last_operation)>0), last_request_ordinal "
        "INTEGER NOT NULL CHECK(typeof(last_request_ordinal)='integer' AND "
        "last_request_ordinal>=1), PRIMARY "
        "KEY(original_scope,original_operation,original_request_ordinal,original_component_kind), "
        "FOREIGN "
        "KEY(original_scope,original_operation,original_request_ordinal,original_component_kind) "
        "REFERENCES "
        "subathon_app_components(operation_scope,operation_ref,request_ordinal,component_kind), "
        "FOREIGN "
        "KEY(original_scope,original_operation,original_request_ordinal,original_component_kind,"
        "basis_position) REFERENCES "
        "subathon_app_reversals(original_scope,original_operation,original_request_ordinal,"
        "original_component_kind,basis_position), FOREIGN "
        "KEY(last_scope,last_operation,last_request_ordinal,original_scope,original_operation,"
        "original_request_ordinal,original_component_kind) REFERENCES "
        "subathon_app_reversals(operation_scope,operation_ref,request_ordinal,original_scope,"
        "original_operation,original_request_ordinal,original_component_kind), "
        "CHECK(original_component_kind IN (0,2,3)))",
    };
    const std::array<int, 14> indexes{1, 1, 0, 6, 1, 2, 1, 1, 1, 3, 1, 1, 2, 1};
    step.expected_schema = manifest.steps.back().expected_schema;
    for (size_t i = 0; i < sql.size(); ++i) {
        const auto table = sql[i].section(' ', 2, 2).section('(', 0, 0);
        step.sql.append(sql[i]);
        step.expected_schema.append({"table", table, table, sql[i]});
        for (int index = 1; index <= indexes[i]; ++index)
            step.expected_schema.append({"index",
                                         "sqlite_autoindex_" + table + "_" + QString::number(index),
                                         table, std::nullopt});
    }
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return a.type < b.type || (a.type == b.type && a.name < b.name);
              });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions = {0, 1, 2, 3, 4, 5};
    return manifest;
}
} // namespace subathon::server::bundle_detail
