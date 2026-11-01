// SPDX-License-Identifier: GPL-3.0-or-later
// Green foundation proof only: existing private writer and independent literals.
// Never includes/calls/links the new receipt adapter, manifest factory or codec.
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <limits>
#include <memory>
#include <subathon/server/sqlite_writer.hpp>

using namespace subathon::server;
namespace {
bool existing_work(SqliteWriter &writer, SqliteWriter::Work body, SqliteWorkResult &result) {
    struct State {
        bool done{false};
        SqliteWorkResult result;
    };
    auto state = std::make_shared<State>();
    if (writer.enqueue_work(std::move(body), [state](SqliteWorkResult value) {
            state->result = std::move(value);
            state->done = true;
        }) != StorageError::none)
        return false;
    if (!QTest::qWaitFor([state] { return state->done; }, 5000))
        return false;
    result = std::move(state->result);
    return true;
}

const QString original_insert =
    "INSERT INTO main.subathon_receipts(receipt_id,provider,source_account,identifier_namespace,"
    "opaque_id,event_type,schema_version,source_timestamp_utc_ms,server_timestamp_utc_ms,"
    "connection_ref,configuration_ref,modifier_context_ref) VALUES (?,?,?,?,?,?,?,?,?,?,?,?)";
} // namespace

class SqliteReceiptsEnvironment final : public QObject {
    Q_OBJECT
  private:
    QJsonObject prefix_, manifest_;
    QJsonArray codec_;

    void create_schema(const SqliteWorkSession &session) {
        // Prefix1 is the unchanged independently frozen retention fixture. These
        // are literal schema/history writes, never a production migration call.
        for (const auto &entry : prefix_["expected_schema"].toArray()) {
            const auto object = entry.toObject();
            if (object["name"] == "subathon_catalog" ||
                object["name"] == "subathon_migration_history")
                (void)session.execute({object["sql"].toString(), {}});
        }
        for (const auto &sql : prefix_["sql"].toArray())
            (void)session.execute({sql.toString(), {}});
        for (const auto &sql : manifest_["sql"].toArray())
            (void)session.execute({sql.toString(), {}});
        (void)session.execute({"INSERT INTO main.subathon_catalog VALUES "
                               "(1,1,'subathon.sqlite-store')",
                               {}});
        (void)session.execute(
            {"INSERT INTO main.subathon_migration_history VALUES "
             "(1,'retain-config-context-v1',?)",
             {QByteArray::fromHex(
                 "0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e")}});
        (void)session.execute(
            {"INSERT INTO main.subathon_migration_history VALUES "
             "(2,'identified-receipts-v1',?)",
             {QByteArray::fromHex(
                 "24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2")}});
    }

    void seed_targets(const SqliteWorkSession &session) {
        (void)session.execute({"INSERT INTO main.subathon_retained_configurations VALUES "
                               "(X'61',X''),(X'62',X'00FF')",
                               {}});
        // Context x knows configuration a, while receipt arrivals below name b.
        // Both resolve; no promised port rule requires those references to match.
        (void)session.execute({"INSERT INTO main.subathon_retained_contexts VALUES "
                               "(X'78',X'EFBBBF6100FF',X'61')",
                               {}});
    }

  private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
        QFile receipt_file(SUBATHON_RECEIPT_VECTOR_PATH),
            retained_file(SUBATHON_RETAINED_VECTOR_PATH);
        QVERIFY(receipt_file.open(QIODevice::ReadOnly));
        QVERIFY(retained_file.open(QIODevice::ReadOnly));
        QJsonParseError error;
        const auto receipts = QJsonDocument::fromJson(receipt_file.readAll(), &error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        const auto retained = QJsonDocument::fromJson(retained_file.readAll(), &error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        manifest_ = receipts.object()["manifest"].toObject();
        prefix_ = retained.object()["manifest"].toObject();
        codec_ = receipts.object()["codec_vectors"].toArray();
        QCOMPARE(prefix_["id"].toString(), QString("retain-config-context-v1"));
        QCOMPARE(prefix_["payload_bytes"].toInt(), 2000);
        QCOMPARE(prefix_["expected_schema"].toArray().size(), 7);
        QCOMPARE(manifest_["version"].toInt(), 2);
        QCOMPARE(manifest_["id"].toString(), QString("identified-receipts-v1"));
        QCOMPARE(manifest_["sql"].toArray().size(), 4);
        QCOMPARE(manifest_["expected_schema"].toArray().size(), 13);
        QCOMPARE(manifest_["supported_starting_versions"].toArray(), QJsonArray({0, 1, 2}));
        QCOMPARE(codec_.size(), 6);
        const auto payload = QByteArray::fromHex(manifest_["payload_hex"].toString().toLatin1());
        QCOMPARE(payload.size(), 10535);
        QCOMPARE(manifest_["payload_bytes"].toInt(), 10535);
        QCOMPARE(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex(),
                 QByteArray("24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2"));
    }

    void receiptSchemaPrefixMatchesLiteralProjection() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("schema.sqlite")), StorageError::none);
        SqlRows schema, allocator, header, history, runtime;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                create_schema(session);
                runtime = session.execute({"SELECT sqlite_version()", {}});
                schema = session.execute({"SELECT type,name,tbl_name,sql,typeof(sql) FROM "
                                          "main.sqlite_schema ORDER BY type,name",
                                          {}});
                allocator =
                    session.execute({"SELECT singleton,typeof(singleton),last_allocated,"
                                     "typeof(last_allocated) FROM main.subathon_receipt_allocator",
                                     {}});
                header = session.execute({"SELECT singleton,catalog_format,store_identity FROM "
                                          "main.subathon_catalog",
                                          {}});
                history =
                    session.execute({"SELECT version,migration_id,hex(content_sha256),"
                                     "typeof(content_sha256) FROM main.subathon_migration_history "
                                     "ORDER BY version",
                                     {}});
                session.commit();
                return QVariant(13);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QVERIFY(result.value.has_value());
        QCOMPARE(runtime.size(), 1);
        QCOMPARE(runtime[0].size(), 1);
        qInfo().noquote() << "Selected Qt" << QT_VERSION_STR << "SQLite"
                          << runtime[0][0].toString();
        QCOMPARE(schema.size(), 13);
        const auto expected = manifest_["expected_schema"].toArray();
        for (qsizetype i = 0; i < schema.size(); ++i) {
            const auto object = expected[i].toObject();
            QCOMPARE(schema[i].size(), 5);
            QCOMPARE(schema[i][0].toString(), object["type"].toString());
            QCOMPARE(schema[i][1].toString(), object["name"].toString());
            QCOMPARE(schema[i][2].toString(), object["table_name"].toString());
            if (object["sql"].isNull()) {
                QVERIFY(schema[i][3].isNull());
                QCOMPARE(schema[i][4].toString(), QString("null"));
            } else {
                QCOMPARE(schema[i][3].toString(), object["sql"].toString());
                QCOMPARE(schema[i][4].toString(), QString("text"));
            }
        }
        QCOMPARE(allocator.size(), 1);
        QCOMPARE(allocator[0].size(), 4);
        QCOMPARE(allocator[0][0].toLongLong(), 1);
        QCOMPARE(allocator[0][1].toString(), QString("integer"));
        QVERIFY(allocator[0][2].isNull());
        QCOMPARE(allocator[0][3].toString(), QString("null"));
        QCOMPARE(header, SqlRows({{1, 1, "subathon.sqlite-store"}}));
        QCOMPARE(
            history,
            SqlRows(
                {{1, "retain-config-context-v1",
                  "0906F121C3BA1193CDEA41A70F1C91013724EA90416B0F1D04B6C23D8F091F5E", "blob"},
                 {2, "identified-receipts-v1",
                  "24614F68E9C41A29751EA9EF6BA08FCF655B1CDB0E0C097CB1601BB1581553A2", "blob"}}));
    }

    void unsignedIdsAndSignedFieldsRemainExact() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("numbers.sqlite")), StorageError::none);
        const auto minimum = std::numeric_limits<qlonglong>::min();
        const auto maximum = std::numeric_limits<qlonglong>::max();
        SqlRows sorted_ids, fields;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                create_schema(session);
                seed_targets(session);
                QList<QVariant> ids;
                for (const auto &entry : codec_)
                    ids.append(QByteArray::fromHex(entry.toObject()["hex"].toString().toLatin1()));
                sorted_ids = session.execute(
                    {"SELECT typeof(id),length(id),hex(id) FROM (SELECT ? AS id UNION ALL SELECT ? "
                     "UNION ALL SELECT ? UNION ALL SELECT ? UNION ALL SELECT ? UNION ALL SELECT ?) "
                     "ORDER BY id",
                     ids});
                // Construct U+FEFF without a decoder, then bind the exact byte array,
                // including NUL/FF. Receipt byte fields must never bind QString text.
                auto opaque = (QString(QChar(0xfeff)) + 'a').toUtf8();
                opaque.append('\0');
                opaque.append(char(0xff));
                const QList<QVariant> common{QByteArray("p"), QByteArray("a"), QByteArray("id")};
                const QList<QByteArray> keys{QByteArray::fromHex("7fffffffffffffff"),
                                             QByteArray::fromHex("8000000000000000"),
                                             QByteArray::fromHex("ffffffffffffffff")};
                for (qsizetype i = 0; i < keys.size(); ++i) {
                    QList<QVariant> values{keys[i]};
                    values.append(common);
                    values.append({keys[i], opaque, qlonglong(i == 0 ? 0 : 4294967295LL),
                                   i == 0   ? minimum
                                   : i == 1 ? maximum
                                            : qlonglong(0),
                                   i == 0   ? maximum
                                   : i == 1 ? minimum
                                            : qlonglong(0),
                                   QByteArray("", 0), QByteArray("b"), QByteArray("x")});
                    (void)session.execute({original_insert, values});
                }
                fields = session.execute(
                    {"SELECT hex(receipt_id),typeof(receipt_id),length(receipt_id),schema_version,"
                     "typeof(schema_version),source_timestamp_utc_ms,typeof(source_timestamp_utc_"
                     "ms),"
                     "server_timestamp_utc_ms,typeof(server_timestamp_utc_ms),typeof(event_type),"
                     "hex(event_type),typeof(connection_ref),length(connection_ref) "
                     "FROM main.subathon_receipts ORDER BY receipt_id",
                     {}});
                session.commit();
                return QVariant(3);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QVERIFY(result.value.has_value());
        QCOMPARE(sorted_ids, SqlRows({{"blob", 8, "0000000000000000"},
                                      {"blob", 8, "0000000000000001"},
                                      {"blob", 8, "0020000000000001"},
                                      {"blob", 8, "7FFFFFFFFFFFFFFF"},
                                      {"blob", 8, "8000000000000000"},
                                      {"blob", 8, "FFFFFFFFFFFFFFFF"}}));
        QCOMPARE(
            fields,
            SqlRows(
                {{"7FFFFFFFFFFFFFFF", "blob", 8, qlonglong(0), "integer", minimum, "integer",
                  maximum, "integer", "blob", "EFBBBF6100FF", "blob", 0},
                 {"8000000000000000", "blob", 8, qlonglong(4294967295LL), "integer", maximum,
                  "integer", minimum, "integer", "blob", "EFBBBF6100FF", "blob", 0},
                 {"FFFFFFFFFFFFFFFF", "blob", 8, qlonglong(4294967295LL), "integer", qlonglong(0),
                  "integer", qlonglong(0), "integer", "blob", "EFBBBF6100FF", "blob", 0}}));
        for (const auto &id : {QVariant(QByteArray::fromHex("0000000000000000")),
                               QVariant(qlonglong(1)), QVariant{}}) {
            QVERIFY(existing_work(
                writer,
                [&](auto context) {
                    auto session = context.begin();
                    (void)session.execute(
                        {original_insert,
                         {id, QByteArray("p"), QByteArray("a"), QByteArray("id"), QByteArray("new"),
                          QByteArray("event"), 0, QVariant{}, qlonglong(0), QByteArray("", 0),
                          QByteArray("b"), QByteArray("x")}});
                    session.commit();
                    return QVariant(0);
                },
                result));
            QCOMPARE(result.transaction, TransactionOutcome::rolled_back);
            QCOMPARE(result.error, WorkError::sql_failure);
            QVERIFY(!result.value);
        }
    }

    void optionalGroupsAndTypedReferencesUseLiteralSchema() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("optional.sqlite")), StorageError::none);
        SqlRows groups, attempts, references, foreign_keys;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                create_schema(session);
                seed_targets(session);
                (void)session.execute(
                    {"INSERT INTO main.subathon_receipts(receipt_id,provider,source_account,"
                     "identifier_namespace,opaque_id,event_type,schema_version,source_timestamp_"
                     "utc_ms,"
                     "actor_id,actor_display_name,money_original_decimal,money_currency,"
                     "payment_namespace,payment_opaque_id,activity_namespace,activity_opaque_id,"
                     "server_timestamp_utc_ms,connection_ref,configuration_ref,modifier_context_"
                     "ref) "
                     "VALUES (X'0000000000000001',X'70',X'61',X'6964',X'31',X'EFBBBF6100FF',"
                     "4294967295,NULL,NULL,X'',?,X'757364',NULL,NULL,X'',X'',17,X'',X'62',X'78')",
                     {QByteArray("9007199254740993.00000000000000000001")}});
                (void)session.execute(
                    {"INSERT INTO main.subathon_receipt_attempts(receipt_id,ordinal,event_type,"
                     "schema_version,source_timestamp_utc_ms,actor_id,actor_display_name,"
                     "money_original_decimal,money_currency,payment_namespace,payment_opaque_id,"
                     "activity_namespace,activity_opaque_id,server_timestamp_utc_ms,connection_ref,"
                     "configuration_ref,modifier_context_ref,disposition) VALUES "
                     "(X'0000000000000001',1,X'',0,0,X'',NULL,NULL,NULL,X'',X'',NULL,NULL,17,X'',X'"
                     "62',X'78',0),"
                     "(X'0000000000000001',2,X'00FF',4294967295,NULL,NULL,X'',X'',X'',"
                     "X'EFBBBF61',X'00FF',X'00FF',X'EFBBBF61',17,X'00FF',X'62',X'78',1)",
                     {}});
                const QString projection =
                    "typeof(source_timestamp_utc_ms),typeof(actor_id),hex(actor_id),"
                    "typeof(actor_display_name),length(actor_display_name),"
                    "typeof(money_original_decimal),hex(money_original_decimal),typeof(money_"
                    "currency),hex(money_currency),"
                    "typeof(payment_namespace),hex(payment_namespace),typeof(payment_opaque_id),"
                    "hex(payment_opaque_id),"
                    "typeof(activity_namespace),hex(activity_namespace),typeof(activity_opaque_id),"
                    "hex(activity_opaque_id)";
                groups = session.execute({"SELECT 0 AS ordinal," + projection +
                                              " FROM main.subathon_receipts "
                                              "UNION ALL SELECT ordinal," +
                                              projection +
                                              " FROM main.subathon_receipt_attempts "
                                              "ORDER BY ordinal",
                                          {}});
                attempts = session.execute({"SELECT "
                                            "ordinal,server_timestamp_utc_ms,typeof(ordinal),"
                                            "typeof(server_timestamp_utc_ms),"
                                            "disposition,typeof(disposition),hex(receipt_id) FROM "
                                            "main.subathon_receipt_attempts "
                                            "ORDER BY ordinal",
                                            {}});
                references = session.execute(
                    {"SELECT "
                     "hex(r.configuration_ref),hex(r.modifier_context_ref),hex(c.configuration_ref)"
                     ","
                     "typeof(r.configuration_ref),typeof(r.modifier_context_ref),typeof(c."
                     "configuration_ref) "
                     "FROM main.subathon_receipts AS r JOIN main.subathon_retained_contexts AS c "
                     "ON r.modifier_context_ref=c.reference",
                     {}});
                foreign_keys =
                    session.execute({"SELECT * FROM pragma_foreign_key_check() LIMIT 1", {}});
                session.commit();
                return QVariant(2);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QVERIFY(result.value.has_value());
        QCOMPARE(groups.size(), 3);
        for (const auto &row : groups)
            QCOMPARE(row.size(), 18);
        QVERIFY(!groups[0][5].isNull());
        QCOMPARE(groups[0][5].toLongLong(), 0);
        QVERIFY(groups[1][5].isNull());
        QVERIFY(!groups[2][5].isNull());
        QCOMPARE(groups[2][5].toLongLong(), 0);
        // SQL NULL may have a driver-specific QVariant type. Its nullness and
        // both actual empty lengths are proven above; compare all other literals.
        for (auto &row : groups)
            row.removeAt(5);
        QCOMPARE(
            groups,
            SqlRows({{0, "null", "null", "", "blob", "blob",
                      "393030373139393235343734303939332E3030303030303030303030303030303030303031",
                      "blob", "757364", "null", "", "null", "", "blob", "", "blob", ""},
                     {1, "integer", "blob", "", "null", "null", "", "null", "", "blob", "", "blob",
                      "", "null", "", "null", ""},
                     {2, "null", "null", "", "blob", "blob", "", "blob", "", "blob", "EFBBBF61",
                      "blob", "00FF", "blob", "00FF", "blob", "EFBBBF61"}}));
        QCOMPARE(attempts,
                 SqlRows({{1, 17, "integer", "integer", 0, "integer", "0000000000000001"},
                          {2, 17, "integer", "integer", 1, "integer", "0000000000000001"}}));
        QCOMPARE(references, SqlRows({{"62", "78", "61", "blob", "blob", "blob"}}));
        QVERIFY(foreign_keys.isEmpty());
        for (const auto &sql :
             {QString("UPDATE main.subathon_receipts SET money_currency=NULL"),
              QString("UPDATE main.subathon_receipts SET payment_namespace=X''"),
              QString("UPDATE main.subathon_receipts SET activity_opaque_id=NULL"),
              QString("UPDATE main.subathon_receipts SET actor_id='text'"),
              QString("UPDATE main.subathon_receipts SET configuration_ref=X'78'"),
              QString("UPDATE main.subathon_receipts SET modifier_context_ref=X'62'")}) {
            QVERIFY(existing_work(
                writer,
                [&](auto context) {
                    auto session = context.begin();
                    (void)session.execute({sql, {}});
                    session.commit();
                    return QVariant(0);
                },
                result));
            QCOMPARE(result.transaction, TransactionOutcome::rolled_back);
            QCOMPARE(result.error, WorkError::sql_failure);
            QVERIFY(!result.value);
        }
    }
};
QTEST_GUILESS_MAIN(SqliteReceiptsEnvironment)
#include "sqlite_receipts_environment.moc"
