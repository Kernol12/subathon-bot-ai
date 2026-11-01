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
#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <subathon/server/sqlite_receipts.hpp>
#include <subathon/server/sqlite_retained_context.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <thread>

using namespace subathon::server;
using namespace subathon::core;
using subathon::server::testing::SqliteWriterWorkAccess;
using subathon::server::testing::WorkControlFault;
namespace {
QByteArray blob(const std::string &s) {
    return s.empty() ? QByteArray("", 0) : QByteArray(s.data(), qsizetype(s.size()));
}
std::string opaque(const QByteArray &s) { return std::string(s.constData(), size_t(s.size())); }
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class Operation> void expect_input(Operation operation, SqliteReceiptInputError error) {
    try {
        operation();
    } catch (const SqliteReceiptInputFailure &failure) {
        require(failure.error() == error, "wrong SqliteReceiptInputFailure category");
        return;
    }
    throw std::runtime_error("expected SqliteReceiptInputFailure missing");
}
template <class Operation> void expect_work(Operation operation, WorkError error) {
    try {
        operation();
    } catch (const SqliteWorkFailure &failure) {
        require(failure.error() == error, "wrong SqliteWorkFailure category");
        return;
    }
    throw std::runtime_error("expected SqliteWorkFailure missing");
}
ReceiptInput input(std::string id = "event-42") {
    return {{{"synthetic.provider-a", "source-account-7"}, "synthetic.event.v1", std::move(id)},
            {"synthetic.payment.received", 1, 1700000000000, "actor-3", "Synthetic Supporter",
             ReceiptMoney{"12.3400", "EUR"}, ReceiptLinkReference{"payment.v1", "payment-9"},
             ReceiptLinkReference{"activity.v1", "activity-2"}},
            {1700000000100, "connection-1", "cfg1", "ctx1"}};
}
ReceiptAttempt attempt(const ReceiptInput &value, ReceiptDisposition disposition) {
    return {value.arrival, value.facts, disposition};
}
const QStringList facts_columns = {
    "event_type",         "schema_version",         "source_timestamp_utc_ms", "actor_id",
    "actor_display_name", "money_original_decimal", "money_currency",          "payment_namespace",
    "payment_opaque_id",  "activity_namespace",     "activity_opaque_id"};
const QStringList arrival_columns = {"server_timestamp_utc_ms", "connection_ref",
                                     "configuration_ref", "modifier_context_ref"};
QList<QVariant> facts_values(const ReceiptFacts &v) {
    const auto optional_blob = [](const std::optional<std::string> &s) {
        return s ? QVariant(blob(*s)) : QVariant();
    };
    return {blob(v.event_type),
            qint64(v.schema_version),
            v.source_timestamp_utc_ms ? QVariant(qint64(*v.source_timestamp_utc_ms)) : QVariant(),
            optional_blob(v.actor_id),
            optional_blob(v.actor_display_name),
            v.money ? QVariant(blob(v.money->original_decimal)) : QVariant(),
            v.money ? QVariant(blob(v.money->currency)) : QVariant(),
            v.payment_reference ? QVariant(blob(v.payment_reference->identifier_namespace))
                                : QVariant(),
            v.payment_reference ? QVariant(blob(v.payment_reference->opaque_id)) : QVariant(),
            v.activity_reference ? QVariant(blob(v.activity_reference->identifier_namespace))
                                 : QVariant(),
            v.activity_reference ? QVariant(blob(v.activity_reference->opaque_id)) : QVariant()};
}
QList<QVariant> arrival_values(const ReceiptArrival &v) {
    return {qint64(v.server_timestamp_utc_ms), blob(v.connection_ref), blob(v.configuration_ref),
            blob(v.modifier_context_ref)};
}
QString quoted(QString name) {
    name.replace('"', "\"\"");
    return '"' + name + '"';
}

// Negative/raw fixtures are independent SQL, never new codec/factory/repository
// output. OFF/ignore-checks applies only to this fixture, not the product writer.
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
    bool open() { return db_.open() && exec("PRAGMA foreign_keys=OFF"); }
    bool exec(const QString &sql, const QList<QVariant> &values = {}) {
        QSqlQuery q(db_);
        if (!q.prepare(sql))
            return false;
        for (const auto &v : values)
            q.addBindValue(v);
        return q.exec();
    }
    SqlRows rows(const QString &sql) {
        QSqlQuery q(db_);
        if (!q.exec(sql)) {
            QTest::qFail(qPrintable(q.lastError().text()), __FILE__, __LINE__);
            return {};
        }
        SqlRows result;
        while (q.next()) {
            SqlRow row;
            for (int i = 0; i < q.record().count(); ++i)
                row.append(q.value(i));
            result.append(row);
        }
        if (q.lastError().isValid())
            QTest::qFail("fixture fetch failed", __FILE__, __LINE__);
        return result;
    }
    QVariant scalar(const QString &sql) {
        const auto r = rows(sql);
        if (r.size() != 1 || r[0].size() != 1) {
            QTest::qFail("fixture scalar shape", __FILE__, __LINE__);
            return {};
        }
        return r[0][0];
    }
    bool released() { return exec("BEGIN IMMEDIATE") && exec("ROLLBACK"); }
    QList<SqlRows> snapshot() {
        QList<SqlRows> result;
        const auto schema =
            rows("SELECT type,name,tbl_name,sql FROM main.sqlite_schema ORDER BY type,name");
        result.append(schema);
        for (const auto &object : schema) {
            if (object[0].toString() != "table")
                continue;
            const auto table = object[1].toString();
            QStringList projection;
            for (const auto &column : rows("PRAGMA main.table_info(" + quoted(table) + ")")) {
                const auto c = quoted(column[1].toString());
                projection.append("typeof(" + c + ")");
                projection.append("hex(CAST(" + c + " AS BLOB))");
            }
            result.append(rows("SELECT " + projection.join(',') + " FROM main." + quoted(table) +
                               " ORDER BY rowid"));
        }
        return result;
    }
    bool original(const QByteArray &id, const ReceiptInput &v) {
        QStringList columns = {"receipt_id", "provider", "source_account", "identifier_namespace",
                               "opaque_id"};
        columns.append(facts_columns);
        columns.append(arrival_columns);
        QList<QVariant> values = {
            id, blob(v.identity.origin.provider), blob(v.identity.origin.source_account),
            blob(v.identity.identifier_namespace), blob(v.identity.opaque_id)};
        values.append(facts_values(v.facts));
        values.append(arrival_values(v.arrival));
        return insert("subathon_receipts", columns, values);
    }
    bool observation(const QByteArray &id, qint64 ordinal, const ReceiptAttempt &v) {
        QStringList columns = {"receipt_id", "ordinal"};
        columns.append(facts_columns);
        columns.append(arrival_columns);
        columns.append("disposition");
        QList<QVariant> values = {id, ordinal};
        values.append(facts_values(v.observed_facts));
        values.append(arrival_values(v.arrival));
        int disposition = v.disposition == ReceiptDisposition::created ? 0
                          : v.disposition == ReceiptDisposition::retry ? 1
                                                                       : 2;
        values.append(disposition);
        return insert("subathon_receipt_attempts", columns, values);
    }
    bool seed(const QByteArray &id, const ReceiptInput &v) {
        return original(id, v) && observation(id, 1, attempt(v, ReceiptDisposition::created)) &&
               exec("UPDATE main.subathon_receipt_allocator SET last_allocated=? WHERE singleton=1",
                    {id});
    }

  private:
    bool insert(const QString &table, const QStringList &columns, const QList<QVariant> &values) {
        QStringList marks;
        for (qsizetype i = 0; i < values.size(); ++i)
            marks.append("?");
        return exec("INSERT INTO main." + table + "(" + columns.join(',') + ") VALUES (" +
                        marks.join(',') + ")",
                    values);
    }
    QString name_;
    QSqlDatabase db_;
};
MigrationStep literal_step(const QJsonObject &o) {
    MigrationStep s;
    s.version = o["version"].toInt();
    s.id = o["id"].toString();
    for (const auto &sql : o["sql"].toArray())
        s.sql.append(sql.toString());
    for (const auto &entry : o["expected_schema"].toArray()) {
        const auto x = entry.toObject();
        s.expected_schema.append(
            {x["type"].toString(), x["name"].toString(), x["table_name"].toString(),
             x["sql"].isNull() ? std::nullopt : std::optional<QString>(x["sql"].toString())});
    }
    return s;
}
struct Run {
    bool admitted{false}, done{false}, stub{false};
    std::optional<SqliteReceiptInputError> invalid;
    std::optional<SqliteReceiptStorageReason> storage;
    std::optional<WorkError> synchronous;
    QString unexpected;
    SqliteWorkResult result;
    QString problem() const {
        if (stub)
            return "SqliteReceiptsNotImplemented: explicit missing receipt behavior";
        if (!unexpected.isEmpty())
            return unexpected;
        return QString("input=%1 storage=%2 synchronous=%3 outer=%4")
            .arg(invalid ? int(*invalid) : -1)
            .arg(storage ? int(*storage) : -1)
            .arg(synchronous ? int(*synchronous) : -1)
            .arg(int(result.error));
    }
};
Run run(SqliteWriter &writer, std::function<QVariant(SqliteWorkContext)> body) {
    auto state = std::make_shared<Run>();
    const auto admission = writer.enqueue_work(
        [state, body = std::move(body)](auto work) {
            try {
                return body(work);
            } catch (const SqliteReceiptsNotImplemented &) {
                state->stub = true;
                throw;
            } catch (const SqliteReceiptInputFailure &e) {
                state->invalid = e.error();
                throw;
            } catch (const SqliteReceiptStorageFailure &e) {
                state->storage = e.reason();
                throw;
            } catch (const SqliteWorkFailure &e) {
                state->synchronous = e.error();
                throw;
            } catch (const std::exception &e) {
                state->unexpected = QString::fromUtf8(e.what());
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
ReceiptRecord found(ReceiptTransaction &tx, const ReceiptIdentity &identity) {
    auto value = tx.find(identity);
    require(value.has_value(), "expected receipt missing");
    return *value;
}
} // namespace
#define OK(r)                                                                                      \
    QVERIFY((r).admitted && (r).done);                                                             \
    QVERIFY2(!(r).stub && !(r).invalid && !(r).storage && (r).unexpected.isEmpty(),                \
             qPrintable((r).problem()));                                                           \
    QCOMPARE((r).result.transaction, TransactionOutcome::committed);                               \
    QCOMPARE((r).result.error, WorkError::none);                                                   \
    QVERIFY((r).result.value.has_value())
#define KNOWN(r, reason, outer)                                                                    \
    QVERIFY((r).admitted && (r).done);                                                             \
    QVERIFY2(!(r).stub, qPrintable((r).problem()));                                                \
    QCOMPARE((r).storage, std::optional<SqliteReceiptStorageReason>(reason));                      \
    QCOMPARE((r).result.transaction, TransactionOutcome::rolled_back);                             \
    QCOMPARE((r).result.error, outer);                                                             \
    QVERIFY(!(r).result.value)

// Fault decorator invokes real primitives, then real failing SQL. It contains no
// identity/facts/disposition policy and never fabricates rollback or SQL results.
class FailAfterStage final : public ReceiptRepository {
  public:
    enum class Point { create, append };
    FailAfterStage(ReceiptRepository &inner, SqliteWorkSession session, Point point, bool &created,
                   bool &appended)
        : inner_(inner), session_(std::move(session)), point_(point), created_(created),
          appended_(appended) {}
    std::unique_ptr<ReceiptTransaction> begin() override {
        class Transaction final : public ReceiptTransaction {
          public:
            Transaction(std::unique_ptr<ReceiptTransaction> inner, SqliteWorkSession session,
                        Point point, bool &created, bool &appended)
                : inner_(std::move(inner)), session_(std::move(session)), point_(point),
                  created_(created), appended_(appended) {}
            std::optional<ReceiptRecord> find(const ReceiptIdentity &i) override {
                return inner_->find(i);
            }
            ReceiptId create(const ReceiptIdentity &i, const ReceiptFacts &f,
                             const ReceiptArrival &a) override {
                const auto id = inner_->create(i, f, a);
                created_ = true; // Only after actual primitive returned successfully.
                require(session_.execute({"SELECT count(*) FROM main.subathon_receipts", {}})[0][0]
                                .toLongLong() > 0,
                        "actual original staging not visible to owner");
                if (point_ == Point::create)
                    fail();
                return id;
            }
            void append_attempt(ReceiptId id, const ReceiptAttempt &a) override {
                inner_->append_attempt(id, a);
                appended_ = true;
                require(session_.execute({"SELECT count(*) FROM main.subathon_receipt_attempts",
                                          {}})[0][0]
                                .toLongLong() > 0,
                        "actual attempt staging not visible to owner");
                if (point_ == Point::append)
                    fail();
            }
            void commit() override { inner_->commit(); }

          private:
            [[noreturn]] void fail() {
                try {
                    (void)session_.execute(
                        {"INSERT INTO main.no_such_receipt_fault_table VALUES (1)", {}});
                } catch (const SqliteWorkFailure &e) {
                    if (e.error() == WorkError::sql_failure)
                        throw SqliteReceiptStorageFailure(SqliteReceiptStorageReason::sql_failure);
                    throw;
                }
                throw std::runtime_error("fault SQL unexpectedly succeeded");
            }
            std::unique_ptr<ReceiptTransaction> inner_;
            SqliteWorkSession session_;
            Point point_;
            bool &created_;
            bool &appended_;
        };
        return std::make_unique<Transaction>(inner_.begin(), session_, point_, created_, appended_);
    }

  private:
    ReceiptRepository &inner_;
    SqliteWorkSession session_;
    Point point_;
    bool &created_;
    bool &appended_;
};

class SqliteReceiptTests final : public QObject {
    Q_OBJECT
  private:
    QJsonObject root_, m1_, m2_;
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<Fixture> fixture_;
    QString path_;
    MigrationManifest literals() const {
        MigrationManifest m;
        m.steps = {literal_step(m1_), literal_step(m2_)};
        m.supported_starting_versions = {0, 1, 2};
        return m;
    }
    bool install() {
        SqliteWriter writer;
        if (writer.open(path_) != StorageError::none)
            return false;
        MigrationRequest request;
        request.mode = MigrationMode::advance;
        request.manifest = literals();
        bool done = false;
        MigrationResult result;
        const auto admission =
            SqliteMigrationRunner::enqueue(writer, std::move(request), [&](auto r) {
                result = std::move(r);
                done = true;
            });
        return admission.error == StorageError::none &&
               QTest::qWaitFor([&] { return done; }, 5000) &&
               result.reason == MigrationReason::none &&
               result.work.transaction == TransactionOutcome::committed;
    }
    qint64 count(const QString &table) {
        return fixture_->scalar("SELECT count(*) FROM main." + table).toLongLong();
    }
    bool retained(const QByteArray &cfg, const QByteArray &ctx, QVariant link) {
        return fixture_->exec("INSERT INTO main.subathon_retained_configurations VALUES (?,X'63')",
                              {cfg}) &&
               fixture_->exec("INSERT INTO main.subathon_retained_contexts VALUES (?,X'74',?)",
                              {ctx, link});
    }
    Run ingest(SqliteWriter &writer, const ReceiptInput &value,
               std::optional<ReceiptIntakeResult> &result) {
        return run(writer, [&](auto work) -> QVariant {
            SqliteReceiptRepository repository(work);
            ReceiptIntake service(repository);
            result = service.ingest(value);
            return int(result->disposition);
        });
    }
    Run read(SqliteWriter &writer, const ReceiptIdentity &identity,
             std::optional<ReceiptRecord> &record, SqliteReceiptLimits limits = {}) {
        return run(writer, [&](auto work) -> QVariant {
            SqliteReceiptRepository repository(work, limits);
            auto tx = repository.begin();
            record = tx->find(identity);
            tx->commit();
            return 1;
        });
    }
  private slots:
    void initTestCase() {
        QVERIFY(QSqlDatabase::isDriverAvailable("QSQLITE"));
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
        for (const auto &pair : {std::pair{QString(SUBATHON_RECEIPT_VECTOR_PATH), &root_},
                                 std::pair{QString(SUBATHON_RETAINED_VECTOR_PATH), &m1_}}) {
            QFile file(pair.first);
            QVERIFY(file.open(QIODevice::ReadOnly));
            QJsonParseError error;
            const auto json = QJsonDocument::fromJson(file.readAll(), &error);
            QCOMPARE(error.error, QJsonParseError::NoError);
            *pair.second = json.object();
        }
        m1_ = m1_["manifest"].toObject();
        m2_ = root_["manifest"].toObject();
        QCOMPARE(m1_["version"].toInt(), 1);
        QCOMPARE(m2_["version"].toInt(), 2);
        QCOMPARE(m2_["payload_bytes"].toInt(), 10535);
        QCOMPARE(m2_["sha256_hex"].toString(),
                 QString("24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2"));
    }
    void init() {
        directory_ = std::make_unique<QTemporaryDir>();
        QVERIFY(directory_->isValid());
        path_ = directory_->filePath("receipts.sqlite");
        QVERIFY(install());
        fixture_ = std::make_unique<Fixture>(path_);
        QVERIFY(fixture_->open());
        QVERIFY(retained("cfg1", "ctx1", QByteArray("cfg1")));
        QVERIFY(retained("cfg2", "ctx2", QByteArray("cfg2")));
        QVERIFY(retained("cfg3", "ctx-unknown", QVariant()));
    }
    void cleanup() {
        fixture_.reset();
        directory_.reset();
    }

    void codecsMatchFullRangeLiteralVectors() {
        constexpr std::array<std::uint64_t, 6> values = {0,
                                                         1,
                                                         9007199254740993ULL,
                                                         9223372036854775807ULL,
                                                         9223372036854775808ULL,
                                                         18446744073709551615ULL};
        const auto vectors = root_["codec_vectors"].toArray();
        QCOMPARE(vectors.size(), 6);
        try {
            for (qsizetype i = 0; i < 6; ++i) {
                const auto v = vectors[i].toObject();
                QCOMPARE(v["decimal"].toString(), QString::number(values[size_t(i)]));
                const auto bytes = QByteArray::fromHex(v["hex"].toString().toLatin1());
                QCOMPARE(bytes.size(), 8);
                QCOMPARE(encode_receipt_uint64(values[size_t(i)]), bytes);
                QCOMPARE(decode_receipt_uint64(bytes), values[size_t(i)]);
            }
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void codecRejectsMalformedBuffers() {
        try {
            for (const auto &bytes :
                 {QByteArray(), QByteArray("", 0), QByteArray(7, '\0'), QByteArray(9, '\0')})
                expect_input([&] { (void)decode_receipt_uint64(bytes); },
                             SqliteReceiptInputError::invalid_codec);
            QCOMPARE(decode_receipt_uint64(QByteArray(8, '\0')), std::uint64_t(0));
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void manifestPreservesRetentionPrefixAndMatchesLiteralReceiptStep() {
        try {
            const auto actual = receipt_repository_manifest();
            const auto expected = literals();
            QCOMPARE(actual.catalog_format, 1);
            QCOMPARE(actual.store_identity, QString("subathon.sqlite-store"));
            QCOMPARE(actual.supported_starting_versions, QList<qint32>({0, 1, 2}));
            QCOMPARE(actual.steps.size(), 2);
            for (qsizetype i = 0; i < 2; ++i) {
                const auto &a = actual.steps[i], &e = expected.steps[i];
                QCOMPARE(a.version, e.version);
                QCOMPARE(a.id, e.id);
                QCOMPARE(a.sql, e.sql);
                QCOMPARE(a.expected_schema, e.expected_schema);
                const auto j = i == 0 ? m1_ : m2_;
                QCOMPARE(canonical_migration_payload(a),
                         QByteArray::fromHex(j["payload_hex"].toString().toLatin1()));
                QCOMPARE(migration_content_sha256(a),
                         QByteArray::fromHex(j["sha256_hex"].toString().toLatin1()));
            }
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void intakeCreatesRetriesConflictsAndPreservesOriginal() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        const auto original = input();
        auto value = original;
        std::optional<ReceiptIntakeResult> created, retry, conflict, later, repeated;
        auto a = ingest(writer, value, created);
        OK(a);
        QVERIFY(created);
        QCOMPARE(created->receipt_id.value, std::uint64_t(1));
        QCOMPARE(created->disposition, ReceiptDisposition::created);
        value.arrival = {1700000000100, "connection-reconnected", "cfg2", "ctx2"};
        auto b = ingest(writer, value, retry);
        OK(b);
        QVERIFY(retry);
        QCOMPARE(retry->receipt_id, created->receipt_id);
        QCOMPARE(retry->disposition, ReceiptDisposition::retry);
        value.facts.money->original_decimal = "12.34";
        auto c = ingest(writer, value, conflict);
        OK(c);
        QVERIFY(conflict);
        QCOMPARE(conflict->disposition, ReceiptDisposition::conflict);
        auto d = ingest(writer, original, later);
        OK(d);
        QVERIFY(later);
        QCOMPARE(later->disposition, ReceiptDisposition::retry);
        auto e = ingest(writer, value, repeated);
        OK(e);
        QVERIFY(repeated);
        QCOMPARE(repeated->disposition, ReceiptDisposition::conflict);
        std::optional<ReceiptRecord> record;
        auto readback = read(writer, original.identity, record);
        OK(readback);
        QVERIFY(record);
        QCOMPARE(record->id, created->receipt_id);
        QCOMPARE(record->original_facts, original.facts);
        QCOMPARE(record->original_arrival, original.arrival);
        QCOMPARE(record->attempts, std::vector<ReceiptAttempt>(
                                       {attempt(original, ReceiptDisposition::created),
                                        attempt({original.identity, original.facts, value.arrival},
                                                ReceiptDisposition::retry),
                                        attempt(value, ReceiptDisposition::conflict),
                                        attempt(original, ReceiptDisposition::retry),
                                        attempt(value, ReceiptDisposition::conflict)}));
        QCOMPARE(count("subathon_receipts"), 1);
        QCOMPARE(count("subathon_receipt_attempts"), 5);
        QCOMPARE(fixture_->rows("SELECT ordinal,disposition,hex(money_original_decimal) FROM "
                                "main.subathon_receipt_attempts ORDER BY ordinal"),
                 SqlRows({{1, 0, "31322E33343030"},
                          {2, 1, "31322E33343030"},
                          {3, 2, "31322E3334"},
                          {4, 1, "31322E33343030"},
                          {5, 2, "31322E3334"}}));
    }
    void allFactsParticipateInConflictWithUnknownsPreserved() {
        using Mutation = std::function<void(ReceiptFacts &)>;
        const QList<Mutation> changes = {
            [](auto &f) { f.event_type = "other"; },
            [](auto &f) { f.schema_version = 0; },
            [](auto &f) { f.source_timestamp_utc_ms.reset(); },
            [](auto &f) { f.actor_id = "other"; },
            [](auto &f) { f.actor_display_name = "other"; },
            [](auto &f) { f.money.reset(); },
            [](auto &f) { f.money->original_decimal = "12.34"; },
            [](auto &f) { f.money->currency = "USD"; },
            [](auto &f) { f.payment_reference->identifier_namespace = "other"; },
            [](auto &f) { f.payment_reference->opaque_id = "other"; },
            [](auto &f) { f.activity_reference->identifier_namespace = "other"; },
            [](auto &f) { f.activity_reference->opaque_id = "other"; },
            [](auto &f) { f.actor_id.reset(); },
            [](auto &f) { f.actor_display_name.reset(); },
            [](auto &f) { f.payment_reference.reset(); },
            [](auto &f) { f.activity_reference.reset(); }};
        SqliteWriter writer;
        StorageLimits limits;
        limits.statements_per_batch = 512;
        QCOMPARE(writer.open(path_, limits), StorageError::none);
        for (qsizetype i = 0; i < changes.size(); ++i) {
            const auto first = input("fact-" + std::to_string(i));
            auto changed = first;
            changes[i](changed.facts);
            std::optional<ReceiptIntakeResult> a, b;
            auto x = ingest(writer, first, a);
            OK(x);
            auto y = ingest(writer, changed, b);
            OK(y);
            QVERIFY(a && b);
            QCOMPARE(b->receipt_id, a->receipt_id);
            QCOMPARE(b->disposition, ReceiptDisposition::conflict);
            std::optional<ReceiptRecord> r;
            auto z = read(writer, first.identity, r);
            OK(z);
            QVERIFY(r);
            QCOMPARE(r->original_facts, first.facts);
            QCOMPARE(r->attempts.size(), size_t(2));
            QCOMPARE(r->attempts[1].observed_facts, changed.facts);
        }
        auto unknown = input("unknown");
        unknown.facts = {"activity", 1, {}, {}, {}, {}, {}, {}};
        std::optional<ReceiptIntakeResult> a, b;
        auto x = ingest(writer, unknown, a);
        OK(x);
        auto newly_known = unknown;
        newly_known.facts.actor_id = std::string();
        newly_known.facts.source_timestamp_utc_ms = 0;
        auto y = ingest(writer, newly_known, b);
        OK(y);
        QVERIFY(b);
        QCOMPARE(b->disposition, ReceiptDisposition::conflict);
        std::optional<ReceiptRecord> r;
        auto z = read(writer, unknown.identity, r);
        OK(z);
        QVERIFY(r);
        QCOMPARE(r->original_facts, unknown.facts);
        QCOMPARE(r->attempts[1].observed_facts, newly_known.facts);
    }
    void identityComponentsAndOpaqueBytesScopeReceipts() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        const auto first = input("opaque");
        QList<ReceiptInput> values = {first};
        auto p = first;
        p.identity.origin.provider = "other";
        values.append(p);
        p = first;
        p.identity.origin.source_account = "other";
        values.append(p);
        p = first;
        p.identity.identifier_namespace = "other";
        values.append(p);
        p = first;
        p.identity.opaque_id = "OPAQUE";
        values.append(p);
        const auto raw = opaque(QByteArray::fromHex("efbbbf6100ff"));
        for (int component = 0; component < 4; ++component) {
            p = first;
            if (component == 0)
                p.identity.origin.provider = raw;
            if (component == 1)
                p.identity.origin.source_account = raw;
            if (component == 2)
                p.identity.identifier_namespace = raw;
            if (component == 3)
                p.identity.opaque_id = raw;
            values.append(p);
        }
        std::vector<ReceiptId> ids;
        for (const auto &v : values) {
            std::optional<ReceiptIntakeResult> result;
            auto r = ingest(writer, v, result);
            OK(r);
            QVERIFY(result);
            QCOMPARE(result->disposition, ReceiptDisposition::created);
            for (const auto id : ids)
                QVERIFY(result->receipt_id != id);
            ids.push_back(result->receipt_id);
            std::optional<ReceiptRecord> record;
            auto q = read(writer, v.identity, record);
            OK(q);
            QVERIFY(record);
            QCOMPARE(record->identity, v.identity);
        }
        QCOMPARE(count("subathon_receipts"), 9);
        QCOMPARE(fixture_
                     ->scalar("SELECT count(*) FROM main.subathon_receipts WHERE "
                              "typeof(provider)='blob' AND typeof(source_account)='blob' AND "
                              "typeof(identifier_namespace)='blob' AND typeof(opaque_id)='blob'")
                     .toInt(),
                 9);
        for (const auto &column : {QString("provider"), QString("source_account"),
                                   QString("identifier_namespace"), QString("opaque_id")})
            QCOMPARE(fixture_
                         ->scalar("SELECT count(*) FROM main.subathon_receipts WHERE hex(" +
                                  column + ")='EFBBBF6100FF'")
                         .toInt(),
                     1);
    }

    void optionalFactsAndIntegerExtremaRoundTripAfterReopen() {
        QList<ReceiptInput> expected;
        auto high = input("extreme-high");
        high.facts.schema_version = std::numeric_limits<std::uint32_t>::max();
        high.facts.source_timestamp_utc_ms = std::numeric_limits<std::int64_t>::min();
        high.arrival.server_timestamp_utc_ms = std::numeric_limits<std::int64_t>::max();
        high.facts.actor_id = std::string();
        high.facts.actor_display_name = opaque(QByteArray::fromHex("efbbbf6100ff"));
        high.facts.money = ReceiptMoney{"9007199254740993.00000000000000000001", "EUR"};
        high.facts.payment_reference = ReceiptLinkReference{
            opaque(QByteArray::fromHex("00ff")), opaque(QByteArray::fromHex("efbbbf61"))};
        high.facts.activity_reference = ReceiptLinkReference{"", ""};
        expected.append(high);
        auto low = input("extreme-low");
        low.facts = {"", 0, 0, {}, {}, {}, {}, {}};
        low.arrival.server_timestamp_utc_ms = std::numeric_limits<std::int64_t>::min();
        low.arrival.connection_ref = "";
        expected.append(low);
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            for (auto value : expected) {
                std::optional<ReceiptIntakeResult> result;
                auto r = ingest(writer, value, result);
                OK(r);
                value.facts = {};
                value.identity.opaque_id = "mutated";
                value.arrival.configuration_ref = "mutated";
            }
        }
        SqliteWriter reopened;
        QCOMPARE(reopened.open(path_), StorageError::none);
        std::optional<ReceiptRecord> owned;
        for (const auto &value : expected) {
            auto r = read(reopened, value.identity, owned);
            OK(r);
            QVERIFY(owned);
            QCOMPARE(owned->original_facts, value.facts);
            QCOMPARE(owned->original_arrival, value.arrival);
            QCOMPARE(owned->attempts,
                     std::vector<ReceiptAttempt>({attempt(value, ReceiptDisposition::created)}));
        }
        QCOMPARE(fixture_->rows(
                     "SELECT "
                     "typeof(actor_id),length(actor_id),typeof(actor_display_name),hex(actor_"
                     "display_name),schema_version,source_timestamp_utc_ms,server_timestamp_utc_ms "
                     "FROM main.subathon_receipts WHERE opaque_id=X'65787472656D652D68696768'"),
                 SqlRows({{"blob", 0, "blob", "EFBBBF6100FF", qint64(4294967295LL),
                           qint64(std::numeric_limits<std::int64_t>::min()),
                           qint64(std::numeric_limits<std::int64_t>::max())}}));
        QCOMPARE(
            fixture_->rows("SELECT "
                           "typeof(actor_id),typeof(money_original_decimal),typeof(money_currency),"
                           "typeof(payment_namespace),typeof(activity_opaque_id),schema_version,"
                           "source_timestamp_utc_ms,typeof(connection_ref),length(connection_ref) "
                           "FROM main.subathon_receipts WHERE opaque_id=X'65787472656D652D6C6F77'"),
            SqlRows({{"null", "null", "null", "null", "null", 0, 0, "blob", 0}}));
        QCOMPARE(
            fixture_
                ->scalar("SELECT hex(money_original_decimal) FROM main.subathon_receipts WHERE "
                         "opaque_id=X'65787472656D652D68696768'")
                .toString(),
            QString("393030373139393235343734303939332E3030303030303030303030303030303030303031"));
    }

    void arrivalReferencesResolveWithoutCrosslinkEquality() {
        const auto cfg = QByteArray::fromHex("efbbbf6100ff"),
                   ctx = QByteArray::fromHex("00ffefbbbf62");
        QVERIFY(retained(cfg, ctx,
                         QByteArray("cfg2"))); // Known historical link differs from arrival cfg.
        auto value = input("binary-arrival");
        value.arrival.configuration_ref = opaque(cfg);
        value.arrival.modifier_context_ref = opaque(ctx);
        value.arrival.connection_ref = opaque(QByteArray::fromHex("00ffefbbbf63"));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        std::optional<ReceiptIntakeResult> result;
        auto r = ingest(writer, value, result);
        OK(r);
        std::optional<ReceiptRecord> record;
        auto q = read(writer, value.identity, record);
        OK(q);
        QVERIFY(record);
        QCOMPARE(record->original_arrival, value.arrival);
        QCOMPARE(fixture_->rows(
                     "SELECT "
                     "typeof(configuration_ref),hex(configuration_ref),typeof(modifier_context_ref)"
                     ",hex(modifier_context_ref),hex(connection_ref) FROM main.subathon_receipts"),
                 SqlRows({{"blob", "EFBBBF6100FF", "blob", "00FFEFBBBF62", "00FFEFBBBF63"}}));
        auto unknown = input("unknown-context-link");
        unknown.arrival.modifier_context_ref = "ctx-unknown";
        auto u = ingest(writer, unknown, result);
        OK(u);
        QCOMPARE(
            fixture_
                ->scalar("SELECT typeof(configuration_ref) FROM main.subathon_retained_contexts "
                         "WHERE reference=X'6374782D756E6B6E6F776E'")
                .toString(),
            QString("null"));
        for (int bad = 0; bad < 3; ++bad) {
            auto missing = input("missing-" + std::to_string(bad));
            if (bad == 0)
                missing.arrival.configuration_ref = "404";
            if (bad == 1)
                missing.arrival.modifier_context_ref = "404";
            if (bad == 2)
                missing.arrival.configuration_ref = "ctx1"; // Context namespace is not config.
            const auto before = fixture_->snapshot();
            bool caught = false;
            auto failure = run(writer, [&](auto work) -> QVariant {
                SqliteReceiptRepository repository(work);
                ReceiptIntake service(repository);
                try {
                    (void)service.ingest(missing);
                } catch (const SqliteReceiptStorageFailure &e) {
                    require(e.reason() == SqliteReceiptStorageReason::missing_reference,
                            "missing target category");
                    require(fixture_->released(), "missing reference rollback before throw");
                    caught = true;
                    throw;
                }
                return 1;
            });
            QVERIFY2(caught, qPrintable(failure.problem()));
            KNOWN(failure, SqliteReceiptStorageReason::missing_reference,
                  WorkError::work_exception);
            QCOMPARE(fixture_->snapshot(), before);
        }
    }

    void allocationCrossesSignedBoundaryAndExhaustsWithoutWrap() {
        QCOMPARE(
            fixture_->rows(
                "SELECT singleton,typeof(last_allocated) FROM main.subathon_receipt_allocator"),
            SqlRows({{1, "null"}}));
        QVERIFY(fixture_->seed(QByteArray::fromHex("7fffffffffffffff"), input("seed-signed-max")));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        std::optional<ReceiptIntakeResult> result;
        auto r = ingest(writer, input("above-signed"), result);
        OK(r);
        QVERIFY(result);
        QCOMPARE(result->receipt_id.value, std::uint64_t(9223372036854775808ULL));
        QCOMPARE(fixture_->scalar("SELECT hex(last_allocated) FROM main.subathon_receipt_allocator")
                     .toString(),
                 QString("8000000000000000"));
        QVERIFY(fixture_->seed(QByteArray::fromHex("fffffffffffffffe"), input("seed-before-max")));
        auto max_input = input("uint64-max");
        auto m = ingest(writer, max_input, result);
        OK(m);
        QVERIFY(result);
        QCOMPARE(result->receipt_id.value, std::numeric_limits<std::uint64_t>::max());
        QCOMPARE(fixture_->rows("SELECT typeof(receipt_id),length(receipt_id),hex(receipt_id) FROM "
                                "main.subathon_receipts WHERE opaque_id=X'75696E7436342D6D6178'"),
                 SqlRows({{"blob", 8, "FFFFFFFFFFFFFFFF"}}));
        const auto before = fixture_->snapshot();
        auto exhausted = ingest(writer, input("no-wrap"), result);
        KNOWN(exhausted, SqliteReceiptStorageReason::allocator_exhausted,
              WorkError::work_exception);
        QCOMPARE(fixture_->snapshot(), before);
        auto retry = ingest(writer, max_input, result);
        OK(retry);
        QVERIFY(result);
        QCOMPARE(result->disposition, ReceiptDisposition::retry);
        QCOMPARE(result->receipt_id.value, std::numeric_limits<std::uint64_t>::max());
        QCOMPARE(fixture_->scalar("SELECT hex(last_allocated) FROM main.subathon_receipt_allocator")
                     .toString(),
                 QString("FFFFFFFFFFFFFFFF"));
    }

    void failedCreateAndAppendRollBackAllocationAndRows() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        for (const auto point : {FailAfterStage::Point::create, FailAfterStage::Point::append}) {
            const auto before = fixture_->snapshot();
            bool created = false, appended = false, caught = false;
            auto r = run(writer, [&](auto work) -> QVariant {
                auto session = work.begin();
                SqliteReceiptRepository real(adopt_active_receipt_session, session);
                FailAfterStage fault(real, session, point, created, appended);
                ReceiptIntake service(fault);
                try {
                    (void)service.ingest(input());
                } catch (const SqliteReceiptStorageFailure &) {
                    require(fixture_->released(), "fault rollback synchronous");
                    caught = true;
                    throw;
                }
                return 1;
            });
            QVERIFY2(created, qPrintable(r.problem()));
            QCOMPARE(appended, point == FailAfterStage::Point::append);
            QVERIFY(caught);
            KNOWN(r, SqliteReceiptStorageReason::sql_failure, WorkError::sql_failure);
            QCOMPARE(fixture_->snapshot(), before);
        }
        std::optional<ReceiptIntakeResult> first;
        auto made = ingest(writer, input(), first);
        OK(made);
        QVERIFY(first);
        QCOMPARE(first->receipt_id.value, std::uint64_t(1));
        for (bool conflict : {false, true}) {
            auto value = input();
            value.arrival.configuration_ref = "cfg2";
            value.arrival.modifier_context_ref = "ctx2";
            if (conflict)
                value.facts.money->original_decimal = "99.0000";
            const auto before = fixture_->snapshot();
            bool created = false, appended = false;
            auto r = run(writer, [&](auto work) -> QVariant {
                auto session = work.begin();
                SqliteReceiptRepository real(adopt_active_receipt_session, session);
                FailAfterStage fault(real, session, FailAfterStage::Point::append, created,
                                     appended);
                ReceiptIntake service(fault);
                (void)service.ingest(value);
                return 1;
            });
            QVERIFY2(appended, qPrintable(r.problem()));
            QVERIFY(!created);
            KNOWN(r, SqliteReceiptStorageReason::sql_failure, WorkError::sql_failure);
            QCOMPARE(fixture_->snapshot(), before);
        }
    }

    void adoptionCommitsRetentionAndReceiptBeforeServiceReturns() {
        auto value = input("adopted");
        value.arrival.configuration_ref = "new-cfg";
        value.arrival.modifier_context_ref = "new-ctx";
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        std::optional<ReceiptIntakeResult> published;
        auto r = run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteRetainedContextStore retained_store(session);
            require(retained_store.retain({{RetainedKind::configuration, "new-cfg"},
                                           QByteArray::fromHex("00ff"),
                                           {}}) == RetainDisposition::inserted,
                    "staged config");
            require(retained_store.retain({{RetainedKind::arrival_context, "new-ctx"},
                                           QByteArray::fromHex("efbbbf61"),
                                           ConfigurationReference{"new-cfg"}}) ==
                        RetainDisposition::inserted,
                    "staged context");
            require(count("subathon_receipts") == 0, "receipt not visible before service");
            require(fixture_->scalar("SELECT count(*) FROM main.subathon_retained_configurations "
                                     "WHERE reference=X'6E65772D636667'")
                            .toInt() == 0,
                    "retention speculative");
            SqliteReceiptRepository repository(adopt_active_receipt_session, session);
            ReceiptIntake service(repository);
            published = service.ingest(value);
            require(count("subathon_receipts") == 1 && count("subathon_receipt_attempts") == 1,
                    "actual commit before ingest returns");
            require(fixture_->scalar("SELECT count(*) FROM main.subathon_retained_configurations "
                                     "WHERE reference=X'6E65772D636667'")
                            .toInt() == 1,
                    "same commit includes retention");
            return int(published->disposition);
        });
        OK(r);
        QVERIFY(published);
        QCOMPARE(published->disposition, ReceiptDisposition::created);
    }

    void uncommittedAndIncompleteTransactionsCannotPublish() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool created = false, appended = false;
        auto abandoned = run(writer, [&](auto work) -> QVariant {
            {
                SqliteReceiptRepository repository(work);
                auto tx = repository.begin();
                const auto value = input();
                const auto id = tx->create(value.identity, value.facts, value.arrival);
                created = true;
                tx->append_attempt(id, attempt(value, ReceiptDisposition::created));
                appended = true;
            }
            require(fixture_->released(), "wrapper destruction rolled back before return");
            return 1;
        });
        QVERIFY2(created && appended, qPrintable(abandoned.problem()));
        QCOMPARE(abandoned.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(abandoned.result.error, WorkError::not_committed);
        QVERIFY(!abandoned.result.value);
        QCOMPARE(count("subathon_receipts"), 0);
        QCOMPARE(count("subathon_receipt_attempts"), 0);
        bool pending = false, caught = false;
        auto incomplete = run(writer, [&](auto work) -> QVariant {
            SqliteReceiptRepository repository(work);
            auto tx = repository.begin();
            const auto v = input();
            (void)tx->create(v.identity, v.facts, v.arrival);
            pending = true;
            try {
                tx->commit();
            } catch (const SqliteReceiptStorageFailure &e) {
                require(e.reason() == SqliteReceiptStorageReason::incomplete_receipt,
                        "first attempt required");
                require(fixture_->released(), "incomplete guard rollback synchronous");
                caught = true;
                throw;
            }
            return 1;
        });
        QVERIFY2(pending && caught, qPrintable(incomplete.problem()));
        KNOWN(incomplete, SqliteReceiptStorageReason::incomplete_receipt,
              WorkError::work_exception);
        QCOMPARE(count("subathon_receipts"), 0);
        QCOMPARE(
            fixture_->scalar("SELECT typeof(last_allocated) FROM main.subathon_receipt_allocator")
                .toString(),
            QString("null"));
    }

    void duplicatePrimitiveCreateIsSqlUniquenessFailure() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        const auto before = fixture_->snapshot();
        bool first_created = false;
        auto r = run(writer, [&](auto work) -> QVariant {
            SqliteReceiptRepository repository(work);
            auto tx = repository.begin();
            const auto value = input();
            (void)tx->create(value.identity, value.facts, value.arrival);
            first_created = true;
            (void)tx->create(value.identity, value.facts, value.arrival);
            return 1;
        });
        QVERIFY2(first_created, qPrintable(r.problem()));
        KNOWN(r, SqliteReceiptStorageReason::sql_failure, WorkError::sql_failure);
        QCOMPARE(fixture_->snapshot(), before);
        std::optional<ReceiptIntakeResult> result;
        auto success = ingest(writer, input(), result);
        OK(success);
        QVERIFY(result);
        const auto stable = fixture_->snapshot();
        auto duplicate = run(writer, [&](auto work) -> QVariant {
            SqliteReceiptRepository repository(work);
            auto tx = repository.begin();
            const auto value = input();
            (void)tx->create(value.identity, value.facts, value.arrival);
            return 1;
        });
        KNOWN(duplicate, SqliteReceiptStorageReason::sql_failure, WorkError::sql_failure);
        QCOMPARE(fixture_->snapshot(), stable);
    }

    void inputPreflightPriorityAndBoundsPreserveEarlierWrites() {
        SqliteWriter writer;
        StorageLimits capacity;
        capacity.statements_per_batch = 512;
        capacity.bound_value_bytes = 4194304;
        capacity.result_value_bytes = 4194304;
        QCOMPARE(writer.open(path_, capacity), StorageError::none);
        auto r = run(writer, [&](auto work) -> QVariant {
            for (const auto limits :
                 {SqliteReceiptLimits{0, 4, 4}, SqliteReceiptLimits{4, 0, 4},
                  SqliteReceiptLimits{4, 4, 0}, SqliteReceiptLimits{-1, 4, 4},
                  SqliteReceiptLimits{4, -1, 4}, SqliteReceiptLimits{4, 4, -1},
                  SqliteReceiptLimits{4, 4, std::numeric_limits<qsizetype>::max()}}) {
                SqliteReceiptRepository bad(SqliteWorkContext{}, limits);
                expect_input([&] { (void)bad.begin(); }, SqliteReceiptInputError::invalid_options);
            }
            SqliteReceiptRepository repository(work, {4, 3, 4});
            auto tx = repository.begin();
            auto tiny = input("id01");
            tiny.identity = {{"prov", "acct"}, "name", "id01"};
            tiny.facts = {"evt",
                          0,
                          {},
                          std::string(),
                          {},
                          ReceiptMoney{"1.0", "EUR"},
                          ReceiptLinkReference{"ns", "id"},
                          {}};
            tiny.arrival = {0, "conn", "cfg1", "ctx1"};
            const auto id = tx->create(tiny.identity, tiny.facts, tiny.arrival);
            tx->append_attempt(id, attempt(tiny, ReceiptDisposition::created));
            auto bad = tiny;
            bad.identity.opaque_id = "";
            bad.facts.event_type = "xxxx";
            bad.arrival.configuration_ref = "";
            expect_input([&] { (void)tx->create(bad.identity, bad.facts, bad.arrival); },
                         SqliteReceiptInputError::invalid_identity);
            bad = tiny;
            bad.identity.opaque_id = "12345";
            expect_input([&] { (void)tx->find(bad.identity); },
                         SqliteReceiptInputError::invalid_identity);
            bad = tiny;
            bad.facts.event_type = "xxxx";
            bad.arrival.configuration_ref = "";
            expect_input([&] { (void)tx->create(bad.identity, bad.facts, bad.arrival); },
                         SqliteReceiptInputError::invalid_facts);
            bad = tiny;
            bad.arrival.configuration_ref = "";
            expect_input([&] { (void)tx->create(bad.identity, bad.facts, bad.arrival); },
                         SqliteReceiptInputError::invalid_arrival);
            auto bad_attempt = attempt(tiny, static_cast<ReceiptDisposition>(99));
            bad_attempt.observed_facts.event_type = "xxxx";
            bad_attempt.arrival.configuration_ref = "";
            expect_input([&] { tx->append_attempt({0}, bad_attempt); },
                         SqliteReceiptInputError::invalid_receipt_id);
            expect_input([&] { tx->append_attempt(id, bad_attempt); },
                         SqliteReceiptInputError::invalid_facts);
            bad_attempt.observed_facts = tiny.facts;
            expect_input([&] { tx->append_attempt(id, bad_attempt); },
                         SqliteReceiptInputError::invalid_arrival);
            bad_attempt.arrival = tiny.arrival;
            expect_input([&] { tx->append_attempt(id, bad_attempt); },
                         SqliteReceiptInputError::invalid_disposition);
            require(found(*tx, tiny.identity).attempts.size() == 1,
                    "caught preflight preserves prior write");
            tx->commit();
            return 1;
        });
        OK(r);
        QCOMPARE(count("subathon_receipts"), 1);
        auto full = input(std::string(4096, 'r'));
        full.facts.event_type = std::string(262144, 'v');
        auto exact = run(writer, [&](auto work) -> QVariant {
            SqliteReceiptRepository repository(work);
            auto tx = repository.begin();
            const auto id = tx->create(full.identity, full.facts, full.arrival);
            tx->append_attempt(id, attempt(full, ReceiptDisposition::created));
            auto bad = full;
            bad.identity.opaque_id.push_back('r');
            expect_input([&] { (void)tx->create(bad.identity, bad.facts, bad.arrival); },
                         SqliteReceiptInputError::invalid_identity);
            bad = full;
            bad.facts.event_type.push_back('v');
            expect_input([&] { (void)tx->create(bad.identity, bad.facts, bad.arrival); },
                         SqliteReceiptInputError::invalid_facts);
            tx->commit();
            return 1;
        });
        OK(exact);
        QCOMPARE(fixture_
                     ->scalar("SELECT count(*) FROM main.subathon_receipts WHERE "
                              "length(opaque_id)=4096 AND length(event_type)=262144")
                     .toInt(),
                 1);
    }

    void selectedMalformedDataAndTargetsRequireKnownRollback() {
        const QList<QString> alterations = {
            "UPDATE main.subathon_receipts SET money_currency=NULL WHERE opaque_id=X'6261642D30'",
            "UPDATE main.subathon_receipts SET actor_id='text' WHERE opaque_id=X'6261642D31'",
            "UPDATE main.subathon_receipts SET source_timestamp_utc_ms=1.5 WHERE "
            "opaque_id=X'6261642D32'",
            "UPDATE main.subathon_receipts SET schema_version=4294967296 WHERE "
            "opaque_id=X'6261642D33'",
            "UPDATE main.subathon_receipts SET receipt_id='00000005' WHERE opaque_id=X'6261642D34'",
            "UPDATE main.subathon_receipts SET receipt_id=X'00000000000006' WHERE "
            "opaque_id=X'6261642D35'",
            "UPDATE main.subathon_receipts SET receipt_id=X'0000000000000000' WHERE "
            "opaque_id=X'6261642D36'",
            "UPDATE main.subathon_receipts SET receipt_id=8 WHERE opaque_id=X'6261642D37'",
            "UPDATE main.subathon_receipts SET receipt_id=9.5 WHERE opaque_id=X'6261642D38'",
            "UPDATE main.subathon_receipts SET configuration_ref=X'343034' WHERE "
            "opaque_id=X'6261642D39'",
            "UPDATE main.subathon_receipts SET payment_opaque_id=NULL WHERE "
            "opaque_id=X'6261642D3130'",
            "UPDATE main.subathon_receipts SET activity_namespace='text' WHERE "
            "opaque_id=X'6261642D3131'",
            "UPDATE main.subathon_receipts SET receipt_id=X'000000000000000e00' WHERE "
            "opaque_id=X'6261642D3132'"};
        const QList<QByteArray> ids = {
            QByteArray::fromHex("0000000000000001"), QByteArray::fromHex("0000000000000002"),
            QByteArray::fromHex("0000000000000003"), QByteArray::fromHex("0000000000000004"),
            QByteArray::fromHex("0000000000000005"), QByteArray::fromHex("0000000000000006"),
            QByteArray::fromHex("0000000000000007"), QByteArray::fromHex("0000000000000008"),
            QByteArray::fromHex("0000000000000009"), QByteArray::fromHex("000000000000000a"),
            QByteArray::fromHex("000000000000000b"), QByteArray::fromHex("000000000000000c"),
            QByteArray::fromHex("000000000000000e")};
        for (qsizetype i = 0; i < ids.size(); ++i)
            QVERIFY(fixture_->seed(ids[i], input("bad-" + std::to_string(i))));
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        for (const auto &sql : alterations)
            QVERIFY(fixture_->exec(sql));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        for (qsizetype i = 0; i < ids.size(); ++i) {
            bool caught = false;
            auto r = run(writer, [&](auto work) -> QVariant {
                SqliteReceiptRepository repository(work);
                auto tx = repository.begin();
                try {
                    (void)tx->find(input("bad-" + std::to_string(i)).identity);
                } catch (const SqliteReceiptStorageFailure &e) {
                    require(e.reason() == SqliteReceiptStorageReason::invalid_stored_data,
                            "selected corruption category");
                    require(fixture_->released(), "selected corruption rollback before throw");
                    caught = true;
                    throw;
                }
                return 1;
            });
            QVERIFY2(caught, qPrintable(r.problem()));
            KNOWN(r, SqliteReceiptStorageReason::invalid_stored_data, WorkError::work_exception);
            QCOMPARE(fixture_->snapshot(), before);
        }
        // A valid larger selected string is a limit, not malformed data/missing.
        const auto healthy = input("selected-large");
        QVERIFY(fixture_->seed(QByteArray::fromHex("000000000000000d"), healthy));
        std::optional<ReceiptRecord> record;
        auto limited = read(writer, healthy.identity, record, {4096, 3, 1024});
        KNOWN(limited, SqliteReceiptStorageReason::result_limit, WorkError::work_exception);
        QVERIFY(!record);
    }

    void allocatorAndAttemptHistoryCorruptionIsRefused() {
        QVERIFY(fixture_->seed(QByteArray::fromHex("0000000000000001"), input("seed")));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        for (const auto &value :
             {QVariant(), QVariant(QByteArray::fromHex("00000000000000")),
              QVariant(QString("00000001")), QVariant(1), QVariant(QByteArray(8, '\0')),
              QVariant(QByteArray::fromHex("0000000000000002"))}) {
            QVERIFY(fixture_->exec("UPDATE main.subathon_receipt_allocator SET last_allocated=?",
                                   {value}));
            const auto before = fixture_->snapshot();
            std::optional<ReceiptIntakeResult> result;
            auto r = ingest(writer, input("must-not-reset"), result);
            KNOWN(r, SqliteReceiptStorageReason::invalid_stored_data, WorkError::work_exception);
            QVERIFY(!result);
            QCOMPARE(fixture_->snapshot(), before);
        }
        QVERIFY(fixture_->exec("DELETE FROM main.subathon_receipt_allocator"));
        std::optional<ReceiptIntakeResult> result;
        auto absent = ingest(writer, input("allocator-absent"), result);
        KNOWN(absent, SqliteReceiptStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(fixture_->exec(
            "INSERT INTO main.subathon_receipt_allocator VALUES (1,X'0000000000000001')"));
        for (const auto &sql :
             {QString("UPDATE main.subathon_receipt_attempts SET ordinal=2"),
              QString("UPDATE main.subathon_receipt_attempts SET ordinal=9223372036854775807"),
              QString("UPDATE main.subathon_receipt_attempts SET ordinal=1,disposition=7")}) {
            QVERIFY(fixture_->exec(sql));
            const auto before = fixture_->snapshot();
            std::optional<ReceiptRecord> record;
            auto r = read(writer, input("seed").identity, record);
            KNOWN(r, SqliteReceiptStorageReason::invalid_stored_data, WorkError::work_exception);
            QVERIFY(!record);
            QCOMPARE(fixture_->snapshot(), before);
        }
        // One MAX ordinal is noncontiguous corruption, not reachable exhaustion.
        // The true MAX-contiguous-history increment guard remains code-review proof.
    }

    void findHistoryLimitDoesNotTruncateOrLimitAppend() {
        const auto value = input("history");
        const auto id = QByteArray::fromHex("0000000000000001");
        QVERIFY(fixture_->seed(id, value));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool appended = false;
        auto r = run(writer, [&](auto work) -> QVariant {
            SqliteReceiptRepository repository(work, {4096, 262144, 1});
            auto tx = repository.begin();
            tx->append_attempt({1}, attempt(value, ReceiptDisposition::retry));
            appended = true;
            tx->commit();
            return 1;
        });
        QVERIFY2(appended, qPrintable(r.problem()));
        OK(r);
        QCOMPARE(count("subathon_receipt_attempts"), 2);
        const auto before = fixture_->snapshot();
        bool caught = false;
        auto refused = run(writer, [&](auto work) -> QVariant {
            SqliteReceiptRepository repository(work, {4096, 262144, 1});
            auto tx = repository.begin();
            try {
                (void)tx->find(value.identity);
            } catch (const SqliteReceiptStorageFailure &e) {
                require(e.reason() == SqliteReceiptStorageReason::result_limit,
                        "history overflow is limit");
                require(fixture_->released(), "history limit synchronous cleanup");
                caught = true;
                throw;
            }
            return 1;
        });
        QVERIFY2(caught, qPrintable(refused.problem()));
        KNOWN(refused, SqliteReceiptStorageReason::result_limit, WorkError::work_exception);
        QCOMPARE(fixture_->snapshot(), before);
        std::optional<ReceiptRecord> record;
        auto exact = read(writer, value.identity, record, {4096, 262144, 2});
        OK(exact);
        QVERIFY(record);
        QCOMPARE(record->attempts,
                 std::vector<ReceiptAttempt>({attempt(value, ReceiptDisposition::created),
                                              attempt(value, ReceiptDisposition::retry)}));
    }

    void cumulativeBudgetFailureRollsBackStagedReceipt() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        const auto before = fixture_->snapshot();
        bool created = false, appended = false, caught = false;
        auto r = run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteReceiptRepository repository(adopt_active_receipt_session, session);
            auto tx = repository.begin();
            const auto value = input();
            const auto id = tx->create(value.identity, value.facts, value.arrival);
            created = true;
            tx->append_attempt(id, attempt(value, ReceiptDisposition::created));
            appended = true;
            try {
                (void)session.execute({"SELECT zeroblob(1048577)", {}});
            } catch (const SqliteWorkFailure &e) {
                require(e.error() == WorkError::result_limit,
                        "literal result exceeds default budget");
                require(fixture_->released(), "budget rollback before catch returns");
                caught = true;
                throw;
            }
            return 1;
        });
        QVERIFY2(created && appended && caught, qPrintable(r.problem()));
        QCOMPARE(r.synchronous, std::optional<WorkError>(WorkError::result_limit));
        QCOMPARE(r.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(r.result.error, WorkError::result_limit);
        QVERIFY(!r.result.value);
        QCOMPARE(fixture_->snapshot(), before);

        // Each result charges 600008 bytes, individually below 1048576. A reset
        // per execute would incorrectly accept both; prior adapter charges may
        // legitimately make the first or second fail, without a query-plan oracle.
        bool cumulative_created = false, cumulative_appended = false, cumulative_caught = false;
        int attempted_query = 0;
        auto cumulative = run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteReceiptRepository repository(adopt_active_receipt_session, session);
            auto tx = repository.begin();
            const auto value = input("cumulative-unit");
            const auto id = tx->create(value.identity, value.facts, value.arrival);
            cumulative_created = true;
            tx->append_attempt(id, attempt(value, ReceiptDisposition::created));
            cumulative_appended = true;
            try {
                for (int query = 1; query <= 2; ++query) {
                    attempted_query = query;
                    (void)session.execute({"SELECT zeroblob(600000)", {}});
                }
            } catch (const SqliteWorkFailure &e) {
                require(e.error() == WorkError::result_limit,
                        "individually valid results exhaust cumulative budget");
                require(fixture_->released(), "cumulative rollback before catch returns");
                cumulative_caught = true;
                throw;
            }
            throw std::runtime_error("both individually valid results incorrectly accepted");
        });
        QVERIFY2(cumulative_created && cumulative_appended && cumulative_caught,
                 qPrintable(cumulative.problem()));
        QVERIFY(attempted_query >= 1 && attempted_query <= 2);
        QCOMPARE(cumulative.synchronous, std::optional<WorkError>(WorkError::result_limit));
        QCOMPARE(cumulative.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(cumulative.result.error, WorkError::result_limit);
        QVERIFY(!cumulative.result.value);
        QCOMPARE(fixture_->snapshot(), before);
    }

    void localSecondBeginPreservesFirstTransaction() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        auto r = run(writer, [&](auto work) -> QVariant {
            SqliteReceiptRepository repository(work);
            auto tx = repository.begin();
            expect_input([&] { (void)repository.begin(); }, SqliteReceiptInputError::already_begun);
            const auto value = input();
            const auto id = tx->create(value.identity, value.facts, value.arrival);
            tx->append_attempt(id, attempt(value, ReceiptDisposition::created));
            tx->commit();
            return 1;
        });
        OK(r);
        QCOMPARE(count("subathon_receipts"), 1);
        QVERIFY(fixture_->exec("BEGIN IMMEDIATE"));
        bool caught = false;
        auto busy = run(writer, [&](auto work) -> QVariant {
            SqliteReceiptRepository repository(work);
            try {
                (void)repository.begin();
            } catch (const SqliteReceiptStorageFailure &e) {
                require(e.reason() == SqliteReceiptStorageReason::busy,
                        "confirmed busy BEGIN category");
                caught = true;
                expect_input([&] { (void)repository.begin(); },
                             SqliteReceiptInputError::already_begun);
                throw;
            }
            return 1;
        });
        QVERIFY(fixture_->exec("ROLLBACK"));
        QVERIFY2(caught, qPrintable(busy.problem()));
        QCOMPARE(busy.storage,
                 std::optional<SqliteReceiptStorageReason>(SqliteReceiptStorageReason::busy));
        QCOMPARE(busy.result.transaction, TransactionOutcome::not_started);
        QCOMPARE(busy.result.error, WorkError::busy);
        QVERIFY(!busy.result.value);
        std::optional<ReceiptIntakeResult> retry;
        auto next = ingest(writer, input(), retry);
        OK(next);
        QVERIFY(retry);
        QCOMPARE(retry->disposition, ReceiptDisposition::retry);
    }

    void foreignStaleTerminalAndOwnedLifetimesAreSafe() {
        const auto value = input("owned");
        QVERIFY(fixture_->seed(QByteArray::fromHex("0000000000000001"), value));
        std::unique_ptr<ReceiptTransaction> expired;
        std::optional<ReceiptRecord> owned;
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            auto foreign = run(writer, [&](auto work) -> QVariant {
                SqliteReceiptRepository repository(work), bad_options(work, {0, 1, 1});
                std::optional<WorkError> work_error;
                std::optional<SqliteReceiptInputError> input_error;
                QString unexpected;
                std::thread thread([&] {
                    try {
                        (void)repository.begin();
                    } catch (const SqliteWorkFailure &e) {
                        work_error = e.error();
                    } catch (const std::exception &e) {
                        unexpected = QString::fromUtf8(e.what());
                    }
                    try {
                        (void)bad_options.begin();
                    } catch (const SqliteReceiptInputFailure &e) {
                        input_error = e.error();
                    } catch (const std::exception &e) {
                        unexpected = QString::fromUtf8(e.what());
                    }
                });
                thread.join();
                require(unexpected.isEmpty(), "unexpected foreign begin exception");
                require(work_error == WorkError::wrong_thread,
                        "foreign begin before mutable guard");
                require(input_error == SqliteReceiptInputError::invalid_options,
                        "pure option precedence before thread");
                auto tx = repository.begin();
                require(tx->find(value.identity).has_value(),
                        "foreign reject did not consume owner begin");
                tx->commit();
                return 1;
            });
            OK(foreign);
            auto terminal = run(writer, [&](auto work) -> QVariant {
                {
                    SqliteReceiptRepository temporary(work);
                    expired = temporary.begin();
                }
                owned = expired->find(value.identity);
                std::optional<WorkError> work_error;
                std::optional<SqliteReceiptInputError> input_error;
                QString unexpected;
                std::thread thread([&] {
                    try {
                        (void)expired->find(value.identity);
                    } catch (const SqliteWorkFailure &e) {
                        work_error = e.error();
                    } catch (const std::exception &e) {
                        unexpected = QString::fromUtf8(e.what());
                    }
                    try {
                        expired->append_attempt({0}, attempt(value, ReceiptDisposition::retry));
                    } catch (const SqliteReceiptInputFailure &e) {
                        input_error = e.error();
                    } catch (const std::exception &e) {
                        unexpected = QString::fromUtf8(e.what());
                    }
                });
                thread.join();
                require(unexpected.isEmpty(), "unexpected foreign primitive exception");
                require(work_error == WorkError::wrong_thread,
                        "foreign primitive capability category");
                require(input_error == SqliteReceiptInputError::invalid_receipt_id,
                        "pure primitive preflight wins foreign");
                expired->commit();
                expect_work([&] { (void)expired->find(value.identity); },
                            WorkError::session_closed);
                return 1;
            });
            QVERIFY(terminal.admitted && terminal.done);
            QVERIFY2(!terminal.stub && terminal.unexpected.isEmpty(),
                     qPrintable(terminal.problem()));
            QCOMPARE(terminal.result.transaction, TransactionOutcome::committed);
            QCOMPARE(terminal.result.error, WorkError::session_closed);
            QVERIFY(!terminal.result.value);
        }
        QVERIFY(owned);
        QCOMPARE(owned->original_facts, value.facts);
        QCOMPARE(owned->original_arrival, value.arrival);
        QCOMPARE(owned->attempts,
                 std::vector<ReceiptAttempt>({attempt(value, ReceiptDisposition::created)}));
        try {
            expect_work([&] { (void)expired->find(value.identity); }, WorkError::stale_session);
            auto invalid = value.identity;
            invalid.opaque_id.clear();
            expect_input([&] { (void)expired->find(invalid); },
                         SqliteReceiptInputError::invalid_identity);
            SqliteReceiptRepository stale(SqliteWorkContext{});
            expect_work([&] { (void)stale.begin(); }, WorkError::stale_session);
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
        expired.reset(); // Stale destructor cleanup must remain harmless/no SQL.
    }

    void lostCommitAckReportsUnknownWithPhysicalReceipt() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_commit_ack),
                 StorageError::none);
        std::optional<ReceiptIntakeResult> returned;
        int body_calls = 0;
        auto r = run(writer, [&](auto work) -> QVariant {
            ++body_calls;
            SqliteReceiptRepository repository(work);
            ReceiptIntake service(repository);
            returned = service.ingest(input());
            return 1;
        });
        QVERIFY2(!r.stub, qPrintable(r.problem()));
        QCOMPARE(body_calls, 1);
        QVERIFY(!returned);
        QVERIFY(!r.storage);
        QCOMPARE(r.synchronous, std::optional<WorkError>(WorkError::uncertain_outcome));
        QCOMPARE(r.result.transaction, TransactionOutcome::outcome_unknown);
        QCOMPARE(r.result.error, WorkError::uncertain_outcome);
        QVERIFY(!r.result.value);
        QCOMPARE(count("subathon_receipts"), 1);
        QCOMPARE(count("subathon_receipt_attempts"), 1);
        QCOMPARE(
            fixture_->rows("SELECT typeof(receipt_id),hex(receipt_id) FROM main.subathon_receipts"),
            SqlRows({{"blob", "0000000000000001"}}));
        QCOMPARE(fixture_->scalar("SELECT hex(last_allocated) FROM main.subathon_receipt_allocator")
                     .toString(),
                 QString("0000000000000001"));
        QCOMPARE(writer.enqueue_work([](auto) { return QVariant(1); }, [](auto) {}),
                 StorageError::shutting_down);
    }

    void lostCleanupAckOutranksLocalStorageFailure() {
        const auto corrupt = input("corrupt");
        QVERIFY(fixture_->seed(QByteArray::fromHex("0000000000000001"), corrupt));
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        QVERIFY(fixture_->exec("UPDATE main.subathon_receipts SET money_currency=NULL"));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_rollback_ack),
                 StorageError::none);
        bool created = false, appended = false;
        int body_calls = 0;
        auto r = run(writer, [&](auto work) -> QVariant {
            ++body_calls;
            auto session = work.begin();
            SqliteReceiptRepository repository(adopt_active_receipt_session, session);
            auto tx = repository.begin();
            const auto good = input("rollback-staged");
            const auto id = tx->create(good.identity, good.facts, good.arrival);
            created = true;
            tx->append_attempt(id, attempt(good, ReceiptDisposition::created));
            appended = true;
            (void)tx->find(corrupt.identity);
            return 1;
        });
        QVERIFY2(created && appended, qPrintable(r.problem()));
        QCOMPARE(body_calls, 1);
        QVERIFY(!r.storage);
        QCOMPARE(r.synchronous, std::optional<WorkError>(WorkError::uncertain_outcome));
        QCOMPARE(r.result.transaction, TransactionOutcome::outcome_unknown);
        QCOMPARE(r.result.error, WorkError::uncertain_outcome);
        QVERIFY(!r.result.value);
        QCOMPARE(fixture_->snapshot(),
                 before); // Actual rollback happened; acknowledgement was lost.
        QCOMPARE(writer.enqueue_work([](auto) { return QVariant(1); }, [](auto) {}),
                 StorageError::shutting_down);
    }
};
QTEST_GUILESS_MAIN(SqliteReceiptTests)
#include "sqlite_receipts_tests.moc"
