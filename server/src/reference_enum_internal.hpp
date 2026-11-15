// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/reference_intake.hpp>
namespace subathon::server::reference_runtime_detail {
template <class E> constexpr unsigned tag_max() {
    if constexpr (std::is_same_v<E, subathon::core::PlanValuationState>)
        return std::max(
            {unsigned(E::not_applicable), unsigned(E::missing_historical), unsigned(E::resolved)});
    if constexpr (std::is_same_v<E, subathon::core::RequestedUnit>)
        return std::max(
            {unsigned(E::milliseconds), unsigned(E::micro_points), unsigned(E::whole_count)});
    if constexpr (std::is_same_v<E, subathon::core::OverrideAuthority>)
        return std::max({unsigned(E::unknown), unsigned(E::denied), unsigned(E::authorized)});
    if constexpr (std::is_same_v<E, subathon::core::PlanPreviewValidity>)
        return std::max({unsigned(E::current), unsigned(E::stale_plan), unsigned(E::stale_state)});
    if constexpr (std::is_same_v<E, subathon::core::PaymentEvidence>)
        return std::max({unsigned(E::unknown), unsigned(E::unconfirmed), unsigned(E::confirmed),
                         unsigned(E::not_applicable)});
    if constexpr (std::is_same_v<E, subathon::core::ContributionReview>)
        return std::max({unsigned(E::pending), unsigned(E::not_required), unsigned(E::accepted),
                         unsigned(E::rejected)});
    if constexpr (std::is_same_v<E, subathon::core::ContributionApplication>)
        return std::max({unsigned(E::not_applied), unsigned(E::applied)});
    if constexpr (std::is_same_v<E, subathon::core::ContributionReversal>)
        return std::max({unsigned(E::none), unsigned(E::partial), unsigned(E::full)});
    if constexpr (std::is_same_v<E, subathon::core::SettlementGate>)
        return std::max({unsigned(E::unresolved), unsigned(E::clear)});
    if constexpr (std::is_same_v<E, subathon::core::EvidencePolicy>)
        return std::max({unsigned(E::require_evidence_review), unsigned(E::accept_provider_report),
                         unsigned(E::record_only)});
    if constexpr (std::is_same_v<E, subathon::core::EvidenceReviewChoice>)
        return std::max({unsigned(E::await_confirmation), unsigned(E::accept_labelled_report)});
    if constexpr (std::is_same_v<E, subathon::core::ContributionBlocker>)
        return std::max({unsigned(E::payment_evidence_required),
                         unsigned(E::evidence_review_required), unsigned(E::record_only_policy),
                         unsigned(E::missing_source_amount), unsigned(E::missing_rate),
                         unsigned(E::valuation_unresolved), unsigned(E::review_pending),
                         unsigned(E::review_rejected), unsigned(E::already_applied),
                         unsigned(E::settlement_review_required)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionDelegationContext>)
        return std::max({unsigned(E::role_edit), unsigned(E::invitation),
                         unsigned(E::individual_override), unsigned(E::automation_authorization)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionEditSide>)
        return std::max({unsigned(E::before), unsigned(E::after)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionAutomationDisposition>)
        return std::max({unsigned(E::authorized), unsigned(E::hold)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionFactsState>)
        return std::max({unsigned(E::unknown), unsigned(E::complete)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionOwnerState>)
        return std::max({unsigned(E::unknown), unsigned(E::absent), unsigned(E::present)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionOperationClass>)
        return std::max({unsigned(E::ordinary), unsigned(E::protected_access_management),
                         unsigned(E::protected_recovery), unsigned(E::helper_password_reset)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionNumberKind>)
        return std::max({unsigned(E::rational), unsigned(E::integer)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionResourceMode>)
        return std::max({unsigned(E::any), unsigned(E::allowed_set)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionQuantityMode>)
        return std::max({unsigned(E::any), unsigned(E::at_most), unsigned(E::closed_interval),
                         unsigned(E::magnitude_at_most)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionOrigin>)
        return std::max({unsigned(E::individual), unsigned(E::role)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionEffect>)
        return std::max({unsigned(E::allow), unsigned(E::deny)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionOutcome>)
        return std::max({unsigned(E::deny), unsigned(E::permit)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionReason>)
        return std::max({unsigned(E::default_deny), unsigned(E::individual_allow),
                         unsigned(E::individual_deny), unsigned(E::individual_limit),
                         unsigned(E::role_allow), unsigned(E::role_deny),
                         unsigned(E::protected_owner), unsigned(E::protected_owner_target)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionSourceKind>)
        return std::max({unsigned(E::protected_owner), unsigned(E::individual), unsigned(E::role)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionMatch>)
        return std::max({unsigned(E::other_operation), unsigned(E::resource_mismatch),
                         unsigned(E::quantity_mismatch), unsigned(E::complete)});
    if constexpr (std::is_same_v<E, subathon::core::PermissionSourceDisposition>)
        return std::max({unsigned(E::not_applicable), unsigned(E::suppressed),
                         unsigned(E::quantity_limit), unsigned(E::permit_witness),
                         unsigned(E::fitting_allow), unsigned(E::decisive_deny)});
    if constexpr (std::is_same_v<E, subathon::core::EffectivePermissionError>)
        return std::max({unsigned(E::invalid_limits), unsigned(E::input_limit),
                         unsigned(E::invalid_reference), unsigned(E::invalid_catalog),
                         unsigned(E::invalid_facts), unsigned(E::invalid_request),
                         unsigned(E::invalid_policy), unsigned(E::contradictory_individual),
                         unsigned(E::invalid_number), unsigned(E::facts_unavailable),
                         unsigned(E::result_limit), unsigned(E::allocation_failure)});
    if constexpr (std::is_same_v<E, subathon::core::ReceiptDisposition>)
        return std::max({unsigned(E::created), unsigned(E::retry), unsigned(E::conflict)});
    if constexpr (std::is_same_v<E, subathon::core::ReferenceDerivationError>)
        return std::max({unsigned(E::invalid_limits), unsigned(E::input_limit),
                         unsigned(E::invalid_reference), unsigned(E::invalid_value),
                         unsigned(E::result_limit), unsigned(E::allocation_failure)});
    if constexpr (std::is_same_v<E, subathon::core::ReferenceDerivationState>)
        return std::max({unsigned(E::matched), unsigned(E::disabled), unsigned(E::nonmatch),
                         unsigned(E::unsupported)});
    if constexpr (std::is_same_v<E, subathon::core::RefundDeficitPolicy>)
        return std::max(
            {unsigned(E::StopAtZero), unsigned(E::CarryTimeDebt), unsigned(E::RequireReview)});
    if constexpr (std::is_same_v<E, subathon::core::TimeReversalDisposition>)
        return std::max({unsigned(E::Applied), unsigned(E::RequiresReview)});
    if constexpr (std::is_same_v<E, subathon::core::TimerLifecycle>)
        return std::max({unsigned(E::Ready), unsigned(E::Active), unsigned(E::Finished)});
    if constexpr (std::is_same_v<E, subathon::core::CountdownPhase>)
        return std::max({unsigned(E::Main), unsigned(E::Grace)});
    if constexpr (std::is_same_v<E, subathon::core::TimerEndMode>)
        return std::max({unsigned(E::Immediate), unsigned(E::Grace), unsigned(E::Confirmation)});
    if constexpr (std::is_same_v<E, subathon::core::GraceReuse>)
        return std::max({unsigned(E::Fresh), unsigned(E::Shared)});
    if constexpr (std::is_same_v<E, subathon::core::TimerTransition>)
        return std::max({unsigned(E::MainExpired), unsigned(E::GraceStarted),
                         unsigned(E::GraceExpired), unsigned(E::ConfirmationRequired),
                         unsigned(E::HardEndReached), unsigned(E::Finished)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationPrincipalKind>)
        return std::max({unsigned(E::account), unsigned(E::automation)});
    if constexpr (std::is_same_v<E, subathon::server::OriginalApplicationPurpose>)
        return std::max({unsigned(E::captured_original), unsigned(E::signed_original_correction)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationPreviewMode>)
        return std::max({unsigned(E::revalidate_current), unsigned(E::require_exact)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationReferencePurpose>)
        return std::max({unsigned(E::advanced_state), unsigned(E::after_state), unsigned(E::hold),
                         unsigned(E::detection), unsigned(E::hold_version)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationServiceInputError>)
        return std::max({unsigned(E::invalid_options), unsigned(E::invalid_key),
                         unsigned(E::invalid_source), unsigned(E::collection_limit),
                         unsigned(E::invalid_body), unsigned(E::invalid_conditional_group),
                         unsigned(E::owned_byte_limit), unsigned(E::result_limit)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationReferenceViewError>)
        return std::max({unsigned(E::expired_callback), unsigned(E::wrong_thread)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationPolicyAdmission>)
        return std::max({unsigned(E::allow_active), unsigned(E::deny_with_reason),
                         unsigned(E::unresolved_with_reason), unsigned(E::requires_other_command)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationServiceDisposition>)
        return std::max({unsigned(E::applied),
                         unsigned(E::existing_operation),
                         unsigned(E::subject_claimed),
                         unsigned(E::source_claimed),
                         unsigned(E::operation_conflict),
                         unsigned(E::denied),
                         unsigned(E::disclose_denied),
                         unsigned(E::missing_plan),
                         unsigned(E::plan_conflict),
                         unsigned(E::state_conflict),
                         unsigned(E::ineligible),
                         unsigned(E::active_hold),
                         unsigned(E::held_numeric),
                         unsigned(E::held_policy),
                         unsigned(E::not_ready),
                         unsigned(E::stale_contribution),
                         unsigned(E::stale_state),
                         unsigned(E::state_token_conflict),
                         unsigned(E::subject_unavailable),
                         unsigned(E::stage_conflict)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationServiceFailureSource>)
        return std::max({unsigned(E::bundle), unsigned(E::hold), unsigned(E::retained),
                         unsigned(E::port_contract), unsigned(E::retained_semantics),
                         unsigned(E::bridge)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationServiceLocalReason>)
        return std::max({unsigned(E::invalid_binding), unsigned(E::invalid_output),
                         unsigned(E::invalid_known_document), unsigned(E::contradictory_document),
                         unsigned(E::result_limit), unsigned(E::invalid_completion_payload)});
    if constexpr (std::is_same_v<E, subathon::server::ContributionObservationKind>)
        return std::max({unsigned(E::support), unsigned(E::confirmation)});
    if constexpr (std::is_same_v<E, subathon::server::ContributionLinkDisposition>)
        return std::max({unsigned(E::created), unsigned(E::linked), unsigned(E::duplicate),
                         unsigned(E::unresolved)});
    if constexpr (std::is_same_v<E, subathon::server::PasswordHashingError>)
        return std::max({unsigned(E::invalid_options), unsigned(E::resource_limit),
                         unsigned(E::invalid_record), unsigned(E::unsupported_record),
                         unsigned(E::invalid_account_reference), unsigned(E::runtime_unavailable),
                         unsigned(E::secret_memory_unavailable), unsigned(E::hash_unavailable),
                         unsigned(E::native_output_invalid), unsigned(E::allocation_unavailable),
                         unsigned(E::source_unavailable), unsigned(E::source_invalid)});
    if constexpr (std::is_same_v<E, subathon::server::PasswordVerification>)
        return std::max({unsigned(E::verified), unsigned(E::not_successful)});
    if constexpr (std::is_same_v<E, subathon::server::PasswordCredentialState>)
        return std::max({unsigned(E::present), unsigned(E::missing), unsigned(E::unavailable)});
    if constexpr (std::is_same_v<E, subathon::server::PasswordLoginCheck>)
        return std::max({unsigned(E::accepted), unsigned(E::denied), unsigned(E::stale),
                         unsigned(E::unavailable)});
    if constexpr (std::is_same_v<E, subathon::server::PasswordUpgradeOutcome>)
        return std::max({unsigned(E::credential_missing), unsigned(E::verification_not_successful),
                         unsigned(E::login_not_accepted), unsigned(E::stale_source),
                         unsigned(E::not_needed), unsigned(E::replacement_ready),
                         unsigned(E::upgrade_unavailable)});
    if constexpr (std::is_same_v<E, subathon::server::ReferenceGate>)
        return std::max({unsigned(E::not_ready), unsigned(E::unsupported_configuration),
                         unsigned(E::denied), unsigned(E::conflict)});
    if constexpr (std::is_same_v<E, subathon::server::ReferenceFactsState>)
        return std::max({unsigned(E::unknown), unsigned(E::complete)});
    if constexpr (std::is_same_v<E, subathon::server::ReferenceInputError>)
        return std::max({unsigned(E::invalid_limits), unsigned(E::input_limit),
                         unsigned(E::invalid_tag), unsigned(E::invalid_reference),
                         unsigned(E::invalid_shape), unsigned(E::invalid_value),
                         unsigned(E::invalid_binding), unsigned(E::unsupported_format),
                         unsigned(E::invalid_document), unsigned(E::result_limit),
                         unsigned(E::allocation_failure)});
    if constexpr (std::is_same_v<E, subathon::server::ReferenceEntropyError>)
        return std::max({unsigned(E::none), unsigned(E::short_read), unsigned(E::unavailable)});
    if constexpr (std::is_same_v<E, subathon::server::ReferenceZeroStep>)
        return std::max({unsigned(E::reuse_unchanged), unsigned(E::fresh)});
    if constexpr (std::is_same_v<E, subathon::server::SessionRetention>)
        return std::max({unsigned(E::normal), unsigned(E::remembered)});
    if constexpr (std::is_same_v<E, subathon::server::AcceptedSignInMethod>)
        return std::max({unsigned(E::local_password), unsigned(E::verified_twitch)});
    if constexpr (std::is_same_v<E, subathon::server::LifecycleWriteResult>)
        return std::max({unsigned(E::staged), unsigned(E::stale), unsigned(E::collision)});
    if constexpr (std::is_same_v<E, subathon::server::SessionFailure>)
        return std::max({unsigned(E::invalid_limits), unsigned(E::resource_limit),
                         unsigned(E::invalid_input), unsigned(E::unavailable),
                         unsigned(E::invalid_stored_data), unsigned(E::missing),
                         unsigned(E::revoked), unsigned(E::expired),
                         unsigned(E::fresh_authentication_required)});
    if constexpr (std::is_same_v<E, subathon::server::SessionRecordState>)
        return std::max({unsigned(E::active), unsigned(E::revoked)});
    if constexpr (std::is_same_v<E, subathon::server::AccountSessionReadFailure>)
        return std::max({unsigned(E::missing), unsigned(E::invalid_data), unsigned(E::unavailable),
                         unsigned(E::resource_limit)});
    if constexpr (std::is_same_v<E, subathon::server::AccountSessionInputError>)
        return std::max(
            {unsigned(E::invalid_limits), unsigned(E::resource_limit), unsigned(E::invalid_input)});
    if constexpr (std::is_same_v<E, subathon::server::SqliteBundleInputError>)
        return std::max({unsigned(E::invalid_options), unsigned(E::invalid_key),
                         unsigned(E::invalid_source), unsigned(E::invalid_state),
                         unsigned(E::invalid_contribution), unsigned(E::collection_limit),
                         unsigned(E::invalid_effect), unsigned(E::invalid_component),
                         unsigned(E::invalid_job), unsigned(E::invalid_transition),
                         unsigned(E::invalid_result), unsigned(E::invalid_audit),
                         unsigned(E::invalid_reversal), unsigned(E::exact_component_limit),
                         unsigned(E::owned_byte_limit)});
    if constexpr (std::is_same_v<E, subathon::server::SqliteBundleStorageReason>)
        return std::max({unsigned(E::busy), unsigned(E::sql_failure), unsigned(E::resource_limit),
                         unsigned(E::result_limit), unsigned(E::missing_state),
                         unsigned(E::missing_receipt), unsigned(E::missing_contribution),
                         unsigned(E::missing_plan), unsigned(E::missing_reference),
                         unsigned(E::source_mismatch), unsigned(E::invalid_stored_data),
                         unsigned(E::sequence_exhausted)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationSubjectKind>)
        return std::max({unsigned(E::contribution), unsigned(E::receipt)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationOperationKind>)
        return std::max({unsigned(E::original), unsigned(E::reversal)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationBalanceKind>)
        return std::max({unsigned(E::score), unsigned(E::whole_count)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationArithmeticRole>)
        return std::max(
            {unsigned(E::direct), unsigned(E::time_addition), unsigned(E::time_reversal)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationComponentKind>)
        return std::max({unsigned(E::main_delta), unsigned(E::debt_delta), unsigned(E::score),
                         unsigned(E::whole_count), unsigned(E::unapplied_time),
                         unsigned(E::cap_limited_time)});
    if constexpr (std::is_same_v<E, subathon::server::SqliteBundleBasisDisposition>)
        return std::max({unsigned(E::ready), unsigned(E::reversal_conflict)});
    if constexpr (std::is_same_v<E, subathon::server::SqliteBundleInitialDisposition>)
        return std::max(
            {unsigned(E::inserted), unsigned(E::already_present), unsigned(E::conflict)});
    if constexpr (std::is_same_v<E, subathon::server::SqliteBundleInputDisposition>)
        return std::max({unsigned(E::missing_plan), unsigned(E::stale_plan), unsigned(E::ready)});
    if constexpr (std::is_same_v<E, subathon::server::SqliteBundleStageDisposition>)
        return std::max({unsigned(E::staged), unsigned(E::already_present),
                         unsigned(E::operation_conflict), unsigned(E::subject_claimed),
                         unsigned(E::source_claimed), unsigned(E::reversal_identity_conflict),
                         unsigned(E::stale_state), unsigned(E::stale_contribution),
                         unsigned(E::stale_plan), unsigned(E::state_token_conflict),
                         unsigned(E::stale_reversal), unsigned(E::reversal_conflict),
                         unsigned(E::subject_unavailable)});
    if constexpr (std::is_same_v<E, subathon::server::SqliteOriginalClaimKind>)
        return std::max({unsigned(E::subject), unsigned(E::source)});
    if constexpr (std::is_same_v<E, subathon::server::SqliteHoldInputError>)
        return std::max({unsigned(E::invalid_options), unsigned(E::invalid_key),
                         unsigned(E::invalid_subject), unsigned(E::collection_limit),
                         unsigned(E::invalid_cause), unsigned(E::invalid_failure),
                         unsigned(E::invalid_plan_evidence), unsigned(E::invalid_observation),
                         unsigned(E::invalid_detection), unsigned(E::invalid_resolution),
                         unsigned(E::invalid_cursor), unsigned(E::exact_component_limit),
                         unsigned(E::owned_byte_limit)});
    if constexpr (std::is_same_v<E, subathon::server::SqliteHoldStorageReason>)
        return std::max({unsigned(E::busy), unsigned(E::sql_failure), unsigned(E::resource_limit),
                         unsigned(E::result_limit), unsigned(E::missing_reference),
                         unsigned(E::source_mismatch), unsigned(E::invalid_stored_data),
                         unsigned(E::sequence_exhausted)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationHoldCauseKind>)
        return std::max({unsigned(E::numeric_overflow), unsigned(E::timer_deficit),
                         unsigned(E::unresolved_policy)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationHoldPlanKind>)
        return std::max({unsigned(E::captured_revision), unsigned(E::failed_capture)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationHoldExtent>)
        return std::max({unsigned(E::captured_complete), unsigned(E::proposed_complete),
                         unsigned(E::known_prefix)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationHoldStoppedFor>)
        return std::max({unsigned(E::numeric_conversion), unsigned(E::unresolved_capture_policy)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationHoldFailureStage>)
        return std::max({unsigned(E::final_conversion), unsigned(E::aggregate_after_finalization)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationHoldComposition>)
        return std::max({unsigned(E::defer_whole_input), unsigned(E::beside_complete_application)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationHoldMutationKind>)
        return std::max({unsigned(E::detection), unsigned(E::resolution)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationHoldState>)
        return std::max({unsigned(E::active), unsigned(E::resolved)});
    if constexpr (std::is_same_v<E, subathon::server::ApplicationHoldApplicationLinkKind>)
        return std::max({unsigned(E::whole_input), unsigned(E::timer_correction)});
    if constexpr (std::is_same_v<E, subathon::server::SqliteHoldHistoryDisposition>)
        return std::max({unsigned(E::ready), unsigned(E::missing_hold), unsigned(E::stale_history),
                         unsigned(E::cursor_conflict)});
    if constexpr (std::is_same_v<E, subathon::server::SqliteHoldMutationDisposition>)
        return std::max({unsigned(E::recorded), unsigned(E::already_recorded),
                         unsigned(E::mutation_conflict), unsigned(E::hold_conflict),
                         unsigned(E::stale_active_set), unsigned(E::stale_version),
                         unsigned(E::version_conflict), unsigned(E::missing_hold),
                         unsigned(E::closed_hold), unsigned(E::replacement_conflict)});
    if constexpr (std::is_same_v<E, subathon::server::MigrationMode>)
        return std::max({unsigned(E::inspect), unsigned(E::advance)});
    if constexpr (std::is_same_v<E, subathon::server::MigrationCheckpoint>)
        return std::max({unsigned(E::before_begin), unsigned(E::before_statement),
                         unsigned(E::before_history), unsigned(E::before_commit)});
    if constexpr (std::is_same_v<E, subathon::server::MigrationReason>)
        return std::max(
            {unsigned(E::none), unsigned(E::feature_not_implemented), unsigned(E::invalid_manifest),
             unsigned(E::cancelled), unsigned(E::foreign_store), unsigned(E::catalog_mismatch),
             unsigned(E::unsupported_catalog_format), unsigned(E::unsupported_version),
             unsigned(E::unsupported_encoding), unsigned(E::schema_mismatch),
             unsigned(E::integrity_failed), unsigned(E::inspection_failed), unsigned(E::busy),
             unsigned(E::sql_failure), unsigned(E::resource_limit), unsigned(E::callback_failure),
             unsigned(E::uncertain_outcome), unsigned(E::writer_unavailable)});
    if constexpr (std::is_same_v<E, subathon::server::CatalogState>)
        return std::max({unsigned(E::fresh), unsigned(E::upgrade_required), unsigned(E::current)});
    if constexpr (std::is_same_v<E, subathon::server::PermissionGraphFailure>)
        return std::max({unsigned(E::missing), unsigned(E::unavailable), unsigned(E::invalid_data),
                         unsigned(E::resource_limit)});
    if constexpr (std::is_same_v<E, subathon::server::PermissionGraphInputError>)
        return std::max(
            {unsigned(E::invalid_limits), unsigned(E::invalid_input), unsigned(E::resource_limit)});
    if constexpr (std::is_same_v<E, subathon::server::PermissionGraphWrite>)
        return std::max({unsigned(E::staged), unsigned(E::stale), unsigned(E::collision)});
    if constexpr (std::is_same_v<E, subathon::server::ReferenceAuthorityUse>)
        return std::max({unsigned(E::execute_time_add), unsigned(E::disclose_application),
                         unsigned(E::disclose_holds)});
    if constexpr (std::is_same_v<E, subathon::server::ReferenceAuthorityState>)
        return std::max({unsigned(E::pending), unsigned(E::authorized), unsigned(E::revoked)});
    if constexpr (std::is_same_v<E, subathon::server::ReferenceAuthorityFailure>)
        return std::max({unsigned(E::denied), unsigned(E::missing), unsigned(E::invalid_data),
                         unsigned(E::unavailable), unsigned(E::resource_limit), unsigned(E::stale),
                         unsigned(E::collision)});
    if constexpr (std::is_same_v<E, subathon::server::ReferenceCaptureRetainDisposition>)
        return std::max({unsigned(E::inserted), unsigned(E::already_present), unsigned(E::conflict),
                         unsigned(E::missing_reference), unsigned(E::source_mismatch)});
    if constexpr (std::is_same_v<E, subathon::server::ReferenceCaptureStorageReason>)
        return std::max({unsigned(E::busy), unsigned(E::sql_failure), unsigned(E::resource_limit),
                         unsigned(E::result_limit), unsigned(E::invalid_stored_data)});
    if constexpr (std::is_same_v<E, subathon::server::RetainedKind>)
        return std::max({unsigned(E::configuration), unsigned(E::arrival_context)});
    if constexpr (std::is_same_v<E, subathon::server::RetainDisposition>)
        return std::max({unsigned(E::inserted), unsigned(E::already_present), unsigned(E::conflict),
                         unsigned(E::missing_reference)});
    if constexpr (std::is_same_v<E, subathon::server::RetentionError>)
        return std::max({unsigned(E::feature_not_implemented), unsigned(E::invalid_options),
                         unsigned(E::invalid_kind), unsigned(E::invalid_reference),
                         unsigned(E::invalid_document), unsigned(E::invalid_link),
                         unsigned(E::invalid_stored_data), unsigned(E::result_limit)});
    if constexpr (std::is_same_v<E, subathon::server::StorageError>)
        return std::max(
            {unsigned(E::none), unsigned(E::feature_not_implemented), unsigned(E::wrong_thread),
             unsigned(E::not_open), unsigned(E::invalid_options), unsigned(E::open_failed),
             unsigned(E::writer_already_open), unsigned(E::invalid_batch), unsigned(E::queue_full),
             unsigned(E::shutting_down), unsigned(E::busy), unsigned(E::sql_failure),
             unsigned(E::result_limit), unsigned(E::invalid_work)});
    if constexpr (std::is_same_v<E, subathon::server::TransactionOutcome>)
        return std::max({unsigned(E::not_started), unsigned(E::committed), unsigned(E::rolled_back),
                         unsigned(E::outcome_unknown)});
    if constexpr (std::is_same_v<E, subathon::server::WorkError>)
        return std::max(
            {unsigned(E::none), unsigned(E::feature_not_implemented), unsigned(E::busy),
             unsigned(E::sql_failure), unsigned(E::resource_limit), unsigned(E::result_limit),
             unsigned(E::invalid_statement), unsigned(E::wrong_thread), unsigned(E::stale_session),
             unsigned(E::session_closed), unsigned(E::protocol_error), unsigned(E::work_exception),
             unsigned(E::missing_transaction), unsigned(E::not_committed), unsigned(E::uncommitted),
             unsigned(E::uncertain_outcome), unsigned(E::writer_unavailable)});
    return 0;
}
} // namespace subathon::server::reference_runtime_detail
