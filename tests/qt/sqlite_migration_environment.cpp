// SPDX-License-Identifier: GPL-3.0-or-later
// Green environment checks use ONLY the existing writer. No migration UUT/header.
#include <QDebug>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <memory>
#include <subathon/server/sqlite_writer.hpp>

using namespace subathon::server;

namespace {
class EnvironmentDatabase final {
  public:
    explicit EnvironmentDatabase(const QString &path)
        : name_(QUuid::createUuid().toString()), db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
    }
    ~EnvironmentDatabase() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }
    bool open() { return db_.open(); }
    bool exec(const QString &sql) {
        QSqlQuery query(db_);
        return query.exec(sql);
    }
    QVariant scalar(const QString &sql) {
        QSqlQuery query(db_);
        return query.exec(sql) && query.next() ? query.value(0) : QVariant();
    }

  private:
    QString name_;
    QSqlDatabase db_;
};

bool existing_work(SqliteWriter &writer, SqliteWriter::Work work, SqliteWorkResult &result) {
    struct State {
        bool done{false};
        SqliteWorkResult result;
    };
    auto state = std::make_shared<State>();
    const auto admission = writer.enqueue_work(std::move(work), [state](SqliteWorkResult r) {
        state->result = std::move(r);
        state->done = true;
    });
    if (admission != StorageError::none)
        return false;
    if (!QTest::qWaitFor([state] { return state->done; }, 5000))
        return false;
    result = std::move(state->result);
    return true;
}
} // namespace

class SqliteMigrationEnvironment final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        QVERIFY(QSqlDatabase::isDriverAvailable("QSQLITE"));
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }

    void fixedFunctionsUseExistingWriterAndUtf8() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("new.sqlite")), StorageError::none);
        SqlRows runtime, encoding, integrity, foreign_keys, text;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](SqliteWorkContext context) {
                auto session = context.begin();
                runtime = session.execute({"SELECT sqlite_version()", {}});
                encoding = session.execute({"SELECT * FROM pragma_encoding()", {}});
                integrity = session.execute({"SELECT * FROM pragma_integrity_check() LIMIT 2", {}});
                foreign_keys =
                    session.execute({"SELECT * FROM pragma_foreign_key_check() LIMIT 1", {}});
                const auto unicode = QString::fromUtf8("é🙂");
                text = session.execute({"SELECT ?, CAST(? AS BLOB)", {unicode, unicode}});
                session.commit();
                return QVariant("environment");
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QCOMPARE(runtime.size(), 1);
        QCOMPARE(runtime[0].size(), 1);
        QVERIFY(!runtime[0][0].toString().isEmpty());
        qInfo().noquote() << "Selected Qt" << QT_VERSION_STR << "SQLite"
                          << runtime[0][0].toString();
        QCOMPARE(encoding, SqlRows({{QString("UTF-8")}}));
        QCOMPARE(integrity, SqlRows({{QString("ok")}}));
        QVERIFY(foreign_keys.isEmpty());
        QCOMPARE(text, SqlRows({{QString::fromUtf8("é🙂"), QByteArray::fromHex("c3a9f09f9982")}}));
    }

    void metadataProjectionHasExpectedNullIndexSql() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath("metadata.sqlite");
        EnvironmentDatabase fixture(path);
        QVERIFY(fixture.open());
        const QString header = "CREATE TABLE subathon_catalog(singleton INTEGER PRIMARY KEY, "
                               "catalog_format INTEGER NOT NULL, store_identity TEXT NOT NULL)";
        const QString history =
            "CREATE TABLE subathon_migration_history(version INTEGER PRIMARY KEY, "
            "migration_id TEXT NOT NULL UNIQUE, content_sha256 BLOB NOT NULL)";
        QVERIFY(fixture.exec(header));
        QVERIFY(fixture.exec(history));
        SqliteWriter writer;
        QCOMPARE(writer.open(path), StorageError::none);
        SqlRows observed;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](SqliteWorkContext context) {
                auto session = context.begin();
                observed = session.execute(
                    {"SELECT type,name,tbl_name,sql FROM main.sqlite_schema ORDER BY type,name",
                     {}});
                session.commit();
                return QVariant(3);
            },
            result));
        QCOMPARE(result.error, WorkError::none);
        QCOMPARE(observed.size(), 3);
        QCOMPARE(observed[0].size(), 4);
        QCOMPARE(observed[0][0].toString(), QString("index"));
        QCOMPARE(observed[0][1].toString(),
                 QString("sqlite_autoindex_subathon_migration_history_1"));
        QCOMPARE(observed[0][2].toString(), QString("subathon_migration_history"));
        QVERIFY(observed[0][3].isNull());
        QCOMPARE(observed[1], SqlRow({"table", "subathon_catalog", "subathon_catalog", header}));
        QCOMPARE(observed[2], SqlRow({"table", "subathon_migration_history",
                                      "subathon_migration_history", history}));
    }

    void integrityFunctionRejectsOrdinaryShadow() {
        shadow("pragma_integrity_check", "value TEXT",
               "INSERT INTO pragma_integrity_check VALUES "
               "('ok')",
               1);
    }
    void foreignKeyFunctionRejectsOrdinaryShadow() {
        shadow("pragma_foreign_key_check", "value TEXT", {}, 0);
    }
    void encodingFunctionRejectsOrdinaryShadow() {
        shadow("pragma_encoding", "value TEXT", "INSERT INTO pragma_encoding VALUES ('UTF-8')", 1);
    }

    void utf16EncodingAndRawBytesAreObservedWithoutConversion() {
        for (const auto &encoding : {QString("UTF-16le"), QString("UTF-16be")}) {
            QTemporaryDir directory;
            QVERIFY(directory.isValid());
            const auto path = directory.filePath("utf16.sqlite");
            EnvironmentDatabase fixture(path);
            QVERIFY(fixture.open());
            // Fixture creation alone chooses the encoding; product runner never sets it.
            QVERIFY(fixture.exec("PRAGMA encoding='" + encoding + "'"));
            QVERIFY(fixture.exec("CREATE TABLE sentinel(label TEXT)"));
            QVERIFY(fixture.exec(QString::fromUtf8("INSERT INTO sentinel VALUES ('é🙂')")));
            SqliteWriter writer;
            QCOMPARE(writer.open(path), StorageError::none);
            SqlRows detected, raw;
            SqliteWorkResult result;
            QVERIFY(existing_work(
                writer,
                [&](SqliteWorkContext context) {
                    auto session = context.begin();
                    detected = session.execute({"SELECT * FROM pragma_encoding()", {}});
                    raw = session.execute({"SELECT CAST(label AS BLOB) FROM main.sentinel", {}});
                    session.commit();
                    return QVariant("observed only");
                },
                result));
            QCOMPARE(result.error, WorkError::none);
            QCOMPARE(detected, SqlRows({{encoding}}));
            QCOMPARE(raw, SqlRows({{QByteArray::fromHex(
                              encoding == "UTF-16le" ? "e9003dd842de" : "00e9d83dde42")}}));
            QCOMPARE(fixture.scalar("PRAGMA encoding").toString(), encoding);
            QCOMPARE(fixture.scalar("SELECT label FROM sentinel").toString(),
                     QString::fromUtf8("é🙂"));
        }
    }

    void integrityAndForeignKeyDiagnosticsAreDistinct() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath("foreign.sqlite");
        EnvironmentDatabase fixture(path);
        QVERIFY(fixture.open());
        QVERIFY(fixture.exec("PRAGMA foreign_keys=OFF"));
        QVERIFY(fixture.exec("CREATE TABLE parents(id INTEGER PRIMARY KEY)"));
        QVERIFY(fixture.exec("CREATE TABLE children(id INTEGER PRIMARY KEY, parent_id INTEGER "
                             "REFERENCES parents(id))"));
        QVERIFY(fixture.exec("INSERT INTO children VALUES (1,404)"));
        SqliteWriter writer;
        QCOMPARE(writer.open(path), StorageError::none);
        SqlRows integrity, violations;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](SqliteWorkContext context) {
                auto session = context.begin();
                integrity = session.execute({"SELECT * FROM pragma_integrity_check() LIMIT 2", {}});
                violations =
                    session.execute({"SELECT * FROM pragma_foreign_key_check() LIMIT 1", {}});
                session.commit();
                return QVariant("observed only");
            },
            result));
        QCOMPARE(result.error, WorkError::none);
        QCOMPARE(integrity, SqlRows({{QString("ok")}}));
        QCOMPARE(violations, SqlRows({{QString("children"), 1, QString("parents"), 0}}));
        QCOMPARE(fixture.scalar("SELECT COUNT(*) FROM children").toInt(), 1);
    }

  private:
    void shadow(const QString &name, const QString &columns, const QString &insert,
                qsizetype rows) {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath("shadow.sqlite");
        EnvironmentDatabase fixture(path);
        QVERIFY(fixture.open());
        QVERIFY(fixture.exec("CREATE TABLE " + name + "(" + columns + ")"));
        if (!insert.isEmpty())
            QVERIFY(fixture.exec(insert));
        SqliteWriter writer;
        QCOMPARE(writer.open(path), StorageError::none);
        SqlRows bare;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](SqliteWorkContext context) {
                auto session = context.begin();
                bare = session.execute({"SELECT * FROM " + name + " LIMIT 2", {}});
                (void)session.execute({"SELECT * FROM " + name + "() LIMIT 2", {}});
                session.commit();
                return QVariant("unreachable after function error");
            },
            result));
        QCOMPARE(bare.size(), rows);
        QCOMPARE(result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(result.error, WorkError::sql_failure);
        QVERIFY(!result.value.has_value());
        const auto count = fixture.scalar("SELECT COUNT(*) FROM " + name);
        QVERIFY(count.isValid());
        QCOMPARE(count.toLongLong(), rows);
    }
};

QTEST_GUILESS_MAIN(SqliteMigrationEnvironment)
#include "sqlite_migration_environment.moc"
