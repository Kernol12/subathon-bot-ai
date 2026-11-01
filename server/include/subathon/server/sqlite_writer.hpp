// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <functional>
#include <memory>

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
    result_limit
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

// Construct, call and destroy on the authoritative Qt command/state thread. Admission
// copies owned statements; completion runs on this same thread after commit/rollback.
// Foreign-thread calls are rejected before any Qt SQL handle is accessed.
class SqliteWriter final : public QObject {
  public:
    using Completion = std::function<void(WriteResult)>;
    explicit SqliteWriter(QObject *parent = nullptr);
    ~SqliteWriter() override;
    [[nodiscard]] StorageError open(const QString &path, StorageLimits limits = {});
    [[nodiscard]] StorageError enqueue(const WriteBatch &batch, Completion completion);
    [[nodiscard]] StorageError shutdown(std::function<void()> completion);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
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
