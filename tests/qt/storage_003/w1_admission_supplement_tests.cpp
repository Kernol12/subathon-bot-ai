// SPDX-License-Identifier: GPL-3.0-or-later
#include "constraint_specimen.hpp"

namespace {
using namespace storage_003_fixture;

void expect_admission(MigrationAdmission actual, StorageError error, MigrationReason reason,
                      const QString &subject) {
    require(actual.error == error && actual.reason == reason,
            QString("%1: expected storage error %2 and reason %3, got %4 and %5")
                .arg(subject)
                .arg(static_cast<int>(error))
                .arg(static_cast<int>(reason))
                .arg(static_cast<int>(actual.error))
                .arg(static_cast<int>(actual.reason)));
}

struct Observation {
    bool generic_completion{false};
    bool guarded_completion{false};
    bool migration_body{false};
    bool drained{false};
};
} // namespace

class Storage003W1AdmissionSupplementTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void guardedAdvanceAdmission_data() {
        QTest::addColumn<bool>("index_drift");
        QTest::newRow("index") << true;
        QTest::newRow("digest") << false;
    }

    void guardedAdvanceAdmission() {
        QFETCH(bool, index_drift);
        check([=] {
            const auto manifest = compiled_successor();
            DisposableFile source;
            source.create_blank();
            const auto witness = candidate(source, {0, detail::ClockPredecessor::blank, {}, {}});
            DisposableFile backup;
            source.copy_to(backup);
            {
                SqliteWriter owner;
                require(owner.open(source.path(), profile(blank_advance)) == StorageError::none,
                        "Open the admitted blank owner");
                expect_guarded_v22_advance(owner, manifest, witness);
            }
            {
                SqliteWriter owner;
                require(owner.open(source.path(), profile(successor_inspect)) == StorageError::none,
                        "Reopen the intact successor for exact catalog inspection");
                expect_current(migrate(owner, request(MigrationMode::inspect, manifest)), 22, 0);
            }

            DisposableFile file;
            source.copy_to(file);
            {
                ConstraintSpecimen specimen(file);
                specimen.drift(index_drift ? ConstraintSpecimen::Drift::index22
                                           : ConstraintSpecimen::Drift::digest22);
            }
            const auto seal = file.seal();
            const auto reason =
                index_drift ? MigrationReason::schema_mismatch : MigrationReason::catalog_mismatch;
            const auto charge = index_drift ? successor_index_stop : successor_history_stop;
            SqliteWriter owner;
            require(owner.open(file.path(), profile(charge)) == StorageError::none,
                    "Drifted v22 opens for catalog diagnosis");
            expect_migration_rollback(migrate(owner, request(MigrationMode::inspect, manifest)),
                                      reason);

            const auto observed = std::make_shared<Observation>();
            auto command = request(MigrationMode::advance, manifest);
            command.cancel = [observed](auto) {
                observed->migration_body = true;
                return false;
            };
            auto invalid = command;
            // Keep the exact guarded manifest and witness so the shared runner
            // reaches manifest preflight and refuses its one-byte input budget.
            invalid.limits.manifest_utf8_bytes = 1;

            const auto generic_done = [observed](auto) { observed->generic_completion = true; };
            const auto guarded_done = [observed](auto) { observed->guarded_completion = true; };
            const auto generic_invalid =
                SqliteMigrationRunner::enqueue(owner, invalid, generic_done);
            const auto guarded_invalid =
                detail::enqueue_clock_checkpoint_advance(owner, invalid, witness, guarded_done);
            const auto generic_invalid_missing = SqliteMigrationRunner::enqueue(owner, invalid, {});
            const auto guarded_invalid_missing =
                detail::enqueue_clock_checkpoint_advance(owner, invalid, witness, {});
            const auto generic_missing = SqliteMigrationRunner::enqueue(owner, command, {});
            const auto guarded_missing =
                detail::enqueue_clock_checkpoint_advance(owner, command, witness, {});
            const auto guarded =
                detail::enqueue_clock_checkpoint_advance(owner, command, witness, guarded_done);

            require(owner.shutdown([observed] { observed->drained = true; }) ==
                            StorageError::none &&
                        QTest::qWaitFor([observed] { return observed->drained; }, 10000),
                    "Shutdown drains the writer before checking callbacks and the stopped seal");
            require(file.seal() == seal, "Before-open and after-shutdown database seals match");

            expect_admission(generic_invalid, StorageError::invalid_options,
                             MigrationReason::invalid_manifest, "Generic manifest preflight");
            expect_admission(guarded_invalid, StorageError::invalid_options,
                             MigrationReason::invalid_manifest, "Guarded manifest preflight");
            expect_admission(generic_invalid_missing, StorageError::invalid_options,
                             MigrationReason::invalid_manifest,
                             "Generic preflight precedes missing completion");
            expect_admission(guarded_invalid_missing, StorageError::invalid_options,
                             MigrationReason::invalid_manifest,
                             "Guarded preflight precedes missing completion");
            expect_admission(generic_missing, StorageError::invalid_work, MigrationReason::none,
                             "Generic missing completion precedes writer state");
            expect_admission(guarded_missing, StorageError::invalid_work, MigrationReason::none,
                             "Guarded missing completion precedes writer state");
            expect_admission(guarded, StorageError::invalid_options, reason,
                             "Guarded advance refuses synchronously with the recognition reason");
            require(!observed->migration_body && !observed->generic_completion &&
                        !observed->guarded_completion,
                    "Refused admissions reach no migration checkpoint or completion");
        });
    }
};

QTEST_GUILESS_MAIN(Storage003W1AdmissionSupplementTests)
#include "w1_admission_supplement_tests.moc"
