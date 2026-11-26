// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/connector_intake_recovery.hpp>

namespace subathon::server::connector_intake::recovery::detail {

inline constexpr qsizetype capture_limit = 4105;
inline constexpr qsizetype detail_limit = 69732;
bool current_capture(const QByteArray &);
std::optional<QByteArray> detail_reference(const QByteArray &);
bool matches_detail(const Closure &, std::optional<HistoryPurpose>, const QByteArray &policy);

} // namespace subathon::server::connector_intake::recovery::detail
