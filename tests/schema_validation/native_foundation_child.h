// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Test-only Linux x86-64 syscall probe. No transport/validator implementation.
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <sys/syscall.h>

#if !defined(__linux__) || !defined(__x86_64__)
#error "Native schema foundations require Linux x86-64"
#endif

namespace schema_foundation {
inline long raw(long number, long a = 0, long b = 0, long c = 0, long d = 0, long e = 0,
                long f = 0) noexcept {
    long result;
    __asm__ volatile("mov %[four], %%r10; mov %[five], %%r8; mov %[six], %%r9; syscall"
                     : "=a"(result)
                     : "a"(number), "D"(a), "S"(b),
                       "d"(c), [four] "r"(d), [five] "r"(e), [six] "r"(f)
                     : "r10", "r8", "r9", "rcx", "r11", "cc", "memory");
    return result; // Raw negative errno, no libc/TLS error path.
}

template <class T> inline long address(T *value) noexcept { return reinterpret_cast<long>(value); }

[[noreturn]] inline void finish(int code) noexcept {
    raw(SYS_exit_group, code);
    __builtin_unreachable();
}

struct KernelAction {
    std::uint64_t handler;
    std::uint64_t flags;
    std::uint64_t restorer;
    std::uint64_t mask;
};
static_assert(sizeof(long) == 8 && sizeof(void *) == 8);
static_assert(sizeof(KernelAction) == 32 && offsetof(KernelAction, mask) == 24);

struct Report {
    std::int64_t magic, stage, pid;
    std::int64_t usr1_result, usr1_handler, pipe_result, pipe_handler;
    std::int64_t mask_result, mask_before, reset_result, unmask_result;
    std::int64_t extra_before, close_result, extra_after, channel_flags, mask_after;
};
static_assert(sizeof(Report) == 128);
inline constexpr std::int64_t report_magic = 0x4e465031; // NFP1, not JSON/protocol.

inline void observe(Report &value, int stage, int channel, int extra) noexcept {
    KernelAction action;
    std::uint64_t mask;
    value.magic = report_magic;
    value.stage = stage;
    value.pid = raw(SYS_getpid);
    value.usr1_result = raw(SYS_rt_sigaction, SIGUSR1, 0, address(&action), 8);
    value.usr1_handler = value.usr1_result == 0 ? static_cast<long>(action.handler) : -1;
    value.pipe_result = raw(SYS_rt_sigaction, SIGPIPE, 0, address(&action), 8);
    value.pipe_handler = value.pipe_result == 0 ? static_cast<long>(action.handler) : -1;
    value.mask_result = raw(SYS_rt_sigprocmask, SIG_SETMASK, 0, address(&mask), 8);
    value.mask_before = value.mask_result == 0 ? static_cast<long>(mask) : -1;
    KernelAction defaults;
    defaults.handler = 0;
    defaults.flags = 0;
    defaults.restorer = 0;
    defaults.mask = 0;
    value.reset_result = 0;
    for (int signal = 1; signal <= 64; ++signal) {
        if (signal == SIGKILL || signal == SIGSTOP)
            continue;
        const long status = raw(SYS_rt_sigaction, signal, address(&defaults), 0, 8);
        if (status != 0 && value.reset_result == 0)
            value.reset_result = status;
    }
    mask = 0;
    value.unmask_result = raw(SYS_rt_sigprocmask, SIG_SETMASK, address(&mask), 0, 8);
    value.extra_before = raw(SYS_fcntl, extra, F_GETFD);
    value.close_result = raw(SYS_close_range, extra, extra, 0);
    value.extra_after = raw(SYS_fcntl, extra, F_GETFD);
    value.channel_flags = raw(SYS_fcntl, channel, F_GETFD);
    const long after = raw(SYS_rt_sigprocmask, SIG_SETMASK, 0, address(&mask), 8);
    value.mask_after = after == 0 ? static_cast<long>(mask) : -1;
}

inline void publish(const Report &report, int channel) noexcept {
    // One <=PIPE_BUF blocking write, never a successful validation result.
    if (raw(SYS_write, channel, address(&report), sizeof(report)) !=
        static_cast<long>(sizeof(report)))
        finish(110);
}

[[noreturn]] inline void hold(int descriptor) noexcept {
    char byte;
    raw(SYS_read, descriptor, address(&byte), 1);
    finish(111); // Unexpected EOF/error is not the planned parent kill.
}

struct ChildPlan {
    int channel, hold_read, close_read, close_write, extra;
    bool exec;
    const char *executable;
    char *const *argv;
    char *const *environment;
};

[[noreturn]] inline void child(const ChildPlan &plan) noexcept {
    // clone3 fork-like return enters here immediately. Never return through
    // parent/GTest/destructors. The independent kill-only watchdog cannot reap.
    if (raw(SYS_alarm, 5) < 0)
        finish(112);
    raw(SYS_close, plan.close_read);
    raw(SYS_close, plan.close_write);
    Report report;
    observe(report, 1, plan.channel, plan.extra);
    publish(report, plan.channel);
    if (!plan.exec)
        hold(plan.hold_read);
    if (raw(SYS_fcntl, plan.channel, F_SETFD, 0) != 0 ||
        raw(SYS_fcntl, plan.hold_read, F_SETFD, 0) != 0)
        finish(113);
    raw(SYS_execve, address(plan.executable), address(plan.argv), address(plan.environment));
    finish(114); // Actual exec failure, never a green facility observation.
}
} // namespace schema_foundation
