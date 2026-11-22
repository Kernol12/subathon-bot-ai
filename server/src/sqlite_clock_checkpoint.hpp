// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_application_bundle.hpp>
#include <subathon/server/sqlite_retained_context.hpp>
#include <variant>

namespace subathon::server::detail {

// Private STORAGE-003 Stage 1 boundaries. A pre-open candidate carries no
// maintenance authority and must be rechecked in the migration transaction.
enum class ClockPredecessor { blank, no_head, ready, active, finished };

struct ClockPreOpenWitness {
    qint32 version;
    ClockPredecessor predecessor;
    QByteArray token;
    QByteArray configuration;
    bool operator==(const ClockPreOpenWitness &) const = default;
};

struct ClockStatementLedger {
    qsizetype attempts;
    qsizetype peak_sql;
    qsizetype total_sql;
    qsizetype bindings;
    qsizetype binding_bytes;
    qsizetype rows;
    qsizetype cells;
    qsizetype result_bytes;
    bool operator==(const ClockStatementLedger &) const = default;
};

enum class ClockProbeQuery {
    encoding,
    schema,
    header,
    history,
    integrity,
    foreign_keys,
    legacy_guard
};

struct ClockPreOpenResult {
    // Active is classified but refused. Only candidate can accompany an advance.
    enum class Disposition { unavailable, active_refused, candidate };
    Disposition disposition;
    MigrationReason reason;
    std::optional<ClockPreOpenWitness> witness;
    // The query reached, including one refused by its budget before prepare.
    // An unavailable implementation carries neither query nor measured ledger.
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

enum class ClockAdvanceRefusal { none, active, changed_basis };
struct ClockAdvanceResult {
    MigrationResult migration;
    ClockAdvanceRefusal refusal;
    // Present only for observations from the actual initial inspection/guard.
    std::optional<ClockProbeQuery> terminal_preflight_query;
    std::optional<ClockStatementLedger> attained_preflight;
};
using ClockAdvanceCompletion = std::function<void(ClockAdvanceResult)>;

[[nodiscard]] MigrationAdmission
enqueue_clock_checkpoint_advance(SqliteWriter &writer, MigrationRequest request,
                                 const ClockPreOpenWitness &witness,
                                 ClockAdvanceCompletion completion);

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
