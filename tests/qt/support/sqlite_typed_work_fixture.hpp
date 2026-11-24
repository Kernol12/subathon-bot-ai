// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_test_support.hpp"
#include <QJsonArray>
#include <QJsonObject>
#include <functional>
#include <optional>
#include <subathon/server/sqlite_migrations.hpp>

namespace qt_test_support::sqlite_fixture {
using namespace subathon::server;

template <class Failure, class Operation, class Error>
void expect_input_error(Operation &operation, Error error, const char *wrong, const char *missing) {
    try {
        operation();
    } catch (const Failure &failure) {
        require(failure.error() == error, wrong);
        return;
    }
    throw std::runtime_error(missing);
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

inline MigrationStep literal_step(const QJsonObject &o) {
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

template <class Errors> struct WorkRun {
    bool admitted{false}, done{false}, stub{false};
    std::optional<typename Errors::InputError> invalid;
    std::optional<typename Errors::StorageReason> storage;
    std::optional<WorkError> synchronous;
    QString unexpected;
    SqliteWorkResult result;

    QString problem() const {
        if (stub)
            return Errors::stub_message;
        if (!unexpected.isEmpty())
            return unexpected;
        return QString(Errors::problem_format)
            .arg(invalid ? int(*invalid) : -1)
            .arg(storage ? int(*storage) : -1)
            .arg(synchronous ? int(*synchronous) : -1)
            .arg(int(result.error));
    }
};

template <class Errors>
WorkRun<Errors> run_typed_work(SqliteWriter &writer,
                               std::function<QVariant(SqliteWorkContext)> body) {
    auto state = std::make_shared<WorkRun<Errors>>();
    state->result = qt_test_support::run_work(
        writer,
        [state, body = std::move(body)](auto work) {
            try {
                return body(work);
            } catch (const typename Errors::NotImplemented &) {
                state->stub = true;
                throw;
            } catch (const typename Errors::InputFailure &e) {
                state->invalid = e.error();
                throw;
            } catch (const typename Errors::StorageFailure &e) {
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
        5000);
    state->admitted = true;
    state->done = true;
    return *state;
}

// These callers share ownership of the completion state with the writer callback.
template <class Manifest>
bool install_with_owned_completion(const QString &path, StorageLimits limits, Manifest literals) {
    SqliteWriter writer;
    if (writer.open(path, limits) != StorageError::none)
        return false;
    MigrationRequest request;
    request.mode = MigrationMode::advance;
    request.manifest = literals();
    struct State {
        bool done = false;
        MigrationResult result;
    };
    auto state = std::make_shared<State>();
    const auto a = SqliteMigrationRunner::enqueue(writer, std::move(request), [state](auto r) {
        state->result = std::move(r);
        state->done = true;
    });
    return a.error == StorageError::none &&
           QTest::qWaitFor([state] { return state->done; }, 5000) &&
           state->result.reason == MigrationReason::none &&
           state->result.work.transaction == TransactionOutcome::committed;
}

} // namespace qt_test_support::sqlite_fixture
