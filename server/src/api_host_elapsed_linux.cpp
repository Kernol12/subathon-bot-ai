// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/api_host_elapsed.hpp>
#include <limits>
#include <time.h>

namespace subathon::server::api_host {
Result<std::uint64_t> convert_monotonic_observation(std::int64_t seconds, std::int64_t nanoseconds) {
    constexpr std::uint64_t billion = 1000000000;
    if (seconds < 0 || nanoseconds < 0 || nanoseconds >= static_cast<std::int64_t>(billion))
        return Failure::elapsed_unavailable;
    const auto s = static_cast<std::uint64_t>(seconds);
    const auto n = static_cast<std::uint64_t>(nanoseconds);
    if (s > (std::numeric_limits<std::uint64_t>::max() - n) / billion)
        return Failure::elapsed_unavailable;
    return s * billion + n;
}
namespace {
class Monotonic final : public native_ws::ElapsedSource {
    std::variant<native_ws::ElapsedInstant, native_ws::ElapsedUnavailable> sample() const noexcept override {
        timespec ts{};
        if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
            return native_ws::ElapsedUnavailable{};
        const auto converted = convert_monotonic_observation(ts.tv_sec, ts.tv_nsec);
        if (const auto *n = std::get_if<std::uint64_t>(&converted))
            return native_ws::ElapsedInstant{*n};
        return native_ws::ElapsedUnavailable{};
    }
};
}
Result<std::shared_ptr<const native_ws::ElapsedSource>> make_linux_monotonic_elapsed() {
    return std::shared_ptr<const native_ws::ElapsedSource>(std::make_shared<Monotonic>());
}
} // namespace subathon::server::api_host
