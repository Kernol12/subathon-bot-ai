// SPDX-License-Identifier: GPL-3.0-or-later
#include "constraint_specimen.hpp"

namespace {
using namespace storage_003_fixture;

enum class Admission { batch, work, advance };

void authenticate_drift(const DisposableFile &file, bool index_drift) {
    SqliteReader reader;
    require(reader.open(file.path()) == StorageError::none, "Open an independent drift reader");

    auto history = expected_history(22);
    if (!index_drift) {
        auto digest = history.back().back().toByteArray();
        digest[0] = char(static_cast<unsigned char>(digest.at(0)) ^ 1);
        history.back().back() = blob(digest);
    }
    const auto actual_history = reader.read({select_history, {}}, 23);
    require(actual_history.error == StorageError::none, "Read the complete drift history");
    expect_rows(actual_history.rows, history, "Exact production prefix and selected v22 digest");

    const auto index =
        reader.read({"SELECT tbl_name,sql FROM main.sqlite_schema WHERE type='index' "
                     "AND name='subathon_test_v22_drift' LIMIT 2",
                     {}},
                    2);
    require(index.error == StorageError::none, "Read the independently added index");
    const SqlRows expected_index{
        {QStringLiteral("subathon_clock_evidence"),
         QStringLiteral("CREATE INDEX subathon_test_v22_drift ON subathon_clock_evidence(kind)")}};
    expect_rows(index.rows, index_drift ? expected_index : SqlRows{},
                "Only the index-drift specimen has the added index");
}

struct Attempt {
    bool body{false};
    bool completion{false};
    bool drained{false};
};
} // namespace

class Storage003W1OpenAdmissionTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void inspectionOnlyAdmission_data() {
        QTest::addColumn<bool>("index_drift");
        QTest::addColumn<int>("entry");
        QTest::newRow("index-batch") << true << int(Admission::batch);
        QTest::newRow("digest-batch") << false << int(Admission::batch);
        QTest::newRow("index-work") << true << int(Admission::work);
        QTest::newRow("digest-work") << false << int(Admission::work);
        QTest::newRow("index-advance") << true << int(Admission::advance);
        QTest::newRow("digest-advance") << false << int(Admission::advance);
    }

    void inspectionOnlyAdmission() {
        QFETCH(bool, index_drift);
        QFETCH(int, entry);
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
            authenticate_drift(file, index_drift);
            const auto seal = file.seal();

            const auto reason =
                index_drift ? MigrationReason::schema_mismatch : MigrationReason::catalog_mismatch;
            const auto charge = index_drift ? successor_index_stop : successor_history_stop;
            const auto attempt = std::make_shared<Attempt>();
            MigrationAdmission admission{StorageError::none, MigrationReason::none};
            StorageError expected = StorageError::none;
            {
                SqliteWriter owner;
                require(owner.open(file.path(), profile(charge)) == StorageError::none,
                        "Drifted v22 opens for catalog diagnosis");
                expect_migration_rollback(migrate(owner, request(MigrationMode::inspect, manifest)),
                                          reason);

                switch (static_cast<Admission>(entry)) {
                case Admission::batch:
                    expected = StorageError::invalid_batch;
                    admission.error = owner.enqueue(
                        {{"SELECT 1", {}}}, [attempt](auto) { attempt->completion = true; });
                    break;
                case Admission::work:
                    expected = StorageError::invalid_work;
                    admission.error = owner.enqueue_work(
                        [attempt](auto context) {
                            attempt->body = true;
                            const auto session = context.begin();
                            session.rollback();
                            return 0;
                        },
                        [attempt](auto) { attempt->completion = true; });
                    break;
                case Admission::advance:
                    expected = StorageError::invalid_options;
                    admission = SqliteMigrationRunner::enqueue(
                        owner, request(MigrationMode::advance, manifest),
                        [attempt](auto) { attempt->completion = true; });
                    break;
                }

                require(owner.shutdown([attempt] { attempt->drained = true; }) ==
                                StorageError::none &&
                            QTest::qWaitFor([attempt] { return attempt->drained; }, 10000),
                        "Shutdown drains the writer before checking admission and callbacks");
            }
            require(file.seal() == seal, "Before-open and after-shutdown database seals match");
            require(admission.error == expected,
                    QString("Inspection-only synchronous admission: expected %1, got %2")
                        .arg(static_cast<int>(expected))
                        .arg(static_cast<int>(admission.error)));
            if (static_cast<Admission>(entry) == Admission::advance)
                require(admission.reason == reason, "Advance returns the precise drift reason");
            require(!attempt->body && !attempt->completion,
                    "Rejected admission invokes neither work body nor completion");
        });
    }
};

QTEST_GUILESS_MAIN(Storage003W1OpenAdmissionTests)
#include "w1_open_admission_tests.moc"
