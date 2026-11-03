// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/server/sqlite_application_bundle.hpp>
#include <variant>
#include <vector>

namespace subathon::server {
// Internal proposal: sqlite-application-holds-api-v1.md. The reference profile's
// byte fields also bound hold fields. Its other ceilings bound selected old
// reference bodies, not returned hold totals or metadata lifetime history.
struct SqliteHoldLimits {
    SqliteBundleLimits references{};
    qsizetype known_occurrences_per_cause{1024};
    qsizetype active_holds_per_result{256};
    qsizetype resolution_entries{64};
    qsizetype history_versions_per_page{64};
    qsizetype input_items{4096};
    qsizetype result_items{8192};
    qsizetype owned_bytes{1048576};
};
enum class SqliteHoldInputError {
    invalid_options,
    invalid_key,
    invalid_subject,
    collection_limit,
    invalid_cause,
    invalid_failure,
    invalid_plan_evidence,
    invalid_observation,
    invalid_detection,
    invalid_resolution,
    invalid_cursor,
    exact_component_limit,
    owned_byte_limit
};
class SqliteHoldInputFailure final : public std::invalid_argument {
  public:
    explicit SqliteHoldInputFailure(SqliteHoldInputError error)
        : std::invalid_argument("Invalid SQLite application hold input"), error_(error) {}
    [[nodiscard]] SqliteHoldInputError error() const noexcept { return error_; }

  private:
    SqliteHoldInputError error_;
};
enum class SqliteHoldStorageReason {
    busy,
    sql_failure,
    resource_limit,
    result_limit,
    missing_reference,
    source_mismatch,
    invalid_stored_data,
    sequence_exhausted
};
class SqliteHoldStorageFailure final : public std::runtime_error {
  public:
    explicit SqliteHoldStorageFailure(SqliteHoldStorageReason reason)
        : std::runtime_error("SQLite application hold unit did not commit"), reason_(reason) {}
    [[nodiscard]] SqliteHoldStorageReason reason() const noexcept { return reason_; }

  private:
    SqliteHoldStorageReason reason_;
};
class SqliteApplicationHoldNotImplemented final : public std::logic_error {
  public:
    SqliteApplicationHoldNotImplemented()
        : std::logic_error(
              "SQLite application holds are intentionally unimplemented; review tests first") {}
};

// Alternative0 maps existing ApplicationSubjectKind::contribution; alternative1
// maps receipt. This ONE logical tag plus its typed ID has no operation scope.
struct ApplicationHoldSubject {
    std::variant<core::ContributionId, core::ReceiptId> id{core::ContributionId{0}};
};
enum class ApplicationHoldCauseKind {
    numeric_overflow = 0,
    timer_deficit = 1,
    unresolved_policy = 2
};
enum class ApplicationHoldPlanKind { captured_revision = 0, failed_capture = 1 };
enum class ApplicationHoldExtent { captured_complete = 0, proposed_complete = 1, known_prefix = 2 };
enum class ApplicationHoldStoppedFor { numeric_conversion = 0, unresolved_capture_policy = 1 };
enum class ApplicationHoldFailureStage { final_conversion = 0, aggregate_after_finalization = 1 };
enum class ApplicationHoldComposition { defer_whole_input = 0, beside_complete_application = 1 };
enum class ApplicationHoldMutationKind { detection = 0, resolution = 1 };
enum class ApplicationHoldState { active = 0, resolved = 1 };
enum class ApplicationHoldApplicationLinkKind { whole_input = 0, timer_correction = 1 };

struct ApplicationHoldSource {
    ApplicationHoldSubject subject;
    core::ReceiptId receipt_id{0};
    std::optional<std::string> event_ref;
    std::optional<std::string> parent_event_ref;
    std::optional<std::string> root_event_ref;
    std::string configuration_ref;
    std::string arrival_context_ref;
};
struct ApplicationHoldCommand {
    ApplicationOperationKey operation;
    ApplicationEnvelope intent;
};
struct ApplicationHoldObservation {
    std::optional<std::string> persisted_state_token;
    ApplicationEnvelope annotation;
    std::optional<std::string> policy_revision_ref;
    std::optional<std::string> permission_revision_ref;
    std::optional<std::string> actor_ref;
    std::optional<std::string> decision_ref;
    std::optional<std::int64_t> observed_utc_ms;
    std::optional<std::int64_t> elapsed_ms;
};
// FailedCapture must retain missing finalized values as absence, never a
// RequestedEffect with fabricated requested_units0. Its provenance is data.
struct ApplicationHoldProposedOccurrence {
    std::int64_t ordinal{0};
    std::optional<std::string> action_ref;
    std::optional<std::string> action_version_ref;
    std::optional<std::string> rule_ref;
    std::optional<std::string> target_ref;
    std::optional<core::RequestedUnit> unit;
    std::optional<core::Exact> attempted_units;
    std::optional<std::int64_t> finalized_units;
    std::optional<std::string> explanation;
};
struct ApplicationHoldPlanEvidence {
    ApplicationHoldPlanKind kind{ApplicationHoldPlanKind::captured_revision};
    std::optional<ApplicationPlanKey> plan;
    ApplicationHoldExtent extent{ApplicationHoldExtent::captured_complete};
    std::optional<ApplicationHoldStoppedFor> stopped_for;
    std::optional<ApplicationEnvelope> failed_capture;
    std::vector<ApplicationHoldProposedOccurrence> occurrences;
};
struct ApplicationHoldFailureEvidence {
    ApplicationHoldFailureStage stage{ApplicationHoldFailureStage::final_conversion};
    std::optional<std::int64_t> failing_ordinal;
    std::optional<std::string> failing_target_ref;
    std::optional<core::Exact> attempted_units;
    ApplicationEnvelope annotation;
};
struct ApplicationHoldAssociatedApplication {
    ApplicationOperationKey operation;
    std::int64_t request_ordinal{0};
};
struct ApplicationHoldTimerEvidence {
    std::int64_t request_ordinal{0};
    ApplicationComponentKind component_kind{ApplicationComponentKind::unapplied_time};
    std::int64_t request_units{0};
    core::RefundDeficitPolicy deficit_policy{core::RefundDeficitPolicy::RequireReview};
    core::TimeReversalDisposition deficit_disposition{
        core::TimeReversalDisposition::RequiresReview};
    std::int64_t unapplied_units{0};
    std::optional<ApplicationHoldComposition>
        composition; // Required explicit choice; no policy default.
    ApplicationEnvelope composition_decision;
    std::optional<ApplicationHoldAssociatedApplication> associated;
};
struct ApplicationHoldCause {
    std::string hold_ref;
    ApplicationHoldSource source;
    ApplicationHoldCommand command;
    ApplicationHoldObservation first_observation;
    ApplicationHoldPlanEvidence plan_evidence;
    ApplicationHoldCauseKind kind{ApplicationHoldCauseKind::numeric_overflow};
    std::optional<ApplicationHoldFailureEvidence> failure;
    std::optional<ApplicationHoldTimerEvidence> timer;
    std::optional<std::string> missing_policy_reason;
    std::optional<std::string> supersedes_hold_ref;
};
// A qualified detection request is cause + this data. Stored first_detection_ref
// is derived from the creating mutation, never overwritten by a later detection.
struct ApplicationHoldDetectionData {
    std::string mutation_ref;
    std::optional<std::string> expected_version_ref;
    std::string proposed_version_ref;
    ApplicationHoldObservation observation;
    ApplicationEnvelope audit;
};
struct ApplicationHoldDetection {
    ApplicationHoldCause cause;
    ApplicationHoldDetectionData request;
};
struct ApplicationHoldVersionLocator {
    std::string hold_ref;
    std::string version_ref;
};
struct ApplicationHoldReplacementLocator {
    std::string hold_ref;
    std::string detection_ref;
};
struct ApplicationHoldVersion {
    ApplicationHoldVersionLocator locator;
    std::int64_t position{0};
    std::optional<std::string> parent_version_ref;
    ApplicationHoldState state{ApplicationHoldState::active};
    std::string producer_mutation_ref;
    std::optional<ApplicationHoldReplacementLocator> replacement;
};
struct ApplicationHoldHeldOrigin {
    ApplicationHoldVersionLocator version;
    std::int64_t request_ordinal{0};
    ApplicationComponentKind component_kind{ApplicationComponentKind::unapplied_time};
    std::optional<std::int64_t> remaining_units;
};
struct ApplicationHoldApplicationLink {
    ApplicationHoldApplicationLinkKind kind{ApplicationHoldApplicationLinkKind::whole_input};
    ApplicationOperationKey operation;
    core::ReceiptId receipt_id{0};
    ApplicationPlanKey plan;
    std::optional<ApplicationHoldHeldOrigin> held_origin;
};
struct ApplicationHoldResolutionEntry {
    std::string hold_ref;
    std::string expected_version_ref;
    std::string proposed_version_ref;
    ApplicationEnvelope decision;
    std::optional<ApplicationHoldDetection> replacement;
};
struct ApplicationHoldResolutionBatch {
    std::string mutation_ref;
    ApplicationHoldSubject subject;
    std::vector<ApplicationHoldVersionLocator> expected_active;
    std::vector<ApplicationHoldResolutionEntry> entries;
    std::string actor_ref;
    std::string decision_ref;
    std::string reason;
    ApplicationEnvelope options;
    std::optional<ApplicationHoldApplicationLink> application;
};
// Finite event ownership: a batch owns replacement Detection/cause data and its
// actual historical produced versions. It never owns sibling/current HoldViews.
struct ApplicationHoldRecordedResolution {
    ApplicationHoldResolutionBatch request;
    std::vector<ApplicationHoldVersion> produced_versions;
};
using ApplicationHoldCurrentEvent =
    std::variant<ApplicationHoldDetectionData, ApplicationHoldRecordedResolution>;
struct ApplicationHoldView {
    ApplicationHoldCause cause;
    ApplicationHoldDetectionData first_detection;
    ApplicationHoldVersion first_version;
    ApplicationHoldVersion current_version;
    // Absent exactly when current is the first produced version. A later
    // detection is qualified by this view's cause; resolution owns its finite batch.
    std::optional<ApplicationHoldCurrentEvent> current_event;
};
struct ApplicationHoldActiveSet {
    ApplicationHoldSubject subject;
    std::vector<ApplicationHoldView> holds;
};
using ApplicationHoldHistoryEvent =
    std::variant<ApplicationHoldDetection, ApplicationHoldRecordedResolution>;
struct ApplicationHoldHistoryEntry {
    ApplicationHoldVersion version;
    ApplicationHoldHistoryEvent event;
};
struct ApplicationHoldHistoryQuery {
    std::string hold_ref;
    std::int64_t after_position{0};
    std::optional<std::string> checked_version_ref;
    qsizetype page_count{64};
};
struct ApplicationHoldHistoryPage {
    std::string hold_ref;
    std::string checked_version_ref;
    std::vector<ApplicationHoldHistoryEntry> entries;
    std::optional<std::int64_t> next_position;
    bool has_more{false};
};
enum class SqliteHoldHistoryDisposition {
    ready = 0,
    missing_hold = 1,
    stale_history = 2,
    cursor_conflict = 3
};
struct ApplicationHoldHistoryResult {
    SqliteHoldHistoryDisposition disposition{SqliteHoldHistoryDisposition::missing_hold};
    std::optional<ApplicationHoldHistoryPage> page;
};
enum class SqliteHoldMutationDisposition {
    recorded = 0,
    already_recorded = 1,
    mutation_conflict = 2,
    hold_conflict = 3,
    stale_active_set = 4,
    stale_version = 5,
    version_conflict = 6,
    missing_hold = 7,
    closed_hold = 8,
    replacement_conflict = 9
};
struct ApplicationHoldMutationOutcome {
    std::string mutation_ref;
    ApplicationHoldMutationKind kind{ApplicationHoldMutationKind::detection};
    // Detection: one active version. Resolution: each entry's resolved version
    // followed immediately by its replacement's initial active version, if any.
    std::vector<ApplicationHoldVersion> produced_versions;
};
struct ApplicationHoldMutationResult {
    SqliteHoldMutationDisposition disposition{SqliteHoldMutationDisposition::recorded};
    // Exactly for recorded/already_recorded; HISTORICAL versions, never current views.
    std::optional<ApplicationHoldMutationOutcome> outcome;
};

// Pure exact prefix1–5 plus approved step6/full89 factory.
[[nodiscard]] MigrationManifest application_hold_repository_manifest();

// Borrowed, inert facade. No construction-thread authority, ownership transfer,
// BEGIN/COMMIT, connection, destructor cleanup, policy grant or publication.
// ALL results are provisional. See API document for complete closure/priorities.
class SqliteApplicationHoldStore final {
  public:
    explicit SqliteApplicationHoldStore(SqliteWorkSession session, SqliteHoldLimits limits = {});
    [[nodiscard]] std::optional<ApplicationHoldView> find(const std::string &hold_ref) const;
    [[nodiscard]] ApplicationHoldActiveSet active(const ApplicationHoldSubject &subject) const;
    [[nodiscard]] ApplicationHoldHistoryResult
    history(const ApplicationHoldHistoryQuery &query) const;
    [[nodiscard]] ApplicationHoldMutationResult
    record_detection(const ApplicationHoldDetection &detection) const;
    [[nodiscard]] ApplicationHoldMutationResult
    stage_resolution(const ApplicationHoldResolutionBatch &batch) const;

  private:
    SqliteWorkSession session_;
    SqliteHoldLimits limits_;
};
} // namespace subathon::server
