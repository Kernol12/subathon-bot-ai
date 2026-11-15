// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "reference_enum_internal.hpp"
#include "reference_validation_internal.hpp"
#include "sqlite_application_bundle_values.hpp"
#include "sqlite_application_hold_values.hpp"
#include <subathon/server/reference_intake.hpp>
namespace subathon::server::reference_runtime_detail {
using namespace reference_detail;
using reference_detail::fields;
#define RF(T, ...)                                                                                 \
    template <class P> void fields(P &p, const T &v) {                                             \
        (void)v;                                                                                   \
        p(__VA_ARGS__);                                                                            \
    }
RF(core::TimerState, v.lifecycle, v.phase, v.main_remaining_ms, v.grace_remaining_ms,
   v.awaiting_confirmation)
RF(core::TimerClockStep, v.elapsed_ms, v.utc_now_ms, v.paused)
RF(ApplicationEnvelope, v.format_ref, v.document)
RF(ApplicationBalance, v.kind, v.target_ref, v.units)
RF(ApplicationStateSnapshot, v.token, v.envelope, v.configuration_ref, v.timer, v.time_debt_ms,
   v.balances)
RF(ApplicationClockObservation, v.clock_lineage_ref, v.observation_ref, v.configuration_ref,
   v.policy_revision_ref, v.checked_head_token, v.observed_utc_ms, v.segments)
RF(ReferenceCommittedState, v.snapshot, v.basis, v.observation, v.policy_revision_ref)
RF(ReferenceClockSample, v.monotonic_ms, v.utc_ms)
RF(ReferenceClockAnchor, v.committed, v.sample)
RF(OriginalApplicationCommand, v.operation, v.subject_kind, v.receipt_id.value, v.contribution_id,
   v.plan, v.principal, v.purpose, v.preview_mode, v.preview_state_token, v.actor_ref,
   v.decision_ref, v.reason, v.choices)
RF(core::ReceiptId, v.value)
RF(core::ContributionId, v.value)
RF(ApplicationOriginalClaimSelector, v.subject_kind, v.receipt_id.value, v.contribution_id)
RF(ApplicationReferenceBinding, v.operation, v.source, v.plan, v.persisted_head_token, v.purpose,
   v.request_ordinal)
RF(IssuedApplicationReference, v.binding, v.reference)
RF(ApplicationUnavailable, v.reason_ref)
RF(ApplicationAccessAllowed, v.permission_revision_ref)
RF(ApplicationAccessDenied)
RF(ApplicationAccessUnknown)
RF(ApplicationPolicyAdmissionDecision, v.disposition, v.reason_ref)
RF(ApplicationSignedCorrectionPolicy, v.request_ordinal, v.deficit_policy)
RF(BoundApplicationPolicy, v.operation, v.source, v.plan, v.checked_state_token,
   v.configuration_ref, v.permission_revision_ref, v.policy_revision_ref, v.maximum_main_ms,
   v.timer_config, v.paused, v.recovery_pending, v.pause_recovery_revision, v.explanation,
   v.admission, v.correction_policies)
RF(ApplicationComponent, v.kind, v.units, v.target_ref)
RF(ApplicationEffect, v.request_ordinal, v.arithmetic_role, v.unit, v.exact_requested_units,
   v.requested_units, v.rounding_residual, v.maximum_main_ms, v.deficit_policy,
   v.deficit_disposition, v.application_explanation, v.components)
RF(ApplicationCalculatedProposal, v.clock_advanced_timer, v.timer_after, v.debt_after,
   v.balances_after, v.effects, v.transitions, v.observation)
RF(ApplicationJobIntent, v.job_ref, v.source_request_ordinal, v.group_ref, v.group_ordinal,
   v.envelope)
RF(PreparedApplicationActions, v.jobs, v.explanation)
RF(ReferenceSnapshot, v.committed, v.maximum_main_ms)
RF(SessionLookupRequest, v.installation_ref, v.digest)
RF(ReferenceInitialRequest, v.owner_session, v.configuration_ref, v.state_token,
   v.clock_lineage_ref, v.observation_ref, v.timer)
RF(ReferenceConnectorRegistration, v.connection_ref, v.origin, v.identifier_namespace)
RF(core::ReceiptMoney, v.original_decimal, v.currency)
RF(core::ReceiptLinkReference, v.identifier_namespace, v.opaque_id)
RF(core::ReceiptFacts, v.event_type, v.schema_version, v.source_timestamp_utc_ms, v.actor_id,
   v.actor_display_name, v.money, v.payment_reference, v.activity_reference)
RF(core::ReceiptArrival, v.server_timestamp_utc_ms, v.connection_ref, v.configuration_ref,
   v.modifier_context_ref)
RF(core::EvidenceReviewDecision, v.choice, v.decision_ref, v.actor_ref)
RF(core::EvidenceAdmission, v.policy, v.policy_ref, v.review_decision)
RF(core::ContributionValuation, v.monetary, v.original_money, v.resolution_ref, v.missing_rate)
RF(core::ContributionConditions, v.valuation, v.review, v.evidence_admission, v.application,
   v.reversal, v.settlement)
RF(core::ContributionState, v.payment_evidence, v.conditions)
RF(core::PlanEvent, v.event_ref, v.parent_event_ref, v.root_event_ref, v.receipt_id, v.identity,
   v.facts, v.arrival, v.contribution_id)
RF(core::HistoricalValuation, v.resolution_ref, v.original_money, v.comparison_currency,
   v.effective_rate_decimal, v.fetched_rate_decimal, v.snapshot_ref, v.source_ref,
   v.rate_timestamp_utc_ms, v.fetch_timestamp_utc_ms, v.manual_decision_ref)
RF(core::PlanValuation, v.state, v.historical)
RF(core::RuleSelection, v.rule_ref, v.version_ref, v.selected, v.explanation)
RF(core::ModifierSelection, v.definition_version_ref, v.instance_revision_ref, v.target_ref,
   v.group_ref, v.group_operator, v.target_operator, v.original_factor_decimal)
RF(core::RequestedEffect, v.action_ref, v.action_version_ref, v.rule_ref, v.target_ref, v.unit,
   v.requires_historical_valuation, v.exact_requested_units, v.requested_units,
   v.calculation_explanation, v.reversal_behavior_ref)
RF(core::PlanCapture, v.event, v.contribution_state, v.valuation, v.requested_effects)
RF(core::PlanOverrideDecision, v.authority, v.actor_ref, v.reason, v.decision_ref)
RF(core::CapturedPlanRevision, v.revision, v.parent_revision, v.captured, v.rules, v.modifiers,
   v.override_decision)
RF(VerifiedActivityIdentity, v.origin, v.identifier_namespace, v.opaque_id)
RF(VerifiedActivityLink, v.identity, v.verification_ref)
RF(ContributionReceipt, v.id, v.identity, v.facts, v.arrival)
RF(ContributionObservation, v.receipt, v.kind, v.evidence, v.verified_activity,
   v.support_conditions)
RF(ContributionEvidenceRecord, v.observation, v.contribution_id)
RF(ContributionRecord, v.id, v.activity_identity, v.original_receipt_id, v.original_facts,
   v.original_arrival, v.state)
RF(ApplicationContributionInput, v.contribution, v.support_anchor)
RF(ApplicationBundleInputs, v.subject_kind, v.plan, v.actual_plan_head, v.selected_plan,
   v.current_state, v.current_contribution)
RF(ApplicationContributionSnapshot, v.contribution_id, v.before, v.application_after)
RF(ApplicationIntent, v.operation, v.kind, v.subject_kind, v.receipt_id, v.contribution_id, v.plan,
   v.expected_state_token, v.expected_contribution_state, v.choices, v.reversal_ref)
RF(ApplicationAudit, v.actor_ref, v.decision_ref, v.reason, v.envelope)
RF(ApplicationOriginalComponent, v.operation, v.request_ordinal, v.component_kind)
RF(ApplicationReversalOccurrence, v.request_ordinal, v.original, v.basis_position,
   v.previous_cumulative, v.cumulative, v.requested_increment, v.cumulative_fraction,
   v.exact_entitlement, v.rounding_residual, v.full_refund)
RF(RecordedApplicationBundle, v.intent, v.persisted_head, v.advanced_before, v.after,
   v.advance_explanation, v.contribution, v.effects, v.jobs, v.transitions, v.reversals, v.result,
   v.audit)
RF(ApplicationHoldSubject, v.id)
RF(ApplicationHoldSource, v.subject, v.receipt_id, v.event_ref, v.parent_event_ref,
   v.root_event_ref, v.configuration_ref, v.arrival_context_ref)
RF(ApplicationHoldCommand, v.operation, v.intent)
RF(ApplicationHoldObservation, v.persisted_state_token, v.annotation, v.policy_revision_ref,
   v.permission_revision_ref, v.actor_ref, v.decision_ref, v.observed_utc_ms, v.elapsed_ms)
RF(ApplicationHoldProposedOccurrence, v.ordinal, v.action_ref, v.action_version_ref, v.rule_ref,
   v.target_ref, v.unit, v.attempted_units, v.finalized_units, v.explanation)
RF(ApplicationHoldPlanEvidence, v.kind, v.plan, v.extent, v.stopped_for, v.failed_capture,
   v.occurrences)
RF(ApplicationHoldFailureEvidence, v.stage, v.failing_ordinal, v.failing_target_ref,
   v.attempted_units, v.annotation)
RF(ApplicationHoldAssociatedApplication, v.operation, v.request_ordinal)
RF(ApplicationHoldTimerEvidence, v.request_ordinal, v.component_kind, v.request_units,
   v.deficit_policy, v.deficit_disposition, v.unapplied_units, v.composition,
   v.composition_decision, v.associated)
RF(ApplicationHoldCause, v.hold_ref, v.source, v.command, v.first_observation, v.plan_evidence,
   v.kind, v.failure, v.timer, v.missing_policy_reason, v.supersedes_hold_ref)
RF(ApplicationHoldDetectionData, v.mutation_ref, v.expected_version_ref, v.proposed_version_ref,
   v.observation, v.audit)
RF(ApplicationHoldDetection, v.cause, v.request)
RF(ApplicationHoldVersionLocator, v.hold_ref, v.version_ref)
RF(ApplicationHoldReplacementLocator, v.hold_ref, v.detection_ref)
RF(ApplicationHoldVersion, v.locator, v.position, v.parent_version_ref, v.state,
   v.producer_mutation_ref, v.replacement)
RF(ApplicationHoldHeldOrigin, v.version, v.request_ordinal, v.component_kind, v.remaining_units)
RF(ApplicationHoldApplicationLink, v.kind, v.operation, v.receipt_id, v.plan, v.held_origin)
RF(ApplicationHoldResolutionEntry, v.hold_ref, v.expected_version_ref, v.proposed_version_ref,
   v.decision, v.replacement)
RF(ApplicationHoldResolutionBatch, v.mutation_ref, v.subject, v.expected_active, v.entries,
   v.actor_ref, v.decision_ref, v.reason, v.options, v.application)
RF(ApplicationHoldRecordedResolution, v.request, v.produced_versions)
RF(ApplicationHoldView, v.cause, v.first_detection, v.first_version, v.current_version,
   v.current_event)
RF(ApplicationHoldActiveSet, v.subject, v.holds)
RF(ApplicationServiceFailure, v.source, v.reason)
RF(ApplicationActiveHoldPayload, v.holds)
RF(ApplicationHoldMutationOutcome, v.mutation_ref, v.kind, v.produced_versions)
RF(ApplicationHeldPayload, v.outcome, v.hold_ref)
RF(ApplicationNotReadyPayload, v.reason_ref)
RF(std::monostate)
RF(ApplicationServiceResponse, v.disposition, v.payload)
RF(ApplicationServiceCompletion, v.transaction, v.work_error, v.failure, v.response)
RF(core::ReceiptIntakeResult, v.receipt_id.value, v.disposition)
RF(ContributionLinkResult, v.contribution_id, v.disposition)
RF(ReferenceCaptureKey, v.installation_ref, v.source)
RF(ReferenceInitialCapture, v.key, v.receipt_id.value, v.contribution_id.value, v.configuration_ref,
   v.arrival_context_ref, v.event_ref, v.plan, v.execution_context_ref)
#undef RF
// Preserve allocator exceptions raised by a borrowed dependency; only our own
// allocations acquire the adapter's allocation_failure tag.
struct DependencyAllocation {
    std::exception_ptr original;
};
template <class F> decltype(auto) borrow(F fn) {
    try {
        return fn();
    } catch (const std::bad_alloc &) {
        throw DependencyAllocation{std::current_exception()};
    } catch (const std::length_error &) {
        throw DependencyAllocation{std::current_exception()};
    }
}
class BorrowedConfiguration final : public ReferenceConfigurationSource {
    ReferenceConfigurationSource &source_;

  public:
    explicit BorrowedConfiguration(ReferenceConfigurationSource &s) : source_(s) {}
    ReferenceConfigurationResolution snapshot(const std::string &r) override {
        return borrow([&] { return source_.snapshot(r); });
    }
    ReferenceConfigurationResolution load(const ApplicationReferenceReadView &v,
                                          const std::string &r) override {
        return borrow([&] { return source_.load(v, r); });
    }
};
// One allocation-free whole-tree tag pass, including losing imported fields.
struct Tags {
    template <class T> void one(const T &v) {
        if constexpr (std::is_enum_v<T>)
            require(unsigned(v) <= tag_max<T>(), Error::invalid_tag);
        else if constexpr (!std::is_integral_v<T>)
            fields(*this, v);
    }
    void one(const std::string &) {}
    void one(const QByteArray &) {}
    void one(const core::Exact &) {}
    void one(const std::monostate &) {}
    template <class T> void one(const std::optional<T> &v) {
        if (v)
            one(*v);
    }
    template <class T> void one(const std::vector<T> &v) {
        for (const auto &x : v)
            one(x);
    }
    template <class T, std::size_t N> void one(const std::array<T, N> &v) {
        for (const auto &x : v)
            one(x);
    }
    template <class... T> void one(const std::variant<T...> &v) {
        std::visit([&](const auto &x) { one(x); }, v);
    }
    template <class... T> void operator()(const T &...v) { (one(v), ...); }
};
// Imported service optionals charge no presence. New wrappers and R6 facts keep
// their own presence accounting; never impose the wrapper rule recursively.
struct Measure {
    Projection p;
    std::uint64_t document;
    std::uint64_t bundle_document = UINT64_MAX, hold_document = UINT64_MAX;
    std::uint64_t exact_component = UINT64_MAX;
    template <class T> void one(const T &v) {
        if constexpr (std::is_integral_v<T> || std::is_enum_v<T>)
            p.one(v);
        else
            fields(*this, v);
    }
    void one(const std::string &s) { p(s); }
    void one(std::string_view s) { p(s); }
    void one(const QByteArray &s) {
        require(std::uint64_t(s.size()) <= document, p.error);
        p.fixed(std::uint64_t(s.size()));
    }
    // Imported text uses its owning DATA role; reference fields still use p.
    // Service optionals retain absent0/present bytes and no item charge.
    void document_text(std::string_view text) {
        require(text.size() <= document && text.size() <= UINT32_MAX &&
                    text.size() < std::string{}.max_size(),
                p.error);
        p.fixed(text.size());
    }
    void document_text(const std::string &text) { document_text(std::string_view(text)); }
    void document_text(const std::optional<std::string> &text) {
        if (text)
            document_text(std::string_view(*text));
    }
    struct DocumentLimit {
        std::uint64_t &current;
        std::uint64_t previous;
        DocumentLimit(std::uint64_t &value, std::uint64_t cap) : current(value), previous(value) {
            current = std::min(value, cap);
        }
        ~DocumentLimit() { current = previous; }
    };
    void one(const OriginalApplicationCommand &v) {
        (*this)(v.operation, v.subject_kind, v.receipt_id.value, v.contribution_id, v.plan,
                v.principal, v.purpose, v.preview_mode, v.preview_state_token, v.actor_ref,
                v.decision_ref);
        document_text(v.reason);
        one(v.choices);
    }
    void one(const ApplicationAudit &v) {
        DocumentLimit bound(document, bundle_document);
        (*this)(v.actor_ref, v.decision_ref);
        document_text(v.reason);
        (*this)(v.envelope);
    }
    void one(const ApplicationBundleInputs &v) {
        DocumentLimit bound(document, bundle_document);
        fields(*this, v);
    }
    void one(const ApplicationContributionSnapshot &v) {
        DocumentLimit bound(document, bundle_document);
        fields(*this, v);
    }
    void one(const ApplicationEffect &v) {
        DocumentLimit bound(document, bundle_document);
        (*this)(v.request_ordinal, v.arithmetic_role, v.unit, v.exact_requested_units,
                v.requested_units, v.rounding_residual, v.maximum_main_ms, v.deficit_policy,
                v.deficit_disposition);
        document_text(v.application_explanation);
        (*this)(v.components);
    }
    void one(const ApplicationHoldActiveSet &v) {
        DocumentLimit bound(document, hold_document);
        fields(*this, v);
    }
    void one(const ApplicationHoldCause &v) {
        DocumentLimit bound(document, hold_document);
        (*this)(v.hold_ref, v.source, v.command, v.first_observation, v.plan_evidence, v.kind,
                v.failure, v.timer);
        document_text(v.missing_policy_reason);
        (*this)(v.supersedes_hold_ref);
    }
    void one(const ApplicationHoldDetection &v) {
        DocumentLimit bound(document, hold_document);
        fields(*this, v);
    }
    void one(const ApplicationHoldDetectionData &v) {
        DocumentLimit bound(document, hold_document);
        fields(*this, v);
    }
    void one(const ApplicationHoldObservation &v) {
        DocumentLimit bound(document, hold_document);
        fields(*this, v);
    }
    void one(const ApplicationHoldPlanEvidence &v) {
        DocumentLimit bound(document, hold_document);
        fields(*this, v);
    }
    void one(const ApplicationHoldProposedOccurrence &v) {
        DocumentLimit bound(document, hold_document);
        (*this)(v.ordinal, v.action_ref, v.action_version_ref, v.rule_ref, v.target_ref, v.unit,
                v.attempted_units, v.finalized_units);
        document_text(v.explanation);
    }
    void one(const ApplicationHoldRecordedResolution &v) {
        DocumentLimit bound(document, hold_document);
        fields(*this, v);
    }
    void one(const ApplicationHoldResolutionBatch &v) {
        DocumentLimit bound(document, hold_document);
        (*this)(v.mutation_ref, v.subject, v.expected_active, v.entries, v.actor_ref,
                v.decision_ref);
        document_text(v.reason);
        (*this)(v.options, v.application);
    }
    void one(const ApplicationHoldTimerEvidence &v) {
        DocumentLimit bound(document, hold_document);
        fields(*this, v);
    }
    void one(const ApplicationHoldView &v) {
        DocumentLimit bound(document, hold_document);
        fields(*this, v);
    }
    void one(const ApplicationJobIntent &v) {
        DocumentLimit bound(document, bundle_document);
        fields(*this, v);
    }
    void one(const ApplicationStateSnapshot &v) {
        DocumentLimit bound(document, bundle_document);
        fields(*this, v);
    }
    void one(const BoundApplicationPolicy &v) {
        (*this)(v.operation, v.source, v.plan, v.checked_state_token, v.configuration_ref,
                v.permission_revision_ref, v.policy_revision_ref, v.maximum_main_ms, v.timer_config,
                v.paused, v.recovery_pending, v.pause_recovery_revision);
        document_text(v.explanation);
        (*this)(v.admission, v.correction_policies);
    }
    void one(const PreparedApplicationActions &v) {
        (*this)(v.jobs);
        document_text(v.explanation);
    }
    void one(const RecordedApplicationBundle &v) {
        DocumentLimit bound(document, bundle_document);
        (*this)(v.intent, v.persisted_head, v.advanced_before, v.after);
        document_text(v.advance_explanation);
        (*this)(v.contribution, v.effects, v.jobs, v.transitions, v.reversals, v.result, v.audit);
    }
    void one(const core::HistoricalValuation &v) {
        (*this)(v.resolution_ref, v.original_money);
        document_text(v.comparison_currency);
        document_text(v.effective_rate_decimal);
        document_text(v.fetched_rate_decimal);
        (*this)(v.snapshot_ref, v.source_ref, v.rate_timestamp_utc_ms, v.fetch_timestamp_utc_ms,
                v.manual_decision_ref);
    }
    void one(const core::ModifierSelection &v) {
        (*this)(v.definition_version_ref, v.instance_revision_ref, v.target_ref, v.group_ref,
                v.group_operator, v.target_operator);
        document_text(v.original_factor_decimal);
    }
    void one(const core::PlanOverrideDecision &v) {
        (*this)(v.authority, v.actor_ref);
        document_text(v.reason);
        (*this)(v.decision_ref);
    }
    void one(const core::ReceiptFacts &v) {
        document_text(v.event_type);
        (*this)(v.schema_version, v.source_timestamp_utc_ms);
        document_text(v.actor_id);
        document_text(v.actor_display_name);
        (*this)(v.money, v.payment_reference, v.activity_reference);
    }
    void one(const core::ReceiptMoney &v) {
        document_text(v.original_decimal);
        document_text(v.currency);
    }
    void one(const core::RequestedEffect &v) {
        (*this)(v.action_ref, v.action_version_ref, v.rule_ref, v.target_ref, v.unit,
                v.requires_historical_valuation, v.exact_requested_units, v.requested_units);
        document_text(v.calculation_explanation);
        (*this)(v.reversal_behavior_ref);
    }
    void one(const core::RuleSelection &v) {
        (*this)(v.rule_ref, v.version_ref, v.selected);
        document_text(v.explanation);
    }
    // ACCESS-011 and graph DTOs keep their owning structural optional charges.
    void one(const StoredPermissionGraph &v) { p(v); }
    void one(const core::EffectivePermissionInput &v) { p(v); }
    void one(const core::PermissionAutomationInput &v) { p(v); }
    void one(const ReferenceSyntheticInput &v) { p(v); }
    void one(const ReferenceArrivalCapture &v) { p(v); }
    void one(const ReferenceConfiguration &v) { p(v); }
    void one(const ReferenceConfigurationFacts &v) { p(v); }
    void one(const ReferenceCommittedState &v) {
        one(v.snapshot);
        p.fixed(1);
        if (v.basis)
            one(*v.basis);
        one(v.observation);
        one(v.policy_revision_ref);
    }
    void one(const ReferenceSnapshot &v) {
        one(v.committed);
        p.fixed(1);
        if (v.maximum_main_ms)
            one(*v.maximum_main_ms);
    }
    template <class T> void one(const std::optional<T> &v) {
        if (v)
            one(*v);
    }
    template <class T, std::size_t N> void one(const std::array<T, N> &v) {
        for (auto x : v)
            one(x);
    }
    template <class T> void one(const std::vector<T> &v) {
        require(v.size() <= p.item_cap - p.items, p.error);
        p.items += v.size();
        for (const auto &x : v)
            one(x);
    }
    template <class... T> void one(const std::variant<T...> &v) {
        require(!v.valueless_by_exception(), Error::invalid_shape);
        std::visit([&](const auto &x) { one(x); }, v);
    }
    void one(const std::monostate &) {}
    void one(const core::Exact &v) {
        // Borrow Boost's backends: its free numerator/denominator accessors copy.
        auto digits = [&](const auto &x) -> std::uint64_t {
            using Limb = std::remove_cvref_t<decltype(*x.limbs())>;
            constexpr auto width = std::uint64_t(std::numeric_limits<Limb>::digits);
            static_assert(width == 32 || width == 64);
            const auto limbs = std::uint64_t(x.size());
            require(limbs <= UINT64_MAX / width && p.bytes <= p.cap, p.error);
            auto bits = limbs ? (limbs - 1) * width : 0;
            if (limbs) {
                auto top = x.limbs()[limbs - 1];
                while (top) {
                    ++bits;
                    top >>= 1;
                }
            }
            const auto cap =
                std::min({p.component_bytes, exact_component, document, p.cap - p.bytes});
            const auto sign = std::uint64_t(x.sign());
            if (bits <= 64) {
                std::uint64_t magnitude = 0;
                for (std::uint64_t i = 0; i < limbs; ++i) {
                    if constexpr (width == 32)
                        magnitude |= std::uint64_t(x.limbs()[i]) << (i * width);
                    else
                        magnitude = std::uint64_t(x.limbs()[i]);
                }
                std::uint64_t count = 1 + sign;
                while (magnitude >= 10) {
                    magnitude /= 10;
                    ++count;
                }
                require(count <= cap, p.error);
                return count;
            }
            // log10(2) > 3/10: reject clearly oversized components from metadata.
            // The remaining base-2^32 scratch is independently bounded BEFORE
            // allocation. Count decimal chunks in place; no integer/text clone.
            require(bits - 1 <= UINT64_MAX / 3, p.error);
            require((bits - 1) * 3 / 10 + 1 + sign <= cap, p.error);
            const auto count = bits / 32 + std::uint64_t(bits % 32 != 0);
            require(count < std::vector<std::uint32_t>{}.max_size() &&
                        count <= cap / sizeof(std::uint32_t),
                    p.error);
            std::vector<std::uint32_t> words(static_cast<std::size_t>(count));
            for (std::size_t i = 0; i < words.size(); ++i) {
                const auto offset = std::uint64_t(i) * 32;
                words[i] =
                    static_cast<std::uint32_t>(x.limbs()[offset / width] >> (offset % width));
            }
            std::uint64_t result = sign;
            while (!words.empty()) {
                std::uint64_t remainder = 0;
                for (auto i = words.size(); i-- > 0;) {
                    const auto value = (remainder << 32) | words[i];
                    words[i] = static_cast<std::uint32_t>(value / 1000000000);
                    remainder = value % 1000000000;
                }
                while (!words.empty() && words.back() == 0)
                    words.pop_back();
                std::uint64_t chunk = 9;
                if (words.empty()) {
                    chunk = 1;
                    while (remainder >= 10) {
                        remainder /= 10;
                        ++chunk;
                    }
                }
                require(chunk <= cap - result, p.error);
                result += chunk;
            }
            return result;
        };
        p.fixed(digits(v.backend().num()));
        p.fixed(digits(v.backend().denom()));
    }
    void component(const std::string &s) { p.component(s); }
    template <class... T> void operator()(const T &...v) { (one(v), ...); }
};
inline Measure measure(const ReferenceAdapterLimits &l, Error e = Error::input_limit) {
    return {projection(l, e), l.document_bytes};
}
inline void thread(std::thread::id id) {
    if (id != std::this_thread::get_id())
        throw SqliteWorkFailure(WorkError::wrong_thread);
}
inline void runtime_limits(const ReferenceRuntimeLimits &l) {
    valid_limits(l.values);
    reference_authority::valid_limits(l.authority);
    try {
        bundle_detail::validate_options(l.service.bundles);
        hold_detail::validate_options(l.service.holds);
    } catch (const SqliteBundleInputFailure &) {
        fail(Error::invalid_limits);
    } catch (const SqliteHoldInputFailure &) {
        fail(Error::invalid_limits);
    }
    const auto max =
        std::min({std::uint64_t(SIZE_MAX), std::uint64_t(PTRDIFF_MAX),
                  std::uint64_t(QByteArray::maxSize()), std::uint64_t(std::string{}.max_size())});
    for (auto n :
         {l.service.reference_bytes, l.service.choice_bytes, l.service.command_bytes,
          l.service.storage_document_bytes, l.service.choices_per_command, l.service.clock_segments,
          l.service.input_owned_bytes, l.service.result_owned_bytes, l.service.result_items,
          l.service.intermediate_owned_bytes, l.service.intermediate_items})
        require(n > 0 && std::uint64_t(n) < max, Error::invalid_limits);
    require(std::uint64_t(l.service.choices_per_command) <
                    std::vector<ApplicationEnvelope>{}.max_size() &&
                std::uint64_t(l.service.clock_segments) <
                    std::vector<core::TimerClockStep>{}.max_size(),
            Error::invalid_limits);
    require(l.pending_reservations &&
                l.pending_reservations < std::vector<std::string>{}.max_size(),
            Error::invalid_limits);
}
inline Measure owning_measure(const ReferenceRuntimeLimits &l, const ReferenceAdapterLimits &values,
                              Error error) {
    auto p = measure(values, error);
    p.bundle_document = std::min(std::uint64_t(l.service.storage_document_bytes),
                                 std::uint64_t(l.service.bundles.value_bytes));
    p.hold_document = std::min(std::uint64_t(l.service.storage_document_bytes),
                               std::uint64_t(l.service.holds.references.value_bytes));
    p.exact_component = std::min(std::uint64_t(l.service.bundles.component_bytes),
                                 std::uint64_t(l.service.holds.references.component_bytes));
    return p;
}
inline Measure runtime_measure(const ReferenceRuntimeLimits &l, Error error = Error::input_limit) {
    return owning_measure(l, l.values, error);
}
inline Measure service_measure(const ReferenceRuntimeLimits &l, Error error) {
    auto limits = l.values;
    limits.reference_bytes =
        std::min(limits.reference_bytes, std::uint64_t(l.service.reference_bytes));
    limits.document_bytes =
        std::min(limits.document_bytes, std::uint64_t(l.service.storage_document_bytes));
    limits.value_owned_bytes =
        std::min(limits.value_owned_bytes, std::uint64_t(l.service.intermediate_owned_bytes));
    limits.value_items = std::min(limits.value_items, std::uint64_t(l.service.intermediate_items));
    auto p = owning_measure(l, limits, error);
    p.p.component_bytes = p.exact_component;
    return p;
}
template <class... T> void service_input(const ReferenceRuntimeLimits &l, const T &...v) {
    auto p = service_measure(l, Error::input_limit);
    p(v...);
}
// Full structural equality, including all duplicates, without re-encoding.
template <class T> bool equal(const T &a, const T &b);
template <class T> bool equal(const std::optional<T> &a, const std::optional<T> &b) {
    return bool(a) == bool(b) && (!a || equal(*a, *b));
}
template <class T> bool equal(const std::vector<T> &a, const std::vector<T> &b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(),
                      [](const auto &x, const auto &y) { return equal(x, y); });
}
template <class T> bool equal(const T &a, const T &b) {
    if constexpr (std::is_integral_v<T> || std::is_enum_v<T> || std::is_same_v<T, std::string> ||
                  std::is_same_v<T, core::Exact>)
        return a == b;
    else if constexpr (std::is_same_v<T, QByteArray>)
        return a == b && a.isNull() == b.isNull();
    else {
        bool eq = true;
        auto left = [&](const auto &...x) {
            auto right = [&](const auto &...y) {
                auto tx = std::tie(x...), ty = std::tie(y...);
                [&]<std::size_t... I>(std::index_sequence<I...>) {
                    eq = (equal(std::get<I>(tx), std::get<I>(ty)) && ...);
                }(std::index_sequence_for<decltype(x)...>{});
            };
            fields(right, b);
        };
        fields(left, a);
        return eq;
    }
}
inline bool active(const ApplicationStateSnapshot &s) {
    return s.timer.lifecycle == core::TimerLifecycle::Active &&
           s.timer.phase == core::CountdownPhase::Main && s.timer.main_remaining_ms > 0 &&
           !s.timer.awaiting_confirmation && s.time_debt_ms == 0 && s.balances.empty();
}
inline bool same_committed(const ReferenceCommittedState &a, const ReferenceCommittedState &b) {
    return bundle_detail::same_state(a.snapshot, b.snapshot) &&
           reference_detail::same(a.basis, b.basis) &&
           a.policy_revision_ref == b.policy_revision_ref &&
           a.observation.clock_lineage_ref == b.observation.clock_lineage_ref &&
           a.observation.observation_ref == b.observation.observation_ref &&
           a.observation.configuration_ref == b.observation.configuration_ref &&
           a.observation.policy_revision_ref == b.observation.policy_revision_ref &&
           a.observation.checked_head_token == b.observation.checked_head_token &&
           a.observation.observed_utc_ms == b.observation.observed_utc_ms &&
           a.observation.segments.size() == b.observation.segments.size() &&
           std::equal(a.observation.segments.begin(), a.observation.segments.end(),
                      b.observation.segments.begin(), [](const auto &x, const auto &y) {
                          return x.elapsed_ms == y.elapsed_ms && x.utc_now_ms == y.utc_now_ms &&
                                 x.paused == y.paused;
                      });
}
} // namespace subathon::server::reference_runtime_detail
