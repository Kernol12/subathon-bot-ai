// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "milestone_group_decisions_fixture.hpp"
#include "sqlite_financial_refunds_fixture.hpp"

#include <subathon/server/sqlite_credential_catalog.hpp>

namespace credential_catalog_fixture {
using namespace subathon::server;
using qt_test_support::require;

inline const QString table_name = "subathon_provider_credentials";
inline const QString index_name = "sqlite_autoindex_subathon_provider_credentials_1";
inline const QString credential_ddl =
    "CREATE TABLE subathon_provider_credentials(scope BLOB PRIMARY KEY NOT NULL "
    "CHECK(typeof(scope)='blob'), record BLOB NOT NULL CHECK(typeof(record)='blob'))";
inline const QString marker = "provider-credentials-v1";

inline constexpr auto schema_sql =
    "SELECT type,name,tbl_name,sql FROM main.sqlite_schema "
    "ORDER BY type COLLATE BINARY,name COLLATE BINARY,tbl_name COLLATE BINARY,sql COLLATE BINARY";
inline constexpr auto history_sql =
    "SELECT version,CAST(migration_id AS BLOB),content_sha256,typeof(version),"
    "typeof(migration_id),typeof(content_sha256) FROM main.subathon_migration_history "
    "ORDER BY version";

inline MigrationManifest literal_v16() {
    // The prefix is the separately protected v15 factory. Only the new
    // ordered statement and projected delta come from the frozen examples.
    auto manifest = financial_refunds_repository_manifest();
    MigrationStep step{16, marker, {credential_ddl}, manifest.steps.back().expected_schema};
    step.expected_schema.append({"table", table_name, table_name, credential_ddl});
    step.expected_schema.append({"index", index_name, table_name, std::nullopt});
    std::ranges::sort(step.expected_schema, [](const auto &a, const auto &b) {
        return std::tie(a.type, a.name) < std::tie(b.type, b.name);
    });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(16);
    return manifest;
}

inline SqlRow history_row(const MigrationStep &step) {
    return {step.version,
            step.id.toUtf8(),
            milestone_group_decisions_fixture::literal_digest(step),
            QString("integer"),
            QString("text"),
            QString("blob")};
}

inline void expect_schema(const SqlRows &rows, const MigrationStep &step) {
    require(rows.size() == step.expected_schema.size(), "Exact SQLite schema object count");
    for (qsizetype i = 0; i < rows.size(); ++i) {
        const auto &row = rows[i];
        const auto &object = step.expected_schema[i];
        require(row.size() == 4 && row[0] == object.type && row[1] == object.name &&
                    row[2] == object.table_name &&
                    (object.sql ? row[3] == *object.sql : row[3].isNull()),
                "Exact SQLite schema object and stored SQL or NULL");
    }
}

inline MigrationResult migrate(SqliteWriter &writer, MigrationMode mode,
                               MigrationManifest manifest) {
    return financial_refund_fixture::completed<MigrationResult>([&](auto done) {
        return SqliteMigrationRunner::enqueue(writer, {mode, std::move(manifest), {}, {}},
                                              std::move(done))
            .error;
    });
}

inline void expect_report(const MigrationResult &result, CatalogState state, int version,
                          int target, qsizetype applied) {
    financial_refund_fixture::normal(result.work);
    require(result.reason == MigrationReason::none && result.report,
            "Migration publishes an acknowledged report");
    const auto &report = *result.report;
    require(
        report.state == state && report.version == version && report.target_version == target &&
            report.applied == applied,
        QString("Catalog report state/current/target/applied: %1/%2/%3/%4; expected %5/%6/%7/%8")
            .arg(int(report.state))
            .arg(report.version)
            .arg(report.target_version)
            .arg(report.applied)
            .arg(int(state))
            .arg(version)
            .arg(target)
            .arg(applied));
}

inline void expect_migration_refusal(const MigrationResult &result, MigrationReason reason) {
    require(result.reason == reason && result.work.transaction == TransactionOutcome::rolled_back &&
                !result.report && !result.work.value,
            "Migration refuses with the selected reason and an acknowledged rollback");
}

class CatalogFile final {
  public:
    explicit CatalogFile(MigrationManifest manifest = financial_refunds_repository_manifest()) {
        const auto head = manifest.steps.back().version;
        require(dir_.isValid(), "Disposable credential catalog directory");
        open();
        expect_report(migrate(*writer, MigrationMode::advance, std::move(manifest)),
                      CatalogState::current, head, head, head);
    }

    void advance_producer() {
        expect_report(
            migrate(*writer, MigrationMode::advance, provider_credentials_repository_manifest()),
            CatalogState::current, 16, 16, 1);
    }

    void reopen() {
        auto stopped = std::make_shared<bool>(false);
        require(writer->shutdown([stopped] { *stopped = true; }) == StorageError::none &&
                    QTest::qWaitFor([stopped] { return *stopped; }, 10000),
                "Writer acknowledges shutdown before reopening the same file");
        reader_.reset();
        writer.reset();
        open();
    }

    SqlRows rows(const QString &sql, QList<QVariant> bindings = {}) const {
        auto result = reader_->read({sql, std::move(bindings)});
        require(result.error == StorageError::none, "Independent committed query succeeds");
        return std::move(result.rows);
    }

    template <class Body> void write(Body body) {
        auto result = milestone_group_restart_fixture::work(*writer, [body](auto context) {
            const auto session = context.begin();
            body(session);
            session.commit();
            return 1;
        });
        financial_refund_fixture::normal(result);
    }

    std::unique_ptr<SqliteWriter> writer;

  private:
    void open() {
        const auto path = dir_.filePath("credential-catalog.sqlite");
        writer = std::make_unique<SqliteWriter>();
        require(writer->open(path, financial_refund_fixture::limits()) == StorageError::none,
                "Real credential catalog writer opens");
        reader_ = std::make_unique<SqliteReader>();
        require(reader_->open(path) == StorageError::none, "Independent committed reader opens");
    }

    QTemporaryDir dir_;
    std::unique_ptr<SqliteReader> reader_;
};

} // namespace credential_catalog_fixture
