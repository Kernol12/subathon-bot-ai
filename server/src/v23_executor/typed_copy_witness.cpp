// SPDX-License-Identifier: GPL-3.0-or-later
#include "typed_copy_witness_access.hpp"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTemporaryDir>
#include <QTextStream>
#include <array>
#include <stdexcept>
#include <subathon/server/sqlite_migration_manifest.hpp>
#include <subathon/server/sqlite_rows.hpp>

namespace {
using namespace subathon::server;

constexpr std::array<const char *, 4> setup{
#include "typed_copy_fixture.inc"
};

void require(bool condition, const QString &message) {
    if (!condition)
        throw std::runtime_error(message.toStdString());
}

SqlRows read(const QSqlDatabase &db, const char *sql) {
    QSqlQuery query(db);
    const bool executed = query.exec(QString::fromUtf8(sql));
    require(executed, QString::fromUtf8(sql) + ": " + query.lastError().text());
    SqlRows rows;
    const auto columns = query.record().count();
    while (query.next()) {
        SqlRow row;
        for (int i = 0; i < columns; ++i)
            row.append(query.value(i));
        rows.append(std::move(row));
    }
    require(!query.lastError().isValid(), query.lastError().text());
    return rows;
}

void check_seed(const QSqlDatabase &db) {
    const auto source = read(db, "SELECT rowid,* FROM main.subathon_plan_revisions");
    const auto staging = read(db, "SELECT rowid,* FROM main.subathon_plan_revisions_rebuild");
    require(source.size() == 1 && staging.size() == 1 && source[0].size() == 35 &&
                staging[0].size() == 39 && source[0][0].toLongLong() == 37 &&
                staging[0][0].toLongLong() == 37,
            "One source and staging row, both rowid 37, with the expected column counts");
    constexpr qsizetype resolution_ref = 8; // The leading SELECT rowid occupies slot zero.
    for (qsizetype column = 0; column < source[0].size(); ++column) {
        if (column == resolution_ref)
            continue;
        const auto &old_value = source[0][column];
        const auto &new_value = staging[0][column];
        require(old_value.isNull() == new_value.isNull() &&
                    (old_value.isNull() ||
                     (old_value.metaType() == new_value.metaType() && old_value == new_value)),
                QString("Unexpected difference at retained column %1").arg(column));
    }
    const auto match =
        read(db, "SELECT o.rowid, o.resolution_ref IS n.resolution_ref COLLATE BINARY,"
                 " typeof(o.resolution_ref), typeof(n.resolution_ref)"
                 " FROM main.subathon_plan_revisions o"
                 " JOIN main.subathon_plan_revisions_rebuild n ON n.rowid=o.rowid");
    require(match == SqlRows{{37, 1, QString("integer"), QString("real")}} &&
                source[0][resolution_ref].toLongLong() == 1 &&
                staging[0][resolution_ref].toDouble() == 1.0,
            "Only rowid 37 differs in storage class, INTEGER 1 versus REAL 1.0, with equal IS");
    require(read(db, "SELECT typeof(valuation_format), valuation_format,"
                     " exact_format_ref IS NULL, exact_document IS NULL, exact_snapshot_ref IS NULL"
                     " FROM main.subathon_plan_revisions_rebuild") ==
                SqlRows{{QString("integer"), 0, 1, 1, 1}},
            "Staging tail matches prescribed legacy values");
}

void check_comparison(const QSqlDatabase &db) {
    for (const auto *statement : setup)
        require(read(db, statement).isEmpty(), "Fixture setup returned an unexpected row");
    check_seed(db);

    bool rejected = false;
    try {
        detail::testing::require_v23_typed_fixture_copy_match(db);
    } catch (const MigrationFailure &failure) {
        require(failure.reason() == MigrationReason::integrity_failed,
                "Type-only difference must report integrity_failed");
        rejected = true;
    }
    require(rejected, "Production comparison accepted a type-only mismatch");

    require(read(db, "UPDATE main.subathon_plan_revisions_rebuild SET resolution_ref=1"
                     " WHERE rowid=37")
                .isEmpty(),
            "Clean control update returned an unexpected row");
    require(read(db, "SELECT typeof(resolution_ref) FROM main.subathon_plan_revisions_rebuild") ==
                SqlRows{{QString("integer")}},
            "Clean control has INTEGER resolution_ref");
    detail::testing::require_v23_typed_fixture_copy_match(db);
}

void witness(const QString &path) {
    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    const bool opened = db.open();
    require(opened, "Open disposable QSQLITE fixture: " + db.lastError().text());
    check_comparison(db);
}

} // namespace

int main(int argc, char **argv) {
    const QCoreApplication application(argc, argv);
    try {
        require(application.arguments().size() == 1, "This fixture takes no arguments");
        const QTemporaryDir directory;
        require(directory.isValid(), "Create disposable database directory");
        try {
            witness(directory.filePath("typed.sqlite"));
        } catch (...) {
            QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnectionName());
            throw;
        }
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnectionName());
        QTextStream(stdout) << "Typed rowid 37 rejected and clean copy accepted" << Qt::endl;
        return 0;
    } catch (const std::exception &failure) {
        QTextStream(stderr) << failure.what() << Qt::endl;
        return 1;
    }
}
