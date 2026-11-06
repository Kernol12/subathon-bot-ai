// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SUBATHON_SCHEMA_VALIDATION_NATIVE_RAW_OPS_V1_H
#define SUBATHON_SCHEMA_VALIDATION_NATIVE_RAW_OPS_V1_H

#include "transport_core_v1.h"
#include <cerrno>
#include <sys/syscall.h>
#include <time.h>

#if !defined(__linux__) || !defined(__x86_64__)
#error "Schema native transport requires the reviewed Linux x86-64 profile"
#endif

namespace subathon::schema_validation::native {

// Stateless production policy. No libc errno/TLS, callback, resource or fault plan.
class [[gnu::visibility("hidden")]] RawOps {
    template <KernelCall Call> static consteval long number() noexcept {
        if constexpr (Call == KernelCall::openat)
            return SYS_openat;
        else if constexpr (Call == KernelCall::fstat)
            return SYS_fstat;
        else if constexpr (Call == KernelCall::newfstatat)
            return SYS_newfstatat;
        else if constexpr (Call == KernelCall::read)
            return SYS_read;
        else if constexpr (Call == KernelCall::write)
            return SYS_write;
        else if constexpr (Call == KernelCall::close)
            return SYS_close;
        else if constexpr (Call == KernelCall::fcntl)
            return SYS_fcntl;
        else if constexpr (Call == KernelCall::pipe2)
            return SYS_pipe2;
        else if constexpr (Call == KernelCall::clone3)
            return SYS_clone3;
        else if constexpr (Call == KernelCall::dup3)
            return SYS_dup3;
        else if constexpr (Call == KernelCall::close_range)
            return SYS_close_range;
        else if constexpr (Call == KernelCall::execve)
            return SYS_execve;
        else if constexpr (Call == KernelCall::exit)
            return SYS_exit_group;
        else if constexpr (Call == KernelCall::clock_gettime)
            return SYS_clock_gettime;
        else if constexpr (Call == KernelCall::poll)
            return SYS_poll;
        else if constexpr (Call == KernelCall::rt_sigaction)
            return SYS_rt_sigaction;
        else if constexpr (Call == KernelCall::rt_sigprocmask)
            return SYS_rt_sigprocmask;
        else if constexpr (Call == KernelCall::pidfd_send_signal)
            return SYS_pidfd_send_signal;
        else if constexpr (Call == KernelCall::waitid)
            return SYS_waitid;
        else {
            static_assert(Call == KernelCall::getpid);
            return SYS_getpid;
        }
    }

  public:
    explicit RawOps(const Request &) noexcept {}

    template <KernelCall Call>
    [[gnu::no_stack_protector]] RawResult syscall(NativeWord a = 0, NativeWord b = 0,
                                                  NativeWord c = 0, NativeWord d = 0,
                                                  NativeWord e = 0, NativeWord f = 0) noexcept {
        long result;
        const long four = static_cast<long>(d), five = static_cast<long>(e),
                   six = static_cast<long>(f);
        __asm__ volatile(
            "mov %[four], %%r10; mov %[five], %%r8; mov %[six], %%r9; syscall"
            : "=a"(result)
            : "a"(number<Call>()), "D"(static_cast<long>(a)), "S"(static_cast<long>(b)),
              "d"(static_cast<long>(c)), [four] "r"(four), [five] "r"(five), [six] "r"(six)
            : "r10", "r8", "r9", "rcx", "r11", "cc", "memory");
        return result;
    }

    [[gnu::no_stack_protector]] RawResult monotonic_ns() noexcept {
        timespec now;
        const auto result =
            syscall<KernelCall::clock_gettime>(CLOCK_MONOTONIC, reinterpret_cast<NativeWord>(&now));
        if (result != 0)
            return result < 0 ? result : -EIO;
        constexpr RawResult second = 1000000000;
        if (now.tv_sec < 0 || now.tv_nsec < 0 || now.tv_nsec >= second ||
            now.tv_sec > INT64_MAX / second)
            return -EOVERFLOW;
        const RawResult whole = now.tv_sec * second;
        return whole <= INT64_MAX - now.tv_nsec ? whole + now.tv_nsec : -EOVERFLOW;
    }

    [[gnu::no_stack_protector]] Continuation checkpoint(Phase, RawResult) noexcept {
        return Continuation::proceed;
    }
    [[gnu::no_stack_protector]] void observe(const Observation &) noexcept {}
};

static_assert(sizeof(long) == 8 && sizeof(timespec) == 16);
static_assert(TransportOps<RawOps>);

} // namespace subathon::schema_validation::native
#endif // SUBATHON_SCHEMA_VALIDATION_NATIVE_RAW_OPS_V1_H
