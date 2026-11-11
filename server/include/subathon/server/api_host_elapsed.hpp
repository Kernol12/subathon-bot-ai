// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/api_host.hpp>
namespace subathon::server::api_host {
Result<std::uint64_t> convert_monotonic_observation(std::int64_t seconds, std::int64_t nanoseconds);
Result<std::shared_ptr<const native_ws::ElapsedSource>> make_linux_monotonic_elapsed();
} // namespace subathon::server::api_host
