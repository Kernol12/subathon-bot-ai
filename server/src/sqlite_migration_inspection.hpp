// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_migrations.hpp>

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

// Inspect the shipped catalog using the same ordered queries and validation as
// the writer. The caller owns the connection, transaction and statement meter.
[[nodiscard]] qint32 inspect_clock_predecessor_catalog(
    MigrationManifest manifest,
    const std::function<SqlRows(ClockProbeQuery, const QString &)> &execute);

} // namespace subathon::server::detail
