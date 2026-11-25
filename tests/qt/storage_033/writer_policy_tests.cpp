// SPDX-License-Identifier: GPL-3.0-or-later
#include "../storage_003/protected_predecessor_fixture.hpp"

namespace {
using namespace storage_003_fixture;
using detail::ClockPredecessor;

constexpr auto allocator = "SELECT last_allocated FROM main.subathon_contribution_allocator "
                           "WHERE singleton=1";
constexpr auto allocate = "UPDATE main.subathon_contribution_allocator "
                          "SET last_allocated=X'000000000000002A' WHERE singleton=1";
constexpr auto reserved_delete = "DELETE FROM main.subathon_migration_history WHERE version=22";

SqlRows read(const DisposableFile &file, const QString &sql) {
    SqliteReader reader;
    require(reader.open(file.path()) == StorageError::none, "Independent reader opens fixture");
    const auto result = reader.read({sql, {}}, 1024);
    require(result.error == StorageError::none, "Independent reader executes selected query");
    return result.rows;
}

void expect_history(const DisposableFile &file, int version) {
    expect_rows(read(file, select_history), expected_history(version), "Independent fixed history");
}

void expect_header(const DisposableFile &file) {
    expect_rows(
        read(file, "SELECT singleton,catalog_format,store_identity FROM main.subathon_catalog"),
        {{integer(1), integer(1), QStringLiteral("subathon.sqlite-store")}},
        "Exact catalog header");
}

void open(SqliteWriter &writer, const DisposableFile &file, StorageLimits limits = setup_profile) {
    require(writer.open(file.path(), limits) == StorageError::none, "Normal writer opens fixture");
}

void create_v22(const DisposableFile &file) {
    file.create_blank();
    const auto witness = candidate(file, {0, ClockPredecessor::blank, {}, {}});
    DisposableFile backup;
    file.copy_to(backup);
    {
        SqliteWriter writer;
        open(writer, file, profile(blank_advance));
        expect_guarded_v22_advance(writer, compiled_successor(), witness);
    }
    expect_history(file, 22);
    expect_header(file);
}

detail::ClockPreOpenWitness create_v21(const DisposableFile &file, DisposableFile &backup) {
    file.create_blank();
    const auto manifest = milestone_slot_interruption_repository_manifest();
    verify_manifest(manifest, 21);
    {
        SqliteWriter writer;
        open(writer, file);
        expect_current(migrate(writer, request(MigrationMode::advance, manifest)), 21, 21);
    }
    expect_history(file, 21);
    expect_rows(read(file, select_head), {}, "Exact v21 queue fixture has no application head");
    const detail::ClockPreOpenWitness expected{21, ClockPredecessor::no_head, {}, {}};
    const auto probe = probe_stopped(file, setup_profile);
    require(probe.disposition == detail::ClockPreOpenResult::Disposition::candidate &&
                probe.reason == MigrationReason::none && probe.witness == expected,
            "Authenticated stopped v21 no-head basis");
    file.copy_to(backup);
    return *probe.witness;
}

void drain(SqliteWriter &writer) {
    auto done = std::make_shared<bool>(false);
    require(writer.shutdown([done] { *done = true; }) == StorageError::none &&
                QTest::qWaitFor([done] { return *done; }, 10000),
            "Shutdown drains all accepted items and closes the writer");
}

void admitted(MigrationAdmission admission) {
    require(admission.error == StorageError::none && admission.reason == MigrationReason::none,
            "Migration accepted on the intended writer mode");
}

MigrationManifest other_target(const QString &target) {
    auto manifest = compiled_successor();
    if (target == "predecessor")
        return milestone_slot_interruption_repository_manifest();
    if (target == "noncanonical") {
        manifest.steps.back().id = "storage-033-noncanonical";
    } else {
        const auto schema = manifest.steps.back().expected_schema;
        manifest.steps.append({23, "storage-033-successor", {"SELECT 1"}, schema});
        manifest.supported_starting_versions.append(23);
    }
    return manifest;
}

struct QueueReceipt {
    std::optional<detail::ClockAdvanceResult> owner;
    QList<MigrationResult> migrations;
    QList<WriteResult> batches;
    int probes{0};
};

void queue_owner(SqliteWriter &writer, const detail::ClockPreOpenWitness &witness,
                 const std::shared_ptr<QueueReceipt> &receipt) {
    admitted(detail::enqueue_clock_checkpoint_advance(
        writer, request(MigrationMode::advance, compiled_successor()), witness,
        [receipt](auto result) { receipt->owner = std::move(result); }));
}

void expect_owner(const QueueReceipt &receipt) {
    require(receipt.owner && receipt.owner->refusal == detail::ClockAdvanceRefusal::none,
            "Guarded owner reaches acknowledged v22 upgrade");
    expect_current(receipt.owner->migration, 22, 1);
}
} // namespace

class Storage033WriterPolicyTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void governedBatch_data() {
        QTest::addColumn<QString>("sql");
        QTest::newRow("create") << QString(
            "CREATE TABLE IF NOT EXISTS subathon_contribution_allocator(x INTEGER)");
        QTest::newRow("temp-table") << QString("CREATE TEMP TABLE storage_033_probe(x INTEGER)");
        QTest::newRow("temp-trigger")
            << QString("CREATE TEMP TRIGGER storage_033_probe AFTER UPDATE ON "
                       "main.subathon_contribution_allocator BEGIN SELECT 1; END");
        QTest::newRow("drop") << QString("DROP TABLE IF EXISTS storage_033_missing");
        QTest::newRow("alter") << QString(
            "ALTER TABLE main.subathon_contribution_allocator ADD COLUMN extra INTEGER");
        QTest::newRow("attach") << QString("ATTACH DATABASE ':memory:' AS storage_033_probe");
        QTest::newRow("detach") << QString("DETACH DATABASE storage_033_probe");
        QTest::newRow("cte-delete")
            << QString("WITH chosen(v) AS (VALUES(22)) DELETE FROM "
                       "main.[Subathon_Migration_History] WHERE version IN (SELECT v FROM chosen)");
        QTest::newRow("double-quoted") << QString(
            "UPDATE \"main\".\"SUBATHON_CATALOG\" SET catalog_format=2 WHERE singleton=1");
        QTest::newRow("single-quoted")
            << QString("UPDATE 'subathon_catalog' SET catalog_format=2 WHERE singleton=1");
        QTest::newRow("backtick") << QString(
            "DELETE FROM main.`Subathon_Migration_History` WHERE version=22");
        QTest::newRow("insert") << QString("INSERT INTO main.subathon_migration_history VALUES "
                                           "(23,'storage-033-probe',zeroblob(32))");
        QTest::newRow("replace") << QString(
            "REPLACE INTO main.subathon_catalog VALUES (1,2,'subathon.sqlite-store')");
    }

    void governedBatch() {
        QFETCH(QString, sql);
        check([&] {
            DisposableFile file;
            create_v22(file);
            const auto schema = read(
                file, "SELECT type,name,tbl_name,sql FROM main.sqlite_schema ORDER BY type,name");
            const auto receipt = std::make_shared<QueueReceipt>();
            SqliteWriter writer;
            open(writer, file);
            const auto status = writer.enqueue(
                {{allocate, {}}, {sql, {}}, {"SELECT 99", {}}},
                [receipt](auto result) { receipt->batches.append(std::move(result)); });
            // An unexpected admission is itself the first mismatch. Destruction
            // closes the disposable writer before the queued SQL can dispatch.
            require(status == StorageError::invalid_batch, "First stop: governed batch admission");
            drain(writer);
            require(receipt->batches.isEmpty(), "Synchronous refusal has no completion");
            expect_rows(read(file, allocator), {{QVariant{}}},
                        "Prior allocator change never executes");
            expect_history(file, 22);
            expect_header(file);
            expect_rows(
                read(file,
                     "SELECT type,name,tbl_name,sql FROM main.sqlite_schema ORDER BY type,name"),
                schema, "Full schema projection survives refusal");
        });
    }

    void stickyWork() {
        check([] {
            DisposableFile file;
            create_v22(file);
            SqliteWriter writer;
            open(writer, file);
            auto observed = std::make_shared<QList<WorkError>>();
            const auto result = qt_test_support::run_work(
                writer,
                [observed](auto context) {
                    const auto session = context.begin();
                    (void)session.execute({allocate, {}});
                    try {
                        (void)session.execute({reserved_delete, {}});
                    } catch (const SqliteWorkFailure &error) {
                        observed->append(error.error());
                    }
                    if (observed->isEmpty()) {
                        session.rollback();
                        return 99;
                    }
                    try {
                        session.commit();
                    } catch (const SqliteWorkFailure &error) {
                        observed->append(error.error());
                    }
                    return 99;
                },
                10000);
            require(*observed ==
                        QList<WorkError>{WorkError::invalid_statement, WorkError::session_closed},
                    "First stop: prohibited execute poisons the live work transaction");
            require(result.transaction == TransactionOutcome::rolled_back &&
                        result.error == WorkError::invalid_statement && !result.value,
                    "Caught failure remains sticky and returns no value");
            drain(writer);
            expect_rows(read(file, allocator), {{QVariant{}}}, "Allocator update rolls back");
            expect_history(file, 22);
        });
    }

    void ordinaryControl() {
        check([] {
            DisposableFile file;
            create_v22(file);
            SqliteWriter writer;
            open(writer, file);
            auto result = std::make_shared<std::optional<WriteResult>>();
            require(writer.enqueue(
                        {{allocate, {}},
                         {"WITH subathon_catalog(v) AS (VALUES('subathon_migration_history')) "
                          "SELECT v FROM subathon_catalog",
                          {}},
                         {"SELECT catalog_format FROM main.subathon_catalog WHERE singleton=1", {}},
                         {"PRAGMA foreign_keys", {}}},
                        [result](auto value) { *result = std::move(value); }) == StorageError::none,
                    "Ordinary DML, CTE, catalog read and diagnostic PRAGMA are admitted");
            drain(writer);
            require(*result && (**result).error == StorageError::none &&
                        (**result).rows.size() == 4,
                    "Ordinary batch commits one result per statement");
            expect_rows((**result).rows[1], {{QStringLiteral("subathon_migration_history")}},
                        "CTE literal is data");
            expect_rows((**result).rows[2], {{integer(1)}}, "Catalog read returns its format");
            expect_rows((**result).rows[3], {{integer(1)}}, "Foreign keys remain enabled");
            expect_rows(read(file, allocator), {{blob(QByteArray::fromHex("000000000000002a"))}},
                        "Committed independently fixed allocator bytes");
        });
    }

    void nonGoverningControls() {
        check([] {
            DisposableFile generic;
            generic.create_blank();
            {
                SqliteWriter writer;
                open(writer, generic);
                expect_commit(qt_test_support::run_work(
                    writer,
                    [](auto context) {
                        const auto session = context.begin();
                        (void)session.execute({"CREATE TABLE subathon_catalog(v INTEGER)", {}});
                        (void)session.execute({"INSERT INTO subathon_catalog VALUES (7)", {}});
                        session.commit();
                        return 1;
                    },
                    10000));
                drain(writer);
            }
            expect_rows(read(generic, "SELECT v FROM subathon_catalog"), {{integer(7)}},
                        "Generic store retains ordinary reserved-looking name behavior");

            DisposableFile legacy;
            DisposableFile backup;
            (void)create_v21(legacy, backup);
            SqliteWriter writer;
            open(writer, legacy);
            auto result = std::make_shared<std::optional<WriteResult>>();
            require(writer.enqueue({{"CREATE TABLE storage_033_probe(v INTEGER)", {}},
                                    {"INSERT INTO storage_033_probe VALUES (7)", {}}},
                                   [result](auto value) { *result = std::move(value); }) ==
                        StorageError::none,
                    "Legacy production writer admits ordinary DDL and DML");
            drain(writer);
            require(*result && (**result).error == StorageError::none,
                    "Legacy production batch commits");
            expect_rows(read(legacy, "SELECT v FROM storage_033_probe"), {{integer(7)}},
                        "Legacy production reader sees committed value");
            expect_history(legacy, 21);
        });
    }

    void queuedWork() {
        check([] {
            DisposableFile file;
            DisposableFile backup;
            const auto witness = create_v21(file, backup);
            SqliteWriter writer;
            open(writer, file);
            const auto receipt = std::make_shared<QueueReceipt>();
            queue_owner(writer, witness, receipt);
            auto result = std::make_shared<std::optional<SqliteWorkResult>>();
            require(writer.enqueue_work(
                        [](auto context) {
                            const auto session = context.begin();
                            (void)session.execute({allocate, {}});
                            (void)session.execute({reserved_delete, {}});
                            session.rollback();
                            return 99;
                        },
                        [result](auto value) { *result = std::move(value); }) == StorageError::none,
                    "Ordinary work is admitted while the v22 owner job is still queued");
            require(!receipt->owner && !*result, "Neither queue item has dispatched");
            drain(writer);
            expect_owner(*receipt);
            require(*result && (**result).transaction == TransactionOutcome::rolled_back &&
                        (**result).error == WorkError::invalid_statement && !(**result).value,
                    "First stop: ordinary work uses the governing policy after dequeue");
            expect_rows(read(file, allocator), {{QVariant{}}},
                        "Queued work allocator update rolls back");
            expect_history(file, 22);
        });
    }

    void genericCurrentInspectionBudget() {
        check([] {
            DisposableFile file;
            create_v22(file);
            SqliteWriter writer;
            open(writer, file, profile(successor_inspect));
            expect_current(migrate(writer, request(MigrationMode::advance, compiled_successor())),
                           22, 0);
            drain(writer);
            expect_history(file, 22);
        });
    }

    void otherGenericAdvance_data() {
        QTest::addColumn<QString>("target");
        for (const auto *target : {"predecessor", "noncanonical", "successor"})
            QTest::newRow(target) << QString::fromLatin1(target);
    }

    void otherGenericAdvance() {
        QFETCH(QString, target);
        check([&] {
            DisposableFile file;
            create_v22(file);
            SqliteWriter writer;
            open(writer, file);
            const auto receipt = std::make_shared<QueueReceipt>();
            auto command = request(MigrationMode::advance, other_target(target));
            command.cancel = [receipt](auto) {
                ++receipt->probes;
                return true;
            };
            const auto admission =
                SqliteMigrationRunner::enqueue(writer, std::move(command), [receipt](auto result) {
                    receipt->migrations.append(std::move(result));
                });
            drain(writer);
            require(admission.error == StorageError::invalid_options &&
                        admission.reason == MigrationReason::invalid_manifest,
                    "First stop: noncanonical or noncurrent generic advance refuses admission");
            require(receipt->probes == 0 && receipt->migrations.isEmpty(),
                    "Refusal reaches neither checkpoint nor completion");
            expect_history(file, 22);
        });
    }

    void queuedGenericAdvance_data() {
        QTest::addColumn<bool>("exact");
        QTest::newRow("canonical-current") << true;
        QTest::newRow("predecessor") << false;
    }

    void queuedGenericAdvance() {
        QFETCH(bool, exact);
        check([&] {
            DisposableFile file;
            DisposableFile backup;
            const auto witness = create_v21(file, backup);
            SqliteWriter writer;
            open(writer, file);
            const auto receipt = std::make_shared<QueueReceipt>();
            queue_owner(writer, witness, receipt);
            auto command = request(MigrationMode::advance,
                                   exact ? compiled_successor() : other_target("predecessor"));
            command.cancel = [receipt, exact](auto) {
                ++receipt->probes;
                return !exact;
            };
            admitted(
                SqliteMigrationRunner::enqueue(writer, std::move(command), [receipt](auto result) {
                    receipt->migrations.append(std::move(result));
                }));
            require(!receipt->owner && receipt->migrations.isEmpty(),
                    "Both requests queued before dispatch");
            drain(writer);
            expect_owner(*receipt);
            require(receipt->migrations.size() == 1, "Accepted item completes exactly once");
            const auto &result = receipt->migrations.front();
            if (exact) {
                expect_current(result, 22, 0);
            } else {
                require(result.work.transaction == TransactionOutcome::not_started &&
                            result.work.error == WorkError::none && !result.work.value &&
                            result.reason == MigrationReason::invalid_manifest && !result.report &&
                            receipt->probes == 0,
                        "First stop: governing dequeue refuses with the typed migration result");
            }
            expect_history(file, 22);
            expect_header(file);
        });
    }

    void queuedBatchAndTempShadow() {
        check([] {
            DisposableFile file;
            DisposableFile backup;
            const auto witness = create_v21(file, backup);
            SqliteWriter writer;
            open(writer, file);
            expect_commit(qt_test_support::run_work(
                writer,
                [](auto context) {
                    const auto session = context.begin();
                    (void)session.execute(
                        {"CREATE TEMP TABLE subathon_catalog(catalog_format INTEGER)", {}});
                    (void)session.execute({"INSERT INTO temp.subathon_catalog VALUES (7)", {}});
                    session.commit();
                    return 1;
                },
                10000));
            const auto receipt = std::make_shared<QueueReceipt>();
            queue_owner(writer, witness, receipt);
            for (const auto *sql : {"DROP TABLE IF EXISTS storage_033_missing",
                                    "UPDATE subathon_catalog SET catalog_format=8",
                                    "UPDATE main.subathon_catalog SET catalog_format=2"}) {
                require(writer.enqueue({{allocate, {}}, {sql, {}}},
                                       [receipt](auto result) {
                                           receipt->batches.append(std::move(result));
                                       }) == StorageError::none,
                        "Legacy mode accepts batch before upgrade dispatch");
            }
            require(!receipt->owner && receipt->batches.isEmpty(),
                    "All items are pending before dispatch");
            drain(writer);
            expect_owner(*receipt);
            require(receipt->batches.size() == 3, "Each accepted batch completes once");
            for (const auto &result : receipt->batches)
                require(result.error == StorageError::invalid_batch && result.rows.isEmpty(),
                        "First stop: dequeue refuses before preparing governed batch");
            expect_rows(read(file, allocator), {{QVariant{}}},
                        "No queued allocator update executes");
            expect_header(file);
            expect_history(file, 22);
        });
    }
};

QTEST_GUILESS_MAIN(Storage033WriterPolicyTests)
#include "writer_policy_tests.moc"
