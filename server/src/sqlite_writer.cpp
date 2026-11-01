// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_writer.hpp>

#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QRegularExpression>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QThread>
#include <QUuid>
#include <deque>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>

namespace subathon::server {
namespace {
std::mutex writer_lease_mutex;
QSet<QString> writer_leases;

// Existing files resolve symlinks; a new file resolves its existing parent first.
QString canonical_file(const QString &path) {
    const QFileInfo file(path);
    const QString existing = file.canonicalFilePath();
    if (!existing.isEmpty())
        return existing;
    const QString parent = file.dir().canonicalPath();
    return parent.isEmpty() ? QString{} : QDir(parent).filePath(file.fileName());
}

bool take_writer_lease(const QString &path) {
    const std::lock_guard lock(writer_lease_mutex);
    if (writer_leases.contains(path))
        return false;
    writer_leases.insert(path);
    return true;
}

void release_writer_lease(QString &path) {
    if (path.isEmpty())
        return;
    const std::lock_guard lock(writer_lease_mutex);
    writer_leases.remove(path);
    path.clear();
}

StorageError sql_error(const QSqlError &error) {
    bool valid = false;
    const int code = error.nativeErrorCode().toInt(&valid);
    // SQLite extended result codes retain the primary error in their low byte.
    return valid && ((code & 0xff) == 5 || (code & 0xff) == 6) ? StorageError::busy
                                                               : StorageError::sql_failure;
}

// All queries are local to individual operations, so none outlive connection close.
class Connection final {
  public:
    ~Connection() { close(); }
    bool open(const QString &path, bool read_only) {
        if (!QSqlDatabase::isDriverAvailable("QSQLITE"))
            return false;
        name_ = "subathon-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        database_ = QSqlDatabase::addDatabase("QSQLITE", name_);
        database_.setDatabaseName(path);
        database_.setConnectOptions(read_only ? "QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=0"
                                              : "QSQLITE_BUSY_TIMEOUT=0");
        if (!database_.open()) {
            close();
            return false;
        }
        return true;
    }
    void close() {
        if (name_.isEmpty())
            return;
        database_.close();
        database_ = {};
        QSqlDatabase::removeDatabase(std::exchange(name_, {}));
    }
    QSqlDatabase &database() { return database_; }
    bool is_open() const { return !name_.isEmpty(); }

  private:
    QString name_;
    QSqlDatabase database_;
};

StorageError control(Connection &connection, const QString &sql) {
    QSqlQuery query(connection.database());
    return query.exec(sql) ? StorageError::none : sql_error(query.lastError());
}

bool pragma_equals(Connection &connection, const QString &sql, const QVariant &expected) {
    QSqlQuery query(connection.database());
    return query.exec(sql) && query.next() && query.value(0) == expected;
}

bool consume(qsizetype &remaining, qsizetype amount) {
    if (amount > remaining)
        return false;
    remaining -= amount;
    return true;
}

// Revision 1 counts a base eight bytes per value plus any UTF-8/blob payload.
bool consume_value(qsizetype &remaining, const QVariant &value) {
    if (!consume(remaining, 8))
        return false;
    if (!value.isValid() || value.isNull())
        return true;
    switch (value.metaType().id()) {
    case QMetaType::QString:
        return consume(remaining, value.toString().toUtf8().size());
    case QMetaType::QByteArray:
        return consume(remaining, value.toByteArray().size());
    case QMetaType::Bool:
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::UChar:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Double:
        return true;
    default:
        return false;
    }
}

bool allowed_writer_sql(const QString &sql) {
    if (sql.contains(';'))
        return false;
    static const QRegularExpression command("^\\s*([A-Za-z]+)\\b");
    const auto match = command.match(sql);
    if (!match.hasMatch())
        return false;
    const QString verb = match.captured(1).toUpper();
    if (verb == "PRAGMA") {
        static const QRegularExpression diagnostic(
            "^\\s*PRAGMA\\s+(journal_mode|synchronous|foreign_keys|busy_timeout)\\s*$",
            QRegularExpression::CaseInsensitiveOption);
        return diagnostic.match(sql).hasMatch();
    }
    return verb == "SELECT" || verb == "WITH" || verb == "INSERT" || verb == "UPDATE" ||
           verb == "DELETE" || verb == "REPLACE" || verb == "CREATE" || verb == "DROP" ||
           verb == "ALTER";
}

bool valid_limits(const StorageLimits &limits) {
    return limits.pending_batches > 0 && limits.statements_per_batch > 0 &&
           limits.statement_utf8_bytes > 0 && limits.bound_value_bytes > 0 &&
           limits.result_rows > 0 && limits.result_value_bytes > 0;
}

bool valid_batch(const WriteBatch &batch, const StorageLimits &limits) {
    if (batch.isEmpty() || batch.size() > limits.statements_per_batch)
        return false;
    qsizetype remaining = limits.bound_value_bytes;
    for (const auto &statement : batch) {
        if (statement.sql.toUtf8().size() > limits.statement_utf8_bytes ||
            !allowed_writer_sql(statement.sql))
            return false;
        for (const auto &value : statement.bindings)
            if (!consume_value(remaining, value))
                return false;
    }
    return true;
}

// Rows/bytes are shared across the whole writer batch; the reader passes its own
// row limit. A query failure or exceeded bound never publishes partial rows.
ReadResult statement_result(Connection &connection, const SqlStatement &statement,
                            qsizetype &remaining_rows, qsizetype &remaining_bytes) {
    QSqlQuery query(connection.database());
    if (!query.prepare(statement.sql))
        return {sql_error(query.lastError()), {}};
    for (const auto &value : statement.bindings)
        query.addBindValue(value);
    if (!query.exec())
        return {sql_error(query.lastError()), {}};
    SqlRows rows;
    while (query.next()) {
        if (!consume(remaining_rows, 1))
            return {StorageError::result_limit, {}};
        SqlRow row;
        const int columns = query.record().count();
        for (int column = 0; column < columns; ++column) {
            QVariant value = query.value(column);
            if (!consume_value(remaining_bytes, value))
                return {StorageError::result_limit, {}};
            row.append(std::move(value));
        }
        rows.append(std::move(row));
    }
    if (query.lastError().isValid())
        return {sql_error(query.lastError()), {}};
    return {StorageError::none, std::move(rows)};
}

template <typename Callback, typename... Arguments>
void complete(Callback &callback, Arguments &&...arguments) noexcept {
    try {
        callback(std::forward<Arguments>(arguments)...);
    } catch (...) {
        // The transaction result is already fixed; a caller exception cannot
        // unwind Qt dispatch, undo a commit or stop the remaining owned queue.
    }
}
} // namespace

class SqliteWriter::Impl {
  public:
    explicit Impl(SqliteWriter *writer) : writer_(writer), owner_(QThread::currentThread()) {}
    ~Impl() {
        Q_ASSERT(on_owner());
        close();
    }
    bool on_owner() const {
        return QThread::currentThread() == owner_ && writer_->thread() == owner_;
    }
    StorageError open(const QString &path, StorageLimits limits) {
        if (!on_owner())
            return StorageError::wrong_thread;
        if (stopping_)
            return StorageError::shutting_down;
        if (connection_.is_open())
            return StorageError::writer_already_open;
        if (path.isEmpty() || path == ":memory:" || !valid_limits(limits))
            return StorageError::invalid_options;
        const QString canonical = canonical_file(path);
        if (canonical.isEmpty())
            return StorageError::open_failed;
        if (!take_writer_lease(canonical))
            return StorageError::writer_already_open;
        lease_ = canonical;
        if (!connection_.open(canonical, false) ||
            !pragma_equals(connection_, "PRAGMA journal_mode=WAL", QString("wal")) ||
            control(connection_, "PRAGMA synchronous=FULL") != StorageError::none ||
            control(connection_, "PRAGMA foreign_keys=ON") != StorageError::none ||
            !pragma_equals(connection_, "PRAGMA synchronous", 2) ||
            !pragma_equals(connection_, "PRAGMA foreign_keys", 1) ||
            !pragma_equals(connection_, "PRAGMA busy_timeout", 0)) {
            close();
            return StorageError::open_failed;
        }
        limits_ = limits;
        return StorageError::none;
    }
    StorageError enqueue(const WriteBatch &batch, Completion completion) {
        if (!on_owner())
            return StorageError::wrong_thread;
        if (stopping_)
            return StorageError::shutting_down;
        if (!connection_.is_open())
            return StorageError::not_open;
        if (!completion || !valid_batch(batch, limits_))
            return StorageError::invalid_batch;
        const auto outstanding = static_cast<qsizetype>(queue_.size()) + (active_ ? 1 : 0);
        if (outstanding >= limits_.pending_batches)
            return StorageError::queue_full;
        queue_.push_back({batch, std::move(completion)});
        schedule();
        return StorageError::none;
    }
    StorageError shutdown(std::function<void()> completion) {
        if (!on_owner())
            return StorageError::wrong_thread;
        if (!completion)
            return StorageError::invalid_options;
        stopping_ = true;
        shutdown_completions_.push_back(std::move(completion));
        schedule();
        return StorageError::none;
    }

  private:
    struct Pending {
        WriteBatch batch;
        Completion completion;
    };
    SqliteWriter *writer_;
    QThread *const owner_;
    Connection connection_;
    QString lease_;
    StorageLimits limits_;
    std::deque<Pending> queue_;
    std::vector<std::function<void()>> shutdown_completions_;
    bool stopping_{false};
    bool active_{false};
    bool scheduled_{false};

    void close() {
        connection_.close();
        release_writer_lease(lease_);
    }
    void schedule() {
        if (scheduled_ || active_)
            return;
        scheduled_ = true;
        QMetaObject::invokeMethod(writer_, [this] { dispatch(); }, Qt::QueuedConnection);
    }
    WriteResult execute(const WriteBatch &batch) {
        if (!connection_.is_open())
            return {StorageError::sql_failure, {}};
        auto error = control(connection_, "BEGIN IMMEDIATE");
        if (error != StorageError::none)
            return {error, {}};
        qsizetype rows = limits_.result_rows;
        qsizetype bytes = limits_.result_value_bytes;
        QList<SqlRows> results;
        for (const auto &statement : batch) {
            auto result = statement_result(connection_, statement, rows, bytes);
            if (result.error != StorageError::none) {
                error = result.error;
                break;
            }
            results.append(std::move(result.rows));
        }
        if (error == StorageError::none)
            error = control(connection_, "COMMIT");
        if (error != StorageError::none) {
            if (control(connection_, "ROLLBACK") != StorageError::none) {
                // Unknown transaction state cannot admit further writes. Already
                // admitted batches still receive an error through normal dispatch.
                stopping_ = true;
                close();
            }
            return {error, {}};
        }
        return {StorageError::none, std::move(results)};
    }
    void dispatch() {
        Q_ASSERT(on_owner());
        scheduled_ = false;
        if (!queue_.empty()) {
            active_ = true;
            Pending pending = std::move(queue_.front());
            queue_.pop_front();
            WriteResult result = execute(pending.batch);
            active_ = false;
            if (!queue_.empty() || (stopping_ && !shutdown_completions_.empty()))
                schedule();
            // No Impl access follows user code: the callback may reenter the API
            // or destroy the writer after the transaction has completed.
            complete(pending.completion, std::move(result));
            return;
        }
        if (stopping_) {
            close();
            auto completions = std::move(shutdown_completions_);
            shutdown_completions_.clear();
            for (auto &completion : completions)
                complete(completion);
        }
    }
};

SqliteWriter::SqliteWriter(QObject *parent)
    : QObject(parent), impl_(std::make_unique<Impl>(this)) {}
SqliteWriter::~SqliteWriter() = default;
StorageError SqliteWriter::open(const QString &path, StorageLimits limits) {
    return impl_->open(path, limits);
}
StorageError SqliteWriter::enqueue(const WriteBatch &batch, Completion completion) {
    return impl_->enqueue(batch, std::move(completion));
}
StorageError SqliteWriter::shutdown(std::function<void()> completion) {
    return impl_->shutdown(std::move(completion));
}

class SqliteReader::Impl {
  public:
    ~Impl() { Q_ASSERT(on_owner()); }
    bool on_owner() const { return QThread::currentThread() == owner_; }
    StorageError open(const QString &path) {
        if (!on_owner())
            return StorageError::wrong_thread;
        if (connection_.is_open())
            return StorageError::writer_already_open;
        if (path.isEmpty() || path == ":memory:")
            return StorageError::invalid_options;
        if (!connection_.open(path, true) ||
            control(connection_, "PRAGMA query_only=ON") != StorageError::none ||
            !pragma_equals(connection_, "PRAGMA query_only", 1)) {
            connection_.close();
            return StorageError::open_failed;
        }
        return StorageError::none;
    }
    ReadResult read(const SqlStatement &statement, qsizetype max_rows) {
        if (!on_owner())
            return {StorageError::wrong_thread, {}};
        if (!connection_.is_open())
            return {StorageError::not_open, {}};
        if (max_rows <= 0)
            return {StorageError::invalid_options, {}};
        qsizetype bytes = std::numeric_limits<qsizetype>::max();
        return statement_result(connection_, statement, max_rows, bytes);
    }
    StorageError close() {
        if (!on_owner())
            return StorageError::wrong_thread;
        connection_.close();
        return StorageError::none;
    }

  private:
    QThread *const owner_{QThread::currentThread()};
    Connection connection_;
};

SqliteReader::SqliteReader() : impl_(std::make_unique<Impl>()) {}
SqliteReader::~SqliteReader() = default;
StorageError SqliteReader::open(const QString &path) { return impl_->open(path); }
ReadResult SqliteReader::read(const SqlStatement &statement, qsizetype max_rows) {
    return impl_->read(statement, max_rows);
}
StorageError SqliteReader::close() { return impl_->close(); }
} // namespace subathon::server
