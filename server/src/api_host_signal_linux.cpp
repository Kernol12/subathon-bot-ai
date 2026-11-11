// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/api_host_signal.hpp>
namespace subathon::server::api_host {
// EXPLICIT UNIMPLEMENTED: no signal registration, pipe or forged stop barrier.
struct StopSignalBridge::Impl {};
StopSignalBridge::StopSignalBridge(Host &) {}
StopSignalBridge::~StopSignalBridge() = default;
Result<StartAccepted> StopSignalBridge::install() { return NotImplemented{}; }
} // namespace subathon::server::api_host
