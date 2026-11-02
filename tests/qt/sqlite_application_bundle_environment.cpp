// SPDX-License-Identifier: GPL-3.0-or-later
// Literal SQL foundation only. Never includes/links/calls the new bundle API or factory.
#include <QCryptographicHash>
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
#include <memory>
#include <stdexcept>
#include <subathon/server/sqlite_writer.hpp>

using namespace subathon::server;
namespace {
void require(bool condition, const QString &message) {
    if (!condition)
        throw std::runtime_error(message.toStdString());
}
QByteArray literal_hex(const QString &text) {
    const auto ascii = text.toLatin1();
    const auto bytes = QByteArray::fromHex(ascii);
    require(bytes.toHex() == ascii.toLower(), "Invalid literal hexadecimal bytes");
    return bytes.isEmpty() ? QByteArray("", 0) : bytes;
}
QVariant literal_value(const QJsonValue &entry) {
    const auto value = entry.toObject();
    const auto type = value["type"].toString();
    if (type == "null")
        return {};
    if (type == "blob")
        return literal_hex(value["hex"].toString());
    if (type == "text")
        return value["value"].toString();
    if (type == "integer") {
        bool ok = false;
        const auto number = value["decimal"].toString().toLongLong(&ok);
        require(ok, "Invalid literal signed64 decimal string");
        return number;
    }
    throw std::runtime_error("Unknown literal SQL value type");
}
SqlStatement literal_statement(const QJsonValue &entry) {
    const auto object = entry.toObject();
    QList<QVariant> bindings;
    for (const auto &value : object["bindings"].toArray())
        bindings.append(literal_value(value));
    return {object["sql"].toString(), bindings};
}
void execute_steps(const SqliteWorkSession &session, const QJsonArray &steps) {
    for (const auto &entry : steps)
        (void)session.execute(literal_statement(entry));
}
// SQL NULL has driver-specific QVariant metatype; nullness is the portable truth.
// Non-null data also checks the Qt representation, without numeric TEXT/REAL coercion.
void check_rows(const SqlRows &actual, const QJsonObject &query) {
    const auto expected = query["expected"].toArray();
    const auto label = query["name"].toString();
    require(actual.size() == expected.size(), label + ": row count");
    for (qsizetype row = 0; row < actual.size(); ++row) {
        const auto cells = expected[row].toArray();
        require(actual[row].size() == cells.size(), label + ": column count");
        for (qsizetype column = 0; column < cells.size(); ++column) {
            const auto descriptor = cells[column].toObject();
            const auto &observed = actual[row][column];
            const auto type = descriptor["type"].toString();
            bool equal = false;
            if (type == "null") {
                equal = observed.isNull();
            } else if (!observed.isNull()) {
                const auto value = literal_value(descriptor);
                if (type == "integer")
                    equal = (observed.metaType().id() == QMetaType::LongLong ||
                             observed.metaType().id() == QMetaType::Int) &&
                            observed.toLongLong() == value.toLongLong();
                else if (type == "blob")
                    equal = observed.metaType().id() == QMetaType::QByteArray &&
                            observed.toByteArray() == value.toByteArray();
                else if (type == "text")
                    equal = observed.metaType().id() == QMetaType::QString &&
                            observed.toString() == value.toString();
            }
            require(equal, label + QString(": value at row%1 column%2").arg(row).arg(column));
        }
    }
}
void check_queries(const SqliteWorkSession &session, const QJsonArray &queries) {
    for (const auto &entry : queries) {
        const auto query = entry.toObject();
        check_rows(session.execute({query["sql"].toString(), {}}), query);
    }
}
QByteArray frame(const QByteArray &bytes) { return QByteArray::number(bytes.size()) + ':' + bytes; }
QByteArray independent_frame(const QJsonObject &manifest) {
    const auto statements = manifest["sql"].toArray();
    const auto objects = manifest["expected_schema"].toArray();
    QByteArray result =
        frame("migration-catalog-v1") + frame(QByteArray::number(manifest["version"].toInt())) +
        frame(manifest["id"].toString().toUtf8()) + frame(QByteArray::number(statements.size()));
    for (const auto &statement : statements)
        result += frame(statement.toString().toUtf8());
    result += frame(QByteArray::number(objects.size()));
    for (const auto &entry : objects) {
        const auto object = entry.toObject();
        for (const auto &field : {"type", "name", "table_name"})
            result += frame(object[field].toString().toUtf8());
        result += frame(object["sql"].isNull() ? "0" : "1");
        if (!object["sql"].isNull())
            result += frame(object["sql"].toString().toUtf8());
    }
    return result;
}
struct Run {
    bool completed{false};
    SqliteWorkResult result;
    QString problem;
};
Run run(SqliteWriter &writer, SqliteWriter::Work body) {
    struct State {
        Run observed;
    };
    auto state = std::make_shared<State>();
    const auto admitted = writer.enqueue_work(
        [state, body = std::move(body)](auto context) {
            try {
                return body(context);
            } catch (const std::exception &error) {
                state->observed.problem = QString::fromUtf8(error.what());
                throw;
            }
        },
        [state](auto result) {
            state->observed.result = std::move(result);
            state->observed.completed = true;
        });
    if (admitted != StorageError::none)
        state->observed.problem = "Existing writer did not admit fixture work";
    else if (!QTest::qWaitFor([state] { return state->observed.completed; }, 5000))
        state->observed.problem = "Existing writer fixture work timed out";
    return state->observed;
}
bool committed(const Run &observed) {
    return observed.completed && observed.result.transaction == TransactionOutcome::committed &&
           observed.result.error == WorkError::none && observed.result.value.has_value();
}
StorageLimits foundation_limits() {
    StorageLimits limits;
    limits.statements_per_batch = 1024;
    limits.statement_utf8_bytes = 262144;
    limits.bound_value_bytes = 16777216;
    limits.result_rows = 8192;
    limits.result_value_bytes = 16777216;
    return limits;
}
QString quoted(QString name) {
    name.replace('"', "\"\"");
    return '"' + name + '"';
}
// Separate disposable observer connection, never a raw handle to the controlled writer.
// Test instrumentation DDL is installed/removed only while the controlled writer is idle.
// Application writes use that writer; BEGIN IMMEDIATE/ROLLBACK also probes lock release.
class Observer final {
  public:
    Observer()
        : name_(QUuid::createUuid().toString()), db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {}
    ~Observer() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }
    bool open(const QString &path) {
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
        return db_.open();
    }
    SqlRows rows(const QString &sql) {
        QSqlQuery query(db_);
        require(query.exec(sql), "Independent observer query failed: " + query.lastError().text());
        SqlRows result;
        while (query.next()) {
            SqlRow row;
            for (int column = 0; column < query.record().count(); ++column)
                row.append(query.value(column));
            result.append(row);
        }
        require(!query.lastError().isValid(), "Independent observer fetch failed");
        return result;
    }
    bool released() {
        QSqlQuery query(db_);
        return query.exec("BEGIN IMMEDIATE") && query.exec("ROLLBACK");
    }
    bool test_instrumentation_ddl(const QString &sql) {
        QSqlQuery query(db_);
        return query.exec(sql);
    }
    QList<SqlRows> snapshot() {
        QList<SqlRows> result;
        const auto schema =
            rows("SELECT type,name,tbl_name,sql FROM sqlite_schema ORDER BY type,name");
        result.append(schema);
        for (const auto &object : schema) {
            if (object[0].toString() != "table")
                continue;
            const auto table = quoted(object[1].toString());
            QStringList projection;
            for (const auto &column : rows("PRAGMA main.table_info(" + table + ")")) {
                const auto name = quoted(column[1].toString());
                projection.append("typeof(" + name + ")");
                projection.append("hex(CAST(" + name + " AS BLOB))");
            }
            result.append(
                rows("SELECT " + projection.join(',') + " FROM main." + table + " ORDER BY rowid"));
        }
        return result;
    }

  private:
    QString name_;
    QSqlDatabase db_;
};
} // namespace

class SqliteApplicationBundleEnvironment final : public QObject {
    Q_OBJECT
  private:
    QJsonObject fixture_, manifest_, scenarios_, queries_;
    QList<QJsonObject> prefixes_;

    void create_literal_schema(const SqliteWorkSession &session) const {
        for (const auto &entry : prefixes_[0]["expected_schema"].toArray()) {
            const auto object = entry.toObject();
            if (object["name"] == "subathon_catalog" ||
                object["name"] == "subathon_migration_history")
                (void)session.execute({object["sql"].toString(), {}});
        }
        for (const auto &manifest : prefixes_)
            for (const auto &sql : manifest["sql"].toArray())
                (void)session.execute({sql.toString(), {}});
        for (const auto &sql : manifest_["sql"].toArray())
            (void)session.execute({sql.toString(), {}});
        (void)session.execute(
            {"INSERT INTO subathon_catalog VALUES(1,1,'subathon.sqlite-store')", {}});
        for (const auto &prefix : prefixes_)
            (void)session.execute({"INSERT INTO subathon_migration_history VALUES(?,?,?)",
                                   {prefix["version"].toInt(), prefix["id"].toString(),
                                    literal_hex(prefix["sha256_hex"].toString())}});
        (void)session.execute(
            {"INSERT INTO subathon_migration_history VALUES(5,'atomic-application-bundles-v1',?)",
             {literal_hex(manifest_["sha256_hex"].toString())}});
    }
    Run bootstrap(SqliteWriter &writer) const {
        return run(writer, [&](auto context) {
            auto session = context.begin();
            create_literal_schema(session);
            execute_steps(session, fixture_["literal_setup"].toArray());
            check_queries(session, QJsonArray{queries_["pending"], queries_["preservation"]});
            session.commit();
            return QVariant(1);
        });
    }
    Run stage_literal(SqliteWriter &writer, const QStringList &names,
                      const QJsonArray &queries = {}) const {
        return run(writer, [&](auto context) {
            auto session = context.begin();
            for (const auto &name : names)
                execute_steps(session, scenarios_[name].toArray());
            check_queries(session, queries);
            session.commit();
            return QVariant(1);
        });
    }

  private slots:
    void initTestCase() {
        QVERIFY(QSqlDatabase::isDriverAvailable("QSQLITE"));
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
        const auto read = [](const char *path) {
            QFile file(QString::fromUtf8(path));
            require(file.open(QIODevice::ReadOnly), "Missing independent literal file");
            QJsonParseError error;
            const auto document = QJsonDocument::fromJson(file.readAll(), &error);
            require(error.error == QJsonParseError::NoError && document.isObject(),
                    "Invalid independent JSON fixture");
            return document.object();
        };
        for (const auto *path : {SUBATHON_RETAINED_VECTOR_PATH, SUBATHON_RECEIPT_VECTOR_PATH,
                                 SUBATHON_CONTRIBUTION_VECTOR_PATH, SUBATHON_PLAN_VECTOR_PATH})
            prefixes_.append(read(path)["manifest"].toObject());
        fixture_ = read(SUBATHON_BUNDLE_VECTOR_PATH);
        manifest_ = fixture_["manifest"].toObject();
        scenarios_ = fixture_["scenarios"].toObject();
        queries_ = fixture_["queries"].toObject();
        QCOMPARE(fixture_["format"].toString(),
                 QString("sqlite-application-bundle-v1-independent-vectors"));
        QCOMPARE(manifest_["version"].toInt(), 5);
        QCOMPARE(manifest_["id"].toString(), QString("atomic-application-bundles-v1"));
        QCOMPARE(manifest_["catalog_format"].toInt(), 1);
        QCOMPARE(manifest_["store_identity"].toString(), QString("subathon.sqlite-store"));
        QCOMPARE(manifest_["supported_starting_versions"].toArray(),
                 QJsonArray({0, 1, 2, 3, 4, 5}));
        QCOMPARE(manifest_["sql"].toArray().size(), 14);
        QCOMPARE(manifest_["expected_schema"].toArray().size(), 66);
        const QList<int> counts{7, 13, 20, 30}, bytes{2000, 10535, 18126, 29015};
        for (qsizetype index = 0; index < prefixes_.size(); ++index) {
            const auto prefix = prefixes_[index];
            QCOMPARE(prefix["expected_schema"].toArray().size(), counts[index]);
            const auto payload = independent_frame(prefix);
            QCOMPARE(payload.size(), bytes[index]);
            QCOMPARE(payload, literal_hex(prefix["payload_hex"].toString()));
            QCOMPARE(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex(),
                     prefix["sha256_hex"].toString().toLatin1());
            for (const auto &object : prefix["expected_schema"].toArray())
                QVERIFY(manifest_["expected_schema"].toArray().contains(object));
        }
        const auto payload = independent_frame(manifest_);
        QCOMPARE(payload, literal_hex(manifest_["payload_hex"].toString()));
        QCOMPARE(payload.size(), 62465);
        QCOMPARE(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex(),
                 QByteArray("c9ba98fa4fe2a42cc6ed9776baa011fc2b93cf879e7374b53e9a6128f7d7afbb"));
        QCOMPARE(fixture_["uint64_vectors"].toArray().size(), 6);
        QCOMPARE(fixture_["constraint_refusals"].toArray().size(), 28);
        QCOMPARE(fixture_["runtime_only_admissions"].toArray().size(), 10);
    }

    void bundleSchemaPrefixMatchesLiteralProjection() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("schema.sqlite"), foundation_limits()),
                 StorageError::none);
        SqlRows actual, history, columns, empty, runtime;
        const auto observed = run(writer, [&](auto context) {
            auto session = context.begin();
            create_literal_schema(session);
            actual = session.execute(
                {"SELECT type,name,tbl_name,sql,typeof(sql) FROM sqlite_schema ORDER BY type,name",
                 {}});
            history = session.execute(
                {"SELECT version,migration_id,typeof(content_sha256),hex(content_sha256) FROM "
                 "subathon_migration_history ORDER BY version",
                 {}});
            for (const auto &entry : fixture_["table_columns"].toArray()) {
                const auto table = entry.toObject()["table"].toString();
                const auto count =
                    session.execute({"SELECT count(*) FROM pragma_table_info(?)", {table}});
                require(count.size() == 1 && count[0].size() == 1,
                        "Actual table column count missing");
                columns.append(count[0]);
                const auto rows = session.execute({"SELECT count(*) FROM " + quoted(table), {}});
                require(rows.size() == 1 && rows[0].size() == 1, "Actual table row count missing");
                empty.append(rows[0]);
            }
            runtime = session.execute({"SELECT sqlite_version()", {}});
            session.commit();
            return QVariant(66);
        });
        QVERIFY2(committed(observed), qPrintable(observed.problem));
        QCOMPARE(actual.size(), 66);
        const auto expected = manifest_["expected_schema"].toArray();
        qsizetype new_null_indexes = 0;
        for (qsizetype index = 0; index < actual.size(); ++index) {
            const auto object = expected[index].toObject();
            QCOMPARE(actual[index].size(), 5);
            QCOMPARE(actual[index][0].toString(), object["type"].toString());
            QCOMPARE(actual[index][1].toString(), object["name"].toString());
            QCOMPARE(actual[index][2].toString(), object["table_name"].toString());
            if (object["sql"].isNull()) {
                QVERIFY(actual[index][3].isNull());
                QCOMPARE(actual[index][4].toString(), QString("null"));
                if (object["name"].toString().startsWith("sqlite_autoindex_subathon_app_"))
                    ++new_null_indexes;
            } else {
                QCOMPARE(actual[index][3].toString(), object["sql"].toString());
                QCOMPARE(actual[index][4].toString(), QString("text"));
            }
        }
        QCOMPARE(new_null_indexes, 22);
        QCOMPARE(columns.size(), 14);
        for (qsizetype index = 0; index < columns.size(); ++index) {
            QCOMPARE(columns[index][0].toLongLong(),
                     fixture_["table_columns"].toArray()[index].toObject()["columns"].toInt());
            QCOMPARE(empty[index], SqlRow({qint64(0)}));
        }
        QCOMPARE(history.size(), 5);
        for (qsizetype index = 0; index < history.size(); ++index) {
            const auto prefix = index < 4 ? prefixes_[index] : manifest_;
            QCOMPARE(history[index].size(), 4);
            QCOMPARE(history[index][0].toLongLong(), index + 1);
            QCOMPARE(history[index][1].toString(), prefix["id"].toString());
            QCOMPARE(history[index][2].toString(), QString("blob"));
            QCOMPARE(history[index][3].toString(), prefix["sha256_hex"].toString().toUpper());
        }
        QCOMPARE(runtime.size(), 1);
        QCOMPARE(runtime[0].size(), 1);
        qInfo().noquote() << "Selected Qt" << QT_VERSION_STR << "SQLite"
                          << runtime[0][0].toString();
    }

    void typedBundlePayloadsAndHistoricalProvenanceRemainExact() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SqliteWriter writer;
        QCOMPARE(writer.open(directory.filePath("typed.sqlite"), foundation_limits()),
                 StorageError::none);
        const auto initial = bootstrap(writer);
        QVERIFY2(committed(initial), qPrintable(initial.problem));
        const auto typed =
            stage_literal(writer, {"boundary_raw_only"}, queries_["typed"].toArray());
        QVERIFY2(committed(typed), qPrintable(typed.problem));
        SqlRows unsigned_vectors, foreign_keys;
        const auto observed = run(writer, [&](auto context) {
            auto session = context.begin();
            QList<QVariant> values;
            for (const auto &entry : fixture_["uint64_vectors"].toArray())
                values.append(literal_hex(entry.toObject()["hex"].toString()));
            unsigned_vectors = session.execute(
                {"SELECT typeof(id),length(id),hex(id) FROM (SELECT ? AS id UNION ALL SELECT ? "
                 "UNION ALL SELECT ? UNION ALL SELECT ? UNION ALL SELECT ? UNION ALL SELECT ?) "
                 "ORDER BY id",
                 values});
            foreign_keys = session.execute({"SELECT * FROM pragma_foreign_key_check()", {}});
            session.commit();
            return QVariant(1);
        });
        QVERIFY2(committed(observed), qPrintable(observed.problem));
        QCOMPARE(unsigned_vectors.size(), 6);
        const auto vectors = fixture_["uint64_vectors"].toArray();
        for (qsizetype index = 0; index < vectors.size(); ++index) {
            QVERIFY(vectors[index].toObject()["decimal"].isString());
            QCOMPARE(
                unsigned_vectors[index],
                SqlRow({"blob", qint64(8), vectors[index].toObject()["hex"].toString().toUpper()}));
        }
        QVERIFY(foreign_keys.isEmpty());
        qInfo() << "Raw boundary operations are INCOMPLETE. Sparse plan count3/headMAX is INVALID; "
                   "typedMAX fields/large Exact bytes are not healthy application/exhaustion proof";
    }

    void multipleOriginalBasesShareOneRequestAndHeads() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath("reversals.sqlite");
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path, foundation_limits()), StorageError::none);
            const auto initial = bootstrap(writer);
            QVERIFY2(committed(initial), qPrintable(initial.problem));
            auto original_checks = queries_["original"].toArray();
            original_checks.append(queries_["preservation"]);
            const auto original =
                stage_literal(writer, {"original_prepare", "original_finish"}, original_checks);
            QVERIFY2(committed(original), qPrintable(original.problem));
            for (const auto &name : {"reversal_one", "reversal_two", "score_reversal"}) {
                const auto reversal = stage_literal(writer, {QString::fromLatin1(name)});
                QVERIFY2(committed(reversal), qPrintable(reversal.problem));
            }
        }
        // Fresh controlled writer/observer connections prove actual disposable-file reopen.
        SqliteWriter reopened;
        QCOMPARE(reopened.open(path, foundation_limits()), StorageError::none);
        auto checks = queries_["reversals"].toArray();
        checks.append(queries_["preservation"]);
        const auto observed = stage_literal(reopened, {}, checks);
        QVERIFY2(committed(observed), qPrintable(observed.problem));
        Observer observer;
        QVERIFY(observer.open(path));
        QCOMPARE(observer.rows("SELECT hex(token) FROM subathon_app_head"),
                 SqlRows({{QString::fromLatin1(QByteArray("score-after").toHex().toUpper())}}));
        QCOMPARE(observer.rows("SELECT count(*) FROM subathon_app_effects WHERE "
                               "operation_ref IN (X'726566756E6431',X'726566756E6432')"),
                 SqlRows({{qint64(2)}}));
        QCOMPARE(observer.rows(
                     "SELECT count(*) FROM subathon_app_reversals WHERE original_component_kind=0"),
                 SqlRows({{qint64(4)}}));
        QVERIFY(observer.rows("SELECT * FROM pragma_foreign_key_check()").isEmpty());
        qInfo() << "One request has two bases: quarter25+50=-request75, full75+150=-request225; "
                   "actual per-basis positions1/2 and both last-head paths agree. Supplied math, "
                   "no policy engine";
    }

    void bundleConstraintsAndRuntimeOnlyAdmissionsAreExplicit() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath("constraints.sqlite");
        SqliteWriter writer;
        QCOMPARE(writer.open(path, foundation_limits()), StorageError::none);
        const auto initial = bootstrap(writer);
        QVERIFY2(committed(initial), qPrintable(initial.problem));
        const auto baseline =
            stage_literal(writer, {"original_prepare", "original_finish", "reversal_one",
                                   "reversal_two", "score_reversal"});
        QVERIFY2(committed(baseline), qPrintable(baseline.problem));
        Observer observer;
        QVERIFY(observer.open(path));
        const auto before = observer.snapshot();
        qsizetype refusals = 0;
        for (const auto &entry : fixture_["constraint_refusals"].toArray()) {
            bool marker_seen = false, caught = false;
            const auto fault = entry.toObject();
            const auto observed = run(writer, [&](auto context) {
                auto session = context.begin();
                (void)session.execute({"INSERT INTO subathon_retained_configurations VALUES "
                                       "(X'726F6C6C6261636B2D6D61726B6572',X'79')",
                                       {}});
                marker_seen = true;
                try {
                    (void)session.execute(literal_statement(fault["statement"]));
                } catch (const SqliteWorkFailure &error) {
                    require(error.error() == WorkError::sql_failure, fault["name"].toString());
                    require(observer.released(), "Actual rollback must release lock inside catch");
                    require(observer.snapshot() == before,
                            "Whole typed snapshot must match inside catch");
                    caught = true;
                    throw;
                }
                throw std::runtime_error("Required physical constraint refusal absent");
                return QVariant(0);
            });
            QVERIFY2(marker_seen && caught,
                     qPrintable(fault["name"].toString() + ": " + observed.problem));
            QVERIFY(observed.completed);
            QCOMPARE(observed.result.transaction, TransactionOutcome::rolled_back);
            QCOMPARE(observed.result.error, WorkError::sql_failure);
            QVERIFY(!observed.result.value);
            QCOMPARE(observer.snapshot(), before);
            ++refusals;
        }
        QCOMPARE(refusals, 28);
        qsizetype admissions = 0;
        for (const auto &entry : fixture_["runtime_only_admissions"].toArray()) {
            const auto admission = entry.toObject();
            bool admitted = false;
            const auto observed = run(writer, [&](auto context) {
                auto session = context.begin();
                execute_steps(session, admission["steps"].toArray());
                check_queries(session, admission["queries"].toArray());
                require(session.execute({"SELECT * FROM pragma_foreign_key_check()", {}}).isEmpty(),
                        "Runtime-only admission must still have valid SQL FKs");
                admitted = true;
                session.rollback();
                return QVariant(0);
            });
            QVERIFY2(admitted, qPrintable(admission["name"].toString() + ": " + observed.problem));
            QVERIFY(observed.completed);
            QCOMPARE(observed.result.transaction, TransactionOutcome::rolled_back);
            QCOMPARE(observed.result.error, WorkError::not_committed);
            QVERIFY(!observed.result.value);
            QCOMPARE(observer.snapshot(), before);
            ++admissions;
            qInfo().noquote() << "SQL admits" << admission["name"].toString()
                              << "; RUNTIME obligation:"
                              << admission["required_runtime_check"].toString();
        }
        QCOMPARE(admissions, 10);
    }

    void conditionalMidstageFaultRollsBackPreparedUnit() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath("midstage.sqlite");
        SqliteWriter writer;
        QCOMPARE(writer.open(path, foundation_limits()), StorageError::none);
        const auto initial = bootstrap(writer);
        QVERIFY2(committed(initial), qPrintable(initial.problem));
        Observer observer;
        QVERIFY(observer.open(path));
        check_rows(observer.rows(queries_["pending"].toObject()["sql"].toString()),
                   queries_["pending"].toObject());
        const auto clean_prior = observer.snapshot();
        const auto fault = fixture_["fault"].toObject();
        // The completed bootstrap leaves the controlled writer idle. Its SQL
        // contract rejects semicolons, so compound test-only trigger DDL belongs
        // to this independent fixture connection, never to a writer bypass.
        QVERIFY(observer.released());
        QVERIFY(observer.test_instrumentation_ddl(fault["create_trigger"].toString()));
        const QString trigger_catalog =
            "SELECT type,name,sql FROM main.sqlite_schema WHERE type='trigger' "
            "AND name='storage002_job_fault'";
        const SqlRows expected_trigger{
            {"trigger", "storage002_job_fault", fault["create_trigger"].toString()}};
        QCOMPARE(observer.rows(trigger_catalog), expected_trigger);
        SqlRows writer_trigger;
        const auto installed = run(writer, [&](auto context) {
            auto session = context.begin();
            writer_trigger = session.execute({trigger_catalog, {}});
            session.commit();
            return QVariant(1);
        });
        QVERIFY2(committed(installed), qPrintable(installed.problem));
        QCOMPARE(writer_trigger, expected_trigger);
        const auto with_trigger = observer.snapshot();
        bool prepared_seen = false, outside_pending = false, caught = false;
        const auto observed = run(writer, [&](auto context) {
            auto session = context.begin();
            execute_steps(session, scenarios_["original_prepare"].toArray());
            check_queries(session, QJsonArray{queries_["prepared"], queries_["preservation"]});
            prepared_seen = true;
            check_rows(observer.rows(queries_["pending"].toObject()["sql"].toString()),
                       queries_["pending"].toObject());
            outside_pending = true;
            try {
                (void)session.execute(literal_statement(fault["statement"]));
            } catch (const SqliteWorkFailure &error) {
                require(error.error() == WorkError::sql_failure,
                        "Real conditional job SQL failure");
                require(observer.released(),
                        "Actual midstage rollback inside catch while body lives");
                require(observer.snapshot() == with_trigger,
                        "Complete typed snapshot after rollback");
                caught = true;
                throw;
            }
            throw std::runtime_error("Required real conditional midstage trigger absent");
            return QVariant(0);
        });
        QVERIFY2(prepared_seen && outside_pending && caught, qPrintable(observed.problem));
        QVERIFY(observed.completed);
        QCOMPARE(observed.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(observed.result.error, WorkError::sql_failure);
        QVERIFY(!observed.result.value);
        QCOMPARE(observer.snapshot(), with_trigger);
        check_rows(observer.rows(queries_["pending"].toObject()["sql"].toString()),
                   queries_["pending"].toObject());
        QVERIFY(observer.released());
        QVERIFY(observer.test_instrumentation_ddl(fault["drop_trigger"].toString()));
        QVERIFY(observer.rows(trigger_catalog).isEmpty());
        writer_trigger.clear();
        const auto removed = run(writer, [&](auto context) {
            auto session = context.begin();
            writer_trigger = session.execute({trigger_catalog, {}});
            session.commit();
            return QVariant(1);
        });
        QVERIFY2(committed(removed), qPrintable(removed.problem));
        QVERIFY(writer_trigger.isEmpty());
        QCOMPARE(observer.snapshot(), clean_prior);
        const auto retry = run(writer, [&](auto context) {
            auto session = context.begin();
            execute_steps(session, scenarios_["original_prepare"].toArray());
            check_queries(session, QJsonArray{queries_["prepared"]});
            execute_steps(session, scenarios_["original_finish"].toArray());
            auto checks = queries_["original"].toArray();
            checks.append(queries_["preservation"]);
            check_queries(session, checks);
            session.commit();
            return QVariant(1);
        });
        QVERIFY2(committed(retry), qPrintable(retry.problem));
        QCOMPARE(observer.rows("SELECT hex(token) FROM subathon_app_head"),
                 SqlRows({{QString::fromLatin1(QByteArray("original-after").toHex().toUpper())}}));
        qInfo()
            << "RAW OLD-WRITER FOUNDATION: pending0 was committed before this unit; observed "
               "intent/effects/components/after-state and application1 with old head, outside "
               "reader still pending0; conditional job abort rolled back all; identical fault-off "
               "retry committed. No future bundle facade reach or product acceptance claimed";
    }
};
QTEST_GUILESS_MAIN(SqliteApplicationBundleEnvironment)
#include "sqlite_application_bundle_environment.moc"
