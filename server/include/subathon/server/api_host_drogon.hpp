// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/api_host.hpp>
namespace subathon::server::api_host {
struct DrogonOptions {
    std::string address = "127.0.0.1";
    std::uint16_t port = 0;              // native WS; zero requests an ephemeral listener
    std::uint16_t http_port = 0;         // ordinary HTTP, independently ephemeral
    std::string scratch_parent = "/tmp"; // existing absolute parent, bounded by contract
};
struct ListenerEndpoints {
    std::array<char, 46> address{};
    std::uint16_t port = 0, http_port = 0; // actual distinct, nonzero ports
    std::array<char, 534> scratch_root{};
    // Actual pre-listen readback: bit0 SO_REUSEADDR, bit1 SO_REUSEPORT; each is 1.
    std::uint8_t native_socket_options = 0, http_socket_options = 0;
};
using ListenersObserver = std::function<void(const ListenerEndpoints &)>;
// Observer runs at most once on Qt for the actually listening pair, before startup
// success; a throw yields unavailable and failed-start cleanup, never serving.
// No observer is required. It is data-only, never readiness or socket authority.
// Internal baseline transport binding, no authentication/production endpoint claim.
// HTTP POST /api-host/{diagnostic,installation,credential,account}, correlation
// is the lowercase-hex X-Host-Correlation header. WS /api-host/ws binary records:
// one correlation byte, one RouteId byte, remaining opaque body; replies retain
// correlation byte then reply bytes. All bounds/owners are host/native enforced.
Result<std::unique_ptr<Transport>> make_drogon_transport(DrogonOptions, ListenersObserver = {});
} // namespace subathon::server::api_host
