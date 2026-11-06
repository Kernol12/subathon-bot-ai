// SPDX-License-Identifier: GPL-3.0-or-later
// Independent fixed-child byte/status controls; this is not the transport owner.
#include "native_foundation_child.h"
#include <gtest/gtest.h>
#include <poll.h>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef SUBATHON_NATIVE_FAULT_CHILD_PATH
#error "Build must provide the fixed negative child artifact"
#endif
namespace {
using schema_foundation::address;
using schema_foundation::raw;
struct Literal {
    unsigned mode;
    const char *activation, *outcome;
    int status, signal;
};
constexpr Literal literals[] = {
    {1, "", "{\"code\":\"ok\"}\n", 0, 0},
    {2, "AA", "{\"code\":\"ok\"}\n", 0, 0},
    {3, "B", "{\"code\":\"ok\"}\n", 0, 0},
    {4, "A", nullptr, 0, 0},
    {5, "A", "{\"code\":\"unknown\"}\n", 0, 0},
    {6, "A", "{\"code\":\"ok\"}", 0, 0},
    {7, "A", "{\"code\":\"ok\"}\nextra", 0, 0},
    {8, "A", "{\"code\":\"usage_error\"}\n", 64, 0},
    {9, "A", "", 0, 0},
    {10, "A", "{\"code\":\"invalid_json\",\"secret\":\"PRIVATE\"}\n", 2, 0},
    {11, "A", "{\"code\":\"invalid_json\"}\n", 0, 0},
    {12, "A", "", 0, SIGKILL},
    {13, "", "{\"code\":\"environment_unavailable\"}\n", 3, 0},
    {14, "", "{\"code\":\"environment_unavailable\"}\n", 0, 0},
    {15, "", "", 3, 0},
    {16, "", "{\"code\":\"environment_unavailable\"}\nextra", 3, 0},
    {17, "", "{\"code\":\"environment_unavailable\"}", 3, 0},
    {18, "", " {\"code\":\"environment_unavailable\"}\n", 3, 0},
    {19, "B", "{\"code\":\"environment_unavailable\"}\n", 3, 0},
    {20, "AA", "{\"code\":\"environment_unavailable\"}\n", 3, 0},
    {21, "", "{\"code\":\"ok\"}\n", 0, 0},
    {22, "", "{\"code\":\"invalid_json\"}\n", 2, 0},
    {24, "B", "", 0, SIGKILL},
    {28, "", nullptr, 3, 0},
};
long now_ns() noexcept {
    timespec value{};
    return raw(SYS_clock_gettime, CLOCK_MONOTONIC, address(&value)) == 0
               ? value.tv_sec * 1000000000L + value.tv_nsec
               : -1;
}
void decimal(unsigned number, char *output) noexcept {
    char reverse[11];
    unsigned count = 0;
    do {
        reverse[count++] = static_cast<char>('0' + number % 10);
        number /= 10;
    } while (number);
    for (unsigned i = 0; i < count; ++i)
        output[i] = reverse[count - i - 1];
    output[count] = 0;
}
std::uint32_t word(const unsigned char *bytes) noexcept {
    return bytes[0] | (std::uint32_t{bytes[1]} << 8) | (std::uint32_t{bytes[2]} << 16) |
           (std::uint32_t{bytes[3]} << 24);
}
std::uint64_t wide(const unsigned char *bytes) noexcept {
    return word(bytes) | (std::uint64_t{word(bytes + 4)} << 32);
}
class NativeFaultChildBytes : public ::testing::TestWithParam<Literal> {};
TEST_P(NativeFaultChildBytes, ActualFixedExecBytesAndStatus) {
    const Literal expected = GetParam();
    // All allocation/file-action preparation is in the ordinary parent. libc
    // posix_spawn is a fixture launcher, not evidence for the future raw owner.
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attributes;
    int action_error = posix_spawn_file_actions_init(&actions);
    int attr_error = posix_spawnattr_init(&attributes);
    const bool have_actions = action_error == 0, have_attributes = attr_error == 0;
    int fds[4]{-1, -1, -1, -1};
    long closed[4]{-1, -1, -1, -1};
    bool setup = have_actions && have_attributes;
    schema_foundation::KernelAction child_profile;
    setup = setup && raw(SYS_rt_sigaction, SIGCHLD, 0, address(&child_profile), 8) == 0 &&
            child_profile.handler == 0 && !(child_profile.flags & SA_NOCLDWAIT);
    for (unsigned channel = 0; channel < 2 && setup; ++channel) {
        int pair[2]{-1, -1};
        if (raw(SYS_pipe2, address(pair), O_CLOEXEC) != 0) {
            setup = false;
            break;
        }
        for (unsigned end = 0; end < 2; ++end) {
            const long copy = raw(SYS_fcntl, pair[end], F_DUPFD_CLOEXEC, 10);
            raw(SYS_close, pair[end]);
            if (copy < 0)
                setup = false;
            else
                fds[2 * channel + end] = static_cast<int>(copy);
        }
    }
    // A deliberately unrelated, non-CLOEXEC read alias must not survive exec.
    // Keep it out of the existing four known-source close actions so the fixed
    // report independently detects incomplete child-table closure.
    int sentinel = -1;
    long sentinel_flags = -1, sentinel_close_result = -1, sentinel_closed = -1;
    if (setup) {
        const long copy = raw(SYS_fcntl, fds[0], F_DUPFD, 64);
        if (copy >= 0) {
            sentinel = static_cast<int>(copy); // Adopt ONLY a successful duplicate.
            sentinel_flags = raw(SYS_fcntl, sentinel, F_GETFD);
        }
        setup = copy >= 64 && sentinel_flags == 0;
    }
    auto action = [&](int result) {
        if (result != 0) {
            action_error = result;
            setup = false;
        }
    };
    if (setup) {
        action(posix_spawn_file_actions_addopen(&actions, 0, "/dev/null", O_RDONLY, 0));
        action(posix_spawn_file_actions_addopen(&actions, 1, "/dev/null", O_WRONLY, 0));
        action(posix_spawn_file_actions_addopen(&actions, 2, "/dev/null", O_WRONLY, 0));
        action(posix_spawn_file_actions_adddup2(&actions, fds[1], 3));
        action(posix_spawn_file_actions_adddup2(&actions, fds[3], 4));
        for (int fd : fds)
            action(posix_spawn_file_actions_addclose(&actions, fd));
        sigset_t defaults, mask;
        sigemptyset(&defaults);
        sigemptyset(&mask);
        for (int signal : {SIGINT, SIGPIPE, SIGCHLD, SIGTERM})
            sigaddset(&defaults, signal);
        attr_error = posix_spawnattr_setsigdefault(&attributes, &defaults);
        if (attr_error == 0)
            attr_error = posix_spawnattr_setsigmask(&attributes, &mask);
        if (attr_error == 0)
            attr_error = posix_spawnattr_setflags(&attributes,
                                                  POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK);
        setup = setup && attr_error == 0;
    }
    char mode[12], parent[12];
    decimal(expected.mode, mode);
    decimal(static_cast<unsigned>(raw(SYS_getpid)), parent);
    char executable[] = SUBATHON_NATIVE_FAULT_CHILD_PATH;
    char flag[] = "--case", parent_flag[] = "--parent";
    char *argv[]{executable, flag, mode, parent_flag, parent, nullptr};
    char locale[] = "LC_ALL=C.UTF-8";
    char *environment[]{locale, nullptr};
    pid_t pid = -1;
    const int spawned =
        setup ? posix_spawn(&pid, executable, &actions, &attributes, argv, environment) : -1;
    int identity = -1;
    bool fallback = false, io = true, killed = false;
    if (spawned == 0) {
        // Default SIGCHLD/no other reaper: our native PID remains a child or
        // zombie until this thread consumes it. No Python conversion window.
        identity = static_cast<int>(raw(SYS_pidfd_open, pid, 0));
        if (identity < 0)
            schema_foundation::finish(136); // No guessed numeric signal.
    }
    if (have_actions)
        posix_spawn_file_actions_destroy(&actions);
    if (have_attributes)
        posix_spawnattr_destroy(&attributes);
    if (sentinel >= 0) {
        sentinel_close_result = raw(SYS_close, sentinel); // Never retry Linux close.
        sentinel_closed = raw(SYS_fcntl, sentinel, F_GETFD);
    }
    for (unsigned index : {1U, 3U})
        if (fds[index] >= 0) {
            raw(SYS_close, fds[index]);
            closed[index] = raw(SYS_fcntl, fds[index], F_GETFD);
            fds[index] = -1;
        }
    unsigned char activation[3]{}, outcome[258]{};
    unsigned lengths[2]{};
    bool eof[2]{};
    const long start = now_ns(), deadline = start < 0 ? -1 : start + 3000000000L;
    while (spawned == 0 && io && (!eof[0] || !eof[1])) {
        const long now = now_ns();
        if (now < 0 || now >= deadline) {
            io = false;
            break;
        }
        pollfd items[2]{{fds[0], POLLIN, 0}, {fds[2], POLLIN, 0}};
        for (unsigned c = 0; c < 2; ++c)
            if (eof[c])
                items[c].fd = -1;
        const long polled = raw(SYS_poll, address(items), 2, (deadline - now + 999999) / 1000000);
        if (polled == -EINTR)
            continue;
        if (polled <= 0) {
            io = false;
            break;
        }
        for (unsigned c = 0; c < 2; ++c)
            if (items[c].revents) {
                if (items[c].revents & (POLLERR | POLLNVAL)) {
                    io = false;
                    break;
                }
                unsigned char *buffer = c ? outcome : activation;
                const unsigned cap = c ? 258 : 3;
                if (lengths[c] == cap) {
                    io = false;
                    break;
                }
                const long received =
                    raw(SYS_read, items[c].fd, address(buffer + lengths[c]), cap - lengths[c]);
                if (received == 0)
                    eof[c] = true;
                else if (received > 0)
                    lengths[c] += static_cast<unsigned>(received);
                else if (received != -EINTR) {
                    io = false;
                    break;
                }
            }
        if (expected.mode == 24 && !killed && lengths[0] == 1 && activation[0] == 'B') {
            killed = raw(SYS_pidfd_send_signal, identity, SIGKILL, 0, 0) == 0;
            if (!killed)
                io = false;
        }
    }
    siginfo_t status{};
    long waited = -1, identity_closed = -1;
    if (identity >= 0) {
        pollfd item{identity, POLLIN, 0};
        long ready;
        do {
            ready = raw(SYS_poll, address(&item), 1, 3000);
        } while (ready == -EINTR);
        if (!io || ready != 1) {
            fallback = true;
            long result;
            do {
                result = raw(SYS_pidfd_send_signal, identity, SIGKILL, 0, 0);
            } while (result == -EINTR);
        }
        do {
            waited = raw(SYS_waitid, 3, identity, address(&status), WEXITED | 0x20000000, 0);
        } while (waited == -EINTR);
        if (waited != 0 || status.si_pid != pid)
            schema_foundation::finish(137);
        raw(SYS_close, identity);
        identity_closed = raw(SYS_fcntl, identity, F_GETFD);
    }
    for (unsigned index : {0U, 2U})
        if (fds[index] >= 0) {
            raw(SYS_close, fds[index]);
            closed[index] = raw(SYS_fcntl, fds[index], F_GETFD);
            fds[index] = -1;
        }
    EXPECT_GE(sentinel, 64);
    EXPECT_EQ(sentinel_flags, 0);
    EXPECT_EQ(sentinel_close_result, 0);
    EXPECT_EQ(sentinel_closed, -EBADF);
    EXPECT_EQ(action_error, 0);
    EXPECT_EQ(attr_error, 0);
    EXPECT_TRUE(setup);
    EXPECT_EQ(spawned, 0);
    EXPECT_GE(identity, 0);
    EXPECT_EQ(identity_closed, -EBADF);
    EXPECT_FALSE(fallback);
    EXPECT_TRUE(io);
    for (long result : closed)
        EXPECT_EQ(result, -EBADF);
    EXPECT_TRUE(eof[0]);
    EXPECT_TRUE(eof[1]);
    EXPECT_EQ(waited, 0);
    EXPECT_EQ(status.si_pid, pid);
    EXPECT_NE(status.si_status, 125);
    EXPECT_EQ(status.si_code, expected.signal ? CLD_KILLED : CLD_EXITED);
    EXPECT_EQ(status.si_status, expected.signal ? expected.signal : expected.status);
    EXPECT_EQ(killed, expected.mode == 24);
    EXPECT_EQ(lengths[0], std::char_traits<char>::length(expected.activation));
    EXPECT_EQ(std::string(reinterpret_cast<char *>(activation), lengths[0]), expected.activation);
    if (expected.mode == 4) {
        EXPECT_EQ(lengths[1], 257U);
        for (unsigned i = 0; i < lengths[1]; ++i)
            EXPECT_EQ(outcome[i], 'x');
    } else if (expected.mode == 28) {
        ASSERT_EQ(lengths[1], 248U);
        EXPECT_EQ(std::string(reinterpret_cast<char *>(outcome), 4), "NTF1");
        EXPECT_EQ(word(outcome + 4), static_cast<unsigned>(pid));
        EXPECT_EQ(word(outcome + 8), 5U);
        EXPECT_EQ(word(outcome + 12), 6U);
        for (unsigned i = 0; i < 17; ++i)
            EXPECT_EQ(word(outcome + 16 + 4 * i), i < 6 ? i : UINT32_MAX);
        for (unsigned i = 0; i < 5; ++i) {
            EXPECT_EQ(word(outcome + 84 + 4 * i), 0U);
            EXPECT_EQ(word(outcome + 104 + 4 * i) & O_ACCMODE, i ? O_WRONLY : O_RDONLY);
            EXPECT_EQ(word(outcome + 104 + 4 * i) & O_NONBLOCK, 0U);
        }
        EXPECT_EQ(wide(outcome + 124), 0U);
        for (unsigned i = 0; i < 4; ++i) {
            const unsigned offset = 132 + 28 * i;
            EXPECT_EQ(word(outcome + offset), 0U);
            EXPECT_EQ(wide(outcome + offset + 4), 0U);
            // libc's fixture launcher may supply its x86-64 restorer metadata.
            // The later raw-owner packet oracle still requires exact flags0.
            EXPECT_EQ(wide(outcome + offset + 12) & ~std::uint64_t{0x04000000}, 0U);
            EXPECT_EQ(wide(outcome + offset + 20), 0U);
        }
        EXPECT_EQ(word(outcome + 244), 0U);
    } else {
        EXPECT_EQ(std::string(reinterpret_cast<char *>(outcome), lengths[1]), expected.outcome);
    }
}
INSTANTIATE_TEST_SUITE_P(ClosedBytes, NativeFaultChildBytes, ::testing::ValuesIn(literals));
} // namespace
