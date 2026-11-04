// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QMetaType>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/server/sqlite_application_holds.hpp>
#include <subathon/server/sqlite_retained_context.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace subathon::server {
// Concrete v1: docs/contracts/application-service-api-v1.md. These are owned
// internal DATA, never authentication, policy, durability or delivery capabilities.
enum class ApplicationPrincipalKind { account = 0, automation = 1 };
enum class OriginalApplicationPurpose { captured_original = 0, signed_original_correction = 1 };
enum class ApplicationPreviewMode { revalidate_current = 0, require_exact = 1 };
enum class ApplicationReferencePurpose {
    advanced_state = 0,
    after_state = 1,
    hold = 2,
    detection = 3,
    hold_version = 4
};

struct ApplicationPrincipalBinding {
    ApplicationPrincipalKind kind{ApplicationPrincipalKind::account};
    std::string installation_ref;
    std::string principal_ref;
};
struct OriginalApplicationCommand {
    ApplicationOperationKey operation;
    ApplicationSubjectKind subject_kind{ApplicationSubjectKind::contribution};
    core::ReceiptId receipt_id{0};
    std::optional<core::ContributionId> contribution_id;
    ApplicationPlanKey plan;
    ApplicationPrincipalBinding principal;
    OriginalApplicationPurpose purpose{OriginalApplicationPurpose::captured_original};
    ApplicationPreviewMode preview_mode{ApplicationPreviewMode::revalidate_current};
    std::optional<std::string> preview_state_token;
    std::optional<std::string> actor_ref;
    std::optional<std::string> decision_ref;
    std::optional<std::string> reason;
    std::vector<ApplicationEnvelope> choices;
};
struct ApplicationServiceLimits {
    SqliteBundleLimits bundles{};
    SqliteHoldLimits holds{};
    qsizetype reference_bytes{4096};
    qsizetype choice_bytes{262144};
    qsizetype command_bytes{262144};
    qsizetype storage_document_bytes{262144};
    qsizetype choices_per_command{1024};
    qsizetype clock_segments{1024};
    qsizetype input_owned_bytes{1048576};
    qsizetype result_owned_bytes{4194304};
    qsizetype result_items{65536};
    qsizetype intermediate_owned_bytes{4194304};
    qsizetype intermediate_items{65536};
};

enum class ApplicationServiceInputError {
    invalid_options = 0,
    invalid_key = 1,
    invalid_source = 2,
    collection_limit = 3,
    invalid_body = 4,
    invalid_conditional_group = 5,
    owned_byte_limit = 6,
    result_limit = 7
};
class ApplicationServiceInputFailure final : public std::invalid_argument {
  public:
    explicit ApplicationServiceInputFailure(ApplicationServiceInputError error)
        : std::invalid_argument("Invalid original application service input"), error_(error) {}
    [[nodiscard]] ApplicationServiceInputError error() const noexcept { return error_; }

  private:
    ApplicationServiceInputError error_;
};
class ApplicationServiceUnsupportedFormat final : public std::runtime_error {
  public:
    explicit ApplicationServiceUnsupportedFormat(std::string format_ref)
        : std::runtime_error("Unsupported original application format"),
          format_ref_(std::move(format_ref)) {}
    [[nodiscard]] const std::string &format_ref() const noexcept { return format_ref_; }

  private:
    std::string format_ref_;
};
class ApplicationServicePreparationFailure final : public std::runtime_error {
  public:
    explicit ApplicationServicePreparationFailure(StorageError error)
        : std::runtime_error("Original application work preparation rejected"), error_(error) {}
    [[nodiscard]] StorageError error() const noexcept { return error_; }

  private:
    StorageError error_;
};
enum class ApplicationReferenceViewError { expired_callback = 0, wrong_thread = 1 };
class ApplicationReferenceViewFailure final : public std::runtime_error {
  public:
    explicit ApplicationReferenceViewFailure(ApplicationReferenceViewError error)
        : std::runtime_error("Application reference read view rejected"), error_(error) {}
    [[nodiscard]] ApplicationReferenceViewError error() const noexcept { return error_; }

  private:
    ApplicationReferenceViewError error_;
};

// Historical baseline markers retained for precise test diagnostics. Implemented
// entrypoints do not throw these missing-feature exceptions.
class OriginalApplicationCommandEncodeNotImplemented final : public std::logic_error {
  public:
    OriginalApplicationCommandEncodeNotImplemented()
        : std::logic_error("Original application command encoding is intentionally unimplemented; "
                           "review tests first") {}
};
class OriginalApplicationCommandDecodeNotImplemented final : public std::logic_error {
  public:
    OriginalApplicationCommandDecodeNotImplemented()
        : std::logic_error("Original application command decoding is intentionally unimplemented; "
                           "review tests first") {}
};
class ApplicationReferenceReadNotImplemented final : public std::logic_error {
  public:
    ApplicationReferenceReadNotImplemented()
        : std::logic_error(
              "Application reference reading is intentionally unimplemented; review tests first") {}
};
class OriginalApplicationServiceNotImplemented final : public std::logic_error {
  public:
    OriginalApplicationServiceNotImplemented()
        : std::logic_error(
              "Original application service is intentionally unimplemented; review tests first") {}
};

namespace detail {
class ApplicationReferenceReadState;
}
// Inert default/copy/destruction. Only a callback-scoped finite retained read is
// exposed; no Session, SQL, writer, adoption or public capability constructor.
class ApplicationReferenceReadView final {
  public:
    ApplicationReferenceReadView() = default;
    [[nodiscard]] std::optional<RetainedDocument> find_retained(const RetainedKey &key) const;

  private:
    std::weak_ptr<detail::ApplicationReferenceReadState> state_;
    friend class OriginalApplicationService;
};

struct ApplicationAccessAllowed {
    std::string permission_revision_ref;
};
struct ApplicationAccessDenied {};
struct ApplicationAccessUnknown {};
// Only allowed owns a current revision; neither other alternative is a grant.
using AccessDecision =
    std::variant<ApplicationAccessAllowed, ApplicationAccessDenied, ApplicationAccessUnknown>;

enum class ApplicationPolicyAdmission {
    allow_active = 0,
    deny_with_reason = 1,
    unresolved_with_reason = 2,
    requires_other_command = 3
};
struct ApplicationPolicyAdmissionDecision {
    ApplicationPolicyAdmission disposition{ApplicationPolicyAdmission::requires_other_command};
    std::optional<std::string> reason_ref;
};
struct ApplicationSignedCorrectionPolicy {
    std::int64_t request_ordinal{0};
    core::RefundDeficitPolicy deficit_policy{core::RefundDeficitPolicy::RequireReview};
};
struct BoundApplicationPolicy {
    ApplicationOperationKey operation;
    ApplicationOriginalClaimSelector source;
    ApplicationPlanKey plan;
    std::string checked_state_token;
    std::string configuration_ref;
    std::string permission_revision_ref;
    std::string policy_revision_ref;
    std::optional<std::int64_t> maximum_main_ms;
    core::TimerConfig timer_config{};
    bool paused{false};
    bool recovery_pending{false};
    std::string pause_recovery_revision;
    std::string explanation;
    ApplicationPolicyAdmissionDecision admission;
    std::vector<ApplicationSignedCorrectionPolicy> correction_policies;
};
struct ApplicationUnavailable {
    std::string reason_ref;
};
using PolicyResolution = std::variant<BoundApplicationPolicy, ApplicationUnavailable>;

struct ApplicationClockObservation {
    std::string clock_lineage_ref;
    std::string observation_ref;
    std::string configuration_ref;
    std::string policy_revision_ref;
    std::string checked_head_token;
    std::int64_t observed_utc_ms{0};
    std::vector<core::TimerClockStep> segments;
};
using ClockResolution = std::variant<ApplicationClockObservation, ApplicationUnavailable>;

// Token-free complete numeric proposal; later occurrences use the running timer.
// No speculative snapshot token or output publication is present in this DTO.
struct ApplicationCalculatedProposal {
    core::TimerState clock_advanced_timer{};
    core::TimerState timer_after{};
    std::int64_t debt_after{0};
    std::vector<ApplicationBalance> balances_after;
    std::vector<ApplicationEffect> effects;
    std::vector<core::TimerTransition> transitions;
    ApplicationClockObservation observation;
};
struct PreparedApplicationActions {
    std::vector<ApplicationJobIntent> jobs;
    std::string explanation;
};
using ActionResolution = std::variant<PreparedApplicationActions, ApplicationUnavailable>;

struct ApplicationReferenceBinding {
    ApplicationOperationKey operation;
    ApplicationOriginalClaimSelector source;
    ApplicationPlanKey plan;
    std::string persisted_head_token;
    ApplicationReferencePurpose purpose{ApplicationReferencePurpose::advanced_state};
    std::optional<std::int64_t> request_ordinal;
};
struct IssuedApplicationReference {
    ApplicationReferenceBinding binding;
    std::string reference;
};
using ReferenceResolution = std::variant<IssuedApplicationReference, ApplicationUnavailable>;

// Trusted synchronous ports receive a finite read view and const owned DATA,
// never the caller's commit/write capability. Pure virtuals implement no policy.
class ExecutionAccess {
  public:
    virtual ~ExecutionAccess() = default;
    [[nodiscard]] virtual AccessDecision check(const ApplicationReferenceReadView &read_view,
                                               const OriginalApplicationCommand &command) = 0;
    [[nodiscard]] virtual bool disclose_application(const ApplicationReferenceReadView &read_view,
                                                    const OriginalApplicationCommand &command,
                                                    const RecordedApplicationBundle &record) = 0;
    [[nodiscard]] virtual bool disclose_holds(const ApplicationReferenceReadView &read_view,
                                              const OriginalApplicationCommand &command,
                                              const ApplicationHoldActiveSet &active_set) = 0;
};
class CurrentApplicationPolicy {
  public:
    virtual ~CurrentApplicationPolicy() = default;
    [[nodiscard]] virtual PolicyResolution resolve(const ApplicationReferenceReadView &read_view,
                                                   const OriginalApplicationCommand &command,
                                                   const ApplicationBundleInputs &inputs,
                                                   const AccessDecision &access) = 0;
};
class SegmentedApplicationClock {
  public:
    virtual ~SegmentedApplicationClock() = default;
    [[nodiscard]] virtual ClockResolution observe(const ApplicationReferenceReadView &read_view,
                                                  const OriginalApplicationCommand &command,
                                                  const ApplicationBundleInputs &inputs,
                                                  const BoundApplicationPolicy &policy) = 0;
};
class RequiredApplicationActions {
  public:
    virtual ~RequiredApplicationActions() = default;
    [[nodiscard]] virtual ActionResolution
    prepare(const ApplicationReferenceReadView &read_view,
            const OriginalApplicationCommand &command, const ApplicationBundleInputs &inputs,
            const BoundApplicationPolicy &policy,
            const ApplicationCalculatedProposal &proposal) = 0;
};
class FreshApplicationReferences {
  public:
    virtual ~FreshApplicationReferences() = default;
    [[nodiscard]] virtual ReferenceResolution issue(ApplicationReferencePurpose purpose,
                                                    const ApplicationReferenceBinding &binding) = 0;
};
struct ApplicationServicePorts {
    ExecutionAccess &access;
    CurrentApplicationPolicy &policy;
    SegmentedApplicationClock &clock;
    RequiredApplicationActions &actions;
    FreshApplicationReferences &references;
};

enum class ApplicationServiceDisposition {
    applied = 0,
    existing_operation = 1,
    subject_claimed = 2,
    source_claimed = 3,
    operation_conflict = 4,
    denied = 5,
    disclose_denied = 6,
    missing_plan = 7,
    plan_conflict = 8,
    state_conflict = 9,
    ineligible = 10,
    active_hold = 11,
    held_numeric = 12,
    held_policy = 13,
    not_ready = 14,
    stale_contribution = 15,
    stale_state = 16,
    state_token_conflict = 17,
    subject_unavailable = 18,
    stage_conflict = 19
};
struct ApplicationActiveHoldPayload {
    std::optional<ApplicationHoldActiveSet> holds;
};
struct ApplicationHeldPayload {
    ApplicationHoldMutationOutcome outcome;
    std::string hold_ref;
};
struct ApplicationNotReadyPayload {
    std::string reason_ref;
};
using ApplicationStageConflict =
    std::variant<SqliteBundleStageDisposition, SqliteHoldMutationDisposition>;
using ApplicationServicePayload =
    std::variant<std::monostate, RecordedApplicationBundle, std::vector<core::ContributionBlocker>,
                 ApplicationActiveHoldPayload, ApplicationHeldPayload, ApplicationNotReadyPayload,
                 ApplicationStageConflict>;
struct ApplicationServiceResponse {
    ApplicationServiceDisposition disposition{ApplicationServiceDisposition::not_ready};
    // The variant selects the contract's payload, not a second logical enum byte.
    // All normal payloads remain provisional until actual committed completion.
    ApplicationServicePayload payload;
};
enum class ApplicationServiceFailureSource {
    bundle = 0,
    hold = 1,
    retained = 2,
    port_contract = 3,
    retained_semantics = 4,
    bridge = 5
};
enum class ApplicationServiceLocalReason {
    invalid_binding = 0,
    invalid_output = 1,
    invalid_known_document = 2,
    contradictory_document = 3,
    result_limit = 4,
    invalid_completion_payload = 5
};
using ApplicationServiceFailureReason =
    std::variant<SqliteBundleStorageReason, SqliteHoldStorageReason, RetentionError, WorkError,
                 ApplicationServiceLocalReason>;
struct ApplicationServiceFailure {
    ApplicationServiceFailureSource source{ApplicationServiceFailureSource::port_contract};
    ApplicationServiceFailureReason reason{ApplicationServiceLocalReason::invalid_output};
};
struct ApplicationServiceCompletion {
    TransactionOutcome transaction{TransactionOutcome::not_started};
    WorkError work_error{WorkError::none};
    std::optional<ApplicationServiceFailure> failure;
    std::optional<ApplicationServiceResponse> response;
};
struct ApplicationServiceAdmission {
    StorageError error{StorageError::none};
};
struct ApplicationServicePreparedWork {
    SqliteWriter::Work body;
    SqliteWriter::WorkCompletion completion;
};

[[nodiscard]] ApplicationEnvelope
encode_original_application_command(const OriginalApplicationCommand &command,
                                    const ApplicationServiceLimits &limits = {});
[[nodiscard]] OriginalApplicationCommand
decode_original_application_command(const ApplicationEnvelope &envelope,
                                    const ApplicationServiceLimits &limits = {});

// Inert borrowed dependencies. The host must outlive every prepared pair, even
// unsubmitted pairs, and drain submitted work before destroying service/ports.
// make_work owns a bounded command snapshot; enqueue admits its matching pair.
// Only real committed normal completion can return a public response.
class OriginalApplicationService final {
  public:
    using Completion = std::function<void(ApplicationServiceCompletion)>;
    OriginalApplicationService(SqliteWriter &writer, ApplicationServicePorts &ports,
                               ApplicationServiceLimits limits = {});
    [[nodiscard]] ApplicationServicePreparedWork
    make_work(const OriginalApplicationCommand &command, Completion completion) const;
    [[nodiscard]] ApplicationServiceAdmission enqueue(const OriginalApplicationCommand &command,
                                                      Completion completion);

  private:
    SqliteWriter &writer_;
    ApplicationServicePorts &ports_;
    ApplicationServiceLimits limits_;
};
} // namespace subathon::server

Q_DECLARE_METATYPE(subathon::server::ApplicationServiceResponse)
