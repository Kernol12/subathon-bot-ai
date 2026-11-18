// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/application_service_fixture.hpp"
#include "support/sqlite_reference_authority_fixture.hpp"

#include <QCryptographicHash>
#include <QTemporaryDir>
#include <QTest>
#include <algorithm>
#include <subathon/server/sqlite_delivery_catalog.hpp>
#include <tuple>

namespace {
using namespace subathon::server;
namespace app = application_service_fixture;
namespace reference = reference_authority_fixture;
using reference::migrate;
using reference::require;
using reference::scenario;

constexpr auto history_query =
    "SELECT version,migration_id,content_sha256 FROM main.subathon_migration_history "
    "ORDER BY version";
constexpr auto schema_query = "SELECT type,name,tbl_name,sql FROM main.sqlite_schema "
                              "WHERE name NOT IN ('subathon_catalog','subathon_migration_history',"
                              "'sqlite_autoindex_subathon_migration_history_1') ORDER BY type,name";
constexpr auto jobs_query =
    "SELECT * FROM main.subathon_app_jobs ORDER BY operation_scope,operation_ref,job_ref";

MigrationManifest literal_catalog() {
    auto expected = reference::literal_prefix();
    expected.steps.append(reference::step10(expected));
    expected.supported_starting_versions.append(10);
    MigrationStep next;
    next.version = 11;
    next.id = "delivery-jobs-v1";
    next.sql = {
        "CREATE TABLE subathon_delivery_captures(operation_scope BLOB NOT NULL "
        "CHECK(typeof(operation_scope)='blob' AND length(operation_scope) BETWEEN 1 AND 4096), "
        "operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND "
        "length(operation_ref) BETWEEN 1 AND 4096), job_ref BLOB NOT NULL "
        "CHECK(typeof(job_ref)='blob' AND length(job_ref) BETWEEN 1 AND 4096), route_ref BLOB "
        "NOT NULL CHECK(typeof(route_ref)='blob' AND length(route_ref) BETWEEN 1 AND 4096), "
        "selection INTEGER NOT NULL CHECK(typeof(selection)='integer' AND selection IN (0,1)), "
        "sender_ref BLOB NOT NULL CHECK(typeof(sender_ref)='blob' AND length(sender_ref) BETWEEN "
        "1 AND 4096), action_ref BLOB NOT NULL CHECK(typeof(action_ref)='blob' AND "
        "length(action_ref) BETWEEN 1 AND 4096), action_version_ref BLOB NOT NULL "
        "CHECK(typeof(action_version_ref)='blob' AND length(action_version_ref) BETWEEN 1 AND "
        "4096), PRIMARY KEY(operation_scope,operation_ref,job_ref), FOREIGN "
        "KEY(operation_scope,operation_ref,job_ref) REFERENCES "
        "subathon_app_jobs(operation_scope,operation_ref,job_ref))",
        "CREATE TABLE subathon_delivery_events(operation_scope BLOB NOT NULL, operation_ref BLOB "
        "NOT NULL, job_ref BLOB NOT NULL, revision INTEGER NOT NULL "
        "CHECK(typeof(revision)='integer' AND revision>=1), state INTEGER NOT NULL "
        "CHECK(typeof(state)='integer' AND state BETWEEN 0 AND 4), attempt INTEGER CHECK(attempt "
        "IS NULL OR (typeof(attempt)='integer' AND attempt>=2 AND attempt<=revision)), "
        "evidence_ref BLOB CHECK(evidence_ref IS NULL OR (typeof(evidence_ref)='blob' AND "
        "length(evidence_ref) BETWEEN 1 AND 4096)), CHECK((state=0 AND attempt IS NULL) OR "
        "(state=1 AND attempt IS NOT NULL AND attempt=revision) OR (state BETWEEN 2 AND 4 AND "
        "attempt IS NOT NULL AND attempt<revision)), CHECK((state=1 OR revision=1)=(evidence_ref "
        "IS NULL)), CHECK(revision<>1 OR state=0), PRIMARY "
        "KEY(operation_scope,operation_ref,job_ref,revision), FOREIGN "
        "KEY(operation_scope,operation_ref,job_ref) REFERENCES "
        "subathon_delivery_captures(operation_scope,operation_ref,job_ref))"};
    next.expected_schema = expected.steps.back().expected_schema;
    next.expected_schema.append({"index", "sqlite_autoindex_subathon_delivery_captures_1",
                                 "subathon_delivery_captures", std::nullopt});
    next.expected_schema.append({"index", "sqlite_autoindex_subathon_delivery_events_1",
                                 "subathon_delivery_events", std::nullopt});
    next.expected_schema.append(
        {"table", "subathon_delivery_captures", "subathon_delivery_captures", next.sql[0]});
    next.expected_schema.append(
        {"table", "subathon_delivery_events", "subathon_delivery_events", next.sql[1]});
    std::sort(next.expected_schema.begin(), next.expected_schema.end(),
              [](const auto &left, const auto &right) {
                  return std::tie(left.type, left.name) < std::tie(right.type, right.name);
              });
    expected.steps.append(std::move(next));
    expected.supported_starting_versions.append(11);
    return expected;
}

QByteArray expected_digest(const MigrationStep &step) {
    // Frame independent literal data in its sorted schema order.
    QByteArray payload;
    const auto frame = [&payload](const QByteArray &field) {
        payload += QByteArray::number(field.size());
        payload += ':';
        payload += field;
    };
    frame("migration-catalog-v1");
    frame(QByteArray::number(step.version));
    frame(step.id.toUtf8());
    frame(QByteArray::number(step.sql.size()));
    for (const auto &sql : step.sql)
        frame(sql.toUtf8());
    frame(QByteArray::number(step.expected_schema.size()));
    for (const auto &object : step.expected_schema) {
        frame(object.type.toUtf8());
        frame(object.name.toUtf8());
        frame(object.table_name.toUtf8());
        frame(object.sql ? "1" : "0");
        if (object.sql)
            frame(object.sql->toUtf8());
    }
    return QCryptographicHash::hash(payload, QCryptographicHash::Sha256);
}

SqlRows expected_history(const MigrationManifest &catalog) {
    SqlRows rows;
    for (const auto &step : catalog.steps)
        rows.append(SqlRow{qint64(step.version), step.id, expected_digest(step)});
    return rows;
}

QList<CatalogObject> stored_schema(app::Observer &observer) {
    QList<CatalogObject> objects;
    for (const auto &row : observer.rows(schema_query)) {
        require(row.size() == 4, "Exact schema projection columns");
        objects.append(
            {row[0].toString(), row[1].toString(), row[2].toString(),
             row[3].isNull() ? std::nullopt : std::optional<QString>{row[3].toString()}});
    }
    return objects;
}

void check_migration(const MigrationResult &result, CatalogState state, qint32 version,
                     qint32 target, qsizetype applied) {
    require(result.reason == MigrationReason::none &&
                result.work.transaction == TransactionOutcome::committed &&
                result.work.error == WorkError::none && result.work.value && result.report &&
                result.report->state == state && result.report->version == version &&
                result.report->target_version == target && result.report->applied == applied,
            "Expected acknowledged migration work and exact catalog report");
}

void check_catalog(app::Observer &observer, const MigrationManifest &expected) {
    require(stored_schema(observer) == expected.steps.back().expected_schema,
            "Actual SQLite schema matches the complete literal projection");
    require(observer.rows(history_query) == expected_history(expected),
            "Actual history matches the independently framed literal digests");
}

void check_empty_delivery(app::Observer &observer) {
    require(observer.rows("SELECT * FROM main.subathon_delivery_captures").isEmpty() &&
                observer.rows("SELECT * FROM main.subathon_delivery_events").isEmpty(),
            "Additive migration does not backfill delivery data");
}
} // namespace

class SqliteDeliveryCatalogTests final : public QObject {
    Q_OBJECT
  private slots:
    void compiledCatalogRetainsPrefixAndAddsExactDeliveryStep() {
        scenario([] {
            const auto expected = literal_catalog();
            const auto actual = delivery_jobs_repository_manifest();
            require(actual.catalog_format == expected.catalog_format &&
                        actual.store_identity == expected.store_identity &&
                        actual.supported_starting_versions ==
                            expected.supported_starting_versions &&
                        actual.steps.size() == 11,
                    "Preserve catalog identity and all starting versions through 11");
            for (qsizetype i = 0; i < actual.steps.size(); ++i) {
                const auto &actual_step = actual.steps[i];
                const auto &literal = expected.steps[i];
                require(actual_step.version == literal.version && actual_step.id == literal.id &&
                            actual_step.sql == literal.sql &&
                            actual_step.expected_schema == literal.expected_schema &&
                            migration_content_sha256(actual_step) == expected_digest(literal),
                        "Exact statements, schema and history digest for every catalog step");
            }
        });
    }

    void freshCatalogAndPopulatedUpgrade() {
        scenario([] {
            QTemporaryDir dir;
            require(dir.isValid(), "Disposable catalog stores");
            const auto full_catalog = delivery_jobs_repository_manifest();
            const auto literal = literal_catalog();
            {
                const auto path = dir.filePath("fresh.sqlite");
                SqliteWriter writer;
                require(writer.open(path, app::limits()) == StorageError::none, "Open fresh store");
                check_migration(migrate(writer, full_catalog), CatalogState::current, 11, 11, 11);
                app::Observer observer(path);
                check_catalog(observer, literal);
                check_empty_delivery(observer);
            }

            const auto path = dir.filePath("upgrade.sqlite");
            SqlRows legacy_jobs;
            {
                SqliteWriter writer;
                require(writer.open(path, app::limits()) == StorageError::none,
                        "Open store for explicit upgrade");
                const auto prefix = reference_authority_repository_manifest();
                check_migration(migrate(writer, prefix), CatalogState::current, 10, 10, 10);
                const app::Data data;
                (void)app::run(writer, [&](auto context) -> QVariant {
                    auto session = context.begin();
                    data.retain_sources(session);
                    data.populate_case(session, "add10");
                    auto bundle = app::prepared(data.case_data("add10")["expected_record"]);
                    require(SqliteApplicationBundleStore(session).stage(bundle).disposition ==
                                SqliteBundleStageDisposition::staged,
                            "Stage a representative legacy bundle at version 10");
                    session.commit();
                    return 1;
                });

                app::Observer observer(path);
                legacy_jobs = observer.rows(jobs_query);
                require(!legacy_jobs.empty(), "Legacy data exists before migration");
                const auto old_schema = stored_schema(observer);
                const auto old_history = observer.rows(history_query);
                require(old_schema == literal.steps[9].expected_schema &&
                            old_history == expected_history(literal).mid(0, 10),
                        "Setup created exact version 10 schema and history");
                check_migration(migrate(writer, full_catalog, MigrationMode::inspect),
                                CatalogState::upgrade_required, 10, 11, 0);
                require(stored_schema(observer) == old_schema &&
                            observer.rows(history_query) == old_history &&
                            observer.rows(jobs_query) == legacy_jobs,
                        "Inspection never changes schema, history or legacy rows");
                check_migration(migrate(writer, full_catalog), CatalogState::current, 11, 11, 1);
            }

            SqliteWriter writer;
            require(writer.open(path, app::limits()) == StorageError::none,
                    "Reopen upgraded store");
            app::Observer observer(path);
            check_catalog(observer, literal);
            require(observer.rows(jobs_query) == legacy_jobs, "Legacy job bytes survive upgrade");
            check_empty_delivery(observer);
            check_migration(migrate(writer, full_catalog), CatalogState::current, 11, 11, 0);
            check_migration(migrate(writer, full_catalog, MigrationMode::inspect),
                            CatalogState::current, 11, 11, 0);
            check_catalog(observer, literal);
            require(observer.rows(jobs_query) == legacy_jobs, "Re-advance preserves legacy jobs");
            check_empty_delivery(observer);
        });
    }
};

QTEST_GUILESS_MAIN(SqliteDeliveryCatalogTests)
#include "sqlite_delivery_catalog_tests.moc"
