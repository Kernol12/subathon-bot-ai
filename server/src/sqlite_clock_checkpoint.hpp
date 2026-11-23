// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_migration_inspection.hpp"

#include <subathon/server/sqlite_application_bundle.hpp>
#include <subathon/server/sqlite_retained_context.hpp>
#include <variant>

namespace subathon::server::detail {

struct ClockPreOpenResult {
    // Active is classified but refused. Only candidate can accompany an advance.
    enum class Disposition { unavailable, active_refused, candidate };
    Disposition disposition;
    MigrationReason reason;
    std::optional<ClockPreOpenWitness> witness;
    // The query reached, including one refused by its budget before prepare.
    // Both observations are absent if inspection stops before its first query.
    std::optional<ClockProbeQuery> terminal_query;
    std::optional<ClockStatementLedger> attained;
};

[[nodiscard]] std::variant<MigrationManifest, MigrationReason> clock_checkpoint_manifest();
[[nodiscard]] ClockPreOpenResult probe_clock_checkpoint_predecessor(const QString &owned_path,
                                                                    StorageLimits limits);

// Acceptance-only observation. Only the private fixture owner object defines
// this symbol. Counts cover actual prepare and BEGIN attempts on this writer.
struct ClockFixtureWriterTrace {
    qsizetype prepares;
    qsizetype begins;
};
[[nodiscard]] std::optional<ClockFixtureWriterTrace>
clock_checkpoint_fixture_trace(const SqliteWriter &writer);

struct ClockInitialCheckpoint {
    QByteArray installation;
    QByteArray timer;
    QByteArray lineage;
    ApplicationOperationKey operation;
    qint64 cadence_ms;
    qint64 ledger_revision;
    QByteArray slot_document;
    QByteArray root_document;
};

[[nodiscard]] std::variant<SqliteBundleInitialDisposition, WorkError>
stage_clock_checkpoint_initial(const SqliteWorkSession &owner,
                               const RetainedDocument &configuration,
                               const ApplicationStateSnapshot &snapshot,
                               const ClockInitialCheckpoint &checkpoint);

} // namespace subathon::server::detail
