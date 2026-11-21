// SPDX-License-Identifier: GPL-3.0-or-later
#include "missing_storage.hpp"
#include "producer_fixture.hpp"

#include <subathon/server/sqlite_reference_authority.hpp>

namespace {
using namespace storage_027_fixture;
using predecessor::check;
using predecessor::mutate;

constexpr auto successor_digest =
    "52b68a611a96311f137c033cf5b0a376a585e555a56b555e20de3938d4db463e";
constexpr qsizetype successor_objects = 276;
constexpr qsizetype successor_manifest_bytes = 1'748'014;
constexpr qsizetype successor_projection_bytes = 147'980;

// v20 inspection (six attempts), nine DDL, one intermediate projection,
// history insertion, then six final inspections. SELECT schema charges 12
// cells and four type names per object, plus each present field twice. Each
// history row charges seven cells, three type names, a digest and two ID
// copies. Encoding, header and integrity each return one row per inspection;
// FK checks and writes return none. History insertion binds three cells.
constexpr StorageLimits exact_advance{64, 23, 1238, 94, 852, 970197};
constexpr StorageLimits fresh_profile{64, 4096, 65536, 1048576, 8192, 16777216};

MigrationRequest successor_request(MigrationMode mode) {
    MigrationLimits limits;
    limits.schema_objects = successor_objects;
    limits.manifest_utf8_bytes = successor_manifest_bytes;
    return {mode, fixture_manifest(), limits, {}};
}

void advance(CatalogFile &db) {
    const auto target = fixture_manifest().steps.back().version;
    db.reopen(exact_advance);
    report(migrate(db.writer(), successor_request(MigrationMode::advance)), CatalogState::current,
           target, target, 1);
    db.reopen(profile());
}

void expect_refusal(CatalogFile &db, MigrationRequest request, MigrationReason expected) {
    const auto result = migrate(db.writer(), std::move(request));
    require(result.reason == expected && !result.report && !result.work.value &&
                result.work.transaction == TransactionOutcome::rolled_back,
            "Migration refuses with acknowledged rollback and no report");
}

class EmptyStore {
  public:
    EmptyStore() {
        require(dir.isValid() &&
                    writer.open(dir.filePath("fresh.sqlite"), fresh_profile) ==
                        StorageError::none &&
                    observer.open(dir.filePath("fresh.sqlite")) == StorageError::none,
                "Real fresh writer and independent observer open an empty disposable store");
    }
    QTemporaryDir dir;
    SqliteWriter writer;
    SqliteReader observer;
};
} // namespace

class Storage027CatalogTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void compiledManifest() {
        check([] {
            Populated protected_store;
            const auto literal = fixture_manifest();
            const auto compiled = compiled_successor();
            require(std::holds_alternative<MigrationManifest>(compiled),
                    "STORAGE-027 NI: compiled successor after protected populated v20");
            expect_same_manifest(std::get<MigrationManifest>(compiled), literal);
        });
    }

    void literalUpgrade_data() {
        QTest::addColumn<bool>("from_v20");
        QTest::newRow("protected-v20") << true;
        QTest::newRow("retained-v19") << false;
    }

    void literalUpgrade() {
        QFETCH(bool, from_v20);
        check([&] {
            Populated f({.predecessor_v20 = from_v20});
            auto &db = f.db;
            const auto request = successor_request(MigrationMode::inspect);
            const auto &tail = request.manifest.steps.back();
            require(tail.sql.size() == 9 && tail.expected_schema.size() == successor_objects &&
                        edit::projection_bytes(tail) == successor_projection_bytes &&
                        tally(request.manifest) == successor_manifest_bytes &&
                        digest(tail).toHex() == successor_digest,
                    "Exact independent successor projection, byte tally and canonical digest");
            const int start = from_v20 ? 20 : 19;
            report(migrate(db.writer(), request), CatalogState::upgrade_required, start,
                   tail.version, 0);
            const auto objects = request.manifest.steps[start - 1].expected_schema;
            const auto before = snapshot(db, objects);
            if (from_v20)
                advance(db);
            else {
                db.reopen(fresh_profile);
                report(migrate(db.writer(), successor_request(MigrationMode::advance)),
                       CatalogState::current, tail.version, tail.version, 2);
                db.reopen(profile());
            }
            report(migrate(db.writer(), request), CatalogState::current, tail.version, tail.version,
                   0);
            expect_schema(db.rows(schema_query), tail);
            auto after = snapshot(db, objects);
            for (auto &table : after)
                if (table.first == "subathon_migration_history")
                    table.second.resize(start);
            require(after == before && db.rows(history_query) == expected_history(request.manifest),
                    "Exact protected rows, history prefix and complete successor history survive");
            for (const auto &table : new_tables)
                require(db.rows("SELECT count(*) FROM main." + QString::fromLatin1(table.name)) ==
                            SqlRows{{0}},
                        "Additive upgrade never infers a new command, capture or playback row");
            for (const auto mode : {MigrationMode::inspect, MigrationMode::advance})
                for (auto old : {connector_intake_repository_manifest(),
                                 reference_authority_repository_manifest()})
                    expect_refusal(db, {mode, std::move(old), {}, {}},
                                   MigrationReason::unsupported_version);

            EmptyStore fresh;
            report(migrate(fresh.writer, request), CatalogState::fresh, 0, tail.version, 0);
            report(migrate(fresh.writer, successor_request(MigrationMode::advance)),
                   CatalogState::current, tail.version, tail.version, tail.version);
            const auto observed = fresh.observer.read({schema_query, {}});
            require(observed.error == StorageError::none && observed.rows == db.rows(schema_query),
                    "Fresh creation and populated upgrade have identical physical projections");
            const auto history = fresh.observer.read({history_query, {}});
            require(history.error == StorageError::none &&
                        history.rows == expected_history(request.manifest),
                    "Fresh creation and populated upgrade have identical catalog history");
        });
    }

    void manifestLimits() {
        check([] {
            Populated f;
            const auto request = successor_request(MigrationMode::inspect);
            auto objects = request;
            --objects.limits.schema_objects;
            auto bytes = request;
            --bytes.limits.manifest_utf8_bytes;
            auto default_objects = request;
            default_objects.limits.schema_objects = MigrationLimits{}.schema_objects;
            for (const auto &candidate : {objects, bytes, default_objects}) {
                bool called = false;
                const auto result = SqliteMigrationRunner::enqueue(f.db.writer(), candidate,
                                                                   [&](auto) { called = true; });
                require(result.error == StorageError::invalid_options &&
                            result.reason == MigrationReason::invalid_manifest && !called,
                        "Exact-minus-one or default object cap refuses synchronously");
            }
            report(migrate(f.db.writer(), request), CatalogState::upgrade_required, 20,
                   request.manifest.steps.back().version, 0);
        });
    }

    void writerLimits_data() {
        QTest::addColumn<int>("dimension");
        QTest::newRow("attempts") << 0;
        QTest::newRow("peak-statement-utf8") << 1;
        QTest::newRow("bound-value-utf8") << 2;
        QTest::newRow("result-rows") << 3;
        QTest::newRow("result-value-bytes") << 4;
    }

    void writerLimits() {
        QFETCH(int, dimension);
        check([&] {
            Populated f;
            const auto schema = f.db.rows(schema_query);
            const auto history = f.db.rows(history_query);
            auto one_below = exact_advance;
            constexpr qsizetype StorageLimits::*fields[]{
                &StorageLimits::statements_per_batch, &StorageLimits::statement_utf8_bytes,
                &StorageLimits::bound_value_bytes, &StorageLimits::result_rows,
                &StorageLimits::result_value_bytes};
            --(one_below.*fields[dimension]);
            f.db.reopen(one_below);
            const auto refused = migrate(f.db.writer(), successor_request(MigrationMode::advance));
            require(refused.reason == MigrationReason::resource_limit &&
                        refused.work.transaction == TransactionOutcome::rolled_back &&
                        refused.work.error ==
                            (dimension < 3 ? WorkError::resource_limit : WorkError::result_limit) &&
                        !refused.work.value && !refused.report,
                    "Exact-minus-one refuses the corresponding writer charge");
            require(f.db.rows(schema_query) == schema && f.db.rows(history_query) == history,
                    "Any single insufficient dimension rolls back schema and history");
            advance(f.db);
        });
    }

    void corruptSuccessor_data() {
        QTest::addColumn<int>("fault");
        QTest::newRow("wrong-version") << 0;
        QTest::newRow("wrong-marker") << 1;
        QTest::newRow("wrong-digest") << 2;
        QTest::newRow("wrong-ddl") << 3;
        QTest::newRow("missing-schema") << 4;
    }

    void corruptSuccessor() {
        QFETCH(int, fault);
        check([&] {
            Populated f;
            advance(f.db);
            constexpr const char *faults[]{
                "UPDATE main.subathon_migration_history SET version=version+1 WHERE "
                "version=(SELECT max(version) FROM main.subathon_migration_history)",
                "UPDATE main.subathon_migration_history SET migration_id='wrong-successor-marker' "
                "WHERE version=(SELECT max(version) FROM main.subathon_migration_history)",
                "UPDATE main.subathon_migration_history SET content_sha256=zeroblob(32) WHERE "
                "version=(SELECT max(version) FROM main.subathon_migration_history)",
                "ALTER TABLE main.subathon_milestone_playback_evidence ADD COLUMN excess BLOB",
                "DROP TABLE main.subathon_milestone_playback_evidence"};
            mutate(f.db, faults[fault]);
            const auto schema = f.db.rows(schema_query);
            const auto history = f.db.rows(history_query);
            for (const auto mode : {MigrationMode::inspect, MigrationMode::advance})
                expect_refusal(f.db, successor_request(mode),
                               fault <= 2 ? MigrationReason::catalog_mismatch
                                          : MigrationReason::schema_mismatch);
            require(f.db.rows(schema_query) == schema && f.db.rows(history_query) == history,
                    "Refused runner does not repair, replace or advance the corrupt catalog");
        });
    }

    void referenceV10OwnExplicitLifecycle() {
        check([] {
            EmptyStore f;
            const auto manifest = reference_authority_repository_manifest();
            const auto before = f.observer.read({schema_query, {}});
            require(before.error == StorageError::none && before.rows.isEmpty(),
                    "Opening a physical writer does not assign any catalog");
            report(migrate(f.writer, {MigrationMode::inspect, manifest, {}, {}}),
                   CatalogState::fresh, 0, 10, 0);
            report(migrate(f.writer, {MigrationMode::advance, manifest, {}, {}}),
                   CatalogState::current, 10, 10, 10);
            report(migrate(f.writer, {MigrationMode::inspect, manifest, {}, {}}),
                   CatalogState::current, 10, 10, 0);
        });
    }

    void cancelAfterLiteralDdl() {
        check([] {
            Populated f;
            const auto schema = f.db.rows(schema_query);
            const auto history = f.db.rows(history_query);
            auto request = successor_request(MigrationMode::advance);
            const auto target = request.manifest.steps.back().version;
            request.cancel = [target](const auto &p) {
                return p.version == target && p.checkpoint == MigrationCheckpoint::before_history &&
                       p.statement == 9;
            };
            f.db.reopen(exact_advance);
            expect_refusal(f.db, std::move(request), MigrationReason::cancelled);
            require(f.db.rows(schema_query) == schema && f.db.rows(history_query) == history,
                    "Cancelling after DDL rolls back the complete successor");
            advance(f.db);
        });
    }
};

QTEST_GUILESS_MAIN(Storage027CatalogTests)
#include "catalog_tests.moc"
