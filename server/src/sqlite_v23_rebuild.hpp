// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

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

struct V23RebuildBudget {
    std::optional<std::uint64_t> disk_bytes;
    std::optional<std::uint64_t> memory_bytes;
};

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
                                      SqliteMigrationRunner::Completion completion);
    friend MigrationAdmission enqueue_v23_rebuild(V22PreOpenWitness, Ops02UpdateHold,
                                                  V23StoppedWriterLease, V23RebuildBudget,
                                                  V23Cancellation,
                                                  SqliteMigrationRunner::Completion);
};

} // namespace subathon::server::detail
