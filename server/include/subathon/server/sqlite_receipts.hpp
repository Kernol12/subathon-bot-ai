// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <cstdint>
#include <subathon/server/receipt_intake.hpp>
#include <subathon/server/sqlite_migrations.hpp>
#include <variant>

class QThread;

namespace subathon::server {
// Internal revision1: docs/contracts/sqlite-receipts-v1.md and its schema companion.
// These defaults bound adapter materialization, not product delivery/retry quotas.
struct SqliteReceiptLimits {
    qsizetype reference_bytes{4096};
    qsizetype value_bytes{262144};
    qsizetype attempts_per_find{1024};
};
enum class SqliteReceiptInputError {
    invalid_options,
    invalid_codec,
    invalid_identity,
    invalid_facts,
    invalid_arrival,
    invalid_receipt_id,
    invalid_disposition,
    already_begun
};
class SqliteReceiptInputFailure final : public std::invalid_argument {
  public:
    explicit SqliteReceiptInputFailure(SqliteReceiptInputError error)
        : std::invalid_argument("Invalid SQLite receipt adapter input"), error_(error) {}
    [[nodiscard]] SqliteReceiptInputError error() const noexcept { return error_; }

  private:
    SqliteReceiptInputError error_;
};

enum class SqliteReceiptStorageReason {
    busy,
    sql_failure,
    resource_limit,
    result_limit,
    missing_reference,
    invalid_stored_data,
    allocator_exhausted,
    attempt_order_exhausted,
    incomplete_receipt
};
// This port exception asserts a known uncommitted outcome. Unknown durability
// and capability/protocol failures retain their original SqliteWorkFailure.
class SqliteReceiptStorageFailure final : public ReceiptStorageError {
  public:
    explicit SqliteReceiptStorageFailure(SqliteReceiptStorageReason reason)
        : ReceiptStorageError("SQLite receipt transaction did not commit"), reason_(reason) {}
    [[nodiscard]] SqliteReceiptStorageReason reason() const noexcept { return reason_; }

  private:
    SqliteReceiptStorageReason reason_;
};

// Baseline-only stub marker: never claims rollback of an adopted active unit.
class SqliteReceiptsNotImplemented final : public std::logic_error {
  public:
    SqliteReceiptsNotImplemented()
        : std::logic_error("SQLite receipts are intentionally unimplemented; review tests first") {}
};

// Pure codec covers FULL uint64 including zero, with exact eight-byte big endian.
// Decode rejects null/wrong-length buffers as invalid_codec. Domain ReceiptId0
// remains invalid independently of this codec; no signed/REAL conversion occurs.
[[nodiscard]] QByteArray encode_receipt_uint64(std::uint64_t value);
[[nodiscard]] std::uint64_t decode_receipt_uint64(const QByteArray &bytes);
// Pure approved migration prefix1 + identified-receipts-v1 step2; no SQL/authority.
[[nodiscard]] MigrationManifest receipt_repository_manifest();

struct AdoptActiveReceiptSessionTag final {};
inline constexpr AdoptActiveReceiptSessionTag adopt_active_receipt_session{};

// Construct/use/destroy on the authoritative owner. Values own no SQL handle.
// The stored construction-thread identity guards mutable single-use state; it
// does not prove a supplied weak ticket's true owner/phase. Adoption requires
// the caller's actual active session and transfers commit/cleanup only when
// begin successfully returns its independently owned transaction wrapper.
class SqliteReceiptRepository final : public ReceiptRepository {
  public:
    explicit SqliteReceiptRepository(SqliteWorkContext context, SqliteReceiptLimits limits = {});
    SqliteReceiptRepository(AdoptActiveReceiptSessionTag, SqliteWorkSession session,
                            SqliteReceiptLimits limits = {});
    ~SqliteReceiptRepository() override = default;
    SqliteReceiptRepository(const SqliteReceiptRepository &) = delete;
    SqliteReceiptRepository &operator=(const SqliteReceiptRepository &) = delete;
    SqliteReceiptRepository(SqliteReceiptRepository &&) = delete;
    SqliteReceiptRepository &operator=(SqliteReceiptRepository &&) = delete;

    // Precedence: pure options, construction-thread guard, local single-use guard,
    // then real context.begin or explicit adoption. Caught already_begun is local
    // input/protocol rejection; it neither poisons nor cleans the first unit.
    [[nodiscard]] std::unique_ptr<ReceiptTransaction> begin() override;

  private:
    std::variant<SqliteWorkContext, SqliteWorkSession> capability_;
    QThread *const construction_thread_;
    SqliteReceiptLimits limits_;
    bool begun_{false};
};
} // namespace subathon::server
