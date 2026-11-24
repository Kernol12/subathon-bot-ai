// SPDX-License-Identifier: GPL-3.0-or-later
#include "constraint_specimen.hpp"

namespace {
using namespace storage_003_fixture;
using Disposition = detail::ClockPreOpenResult::Disposition;
using Predecessor = detail::ClockPredecessor;

using Query = detail::ClockProbeQuery;

// The probe binds no values, so there is no positive binding-byte edge.
void probe_one_less(const DisposableFile &file, const Charge &charge,
                    const std::array<Query, 4> &stops) {
    for (std::size_t i = 0; i < stops.size(); ++i) {
        auto limits = profile(charge);
        --(limits.*metered_limits[i]);
        const auto result = probe_stopped(file, limits);
        require(result.disposition == Disposition::unavailable &&
                    result.reason == MigrationReason::resource_limit && !result.witness &&
                    result.terminal_query == stops[i] && result.attained.has_value(),
                QString("One-less pre-open resource limit at query %1, dimension %2")
                    .arg(static_cast<int>(stops[i]))
                    .arg(i));
    }
}

constexpr std::array guard_stops{Query::legacy_guard, Query::legacy_guard, Query::legacy_guard,
                                 Query::legacy_guard};
constexpr std::array blank_stops{Query::foreign_keys, Query::schema, Query::integrity,
                                 Query::integrity};
constexpr std::array orphan_stops{Query::foreign_keys, Query::schema, Query::foreign_keys,
                                  Query::foreign_keys};
constexpr std::array history_stops{Query::history, Query::schema, Query::history, Query::history};

void probe_refusal(const DisposableFile &file, MigrationReason expected, const Charge &charge,
                   Query terminal, const std::array<Query, 4> &stops) {
    const auto result = probe_stopped(file, profile(charge));
    require(result.disposition == Disposition::unavailable && result.reason == expected &&
                !result.witness,
            QString("Read-only pre-open refuses at the expected inspection gate, reason %1")
                .arg(static_cast<int>(result.reason)));
    expect_probe_ledger(result, charge, terminal);
    probe_one_less(file, charge, stops);
}

void inspect_old(const DisposableFile &file, bool ready) {
    {
        SqliteWriter owner;
        require(owner.open(file.path(), profile(old_inspect)) == StorageError::none,
                "Independent normal v21 inspection opens");
        expect_current(migrate(owner, request(MigrationMode::inspect,
                                              milestone_slot_interruption_repository_manifest())),
                       21, 0);
    }
    SqliteWriter owner;
    require(owner.open(file.path(), setup_profile) == StorageError::none,
            "Independent protected reader work opens");
    ProtectedPredecessor::verify(owner, ready);
}

void inspect_successor(const DisposableFile &file, const MigrationManifest &manifest, bool ready) {
    {
        SqliteWriter owner;
        require(owner.open(file.path(), profile(successor_inspect)) == StorageError::none,
                "Independent normal v22 inspection opens");
        expect_current(migrate(owner, request(MigrationMode::inspect, manifest)), 22, 0);
    }
    SqliteWriter owner;
    require(owner.open(file.path(), setup_profile) == StorageError::none,
            "Independent selected v22 owner read opens");
    expect_commit(qt_test_support::run_work(
        owner,
        [ready](auto context) {
            const auto session = context.begin();
            expect_rows(session.execute({empty_additions, {}}), {{integer(1)}},
                        "Both new tables are empty");
            expect_rows(
                session.execute({select_history, {}}), expected_history(22),
                "Exact ordered v21 prefix and independently fixed v22 INTEGER/TEXT/BLOB row");
            const auto state = SqliteApplicationBundleStore(session).read_state();
            if (ready) {
                require(state.has_value(), "Slotless predecessor remains readable by its owner");
                expect_state(*state, true);
                expect_rows(session.execute({select_state, {}}), {ready_state}, "Only Ready A");
                expect_rows(session.execute({select_head, {}}), initial_head, "Exact legacy head");
                expect_rows(session.execute({select_configuration, {}}), initial_configuration,
                            "Exact legacy configuration");
                verify_k_children(session);
            } else {
                require(!state, "Fresh v22 has no application state");
                expect_rows(session.execute({select_state, {}}), {}, "Fresh v22 state is empty");
                expect_rows(session.execute({select_head, {}}), {}, "Fresh v22 head is empty");
            }
            session.commit();
            return 1;
        },
        10000));
}

void advance_current(const DisposableFile &file, const MigrationManifest &manifest,
                     const detail::ClockPreOpenWitness &witness, StorageLimits limits) {
    SqliteWriter owner;
    require(owner.open(file.path(), limits) == StorageError::none,
            "Open the owner after pre-open and matching backup");
    expect_guarded_v22_advance(owner, manifest, witness);
}

const detail::ClockPreOpenWitness ready_witness{21, Predecessor::ready, "A", "K"};
const detail::ClockPreOpenWitness active_witness{21, Predecessor::active, "A", "K"};
const detail::ClockPreOpenWitness blank_witness{0, Predecessor::blank, {}, {}};

} // namespace

class Storage003CatalogTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void compiledCatalog() {
        check([] {
            verify_manifest(milestone_slot_interruption_repository_manifest(), 21);
            (void)compiled_successor();
        });
    }

    void readyUpgrade() {
        check([] {
            ProtectedPredecessor predecessor(true);
            const auto witness = candidate(predecessor.file, ready_witness);
            DisposableFile backup;
            predecessor.file.copy_to(backup);
            const auto manifest = compiled_successor();
            advance_current(predecessor.file, manifest, witness, profile(ready_advance));
            inspect_successor(predecessor.file, manifest, true);
            inspect_old(backup, true);
        });
    }

    void blankUpgrade() {
        check([] {
            DisposableFile file;
            file.create_blank();
            const auto witness = candidate(file, blank_witness);
            DisposableFile backup;
            file.copy_to(backup);
            const auto manifest = compiled_successor();
            advance_current(file, manifest, witness, profile(blank_advance));
            inspect_successor(file, manifest, false);
        });
    }

    void healthyProbeOneLess() {
        check([] {
            DisposableFile blank;
            blank.create_blank();
            (void)candidate(blank, blank_witness);
            probe_one_less(blank, blank_probe, blank_stops);
            ProtectedPredecessor ready(true);
            (void)candidate(ready.file, ready_witness);
            probe_one_less(ready.file, ready_probe, guard_stops);
        });
    }

    void advanceBudgetOneLess_data() {
        QTest::addColumn<bool>("ready");
        QTest::addColumn<int>("dimension");
        const auto dimensions = {"attempts", "peak-sql", "rows", "value-bytes", "binding-bytes"};
        for (const bool ready : {false, true}) {
            int i = 0;
            for (const auto *dimension : dimensions) {
                const auto tag = QByteArray(ready ? "ready-" : "blank-") + dimension;
                QTest::newRow(tag.constData()) << ready << i++;
            }
        }
    }

    void advanceBudgetOneLess() {
        QFETCH(bool, ready);
        QFETCH(int, dimension);
        check([=] {
            auto run = [&](const DisposableFile &file, const detail::ClockPreOpenWitness &basis,
                           const Charge &charge) {
                const auto witness = candidate(file, basis);
                DisposableFile backup;
                file.copy_to(backup);
                const auto manifest = compiled_successor();
                auto limits = profile(charge);
                --(limits.*metered_limits[dimension]);
                {
                    SqliteWriter owner;
                    require(owner.open(file.path(), limits) == StorageError::none,
                            "Open with just one charged dimension reduced");
                    auto migration = request(MigrationMode::advance, manifest);
                    const auto observed = observe_v22(migration);
                    const auto result = advance(owner, std::move(migration), witness);
                    const auto diagnostic = observed->diagnose(result.migration);
                    if (metered_limits[dimension] != &StorageLimits::statement_utf8_bytes)
                        require(observed->reached_history(),
                                "Expected before_history(22,15) before the reduced budget stop" +
                                    diagnostic);
                    expect_migration_rollback(result.migration, MigrationReason::resource_limit,
                                              diagnostic);
                }
                (void)candidate(file, basis);
            };
            if (ready) {
                ProtectedPredecessor predecessor(true);
                run(predecessor.file, ready_witness, ready_advance);
                inspect_old(predecessor.file, true);
            } else {
                DisposableFile file;
                file.create_blank();
                run(file, blank_witness, blank_advance);
            }
        });
    }

    void activePreOpenRefusal() {
        check([] {
            ProtectedPredecessor predecessor(false);
            const auto result = probe_stopped(predecessor.file, profile(ready_probe));
            require(result.disposition == Disposition::active_refused &&
                        result.witness == active_witness,
                    QString("Active 114A refusal before writer open, reason %1")
                        .arg(static_cast<int>(result.reason)));
            expect_probe_ledger(result, ready_probe, Query::legacy_guard);
            probe_one_less(predecessor.file, ready_probe, guard_stops);
            inspect_old(predecessor.file, false);
        });
    }

    void inUnitRefusal_data() {
        QTest::addColumn<bool>("actual_active");
        QTest::addColumn<int>("dimension");
        QTest::newRow("stale-configuration-echo") << false << -1;
        QTest::newRow("active-at-recheck") << true << -1;
        QTest::newRow("guard-attempt-limit") << false << 0;
        QTest::newRow("guard-sql-limit") << false << 1;
        QTest::newRow("guard-row-limit") << false << 2;
        QTest::newRow("guard-value-limit") << false << 3;
    }

    void inUnitRefusal() {
        QFETCH(bool, actual_active);
        QFETCH(int, dimension);
        check([=] {
            ProtectedPredecessor probe_origin(true);
            auto witness = candidate(probe_origin.file, ready_witness);
            ProtectedPredecessor selected(!actual_active);
            if (!actual_active)
                witness.configuration = "Z";
            const auto manifest = compiled_successor();
            auto calls = std::make_shared<QList<MigrationProgress>>();
            auto guarded = request(MigrationMode::advance, manifest);
            guarded.cancel = [calls](const auto &stage) {
                calls->append(stage);
                return false;
            };
            {
                SqliteWriter owner;
                auto limits = profile(ready_probe);
                if (dimension >= 0)
                    --(limits.*metered_limits[dimension]);
                require(owner.open(selected.file.path(), limits) == StorageError::none,
                        "Recheck on the separately isolated owner file");
                const auto result = advance(owner, std::move(guarded), witness);
                require(result.terminal_preflight_query == Query::legacy_guard &&
                            result.attained_preflight.has_value(),
                        "Actual in-unit inspection reaches the typed guard query");
                if (dimension >= 0) {
                    expect_migration_rollback(result.migration, MigrationReason::resource_limit);
                    require(result.refusal == detail::ClockAdvanceRefusal::none,
                            "Budget stop precedes the typed guard refusal");
                } else {
                    require(result.refusal == (actual_active
                                                   ? detail::ClockAdvanceRefusal::active
                                                   : detail::ClockAdvanceRefusal::changed_basis) &&
                                result.migration.work.transaction ==
                                    TransactionOutcome::rolled_back &&
                                !result.migration.report && !result.migration.work.value,
                            "Actual 114A in-unit refusal, rollback and no report");
                    require(result.attained_preflight == ready_probe,
                            "Exact independent eight-field ledger through the in-unit guard");
                }
                require(calls->size() == 1 &&
                            calls->front().checkpoint == MigrationCheckpoint::before_begin,
                        "No DDL or history checkpoint can run before 114A");
            }
            inspect_old(selected.file, !actual_active);
        });
    }

    void missingK() {
        check([] {
            ProtectedPredecessor predecessor(true);
            DisposableFile copy;
            predecessor.file.copy_to(copy);
            inspect_old(copy, true);
            {
                ConstraintSpecimen preparation(copy);
                preparation.make_k_orphan();
            }
            const auto orphan = copy.seal();
            {
                ConstraintSpecimen independent(copy, ConstraintSpecimen::Open::read_only);
                independent.verify_k_orphan();
            }
            require(copy.seal() == orphan, "Independent FK witness leaves orphan sealed");
            probe_refusal(copy, MigrationReason::integrity_failed, missing_k, Query::foreign_keys,
                          orphan_stops);
            inspect_old(predecessor.file, true);
        });
    }

    void damagedPredecessor_data() {
        QTest::addColumn<bool>("digest");
        QTest::newRow("marker") << false;
        QTest::newRow("digest") << true;
    }

    void damagedPredecessor() {
        QFETCH(bool, digest);
        check([=] {
            ProtectedPredecessor predecessor(true);
            DisposableFile copy;
            predecessor.file.copy_to(copy);
            {
                ConstraintSpecimen preparation(copy);
                preparation.drift(digest ? ConstraintSpecimen::Drift::digest21
                                         : ConstraintSpecimen::Drift::marker21);
            }
            probe_refusal(copy, MigrationReason::catalog_mismatch,
                          digest ? wrong_digest_probe : wrong_marker_probe, Query::history,
                          history_stops);
            inspect_old(predecessor.file, true);
        });
    }

    void cancelAfterAllDdl() {
        check([] {
            ProtectedPredecessor predecessor(true);
            const auto witness = candidate(predecessor.file, ready_witness);
            DisposableFile backup;
            predecessor.file.copy_to(backup);
            const auto manifest = compiled_successor();
            auto migration = request(MigrationMode::advance, manifest);
            const auto observed = observe_v22(migration, true);
            {
                SqliteWriter owner;
                require(owner.open(predecessor.file.path(), profile(ready_cancel)) ==
                            StorageError::none,
                        "Open the owner with the exact cancel-path profile");
                const auto result = advance(owner, std::move(migration), witness);
                const auto diagnostic = observed->diagnose(result.migration);
                require(
                    observed->statements() == v22_statement_order && observed->reached_history(),
                    "Expected v22 statements 1..15 and before_history(22,15) before cancellation" +
                        diagnostic);
                expect_migration_rollback(result.migration, MigrationReason::cancelled, diagnostic);
                require(observed->statements() ==
                            QList<qsizetype>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
                        "All 15 DDL attempts precede the cancellation boundary");
            }
            inspect_old(predecessor.file, true);
            inspect_old(backup, true);
        });
    }

    void damagedSuccessorAndOldReader() {
        check([] {
            ProtectedPredecessor predecessor(true);
            const auto witness = candidate(predecessor.file, ready_witness);
            DisposableFile backup;
            predecessor.file.copy_to(backup);
            const auto manifest = compiled_successor();
            advance_current(predecessor.file, manifest, witness, profile(ready_advance));
            inspect_successor(predecessor.file, manifest, true);
            {
                SqliteWriter older;
                require(older.open(predecessor.file.path(), profile(successor_history_stop)) ==
                            StorageError::none,
                        "Open independent v21 consumer against exact v22 file");
                expect_migration_rollback(
                    migrate(older, request(MigrationMode::inspect,
                                           milestone_slot_interruption_repository_manifest())),
                    MigrationReason::unsupported_version);
            }
            for (const auto drift :
                 {ConstraintSpecimen::Drift::index22, ConstraintSpecimen::Drift::digest22}) {
                DisposableFile copy;
                predecessor.file.copy_to(copy);
                {
                    ConstraintSpecimen preparation(copy);
                    preparation.drift(drift);
                }
                SqliteWriter owner;
                const bool index = drift == ConstraintSpecimen::Drift::index22;
                require(owner.open(copy.path(), profile(index ? successor_index_stop
                                                              : successor_history_stop)) ==
                            StorageError::none,
                        "Reopen the independently damaged successor");
                expect_migration_rollback(migrate(owner, request(MigrationMode::inspect, manifest)),
                                          index ? MigrationReason::schema_mismatch
                                                : MigrationReason::catalog_mismatch);
            }
            DisposableFile restored;
            backup.copy_to(restored);
            inspect_old(restored, true);
        });
    }

    void ordinaryWorkRefusesCompiledTrigger() {
        check([] {
            ProtectedPredecessor predecessor(true);
            const auto exact = compiled_successor().steps.back().sql[10];
            {
                SqliteWriter owner;
                require(owner.open(predecessor.file.path(), setup_profile) == StorageError::none,
                        "Ordinary work opens the protected v21 writer");
                const auto work = qt_test_support::run_work(
                    owner,
                    [exact](auto context) {
                        const auto session = context.begin();
                        (void)session.execute({exact, {}});
                        session.commit();
                        return 1;
                    },
                    10000);
                require(
                    work.transaction == TransactionOutcome::rolled_back &&
                        work.error == WorkError::invalid_statement && !work.value,
                    "Ordinary work refuses the exact compiled trigger with acknowledged rollback");
            }
            inspect_old(predecessor.file, true);
        });
    }

    void guardedRejectsDivergentManifest_data() {
        QTest::addColumn<bool>("paired");
        QTest::newRow("statement-and-projection") << true;
        QTest::newRow("projection-only") << false;
    }

    void guardedRejectsDivergentManifest() {
        QFETCH(bool, paired);
        check([=] {
            ProtectedPredecessor predecessor(true);
            const auto witness = candidate(predecessor.file, ready_witness);
            const auto seal = predecessor.file.seal();
            auto manifest = compiled_successor();
            auto &step = manifest.steps.back();
            const auto exact = step.sql[10];
            auto divergent = exact;
            divergent.replace(" BEGIN ", "  BEGIN ");
            require(divergent != exact, "The inert whitespace divergence changes exact SQL bytes");
            qsizetype changed = 0;
            for (auto &object : step.expected_schema) {
                if (object.sql == exact) {
                    object.sql = divergent;
                    ++changed;
                }
            }
            require(changed == 1, "Change exactly the matching trigger projection");
            if (paired)
                step.sql[10] = divergent;

            const auto completed = std::make_shared<bool>(false);
            const auto drained = std::make_shared<bool>(false);
            MigrationAdmission admission;
            {
                SqliteWriter owner;
                require(owner.open(predecessor.file.path(), setup_profile) == StorageError::none,
                        "Guarded admission uses the real protected v21 writer");
                admission = detail::enqueue_clock_checkpoint_advance(
                    owner, request(MigrationMode::advance, std::move(manifest)), witness,
                    [completed](auto) { *completed = true; });
                require(owner.shutdown([drained] { *drained = true; }) == StorageError::none &&
                            QTest::qWaitFor([drained] { return *drained; }, 10000),
                        "Writer drains and closes before checking completion and file seal");
            }
            require(
                admission.error == StorageError::invalid_options &&
                    admission.reason == MigrationReason::invalid_manifest && !*completed,
                "Divergent manifest refuses before enqueue and yields no callback through drain");
            require(predecessor.file.seal() == seal,
                    "Divergent manifest leaves the exact v21 seal");
        });
    }

    void publicMigrationCannotUseCompiledTriggerAdmission() {
        check([] {
            ProtectedPredecessor predecessor(false);
            auto migration = request(MigrationMode::advance, compiled_successor());
            const auto observed = observe_v22(migration);
            {
                SqliteWriter owner;
                require(owner.open(predecessor.file.path(), setup_profile) == StorageError::none,
                        "Public migration opens the real Active v21 writer");
                const auto result = migrate(owner, std::move(migration));
                require(observed->statements() == v22_statement_order.first(11) &&
                            observed->events.size() == 11 &&
                            result.work.error == WorkError::invalid_statement,
                        "Public migration must stop at the protected trigger rejection" +
                            observed->diagnose(result));
                expect_migration_rollback(result, MigrationReason::sql_failure,
                                          observed->diagnose(result));
            }
            inspect_old(predecessor.file, false);
        });
    }
};

QTEST_GUILESS_MAIN(Storage003CatalogTests)
#include "catalog_tests.moc"
