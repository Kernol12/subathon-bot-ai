// SPDX-License-Identifier: GPL-3.0-or-later
// Independent SQL foundation: existing writer + literal fixtures only. No plan API/library.
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
#include <stdexcept>
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
QByteArray literal_bytes(const QJsonValue &value) {
    const auto bytes = QByteArray::fromHex(value.toString().toLatin1());
    return bytes.isEmpty() ? QByteArray("", 0) : bytes;
}
QVariant optional_bytes(const QJsonValue &value) {
    return value.isNull() ? QVariant{} : QVariant(literal_bytes(value));
}
QVariant optional_signed(const QJsonValue &value) {
    if (value.isNull())
        return {};
    bool ok = false;
    const auto number = value.toString().toLongLong(&ok);
    if (!ok)
        throw std::runtime_error("Invalid independent signed64 fixture");
    return QVariant(number);
}
QByteArray frame(const QByteArray &bytes) { return QByteArray::number(bytes.size()) + ':' + bytes; }
const auto one = QByteArray::fromHex("0000000000000001");
const auto two = QByteArray::fromHex("0000000000000002");
const auto zero = QByteArray::fromHex("0000000000000000");
const QString header_insert =
    "INSERT INTO main.subathon_plan_headers VALUES (?,?,?,X'726F6F74',?,?,?)";
const QString revision_insert =
    "INSERT INTO main.subathon_plan_revisions(plan_ref,revision,parent_revision,payment_evidence,"
    "monetary,missing_rate,review,evidence_policy,policy_ref,application,reversal,settlement,"
    "valuation_state,override_authority,override_actor_ref,override_reason,override_decision_ref) "
    "VALUES (?,?,?,0,1,0,0,0,X'',0,0,0,0,?,?,?,?)";
void insert_revision(const SqliteWorkSession &session, const QByteArray &plan,
                     const QVariant &revision, const QVariant &parent = {}) {
    const bool later = !parent.isNull();
    (void)session.execute({revision_insert,
                           {plan, revision, parent, later ? QVariant(2) : QVariant{},
                            later ? QVariant(QByteArray("actor")) : QVariant{},
                            later ? QVariant(QByteArray("reason")) : QVariant{},
                            later ? QVariant(QByteArray("decision")) : QVariant{}}});
}
const QStringList historical_columns{"historical_resolution_ref",
                                     "historical_original_decimal",
                                     "historical_currency",
                                     "comparison_currency",
                                     "effective_rate_decimal",
                                     "fetched_rate_decimal",
                                     "snapshot_ref",
                                     "rate_source_ref",
                                     "rate_timestamp_utc_ms",
                                     "fetch_timestamp_utc_ms",
                                     "manual_decision_ref"};
} // namespace

class SqliteApplicationPlansEnvironment final : public QObject {
    Q_OBJECT
  private:
    QJsonObject retained_, receipts_, contributions_, fixture_, manifest_;
    QJsonArray uint64_, exact_, historical_;

    void create_literal_schema(const SqliteWorkSession &session) const {
        for (const auto &entry : retained_["expected_schema"].toArray()) {
            const auto object = entry.toObject();
            if (object["name"] == "subathon_catalog" ||
                object["name"] == "subathon_migration_history")
                (void)session.execute({object["sql"].toString(), {}});
        }
        for (const auto &manifest : {retained_, receipts_, contributions_, manifest_})
            for (const auto &sql : manifest["sql"].toArray())
                (void)session.execute({sql.toString(), {}});
        (void)session.execute(
            {"INSERT INTO main.subathon_catalog VALUES (1,1,'subathon.sqlite-store')", {}});
        (void)session.execute(
            {"INSERT INTO main.subathon_migration_history VALUES "
             "(1,'retain-config-context-v1',?),(2,'identified-receipts-v1',?),"
             "(3,'contribution-evidence-v1',?),(4,'captured-application-plans-v1',?)",
             {literal_bytes(retained_["sha256_hex"]), literal_bytes(receipts_["sha256_hex"]),
              literal_bytes(contributions_["sha256_hex"]),
              literal_bytes(manifest_["sha256_hex"])}});
    }
    // Disposable physical targets, not proof of normalizer/relationship/permission semantics.
    void seed_sources(const SqliteWorkSession &session) const {
        (void)session.execute({"INSERT INTO main.subathon_retained_configurations VALUES "
                               "(X'61',X''),(X'62',X'00FF')",
                               {}});
        (void)session.execute({"INSERT INTO main.subathon_retained_contexts VALUES "
                               "(X'78',X'EFBBBF6100FF',X'61')",
                               {}});
        for (const auto &id : {one, two}) {
            (void)session.execute(
                {"INSERT INTO main.subathon_receipts(receipt_id,provider,source_account,"
                 "identifier_namespace,opaque_id,event_type,schema_version,money_original_decimal,"
                 "money_currency,server_timestamp_utc_ms,connection_ref,configuration_ref,"
                 "modifier_context_ref) VALUES (?,X'70',X'61',X'6E',?,X'65',0,?,X'455552',0,X'',"
                 "X'62',X'78')",
                 {id, id, QByteArray("9007199254740993.00000000000000000001")}});
            (void)session.execute(
                {"INSERT INTO main.subathon_receipt_attempts(receipt_id,ordinal,event_type,"
                 "schema_version,server_timestamp_utc_ms,connection_ref,configuration_ref,"
                 "modifier_context_ref,disposition) VALUES (?,1,X'65',0,0,X'',X'62',X'78',0)",
                 {id}});
        }
        (void)session.execute(
            {"INSERT INTO main.subathon_contributions(contribution_id,original_receipt_id,"
             "payment_evidence,monetary,missing_rate,review,evidence_policy,policy_ref,application,"
             "reversal,settlement) VALUES (?, ?,0,1,0,0,0,X'',0,0,0)",
             {zero, one}});
        (void)session.execute(
            {"INSERT INTO main.subathon_contribution_observations(receipt_id,contribution_id,kind,"
             "evidence,support_monetary,support_missing_rate,support_review,support_evidence_"
             "policy,"
             "support_policy_ref,support_application,support_reversal,support_settlement) "
             "VALUES (?,?,0,0,1,0,0,0,X'',0,0,0)",
             {one, zero}});
        (void)session.execute({"UPDATE main.subathon_receipt_allocator SET last_allocated=? "
                               "WHERE singleton=1",
                               {two}});
        (void)session.execute({"UPDATE main.subathon_contribution_allocator SET last_allocated=? "
                               "WHERE singleton=1",
                               {zero}});
    }

  private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
        const auto read = [](const char *path, QJsonParseError &error) {
            QFile file(QString::fromUtf8(path));
            if (!file.open(QIODevice::ReadOnly))
                return QJsonDocument{};
            return QJsonDocument::fromJson(file.readAll(), &error);
        };
        QJsonParseError error;
        const auto retained = read(SUBATHON_RETAINED_VECTOR_PATH, error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        QVERIFY(retained.isObject());
        const auto receipts = read(SUBATHON_RECEIPT_VECTOR_PATH, error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        QVERIFY(receipts.isObject());
        const auto contributions = read(SUBATHON_CONTRIBUTION_VECTOR_PATH, error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        QVERIFY(contributions.isObject());
        const auto plans = read(SUBATHON_PLAN_VECTOR_PATH, error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        QVERIFY(plans.isObject());
        retained_ = retained.object()["manifest"].toObject();
        receipts_ = receipts.object()["manifest"].toObject();
        contributions_ = contributions.object()["manifest"].toObject();
        fixture_ = plans.object();
        manifest_ = fixture_["manifest"].toObject();
        uint64_ = fixture_["uint64_vectors"].toArray();
        exact_ = fixture_["exact_vectors"].toArray();
        historical_ = fixture_["historical_vectors"].toArray();
        QCOMPARE(retained_["expected_schema"].toArray().size(), 7);
        QCOMPARE(receipts_["expected_schema"].toArray().size(), 13);
        QCOMPARE(contributions_["expected_schema"].toArray().size(), 20);
        QCOMPARE(manifest_["expected_schema"].toArray().size(), 30);
        QCOMPARE(manifest_["catalog_format"].toInt(), 1);
        QCOMPARE(manifest_["store_identity"].toString(), QString("subathon.sqlite-store"));
        QCOMPARE(manifest_["version"].toInt(), 4);
        QCOMPARE(manifest_["id"].toString(), QString("captured-application-plans-v1"));
        QCOMPARE(manifest_["supported_starting_versions"].toArray(), QJsonArray({0, 1, 2, 3, 4}));
        QCOMPARE(manifest_["sql"].toArray().size(), 5);
        for (const auto &prefix : {retained_, receipts_, contributions_})
            for (const auto &object : prefix["expected_schema"].toArray())
                QVERIFY(manifest_["expected_schema"].toArray().contains(object));
        // Independent framing consumes literals, never a new catalog/plan producer.
        QByteArray payload = frame("migration-catalog-v1") + frame("4") +
                             frame("captured-application-plans-v1") + frame("5");
        for (const auto &sql : manifest_["sql"].toArray())
            payload += frame(sql.toString().toUtf8());
        payload += frame("30");
        for (const auto &entry : manifest_["expected_schema"].toArray()) {
            const auto object = entry.toObject();
            for (const auto &field : {"type", "name", "table_name"})
                payload += frame(object[field].toString().toUtf8());
            payload += frame(object["sql"].isNull() ? "0" : "1");
            if (!object["sql"].isNull())
                payload += frame(object["sql"].toString().toUtf8());
        }
        QCOMPARE(payload, literal_bytes(manifest_["payload_hex"]));
        QCOMPARE(payload.size(), 29015);
        QCOMPARE(manifest_["payload_bytes"].toInt(), 29015);
        QCOMPARE(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex(),
                 QByteArray("bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50"));
        QCOMPARE(uint64_.size(), 6);
        QCOMPARE(exact_.size(), 4);
        QCOMPARE(historical_.size(), 2);
        for (const auto &entry : uint64_) {
            QVERIFY(entry.toObject()["decimal"].isString());
            QVERIFY(entry.toObject()["hex"].isString());
        }
        for (const auto &entry : exact_) {
            const auto value = entry.toObject();
            QCOMPARE(value["numerator"].toString().toLatin1(),
                     literal_bytes(value["numerator_hex"]));
            QCOMPARE(value["denominator"].toString().toLatin1(),
                     literal_bytes(value["denominator_hex"]));
        }
        QVERIFY(!fixture_["sparse_history"].toObject()["complete_history_valid"].toBool(true));
    }

    void planSchemaPrefixMatchesLiteralProjection() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("schema.sqlite")), StorageError::none);
        SqlRows schema, history, columns, allocators, runtime;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                create_literal_schema(session);
                schema = session.execute({"SELECT type,name,tbl_name,sql,typeof(sql) FROM "
                                          "main.sqlite_schema ORDER BY type,name",
                                          {}});
                history =
                    session.execute({"SELECT version,migration_id,typeof(content_sha256),"
                                     "hex(content_sha256) FROM main.subathon_migration_history "
                                     "ORDER BY version",
                                     {}});
                for (const auto &table :
                     {"subathon_plan_headers", "subathon_plan_revisions", "subathon_plan_rules",
                      "subathon_plan_modifiers", "subathon_plan_requests"}) {
                    const auto row = session.execute({"SELECT COUNT(*) FROM pragma_table_info(?)",
                                                      {QString::fromLatin1(table)}});
                    if (row.size() != 1 || row[0].size() != 1)
                        throw std::runtime_error("Missing literal plan table");
                    columns.append(row[0]);
                }
                allocators = session.execute({"SELECT 'receipt',typeof(last_allocated) FROM "
                                              "main.subathon_receipt_allocator UNION ALL SELECT "
                                              "'contribution',typeof(last_allocated) FROM "
                                              "main.subathon_contribution_allocator ORDER BY 1",
                                              {}});
                runtime = session.execute({"SELECT sqlite_version()", {}});
                session.commit();
                return QVariant(30);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QVERIFY(result.value);
        QCOMPARE(schema.size(), 30);
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
        QCOMPARE(columns, SqlRows({{7}, {34}, {7}, {10}, {14}}));
        QCOMPARE(allocators, SqlRows({{"contribution", "null"}, {"receipt", "null"}}));
        QCOMPARE(history,
                 SqlRows({{1, "retain-config-context-v1", "blob",
                           "0906F121C3BA1193CDEA41A70F1C91013724EA90416B0F1D04B6C23D8F091F5E"},
                          {2, "identified-receipts-v1", "blob",
                           "24614F68E9C41A29751EA9EF6BA08FCF655B1CDB0E0C097CB1601BB1581553A2"},
                          {3, "contribution-evidence-v1", "blob",
                           "B72249BA22C4838AF98B9AE29600E70969CF3295EEA958A30074167221D2AD1D"},
                          {4, "captured-application-plans-v1", "blob",
                           "BBB8140119B29F54F419479851D4FDF42B6A928F5451E4DE756389D2BC359C50"}}));
        QCOMPARE(runtime.size(), 1);
        QCOMPARE(runtime[0].size(), 1);
        qInfo().noquote() << "Selected Qt" << QT_VERSION_STR << "SQLite"
                          << runtime[0][0].toString();
    }

    void unsignedRevisionFieldsAndSparseHistoryStayExplicit() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("revisions.sqlite")), StorageError::none);
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                create_literal_schema(session);
                seed_sources(session);
                session.commit();
                return QVariant(0);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        SqlRows vectors, selected, anchor, summary, links, foreign_keys;
        const auto sparse = fixture_["sparse_history"].toObject();
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                const auto head = literal_bytes(sparse["head_hex"]);
                (void)session.execute(
                    {header_insert,
                     {QByteArray("p"), QByteArray("event"), QVariant{}, two, zero, head}});
                for (const auto &entry : sparse["revisions"].toArray()) {
                    const auto row = entry.toObject();
                    insert_revision(session, QByteArray("p"), literal_bytes(row["hex"]),
                                    optional_bytes(row["parent_hex"]));
                }
                (void)session.execute({header_insert,
                                       {QByteArray("no-cid"), QByteArray("event"),
                                        QByteArray("", 0), two, QVariant{}, one}});
                insert_revision(session, QByteArray("no-cid"), one);
                QList<QVariant> bytes;
                for (const auto &entry : uint64_)
                    bytes.append(literal_bytes(entry.toObject()["hex"]));
                vectors =
                    session.execute({"SELECT typeof(id),length(id),hex(id) FROM (SELECT ? AS id "
                                     "UNION ALL SELECT ? UNION ALL SELECT ? UNION ALL SELECT ? "
                                     "UNION ALL SELECT ? UNION ALL SELECT ?) ORDER BY id",
                                     bytes});
                selected = session.execute(
                    {"SELECT typeof(revision),length(revision),hex(revision),"
                     "typeof(parent_revision),length(parent_revision),hex(parent_revision) "
                     "FROM main.subathon_plan_revisions WHERE plan_ref=X'70' AND revision=?",
                     {head}});
                anchor = session.execute(
                    {"SELECT typeof(parent_revision),typeof(override_authority) "
                     "FROM main.subathon_plan_revisions WHERE plan_ref=X'70' AND revision=?",
                     {one}});
                summary = session.execute({"SELECT COUNT(*),hex(MIN(revision)),hex(MAX(revision)),"
                                           "(SELECT hex(head_revision) FROM "
                                           "main.subathon_plan_headers WHERE plan_ref=X'70') "
                                           "FROM main.subathon_plan_revisions WHERE plan_ref=X'70'",
                                           {}});
                links = session.execute(
                    {"SELECT hex(plan_ref),typeof(parent_event_ref),length(parent_event_ref),"
                     "typeof(contribution_id),length(contribution_id),hex(contribution_id),"
                     "hex(receipt_id) FROM main.subathon_plan_headers ORDER BY plan_ref",
                     {}});
                foreign_keys =
                    session.execute({"SELECT * FROM pragma_foreign_key_check() LIMIT 1", {}});
                session.commit();
                return QVariant(3);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QVERIFY(result.value);
        QCOMPARE(vectors, SqlRows({{"blob", 8, "0000000000000000"},
                                   {"blob", 8, "0000000000000001"},
                                   {"blob", 8, "0020000000000001"},
                                   {"blob", 8, "7FFFFFFFFFFFFFFF"},
                                   {"blob", 8, "8000000000000000"},
                                   {"blob", 8, "FFFFFFFFFFFFFFFF"}}));
        QCOMPARE(selected,
                 SqlRows({{"blob", 8, "FFFFFFFFFFFFFFFF", "blob", 8, "FFFFFFFFFFFFFFFE"}}));
        QCOMPARE(anchor, SqlRows({{"null", "null"}}));
        QCOMPARE(summary,
                 SqlRows({{3, "0000000000000001", "FFFFFFFFFFFFFFFF", "FFFFFFFFFFFFFFFF"}}));
        QCOMPARE(links.size(), 2);
        QCOMPARE(links[0].size(), 7);
        QCOMPARE(links[0][0].toString(), QString("6E6F2D636964"));
        QCOMPARE(links[0][1].toString(), QString("blob"));
        QCOMPARE(links[0][2].toLongLong(), 0);
        QCOMPARE(links[0][3].toString(), QString("null"));
        QVERIFY(links[0][4].isNull());
        QCOMPARE(links[0][5].toString(), QString(""));
        QCOMPARE(links[0][6].toString(), QString("0000000000000002"));
        QCOMPARE(links[1].size(), 7);
        QCOMPARE(links[1][0].toString(), QString("70"));
        QCOMPARE(links[1][1].toString(), QString("null"));
        QVERIFY(links[1][2].isNull());
        QCOMPARE(links[1][3].toString(), QString("blob"));
        QCOMPARE(links[1][4].toLongLong(), 8);
        QCOMPARE(links[1][5].toString(), QString("0000000000000000"));
        QCOMPARE(links[1][6].toString(), QString("0000000000000002"));
        QVERIFY(foreign_keys.isEmpty());
        // Numeric storage/local selected parent is proven. Global core history is INVALID.
        const auto head = sparse["head_decimal"].toString().toULongLong();
        QVERIFY(summary[0][0].toULongLong() != head);
        QCOMPARE(summary[0][0].toString(), sparse["actual_count_decimal"].toString());
        qInfo() << "Sparse history explicitly INVALID: count3 differs from headUINT64_MAX; no "
                   "valid exhaustion/import proof";
    }

    void exactValuationAndOrderedPayloadsKeepLiteralTypes() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("payloads.sqlite")), StorageError::none);
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                create_literal_schema(session);
                seed_sources(session);
                session.commit();
                return QVariant(0);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QList<SqlRows> valuations;
        SqlRows requests, rules, modifiers, foreign_keys;
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                for (qsizetype i = 0; i < historical_.size(); ++i) {
                    const auto plan = QByteArray::number(i);
                    (void)session.execute(
                        {header_insert,
                         {plan, QByteArray("event"), QVariant{}, two, QVariant{}, one}});
                    insert_revision(session, plan, one);
                    const auto quote = historical_[i].toObject();
                    QStringList assignments;
                    QList<QVariant> values;
                    for (const auto &column : historical_columns) {
                        assignments.append(column + "=?");
                        const bool timestamp = column.endsWith("_utc_ms");
                        values.append(timestamp ? optional_signed(quote[column + "_decimal"])
                                                : optional_bytes(quote[column + "_hex"]));
                    }
                    values.append(plan);
                    (void)session.execute(
                        {"UPDATE main.subathon_plan_revisions SET payment_evidence=3,monetary=0,"
                         "original_decimal=X'32302E30303030',currency=X'455552',resolution_ref=X'',"
                         "missing_rate=1,review=3,evidence_policy=2,policy_ref=X'',decision_choice="
                         "1,"
                         "decision_ref=X'',decision_actor_ref=X'00FF',application=1,reversal=2,"
                         "settlement=0,"
                         "valuation_state=2," +
                             assignments.join(',') + " WHERE plan_ref=?",
                         values});
                    QStringList projection;
                    for (const auto &column : historical_columns)
                        projection.append(column + ",typeof(" + column + ")");
                    valuations.append(
                        session.execute({"SELECT " + projection.join(',') +
                                             " FROM main.subathon_plan_revisions WHERE plan_ref=?",
                                         {plan}}));
                }
                for (qsizetype i = 0; i < exact_.size(); ++i) {
                    const auto value = exact_[i].toObject();
                    (void)session.execute(
                        {"INSERT INTO main.subathon_plan_requests VALUES "
                         "(X'30',?, "
                         "?,X'00FF',X'',X'72',X'EFBBBF6100FF',?,1,?,?,?,X'EFBBBF6100FF',X'')",
                         {one, qlonglong(i + 1), qlonglong(i % 3),
                          literal_bytes(value["numerator_hex"]),
                          literal_bytes(value["denominator_hex"]),
                          i % 2 == 0 ? std::numeric_limits<qlonglong>::min()
                                     : std::numeric_limits<qlonglong>::max()}});
                }
                for (qlonglong ordinal : {1, 2}) {
                    (void)session.execute({"INSERT INTO main.subathon_plan_rules VALUES "
                                           "(X'30',?,?,X'00FF',X'',0,X'EFBBBF6100FF')",
                                           {one, ordinal}});
                    (void)session.execute({"INSERT INTO main.subathon_plan_modifiers VALUES "
                                           "(X'30',?,?,X'00FF',X'',X'EFBBBF6100FF',X'67',X'6D756C',"
                                           "X'616464',X'312E3235303030')",
                                           {one, ordinal}});
                }
                requests = session.execute(
                    {"SELECT ordinal,typeof(exact_numerator),hex(exact_numerator),"
                     "typeof(exact_denominator),hex(exact_denominator),requested_units,"
                     "typeof(requested_units),unit,requires_historical_valuation,"
                     "hex(action_ref),hex(action_version_ref),hex(rule_ref),hex(target_ref),"
                     "hex(calculation_explanation),hex(reversal_behavior_ref) FROM "
                     "main.subathon_plan_requests ORDER BY ordinal",
                     {}});
                rules = session.execute({"SELECT ordinal,hex(rule_ref),hex(version_ref),selected,"
                                         "typeof(selected),hex(explanation) FROM "
                                         "main.subathon_plan_rules ORDER BY ordinal",
                                         {}});
                modifiers = session.execute(
                    {"SELECT ordinal,hex(definition_version_ref),hex(instance_revision_ref),"
                     "hex(target_ref),hex(group_ref),hex(group_operator),hex(target_operator),"
                     "hex(original_factor_decimal) FROM main.subathon_plan_modifiers ORDER BY "
                     "ordinal",
                     {}});
                foreign_keys =
                    session.execute({"SELECT * FROM pragma_foreign_key_check() LIMIT 1", {}});
                session.commit();
                return QVariant(4);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QVERIFY(result.value);
        QCOMPARE(valuations.size(), historical_.size());
        for (qsizetype i = 0; i < historical_.size(); ++i) {
            QCOMPARE(valuations[i].size(), 1);
            QCOMPARE(valuations[i][0].size(), historical_columns.size() * 2);
            const auto quote = historical_[i].toObject();
            for (qsizetype j = 0; j < historical_columns.size(); ++j) {
                const auto &column = historical_columns[j];
                const bool timestamp = column.endsWith("_utc_ms");
                const auto value = quote[column + (timestamp ? "_decimal" : "_hex")];
                const auto &observed = valuations[i][0][j * 2];
                const auto type = valuations[i][0][j * 2 + 1].toString();
                if (value.isNull()) {
                    QVERIFY(observed.isNull());
                    QCOMPARE(type, QString("null"));
                } else if (timestamp) {
                    QCOMPARE(type, QString("integer"));
                    QCOMPARE(observed.toLongLong(), optional_signed(value).toLongLong());
                } else {
                    QCOMPARE(type, QString("blob"));
                    QVERIFY(!observed.isNull());
                    QCOMPARE(observed.toByteArray(), literal_bytes(value));
                }
            }
        }
        QCOMPARE(requests.size(), exact_.size());
        for (qsizetype i = 0; i < exact_.size(); ++i) {
            const auto value = exact_[i].toObject();
            const auto units = i % 2 == 0 ? std::numeric_limits<qlonglong>::min()
                                          : std::numeric_limits<qlonglong>::max();
            QCOMPARE(requests[i],
                     SqlRow({qlonglong(i + 1), "blob", value["numerator_hex"].toString().toUpper(),
                             "blob", value["denominator_hex"].toString().toUpper(), units,
                             "integer", qlonglong(i % 3), 1, "00FF", "", "72", "EFBBBF6100FF",
                             "EFBBBF6100FF", ""}));
        }
        QVERIFY(exact_[3].toObject()["numerator"].toString().size() > 1234);
        QVERIFY(exact_[3].toObject()["denominator"].toString().size() > 1234);
        QCOMPARE(rules, SqlRows({{1, "00FF", "", 0, "integer", "EFBBBF6100FF"},
                                 {2, "00FF", "", 0, "integer", "EFBBBF6100FF"}}));
        QCOMPARE(
            modifiers,
            SqlRows({{1, "00FF", "", "EFBBBF6100FF", "67", "6D756C", "616464", "312E3235303030"},
                     {2, "00FF", "", "EFBBBF6100FF", "67", "6D756C", "616464", "312E3235303030"}}));
        QVERIFY(foreign_keys.isEmpty());
    }

    void planConstraintsRefuseInvalidPhysicalValues() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath("constraints.sqlite");
        SqliteWriter writer;
        SqliteReader observer;
        QCOMPARE(writer.open(path), StorageError::none);
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                create_literal_schema(session);
                seed_sources(session);
                (void)session.execute(
                    {header_insert,
                     {QByteArray("p"), QByteArray("event"), QVariant{}, two, QVariant{}, one}});
                insert_revision(session, QByteArray("p"), one);
                (void)session.execute({"INSERT INTO main.subathon_plan_requests VALUES "
                                       "(X'70',?,1,X'',X'',X'',X'',0,0,X'30',X'31',0,X'',X'')",
                                       {one}});
                session.commit();
                return QVariant(0);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QCOMPARE(observer.open(path), StorageError::none);
        QList<SqlStatement> invalid;
        for (const auto &revision :
             {QVariant(QString("12345678")), QVariant(qlonglong(2)), QVariant(QByteArray(7, 'x')),
              QVariant(QByteArray(9, 'x')), QVariant{}, QVariant(zero)})
            invalid.append({revision_insert,
                            {QByteArray("p"), revision, one, 2, QByteArray("a"), QByteArray("r"),
                             QByteArray("d")}});
        for (const auto &sql :
             {"UPDATE main.subathon_plan_headers SET head_revision=X'0000000000000000'",
              "UPDATE main.subathon_plan_headers SET receipt_id=X'0000000000000000'",
              "UPDATE main.subathon_plan_headers SET contribution_id=X'0000000000000001'",
              "UPDATE main.subathon_plan_revisions SET parent_revision=X'0000000000000001'",
              "UPDATE main.subathon_plan_revisions SET monetary=2",
              "UPDATE main.subathon_plan_revisions SET currency=X''",
              "UPDATE main.subathon_plan_revisions SET decision_ref=X''",
              "UPDATE main.subathon_plan_revisions SET valuation_state=2",
              "UPDATE main.subathon_plan_revisions SET snapshot_ref=X''",
              "UPDATE main.subathon_plan_requests SET ordinal=0",
              "UPDATE main.subathon_plan_requests SET unit=3",
              "UPDATE main.subathon_plan_requests SET requires_historical_valuation=2",
              "UPDATE main.subathon_plan_requests SET exact_denominator=1",
              "UPDATE main.subathon_plan_requests SET exact_numerator=X''",
              "UPDATE main.subathon_plan_requests SET revision=X'0000000000000002'",
              "INSERT INTO main.subathon_plan_rules VALUES "
              "(X'70',X'0000000000000001',1,X'',X'',2,X'')"})
            invalid.append({QString::fromLatin1(sql), {}});
        qsizetype reached_count = 0;
        for (const auto &statement : invalid) {
            bool reached = false;
            QVERIFY(existing_work(
                writer,
                [&](auto context) {
                    auto session = context.begin();
                    (void)session.execute({header_insert,
                                           {QByteArray("marker"), QByteArray("event"), QVariant{},
                                            two, QVariant{}, one}});
                    reached = true;
                    (void)session.execute(statement);
                    session.commit();
                    return QVariant(1);
                },
                result));
            QVERIFY(reached);
            ++reached_count;
            QCOMPARE(result.transaction, TransactionOutcome::rolled_back);
            QCOMPARE(result.error, WorkError::sql_failure);
            QVERIFY(!result.value);
            // No catch-and-continue on an auto-aborted ticket. A fresh real writer unit
            // must BEGIN; a separate read-only connection observes durable rollback.
            SqlRows durable;
            QVERIFY(existing_work(
                writer,
                [&](auto context) {
                    auto session = context.begin();
                    durable =
                        session.execute({"SELECT (SELECT COUNT(*) FROM main.subathon_plan_headers),"
                                         "(SELECT COUNT(*) FROM main.subathon_plan_headers WHERE "
                                         "plan_ref=X'6D61726B6572'),"
                                         "(SELECT COUNT(*) FROM main.subathon_plan_revisions),"
                                         "(SELECT COUNT(*) FROM main.subathon_plan_requests),"
                                         "(SELECT hex(head_revision) FROM "
                                         "main.subathon_plan_headers WHERE plan_ref=X'70')",
                                         {}});
                    session.commit();
                    return QVariant(0);
                },
                result));
            QCOMPARE(result.transaction, TransactionOutcome::committed);
            QCOMPARE(result.error, WorkError::none);
            QVERIFY(result.value);
            QCOMPARE(durable, SqlRows({{1, 0, 1, 1, "0000000000000001"}}));
            const auto committed = observer.read(
                {"SELECT (SELECT COUNT(*) FROM main.subathon_plan_headers WHERE "
                 "plan_ref=X'6D61726B6572'),(SELECT COUNT(*) FROM main.subathon_plan_revisions),"
                 "(SELECT COUNT(*) FROM main.subathon_plan_requests)",
                 {}});
            QCOMPARE(committed.error, StorageError::none);
            QCOMPARE(committed.rows, SqlRows({{0, 1, 1}}));
        }
        QCOMPARE(reached_count, 22);
        qInfo() << "All22 isolated physical constraint refusals reached; fresh BEGIN and "
                   "independent durable rollback observations passed";
    }
};
QTEST_GUILESS_MAIN(SqliteApplicationPlansEnvironment)
#include "sqlite_application_plans_environment.moc"
