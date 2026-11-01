// SPDX-License-Identifier: GPL-3.0-or-later
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <memory>
#include <subathon/server/sqlite_migrations.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <thread>

using namespace subathon::server;
using subathon::server::testing::SqliteWriterWorkAccess;
using subathon::server::testing::WorkControlFault;

namespace {
const QString header_ddl = "CREATE TABLE subathon_catalog(singleton INTEGER PRIMARY KEY, "
                           "catalog_format INTEGER NOT NULL, store_identity TEXT NOT NULL)";
const QString history_ddl = "CREATE TABLE subathon_migration_history(version INTEGER PRIMARY KEY, "
                            "migration_id TEXT NOT NULL UNIQUE, content_sha256 BLOB NOT NULL)";
const QString sample_ddl = "CREATE TABLE sample(id INTEGER PRIMARY KEY, amount INTEGER)";

// Independent literal contract vector: version1/bom-name, quoted U+FEFF+a table,
// CREATE + literal INSERT17, reserved catalog objects. Python/hashlib reference;
// never generated from the production codec or from QString::fromUtf8(BOM bytes).
const QByteArray bom_payload = QByteArray::fromHex(
    "32303a6d6967726174696f6e2d636174616c6f672d7631313a31383a626f6d2d6e616d65313a3233343a4352454154"
    "45205441424c452022efbbbf61222876616c756520494e54454745522933303a494e5345525420494e544f2022efbb"
    "bf61222056414c5545532028313729313a34353a696e64657834353a73716c6974655f6175746f696e6465785f7375"
    "626174686f6e5f6d6967726174696f6e5f686973746f72795f3132363a7375626174686f6e5f6d6967726174696f6e"
    "5f686973746f7279313a30353a7461626c6531363a7375626174686f6e5f636174616c6f6731363a7375626174686f"
    "6e5f636174616c6f67313a313132333a435245415445205441424c45207375626174686f6e5f636174616c6f672873"
    "696e676c65746f6e20494e5445474552205052494d415259204b45592c20636174616c6f675f666f726d617420494e"
    "5445474552204e4f54204e554c4c2c2073746f72655f6964656e746974792054455854204e4f54204e554c4c29353a"
    "7461626c6532363a7375626174686f6e5f6d6967726174696f6e5f686973746f727932363a7375626174686f6e5f6d"
    "6967726174696f6e5f686973746f7279313a313133333a435245415445205441424c45207375626174686f6e5f6d69"
    "67726174696f6e5f686973746f72792876657273696f6e20494e5445474552205052494d415259204b45592c206d69"
    "67726174696f6e5f69642054455854204e4f54204e554c4c20554e495155452c20636f6e74656e745f736861323536"
    "20424c4f42204e4f54204e554c4c29353a7461626c65343aefbbbf61343aefbbbf61313a3133343a43524541544520"
    "5441424c452022efbbbf61222876616c756520494e544547455229");
const QByteArray bom_digest =
    QByteArray::fromHex("baf9a94eeb7cbdd3994a277363ae6dd1cd4b63af76f99b02b48f8f3ac4775b7d");

QString quoted(QString name) {
    name.replace('"', "\"\"");
    return '"' + name + '"';
}

// Independent fixture SQL creates known historical rows; it never calls the
// migration runner, codec or digest to prepare an expected catalog/result.
class CatalogFixture final {
  public:
    explicit CatalogFixture(const QString &path)
        : name_(QUuid::createUuid().toString()), db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
    }
    ~CatalogFixture() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }
    bool open() { return db_.open(); }
    bool exec(const QString &sql, const QList<QVariant> &bindings = {}) {
        QSqlQuery query(db_);
        if (!query.prepare(sql))
            return false;
        for (const auto &value : bindings)
            query.addBindValue(value);
        return query.exec();
    }
    SqlRows rows(const QString &sql) {
        QSqlQuery query(db_);
        if (!query.exec(sql)) {
            QTest::qFail(
                qPrintable("Independent fixture query failed: " + query.lastError().text()),
                __FILE__, __LINE__);
            return {};
        }
        SqlRows result;
        while (query.next()) {
            SqlRow row;
            for (int i = 0; i < query.record().count(); ++i)
                row.append(query.value(i));
            result.append(std::move(row));
        }
        if (query.lastError().isValid())
            QTest::qFail("Independent fixture fetch failed", __FILE__, __LINE__);
        return result;
    }
    QVariant scalar(const QString &sql) {
        const auto result = rows(sql);
        if (result.size() != 1 || result[0].size() != 1) {
            QTest::qFail("Independent fixture scalar shape failed", __FILE__, __LINE__);
            return {};
        }
        return result[0][0];
    }
    bool lock_released() { return exec("BEGIN IMMEDIATE") && exec("ROLLBACK"); }
    QList<SqlRows> snapshot() {
        QList<SqlRows> result;
        const auto schema = rows("SELECT type,name,tbl_name,sql FROM main.sqlite_schema "
                                 "ORDER BY type,name");
        result.append(schema);
        result.append(rows("PRAGMA encoding"));
        for (const auto &object : schema) {
            if (object[0].toString() != "table")
                continue;
            const auto table = object[1].toString();
            const auto columns = rows("PRAGMA main.table_info(" + quoted(table) + ")");
            QStringList projection;
            for (const auto &column : columns) {
                const auto name = quoted(column[1].toString());
                projection.append("typeof(" + name + ")");
                projection.append("hex(CAST(" + name + " AS BLOB))");
            }
            result.append(rows("SELECT " + projection.join(',') + " FROM main." + quoted(table) +
                               " ORDER BY rowid"));
        }
        return result;
    }

  private:
    QString name_;
    QSqlDatabase db_;
};

MigrationStep step_from_json(const QJsonObject &input) {
    MigrationStep step;
    step.version = input["version"].toInt();
    step.id = input["id"].toString();
    for (const auto &sql : input["sql"].toArray())
        step.sql.append(sql.toString());
    for (const auto &item : input["expected_schema"].toArray()) {
        const auto object = item.toObject();
        step.expected_schema.append(
            {object["type"].toString(), object["name"].toString(), object["table_name"].toString(),
             object["sql"].isNull() ? std::nullopt
                                    : std::optional<QString>(object["sql"].toString())});
    }
    return step;
}

void failed(const MigrationResult &result, MigrationReason reason,
            WorkError error = WorkError::work_exception,
            TransactionOutcome transaction = TransactionOutcome::rolled_back) {
    QCOMPARE(result.reason, reason);
    QCOMPARE(result.work.error, error);
    QCOMPARE(result.work.transaction, transaction);
    QVERIFY(!result.work.value.has_value());
    QVERIFY(!result.report.has_value());
}

void current(const MigrationResult &result, qint32 version, qsizetype applied) {
    QCOMPARE(result.reason, MigrationReason::none);
    QCOMPARE(result.work.transaction, TransactionOutcome::committed);
    QCOMPARE(result.work.error, WorkError::none);
    QVERIFY(result.work.value.has_value());
    QVERIFY(result.report.has_value());
    QCOMPARE(result.report->state, CatalogState::current);
    QCOMPARE(result.report->version, version);
    QCOMPARE(result.report->target_version, version);
    QCOMPARE(result.report->applied, applied);
}
} // namespace

#define RUN_MIGRATION(writer, request, result)                                                     \
    QVERIFY2(run(writer, request, result), qPrintable(last_error_))

class SqliteMigrationTests final : public QObject {
    Q_OBJECT
  private:
    QJsonObject literals_;
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<CatalogFixture> fixture_;
    QString path_, last_error_;

    MigrationManifest base() const {
        const auto json = literals_["base"].toObject();
        return manifest_from_json(json);
    }
    static MigrationManifest manifest_from_json(const QJsonObject &json) {
        MigrationManifest manifest;
        manifest.catalog_format = json["catalog_format"].toInt();
        manifest.store_identity = json["store_identity"].toString();
        for (const auto &step : json["steps"].toArray())
            manifest.steps.append(step_from_json(step.toObject()));
        for (const auto &version : json["supported_starting_versions"].toArray())
            manifest.supported_starting_versions.append(version.toInt());
        return manifest;
    }
    bool seed_profile(const QString &name) {
        const auto profile = literals_["profiles"].toObject()[name].toObject();
        const auto step = profile["steps"].toArray()[0].toObject();
        if (!fixture_->exec(header_ddl) || !fixture_->exec(history_ddl) ||
            !fixture_->exec("INSERT INTO subathon_catalog VALUES (1,1,'subathon.sqlite-store')"))
            return false;
        for (const auto &sql : profile["fixture_sql"].toArray())
            if (!fixture_->exec(sql.toString()))
                return false;
        return fixture_->exec(
            "INSERT INTO subathon_migration_history VALUES (1,?,?)",
            {step["id"].toString(), QByteArray::fromHex(step["sha256_hex"].toString().toLatin1())});
    }
    MigrationRequest profile_request(const QString &name) const {
        auto value = request();
        value.manifest = manifest_from_json(literals_["profiles"].toObject()[name].toObject());
        return value;
    }
    MigrationRequest request(MigrationMode mode = MigrationMode::advance) const {
        MigrationRequest value;
        value.mode = mode; // Advancement is always explicit; API default is inspect.
        value.manifest = base();
        return value;
    }
    MigrationStep bom_step() const {
        const QString name = QString(QChar(0xfeff)) + 'a';
        const QString ddl = "CREATE TABLE " + quoted(name) + "(value INTEGER)";
        MigrationStep step;
        step.version = 1;
        step.id = "bom-name";
        step.sql = {ddl, "INSERT INTO " + quoted(name) + " VALUES (17)"};
        const auto manifest = base();
        for (const auto &object : manifest.steps[0].expected_schema)
            if (object.name != "sample")
                step.expected_schema.append(object);
        step.expected_schema.append({"table", name, name, ddl});
        return step;
    }
    QByteArray digest(int version) const {
        const auto step = literals_["base"].toObject()["steps"].toArray()[version - 1].toObject();
        return QByteArray::fromHex(step["sha256_hex"].toString().toLatin1());
    }
    bool seed(int version) {
        if (!fixture_->exec(header_ddl) || !fixture_->exec(history_ddl) ||
            !fixture_->exec(
                "INSERT INTO main.subathon_catalog VALUES (1,1,'subathon.sqlite-store')"))
            return false;
        if (version == 0)
            return true; // Deliberately partial, never a supported fresh catalog.
        if (!fixture_->exec(sample_ddl) || !fixture_->exec("INSERT INTO sample VALUES (1,10)") ||
            !fixture_->exec("INSERT INTO main.subathon_migration_history VALUES (1,?,?)",
                            {"create-sample", digest(1)}))
            return false;
        if (version >= 2 &&
            (!fixture_->exec("INSERT INTO sample VALUES (2,20)") ||
             !fixture_->exec("INSERT INTO main.subathon_migration_history VALUES (2,?,?)",
                             {"add-second", digest(2)})))
            return false;
        return true;
    }
    void reset_fixture() {
        fixture_.reset();
        directory_.reset();
        directory_ = std::make_unique<QTemporaryDir>();
        path_ = directory_->filePath("catalog.sqlite");
        fixture_ = std::make_unique<CatalogFixture>(path_);
        if (!directory_->isValid() || !fixture_->open())
            QTest::qFail("Disposable fixture reset failed", __FILE__, __LINE__);
    }
    bool run(SqliteWriter &writer, MigrationRequest input, MigrationResult &result) {
        struct State {
            bool done{false};
            MigrationResult result;
        };
        auto state = std::make_shared<State>();
        const auto admission = SqliteMigrationRunner::enqueue(writer, std::move(input),
                                                              [state](MigrationResult value) {
                                                                  state->result = std::move(value);
                                                                  state->done = true;
                                                              });
        if (admission.error != StorageError::none || admission.reason != MigrationReason::none) {
            last_error_ = QString("Migration admission error=%1 reason=%2%3")
                              .arg(static_cast<int>(admission.error))
                              .arg(static_cast<int>(admission.reason))
                              .arg(admission.error == StorageError::feature_not_implemented
                                       ? " feature_not_implemented"
                                       : "");
            return false;
        }
        if (!QTest::qWaitFor([state] { return state->done; }, 5000)) {
            last_error_ = "Admitted migration completion timed out";
            return false;
        }
        result = std::move(state->result);
        return true;
    }

  private slots:
    void initTestCase() {
        QVERIFY(QSqlDatabase::isDriverAvailable("QSQLITE"));
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
        QFile file(QString::fromUtf8(SUBATHON_MIGRATION_VECTOR_PATH));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJsonParseError parse;
        literals_ = QJsonDocument::fromJson(file.readAll(), &parse).object();
        QCOMPARE(parse.error, QJsonParseError::NoError);
        QCOMPARE(literals_["base"].toObject()["steps"].toArray().size(), 2);
    }
    void init() { reset_fixture(); }
    void cleanup() {
        fixture_.reset();
        directory_.reset();
    }

    void canonicalPayloadAndDigestMatchLiteralVector() {
        const auto vector = literals_["vectors"].toObject()["m1"].toObject();
        const auto step = step_from_json(vector);
        QByteArray payload, hash;
        try {
            payload = canonical_migration_payload(step);
            hash = migration_content_sha256(step);
        } catch (const MigrationFailure &error) {
            QVERIFY2(error.reason() != MigrationReason::feature_not_implemented,
                     "feature_not_implemented: canonical migration codec stub");
            QFAIL("Unexpected canonical migration failure");
        }
        QCOMPARE(payload, QByteArray::fromHex(vector["payload_hex"].toString().toLatin1()));
        QCOMPARE(payload.size(), 636);
        QCOMPARE(hash, QByteArray::fromHex(
                           "3d1e0f776ae0a3f84c9036c099b0a1333ea735eb88e22fc666f5f3f72bf972bc"));
    }

    void canonicalNullUnicodeOrderingAndInvalidText() {
        const auto vectors = literals_["vectors"].toObject();
        for (const auto &name : {QString("m1"), QString("empty_sql"), QString("unicode_order")}) {
            const auto vector = vectors[name].toObject();
            const auto step = step_from_json(vector);
            QByteArray payload, hash;
            try {
                payload = canonical_migration_payload(step);
                hash = migration_content_sha256(step);
            } catch (const MigrationFailure &error) {
                QVERIFY2(error.reason() != MigrationReason::feature_not_implemented,
                         "feature_not_implemented: canonical migration codec stub");
                QFAIL("Unexpected canonical migration failure");
            }
            QCOMPARE(payload, QByteArray::fromHex(vector["payload_hex"].toString().toLatin1()));
            QCOMPARE(hash, QByteArray::fromHex(vector["sha256_hex"].toString().toLatin1()));
        }
        for (int field = 0; field < 4; ++field) {
            auto invalid = step_from_json(vectors["m1"].toObject());
            QCOMPARE(invalid.expected_schema.size(), 4);
            switch (field) {
            case 0:
                invalid.sql[0].append(QChar(0xd800));
                break;
            case 1:
                invalid.expected_schema[0].name.append(QChar(0xd800));
                break;
            case 2:
                invalid.expected_schema[3].sql = QString(QChar(0xd800));
                break;
            case 3:
                invalid.id.append(QChar(0xd800));
                break;
            }
            try {
                (void)canonical_migration_payload(invalid);
                QFAIL("Unpaired surrogate was accepted");
            } catch (const MigrationFailure &error) {
                QCOMPARE(error.reason(), MigrationReason::invalid_manifest);
            }
        }
    }

    void freshAdvanceCommitsWholePrefix() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QList<qint64> visible_history;
        auto input = request();
        input.cancel = [&](const MigrationProgress &progress) {
            if (progress.checkpoint == MigrationCheckpoint::before_history)
                visible_history.append(
                    fixture_
                        ->scalar(
                            "SELECT COUNT(*) FROM main.sqlite_schema WHERE name='subathon_catalog'")
                        .toLongLong());
            return false;
        };
        MigrationResult result;
        RUN_MIGRATION(writer, input, result);
        current(result, 2, 2);
        QCOMPARE(visible_history, QList<qint64>({0, 0}));
        QCOMPARE(fixture_->rows("SELECT id,amount FROM sample ORDER BY id"),
                 SqlRows({{1, 10}, {2, 20}}));
        QCOMPARE(fixture_->rows("SELECT version,migration_id,content_sha256 FROM "
                                "subathon_migration_history ORDER BY version"),
                 SqlRows({{1, "create-sample", digest(1)}, {2, "add-second", digest(2)}}));
    }

    void supportedOldAdvancesOnceAndCurrentDoesNotReplay() {
        QVERIFY(seed(1));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        MigrationResult result;
        RUN_MIGRATION(writer, request(), result);
        current(result, 2, 1);
        const auto after = fixture_->snapshot();
        RUN_MIGRATION(writer, request(), result);
        current(result, 2, 0);
        QCOMPARE(fixture_->snapshot(), after);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM sample").toInt(), 30);
    }

    void inspectionReportsFreshOldAndCurrentWithoutAdvancement() {
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            MigrationRequest input;
            QCOMPARE(input.mode, MigrationMode::inspect);
            input.manifest = base();
            MigrationResult result;
            RUN_MIGRATION(writer, input, result);
            QVERIFY(result.report.has_value());
            QCOMPARE(result.report->state, CatalogState::fresh);
            QCOMPARE(result.report->version, 0);
            QCOMPARE(result.report->applied, 0);
            QCOMPARE(result.work.transaction, TransactionOutcome::committed);
            QCOMPARE(result.work.error, WorkError::none);
            QVERIFY(result.work.value.has_value());
            QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM main.sqlite_schema").toInt(), 0);
        }
        for (const int version : {1, 2}) {
            reset_fixture();
            QVERIFY(seed(version));
            const auto before = fixture_->snapshot();
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            MigrationResult result;
            RUN_MIGRATION(writer, request(MigrationMode::inspect), result);
            QVERIFY(result.report.has_value());
            QCOMPARE(result.report->state,
                     version == 1 ? CatalogState::upgrade_required : CatalogState::current);
            QCOMPARE(result.report->version, version);
            QCOMPARE(result.report->target_version, 2);
            QCOMPARE(result.report->applied, 0);
            QCOMPARE(result.work.transaction, TransactionOutcome::committed);
            QCOMPARE(result.work.error, WorkError::none);
            QCOMPARE(fixture_->snapshot(), before);
        }
    }

    void foreignPartialAndMismatchingCatalogsAreRefused() {
        for (const int variant : {0, 1, 2, 3, 4}) {
            reset_fixture();
            MigrationReason expected = MigrationReason::catalog_mismatch;
            if (variant == 0) {
                QVERIFY(fixture_->exec("CREATE TABLE sentinel(id INTEGER,amount INTEGER)"));
                QVERIFY(fixture_->exec("INSERT INTO sentinel VALUES (7,70)"));
                expected = MigrationReason::foreign_store;
            } else if (variant == 1) {
                QVERIFY(fixture_->exec(header_ddl));
            } else if (variant == 2) {
                QVERIFY(fixture_->exec(history_ddl));
            } else if (variant == 3) {
                QVERIFY(seed(0));
            } else {
                QVERIFY(seed(1));
                QVERIFY(fixture_->exec("UPDATE subathon_catalog SET store_identity='other-store'"));
                expected = MigrationReason::foreign_store;
            }
            const auto before = fixture_->snapshot();
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            MigrationResult result;
            RUN_MIGRATION(writer, request(), result);
            failed(result, expected);
            QVERIFY(fixture_->lock_released());
            QCOMPARE(fixture_->snapshot(), before);
        }
    }

    void futureAndUnsupportedOldVersionsAreRefused() {
        for (const bool future : {false, true}) {
            reset_fixture();
            QVERIFY(seed(future ? 2 : 1));
            auto input = request();
            if (future)
                QVERIFY(
                    fixture_->exec("INSERT INTO subathon_migration_history VALUES (3,'future',?)",
                                   {QByteArray(32, 'x')}));
            else
                input.manifest.supported_starting_versions = {0, 2};
            const auto before = fixture_->snapshot();
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            MigrationResult result;
            RUN_MIGRATION(writer, input, result);
            failed(result, MigrationReason::unsupported_version);
            QCOMPARE(fixture_->snapshot(), before);
        }
    }

    void invalidManifestsAreRejectedBeforeAdmission() {
        for (int variant = 0; variant < 20; ++variant) {
            auto input = request();
            switch (variant) {
            case 0:
                input.manifest.steps.clear();
                break;
            case 1:
                input.manifest.steps[1].version = 1;
                break;
            case 2:
                input.manifest.steps[1].version = 3;
                break;
            case 3:
                input.manifest.steps[1].id = input.manifest.steps[0].id;
                break;
            case 4:
                input.manifest.supported_starting_versions = {0, 1, 1, 2};
                break;
            case 5:
                input.manifest.supported_starting_versions = {1, 2};
                break;
            case 6:
                input.manifest.supported_starting_versions = {0, 3};
                break;
            case 7:
                input.manifest.steps[0].id.clear();
                break;
            case 8:
                input.manifest.steps[0].id = "has space";
                break;
            case 9:
                input.manifest.steps[0].sql[0].append(QChar(0xd800));
                break;
            case 10:
                input.manifest.steps[0].expected_schema[0].name.append(QChar(0));
                break;
            case 11:
                input.limits.versions = 0;
                break;
            case 12:
                input.manifest.steps[0].id = QString(129, 'a');
                break;
            case 13:
                input.mode = static_cast<MigrationMode>(99);
                break;
            case 14:
                input.manifest.catalog_format = 2;
                break;
            case 15:
                input.manifest.supported_starting_versions = {0, 1};
                break;
            case 16:
                input.manifest.supported_starting_versions = {0, 2, 1};
                break;
            case 17:
                input.manifest.steps[0].id = QString::fromUtf8("é");
                break;
            case 18:
                input.manifest.steps[0].id = "has\tcontrol";
                break;
            case 19:
                input.manifest.steps[0].expected_schema.append(
                    input.manifest.steps[0].expected_schema[0]);
                break;
            }
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            bool called = false;
            const auto admission = SqliteMigrationRunner::enqueue(
                writer, input, [&](MigrationResult) { called = true; });
            QCOMPARE(admission.error, StorageError::invalid_options);
            QCOMPARE(admission.reason, MigrationReason::invalid_manifest);
            bool stopped = false;
            QCOMPARE(writer.shutdown([&] { stopped = true; }), StorageError::none);
            QTRY_VERIFY(stopped);
            QVERIFY(!called);
            QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM main.sqlite_schema").toInt(), 0);
        }
    }

    void historyAndSchemaMismatchesAreRefused() {
        for (int variant = 0; variant < 7; ++variant) {
            reset_fixture();
            QVERIFY(seed(2));
            auto input = request();
            MigrationReason expected = MigrationReason::catalog_mismatch;
            switch (variant) {
            case 0:
                QVERIFY(fixture_->exec(
                    "UPDATE subathon_migration_history SET content_sha256=? WHERE version=1",
                    {QByteArray(32, 'z')}));
                break;
            case 1:
                QVERIFY(fixture_->exec("DELETE FROM subathon_migration_history WHERE version=1"));
                break;
            case 2:
                input.manifest.steps[0].sql.append("SELECT 99");
                break;
            case 3:
                QVERIFY(fixture_->exec("CREATE TABLE extra(value TEXT)"));
                expected = MigrationReason::schema_mismatch;
                break;
            case 4:
                QVERIFY(fixture_->exec("DROP TABLE sample"));
                expected = MigrationReason::schema_mismatch;
                break;
            case 5:
                QVERIFY(fixture_->exec("UPDATE subathon_migration_history SET "
                                       "migration_id='changed' WHERE version=1"));
                break;
            case 6:
                // All required rows/history remain valid and physical integrity
                // is clean; only the reserved table's exact DDL has changed.
                QVERIFY(fixture_->exec("ALTER TABLE subathon_catalog ADD COLUMN extra TEXT"));
                QCOMPARE(fixture_->rows("SELECT singleton,catalog_format,store_identity FROM "
                                        "main.subathon_catalog"),
                         SqlRows({{1, 1, "subathon.sqlite-store"}}));
                QCOMPARE(fixture_->rows("SELECT version,migration_id,content_sha256 FROM "
                                        "main.subathon_migration_history ORDER BY version"),
                         SqlRows({{1, "create-sample", digest(1)}, {2, "add-second", digest(2)}}));
                QCOMPARE(fixture_->scalar("PRAGMA integrity_check").toString(), QString("ok"));
                expected = MigrationReason::schema_mismatch;
                break;
            }
            const auto before = fixture_->snapshot();
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            MigrationResult result;
            RUN_MIGRATION(writer, input, result);
            failed(result, expected);
            QVERIFY(fixture_->lock_released());
            QCOMPARE(fixture_->snapshot(), before);
        }
    }

    void storedTypesRangesAndCardinalityAreValidated() {
        for (int variant = 0; variant < 9; ++variant) {
            reset_fixture();
            QVERIFY(seed(1));
            MigrationReason expected = MigrationReason::catalog_mismatch;
            switch (variant) {
            case 0:
                QVERIFY(fixture_->exec(
                    "INSERT INTO subathon_catalog VALUES (2,1,'subathon.sqlite-store')"));
                break;
            case 1:
                QVERIFY(fixture_->exec("UPDATE subathon_catalog SET catalog_format='wrong'"));
                break;
            case 2:
                QVERIFY(fixture_->exec("UPDATE subathon_catalog SET store_identity=X'FF'"));
                break;
            case 3:
                QVERIFY(fixture_->exec("UPDATE subathon_migration_history SET version=-1"));
                break;
            case 4:
                QVERIFY(fixture_->exec("UPDATE subathon_migration_history SET version=2147483648"));
                break;
            case 5:
                QVERIFY(
                    fixture_->exec("UPDATE subathon_migration_history SET content_sha256=X'01'"));
                break;
            case 6:
                QVERIFY(fixture_->exec("UPDATE subathon_migration_history SET "
                                       "content_sha256='01234567890123456789012345678901'"));
                break;
            case 7:
                QVERIFY(fixture_->exec(
                    "UPDATE subathon_migration_history SET migration_id=CAST(X'FF' AS TEXT)"));
                break;
            case 8:
                QVERIFY(fixture_->exec("UPDATE subathon_catalog SET catalog_format=2"));
                expected = MigrationReason::unsupported_catalog_format;
                break;
            }
            const auto before = fixture_->snapshot();
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            MigrationResult result;
            RUN_MIGRATION(writer, request(), result);
            failed(result, expected);
            QCOMPARE(fixture_->snapshot(), before);
        }
    }

    void checkConstraintIntegrityFailurePreservesData() {
        QVERIFY(seed_profile("checked"));
        QVERIFY(fixture_->scalar("PRAGMA integrity_check").toString() != "ok");
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        MigrationResult result;
        RUN_MIGRATION(writer, profile_request("checked"), result);
        failed(result, MigrationReason::integrity_failed);
        QCOMPARE(fixture_->snapshot(), before);
        QCOMPARE(fixture_->scalar("SELECT n FROM checked").toInt(), -1);
        QVERIFY(fixture_->lock_released());
    }

    void foreignKeyFailureCannotPassIntegrity() {
        QVERIFY(seed_profile("foreign_keys"));
        QCOMPARE(fixture_->scalar("PRAGMA integrity_check").toString(), QString("ok"));
        QCOMPARE(fixture_->rows("PRAGMA foreign_key_check"),
                 SqlRows({{"children", 1, "parents", 0}}));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        MigrationResult result;
        RUN_MIGRATION(writer, profile_request("foreign_keys"), result);
        failed(result, MigrationReason::integrity_failed);
        QCOMPARE(fixture_->snapshot(), before);
        QVERIFY(fixture_->lock_released());
    }

    void failedMigrationRollsBackDdlHistoryAndPriorWrites() {
        QVERIFY(seed(1));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto input = request();
        input.manifest.steps[1].sql = {"CREATE TABLE added(id INTEGER PRIMARY KEY, amount INTEGER)",
                                       "INSERT INTO added VALUES (2,20)",
                                       "INSERT INTO sample VALUES (1,999)"};
        input.manifest.steps[1].expected_schema.append(
            {"table", "added", "added",
             "CREATE TABLE added(id INTEGER PRIMARY KEY, amount INTEGER)"});
        bool prior_writes_reached = false;
        input.cancel = [&](const MigrationProgress &progress) {
            if (progress.checkpoint == MigrationCheckpoint::before_statement &&
                progress.version == 2 && progress.statement == 3)
                prior_writes_reached = true;
            return false;
        };
        MigrationResult result;
        RUN_MIGRATION(writer, input, result);
        failed(result, MigrationReason::sql_failure, WorkError::sql_failure);
        QVERIFY(prior_writes_reached);
        QVERIFY(fixture_->lock_released());
        QCOMPARE(fixture_->snapshot(), before);
        bool stopped = false;
        QCOMPARE(writer.shutdown([&] { stopped = true; }), StorageError::none);
        QTRY_VERIFY(stopped);
        SqliteWriter reopened;
        QCOMPARE(reopened.open(path_), StorageError::none);
        RUN_MIGRATION(reopened, request(), result);
        current(result, 2, 1);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM sample").toInt(), 30);
    }

    void cancellationRollsBackWholeBootstrapAndPrefix() {
        for (int variant = 0; variant < 5; ++variant) {
            reset_fixture();
            if (variant != 0 && variant != 3)
                QVERIFY(seed(1));
            const auto before = fixture_->snapshot();
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            auto input = request();
            bool checkpoint_reached = false;
            if (variant == 1) {
                input.manifest.steps[1].sql = {"CREATE TABLE added(value INTEGER)",
                                               "INSERT INTO added VALUES (20)"};
                input.manifest.steps[1].expected_schema.append(
                    {"table", "added", "added", "CREATE TABLE added(value INTEGER)"});
            }
            input.cancel = [&](const MigrationProgress &progress) {
                const bool match =
                    (variant == 0 && progress.checkpoint == MigrationCheckpoint::before_history &&
                     progress.version == 1) ||
                    (variant == 1 && progress.checkpoint == MigrationCheckpoint::before_statement &&
                     progress.version == 2 && progress.statement == 2) ||
                    (variant == 2 && progress.checkpoint == MigrationCheckpoint::before_commit) ||
                    (variant == 3 && progress.checkpoint == MigrationCheckpoint::before_begin) ||
                    (variant == 4 && progress.checkpoint == MigrationCheckpoint::before_history &&
                     progress.version == 2);
                if (!match)
                    return false;
                checkpoint_reached = true;
                if (variant == 4)
                    throw std::runtime_error("synthetic cancellation-probe failure");
                return true;
            };
            MigrationResult result;
            RUN_MIGRATION(writer, input, result);
            failed(result,
                   variant == 4 ? MigrationReason::callback_failure : MigrationReason::cancelled,
                   WorkError::work_exception,
                   variant == 3 ? TransactionOutcome::not_started
                                : TransactionOutcome::rolled_back);
            QVERIFY(checkpoint_reached);
            QVERIFY(fixture_->lock_released());
            QCOMPARE(fixture_->snapshot(), before);
        }
    }

    void busyMigrationDoesNotReplay() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QVERIFY(fixture_->exec("BEGIN IMMEDIATE"));
        int starts = 0;
        auto input = request();
        input.cancel = [&](const MigrationProgress &progress) {
            if (progress.checkpoint == MigrationCheckpoint::before_begin)
                ++starts;
            return false;
        };
        MigrationResult result;
        RUN_MIGRATION(writer, input, result);
        failed(result, MigrationReason::busy, WorkError::busy, TransactionOutcome::not_started);
        QCOMPARE(starts, 1);
        QVERIFY(fixture_->exec("ROLLBACK"));
        RUN_MIGRATION(writer, request(), result);
        current(result, 2, 2);
        QCOMPARE(starts, 1);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM sample").toInt(), 30);
    }

    void lostCommitAckFailsClosedWithPhysicalTarget() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_commit_ack),
                 StorageError::none);
        QList<int> order;
        MigrationResult first, next;
        WriteResult old_batch;
        int queued_starts = 0;
        auto admission =
            SqliteMigrationRunner::enqueue(writer, request(), [&](MigrationResult value) {
                first = std::move(value);
                order.append(1);
            });
        QCOMPARE(admission.error, StorageError::none);
        QCOMPARE(admission.reason, MigrationReason::none);
        QCOMPARE(writer.enqueue({{"INSERT INTO sample VALUES (3,30)", {}}},
                                [&](WriteResult value) {
                                    old_batch = std::move(value);
                                    order.append(2);
                                }),
                 StorageError::none);
        auto queued = request();
        queued.cancel = [&](const MigrationProgress &) {
            ++queued_starts;
            return false;
        };
        admission = SqliteMigrationRunner::enqueue(writer, queued, [&](MigrationResult value) {
            next = std::move(value);
            order.append(3);
        });
        QCOMPARE(admission.error, StorageError::none);
        QTRY_COMPARE(order.size(), 3);
        QCOMPARE(order, QList<int>({1, 2, 3}));
        failed(first, MigrationReason::uncertain_outcome, WorkError::uncertain_outcome,
               TransactionOutcome::outcome_unknown);
        failed(next, MigrationReason::writer_unavailable, WorkError::writer_unavailable,
               TransactionOutcome::not_started);
        QCOMPARE(old_batch.error, StorageError::sql_failure);
        QVERIFY(old_batch.rows.isEmpty());
        QCOMPARE(queued_starts, 0);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM sample").toInt(), 30);
        QCOMPARE(
            fixture_->rows(
                "SELECT version,content_sha256 FROM subathon_migration_history ORDER BY version"),
            SqlRows({{1, digest(1)}, {2, digest(2)}}));
        admission = SqliteMigrationRunner::enqueue(writer, request(), [](MigrationResult) {});
        QCOMPARE(admission.error, StorageError::shutting_down);
        QCOMPARE(admission.reason, MigrationReason::none);
        QCOMPARE(writer.open(path_), StorageError::shutting_down);
    }

    void lostRollbackAckOverridesSqlFailure() {
        QVERIFY(seed(1));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_rollback_ack),
                 StorageError::none);
        auto input = request();
        input.manifest.steps[1].sql = {"CREATE TABLE added(value INTEGER)",
                                       "INSERT INTO added VALUES (20)",
                                       "INSERT INTO sample VALUES (1,999)"};
        input.manifest.steps[1].expected_schema.append(
            {"table", "added", "added", "CREATE TABLE added(value INTEGER)"});
        QList<int> order;
        MigrationResult first, next;
        WriteResult old_batch;
        int queued_starts = 0;
        auto admission = SqliteMigrationRunner::enqueue(writer, input, [&](MigrationResult value) {
            first = std::move(value);
            order.append(1);
        });
        QCOMPARE(admission.error, StorageError::none);
        QCOMPARE(writer.enqueue({{"INSERT INTO sample VALUES (3,30)", {}}},
                                [&](WriteResult value) {
                                    old_batch = std::move(value);
                                    order.append(2);
                                }),
                 StorageError::none);
        auto queued = request();
        queued.cancel = [&](const MigrationProgress &) {
            ++queued_starts;
            return false;
        };
        admission = SqliteMigrationRunner::enqueue(writer, queued, [&](MigrationResult value) {
            next = std::move(value);
            order.append(3);
        });
        QCOMPARE(admission.error, StorageError::none);
        QTRY_COMPARE(order.size(), 3);
        QCOMPARE(order, QList<int>({1, 2, 3}));
        failed(first, MigrationReason::uncertain_outcome, WorkError::uncertain_outcome,
               TransactionOutcome::outcome_unknown);
        failed(next, MigrationReason::writer_unavailable, WorkError::writer_unavailable,
               TransactionOutcome::not_started);
        QCOMPARE(old_batch.error, StorageError::sql_failure);
        QVERIFY(old_batch.rows.isEmpty());
        QCOMPARE(queued_starts, 0);
        QCOMPARE(fixture_->snapshot(), before);
        admission = SqliteMigrationRunner::enqueue(writer, request(), [](MigrationResult) {});
        QCOMPARE(admission.error, StorageError::shutting_down);
    }

    void cumulativeBudgetsAndManifestLimitsHaveExactBoundaries() {
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            auto exact = request();
            exact.limits = {2, 13, 4, 1127}; // Independent frozen literal counts/UTF-8 sum.
            QCOMPARE(literals_["base"].toObject()["manifest_utf8_bytes"].toInt(), 1127);
            MigrationResult result;
            RUN_MIGRATION(writer, exact, result);
            current(result, 2, 2);
        }
        reset_fixture();
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            auto exact_id = request();
            exact_id.manifest.steps.resize(1);
            exact_id.manifest.supported_starting_versions = {0, 1};
            exact_id.manifest.steps[0].id = QString(128, 'a');
            exact_id.limits.id_bytes = 256; // Config cannot lift the hard 128-byte cap.
            MigrationResult result;
            RUN_MIGRATION(writer, exact_id, result);
            current(result, 1, 1);
            QCOMPARE(fixture_->scalar("SELECT length(migration_id) FROM subathon_migration_history")
                         .toInt(),
                     128);
        }
        for (int variant = 0; variant < 5; ++variant) {
            reset_fixture();
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            auto over = request();
            over.limits = {2, 13, 4, 1127};
            switch (variant) {
            case 0:
                over.limits.versions = 1;
                break;
            case 1:
                over.limits.id_bytes = 12;
                break;
            case 2:
                over.limits.schema_objects = 3;
                break;
            case 3:
                over.limits.manifest_utf8_bytes = 1126;
                break;
            case 4:
                over.limits.id_bytes = 256;
                over.manifest.steps[0].id = QString(129, 'a');
                break;
            }
            bool called = false;
            const auto admission = SqliteMigrationRunner::enqueue(
                writer, over, [&](MigrationResult) { called = true; });
            QCOMPARE(admission.error, StorageError::invalid_options);
            QCOMPARE(admission.reason, MigrationReason::invalid_manifest);
            bool stopped = false;
            QCOMPARE(writer.shutdown([&] { stopped = true; }), StorageError::none);
            QTRY_VERIFY(stopped);
            QVERIFY(!called);
            QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM main.sqlite_schema").toInt(), 0);
        }
        reset_fixture();
        QVERIFY(seed(1));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        StorageLimits limits;
        limits.result_value_bytes = 65536;
        QCOMPARE(writer.open(path_, limits), StorageError::none);
        auto oversized = request();
        oversized.manifest.steps[1].sql = {"CREATE TABLE scratch AS SELECT 10 AS amount",
                                           "SELECT zeroblob(65537) FROM main.scratch"};
        oversized.manifest.steps[1].expected_schema.append(
            {"table", "scratch", "scratch", "CREATE TABLE scratch(amount)"});
        bool reached_second = false;
        oversized.cancel = [&](const MigrationProgress &progress) {
            if (progress.checkpoint == MigrationCheckpoint::before_statement &&
                progress.version == 2 && progress.statement == 2)
                reached_second = true;
            return false;
        };
        MigrationResult result;
        RUN_MIGRATION(writer, oversized, result);
        failed(result, MigrationReason::resource_limit, WorkError::result_limit);
        // SELECT references the real newly created/populated table; absent DDL
        // would be sql_failure, empty output would not exhaust this byte budget.
        QVERIFY(reached_second);
        QVERIFY(fixture_->lock_released());
        QCOMPARE(fixture_->snapshot(), before);
    }

    void diagnosticShadowsAreRefusedWithoutRepair() {
        for (const auto &name : {QString("pragma_encoding"), QString("pragma_integrity_check"),
                                 QString("pragma_foreign_key_check")}) {
            reset_fixture();
            QVERIFY(seed_profile(name));
            const auto before = fixture_->snapshot();
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            MigrationResult result;
            RUN_MIGRATION(writer, profile_request(name), result);
            failed(result, MigrationReason::inspection_failed, WorkError::sql_failure);
            QCOMPARE(fixture_->snapshot(), before);
            QVERIFY(fixture_->lock_released());
        }
    }

    void utf16StoresAreRefusedWithoutConversion() {
        for (const auto &encoding : {QString("UTF-16le"), QString("UTF-16be")}) {
            reset_fixture();
            QVERIFY(fixture_->exec("PRAGMA encoding='" + encoding + "'"));
            QVERIFY(seed(1));
            const auto before = fixture_->snapshot();
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            MigrationResult result;
            RUN_MIGRATION(writer, request(), result);
            failed(result, MigrationReason::unsupported_encoding);
            QCOMPARE(fixture_->scalar("PRAGMA encoding").toString(), encoding);
            QCOMPARE(fixture_->snapshot(), before);
        }
    }

    void ownedRequestReportAndAdmissionRemainBounded() {
        MigrationResult owned;
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            auto caller = request();
            bool done = false;
            const auto admission =
                SqliteMigrationRunner::enqueue(writer, caller, [&](MigrationResult result) {
                    owned = std::move(result);
                    done = true;
                });
            QCOMPARE(admission.error, StorageError::none);
            caller.manifest.steps[0].sql[0] = "DROP TABLE sample";
            caller.manifest.steps.clear();
            caller.manifest.store_identity.clear();
            QTRY_VERIFY(done);
            current(owned, 2, 2);
        }
        QVERIFY(owned.report.has_value());
        QVERIFY(owned.work.value.has_value());
        QCOMPARE(owned.report->version, 2);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM sample").toInt(), 30);
        SqliteWriter closed;
        auto invalid = request();
        invalid.mode = static_cast<MigrationMode>(99);
        auto admission = SqliteMigrationRunner::enqueue(closed, invalid, {});
        QCOMPARE(admission.error, StorageError::invalid_options);
        QCOMPARE(admission.reason, MigrationReason::invalid_manifest);
        admission = SqliteMigrationRunner::enqueue(closed, request(), {});
        QCOMPARE(admission.error, StorageError::invalid_work);
        admission = SqliteMigrationRunner::enqueue(closed, request(), [](MigrationResult) {});
        QCOMPARE(admission.error, StorageError::not_open);
        QCOMPARE(admission.reason, MigrationReason::none);
        MigrationAdmission invalid_foreign, valid_foreign;
        std::thread other([&] {
            invalid_foreign =
                SqliteMigrationRunner::enqueue(closed, invalid, [](MigrationResult) {});
            valid_foreign =
                SqliteMigrationRunner::enqueue(closed, request(), [](MigrationResult) {});
        });
        other.join();
        QCOMPARE(invalid_foreign.error, StorageError::invalid_options);
        QCOMPARE(invalid_foreign.reason, MigrationReason::invalid_manifest);
        QCOMPARE(valid_foreign.error, StorageError::wrong_thread);
        QCOMPARE(valid_foreign.reason, MigrationReason::none);
        SqliteWriter writer;
        StorageLimits limits;
        limits.pending_batches = 1;
        QCOMPARE(writer.open(path_, limits), StorageError::none);
        bool done = false, stopped = false, rejected_called = false;
        admission = SqliteMigrationRunner::enqueue(writer, request(),
                                                   [&](MigrationResult) { done = true; });
        QCOMPARE(admission.error, StorageError::none);
        admission = SqliteMigrationRunner::enqueue(
            writer, invalid, [&](MigrationResult) { rejected_called = true; });
        QCOMPARE(admission.error, StorageError::invalid_options);
        admission = SqliteMigrationRunner::enqueue(
            writer, request(), [&](MigrationResult) { rejected_called = true; });
        QCOMPARE(admission.error, StorageError::queue_full);
        QCOMPARE(writer.shutdown([&] { stopped = true; }), StorageError::none);
        admission = SqliteMigrationRunner::enqueue(
            writer, invalid, [&](MigrationResult) { rejected_called = true; });
        QCOMPARE(admission.error, StorageError::invalid_options);
        admission = SqliteMigrationRunner::enqueue(
            writer, request(), [&](MigrationResult) { rejected_called = true; });
        QCOMPARE(admission.error, StorageError::shutting_down);
        QTRY_VERIFY(done && stopped);
        QVERIFY(!rejected_called);
    }

    void shutdownAndCompletionLifetimePreserveWriterRules() {
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            QList<int> order;
            MigrationResult first, inspection;
            auto admission =
                SqliteMigrationRunner::enqueue(writer, request(), [&](MigrationResult value) {
                    first = std::move(value);
                    order.append(1);
                });
            QCOMPARE(admission.error, StorageError::none);
            QCOMPARE(writer.enqueue({{"INSERT INTO sample VALUES (3,30)", {}}},
                                    [&](WriteResult result) {
                                        QCOMPARE(result.error, StorageError::none);
                                        order.append(2);
                                    }),
                     StorageError::none);
            admission = SqliteMigrationRunner::enqueue(writer, request(MigrationMode::inspect),
                                                       [&](MigrationResult value) {
                                                           inspection = std::move(value);
                                                           order.append(3);
                                                       });
            QCOMPARE(admission.error, StorageError::none);
            SqliteWriter replacement;
            StorageError reopened = StorageError::not_open;
            QCOMPARE(writer.shutdown([&] {
                reopened = replacement.open(path_);
                order.append(4);
            }),
                     StorageError::none);
            QTRY_COMPARE(order.size(), 4);
            QCOMPARE(order, QList<int>({1, 2, 3, 4}));
            QCOMPARE(reopened, StorageError::none);
            current(first, 2, 2);
            current(inspection, 2, 0);
            QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM sample").toInt(), 60);
        }
        reset_fixture();
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            bool called = false, next = false;
            MigrationResult result;
            auto admission =
                SqliteMigrationRunner::enqueue(writer, request(), [&](MigrationResult value) {
                    result = std::move(value);
                    called = true;
                    throw std::runtime_error("synthetic completion failure");
                });
            QCOMPARE(admission.error, StorageError::none);
            QCOMPARE(writer.enqueue({{"INSERT INTO sample VALUES (3,30)", {}}},
                                    [&](WriteResult value) {
                                        QCOMPARE(value.error, StorageError::none);
                                        next = true;
                                    }),
                     StorageError::none);
            QTRY_VERIFY(called && next);
            current(result, 2, 2);
            QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM sample").toInt(), 60);
        }
        reset_fixture();
        auto writer = std::make_unique<SqliteWriter>();
        QCOMPARE(writer->open(path_), StorageError::none);
        bool destroyed = false;
        MigrationResult result;
        auto admission =
            SqliteMigrationRunner::enqueue(*writer, request(), [&](MigrationResult value) {
                result = std::move(value);
                writer.reset();
                destroyed = true;
            });
        QCOMPARE(admission.error, StorageError::none);
        QTRY_VERIFY(destroyed);
        QVERIFY(!writer);
        current(result, 2, 2);
        SqliteWriter replacement;
        QCOMPARE(replacement.open(path_), StorageError::none);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM sample").toInt(), 30);
    }

    void canonicalCodecPreservesInitialBom() {
        const auto step = bom_step();
        const QString name = QString(QChar(0xfeff)) + 'a';
        QCOMPARE(name.size(), 2);
        QCOMPARE(name.front(), QChar(0xfeff));
        QCOMPARE(name.toUtf8(), QByteArray::fromHex("efbbbf61"));
        QByteArray payload, digest;
        try {
            payload = canonical_migration_payload(step);
            digest = migration_content_sha256(step);
        } catch (const MigrationFailure &error) {
            QFAIL(qPrintable(QString("Valid initial U+FEFF was rejected by the codec, reason=%1")
                                 .arg(static_cast<int>(error.reason()))));
        }
        QCOMPARE(payload, bom_payload);
        QCOMPARE(payload.size(), 638);
        QCOMPARE(digest, bom_digest);
    }

    void storedSchemaPreservesInitialBom() {
        const auto step = bom_step();
        const QString name = QString(QChar(0xfeff)) + 'a';
        QVERIFY(fixture_->exec(header_ddl));
        QVERIFY(fixture_->exec(history_ddl));
        QVERIFY(
            fixture_->exec("INSERT INTO subathon_catalog VALUES (1,1,'subathon.sqlite-store')"));
        // Literal quoted SQL keeps U+FEFF in the identifier; binding a leading
        // mark through some SQLite text APIs can consume it before storage.
        QVERIFY(fixture_->exec("CREATE TABLE " + quoted(name) + "(value INTEGER)"));
        QVERIFY(fixture_->exec("INSERT INTO " + quoted(name) + " VALUES (17)"));
        QVERIFY(fixture_->exec("INSERT INTO subathon_migration_history VALUES (1,'bom-name',?)",
                               {bom_digest}));
        const auto observed = fixture_->rows(
            "SELECT name,tbl_name,CAST(name AS BLOB),CAST(tbl_name AS BLOB) "
            "FROM main.sqlite_schema WHERE type='table' AND name!='subathon_catalog' "
            "AND name!='subathon_migration_history'");
        QCOMPARE(observed.size(), 1);
        QCOMPARE(observed[0].size(), 4);
        QCOMPARE(observed[0][0].toString(), name);
        QCOMPARE(observed[0][1].toString(), name);
        QCOMPARE(observed[0][0].toString().toUtf8(), QByteArray::fromHex("efbbbf61"));
        QCOMPARE(observed[0][2].metaType().id(), QMetaType::QByteArray);
        QCOMPARE(observed[0][3].metaType().id(), QMetaType::QByteArray);
        QCOMPARE(observed[0][2].toByteArray(), QByteArray::fromHex("efbbbf61"));
        QCOMPARE(observed[0][3].toByteArray(), QByteArray::fromHex("efbbbf61"));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        MigrationRequest input;
        input.mode = MigrationMode::inspect;
        input.manifest.steps = {step};
        input.manifest.supported_starting_versions = {0, 1};
        MigrationResult result;
        RUN_MIGRATION(writer, input, result);
        current(result, 1, 0);
        QCOMPARE(fixture_->scalar("SELECT value FROM main." + quoted(name)).toInt(), 17);
        QCOMPARE(fixture_->snapshot(), before);
        QVERIFY(fixture_->lock_released());
    }
};

QTEST_GUILESS_MAIN(SqliteMigrationTests)
#include "sqlite_migration_tests.moc"
