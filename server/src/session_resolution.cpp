// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/session_resolution.hpp>

#include <stdexcept>

namespace subathon::server {

PreparedAccountSession prepare_account_session(std::string_view) {
    throw std::logic_error(
        "Account session resolution is intentionally unimplemented; review tests first");
}

AccountSessionResolution resolve_current_account_session(const SessionLookupRequest &, bool,
                                                         CurrentSessionSource &,
                                                         CurrentSessionClock &,
                                                         const SessionLimits &) {
    throw std::logic_error(
        "Account session resolution is intentionally unimplemented; review tests first");
}

} // namespace subathon::server
