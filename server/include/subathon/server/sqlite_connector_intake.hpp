// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/contribution_evidence.hpp>
#include <subathon/server/sqlite_migrations.hpp>
#include <subathon/server/sqlite_retained_context.hpp>

namespace subathon::server {

MigrationManifest connector_intake_repository_manifest();

namespace connector_intake {

struct Scope {
    QByteArray installation, provider, account, stream;
    bool operator==(const Scope &) const = default;
};

struct Fence {
    qint64 epoch, revision, policy_revision;
    QByteArray generation;
    bool operator==(const Fence &) const = default;
};

struct PageSlot {
    QByteArray operation;
    qint64 page, item;
    bool operator==(const PageSlot &) const = default;
};

struct QuarantinePageSlot {
    PageSlot slot;
    qint64 operation_revision, gap_revision;
    QByteArray expected_continuation, page;
    QByteArray page_reconnect{};
};

struct QuarantineEntry {
    Scope scope;
    QByteArray key, normalized, provenance;
    qint64 arrival_ms;
    Fence expected_source;
    std::optional<QuarantinePageSlot> history{};
};

enum class QuarantineStatus {
    staged,
    exact_replay,
    wrong_scope,
    invalid_authority,
    intake_limit,
    quarantine_full,
    conflict,
    stale
};

// These functions consume an already authorized selection. Staging leaves the
// commit and acknowledgement with the existing writer unit owner.
[[nodiscard]] QuarantineStatus preflight_quarantine(const SqliteWorkSession &,
                                                    const QuarantineEntry &);
[[nodiscard]] QuarantineStatus stage_quarantine(const SqliteWorkSession &, const QuarantineEntry &);
[[nodiscard]] QuarantineStatus stage_quarantine_resolution(const SqliteWorkSession &,
                                                           const QuarantineEntry &,
                                                           const QByteArray &current_authority);

struct SelectedKey {
    Scope scope;
    QByteArray after;
    bool include_documents{};
    bool exact{};
};

struct SelectedEntry {
    QByteArray ref;
    std::optional<QByteArray> document;
    bool operator==(const SelectedEntry &) const = default;
};

enum class SelectionStatus { selected, wrong_scope, intake_limit, stale, inconsistent };

struct SelectedPage {
    SelectionStatus status;
    QList<SelectedEntry> entries{};
    std::optional<QByteArray> next_after{};
};

[[nodiscard]] SelectedPage inspect(const SqliteWorkSession &, const SelectedKey &);

enum class HistoryStatus {
    staged,
    exact_replay,
    wrong_scope,
    invalid_authority,
    intake_limit,
    stale,
    conflict,
    busy,
    incomplete_prefix,
    unproved_range,
    quarantine_full
};

enum class SourcePurpose { baseline = 2, adoption = 3, handoff = 4 };

struct PolicyMember {
    Scope scope;
    QByteArray award_class, effect_scope, overlap_set;
    bool primary;
    QByteArray document;
    bool operator==(const PolicyMember &) const = default;
};

struct SourceTransition {
    Scope scope;
    Fence expected;
    QByteArray new_generation, authorized_cut, history_ref, current_authority;
    QByteArray qualified_capability, boundary_proof;
    SourcePurpose purpose;
};

struct SourceChange {
    Scope scope;
    Fence expected;
    QByteArray new_generation, policy;
    QList<PolicyMember> members;
    QByteArray authorized_cut, history_ref, current_authority, qualified_capability, boundary_proof;
    SourcePurpose purpose;
    qint64 expected_installation_policy_revision;
    // Include one explicit transition for every additional affected source.
    QList<SourceTransition> affected_sources{};
};

[[nodiscard]] HistoryStatus stage_source_change(const SqliteWorkSession &, const SourceChange &);

enum class HistoryPurpose { catch_up = 0, import = 1 };
enum class Endpoint : quint8 { inclusive = 1, exclusive = 2 };

struct HistoryInterval {
    Endpoint start, end;
    QByteArray membership_basis, order_basis;
};

struct QualifiedCut {
    QByteArray format, position_namespace;
    std::optional<QByteArray> position;
    QByteArray qualified_generation, proof;
};

struct OperationIntent {
    Scope scope;
    QByteArray operation_ref;
    qint64 epoch, policy_revision;
    QByteArray generation;
    std::optional<QByteArray> initial_continuation;
    QByteArray local_date, zone_id, zone_basis;
    qint64 resolved_utc_start;
    HistoryInterval interval;
    QualifiedCut upper_cut;
    QByteArray qualified_capability, retained_authority, current_authority;
    HistoryPurpose purpose;
    std::optional<QByteArray> gap{};
    qint64 source_revision{};
};

[[nodiscard]] HistoryStatus stage_operation(const SqliteWorkSession &, const OperationIntent &);

struct GapOpening {
    Scope scope;
    Fence expected;
    QByteArray gap_ref, new_generation, frontier, upper_cut, cursor_contract, observation;
    QByteArray cut_request;
};

[[nodiscard]] HistoryStatus stage_gap(const SqliteWorkSession &, const GapOpening &);

struct Reconnect {
    Scope scope;
    Fence expected;
    QByteArray new_generation, cursor_contract, history_ref, current_authority, observation;
    // Selects the existing witness when a retained page already crossed a generation.
    QByteArray prior_page_reconnect{};
};

[[nodiscard]] HistoryStatus stage_reconnect(const SqliteWorkSession &, const Reconnect &);

enum class Coverage { contiguous, unproved, lost, recovered, new_baseline };

struct PageBinding {
    Scope scope;
    Fence source;
    QByteArray operation_ref;
    qint64 operation_revision, gap_revision;
    QByteArray expected_continuation, page;
    qint64 ordinal, items;
    QByteArray next_continuation, frontier_before, frontier_after, qualified_proof;
    Coverage coverage;
    QByteArray upper_bound;
    // Exact source-history key for a page transferred from an earlier generation.
    QByteArray page_reconnect{};
};

struct PageAdmission {
    PageBinding binding;
    QList<QByteArray> normalized_items;
    QList<QuarantineEntry> quarantine_preflight{};
};

struct BoundPage {
    HistoryStatus status;
    std::optional<qint64> committed_prefix{};
};

[[nodiscard]] BoundPage stage_page(const SqliteWorkSession &, const PageAdmission &);
[[nodiscard]] HistoryStatus stage_page_finalization(const SqliteWorkSession &, const PageBinding &);

struct Rollover {
    Scope scope;
    QByteArray operation, replacement, continuation, frontier, immutable_intent, current_authority;
    qint64 operation_revision, source_revision;
};

[[nodiscard]] HistoryStatus stage_rollover(const SqliteWorkSession &, const Rollover &);

struct AttemptLocator {
    core::ReceiptId receipt;
    qint64 ordinal;
    bool operator==(const AttemptLocator &) const = default;
};

// Admission is already classified against the current policy and connector proof.
enum class IntakeAdmission { creation_eligible_verified_support, audit_only, confirmation_only };

struct ManagedInput {
    Scope scope;
    QByteArray intake_ref, normalized, capture;
    core::ReceiptInput receipt;
    std::optional<VerifiedActivityLink> activity;
    IntakeAdmission admission;
    std::optional<PageSlot> history{};
    std::optional<RetainedDocument> configuration{}, context{};
    Fence expected_source{};
    QByteArray page_reconnect{};
};

enum class ManagedStatus {
    committed,
    known_rollback,
    exact_replay,
    different_ordinal,
    different_facts,
    different_configuration,
    different_context,
    intake_limit,
    wrong_scope,
    wrong_document_role,
    conflict,
    stale
};

struct ManagedResult {
    ManagedStatus status;
    std::optional<AttemptLocator> attempt{};
    std::optional<core::ReceiptAttempt> selected{};
};

// Takes ownership of the active transaction and returns committed only after
// the receipt owner acknowledges COMMIT. Any exception carries its real writer
// outcome. The optional callback can veto commit after the complete unit stages.
[[nodiscard]] ManagedResult admit_managed(const SqliteWorkSession &, const ManagedInput &,
                                          const std::function<bool()> &before_commit = {});
[[nodiscard]] ManagedResult read_attempt(const SqliteWorkSession &, const ManagedInput &,
                                         AttemptLocator);

enum class EvidenceStatus { may_create, may_link, pending_original, inconsistent_original };
struct EvidenceSelection {
    EvidenceStatus status;
    std::optional<AttemptLocator> basis{};
    std::optional<core::ContributionId> original{};
    std::optional<QByteArray> capture{};
};

// Selection carries durable provenance for the ACCT-002 owner. It does not
// stage its evidence or supply current authority.
[[nodiscard]] EvidenceSelection select_evidence(const SqliteWorkSession &, const ManagedInput &);

enum class IntakeDisposition { observed_only, waiting_original, evidence_committed };

// Reconciles a separately owned outcome without granting evidence, plan or
// application authority. A staged disposition can qualify a durable page item.
[[nodiscard]] HistoryStatus stage_intake_disposition(const SqliteWorkSession &,
                                                     const ManagedInput &, AttemptLocator,
                                                     IntakeDisposition);

} // namespace connector_intake
} // namespace subathon::server
