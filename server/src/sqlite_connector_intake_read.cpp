// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_connector_intake_private.hpp"

namespace subathon::server::connector_intake {
namespace {

using Status = SelectionStatus;
constexpr qsizetype selected_limit = 16;
constexpr qint64 entry_frame_bytes = 9;
constexpr qint64 response_frame_bytes = 5;

struct SelectedMetadata {
    QByteArray reference;
    qint64 document_bytes;
    qint64 closure_bytes;
    bool link_matches;
};

QList<SelectedMetadata> metadata(const SqliteWorkSession &session, const SelectedKey &key) {
    auto args = detail::scope_bindings(key.scope);
    args.append(key.after);
    const QString prefix = "SELECT e.intake_ref,length(e.input),length(e.provenance),";
    const QString suffix =
        " WHERE e.installation=? AND e.provider=? AND e.account=? AND e.stream=? ";
    // Only the indivisible read includes the exact retained receipt/attempt and
    // their arrival context links. Sum sizes in SQL before any payload fetch.
    const auto rows =
        key.exact
            ? session.execute(
                  {prefix +
                       " coalesce(length(c.document),0)+coalesce(length(m.document),0)+"
                       "coalesce(length(mc.document),0)+coalesce(length(rc.document),0)+"
                       "coalesce(length(rm.document),0)+coalesce(length(rmc.document),0)+"
                       "coalesce(length(a.connection_ref),0)+coalesce(length(a.event_type),0)+"
                       "coalesce(length(a.actor_id),0)+coalesce(length(a.actor_display_name),0)+"
                       "coalesce(length(a.money_original_decimal),0)+coalesce(length(a.money_"
                       "currency),0)+"
                       "coalesce(length(a.payment_namespace),0)+coalesce(length(a.payment_opaque_"
                       "id),0)+"
                       "coalesce(length(a.activity_namespace),0)+coalesce(length(a.activity_opaque_"
                       "id),0)+"
                       "coalesce(length(r.identifier_namespace),0)+coalesce(length(r.opaque_id),0)+"
                       "coalesce(length(r.connection_ref),0)+coalesce(length(r.event_type),0)+"
                       "coalesce(length(r.actor_id),0)+coalesce(length(r.actor_display_name),0)+"
                       "coalesce(length(r.money_original_decimal),0)+coalesce(length(r.money_"
                       "currency),0)+"
                       "coalesce(length(r.payment_namespace),0)+coalesce(length(r.payment_opaque_"
                       "id),0)+"
                       "coalesce(length(r.activity_namespace),0)+coalesce(length(r.activity_opaque_"
                       "id),0)+"
                       "coalesce(length(e.configuration_ref),0)+coalesce(length(e.modifier_context_"
                       "ref),0)+"
                       "coalesce(length(m.configuration_ref),0)+coalesce(length(r.configuration_"
                       "ref),0)+"
                       "coalesce(length(r.modifier_context_ref),0)+coalesce(length(rm."
                       "configuration_ref),0)+"
                       "coalesce(length(e.activity_namespace),0)+coalesce(length(e.activity_id),0)+"
                       "coalesce(length(e.verification_ref),0),"
                       "e.receipt_id IS NULL OR ("
                       "a.receipt_id IS NOT NULL AND a.configuration_ref=e.configuration_ref "
                       "AND a.modifier_context_ref=e.modifier_context_ref "
                       "AND a.server_timestamp_utc_ms=e.arrival_ms AND c.reference IS NOT NULL "
                       "AND m.reference IS NOT NULL AND r.provider=e.provider "
                       "AND r.source_account=e.account AND rc.reference IS NOT NULL "
                       "AND rm.reference IS NOT NULL) "
                       "FROM main.subathon_intake_entries e "
                       "LEFT JOIN main.subathon_receipts r ON r.receipt_id=e.receipt_id "
                       "LEFT JOIN main.subathon_receipt_attempts a ON a.receipt_id=e.receipt_id "
                       "AND a.ordinal=e.attempt_ordinal "
                       "LEFT JOIN main.subathon_retained_configurations c ON "
                       "c.reference=e.configuration_ref "
                       "LEFT JOIN main.subathon_retained_contexts m ON "
                       "m.reference=e.modifier_context_ref "
                       "LEFT JOIN main.subathon_retained_configurations mc ON "
                       "mc.reference=m.configuration_ref "
                       "LEFT JOIN main.subathon_retained_configurations rc ON "
                       "rc.reference=r.configuration_ref "
                       "LEFT JOIN main.subathon_retained_contexts rm ON "
                       "rm.reference=r.modifier_context_ref "
                       "LEFT JOIN main.subathon_retained_configurations rmc ON "
                       "rmc.reference=rm.configuration_ref" +
                       suffix + "AND e.intake_ref=?",
                   args})
            : session.execute({prefix + "0,1 FROM main.subathon_intake_entries e" + suffix +
                                   "AND e.intake_ref>? ORDER BY e.intake_ref LIMIT 17",
                               args});
    QList<SelectedMetadata> result;
    for (const auto &row : rows)
        result.append({row[0].toByteArray(), row[1].toLongLong(),
                       row[2].toLongLong() + row[3].toLongLong(), row[4].toLongLong() == 1});
    return result;
}

QByteArray input_document(const SqliteWorkSession &session, const Scope &scope,
                          const SelectedMetadata &selected) {
    auto args = detail::scope_bindings(scope);
    args.append(selected.reference);
    const auto rows = session.execute(
        {"SELECT input FROM main.subathon_intake_entries WHERE installation=? AND provider=? "
         "AND account=? AND stream=? AND intake_ref=?",
         args});
    if (rows.size() != 1 || rows.front().front().toByteArray().size() != selected.document_bytes)
        throw SqliteWorkFailure(WorkError::sql_failure);
    return rows.front().front().toByteArray();
}

} // namespace

SelectedPage inspect(const SqliteWorkSession &session, const SelectedKey &key) {
    if (!detail::valid_scope(key.scope) ||
        !detail::local_installation(session, key.scope.installation))
        return {Status::wrong_scope};
    if (key.after.size() > detail::reference_limit || (key.exact && key.after.isEmpty()))
        return {Status::intake_limit};

    auto cursor = key;
    if (cursor.after.isNull())
        cursor.after = QByteArray("", 0);
    const auto selected = metadata(session, cursor);
    if (key.exact && selected.isEmpty())
        return {Status::stale};

    qsizetype count = 0;
    qint64 used = response_frame_bytes;
    for (const auto &row : selected) {
        if (count == selected_limit)
            break;
        if (!row.link_matches || !detail::bounded(row.reference, detail::reference_limit) ||
            row.document_bytes < 1 || row.document_bytes > detail::item_limit ||
            row.closure_bytes < 1)
            return {Status::inconsistent};

        const auto available = detail::page_limit - used - entry_frame_bytes - row.reference.size();
        if (available < 0 || ((key.include_documents || key.exact) &&
                              (row.document_bytes > available ||
                               row.closure_bytes > available - row.document_bytes))) {
            if (count == 0)
                return {Status::intake_limit};
            break;
        }
        used += entry_frame_bytes + row.reference.size();
        if (key.include_documents || key.exact)
            used += row.document_bytes + row.closure_bytes;
        ++count;
    }

    SelectedPage result{Status::selected};
    for (qsizetype i = 0; i < count; ++i) {
        const auto &row = selected[i];
        result.entries.append({row.reference, key.include_documents ? std::optional{input_document(
                                                                          session, key.scope, row)}
                                                                    : std::nullopt});
    }
    if (count < selected.size())
        result.next_after = result.entries.back().ref;
    return result;
}

} // namespace subathon::server::connector_intake
