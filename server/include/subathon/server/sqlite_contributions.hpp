// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <subathon/server/contribution_evidence.hpp>
#include <subathon/server/sqlite_migrations.hpp>
#include <variant>

class QThread;

namespace subathon::server {
// Internal revision1: docs/contracts/sqlite-contributions-v1.md/schema companion.
// Read materialization/byte limits are not observation quotas or semantic policy.
struct SqliteContributionLimits {
    qsizetype reference_bytes{4096};
    qsizetype value_bytes{262144};
    qsizetype unresolved_per_lookup{1024};
};
enum class SqliteContributionInputError {
    invalid_options,
    invalid_codec,
    invalid_receipt_id,
    invalid_receipt,
    invalid_activity,
    invalid_state,
    invalid_observation,
    already_begun
};
class SqliteContributionInputFailure final : public std::invalid_argument {
  public:
    explicit SqliteContributionInputFailure(SqliteContributionInputError error)
        : std::invalid_argument("Invalid SQLite contribution adapter input"), error_(error) {}
    [[nodiscard]] SqliteContributionInputError error() const noexcept { return error_; }

  private:
    SqliteContributionInputError error_;
};

enum class SqliteContributionStorageReason {
    busy,
    sql_failure,
    resource_limit,
    result_limit,
    missing_receipt,
    receipt_mismatch,
    missing_observation,
    missing_contribution,
    association_conflict,
    invalid_stored_data,
    allocator_exhausted,
    incomplete_contribution
};
// Only established known-uncommitted failures derive from the existing port error.
// Capability/protocol/unknown durability remains actual SqliteWorkFailure.
class SqliteContributionStorageFailure final : public ContributionStorageError {
  public:
    explicit SqliteContributionStorageFailure(SqliteContributionStorageReason reason)
        : ContributionStorageError("SQLite contribution transaction did not commit"),
          reason_(reason) {}
    [[nodiscard]] SqliteContributionStorageReason reason() const noexcept { return reason_; }

  private:
    SqliteContributionStorageReason reason_;
};

// Baseline marker makes no assertion that an adopted caller unit rolled back.
class SqliteContributionsNotImplemented final : public std::logic_error {
  public:
    SqliteContributionsNotImplemented()
        : std::logic_error(
              "SQLite contributions are intentionally unimplemented; review tests first") {}
};

// Actual typed ContributionId includes zero. SQL NULL/std::nullopt is absence;
// it is never encoded as ID0. Stored SQL type validation remains the adapter's job.
[[nodiscard]] QByteArray encode_contribution_id(core::ContributionId id);
[[nodiscard]] core::ContributionId decode_contribution_id(const QByteArray &bytes);
// Pure approved prefix1/2 + contribution-evidence-v1 step3; no migration authority.
[[nodiscard]] MigrationManifest contribution_repository_manifest();

struct AdoptActiveContributionSessionTag final {};
inline constexpr AdoptActiveContributionSessionTag adopt_active_contribution_session{};

// Inert value facade over revocable tickets. Construct/use/destroy on the owner.
// Explicit adoption transfers real commit/cleanup only after begin successfully
// constructs its independent transaction wrapper. Caller excludes double adoption;
// copied weak tickets grant no ownership/lifetime/active-phase proof by themselves.
class SqliteContributionRepository final : public ContributionRepository {
  public:
    explicit SqliteContributionRepository(SqliteWorkContext context,
                                          SqliteContributionLimits limits = {});
    SqliteContributionRepository(AdoptActiveContributionSessionTag, SqliteWorkSession session,
                                 SqliteContributionLimits limits = {});
    ~SqliteContributionRepository() override = default;
    SqliteContributionRepository(const SqliteContributionRepository &) = delete;
    SqliteContributionRepository &operator=(const SqliteContributionRepository &) = delete;
    SqliteContributionRepository(SqliteContributionRepository &&) = delete;
    SqliteContributionRepository &operator=(SqliteContributionRepository &&) = delete;

    // Pure options precede construction-owner and local single-use checks. A caught
    // already_begun is local input rejection, leaving the first unit healthy.
    [[nodiscard]] std::unique_ptr<ContributionTransaction> begin() override;

  private:
    std::variant<SqliteWorkContext, SqliteWorkSession> capability_;
    QThread *const construction_thread_;
    SqliteContributionLimits limits_;
    bool begun_{false};
};
} // namespace subathon::server
