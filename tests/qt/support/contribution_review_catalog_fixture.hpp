// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_financial_refunds_fixture.hpp"

#include <QCryptographicHash>
#include <subathon/server/contribution_review.hpp>
#include <subathon/server/sqlite_milestone_target_edits.hpp>
#include <tuple>

namespace contribution_review_fixture {
using namespace subathon::server;
namespace finance = financial_refund_fixture;
namespace core = subathon::core;
using qt_test_support::require;

// Physical proposal. Decision kind 1 accepts the ordinary dimension (including
// accept-and-apply); 2 records terminal no-claim rejection; 3 audits other verbs.
// Status tags follow the frozen outcome order. SelectorUnavailable is tag 8
// physically so it can share the mandatory protected-NULL checks with Denied.
inline const QList<QString> v19_sql{
    "CREATE TABLE subathon_review_work("
    "installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND "
    "length(installation_ref)>0), "
    "work_scope BLOB NOT NULL CHECK(typeof(work_scope)='blob' AND length(work_scope)>0), "
    "work_ref BLOB NOT NULL CHECK(typeof(work_ref)='blob' AND length(work_ref)>0), "
    "manifest BLOB NOT NULL CHECK(typeof(manifest)='blob' AND length(manifest) BETWEEN 1 AND "
    "987136 "
    "AND substr(manifest,1,4)=X'53525731'), "
    "entry_count INTEGER NOT NULL CHECK(typeof(entry_count)='integer' AND entry_count BETWEEN 1 "
    "AND 16), "
    "next_ordinal INTEGER NOT NULL CHECK(typeof(next_ordinal)='integer' AND next_ordinal BETWEEN 1 "
    "AND entry_count+1), "
    "PRIMARY KEY(installation_ref,work_scope,work_ref))",
    "CREATE TABLE subathon_review_entries("
    "installation_ref BLOB NOT NULL, work_scope BLOB NOT NULL, work_ref BLOB NOT NULL, "
    "ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal BETWEEN 1 AND 16), "
    "status INTEGER NOT NULL CHECK(typeof(status)='integer' AND status BETWEEN 0 AND 8), "
    "contribution_id BLOB REFERENCES subathon_contributions(contribution_id), "
    "receipt_id BLOB REFERENCES subathon_receipts(receipt_id), "
    "decision_kind INTEGER CHECK(decision_kind IS NULL OR (typeof(decision_kind)='integer' AND "
    "decision_kind BETWEEN 1 AND 3)), "
    "outcome BLOB CHECK(outcome IS NULL OR (typeof(outcome)='blob' AND length(outcome) BETWEEN 1 "
    "AND 61440)), "
    "application_scope BLOB, application_ref BLOB, "
    "PRIMARY KEY(installation_ref,work_scope,work_ref,ordinal), "
    "UNIQUE(installation_ref,work_scope,work_ref,contribution_id), "
    "UNIQUE(application_scope,application_ref), "
    "FOREIGN KEY(installation_ref,work_scope,work_ref) REFERENCES "
    "subathon_review_work(installation_ref,work_scope,work_ref), "
    "FOREIGN KEY(application_scope,application_ref) REFERENCES "
    "subathon_app_operations(operation_scope,operation_ref), "
    "CHECK((contribution_id IS NULL AND receipt_id IS NULL) OR "
    "(typeof(contribution_id)='blob' AND length(contribution_id)=8 AND typeof(receipt_id)='blob' "
    "AND "
    "length(receipt_id)=8 AND receipt_id<>X'0000000000000000')), "
    "CHECK((application_scope IS NULL AND application_ref IS NULL AND status NOT IN (1,3)) OR "
    "(typeof(application_scope)='blob' AND length(application_scope)>0 AND "
    "typeof(application_ref)='blob' AND "
    "length(application_ref)>0 AND status IN (1,3) AND contribution_id IS NOT NULL)), "
    "CHECK((status IN (7,8) AND contribution_id IS NULL AND decision_kind IS NULL AND outcome IS "
    "NULL) OR "
    "(status NOT IN (7,8) AND contribution_id IS NOT NULL AND outcome IS NOT NULL)), "
    "CHECK((status BETWEEN 5 AND 8 AND decision_kind IS NULL) OR "
    "(decision_kind IS NOT NULL AND ((decision_kind=1 AND status IN (0,1)) OR "
    "(decision_kind=2 AND status=2) OR (decision_kind=3 AND status IN (1,2,3,4))))))",
    "CREATE INDEX subathon_review_entries_cid ON subathon_review_entries(contribution_id)",
    "CREATE UNIQUE INDEX subathon_review_entries_accept_once ON "
    "subathon_review_entries(contribution_id) WHERE decision_kind=1",
    "CREATE UNIQUE INDEX subathon_review_entries_reject_once ON "
    "subathon_review_entries(contribution_id) WHERE decision_kind=2",
    "CREATE TABLE subathon_review_terminal_requests("
    "operation_scope BLOB NOT NULL, operation_ref BLOB NOT NULL, "
    "plan_ref BLOB NOT NULL, plan_revision BLOB NOT NULL, "
    "request_ordinal INTEGER NOT NULL CHECK(typeof(request_ordinal)='integer' AND "
    "request_ordinal>0), "
    "disposition INTEGER NOT NULL CHECK(typeof(disposition)='integer' AND disposition IN (1,2)), "
    "PRIMARY KEY(operation_scope,operation_ref,request_ordinal), "
    "FOREIGN KEY(operation_scope,operation_ref,plan_ref,plan_revision) "
    "REFERENCES subathon_app_operations(operation_scope,operation_ref,plan_ref,plan_revision), "
    "FOREIGN KEY(plan_ref,plan_revision,request_ordinal) REFERENCES "
    "subathon_plan_requests(plan_ref,revision,ordinal))",
};

inline const QList<CatalogObject> v19_projection{
    {"table", "subathon_review_work", "subathon_review_work", v19_sql[0]},
    {"table", "subathon_review_entries", "subathon_review_entries", v19_sql[1]},
    {"index", "subathon_review_entries_cid", "subathon_review_entries", v19_sql[2]},
    {"index", "subathon_review_entries_accept_once", "subathon_review_entries", v19_sql[3]},
    {"index", "subathon_review_entries_reject_once", "subathon_review_entries", v19_sql[4]},
    {"table", "subathon_review_terminal_requests", "subathon_review_terminal_requests", v19_sql[5]},
    {"index", "sqlite_autoindex_subathon_review_work_1", "subathon_review_work", std::nullopt},
    {"index", "sqlite_autoindex_subathon_review_entries_1", "subathon_review_entries",
     std::nullopt},
    {"index", "sqlite_autoindex_subathon_review_entries_2", "subathon_review_entries",
     std::nullopt},
    {"index", "sqlite_autoindex_subathon_review_entries_3", "subathon_review_entries",
     std::nullopt},
    {"index", "sqlite_autoindex_subathon_review_terminal_requests_1",
     "subathon_review_terminal_requests", std::nullopt},
};

inline auto schema_order(const CatalogObject &a, const CatalogObject &b) {
    return std::tie(a.type, a.name) < std::tie(b.type, b.name);
}

inline MigrationManifest literal_v19() {
    auto manifest = milestone_target_edits_repository_manifest();
    require(manifest.steps.size() == 18 && manifest.steps.back().version == 18 &&
                manifest.steps.back().id == "milestone-target-edits-v1",
            "Protected v18 producer supplies the old manifest");
    MigrationStep next{19, "contribution-review-v1", v19_sql,
                       manifest.steps.back().expected_schema};
    next.expected_schema.append(v19_projection);
    std::ranges::sort(next.expected_schema, schema_order);
    manifest.steps.append(std::move(next));
    manifest.supported_starting_versions.append(19);
    return manifest;
}

inline qsizetype tally(const MigrationManifest &manifest) {
    qsizetype bytes = 0;
    for (const auto &step : manifest.steps) {
        for (const auto &sql : step.sql)
            bytes += sql.toUtf8().size();
        for (const auto &object : step.expected_schema) {
            bytes += object.type.toUtf8().size() + object.name.toUtf8().size() +
                     object.table_name.toUtf8().size();
            if (object.sql)
                bytes += object.sql->toUtf8().size();
        }
    }
    return bytes;
}

inline QByteArray digest(const MigrationStep &step) {
    QCryptographicHash hash(QCryptographicHash::Sha256);
    const auto frame = [&](const QByteArray &field) {
        hash.addData(QByteArray::number(field.size()) + ':');
        hash.addData(field);
    };
    frame("migration-catalog-v1");
    frame(QByteArray::number(step.version));
    frame(step.id.toUtf8());
    frame(QByteArray::number(step.sql.size()));
    for (const auto &sql : step.sql)
        frame(sql.toUtf8());
    auto schema = step.expected_schema;
    std::ranges::sort(schema, schema_order);
    frame(QByteArray::number(schema.size()));
    for (const auto &object : schema) {
        frame(object.type.toUtf8());
        frame(object.name.toUtf8());
        frame(object.table_name.toUtf8());
        frame(object.sql ? "1" : "0");
        if (object.sql)
            frame(object.sql->toUtf8());
    }
    return hash.result();
}

inline void expect_schema(const SqlRows &physical, const MigrationStep &step) {
    require(physical.size() == step.expected_schema.size(), "Exact projected object count");
    for (qsizetype i = 0; i < physical.size(); ++i) {
        const auto &row = physical[i];
        const auto &object = step.expected_schema[i];
        require(row.size() == 4 && row[0] == object.type && row[1] == object.name &&
                    row[2] == object.table_name &&
                    (object.sql ? row[3] == *object.sql : row[3].isNull()),
                "Exact projected type, name, table and SQL bytes or SQL NULL");
    }
}

inline SqlRows expected_history(const MigrationManifest &manifest) {
    SqlRows rows;
    for (const auto &step : manifest.steps)
        rows.append({step.version, step.id.toUtf8(), digest(step), QString("integer"),
                     QString("text"), QString("blob")});
    return rows;
}

inline constexpr auto schema_query =
    "SELECT type,name,tbl_name,sql FROM main.sqlite_schema "
    "ORDER BY type COLLATE BINARY,name COLLATE BINARY,tbl_name COLLATE BINARY";
inline constexpr auto history_query =
    "SELECT version,CAST(migration_id AS BLOB),content_sha256,typeof(version),"
    "typeof(migration_id),typeof(content_sha256) FROM main.subathon_migration_history ORDER BY "
    "version";

inline constexpr qsizetype v18_manifest_bytes = 1311210;
inline constexpr qsizetype v19_manifest_bytes = 1442135;
inline constexpr auto v19_digest_hex =
    "407229e831e3f1368f2b5cba415c56fe0cc1b45a30ddfb496c39ec72504e8e87";

inline MigrationRequest request(MigrationMode mode, MigrationManifest manifest) {
    const auto measured = tally(manifest);
    constexpr qsizetype ceiling = 2 * 1024 * 1024;
    const auto expected = manifest.steps.size() == 19 ? v19_manifest_bytes : v18_manifest_bytes;
    require(measured == expected && measured < ceiling,
            "Bound cumulative migration fixture charge matches the independent source tally");
    MigrationLimits limits;
    limits.manifest_utf8_bytes = measured + 1;
    return {mode, std::move(manifest), limits, {}};
}

inline MigrationResult migrate(SqliteWriter &writer, MigrationRequest request) {
    return finance::completed<MigrationResult>([&](auto done) {
        const auto admitted =
            SqliteMigrationRunner::enqueue(writer, std::move(request), std::move(done));
        require(admitted.reason == MigrationReason::none,
                "Catalog preflight accepted the explicit budget");
        return admitted.error;
    });
}

inline void report(const MigrationResult &result, CatalogState state, int version, int target,
                   qsizetype applied) {
    finance::normal(result.work);
    require(result.reason == MigrationReason::none && result.report &&
                result.report->state == state && result.report->version == version &&
                result.report->target_version == target && result.report->applied == applied,
            "Real migration report matches the selected prefix and number of new steps");
}

inline StorageLimits profile() {
    StorageLimits limits;
    limits.statements_per_batch = 768;
    limits.statement_utf8_bytes = 65536;
    limits.bound_value_bytes = 9 * 1024 * 1024;
    limits.result_rows = 768;
    limits.result_value_bytes = 16 * 1024 * 1024;
    return limits;
}

class CatalogFile {
  public:
    explicit CatalogFile(StorageLimits limits = finance::limits()) {
        require(directory_.isValid(), "Disposable SQLite directory exists");
        open(limits);
        auto shipped = milestone_target_edits_repository_manifest();
        require(tally(shipped) == v18_manifest_bytes,
                "Protected v18 cumulative projection stays exact");
        report(migrate(writer(), request(MigrationMode::advance, std::move(shipped))),
               CatalogState::current, 18, 18, 18);
    }

    void advance_literal() {
        report(migrate(writer(), request(MigrationMode::advance, literal_v19())),
               CatalogState::current, 19, 19, 1);
    }

    void reopen(StorageLimits limits = profile()) {
        auto stopped = std::make_shared<bool>(false);
        require(writer().shutdown([stopped] { *stopped = true; }) == StorageError::none &&
                    QTest::qWaitFor([stopped] { return *stopped; }, 10000),
                "Writer closes before the next single writer opens");
        require(reader_.close() == StorageError::none, "Committed observer closes");
        writer_.reset();
        open(limits);
    }

    SqliteWriter &writer() { return *writer_; }

    SqlRows rows(const QString &sql, QList<QVariant> bindings = {}) {
        const auto result = reader_.read({sql, std::move(bindings)});
        require(result.error == StorageError::none, "Independent committed SQLite read succeeds");
        return result.rows;
    }

    template <class Body> void write(Body body) {
        auto failure = std::make_shared<std::exception_ptr>();
        const auto result = qt_test_support::run_work(
            writer(),
            [failure, body](auto context) -> QVariant {
                const auto session = context.begin();
                try {
                    body(session);
                    session.commit();
                } catch (...) {
                    *failure = std::current_exception();
                    throw;
                }
                return 1;
            },
            10000);
        if (*failure)
            std::rethrow_exception(*failure);
        finance::normal(result);
    }

  private:
    void open(StorageLimits limits) {
        writer_ = std::make_unique<SqliteWriter>();
        const auto path = directory_.filePath("contribution-review.sqlite");
        require(writer().open(path, limits) == StorageError::none, "Single real writer opens");
        require(reader_.open(path) == StorageError::none, "Read-only observer opens");
    }

    QTemporaryDir directory_;
    SqliteReader reader_;
    std::unique_ptr<SqliteWriter> writer_;
};
} // namespace contribution_review_fixture
