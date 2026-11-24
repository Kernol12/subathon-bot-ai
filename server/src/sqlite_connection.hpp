// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <subathon/server/sqlite_writer.hpp>
#include <utility>

namespace subathon::server::detail {

inline StorageError sql_error(const QSqlError &error) {
    bool valid = false;
    const int code = error.nativeErrorCode().toInt(&valid);
    // SQLite extended result codes retain the primary error in their low byte.
    return valid && ((code & 0xff) == 5 || (code & 0xff) == 6) ? StorageError::busy
                                                               : StorageError::sql_failure;
}

// All queries are local to individual operations, so none outlive connection close.
class Connection final {
  public:
    Connection() = default;
    Connection(const Connection &) = delete;
    Connection &operator=(const Connection &) = delete;
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

inline StorageError control(Connection &connection, const QString &sql) {
    QSqlQuery query(connection.database());
    return query.exec(sql) ? StorageError::none : sql_error(query.lastError());
}

inline bool pragma_equals(Connection &connection, const QString &sql, const QVariant &expected) {
    QSqlQuery query(connection.database());
    return query.exec(sql) && query.next() && query.value(0) == expected;
}

inline bool consume(qsizetype &remaining, qsizetype amount) {
    if (amount > remaining)
        return false;
    remaining -= amount;
    return true;
}

// Revision 1 counts a base eight bytes per value plus any UTF-8/blob payload.
inline bool consume_value(qsizetype &remaining, const QVariant &value) {
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

} // namespace subathon::server::detail
