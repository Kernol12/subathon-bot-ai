// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <QList>
#include <QString>
#include <optional>
#include <stdexcept>

namespace subathon::server {
struct CatalogObject {
    QString type;
    QString name;
    QString table_name;
    std::optional<QString> sql;
    bool operator==(const CatalogObject &) const = default;
};

struct MigrationStep {
    qint32 version{0};
    QString id;
    QList<QString> sql;
    QList<CatalogObject> expected_schema;
};

struct MigrationManifest {
    qint32 catalog_format{1};
    QString store_identity{"subathon.sqlite-store"};
    QList<MigrationStep> steps;
    QList<qint32> supported_starting_versions;
};

enum class MigrationReason {
    none,
    feature_not_implemented,
    invalid_manifest,
    cancelled,
    foreign_store,
    catalog_mismatch,
    unsupported_catalog_format,
    unsupported_version,
    unsupported_encoding,
    schema_mismatch,
    integrity_failed,
    inspection_failed,
    busy,
    sql_failure,
    resource_limit,
    callback_failure,
    uncertain_outcome,
    writer_unavailable,
    // Appended under 607.6: a confirmed v23 commit whose owned connection
    // settings could not be restored. The connection is retired.
    committed_restore_failed
};

class MigrationFailure final : public std::runtime_error {
  public:
    explicit MigrationFailure(MigrationReason reason)
        : std::runtime_error("SQLite migration operation failed"), reason_(reason) {}
    [[nodiscard]] MigrationReason reason() const noexcept { return reason_; }

  private:
    MigrationReason reason_;
};

} // namespace subathon::server
