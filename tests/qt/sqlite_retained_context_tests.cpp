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
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <functional>
#include <memory>
#include <subathon/server/sqlite_retained_context.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <thread>

using namespace subathon::server;
using subathon::server::testing::SqliteWriterWorkAccess;
using subathon::server::testing::WorkControlFault;

namespace {
std::string opaque(const QByteArray &s) { return std::string(s.constData(), size_t(s.size())); }
RetainedKey config(std::string ref) { return {RetainedKind::configuration, std::move(ref)}; }
RetainedKey context(std::string ref) { return {RetainedKind::arrival_context, std::move(ref)}; }
RetainedDocument document(RetainedKey key, QByteArray value,
                          std::optional<ConfigurationReference> link = std::nullopt) {
    return {std::move(key), std::move(value), std::move(link)};
}
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
RetainedDocument must_find(const SqliteRetainedContextStore &store, const RetainedKey &key) {
    const auto found = store.find(key);
    require(found.has_value(), "expected retained row is missing");
    return *found;
}
template <class Operation> void expect_local(Operation operation, RetentionError expected) {
    try {
        operation();
    } catch (const RetentionFailure &failure) {
        if (failure.error() == expected)
            return;
        throw std::runtime_error(
            "wrong RetentionFailure category (feature_not_implemented=0), got " +
            std::to_string(int(failure.error())));
    }
    throw std::runtime_error("expected RetentionFailure was not thrown");
}
template <class Operation> void expect_work(Operation operation, WorkError expected) {
    try {
        operation();
    } catch (const SqliteWorkFailure &failure) {
        if (failure.error() == expected)
            return;
        throw std::runtime_error("wrong SqliteWorkFailure category");
    }
    throw std::runtime_error("expected SqliteWorkFailure was not thrown");
}

// Independent literal SQL seeds. Never calls the new factory/store or computes a
// catalog expectation through the implementation being tested.
class Fixture final {
  public:
    explicit Fixture(const QString &path)
        : name_(QUuid::createUuid().toString()), db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
    }
    ~Fixture() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }
    bool open() {
        // Only this independent negative-fixture connection permits dangling
        // stored-link seeds. Product writer FK enforcement remains unchanged.
        return db_.open() && exec("PRAGMA foreign_keys=OFF");
    }
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
            QTest::qFail(qPrintable(query.lastError().text()), __FILE__, __LINE__);
            return {};
        }
        SqlRows result;
        while (query.next()) {
            SqlRow row;
            for (int i = 0; i < query.record().count(); ++i)
                row.append(query.value(i));
            result.append(row);
        }
        if (query.lastError().isValid())
            QTest::qFail("Independent fixture fetch failed", __FILE__, __LINE__);
        return result;
    }
    QVariant scalar(const QString &sql) {
        const auto values = rows(sql);
        if (values.size() != 1 || values[0].size() != 1) {
            QTest::qFail("Independent fixture scalar shape", __FILE__, __LINE__);
            return {};
        }
        return values[0][0];
    }
    bool released() { return exec("BEGIN IMMEDIATE") && exec("ROLLBACK"); }
    QList<SqlRows> snapshot() {
        return {rows("SELECT type,name,tbl_name,sql FROM main.sqlite_schema ORDER BY type,name"),
                rows("SELECT typeof(reference),hex(reference),typeof(document),hex(document) "
                     "FROM main.subathon_retained_configurations ORDER BY reference"),
                rows("SELECT typeof(reference),hex(reference),typeof(document),hex(document),"
                     "typeof(configuration_ref),hex(configuration_ref) "
                     "FROM main.subathon_retained_contexts ORDER BY reference"),
                rows("SELECT singleton,catalog_format,store_identity FROM main.subathon_catalog"),
                rows("SELECT version,migration_id,hex(content_sha256) FROM "
                     "main.subathon_migration_history ORDER BY version")};
    }

  private:
    QString name_;
    QSqlDatabase db_;
};
struct Run {
    bool admitted{false}, done{false};
    SqliteWorkResult result;
    std::optional<RetentionError> local;
    std::optional<WorkError> synchronous;
    QString unexpected;
    QString problem() const {
        if (local)
            return QString("RetentionFailure %1 (feature_not_implemented=0)").arg(int(*local));
        return unexpected.isEmpty() ? QString("work category %1").arg(int(result.error))
                                    : unexpected;
    }
};
Run run(SqliteWriter &writer, std::function<QVariant(SqliteWorkSession)> body) {
    auto state = std::make_shared<Run>();
    const auto admission = writer.enqueue_work(
        [state, body = std::move(body)](SqliteWorkContext work) {
            try {
                return body(work.begin());
            } catch (const RetentionFailure &failure) {
                state->local = failure.error();
                throw;
            } catch (const SqliteWorkFailure &failure) {
                state->synchronous = failure.error();
                throw;
            } catch (const std::exception &failure) {
                state->unexpected = QString::fromUtf8(failure.what());
                throw;
            }
        },
        [state](SqliteWorkResult result) {
            state->result = std::move(result);
            state->done = true;
        });
    state->admitted = admission == StorageError::none;
    if (state->admitted && !QTest::qWaitFor([state] { return state->done; }, 5000))
        state->unexpected = "Timed out waiting for writer completion";
    return *state;
}
} // namespace

#define OK(run_result)                                                                             \
    QVERIFY((run_result).admitted && (run_result).done);                                           \
    QVERIFY2(!(run_result).local && (run_result).unexpected.isEmpty(),                             \
             qPrintable((run_result).problem()));                                                  \
    QCOMPARE((run_result).result.transaction, TransactionOutcome::committed);                      \
    QCOMPARE((run_result).result.error, WorkError::none);                                          \
    QVERIFY((run_result).result.value.has_value())

class SqliteRetainedContextTests final : public QObject {
    Q_OBJECT
  private:
    QJsonObject manifest_, vectors_;
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<Fixture> fixture_;
    QString path_;
    bool seed() {
        const auto objects = manifest_["expected_schema"].toArray();
        for (const auto &entry : objects) {
            const auto object = entry.toObject();
            if (object["name"] == "subathon_catalog" ||
                object["name"] == "subathon_migration_history") {
                if (!fixture_->exec(object["sql"].toString()))
                    return false;
            }
        }
        for (const auto &sql : manifest_["sql"].toArray())
            if (!fixture_->exec(sql.toString()))
                return false;
        return fixture_->exec("INSERT INTO main.subathon_catalog VALUES "
                              "(1,1,'subathon.sqlite-store')") &&
               fixture_->exec("INSERT INTO main.subathon_migration_history VALUES "
                              "(1,'retain-config-context-v1',?)",
                              {QByteArray::fromHex(manifest_["sha256_hex"].toString().toLatin1())});
    }
    bool seed_config(const QByteArray &ref, const QByteArray &value) {
        return fixture_->exec("INSERT INTO main.subathon_retained_configurations VALUES (?,?)",
                              {ref, value});
    }
    bool seed_context(const QByteArray &ref, const QByteArray &value, QVariant link = QVariant()) {
        return fixture_->exec("INSERT INTO main.subathon_retained_contexts VALUES (?,?,?)",
                              {ref, value, link});
    }
    qint64 count_config() {
        return fixture_->scalar("SELECT count(*) FROM main.subathon_retained_configurations")
            .toLongLong();
    }
    qint64 count_context() {
        return fixture_->scalar("SELECT count(*) FROM main.subathon_retained_contexts")
            .toLongLong();
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
        vectors_ = json.object();
        manifest_ = vectors_["manifest"].toObject();
        QCOMPARE(manifest_["payload_bytes"].toInt(), 2000);
        QCOMPARE(manifest_["sha256_hex"].toString(),
                 QString("0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e"));
    }
    void init() {
        directory_ = std::make_unique<QTemporaryDir>();
        QVERIFY(directory_->isValid());
        path_ = directory_->filePath("retained.sqlite");
        fixture_ = std::make_unique<Fixture>(path_);
        QVERIFY(fixture_->open());
        QVERIFY(seed());
    }
    void cleanup() {
        fixture_.reset();
        directory_.reset();
    }

    void manifestMatchesApprovedRetentionMigration() {
        MigrationManifest actual;
        try {
            actual = retained_context_manifest();
        } catch (const RetentionFailure &failure) {
            QFAIL(qPrintable(QString("RetentionFailure %1 (feature_not_implemented=0)")
                                 .arg(int(failure.error()))));
        }
        QCOMPARE(actual.catalog_format, 1);
        QCOMPARE(actual.store_identity, QString("subathon.sqlite-store"));
        QCOMPARE(actual.supported_starting_versions, QList<qint32>({0, 1}));
        QCOMPARE(actual.steps.size(), 1);
        const auto &step = actual.steps[0];
        QCOMPARE(step.version, 1);
        QCOMPARE(step.id, QString("retain-config-context-v1"));
        QList<QString> expected_sql;
        for (const auto &sql : manifest_["sql"].toArray())
            expected_sql.append(sql.toString());
        QCOMPARE(step.sql, expected_sql);
        QList<CatalogObject> expected;
        for (const auto &entry : manifest_["expected_schema"].toArray()) {
            const auto o = entry.toObject();
            expected.append(
                {o["type"].toString(), o["name"].toString(), o["table_name"].toString(),
                 o["sql"].isNull() ? std::nullopt : std::optional<QString>(o["sql"].toString())});
        }
        QCOMPARE(step.expected_schema, expected);
        const auto payload = QByteArray::fromHex(manifest_["payload_hex"].toString().toLatin1());
        QCOMPARE(payload.size(), 2000);
        // Existing separately implemented codec is an actual observation only;
        // expected payload/digest are complete independent frozen JSON literals.
        QCOMPARE(canonical_migration_payload(step), payload);
        QCOMPARE(migration_content_sha256(step),
                 QByteArray::fromHex(manifest_["sha256_hex"].toString().toLatin1()));
    }

    void retainsExactBytesAndResolvesAfterReopen() {
        const auto cfg = document(config("config-v1"), QByteArray::fromHex("00ff4100"));
        const auto ctx = document(context("ctx-v1"), QByteArray::fromHex("efbbbf6100ff"),
                                  ConfigurationReference{"config-v1"});
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            auto r = run(writer, [&](auto session) -> QVariant {
                SqliteRetainedContextStore store(session);
                auto input = cfg;
                require(store.retain(input) == RetainDisposition::inserted, "config insert");
                input.key.reference.assign("mutated-after-return");
                input.document.fill('z');
                require(store.retain(ctx) == RetainDisposition::inserted, "context insert");
                require(store.find(cfg.key) == cfg && store.find(ctx.key) == ctx,
                        "same session bytes");
                session.commit();
                require(count_config() == 1 && count_context() == 1, "visible before body success");
                return 2;
            });
            OK(r);
        }
        SqliteWriter reopened;
        QCOMPARE(reopened.open(path_), StorageError::none);
        auto r = run(reopened, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            require(store.find(cfg.key) == cfg && store.find(ctx.key) == ctx,
                    "exact reopened bytes");
            require(!store.find(config("missing")), "explicit missing");
            session.commit();
            return 2;
        });
        OK(r);
        QCOMPARE(fixture_->rows("SELECT typeof(document),hex(document) FROM "
                                "main.subathon_retained_configurations"),
                 SqlRows({{"blob", "00FF4100"}}));

        // These are distinct codec paths: context identity and present link must
        // preserve nontext/NUL/BOM bytes just as configuration identity does.
        const auto binary_cfg = document(config(opaque(QByteArray::fromHex("efbbbf6100ff"))),
                                         QByteArray::fromHex("ff00"));
        const auto binary_ctx =
            document(context(opaque(QByteArray::fromHex("00ffefbbbf62"))),
                     QByteArray::fromHex("00ff"), ConfigurationReference{binary_cfg.key.reference});
        auto binary = run(reopened, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            require(store.retain(binary_cfg) == RetainDisposition::inserted, "binary link target");
            require(store.retain(binary_ctx) == RetainDisposition::inserted,
                    "binary context key/link");
            require(store.find(binary_ctx.key) == binary_ctx, "exact binary context DTO");
            session.commit();
            return 2;
        });
        OK(binary);
        QCOMPARE(
            fixture_->rows("SELECT typeof(reference),hex(reference),typeof(document),"
                           "hex(document),typeof(configuration_ref),hex(configuration_ref) "
                           "FROM main.subathon_retained_contexts WHERE reference=X'00FFEFBBBF62'"),
            SqlRows({{"blob", "00FFEFBBBF62", "blob", "00FF", "blob", "EFBBBF6100FF"}}));
        bool stopped = false;
        QCOMPARE(reopened.shutdown([&] { stopped = true; }), StorageError::none);
        QVERIFY(QTest::qWaitFor([&] { return stopped; }));
        SqliteWriter binary_reopened;
        QCOMPARE(binary_reopened.open(path_), StorageError::none);
        auto again = run(binary_reopened, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            require(store.find(binary_cfg.key) == binary_cfg &&
                        store.find(binary_ctx.key) == binary_ctx,
                    "binary context key and link survive reopen");
            session.commit();
            return 2;
        });
        OK(again);
    }

    void presentEmptyRoundTripsAndRetainsAsDuplicate() {
        QVERIFY(seed_config("empty", QByteArray("", 0)));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto r = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            const auto found = store.find(config("empty"));
            require(found.has_value(), "empty is present");
            require(found->document.isEmpty() && !found->document.isNull(),
                    "normalize present empty");
            require(!found->configuration, "configuration has no invented link");
            require(store.retain(*found) == RetainDisposition::already_present, "empty duplicate");
            require(!store.find(config("absent")), "missing differs from empty");
            session.commit();
            return 1;
        });
        OK(r);
        QCOMPARE(fixture_->rows("SELECT typeof(document),length(document) FROM "
                                "main.subathon_retained_configurations"),
                 SqlRows({{"blob", 0}}));
        QCOMPARE(count_config(), 1);
    }

    void opaqueReferencesAndFullRangeBytesStayDistinct() {
        SqliteWriter writer;
        StorageLimits limits;
        limits.statements_per_batch = 512; // Many vectors; no prescribed internal query plan.
        QCOMPARE(writer.open(path_, limits), StorageError::none);
        const auto entries = vectors_["opaque_vectors"].toArray();
        QCOMPARE(entries.size(), 12);
        auto r = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            for (const auto &entry : entries) {
                const auto e = entry.toObject();
                const auto ref = QByteArray::fromHex(e["reference_hex"].toString().toLatin1());
                const auto value = QByteArray::fromHex(e["document_hex"].toString().toLatin1());
                const auto item = document(config(opaque(ref)), value);
                require(store.retain(item) == RetainDisposition::inserted,
                        "opaque identities distinct");
                require(store.find(item.key) == item, "opaque bytes unchanged");
            }
            session.commit();
            return 12;
        });
        OK(r);
        QCOMPARE(count_config(), 12);
        for (const auto &entry : entries) {
            const auto e = entry.toObject();
            const auto refhex = e["reference_hex"].toString().toUpper();
            QCOMPARE(fixture_->rows("SELECT typeof(reference),hex(reference),typeof(document),"
                                    "hex(document) FROM main.subathon_retained_configurations "
                                    "WHERE reference=X'" +
                                    refhex + "'"),
                     SqlRows({{"blob", refhex, "blob", e["document_hex"].toString().toUpper()}}));
        }
    }

    void sameBytesRemainSeparateAcrossNamespaces() {
        const auto cfg = document(config("same"), "configuration");
        const auto ctx = document(context("same"), "context", ConfigurationReference{"same"});
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto r = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            require(store.retain(cfg) == RetainDisposition::inserted, "config identity");
            require(store.retain(ctx) == RetainDisposition::inserted, "context identity");
            require(store.find(cfg.key) == cfg && store.find(ctx.key) == ctx, "kind lookup");
            session.commit();
            return 2;
        });
        OK(r);
        QCOMPARE(count_config(), 1);
        QCOMPARE(count_context(), 1);
    }

    void exactDuplicateAndConflictDoNotMutate() {
        QVERIFY(seed_config("stable", "original"));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto r = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            require(store.retain(document(config("stable"), "original")) ==
                        RetainDisposition::already_present,
                    "exact duplicate");
            require(store.retain(document(config("stable"), "changed")) ==
                        RetainDisposition::conflict,
                    "immutable payload conflict");
            require(must_find(store, config("stable")).document == "original", "old bytes survive");
            session.commit();
            return int(RetainDisposition::conflict);
        });
        OK(r);
        QCOMPARE(fixture_->snapshot(), before);
    }

    void optionalConfigurationLinkPresenceIsImmutable() {
        QVERIFY(seed_config("cfg-a", "a"));
        QVERIFY(seed_config("cfg-b", "b"));
        QVERIFY(seed_context("unknown", "u"));
        QVERIFY(seed_context("known", "k", QByteArray("cfg-a")));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto r = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            const auto absent = document(context("unknown"), "u");
            const auto known = document(context("known"), "k", ConfigurationReference{"cfg-a"});
            require(store.find(absent.key) == absent && store.find(known.key) == known,
                    "link presence");
            require(store.retain(absent) == RetainDisposition::already_present, "absent duplicate");
            require(store.retain(known) == RetainDisposition::already_present, "known duplicate");
            require(store.retain(document(absent.key, "u", ConfigurationReference{"cfg-a"})) ==
                        RetainDisposition::conflict,
                    "cannot fill unknown");
            require(store.retain(document(known.key, "k")) == RetainDisposition::conflict,
                    "cannot erase known");
            require(store.retain(document(known.key, "k", ConfigurationReference{"cfg-b"})) ==
                        RetainDisposition::conflict,
                    "cannot retarget");
            require(
                store.retain(document(known.key, "changed", ConfigurationReference{"missing"})) ==
                    RetainDisposition::conflict,
                "existing conflict before new target lookup");
            session.commit();
            return 1;
        });
        OK(r);
        QCOMPARE(fixture_->snapshot(), before);
        QCOMPARE(fixture_->rows("SELECT typeof(configuration_ref),hex(configuration_ref) FROM "
                                "main.subathon_retained_contexts ORDER BY reference"),
                 SqlRows({{"blob", "6366672D61"}, {"null", ""}}));
        // A new writer must still return the absent link without a placeholder.
        bool stopped = false;
        QCOMPARE(writer.shutdown([&] { stopped = true; }), StorageError::none);
        QVERIFY(QTest::qWaitFor([&] { return stopped; }));
        SqliteWriter reopened;
        QCOMPARE(reopened.open(path_), StorageError::none);
        auto again = run(reopened, [&](auto session) -> QVariant {
            require(
                !must_find(SqliteRetainedContextStore(session), context("unknown")).configuration,
                "absence survives reopen");
            session.commit();
            return 1;
        });
        OK(again);
    }

    void missingAndWrongNamespaceLinksCannotCreateContext() {
        QVERIFY(seed_context("context-only", "x"));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto r = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            require(store.retain(
                        document(context("missing-target"), "m", ConfigurationReference{"404"})) ==
                        RetainDisposition::missing_reference,
                    "missing known target");
            require(store.retain(document(context("wrong-kind"), "w",
                                          ConfigurationReference{"context-only"})) ==
                        RetainDisposition::missing_reference,
                    "context cannot satisfy config link");
            require(store.retain(document(config("healthy"), "h")) == RetainDisposition::inserted,
                    "missing result leaves session healthy");
            require(!store.find(context("missing-target")) && !store.find(context("wrong-kind")),
                    "no orphan insertion");
            session.commit();
            return 1;
        });
        OK(r);
        QCOMPARE(count_config(), 1);
        QCOMPARE(count_context(), 1);
    }

    void newVersionsPreserveHistoricalReferences() {
        QVERIFY(seed_config("config-v1", "old-config"));
        QVERIFY(seed_context("ctx-v1", "old-context", QByteArray("config-v1")));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto r = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            require(store.retain(document(config("config-v2"), "new-config")) ==
                        RetainDisposition::inserted,
                    "new config version");
            require(store.retain(document(context("ctx-v2"), "new-context",
                                          ConfigurationReference{"config-v2"})) ==
                        RetainDisposition::inserted,
                    "new context version");
            require(store.find(context("ctx-v1")) == document(context("ctx-v1"), "old-context",
                                                              ConfigurationReference{"config-v1"}),
                    "history still names old config");
            require(must_find(store, config("config-v1")).document == "old-config",
                    "old version unchanged");
            session.commit();
            return 4;
        });
        OK(r);
        QCOMPARE(count_config(), 2);
        QCOMPARE(count_context(), 2);
    }

    void sameSessionStagesConfigurationAndContext() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto r = run(writer, [&](auto session) -> QVariant {
            {
                SqliteRetainedContextStore store(session);
                require(store.retain(document(config("staged"), "s")) ==
                            RetainDisposition::inserted,
                        "stage config");
                require(store.retain(
                            document(context("linked"), "l", ConfigurationReference{"staged"})) ==
                            RetainDisposition::inserted,
                        "resolve uncommitted same-session target");
                require(store.find(context("linked")).has_value(), "owner sees staged context");
            }
            require(count_config() == 0 && count_context() == 0,
                    "facade destruction did not commit");
            require(
                session.execute({"SELECT count(*) FROM main.subathon_retained_contexts", {}})[0][0]
                        .toInt() == 1,
                "facade destruction did not rollback");
            session.commit();
            require(count_config() == 1 && count_context() == 1, "physical commit before return");
            return 2;
        });
        OK(r);
    }

    void callerOwnsCommitRollbackAndConflictPolicy() {
        QVERIFY(seed_config("existing", "old"));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto aborted = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            require(store.retain(document(config("aborted"), "a")) == RetainDisposition::inserted,
                    "stage before conflict");
            require(store.retain(document(config("existing"), "changed")) ==
                        RetainDisposition::conflict,
                    "conflict disposition");
            require(session.execute({"SELECT count(*) FROM main.subathon_retained_configurations",
                                     {}})[0][0]
                            .toInt() == 2,
                    "conflict must not undo earlier work");
            session.rollback();
            return 0;
        });
        QVERIFY(aborted.admitted && aborted.done);
        QVERIFY2(!aborted.local && aborted.unexpected.isEmpty(), qPrintable(aborted.problem()));
        QCOMPARE(aborted.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(aborted.result.error, WorkError::not_committed);
        QVERIFY(!aborted.result.value);
        QCOMPARE(count_config(), 1);
        auto diagnostic = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            require(store.retain(document(config("kept"), "k")) == RetainDisposition::inserted,
                    "new write");
            const auto refusal = store.retain(document(config("existing"), "changed"));
            require(refusal == RetainDisposition::conflict, "caller chooses diagnostic commit");
            session.commit();
            return int(refusal);
        });
        OK(diagnostic);
        QCOMPARE(count_config(), 2);
        auto thrown = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session);
            require(store.retain(document(config("thrown"), "t")) == RetainDisposition::inserted,
                    "stage");
            require(store.retain(document(context("thrown-context"), "tc",
                                          ConfigurationReference{"thrown"})) ==
                        RetainDisposition::inserted,
                    "stage linked context in same abandoned unit");
            throw std::runtime_error("intentional caller abandonment");
        });
        QCOMPARE(thrown.unexpected, QString("intentional caller abandonment"));
        QCOMPARE(thrown.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(thrown.result.error, WorkError::work_exception);
        QVERIFY(!thrown.result.value);
        QCOMPARE(count_config(), 2);
        QCOMPARE(count_context(), 0);
    }

    void invalidInputPrecedencePreservesHealthySession() {
        // Default ticket proves pure checks precede any SQL/capability access.
        for (const auto limits : {RetainedLimits{0, 0}, RetainedLimits{0, 4}, RetainedLimits{4, 0},
                                  RetainedLimits{-1, 4}, RetainedLimits{4, -1}}) {
            SqliteRetainedContextStore stale({}, limits);
            try {
                expect_local([&] { (void)stale.find({static_cast<RetainedKind>(99), ""}); },
                             RetentionError::invalid_options);
            } catch (const std::exception &failure) {
                QFAIL(failure.what());
            }
        }
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto r = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore store(session, {4, 4});
            require(store.retain(document(config("keep"), "keep")) == RetainDisposition::inserted,
                    "write before validation");
            expect_local([&] { (void)store.find({static_cast<RetainedKind>(99), ""}); },
                         RetentionError::invalid_kind);
            expect_local([&] { (void)store.retain(document(config(""), QByteArray{})); },
                         RetentionError::invalid_reference);
            expect_local([&] { (void)store.find(config("12345")); },
                         RetentionError::invalid_reference);
            expect_local(
                [&] {
                    (void)store.retain(
                        document(config("ok"), QByteArray{}, ConfigurationReference{""}));
                },
                RetentionError::invalid_document);
            expect_local([&] { (void)store.retain(document(config("ok"), "12345")); },
                         RetentionError::invalid_document);
            expect_local(
                [&] {
                    (void)store.retain(document(config("ok"), "v", ConfigurationReference{"c"}));
                },
                RetentionError::invalid_link);
            expect_local(
                [&] {
                    (void)store.retain(document(context("ok"), "v", ConfigurationReference{""}));
                },
                RetentionError::invalid_link);
            expect_local(
                [&] {
                    (void)store.retain(
                        document(context("ok"), "v", ConfigurationReference{"12345"}));
                },
                RetentionError::invalid_link);
            require(must_find(store, config("keep")).document == "keep",
                    "validation preserves prior write");
            session.commit();
            return 1;
        });
        OK(r);
        QCOMPARE(count_config(), 1);
    }

    void configuredBoundsAndSelectedLimitsAreExact() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        const std::string full_reference(4096, 'r');
        const QByteArray full_document(262144, 'd');
        auto boundary = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore defaults(session);
            require(defaults.retain(document(config(full_reference), full_document)) ==
                        RetainDisposition::inserted,
                    "default exact raw-byte boundary");
            SqliteRetainedContextStore tiny(session, {4, 3});
            require(tiny.retain(document(config("1234"), "abc")) == RetainDisposition::inserted,
                    "configured exact boundary");
            expect_local([&] { (void)tiny.find(config("12345")); },
                         RetentionError::invalid_reference);
            expect_local([&] { (void)tiny.retain(document(config("ok"), "abcd")); },
                         RetentionError::invalid_document);
            session.commit();
            return 2;
        });
        OK(boundary);
        auto selected = run(writer, [&](auto session) -> QVariant {
            SqliteRetainedContextStore tiny(session, {4096, 3});
            try {
                (void)tiny.find(config(full_reference));
            } catch (const RetentionFailure &failure) {
                require(failure.error() == RetentionError::result_limit,
                        "valid large row is result_limit");
                require(fixture_->released(), "selected limit synchronously releases write lock");
                throw;
            }
            throw std::runtime_error("selected limit missing");
        });
        QCOMPARE(selected.local, std::optional<RetentionError>(RetentionError::result_limit));
        QCOMPARE(selected.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(selected.result.error, WorkError::work_exception);
        QVERIFY(!selected.result.value);
        QCOMPARE(count_config(), 2);
        // Literal result cap: 1048577-byte SQL result after a proved prior retain.
        // No guessed internal query-count or calibrated overhead expectation.
        bool budget_retain_completed = false;
        auto exhausted = run(writer, [&](auto session) -> QVariant {
            require(SqliteRetainedContextStore(session).retain(
                        document(config("rollback-budget"), "b")) == RetainDisposition::inserted,
                    "prior retain checkpoint reached");
            budget_retain_completed = true;
            (void)session.execute({"SELECT zeroblob(1048577)", {}});
            throw std::runtime_error("literal over-budget result accepted");
        });
        QVERIFY2(budget_retain_completed, qPrintable(exhausted.problem()));
        QVERIFY2(!exhausted.local, qPrintable(exhausted.problem()));
        QCOMPARE(exhausted.synchronous, std::optional<WorkError>(WorkError::result_limit));
        QCOMPARE(exhausted.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(exhausted.result.error, WorkError::result_limit);
        QVERIFY(!exhausted.result.value);
        QCOMPARE(count_config(), 2);
    }

    void selectedInvalidRowsAndDanglingLinksAbort() {
        QVERIFY(seed_config("healthy", "h"));
        QVERIFY(seed_config("bad", "b"));
        QVERIFY(seed_context("dangling", "d", QByteArray("absent")));
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        QVERIFY(fixture_->exec("UPDATE main.subathon_retained_configurations SET document='text' "
                               "WHERE reference=X'626164'"));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto healthy = run(writer, [&](auto session) -> QVariant {
            require(must_find(SqliteRetainedContextStore(session), config("healthy")).document ==
                        "h",
                    "unrelated corrupt rows are not an unbounded audit");
            session.commit();
            return 1;
        });
        OK(healthy);
        for (const auto &key : {config("bad"), context("dangling")}) {
            auto failed = run(writer, [&](auto session) -> QVariant {
                try {
                    // Conflict cannot mask selected corruption/dangling stored link.
                    (void)SqliteRetainedContextStore(session).retain(document(key, "different"));
                } catch (const RetentionFailure &failure) {
                    require(failure.error() == RetentionError::invalid_stored_data,
                            "selected corrupt category");
                    require(fixture_->released(), "known corruption rollback before throw");
                    throw;
                }
                throw std::runtime_error("corrupt selected row accepted");
            });
            QCOMPARE(failed.local,
                     std::optional<RetentionError>(RetentionError::invalid_stored_data));
            QCOMPARE(failed.result.transaction, TransactionOutcome::rolled_back);
            QCOMPARE(failed.result.error, WorkError::work_exception);
            QVERIFY(!failed.result.value);
            QCOMPARE(fixture_->snapshot(), before);
        }
        // Stored known target is validated too, even if proposed link is absent.
        QVERIFY(seed_context("bad-target", "c", QByteArray("bad")));
        auto bad_target = run(writer, [&](auto session) -> QVariant {
            (void)SqliteRetainedContextStore(session).find(context("bad-target"));
            session.commit();
            return 1;
        });
        QCOMPARE(bad_target.local,
                 std::optional<RetentionError>(RetentionError::invalid_stored_data));
        QCOMPARE(bad_target.result.transaction, TransactionOutcome::rolled_back);
        QVERIFY(!bad_target.result.value);
    }

    void ownedResultsAndCapabilityFailuresRemainSafe() {
        const auto refbytes = QByteArray::fromHex("efbbbf6100ff");
        const auto payload = QByteArray::fromHex("00ffefbbbf61");
        QVERIFY(seed_config(refbytes, payload));
        std::optional<RetainedDocument> owned;
        SqliteWorkSession expired;
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            auto r = run(writer, [&](auto session) -> QVariant {
                expired = session;
                SqliteRetainedContextStore store(session);
                owned = store.find(config(opaque(refbytes)));
                std::optional<WorkError> foreign;
                std::optional<RetentionError> invalid;
                QString thread_failure;
                std::thread worker([&] {
                    try {
                        (void)store.find(config("valid"));
                    } catch (const SqliteWorkFailure &failure) {
                        foreign = failure.error();
                    } catch (const std::exception &failure) {
                        thread_failure = QString::fromUtf8(failure.what());
                    }
                    try {
                        (void)store.find(config(""));
                    } catch (const RetentionFailure &failure) {
                        invalid = failure.error();
                    } catch (const std::exception &failure) {
                        thread_failure = QString::fromUtf8(failure.what());
                    }
                });
                worker.join();
                require(thread_failure.isEmpty(), "unexpected foreign-thread exception category");
                require(foreign == WorkError::wrong_thread,
                        "foreign capability delegates unchanged");
                require(invalid == RetentionError::invalid_reference,
                        "invalid input precedes foreign ticket");
                session.commit();
                expect_work([&] { (void)store.find(config("valid")); }, WorkError::session_closed);
                return 1;
            });
            QVERIFY(r.admitted && r.done);
            QVERIFY2(!r.local && r.unexpected.isEmpty(), qPrintable(r.problem()));
            // The inherited terminal protocol error remains sticky after commit.
            QCOMPARE(r.result.transaction, TransactionOutcome::committed);
            QCOMPARE(r.result.error, WorkError::session_closed);
            QVERIFY(!r.result.value);
        }
        QVERIFY(owned.has_value());
        QCOMPARE(owned->key.reference, opaque(refbytes));
        QCOMPARE(owned->document, payload);
        SqliteRetainedContextStore stale(expired);
        try {
            expect_work([&] { (void)stale.find(config("valid")); }, WorkError::stale_session);
            expect_local([&] { (void)stale.find(config("")); }, RetentionError::invalid_reference);
        } catch (const std::exception &failure) {
            QFAIL(failure.what());
        }
    }

    void sqlFailureAndUnknownOutcomeAreNotSuccess() {
        // Known SQL failure after an actual retain must roll back the full unit.
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            bool sql_retain_completed = false;
            auto failed = run(writer, [&](auto session) -> QVariant {
                require(SqliteRetainedContextStore(session).retain(
                            document(config("sql-abort"), "s")) == RetainDisposition::inserted,
                        "prior write checkpoint");
                sql_retain_completed = true;
                (void)session.execute({"INSERT INTO main.no_such_retention_table VALUES (1)", {}});
                return 1;
            });
            QVERIFY2(sql_retain_completed, qPrintable(failed.problem()));
            QVERIFY2(!failed.local, qPrintable(failed.problem()));
            QCOMPARE(failed.synchronous, std::optional<WorkError>(WorkError::sql_failure));
            QCOMPARE(failed.result.transaction, TransactionOutcome::rolled_back);
            QCOMPARE(failed.result.error, WorkError::sql_failure);
            QVERIFY(!failed.result.value);
            QCOMPARE(count_config(), 0);
        }
        // Fault is consumed only AFTER the existing writer's real COMMIT succeeds.
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                         writer, WorkControlFault::lose_next_commit_ack),
                     StorageError::none);
            bool commit_retain_completed = false;
            auto unknown = run(writer, [&](auto session) -> QVariant {
                require(SqliteRetainedContextStore(session).retain(
                            document(config("commit-unknown"), "c")) == RetainDisposition::inserted,
                        "prior actual retain");
                commit_retain_completed = true;
                session.commit();
                return 1;
            });
            QVERIFY2(commit_retain_completed, qPrintable(unknown.problem()));
            QVERIFY2(!unknown.local, qPrintable(unknown.problem()));
            QCOMPARE(unknown.synchronous, std::optional<WorkError>(WorkError::uncertain_outcome));
            QCOMPARE(unknown.result.transaction, TransactionOutcome::outcome_unknown);
            QCOMPARE(unknown.result.error, WorkError::uncertain_outcome);
            QVERIFY(!unknown.result.value);
            QCOMPARE(count_config(), 1); // Physical commit, no normal reported success.
            QCOMPARE(writer.enqueue_work([](auto) { return QVariant(1); }, [](auto) {}),
                     StorageError::shutting_down);
        }
        QVERIFY(seed_config("corrupt", "x"));
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        QVERIFY(fixture_->exec("UPDATE main.subathon_retained_configurations SET document=5 "
                               "WHERE reference=X'636F7272757074'"));
        const auto before = fixture_->snapshot();
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                         writer, WorkControlFault::lose_next_rollback_ack),
                     StorageError::none);
            bool rollback_retain_completed = false;
            auto unknown = run(writer, [&](auto session) -> QVariant {
                require(SqliteRetainedContextStore(session).retain(
                            document(config("rolled-back"), "r")) == RetainDisposition::inserted,
                        "stage before corrupt read");
                rollback_retain_completed = true;
                (void)SqliteRetainedContextStore(session).find(config("corrupt"));
                return 1;
            });
            QVERIFY2(rollback_retain_completed, qPrintable(unknown.problem()));
            QVERIFY2(!unknown.local, qPrintable(unknown.problem()));
            QCOMPARE(unknown.synchronous, std::optional<WorkError>(WorkError::uncertain_outcome));
            QCOMPARE(unknown.result.transaction, TransactionOutcome::outcome_unknown);
            QCOMPARE(unknown.result.error, WorkError::uncertain_outcome);
            QVERIFY(!unknown.result.value);
            QCOMPARE(fixture_->snapshot(), before); // Real rollback happened; ack was lost.
        }
    }
};
QTEST_GUILESS_MAIN(SqliteRetainedContextTests)
#include "sqlite_retained_context_tests.moc"
