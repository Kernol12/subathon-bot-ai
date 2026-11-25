// SPDX-License-Identifier: GPL-3.0-or-later
#include "../storage_003/protected_predecessor_fixture.hpp"
#include "contribution_review_writer_access.hpp"
#include "fake_ops02_update_hold.hpp"

#include <QCoreApplication>
#include <QTest>

#include <thread>

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

void cancel_before_begin(Fixture &f, V23StoppedWriterLease lease, Ops02UpdateHold hold,
                         V23RebuildBudget budget = {}) {
    const auto before = f.file_state();
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
    f.require_unchanged(before);
    auto replacement = f.reserve();
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
    return passed ? 0 : 1;
}
