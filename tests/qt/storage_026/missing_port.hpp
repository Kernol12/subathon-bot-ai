// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "catalog_fixture.hpp"

#include <subathon/server/sqlite_connector_intake.hpp>

namespace storage_026_fixture {

// Fixture result names used by the frozen Stage 1 assertions. The adapters
// below use the supplied writer unit for Stage 2 storage.
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

// Fixture INTEGER codes used by the literal DDL and raw witness rows.
enum class Purpose { catch_up = 0, import = 1, baseline = 2, adoption = 3, handoff = 4 };
enum class HistoryOutcome { active = 0, completed = 1, refused = 2, superseded = 3 };
enum class SegmentCoverage { covered = 0, lost = 1, unknown = 2 };
enum class BaselineState { pending = 0, qualified = 1, discontinuity = 2 };
enum class GapStatus { open = 0, loss_recorded = 1, resolved = 2 };
struct ManifestMissing {};
inline std::variant<ManifestMissing, MigrationManifest> compiled_connector_manifest() {
    return connector_intake_repository_manifest();
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
inline connector_intake::QuarantineEntry quarantine_input(const QuarantineInput &input) {
    return {{input.installation, input.provider, input.account, input.stream}, input.key,
            input.normalized, input.provenance, 8000};
}
inline Stop quarantine_result(connector_intake::QuarantineStatus result) {
    using Status = connector_intake::QuarantineStatus;
    switch (result) {
    case Status::staged:
        return Stop::committed;
    case Status::exact_replay:
        return Stop::exact_replay;
    case Status::wrong_scope:
    case Status::invalid_authority:
        return Stop::wrong_scope;
    case Status::intake_limit:
        return Stop::intake_limit;
    case Status::quarantine_full:
        return Stop::quarantine_full;
    case Status::conflict:
        return Stop::conflict;
    case Status::stale:
        return Stop::stale;
    }
    throw std::logic_error("Unexpected intake quarantine result");
}
inline Stop admit_quarantine(const SqliteWorkSession &session, const QuarantineInput &input) {
    return quarantine_result(connector_intake::stage_quarantine(session, quarantine_input(input)));
}

struct SourceChange {
    QByteArray installation, provider, account, stream;
    qint64 expected_epoch;
    QByteArray expected_generation;
    qint64 expected_source_revision, expected_policy_revision;
    QByteArray new_generation, new_policy, new_member, authorized_cut;
    QByteArray history_ref, current_authority, qualified_capability, boundary_proof;
    Purpose purpose{Purpose::handoff};
};
inline Stop switch_source_policy(const SqliteWorkSession &, const SourceChange &) {
    return Stop::not_implemented;
}

struct OperationIntent {
    QByteArray installation, provider, account, stream, operation_ref;
    qint64 epoch, policy_revision;
    QByteArray generation;
    std::optional<QByteArray> initial_continuation;
    QByteArray local_date, zone_id, zone_basis;
    qint64 resolved_utc_start;
    enum class Endpoint : quint8 { inclusive = 1, exclusive = 2 };
    struct Interval {
        Endpoint start, end;
        QByteArray membership_basis, order_basis;
    } interval;
    struct UpperCut {
        QByteArray format, position_namespace;
        std::optional<QByteArray> position;
        QByteArray qualified_generation;
        QByteArray proof;
    } upper_cut;
    QByteArray qualified_capability, retained_authority, current_authority;
    Purpose purpose{Purpose::import};
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
struct PageBindResult {
    Stop stop;
    std::optional<qint64> committed_prefix;
};
inline PageBindResult bind_page(const SqliteWorkSession &, const PageAdmission &) {
    return {Stop::not_implemented, {}};
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
using SelectedEntry = connector_intake::SelectedEntry;
struct SelectedPage {
    Stop stop;
    QList<SelectedEntry> entries;
    std::optional<QByteArray> next_after;
};
inline SelectedPage inspect_intake(const SqliteWorkSession &session, const SelectedKey &key) {
    using Status = connector_intake::SelectionStatus;
    auto selected = connector_intake::inspect(
        session, {{key.installation, key.provider, key.account, key.stream}, key.after,
                  key.include_documents, key.exact});
    switch (selected.status) {
    case Status::selected:
        return {Stop::exact_replay, std::move(selected.entries), std::move(selected.next_after)};
    case Status::wrong_scope:
        return {Stop::wrong_scope, {}, {}};
    case Status::intake_limit:
        return {Stop::intake_limit, {}, {}};
    case Status::stale:
        return {Stop::stale, {}, {}};
    case Status::inconsistent:
        return {Stop::inconsistent_original, {}, {}};
    }
    throw std::logic_error("Unexpected intake selection result");
}

struct QuarantineResolution {
    QuarantineInput selected;
    QByteArray current_authority;
};
inline Stop resolve_quarantine(const SqliteWorkSession &session,
                               const QuarantineResolution &request) {
    return quarantine_result(connector_intake::stage_quarantine_resolution(
        session, quarantine_input(request.selected), request.current_authority));
}

} // namespace storage_026_fixture
