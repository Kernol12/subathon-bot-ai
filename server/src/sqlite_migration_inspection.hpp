// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_catalog_inspection.hpp"

#include <subathon/server/sqlite_migrations.hpp>

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

// Only the checkpoint binding may queue the guarded executor, after exact
// compiled-manifest and typed-witness validation.
class ClockAdvanceExecution final {
    static MigrationAdmission enqueue(SqliteWriter &writer, MigrationRequest request,
                                      ClockPreOpenWitness witness,
                                      ClockAdvanceCompletion completion);
    friend MigrationAdmission enqueue_clock_checkpoint_advance(
        SqliteWriter &, MigrationRequest, const ClockPreOpenWitness &, ClockAdvanceCompletion);
};

class CatalogWriterAccess final {
  public:
    // Private queue policy runs before the migration body, with the live mode.
    // False skips the body and delivers the default not-started work result.
    // The migration completion adapter supplies the request's refusal reason.
    using DequeueCheck = std::function<bool(bool governing)>;
    [[nodiscard]] static bool verified_v22(const SqliteWorkSession &);

  private:
    friend class MigrationExecution;
    static MigrationAdmission enqueue(SqliteWriter &, MigrationMode, bool governing_allowed,
                                      SqliteWriter::Work, SqliteWriter::WorkCompletion,
                                      DequeueCheck);
    static void verified_advance(const SqliteWorkSession &);
};

inline constexpr qint32 clock_predecessor_version = 21;
using ClockInspectionQuery = std::function<SqlRows(ClockProbeQuery, const QString &)>;

// Inspect the shipped catalog using the same ordered queries and validation as
// the writer. The caller owns the connection, transaction and statement meter.
[[nodiscard]] qint32 inspect_clock_predecessor_catalog(MigrationManifest manifest,
                                                      const ClockInspectionQuery &execute);

// The version must come from a successful inspection on the same transaction.
[[nodiscard]] ClockPreOpenWitness read_clock_predecessor(qint32 version,
                                                        const ClockInspectionQuery &execute);

} // namespace subathon::server::detail
