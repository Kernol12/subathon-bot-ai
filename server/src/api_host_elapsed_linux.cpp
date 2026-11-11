// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/api_host_elapsed.hpp>
namespace subathon::server::api_host {
// EXPLICIT UNIMPLEMENTED: no native observation or substitute conversion.
Result<std::uint64_t> convert_monotonic_observation(std::int64_t, std::int64_t) {
    return NotImplemented{};
}
Result<std::shared_ptr<const native_ws::ElapsedSource>> make_linux_monotonic_elapsed() {
    return NotImplemented{};
}
} // namespace subathon::server::api_host
