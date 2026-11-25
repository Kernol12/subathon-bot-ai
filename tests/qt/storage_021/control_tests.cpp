// SPDX-License-Identifier: GPL-3.0-or-later
#include "../storage_003/constraint_specimen.hpp"
#include "contribution_review_writer_access.hpp"
#include "fake_ops02_update_hold.hpp"

#include <QCoreApplication>
#include <QScopeGuard>
#include <QTest>

#include <thread>

namespace subathon::server::detail {

// A dependent call picks up the proposed private overload through ADL when
// declared in the private header. The candidate can run without that overload.
struct V23InspectionTestPath {
    const QString &path;
    operator const QString &() const { return path; }
};

} // namespace subathon::server::detail

namespace {
using namespace storage_003_fixture;
using namespace subathon::server::detail;
using subathon::server::detail::testing::FakeOps02UpdateHoldIssuer;

constexpr MigrationAdmission invalid{StorageError::invalid_options,
                                     MigrationReason::invalid_manifest};

void expect_admission(MigrationAdmission actual, MigrationAdmission expected) {
    require(actual.error == expected.error && actual.reason == expected.reason,
            QString("Admission error/reason %1/%2, expected %3/%4")
                .arg(static_cast<int>(actual.error))
                .arg(static_cast<int>(actual.reason))
                .arg(static_cast<int>(expected.error))
                .arg(static_cast<int>(expected.reason)));
}

QByteArray main_digest(const QString &path) {
    QFile file(path);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    require(file.open(QIODevice::ReadOnly) && hash.addData(&file), "Hash disposable main file");
    return hash.result();
}

QByteArray wal_bytes(const QString &path) {
    QFile wal(path + "-wal");
    if (!wal.exists())
        return {};
    require(wal.open(QIODevice::ReadOnly), "Open disposable WAL for byte comparison");
    auto bytes = wal.readAll();
    require(wal.error() == QFileDevice::NoError && bytes.size() == wal.size(),
            "Read all disposable WAL bytes");
    return bytes;
}

struct FileState {
    QByteArray main;
    QByteArray wal;

    bool operator==(const FileState &) const = default;
};

class Fixture final {
  public:
    explicit Fixture(const QString &donor) {
        require(!QFileInfo::exists(donor + "-wal") && !QFileInfo::exists(donor + "-shm") &&
                    !QFileInfo::exists(donor + "-journal"),
                "Stopped donor has no sidecars");
        require(QFile::copy(donor, file.path()), "Copy stopped donor to disposable storage");
        require(file.seal().toHex() ==
                    "729cec12d8abada51bb92459f5bb1f8b4580a236c585be29cf6ce08669218d11",
                "Disposable copy matches independently pinned v22 donor");
        const auto before = file.seal();
        auto inspected = inspect_v22_pre_open(file.path(), setup_profile);
        require(std::holds_alternative<V22PreOpenWitness>(inspected) && file.seal() == before,
                "Authenticate v22 donor without changing its bytes");
        witness = std::get<V22PreOpenWitness>(std::move(inspected));
        open(writer);
    }

    void open(SqliteWriter &owner) const {
        require(owner.open(file.path(), setup_profile) == StorageError::none,
                "Open ordinary writer on disposable v22 donor");
    }

    // Queue rejection is the observable check that setup obtained a live reservation.
    V23StoppedWriterLease reserve() {
        auto lease = V23RebuildAccess::acquire(writer);
        require(writer.enqueue_work([](auto) { return QVariant{}; }, [](auto) {}) ==
                    StorageError::busy,
                "Idle owner obtained a live reservation");
        return lease;
    }

    FileState file_state() const { return {main_digest(file.path()), wal_bytes(file.path())}; }

    void require_unchanged(const FileState &before) const {
        require(file_state() == before && !QFileInfo::exists(file.path() + "-journal"),
                "Main and WAL bytes are unchanged and no rollback journal remains");
    }

    void reject(V23StoppedWriterLease lease, Ops02UpdateHold hold, MigrationAdmission expected,
                V23RebuildBudget budget = {}, bool null_completion = false) {
        const auto before = file_state();
        auto calls = std::make_shared<int>(0);
        SqliteMigrationRunner::Completion completion;
        if (!null_completion)
            completion = [calls](auto) { ++*calls; };
        expect_admission(enqueue_v23_rebuild(witness, std::move(hold), std::move(lease), budget, {},
                                             std::move(completion)),
                         expected);
        QCoreApplication::processEvents();
        require(*calls == 0, "Synchronous refusal delivers no completion");
        require_unchanged(before);
    }

    DisposableFile file;
    V22PreOpenWitness witness;
    SqliteWriter writer;
};

// Literals from the independently checked 609.9 step and projection witness.
constexpr auto v23_id = "exact-valuation-plan-revisions-rebuild-and-snapshots-v1";
constexpr auto v23_digest = "236e900671482ea7b73162b45c78b9bd576f46cdf20191414748bc7e4632de28";
constexpr auto v23_prefix = "cbadf8cc4def9035e315c94af7174f87e4abba3dfca4e7970523e0e1ca7376d9";

template <class Path> auto inspect_v23_candidate(Path path) {
    if constexpr (requires { inspect_v23_pre_open(path, setup_profile); }) {
        return inspect_v23_pre_open(path, setup_profile);
    } else {
        return std::variant<V22PreOpenWitness, MigrationReason>{
            MigrationReason::feature_not_implemented};
    }
}

void expect_v23_inspection(const DisposableFile &file, MigrationReason expected) {
    const auto before = file.seal();
    const auto path = file.path();
    const auto inspected = inspect_v23_candidate(V23InspectionTestPath{path});
    require(file.seal() == before && !QFileInfo::exists(path + "-journal"),
            "Private v23 inspection leaves the stopped database unchanged");
    if (const auto *reason = std::get_if<MigrationReason>(&inspected)) {
        require(expected != MigrationReason::none && *reason == expected,
                QString("Private v23 inspection reason %1, expected %2")
                    .arg(static_cast<int>(*reason))
                    .arg(static_cast<int>(expected)));
    } else {
        const auto &witness = std::get<0>(inspected);
        require(expected == MigrationReason::none && witness.version == 23 &&
                    witness.prefix == QByteArray::fromHex(v23_prefix) &&
                    witness.projection_objects == 294,
                "Private inspection returns the exact v23 identity only for the authentic store");
    }
}

SqlRows read_stopped(const DisposableFile &file, const QString &sql, qsizetype max_rows) {
    SqliteReader reader;
    require(reader.open(file.path()) == StorageError::none, "Open independent read-only reader");
    const auto result = reader.read({sql, {}}, max_rows);
    require(result.error == StorageError::none, "Read independent physical evidence");
    return result.rows;
}

SqlRows history23() {
    auto rows = expected_history(22);
    rows.append(
        SqlRow{integer(23), QString::fromLatin1(v23_id), blob(QByteArray::fromHex(v23_digest))});
    return rows;
}

void shutdown_writer(SqliteWriter &writer) {
    const auto stopped = std::make_shared<bool>(false);
    require(writer.shutdown([stopped] { *stopped = true; }) == StorageError::none &&
                QTest::qWaitFor([stopped] { return *stopped; }, 10000),
            "Writer closes before stopped-store inspection");
}

void commit_v23(Fixture &f) {
    require(f.witness.version == 22 && f.witness.prefix == QByteArray::fromHex(prefix22) &&
                f.witness.projection_objects == 292,
            "Exact independent v22 predecessor identity");
    const auto result = collect_migration<MigrationResult>([&](auto done) {
        return enqueue_v23_rebuild(f.witness, FakeOps02UpdateHoldIssuer::issue(), f.reserve(), {},
                                   {}, std::move(done));
    });
    require(result.reason == MigrationReason::none,
            QString("Ordinary private v23 owner failed to commit, reason %1")
                .arg(static_cast<int>(result.reason)));
    expect_current(result, 23, 1);
}

void authenticate_stopped_v23(const DisposableFile &file) {
    expect_rows(read_stopped(file, select_history, 24), history23(),
                "Complete ordered typed v23 history on a stopped store");
    expect_rows(read_stopped(file, "SELECT count(*) FROM subathon_plan_revisions", 1),
                {{integer(128)}}, "Populated donor retains all 128 plan revisions");
}

void commit_and_stop_v23(Fixture &f) {
    commit_v23(f);
    shutdown_writer(f.writer);
    authenticate_stopped_v23(f.file);
}

void require_not_v22(const DisposableFile &file) {
    const auto before = file.seal();
    const auto inspected = inspect_v22_pre_open(file.path(), setup_profile);
    const auto *reason = std::get_if<MigrationReason>(&inspected);
    require(reason && *reason == MigrationReason::unsupported_version && file.seal() == before,
            "Authentic v23 does not yield a v22 predecessor witness");
}

void normal_v23_reopen(Fixture &f) {
    commit_and_stop_v23(f);
    const auto before = f.file.seal();
    SqliteWriter reopened;
    require(reopened.open(f.file.path(), setup_profile) == StorageError::none,
            "Normal v23 writer opens");
    require(reopened.enqueue({{"CREATE TABLE subathon_test_reopen_probe(v INTEGER)", {}}},
                             [](auto) {}) == StorageError::invalid_batch,
            "Reopened writer enforces the governing SQL policy");
    const auto result = reopened.run_idle_owner_work([](auto context) {
        const auto session = context.begin();
        expect_rows(session.execute({"PRAGMA foreign_keys", {}}), {{integer(1)}},
                    "Normal reopen enables foreign keys");
        require(!CatalogWriterAccess::verified_v22(session), "Reopen does not classify as v22");
        session.commit();
        return 1;
    });
    expect_commit(result);
    expect_migration_rollback(
        migrate(reopened, request(MigrationMode::inspect, compiled_successor())),
        MigrationReason::unsupported_version);
    shutdown_writer(reopened);
    require(f.file.seal() == before, "Normal reopen and v22-only inspection preserve the store");
    require_not_v22(f.file);
}

enum class V23Drift { digest, projection, future };

MigrationReason prepare_v23_drift(const DisposableFile &file, V23Drift drift) {
    // Each stopped copy gets one alteration. Authenticate it independently of
    // both product inspection paths before checking the refusal.
    auto history = history23();
    if (drift == V23Drift::projection) {
        ConstraintSpecimen(file).drift(ConstraintSpecimen::Drift::index22);
        expect_rows(read_stopped(file,
                                 "SELECT tbl_name FROM sqlite_schema WHERE type='index' "
                                 "AND name='subathon_test_v22_drift'",
                                 2),
                    {{QStringLiteral("subathon_clock_evidence")}}, "Projection drift is present");
    } else {
        const auto name = QUuid::createUuid().toString(QUuid::WithoutBraces);
        {
            auto db = QSqlDatabase::addDatabase("QSQLITE", name);
            const auto remove = qScopeGuard([&] {
                db = {};
                QSqlDatabase::removeDatabase(name);
            });
            db.setDatabaseName(file.path());
            require(db.open(), "Open detached v23 history specimen");
            QSqlQuery query(db);
            if (drift == V23Drift::digest) {
                auto digest = QByteArray::fromHex(v23_digest);
                digest[0] = char(static_cast<unsigned char>(digest.at(0)) ^ 1);
                require(query.prepare("UPDATE subathon_migration_history SET content_sha256=? "
                                      "WHERE version=23"),
                        "Prepare v23 digest drift");
                query.addBindValue(digest);
                history.back().back() = blob(digest);
            } else {
                require(query.prepare("INSERT INTO subathon_migration_history VALUES(24,?,?)"),
                        "Prepare a structurally valid unknown head");
                const auto digest = QByteArray(32, 'F');
                query.addBindValue(QStringLiteral("unknown-future-head"));
                query.addBindValue(digest);
                history.append(
                    SqlRow{integer(24), QStringLiteral("unknown-future-head"), blob(digest)});
            }
            require(query.exec() && query.numRowsAffected() == 1, "Change exactly one history row");
        }
    }
    expect_rows(read_stopped(file, select_history, 25), history, "Exact altered history specimen");
    switch (drift) {
    case V23Drift::digest:
        return MigrationReason::catalog_mismatch;
    case V23Drift::projection:
        return MigrationReason::schema_mismatch;
    case V23Drift::future:
        return MigrationReason::unsupported_version;
    }
    Q_UNREACHABLE();
}

void refuse_v23_reopen(const DisposableFile &file, MigrationReason expected) {
    const auto before = file.seal();
    SqliteWriter owner;
    const auto opened = owner.open(file.path(), setup_profile);
    require(opened == StorageError::none || opened == StorageError::open_failed,
            "Normal open either refuses or permits inspection only");
    if (opened == StorageError::none) {
        const auto calls = std::make_shared<int>(0);
        const auto admission = SqliteMigrationRunner::enqueue(
            owner, request(MigrationMode::advance, compiled_successor()),
            [calls](auto) { ++*calls; });
        bool ran = false;
        const auto work = owner.run_idle_owner_work([&](auto) {
            ran = true;
            return 1;
        });
        shutdown_writer(owner);
        expect_admission(admission, {StorageError::invalid_options, expected});
        require(*calls == 0 && !ran && work.error == WorkError::protocol_error &&
                    work.transaction == TransactionOutcome::not_started && !work.value,
                "Unusable store runs no ordinary migration or idle work");
    }
    require(file.seal() == before && !QFileInfo::exists(file.path() + "-journal"),
            "Refused reopen leaves the stopped store unchanged");
}

auto attempt_w1(SqliteWriter &owner) {
    using Result = std::variant<SqliteBundleInitialDisposition, WorkError>;
    const auto staged = std::make_shared<std::optional<Result>>();
    const auto work = owner.run_idle_owner_work([staged](auto context) {
        using namespace subathon::core;
        const auto session = context.begin();
        const ApplicationStateSnapshot snapshot{
            "A", {"E", "active"},
            "W", {TimerLifecycle::Active, CountdownPhase::Main, 10000, 0, false},
            0,   {}};
        *staged = stage_clock_checkpoint_initial(
            session, {{RetainedKind::configuration, "W"}, "configuration", {}}, snapshot,
            {"I", "T", "L", {"S", "J"}, 1000, 0, slot_document, seed_root});
        // If a refusal accidentally left a pending write, commit on this
        // disposable store so the stopped-file check can observe it.
        session.commit();
        return 1;
    });
    expect_commit(work);
    require(staged->has_value(), "W1 staging body reached its typed result");
    return staged->value();
}

void w1_v22_positive(Fixture &f) {
    const auto staged = attempt_w1(f.writer);
    const auto *inserted = std::get_if<SqliteBundleInitialDisposition>(&staged);
    require(inserted && *inserted == SqliteBundleInitialDisposition::inserted,
            "Identical W1 input is valid and inserts on authentic v22");
    shutdown_writer(f.writer);
    expect_rows(read_stopped(f.file, select_head, 2), initial_head,
                "Positive control physically installed its initial head");
}

void w1_refuses_v23(Fixture &f) {
    commit_v23(f);
    const auto before = f.file_state();
    const auto staged = attempt_w1(f.writer);
    const auto *refusal = std::get_if<WorkError>(&staged);
    f.require_unchanged(before);
    shutdown_writer(f.writer);
    require(refusal && *refusal == WorkError::invalid_statement,
            "W1 refuses exact v23 with a type");
    authenticate_stopped_v23(f.file);
    expect_rows(read_stopped(f.file, select_head, 2), {}, "W1 leaves the head empty");
    expect_rows(read_stopped(f.file, "SELECT singleton FROM subathon_clock_checkpoint", 2), {},
                "W1 leaves the checkpoint empty");
    expect_rows(read_stopped(f.file, "SELECT revision FROM subathon_clock_evidence", 2), {},
                "W1 leaves the evidence empty");
    require_not_v22(f.file);
}

QByteArray cancelled_main_bytes(const QString &path) {
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "Open disposable main file for byte comparison");
    auto bytes = file.readAll();
    require(file.error() == QFileDevice::NoError && bytes.size() == file.size() &&
                bytes.size() >= 100,
            "Read the complete disposable main file and SQLite header");
    require(bytes.sliced(18, 2) == QByteArray::fromHex("0202"),
            "SQLite header read and write versions are both WAL");
    // The WAL/DELETE round trip may update the change counter, version-valid-for
    // number and SQLite library version. Compare all other bytes exactly.
    bytes.replace(24, 4, QByteArray(4, '\0'));
    bytes.replace(92, 8, QByteArray(8, '\0'));
    return bytes;
}

void require_frameless_wal(const QString &path) {
    const auto wal = wal_bytes(path);
    require(wal.isEmpty() || wal.size() == 32, "WAL is empty or contains only its header");
}

void require_restored_owner(SqliteWriter &writer) {
    auto observed = std::make_shared<std::optional<WriteResult>>();
    require(writer.enqueue({{"PRAGMA journal_mode", {}}, {"PRAGMA foreign_keys", {}}},
                           [observed](auto result) { *observed = std::move(result); }) ==
                StorageError::none,
            "Released owner admits read-only PRAGMA checks");
    require(QTest::qWaitFor([observed] { return observed->has_value(); }, 10000),
            "Owner delivers restored PRAGMA values");
    const auto &result = observed->value();
    require(result.error == StorageError::none && result.rows.size() == 2,
            "Owner completed both PRAGMA checks");
    expect_rows(result.rows[0], {{QString("wal")}}, "Restored owner journal_mode");
    expect_rows(result.rows[1], {{integer(1)}}, "Restored owner foreign_keys");
}

void cancel_before_begin(Fixture &f, V23StoppedWriterLease lease, Ops02UpdateHold hold,
                         V23RebuildBudget budget = {}) {
    const auto path = f.file.path();
    const auto before = cancelled_main_bytes(path);
    require_frameless_wal(path);
    struct Observation {
        QList<MigrationCheckpoint> checkpoints;
        int completions = 0;
        std::optional<MigrationResult> result;
    };
    auto observed = std::make_shared<Observation>();
    expect_admission(enqueue_v23_rebuild(
                         f.witness, std::move(hold), std::move(lease), budget,
                         [observed](const MigrationProgress &progress) {
                             observed->checkpoints.append(progress.checkpoint);
                             return true;
                         },
                         [observed](auto result) {
                             ++observed->completions;
                             observed->result = std::move(result);
                         }),
                     {});
    require(QTest::qWaitFor([observed] { return observed->result.has_value(); }, 10000),
            "Admitted rebuild delivers cancellation completion");
    QCoreApplication::processEvents();
    const auto &result = *observed->result;
    require(observed->checkpoints == QList{MigrationCheckpoint::before_begin} &&
                observed->completions == 1 && result.reason == MigrationReason::cancelled &&
                result.work.transaction == TransactionOutcome::not_started && !result.report &&
                !result.work.value,
            "One before_begin probe and one cancelled completion without a transaction or value");
    require(cancelled_main_bytes(path) == before, "Cancellation preserves main file contents");
    require_frameless_wal(path);
    require(!QFileInfo::exists(path + "-journal"), "Cancellation leaves no rollback journal");
    {
        auto replacement = f.reserve();
    }
    require_restored_owner(f.writer);
}

void zero_budget(Fixture &f, V23RebuildBudget budget) {
    const auto before = f.file_state();
    struct Observation {
        int calls = 0;
        std::optional<MigrationResult> result;
    };
    auto observed = std::make_shared<Observation>();
    expect_admission(enqueue_v23_rebuild(f.witness, FakeOps02UpdateHoldIssuer::issue(), f.reserve(),
                                         budget, {},
                                         [observed](auto result) {
                                             ++observed->calls;
                                             observed->result = std::move(result);
                                         }),
                     {});
    require(QTest::qWaitFor([observed] { return observed->result.has_value(); }, 10000),
            "Explicit zero budget delivers the resource refusal");
    QCoreApplication::processEvents();
    const auto &result = *observed->result;
    require(observed->calls == 1 && result.reason == MigrationReason::resource_limit &&
                result.work.transaction == TransactionOutcome::not_started && !result.report &&
                !result.work.value,
            "Zero budget refuses before a transaction");
    f.require_unchanged(before);
    auto replacement = f.reserve();
}

enum class OrdinaryCase { advance, changed_starts, changed_prefix, caller_id, inspect };

void ordinary_v23(Fixture &f, OrdinaryCase specimen) {
    auto manifest = compiled_successor();
    // Supply a structurally valid caller manifest. The projected schema stays
    // identical and the only v23 SQL is a read-only statement.
    const auto schema = manifest.steps.back().expected_schema;
    manifest.steps.append({23, "exact-valuation-parent-rebuild-v1", {"SELECT 1"}, schema});
    if (specimen == OrdinaryCase::caller_id)
        manifest.steps.back().id = "caller-owned-v23";
    manifest.supported_starting_versions.append(23);
    if (specimen == OrdinaryCase::changed_starts)
        manifest.supported_starting_versions.removeAt(1);
    if (specimen == OrdinaryCase::changed_prefix)
        manifest.steps[21].id = "caller-edited-v22";
    const auto mode =
        specimen == OrdinaryCase::inspect ? MigrationMode::inspect : MigrationMode::advance;
    auto command = request(mode, std::move(manifest));
    struct Observation {
        int probes = 0;
        int completions = 0;
    };
    auto observed = std::make_shared<Observation>();
    // An unexpected admission drains only this disposable writer job. Cancel
    // before BEGIN to prevent even a migration-history write.
    command.cancel = [observed](const auto &) {
        ++observed->probes;
        return true;
    };
    const auto before = f.file_state();
    const auto admission = SqliteMigrationRunner::enqueue(
        f.writer, std::move(command), [observed](auto) { ++observed->completions; });
    const bool drained = admission.error != StorageError::none ||
                         QTest::qWaitFor([observed] { return observed->completions > 0; }, 10000);
    QCoreApplication::processEvents();
    // Diagnose an admission regression at the admission boundary even if drain
    // also timed out or the unexpected job changed the disposable copy.
    if (admission.error == StorageError::none) {
        qCritical() << "Unexpected ordinary admission, drained:" << drained
                    << "probes:" << observed->probes << "completions:" << observed->completions
                    << "file unchanged:" << (f.file_state() == before)
                    << "journal exists:" << QFileInfo::exists(f.file.path() + "-journal");
    }
    expect_admission(admission, invalid);
    require(observed->probes == 0 && observed->completions == 0,
            "Ordinary refusal invokes neither cancellation nor completion");
    f.require_unchanged(before);
}

void exclusion(Fixture &f) {
    const WriteBatch batch{{"SELECT 1", {}}};
    struct Observation {
        int batches = 0;
        int bodies = 0;
        int work = 0;
        StorageError batch_error = StorageError::not_open;
        V23StoppedWriterLease during_work;
    };
    auto observed = std::make_shared<Observation>();
    auto batch_done = [observed](WriteResult result) {
        observed->batch_error = result.error;
        ++observed->batches;
    };
    auto body = [observed](SqliteWorkContext) {
        ++observed->bodies;
        return QVariant{};
    };
    auto work_done = [observed](SqliteWorkResult) { ++observed->work; };

    {
        auto lease = f.reserve();
        require(f.writer.enqueue(batch, batch_done) == StorageError::busy &&
                    f.writer.enqueue_work(body, work_done) == StorageError::busy &&
                    !ContributionReviewWriterAccess::acquire(f.writer),
                "Live reservation excludes batch, work and review");
        f.reject(V23RebuildAccess::acquire(f.writer), FakeOps02UpdateHoldIssuer::issue(), invalid);
        require(observed->batches == 0 && observed->bodies == 0 && observed->work == 0,
                "Blocked work never ran");
        require(f.writer.enqueue(batch, batch_done) == StorageError::busy,
                "Competing lease refusal preserves original reservation");
    }

    require(f.writer.enqueue(batch, batch_done) == StorageError::none,
            "Batch admitted after lease release");
    f.reject(V23RebuildAccess::acquire(f.writer), FakeOps02UpdateHoldIssuer::issue(), invalid);
    require(QTest::qWaitFor([observed] { return observed->batches == 1; }) &&
                observed->batch_error == StorageError::none,
            "Admitted batch drains once");

    require(f.writer.enqueue_work(
                [observed, body, &writer = f.writer](auto context) {
                    observed->during_work = V23RebuildAccess::acquire(writer);
                    return body(context);
                },
                work_done) == StorageError::none,
            "Work admitted after lease release");
    f.reject(V23RebuildAccess::acquire(f.writer), FakeOps02UpdateHoldIssuer::issue(), invalid);
    require(QTest::qWaitFor([observed] { return observed->work == 1; }) && observed->bodies == 1,
            "Admitted work drains once");
    f.reject(std::move(observed->during_work), FakeOps02UpdateHoldIssuer::issue(), invalid);

    require(ContributionReviewWriterAccess::acquire(f.writer),
            "Review admitted after lease release");
    f.reject(V23RebuildAccess::acquire(f.writer), FakeOps02UpdateHoldIssuer::issue(), invalid);
    ContributionReviewWriterAccess::release(f.writer);
    auto lease = f.reserve();
}

void idle_owner_work_exclusion(Fixture &f) {
    int bodies = 0;
    const auto work = [&](SqliteWorkContext context) {
        ++bodies;
        context.begin().commit();
        return QVariant{};
    };

    {
        auto lease = f.reserve();
        const auto refused = f.writer.run_idle_owner_work(work);
        require(refused.transaction == TransactionOutcome::not_started &&
                    refused.error == WorkError::protocol_error,
                QString("Live reservation must refuse idle owner work as not_started/protocol_error "
                        "(transaction=%1, error=%2, body calls=%3)")
                    .arg(static_cast<int>(refused.transaction))
                    .arg(static_cast<int>(refused.error))
                    .arg(bodies));
        require(bodies == 0, "Refused idle owner work never ran its body");
    }

    const auto admitted = f.writer.run_idle_owner_work(work);
    require(admitted.transaction == TransactionOutcome::committed &&
                admitted.error == WorkError::none && bodies == 1,
            "Idle owner work runs and commits once after lease release");
}
} // namespace

int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);
    const auto args = application.arguments();
    if (args.size() != 2) {
        qCritical("Usage: server_storage_021_stage1_controls pinned-v22-1024.sqlite");
        return 1;
    }
    bool passed = true;
    const auto run = [&](const char *name, auto check) {
        try {
            Fixture fixture(args[1]);
            check(fixture);
        } catch (const std::exception &failure) {
            qCritical().noquote() << name << failure.what();
            passed = false;
        }
    };

    run("both omitted", [](Fixture &f) {
        cancel_before_begin(f, f.reserve(), FakeOps02UpdateHoldIssuer::issue());
    });
    run("disk omitted", [](Fixture &f) {
        cancel_before_begin(f, f.reserve(), FakeOps02UpdateHoldIssuer::issue(), {{}, 184'549'376});
    });
    run("memory omitted", [](Fixture &f) {
        cancel_before_begin(f, f.reserve(), FakeOps02UpdateHoldIssuer::issue(), {83'886'080, {}});
    });
    run("disk compiled plus one", [](Fixture &f) {
        f.reject(f.reserve(), FakeOps02UpdateHoldIssuer::issue(), invalid, {83'886'081, {}});
    });
    run("memory compiled plus one", [](Fixture &f) {
        f.reject(f.reserve(), FakeOps02UpdateHoldIssuer::issue(), invalid, {{}, 184'549'377});
    });
    run("zero disk", [](Fixture &f) { zero_budget(f, {0, {}}); });
    run("zero memory", [](Fixture &f) { zero_budget(f, {{}, 0}); });
    run("invalid lease",
        [](Fixture &f) { f.reject({}, FakeOps02UpdateHoldIssuer::issue(), invalid); });
    run("moved lease", [](Fixture &f) {
        auto source = f.reserve();
        auto owner = std::move(source);
        f.reject(std::move(source), FakeOps02UpdateHoldIssuer::issue(), invalid);
        cancel_before_begin(f, std::move(owner), FakeOps02UpdateHoldIssuer::issue());
    });
    run("destroyed writer lease", [&](Fixture &f) {
        V23StoppedWriterLease stale;
        {
            Fixture owner(args[1]);
            stale = owner.reserve();
        }
        f.reject(std::move(stale), FakeOps02UpdateHoldIssuer::issue(), invalid);
    });
    run("foreign thread", [](Fixture &f) {
        auto lease = f.reserve();
        const auto before = f.file_state();
        MigrationAdmission admission;
        auto calls = std::make_shared<int>(0);
        auto witness = f.witness;
        auto hold = FakeOps02UpdateHoldIssuer::issue();
        std::thread foreign([&] {
            admission = enqueue_v23_rebuild(std::move(witness), std::move(hold), std::move(lease),
                                            {}, {}, [calls](auto) { ++*calls; });
        });
        foreign.join();
        expect_admission(admission, invalid);
        QCoreApplication::processEvents();
        require(*calls == 0, "Foreign call delivers no completion");
        f.require_unchanged(before);
        auto replacement = f.reserve();
    });
    run("empty hold", [](Fixture &f) { f.reject(f.reserve(), {}, invalid); });
    run("moved hold", [](Fixture &f) {
        auto source = FakeOps02UpdateHoldIssuer::issue();
        auto owner = std::move(source);
        f.reject(f.reserve(), std::move(source), invalid);
        cancel_before_begin(f, f.reserve(), std::move(owner));
    });
    run("null completion", [](Fixture &f) {
        f.reject(f.reserve(), FakeOps02UpdateHoldIssuer::issue(),
                 {StorageError::invalid_work, MigrationReason::none}, {}, true);
    });
    run("invalid authority precedes null completion",
        [](Fixture &f) { f.reject({}, FakeOps02UpdateHoldIssuer::issue(), invalid, {}, true); });
    run("null completion precedes over-ceiling budget", [](Fixture &f) {
        f.reject(f.reserve(), FakeOps02UpdateHoldIssuer::issue(),
                 {StorageError::invalid_work, MigrationReason::none}, {83'886'081, {}}, true);
    });
    run("tampered witness version", [](Fixture &f) {
        ++f.witness.version;
        f.reject(f.reserve(), FakeOps02UpdateHoldIssuer::issue(), invalid);
    });
    run("tampered witness prefix", [](Fixture &f) {
        // Keep the original digest width and change one byte.
        f.witness.prefix[0] = static_cast<char>(f.witness.prefix.at(0) ^ 1);
        f.reject(f.reserve(), FakeOps02UpdateHoldIssuer::issue(), invalid);
    });
    run("tampered witness projection count", [](Fixture &f) {
        ++f.witness.projection_objects;
        f.reject(f.reserve(), FakeOps02UpdateHoldIssuer::issue(), invalid);
    });
    run("ordinary exact v22 advance", [](Fixture &f) {
        expect_current(migrate(f.writer, request(MigrationMode::advance, compiled_successor())), 22,
                       0);
    });
    run("ordinary exact v22 inspect", [](Fixture &f) {
        expect_current(migrate(f.writer, request(MigrationMode::inspect, compiled_successor())), 22,
                       0);
    });
    run("ordinary v23 advance", [](Fixture &f) { ordinary_v23(f, OrdinaryCase::advance); });
    run("ordinary v23 advance with changed starts",
        [](Fixture &f) { ordinary_v23(f, OrdinaryCase::changed_starts); });
    run("ordinary v23 advance with changed prefix",
        [](Fixture &f) { ordinary_v23(f, OrdinaryCase::changed_prefix); });
    run("ordinary v23 advance with caller-chosen ID",
        [](Fixture &f) { ordinary_v23(f, OrdinaryCase::caller_id); });
    run("ordinary v23 inspect", [](Fixture &f) { ordinary_v23(f, OrdinaryCase::inspect); });
    run("mutual exclusion", exclusion);
    run("idle owner work exclusion", idle_owner_work_exclusion);
    run("shutdown revokes reservation", [](Fixture &f) {
        auto lease = f.reserve();
        auto stopped = std::make_shared<int>(0);
        require(f.writer.shutdown([stopped] { ++*stopped; }) == StorageError::none,
                "Shutdown remains available with a live reservation");
        auto while_stopping = V23RebuildAccess::acquire(f.writer);
        f.reject(std::move(lease), FakeOps02UpdateHoldIssuer::issue(), invalid);
        f.reject(std::move(while_stopping), FakeOps02UpdateHoldIssuer::issue(), invalid);
        f.reject(V23RebuildAccess::acquire(f.writer), FakeOps02UpdateHoldIssuer::issue(), invalid);
        require(QTest::qWaitFor([stopped] { return *stopped == 1; }), "Shutdown completed once");
    });
    run("normal v23 reopen", normal_v23_reopen);
    run("private read-only v23 inspection", [](Fixture &f) {
        commit_and_stop_v23(f);
        expect_v23_inspection(f.file, MigrationReason::none);
    });
    for (const auto &[name, drift] : {std::pair{"digest drift", V23Drift::digest},
                                      {"projection drift", V23Drift::projection},
                                      {"unknown head", V23Drift::future}}) {
        const auto specimen = [drift](auto check) {
            return [drift, check](Fixture &f) {
                commit_and_stop_v23(f);
                DisposableFile altered;
                f.file.copy_to(altered);
                check(altered, prepare_v23_drift(altered, drift));
            };
        };
        run(qPrintable(QString("normal reopen: %1").arg(name)), specimen(refuse_v23_reopen));
        run(qPrintable(QString("private inspection: %1").arg(name)),
            specimen(expect_v23_inspection));
    }
    run("v22 W1 valid-input control", [](Fixture &f) {
        w1_v22_positive(f);
        qInfo("Passed v22 W1 valid-input control");
    });
    run("v23 W1 staging refusal control", [](Fixture &f) {
        w1_refuses_v23(f);
        qInfo("Passed v23 W1 staging refusal control");
    });
    return passed ? 0 : 1;
}
