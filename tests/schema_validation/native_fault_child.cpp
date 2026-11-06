// SPDX-License-Identifier: GPL-3.0-or-later
// Fixed negative exec fixture only: no owner, Python, SDK or successful validator.
#include "native_foundation_child.h"
#include <climits>
#include <sys/prctl.h>

namespace {
using schema_foundation::address;
using schema_foundation::finish;
using schema_foundation::KernelAction;
using schema_foundation::raw;

constexpr int fixture_failure = 125;
static_assert(CHAR_BIT == 8 && sizeof(int) == 4 && sizeof(std::uint64_t) == 8);

[[gnu::no_stack_protector]] bool literal(const char *value, const char *expected) noexcept {
    for (unsigned index = 0; index != 16; ++index) {
        if (value[index] != expected[index])
            return false;
        if (expected[index] == '\0')
            return true;
    }
    return false;
}
[[gnu::no_stack_protector]] int positive_decimal(const char *value) noexcept {
    if (value[0] < '1' || value[0] > '9')
        return -1;
    unsigned result = 0;
    for (unsigned index = 0; index <= 10; ++index) {
        const unsigned byte = static_cast<unsigned char>(value[index]);
        if (byte == 0)
            return static_cast<int>(result);
        if (byte < '0' || byte > '9' || result > (INT_MAX - (byte - '0')) / 10U)
            return -1;
        result = result * 10 + byte - '0';
    }
    return -1;
}
[[gnu::no_stack_protector]] void send(int descriptor, const char *bytes, unsigned count) noexcept {
    // Each payload fits PIPE_BUF. A short/error write is a FIXTURE failure;
    // do not normalize, retry a partial packet or manufacture a result line.
    if (raw(SYS_write, descriptor, address(bytes), count) != count)
        finish(fixture_failure);
}
template <std::size_t Count>
[[gnu::no_stack_protector]] void send(int descriptor, const char (&bytes)[Count]) noexcept {
    static_assert(Count >= 1 && Count <= 258);
    send(descriptor, bytes, static_cast<unsigned>(Count - 1));
}
[[gnu::no_stack_protector]] void close_checked(int descriptor) noexcept {
    if (raw(SYS_close, descriptor) != 0)
        finish(fixture_failure); // No Linux close retry on a released number.
}
[[gnu::no_stack_protector]] void put32(unsigned char *packet, unsigned &offset,
                                       std::int64_t value) noexcept {
    const auto bits = static_cast<std::uint32_t>(value);
    for (unsigned byte = 0; byte != 4; ++byte)
        packet[offset++] = static_cast<unsigned char>(bits >> (8 * byte));
}
[[gnu::no_stack_protector]] void put64(unsigned char *packet, unsigned &offset,
                                       std::uint64_t bits) noexcept {
    for (unsigned byte = 0; byte != 8; ++byte)
        packet[offset++] = static_cast<unsigned char>(bits >> (8 * byte));
}
[[gnu::no_stack_protector]] bool dot_name(const unsigned char *name, unsigned bytes) noexcept {
    return bytes >= 2 && name[0] == '.' &&
           (name[1] == 0 || (bytes >= 3 && name[1] == '.' && name[2] == 0));
}
[[gnu::no_stack_protector]] int fd_name(const unsigned char *name, unsigned bytes) noexcept {
    unsigned value = 0;
    for (unsigned index = 0; index != bytes && index <= 10; ++index) {
        const unsigned byte = name[index];
        if (byte == 0)
            return index ? static_cast<int>(value) : -1;
        if (byte < '0' || byte > '9' || value > (INT_MAX - (byte - '0')) / 10U)
            return -1;
        value = value * 10 + byte - '0';
    }
    return -1;
}

[[noreturn, gnu::no_stack_protector]] void report() noexcept {
    unsigned char packet[248];
    unsigned offset = 0;
    packet[offset++] = 'N';
    packet[offset++] = 'T';
    packet[offset++] = 'F';
    packet[offset++] = '1';
    put32(packet, offset, raw(SYS_getpid));

    const long directory =
        raw(SYS_openat, AT_FDCWD, address("/proc/self/fd"), O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
    put32(packet, offset, directory);
    int descriptors[17];
    for (auto &descriptor : descriptors)
        descriptor = -1;
    int count = directory >= 0 ? 0 : -1;
    long terminal = -1;
    if (directory >= 0) {
        unsigned char buffer[4096];
        // At most 17 numeric names plus dot entries and actual EOF. The cap
        // also bounds unusual short kernel batches; an incomplete scan is not EOF.
        for (unsigned batch = 0; batch != 20 && count < 17; ++batch) {
            terminal = raw(SYS_getdents64, directory, address(buffer), sizeof(buffer));
            if (terminal <= 0)
                break;
            if (terminal > static_cast<long>(sizeof(buffer)))
                finish(fixture_failure);
            unsigned cursor = 0;
            while (cursor < static_cast<unsigned long>(terminal) && count < 17) {
                if (static_cast<unsigned long>(terminal) - cursor < 20)
                    finish(fixture_failure);
                const unsigned record_bytes =
                    buffer[cursor + 16] | (static_cast<unsigned>(buffer[cursor + 17]) << 8);
                if (record_bytes < 20 ||
                    record_bytes > static_cast<unsigned long>(terminal) - cursor)
                    finish(fixture_failure);
                const auto *name = buffer + cursor + 19;
                const unsigned name_bytes = record_bytes - 19;
                if (!dot_name(name, name_bytes)) {
                    const int descriptor = fd_name(name, name_bytes);
                    if (descriptor < 0)
                        finish(fixture_failure);
                    descriptors[count++] = descriptor;
                }
                cursor += record_bytes;
            }
        }
    }
    // These are OBSERVED integers, not a clean/truncated-prefix boolean.
    for (int index = 1; index < count; ++index) {
        const int value = descriptors[index];
        int previous = index;
        while (previous && descriptors[previous - 1] > value) {
            descriptors[previous] = descriptors[previous - 1];
            --previous;
        }
        descriptors[previous] = value;
    }
    put32(packet, offset, count);
    for (int descriptor : descriptors)
        put32(packet, offset, descriptor);
    for (int descriptor = 0; descriptor != 5; ++descriptor)
        put32(packet, offset, raw(SYS_fcntl, descriptor, F_GETFD));
    for (int descriptor = 0; descriptor != 5; ++descriptor)
        put32(packet, offset, raw(SYS_fcntl, descriptor, F_GETFL));

    std::uint64_t mask = UINT64_MAX;
    if (raw(SYS_rt_sigprocmask, SIG_SETMASK, 0, address(&mask), 8) != 0)
        mask = UINT64_MAX;
    put64(packet, offset, mask);
    constexpr int signals[] = {SIGINT, SIGPIPE, SIGCHLD, SIGTERM};
    for (int signal : signals) {
        KernelAction action{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
        const long result = raw(SYS_rt_sigaction, signal, 0, address(&action), 8);
        put32(packet, offset, result);
        put64(packet, offset, result == 0 ? action.handler : UINT64_MAX);
        put64(packet, offset, result == 0 ? action.flags : UINT64_MAX);
        put64(packet, offset, result == 0 ? action.mask : UINT64_MAX);
    }
    put32(packet, offset, terminal);
    if (offset != sizeof(packet))
        finish(fixture_failure);
    if (directory >= 0)
        close_checked(static_cast<int>(directory));
    // 3/4 remained OPEN for ALL observations. Close only the newly opened scan
    // FD, commit ALL248 atomically on4 WHILE3 OPEN, only then close3 WITHOUT A.
    if (raw(SYS_write, 4, address(packet), sizeof(packet)) != static_cast<long>(sizeof(packet)))
        finish(fixture_failure);
    close_checked(3);
    close_checked(4);
    finish(3); // Owner may instead have already killed on the invalid NTF1 packet.
}

[[noreturn, gnu::no_stack_protector]] void protocol(unsigned mode) noexcept {
    if (mode == 24) {
        // ONLY cleanup case24 phase34/35 selects this exec mode. The tracked
        // parent watchdog/pidfd is the finite backup; no hidden child timer can
        // make a missed owner kill look like successful cleanup.
        send(3, "B");
        // Keep BOTH write channels open and publish no outcome until real termination.
        for (;;)
            if (raw(SYS_pause) != -EINTR)
                finish(fixture_failure); // Unsupported hold is not a working fixture.
    }
    if (mode == 2 || mode == 20)
        send(3, "AA");
    else if (mode == 3 || mode == 19)
        send(3, "B");
    else if (mode >= 4 && mode <= 12)
        send(3, "A");
    close_checked(3);

    // Use explicit conditional groups; the generated raw-graph audit still applies.
    if (mode == 1 || mode == 2 || mode == 3 || mode == 21) {
        send(4, "{\"code\":\"ok\"}\n");
    } else if (mode == 4) {
        char excess[257];
        for (auto &byte : excess)
            byte = 'x';
        send(4, excess, static_cast<unsigned>(sizeof(excess)));
    } else if (mode == 5) {
        send(4, "{\"code\":\"unknown\"}\n");
    } else if (mode == 6) {
        send(4, "{\"code\":\"ok\"}");
    } else if (mode == 7) {
        send(4, "{\"code\":\"ok\"}\nextra");
    } else if (mode == 8) {
        send(4, "{\"code\":\"usage_error\"}\n");
    } else if (mode == 10) {
        send(4, "{\"code\":\"invalid_json\",\"secret\":\"PRIVATE\"}\n");
    } else if (mode == 11 || mode == 22) {
        send(4, "{\"code\":\"invalid_json\"}\n");
    } else if (mode == 13 || mode == 14 || mode == 19 || mode == 20) {
        send(4, "{\"code\":\"environment_unavailable\"}\n");
    } else if (mode == 16) {
        send(4, "{\"code\":\"environment_unavailable\"}\nextra");
    } else if (mode == 17) {
        send(4, "{\"code\":\"environment_unavailable\"}");
    } else if (mode == 18) {
        send(4, " {\"code\":\"environment_unavailable\"}\n");
    } else if (mode == 9 || mode == 12 || mode == 15) {
        // Literal empty outcome plus actual EOF.
    } else {
        finish(fixture_failure);
    }
    close_checked(4);
    if (mode == 12) {
        raw(SYS_tgkill, raw(SYS_getpid), raw(SYS_gettid), SIGKILL); // SELF only.
        finish(fixture_failure); // A failed kill cannot become the intended death.
    }
    const int status = mode == 8                                ? 64
                       : mode == 10 || mode == 22               ? 2
                       : mode >= 13 && mode <= 20 && mode != 14 ? 3
                                                                : 0;
    finish(status);
}
} // namespace

// Narrow fixed raw child graph, not a target-wide parent hardening exception.
[[gnu::no_stack_protector]] int main(int argc, char **argv) {
    if (argc != 5 || !literal(argv[1], "--case") || !literal(argv[3], "--parent"))
        finish(fixture_failure);
    const int mode = positive_decimal(argv[2]), parent = positive_decimal(argv[4]);
    if (parent < 1 || !((mode >= 1 && mode <= 22) || mode == 24 || mode == 28))
        finish(fixture_failure);
    // The caller precomputed its genuine PID. Never discover/signal a guessed
    // numeric parent: compare kernel parent identity before AND after PDEATHSIG.
    if (raw(SYS_getppid) != parent || raw(SYS_prctl, PR_SET_PDEATHSIG, SIGKILL) != 0 ||
        raw(SYS_getppid) != parent)
        finish(fixture_failure);
    if (mode == 28)
        report();
    protocol(static_cast<unsigned>(mode));
}
