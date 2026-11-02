// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/core/application_plan.hpp>
#include <subathon/server/sqlite_migrations.hpp>

namespace subathon::server {
// Internal revision1: docs/contracts/sqlite-application-plans-v1.md.
// Full-read limits are materialization limits, never retained-history/append quotas.
struct SqlitePlanLimits {
    qsizetype reference_bytes{4096};
    qsizetype value_bytes{262144};
    qsizetype component_bytes{16384};
    qsizetype revisions_per_full_find{1024};
    qsizetype items_per_vector{1024};
    qsizetype items_per_full_find{4096};
};
enum class SqlitePlanInputError {
    invalid_options,
    invalid_codec,
    invalid_exact,
    exact_component_limit,
    invalid_plan_ref,
    invalid_revision,
    collection_limit,
    invalid_override,
    invalid_receipt_id,
    invalid_event,
    invalid_contribution_state,
    invalid_valuation,
    invalid_rule,
    invalid_modifier,
    invalid_request
};
class SqlitePlanInputFailure final : public std::invalid_argument {
  public:
    explicit SqlitePlanInputFailure(SqlitePlanInputError error)
        : std::invalid_argument("Invalid SQLite plan persistence input"), error_(error) {}
    [[nodiscard]] SqlitePlanInputError error() const noexcept { return error_; }

  private:
    SqlitePlanInputError error_;
};

enum class SqlitePlanStorageReason {
    busy,
    sql_failure,
    resource_limit,
    result_limit,
    missing_receipt,
    missing_contribution,
    receipt_mismatch,
    invalid_stored_data,
    revision_exhausted
};
// A known-uncommitted port error, only after actual rollback is established.
// Actual capability/protocol/unknown-durability errors stay SqliteWorkFailure.
class SqlitePlanStorageFailure final : public std::runtime_error {
  public:
    explicit SqlitePlanStorageFailure(SqlitePlanStorageReason reason)
        : std::runtime_error("SQLite plan transaction did not commit"), reason_(reason) {}
    [[nodiscard]] SqlitePlanStorageReason reason() const noexcept { return reason_; }

  private:
    SqlitePlanStorageReason reason_;
};

class SqliteApplicationPlansNotImplemented final : public std::logic_error {
  public:
    SqliteApplicationPlansNotImplemented()
        : std::logic_error(
              "SQLite application plans are intentionally unimplemented; review tests first") {}
};

// Pure full0..MAX codec, distinct from persisted revision/head/parent1..MAX.
[[nodiscard]] QByteArray encode_plan_revision_uint64(std::uint64_t value);
[[nodiscard]] std::uint64_t decode_plan_revision_uint64(const QByteArray &bytes);
struct SqlitePlanExactComponents {
    QByteArray numerator;
    QByteArray denominator;
    bool operator==(const SqlitePlanExactComponents &) const = default;
};
// Canonical signed numerator/positive reduced denominator ASCII BLOB values.
// These independent byte bounds do not invoke numeric parsing/final rounding policy.
[[nodiscard]] SqlitePlanExactComponents encode_plan_exact(const core::Exact &value,
                                                          qsizetype component_bytes = 16384);
[[nodiscard]] core::Exact decode_plan_exact(const SqlitePlanExactComponents &components,
                                            qsizetype component_bytes = 16384);
// Pure immutable prefix1/2/3 + approved captured-application-plans-v1 step4.
[[nodiscard]] MigrationManifest application_plan_repository_manifest();

// This selected snapshot does not certify a complete ApplicationPlan/history.
struct SqlitePlanRevisionSnapshot {
    std::string plan_ref;
    core::CapturedPlanRevision revision;
    bool operator==(const SqlitePlanRevisionSnapshot &) const = default;
};
enum class SqlitePlanInitialDisposition { inserted, already_present, conflict };
enum class SqlitePlanAppendDisposition {
    appended,
    missing_plan,
    stale_head,
    revision_conflict,
    event_conflict
};

// Inert copyable facade over the caller's active revocable session. Constructor/
// destructor do no SQL, BEGIN, COMMIT, cleanup or transfer of caller authority.
// Successful writes are staged only; durable publication requires real caller
// COMMIT and normal work completion. Domain refusals make no own writes.
class SqliteApplicationPlanStore final {
  public:
    explicit SqliteApplicationPlanStore(SqliteWorkSession session, SqlitePlanLimits limits = {});
    [[nodiscard]] SqlitePlanInitialDisposition
    retain_initial(const std::string &plan_ref, const core::CapturedPlanRevision &initial) const;
    [[nodiscard]] std::optional<core::ApplicationPlan> find(const std::string &plan_ref) const;
    [[nodiscard]] std::optional<SqlitePlanRevisionSnapshot>
    find_revision(const std::string &plan_ref, std::uint64_t revision) const;
    // Expected0 is a valid optimistic token (stale against every valid stored head).
    [[nodiscard]] SqlitePlanAppendDisposition append(const std::string &plan_ref,
                                                     std::uint64_t expected_head,
                                                     const core::CapturedPlanRevision &next) const;

  private:
    SqliteWorkSession session_;
    SqlitePlanLimits limits_;
};
} // namespace subathon::server
