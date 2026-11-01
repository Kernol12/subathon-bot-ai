// SPDX-License-Identifier: GPL-3.0-or-later
// Independent SQL foundation only: no contribution/receipt codec, factory or service.
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
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
const QList<QByteArray> receipt_ids{
    QByteArray::fromHex("0000000000000001"), QByteArray::fromHex("0000000000000002"),
    QByteArray::fromHex("0000000000000003"), QByteArray::fromHex("0000000000000004"),
    QByteArray::fromHex("0000000000000005"), QByteArray::fromHex("0000000000000006"),
    QByteArray::fromHex("0000000000000007")};
const QString minimal_contribution =
    "INSERT INTO main.subathon_contributions(contribution_id,original_receipt_id,"
    "payment_evidence,monetary,missing_rate,review,evidence_policy,policy_ref,application,"
    "reversal,settlement) VALUES (?,?,0,1,0,0,0,X'',0,0,0)";
const QString minimal_support =
    "INSERT INTO main.subathon_contribution_observations(receipt_id,contribution_id,kind,evidence,"
    "support_monetary,support_missing_rate,support_review,support_evidence_policy,support_policy_"
    "ref,"
    "support_application,support_reversal,support_settlement) VALUES (?,?,0,0,1,0,0,0,X'',0,0,0)";
} // namespace

class SqliteContributionsEnvironment final : public QObject {
    Q_OBJECT
  private:
    QJsonObject retained_, receipts_, manifest_;
    QJsonArray codec_;
    void create_schema(const SqliteWorkSession &session) {
        for (const auto &entry : retained_["expected_schema"].toArray()) {
            const auto object = entry.toObject();
            if (object["name"] == "subathon_catalog" ||
                object["name"] == "subathon_migration_history")
                (void)session.execute({object["sql"].toString(), {}});
        }
        for (const auto &manifest : {retained_, receipts_, manifest_})
            for (const auto &sql : manifest["sql"].toArray())
                (void)session.execute({sql.toString(), {}});
        (void)session.execute(
            {"INSERT INTO main.subathon_catalog VALUES (1,1,'subathon.sqlite-store')", {}});
        (void)session.execute(
            {"INSERT INTO main.subathon_migration_history VALUES "
             "(1,'retain-config-context-v1',?),(2,'identified-receipts-v1',?),"
             "(3,'contribution-evidence-v1',?)",
             {QByteArray::fromHex(
                  "0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e"),
              QByteArray::fromHex(
                  "24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2"),
              QByteArray::fromHex(
                  "b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d")}});
    }
    void seed_receipts(const SqliteWorkSession &session, qsizetype count) {
        (void)session.execute(
            {"INSERT INTO main.subathon_retained_configurations VALUES (X'63',X'00FF')", {}});
        (void)session.execute(
            {"INSERT INTO main.subathon_retained_contexts VALUES (X'78',X'EFBBBF6100FF',X'63')",
             {}});
        for (qsizetype i = 0; i < count; ++i) {
            const auto &id = receipt_ids[i];
            (void)session.execute(
                {"INSERT INTO "
                 "main.subathon_receipts(receipt_id,provider,source_account,identifier_namespace,"
                 "opaque_id,event_type,schema_version,server_timestamp_utc_ms,connection_ref,"
                 "configuration_ref,"
                 "modifier_context_ref) VALUES (?,X'70',X'61',X'6E',?,X'65',0,0,X'',X'63',X'78')",
                 {id, id}});
            (void)session.execute(
                {"INSERT INTO "
                 "main.subathon_receipt_attempts(receipt_id,ordinal,event_type,schema_version,"
                 "server_timestamp_utc_ms,connection_ref,configuration_ref,modifier_context_ref,"
                 "disposition) "
                 "VALUES (?,1,X'65',0,0,X'',X'63',X'78',0)",
                 {id}});
        }
        (void)session.execute(
            {"UPDATE main.subathon_receipt_allocator SET last_allocated=? WHERE singleton=1",
             {receipt_ids[count - 1]}});
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
        const auto contribution = read(SUBATHON_CONTRIBUTION_VECTOR_PATH, error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        QVERIFY(contribution.isObject());
        retained_ = retained.object()["manifest"].toObject();
        receipts_ = receipts.object()["manifest"].toObject();
        manifest_ = contribution.object()["manifest"].toObject();
        codec_ = contribution.object()["codec_vectors"].toArray();
        QCOMPARE(retained_["expected_schema"].toArray().size(), 7);
        QCOMPARE(receipts_["expected_schema"].toArray().size(), 13);
        QCOMPARE(manifest_["expected_schema"].toArray().size(), 20);
        QCOMPARE(manifest_["version"].toInt(), 3);
        QCOMPARE(manifest_["id"].toString(), QString("contribution-evidence-v1"));
        QCOMPARE(manifest_["sql"].toArray().size(), 4);
        QCOMPARE(manifest_["supported_starting_versions"].toArray(), QJsonArray({0, 1, 2, 3}));
        const auto objects = manifest_["expected_schema"].toArray();
        for (const auto &prefix : {retained_, receipts_})
            for (const auto &object : prefix["expected_schema"].toArray())
                QVERIFY(objects.contains(object));
        const auto payload = QByteArray::fromHex(manifest_["payload_hex"].toString().toLatin1());
        QCOMPARE(payload.size(), 18126);
        QCOMPARE(manifest_["payload_bytes"].toInt(), 18126);
        QCOMPARE(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex(),
                 QByteArray("b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d"));
        QCOMPARE(codec_.size(), 6);
        for (const auto &entry : codec_) {
            QVERIFY(entry.toObject()["decimal"].isString());
            QVERIFY(entry.toObject()["hex"].isString());
        }
    }

    void contributionSchemaPrefixMatchesLiteralProjection() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("schema.sqlite")), StorageError::none);
        SqlRows schema, allocator, history, runtime;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                create_schema(session);
                schema = session.execute({"SELECT type,name,tbl_name,sql,typeof(sql) FROM "
                                          "main.sqlite_schema ORDER BY type,name",
                                          {}});
                allocator = session.execute(
                    {"SELECT singleton,typeof(singleton),last_allocated,typeof(last_allocated) "
                     "FROM main.subathon_contribution_allocator",
                     {}});
                history = session.execute(
                    {"SELECT version,migration_id,typeof(content_sha256),hex(content_sha256) "
                     "FROM main.subathon_migration_history ORDER BY version",
                     {}});
                runtime = session.execute({"SELECT sqlite_version()", {}});
                session.commit();
                return QVariant(20);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QVERIFY(result.value);
        QCOMPARE(runtime.size(), 1);
        QCOMPARE(runtime[0].size(), 1);
        qInfo().noquote() << "Selected Qt" << QT_VERSION_STR << "SQLite"
                          << runtime[0][0].toString();
        QCOMPARE(schema.size(), 20);
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
        QCOMPARE(history,
                 SqlRows({{1, "retain-config-context-v1", "blob",
                           "0906F121C3BA1193CDEA41A70F1C91013724EA90416B0F1D04B6C23D8F091F5E"},
                          {2, "identified-receipts-v1", "blob",
                           "24614F68E9C41A29751EA9EF6BA08FCF655B1CDB0E0C097CB1601BB1581553A2"},
                          {3, "contribution-evidence-v1", "blob",
                           "B72249BA22C4838AF98B9AE29600E70969CF3295EEA958A30074167221D2AD1D"}}));
    }

    void unsignedContributionIdsAndNullableAssociationsStayDistinct() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("ids.sqlite")), StorageError::none);
        SqlRows ids, zero_allocator, associated, unresolved, foreign_keys;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                create_schema(session);
                seed_receipts(session, 7);
                for (qsizetype i = 0; i < codec_.size(); ++i) {
                    const auto id =
                        QByteArray::fromHex(codec_[i].toObject()["hex"].toString().toLatin1());
                    (void)session.execute({minimal_contribution, {id, receipt_ids[i]}});
                    (void)session.execute({minimal_support, {receipt_ids[i], id}});
                    if (i == 0) {
                        (void)session.execute({"UPDATE main.subathon_contribution_allocator "
                                               "SET last_allocated=? WHERE singleton=1",
                                               {id}});
                        zero_allocator = session.execute(
                            {"SELECT "
                             "typeof(last_allocated),length(last_allocated),hex(last_allocated) "
                             "FROM main.subathon_contribution_allocator",
                             {}});
                    }
                }
                (void)session.execute({minimal_support, {receipt_ids[6], QVariant{}}});
                (void)session.execute({"UPDATE main.subathon_contribution_allocator SET "
                                       "last_allocated=X'FFFFFFFFFFFFFFFF' WHERE singleton=1",
                                       {}});
                ids = session.execute(
                    {"SELECT typeof(contribution_id),length(contribution_id),hex(contribution_id) "
                     "FROM main.subathon_contributions ORDER BY contribution_id",
                     {}});
                associated = session.execute(
                    {"SELECT hex(receipt_id),typeof(contribution_id),length(contribution_id),"
                     "hex(contribution_id),contribution_id IS NULL FROM "
                     "main.subathon_contribution_observations "
                     "WHERE receipt_id IN (X'0000000000000001',X'0000000000000007') ORDER BY "
                     "receipt_id",
                     {}});
                unresolved = session.execute(
                    {"SELECT hex(receipt_id) FROM main.subathon_contribution_observations "
                     "WHERE contribution_id IS NULL ORDER BY receipt_id",
                     {}});
                foreign_keys =
                    session.execute({"SELECT * FROM pragma_foreign_key_check() LIMIT 1", {}});
                session.commit();
                return QVariant(6);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::committed);
        QCOMPARE(result.error, WorkError::none);
        QVERIFY(result.value);
        QCOMPARE(ids, SqlRows({{"blob", 8, "0000000000000000"},
                               {"blob", 8, "0000000000000001"},
                               {"blob", 8, "0020000000000001"},
                               {"blob", 8, "7FFFFFFFFFFFFFFF"},
                               {"blob", 8, "8000000000000000"},
                               {"blob", 8, "FFFFFFFFFFFFFFFF"}}));
        QCOMPARE(zero_allocator, SqlRows({{"blob", 8, "0000000000000000"}}));
        QCOMPARE(associated.size(), 2);
        QCOMPARE(associated[0], SqlRow({"0000000000000001", "blob", 8, "0000000000000000", 0}));
        QCOMPARE(associated[1].size(), 5);
        QCOMPARE(associated[1][0].toString(), QString("0000000000000007"));
        QCOMPARE(associated[1][1].toString(), QString("null"));
        QVERIFY(associated[1][2].isNull());
        QCOMPARE(associated[1][3].toString(), QString(""));
        QCOMPARE(associated[1][4].toInt(), 1);
        QCOMPARE(unresolved, SqlRows({{"0000000000000007"}}));
        QVERIFY(foreign_keys.isEmpty());
    }

    void groupedConditionsRolesAndProofUseLiteralSchema() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("groups.sqlite")), StorageError::none);
        SqlRows dimensions, support_dimensions, decisions, groups, roles, foreign_keys;
        SqliteWorkResult result;
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                create_schema(session);
                seed_receipts(session, 3);
                (void)session.execute(
                    {"INSERT INTO main.subathon_contributions VALUES "
                     "(X'0000000000000000',X'0000000000000001',X'70',X'61',X'EFBBBF6100FF',X'00FF',"
                     "3,0,?,X'455552',X'',1,3,2,X'',1,X'',X'00FF',1,2,0),"
                     "(X'0000000000000001',X'0000000000000002',NULL,NULL,NULL,NULL,"
                     "2,1,NULL,NULL,NULL,0,0,0,X'EFBBBF6100FF',NULL,NULL,NULL,0,0,1)",
                     {QByteArray("9007199254740993.00000000000000000001")}});
                (void)session.execute(
                    {"INSERT INTO main.subathon_contribution_observations VALUES "
                     "(X'0000000000000001',X'0000000000000000',0,0,X'70',X'61',X'EFBBBF6100FF',X'"
                     "00FF',X'EFBBBF6100FF',"
                     "1,X'32302E30303030',X'455552',NULL,1,0,1,X'',0,X'',X'00FF',0,0,1),"
                     "(X'0000000000000002',X'0000000000000001',0,1,NULL,NULL,NULL,NULL,NULL,"
                     "0,X'',X'',X'',0,1,2,X'',NULL,NULL,NULL,1,1,0),"
                     "(X'0000000000000003',NULL,1,2,X'70',X'61',X'EFBBBF6100FF',X'00FF',X'00FF',"
                     "NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL)",
                     {}});
                (void)session.execute({"UPDATE main.subathon_contribution_allocator SET "
                                       "last_allocated=X'0000000000000001' WHERE singleton=1",
                                       {}});
                dimensions =
                    session.execute({"SELECT "
                                     "hex(contribution_id),payment_evidence,monetary,missing_rate,"
                                     "review,evidence_policy,"
                                     "application,reversal,settlement,typeof(payment_evidence),"
                                     "typeof(monetary),typeof(missing_rate),"
                                     "typeof(review),typeof(evidence_policy),typeof(application),"
                                     "typeof(reversal),typeof(settlement) "
                                     "FROM main.subathon_contributions ORDER BY contribution_id",
                                     {}});
                support_dimensions = session.execute(
                    {"SELECT hex(receipt_id),support_monetary,support_missing_rate,support_review,"
                     "support_evidence_policy,support_application,support_reversal,support_"
                     "settlement,"
                     "typeof(support_monetary),typeof(support_missing_rate),typeof(support_review),"
                     "typeof(support_evidence_policy),typeof(support_application),typeof(support_"
                     "reversal),"
                     "typeof(support_settlement) FROM main.subathon_contribution_observations "
                     "WHERE kind=0 ORDER BY receipt_id",
                     {}});
                decisions = session.execute(
                    {"SELECT "
                     "'current',decision_choice,typeof(decision_choice),typeof(decision_ref),"
                     "length(decision_ref),typeof(decision_actor_ref),length(decision_actor_ref),"
                     "hex(decision_actor_ref) "
                     "FROM main.subathon_contributions WHERE contribution_id=X'0000000000000000' "
                     "UNION ALL "
                     "SELECT "
                     "'support',support_decision_choice,typeof(support_decision_choice),typeof("
                     "support_decision_ref),"
                     "length(support_decision_ref),typeof(support_decision_actor_ref),length("
                     "support_decision_actor_ref),"
                     "hex(support_decision_actor_ref) FROM main.subathon_contribution_observations "
                     "WHERE receipt_id=X'0000000000000001' ORDER BY 1",
                     {}});
                groups = session.execute({"SELECT "
                                          "hex(contribution_id),typeof(activity_namespace),hex("
                                          "activity_namespace),typeof(activity_id),"
                                          "hex(activity_id),typeof(original_decimal),hex(original_"
                                          "decimal),typeof(currency),hex(currency),"
                                          "typeof(resolution_ref),hex(resolution_ref),typeof("
                                          "policy_ref),hex(policy_ref),typeof(decision_choice),"
                                          "hex(decision_ref),hex(decision_actor_ref) FROM "
                                          "main.subathon_contributions ORDER BY contribution_id",
                                          {}});
                roles = session.execute(
                    {"SELECT "
                     "hex(receipt_id),typeof(contribution_id),kind,evidence,typeof(verification_"
                     "ref),hex(verification_ref),"
                     "typeof(support_monetary),typeof(support_original_decimal),hex(support_"
                     "original_decimal),"
                     "typeof(support_currency),hex(support_currency),typeof(support_resolution_ref)"
                     ",hex(support_resolution_ref),"
                     "typeof(support_policy_ref),hex(support_policy_ref),typeof(support_decision_"
                     "choice),"
                     "typeof(support_decision_ref),hex(support_decision_ref),typeof(support_"
                     "decision_actor_ref),hex(support_decision_actor_ref) "
                     "FROM main.subathon_contribution_observations ORDER BY receipt_id",
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
        QCOMPARE(dimensions,
                 SqlRows({{"0000000000000000", 3, 0, 1, 3, 2, 1, 2, 0, "integer", "integer",
                           "integer", "integer", "integer", "integer", "integer", "integer"},
                          {"0000000000000001", 2, 1, 0, 0, 0, 0, 0, 1, "integer", "integer",
                           "integer", "integer", "integer", "integer", "integer", "integer"}}));
        QCOMPARE(support_dimensions,
                 SqlRows({{"0000000000000001", 1, 1, 0, 1, 0, 0, 1, "integer", "integer", "integer",
                           "integer", "integer", "integer", "integer"},
                          {"0000000000000002", 0, 0, 1, 2, 1, 1, 0, "integer", "integer", "integer",
                           "integer", "integer", "integer", "integer"}}));
        QCOMPARE(decisions, SqlRows({{"current", 1, "integer", "blob", 0, "blob", 2, "00FF"},
                                     {"support", 0, "integer", "blob", 0, "blob", 2, "00FF"}}));
        QCOMPARE(
            groups,
            SqlRows({{"0000000000000000", "blob", "EFBBBF6100FF", "blob", "00FF", "blob",
                      "393030373139393235343734303939332E3030303030303030303030303030303030303031",
                      "blob", "455552", "blob", "", "blob", "", "integer", "", "00FF"},
                     {"0000000000000001", "null", "", "null", "", "null", "", "null", "", "null",
                      "", "blob", "EFBBBF6100FF", "null", "", ""}}));
        QCOMPARE(roles, SqlRows({{"0000000000000001",
                                  "blob",
                                  0,
                                  0,
                                  "blob",
                                  "EFBBBF6100FF",
                                  "integer",
                                  "blob",
                                  "32302E30303030",
                                  "blob",
                                  "455552",
                                  "null",
                                  "",
                                  "blob",
                                  "",
                                  "integer",
                                  "blob",
                                  "",
                                  "blob",
                                  "00FF"},
                                 {"0000000000000002",
                                  "blob",
                                  0,
                                  1,
                                  "null",
                                  "",
                                  "integer",
                                  "blob",
                                  "",
                                  "blob",
                                  "",
                                  "blob",
                                  "",
                                  "blob",
                                  "",
                                  "null",
                                  "null",
                                  "",
                                  "null",
                                  ""},
                                 {"0000000000000003",
                                  "null",
                                  1,
                                  2,
                                  "blob",
                                  "00FF",
                                  "null",
                                  "null",
                                  "",
                                  "null",
                                  "",
                                  "null",
                                  "",
                                  "null",
                                  "",
                                  "null",
                                  "null",
                                  "",
                                  "null",
                                  ""}}));
        QVERIFY(foreign_keys.isEmpty());
        for (const auto &sql :
             {QString("UPDATE main.subathon_contributions SET payment_evidence=4 WHERE "
                      "contribution_id=X'0000000000000000'"),
              QString("UPDATE main.subathon_contributions SET monetary=2 WHERE "
                      "contribution_id=X'0000000000000000'"),
              QString("UPDATE main.subathon_contributions SET activity_id=NULL WHERE "
                      "contribution_id=X'0000000000000000'"),
              QString("UPDATE main.subathon_contributions SET activity_id=X'' WHERE "
                      "contribution_id=X'0000000000000000'"),
              QString("UPDATE main.subathon_contributions SET currency=NULL WHERE "
                      "contribution_id=X'0000000000000000'"),
              QString("UPDATE main.subathon_contributions SET decision_actor_ref=NULL WHERE "
                      "contribution_id=X'0000000000000000'"),
              QString("UPDATE main.subathon_contributions SET decision_choice=2 WHERE "
                      "contribution_id=X'0000000000000000'"),
              QString("UPDATE main.subathon_contributions SET contribution_id='text' WHERE "
                      "contribution_id=X'0000000000000001'"),
              QString("UPDATE main.subathon_contributions SET contribution_id=NULL WHERE "
                      "contribution_id=X'0000000000000001'"),
              QString("UPDATE main.subathon_contribution_observations SET verification_ref=X'' "
                      "WHERE receipt_id=X'0000000000000001'"),
              QString("UPDATE main.subathon_contribution_observations SET support_review=NULL "
                      "WHERE receipt_id=X'0000000000000001'"),
              QString("UPDATE main.subathon_contribution_observations SET evidence=0 WHERE "
                      "receipt_id=X'0000000000000003'"),
              QString("UPDATE main.subathon_contribution_observations SET support_monetary=1 WHERE "
                      "receipt_id=X'0000000000000003'"),
              QString("UPDATE main.subathon_contribution_observations SET "
                      "contribution_id=X'FFFFFFFFFFFFFFFE' WHERE receipt_id=X'0000000000000003'"),
              QString("UPDATE main.subathon_contribution_observations SET "
                      "receipt_id=X'0000000000000000' WHERE receipt_id=X'0000000000000003'")}) {
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
        // Receipt3 exists and has no contribution anchor; every other value is
        // valid. This INSERT isolates required ContributionId NULL rejection.
        QVERIFY(existing_work(
            writer,
            [&](auto context) {
                auto session = context.begin();
                (void)session.execute({minimal_contribution, {QVariant{}, receipt_ids[2]}});
                session.commit();
                return QVariant(0);
            },
            result));
        QCOMPARE(result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(result.error, WorkError::sql_failure);
        QVERIFY(!result.value);
    }
};
QTEST_GUILESS_MAIN(SqliteContributionsEnvironment)
#include "sqlite_contributions_environment.moc"
