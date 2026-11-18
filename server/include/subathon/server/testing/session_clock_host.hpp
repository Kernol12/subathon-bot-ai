// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/session_clock.hpp>
namespace subathon::server::session_clock::testing {
// Sample substitution only, following HostAccess; actual Host membership,
// startup/stop/lifetime and one-service registration cannot be substituted.
class HostClockAccess final {
  public:
    static std::unique_ptr<HostClock> create(api_host::Host &,
                                             std::shared_ptr<CurrentSessionClock>);
};
} // namespace subathon::server::session_clock::testing
