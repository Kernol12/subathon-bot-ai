// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/api_host.hpp>
namespace subathon::server::api_host {
struct DrogonOptions {
    std::string address = "127.0.0.1";
    std::uint16_t port = 0;
};
// Internal baseline transport binding, no authentication/production endpoint claim.
// HTTP POST /api-host/{diagnostic,installation,credential,account}, correlation
// is the lowercase-hex X-Host-Correlation header. WS /api-host/ws binary records:
// one correlation byte, one RouteId byte, remaining opaque body; replies retain
// correlation byte then reply bytes. All bounds/owners are host/native enforced.
Result<std::unique_ptr<Transport>> make_drogon_transport(DrogonOptions);
} // namespace subathon::server::api_host
