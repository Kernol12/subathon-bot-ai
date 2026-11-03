// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/core/application_plan.hpp>
#include <subathon/core/time_debt.hpp>
#include <subathon/core/timer_lifecycle.hpp>
#include <subathon/server/contribution_evidence.hpp>
#include <subathon/server/sqlite_migrations.hpp>
#include <vector>

namespace subathon::server {
// Internal proposal revision1: docs/contracts/sqlite-application-bundle-v1.md.
// Counts are positive option CEILINGS; empty input vectors remain valid. Each
// ceiling reserves n+1 below qsizetype/qint64 and each relevant vector max_size.
// Checked input/result totals additionally fit size_t; no invented aggregate
// vector capacity substitutes for separate vector bounds. Metadata pages64/65
// never acquire a materialization/lifetime quota from these options.
struct SqliteBundleLimits {
    qsizetype reference_bytes{4096};
    qsizetype value_bytes{262144};
    qsizetype component_bytes{16384};
    qsizetype balances_per_state{1024};
    qsizetype plan_items_per_vector{1024};
    qsizetype effects_per_bundle{1024};
    qsizetype jobs_per_bundle{1024};
    qsizetype transitions_per_bundle{1024};
    qsizetype links_per_bundle{1024};
    qsizetype input_items{12288};
    qsizetype result_items{24576};
    qsizetype owned_bytes{1048576};
};

enum class SqliteBundleInputError {
    invalid_options,
    invalid_key,
    invalid_source,
    invalid_state,
    invalid_contribution,
    collection_limit,
    invalid_effect,
    invalid_component,
    invalid_job,
    invalid_transition,
    invalid_result,
    invalid_audit,
    invalid_reversal,
    exact_component_limit,
    owned_byte_limit
};
class SqliteBundleInputFailure final : public std::invalid_argument {
  public:
    explicit SqliteBundleInputFailure(SqliteBundleInputError error)
        : std::invalid_argument("Invalid SQLite application bundle input"), error_(error) {}
    [[nodiscard]] SqliteBundleInputError error() const noexcept { return error_; }

  private:
    SqliteBundleInputError error_;
};
enum class SqliteBundleStorageReason {
    busy,
    sql_failure,
    resource_limit,
    result_limit,
    missing_state,
    missing_receipt,
    missing_contribution,
    missing_plan,
    missing_reference,
    source_mismatch,
    invalid_stored_data,
    sequence_exhausted
};
// Only established known-uncommitted failures use this port error. Actual
// capability/protocol/invalid-statement/unknown WorkFailure remains original.
class SqliteBundleStorageFailure final : public std::runtime_error {
  public:
    explicit SqliteBundleStorageFailure(SqliteBundleStorageReason reason)
        : std::runtime_error("SQLite application bundle did not commit"), reason_(reason) {}
    [[nodiscard]] SqliteBundleStorageReason reason() const noexcept { return reason_; }

  private:
    SqliteBundleStorageReason reason_;
};
class SqliteApplicationBundleNotImplemented final : public std::logic_error {
  public:
    SqliteApplicationBundleNotImplemented()
        : std::logic_error(
              "SQLite application bundle is intentionally unimplemented; review tests first") {}
};

// Explicit missing-feature marker for the additive original-claim reader only.
class SqliteOriginalClaimNotImplemented final : public std::logic_error {
  public:
    SqliteOriginalClaimNotImplemented()
        : std::logic_error(
              "SQLite original-claim lookup is intentionally unimplemented; review tests first") {}
};

// Owned values; caller supplies owned immutable buffers through the synchronous
// call, including QByteArray data rather than borrowed raw Qt storage. No DTO is
// an authority/durability capability or a provider/semantic namespace proof.
struct ApplicationOperationKey {
    std::string operation_scope;
    std::string operation_ref;
};
struct ApplicationPlanKey {
    std::string plan_ref;
    std::uint64_t revision{0}; // Stored domain1..MAX, distinct from pure codec0.
};
struct ApplicationEnvelope {
    std::string format_ref;
    QByteArray document; // Non-null required; present empty BLOB is valid.
};
enum class ApplicationSubjectKind { contribution = 0, receipt = 1 };
enum class ApplicationOperationKind { original = 0, reversal = 1 };
enum class ApplicationBalanceKind { score = 2, whole_count = 3 };
enum class ApplicationArithmeticRole { direct = 0, time_addition = 1, time_reversal = 2 };
enum class ApplicationComponentKind {
    main_delta = 0,
    debt_delta = 1,
    score = 2,
    whole_count = 3,
    unapplied_time = 4,
    cap_limited_time = 5
};
struct ApplicationBalance {
    ApplicationBalanceKind kind{ApplicationBalanceKind::score};
    std::string target_ref;
    std::int64_t units{0};
};
struct ApplicationStateSnapshot {
    std::string token;
    ApplicationEnvelope envelope;
    std::string configuration_ref;
    core::TimerState timer{};
    std::int64_t time_debt_ms{0};
    // Strict kind/raw-byte-key order, no duplicate key; no numeric StateRevision.
    std::vector<ApplicationBalance> balances;
};
struct ApplicationIntent {
    ApplicationOperationKey operation;
    ApplicationOperationKind kind{ApplicationOperationKind::original};
    // Explicit caller classification; absent CID alone never means non-support.
    ApplicationSubjectKind subject_kind{ApplicationSubjectKind::contribution};
    core::ReceiptId receipt_id{0};                       // Domain1..MAX.
    std::optional<core::ContributionId> contribution_id; // Present0..MAX versus absence.
    ApplicationPlanKey plan;
    std::string expected_state_token;
    std::optional<core::ContributionState> expected_contribution_state;
    ApplicationEnvelope choices;
    std::optional<std::string> reversal_ref;
};
struct ApplicationComponent {
    ApplicationComponentKind kind{ApplicationComponentKind::main_delta};
    std::int64_t units{0};
    std::optional<std::string> target_ref; // Only score/count, matching actual before balance.
};
struct ApplicationEffect {
    std::int64_t request_ordinal{0}; // Existing immutable plan position1..N, not EffectId.
    ApplicationArithmeticRole arithmetic_role{ApplicationArithmeticRole::direct};
    core::RequestedUnit unit{core::RequestedUnit::milliseconds};
    core::Exact exact_requested_units{0};
    std::int64_t requested_units{0};
    core::Exact rounding_residual{0};
    std::optional<std::int64_t> maximum_main_ms;
    std::optional<core::RefundDeficitPolicy> deficit_policy;
    std::optional<core::TimeReversalDisposition> deficit_disposition;
    std::string application_explanation;
    // Ascending kind; role exact set has one or three entries, including zeros.
    std::vector<ApplicationComponent> components;
};
struct ApplicationJobIntent {
    std::string job_ref;
    std::optional<std::int64_t> source_request_ordinal;
    std::optional<std::string> group_ref;
    std::optional<std::int64_t> group_ordinal;
    ApplicationEnvelope envelope;
};
struct ApplicationAudit {
    std::optional<std::string> actor_ref;
    std::optional<std::string> decision_ref;
    std::optional<std::string> reason;
    ApplicationEnvelope envelope;
};
struct ApplicationOriginalComponent {
    ApplicationOperationKey operation;
    std::int64_t request_ordinal{0};
    // Credited source subset0/2/3 only; no cap/held/debt allocation inference.
    ApplicationComponentKind component_kind{ApplicationComponentKind::main_delta};
};
struct ApplicationReversalLink {
    std::int64_t request_ordinal{0};
    ApplicationOriginalComponent original;
    std::int64_t expected_prior_position{0}; // Optimistic0 valid; never stored row0.
    std::int64_t expected_prior_cumulative{0};
    std::int64_t cumulative{0};
    std::int64_t requested_increment{0};
    core::Exact cumulative_fraction{0};
    core::Exact exact_entitlement{0};
    core::Exact rounding_residual{0};
    bool full_refund{false};
};
struct ApplicationReversalOccurrence {
    std::int64_t request_ordinal{0};
    ApplicationOriginalComponent original;
    std::int64_t basis_position{0}; // Actual positive signed64 append position, not domain ID.
    std::int64_t previous_cumulative{0};
    std::int64_t cumulative{0};
    std::int64_t requested_increment{0};
    core::Exact cumulative_fraction{0};
    core::Exact exact_entitlement{0};
    core::Exact rounding_residual{0};
    bool full_refund{false};
};
struct ApplicationReversalOccurrenceKey {
    ApplicationOperationKey operation;
    std::int64_t request_ordinal{0};
    ApplicationOriginalComponent original;
};
struct ApplicationReversalProgress {
    std::int64_t position{0};
    std::int64_t cumulative{0};
    core::Exact cumulative_fraction{0};
    bool full_refund{false};
    // Absent only for an actual valid basis with no prior occurrences.
    std::optional<ApplicationReversalOccurrenceKey> last;
};
struct ApplicationReversalBasis {
    ApplicationOriginalComponent original;
    ApplicationSubjectKind subject_kind{ApplicationSubjectKind::contribution};
    ApplicationPlanKey plan;
    // Actual immutable receipt/CID/arrival/valuation/rule/modifier/request provenance.
    core::CapturedPlanRevision selected_plan{};
    ApplicationEffect effect; // Complete selected effect/component set, not whole bundle.
    std::int64_t actual_credit_units{0};
    std::optional<ApplicationBalance> target; // Selected before target for score/count only.
    std::string application_before_token;
    std::string application_configuration_ref;
    std::optional<core::ReceiptId> contribution_original_receipt_id;
    ApplicationReversalProgress progress;
};
enum class SqliteBundleBasisDisposition { ready, reversal_conflict };
struct ApplicationReversalBasisResult {
    SqliteBundleBasisDisposition disposition{SqliteBundleBasisDisposition::reversal_conflict};
    // Present exactly for ready; conflict has no invented zero basis/progress.
    std::optional<ApplicationReversalBasis> basis;
};
struct PreparedApplicationBundle {
    ApplicationIntent intent;
    ApplicationStateSnapshot advanced_before;
    ApplicationStateSnapshot after;
    std::string advance_explanation;
    std::vector<ApplicationEffect> effects;
    std::vector<ApplicationJobIntent> jobs;
    std::vector<core::TimerTransition> transitions;
    // MANY original bases per current request; no invented/split plan ordinals.
    std::vector<ApplicationReversalLink> reversals;
    ApplicationEnvelope result;
    ApplicationAudit audit;
};
struct ApplicationContributionSnapshot {
    core::ContributionId contribution_id{0};
    core::ContributionState before{};
    core::ContributionApplication application_after{core::ContributionApplication::not_applied};
};
struct RecordedApplicationBundle {
    ApplicationIntent intent;
    ApplicationStateSnapshot persisted_head;
    ApplicationStateSnapshot advanced_before;
    ApplicationStateSnapshot after;
    std::string advance_explanation;
    std::optional<ApplicationContributionSnapshot> contribution;
    std::vector<ApplicationEffect> effects;
    std::vector<ApplicationJobIntent> jobs;
    std::vector<core::TimerTransition> transitions;
    std::vector<ApplicationReversalOccurrence> reversals;
    ApplicationEnvelope result;
    ApplicationAudit audit;
};
struct ApplicationContributionInput {
    ContributionRecord contribution{};           // All CURRENT dimensions, original source fields.
    ContributionEvidenceRecord support_anchor{}; // Bounded immutable full original/support.
};
struct ApplicationBundleInputs {
    ApplicationSubjectKind subject_kind{ApplicationSubjectKind::contribution};
    ApplicationPlanKey plan;
    std::uint64_t actual_plan_head{0};
    core::CapturedPlanRevision selected_plan{};
    ApplicationStateSnapshot current_state;
    std::optional<ApplicationContributionInput> current_contribution;
};
enum class SqliteBundleInitialDisposition { inserted, already_present, conflict };
enum class SqliteBundleInputDisposition { missing_plan, stale_plan, ready };
struct ApplicationBundleInputResult {
    SqliteBundleInputDisposition disposition{SqliteBundleInputDisposition::missing_plan};
    // Present exactly for ready; data/projection only, no authority certificate.
    std::optional<ApplicationBundleInputs> inputs;
};
enum class SqliteBundleStageDisposition {
    staged,
    already_present,
    operation_conflict,
    subject_claimed,
    source_claimed,
    reversal_identity_conflict,
    stale_state,
    stale_contribution,
    stale_plan,
    state_token_conflict,
    stale_reversal,
    reversal_conflict,
    subject_unavailable
};
struct ApplicationBundleStageResult {
    SqliteBundleStageDisposition disposition{SqliteBundleStageDisposition::staged};
    // Present for already_present/subject_claimed/source_claimed ONLY. Its full
    // selected closure was read, but data remains provisional in this session.
    // Other outcomes carry no existing record; staged can be read by lookup.
    std::optional<RecordedApplicationBundle> existing;
};

// Additive reader revision1: docs/contracts/sqlite-original-claims-v1.md.
// A fixed scalar predicate, not source classification, authorization or durability.
struct ApplicationOriginalClaimSelector {
    ApplicationSubjectKind subject_kind{ApplicationSubjectKind::contribution};
    core::ReceiptId receipt_id{0};
    std::optional<core::ContributionId> contribution_id;
};
enum class SqliteOriginalClaimKind { subject = 0, source = 1 };
struct ApplicationOriginalClaimMatch {
    SqliteOriginalClaimKind kind{SqliteOriginalClaimKind::subject};
    RecordedApplicationBundle existing;
};

// Pure exact prefix1–4 + proposed step5/full66 description, no update authority.
[[nodiscard]] MigrationManifest application_bundle_repository_manifest();

// Inert copyable facade over the caller's actual active revocable session.
// No construction-thread tag, BEGIN/commit, adoption or destructor cleanup.
// ALL read/lookup/duplicate/claim/stage values are owned transaction-visible data,
// possibly own staged rows. Prior intake/capture COMMIT is a caller protocol.
// Publication requires actual caller COMMIT plus normal committed completion;
// an old recorded result never turns this work's actual unknown into success.
class SqliteApplicationBundleStore final {
  public:
    explicit SqliteApplicationBundleStore(SqliteWorkSession session,
                                          SqliteBundleLimits limits = {});
    // Options, count, then snapshot shape preflight; no input mutation/SQL cleanup.
    // Only empty head+snapshot namespace initializes. Current exact equality is
    // already_present; otherwise conflict without reset/repoint or own writes.
    [[nodiscard]] SqliteBundleInitialDisposition
    retain_initial_state(const ApplicationStateSnapshot &snapshot) const;
    // Options first. No head+no snapshots -> absent; inconsistent metadata fails.
    [[nodiscard]] std::optional<ApplicationStateSnapshot> read_state() const;
    // Options, PlanKey and known SubjectKind precede SQL. Missing plan is local
    // absence; present full structural metadata is checked before stale_plan.
    // Ready owns selected full head capture/current state/current contribution.
    // Actual role/CID mismatch is source_mismatch, not post-SQL input validation.
    [[nodiscard]] ApplicationBundleInputResult
    read_inputs(const ApplicationPlanKey &plan, ApplicationSubjectKind subject_kind) const;
    // Options/key first; missing key -> absent, no global orphan audit. Present
    // rows require FULL selected record closure; historical snapshots need not
    // equal today's head/current contribution and are not freshly revalued.
    [[nodiscard]] std::optional<RecordedApplicationBundle>
    lookup(const ApplicationOperationKey &operation) const;
    // Contract: options then role/positive receipt/exact CID presence before SQL.
    // Fixed selector has no input_items/aggregate input owned_bytes quota. Global
    // tagged subject wins before ORIGINAL receipt source; absence admits nothing.
    // A match owns FULL selected lookup closure plus one enum byte, zero extra
    // vector items. All values remain provisional, never authority/durability.
    // Healthy reads do no writes/cleanup; known failures require actual whole-unit
    // rollback, while capability/protocol/unknown retain original WorkFailure.
    [[nodiscard]] std::optional<ApplicationOriginalClaimMatch>
    lookup_original_claim(const ApplicationOriginalClaimSelector &selector) const;
    // Options then original operation key, positive request position and credited
    // kind0/2/3 precede SQL. Uses EXACT reviewed selected-source closure plus per-
    // basis64/65 progress audit, never full RecordedBundle/global payload loading.
    // Ready includes selected actual credit/unit/target/provenance/current progress.
    // Valid basis with no occurrences -> position0/cumulative0/fraction0/fullfalse/
    // absent last. Missing/corrupt source is a typed reference/stored failure, not
    // an invented zero basis. Non-original kind -> source_mismatch; a valid original
    // with unsupported actual credited basis -> conflict, no own writes/cleanup.
    // All returned data remains provisional, never authority.
    [[nodiscard]] ApplicationReversalBasisResult
    read_reversal_basis(const ApplicationOriginalComponent &original) const;
    // Pure priority: options/key/source; ALL counts; snapshot shape; expected
    // current contribution/choices; effect/component/Exact/local conservation;
    // aggregate deltas; jobs/groups; transitions; result/audit; reversal algebra;
    // owned bytes. Then exact existing intent/claims BEFORE current freshness,
    // current token+15dim comparison, selected plan/source/head, token uniqueness,
    // scoped source/progress checks, prepared whole-unit writes. Contract source
    // metadata64/65 scope does not certify unselected historical payloads.
    // Healthy refusals make no own writes/cleanup. Local known failures require
    // actual whole-unit rollback; capability/protocol/unknown stays WorkFailure.
    [[nodiscard]] ApplicationBundleStageResult stage(const PreparedApplicationBundle &bundle) const;

  private:
    SqliteWorkSession session_;
    SqliteBundleLimits limits_;
};
} // namespace subathon::server
