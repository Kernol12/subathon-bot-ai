// SPDX-License-Identifier: GPL-3.0-or-later
// Green environment proof: existing writer/Qt SQL only, never the new adapter/factory.
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <subathon/server/sqlite_writer.hpp>

using namespace subathon::server;
namespace {
bool existing_work(SqliteWriter &writer, SqliteWriter::Work work, SqliteWorkResult &result) {
    struct State {
        bool done{false};
        SqliteWorkResult result;
    };
    auto state = std::make_shared<State>();
    if (writer.enqueue_work(std::move(work), [state](SqliteWorkResult r) {
            state->result = std::move(r);
            state->done = true;
        }) != StorageError::none)
        return false;
    if (!QTest::qWaitFor([state] { return state->done; }, 5000))
        return false;
    result = std::move(state->result);
    return true;
}
} // namespace

class SqliteRetainedContextEnvironment final : public QObject {
    Q_OBJECT
  private:
    QJsonObject manifest_;
    void create_schema(const SqliteWorkSession &session) {
        // Expected DDL is independent literal fixture data, not a UUT factory.
        for (const auto &entry : manifest_["expected_schema"].toArray()) {
            const auto o = entry.toObject();
            if (o["name"] == "subathon_catalog" || o["name"] == "subathon_migration_history")
                (void)session.execute({o["sql"].toString(), {}});
        }
        for (const auto &sql : manifest_["sql"].toArray())
            (void)session.execute({sql.toString(), {}});
    }

  private slots:
    void initTestCase() {
        QVERIFY(QSqlDatabase::isDriverAvailable("QSQLITE"));
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
        QFile file(SUBATHON_RETAINED_VECTOR_PATH);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJsonParseError error;
        const auto json = QJsonDocument::fromJson(file.readAll(), &error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        manifest_ = json.object()["manifest"].toObject();
        QCOMPARE(manifest_["sql"].toArray().size(), 2);
        QCOMPARE(manifest_["expected_schema"].toArray().size(), 7);
    }

    void emptyNullAndOpaqueBindingsRemainDistinct() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("bindings.sqlite")), StorageError::none);
        SqlRows runtime, empty, null, binary;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto work) {
                auto session = work.begin();
                runtime = session.execute({"SELECT sqlite_version()", {}});
                const QByteArray empty_blob("", 0), null_blob;
                const auto opaque = QByteArray::fromHex("00ffefbbbf61");
                empty = session.execute(
                    {"SELECT typeof(?),length(?),hex(?)", {empty_blob, empty_blob, empty_blob}});
                null = session.execute(
                    {"SELECT typeof(?),length(?),hex(?)", {null_blob, null_blob, null_blob}});
                binary = session.execute(
                    {"SELECT typeof(?),length(?),hex(?)", {opaque, opaque, opaque}});
                session.commit();
                return QVariant(3);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QCOMPARE(runtime.size(), 1);
        QCOMPARE(runtime[0].size(), 1);
        qInfo().noquote() << "Selected Qt" << QT_VERSION_STR << "SQLite"
                          << runtime[0][0].toString();
        QCOMPARE(empty, SqlRows({{"blob", 0, ""}}));
        QCOMPARE(null.size(), 1);
        QCOMPARE(null[0].size(), 3);
        QCOMPARE(null[0][0].toString(), QString("null"));
        QVERIFY(null[0][1].isNull());
        QCOMPARE(null[0][2].toString(), QString(""));
        QCOMPARE(binary, SqlRows({{"blob", 6, "00FFEFBBBF61"}}));
    }

    void retentionMigrationShapeAndLinksUseExistingSql() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("shape.sqlite")), StorageError::none);
        SqlRows objects, settings, valid_link;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto work) {
                auto session = work.begin();
                create_schema(session);
                objects = session.execute(
                    {"SELECT type,name,tbl_name,sql FROM main.sqlite_schema ORDER BY type,name",
                     {}});
                settings = session.execute({"SELECT * FROM pragma_foreign_keys()", {}});
                (void)session.execute(
                    {"INSERT INTO main.subathon_retained_configurations VALUES (X'63',X'00FF')",
                     {}});
                (void)session.execute(
                    {"INSERT INTO main.subathon_retained_contexts VALUES (X'78',X'',X'63')", {}});
                valid_link =
                    session.execute({"SELECT * FROM pragma_foreign_key_check() LIMIT 1", {}});
                session.commit();
                return QVariant(7);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QCOMPARE(objects.size(), 7);
        const auto expected = manifest_["expected_schema"].toArray();
        for (qsizetype i = 0; i < objects.size(); ++i) {
            const auto o = expected[i].toObject();
            QCOMPARE(objects[i].size(), 4);
            QCOMPARE(objects[i][0].toString(), o["type"].toString());
            QCOMPARE(objects[i][1].toString(), o["name"].toString());
            QCOMPARE(objects[i][2].toString(), o["table_name"].toString());
            if (o["sql"].isNull())
                QVERIFY(objects[i][3].isNull());
            else
                QCOMPARE(objects[i][3].toString(), o["sql"].toString());
        }
        QCOMPARE(settings, SqlRows({{1}}));
        QVERIFY(valid_link.isEmpty());
        SqliteWorkResult orphan;
        QVERIFY(existing_work(
            writer,
            [](auto work) {
                auto session = work.begin();
                (void)session.execute(
                    {"INSERT INTO main.subathon_retained_contexts VALUES (X'79',X'',X'4040')", {}});
                session.commit();
                return QVariant(1);
            },
            orphan));
        QCOMPARE(orphan.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(orphan.error, WorkError::sql_failure);
        QVERIFY(!orphan.value);
    }

    void selectedEmptyAndOptionalValuesKeepTheirSqlTypes() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("selected.sqlite")), StorageError::none);
        SqlRows selected, raw;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto work) {
                auto session = work.begin();
                create_schema(session);
                (void)session.execute({"INSERT INTO main.subathon_retained_configurations VALUES "
                                       "(X'EFBBBF6100FF',X'')",
                                       {}});
                (void)session.execute(
                    {"INSERT INTO main.subathon_retained_contexts VALUES (X'00FF',X'',NULL)", {}});
                selected = session.execute(
                    {"SELECT typeof(reference),hex(reference),typeof(document),length(document),"
                     "typeof(configuration_ref),length(configuration_ref) "
                     "FROM main.subathon_retained_contexts",
                     {}});
                raw = session.execute(
                    {"SELECT reference,document,typeof(document),length(document) FROM "
                     "main.subathon_retained_configurations",
                     {}});
                session.commit();
                return QVariant(2);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QCOMPARE(selected.size(), 1);
        QCOMPARE(selected[0].size(), 6);
        QCOMPARE(selected[0][0].toString(), QString("blob"));
        QCOMPARE(selected[0][1].toString(), QString("00FF"));
        QCOMPARE(selected[0][2].toString(), QString("blob"));
        QCOMPARE(selected[0][3].toInt(), 0);
        QCOMPARE(selected[0][4].toString(), QString("null"));
        QVERIFY(selected[0][5].isNull());
        QCOMPARE(raw.size(), 1);
        QCOMPARE(raw[0].size(), 4);
        QCOMPARE(raw[0][0].toByteArray(), QByteArray::fromHex("efbbbf6100ff"));
        QVERIFY(raw[0][1].toByteArray().isEmpty());
        // Driver empty-buffer nullness is deliberately not asserted. SQL type/length
        // prove presence; the new adapter must normalize its returned value.
        QCOMPARE(raw[0][2].toString(), QString("blob"));
        QCOMPARE(raw[0][3].toInt(), 0);
        for (const auto &sql :
             {QString("INSERT INTO main.subathon_retained_configurations VALUES (X'01',NULL)"),
              QString("INSERT INTO main.subathon_retained_configurations VALUES ('text',X'')"),
              QString("INSERT INTO main.subathon_retained_contexts VALUES (X'02',X'',X'')")}) {
            SqliteWorkResult rejected;
            QVERIFY(existing_work(
                writer,
                [sql](auto work) {
                    auto session = work.begin();
                    (void)session.execute({sql, {}});
                    session.commit();
                    return QVariant(1);
                },
                rejected));
            QCOMPARE(rejected.transaction, TransactionOutcome::rolled_back);
            QCOMPARE(rejected.error, WorkError::sql_failure);
            QVERIFY(!rejected.value);
        }
    }
};
QTEST_GUILESS_MAIN(SqliteRetainedContextEnvironment)
#include "sqlite_retained_context_environment.moc"
