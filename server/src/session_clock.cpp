// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/session_clock.hpp>
#include <subathon/server/testing/session_clock_host.hpp>
#include <subathon/server/testing/session_clock_linux.hpp>
namespace subathon::server::session_clock {
struct Monitor::Impl {};
struct HostClock::Impl {};
std::unique_ptr<Observations> linux_observations() { throw NotImplemented{}; }
SessionClockResolution utc_milliseconds(Time) { throw NotImplemented{}; }
Monitor::Monitor(std::unique_ptr<Observations>) : impl_(std::make_unique<Impl>()) {}
Monitor::~Monitor() = default;
SessionClockResolution Monitor::sample_current() { throw NotImplemented{}; }
HostClock::HostClock(api_host::Host &) : impl_(std::make_unique<Impl>()) {}
HostClock::HostClock(api_host::Host &, std::shared_ptr<CurrentSessionClock>)
    : impl_(std::make_unique<Impl>()) {}
HostClock::~HostClock() = default;
std::shared_ptr<CurrentSessionClock> HostClock::bind(const SqliteWorkContext &) {
    throw NotImplemented{};
}
std::shared_ptr<CurrentSessionClock> HostClock::bind_completion(const api_host::CommandHandle &) {
    throw NotImplemented{};
}
} // namespace subathon::server::session_clock

namespace subathon::server::session_clock::testing {
std::unique_ptr<Observations> linux_observations(std::unique_ptr<LinuxCalls>) {
    throw NotImplemented{};
}
} // namespace subathon::server::session_clock::testing

namespace subathon::server::session_clock::testing {
std::unique_ptr<HostClock> HostClockAccess::create(api_host::Host &host,
                                                   std::shared_ptr<CurrentSessionClock> source) {
    // Inert stage1 construction, matching the unchanged NI constructors above.
    return std::unique_ptr<HostClock>(new HostClock(host, std::move(source)));
}
} // namespace subathon::server::session_clock::testing
