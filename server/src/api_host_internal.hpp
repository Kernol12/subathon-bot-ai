// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/api_host.hpp>
namespace subathon::server::api_host::detail {
// The Host Engine's once-only fatal relay (API-023 r4 §2). publish is noexcept,
// callable from the transport's IO owner, and only stores the first valid value:
// no allocation, Qt call, log, wait, wake or quit. The Engine outlives its
// transport, so the binding stays valid for every transport callback.
struct FatalRelay {
    void *owner;
    void (*publish)(void *owner, bounded_http::FatalElapsed) noexcept;
};
// Private production composition; no caller can inject a second native clock.
class TransportComposition {
  public:
    virtual ~TransportComposition() = default;
    virtual void peer_connected(const void *) = 0;
    virtual void close_peer(const void *) = 0;
    virtual void handoff_peer(const void *, ResponseLease) = 0;
    virtual void configure(std::shared_ptr<const native_ws::ElapsedSource>, const Limits &) = 0;
    // Called once on the Qt owner before transport start.
    virtual void bind_fatal(FatalRelay) noexcept = 0;
};
bool signal_pending() noexcept;
} // namespace subathon::server::api_host::detail
