// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/milestone_target_edit_fixture.hpp"
#include "support/sqlite_rate_override_writer_fixture.hpp"

namespace {
using namespace milestone_target_edit_fixture;

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

void expect_exact(const MigrationManifest &actual, const MigrationManifest &expected) {
    require(actual.catalog_format == expected.catalog_format &&
                actual.store_identity == expected.store_identity &&
                actual.supported_starting_versions == expected.supported_starting_versions &&
                actual.steps.size() == expected.steps.size(),
            "Same catalog identity and exact starting-version set");
    for (qsizetype i = 0; i < expected.steps.size(); ++i) {
        const auto &a = actual.steps[i];
        const auto &e = expected.steps[i];
        require(a.version == e.version && a.id == e.id && a.sql == e.sql &&
                    a.expected_schema == e.expected_schema,
                QString("Exact ordered SQL and canonical projection for v%1").arg(e.version));
    }
}

void expect_all(const SqliteWorkSession &s, const FinancialPaymentKey &payment) {
    cc::expect_v15_evidence(s, cc::Consumer::financial, payment);
    cc::expect_v15_evidence(s, cc::Consumer::milestone, payment);
    cc::expect_v15_evidence(s, cc::Consumer::modifier, payment);
    const auto delivery = SqliteDeliveryJobStore(s).lookup({finance::original, "original-job"});
    require(delivery && delivery->state == DeliveryJobState::queued && delivery->revision == 1 &&
                delivery->action_document == "synthetic-action",
            "Original application job selected with full capture");
    const auto basis = SqliteApplicationBundleStore(s).read_reversal_basis(
        {finance::original, 2, finance::CK::score});
    require(basis.basis && basis.basis->progress.cumulative == 10000000,
            "Bundle consumer selects the financial score basis");
}

class LegacyDelivery final : public DeliveryDispatchAuthority, public DeliveryAdapter {
  public:
    bool allows(const DeliveryJob &job) override {
        return job.capture.sender_ref == "sender" &&
               job.action_document == cc::milestones::action_document;
    }

    void start(DeliveryJob job) override { started = std::move(job); }

    std::optional<DeliveryJob> started;
};

} // namespace

class SqliteMilestoneTargetEditCatalogTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void legacyV11IndependentDispatchAndV12Release() {
        check([] {
            cc::milestones::File old(cc::milestones::File::Schema::v11);
            (void)cc::milestones::committed(*old.writer, [&](const auto &s) {
                old.seed_origin(s);
                return true;
            });
            const auto ports = std::make_shared<LegacyDelivery>();
            const auto claim = finance::completed<DeliveryDispatchCompletion>([&](auto done) {
                return dispatch_delivery_job(*old.writer, {cc::milestones::origin, "legacy"}, 1,
                                             ports, ports, std::move(done));
            });
            finance::normal(claim.work);
            require(claim.result && claim.result->disposition == DeliveryDisposition::changed &&
                        !claim.adapter_threw && ports->started && ports->started->attempt == 2,
                    "Independent v11 application claim has no milestone prerequisite");

            cc::milestones::File original(cc::milestones::File::Schema::literal_v12);
            original.record();
            const auto released = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                return release_milestone_head(*original.writer, {"z-first"}, std::move(done));
            });
            finance::normal(released.work);
            require(released.disposition == MilestoneReleaseDisposition::released &&
                        original.reads("SELECT released FROM subathon_milestone_groups "
                                       "WHERE queue_order=1") == SqlRows{{1}},
                    "Ordinary v12 release persists the exact selected head");
        });
    }

    // Keep the reviewed Qt slot names for direct selection.
    void compiledProducerAdvancesPopulatedProtectedV16() {
        check([] {
            finance::File file(finance::OriginalCredit{.milestone = true});
            cc::populate_v15(file);
            const auto ordinary = file.detect(file.ordinary_detection("legacy-hold", false));
            advance_v16(*file.writer);
            file.write([&](const auto &s) {
                expect_all(s, file.payment);
                write(s, "INSERT INTO subathon_provider_credentials(scope,record) VALUES(?,?)",
                      {QByteArray::fromHex("00ff"), QByteArray::fromHex("00811600")});
            });
            const auto v16_history = file.rows(cc::history_sql);
            advance_v17(*file.writer);
            const auto before = file.rows(cc::history_sql);
            const auto rate_image = [&] {
                return std::array{file.rows(rate_fixture::revision_rows),
                                  file.rows(rate_fixture::observation_rows),
                                  file.rows(rate_fixture::decision_rows),
                                  file.rows(rate_fixture::occurrence_rows),
                                  file.rows(rate_fixture::head_rows)};
            };
            const auto old_rate_image = rate_image();
            auto expected_v17_history = v16_history;
            expected_v17_history.append(cc::history_row(rate_fixture::literal_v17().steps.back()));
            require(before == expected_v17_history &&
                        before.back()[2].toByteArray() == rate_fixture::v17_digest,
                    "Rate upgrade appends exactly the protected v17 history row");
            require(old_rate_image[0].size() == 1 &&
                        old_rate_image[4] == SqlRows{{QByteArray("H"), QByteArray("E")}},
                    "Protected v17 creates the exact reserved revision and head");
            rate_fixture::expect_reserved_root(old_rate_image[0].front());
            const auto literal = literal_v18();

            const auto actual = milestone_target_edits_repository_manifest();
            expect_exact(actual, literal);
            cc::expect_report(migrate(*file.writer, MigrationMode::inspect, actual),
                              CatalogState::upgrade_required, 17, 18, 0);
            const auto rejected = SqliteMigrationRunner::enqueue(
                *file.writer,
                {MigrationMode::advance, actual, {.manifest_utf8_bytes = v18_bytes - 1}, {}},
                [](MigrationResult) {});
            require(rejected.error == StorageError::invalid_options &&
                        rejected.reason == MigrationReason::invalid_manifest &&
                        file.rows(cc::history_sql) == before,
                    "An explicit budget one byte short refuses preflight");

            cc::expect_report(migrate(*file.writer, MigrationMode::advance, actual),
                              CatalogState::current, 18, 18, 1);
            auto history = before;
            history.append(cc::history_row(literal.steps.back()));
            require(file.rows(cc::history_sql) == history,
                    "All v17 digest bytes and exact v18 hash");
            require(history.back()[2].toByteArray() == v18_digest &&
                        canonical_migration_payload(literal.steps.back()).size() ==
                            v18_framed_bytes,
                    "The independent ASCII frame has the exact v18 size and digest");
            require(rate_image() == old_rate_image,
                    "Additive milestone upgrade preserves the protected rate root and rows");
            cc::expect_schema(file.rows(cc::schema_sql), literal.steps.back());
            require(file.rows("SELECT scope,record,typeof(scope),typeof(record) "
                              "FROM subathon_provider_credentials") ==
                        SqlRows{{QByteArray::fromHex("00ff"), QByteArray::fromHex("00811600"),
                                 QString("blob"), QString("blob")}},
                    "The protected v16 table retains the exact synthetic BLOB row");
            for (const auto &table : edit_tables)
                require(file.rows("SELECT count(*) FROM main." + QString::fromLatin1(table.name)) ==
                            SqlRows{{0}},
                        "Additive upgrade leaves every edit table empty");

            const auto detection = file.ordinary_detection("second-hold", false);
            file.write([&](const auto &s) {
                expect_all(s, file.payment);
                const auto hold = SqliteApplicationHoldStore(s).find(ordinary.hold_ref);
                require(hold && hold->current_version.locator.version_ref == ordinary.version_ref,
                        "Existing ordinary hold and version survive the additive migration");
                require(SqliteApplicationHoldStore(s).record_detection(detection).disposition ==
                            SqliteHoldMutationDisposition::recorded,
                        "Hold mutation selects the v18 financial lane");
            });
        });
    }

    void exactV18PreservesTypedRateRepository() {
        check([] {
            rate_fixture::RateFile file;
            const auto first = rate_fixture::fetched_preview();
            rate_fixture::seed_first(file, true);
            const auto candidate = rate_fixture::snapshot_bytes(first.proposal.candidate);
            const auto expect_retained = [&] {
                const auto head = rate_fixture::selected<RateRevisionResult>(
                    file, [](const auto &unit) { return unit.head(); });
                const auto *snapshot = std::get_if<subathon::core::EffectiveRateSnapshot>(&head);
                require(snapshot && rate_fixture::snapshot_bytes(*snapshot) == candidate,
                        "Typed rate head selects the preserved non-root revision");
                file.write([&](const SqliteWorkSession &s) {
                    const SqliteRateOverrideUnit unit(s);
                    const auto root = unit.revision("E");
                    const auto *reserved =
                        std::get_if<subathon::core::EffectiveRateSnapshot>(&root);
                    require(reserved &&
                                rate_fixture::snapshot_bytes(*reserved) == rate_fixture::root_bytes,
                            "Typed revision still selects the exact reserved root");
                    rate_fixture::Authority authority;
                    const auto recovery = unit.recover(
                        authority, rate_fixture::literal_rate_trust(), first.edit, first);
                    const auto *receipt = std::get_if<RateReceiptSelection>(&recovery);
                    require(receipt && authority.reads == 1,
                            "Typed recovery selects the existing authorized rate decision");
                    rate_fixture::expect_receipt(*receipt, RateReceiptOrigin::retained, first);
                });
            };
            expect_retained();
            file.reopen(finance::limits());
            cc::expect_report(migrate(*file.writer, MigrationMode::advance, literal_v18()),
                              CatalogState::current, 18, 18, 1);
            file.reopen();
            expect_retained();

            const auto expect_wrong_pair = [&](int version, const QString &id) {
                file.write([&](const auto &s) {
                    write(s,
                          "UPDATE subathon_migration_history SET version=?,migration_id=? "
                          "WHERE version>17",
                          {version, id});
                    rate_fixture::expect_refusal(SqliteRateOverrideUnit(s).head(),
                                                 RateStoreRefusal::unavailable);
                });
            };
            expect_wrong_pair(18, "Milestone-target-edits-v1");
            expect_wrong_pair(19, v18_marker);
        });
    }

    void freshExactV17HasEmptyLegacyAndEditLanes() {
        check([] {
            cc::milestones::File file(cc::milestones::File::Schema::empty);
            const auto manifest = literal_v18();
            cc::expect_report(migrate(*file.writer, MigrationMode::inspect, manifest),
                              CatalogState::fresh, 0, 18, 0);
            cc::expect_report(migrate(*file.writer, MigrationMode::advance, manifest),
                              CatalogState::current, 18, 18, 18);
            cc::expect_schema(file.reads(cc::schema_sql), manifest.steps.back());
            (void)cc::milestones::committed(*file.writer, [](const auto &s) {
                require(SqliteMilestoneGroupStore(s).pending().empty() &&
                            !SqliteFinancialRefundStore(s).lookup({"test", "absent"}) &&
                            !SqliteModifierActivationStore(s).instance_revision("absent"),
                        "Exact marker allows matching empty v18 reads");
                return true;
            });
        });
    }

    void exactPairAndRequiredSchema_data() {
        QTest::addColumn<int>("selector");
        QTest::addColumn<int>("variation");
        for (int selector = 0; selector < 3; ++selector)
            for (int variation = 0; variation < 4; ++variation)
                QTest::newRow(qPrintable(QString("%1-%2").arg(
                    QStringList{"financial", "milestone", "modifier"}[selector],
                    QStringList{"exact", "wrong-version", "wrong-case",
                                "missing-schema"}[variation])))
                    << selector << variation;
    }

    void exactPairAndRequiredSchema() {
        QFETCH(int, selector);
        QFETCH(int, variation);
        check([&] {
            const auto consumer = cc::Consumer(selector);
            finance::File file(finance::OriginalCredit{.milestone = true});
            cc::populate_v15(file);
            advance_v16(*file.writer);
            file.write([&](const auto &s) { expect_all(s, file.payment); });
            advance_v17(*file.writer);
            advance_literal(*file.writer);

            const auto read = [=, payment = file.payment](const SqliteWorkSession &s) {
                cc::expect_v15_evidence(s, consumer, payment);
            };
            if (variation == 0) {
                file.write(read);
                return;
            }
            if (variation == 3) {
                // Keep the exact v18 marker while removing the required
                // closure for the selected reader. Each table is leaf data.
                const auto table = QStringList{"subathon_financial_resolutions",
                                               "subathon_milestone_edit_refund_decisions",
                                               "subathon_modifier_targets"}[selector];
                file.write([&](const auto &s) { write(s, "DROP TABLE main." + table); });
                require(
                    file.rows("SELECT version,migration_id FROM subathon_migration_history "
                              "ORDER BY version DESC LIMIT 1") == SqlRows{{18, v18_marker}} &&
                        file.rows("SELECT name FROM sqlite_schema WHERE name=?", {table}).empty(),
                    "Exact marker and missing schema physically established");
                bool started = false;
                bool invalid_stored_data = false;
                const auto result = file.work(
                    [&](auto context) -> QVariant {
                        const auto s = context.begin();
                        started = true;
                        try {
                            read(s);
                        } catch (const SqliteModifierStorageFailure &e) {
                            invalid_stored_data =
                                e.reason() == SqliteModifierStorageReason::invalid_stored_data;
                            throw;
                        } catch (const SqliteBundleStorageFailure &e) {
                            invalid_stored_data =
                                e.reason() == SqliteBundleStorageReason::invalid_stored_data;
                            throw;
                        }
                        s.commit();
                        return 1;
                    },
                    false);
                require(started && result.transaction == TransactionOutcome::rolled_back &&
                            !result.value &&
                            (result.error == WorkError::sql_failure ||
                             (invalid_stored_data && result.error == WorkError::work_exception)),
                        "Recognized marker fails closed on missing required schema");
                return;
            }
            const int version = variation == 1 ? 19 : 18;
            const QString id = variation == 1 ? v18_marker : QString("Milestone-target-edits-v1");
            file.write([&](const auto &s) {
                write(s,
                      "UPDATE subathon_migration_history SET version=?,migration_id=? "
                      "WHERE version=18",
                      {version, id});
            });
            require(file.rows("SELECT version,migration_id FROM subathon_migration_history "
                              "ORDER BY version DESC LIMIT 1") == SqlRows{{version, id}},
                    "Exact wrong pair is committed before the public consumer");
            if (consumer == cc::Consumer::modifier)
                cc::expect_consumer_refusal<SqliteModifierStorageFailure>(
                    *file.writer, SqliteModifierStorageReason::invalid_stored_data, read);
            else
                cc::expect_consumer_refusal<SqliteBundleStorageFailure>(
                    *file.writer, SqliteBundleStorageReason::invalid_stored_data, read);
        });
    }
};

QTEST_GUILESS_MAIN(SqliteMilestoneTargetEditCatalogTests)
#include "sqlite_milestone_target_edit_catalog_tests.moc"
