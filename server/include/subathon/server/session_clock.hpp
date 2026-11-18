// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <exception>
#include <memory>
#include <subathon/server/api_host.hpp>
#include <subathon/server/session_resolution.hpp>

namespace subathon::server::session_clock {
namespace testing {
class HostClockAccess;
}
struct NotImplemented final : std::exception {
    const char *what() const noexcept override { return "SESSION-009 clock unimplemented"; }
};
// Normalized pairs preserve native representation separately from consumer i64 ms.
struct Time {
    std::int64_t seconds, nanoseconds;
};
enum class Guard { again, count, cancelled, interrupted, short_read, failed };
struct GuardRead {
    Guard result = Guard::again;
    std::uint64_t count = 0;
};
struct Sync {
    bool time_ok = true, unsync = false, clockerr = false, insert = false, remove = false;
};
struct Frame {
    Time bs, b0, realtime, b1, be;
    GuardRead d0, d1;
    Sync a0, a1;
};
// Internal observation port, not a public trust/readiness setter. Linux implementation
// owns ONE exclusive timerfd. Each acquire performs the exact §3 ordered sequence,
// stops at first failure, zeroes both timex objects/modes, never retries or rearms.
// initialize preflights resolutions and native expiry; failure closes partial state.
class Observations {
  public:
    virtual ~Observations() = default;
    virtual std::variant<Time, SessionSourceUnavailable> initialize() = 0; // q
    // Cancellation stops immediately and returns a value-initialized partial Frame
    // with d0/d1.cancelled; Monitor inspects cancellation BEFORE unused fields and
    // latches hold. Other failed observations return unavailable, never partial UTC.
    virtual std::variant<Frame, SessionSourceUnavailable> acquire() = 0;
    virtual void close() noexcept = 0;
};
std::unique_ptr<Observations> linux_observations();
SessionClockResolution utc_milliseconds(Time);
// Policy-only component: fixed exact K/floor/last-B, one observation owner, no SQL,
// persistent anchor, clock adjustment, readiness claim or history. One attempt/call.
class Monitor final : public CurrentSessionClock {
  public:
    explicit Monitor(std::unique_ptr<Observations>);
    ~Monitor() override;
    SessionClockResolution sample_current() override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
// Host-owned binding integration. Constructor is inert; service/handles remain safely inert after
// host destruction. bind accepts ONLY the context currently entering this Host's actual prepared
// unit, on Qt, after real activation. Default/other-writer contexts grant nothing. Handle expires
// at body exit (also throw), before completion/next queued work, stop or host destruction. Each
// call checks scope/thread/reentrancy/readiness FIRST, then delegates exactly once. No
// callback/time/readiness supplied by ingress. Publication uses a distinct operation-owned scope at
// normal matching completion; bind(context) never grants publication authority or extends a body
// ticket.
// Internal binding uses detail/session_clock_host.hpp. Stage2 touchpoints:
// api_host.cpp Engine slot/forward scopes/stop/end/Host destruction; private friend
// declarations in api_host.hpp and SqliteWorkContext in sqlite_writer.hpp. No writer
// .cpp behavior change or public Host API change. A second service is permanently
// inert; only the first lifetime claim may own the one lazy monitor/descriptor.
class HostClock final {
  public:
    explicit HostClock(api_host::Host &);
    ~HostClock();
    // Denied binding returns a nonnull inert unavailable handle, never throws a
    // source diagnostic. NI scaffolding remains an explicit exception until stage2.
    std::shared_ptr<CurrentSessionClock> bind(const SqliteWorkContext &);
    // Only this command's real last-unit completion after committed/none/value,
    // normal body return and no intervening state work. Host's actual outcome is
    // authoritative, never a caller-supplied result/ready flag. Expires on callback
    // exit/finish/stop; lifecycle still validates its private exact envelope/emitter.
    std::shared_ptr<CurrentSessionClock> bind_completion(const api_host::CommandHandle &);

  private:
    HostClock(api_host::Host &, std::shared_ptr<CurrentSessionClock>);
    friend class testing::HostClockAccess;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace subathon::server::session_clock
