// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QtCore/qxpfunctional.h>
#include <array>
#include <subathon/server/sqlite_migration_manifest.hpp>
#include <subathon/server/sqlite_rows.hpp>

namespace subathon::server::detail {

enum class ClockProbeQuery {
    encoding,
    schema,
    header,
    history,
    integrity,
    foreign_keys,
    legacy_guard
};

// Callers own the connection and any transaction or query metering.
using CatalogQuery = qxp::function_ref<SqlRows(ClockProbeQuery, const QString &) const>;

[[nodiscard]] qint32 inspect_store(const MigrationManifest &manifest,
                                   const QList<QByteArray> &hashes, CatalogQuery execute);
[[nodiscard]] QList<CatalogObject> read_catalog_schema(CatalogQuery execute);
void verify_catalog_schema(const QList<CatalogObject> &actual,
                           const QList<CatalogObject> &expected);

extern const QString header_ddl;
extern const QString history_ddl;
extern const QString store_identity;

struct EncodedObject {
    CatalogObject object;
    std::array<QByteArray, 5> fields;
};

[[noreturn]] void invalid_manifest();
void add_size(qsizetype &total, qsizetype amount);
qsizetype utf8_size(const QString &text);
QByteArray strict_utf8(const QString &text);
QList<EncodedObject> encoded_objects(const QList<CatalogObject> &objects);
QList<CatalogObject> reserved_objects();
bool valid_id(const QString &id, qsizetype limit);
bool integer_value(const QVariant &value);

} // namespace subathon::server::detail
