// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/api_host_drogon.hpp>
namespace subathon::server::api_host {
// EXPLICIT UNIMPLEMENTED: no framework configuration/listener/thread or success.
Result<std::unique_ptr<Transport>> make_drogon_transport(DrogonOptions) { return NotImplemented{}; }
} // namespace subathon::server::api_host
