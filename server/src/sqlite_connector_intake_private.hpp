// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_connector_intake.hpp>

#include <variant>

namespace subathon::server::connector_intake::detail {

inline constexpr qsizetype reference_limit = 4096;
inline constexpr qsizetype document_limit = 16384;
inline constexpr qsizetype item_limit = 32768;
inline constexpr qsizetype page_limit = 262144;

inline bool bounded(const QByteArray &value, qsizetype limit) {
    return !value.isEmpty() && value.size() <= limit;
}

inline bool valid_scope(const Scope &scope) {
    return bounded(scope.installation, reference_limit) &&
           bounded(scope.provider, reference_limit) && bounded(scope.account, reference_limit) &&
           bounded(scope.stream, reference_limit);
}

inline bool local_installation(const SqliteWorkSession &session, const QByteArray &installation) {
    const auto rows = session.execute(
        {"SELECT installation_ref FROM main.subathon_account_installation "
         "WHERE singleton=1 AND typeof(installation_ref)='blob' AND length(installation_ref)<=4096",
         {}});
    return rows.size() == 1 && rows.front().front().toByteArray() == installation;
}

inline QList<QVariant> scope_bindings(const Scope &scope) {
    return {scope.installation, scope.provider, scope.account, scope.stream};
}

inline void require_changed_one(const SqliteWorkSession &session) {
    const auto rows = session.execute({"SELECT changes()", {}});
    if (rows.size() != 1 || rows.front().front().toLongLong() != 1)
        throw SqliteWorkFailure(WorkError::sql_failure);
}

// Shape and canonical size checks shared by operation staging and collision preflight.
[[nodiscard]] std::variant<QByteArray, HistoryStatus>
checked_operation_document(const OperationIntent &);

// One writer unit captures the exact active page before its generation moves.
[[nodiscard]] std::optional<QByteArray> retain_page_reconnect(const SqliteWorkSession &,
                                                              const Reconnect &,
                                                              const QByteArray &operation_ref,
                                                              const QByteArray &source_history);

[[nodiscard]] bool qualified_page_slot(const SqliteWorkSession &, const ManagedInput &,
                                       const Fence &current);

[[nodiscard]] QuarantineStatus preflight_page_quarantines(const SqliteWorkSession &,
                                                          const PageAdmission &);
[[nodiscard]] bool qualified_quarantine_slot(const SqliteWorkSession &, const QuarantineEntry &,
                                             bool replay);
void advance_page_prefix(const SqliteWorkSession &, const Scope &, const PageSlot &);

} // namespace subathon::server::connector_intake::detail

namespace subathon::server::connector_intake::recovery {
struct Recovery;
}

namespace subathon::server::connector_intake::recovery::detail {

bool matches_scope(const Scope &, const core::ReceiptIdentity &);
// Admission owns the authorized scope and replays only its own entry.
Recovery read_admitted_closure(const SqliteWorkSession &, const Scope &, const QByteArray &);
qsizetype admitted_readback_statement_width();

} // namespace subathon::server::connector_intake::recovery::detail
