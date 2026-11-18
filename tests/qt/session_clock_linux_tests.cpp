// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/sqlite_test_support.hpp"
#include <QTest>
#include <array>
#include <cerrno>
#include <cstring>
#include <limits>
#include <subathon/server/testing/session_clock_linux.hpp>
#include <sys/timerfd.h>

namespace c = subathon::server::session_clock;
using namespace subathon::server;
using qt_test_support::require;
namespace {
template <class F> void scenario(F f) {
    try {
        f();
    } catch (const std::exception &e) {
        QTest::qFail(e.what(), __FILE__, __LINE__);
    }
}
enum class Call { real_res, boot_res, create, real_time, arm, boot_time, guard, sync, close_fd };
// Fixed-storage ordinary QtTest fixture; no process/launcher/identity observation mode.
struct Audit {
    bool broken = false, descriptor_live = false;
    const char *reason = "";
};
// Runs after adapter destruction, OUTSIDE any adapter exception-to-unavailable catch.
struct AuditCheck {
    std::shared_ptr<Audit> state;
    ~AuditCheck() {
        if (state->broken)
            QTest::qFail(state->reason, __FILE__, __LINE__);
        if (state->descriptor_live)
            QTest::qFail("Fixture descriptor leaked at adapter destruction", __FILE__, __LINE__);
    }
};
struct Calls final : c::testing::LinuxCalls {
    std::shared_ptr<Audit> audit = std::make_shared<Audit>();
    std::array<Call, 64> log{};
    unsigned used = 0, fail_at = 99, closes = 0, fault_guard = 7, fault_sync = 8;
    std::shared_ptr<unsigned> released = std::make_shared<unsigned>(0);
    timespec realtime{10, 0}, boottime{1, 0}, real_resolution{0, 1}, boot_resolution{0, 1};
    ssize_t guard_bytes = 8;
    std::uint64_t count = 3;
    int guard_errno = EAGAIN, sync_return = TIME_OK, status = 0;
    bool use_guard_error = true;
    void validate(bool ok, const char *why) {
        if (!ok) {
            if (!audit->broken)
                audit->reason = why;
            audit->broken = true;
            throw std::runtime_error(why);
        }
    }
    bool record(Call call) {
        validate(used < log.size(), "Fixed native log overflow");
        log[used++] = call; // attempted operation recorded BEFORE argument validation
        if (used == fail_at) {
            errno = EIO;
            return false;
        }
        return true;
    }
    int resolution(clockid_t id, timespec *out) override {
        bool ok = record(id == CLOCK_REALTIME ? Call::real_res : Call::boot_res);
        validate((id == CLOCK_REALTIME || id == CLOCK_BOOTTIME) && out,
                 "Only real/boot resolution");
        if (!ok)
            return -1;
        *out = id == CLOCK_REALTIME ? real_resolution : boot_resolution;
        return 0;
    }
    int create(clockid_t id, int flags) override {
        bool ok = record(Call::create);
        validate(id == CLOCK_REALTIME && flags == (TFD_NONBLOCK | TFD_CLOEXEC),
                 "Exact guard creation flags");
        if (ok) {
            validate(!audit->descriptor_live, "Second simultaneous descriptor creation");
            audit->descriptor_live = true;
        }
        return ok ? 17 : -1;
    }
    int time(clockid_t id, timespec *out) override {
        bool ok = record(id == CLOCK_REALTIME ? Call::real_time : Call::boot_time);
        validate((id == CLOCK_REALTIME || id == CLOCK_BOOTTIME) && out,
                 "No fallback clock/null output");
        if (!ok)
            return -1;
        *out = id == CLOCK_REALTIME ? realtime : boottime;
        return 0;
    }
    int arm(int fd, int flags, const itimerspec *value) override {
        bool ok = record(Call::arm);
        validate(fd == 17 && flags == (TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET) && value,
                 "Exact arm arguments");
        validate(value->it_interval.tv_sec == 1 && value->it_interval.tv_nsec == 0,
                 "Exactly one-second interval");
        validate(realtime.tv_sec < std::numeric_limits<time_t>::max(),
                 "Overflowing native expiry attempted arm");
        validate(value->it_value.tv_sec == realtime.tv_sec + 1 &&
                     value->it_value.tv_nsec == realtime.tv_nsec,
                 "Initial expiry R+1s");
        return ok ? 0 : -1;
    }
    ssize_t read(int fd, void *out, std::size_t size) override {
        bool ok = record(Call::guard);
        validate(fd == 17 && size == 8 && out, "Exclusive fd and exactly8byte buffer");
        if (!ok)
            return -1;
        if (used != fault_guard) {
            errno = EAGAIN;
            return -1;
        }
        if (use_guard_error) {
            errno = guard_errno;
            return -1;
        }
        if (guard_bytes == 8)
            std::memcpy(out, &count, 8);
        return guard_bytes;
    }
    int synchronization(timex *out) override {
        bool ok = record(Call::sync);
        validate(out, "Null timex");
        validate(out->modes == 0 && out->offset == 0 && out->freq == 0 && out->maxerror == 0 &&
                     out->esterror == 0 && out->status == 0,
                 "Fresh zeroed read-only timex for BOTH reads");
        if (!ok)
            return -1;
        out->status = used == fault_sync ? status : 0;
        out->offset = 123;
        return used == fault_sync ? sync_return : TIME_OK;
    }
    int close(int fd) noexcept override {
        if (used == log.size()) {
            audit->broken = true;
            audit->reason = "Close log overflow";
            return -1;
        }
        log[used++] = Call::close_fd;
        if (fd != 17 || !audit->descriptor_live) {
            audit->broken = true;
            audit->reason = "Invalid/double close";
            return -1;
        }
        audit->descriptor_live = false;
        ++closes;
        ++*released;
        return 0;
    }
};
struct Adapter {
    AuditCheck audit;
    Calls *calls;
    std::unique_ptr<c::Observations> adapter;
    Adapter() : Adapter(std::make_unique<Calls>()) {}
    explicit Adapter(std::unique_ptr<Calls> port)
        : audit{port->audit}, calls(port.get()),
          adapter(c::testing::linux_observations(std::move(port))) {}
};
constexpr std::array<Call, 14> healthy{
    Call::real_res,  Call::boot_res, Call::create, Call::real_time, Call::arm,
    Call::boot_time, Call::guard,    Call::sync,   Call::boot_time, Call::real_time,
    Call::boot_time, Call::sync,     Call::guard,  Call::boot_time};
void prefix(const Calls &calls, unsigned count, bool closed = false) {
    require(!calls.audit->broken, calls.audit->reason);
    require(calls.used == count + unsigned(closed), "Exact attempted native prefix size");
    for (unsigned i = 0; i < count; ++i)
        require(calls.log[i] == healthy[i], "Exact native prefix order");
    if (closed)
        require(calls.log[count] == Call::close_fd, "Cleanup after prefix, before refusal returns");
}
void initialized(Adapter &a) {
    auto r = a.adapter->initialize();
    require(std::holds_alternative<c::Time>(r), "Healthy native init");
}
void refused(const std::variant<c::Frame, SessionSourceUnavailable> &r) {
    require(std::holds_alternative<SessionSourceUnavailable>(r), "Native frame refuses");
}
} // namespace
class SessionClockLinuxTests final : public QObject {
    Q_OBJECT
  private Q_SLOTS:
    void exact_order_and_cleanup() {
        scenario([] {
            Adapter a;
            initialized(a);
            auto r = a.adapter->acquire();
            require(std::holds_alternative<c::Frame>(r), "Complete native frame");
            require(a.calls->used == healthy.size(), "Five init + nine acquisition calls");
            for (unsigned i = 0; i < healthy.size(); ++i)
                require(a.calls->log[i] == healthy[i], "Contract exact native order");
            a.adapter->close();
            a.adapter->close();
            require(a.calls->closes == 1, "Exclusive descriptor closed exactly once");
            refused(a.adapter->acquire());
            require(a.calls->used == 15, "Closed descriptor never read or silently reopened");
        });
    }
    void syscall_failure_stops_data() {
        QTest::addColumn<int>("position");
        for (int i = 1; i <= 14; ++i)
            QTest::newRow(qPrintable(QString::number(i))) << i;
    }
    void syscall_failure_stops() {
        QFETCH(int, position);
        scenario([&] {
            Adapter a;
            a.calls->fail_at = position;
            auto init = a.adapter->initialize();
            if (position <= 5)
                require(std::holds_alternative<SessionSourceUnavailable>(init),
                        "Init failure refuses");
            else {
                require(std::holds_alternative<c::Time>(init), "Init healthy before frame fault");
                refused(a.adapter->acquire());
            }
            // Initialization with an acquired descriptor cleans it; frame failure
            // retains it until owner close. Neither retries or samples after failure.
            unsigned expected = position + (position == 4 || position == 5 ? 1 : 0);
            require(a.calls->used == expected,
                    "Fail immediately; only partial-init cleanup permitted");
            prefix(*a.calls, position, position == 4 || position == 5);
            require(a.calls->closes == unsigned(position == 4 || position == 5),
                    "Partial-init cleanup BEFORE ownerclose");
            a.adapter->close();
            require(a.calls->closes == unsigned(position >= 4),
                    "No lost or double closed descriptor");
        });
    }
    void destructor_and_expiration_count() {
        scenario([] {
            Adapter a;
            initialized(a);
            a.calls->use_guard_error = false;
            auto r = a.adapter->acquire();
            require(std::holds_alternative<c::Frame>(r) &&
                        std::get<c::Frame>(r).d0.result == c::Guard::count &&
                        std::get<c::Frame>(r).d0.count == 3,
                    "Positive8byte expiration count preserved");
            auto released = a.calls->released;
            a.adapter.reset();
            require(*released == 1, "Destructor releases one exclusive descriptor");
        });
    }
    void admitted_native_boundaries() {
        scenario([] {
            Adapter a;
            a.calls->real_resolution = {0, 1000000};
            a.calls->boot_resolution = {0, 1};
            a.calls->realtime = {9223372035LL, 854775807};
            auto init = a.adapter->initialize();
            require(std::holds_alternative<c::Time>(init) && std::get<c::Time>(init).seconds == 0 &&
                        std::get<c::Time>(init).nanoseconds == 1000000,
                    "q=max resolutions; exact1ms accepted; expiry exactly9223372036854775807ns");
            a.adapter->close();
        });
    }
    void monitor_uses_adapter_hold_and_single_descriptor_data() {
        QTest::addColumn<bool>("second");
        QTest::newRow("D0") << false;
        QTest::newRow("D1") << true;
    }
    void monitor_uses_adapter_hold_and_single_descriptor() {
        QFETCH(bool, second);
        scenario([&] {
            auto calls = std::make_unique<Calls>();
            auto raw = calls.get();
            AuditCheck fixture_audit{calls->audit};
            auto released = raw->released;
            {
                c::Monitor monitor(c::testing::linux_observations(std::move(calls)));
                auto initial = monitor.sample_current();
                require(std::holds_alternative<SessionClockSample>(initial) &&
                            std::get<SessionClockSample>(initial).now_utc_ms == 10000,
                        "Native adapter through real policy");
                raw->fault_guard = raw->used + (second ? 8 : 2);
                raw->guard_errno = ECANCELED;
                require(std::holds_alternative<SessionSourceUnavailable>(monitor.sample_current()),
                        "Cancellation detecting call latches hold, no same-call reopen");
                require(raw->used == unsigned(second ? 22 : 16) && raw->closes == 0,
                        "Stop at D0/D1 cancellation; no reads of unused partial-frame fields");
                for (unsigned i = 0; i < 14; ++i)
                    require(raw->log[i] == healthy[i], "Initial composition exact native prefix");
                const unsigned stopped = second ? 22 : 16;
                for (unsigned i = 14; i < stopped; ++i)
                    require(raw->log[i] == healthy[5 + i - 14],
                            "Exact prefix through detecting guard");
                raw->fault_guard = 99; // later healthy evidence, not a readiness setter
                auto later = monitor.sample_current();
                require(std::holds_alternative<SessionClockSample>(later),
                        "Later invocation may reestablish at same floor");
                require(raw->closes == 1 && raw->used == unsigned(second ? 37 : 31),
                        "Close+fresh14call path; no parallel guard");
                require(raw->log[stopped] == Call::close_fd,
                        "Old guard close precedes fresh initialization");
                for (unsigned i = 0; i < 14; ++i)
                    require(raw->log[stopped + 1 + i] == healthy[i],
                            "Exact fresh establishment prefix");
            }
            require(*released == 2,
                    "Old and new guards each closed once including final destruction");
        });
    }
    void leaving_native_range_refuses_without_wrapping() {
        scenario([] {
            auto calls = std::make_unique<Calls>();
            auto raw = calls.get();
            AuditCheck fixture_audit{calls->audit};
            raw->realtime = {9223372035LL, 854775807}; // R+1s exactly native MAX
            c::Monitor monitor(c::testing::linux_observations(std::move(calls)));
            auto initial = monitor.sample_current();
            require(std::holds_alternative<SessionClockSample>(initial),
                    "Healthy near-limit native control");
            require(std::get<SessionClockSample>(initial).now_utc_ms == 9223372035854LL,
                    "Exact near-limit UTC conversion");
            // B stays1s. I0=[9223372034854775806,9223372034854775808];
            // I1=[9223372034854775807,9223372034854775809] overlaps.
            raw->realtime = {9223372035LL, 854775808}; // +1ns => expiry MAX+1, not a discontinuity
            require(std::holds_alternative<SessionSourceUnavailable>(monitor.sample_current()),
                    "Live R+1s outside signed-nanosecond timer range refuses, no wrap/cache");
        });
    }
    void real_linux_read_only_observation() {
        scenario([] {
            auto adapter = c::linux_observations();
            require(bool(adapter), "Real Linux adapter wiring, not a fake TIME_OK");
            auto init = adapter->initialize();
            if (std::holds_alternative<c::Time>(init)) {
                auto result = adapter->acquire();
                if (std::holds_alternative<c::Frame>(result)) {
                    const auto &f = std::get<c::Frame>(result);
                    // A real failure/hold is a legitimate observation, never forced
                    // into supported UTC. Policy and host readiness remain separate.
                    require(f.d0.result == c::Guard::cancelled ||
                                f.d1.result == c::Guard::cancelled ||
                                (f.a0.time_ok && f.a1.time_ok),
                            "Classified real native observation");
                }
            }
            adapter->close();
        });
    }
    void resolution_and_native_range_data() {
        QTest::addColumn<int>("variant");
        int id = 0;
        for (auto name :
             {"zero-resolution", "over-1ms", "nonnormal-resolution", "negative-resolution",
              "expiry-zero", "expiry-over-native-max", "time-t-overflow", "nonnormal-realtime"}) {
            QTest::newRow(name) << id++;
        }
    }
    void resolution_and_native_range() {
        QFETCH(int, variant);
        scenario([&] {
            Adapter a;
            if (variant == 0)
                a.calls->real_resolution = {0, 0};
            if (variant == 1)
                a.calls->boot_resolution = {0, 1000001};
            if (variant == 2)
                a.calls->real_resolution = {0, 1000000000};
            if (variant == 3)
                a.calls->boot_resolution = {-1, 0};
            if (variant == 4)
                a.calls->realtime = {-1, 0};
            if (variant == 5)
                a.calls->realtime = {9223372035LL, 854775808};
            if (variant == 6)
                a.calls->realtime = {std::numeric_limits<time_t>::max(), 0};
            if (variant == 7)
                a.calls->realtime = {10, 1000000000};
            auto r = a.adapter->initialize();
            require(std::holds_alternative<SessionSourceUnavailable>(r),
                    "Native precision/expiry refuses");
            const auto count = variant < 4 ? 2u : 4u;
            prefix(*a.calls, count, variant >= 4);
            require(a.calls->closes == unsigned(variant >= 4),
                    "Init owns cleanup before returning refusal");
            a.adapter->close();
            prefix(*a.calls, count, variant >= 4); // ownerclose is idempotent, no delayed cleanup
            require(a.calls->closes == unsigned(variant >= 4),
                    "Exactly one close, no hidden descriptor");
        });
    }
    void guard_and_status_failures_data() {
        QTest::addColumn<int>("variant");
        QTest::addColumn<bool>("second");
        const std::array names{"cancelled",    "interrupted", "zero-read",       "short7",
                               "zero-count",   "TIME_ERROR",  "TIME_INS",        "TIME_DEL",
                               "TIME_OOP",     "TIME_WAIT",   "negative-status", "STA_UNSYNC",
                               "STA_CLOCKERR", "STA_INS",     "STA_DEL"};
        for (unsigned i = 0; i < names.size(); ++i)
            for (bool second : {false, true})
                QTest::newRow(
                    qPrintable(QString::fromLatin1(names[i]) + (second ? "-second" : "-first")))
                    << int(i) << second;
    }
    void guard_and_status_failures() {
        QFETCH(int, variant);
        QFETCH(bool, second);
        scenario([&] {
            Adapter a;
            initialized(a);
            a.calls->fault_guard = second ? 13 : 7;
            a.calls->fault_sync = second ? 12 : 8;
            if (variant < 2)
                a.calls->guard_errno = variant == 0 ? ECANCELED : EINTR;
            if (variant >= 2 && variant <= 4) {
                a.calls->use_guard_error = false;
                a.calls->guard_bytes = variant == 2 ? 0 : variant == 3 ? 7 : 8;
                a.calls->count = 0;
            }
            const std::array states{TIME_ERROR, TIME_INS, TIME_DEL, TIME_OOP, TIME_WAIT, -1};
            if (variant >= 5 && variant <= 10)
                a.calls->sync_return = states[variant - 5];
            const std::array flags{STA_UNSYNC, STA_CLOCKERR, STA_INS, STA_DEL};
            if (variant >= 11)
                a.calls->status = flags[variant - 11];
            auto result = a.adapter->acquire();
            // Cancellation must reach policy as a latched witness, never disappear
            // into a transient error that could recover on the old anchor.
            if (variant == 0) {
                require(std::holds_alternative<c::Frame>(result) &&
                            (second ? std::get<c::Frame>(result).d1 : std::get<c::Frame>(result).d0)
                                    .result == c::Guard::cancelled,
                        "Cancellation propagated to hold owner");
            } else
                refused(result);
            const unsigned count = variant <= 4 ? (second ? 13 : 7) : (second ? 12 : 8);
            prefix(*a.calls, count);
            require(a.calls->closes == 0,
                    "Frame refusal retains exclusive guard until owner close");
            a.adapter->close();
            prefix(*a.calls, count, true);
            require(a.calls->closes == 1, "Exactly once frame cleanup");
        });
    }
};
QTEST_GUILESS_MAIN(SessionClockLinuxTests)
#include "session_clock_linux_tests.moc"
