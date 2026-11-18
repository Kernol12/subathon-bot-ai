// SPDX-License-Identifier: GPL-3.0-or-later
#include <cerrno>
#include <limits>
#include <optional>
#include <subathon/server/session_clock.hpp>
#include <subathon/server/testing/session_clock_linux.hpp>
#include <sys/timerfd.h>
#include <thread>
#include <unistd.h>
#include <utility>

namespace subathon::server::session_clock {
namespace {
constexpr std::uint64_t billion = 1000000000;
bool normalized(Time t) { return t.nanoseconds >= 0 && t.nanoseconds < std::int64_t(billion); }
int compare(Time a, Time b) {
    if (a.seconds != b.seconds)
        return a.seconds < b.seconds ? -1 : 1;
    return (a.nanoseconds > b.nanoseconds) - (a.nanoseconds < b.nanoseconds);
}
bool precision(Time q) { return q.seconds == 0 && q.nanoseconds > 0 && q.nanoseconds <= 1000000; }
bool native_expiry(Time r) {
    // Native timer range is distinct from the consumer's full signed64 ms range.
    if (!normalized(r) || r.seconds == std::numeric_limits<std::int64_t>::max())
        return false;
    const Time expiry{r.seconds + 1, r.nanoseconds};
    return compare(expiry, {0, 0}) > 0 && compare(expiry, {9223372036LL, 854775807}) <= 0;
}

// Fixed signed-magnitude normalized seconds/nanoseconds. This is exact even when
// an offset or total nanoseconds cannot fit i64; no extension/float/big-int heap.
struct Magnitude {
    std::uint64_t seconds, nanos;
};
struct Offset {
    bool negative;
    Magnitude mag;
};
int compare(Magnitude a, Magnitude b) {
    if (a.seconds != b.seconds)
        return a.seconds < b.seconds ? -1 : 1;
    return (a.nanos > b.nanos) - (a.nanos < b.nanos);
}
int compare(Offset a, Offset b) {
    if (a.negative != b.negative)
        return a.negative ? -1 : 1;
    const int result = compare(a.mag, b.mag);
    return a.negative ? -result : result;
}
Offset canonical(Offset a) {
    if (a.mag.seconds == 0 && a.mag.nanos == 0)
        a.negative = false;
    return a;
}
Offset exact(Time t) {
    if (t.seconds >= 0)
        return {false, {std::uint64_t(t.seconds), std::uint64_t(t.nanoseconds)}};
    const auto seconds = std::uint64_t(-(t.seconds + 1));
    return {true, t.nanoseconds ? Magnitude{seconds, billion - std::uint64_t(t.nanoseconds)}
                                : Magnitude{seconds + 1, 0}};
}
std::optional<Offset> add(Offset a, Offset b) {
    if (a.negative == b.negative) {
        const auto ns = a.mag.nanos + b.mag.nanos;
        const auto carry = ns / billion;
        constexpr auto max = std::numeric_limits<std::uint64_t>::max();
        if (a.mag.seconds > max - b.mag.seconds || a.mag.seconds + b.mag.seconds > max - carry)
            return {};
        return canonical({a.negative, {a.mag.seconds + b.mag.seconds + carry, ns % billion}});
    }
    if (compare(a.mag, b.mag) < 0)
        std::swap(a, b);
    const bool borrow = a.mag.nanos < b.mag.nanos;
    return canonical({a.negative,
                      {a.mag.seconds - b.mag.seconds - std::uint64_t(borrow),
                       a.mag.nanos + (borrow ? billion : 0) - b.mag.nanos}});
}
std::optional<Offset> difference(Time a, Time b) {
    auto negative_b = exact(b);
    negative_b.negative = !negative_b.negative;
    return add(exact(a), canonical(negative_b));
}
struct Interval {
    Offset lower, upper;
};
std::optional<Interval> interval(const Frame &f, Time q) {
    const auto low = difference(f.realtime, f.b1), high = difference(f.realtime, f.b0);
    if (!low || !high)
        return {};
    const Magnitude allowance{0, std::uint64_t(q.nanoseconds)};
    const auto lower = add(*low, {true, allowance}), upper = add(*high, {false, allowance});
    if (!lower || !upper)
        return {};
    return Interval{*lower, *upper};
}
bool healthy(Sync s) { return s.time_ok && !s.unsync && !s.clockerr && !s.insert && !s.remove; }
bool healthy(GuardRead g) {
    return g.result == Guard::again || (g.result == Guard::count && g.count > 0);
}

class LibcCalls final : public testing::LinuxCalls {
  public:
    int resolution(clockid_t id, timespec *out) override { return ::clock_getres(id, out); }
    int create(clockid_t id, int flags) override { return ::timerfd_create(id, flags); }
    int time(clockid_t id, timespec *out) override { return ::clock_gettime(id, out); }
    int arm(int fd, int flags, const itimerspec *value) override {
        return ::timerfd_settime(fd, flags, value, nullptr);
    }
    ssize_t read(int fd, void *out, std::size_t size) override { return ::read(fd, out, size); }
    int synchronization(timex *out) override { return ::adjtimex(out); }
    int close(int fd) noexcept override { return ::close(fd); }
};
class LinuxObservations final : public Observations {
  public:
    explicit LinuxObservations(std::unique_ptr<testing::LinuxCalls> calls)
        : calls_(std::move(calls)) {}
    ~LinuxObservations() override { close(); }
    std::variant<Time, SessionSourceUnavailable> initialize() override {
        if (!calls_ || fd_ >= 0)
            return SessionSourceUnavailable{};
        try {
            timespec real{}, boot{};
            // Both actual observations precede either value refusal (S9-CC-01).
            if (calls_->resolution(CLOCK_REALTIME, &real) != 0)
                return SessionSourceUnavailable{};
            if (calls_->resolution(CLOCK_BOOTTIME, &boot) != 0)
                return SessionSourceUnavailable{};
            const auto r = convert(real), b = convert(boot);
            if (!r || !b || !precision(*r) || !precision(*b))
                return SessionSourceUnavailable{};
            const Time q = compare(*r, *b) < 0 ? *b : *r;
            fd_ = calls_->create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
            if (fd_ < 0)
                return SessionSourceUnavailable{};
            Time now{};
            if (!time(CLOCK_REALTIME, now) || !native_expiry(now)) {
                close();
                return SessionSourceUnavailable{};
            }
            itimerspec timer{};
            const auto expiry = now.seconds + 1;
            if (!std::in_range<time_t>(expiry)) {
                close();
                return SessionSourceUnavailable{};
            }
            timer.it_value = {static_cast<time_t>(expiry), static_cast<long>(now.nanoseconds)};
            timer.it_interval = {1, 0};
            if (calls_->arm(fd_, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &timer) != 0) {
                close();
                return SessionSourceUnavailable{};
            }
            lost_ = false;
            return q;
        } catch (...) {
            close();
            return SessionSourceUnavailable{};
        }
    }
    std::variant<Frame, SessionSourceUnavailable> acquire() override {
        if (fd_ < 0 || lost_) {
            lost_ = true;
            return SessionSourceUnavailable{};
        }
        try {
            Frame f{};
            if (!time(CLOCK_BOOTTIME, f.bs) || !guard(f.d0))
                return SessionSourceUnavailable{};
            if (f.d0.result == Guard::cancelled)
                return f;
            if (!synchronization(f.a0) || !time(CLOCK_BOOTTIME, f.b0) ||
                !time(CLOCK_REALTIME, f.realtime) || !time(CLOCK_BOOTTIME, f.b1) ||
                !synchronization(f.a1) || !guard(f.d1))
                return SessionSourceUnavailable{};
            if (f.d1.result == Guard::cancelled)
                return f;
            if (!time(CLOCK_BOOTTIME, f.be))
                return SessionSourceUnavailable{};
            return f;
        } catch (...) {
            return SessionSourceUnavailable{};
        }
    }
    void close() noexcept override {
        if (fd_ >= 0) {
            const int old = std::exchange(fd_, -1);
            (void)calls_->close(old);
        }
        lost_ = true;
    }
    // Definite descriptor loss is a hold, unlike transient observation failure.
    // This is private producer state, not an injected trust/readiness capability.
    bool lost_guard() const noexcept { return lost_; }

  private:
    static std::optional<Time> convert(const timespec &t) {
        if (!std::in_range<std::int64_t>(t.tv_sec) || !std::in_range<std::int64_t>(t.tv_nsec))
            return {};
        const Time value{static_cast<std::int64_t>(t.tv_sec), static_cast<std::int64_t>(t.tv_nsec)};
        return normalized(value) ? std::optional{value} : std::nullopt;
    }
    bool time(clockid_t id, Time &out) {
        timespec t{};
        if (calls_->time(id, &t) != 0)
            return false;
        const auto value = convert(t);
        if (!value)
            return false;
        out = *value;
        return true;
    }
    bool guard(GuardRead &out) {
        std::uint64_t count = 0;
        const auto n = calls_->read(fd_, &count, sizeof(count));
        if (n < 0) {
            const int error = errno;
            if (error == EAGAIN) {
                out = {Guard::again, 0};
                return true;
            }
            if (error == ECANCELED) {
                out = {Guard::cancelled, 0};
                return true;
            }
            if (error == EBADF)
                lost_ = true;
            return false;
        }
        if (n != 8 || count == 0)
            return false;
        out = {Guard::count, count};
        return true;
    }
    bool synchronization(Sync &out) {
        timex status{};
        const int result = calls_->synchronization(&status);
        out = {result == TIME_OK, bool(status.status & STA_UNSYNC),
               bool(status.status & STA_CLOCKERR), bool(status.status & STA_INS),
               bool(status.status & STA_DEL)};
        return healthy(out);
    }
    std::unique_ptr<testing::LinuxCalls> calls_;
    int fd_ = -1;
    bool lost_ = false;
};
} // namespace

SessionClockResolution utc_milliseconds(Time t) {
    if (!normalized(t))
        return SessionSourceUnavailable{};
    const auto ms = std::uint64_t(t.nanoseconds / 1000000);
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    if (t.seconds >= 0) {
        if (t.seconds > max / 1000 || (t.seconds == max / 1000 && ms > std::uint64_t(max % 1000)))
            return SessionSourceUnavailable{};
        return SessionClockSample{t.seconds * 1000 + static_cast<std::int64_t>(ms)};
    }
    constexpr auto limit = std::uint64_t(max) + 1;
    const auto whole = std::uint64_t(-(t.seconds + 1)), tail = 1000 - ms;
    if (whole > (limit - tail) / 1000)
        return SessionSourceUnavailable{};
    const auto magnitude = whole * 1000 + tail;
    return SessionClockSample{magnitude == limit ? std::numeric_limits<std::int64_t>::min()
                                                 : -static_cast<std::int64_t>(magnitude)};
}
struct Monitor::Impl {
    const std::thread::id owner = std::this_thread::get_id();
    std::unique_ptr<Observations> observations;
    bool initialized = false, held = false, sampling = false;
    Time q{};
    std::optional<Interval> anchor;
    std::optional<Time> previous_boot, floor;
    explicit Impl(std::unique_ptr<Observations> source) : observations(std::move(source)) {}
    SessionClockResolution sample() {
        if (!observations)
            return SessionSourceUnavailable{};
        if (held && initialized) {
            observations->close();
            initialized = false;
        }
        if (!initialized) {
            const auto initial = observations->initialize();
            const auto value = std::get_if<Time>(&initial);
            if (!value)
                return SessionSourceUnavailable{};
            if (!precision(*value)) {
                observations->close();
                return SessionSourceUnavailable{};
            }
            q = *value;
            initialized = true;
        }
        const auto acquired = observations->acquire();
        const auto frame = std::get_if<Frame>(&acquired);
        if (!frame) {
            if (const auto native = dynamic_cast<const LinuxObservations *>(observations.get());
                native && native->lost_guard())
                held = true;
            return SessionSourceUnavailable{};
        }
        const auto &f = *frame;
        // Cancellation precedes every access to unused fields in a partial frame.
        if (f.d0.result == Guard::cancelled || f.d1.result == Guard::cancelled) {
            held = true;
            return SessionSourceUnavailable{};
        }
        if (!healthy(f.d0) || !healthy(f.d1) || !healthy(f.a0) || !healthy(f.a1))
            return SessionSourceUnavailable{};
        if (!normalized(f.bs) || !normalized(f.b0) || !normalized(f.b1) || !normalized(f.be) ||
            !normalized(f.realtime) || f.bs.seconds < 0 || f.b0.seconds < 0 || f.b1.seconds < 0 ||
            f.be.seconds < 0)
            return SessionSourceUnavailable{};
        if (compare(f.bs, f.b0) > 0 || compare(f.b0, f.b1) > 0 || compare(f.b1, f.be) > 0) {
            held = true;
            return SessionSourceUnavailable{};
        }
        const auto duration = difference(f.be, f.bs);
        if (!duration || duration->mag.seconds != 0 || duration->mag.nanos > 50000000 ||
            !native_expiry(f.realtime))
            return SessionSourceUnavailable{};
        auto candidate = interval(f, q);
        if (!candidate)
            return SessionSourceUnavailable{};
        if (!held && anchor) {
            if (compare(f.b0, *previous_boot) < 0 || compare(f.realtime, *floor) < 0) {
                held = true;
                return SessionSourceUnavailable{};
            }
            if (compare(anchor->lower, candidate->lower) > 0)
                candidate->lower = anchor->lower;
            if (compare(anchor->upper, candidate->upper) < 0)
                candidate->upper = anchor->upper;
            if (compare(candidate->lower, candidate->upper) > 0) {
                held = true;
                return SessionSourceUnavailable{};
            }
        }
        if (floor && compare(f.realtime, *floor) < 0) {
            held = true;
            return SessionSourceUnavailable{};
        }
        auto result = utc_milliseconds(f.realtime);
        if (!std::holds_alternative<SessionClockSample>(result))
            return SessionSourceUnavailable{};
        anchor = *candidate;
        previous_boot = f.b1;
        floor = f.realtime;
        held = false;
        return result;
    }
};
Monitor::Monitor(std::unique_ptr<Observations> source)
    : impl_(std::make_unique<Impl>(std::move(source))) {}
Monitor::~Monitor() {
    if (impl_->observations)
        impl_->observations->close();
}
SessionClockResolution Monitor::sample_current() {
    if (impl_->owner != std::this_thread::get_id() || impl_->sampling)
        return SessionSourceUnavailable{};
    impl_->sampling = true;
    struct Release {
        bool &value;
        ~Release() { value = false; }
    } release{impl_->sampling};
    try {
        return impl_->sample();
    } catch (...) {
        return SessionSourceUnavailable{};
    }
}
std::unique_ptr<Observations> linux_observations() {
    return std::make_unique<LinuxObservations>(std::make_unique<LibcCalls>());
}
} // namespace subathon::server::session_clock
namespace subathon::server::session_clock::testing {
std::unique_ptr<Observations> linux_observations(std::unique_ptr<LinuxCalls> calls) {
    return std::make_unique<LinuxObservations>(std::move(calls));
}
} // namespace subathon::server::session_clock::testing
