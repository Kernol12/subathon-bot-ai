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

} // namespace connector_intake
} // namespace subathon::server
