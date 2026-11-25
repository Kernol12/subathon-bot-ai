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
constexpr MigrationAdmission inert{StorageError::feature_not_implemented,
                                   MigrationReason::feature_not_implemented};

void expect_admission(MigrationAdmission actual, MigrationAdmission expected) {
    require(actual.error == expected.error && actual.reason == expected.reason,
            QString("Admission error/reason %1/%2, expected %3/%4")
                .arg(static_cast<int>(actual.error))
                .arg(static_cast<int>(actual.reason))
                .arg(static_cast<int>(expected.error))
                .arg(static_cast<int>(expected.reason)));
}

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

    void reject(V23StoppedWriterLease lease, Ops02UpdateHold hold, MigrationAdmission expected,
                V23RebuildBudget budget = {}, bool null_completion = false) {
        const auto before = file.seal();
        auto calls = std::make_shared<int>(0);
        SqliteMigrationRunner::Completion completion;
        if (!null_completion)
            completion = [calls](auto) { ++*calls; };
        expect_admission(enqueue_v23_rebuild(witness, std::move(hold), std::move(lease), budget, {},
                                             std::move(completion)),
                         expected);
        QCoreApplication::processEvents();
        require(*calls == 0 && file.seal() == before &&
                    !QFileInfo::exists(file.path() + "-journal"),
                "Synchronous refusal leaves no callback, file change or rollback journal");
    }

    DisposableFile file;
    V22PreOpenWitness witness;
    SqliteWriter writer;
};

void zero_budget(Fixture &f, V23RebuildBudget budget) {
    const auto before = f.file.seal();
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
                !result.work.value && f.file.seal() == before &&
                !QFileInfo::exists(f.file.path() + "-journal"),
            "Zero budget refuses before a transaction and leaves the donor unchanged");
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

    run("both omitted",
        [](Fixture &f) { f.reject(f.reserve(), FakeOps02UpdateHoldIssuer::issue(), inert); });
    run("disk omitted", [](Fixture &f) {
        f.reject(f.reserve(), FakeOps02UpdateHoldIssuer::issue(), inert, {{}, 184'549'376});
    });
    run("memory omitted", [](Fixture &f) {
        f.reject(f.reserve(), FakeOps02UpdateHoldIssuer::issue(), inert, {83'886'080, {}});
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
        f.reject(std::move(owner), FakeOps02UpdateHoldIssuer::issue(), inert);
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
        const auto before = f.file.seal();
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
        require(*calls == 0 && f.file.seal() == before, "Foreign call leaves the owner untouched");
        auto replacement = f.reserve();
    });
    run("empty hold", [](Fixture &f) { f.reject(f.reserve(), {}, invalid); });
    run("moved hold", [](Fixture &f) {
        auto source = FakeOps02UpdateHoldIssuer::issue();
        auto owner = std::move(source);
        f.reject(f.reserve(), std::move(source), invalid);
        f.reject(f.reserve(), std::move(owner), inert);
    });
    run("null completion", [](Fixture &f) {
        f.reject(f.reserve(), FakeOps02UpdateHoldIssuer::issue(),
                 {StorageError::invalid_work, MigrationReason::none}, {}, true);
    });
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
