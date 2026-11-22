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

struct ClockPreOpenResult {
    // Active is classified but refused. Only candidate can accompany an advance.
    enum class Disposition { unavailable, active_refused, candidate };
    Disposition disposition;
    MigrationReason reason;
    std::optional<ClockPreOpenWitness> witness;
};

[[nodiscard]] std::variant<MigrationManifest, MigrationReason> clock_checkpoint_manifest();
[[nodiscard]] ClockPreOpenResult probe_clock_checkpoint_predecessor(const QString &owned_path);

enum class ClockAdvanceRefusal { none, active, changed_basis };
struct ClockAdvanceResult {
    MigrationResult migration;
    ClockAdvanceRefusal refusal;
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
