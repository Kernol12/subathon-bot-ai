// SPDX-License-Identifier: GPL-3.0-or-later
// Independent foundation: old writer/migration plus selected native SDK only.
// This executable does not link or include the new storage/supplier implementation.
#include "support/sqlite_account_sessions_fixture.hpp"
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QVersionNumber>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <sodium.h>

using namespace account_sessions_fixture;
namespace {
QByteArray frame(const QByteArray &value) { return QByteArray::number(value.size()) + ':' + value; }
QByteArray payload(const QJsonObject &manifest) {
    auto result = frame("migration-catalog-v1") +
                  frame(QByteArray::number(manifest["version"].toInt())) +
                  frame(manifest["id"].toString().toUtf8());
    const auto statements = manifest["sql"].toArray(),
               objects = manifest["expected_schema"].toArray();
    result += frame(QByteArray::number(statements.size()));
    for (const auto &statement : statements)
        result += frame(statement.toString().toUtf8());
    result += frame(QByteArray::number(objects.size()));
    for (const auto &entry : objects) {
        const auto object = entry.toObject();
        for (const auto *key : {"type", "name", "table_name"})
            result += frame(object[key].toString().toUtf8());
        result += frame(object["sql"].isNull() ? "0" : "1");
        if (!object["sql"].isNull())
            result += frame(object["sql"].toString().toUtf8());
    }
    return result;
}
void catalog(Observer &observer, const Data &data) {
    const auto expected = data.frozen["manifest"].toObject()["expected_schema"].toArray();
    const auto actual =
        observer.rows("SELECT type,name,tbl_name,sql FROM main.sqlite_schema ORDER BY type,name");
    require(actual.size() == 102 && expected.size() == 102, "Full literal 102-object head");
    for (qsizetype i = 0; i < actual.size(); ++i) {
        const auto object = expected[i].toObject();
        require(actual[i].size() == 4, "Complete schema tuple");
        for (int column = 0; column < 3; ++column) {
            const char *key = column == 0 ? "type" : column == 1 ? "name" : "table_name";
            require(actual[i][column].metaType().id() == QMetaType::QString &&
                        actual[i][column].toString() == object[key].toString(),
                    "Exact schema names/types");
        }
        require(object["sql"].isNull() ? actual[i][3].isNull()
                                       : actual[i][3].metaType().id() == QMetaType::QString &&
                                             actual[i][3].toString() == object["sql"].toString(),
                "Exact SQL including autoindex NULL");
    }
    auto manifests = data.prefixes;
    manifests.append(data.frozen["manifest"].toObject());
    const auto history = observer.rows("SELECT version,migration_id,content_sha256 FROM "
                                       "main.subathon_migration_history ORDER BY version");
    require(history.size() == 8, "Complete unchanged-prefix history");
    for (qsizetype i = 0; i < history.size(); ++i) {
        require(history[i][0].toLongLong() == i + 1 &&
                    history[i][1].metaType().id() == QMetaType::QString &&
                    history[i][1].toString() == manifests[i]["id"].toString() &&
                    history[i][2].metaType().id() == QMetaType::QByteArray &&
                    history[i][2].toByteArray().toHex() ==
                        manifests[i]["sha256_hex"].toString().toLatin1(),
                "Literal version/id/hash preserved by real old migration runner");
    }
    healthy(observer);
}
} // namespace
class SqliteAccountSessionsEnvironment final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
        QVERIFY(QT_VERSION >= QT_VERSION_CHECK(6, 12, 0));
        QVERIFY(QVersionNumber::fromString(QString::fromLatin1(qVersion())) >=
                QVersionNumber(6, 12, 0));
        QVERIFY(QSqlDatabase::isDriverAvailable("QSQLITE"));
    }
    void selectedSdkInitializationAndFixedRandomCall() {
        scenario([] {
            require(std::strcmp(SODIUM_VERSION_STRING, "1.0.22") == 0 &&
                        SODIUM_LIBRARY_VERSION_MAJOR == 26 && SODIUM_LIBRARY_VERSION_MINOR == 4,
                    "Selected libsodium header 1.0.22 ABI26/4");
            const auto initialized = sodium_init();
            require((initialized == 0 || initialized == 1) && sodium_init() == 1 &&
                        std::strcmp(sodium_version_string(), "1.0.22") == 0 &&
                        sodium_library_version_major() == 26 && sodium_library_version_minor() == 4,
                    "Actual initialized selected native runtime");
            std::array<unsigned char, 34> buffer{};
            buffer.front() = 0x31;
            buffer.back() = 0x79;
            randombytes_buf(buffer.data() + 1, 32);
            require(buffer.front() == 0x31 && buffer.back() == 0x79,
                    "Actual fixed32 random call preserves guards");
            sodium_memzero(buffer.data(), buffer.size());
            // No expected random bytes or historical uniqueness claim follows from this call.
        });
    }
    void literalPrefixFullSchemaAndCatalogFrames() {
        scenario([] {
            const Data data;
            require(data.prefixes.size() == 7, "Seven frozen prefixes");
            const auto pins = data.frozen["prefixes"].toArray();
            for (qsizetype i = 0; i < data.prefixes.size(); ++i) {
                const auto encoded = payload(data.prefixes[i]);
                const auto pin = pins[i].toObject();
                require(encoded.size() == pin["payload_bytes"].toInt() &&
                            QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex() ==
                                pin["sha256_hex"].toString().toLatin1(),
                        "Independent framing of every old prefix");
            }
            const auto manifest = data.frozen["manifest"].toObject();
            const auto encoded = payload(manifest);
            require(encoded.size() == 77258 && manifest["sql"].toArray().size() == 5 &&
                        QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex() ==
                            "ec41d4b3bf7d04a3e846b401060cc3ed9430020db3dfce0e2650ed76f6a46d5b",
                    "Independent proposed8 five-statement/full102 literal frame");
            Database database(false);
            Observer observer(database.path);
            catalog(observer, database.data);
            for (const auto *table : {"subathon_account_installation", "subathon_accounts",
                                      "subathon_local_credentials", "subathon_account_sessions"})
                require(observer.rows(QString("SELECT COUNT(*) FROM main.") + table) ==
                            SqlRows{{0}},
                        "No seeded account/install/credential/session");
        });
    }
    void realConstraintsExactBlobClassesAndSignedExtremes() {
        scenario([] {
            Database database;
            for (const auto &sql : database.data.frozen["constraints"].toArray())
                constraint_refusal(database.writer, sql.toString());
            Observer observer(database.path);
            const auto typed =
                observer.rows("SELECT "
                              "typeof(installation_ref),typeof(digest),length(digest),typeof("
                              "lifetime_started_utc_ms),typeof(encoded_verifier) FROM "
                              "main.subathon_account_sessions JOIN main.subathon_local_credentials "
                              "USING(installation_ref,account_ref)");
            require(typed == SqlRows{{QString("blob"), QString("blob"), qint64(32),
                                      QString("integer"), QString("blob")}},
                    "Actual QtSQLite exact SQL classes");
            (void)run(database.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                (void)s.execute(
                    {"UPDATE main.subathon_account_sessions SET "
                     "lifetime_started_utc_ms=?,authenticated_utc_ms=?,expires_utc_ms=?",
                     {qint64(std::numeric_limits<std::int64_t>::min()),
                      qint64(std::numeric_limits<std::int64_t>::min()),
                      qint64(std::numeric_limits<std::int64_t>::max())}});
                s.commit();
                return 1;
            });
            const auto extremes =
                observer.rows("SELECT lifetime_started_utc_ms,authenticated_utc_ms,expires_utc_ms "
                              "FROM main.subathon_account_sessions");
            require(extremes.size() == 1 && extremes[0].size() == 3, "Exact signed tuple");
            for (const auto &value : extremes[0])
                require(value.metaType().id() == QMetaType::LongLong,
                        "QSQLITE signed64 QVariant, no REAL/text");
            require(extremes[0][0].toLongLong() == std::numeric_limits<std::int64_t>::min() &&
                        extremes[0][1].toLongLong() == std::numeric_limits<std::int64_t>::min() &&
                        extremes[0][2].toLongLong() == std::numeric_limits<std::int64_t>::max(),
                    "Both signed extremes survive actual driver");
            healthy(observer);
        });
    }
    void blobIndexRangeAndLiteralCorruptionMechanism() {
        scenario([] {
            Database database;
            Observer observer(database.path);
            const auto first = observer.rows(
                "EXPLAIN QUERY PLAN SELECT session_ref FROM main.subathon_account_sessions WHERE "
                "installation_ref=X'49' AND account_ref=X'41' ORDER BY session_ref ASC LIMIT 3");
            const auto after = observer.rows(
                "EXPLAIN QUERY PLAN SELECT session_ref FROM main.subathon_account_sessions WHERE "
                "installation_ref=X'49' AND account_ref=X'41' AND session_ref>X'40' ORDER BY "
                "session_ref ASC LIMIT 3");
            for (const auto &rows : {first, after}) {
                require(rows.size() == 1 && rows[0].size() == 4, "One selected index-range plan");
                const auto detail = rows[0][3].toString();
                require(detail.contains("SEARCH") &&
                            detail.contains("subathon_account_sessions_page_idx") &&
                            !detail.contains("SCAN") && !detail.contains("TEMP B-TREE"),
                        "Actual selected engine index/no global scan or sort");
            }
            corrupt(database.path, {"UPDATE main.subathon_account_sessions SET "
                                    "retention=2,state=1,expires_utc_ms=1000"});
            require(
                observer.rows(
                    "SELECT retention,state,expires_utc_ms FROM main.subathon_account_sessions") ==
                    SqlRows{{2, 1, 1000}},
                "Isolated malformed selected row genuinely persisted");
            // The writable diagnostic connection is closed before any product read.
        });
    }
    void additiveUpgradeReopenAndOlderExecutableRefusal() {
        scenario([] {
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable migration/reopen directory");
            const auto path = directory.filePath("upgrade.sqlite");
            const Data data;
            {
                SqliteWriter writer;
                require(writer.open(path, limits()) == StorageError::none,
                        "Old writer opens prefix");
                const auto old = data.migrate(writer, false);
                require(old.reason == MigrationReason::none && old.report &&
                            old.report->version == 7,
                        "Actual immutable prefix7 first");
                const auto advance = data.migrate(writer);
                require(advance.reason == MigrationReason::none && advance.report &&
                            advance.report->applied == 1 && advance.report->version == 8,
                        "Exactly one additive literal step");
                (void)run(writer, [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    data.populate(s);
                    s.commit();
                    return 1;
                });
                const auto narrow = data.migrate(writer, false, MigrationMode::inspect);
                require(narrow.reason == MigrationReason::unsupported_version && !narrow.report,
                        "Old executable cannot silently accept newer schema");
            }
            SqliteWriter reopened;
            require(reopened.open(path, limits()) == StorageError::none,
                    "Normal reopen of same database");
            const auto inspect = data.migrate(reopened, true, MigrationMode::inspect);
            require(inspect.reason == MigrationReason::none && inspect.report &&
                        inspect.report->version == 8 && inspect.report->applied == 0,
                    "No constructor/open migration or repair");
            Observer observer(path);
            catalog(observer, data);
            require(observer.rows("SELECT hex(session_epoch_ref),hex(revision_ref) FROM "
                                  "main.subathon_account_installation") ==
                        SqlRows{{QString("45"), QString("4A")}},
                    "Normal reopen preserves installation epoch/revision");
        });
    }
};
QTEST_GUILESS_MAIN(SqliteAccountSessionsEnvironment)
#include "sqlite_account_sessions_environment.moc"
