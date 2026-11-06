// SPDX-License-Identifier: GPL-3.0-or-later
// Nonvalidating fixture foundations, never an owner/decoder/worker substitute.
#include "native_fault_state.h"
#include "native_fault_v1.h"
#include "native_foundation_child.h"
#include "transport_v1.h"
#include <gtest/gtest.h>
#include <linux/sched.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <type_traits>
#include <unistd.h>

namespace {
namespace native = subathon::schema_validation::native;
namespace fault = native::fault;
using schema_foundation::address;
using schema_foundation::raw;
constexpr int own_thread = 0x20000000;
constexpr native::Phase read_before{native::PhaseBase::config_read, native::PhaseEdge::before};
constexpr native::Phase birth_after{native::PhaseBase::atomic_birth, native::PhaseEdge::after};
constexpr native::Phase close_after{native::PhaseBase::pidfd_close, native::PhaseEdge::after};
constexpr long nanosecond = 1000000000;
static_assert(std::is_same_v<decltype(&subathon_schema_fault_arm_v1),
                             int (*)(unsigned int, unsigned int, int) noexcept>);
static_assert(std::is_same_v<decltype(&subathon_schema_fault_snapshot_v1),
                             int (*)(std::int64_t *, unsigned int) noexcept>);
static_assert(std::is_same_v<decltype(&subathon_schema_fault_report_v1),
                             int (*)(unsigned char *, unsigned int) noexcept>);
static_assert(std::is_same_v<decltype(&subathon_schema_fault_fallback_v1), int (*)(void) noexcept>);

long clock_now() noexcept {
    timespec value;
    if (raw(SYS_clock_gettime, CLOCK_MONOTONIC, address(&value)) != 0 || value.tv_sec < 0 ||
        value.tv_nsec < 0 || value.tv_nsec >= nanosecond ||
        value.tv_sec > (INT64_MAX - value.tv_nsec) / nanosecond)
        return -1;
    return value.tv_sec * nanosecond + value.tv_nsec;
}
long wait_owned(int descriptor, siginfo_t &info, int flags) noexcept {
    long result;
    do {
        info = {};
        result = raw(SYS_waitid, 3, descriptor, address(&info), flags | own_thread, 0);
    } while (result == -EINTR);
    return result;
}
bool exit_ready(int descriptor) noexcept {
    pollfd item{descriptor, POLLIN, 0};
    long result;
    do {
        result = raw(SYS_poll, address(&item), 1, 0);
    } while (result == -EINTR);
    return result == 1 && (item.revents & (POLLIN | POLLHUP)) &&
           !(item.revents & (POLLERR | POLLNVAL));
}

struct Fixture {
    int armed = -2, initial = -2, disposed = -2, fd_count = 0;
    const void *map = nullptr;
    long mapped_before = -1, mapped_after = -1;
    int descriptors[6]{-1, -1, -1, -1, -1, -1};
    long flags[6]{}, closed[6]{};
    std::int64_t cells[1024]{};
    bool finished = false;

    void start(unsigned which, unsigned phase = 0, int error = 0) noexcept {
        armed = subathon_schema_fault_arm_v1(which, phase, error);
        map = fault::mapping_address();
        if (armed != 0 || !map)
            return;
        unsigned char residency[3];
        mapped_before = raw(SYS_mincore, address(map), 12288, address(residency));
        initial = subathon_schema_fault_snapshot_v1(cells, 1024);
        if (initial < 0 || initial > 128)
            return;
        for (int row = 0; row < initial; ++row) {
            const auto *entry = cells + 8 * row;
            if ((entry[0] >> 32) == 2 && entry[1] == 3 && entry[3] >= 0 && fd_count < 6) {
                descriptors[fd_count] = static_cast<int>(entry[3]);
                flags[fd_count] = raw(SYS_fcntl, descriptors[fd_count], F_GETFD);
                ++fd_count;
            }
        }
    }
    int snapshot() noexcept { return subathon_schema_fault_snapshot_v1(cells, 1024); }
    void finish() noexcept {
        if (finished)
            return;
        disposed = subathon_schema_fault_fallback_v1();
        if (fault::mapping_address())
            schema_foundation::finish(130); // Unproved disposal is fixture failure, not green.
        for (int index = 0; index < fd_count; ++index)
            closed[index] = raw(SYS_fcntl, descriptors[index], F_GETFD);
        if (map) {
            unsigned char residency[3];
            mapped_after = raw(SYS_mincore, address(map), 12288, address(residency));
        }
        finished = true;
    }
    ~Fixture() { finish(); }
};

void check_fixture(const Fixture &value, int disposal = 0) {
    EXPECT_EQ(value.armed, 0);
    EXPECT_NE(value.map, nullptr);
    EXPECT_EQ(value.mapped_before, 0);
    EXPECT_GE(value.initial, 7);
    EXPECT_LE(value.initial, 128);
    EXPECT_EQ(value.fd_count, 6);
    EXPECT_EQ(value.disposed, disposal);
    EXPECT_EQ(value.mapped_after, -ENOMEM);
    for (int index = 0; index < value.fd_count; ++index) {
        EXPECT_GE(value.descriptors[index], 0);
        EXPECT_GE(value.flags[index], 0);
        EXPECT_EQ(value.closed[index], -EBADF);
        for (int previous = 0; previous < index; ++previous) {
            EXPECT_NE(value.descriptors[index], value.descriptors[previous]);
        }
    }
}
int count_writer(const std::int64_t *cells, int count, unsigned writer) noexcept {
    int found = 0;
    if (count < 0 || count > 128)
        return -1;
    for (int row = 0; row < count; ++row)
        found += (cells[8 * row] >> 32) == writer;
    return found;
}
bool event(const std::int64_t *cells, int count, unsigned writer, long kind, long result,
           long phase = -2, long pid = -2) noexcept {
    if (count < 0 || count > 128)
        return false;
    for (int row = 0; row < count; ++row) {
        const auto *entry = cells + 8 * row;
        if ((entry[0] >> 32) == writer && entry[1] == kind && entry[5] == result &&
            (phase == -2 || entry[2] == phase) && (pid == -2 || entry[4] == pid))
            return true;
    }
    return false;
}
bool token(int descriptor, char expected) noexcept {
    const long start = clock_now();
    if (start < 0 || start > INT64_MAX - 3 * nanosecond)
        return false;
    for (;;) {
        const long now = clock_now();
        if (now < 0 || now >= start + 3 * nanosecond)
            return false;
        pollfd item{descriptor, POLLIN, 0};
        const long remaining = (start + 3 * nanosecond - now + 999999) / 1000000;
        const long ready = raw(SYS_poll, address(&item), 1, remaining);
        if (ready == -EINTR)
            continue; // Same absolute deadline, never repeated sleeps/poll guesses.
        if (ready != 1 || !(item.revents & POLLIN))
            return false;
        char value = 0;
        long read;
        do {
            read = raw(SYS_read, descriptor, address(&value), 1);
        } while (read == -EINTR);
        return read == 1 && value == expected;
    }
}

// Only this fixed primitive child graph excludes instrumentation; generated
// calls/TLS/helpers must be audited before execution, as for the prior raw child.
[[noreturn, gnu::no_stack_protector]] void primitive_child(unsigned mode,
                                                           const int *coord) noexcept {
    schema_foundation::KernelAction action;
    action.handler = 0;
    action.flags = 0;
    action.restorer = 0;
    action.mask = 0;
    std::uint64_t mask = 0;
    if (raw(SYS_rt_sigaction, SIGALRM, address(&action), 0, 8) != 0 ||
        raw(SYS_rt_sigprocmask, SIG_SETMASK, address(&mask), 0, 8) != 0 || raw(SYS_alarm, 5) < 0)
        schema_foundation::finish(76);
    if (mode == 1)
        schema_foundation::finish(73); // Fixed exit for the deliberate competing reaper.
    if (mode == 3)
        schema_foundation::finish(125); // Real reserved fixture failure, never owner refusal.
    if (mode == 2) {
        raw(SYS_close, coord[0]);
        raw(SYS_close, coord[3]);
        for (unsigned index = 0; index < 32; ++index) {
            const native::Observation value{native::ObservationKind::syscall_return, read_before,
                                            -1, raw(SYS_getpid), -EIO};
            if (!fault::record(fault::Writer::child, value))
                schema_foundation::finish(77);
        }
        const char ready = 'R';
        char release = 0;
        if (raw(SYS_write, coord[1], address(&ready), 1) != 1 ||
            raw(SYS_read, coord[2], address(&release), 1) != 1 || release != 'G')
            schema_foundation::finish(78);
    } else if (!fault::child_ready_and_hold()) {
        schema_foundation::finish(74);
    }
    if (mode == 2) {
        const native::Observation excess{native::ObservationKind::syscall_return, read_before, -1,
                                         raw(SYS_getpid), 0};
        if (fault::record(fault::Writer::child, excess))
            schema_foundation::finish(75);
    }
    schema_foundation::finish(73);
}

struct Driver {
    long pid = -1, birth_error = 0, waited = -1, close_result = -1, fd_after = -1;
    int pidfd = -1, code = -1, status = -1;
    bool recorded = false, pinned = false, gone = false, own_reap = false;
    bool fallback = false, finished = false;

    void start(unsigned mode = 0, const int *coord = nullptr) noexcept {
        int committed = -1;
        clone_args arguments{};
        arguments.flags = CLONE_PIDFD | CLONE_CLEAR_SIGHAND;
        arguments.exit_signal = SIGCHLD;
        arguments.pidfd = reinterpret_cast<std::uint64_t>(&committed);
        pid = ::syscall(SYS_clone3, &arguments, sizeof(arguments));
        if (pid == 0)
            primitive_child(mode, coord); // Never C++/GTest unwinding in the copied child.
        if (pid < 0) {
            birth_error = errno;
            return; // Failed output cell remains entirely unowned.
        }
        pidfd = committed; // Successful PID+pidfd adopted BEFORE fallible work.
        if (pidfd < 0) {
            int ignored;
            long result;
            do {
                result = raw(SYS_wait4, pid, address(&ignored), own_thread, 0);
            } while (result == -EINTR);
            schema_foundation::finish(132); // No guessed output descriptor or numeric kill.
        }
        const native::Observation adopted{native::ObservationKind::adopt, birth_after, pidfd, pid,
                                          pidfd};
        recorded = fault::record(fault::Writer::parent, adopted);
        if (recorded)
            pinned = fault::pin_birth(static_cast<int>(pid), pidfd);
    }
    void reap() noexcept {
        if (pid <= 0 || pidfd < 0)
            return;
        siginfo_t info;
        waited = wait_owned(pidfd, info, WEXITED);
        if (waited == 0 && info.si_pid == pid) {
            own_reap = gone = true;
            code = info.si_code;
            status = info.si_status;
        } else if (waited == -ECHILD && exit_ready(pidfd)) {
            gone = wait_owned(pidfd, info, WEXITED | WNOHANG | WNOWAIT) == -ECHILD;
        }
    }
    void finish() noexcept {
        if (finished)
            return;
        if (pid > 0 && !gone) {
            fallback = true; // Recovery can never repair the planned primitive oracle.
            long result;
            do {
                result = raw(SYS_pidfd_send_signal, pidfd, SIGKILL, 0, 0);
            } while (result == -EINTR);
            reap();
            if (!gone)
                schema_foundation::finish(131);
        }
        if (pidfd >= 0) {
            const int actual = pidfd;
            close_result = raw(SYS_close, actual);
            pidfd = -1; // Actual Linux close first; never retry this number.
            fd_after = raw(SYS_fcntl, actual, F_GETFD);
            const native::Observation released{native::ObservationKind::release, close_after,
                                               actual, pid, close_result};
            fault::record(fault::Writer::parent, released);
        }
        finished = true;
    }
    ~Driver() { finish(); }
};
void check_driver(const Driver &value, bool own = true) {
    EXPECT_GT(value.pid, 0) << "clone errno=" << value.birth_error;
    EXPECT_TRUE(value.recorded);
    EXPECT_TRUE(value.pinned);
    EXPECT_TRUE(value.gone);
    EXPECT_FALSE(value.fallback);
    EXPECT_EQ(value.own_reap, own);
    EXPECT_EQ(value.close_result, 0);
    EXPECT_EQ(value.fd_after, -EBADF);
    if (own) {
        EXPECT_EQ(value.waited, 0);
        EXPECT_EQ(value.code, CLD_EXITED);
        EXPECT_EQ(value.status, 73); // Watchdog SIGALRM is NEVER a passing status.
    }
}

TEST(NativeFaultFoundation, FreshPlanOneShotAndExactSnapshotEncoding) {
    Fixture value;
    value.start(24, 8, ENOSYS);
    const auto selected = fault::selected_plan();
    const int second_arm = subathon_schema_fault_arm_v1(1, 0, 0);
    const bool first =
        fault::checkpoint({native::PhaseBase::activation_pipe, native::PhaseEdge::before}, -1);
    const bool second =
        fault::checkpoint({native::PhaseBase::activation_pipe, native::PhaseEdge::before}, -1);
    const native::Observation returned{native::ObservationKind::syscall_return, read_before, -1,
                                       raw(SYS_getpid), -EIO};
    const bool recorded = fault::record(fault::Writer::parent, returned);
    const int wrong_count = subathon_schema_fault_snapshot_v1(value.cells, 1023);
    const int null_output = subathon_schema_fault_snapshot_v1(nullptr, 1024);
    const int count = value.snapshot();
    const long pid = raw(SYS_getpid);
    value.finish();
    check_fixture(value);
    EXPECT_EQ(selected.case_id, 24U);
    EXPECT_EQ(selected.phase_id, 8U);
    EXPECT_EQ(selected.fault_errno, ENOSYS);
    EXPECT_EQ(second_arm, -1);
    EXPECT_TRUE(first);
    EXPECT_FALSE(second);
    EXPECT_TRUE(recorded);
    EXPECT_EQ(wrong_count, -1);
    EXPECT_EQ(null_output, -1);
    EXPECT_EQ(count, 9);
    EXPECT_EQ(value.cells[0], 0);
    EXPECT_EQ(value.cells[1], 1);
    EXPECT_EQ(value.cells[2], 8);
    EXPECT_EQ(value.cells[3], -1);
    EXPECT_EQ(value.cells[4], pid);
    EXPECT_EQ(value.cells[5], -1);
    EXPECT_EQ(value.cells[6], -1);
    EXPECT_GT(value.cells[7], 0);
    EXPECT_EQ(value.cells[8], 1);
    EXPECT_EQ(value.cells[9], 2);
    EXPECT_EQ(value.cells[10], 4);
    EXPECT_EQ(value.cells[11], -1);
    EXPECT_EQ(value.cells[12], pid);
    EXPECT_EQ(value.cells[13], -EIO);
    EXPECT_EQ(value.cells[14], EIO);
    EXPECT_GT(value.cells[15], 0);
    EXPECT_EQ(value.cells[16], std::int64_t{2} << 32);
    EXPECT_EQ(value.cells[17], 3);
    EXPECT_EQ(value.cells[18], 0);
    EXPECT_EQ(value.cells[19], -1);
    EXPECT_EQ(value.cells[20], pid);
    EXPECT_EQ(value.cells[21], reinterpret_cast<std::intptr_t>(value.map));
    EXPECT_EQ(value.cells[22], -1);
    EXPECT_GT(value.cells[23], 0);
    for (int fd = 0; fd < value.fd_count; ++fd) {
        const int row = 3 + fd;
        EXPECT_EQ(value.cells[8 * row], (std::int64_t{2} << 32) | (fd + 1));
        EXPECT_EQ(value.cells[8 * row + 1], 3);
        EXPECT_EQ(value.cells[8 * row + 2], 0);
        EXPECT_EQ(value.cells[8 * row + 3], value.descriptors[fd]);
        EXPECT_EQ(value.cells[8 * row + 4], pid);
        EXPECT_EQ(value.cells[8 * row + 5], value.descriptors[fd]);
        EXPECT_EQ(value.cells[8 * row + 6], -1);
        EXPECT_GT(value.cells[8 * row + 7], 0);
    }
    for (int index = 8 * count; count >= 0 && index < 1024; ++index) {
        EXPECT_EQ(value.cells[index], -1);
    }
}

TEST(NativeFaultFoundation, IdleArmActualStub31ThenTerminalCancel) {
    Fixture value;
    const long before = clock_now();
    value.start(25, 19, 0); // A real idle signal controller must not send without a phase/child.
    const unsigned char manifest[] = "manifest.json", interpreter[] = "/usr/bin/python3";
    const unsigned char worker[] = "/synthetic/worker.py", sdk[] = "/synthetic/sdk";
    const int result = subathon_schema_transport_v1(manifest, sizeof(manifest) - 1, interpreter,
                                                    sizeof(interpreter) - 1, worker,
                                                    sizeof(worker) - 1, sdk, sizeof(sdk) - 1);
    const int count = value.snapshot();
    value.finish();
    const long after = clock_now();
    const long elapsed = before >= 0 && after >= before ? after - before : -1;
    check_fixture(value);
    EXPECT_EQ(result, 31); // Temporary resource-free actual stub, not owner success.
    EXPECT_EQ(count_writer(value.cells, count, 0), 0);
    EXPECT_EQ(count_writer(value.cells, count, 1), 0);
    EXPECT_FALSE(event(value.cells, count, 2, 6, 0));
    EXPECT_GE(before, 0);
    EXPECT_GE(elapsed, 0);
    EXPECT_LT(elapsed, 3 * nanosecond);
    EXPECT_EQ(subathon_schema_fault_snapshot_v1(value.cells, 1024), -1);
    EXPECT_EQ(subathon_schema_fault_fallback_v1(), 0);
}

TEST(NativeFaultFoundation, Parent64RecordBoundaryPoisonsOnlyOn65th) {
    Fixture value;
    value.start(1);
    bool admitted = true;
    const native::Observation item{native::ObservationKind::syscall_return, read_before, -1,
                                   raw(SYS_getpid), 0};
    for (unsigned index = 0; index < 64; ++index)
        admitted &= fault::record(fault::Writer::parent, item);
    const int at_limit = value.snapshot();
    const bool excess = fault::record(fault::Writer::parent, item);
    const int after = value.snapshot();
    value.finish();
    check_fixture(value, -1);
    EXPECT_TRUE(admitted);
    EXPECT_EQ(at_limit, 71);
    EXPECT_FALSE(excess);
    EXPECT_EQ(after, -1);
}

TEST(NativeFaultFoundation, CallerCannotForgeControllerJournal) {
    Fixture value;
    value.start(1);
    const native::Observation item{native::ObservationKind::syscall_return, read_before, -1,
                                   raw(SYS_getpid), 0};
    const bool accepted = fault::record(fault::Writer::controller, item);
    const int after = value.snapshot();
    value.finish();
    check_fixture(value, -1);
    EXPECT_FALSE(accepted);
    EXPECT_EQ(after, -1);
}

TEST(NativeFaultFoundation, Case28HasOneActualNonCloexecPrebirthSentinel) {
    Fixture value;
    value.start(28);
    value.finish();
    check_fixture(value);
    int no_cloexec = 0;
    for (int index = 0; index < value.fd_count; ++index) {
        if (value.flags[index] == 0) {
            ++no_cloexec;
            EXPECT_GE(value.descriptors[index], 5);
        } else {
            EXPECT_EQ(value.flags[index], FD_CLOEXEC);
        }
    }
    EXPECT_EQ(no_cloexec, 1); // Only helper sentinel, not a remap/exec/Case28 owner proof.
}

struct BadPlan {
    unsigned which, phase;
    int error;
};
class NativeFaultInvalidPlan : public ::testing::TestWithParam<BadPlan> {};
TEST_P(NativeFaultInvalidPlan, RefusesFreshInvalidTupleWithoutPublishedState) {
    const auto input = GetParam();
    const int result = subathon_schema_fault_arm_v1(input.which, input.phase, input.error);
    const void *map = fault::mapping_address();
    std::int64_t cells[1024];
    const int snapshot = subathon_schema_fault_snapshot_v1(cells, 1024);
    const int retry = subathon_schema_fault_arm_v1(1, 0, 0);
    const int disposal = subathon_schema_fault_fallback_v1();
    EXPECT_EQ(result, -1);
    EXPECT_EQ(map, nullptr);
    EXPECT_EQ(snapshot, -1);
    EXPECT_EQ(retry, -1);
    EXPECT_EQ(disposal, 0);
}
INSTANTIATE_TEST_SUITE_P(ClosedTuples, NativeFaultInvalidPlan,
                         ::testing::Values(BadPlan{0, 0, 0}, BadPlan{1, 2, 0}, BadPlan{24, 6, EIO},
                                           BadPlan{25, 17, 0}, BadPlan{26, 16, 0},
                                           BadPlan{28, 0, EINTR}));

struct CapturePlan {
    unsigned bytes;
    unsigned eof;
    int expected;
};
class NativeFaultCapture : public ::testing::TestWithParam<CapturePlan> {};
TEST_P(NativeFaultCapture, ActualPipeTeeSuffixAndEofAreRequired) {
    const auto input = GetParam();
    int wire[2]{-1, -1};
    const long pipe = raw(SYS_pipe2, address(wire), O_CLOEXEC | O_NONBLOCK);
    Fixture value;
    value.start(28);
    unsigned char payload[258], prefix[16], output[248];
    for (unsigned index = 0; index < 258; ++index)
        payload[index] = static_cast<unsigned char>(index);
    long written = -1, read = -1, wire_closed[2]{-1, -1};
    bool pinned = false, copied = false;
    int report = -2;
    if (pipe == 0 && value.armed == 0) {
        pinned = fault::pin_report_read(wire[0]);
        written = raw(SYS_write, wire[1], address(payload), input.bytes);
        read = raw(SYS_read, wire[0], address(prefix), 16);
        if (read == 16)
            copied = fault::capture_outcome(prefix, 16);
        if (input.eof) {
            const int actual = wire[1];
            raw(SYS_close, wire[1]);
            wire[1] = -1;
            wire_closed[1] = raw(SYS_fcntl, actual, F_GETFD);
        }
        report = subathon_schema_fault_report_v1(output, 248);
    }
    for (unsigned index = 0; index < 2; ++index)
        if (wire[index] >= 0) {
            const int actual = wire[index];
            raw(SYS_close, actual);
            wire[index] = -1;
            wire_closed[index] = raw(SYS_fcntl, actual, F_GETFD);
        }
    value.finish();
    check_fixture(value, input.expected == 248 ? 0 : -1);
    EXPECT_EQ(pipe, 0);
    EXPECT_TRUE(pinned);
    EXPECT_EQ(written, input.bytes);
    EXPECT_EQ(read, 16);
    EXPECT_TRUE(copied);
    EXPECT_EQ(report, input.expected);
    EXPECT_EQ(wire_closed[0], -EBADF);
    EXPECT_EQ(wire_closed[1], -EBADF);
    if (report == 248) {
        for (unsigned index = 0; index < 248; ++index) {
            EXPECT_EQ(output[index], static_cast<unsigned char>(index));
        }
    }
}
INSTANTIATE_TEST_SUITE_P(ClosedCapture, NativeFaultCapture,
                         ::testing::Values(CapturePlan{248, 1, 248}, CapturePlan{247, 1, -1},
                                           CapturePlan{258, 1, -1}, CapturePlan{248, 0, -1}));

TEST(NativeFaultFoundation, CaptureTee257ThenOneRefusesWithoutOverflow) {
    Fixture value;
    value.start(28);
    unsigned char payload[257]{};
    const bool at_limit = fault::capture_outcome(payload, 257);
    const bool excess = fault::capture_outcome(payload, 1);
    const int snapshot = value.snapshot();
    value.finish();
    check_fixture(value, -1);
    EXPECT_TRUE(at_limit);
    EXPECT_FALSE(excess);
    EXPECT_EQ(snapshot, -1);
}

TEST(NativeFaultFoundation, TinyFdLimitRollbackRestoresCallerLimit) {
    struct Limit {
        std::uint64_t current, maximum;
    } original{}, limited{};
    static_assert(sizeof(Limit) == 16);
    const long got = raw(SYS_prlimit64, 0, RLIMIT_NOFILE, 0, address(&original));
    long set = -1, restored = -1, pipe_result = -1;
    int armed = -2, disposed = -2;
    bool no_map = false;
    if (got == 0 && original.current >= 3 && raw(SYS_fcntl, 0, F_GETFD) >= 0 &&
        raw(SYS_fcntl, 1, F_GETFD) >= 0 && raw(SYS_fcntl, 2, F_GETFD) >= 0) {
        limited = original;
        limited.current = 3;
        set = raw(SYS_prlimit64, 0, RLIMIT_NOFILE, address(&limited), 0);
        if (set == 0) {
            int pair[2]{-1, -1};
            pipe_result = raw(SYS_pipe2, address(pair), O_CLOEXEC);
            if (pipe_result == 0) {
                raw(SYS_close, pair[0]);
                raw(SYS_close, pair[1]);
            }
            armed = subathon_schema_fault_arm_v1(1, 0, 0);
            disposed = subathon_schema_fault_fallback_v1();
            no_map = fault::mapping_address() == nullptr;
            restored = raw(SYS_prlimit64, 0, RLIMIT_NOFILE, address(&original), 0);
            if (restored != 0)
                schema_foundation::finish(133); // Never return a changed caller limit.
            if (!no_map)
                schema_foundation::finish(130); // No unproved helper-disposal return.
        }
    }
    Limit after{};
    const long queried = raw(SYS_prlimit64, 0, RLIMIT_NOFILE, 0, address(&after));
    EXPECT_EQ(got, 0);
    EXPECT_EQ(set, 0);
    EXPECT_EQ(pipe_result, -EMFILE);
    EXPECT_EQ(armed, -1);
    EXPECT_EQ(disposed, -1);
    EXPECT_TRUE(no_map);
    EXPECT_EQ(restored, 0);
    EXPECT_EQ(queried, 0);
    EXPECT_EQ(after.current, original.current);
    EXPECT_EQ(after.maximum, original.maximum);
}

TEST(NativeFaultFoundation, SharedChildJournal32ThenOverflowIsVisible) {
    int coord[4]{-1, -1, -1, -1}, pair[2];
    const long ready_pipe = raw(SYS_pipe2, address(pair), O_CLOEXEC);
    if (ready_pipe == 0) {
        coord[0] = pair[0];
        coord[1] = pair[1];
    }
    const long release_pipe = raw(SYS_pipe2, address(pair), O_CLOEXEC);
    if (release_pipe == 0) {
        coord[2] = pair[0];
        coord[3] = pair[1];
    }
    int actual[4];
    long closed[4]{-1, -1, -1, -1};
    for (unsigned index = 0; index < 4; ++index)
        actual[index] = coord[index];
    Fixture value;
    value.start(1);
    Driver driver;
    if (value.armed == 0 && ready_pipe == 0 && release_pipe == 0)
        driver.start(2, coord);
    // Keep the unused parent release-read copy until after the write: even a
    // failed/dead child cannot turn driver cleanup into parent SIGPIPE.
    if (coord[1] >= 0) {
        raw(SYS_close, coord[1]);
        coord[1] = -1;
        closed[1] = raw(SYS_fcntl, actual[1], F_GETFD);
    }
    const bool published = driver.pinned && token(coord[0], 'R');
    const int count = value.snapshot();
    std::int64_t saved[1024];
    for (unsigned index = 0; index < 1024; ++index)
        saved[index] = value.cells[index];
    const char release = 'G';
    const bool released = published && raw(SYS_write, coord[3], address(&release), 1) == 1;
    if (released)
        driver.reap();
    driver.finish();
    const int overflow = value.snapshot();
    for (unsigned index = 0; index < 4; ++index)
        if (coord[index] >= 0) {
            raw(SYS_close, coord[index]);
            coord[index] = -1;
            closed[index] = raw(SYS_fcntl, actual[index], F_GETFD);
        }
    value.finish();
    check_fixture(value, -1);
    check_driver(driver);
    EXPECT_EQ(ready_pipe, 0);
    EXPECT_EQ(release_pipe, 0);
    for (long state : closed) {
        EXPECT_EQ(state, -EBADF);
    }
    EXPECT_TRUE(published);
    EXPECT_TRUE(released);
    EXPECT_EQ(count_writer(saved, count, 1), 32);
    EXPECT_EQ(overflow, -1);
    unsigned child_index = 0;
    for (int row = 0; row < count && row < 128; ++row) {
        if ((saved[8 * row] >> 32) == 1) {
            EXPECT_EQ(saved[8 * row], (std::int64_t{1} << 32) | child_index);
            EXPECT_EQ(saved[8 * row + 4], driver.pid);
            EXPECT_EQ(saved[8 * row + 1], 2);
            EXPECT_EQ(saved[8 * row + 6], EIO);
            ++child_index;
        }
    }
}

TEST(NativeFaultFoundation, RealReadyChildAdvancesOnlyTheOffsetClock) {
    Fixture value;
    value.start(23);
    Driver driver;
    const long real_before = clock_now(), epoch = fault::monotonic_ns();
    if (value.armed == 0)
        driver.start();
    // The required case23 pin_birth contract includes the genuine readiness +
    // controller action ACK. No snapshot polling, sleeps or private Shared reads.
    const int bound_count = value.snapshot();
    const bool observed =
        driver.pinned && event(value.cells, bound_count, 2, 5, 10000000001L, 0, driver.pid);
    const long adjusted = fault::monotonic_ns(), real_after = clock_now();
    const long elapsed =
        real_before >= 0 && real_after >= real_before ? real_after - real_before : -1;
    const long minimum_adjusted =
        epoch > 0 && epoch <= INT64_MAX - 10000000001L ? epoch + 10000000001L : -1;
    const bool released = driver.pinned && fault::release_child();
    if (released)
        driver.reap();
    driver.finish();
    const int count = value.snapshot();
    value.finish();
    check_fixture(value);
    check_driver(driver);
    EXPECT_TRUE(observed);
    EXPECT_TRUE(released);
    EXPECT_GT(epoch, 0);
    EXPECT_GT(minimum_adjusted, 0);
    EXPECT_GE(adjusted, minimum_adjusted);
    EXPECT_GE(real_before, 0);
    EXPECT_GE(real_after, real_before);
    EXPECT_GE(elapsed, 0);
    EXPECT_LT(elapsed, 3 * nanosecond);
    EXPECT_TRUE(event(value.cells, count, 2, 5, 10000000001L, 0, driver.pid));
}

volatile sig_atomic_t signal_seen = 0;
void count_signal(int) { signal_seen = 1; }
struct SignalScope {
    schema_foundation::KernelAction original{}, after{};
    std::uint64_t mask = 0, mask_after = 0;
    long saved = -1, saved_mask = -1, installed = -1, unblocked = -1, restored = -1,
         restored_mask = -1, queried_action = -1, queried_mask = -1;
    bool finished = false;
    void start() noexcept {
        saved = raw(SYS_rt_sigaction, SIGINT, 0, address(&original), 8);
        saved_mask = raw(SYS_rt_sigprocmask, SIG_SETMASK, 0, address(&mask), 8);
        if (saved != 0 || saved_mask != 0)
            return;
        struct sigaction action{};
        action.sa_handler = count_signal;
        ::sigemptyset(&action.sa_mask);
        installed = ::sigaction(SIGINT, &action, nullptr);
        std::uint64_t ready = mask & ~(std::uint64_t{1} << (SIGINT - 1));
        unblocked = raw(SYS_rt_sigprocmask, SIG_SETMASK, address(&ready), 0, 8);
    }
    void finish() noexcept {
        if (finished)
            return;
        if (saved == 0 && saved_mask == 0) {
            restored = raw(SYS_rt_sigaction, SIGINT, address(&original), 0, 8);
            restored_mask = raw(SYS_rt_sigprocmask, SIG_SETMASK, address(&mask), 0, 8);
            if (restored != 0 || restored_mask != 0)
                schema_foundation::finish(134);
            queried_action = raw(SYS_rt_sigaction, SIGINT, 0, address(&after), 8);
            queried_mask = raw(SYS_rt_sigprocmask, SIG_SETMASK, 0, address(&mask_after), 8);
        }
        finished = true;
    }
    ~SignalScope() { finish(); }
};
class NativeFaultSignal : public ::testing::TestWithParam<unsigned> {};
TEST_P(NativeFaultSignal, ActualMainThreadSigintPreservesCallerState) {
    const unsigned encoded = GetParam();
    int pair[2]{-1, -1};
    const long pipe = raw(SYS_pipe2, address(pair), O_CLOEXEC);
    SignalScope signals;
    signals.start();
    Fixture value;
    if (signals.installed == 0 && signals.unblocked == 0)
        value.start(25, encoded, 0);
    Driver driver;
    if (value.armed == 0)
        driver.start();
    long closed = -1, closed_state = -1;
    if (encoded == 19 && pipe == 0) {
        closed = raw(SYS_close, pair[1]);
        closed_state = raw(SYS_fcntl, pair[1], F_GETFD);
        pair[1] = -1;
    }
    const bool triggered =
        driver.pinned &&
        fault::checkpoint({native::PhaseBase::parent_write_close,
                           encoded == 18 ? native::PhaseEdge::before : native::PhaseEdge::after},
                          closed);
    if (encoded == 18 && pipe == 0) {
        closed = raw(SYS_close, pair[1]);
        closed_state = raw(SYS_fcntl, pair[1], F_GETFD);
        pair[1] = -1;
    }
    raw(SYS_getpid); // Kernel return delivers the already targeted, unblocked pending SIGINT.
    const int caught = signal_seen;
    const bool released = driver.pinned && fault::release_child();
    if (released)
        driver.reap();
    driver.finish();
    const int count = value.snapshot();
    for (int &fd : pair)
        if (fd >= 0) {
            raw(SYS_close, fd);
            fd = -1;
        }
    value.finish();
    signals.finish();
    check_fixture(value);
    check_driver(driver);
    EXPECT_EQ(pipe, 0);
    EXPECT_EQ(signals.saved, 0);
    EXPECT_EQ(signals.saved_mask, 0);
    EXPECT_EQ(signals.installed, 0);
    EXPECT_EQ(signals.unblocked, 0);
    EXPECT_TRUE(triggered);
    EXPECT_TRUE(released);
    EXPECT_EQ(caught, 1);
    EXPECT_EQ(closed, 0);
    EXPECT_EQ(closed_state, -EBADF);
    EXPECT_TRUE(event(value.cells, count, 2, 6, 0, encoded, raw(SYS_getpid)));
    EXPECT_EQ(signals.restored, 0);
    EXPECT_EQ(signals.restored_mask, 0);
    EXPECT_EQ(signals.queried_action, 0);
    EXPECT_EQ(signals.queried_mask, 0);
    EXPECT_EQ(signals.after.handler, signals.original.handler);
    EXPECT_EQ(signals.after.flags, signals.original.flags);
    EXPECT_EQ(signals.after.restorer, signals.original.restorer);
    EXPECT_EQ(signals.after.mask, signals.original.mask);
    EXPECT_EQ(signals.mask_after, signals.mask);
}
INSTANTIATE_TEST_SUITE_P(CloseEdges, NativeFaultSignal, ::testing::Values(18U, 19U));

TEST(NativeFaultFoundation, PostBirthResourceHookNeedsRealReadinessWithoutClassifying) {
    Fixture value;
    value.start(26, 17, 0);
    Driver driver;
    if (value.armed == 0)
        driver.start();
    const bool triggered = driver.pinned && fault::checkpoint(birth_after, driver.pid);
    const bool released = driver.pinned && fault::release_child();
    if (released)
        driver.reap();
    driver.finish();
    const int count = value.snapshot();
    value.finish();
    check_fixture(value);
    check_driver(driver);
    EXPECT_TRUE(triggered);
    EXPECT_TRUE(released);
    EXPECT_TRUE(event(value.cells, count, 2, 7, 1, 17, driver.pid));
    // No returned5 or fake final-commit record: only the actual helper trigger is proved.
}

TEST(NativeFaultFoundation, DeliberateControllerReapIsDistinctFromFallback) {
    Fixture value;
    value.start(27);
    Driver driver;
    if (value.armed == 0)
        driver.start(1);
    const bool triggered =
        driver.pinned &&
        fault::checkpoint({native::PhaseBase::wait_reap, native::PhaseEdge::before}, -1);
    if (triggered)
        driver.reap();
    const bool pinned_exit = driver.pidfd >= 0 && exit_ready(driver.pidfd);
    driver.finish();
    const int count = value.snapshot();
    value.finish();
    check_fixture(value);
    check_driver(driver, false);
    EXPECT_TRUE(triggered);
    EXPECT_TRUE(pinned_exit);
    EXPECT_EQ(driver.waited, -ECHILD);
    EXPECT_TRUE(event(value.cells, count, 2, 4, 73, 32, driver.pid));
}

TEST(NativeFaultFoundation, ControllerConsumedNormal125PoisonsBeforeActionAck) {
    Fixture value;
    value.start(27);
    Driver driver;
    if (value.armed == 0)
        driver.start(3);
    siginfo_t original{};
    const long before = driver.pinned ? wait_owned(driver.pidfd, original, WEXITED | WNOWAIT) : -1;
    // Observe the genuine normal125 WITHOUT consuming it before the explicit
    // controller is allowed to reap. No synthetic journal/final-result payload.
    const bool acknowledged =
        driver.pinned &&
        fault::checkpoint({native::PhaseBase::wait_reap, native::PhaseEdge::before}, -1);
    siginfo_t after{};
    const long fresh =
        driver.pinned ? wait_owned(driver.pidfd, after, WEXITED | WNOHANG | WNOWAIT) : -1;
    const bool pinned_exit = driver.pidfd >= 0 && exit_ready(driver.pidfd);
    if (fresh == -ECHILD && pinned_exit)
        driver.reap(); // Only revalidate ECHILD after the independent nonconsuming proof.
    const int poisoned = value.snapshot();
    driver.finish();
    value.finish();
    check_fixture(value, -1);
    check_driver(driver, false);
    EXPECT_EQ(before, 0);
    EXPECT_EQ(original.si_pid, driver.pid);
    EXPECT_EQ(original.si_code, CLD_EXITED);
    EXPECT_EQ(original.si_status, 125);
    EXPECT_FALSE(acknowledged);
    EXPECT_EQ(poisoned, -1);
    EXPECT_EQ(fresh, -ECHILD);
    EXPECT_TRUE(pinned_exit);
    EXPECT_EQ(driver.waited, -ECHILD);
}
} // namespace
