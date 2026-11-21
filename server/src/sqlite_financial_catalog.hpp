// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_milestone_detail.hpp"

namespace subathon::server::financial_sql {

// A legacy closure has a v1 document and carries the same envelope across resolution.
// Any other selected hold needs the financial owner's readback before it can be excluded.
inline const QString hold_candidates_for_subject =
    " FROM main.subathon_financial_refunds AS f "
    "LEFT JOIN main.subathon_financial_resolutions AS r ON r.hold_ref=f.hold_ref "
    "WHERE f.subject=? AND f.hold_ref IS NOT NULL "
    "AND (r.hold_ref IS NULL OR typeof(r.document)<>'blob' "
    "OR substr(r.document,1,8)<>X'0000000000000001' OR NOT EXISTS ("
    "SELECT 1 FROM main.subathon_app_states b CROSS JOIN main.subathon_app_states a "
    "WHERE b.token=r.before_token AND a.token=r.after_token "
    "AND b.format_ref=a.format_ref AND b.document=a.document))";

inline bool selected(const SqliteWorkSession &session) {
    const auto rows = session.execute(
        {"SELECT CASE WHEN typeof(version)!='integer' THEN NULL WHEN version BETWEEN 1 AND 14 THEN "
         "0 "
         "WHEN version=15 AND migration_id COLLATE BINARY='financial-refunds-v1' THEN 1 "
         "WHEN version=16 AND migration_id COLLATE BINARY='provider-credentials-v1' THEN 1 "
         "WHEN version=17 AND migration_id COLLATE BINARY='rate-overrides-v1' THEN 1 "
         "WHEN version=18 AND migration_id COLLATE BINARY='milestone-target-edits-v1' THEN 2 "
         "WHEN version=19 AND migration_id COLLATE BINARY='contribution-review-v1' THEN 2 "
         "WHEN version=20 AND typeof(migration_id)='text' "
         "AND migration_id COLLATE BINARY='connector-intake-storage-v1' THEN 2 END "
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
