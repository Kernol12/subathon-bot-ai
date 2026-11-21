// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_migrations.hpp>

namespace subathon::server {

MigrationManifest connector_intake_repository_manifest();

namespace connector_intake {

struct Scope {
    QByteArray installation, provider, account, stream;
    bool operator==(const Scope &) const = default;
};

struct QuarantineEntry {
    Scope scope;
    QByteArray key, normalized, provenance;
    qint64 arrival_ms;
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
    unproved_range
};

enum class SourcePurpose { baseline = 2, adoption = 3, handoff = 4 };

struct Fence {
    qint64 epoch, revision, policy_revision;
    QByteArray generation;
};

struct PolicyMember {
    Scope scope;
    QByteArray award_class, effect_scope, overlap_set;
    bool primary;
    QByteArray document;
};

struct SourceChange {
    Scope scope;
    Fence expected;
    QByteArray new_generation, policy;
    QList<PolicyMember> members;
    QByteArray authorized_cut, history_ref, current_authority, qualified_capability, boundary_proof;
    SourcePurpose purpose;
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
};

[[nodiscard]] HistoryStatus stage_operation(const SqliteWorkSession &, const OperationIntent &);

struct GapOpening {
    Scope scope;
    Fence expected;
    QByteArray gap_ref, new_generation, frontier, upper_cut, cursor_contract, observation;
    QByteArray cut_request;
};

[[nodiscard]] HistoryStatus stage_gap(const SqliteWorkSession &, const GapOpening &);

} // namespace connector_intake
} // namespace subathon::server
