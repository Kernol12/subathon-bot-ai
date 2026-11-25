// SPDX-License-Identifier: GPL-3.0-or-later
#include "connection_pragmas.hpp"

#include <QCoreApplication>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTemporaryDir>
#include <QTextStream>
#include <QVariant>

#include <stdexcept>

namespace {

namespace pragma = subathon::server::detail::v23_pragma;

constexpr auto sqlite_version = "3.53.4";
constexpr auto sqlite_source_id =
    "2026-07-24 19:02:57 bf7c7f30031888f4e796e429ab3978879485813aaca6f641c7b33e4e09459bcc";

void require(bool condition, const QString &message) {
    if (!condition)
        throw std::runtime_error(message.toStdString());
}

class Connection final {
  public:
    explicit Connection(const QString &path)
        : db(QSqlDatabase::addDatabase("QSQLITE", "v23-spill-evidence")) {
        db.setDatabaseName(path);
    }

    ~Connection() {
        const auto name = db.connectionName();
        db.close();
        db = {};
        QSqlDatabase::removeDatabase(name);
    }

    QSqlDatabase db;
};

QSqlQuery query(const QSqlDatabase &db, const char *sql) {
    QSqlQuery result(db);
    const bool executed = result.exec(QString::fromLatin1(sql));
    require(executed, QString::fromLatin1(sql) + ": " + result.lastError().text());
    return result;
}

void require_end(QSqlQuery &result, const char *sql) {
    require(!result.next() && !result.lastError().isValid(),
            QString::fromLatin1(sql) + ": expected no further rows or query error");
}

void command(const QSqlDatabase &db, const char *sql) {
    auto result = query(db, sql);
    require_end(result, sql);
}

QVariant scalar(const QSqlDatabase &db, const char *sql) {
    auto result = query(db, sql);
    require(result.next() && result.record().count() == 1,
            QString::fromLatin1(sql) + ": expected one scalar row");
    auto value = result.value(0);
    require_end(result, sql);
    return value;
}

QString text(const QSqlDatabase &db, const char *sql) {
    const auto value = scalar(db, sql);
    require(!value.isNull() && value.metaType().id() == QMetaType::QString,
            QString::fromLatin1(sql) + ": expected TEXT");
    return value.toString();
}

qint64 integer(const QSqlDatabase &db, const char *sql) {
    const auto value = scalar(db, sql);
    const auto type = value.metaType().id();
    require(!value.isNull() && (type == QMetaType::LongLong || type == QMetaType::Int),
            QString::fromLatin1(sql) + ": expected INTEGER");
    return value.toLongLong();
}

void expect_integer(const QSqlDatabase &db, const char *sql, qint64 expected) {
    const auto actual = integer(db, sql);
    require(actual == expected, QString("%1: expected %2, observed %3")
                                    .arg(QString::fromLatin1(sql))
                                    .arg(expected)
                                    .arg(actual));
}

void expect_mode(const QSqlDatabase &db, const char *sql, const QString &expected) {
    const auto actual = text(db, sql);
    require(
        actual == expected,
        QString("%1: expected %2, observed %3").arg(QString::fromLatin1(sql), expected, actual));
}

void require_supported_engine(const QSqlDatabase &db, QTextStream &out) {
    const auto version = text(db, "SELECT sqlite_version()");
    const auto source = text(db, "SELECT sqlite_source_id()");
    out << "sqlite_version " << version << Qt::endl;
    out << "sqlite_source_id " << source << Qt::endl;
    auto options = query(db, "PRAGMA compile_options");
    QStringList temp_options;
    while (options.next()) {
        const auto option = options.value(0);
        require(!option.isNull() && option.metaType().id() == QMetaType::QString,
                "Expected TEXT compile option");
        const auto name = option.toString();
        out << "compile_option " << name << Qt::endl;
        if (name.startsWith("TEMP_STORE="))
            temp_options.append(name);
    }
    require(!options.lastError().isValid(), "Could not read all SQLite compile options");
    require(version == sqlite_version && source == sqlite_source_id &&
                temp_options == QStringList{"TEMP_STORE=1"},
            "Unsupported QSQLITE engine: require exact upstream SQLite 3.53.4 with TEMP_STORE=1");
}

void check_restored_spill(const QSqlDatabase &db) {
    expect_integer(db, pragma::cache_spill, 2048);
    expect_integer(db, pragma::cache_size, -2000);
}

void exercise_rollback(const QSqlDatabase &db) {
    command(db, "BEGIN IMMEDIATE");
    try {
        command(db, "INSERT INTO spill_probe VALUES (2)");
        expect_integer(db, "SELECT count(*) FROM spill_probe", 2);
        expect_integer(db, pragma::cache_spill, 0);
    } catch (...) {
        command(db, "ROLLBACK");
        throw;
    }
    command(db, "ROLLBACK");
    expect_integer(db, "SELECT value FROM spill_probe", 1);
    expect_integer(db, pragma::cache_spill, 0);
}

void witness(const QString &path, QTextStream &out) {
    Connection connection(path);
    auto &db = connection.db;
    const bool opened = db.open();
    require(opened, "Open disposable QSQLITE file: " + db.lastError().text());
    require_supported_engine(db, out);

    command(db, "PRAGMA main.page_size=4096");
    command(db, pragma::foreign_keys_on);
    expect_integer(db, pragma::foreign_keys, 1);
    command(db, "CREATE TABLE spill_probe(value INTEGER PRIMARY KEY)");
    command(db, "INSERT INTO spill_probe VALUES (1)");
    expect_integer(db, "PRAGMA main.page_size", 4096);
    expect_mode(db, pragma::journal_wal, "wal");
    expect_mode(db, pragma::journal_mode, "wal");
    command(db, "PRAGMA main.cache_size=-2000");
    expect_integer(db, pragma::cache_size, -2000);
    const auto baseline = integer(db, pragma::cache_spill);
    require(baseline >= 0 && baseline < 2048, "Baseline spill readback must be below 2048");
    out << "baseline_cache_spill " << baseline << Qt::endl;
    command(db, "PRAGMA main.cache_spill=2048");
    check_restored_spill(db);
    out << "seeded_cache_spill 2048" << Qt::endl;

    const auto original_temp = integer(db, pragma::temp_store);
    require(original_temp >= 0 && original_temp <= 2, "Unexpected original temp_store value");
    command(db, pragma::temp_schema_probe);
    expect_mode(db, pragma::journal_delete, "delete");
    expect_mode(db, pragma::journal_mode, "delete");
    check_restored_spill(db);
    command(db, pragma::spill_off);
    expect_integer(db, pragma::cache_spill, 0);
    if (original_temp != 2)
        command(db, pragma::temp_memory);
    expect_integer(db, pragma::temp_store, 2);
    command(db, pragma::foreign_keys_off);
    expect_integer(db, pragma::foreign_keys, 0);
    exercise_rollback(db);
    out << "rollback_preserved_seed 1; disabled_cache_spill 0" << Qt::endl;

    command(db, pragma::foreign_keys_on);
    expect_integer(db, pragma::foreign_keys, 1);
    command(db, pragma::temp_schema_probe);
    if (original_temp != 2)
        command(db, original_temp == 0 ? pragma::temp_default : pragma::temp_file);
    expect_integer(db, pragma::temp_store, original_temp);
    command(db, pragma::spill_on);
    check_restored_spill(db);
    out << "restored_cache_spill_delete 2048" << Qt::endl;

    expect_mode(db, pragma::journal_wal, "wal");
    expect_mode(db, pragma::journal_mode, "wal");
    check_restored_spill(db);
    expect_integer(db, pragma::foreign_keys, 1);
    expect_integer(db, pragma::temp_store, original_temp);
    require(!QFileInfo::exists(path + "-journal"), "Rollback journal remains");
    out << "restored_cache_spill_wal 2048; cache_size -2000; journal_mode wal; foreign_keys 1"
        << Qt::endl;
}

} // namespace

int main(int argc, char **argv) {
    const QCoreApplication application(argc, argv);
    QTextStream out(stdout);
    try {
        require(application.arguments().size() == 1, "This fixture takes no arguments");
        const QTemporaryDir directory;
        require(directory.isValid(), "Create disposable database directory");
        witness(directory.filePath("spill.sqlite"), out);
        return 0;
    } catch (const std::exception &failure) {
        QTextStream(stderr) << "v23 spill evidence failed: " << failure.what() << Qt::endl;
        return 1;
    }
}
