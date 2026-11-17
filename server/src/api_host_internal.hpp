// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/api_host.hpp>
namespace subathon::server::api_host::detail {
// Private production composition; no caller can inject a second native clock.
class TransportComposition {
  public:
    virtual ~TransportComposition() = default;
    virtual void peer_connected(const void *) = 0;
    virtual void handoff_peer(const void *, ResponseLease) = 0;
    virtual void configure(std::shared_ptr<const native_ws::ElapsedSource>, const Limits &) = 0;
};
bool signal_pending() noexcept;
} // namespace subathon::server::api_host::detail
