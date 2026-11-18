// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/session_clock.hpp>
#include <sys/timex.h>
#include <sys/types.h>
#include <time.h>

namespace subathon::server::session_clock::testing {
// Trusted internal syscall substitution for deterministic adapter tests. Production
// linux_observations() binds libc directly; no environment switch or caller trust.
// Return values/errno preserve native meanings. Calls are synchronous and finite.
class LinuxCalls {
  public:
    virtual ~LinuxCalls() = default;
    virtual int resolution(clockid_t, timespec *) = 0;
    virtual int create(clockid_t, int flags) = 0;
    virtual int time(clockid_t, timespec *) = 0;
    virtual int arm(int fd, int flags, const itimerspec *) = 0;
    virtual ssize_t read(int fd, void *, std::size_t) = 0;
    virtual int synchronization(timex *) = 0;
    virtual int close(int fd) noexcept = 0;
};
// Owns calls through descriptor cleanup. Same adapter/policy as real factory;
// replacing syscalls supplies observations only, never host scope or readiness.
std::unique_ptr<Observations> linux_observations(std::unique_ptr<LinuxCalls>);
} // namespace subathon::server::session_clock::testing
