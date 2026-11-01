// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace subathon::server {
// Internal trusted-SQL boundary, revision 1: docs/contracts/sqlite-writer-v1.md.
enum class StorageError {
    none,
    feature_not_implemented,
    wrong_thread,
    not_open,
    invalid_options,
    open_failed,
    writer_already_open,
    invalid_batch,
    queue_full,
    shutting_down,
    busy,
    sql_failure,
    result_limit,
    invalid_work
};

struct SqlStatement {
    QString sql;
    QList<QVariant> bindings;
};
using SqlRow = QList<QVariant>;
using SqlRows = QList<SqlRow>;
using WriteBatch = QList<SqlStatement>;

struct StorageLimits {
    qsizetype pending_batches{64};
    qsizetype statements_per_batch{64};
    qsizetype statement_utf8_bytes{65536};
    qsizetype bound_value_bytes{1048576};
    qsizetype result_rows{1024};
    qsizetype result_value_bytes{1048576};
};

struct WriteResult {
    StorageError error{StorageError::none};
    // One entry per statement, populated only for a successful committed batch.
    QList<SqlRows> rows;
};

struct ReadResult {
    StorageError error{StorageError::none};
    SqlRows rows;
};

// Additive internal work boundary: docs/contracts/writer-transactions-v1.md.
enum class TransactionOutcome { not_started, committed, rolled_back, outcome_unknown };
enum class WorkError {
    none,
    feature_not_implemented,
    busy,
    sql_failure,
    resource_limit,
    result_limit,
    invalid_statement,
    wrong_thread,
    stale_session,
    session_closed,
    protocol_error,
    work_exception,
    missing_transaction,
    not_committed,
    uncommitted,
    uncertain_outcome,
    writer_unavailable
};

// Category mapping for execute: SQL-text/statement-count/binding-byte overages
// use resource_limit; result row/value overages use result_limit. Unsupported
// binding QVariant types and prohibited SQL use invalid_statement. Terminal
// session operations in the current body use session_closed (a protocol failure);
// a second begin uses protocol_error. Expired work tickets use stale_session.

class SqliteWorkFailure final : public std::runtime_error {
  public:
    explicit SqliteWorkFailure(WorkError error)
        : std::runtime_error("SQLite work operation failed"), error_(error) {}
    [[nodiscard]] WorkError error() const noexcept { return error_; }

  private:
    WorkError error_;
};

struct SqliteWorkResult {
    TransactionOutcome transaction{TransactionOutcome::not_started};
    WorkError error{WorkError::none};
    // Present only after a normal body return and an acknowledged durable commit.
    std::optional<QVariant> value;
};

namespace detail {
class SqliteWorkState;
}
namespace testing {
class SqliteWriterWorkAccess;
}

// Inert copyable tickets: no raw Qt SQL handle, connection or writer ownership.
// Default/expired tickets reject operations as stale_session. A live foreign-thread
// call rejects as wrong_thread before accessing any owner SQL or transaction state.
class SqliteWorkSession final {
  public:
    SqliteWorkSession() = default;
    [[nodiscard]] SqlRows execute(const SqlStatement &statement) const;
    void commit() const; // Performs the actual synchronous COMMIT, never an outer vote.
    void rollback() const;
    // Idempotent RAII cleanup: inactive/stale tickets return none, foreign tickets
    // return wrong_thread. Active rollback failure returns uncertain_outcome and
    // closes the writer. This function never throws, including after destruction.
    [[nodiscard]] WorkError cleanup() const noexcept;

  private:
    explicit SqliteWorkSession(std::weak_ptr<detail::SqliteWorkState> state)
        : state_(std::move(state)) {}
    std::weak_ptr<detail::SqliteWorkState> state_;
    friend class SqliteWorkContext;
    friend class SqliteWriter;
};

class SqliteWorkContext final {
  public:
    SqliteWorkContext() = default;
    [[nodiscard]] SqliteWorkSession begin() const; // One actual BEGIN IMMEDIATE attempt.

  private:
    explicit SqliteWorkContext(std::weak_ptr<detail::SqliteWorkState> state)
        : state_(std::move(state)) {}
    std::weak_ptr<detail::SqliteWorkState> state_;
    friend class SqliteWriter;
};

// Construct, call and destroy on the authoritative Qt command/state thread. Admission
// copies owned statements; completion runs on this same thread after commit/rollback.
// Foreign-thread calls are rejected before any Qt SQL handle is accessed.
class SqliteWriter final : public QObject {
  public:
    using Completion = std::function<void(WriteResult)>;
    // Trusted callers supply owned captures/results; the writer cannot inspect
    // arbitrary captured pointers. SQL/bindings and returned rows are snapshotted
    // by execute. Existing enqueue(WriteBatch) deep-ownership guarantees persist.
    using Work = std::function<QVariant(SqliteWorkContext)>;
    using WorkCompletion = std::function<void(SqliteWorkResult)>;
    explicit SqliteWriter(QObject *parent = nullptr);
    ~SqliteWriter() override;
    [[nodiscard]] StorageError open(const QString &path, StorageLimits limits = {});
    [[nodiscard]] StorageError enqueue(const WriteBatch &batch, Completion completion);
    [[nodiscard]] StorageError enqueue_work(Work work, WorkCompletion completion);
    [[nodiscard]] StorageError shutdown(std::function<void()> completion);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    friend class testing::SqliteWriterWorkAccess;
};

// A distinct connection opened with SQLite's real read-only flag as well as
// query_only. No QSqlDatabase/QSqlQuery handle escapes its owning thread.
class SqliteReader final {
  public:
    SqliteReader();
    ~SqliteReader();
    SqliteReader(const SqliteReader &) = delete;
    SqliteReader &operator=(const SqliteReader &) = delete;
    [[nodiscard]] StorageError open(const QString &path);
    [[nodiscard]] ReadResult read(const SqlStatement &statement, qsizetype max_rows = 1024);
    [[nodiscard]] StorageError close();

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace subathon::server
