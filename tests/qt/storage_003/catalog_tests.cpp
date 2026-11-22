// SPDX-License-Identifier: GPL-3.0-or-later
#include "constraint_specimen.hpp"

namespace {
using namespace storage_003_fixture;
using Disposition = detail::ClockPreOpenResult::Disposition;
using Predecessor = detail::ClockPredecessor;

void expect_unchanged_probe(const DisposableFile &file, const QByteArray &seal) {
    require(file.seal() == seal, "Read-only pre-open leaves the stopped main file unchanged");
}

detail::ClockPreOpenWitness candidate(const DisposableFile &file,
                                      const detail::ClockPreOpenWitness &expected) {
    const auto before = file.seal();
    const auto result = detail::probe_clock_checkpoint_predecessor(file.path());
    expect_unchanged_probe(file, before);
    require(result.disposition == Disposition::candidate &&
                result.reason == MigrationReason::none && result.witness == expected,
            QString("Read-only pre-open candidate and exact basis, reason %1")
                .arg(static_cast<int>(result.reason)));
    return *result.witness;
}

void probe_refusal(const DisposableFile &file, MigrationReason expected) {
    const auto before = file.seal();
    const auto result = detail::probe_clock_checkpoint_predecessor(file.path());
    expect_unchanged_probe(file, before);
    require(result.disposition == Disposition::unavailable && result.reason == expected &&
                !result.witness,
            QString("Read-only pre-open refuses at the expected inspection gate, reason %1")
                .arg(static_cast<int>(result.reason)));
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

inline constexpr auto empty_additions =
    "SELECT CASE WHEN NOT EXISTS(SELECT 1 FROM main.subathon_clock_checkpoint) AND "
    "NOT EXISTS(SELECT 1 FROM main.subathon_clock_evidence) THEN 1 ELSE 0 END";

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
    const auto result = advance(owner, request(MigrationMode::advance, manifest), witness);
    require(result.refusal == detail::ClockAdvanceRefusal::none, "Admitted unchanged basis");
    expect_current(result.migration, 22, 22 - witness.version);
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

    void blankBudgetOneLess_data() {
        QTest::addColumn<int>("dimension");
        QTest::newRow("attempts") << 0;
        QTest::newRow("peak-sql") << 1;
        QTest::newRow("binding-bytes") << 2;
        QTest::newRow("rows") << 3;
        QTest::newRow("value-bytes") << 4;
    }

    void blankBudgetOneLess() {
        QFETCH(int, dimension);
        check([=] {
            DisposableFile file;
            file.create_blank();
            const auto witness = candidate(file, blank_witness);
            DisposableFile backup;
            file.copy_to(backup);
            const auto manifest = compiled_successor();
            auto limits = profile(blank_advance);
            const std::array dimensions{
                &StorageLimits::statements_per_batch, &StorageLimits::statement_utf8_bytes,
                &StorageLimits::bound_value_bytes, &StorageLimits::result_rows,
                &StorageLimits::result_value_bytes};
            --(limits.*dimensions[dimension]);
            {
                SqliteWriter owner;
                require(owner.open(file.path(), limits) == StorageError::none,
                        "Open with just one charged dimension reduced");
                const auto result =
                    advance(owner, request(MigrationMode::advance, manifest), witness);
                expect_migration_rollback(result.migration, MigrationReason::resource_limit);
            }
            (void)candidate(file, blank_witness);
        });
    }

    void activePreOpenRefusal() {
        check([] {
            ProtectedPredecessor predecessor(false);
            const auto before = predecessor.file.seal();
            const auto result = detail::probe_clock_checkpoint_predecessor(predecessor.file.path());
            expect_unchanged_probe(predecessor.file, before);
            require(result.disposition == Disposition::active_refused &&
                        result.witness == active_witness,
                    QString("Active 114A refusal before writer open, reason %1")
                        .arg(static_cast<int>(result.reason)));
            inspect_old(predecessor.file, false);
        });
    }

    void inUnitRefusal_data() {
        QTest::addColumn<bool>("actual_active");
        QTest::newRow("stale-configuration-echo") << false;
        QTest::newRow("active-at-recheck") << true;
    }

    void inUnitRefusal() {
        QFETCH(bool, actual_active);
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
                require(owner.open(selected.file.path(), profile(ready_probe)) ==
                            StorageError::none,
                        "Recheck on the separately isolated owner file");
                const auto result = advance(owner, std::move(guarded), witness);
                require(result.refusal == (actual_active
                                               ? detail::ClockAdvanceRefusal::active
                                               : detail::ClockAdvanceRefusal::changed_basis) &&
                            result.migration.work.transaction == TransactionOutcome::rolled_back &&
                            !result.migration.report && !result.migration.work.value,
                        "Actual 114A in-unit refusal, rollback and no report");
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
            probe_refusal(copy, MigrationReason::integrity_failed);
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
            probe_refusal(copy, MigrationReason::catalog_mismatch);
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
            auto seen = std::make_shared<QList<qsizetype>>();
            migration.cancel = [seen](const auto &progress) {
                if (progress.version != 22)
                    return false;
                if (progress.checkpoint == MigrationCheckpoint::before_statement)
                    seen->append(progress.statement);
                return progress.checkpoint == MigrationCheckpoint::before_history &&
                       progress.statement == 15;
            };
            {
                SqliteWriter owner;
                require(owner.open(predecessor.file.path(), profile(ready_cancel)) ==
                            StorageError::none,
                        "Open the owner with the exact cancel-path profile");
                const auto result = advance(owner, std::move(migration), witness);
                expect_migration_rollback(result.migration, MigrationReason::cancelled);
                require(*seen ==
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
                require(older.open(predecessor.file.path(), setup_profile) == StorageError::none,
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
                require(owner.open(copy.path(), setup_profile) == StorageError::none,
                        "Reopen the independently damaged successor");
                expect_migration_rollback(migrate(owner, request(MigrationMode::inspect, manifest)),
                                          drift == ConstraintSpecimen::Drift::index22
                                              ? MigrationReason::schema_mismatch
                                              : MigrationReason::catalog_mismatch);
            }
            DisposableFile restored;
            backup.copy_to(restored);
            inspect_old(restored, true);
        });
    }
};

QTEST_GUILESS_MAIN(Storage003CatalogTests)
#include "catalog_tests.moc"
