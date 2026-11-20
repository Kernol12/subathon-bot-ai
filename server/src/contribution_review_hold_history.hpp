// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "contribution_review_writer_access.hpp"

#include <subathon/server/sqlite_application_holds.hpp>

namespace subathon::server::review_detail {

struct ActiveHoldReadCost {
    detail::ReviewReadCost metadata;
    detail::ReviewReadCost full;
};

ApplicationHoldActiveSet read_active_holds(const SqliteWorkSession &, ApplicationHoldSubject,
                                           ActiveHoldReadCost &);

} // namespace subathon::server::review_detail
