// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/sqlite_rate_override_catalog_fixture.hpp"

namespace {
using namespace rate_fixture;

template <class Test> void check(Test test) {
    try {
        test();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

class SqliteRateOverrideCatalogTests : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void literalProjectionBudgetAndReservedSeed_R09() {
        check([] {
            const auto literal = literal_v17();
            const auto budget = manifest_bytes(literal);
            auto prefix = literal;
            prefix.steps.removeLast();
            require(manifest_bytes(prefix) == v16_manifest_bytes && budget == v17_manifest_bytes &&
                        MigrationLimits{}.manifest_utf8_bytes == 1048576,
                    "Protected measured prefix and default UTF-8 cap");
            require(literal.steps.back().sql.size() == 7 &&
                        literal.steps.back().expected_schema.size() == v17_projection_objects &&
                        prefix.steps.back().expected_schema.size() == v17_projection_objects - 14,
                    "Five tables, nine implicit indexes and two seeds");
            CatalogFile file(provider_credentials_repository_manifest());
            const auto original_schema = file.rows(schema_sql);
            const auto original_history = file.rows(history_sql);

            MigrationLimits one_below;
            one_below.manifest_utf8_bytes = budget - 1;
            const auto bounded = SqliteMigrationRunner::enqueue(
                *file.writer, {MigrationMode::advance, literal, one_below, {}}, [](auto) {});
            require(bounded.error == StorageError::invalid_options &&
                        bounded.reason == MigrationReason::invalid_manifest,
                    "One byte below the measured cumulative UTF-8 sum refuses before SQL");
            require(file.rows(schema_sql) == original_schema &&
                        file.rows(history_sql) == original_history,
                    "Rejected request leaves the genuine v16 catalog intact");

            const auto cancelled = run_catalog(
                *file.writer, MigrationMode::advance, literal, budget, [](const auto &progress) {
                    return progress.version == 17 &&
                           progress.checkpoint == MigrationCheckpoint::before_history;
                });
            expect_migration_refusal(cancelled, MigrationReason::cancelled);
            require(file.rows(schema_sql) == original_schema &&
                        file.rows(history_sql) == original_history,
                    "Rollback after both seeds removes the whole v17 step");

            receive_literal_v17(file);
            expect_schema(file.rows(schema_sql), literal.steps.back());
            auto history = original_history;
            history.append(history_row(literal.steps.back()));
            require(file.rows(history_sql) == history,
                    "Full independently framed v17 history digest");
            require(history.back()[2].toByteArray() == v17_digest &&
                        canonical_migration_payload(literal.steps.back()).size() ==
                            v17_framed_bytes &&
                        migration_content_sha256(literal.steps.back()) == v17_digest,
                    "Catalog encoder matches the independent ASCII framing oracle");
            require(snapshot_bytes(core::EffectiveRateSnapshotInput{"E", {}, {}, {}, {}, {}, {}}) ==
                        root_bytes,
                    "The shipped RSV1 encoder agrees with the literal E document");
            const auto revisions = file.rows(revision_rows);
            require(revisions.size() == 1, "Only E is seeded in the revision relation");
            expect_reserved_root(revisions.front());
            require(file.rows(head_rows) == SqlRows{{blob("H"), blob("E")}} &&
                        file.rows(observation_rows).empty() && file.rows(decision_rows).empty() &&
                        file.rows(occurrence_rows).empty(),
                    "H points at E and seeds imply no observations, decisions or occurrences");
        });
    }

    void populatedProtectedUpgradeAndOldSelectionRefusal_R09() {
        check([] {
            CatalogFile file(provider_credentials_repository_manifest());
            file.write([](const auto &session) { add_blob_for_upgrade(session); });
            const auto prefix = provider_credentials_repository_manifest();
            const auto literal = literal_v17();
            const auto compiled = rate_overrides_repository_manifest(); // Catalog NI
            require(compiled.catalog_format == literal.catalog_format &&
                        compiled.store_identity == literal.store_identity &&
                        compiled.supported_starting_versions ==
                            literal.supported_starting_versions &&
                        compiled.steps.size() == 17,
                    "Exact migration catalog identity and starting versions");
            for (qsizetype i = 0; i < compiled.steps.size(); ++i) {
                const auto &a = compiled.steps[i];
                const auto &e = literal.steps[i];
                require(a.version == e.version && a.id == e.id && a.sql == e.sql &&
                            a.expected_schema == e.expected_schema,
                        "Exact protected prefix and literal v17 step");
            }

            constexpr auto sentinel = "SELECT scope,record,typeof(scope),typeof(record) "
                                      "FROM subathon_provider_credentials";
            const SqlRows prior{{QByteArray::fromHex("4900"), QByteArray::fromHex("00ff7316"),
                                 QString("blob"), QString("blob")}};
            const auto history = file.rows(history_sql);
            const auto schema = file.rows(schema_sql);
            const auto budget = manifest_bytes(literal);
            file.reopen();
            require(file.rows(schema_sql) == schema && file.rows(history_sql) == history,
                    "Opening the writer does not advance the catalog");
            expect_report(run_catalog(*file.writer, MigrationMode::inspect, compiled, budget),
                          CatalogState::upgrade_required, 16, 17, 0);
            require(file.rows(schema_sql) == schema && file.rows(history_sql) == history &&
                        file.rows(sentinel) == prior,
                    "Inspection leaves populated v16 untouched");
            expect_report(run_catalog(*file.writer, MigrationMode::advance, compiled, budget),
                          CatalogState::current, 17, 17, 1);
            file.reopen();
            expect_report(run_catalog(*file.writer, MigrationMode::inspect, compiled, budget),
                          CatalogState::current, 17, 17, 0);
            expect_report(run_catalog(*file.writer, MigrationMode::advance, compiled, budget),
                          CatalogState::current, 17, 17, 0);
            expect_migration_refusal(
                run_catalog(*file.writer, MigrationMode::inspect, prefix, v16_manifest_bytes),
                MigrationReason::unsupported_version);
            expect_migration_refusal(
                run_catalog(*file.writer, MigrationMode::advance, prefix, v16_manifest_bytes),
                MigrationReason::unsupported_version);
            auto expected_history = history;
            expected_history.append(history_row(literal.steps.back()));
            expect_schema(file.rows(schema_sql), literal.steps.back());
            require(file.rows(sentinel) == prior && file.rows(history_sql) == expected_history &&
                        file.rows(head_rows) == SqlRows{{blob("H"), blob("E")}},
                    "Upgrade appends one marker, preserves every prior marker and retains opaque "
                    "bytes");
        });
    }

    void markerConsumers_R09_data() {
        QTest::addColumn<int>("shape");
        QTest::addColumn<int>("consumer");
        for (const auto &[name, kind] : {std::pair{"financial", Consumer::financial},
                                         {"milestone", Consumer::milestone},
                                         {"modifier", Consumer::modifier}}) {
            QTest::newRow(qPrintable(QString("%1-exact-v17").arg(name))) << 0 << int(kind);
            QTest::newRow(qPrintable(QString("%1-wrong-case").arg(name))) << 1 << int(kind);
            QTest::newRow(qPrintable(QString("%1-wrong-version").arg(name))) << 2 << int(kind);
        }
        QTest::newRow("missing-rate-schema") << 3 << 0;
    }

    void markerConsumers_R09() {
        QFETCH(int, shape);
        QFETCH(int, consumer);
        check([&] {
            finance::File file(finance::OriginalCredit{.milestone = true});
            populate_v15(file);
            expect_report(migrate(*file.writer, MigrationMode::advance,
                                  provider_credentials_repository_manifest()),
                          CatalogState::current, 16, 16, 1);
            const auto read = [which = Consumer(consumer),
                               payment = file.payment](const auto &session) {
                expect_v15_evidence(session, which, payment);
            };
            file.write(read);
            const auto literal = literal_v17();
            expect_report(
                run_catalog(*file.writer, MigrationMode::advance, literal, manifest_bytes(literal)),
                CatalogState::current, 17, 17, 1);
            if (shape != 0) {
                file.write([&](const auto &session) {
                    if (shape == 3) {
                        (void)session.execute({"DROP TABLE subathon_rate_head", {}});
                    } else {
                        (void)session.execute(
                            {"UPDATE subathon_migration_history SET version=?,migration_id=? WHERE "
                             "version=17",
                             {shape == 2 ? 18 : 17,
                              shape == 1 ? QString("Rate-overrides-v1") : rate_marker}});
                    }
                });
                const auto why = shape == 3 ? MigrationReason::schema_mismatch
                                            : MigrationReason::catalog_mismatch;
                expect_migration_refusal(run_catalog(*file.writer, MigrationMode::inspect, literal,
                                                     manifest_bytes(literal)),
                                         why);
                if (shape == 3)
                    return; // Missing schema is independently verified by the catalog runner.
            }

            if (shape == 0)
                file.write(read);
            else if (Consumer(consumer) == Consumer::modifier)
                expect_consumer_refusal<SqliteModifierStorageFailure>(
                    *file.writer, SqliteModifierStorageReason::invalid_stored_data, read);
            else
                expect_consumer_refusal<SqliteBundleStorageFailure>(
                    *file.writer, SqliteBundleStorageReason::invalid_stored_data, read);
        });
    }

  private:
    static void add_blob_for_upgrade(const SqliteWorkSession &session) {
        (void)session.execute({"INSERT INTO subathon_provider_credentials VALUES(?,?)",
                               {QByteArray::fromHex("4900"), QByteArray::fromHex("00ff7316")}});
    }
};
} // namespace

QTEST_GUILESS_MAIN(SqliteRateOverrideCatalogTests)
#include "sqlite_rate_override_catalog_tests.moc"
