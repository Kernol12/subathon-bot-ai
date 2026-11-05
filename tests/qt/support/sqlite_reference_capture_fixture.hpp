// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Literal data and accepted migration/writer primitives only; no new capture symbol is called.
#include "sqlite_original_claim_fixture.hpp"
#include <subathon/server/sqlite_migrations.hpp>

namespace reference_capture_fixture {
using namespace subathon::server;
using original_claim_fixture::check_rows;
using original_claim_fixture::execute;
using original_claim_fixture::limits;
using original_claim_fixture::load;
using original_claim_fixture::Observer;
using original_claim_fixture::require;
using original_claim_fixture::run;
using original_claim_fixture::statement;

inline MigrationStep literal_step(const QJsonObject &input) {
    MigrationStep result;
    result.version = input["version"].toInt();
    result.id = input["id"].toString();
    for (const auto &sql : input["sql"].toArray())
        result.sql.append(sql.toString());
    for (const auto &value : input["expected_schema"].toArray()) {
        const auto o = value.toObject();
        result.expected_schema.append(
            {o["type"].toString(), o["name"].toString(), o["table_name"].toString(),
             o["sql"].isNull() ? std::nullopt : std::optional{o["sql"].toString()}});
    }
    return result;
}
// Raw result helper also permits observing real rollback rather than inventing committed success.
inline SqliteWorkResult outcome(SqliteWriter &writer, SqliteWriter::Work body) {
    struct State {
        bool done{false};
        SqliteWorkResult result;
    };
    const auto state = std::make_shared<State>();
    require(writer.enqueue_work(std::move(body),
                                [state](auto result) {
                                    state->result = std::move(result);
                                    state->done = true;
                                }) == StorageError::none,
            "Old writer admits diagnostic unit");
    require(QTest::qWaitFor([state] { return state->done; }, 10000),
            "Old writer completes diagnostic unit");
    return std::move(state->result);
}
struct Data {
    QJsonObject frozen{load(SUBATHON_REFERENCE_CAPTURE_VECTOR_PATH)};
    QList<QJsonObject> prefixes;
    Data() {
        for (const auto *path : {SUBATHON_RETAINED_VECTOR_PATH, SUBATHON_RECEIPT_VECTOR_PATH,
                                 SUBATHON_CONTRIBUTION_VECTOR_PATH, SUBATHON_PLAN_VECTOR_PATH,
                                 SUBATHON_BUNDLE_VECTOR_PATH, SUBATHON_HOLD_VECTOR_PATH})
            prefixes.append(load(path)["manifest"].toObject());
    }
    MigrationManifest manifest() const {
        MigrationManifest result;
        result.supported_starting_versions = {0, 1, 2, 3, 4, 5, 6, 7};
        for (const auto &p : prefixes)
            result.steps.append(literal_step(p));
        result.steps.append(literal_step(frozen["manifest"].toObject()));
        return result;
    }
    void schema(SqliteWriter &writer) const {
        struct State {
            bool done{false};
            MigrationResult result;
        };
        const auto state = std::make_shared<State>();
        const auto admission = SqliteMigrationRunner::enqueue(
            writer, {MigrationMode::advance, manifest(), {}, {}}, [state](auto result) {
                state->result = std::move(result);
                state->done = true;
            });
        require(admission.error == StorageError::none && admission.reason == MigrationReason::none,
                "Accepted OLD migration runner admits independent literal seven-step manifest");
        require(QTest::qWaitFor([state] { return state->done; }, 10000),
                "Old migration runner completes");
        const auto &r = state->result;
        require(r.reason == MigrationReason::none && r.work.error == WorkError::none &&
                    r.work.transaction == TransactionOutcome::committed && r.work.value &&
                    r.report && r.report->state == CatalogState::current &&
                    r.report->version == 7 && r.report->target_version == 7 &&
                    r.report->applied == 7,
                "Fresh disposable schema reaches actual acknowledged normal step7 commit");
    }
    QJsonObject capture(const char *name) const {
        const auto value = frozen["captures"].toObject()[name];
        require(value.isObject(), "Named independent capture fixture exists");
        return value.toObject();
    }
    void intake(const SqliteWorkSession &s, const char *name) const {
        execute(s, capture(name)["intake"].toArray());
    }
    void stage_capture(const SqliteWorkSession &s, const char *name) const {
        execute(s, capture(name)["capture"].toArray());
    }
    void index(const SqliteWorkSession &s, const char *name) const {
        (void)s.execute(statement(capture(name)["index"]));
    }
    void verify(Observer &o, const char *name) const {
        for (const auto &q : capture(name)["queries"].toArray()) {
            const auto query = q.toObject();
            check_rows(o.rows(query["sql"].toString()), query);
        }
    }
};
} // namespace reference_capture_fixture
