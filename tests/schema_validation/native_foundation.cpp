// SPDX-License-Identifier: GPL-3.0-or-later
// Nonvalidating real-kernel prerequisites, not the future transport algorithm.
#include "native_foundation_child.h"
#include "transport_v1.h"
#include <atomic>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <elf.h>
#include <gnu/libc-version.h>
#include <gtest/gtest.h>
#include <initializer_list>
#include <linux/futex.h>
#include <linux/sched.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#ifndef SUBATHON_NATIVE_FOUNDATION_CHILD_PATH
#error "Build must provide the actual fixed nonvalidating child artifact"
#endif

namespace {
using namespace schema_foundation;
constexpr int own_thread = 0x20000000;
constexpr int pidfd_selector = 3;
constexpr std::uint64_t usr2_bit = std::uint64_t{1} << (SIGUSR2 - 1);
static_assert(sizeof(int) == 4 && sizeof(unsigned int) == 4 && CHAR_BIT == 8);
static_assert(sizeof(siginfo_t) == 128);
static_assert(sizeof(std::atomic<int>) == 4 && std::atomic<int>::is_always_lock_free);

long wait_fd(int descriptor, siginfo_t &info, int flags) noexcept {
    long result;
    do {
        info.si_pid = 0;
        info.si_code = -1;
        info.si_status = -1;
        result = raw(SYS_waitid, pidfd_selector, descriptor, address(&info), flags | own_thread, 0);
    } while (result == -EINTR);
    return result;
}
long wait_pid(long pid, int &status, int flags) noexcept {
    long result;
    do {
        result = raw(SYS_wait4, pid, address(&status), flags, 0);
    } while (result == -EINTR);
    return result;
}
bool pinned_reaped(int descriptor) noexcept {
    // This thread is still the recorded birth thread. There is no reparenting,
    // no signal-profile mutation and no other birth in this standalone fixture.
    KernelAction action;
    if (raw(SYS_rt_sigaction, SIGCHLD, 0, address(&action), 8) != 0 || action.handler != 0 ||
        (action.flags & SA_NOCLDWAIT))
        return false;
    pollfd item{descriptor, POLLIN, 0};
    long events;
    do {
        events = raw(SYS_poll, address(&item), 1, 0);
    } while (events == -EINTR);
    if (events != 1 || !(item.revents & (POLLIN | POLLHUP)) ||
        (item.revents & (POLLERR | POLLNVAL)))
        return false;
    siginfo_t info;
    return wait_fd(descriptor, info, WEXITED | WNOHANG | WNOWAIT) == -ECHILD;
}
void cleanup_child(long pid, int descriptor) noexcept {
    // Disposal only: callers have already latched a failed observation. Never
    // close the pinned identity or return a cleanup claim on an unchecked wait.
    if (descriptor >= 0) {
        long signalled;
        do {
            signalled = raw(SYS_pidfd_send_signal, descriptor, SIGKILL, 0, 0);
        } while (signalled == -EINTR);
        siginfo_t info;
        const long result = wait_fd(descriptor, info, WEXITED);
        if (result == 0 && info.si_pid == pid)
            return;
        if (result == -ECHILD && pinned_reaped(descriptor))
            return;
    }
    // The saved successful native PID belongs to THIS still-live birth thread;
    // __WNOTHREAD prevents adopting another thread's reused-PID child. This is
    // a wait only, never numeric-PID signalling or failed-output-cell adoption.
    int status;
    const long result = wait_pid(pid, status, own_thread);
    if (result == pid || (result == -ECHILD && descriptor >= 0 && pinned_reaped(descriptor)))
        return;
    finish(119); // Fatal fixture error: no unproved child-gone return/assertion.
}
long readable(int descriptor) noexcept {
    pollfd item;
    item.fd = descriptor;
    item.events = POLLIN;
    item.revents = 0;
    const long result = raw(SYS_poll, address(&item), 1, 3000);
    return result == 1 ? item.revents : result == 0 ? -ETIMEDOUT : result;
}
long receive(int descriptor, Report &report) noexcept {
    const long events = readable(descriptor);
    if (events < 0)
        return events;
    if (!(events & (POLLIN | POLLHUP)))
        return -EIO;
    return raw(SYS_read, descriptor, address(&report), sizeof(report));
}
void decimal(int descriptor, char (&buffer)[12]) noexcept {
    char reversed[11];
    unsigned count = 0, value = static_cast<unsigned>(descriptor);
    do {
        reversed[count++] = static_cast<char>('0' + value % 10);
        value /= 10;
    } while (value);
    for (unsigned i = 0; i < count; ++i)
        buffer[i] = reversed[count - i - 1];
    buffer[count] = '\0';
}

void caught_handler(int) {} // Installed only in this standalone test process.
struct SignalFixture {
    struct sigaction usr1, pipe;
    sigset_t mask;
    bool have_usr1 = false, have_pipe = false, have_mask = false;
    long setup = 0, restoration = 0;
    SignalFixture() noexcept {
        struct sigaction current;
        if (::sigaction(SIGCHLD, nullptr, &current) != 0 || current.sa_handler != SIG_DFL ||
            (current.sa_flags & SA_NOCLDWAIT)) {
            setup = -1;
            return;
        }
        struct sigaction action{};
        ::sigemptyset(&action.sa_mask);
        action.sa_handler = caught_handler;
        if (::sigaction(SIGUSR1, &action, &usr1) != 0) {
            setup = -2;
            return;
        }
        have_usr1 = true;
        action.sa_handler = SIG_IGN;
        if (::sigaction(SIGPIPE, &action, &pipe) != 0) {
            setup = -3;
            return;
        }
        have_pipe = true;
        sigset_t blocked;
        ::sigemptyset(&blocked);
        ::sigaddset(&blocked, SIGUSR2);
        if (::pthread_sigmask(SIG_BLOCK, &blocked, &mask) != 0) {
            setup = -4;
            return;
        }
        have_mask = true;
    }
    void restore() noexcept {
        if (have_mask) {
            if (::pthread_sigmask(SIG_SETMASK, &mask, nullptr) != 0)
                restoration = -1;
            have_mask = false;
        }
        if (have_pipe) {
            if (::sigaction(SIGPIPE, &pipe, nullptr) != 0)
                restoration = -2;
            have_pipe = false;
        }
        if (have_usr1) {
            if (::sigaction(SIGUSR1, &usr1, nullptr) != 0)
                restoration = -3;
            have_usr1 = false;
        }
    }
    ~SignalFixture() { restore(); }
};

struct Controller {
    std::atomic<int> phase{0};
    int pid = -1, pidfd = -1;
    long failure = 0, foreign_wait = LONG_MIN, default_wait = LONG_MIN, default_pid = -1,
         reaped = LONG_MIN;
    int status = 0;
};
bool phase_wait(Controller &state, int wanted) noexcept {
    timespec until;
    if (raw(SYS_clock_gettime, CLOCK_MONOTONIC, address(&until)) != 0)
        return false;
    until.tv_sec += 3;
    for (int attempt = 0; attempt < 16; ++attempt) {
        const int seen = state.phase.load(std::memory_order_acquire);
        if (seen >= wanted)
            return true;
        const long result = raw(SYS_futex, address(&state.phase), FUTEX_WAIT_BITSET_PRIVATE, seen,
                                address(&until), 0, FUTEX_BITSET_MATCH_ANY);
        if (result != 0 && result != -EAGAIN && result != -EINTR)
            return false;
    }
    return false;
}
void advance(Controller &state, int phase) noexcept {
    state.phase.store(phase, std::memory_order_release);
    raw(SYS_futex, address(&state.phase), FUTEX_WAKE_PRIVATE, INT_MAX);
}
void foreign_reaper(Controller &state) noexcept {
    if (!phase_wait(state, 1)) {
        state.failure = 1;
        return;
    }
    siginfo_t info;
    info.si_pid = 0;
    state.default_wait = raw(SYS_waitid, pidfd_selector, state.pidfd, address(&info),
                             WEXITED | WNOHANG | WNOWAIT, 0);
    state.default_pid = info.si_pid;
    state.foreign_wait = wait_fd(state.pidfd, info, WEXITED | WNOHANG | WNOWAIT);
    advance(state, 2);
    if (!phase_wait(state, 3)) {
        state.failure = 2;
        return;
    }
    state.reaped = wait_pid(state.pid, state.status, 0);
    if (state.reaped != state.pid)
        state.failure = 3; // Birth thread retains the pidfd and finishes disposal.
    advance(state, 4);
}

struct Result {
    long setup = LONG_MIN, clone = LONG_MIN, clone_errno = 0, packet = LONG_MIN, second = LONG_MIN;
    long live_wait = LONG_MIN, live_pid = -1, kill = LONG_MIN, exit_events = LONG_MIN,
         exited = LONG_MIN, no_child = LONG_MIN;
    long stale_signal = LONG_MIN, stale_wait = LONG_MIN, parent_extra = LONG_MIN;
    long restore = LONG_MIN, parent_mask = -1, close_error = 0;
    int exit_pid = -1, exit_code = -1, exit_status = -1;
    int original[6]{-1, -1, -1, -1, -1, -1};
    long closed[6]{LONG_MIN, LONG_MIN, LONG_MIN, LONG_MIN, LONG_MIN, LONG_MIN};
    Report first{}, after_exec{};
    bool fallback = false;
};

Result probe(bool exec, Controller *controller = nullptr) noexcept {
    Result result;
    SignalFixture signals;
    result.setup = signals.setup;
    int descriptors[6]{-1, -1, -1, -1, -1, -1};
    int pidfd_output = -1;
    long pid = -1;
    bool reaped = false;
    const auto close_slot = [&](int slot) noexcept {
        if (descriptors[slot] >= 0) {
            const long status = raw(SYS_close, descriptors[slot]);
            if (status != 0)
                result.close_error = status;
            descriptors[slot] = -1; // AFTER actual Linux close, never before.
        }
    };
    do {
        if (signals.setup != 0)
            break;
        int pair[2];
        if (raw(SYS_pipe2, address(pair), O_CLOEXEC) != 0) {
            result.setup = -5;
            break;
        }
        descriptors[0] = pair[0];
        descriptors[1] = pair[1];
        if (raw(SYS_pipe2, address(pair), O_CLOEXEC) != 0) {
            result.setup = -6;
            break;
        }
        descriptors[2] = pair[0];
        descriptors[3] = pair[1];
        descriptors[4] = static_cast<int>(raw(SYS_fcntl, descriptors[0], F_DUPFD_CLOEXEC, 64));
        if (descriptors[4] < 0) {
            result.setup = -7;
            break;
        }
        char channel[12], hold_read[12], extra[12];
        decimal(descriptors[1], channel);
        decimal(descriptors[2], hold_read);
        decimal(descriptors[4], extra);
        char *arguments[]{const_cast<char *>(SUBATHON_NATIVE_FOUNDATION_CHILD_PATH),
                          const_cast<char *>("--facility-observation"),
                          channel,
                          hold_read,
                          extra,
                          nullptr};
        char *environment[]{nullptr};
        const ChildPlan plan{descriptors[1],
                             descriptors[2],
                             descriptors[0],
                             descriptors[3],
                             descriptors[4],
                             exec,
                             SUBATHON_NATIVE_FOUNDATION_CHILD_PATH,
                             arguments,
                             environment};
        clone_args arguments_native{};
        arguments_native.flags = CLONE_PIDFD | CLONE_CLEAR_SIGHAND;
        arguments_native.exit_signal = SIGCHLD;
        arguments_native.pidfd = reinterpret_cast<std::uint64_t>(&pidfd_output);
        // No CLONE_VM and stack/stack_size BOTH zero: the libc return address and
        // this complete C++ stack are copied. No new-stack C-function return.
        pid = ::syscall(SYS_clone3, &arguments_native, sizeof(arguments_native));
        if (pid == 0)
            child(plan); // Raw-only noreturn before any parent operation.
        result.clone = pid;
        if (pid < 0) {
            result.clone_errno = errno;
            break;
        }
        descriptors[5] = pidfd_output; // Atomic successful commit, native ints only.
        if (pidfd_output < 0) {
            result.setup = -8;
            break;
        }
        for (int i = 0; i < 6; ++i)
            result.original[i] = descriptors[i];
        close_slot(1);
        close_slot(2);
        result.packet = receive(descriptors[0], result.first);
        if (result.packet != static_cast<long>(sizeof(Report)))
            break;
        if (exec) {
            result.second = receive(descriptors[0], result.after_exec);
            if (result.second != static_cast<long>(sizeof(Report)))
                break;
        }
        result.parent_extra = raw(SYS_fcntl, descriptors[4], F_GETFD);
        std::uint64_t mask;
        if (raw(SYS_rt_sigprocmask, SIG_SETMASK, 0, address(&mask), 8) == 0)
            result.parent_mask = static_cast<long>(mask);
        siginfo_t info;
        result.live_wait = wait_fd(descriptors[5], info, WEXITED | WNOHANG | WNOWAIT);
        result.live_pid = info.si_pid;
        if (result.live_wait != 0 || info.si_pid != 0)
            break;
        if (controller) {
            controller->pid = static_cast<int>(pid);
            controller->pidfd = descriptors[5];
            advance(*controller, 1);
            if (!phase_wait(*controller, 2)) {
                result.setup = -9;
                break;
            }
        }
        result.kill = raw(SYS_pidfd_send_signal, descriptors[5], SIGKILL, 0, 0);
        if (result.kill != 0)
            break;
        result.exit_events = readable(descriptors[5]);
        if (result.exit_events < 0 || !(result.exit_events & (POLLIN | POLLHUP)) ||
            (result.exit_events & (POLLERR | POLLNVAL)))
            break;
        if (controller) {
            advance(*controller, 3);
            if (!phase_wait(*controller, 4)) {
                result.setup = -10;
                break;
            }
            reaped = controller->reaped == pid;
            result.stale_wait = wait_fd(descriptors[5], info, WEXITED | WNOHANG | WNOWAIT);
        } else {
            result.exited = wait_fd(descriptors[5], info, WEXITED);
            result.exit_pid = info.si_pid;
            result.exit_code = info.si_code;
            result.exit_status = info.si_status;
            reaped = result.exited == 0 && info.si_pid == pid;
            result.stale_wait = wait_fd(descriptors[5], info, WEXITED | WNOHANG | WNOWAIT);
        }
        result.stale_signal = raw(SYS_pidfd_send_signal, descriptors[5], SIGKILL, 0, 0);
        int status;
        result.no_child = raw(SYS_wait4, -1, address(&status), WNOHANG | own_thread, 0);
    } while (false);
    if (pid > 0 && !reaped) {
        result.fallback = true; // Cannot rescue a failed observation as green.
        // Without a valid successful output, wait for the kill-only child timer;
        // never guess a descriptor or signal a numeric PID.
        cleanup_child(pid, descriptors[5]);
    }
    for (int i = 0; i < 6; ++i)
        close_slot(i);
    for (int i = 0; i < 6; ++i)
        if (result.original[i] >= 0)
            result.closed[i] = raw(SYS_fcntl, result.original[i], F_GETFD);
    signals.restore();
    result.restore = signals.restoration;
    return result; // Every FD/child is gone before GTest formats/asserts.
}

void check_probe(const Result &result) {
    EXPECT_EQ(result.setup, 0);
    EXPECT_GT(result.clone, 0) << "clone errno=" << result.clone_errno;
    EXPECT_FALSE(result.fallback);
    EXPECT_EQ(result.packet, 128);
    EXPECT_EQ(result.first.magic, report_magic);
    EXPECT_EQ(result.first.pid, result.clone);
    EXPECT_EQ(result.first.stage, 1);
    EXPECT_EQ(result.first.usr1_result, 0);
    EXPECT_EQ(result.first.usr1_handler, 0); // Actual CLONE_CLEAR_SIGHAND effect.
    EXPECT_EQ(result.first.pipe_result, 0);
    EXPECT_EQ(result.first.pipe_handler, 1); // Ignored dispositions initially survive.
    EXPECT_EQ(result.first.mask_result, 0);
    EXPECT_NE(static_cast<std::uint64_t>(result.first.mask_before) & usr2_bit, 0U);
    EXPECT_EQ(result.first.reset_result, 0);
    EXPECT_EQ(result.first.unmask_result, 0);
    EXPECT_EQ(result.first.mask_after, 0);
    EXPECT_EQ(result.first.extra_before, FD_CLOEXEC);
    EXPECT_EQ(result.first.close_result, 0);
    EXPECT_EQ(result.first.extra_after, -EBADF);
    EXPECT_EQ(result.first.channel_flags, FD_CLOEXEC);
    EXPECT_EQ(result.parent_extra, FD_CLOEXEC); // No shared FD table.
    EXPECT_NE(static_cast<std::uint64_t>(result.parent_mask) & usr2_bit, 0U);
    EXPECT_EQ(result.live_wait, 0);
    EXPECT_EQ(result.live_pid, 0); // Live is not already-reaped.
    EXPECT_EQ(result.kill, 0);
    EXPECT_NE(result.exit_events & (POLLIN | POLLHUP), 0);
    EXPECT_EQ(result.exit_events & (POLLERR | POLLNVAL), 0);
    EXPECT_EQ(result.stale_wait, -ECHILD);
    EXPECT_EQ(result.stale_signal, -ESRCH);
    EXPECT_EQ(result.no_child, -ECHILD);
    EXPECT_EQ(result.close_error, 0);
    EXPECT_EQ(result.restore, 0);
    for (int i = 0; i < 6; ++i)
        EXPECT_EQ(result.closed[i], -EBADF);
}

void diagnostics() {
    utsname identity;
    const long status = raw(SYS_uname, address(&identity));
    const long seccomp = raw(SYS_prctl, PR_GET_SECCOMP);
    ::testing::Test::RecordProperty("native_uname_result", static_cast<int>(status));
    if (status == 0) {
        ::testing::Test::RecordProperty("native_kernel", identity.release);
        ::testing::Test::RecordProperty("native_machine", identity.machine);
    }
    ::testing::Test::RecordProperty("native_seccomp_mode", static_cast<int>(seccomp));
    ::testing::Test::RecordProperty("native_libc", ::gnu_get_libc_version());
    std::fprintf(stderr,
                 "native foundation host: uname=%ld kernel=%s machine=%s seccomp=%ld libc=%s\n",
                 status, status == 0 ? identity.release : "unavailable",
                 status == 0 ? identity.machine : "unavailable", seccomp, ::gnu_get_libc_version());
}

TEST(SchemaNativeFoundation, SyscallBindingToActualLibc) {
    // No generic ::syscall invocation, new FD, child, thread or signal mutation.
    // The loader-supplied defining object is inspected only at its fixed header.
    for (const char *key :
         {"LD_PRELOAD", "LD_LIBRARY_PATH", "LD_AUDIT", "LD_DYNAMIC_WEAK", "LD_BIND_NOT", "LD_DEBUG",
          "LD_DEBUG_OUTPUT", "LD_ORIGIN_PATH", "GLIBC_TUNABLES"}) {
        ASSERT_EQ(std::getenv(key), nullptr) << key;
    }
    const void *bound = reinterpret_cast<const void *>(&::syscall);
    Dl_info defining{};
    ASSERT_NE(::dladdr(bound, &defining), 0);
    ASSERT_NE(defining.dli_fname, nullptr);
    ASSERT_NE(defining.dli_fbase, nullptr);
    ASSERT_NE(defining.dli_sname, nullptr);
    ASSERT_STREQ(defining.dli_sname, "syscall");
    ASSERT_EQ(defining.dli_saddr, bound);
    char path[4097];
    const std::size_t bytes = ::strnlen(defining.dli_fname, sizeof(path));
    ASSERT_GT(bytes, 0U);
    ASSERT_LT(bytes, sizeof(path));
    std::memcpy(path, defining.dli_fname, bytes + 1);
    bool standard = false;
    for (const char *admitted :
         {"/lib/x86_64-linux-gnu/libc.so.6", "/usr/lib/x86_64-linux-gnu/libc.so.6",
          "/lib/libc.so.6", "/usr/lib/libc.so.6", "/lib64/libc.so.6", "/usr/lib64/libc.so.6"}) {
        if (std::strcmp(path, admitted) == 0)
            standard = true;
    }
    ASSERT_TRUE(standard); // Exact defining path only; no search/fallback library.
    static_assert(sizeof(Elf64_Ehdr) == 64);
    Elf64_Ehdr header;
    std::memcpy(&header, defining.dli_fbase, 64); // Already loaded ELF header.
    ASSERT_EQ(header.e_ident[EI_MAG0], 0x7f);
    ASSERT_EQ(header.e_ident[EI_MAG1], 'E');
    ASSERT_EQ(header.e_ident[EI_MAG2], 'L');
    ASSERT_EQ(header.e_ident[EI_MAG3], 'F');
    ASSERT_EQ(header.e_ident[EI_CLASS], ELFCLASS64);
    ASSERT_EQ(header.e_ident[EI_DATA], ELFDATA2LSB);
    ASSERT_EQ(header.e_ident[EI_VERSION], EV_CURRENT);
    ASSERT_TRUE(header.e_ident[EI_OSABI] == ELFOSABI_SYSV ||
                header.e_ident[EI_OSABI] == ELFOSABI_GNU);
    ASSERT_EQ(header.e_type, ET_DYN);
    ASSERT_EQ(header.e_machine, EM_X86_64);
    ASSERT_EQ(header.e_version, EV_CURRENT);
    ASSERT_EQ(header.e_ehsize, 64);
    const auto base = reinterpret_cast<std::uintptr_t>(defining.dli_fbase);
    const auto function = reinterpret_cast<std::uintptr_t>(bound);
    ASSERT_GE(function, base);
    ASSERT_LT(function - base, 64U * 1024U * 1024U);

    void *handle = ::dlopen(path, RTLD_NOLOAD | RTLD_NOW | RTLD_LOCAL);
    void *symbol = nullptr, *versioned = nullptr;
    bool symbol_ok = false, version_ok = false, same_object = false;
    int closed = -1;
    if (handle) {
        ::dlerror();
        symbol = ::dlsym(handle, "syscall");
        symbol_ok = ::dlerror() == nullptr;
        ::dlerror();
        versioned = ::dlvsym(handle, "syscall", "GLIBC_2.2.5");
        version_ok = ::dlerror() == nullptr;
        Dl_info looked_up{};
        same_object = symbol && ::dladdr(symbol, &looked_up) != 0 &&
                      looked_up.dli_fbase == defining.dli_fbase && looked_up.dli_fname &&
                      std::strcmp(looked_up.dli_fname, path) == 0 && looked_up.dli_saddr == symbol;
        closed = ::dlclose(handle); // No assertion/unwind while this ref is owned.
    }
    EXPECT_NE(handle, nullptr);
    EXPECT_TRUE(symbol_ok);
    EXPECT_TRUE(version_ok);
    EXPECT_TRUE(same_object);
    EXPECT_EQ(symbol, bound);
    EXPECT_EQ(versioned, bound);
    EXPECT_EQ(closed, 0);
    ::testing::Test::RecordProperty("native_bound_libc_path", path);
    char addresses[96];
    const int address_bytes =
        std::snprintf(addresses, sizeof(addresses), "syscall=%p base=%p offset=%#lx", bound,
                      defining.dli_fbase, static_cast<unsigned long>(function - base));
    ASSERT_GT(address_bytes, 0);
    ASSERT_LT(static_cast<unsigned>(address_bytes), sizeof(addresses));
    ::testing::Test::RecordProperty("native_bound_libc_addresses", addresses);
    ::testing::Test::RecordProperty("native_bound_syscall_version", "GLIBC_2.2.5");
    std::fprintf(stderr, "native binding: libc=%s %s version=GLIBC_2.2.5\n", path, addresses);
    // This address comparison alone does NOT certify a different PLT call edge.
    // Root must pair these values with actual relocation/version/code/hash audit.
    diagnostics();
}

TEST(SchemaNativeFoundation, InertBaselineLiteral31) {
    diagnostics(); // No native resource interval is active.
    // TEMPORARY inert-stage control. Replacement needs reviewed stage migration.
    const unsigned char manifest[] = "manifest.json", interpreter[] = "/usr/bin/python3";
    const unsigned char worker[] = "/synthetic/worker.py", sdk[] = "/synthetic/sdk";
    EXPECT_EQ(subathon_schema_transport_v1(manifest, sizeof(manifest) - 1, interpreter,
                                           sizeof(interpreter) - 1, worker, sizeof(worker) - 1, sdk,
                                           sizeof(sdk) - 1),
              31);
}

TEST(SchemaNativeFoundation, SigchldProfileQueryWithoutBirth) {
    KernelAction original, ignored, queried;
    const long initial = raw(SYS_rt_sigaction, SIGCHLD, 0, address(&original), 8);
    long set_ignored = LONG_MIN, read_ignored = LONG_MIN, set_no_wait = LONG_MIN,
         read_no_wait = LONG_MIN, restore = LONG_MIN;
    std::uint64_t observed_ignored = ~std::uint64_t{0}, observed_flags = 0;
    if (initial == 0) {
        ignored.handler = 1;
        ignored.flags = 0;
        ignored.restorer = 0;
        ignored.mask = 0;
        set_ignored = raw(SYS_rt_sigaction, SIGCHLD, address(&ignored), 0, 8);
        read_ignored = raw(SYS_rt_sigaction, SIGCHLD, 0, address(&queried), 8);
        if (read_ignored == 0)
            observed_ignored = queried.handler;
        ignored.handler = 0;
        ignored.flags = SA_NOCLDWAIT;
        set_no_wait = raw(SYS_rt_sigaction, SIGCHLD, address(&ignored), 0, 8);
        read_no_wait = raw(SYS_rt_sigaction, SIGCHLD, 0, address(&queried), 8);
        if (read_no_wait == 0)
            observed_flags = queried.flags;
        restore = raw(SYS_rt_sigaction, SIGCHLD, address(&original), 0, 8);
    }
    EXPECT_EQ(initial, 0);
    if (initial == 0) {
        EXPECT_EQ(original.handler, 0U);
        EXPECT_EQ(original.flags & SA_NOCLDWAIT, 0U);
    }
    EXPECT_EQ(set_ignored, 0);
    EXPECT_EQ(read_ignored, 0);
    EXPECT_EQ(observed_ignored, 1U);
    EXPECT_EQ(set_no_wait, 0);
    EXPECT_EQ(read_no_wait, 0);
    EXPECT_NE(observed_flags & SA_NOCLDWAIT, 0U);
    EXPECT_EQ(restore, 0); // Detection controls only; no child or owner was invoked.
    diagnostics();
}

TEST(SchemaNativeFoundation, AtomicPidfdSignalsAndCloseRange) {
    const Result result = probe(false);
    check_probe(result);
    EXPECT_EQ(result.exited, 0);
    EXPECT_EQ(result.exit_pid, result.clone);
    EXPECT_EQ(result.exit_code, CLD_KILLED);
    EXPECT_EQ(result.exit_status, SIGKILL);
    diagnostics();
}

TEST(SchemaNativeFoundation, ActualExecveNonvalidatingProbe) {
    const Result result = probe(true);
    check_probe(result);
    EXPECT_EQ(result.second, 128);
    EXPECT_EQ(result.after_exec.magic, report_magic);
    EXPECT_EQ(result.after_exec.stage, 2);
    EXPECT_EQ(result.after_exec.pid, result.clone);
    EXPECT_EQ(result.after_exec.usr1_result, 0);
    EXPECT_EQ(result.after_exec.usr1_handler, 0);
    EXPECT_EQ(result.after_exec.pipe_result, 0);
    EXPECT_EQ(result.after_exec.pipe_handler, 0);
    EXPECT_EQ(result.after_exec.mask_result, 0);
    EXPECT_EQ(result.after_exec.mask_before, 0);
    EXPECT_EQ(result.after_exec.reset_result, 0);
    EXPECT_EQ(result.after_exec.unmask_result, 0);
    EXPECT_EQ(result.after_exec.mask_after, 0);
    EXPECT_EQ(result.after_exec.channel_flags, 0);
    EXPECT_EQ(result.after_exec.extra_before, -EBADF);
    EXPECT_EQ(result.after_exec.close_result, 0);
    EXPECT_EQ(result.after_exec.extra_after, -EBADF);
    EXPECT_EQ(result.exited, 0);
    EXPECT_EQ(result.exit_pid, result.clone);
    EXPECT_EQ(result.exit_code, CLD_KILLED);
    EXPECT_EQ(result.exit_status, SIGKILL);
    diagnostics();
}

TEST(SchemaNativeFoundation, ForeignThreadAndCompetingReapKeepPinnedIdentity) {
    Controller control;
    // Thread/framework allocations occur BEFORE any owned FD/child acquisition.
    std::thread controller([&] { foreign_reaper(control); });
    const Result result = probe(false, &control);
    controller.join();
    check_probe(result);
    EXPECT_EQ(control.failure, 0);
    EXPECT_EQ(control.default_wait, 0);
    EXPECT_EQ(control.default_pid, 0);
    EXPECT_EQ(control.foreign_wait, -ECHILD);
    EXPECT_EQ(control.reaped, result.clone);
    EXPECT_TRUE(WIFSIGNALED(control.status));
    EXPECT_EQ(WTERMSIG(control.status), SIGKILL);
    diagnostics();
}

// Actual controlled rollback below also observes all native descriptor identities.
struct Inventory {
    int count = 0;
    int descriptors[128];
    long error = 0;
};
void inventory(int directory, Inventory &result) noexcept {
    result.count = 0;
    if (raw(SYS_lseek, directory, 0, SEEK_SET) != 0) {
        result.error = -1;
        return;
    }
    alignas(8) unsigned char bytes[4096];
    for (int chunk = 0; chunk < 4; ++chunk) {
        const long length = raw(SYS_getdents64, directory, address(bytes), sizeof(bytes));
        if (length == 0)
            return;
        if (length < 0) {
            result.error = length;
            return;
        }
        long offset = 0;
        while (offset < length) {
            if (length - offset < 20) {
                result.error = -2;
                return;
            }
            const unsigned size =
                bytes[offset + 16] | (static_cast<unsigned>(bytes[offset + 17]) << 8);
            if (size < 20 || static_cast<long>(size) > length - offset) {
                result.error = -3;
                return;
            }
            unsigned value = 0, i = 19;
            bool numeric = true, ended = false;
            for (; i < size; ++i) {
                const unsigned byte = bytes[offset + i];
                if (byte == 0) {
                    ended = true;
                    break;
                }
                if (byte < '0' || byte > '9' || value > (INT_MAX - (byte - '0')) / 10U) {
                    numeric = false;
                    continue;
                }
                if (numeric)
                    value = value * 10 + byte - '0';
            }
            if (!ended) {
                result.error = -4;
                return;
            }
            const bool dot = size >= 21 && bytes[offset + 19] == '.' &&
                             (bytes[offset + 20] == 0 ||
                              (size >= 22 && bytes[offset + 20] == '.' && bytes[offset + 21] == 0));
            if ((!numeric && !dot) || (numeric && (i == 19 || i > 29))) {
                result.error = -7;
                return;
            }
            if (numeric) {
                if (result.count == 128) {
                    result.error = -5;
                    return;
                }
                int index = result.count++;
                while (index && result.descriptors[index - 1] > static_cast<int>(value)) {
                    result.descriptors[index] = result.descriptors[index - 1];
                    --index;
                }
                result.descriptors[index] = static_cast<int>(value);
            }
            offset += size;
        }
    }
    result.error = -6; // No observed EOF is never a fitting/truncated inventory.
}

TEST(SchemaNativeFoundation, ControlledCloneErrorsRollbackWithoutGuessingOutput) {
    Inventory before, after;
    long bad_size = LONG_MIN, bad_size_errno = 0, bad_pointer = LONG_MIN, bad_pointer_errno = 0;
    long no_child = LONG_MIN, close_result = LONG_MIN, unmap_result = LONG_MIN;
    int pidfd_cell = -1;
    const int directory = static_cast<int>(
        raw(SYS_openat, AT_FDCWD, address("/proc/self/fd"), O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0));
    const long guard = raw(SYS_mmap, 0, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (directory >= 0 && guard >= 0) {
        inventory(directory, before);
        clone_args arguments{};
        arguments.flags = CLONE_PIDFD | CLONE_CLEAR_SIGHAND;
        arguments.exit_signal = SIGCHLD;
        arguments.pidfd = reinterpret_cast<std::uint64_t>(&pidfd_cell);
        bad_size = ::syscall(SYS_clone3, &arguments, CLONE_ARGS_SIZE_VER0 - 1);
        if (bad_size == 0)
            finish(117);
        bad_size_errno = bad_size < 0 ? errno : 0;
        if (bad_size > 0) {
            // ONLY successful clone return commits this writable output cell.
            const int committed_pidfd = pidfd_cell;
            cleanup_child(bad_size, committed_pidfd);
            if (committed_pidfd < 0)
                finish(121); // Reaped child, but no exposed FD identity to own.
            if (committed_pidfd >= 0 && raw(SYS_close, committed_pidfd) != 0)
                finish(120); // Actual close, never retry a released number.
        }
        arguments.pidfd =
            static_cast<std::uint64_t>(guard); // Actual PROT_NONE, not a guessed unmapped address.
        bad_pointer = ::syscall(SYS_clone3, &arguments, sizeof(arguments));
        if (bad_pointer == 0)
            finish(118);
        bad_pointer_errno = bad_pointer < 0 ? errno : 0;
        if (bad_pointer > 0) {
            cleanup_child(bad_pointer, -1); // Child immediately raw-exits118.
            // A successful PROT_NONE output cannot expose its actual committed
            // FD. Do not inspect inaccessible memory or guess an FD to close.
            finish(121); // Fatal kernel/fixture defect, never normal assertions.
        }
        // Never adopt/close/signal a cell after either failed clone3.
        inventory(directory, after);
        int status;
        no_child = raw(SYS_wait4, -1, address(&status), WNOHANG | own_thread, 0);
    }
    if (guard >= 0)
        unmap_result = raw(SYS_munmap, guard, 4096);
    if (directory >= 0)
        close_result = raw(SYS_close, directory);
    EXPECT_GE(directory, 0);
    EXPECT_GE(guard, 0);
    EXPECT_EQ(bad_size, -1);
    EXPECT_EQ(bad_size_errno, EINVAL);
    EXPECT_EQ(bad_pointer, -1);
    EXPECT_EQ(bad_pointer_errno, EFAULT);
    EXPECT_EQ(before.error, 0);
    EXPECT_EQ(after.error, 0);
    EXPECT_EQ(before.count, after.count);
    if (before.count == after.count) {
        for (int i = 0; i < before.count; ++i) {
            EXPECT_EQ(before.descriptors[i], after.descriptors[i]);
        }
    }
    EXPECT_EQ(no_child, -ECHILD);
    EXPECT_EQ(close_result, 0);
    EXPECT_EQ(unmap_result, 0);
    // pidfd_cell is diagnostic only; no sentinel/unchanged-cell rollback oracle.
    ::testing::Test::RecordProperty("clone_failure_output_cell", pidfd_cell);
    diagnostics();
}
} // namespace
