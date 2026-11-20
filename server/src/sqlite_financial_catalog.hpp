// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_milestone_detail.hpp"

namespace subathon::server::financial_sql {

inline bool selected(const SqliteWorkSession &session) {
    const auto rows = session.execute(
        {"SELECT CASE WHEN typeof(version)!='integer' THEN NULL WHEN version BETWEEN 1 AND 14 THEN "
         "0 "
         "WHEN version=15 AND migration_id COLLATE BINARY='financial-refunds-v1' THEN 1 "
         "WHEN version=16 AND migration_id COLLATE BINARY='provider-credentials-v1' THEN 1 "
         "WHEN version=17 AND migration_id COLLATE BINARY='rate-overrides-v1' THEN 1 "
         "WHEN version=18 AND migration_id COLLATE BINARY='milestone-target-edits-v1' THEN 2 END "
         "FROM main.subathon_migration_history WHERE EXISTS(SELECT 1 FROM main.subathon_catalog "
         "WHERE singleton=1 AND typeof(catalog_format)='integer' AND catalog_format=1 "
         "AND store_identity='subathon.sqlite-store') ORDER BY version DESC LIMIT 1",
         {}});
    if (rows.size() != 1 || rows.front().front().isNull())
        throw SqliteBundleStorageFailure(SqliteBundleStorageReason::invalid_stored_data);
    const auto selected = milestone_sql::integer(rows.front().front());
    if (selected == 2) {
        milestone_sql::require_edit_schema(session);
        (void)session.execute(
            {"SELECT f.operation_scope,f.operation_ref,f.payment,f.position,f.receipt_id,"
             "f.economic,f.subject,f.time_ordinal,f.restored,f.hold_ref,f.document,"
             "h.payment,h.support_receipt,h.position,b.original_scope,b.original_operation,"
             "b.original_ordinal,b.kind,b.position,b.operation_scope,b.operation_ref,"
             "r.operation_scope,r.operation_ref,r.hold_ref,r.before_token,r.advanced_token,"
             "r.after_token,r.document "
             "FROM main.subathon_financial_refunds f "
             "CROSS JOIN main.subathon_financial_payment_heads h "
             "CROSS JOIN main.subathon_financial_bases b "
             "CROSS JOIN main.subathon_financial_resolutions r LIMIT 0",
             {}});
    }
    return selected != 0;
}

} // namespace subathon::server::financial_sql
