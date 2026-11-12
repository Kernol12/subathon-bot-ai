// SPDX-License-Identifier: GPL-3.0-or-later
#include <limits>
#include <subathon/server/api_host_elapsed.hpp>
#include <time.h>

namespace subathon::server::api_host {
Result<std::uint64_t> convert_monotonic_observation(std::int64_t sec, std::int64_t ns) {
    constexpr std::uint64_t billion = 1000000000;
    if (sec < 0 || ns < 0 || ns >= std::int64_t(billion) ||
        std::uint64_t(sec) >
            (std::numeric_limits<std::uint64_t>::max() - std::uint64_t(ns)) / billion)
        return Failure::elapsed_unavailable;
    return std::uint64_t(sec) * billion + std::uint64_t(ns);
}
namespace {
class Monotonic final : public native_ws::ElapsedSource {
    std::variant<native_ws::ElapsedInstant, native_ws::ElapsedUnavailable>
    sample() const noexcept override {
        timespec t{};
        if (::clock_gettime(CLOCK_MONOTONIC, &t) != 0)
            return native_ws::ElapsedUnavailable{};
        const auto value = convert_monotonic_observation(t.tv_sec, t.tv_nsec);
        if (const auto *ns = std::get_if<std::uint64_t>(&value))
            return native_ws::ElapsedInstant{*ns};
        return native_ws::ElapsedUnavailable{};
    }
};
} // namespace
Result<std::shared_ptr<const native_ws::ElapsedSource>> make_linux_monotonic_elapsed() {
    try {
        return std::shared_ptr<const native_ws::ElapsedSource>(std::make_shared<Monotonic>());
    } catch (...) {
        return Failure::elapsed_unavailable;
    }
}
} // namespace subathon::server::api_host
