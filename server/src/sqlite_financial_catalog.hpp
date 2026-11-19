// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_application_bundle.hpp>

namespace subathon::server::financial_sql {

inline bool selected(const SqliteWorkSession &session) {
    const auto rows = session.execute(
        {"SELECT CASE WHEN typeof(version)!='integer' THEN NULL WHEN version BETWEEN 1 AND 14 THEN "
         "0 "
         "WHEN version=15 AND migration_id COLLATE BINARY='financial-refunds-v1' THEN 1 "
         "WHEN version=16 AND migration_id COLLATE BINARY='provider-credentials-v1' THEN 1 END "
         "FROM main.subathon_migration_history WHERE EXISTS(SELECT 1 FROM main.subathon_catalog "
         "WHERE singleton=1 AND typeof(catalog_format)='integer' AND catalog_format=1 "
         "AND store_identity='subathon.sqlite-store') ORDER BY version DESC LIMIT 1",
         {}});
    if (rows.size() != 1 || rows.front().front().isNull())
        throw SqliteBundleStorageFailure(SqliteBundleStorageReason::invalid_stored_data);
    return rows.front().front().toInt() == 1;
}

} // namespace subathon::server::financial_sql
