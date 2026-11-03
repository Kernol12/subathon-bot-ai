// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sqlite_application_bundle_values.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <string_view>
#include <subathon/server/sqlite_application_holds.hpp>
#include <type_traits>

namespace subathon::server::hold_detail {
using Input = SqliteHoldInputError;
[[noreturn]] inline void invalid(Input error) { throw SqliteHoldInputFailure(error); }
template <class E> inline bool enum_in(E value, int maximum) {
    return int(value) >= 0 && int(value) <= maximum;
}
inline bool raw_less(std::string_view a, std::string_view b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                        [](unsigned char x, unsigned char y) { return x < y; });
}
struct RawLess {
    bool operator()(std::string_view a, std::string_view b) const { return raw_less(a, b); }
};
inline bool bounded(const std::string &s, qsizetype limit) { return s.size() <= size_t(limit); }
inline void key(const std::string &s, const SqliteHoldLimits &l) {
    if (s.empty() || !bounded(s, l.references.reference_bytes))
        invalid(Input::invalid_key);
}
inline void key(const std::optional<std::string> &s, const SqliteHoldLimits &l) {
    if (s)
        key(*s, l);
}
inline void key(const ApplicationOperationKey &v, const SqliteHoldLimits &l) {
    key(v.operation_scope, l);
    key(v.operation_ref, l);
}
inline void key(const ApplicationPlanKey &v, const SqliteHoldLimits &l) {
    key(v.plan_ref, l);
    if (v.revision == 0)
        invalid(Input::invalid_key);
}
inline void key(const ApplicationEnvelope &v, const SqliteHoldLimits &l) { key(v.format_ref, l); }
inline void key_subject(const ApplicationHoldSubject &v) {
    if (v.id.valueless_by_exception() || v.id.index() > 1 ||
        (v.id.index() == 1 && std::get<core::ReceiptId>(v.id).value == 0))
        invalid(Input::invalid_subject);
}
inline void key_observation(const ApplicationHoldObservation &v, const SqliteHoldLimits &l) {
    key(v.persisted_state_token, l);
    key(v.annotation, l);
    key(v.policy_revision_ref, l);
    key(v.permission_revision_ref, l);
    key(v.actor_ref, l);
    key(v.decision_ref, l);
}
inline void key_cause(const ApplicationHoldCause &v, const SqliteHoldLimits &l) {
    key(v.hold_ref, l);
    key_subject(v.source.subject);
    if (v.source.receipt_id.value == 0)
        invalid(Input::invalid_subject);
    key(v.source.configuration_ref, l);
    key(v.source.arrival_context_ref, l);
    key(v.command.operation, l);
    key(v.command.intent, l);
    key_observation(v.first_observation, l);
    if (v.plan_evidence.plan)
        key(*v.plan_evidence.plan, l);
    if (v.plan_evidence.failed_capture)
        key(*v.plan_evidence.failed_capture, l);
    if (v.failure)
        key(v.failure->annotation, l);
    if (v.timer) {
        key(v.timer->composition_decision, l);
        if (v.timer->associated)
            key(v.timer->associated->operation, l);
    }
    key(v.supersedes_hold_ref, l);
}
inline void key_detection_data(const ApplicationHoldDetectionData &v, const SqliteHoldLimits &l) {
    key(v.mutation_ref, l);
    key(v.expected_version_ref, l);
    key(v.proposed_version_ref, l);
    key_observation(v.observation, l);
    key(v.audit, l);
}
inline void key_detection(const ApplicationHoldDetection &v, const SqliteHoldLimits &l) {
    key_cause(v.cause, l);
    key_detection_data(v.request, l);
}
inline void key_resolution(const ApplicationHoldResolutionBatch &v, const SqliteHoldLimits &l) {
    key(v.mutation_ref, l);
    key_subject(v.subject);
    key(v.actor_ref, l);
    key(v.decision_ref, l);
    key(v.options, l);
    for (const auto &x : v.expected_active) {
        key(x.hold_ref, l);
        key(x.version_ref, l);
    }
    for (const auto &x : v.entries) {
        key(x.hold_ref, l);
        key(x.expected_version_ref, l);
        key(x.proposed_version_ref, l);
        key(x.decision, l);
        if (x.replacement)
            key_detection(*x.replacement, l);
    }
    if (v.application) {
        const auto &a = *v.application;
        key(a.operation, l);
        key(a.plan, l);
        if (a.receipt_id.value == 0)
            invalid(Input::invalid_subject);
        if (a.held_origin) {
            key(a.held_origin->version.hold_ref, l);
            key(a.held_origin->version.version_ref, l);
        }
    }
}

inline void validate_options(const SqliteHoldLimits &l) {
    try {
        bundle_detail::validate_options(l.references);
    } catch (const SqliteBundleInputFailure &e) {
        if (e.error() != SqliteBundleInputError::invalid_options)
            throw;
        invalid(Input::invalid_options);
    }
    const auto count = [](qsizetype n) {
        return n > 0 && std::uintmax_t(n) < std::uintmax_t(std::numeric_limits<qsizetype>::max()) &&
               std::uintmax_t(n) < std::uintmax_t(std::numeric_limits<std::int64_t>::max()) &&
               std::uintmax_t(n) < std::uintmax_t(std::numeric_limits<size_t>::max());
    };
    if (!count(l.known_occurrences_per_cause) || !count(l.active_holds_per_result) ||
        !count(l.resolution_entries) || !count(l.history_versions_per_page) ||
        !count(l.input_items) || !count(l.result_items) || l.owned_bytes <= 0 ||
        std::uintmax_t(l.known_occurrences_per_cause) >=
            std::vector<ApplicationHoldProposedOccurrence>{}.max_size() ||
        std::uintmax_t(l.active_holds_per_result) >=
            std::vector<ApplicationHoldView>{}.max_size() ||
        std::uintmax_t(l.active_holds_per_result) >=
            std::vector<ApplicationHoldVersionLocator>{}.max_size() ||
        std::uintmax_t(l.resolution_entries) >=
            std::vector<ApplicationHoldResolutionEntry>{}.max_size() ||
        std::uintmax_t(l.history_versions_per_page) >=
            std::vector<ApplicationHoldHistoryEntry>{}.max_size())
        invalid(Input::invalid_options);
    const auto entries = std::uintmax_t(l.resolution_entries);
    for (const auto maximum : {std::uintmax_t(std::numeric_limits<qsizetype>::max()),
                               std::uintmax_t(std::numeric_limits<std::int64_t>::max()),
                               std::uintmax_t(std::numeric_limits<size_t>::max()),
                               std::uintmax_t(std::vector<ApplicationHoldVersion>{}.max_size())})
        if (entries > (maximum - 1) / 2)
            invalid(Input::invalid_options);
}
inline bool envelope_body(const ApplicationEnvelope &v, const SqliteHoldLimits &l) {
    return !v.document.isNull() && v.document.size() <= l.references.value_bytes;
}
inline bool optional_data(const std::optional<std::string> &v, qsizetype limit) {
    return !v || bounded(*v, limit);
}
inline void shape_exact(const core::Exact &v, const SqliteHoldLimits &l) {
    const auto numerator = boost::multiprecision::numerator(v).str();
    const auto denominator = boost::multiprecision::denominator(v).str();
    if (numerator.size() > size_t(l.references.component_bytes) ||
        denominator.size() > size_t(l.references.component_bytes))
        invalid(Input::exact_component_limit);
}
// These internal passes are request-wide: no matrix/Exact work starts until
// every required body is checked, and no ordering starts until all matrices fit.
inline void body_observation(const ApplicationHoldObservation &v, const SqliteHoldLimits &l) {
    if (!envelope_body(v.annotation, l) || (v.elapsed_ms && *v.elapsed_ms < 0))
        invalid(Input::invalid_observation);
}
inline void body_cause(const ApplicationHoldCause &v, const SqliteHoldLimits &l) {
    if (!enum_in(v.kind, 2) || !envelope_body(v.command.intent, l) ||
        !optional_data(v.source.event_ref, l.references.reference_bytes) ||
        !optional_data(v.source.parent_event_ref, l.references.reference_bytes) ||
        !optional_data(v.source.root_event_ref, l.references.reference_bytes) ||
        !optional_data(v.missing_policy_reason, l.references.value_bytes))
        invalid(Input::invalid_cause);
    body_observation(v.first_observation, l);
    const auto &p = v.plan_evidence;
    if (!enum_in(p.kind, 1) || !enum_in(p.extent, 2) ||
        (p.stopped_for && !enum_in(*p.stopped_for, 1)) ||
        (p.failed_capture && !envelope_body(*p.failed_capture, l)))
        invalid(Input::invalid_plan_evidence);
    const bool captured = p.kind == ApplicationHoldPlanKind::captured_revision;
    if ((captured && (!p.plan || !v.first_observation.persisted_state_token)) ||
        (!captured && (!p.stopped_for || !p.failed_capture)))
        invalid(Input::invalid_plan_evidence);
    for (const auto &o : p.occurrences)
        if (o.ordinal <= 0 || (o.unit && !enum_in(*o.unit, 2)) ||
            !optional_data(o.action_ref, l.references.reference_bytes) ||
            !optional_data(o.action_version_ref, l.references.reference_bytes) ||
            !optional_data(o.rule_ref, l.references.reference_bytes) ||
            !optional_data(o.target_ref, l.references.reference_bytes) ||
            !optional_data(o.explanation, l.references.value_bytes) ||
            (captured && (!o.unit || !o.attempted_units || !o.finalized_units)))
            invalid(Input::invalid_plan_evidence);
    if (v.kind == ApplicationHoldCauseKind::numeric_overflow && !v.failure)
        invalid(Input::invalid_failure);
    if (v.kind != ApplicationHoldCauseKind::numeric_overflow && v.failure)
        invalid(Input::invalid_failure);
    if ((v.kind == ApplicationHoldCauseKind::timer_deficit) != bool(v.timer) ||
        (v.kind == ApplicationHoldCauseKind::unresolved_policy) != bool(v.missing_policy_reason))
        invalid(Input::invalid_cause);
    if (v.failure) {
        const auto &f = *v.failure;
        if (!enum_in(f.stage, 1) || !envelope_body(f.annotation, l) ||
            !optional_data(f.failing_target_ref, l.references.reference_bytes) ||
            (f.failing_ordinal && *f.failing_ordinal <= 0))
            invalid(Input::invalid_failure);
    }
    if (v.timer) {
        const auto &t = *v.timer;
        if (t.request_ordinal <= 0 ||
            t.component_kind != ApplicationComponentKind::unapplied_time || t.request_units >= 0 ||
            t.unapplied_units != t.request_units ||
            t.deficit_policy != core::RefundDeficitPolicy::RequireReview ||
            t.deficit_disposition != core::TimeReversalDisposition::RequiresReview ||
            !t.composition || !enum_in(*t.composition, 1) ||
            !envelope_body(t.composition_decision, l) ||
            bool(t.associated) !=
                (*t.composition == ApplicationHoldComposition::beside_complete_application) ||
            (t.associated && t.associated->request_ordinal <= 0))
            invalid(Input::invalid_cause);
    }
}
inline void body_detection_data(const ApplicationHoldDetectionData &v, const SqliteHoldLimits &l) {
    body_observation(v.observation, l);
    if (!envelope_body(v.audit, l))
        invalid(Input::invalid_detection);
}
inline void body_detection(const ApplicationHoldDetection &v, const SqliteHoldLimits &l) {
    body_cause(v.cause, l);
    body_detection_data(v.request, l);
    if (v.cause.plan_evidence.kind == ApplicationHoldPlanKind::captured_revision &&
        !v.request.observation.persisted_state_token)
        invalid(Input::invalid_plan_evidence);
}
inline void body_resolution(const ApplicationHoldResolutionBatch &v, const SqliteHoldLimits &l) {
    if (v.entries.empty() || !bounded(v.reason, l.references.value_bytes) ||
        !envelope_body(v.options, l))
        invalid(Input::invalid_resolution);
    for (const auto &e : v.entries) {
        if (!envelope_body(e.decision, l))
            invalid(Input::invalid_resolution);
        if (e.replacement)
            body_detection(*e.replacement, l);
    }
    if (v.application) {
        const auto &a = *v.application;
        if (!enum_in(a.kind, 1) ||
            bool(a.held_origin) !=
                (a.kind == ApplicationHoldApplicationLinkKind::timer_correction) ||
            (a.held_origin &&
             (a.held_origin->request_ordinal <= 0 ||
              a.held_origin->component_kind != ApplicationComponentKind::unapplied_time)))
            invalid(Input::invalid_resolution);
    }
}
inline void matrix_cause(const ApplicationHoldCause &v, const SqliteHoldLimits &l) {
    const auto &p = v.plan_evidence;
    const bool captured = p.kind == ApplicationHoldPlanKind::captured_revision;
    if (captured) {
        if (!p.plan || p.extent != ApplicationHoldExtent::captured_complete || p.stopped_for ||
            p.failed_capture || !v.first_observation.persisted_state_token)
            invalid(Input::invalid_plan_evidence);
        for (const auto &o : p.occurrences)
            if (!o.unit || !o.attempted_units || !o.finalized_units)
                invalid(Input::invalid_plan_evidence);
    } else {
        if (p.extent == ApplicationHoldExtent::captured_complete || !p.stopped_for ||
            !p.failed_capture || v.kind == ApplicationHoldCauseKind::timer_deficit ||
            (v.kind == ApplicationHoldCauseKind::numeric_overflow &&
             *p.stopped_for != ApplicationHoldStoppedFor::numeric_conversion) ||
            (v.kind == ApplicationHoldCauseKind::unresolved_policy &&
             *p.stopped_for != ApplicationHoldStoppedFor::unresolved_capture_policy))
            invalid(Input::invalid_plan_evidence);
    }
    if (v.failure) {
        const auto &f = *v.failure;
        if (f.stage != (captured ? ApplicationHoldFailureStage::aggregate_after_finalization
                                 : ApplicationHoldFailureStage::final_conversion))
            invalid(Input::invalid_failure);
        if (f.failing_ordinal) {
            // Membership uses the supplied ordinal; its full sequence order is
            // validated later, rather than assumed to be an array index here.
            const auto it =
                std::find_if(p.occurrences.begin(), p.occurrences.end(),
                             [&](const auto &o) { return o.ordinal == *f.failing_ordinal; });
            if (it == p.occurrences.end() || (!captured && it->finalized_units) ||
                (f.failing_target_ref && it->target_ref &&
                 *f.failing_target_ref != *it->target_ref))
                invalid(Input::invalid_failure);
        }
    }
    if (v.timer) {
        const auto &t = *v.timer;
        const auto it = std::find_if(p.occurrences.begin(), p.occurrences.end(),
                                     [&](const auto &o) { return o.ordinal == t.request_ordinal; });
        if (it == p.occurrences.end() || it->unit != core::RequestedUnit::milliseconds ||
            it->finalized_units != t.request_units ||
            (t.associated && t.associated->request_ordinal != t.request_ordinal))
            invalid(Input::invalid_cause);
    }
    for (const auto &o : p.occurrences)
        if (o.attempted_units)
            shape_exact(*o.attempted_units, l);
    if (v.failure && v.failure->attempted_units)
        shape_exact(*v.failure->attempted_units, l);
}
inline void matrix_detection(const ApplicationHoldDetection &v, const SqliteHoldLimits &l) {
    matrix_cause(v.cause, l);
    if (v.cause.plan_evidence.kind == ApplicationHoldPlanKind::captured_revision &&
        !v.request.observation.persisted_state_token)
        invalid(Input::invalid_plan_evidence);
}
inline void matrix_resolution(const ApplicationHoldResolutionBatch &v, const SqliteHoldLimits &l) {
    for (const auto &e : v.entries)
        if (e.replacement)
            matrix_detection(*e.replacement, l);
}
inline void order_cause(const ApplicationHoldCause &v) {
    const auto &occurrences = v.plan_evidence.occurrences;
    for (size_t n = 0; n < occurrences.size(); ++n)
        if (n >= size_t(std::numeric_limits<std::int64_t>::max()) ||
            occurrences[n].ordinal != std::int64_t(n + 1))
            invalid(Input::invalid_plan_evidence);
}
inline void order_detection(const ApplicationHoldDetection &v) { order_cause(v.cause); }
inline void order_resolution(const ApplicationHoldResolutionBatch &v) {
    for (size_t n = 0; n < v.expected_active.size(); ++n)
        if (n && !raw_less(v.expected_active[n - 1].hold_ref, v.expected_active[n].hold_ref))
            invalid(Input::invalid_resolution);
    std::set<std::string_view, RawLess> replacement_holds, replacement_mutations;
    for (size_t n = 0; n < v.entries.size(); ++n) {
        const auto &e = v.entries[n];
        if (n && !raw_less(v.entries[n - 1].hold_ref, e.hold_ref))
            invalid(Input::invalid_resolution);
        if (e.replacement) {
            const auto &r = *e.replacement;
            order_detection(r);
            if (r.request.expected_version_ref || r.cause.source.subject.id != v.subject.id ||
                r.cause.hold_ref == e.hold_ref || r.cause.supersedes_hold_ref != e.hold_ref ||
                r.request.mutation_ref == v.mutation_ref ||
                !replacement_holds.insert(r.cause.hold_ref).second ||
                !replacement_mutations.insert(r.request.mutation_ref).second || v.application)
                invalid(Input::invalid_resolution);
        }
    }
}
// Stored-body wrappers deliberately have no input aggregate or collection quota.
// Runtime proves actual selected counts before allocating, then applies result limits.
inline void shape_observation(const ApplicationHoldObservation &v, const SqliteHoldLimits &l) {
    key_observation(v, l);
    body_observation(v, l);
}
inline void shape_cause(const ApplicationHoldCause &v, const SqliteHoldLimits &l) {
    key_cause(v, l);
    body_cause(v, l);
    matrix_cause(v, l);
    order_cause(v);
}
inline void shape_detection_data(const ApplicationHoldDetectionData &v, const SqliteHoldLimits &l) {
    key_detection_data(v, l);
    body_detection_data(v, l);
}
inline void shape_detection(const ApplicationHoldDetection &v, const SqliteHoldLimits &l) {
    key_detection(v, l);
    body_detection(v, l);
    matrix_detection(v, l);
    order_detection(v);
}
inline void shape_resolution(const ApplicationHoldResolutionBatch &v, const SqliteHoldLimits &l) {
    key_resolution(v, l);
    body_resolution(v, l);
    matrix_resolution(v, l);
    order_resolution(v);
}

// A checked sum is shared only as arithmetic, never as a fictional aggregate
// vector capacity. Optional engagement and vector length add no byte framing.
struct Size {
    size_t value{0};
    void add(size_t n) {
        if (n > std::numeric_limits<size_t>::max() - value)
            throw std::length_error("Hold logical size overflow");
        value += n;
    }
};
inline size_t bytes(const ApplicationOperationKey &v);
inline bool same(const ApplicationOperationKey &a, const ApplicationOperationKey &b);
inline size_t bytes(const ApplicationPlanKey &v);
inline bool same(const ApplicationPlanKey &a, const ApplicationPlanKey &b);
inline size_t bytes(const ApplicationEnvelope &v);
inline bool same(const ApplicationEnvelope &a, const ApplicationEnvelope &b);
inline size_t bytes(const ApplicationHoldSubject &v);
inline bool same(const ApplicationHoldSubject &a, const ApplicationHoldSubject &b);
inline size_t bytes(const ApplicationHoldSource &v);
inline bool same(const ApplicationHoldSource &a, const ApplicationHoldSource &b);
inline size_t bytes(const ApplicationHoldCommand &v);
inline bool same(const ApplicationHoldCommand &a, const ApplicationHoldCommand &b);
inline size_t bytes(const ApplicationHoldObservation &v);
inline bool same(const ApplicationHoldObservation &a, const ApplicationHoldObservation &b);
inline size_t bytes(const ApplicationHoldProposedOccurrence &v);
inline bool same(const ApplicationHoldProposedOccurrence &a,
                 const ApplicationHoldProposedOccurrence &b);
inline size_t bytes(const ApplicationHoldPlanEvidence &v);
inline bool same(const ApplicationHoldPlanEvidence &a, const ApplicationHoldPlanEvidence &b);
inline size_t bytes(const ApplicationHoldFailureEvidence &v);
inline bool same(const ApplicationHoldFailureEvidence &a, const ApplicationHoldFailureEvidence &b);
inline size_t bytes(const ApplicationHoldAssociatedApplication &v);
inline bool same(const ApplicationHoldAssociatedApplication &a,
                 const ApplicationHoldAssociatedApplication &b);
inline size_t bytes(const ApplicationHoldTimerEvidence &v);
inline bool same(const ApplicationHoldTimerEvidence &a, const ApplicationHoldTimerEvidence &b);
inline size_t bytes(const ApplicationHoldCause &v);
inline bool same(const ApplicationHoldCause &a, const ApplicationHoldCause &b);
inline size_t bytes(const ApplicationHoldDetectionData &v);
inline bool same(const ApplicationHoldDetectionData &a, const ApplicationHoldDetectionData &b);
inline size_t bytes(const ApplicationHoldDetection &v);
inline bool same(const ApplicationHoldDetection &a, const ApplicationHoldDetection &b);
inline size_t bytes(const ApplicationHoldVersionLocator &v);
inline bool same(const ApplicationHoldVersionLocator &a, const ApplicationHoldVersionLocator &b);
inline size_t bytes(const ApplicationHoldReplacementLocator &v);
inline bool same(const ApplicationHoldReplacementLocator &a,
                 const ApplicationHoldReplacementLocator &b);
inline size_t bytes(const ApplicationHoldVersion &v);
inline bool same(const ApplicationHoldVersion &a, const ApplicationHoldVersion &b);
inline size_t bytes(const ApplicationHoldHeldOrigin &v);
inline bool same(const ApplicationHoldHeldOrigin &a, const ApplicationHoldHeldOrigin &b);
inline size_t bytes(const ApplicationHoldApplicationLink &v);
inline bool same(const ApplicationHoldApplicationLink &a, const ApplicationHoldApplicationLink &b);
inline size_t bytes(const ApplicationHoldResolutionEntry &v);
inline bool same(const ApplicationHoldResolutionEntry &a, const ApplicationHoldResolutionEntry &b);
inline size_t bytes(const ApplicationHoldResolutionBatch &v);
inline bool same(const ApplicationHoldResolutionBatch &a, const ApplicationHoldResolutionBatch &b);
inline size_t bytes(const ApplicationHoldRecordedResolution &v);
inline bool same(const ApplicationHoldRecordedResolution &a,
                 const ApplicationHoldRecordedResolution &b);
inline size_t bytes(const ApplicationHoldView &v);
inline bool same(const ApplicationHoldView &a, const ApplicationHoldView &b);
inline size_t bytes(const ApplicationHoldActiveSet &v);
inline bool same(const ApplicationHoldActiveSet &a, const ApplicationHoldActiveSet &b);
inline size_t bytes(const ApplicationHoldHistoryEntry &v);
inline bool same(const ApplicationHoldHistoryEntry &a, const ApplicationHoldHistoryEntry &b);
inline size_t bytes(const ApplicationHoldHistoryQuery &v);
inline bool same(const ApplicationHoldHistoryQuery &a, const ApplicationHoldHistoryQuery &b);
inline size_t bytes(const ApplicationHoldHistoryPage &v);
inline bool same(const ApplicationHoldHistoryPage &a, const ApplicationHoldHistoryPage &b);
inline size_t bytes(const ApplicationHoldHistoryResult &v);
inline bool same(const ApplicationHoldHistoryResult &a, const ApplicationHoldHistoryResult &b);
inline size_t bytes(const ApplicationHoldMutationOutcome &v);
inline bool same(const ApplicationHoldMutationOutcome &a, const ApplicationHoldMutationOutcome &b);
inline size_t bytes(const ApplicationHoldMutationResult &v);
inline bool same(const ApplicationHoldMutationResult &a, const ApplicationHoldMutationResult &b);

template <class T>
    requires(std::is_integral_v<T> || std::is_enum_v<T>)
inline size_t bytes(const T &) {
    return std::is_enum_v<T> || std::is_same_v<T, bool> ? 1 : 8;
}
inline size_t bytes(const std::string &v) { return v.size(); }
inline size_t bytes(const QByteArray &v) { return size_t(v.size()); }
inline size_t bytes(const core::ReceiptId &) { return 8; }
inline size_t bytes(const core::ContributionId &) { return 8; }
inline size_t bytes(const core::Exact &v) {
    Size n;
    n.add(boost::multiprecision::numerator(v).str().size());
    n.add(boost::multiprecision::denominator(v).str().size());
    return n.value;
}
template <class T> inline size_t bytes(const std::optional<T> &v);
template <class T> inline size_t bytes(const std::vector<T> &v);
template <class... T> inline size_t bytes(const std::variant<T...> &v);
template <class... T> inline size_t sum_bytes(const T &...v) {
    Size n;
    (n.add(bytes(v)), ...);
    return n.value;
}
template <class T> inline size_t bytes(const std::optional<T> &v) { return v ? bytes(*v) : 0; }
template <class T> inline size_t bytes(const std::vector<T> &v) {
    Size n;
    for (const auto &x : v)
        n.add(bytes(x));
    return n.value;
}
template <class... T> inline size_t bytes(const std::variant<T...> &v) {
    Size n;
    n.add(1);
    n.add(std::visit([](const auto &x) { return bytes(x); }, v));
    return n.value;
}
template <class T> inline bool same(const T &a, const T &b) { return a == b; }
inline bool same(const QByteArray &a, const QByteArray &b) {
    return a.isNull() == b.isNull() && a == b;
}
template <class T> inline bool same(const std::optional<T> &a, const std::optional<T> &b);
template <class T> inline bool same(const std::vector<T> &a, const std::vector<T> &b);
template <class... T> inline bool same(const std::variant<T...> &a, const std::variant<T...> &b);
template <class T> inline bool same(const std::optional<T> &a, const std::optional<T> &b) {
    return bool(a) == bool(b) && (!a || same(*a, *b));
}
template <class T> inline bool same(const std::vector<T> &a, const std::vector<T> &b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(),
                      [](const auto &x, const auto &y) { return same(x, y); });
}
template <class... T> inline bool same(const std::variant<T...> &a, const std::variant<T...> &b) {
    return !a.valueless_by_exception() && !b.valueless_by_exception() && a.index() == b.index() &&
           std::visit(
               [&](const auto &x) { return same(x, std::get<std::decay_t<decltype(x)>>(b)); }, a);
}
inline size_t bytes(const ApplicationOperationKey &v) {
    return sum_bytes(v.operation_scope, v.operation_ref);
}
inline bool same(const ApplicationOperationKey &a, const ApplicationOperationKey &b) {
    return same(a.operation_scope, b.operation_scope) && same(a.operation_ref, b.operation_ref);
}
inline size_t bytes(const ApplicationPlanKey &v) { return sum_bytes(v.plan_ref, v.revision); }
inline bool same(const ApplicationPlanKey &a, const ApplicationPlanKey &b) {
    return same(a.plan_ref, b.plan_ref) && same(a.revision, b.revision);
}
inline size_t bytes(const ApplicationEnvelope &v) { return sum_bytes(v.format_ref, v.document); }
inline bool same(const ApplicationEnvelope &a, const ApplicationEnvelope &b) {
    return same(a.format_ref, b.format_ref) && same(a.document, b.document);
}
inline size_t bytes(const ApplicationHoldSubject &v) { return sum_bytes(v.id); }
inline bool same(const ApplicationHoldSubject &a, const ApplicationHoldSubject &b) {
    return same(a.id, b.id);
}
inline size_t bytes(const ApplicationHoldSource &v) {
    return sum_bytes(v.subject, v.receipt_id, v.event_ref, v.parent_event_ref, v.root_event_ref,
                     v.configuration_ref, v.arrival_context_ref);
}
inline bool same(const ApplicationHoldSource &a, const ApplicationHoldSource &b) {
    return same(a.subject, b.subject) && same(a.receipt_id, b.receipt_id) &&
           same(a.event_ref, b.event_ref) && same(a.parent_event_ref, b.parent_event_ref) &&
           same(a.root_event_ref, b.root_event_ref) &&
           same(a.configuration_ref, b.configuration_ref) &&
           same(a.arrival_context_ref, b.arrival_context_ref);
}
inline size_t bytes(const ApplicationHoldCommand &v) { return sum_bytes(v.operation, v.intent); }
inline bool same(const ApplicationHoldCommand &a, const ApplicationHoldCommand &b) {
    return same(a.operation, b.operation) && same(a.intent, b.intent);
}
inline size_t bytes(const ApplicationHoldObservation &v) {
    return sum_bytes(v.persisted_state_token, v.annotation, v.policy_revision_ref,
                     v.permission_revision_ref, v.actor_ref, v.decision_ref, v.observed_utc_ms,
                     v.elapsed_ms);
}
inline bool same(const ApplicationHoldObservation &a, const ApplicationHoldObservation &b) {
    return same(a.persisted_state_token, b.persisted_state_token) &&
           same(a.annotation, b.annotation) && same(a.policy_revision_ref, b.policy_revision_ref) &&
           same(a.permission_revision_ref, b.permission_revision_ref) &&
           same(a.actor_ref, b.actor_ref) && same(a.decision_ref, b.decision_ref) &&
           same(a.observed_utc_ms, b.observed_utc_ms) && same(a.elapsed_ms, b.elapsed_ms);
}
inline size_t bytes(const ApplicationHoldProposedOccurrence &v) {
    return sum_bytes(v.ordinal, v.action_ref, v.action_version_ref, v.rule_ref, v.target_ref,
                     v.unit, v.attempted_units, v.finalized_units, v.explanation);
}
inline bool same(const ApplicationHoldProposedOccurrence &a,
                 const ApplicationHoldProposedOccurrence &b) {
    return same(a.ordinal, b.ordinal) && same(a.action_ref, b.action_ref) &&
           same(a.action_version_ref, b.action_version_ref) && same(a.rule_ref, b.rule_ref) &&
           same(a.target_ref, b.target_ref) && same(a.unit, b.unit) &&
           same(a.attempted_units, b.attempted_units) &&
           same(a.finalized_units, b.finalized_units) && same(a.explanation, b.explanation);
}
inline size_t bytes(const ApplicationHoldPlanEvidence &v) {
    return sum_bytes(v.kind, v.plan, v.extent, v.stopped_for, v.failed_capture, v.occurrences);
}
inline bool same(const ApplicationHoldPlanEvidence &a, const ApplicationHoldPlanEvidence &b) {
    return same(a.kind, b.kind) && same(a.plan, b.plan) && same(a.extent, b.extent) &&
           same(a.stopped_for, b.stopped_for) && same(a.failed_capture, b.failed_capture) &&
           same(a.occurrences, b.occurrences);
}
inline size_t bytes(const ApplicationHoldFailureEvidence &v) {
    return sum_bytes(v.stage, v.failing_ordinal, v.failing_target_ref, v.attempted_units,
                     v.annotation);
}
inline bool same(const ApplicationHoldFailureEvidence &a, const ApplicationHoldFailureEvidence &b) {
    return same(a.stage, b.stage) && same(a.failing_ordinal, b.failing_ordinal) &&
           same(a.failing_target_ref, b.failing_target_ref) &&
           same(a.attempted_units, b.attempted_units) && same(a.annotation, b.annotation);
}
inline size_t bytes(const ApplicationHoldAssociatedApplication &v) {
    return sum_bytes(v.operation, v.request_ordinal);
}
inline bool same(const ApplicationHoldAssociatedApplication &a,
                 const ApplicationHoldAssociatedApplication &b) {
    return same(a.operation, b.operation) && same(a.request_ordinal, b.request_ordinal);
}
inline size_t bytes(const ApplicationHoldTimerEvidence &v) {
    return sum_bytes(v.request_ordinal, v.component_kind, v.request_units, v.deficit_policy,
                     v.deficit_disposition, v.unapplied_units, v.composition,
                     v.composition_decision, v.associated);
}
inline bool same(const ApplicationHoldTimerEvidence &a, const ApplicationHoldTimerEvidence &b) {
    return same(a.request_ordinal, b.request_ordinal) && same(a.component_kind, b.component_kind) &&
           same(a.request_units, b.request_units) && same(a.deficit_policy, b.deficit_policy) &&
           same(a.deficit_disposition, b.deficit_disposition) &&
           same(a.unapplied_units, b.unapplied_units) && same(a.composition, b.composition) &&
           same(a.composition_decision, b.composition_decision) && same(a.associated, b.associated);
}
inline size_t bytes(const ApplicationHoldCause &v) {
    return sum_bytes(v.hold_ref, v.source, v.command, v.first_observation, v.plan_evidence, v.kind,
                     v.failure, v.timer, v.missing_policy_reason, v.supersedes_hold_ref);
}
inline bool same(const ApplicationHoldCause &a, const ApplicationHoldCause &b) {
    return same(a.hold_ref, b.hold_ref) && same(a.source, b.source) && same(a.command, b.command) &&
           same(a.first_observation, b.first_observation) &&
           same(a.plan_evidence, b.plan_evidence) && same(a.kind, b.kind) &&
           same(a.failure, b.failure) && same(a.timer, b.timer) &&
           same(a.missing_policy_reason, b.missing_policy_reason) &&
           same(a.supersedes_hold_ref, b.supersedes_hold_ref);
}
inline size_t bytes(const ApplicationHoldDetectionData &v) {
    return sum_bytes(v.mutation_ref, v.expected_version_ref, v.proposed_version_ref, v.observation,
                     v.audit);
}
inline bool same(const ApplicationHoldDetectionData &a, const ApplicationHoldDetectionData &b) {
    return same(a.mutation_ref, b.mutation_ref) &&
           same(a.expected_version_ref, b.expected_version_ref) &&
           same(a.proposed_version_ref, b.proposed_version_ref) &&
           same(a.observation, b.observation) && same(a.audit, b.audit);
}
inline size_t bytes(const ApplicationHoldDetection &v) { return sum_bytes(v.cause, v.request); }
inline bool same(const ApplicationHoldDetection &a, const ApplicationHoldDetection &b) {
    return same(a.cause, b.cause) && same(a.request, b.request);
}
inline size_t bytes(const ApplicationHoldVersionLocator &v) {
    return sum_bytes(v.hold_ref, v.version_ref);
}
inline bool same(const ApplicationHoldVersionLocator &a, const ApplicationHoldVersionLocator &b) {
    return same(a.hold_ref, b.hold_ref) && same(a.version_ref, b.version_ref);
}
inline size_t bytes(const ApplicationHoldReplacementLocator &v) {
    return sum_bytes(v.hold_ref, v.detection_ref);
}
inline bool same(const ApplicationHoldReplacementLocator &a,
                 const ApplicationHoldReplacementLocator &b) {
    return same(a.hold_ref, b.hold_ref) && same(a.detection_ref, b.detection_ref);
}
inline size_t bytes(const ApplicationHoldVersion &v) {
    return sum_bytes(v.locator, v.position, v.parent_version_ref, v.state, v.producer_mutation_ref,
                     v.replacement);
}
inline bool same(const ApplicationHoldVersion &a, const ApplicationHoldVersion &b) {
    return same(a.locator, b.locator) && same(a.position, b.position) &&
           same(a.parent_version_ref, b.parent_version_ref) && same(a.state, b.state) &&
           same(a.producer_mutation_ref, b.producer_mutation_ref) &&
           same(a.replacement, b.replacement);
}
inline size_t bytes(const ApplicationHoldHeldOrigin &v) {
    return sum_bytes(v.version, v.request_ordinal, v.component_kind, v.remaining_units);
}
inline bool same(const ApplicationHoldHeldOrigin &a, const ApplicationHoldHeldOrigin &b) {
    return same(a.version, b.version) && same(a.request_ordinal, b.request_ordinal) &&
           same(a.component_kind, b.component_kind) && same(a.remaining_units, b.remaining_units);
}
inline size_t bytes(const ApplicationHoldApplicationLink &v) {
    return sum_bytes(v.kind, v.operation, v.receipt_id, v.plan, v.held_origin);
}
inline bool same(const ApplicationHoldApplicationLink &a, const ApplicationHoldApplicationLink &b) {
    return same(a.kind, b.kind) && same(a.operation, b.operation) &&
           same(a.receipt_id, b.receipt_id) && same(a.plan, b.plan) &&
           same(a.held_origin, b.held_origin);
}
inline size_t bytes(const ApplicationHoldResolutionEntry &v) {
    return sum_bytes(v.hold_ref, v.expected_version_ref, v.proposed_version_ref, v.decision,
                     v.replacement);
}
inline bool same(const ApplicationHoldResolutionEntry &a, const ApplicationHoldResolutionEntry &b) {
    return same(a.hold_ref, b.hold_ref) && same(a.expected_version_ref, b.expected_version_ref) &&
           same(a.proposed_version_ref, b.proposed_version_ref) && same(a.decision, b.decision) &&
           same(a.replacement, b.replacement);
}
inline size_t bytes(const ApplicationHoldResolutionBatch &v) {
    return sum_bytes(v.mutation_ref, v.subject, v.expected_active, v.entries, v.actor_ref,
                     v.decision_ref, v.reason, v.options, v.application);
}
inline bool same(const ApplicationHoldResolutionBatch &a, const ApplicationHoldResolutionBatch &b) {
    return same(a.mutation_ref, b.mutation_ref) && same(a.subject, b.subject) &&
           same(a.expected_active, b.expected_active) && same(a.entries, b.entries) &&
           same(a.actor_ref, b.actor_ref) && same(a.decision_ref, b.decision_ref) &&
           same(a.reason, b.reason) && same(a.options, b.options) &&
           same(a.application, b.application);
}
inline size_t bytes(const ApplicationHoldRecordedResolution &v) {
    return sum_bytes(v.request, v.produced_versions);
}
inline bool same(const ApplicationHoldRecordedResolution &a,
                 const ApplicationHoldRecordedResolution &b) {
    return same(a.request, b.request) && same(a.produced_versions, b.produced_versions);
}
inline size_t bytes(const ApplicationHoldView &v) {
    return sum_bytes(v.cause, v.first_detection, v.first_version, v.current_version,
                     v.current_event);
}
inline bool same(const ApplicationHoldView &a, const ApplicationHoldView &b) {
    return same(a.cause, b.cause) && same(a.first_detection, b.first_detection) &&
           same(a.first_version, b.first_version) && same(a.current_version, b.current_version) &&
           same(a.current_event, b.current_event);
}
inline size_t bytes(const ApplicationHoldActiveSet &v) { return sum_bytes(v.subject, v.holds); }
inline bool same(const ApplicationHoldActiveSet &a, const ApplicationHoldActiveSet &b) {
    return same(a.subject, b.subject) && same(a.holds, b.holds);
}
inline size_t bytes(const ApplicationHoldHistoryEntry &v) { return sum_bytes(v.version, v.event); }
inline bool same(const ApplicationHoldHistoryEntry &a, const ApplicationHoldHistoryEntry &b) {
    return same(a.version, b.version) && same(a.event, b.event);
}
inline size_t bytes(const ApplicationHoldHistoryQuery &v) {
    return sum_bytes(v.hold_ref, v.after_position, v.checked_version_ref, v.page_count);
}
inline bool same(const ApplicationHoldHistoryQuery &a, const ApplicationHoldHistoryQuery &b) {
    return same(a.hold_ref, b.hold_ref) && same(a.after_position, b.after_position) &&
           same(a.checked_version_ref, b.checked_version_ref) && same(a.page_count, b.page_count);
}
inline size_t bytes(const ApplicationHoldHistoryPage &v) {
    return sum_bytes(v.hold_ref, v.checked_version_ref, v.entries, v.next_position, v.has_more);
}
inline bool same(const ApplicationHoldHistoryPage &a, const ApplicationHoldHistoryPage &b) {
    return same(a.hold_ref, b.hold_ref) && same(a.checked_version_ref, b.checked_version_ref) &&
           same(a.entries, b.entries) && same(a.next_position, b.next_position) &&
           same(a.has_more, b.has_more);
}
inline size_t bytes(const ApplicationHoldHistoryResult &v) {
    return sum_bytes(v.disposition, v.page);
}
inline bool same(const ApplicationHoldHistoryResult &a, const ApplicationHoldHistoryResult &b) {
    return same(a.disposition, b.disposition) && same(a.page, b.page);
}
inline size_t bytes(const ApplicationHoldMutationOutcome &v) {
    return sum_bytes(v.mutation_ref, v.kind, v.produced_versions);
}
inline bool same(const ApplicationHoldMutationOutcome &a, const ApplicationHoldMutationOutcome &b) {
    return same(a.mutation_ref, b.mutation_ref) && same(a.kind, b.kind) &&
           same(a.produced_versions, b.produced_versions);
}
inline size_t bytes(const ApplicationHoldMutationResult &v) {
    return sum_bytes(v.disposition, v.outcome);
}
inline bool same(const ApplicationHoldMutationResult &a, const ApplicationHoldMutationResult &b) {
    return same(a.disposition, b.disposition) && same(a.outcome, b.outcome);
}
inline size_t items(const ApplicationHoldPlanEvidence &v);
inline size_t items(const ApplicationHoldCause &v);
inline size_t items(const ApplicationHoldDetection &v);
inline size_t items(const ApplicationHoldResolutionEntry &v);
inline size_t items(const ApplicationHoldResolutionBatch &v);
inline size_t items(const ApplicationHoldRecordedResolution &v);
inline size_t items(const ApplicationHoldView &v);
inline size_t items(const ApplicationHoldActiveSet &v);
inline size_t items(const ApplicationHoldHistoryEntry &v);
inline size_t items(const ApplicationHoldHistoryPage &v);
inline size_t items(const ApplicationHoldHistoryResult &v);
inline size_t items(const ApplicationHoldMutationOutcome &v);
inline size_t items(const ApplicationHoldMutationResult &v);

template <class T> inline size_t items(const T &) { return 0; }
template <class T> inline size_t items(const std::optional<T> &v);
template <class T> inline size_t items(const std::vector<T> &v);
template <class... T> inline size_t items(const std::variant<T...> &v);
template <class... T> inline size_t sum_items(const T &...v) {
    Size n;
    (n.add(items(v)), ...);
    return n.value;
}
template <class T> inline size_t items(const std::optional<T> &v) { return v ? items(*v) : 0; }
template <class T> inline size_t items(const std::vector<T> &v) {
    Size n;
    n.add(v.size());
    for (const auto &x : v)
        n.add(items(x));
    return n.value;
}
template <class... T> inline size_t items(const std::variant<T...> &v) {
    return std::visit([](const auto &x) { return items(x); }, v);
}
inline size_t items(const ApplicationHoldPlanEvidence &v) { return sum_items(v.occurrences); }
inline size_t items(const ApplicationHoldCause &v) { return sum_items(v.plan_evidence); }
inline size_t items(const ApplicationHoldDetection &v) { return sum_items(v.cause); }
inline size_t items(const ApplicationHoldResolutionEntry &v) { return sum_items(v.replacement); }
inline size_t items(const ApplicationHoldResolutionBatch &v) {
    return sum_items(v.expected_active, v.entries);
}
inline size_t items(const ApplicationHoldRecordedResolution &v) {
    return sum_items(v.request, v.produced_versions);
}
inline size_t items(const ApplicationHoldView &v) { return sum_items(v.cause, v.current_event); }
inline size_t items(const ApplicationHoldActiveSet &v) { return sum_items(v.holds); }
inline size_t items(const ApplicationHoldHistoryEntry &v) { return sum_items(v.event); }
inline size_t items(const ApplicationHoldHistoryPage &v) { return sum_items(v.entries); }
inline size_t items(const ApplicationHoldHistoryResult &v) { return sum_items(v.page); }
inline size_t items(const ApplicationHoldMutationOutcome &v) {
    return sum_items(v.produced_versions);
}
inline size_t items(const ApplicationHoldMutationResult &v) { return sum_items(v.outcome); }

inline void occurrence_count(const ApplicationHoldCause &v, const SqliteHoldLimits &l) {
    if (v.plan_evidence.occurrences.size() > size_t(l.known_occurrences_per_cause))
        invalid(Input::collection_limit);
}
template <class T> inline void input_count(const T &v, const SqliteHoldLimits &l) {
    try {
        if (items(v) > size_t(l.input_items))
            invalid(Input::collection_limit);
    } catch (const std::length_error &) {
        invalid(Input::collection_limit);
    }
}
template <class T> inline void input_bytes(const T &v, const SqliteHoldLimits &l) {
    try {
        if (bytes(v) > size_t(l.owned_bytes))
            invalid(Input::owned_byte_limit);
    } catch (const std::length_error &) {
        invalid(Input::owned_byte_limit);
    }
}
inline void validate_find(const std::string &v, const SqliteHoldLimits &l) {
    validate_options(l);
    key(v, l);
    input_bytes(v, l);
}
inline void validate_active(const ApplicationHoldSubject &v, const SqliteHoldLimits &l) {
    validate_options(l);
    key_subject(v);
    input_bytes(v, l);
}
inline void validate_history(const ApplicationHoldHistoryQuery &v, const SqliteHoldLimits &l) {
    validate_options(l);
    key(v.hold_ref, l);
    key(v.checked_version_ref, l);
    if (v.page_count <= 0 || v.page_count > l.history_versions_per_page)
        invalid(Input::collection_limit);
    if (v.after_position < 0)
        invalid(Input::invalid_cursor);
    input_bytes(v, l);
}
inline void validate_detection(const ApplicationHoldDetection &v, const SqliteHoldLimits &l) {
    validate_options(l);
    key_detection(v, l);
    occurrence_count(v.cause, l);
    input_count(v, l);
    shape_detection(v, l);
    input_bytes(v, l);
}
inline void validate_resolution(const ApplicationHoldResolutionBatch &v,
                                const SqliteHoldLimits &l) {
    validate_options(l);
    key_resolution(v, l);
    if (v.expected_active.size() > size_t(l.active_holds_per_result) ||
        v.entries.size() > size_t(l.resolution_entries))
        invalid(Input::collection_limit);
    for (const auto &e : v.entries)
        if (e.replacement)
            occurrence_count(e.replacement->cause, l);
    input_count(v, l);
    shape_resolution(v, l);
    input_bytes(v, l);
}

// Exact approved physical step6. This pure factory performs no I/O or migration.
inline MigrationManifest make_manifest() {
    auto manifest = bundle_detail::make_manifest();
    MigrationStep step;
    step.version = 6;
    step.id = "durable-application-holds-v1";
    const std::array<QString, 12>
        statements{
            QString::fromUtf8(
                R"SQL(CREATE TABLE subathon_hold_mutations(mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), mutation_kind INTEGER NOT NULL CHECK(typeof(mutation_kind)='integer' AND mutation_kind BETWEEN 0 AND 1), subject_kind INTEGER NOT NULL CHECK(typeof(subject_kind)='integer' AND subject_kind BETWEEN 0 AND 1), subject_id BLOB NOT NULL CHECK(typeof(subject_id)='blob' AND length(subject_id)=8), expected_count INTEGER NOT NULL CHECK(typeof(expected_count)='integer' AND expected_count>=0), entry_count INTEGER NOT NULL CHECK(typeof(entry_count)='integer' AND entry_count>=1), actor_ref BLOB CHECK(actor_ref IS NULL OR (typeof(actor_ref)='blob' AND length(actor_ref)>0)), decision_ref BLOB CHECK(decision_ref IS NULL OR (typeof(decision_ref)='blob' AND length(decision_ref)>0)), reason BLOB CHECK(reason IS NULL OR (typeof(reason)='blob')), options_format_ref BLOB CHECK(options_format_ref IS NULL OR (typeof(options_format_ref)='blob' AND length(options_format_ref)>0)), options_document BLOB CHECK(options_document IS NULL OR (typeof(options_document)='blob')), application_link_kind INTEGER CHECK(application_link_kind IS NULL OR (typeof(application_link_kind)='integer' AND application_link_kind BETWEEN 0 AND 1)), application_scope BLOB CHECK(application_scope IS NULL OR (typeof(application_scope)='blob' AND length(application_scope)>0)), application_ref BLOB CHECK(application_ref IS NULL OR (typeof(application_ref)='blob' AND length(application_ref)>0)), application_receipt_id BLOB CHECK(application_receipt_id IS NULL OR (typeof(application_receipt_id)='blob' AND length(application_receipt_id)=8 AND application_receipt_id<>X'0000000000000000')), application_plan_ref BLOB CHECK(application_plan_ref IS NULL OR (typeof(application_plan_ref)='blob' AND length(application_plan_ref)>0)), application_plan_revision BLOB CHECK(application_plan_revision IS NULL OR (typeof(application_plan_revision)='blob' AND length(application_plan_revision)=8 AND application_plan_revision<>X'0000000000000000')), held_hold_ref BLOB CHECK(held_hold_ref IS NULL OR (typeof(held_hold_ref)='blob' AND length(held_hold_ref)>0)), held_version_ref BLOB CHECK(held_version_ref IS NULL OR (typeof(held_version_ref)='blob' AND length(held_version_ref)>0)), held_request_ordinal INTEGER CHECK(held_request_ordinal IS NULL OR (typeof(held_request_ordinal)='integer' AND held_request_ordinal>=1)), held_component_kind INTEGER CHECK(held_component_kind IS NULL OR (typeof(held_component_kind)='integer' AND held_component_kind=4)), remaining_units INTEGER CHECK(remaining_units IS NULL OR (typeof(remaining_units)='integer')), PRIMARY KEY(mutation_ref), CHECK(subject_kind=0 OR subject_id<>X'0000000000000000'), CHECK((actor_ref IS NULL AND decision_ref IS NULL AND reason IS NULL AND options_format_ref IS NULL AND options_document IS NULL) OR (actor_ref IS NOT NULL AND decision_ref IS NOT NULL AND reason IS NOT NULL AND options_format_ref IS NOT NULL AND options_document IS NOT NULL)), CHECK((application_link_kind IS NULL AND application_scope IS NULL AND application_ref IS NULL AND application_receipt_id IS NULL AND application_plan_ref IS NULL AND application_plan_revision IS NULL) OR (application_link_kind IS NOT NULL AND application_scope IS NOT NULL AND application_ref IS NOT NULL AND application_receipt_id IS NOT NULL AND application_plan_ref IS NOT NULL AND application_plan_revision IS NOT NULL)), CHECK((held_hold_ref IS NULL AND held_version_ref IS NULL AND held_request_ordinal IS NULL AND held_component_kind IS NULL) OR (held_hold_ref IS NOT NULL AND held_version_ref IS NOT NULL AND held_request_ordinal IS NOT NULL AND held_component_kind IS NOT NULL)), CHECK((mutation_kind=0 AND expected_count=0 AND entry_count=1 AND actor_ref IS NULL AND application_link_kind IS NULL AND held_hold_ref IS NULL AND remaining_units IS NULL) OR (mutation_kind=1 AND actor_ref IS NOT NULL)), CHECK((application_link_kind IS NULL AND held_hold_ref IS NULL AND remaining_units IS NULL) OR (application_link_kind IS NOT NULL AND ((application_link_kind=0 AND held_hold_ref IS NULL AND remaining_units IS NULL) OR (application_link_kind=1 AND held_hold_ref IS NOT NULL)))), FOREIGN KEY(application_scope,application_ref) REFERENCES subathon_app_operations(operation_scope,operation_ref), FOREIGN KEY(application_receipt_id) REFERENCES subathon_receipts(receipt_id), FOREIGN KEY(application_plan_ref,application_plan_revision) REFERENCES subathon_plan_revisions(plan_ref,revision), FOREIGN KEY(held_hold_ref,held_version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref), FOREIGN KEY(held_hold_ref,held_request_ordinal) REFERENCES subathon_hold_occurrences(hold_ref,ordinal)))SQL"),
            QString::fromUtf8(
                R"SQL(CREATE TABLE subathon_hold_causes(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), subject_kind INTEGER NOT NULL CHECK(typeof(subject_kind)='integer' AND subject_kind BETWEEN 0 AND 1), subject_id BLOB NOT NULL CHECK(typeof(subject_id)='blob' AND length(subject_id)=8), contribution_id BLOB CHECK(contribution_id IS NULL OR (typeof(contribution_id)='blob' AND length(contribution_id)=8)), source_receipt_id BLOB NOT NULL CHECK(typeof(source_receipt_id)='blob' AND length(source_receipt_id)=8 AND source_receipt_id<>X'0000000000000000'), event_ref BLOB CHECK(event_ref IS NULL OR (typeof(event_ref)='blob')), parent_event_ref BLOB CHECK(parent_event_ref IS NULL OR (typeof(parent_event_ref)='blob')), root_event_ref BLOB CHECK(root_event_ref IS NULL OR (typeof(root_event_ref)='blob')), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref)>0), arrival_context_ref BLOB NOT NULL CHECK(typeof(arrival_context_ref)='blob' AND length(arrival_context_ref)>0), command_scope BLOB NOT NULL CHECK(typeof(command_scope)='blob' AND length(command_scope)>0), command_ref BLOB NOT NULL CHECK(typeof(command_ref)='blob' AND length(command_ref)>0), command_format_ref BLOB NOT NULL CHECK(typeof(command_format_ref)='blob' AND length(command_format_ref)>0), command_document BLOB NOT NULL CHECK(typeof(command_document)='blob'), cause_kind INTEGER NOT NULL CHECK(typeof(cause_kind)='integer' AND cause_kind BETWEEN 0 AND 2), supersedes_hold_ref BLOB CHECK(supersedes_hold_ref IS NULL OR (typeof(supersedes_hold_ref)='blob' AND length(supersedes_hold_ref)>0)), first_detection_ref BLOB NOT NULL CHECK(typeof(first_detection_ref)='blob' AND length(first_detection_ref)>0), plan_evidence_kind INTEGER NOT NULL CHECK(typeof(plan_evidence_kind)='integer' AND plan_evidence_kind BETWEEN 0 AND 1), plan_ref BLOB CHECK(plan_ref IS NULL OR (typeof(plan_ref)='blob' AND length(plan_ref)>0)), plan_revision BLOB CHECK(plan_revision IS NULL OR (typeof(plan_revision)='blob' AND length(plan_revision)=8 AND plan_revision<>X'0000000000000000')), extent INTEGER NOT NULL CHECK(typeof(extent)='integer' AND extent BETWEEN 0 AND 2), stopped_for INTEGER CHECK(stopped_for IS NULL OR (typeof(stopped_for)='integer' AND stopped_for BETWEEN 0 AND 1)), failed_capture_format_ref BLOB CHECK(failed_capture_format_ref IS NULL OR (typeof(failed_capture_format_ref)='blob' AND length(failed_capture_format_ref)>0)), failed_capture_document BLOB CHECK(failed_capture_document IS NULL OR (typeof(failed_capture_document)='blob')), occurrence_count INTEGER NOT NULL CHECK(typeof(occurrence_count)='integer' AND occurrence_count>=0), failure_stage INTEGER CHECK(failure_stage IS NULL OR (typeof(failure_stage)='integer' AND failure_stage BETWEEN 0 AND 1)), failing_ordinal INTEGER CHECK(failing_ordinal IS NULL OR (typeof(failing_ordinal)='integer' AND failing_ordinal>=1)), failing_target_ref BLOB CHECK(failing_target_ref IS NULL OR (typeof(failing_target_ref)='blob')), failure_numerator BLOB CHECK(failure_numerator IS NULL OR (typeof(failure_numerator)='blob' AND length(failure_numerator)>0)), failure_denominator BLOB CHECK(failure_denominator IS NULL OR (typeof(failure_denominator)='blob' AND length(failure_denominator)>0)), failure_format_ref BLOB CHECK(failure_format_ref IS NULL OR (typeof(failure_format_ref)='blob' AND length(failure_format_ref)>0)), failure_document BLOB CHECK(failure_document IS NULL OR (typeof(failure_document)='blob')), timer_request_ordinal INTEGER CHECK(timer_request_ordinal IS NULL OR (typeof(timer_request_ordinal)='integer' AND timer_request_ordinal>=1)), timer_component_kind INTEGER CHECK(timer_component_kind IS NULL OR (typeof(timer_component_kind)='integer' AND timer_component_kind=4)), timer_request_units INTEGER CHECK(timer_request_units IS NULL OR (typeof(timer_request_units)='integer' AND timer_request_units<0)), deficit_policy INTEGER CHECK(deficit_policy IS NULL OR (typeof(deficit_policy)='integer' AND deficit_policy=2)), deficit_disposition INTEGER CHECK(deficit_disposition IS NULL OR (typeof(deficit_disposition)='integer' AND deficit_disposition=1)), unapplied_units INTEGER CHECK(unapplied_units IS NULL OR (typeof(unapplied_units)='integer' AND unapplied_units<0)), composition INTEGER CHECK(composition IS NULL OR (typeof(composition)='integer' AND composition BETWEEN 0 AND 1)), composition_format_ref BLOB CHECK(composition_format_ref IS NULL OR (typeof(composition_format_ref)='blob' AND length(composition_format_ref)>0)), composition_document BLOB CHECK(composition_document IS NULL OR (typeof(composition_document)='blob')), missing_policy_reason BLOB CHECK(missing_policy_reason IS NULL OR (typeof(missing_policy_reason)='blob')), associated_scope BLOB CHECK(associated_scope IS NULL OR (typeof(associated_scope)='blob' AND length(associated_scope)>0)), associated_ref BLOB CHECK(associated_ref IS NULL OR (typeof(associated_ref)='blob' AND length(associated_ref)>0)), associated_request_ordinal INTEGER CHECK(associated_request_ordinal IS NULL OR (typeof(associated_request_ordinal)='integer' AND associated_request_ordinal>=1)), PRIMARY KEY(hold_ref), CHECK(subject_kind=0 OR subject_id<>X'0000000000000000'), CHECK((subject_kind=0 AND contribution_id IS NOT NULL AND contribution_id=subject_id) OR (subject_kind=1 AND contribution_id IS NULL AND subject_id=source_receipt_id)), CHECK((plan_ref IS NULL AND plan_revision IS NULL) OR (plan_ref IS NOT NULL AND plan_revision IS NOT NULL)), CHECK((failed_capture_format_ref IS NULL AND failed_capture_document IS NULL) OR (failed_capture_format_ref IS NOT NULL AND failed_capture_document IS NOT NULL)), CHECK((failure_numerator IS NULL AND failure_denominator IS NULL) OR (failure_numerator IS NOT NULL AND failure_denominator IS NOT NULL)), CHECK((failure_stage IS NULL AND failure_format_ref IS NULL AND failure_document IS NULL) OR (failure_stage IS NOT NULL AND failure_format_ref IS NOT NULL AND failure_document IS NOT NULL)), CHECK((timer_request_ordinal IS NULL AND timer_component_kind IS NULL AND timer_request_units IS NULL AND deficit_policy IS NULL AND deficit_disposition IS NULL AND unapplied_units IS NULL AND composition IS NULL AND composition_format_ref IS NULL AND composition_document IS NULL) OR (timer_request_ordinal IS NOT NULL AND timer_component_kind IS NOT NULL AND timer_request_units IS NOT NULL AND deficit_policy IS NOT NULL AND deficit_disposition IS NOT NULL AND unapplied_units IS NOT NULL AND composition IS NOT NULL AND composition_format_ref IS NOT NULL AND composition_document IS NOT NULL)), CHECK((associated_scope IS NULL AND associated_ref IS NULL AND associated_request_ordinal IS NULL) OR (associated_scope IS NOT NULL AND associated_ref IS NOT NULL AND associated_request_ordinal IS NOT NULL)), CHECK(supersedes_hold_ref IS NULL OR supersedes_hold_ref<>hold_ref), CHECK((plan_evidence_kind=0 AND plan_ref IS NOT NULL AND extent=0 AND stopped_for IS NULL AND failed_capture_format_ref IS NULL) OR (plan_evidence_kind=1 AND extent IN (1,2) AND stopped_for IS NOT NULL AND failed_capture_format_ref IS NOT NULL)), CHECK((cause_kind=0 AND failure_stage IS NOT NULL AND timer_request_ordinal IS NULL AND missing_policy_reason IS NULL AND associated_scope IS NULL AND ((plan_evidence_kind=0 AND failure_stage=1) OR (plan_evidence_kind=1 AND stopped_for=0 AND failure_stage=0))) OR (cause_kind=1 AND plan_evidence_kind=0 AND failure_stage IS NULL AND failing_ordinal IS NULL AND failing_target_ref IS NULL AND failure_numerator IS NULL AND timer_request_ordinal IS NOT NULL AND unapplied_units=timer_request_units AND missing_policy_reason IS NULL AND ((composition=0 AND associated_scope IS NULL) OR (composition=1 AND associated_scope IS NOT NULL AND associated_request_ordinal=timer_request_ordinal))) OR (cause_kind=2 AND failure_stage IS NULL AND failing_ordinal IS NULL AND failing_target_ref IS NULL AND failure_numerator IS NULL AND timer_request_ordinal IS NULL AND missing_policy_reason IS NOT NULL AND associated_scope IS NULL AND (plan_evidence_kind=0 OR stopped_for=1))), FOREIGN KEY(contribution_id) REFERENCES subathon_contributions(contribution_id), FOREIGN KEY(source_receipt_id) REFERENCES subathon_receipts(receipt_id), FOREIGN KEY(configuration_ref) REFERENCES subathon_retained_configurations(reference), FOREIGN KEY(arrival_context_ref) REFERENCES subathon_retained_contexts(reference), FOREIGN KEY(supersedes_hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(first_detection_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN KEY(plan_ref,plan_revision) REFERENCES subathon_plan_revisions(plan_ref,revision), FOREIGN KEY(associated_scope,associated_ref,associated_request_ordinal) REFERENCES subathon_app_effects(operation_scope,operation_ref,request_ordinal)))SQL"),
            QString::fromUtf8(
                R"SQL(CREATE TABLE subathon_hold_cause_observations(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), persisted_state_token BLOB CHECK(persisted_state_token IS NULL OR (typeof(persisted_state_token)='blob' AND length(persisted_state_token)>0)), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), policy_revision_ref BLOB CHECK(policy_revision_ref IS NULL OR (typeof(policy_revision_ref)='blob' AND length(policy_revision_ref)>0)), permission_revision_ref BLOB CHECK(permission_revision_ref IS NULL OR (typeof(permission_revision_ref)='blob' AND length(permission_revision_ref)>0)), actor_ref BLOB CHECK(actor_ref IS NULL OR (typeof(actor_ref)='blob' AND length(actor_ref)>0)), decision_ref BLOB CHECK(decision_ref IS NULL OR (typeof(decision_ref)='blob' AND length(decision_ref)>0)), observed_utc_ms INTEGER CHECK(observed_utc_ms IS NULL OR (typeof(observed_utc_ms)='integer')), elapsed_ms INTEGER CHECK(elapsed_ms IS NULL OR (typeof(elapsed_ms)='integer' AND elapsed_ms>=0)), PRIMARY KEY(hold_ref), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(persisted_state_token) REFERENCES subathon_app_states(token)))SQL"),
            QString::
                fromUtf8(R"SQL(CREATE TABLE subathon_hold_occurrences(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>=1), action_ref BLOB CHECK(action_ref IS NULL OR (typeof(action_ref)='blob')), action_version_ref BLOB CHECK(action_version_ref IS NULL OR (typeof(action_version_ref)='blob')), rule_ref BLOB CHECK(rule_ref IS NULL OR (typeof(rule_ref)='blob')), target_ref BLOB CHECK(target_ref IS NULL OR (typeof(target_ref)='blob')), unit INTEGER CHECK(unit IS NULL OR (typeof(unit)='integer' AND unit BETWEEN 0 AND 2)), attempted_numerator BLOB CHECK(attempted_numerator IS NULL OR (typeof(attempted_numerator)='blob' AND length(attempted_numerator)>0)), attempted_denominator BLOB CHECK(attempted_denominator IS NULL OR (typeof(attempted_denominator)='blob' AND length(attempted_denominator)>0)), finalized_units INTEGER CHECK(finalized_units IS NULL OR (typeof(finalized_units)='integer')), explanation BLOB CHECK(explanation IS NULL OR (typeof(explanation)='blob')), PRIMARY KEY(hold_ref,ordinal), CHECK((attempted_numerator IS NULL AND attempted_denominator IS NULL) OR (attempted_numerator IS NOT NULL AND attempted_denominator IS NOT NULL)), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref)))SQL"),
            QString::fromUtf8(
                R"SQL(CREATE TABLE subathon_hold_detections(mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), expected_version_ref BLOB CHECK(expected_version_ref IS NULL OR (typeof(expected_version_ref)='blob' AND length(expected_version_ref)>0)), proposed_version_ref BLOB NOT NULL CHECK(typeof(proposed_version_ref)='blob' AND length(proposed_version_ref)>0), audit_format_ref BLOB NOT NULL CHECK(typeof(audit_format_ref)='blob' AND length(audit_format_ref)>0), audit_document BLOB NOT NULL CHECK(typeof(audit_document)='blob'), PRIMARY KEY(mutation_ref), FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref)))SQL"),
            QString::fromUtf8(
                R"SQL(CREATE TABLE subathon_hold_detection_observations(mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), persisted_state_token BLOB CHECK(persisted_state_token IS NULL OR (typeof(persisted_state_token)='blob' AND length(persisted_state_token)>0)), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), policy_revision_ref BLOB CHECK(policy_revision_ref IS NULL OR (typeof(policy_revision_ref)='blob' AND length(policy_revision_ref)>0)), permission_revision_ref BLOB CHECK(permission_revision_ref IS NULL OR (typeof(permission_revision_ref)='blob' AND length(permission_revision_ref)>0)), actor_ref BLOB CHECK(actor_ref IS NULL OR (typeof(actor_ref)='blob' AND length(actor_ref)>0)), decision_ref BLOB CHECK(decision_ref IS NULL OR (typeof(decision_ref)='blob' AND length(decision_ref)>0)), observed_utc_ms INTEGER CHECK(observed_utc_ms IS NULL OR (typeof(observed_utc_ms)='integer')), elapsed_ms INTEGER CHECK(elapsed_ms IS NULL OR (typeof(elapsed_ms)='integer' AND elapsed_ms>=0)), PRIMARY KEY(mutation_ref), FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_detections(mutation_ref), FOREIGN KEY(persisted_state_token) REFERENCES subathon_app_states(token)))SQL"),
            QString::fromUtf8(
                R"SQL(CREATE TABLE subathon_hold_versions(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND length(version_ref)>0), position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>=1), parent_version_ref BLOB CHECK(parent_version_ref IS NULL OR (typeof(parent_version_ref)='blob' AND length(parent_version_ref)>0)), state INTEGER NOT NULL CHECK(typeof(state)='integer' AND state BETWEEN 0 AND 1), mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), replacement_hold_ref BLOB CHECK(replacement_hold_ref IS NULL OR (typeof(replacement_hold_ref)='blob' AND length(replacement_hold_ref)>0)), replacement_detection_ref BLOB CHECK(replacement_detection_ref IS NULL OR (typeof(replacement_detection_ref)='blob' AND length(replacement_detection_ref)>0)), PRIMARY KEY(hold_ref,version_ref), UNIQUE(hold_ref,position), CHECK((replacement_hold_ref IS NULL AND replacement_detection_ref IS NULL) OR (replacement_hold_ref IS NOT NULL AND replacement_detection_ref IS NOT NULL)), CHECK((position=1 AND parent_version_ref IS NULL AND state=0) OR (position>1 AND parent_version_ref IS NOT NULL)), CHECK(state=1 OR replacement_hold_ref IS NULL), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(hold_ref,parent_version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref), FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN KEY(replacement_hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(replacement_detection_ref) REFERENCES subathon_hold_detections(mutation_ref)))SQL"),
            QString::fromUtf8(
                R"SQL(CREATE TABLE subathon_hold_heads(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND length(version_ref)>0), version_count INTEGER NOT NULL CHECK(typeof(version_count)='integer' AND version_count>=1), PRIMARY KEY(hold_ref), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(hold_ref,version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref)))SQL"),
            QString::fromUtf8(
                R"SQL(CREATE TABLE subathon_hold_resolution_expected(mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND length(version_ref)>0), PRIMARY KEY(mutation_ref,hold_ref), FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN KEY(hold_ref,version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref)))SQL"),
            QString::fromUtf8(
                R"SQL(CREATE TABLE subathon_hold_resolution_entries(mutation_ref BLOB NOT NULL CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), expected_version_ref BLOB NOT NULL CHECK(typeof(expected_version_ref)='blob' AND length(expected_version_ref)>0), proposed_version_ref BLOB NOT NULL CHECK(typeof(proposed_version_ref)='blob' AND length(proposed_version_ref)>0), decision_format_ref BLOB NOT NULL CHECK(typeof(decision_format_ref)='blob' AND length(decision_format_ref)>0), decision_document BLOB NOT NULL CHECK(typeof(decision_document)='blob'), replacement_hold_ref BLOB CHECK(replacement_hold_ref IS NULL OR (typeof(replacement_hold_ref)='blob' AND length(replacement_hold_ref)>0)), replacement_detection_ref BLOB CHECK(replacement_detection_ref IS NULL OR (typeof(replacement_detection_ref)='blob' AND length(replacement_detection_ref)>0)), PRIMARY KEY(mutation_ref,hold_ref), CHECK((replacement_hold_ref IS NULL AND replacement_detection_ref IS NULL) OR (replacement_hold_ref IS NOT NULL AND replacement_detection_ref IS NOT NULL)), FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN KEY(hold_ref,expected_version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref), FOREIGN KEY(hold_ref,proposed_version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref), FOREIGN KEY(replacement_hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(replacement_detection_ref) REFERENCES subathon_hold_detections(mutation_ref)))SQL"),
            QString::fromUtf8(
                R"SQL(CREATE INDEX subathon_hold_subject_idx ON subathon_hold_causes(subject_kind,subject_id,hold_ref))SQL"),
            QString::fromUtf8(
                R"SQL(CREATE INDEX subathon_hold_producer_idx ON subathon_hold_versions(mutation_ref,hold_ref,position))SQL"),
    };
    step.expected_schema = manifest.steps.back().expected_schema;
    for (size_t i = 0; i < statements.size(); ++i) {
        step.sql.append(statements[i]);
        if (i < 10) {
            const auto table = statements[i].section(' ', 2, 2).section('(', 0, 0);
            step.expected_schema.append({"table", table, table, statements[i]});
            const int count = i == 6 ? 2 : 1;
            for (int n = 1; n <= count; ++n)
                step.expected_schema.append({"index",
                                             "sqlite_autoindex_" + table + "_" + QString::number(n),
                                             table, std::nullopt});
        } else {
            const auto name = i == 10 ? QString("subathon_hold_subject_idx")
                                      : QString("subathon_hold_producer_idx");
            const auto table =
                i == 10 ? QString("subathon_hold_causes") : QString("subathon_hold_versions");
            step.expected_schema.append({"index", name, table, statements[i]});
        }
    }
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return a.type < b.type || (a.type == b.type && a.name < b.name);
              });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions = {0, 1, 2, 3, 4, 5, 6};
    return manifest;
}

} // namespace subathon::server::hold_detail
