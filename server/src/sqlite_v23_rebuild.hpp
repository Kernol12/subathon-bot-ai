// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "v23_executor/fixture_bounds.hpp"

#include <cstdint>
#include <memory>
#include <subathon/server/sqlite_migrations.hpp>
#include <variant>

namespace subathon::server::detail {

struct V22PreOpenWitness {
    qint32 version{};
    QByteArray prefix;
    qsizetype projection_objects{};
};

[[nodiscard]] std::variant<V22PreOpenWitness, MigrationReason>
inspect_v22_pre_open(const QString &path, StorageLimits limits);

inline constexpr std::uint64_t v23_fixture_disk_bytes = 83'886'080;
inline constexpr std::uint64_t v23_fixture_memory_bytes = 184'549'376;

// One bounds type for the private entry and the fixture calculator. A caller
// value can only lower the compiled fixture ceiling.
using V23RebuildBudget = V23FixtureBudget;

// The writer's owned connection inside the stopped-writer fence. Only the
// writer constructs it, on its owner thread, while the reservation is held.
class V23OwnerConnection {
  public:
    enum class Transaction { begin, commit, rollback };
    struct Control {
        StorageError error;
        bool acknowledged;
    };
    virtual ~V23OwnerConnection() = default;
    // One SQL attempt. Rows are bounded by the writer's result limits.
    [[nodiscard]] virtual ReadResult read(const QString &sql) = 0;
    // BEGIN IMMEDIATE, COMMIT or ROLLBACK through the writer's control seam.
    [[nodiscard]] virtual Control transaction(Transaction operation) = 0;
    [[nodiscard]] virtual QString path() const = 0;
};

// How the writer continues after the fenced job. Every non-normal state
// other than a verified v23 commit closes the connection before delivery.
enum class V23OwnerState { normal, committed_v23, retire };
using V23OwnerJob = std::function<V23OwnerState(V23OwnerConnection &)>;

namespace testing {
class FakeOps02UpdateHoldIssuer;
}

class Ops02UpdateHold final {
  public:
    Ops02UpdateHold() = default;
    Ops02UpdateHold(Ops02UpdateHold &&) noexcept = default;
    Ops02UpdateHold &operator=(Ops02UpdateHold &&) noexcept = default;

  private:
    struct FixtureHold {};
    std::unique_ptr<FixtureHold> fixture_hold_;
    friend class testing::FakeOps02UpdateHoldIssuer;
    friend class V23RebuildAccess;
};

class V23StoppedWriterLease final {
  public:
    V23StoppedWriterLease() = default;
    V23StoppedWriterLease(V23StoppedWriterLease &&) noexcept = default;
    V23StoppedWriterLease &operator=(V23StoppedWriterLease &&) noexcept = default;

  private:
    // The writer keeps a weak reservation and independently owns the generation.
    struct Generation {
        SqliteWriter *writer;
        QThread *owner;
    };
    struct Reservation {};
    std::shared_ptr<const Reservation> reservation_;
    std::weak_ptr<const Generation> generation_;
    QThread *owner_{};
    friend class V23RebuildAccess;
    friend class subathon::server::SqliteWriter;
};

using V23Cancellation = decltype(MigrationRequest::cancel);

[[nodiscard]] MigrationAdmission
enqueue_v23_rebuild(V22PreOpenWitness witness, Ops02UpdateHold hold, V23StoppedWriterLease lease,
                    V23RebuildBudget budget, V23Cancellation cancel,
                    SqliteMigrationRunner::Completion completion);

class V23RebuildAccess final {
  public:
    [[nodiscard]] static V23StoppedWriterLease acquire(SqliteWriter &writer);

  private:
    static MigrationAdmission enqueue(V22PreOpenWitness witness, Ops02UpdateHold hold,
                                      V23StoppedWriterLease lease, V23RebuildBudget budget,
                                      V23Cancellation cancel,
                                      SqliteMigrationRunner::Completion completion);
    // Queues the job under the lease's live reservation. The writer keeps the
    // reservation through the job, releases it, then calls deliver.
    static StorageError submit(V23StoppedWriterLease lease, V23OwnerJob job,
                               std::function<void()> deliver);
    friend MigrationAdmission enqueue_v23_rebuild(V22PreOpenWitness, Ops02UpdateHold,
                                                  V23StoppedWriterLease, V23RebuildBudget,
                                                  V23Cancellation,
                                                  SqliteMigrationRunner::Completion);
};

} // namespace subathon::server::detail
