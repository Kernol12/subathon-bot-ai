// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/connector_intake_recovery.hpp>

namespace subathon::server::connector_intake::recovery::detail {

inline constexpr qsizetype capture_limit = 4105;
inline constexpr qsizetype detail_limit = 69732;
bool matches_scope(const Scope &, const core::ReceiptIdentity &);
bool current_capture(const QByteArray &);
std::optional<QByteArray> detail_reference(const QByteArray &);
bool matches_detail(const Closure &, std::optional<HistoryPurpose>);
// Admission already owns the authorized scope, and only replays its own entry.
Recovery read_admitted_closure(const SqliteWorkSession &, const Scope &, const QByteArray &,
                               AttemptLocator);

} // namespace subathon::server::connector_intake::recovery::detail
