// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/server/sqlite_migrations.hpp>
#include <utility>

namespace subathon::server {
// Internal revision 1: docs/contracts/retained-context-v1.md. Opaque references
// preserve the exact existing std::string bytes, including non-text bytes.
enum class RetainedKind { configuration, arrival_context };
struct RetainedKey {
    RetainedKind kind{RetainedKind::configuration};
    std::string reference;
    bool operator==(const RetainedKey &) const = default;
};
struct ConfigurationReference {
    std::string reference;
    bool operator==(const ConfigurationReference &) const = default;
};
struct RetainedDocument {
    RetainedKey key;
    // Required: null input is invalid; non-null empty is a present empty BLOB.
    // Loaded typeof(blob)/length0 is returned as a non-null empty QByteArray.
    QByteArray document;
    std::optional<ConfigurationReference> configuration;
    bool operator==(const RetainedDocument &) const = default;
};
struct RetainedLimits {
    qsizetype reference_bytes{4096};
    qsizetype document_bytes{262144};
};
enum class RetainDisposition { inserted, already_present, conflict, missing_reference };
enum class RetentionError {
    feature_not_implemented,
    invalid_options,
    invalid_kind,
    invalid_reference,
    invalid_document,
    invalid_link,
    invalid_stored_data,
    result_limit
};
class RetentionFailure final : public std::runtime_error {
  public:
    explicit RetentionFailure(RetentionError error)
        : std::runtime_error("Retained context operation failed"), error_(error) {}
    [[nodiscard]] RetentionError error() const noexcept { return error_; }

  private:
    RetentionError error_;
};

// Pure factory for approved migration1/retain-config-context-v1, supported starts
// 0/1. It grants no upgrade authority and does not open/create a store implicitly.
[[nodiscard]] MigrationManifest retained_context_manifest();

// A value facade over the caller's revocable session capability. Its
// construction/destruction neither touches SQL nor finalizes the transaction.
class SqliteRetainedContextStore final {
  public:
    explicit SqliteRetainedContextStore(SqliteWorkSession session, RetainedLimits limits = {})
        : session_(std::move(session)), limits_(limits) {}

    // Precedence before SQL: positive options, known kind, valid key reference,
    // then retain-only document and link validation. Invalid input wins even
    // with stale/foreign capabilities and does not abort earlier caller writes.
    // Valid input delegates existing capability/SQL/budget exceptions unchanged.
    [[nodiscard]] std::optional<RetainedDocument> find(const RetainedKey &key) const;
    [[nodiscard]] RetainDisposition retain(const RetainedDocument &document) const;

    // Configuration rows reject all link metadata; context links are typed to
    // configuration, nonempty and bounded. Domain conflict/missing_reference
    // makes no own writes and leaves a healthy caller transaction active.
    // Selected malformed rows/dangling links cause known rollback before
    // invalid_stored_data; selected well-typed over-limit rows use result_limit.
    // If cleanup is uncertain, propagate SqliteWorkFailure(uncertain_outcome).
    // No global audit or auto-BEGIN/COMMIT/retry is supplied.

  private:
    SqliteWorkSession session_;
    RetainedLimits limits_;
};
} // namespace subathon::server
