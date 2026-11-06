// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <subathon/server/reference_capture_values.hpp>
#include <subathon/server/sqlite_migrations.hpp>
#include <subathon/server/sqlite_writer.hpp>
#include <utility>

namespace subathon::server {
// Concrete v1: docs/contracts/sqlite-initial-captures-v1.md. The facade retains
// only the caller's revocable capability; it owns no transaction or connection.
struct ReferenceCaptureLimits {
    qsizetype reference_bytes{4096};
    qsizetype document_bytes{262144};
    qsizetype owned_bytes{1048576};
};
enum class ReferenceCaptureRetainDisposition : std::uint8_t {
    inserted,
    already_present,
    conflict,
    missing_reference,
    source_mismatch
};
enum class ReferenceCaptureStorageReason : std::uint8_t {
    busy,
    sql_failure,
    resource_limit,
    result_limit,
    invalid_stored_data
};
class ReferenceCaptureStorageFailure final : public std::runtime_error {
  public:
    explicit ReferenceCaptureStorageFailure(ReferenceCaptureStorageReason reason)
        : std::runtime_error("Reference initial capture did not commit"), reason_(reason) {}
    [[nodiscard]] ReferenceCaptureStorageReason reason() const noexcept { return reason_; }

  private:
    ReferenceCaptureStorageReason reason_;
};
class SqliteReferenceCaptureNotImplemented final : public std::logic_error {
  public:
    SqliteReferenceCaptureNotImplemented()
        : std::logic_error(
              "SQLite initial captures are intentionally unimplemented; review tests first") {}
};

// Return the additive catalog manifest without opening or updating a database.
// The caller retains explicit migration/readiness and operational prerequisites.
[[nodiscard]] MigrationManifest reference_capture_repository_manifest();

class SqliteReferenceCaptureStore final {
  public:
    explicit SqliteReferenceCaptureStore(SqliteWorkSession session,
                                         ReferenceCaptureLimits limits = {})
        : session_(std::move(session)), limits_(limits) {}

    // Use the caller's active revocable session; results stay provisional until
    // its acknowledged commit and normal work completion.
    [[nodiscard]] std::optional<ReferenceInitialCapture> find(const ReferenceCaptureKey &) const;
    [[nodiscard]] ReferenceCaptureRetainDisposition
    retain_initial(const ReferenceInitialCapture &) const;

  private:
    SqliteWorkSession session_;
    ReferenceCaptureLimits limits_;
};
} // namespace subathon::server
