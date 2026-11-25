// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <QList>
#include <QString>
#include <functional>
#include <optional>
#include <subathon/server/sqlite_migration_manifest.hpp>
#include <subathon/server/sqlite_writer.hpp>

namespace subathon::server {
// Internal revision 2: docs/contracts/migration-catalog-v1.md. These DTOs grant
// no update/backup/maintenance authority. Trusted callers satisfy OPS-02 first.
struct MigrationLimits {
    qsizetype versions{256};
    // Migration IDs only: effective cap min(id_bytes,128); the fixed 21-byte
    // store identity is checked for equality, not against this configurable cap.
    qsizetype id_bytes{128};
    qsizetype schema_objects{256};
    // Sum UTF-8 bytes of every SQL text and every projected type/name/table/SQL
    // occurrence across all steps. No ID/identity, count, framing or NULL bytes.
    // Default 16 MiB. Callers can supply a different bounded budget.
    qsizetype manifest_utf8_bytes{16'777'216};
};

enum class MigrationMode { inspect, advance };
enum class MigrationCheckpoint { before_begin, before_statement, before_history, before_commit };
struct MigrationProgress {
    MigrationCheckpoint checkpoint;
    qint32 version;
    // One-based before_statement; zero for before_begin/before_commit. At
    // before_history, this is the completed SQL count for that version.
    qsizetype statement;
};

enum class CatalogState { fresh, upgrade_required, current };
struct MigrationReport {
    CatalogState state{CatalogState::fresh};
    qint32 version{0};
    qint32 target_version{0};
    qsizetype applied{0};
};

struct MigrationRequest {
    MigrationMode mode{MigrationMode::inspect};
    MigrationManifest manifest;
    MigrationLimits limits;
    // Trusted nonblocking cooperative probe. An empty probe never cancels. It
    // receives no SQL handle and cannot establish operational authority.
    std::function<bool(const MigrationProgress &)> cancel;
};

struct MigrationAdmission {
    StorageError error{StorageError::none};
    MigrationReason reason{MigrationReason::none};
};
struct MigrationResult {
    SqliteWorkResult work;
    // Work resource_limit/result_limit map to resource_limit here. A cancellation
    // probe exception maps to callback_failure, preserving the underlying first
    // work error; uncertain_outcome overrides every earlier migration reason.
    MigrationReason reason{MigrationReason::none};
    // Present only for normal, committed work with a normal owned work value.
    // Fresh/upgrade_required inspection reports never authorize normal store use.
    std::optional<MigrationReport> report;
};

// Pure canonical encoding boundaries, not catalog/manifest acceptance. They
// validate framing text/ranges and throw invalid_manifest for invalid encoding.
// A well-formed object encoding can still be an invalid catalog schema.
[[nodiscard]] QByteArray canonical_migration_payload(const MigrationStep &step);
[[nodiscard]] QByteArray migration_content_sha256(const MigrationStep &step);

class SqliteMigrationRunner final {
  public:
    using Completion = std::function<void(MigrationResult)>;
    // Precedence: pure request preflight (including mode enum), then nonempty
    // completion, then delegate to existing enqueue_work. Invalid requests return
    // invalid_options+invalid_manifest even for closed/foreign/full writers; null
    // completion is invalid_work+none. Valid requests retain delegated writer
    // errors with reason none. Rejection performs no SQL/callback. Accepted work owns ordinary
    // values; raw-backed captures/strings remain outside the trusted caller's ownership contract.
    [[nodiscard]] static MigrationAdmission enqueue(SqliteWriter &writer, MigrationRequest request,
                                                    Completion completion);
};
} // namespace subathon::server
