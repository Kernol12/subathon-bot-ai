// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "catalog_fixture.hpp"

namespace storage_026_fixture {

// Stage 1's linkable, test-only entry points. Future owning adapters must replace
// these calls. The stubs own no persistence, never claim a COMMIT, and cannot
// parse or manufacture a durable result.
enum class Stop {
    not_implemented,
    committed,
    known_rollback,
    exact_replay,
    different_ordinal,
    different_facts,
    different_configuration,
    different_context,
    intake_limit,
    quarantine_full,
    pending_original,
    inconsistent_original,
    may_create,
    may_link,
    stale,
    conflict,
    busy,
    incomplete_prefix,
    wrong_scope,
    wrong_document_role,
    unproved_range
};
struct ManifestMissing {};
inline std::variant<ManifestMissing, MigrationManifest> compiled_connector_manifest() {
    return ManifestMissing{};
}

struct ManagedInput {
    QByteArray intake_ref;
    core::ReceiptInput receipt;
    VerifiedActivityLink activity;
    QByteArray capture;
    QByteArray installation{"install"};
    QByteArray stream{"S"};
    struct PageSlot {
        QByteArray operation;
        qint64 page, item;
    };
    std::optional<PageSlot> history{};
    enum class Admission { creation_eligible, audit_only, confirmation_only };
    Admission admission{Admission::creation_eligible};
};
struct AttemptLocator {
    core::ReceiptId receipt;
    qint64 ordinal;
    bool operator==(const AttemptLocator &) const = default;
};
struct ManagedResult {
    Stop stop;
    std::optional<AttemptLocator> attempt;
    std::optional<core::ReceiptAttempt> selected;
};
inline ManagedResult stage_managed(const SqliteWorkSession &, const ManagedInput &,
                                   const std::function<bool()> & = {}) {
    return {Stop::not_implemented, {}, {}};
}
inline ManagedResult read_attempt(const SqliteWorkSession &, const ManagedInput &, AttemptLocator) {
    return {Stop::not_implemented, {}, {}};
}

struct EvidenceSelection {
    Stop stop;
    std::optional<AttemptLocator> selected_basis;
    std::optional<core::ContributionId> original;
    std::optional<QByteArray> capture;
};
inline EvidenceSelection select_evidence(const SqliteWorkSession &, const ManagedInput &) {
    return {Stop::not_implemented, {}, {}, {}};
}

struct QuarantineInput {
    QByteArray installation, provider, account, stream, key;
    QByteArray normalized, provenance;
};
inline Stop admit_quarantine(const SqliteWorkSession &, const QuarantineInput &) {
    return Stop::not_implemented;
}

struct SourceChange {
    QByteArray installation, provider, account, stream;
    qint64 expected_epoch;
    QByteArray expected_generation;
    qint64 expected_source_revision, expected_policy_revision;
    QByteArray new_generation, new_policy, new_member, authorized_cut;
};
inline Stop switch_source_policy(const SqliteWorkSession &, const SourceChange &) {
    return Stop::not_implemented;
}

struct OperationIntent {
    QByteArray installation, provider, account, stream, operation_ref;
    qint64 epoch, policy_revision;
    QByteArray generation, initial_continuation, local_date, zone_id, zone_basis;
    qint64 resolved_utc_start;
};
inline Stop bind_history_operation(const SqliteWorkSession &, const OperationIntent &) {
    return Stop::not_implemented;
}

struct PageBinding {
    QByteArray operation_ref;
    qint64 source_revision;
    qint64 operation_revision;
    QByteArray expected_continuation;
    QByteArray page;
    qint64 ordinal;
    qint64 items;
    QByteArray next_continuation;
    QByteArray frontier_before;
    QByteArray frontier_after;
    QByteArray qualified_proof;
    enum class Coverage { contiguous, unproved, lost, recovered, new_baseline };
    Coverage coverage{Coverage::contiguous};
    QByteArray upper_bound;
    qint64 epoch{1};
    qint64 gap_revision{1};
    qint64 policy_revision{1};
    QByteArray generation{"g3"};
};
inline Stop finalize_page(const SqliteWorkSession &, const PageBinding &) {
    return Stop::not_implemented;
}

struct GapOpening {
    QByteArray installation, provider, account, stream, gap_ref;
    qint64 epoch, source_revision, policy_revision;
    QByteArray old_generation, new_generation, frontier, upper_cut, cursor_contract, observation;
    QByteArray cut_request;
};
inline Stop open_gap(const SqliteWorkSession &, const GapOpening &) {
    return Stop::not_implemented;
}

struct PageAdmission {
    PageBinding binding;
    QList<QByteArray> normalized_items;
    std::optional<QuarantineInput> quarantine_preflight{};
};
inline Stop bind_page(const SqliteWorkSession &, const PageAdmission &) {
    return Stop::not_implemented;
}

struct Rollover {
    QByteArray operation, replacement, continuation, frontier, immutable_intent, authority;
    qint64 operation_revision, source_revision;
};
inline Stop rollover_operation(const SqliteWorkSession &, const Rollover &) {
    return Stop::not_implemented;
}

struct SelectedKey {
    QByteArray installation, provider, account, stream;
    QByteArray after;
    bool include_documents{};
    bool exact{};
};
struct SelectedEntry {
    QByteArray ref;
    std::optional<QByteArray> document;
    bool operator==(const SelectedEntry &) const = default;
};
struct SelectedPage {
    Stop stop;
    QList<SelectedEntry> entries;
    std::optional<QByteArray> next_after;
};
inline SelectedPage inspect_intake(const SqliteWorkSession &, const SelectedKey &) {
    return {Stop::not_implemented, {}, {}};
}

struct QuarantineResolution {
    QuarantineInput selected;
    QByteArray current_authority;
};
inline Stop resolve_quarantine(const SqliteWorkSession &, const QuarantineResolution &) {
    return Stop::not_implemented;
}

} // namespace storage_026_fixture
