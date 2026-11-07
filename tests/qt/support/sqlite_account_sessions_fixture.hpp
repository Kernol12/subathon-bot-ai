// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sqlite_test_support.hpp"
// Independent literal schema/setup plus existing writer/migration only.
// No new account-session symbol is included or invoked by this shared foundation.
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <subathon/server/sqlite_migrations.hpp>
#include <utility>

namespace account_sessions_fixture {
using namespace subathon::server;
using qt_test_support::require;
template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QTest::qFail(error.what(), __FILE__, __LINE__);
    }
}
inline QJsonObject load(const QString &path) {
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "Literal fixture file exists");
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);
    require(error.error == QJsonParseError::NoError && document.isObject(), "Literal fixture JSON");
    return document.object();
}
inline QByteArray blob(std::string_view value) {
    return value.empty() ? QByteArray("", 0) : QByteArray(value.data(), qsizetype(value.size()));
}
inline StorageLimits limits() {
    StorageLimits result;
    result.statements_per_batch = 16384;
    result.statement_utf8_bytes = 4194304;
    result.bound_value_bytes = 33554432;
    result.result_rows = 65536;
    result.result_value_bytes = 33554432;
    return result;
}
// Always retains actual work classification. Callers assert expected failure explicitly.
inline SqliteWorkResult outcome(SqliteWriter &writer, SqliteWriter::Work body) {
    return qt_test_support::run_work(writer, std::move(body), 10000);
}
// A feature marker must escape the writer body into an ordinary assertion failure,
// rather than being mistaken for an intended work_exception/rollback scenario.
inline SqliteWorkResult run(SqliteWriter &writer, SqliteWriter::Work body) {
    const auto problem = std::make_shared<std::string>();
    const auto result = outcome(writer, [body = std::move(body), problem](auto context) {
        try {
            return body(context);
        } catch (const std::exception &error) {
            *problem = error.what();
            throw;
        }
    });
    if (!problem->empty())
        throw std::runtime_error(*problem);
    require(result.transaction == TransactionOutcome::committed &&
                result.error == WorkError::none && result.value.has_value(),
            "Actual acknowledged normal committed completion");
    return result;
}
struct Cleanup {
    SqliteWorkSession session;
    ~Cleanup() { (void)session.cleanup(); }
};
class Observer final {
  public:
    explicit Observer(const QString &path)
        : name_(QUuid::createUuid().toString()), db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
        require(db_.open(), "Isolated fixture observer opens");
    }
    ~Observer() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }
    SqlRows rows(const QString &sql) {
        QSqlQuery query(db_);
        require(query.exec(sql), "Isolated fixture query executes");
        SqlRows result;
        while (query.next()) {
            SqlRow row;
            for (int i = 0; i < query.record().count(); ++i)
                row.append(query.value(i));
            result.append(std::move(row));
        }
        require(!query.lastError().isValid(), "Isolated fixture fetch completes");
        return result;
    }

  private:
    QString name_;
    QSqlDatabase db_;
};
// Explicit isolated corruption fixture: only while production writer is idle.
// No product path disables constraints or gets this diagnostic connection.
inline void corrupt(const QString &path, const QStringList &updates) {
    Observer observer(path);
    (void)observer.rows("PRAGMA foreign_keys=OFF");
    (void)observer.rows("PRAGMA ignore_check_constraints=ON");
    (void)observer.rows("BEGIN IMMEDIATE");
    for (const auto &sql : updates)
        (void)observer.rows(sql);
    (void)observer.rows("COMMIT");
}
inline MigrationStep literal_step(const QJsonObject &input) {
    MigrationStep step;
    step.version = input["version"].toInt();
    step.id = input["id"].toString();
    for (const auto &sql : input["sql"].toArray())
        step.sql.append(sql.toString());
    for (const auto &entry : input["expected_schema"].toArray()) {
        const auto object = entry.toObject();
        step.expected_schema.append(
            {object["type"].toString(), object["name"].toString(), object["table_name"].toString(),
             object["sql"].isNull() ? std::nullopt : std::optional{object["sql"].toString()}});
    }
    return step;
}
struct Data {
    QJsonObject frozen{load(QString::fromUtf8(SUBATHON_ACCOUNT_SESSION_VECTOR_PATH))};
    QList<QJsonObject> prefixes;
    Data() {
        for (const auto &entry : frozen["prefixes"].toArray()) {
            const auto path = QString::fromUtf8(SUBATHON_FIXTURE_DIRECTORY) + "/" +
                              entry.toObject()["fixture"].toString();
            prefixes.append(load(path)["manifest"].toObject());
        }
    }
    MigrationManifest manifest(bool new_step = true) const {
        MigrationManifest result;
        result.supported_starting_versions = {0, 1, 2, 3, 4, 5, 6, 7};
        for (const auto &prefix : prefixes)
            result.steps.append(literal_step(prefix));
        if (new_step) {
            result.steps.append(literal_step(frozen["manifest"].toObject()));
            result.supported_starting_versions.append(8);
        }
        return result;
    }
    MigrationResult migrate(SqliteWriter &writer, bool new_step = true,
                            MigrationMode mode = MigrationMode::advance) const {
        struct State {
            bool done{false};
            MigrationResult result;
        };
        const auto state = std::make_shared<State>();
        const auto admission = SqliteMigrationRunner::enqueue(
            writer, {mode, manifest(new_step), {}, {}}, [state](auto result) {
                state->result = std::move(result);
                state->done = true;
            });
        require(admission.error == StorageError::none && admission.reason == MigrationReason::none,
                "Existing runner admits independently literal manifest");
        require(QTest::qWaitFor([state] { return state->done; }, 10000),
                "Existing migration completes");
        return state->result;
    }
    void schema(SqliteWriter &writer) const {
        const auto result = migrate(writer);
        require(result.reason == MigrationReason::none && result.report &&
                    result.report->version == 8 && result.report->target_version == 8 &&
                    result.report->state == CatalogState::current && result.report->applied == 8 &&
                    result.work.error == WorkError::none &&
                    result.work.transaction == TransactionOutcome::committed && result.work.value,
                "Literal disposable proposed step8 acknowledged, no new migration factory");
    }
    void populate(const SqliteWorkSession &session) const {
        for (const auto &sql : frozen["base_sql"].toArray())
            (void)session.execute({sql.toString(), {}});
    }
};
struct Database {
    QTemporaryDir directory;
    Data data;
    QString path;
    SqliteWriter writer;
    explicit Database(bool populate = true, StorageLimits budget = limits())
        : path(directory.filePath("sessions.sqlite")) {
        require(directory.isValid(), "Disposable database directory");
        require(writer.open(path, budget) == StorageError::none,
                "Existing writer opens disposable file");
        data.schema(writer);
        if (populate)
            (void)run(writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                data.populate(s);
                s.commit();
                return 1;
            });
    }
};
inline void healthy(Observer &observer) {
    require(observer.rows("PRAGMA foreign_key_check").isEmpty(), "Physical foreign keys valid");
    require(observer.rows("PRAGMA integrity_check") == SqlRows{{QString("ok")}},
            "Physical integrity valid");
}
inline void constraint_refusal(SqliteWriter &writer, const QString &sql) {
    bool reached = false;
    const auto result = outcome(writer, [&](auto context) -> QVariant {
        const auto s = context.begin();
        Cleanup cleanup{s};
        try {
            (void)s.execute({sql, {}});
        } catch (const SqliteWorkFailure &error) {
            reached = error.error() == WorkError::sql_failure;
            throw;
        }
        s.rollback();
        return 0;
    });
    require(reached && result.error == WorkError::sql_failure &&
                result.transaction == TransactionOutcome::rolled_back && !result.value,
            "Real reached constraint failure and known rollback");
}
} // namespace account_sessions_fixture
