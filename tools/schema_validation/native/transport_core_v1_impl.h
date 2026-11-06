// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SUBATHON_SCHEMA_VALIDATION_NATIVE_TRANSPORT_CORE_V1_IMPL_H
#define SUBATHON_SCHEMA_VALIDATION_NATIVE_TRANSPORT_CORE_V1_IMPL_H

#include "transport_core_v1.h"
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <fcntl.h>
#include <linux/sched.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/wait.h>

namespace subathon::schema_validation::native::detail {

constexpr RawResult second = 1000000000;
constexpr RawResult work_budget = 10 * second;
constexpr int same_thread = 0x20000000;
constexpr unsigned pidfd_selector = 3;
constexpr int invalid_path = 2, environment_unavailable = 4, resource_limit = 5;
constexpr int validation_unavailable = 17, interrupted = 18;

template <class T> [[gnu::no_stack_protector]] NativeWord address(T *value) noexcept {
    return reinterpret_cast<NativeWord>(value);
}
[[gnu::no_stack_protector]] inline NativeWord word(int value) noexcept {
    return static_cast<NativeWord>(value);
}

struct KernelAction {
    std::uint64_t handler, flags, restorer, mask;
};
static_assert(sizeof(KernelAction) == 32 && offsetof(KernelAction, mask) == 24);
static_assert(sizeof(siginfo_t) == 128 && sizeof(struct stat) == 144);

// No large aggregate zero-initialization: copies/records are filled explicitly
// before use. In particular the post-clone path cannot lower to libc memset.
struct Scratch {
    unsigned char paths[4][4097];
    unsigned char config_path[4097], config_bytes[4097];
    unsigned char activation[2], outcome[257];
    struct stat initial, final, named;
    siginfo_t status;
    clone_args birth;
    KernelAction defaults, sigchld;
    std::uint64_t empty_mask;
    const char *argv[13], *environment[4];
    pollfd watched[3];
    int birth_pidfd;
    int pipe_result[2];
};
static_assert(sizeof(Scratch) <= 28 * 1024);

[[gnu::no_stack_protector]] inline bool shape(PathView value) noexcept {
    return value.data && value.bytes >= 1 && value.bytes <= 4096;
}
[[gnu::no_stack_protector]] inline bool copy_path(PathView value,
                                                  unsigned char *destination) noexcept {
    if (!shape(value))
        return false;
    const volatile unsigned char *source = value.data;
    volatile unsigned char *output = destination;
    unsigned remaining = 0, lower = 0x80, upper = 0xbf;
    for (unsigned index = 0; index != value.bytes; ++index) {
        const unsigned byte = source[index];
        if (byte == 0)
            return false;
        output[index] = static_cast<unsigned char>(byte);
        if (remaining) {
            if (byte < lower || byte > upper)
                return false;
            --remaining;
            lower = 0x80;
            upper = 0xbf;
        } else if (byte >= 0x80) {
            if (byte >= 0xc2 && byte <= 0xdf) {
                remaining = 1;
            } else if (byte >= 0xe0 && byte <= 0xef) {
                remaining = 2;
                lower = byte == 0xe0 ? 0xa0 : 0x80;
                upper = byte == 0xed ? 0x9f : 0xbf;
            } else if (byte >= 0xf0 && byte <= 0xf4) {
                remaining = 3;
                lower = byte == 0xf0 ? 0x90 : 0x80;
                upper = byte == 0xf4 ? 0x8f : 0xbf;
            } else {
                return false;
            }
        }
    }
    if (remaining || source[value.bytes] != 0)
        return false;
    output[value.bytes] = 0;
    return true;
}
[[gnu::no_stack_protector]] inline bool ascii_alpha(unsigned byte) noexcept {
    return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z');
}
[[gnu::no_stack_protector]] inline bool uri_scheme(const unsigned char *path,
                                                   unsigned count) noexcept {
    if (!ascii_alpha(path[0]))
        return false;
    for (unsigned index = 1; index != count; ++index) {
        const unsigned byte = path[index];
        if (byte == ':')
            return true;
        if (!ascii_alpha(byte) && !(byte >= '0' && byte <= '9') && byte != '+' && byte != '-' &&
            byte != '.')
            return false;
    }
    return false;
}
inline bool same_snapshot(const struct stat &left, const struct stat &right) noexcept {
    return left.st_dev == right.st_dev && left.st_ino == right.st_ino &&
           left.st_mode == right.st_mode && left.st_nlink == right.st_nlink &&
           left.st_size == right.st_size && left.st_mtim.tv_sec == right.st_mtim.tv_sec &&
           left.st_mtim.tv_nsec == right.st_mtim.tv_nsec &&
           left.st_ctim.tv_sec == right.st_ctim.tv_sec &&
           left.st_ctim.tv_nsec == right.st_ctim.tv_nsec;
}

struct WireOutcome {
    const char *line;
    unsigned bytes;
    int value, status;
};
template <unsigned Count>
constexpr WireOutcome wire(const char (&line)[Count], int value, int status) noexcept {
    static_assert(Count >= 2 && Count <= 257);
    return {line, Count - 1, value, status};
}
// Exact wire bytes only. No JSON parser, exception text or worker usage_error.
inline constexpr WireOutcome outcomes[] = {
    wire("{\"code\":\"ok\"}\n", 0, 0),
    wire("{\"code\":\"invalid_path\"}\n", 2, 2),
    wire("{\"code\":\"io_unavailable\"}\n", 3, 3),
    wire("{\"code\":\"environment_unavailable\"}\n", 4, 3),
    wire("{\"code\":\"resource_limit\"}\n", 5, 4),
    wire("{\"code\":\"invalid_json\"}\n", 6, 2),
    wire("{\"code\":\"invalid_manifest\"}\n", 7, 2),
    wire("{\"code\":\"unsupported_dialect\"}\n", 8, 2),
    wire("{\"code\":\"unsupported_format\"}\n", 9, 2),
    wire("{\"code\":\"dependency_unavailable\"}\n", 10, 3),
    wire("{\"code\":\"invalid_schema\"}\n", 11, 2),
    wire("{\"code\":\"unresolved_reference\"}\n", 12, 2),
    wire("{\"code\":\"reference_cycle\"}\n", 13, 2),
    wire("{\"code\":\"invalid_inventory\"}\n", 14, 2),
    wire("{\"code\":\"unsupported_feature\"}\n", 15, 2),
    wire("{\"code\":\"invalid_instance\"}\n", 16, 2),
    wire("{\"code\":\"validation_unavailable\"}\n", 17, 3),
};
inline constexpr char setup_refusal[] = "{\"code\":\"environment_unavailable\"}\n";
inline constexpr char config_suffix[] = "/pyvenv.cfg";
static_assert(sizeof(setup_refusal) - 1 == 35);

template <TransportOps Ops> class [[gnu::visibility("hidden")]] Owner {
    // Five source roles are preserved while each genuine duplicate replaces its
    // original through the one temporary slot. PIDFD is always released last.
    enum Slot : unsigned {
        activation_read,
        activation_write,
        outcome_read,
        outcome_write,
        null_source,
        config,
        temporary,
        pidfd,
        slot_count
    };
    Ops &ops_;
    Scratch &scratch_;
    const RawResult deadline_;
    RawResult last_clock_;
    int descriptors_[slot_count] = {-1, -1, -1, -1, -1, -1, -1, -1};
    RawResult pid_ = -1;
    int decision_ = -1, child_code_ = 0, child_status_ = -1;
    bool quiescent_ = false, finished_ = false, had_resource_ = false;
    bool activation_eof_ = false, outcome_eof_ = false;
    unsigned activation_bytes_ = 0, outcome_bytes_ = 0;
    Phase phase_{PhaseBase::config_open, PhaseEdge::before};

    void refuse(int value) noexcept {
        if (decision_ == resource_limit)
            return;
        if (value == resource_limit || (value == interrupted && decision_ != interrupted))
            decision_ = value;
        else if (decision_ != interrupted && (value == validation_unavailable || decision_ < 0))
            decision_ = value;
    }
    static bool capacity_error(RawResult result) noexcept {
        return result == -ENOMEM || result == -EMFILE || result == -ENFILE;
    }
    void syscall_failure(RawResult result, int other, bool work = true) noexcept {
        if (capacity_error(result))
            refuse(resource_limit);
        else if (result == -EINTR && work)
            refuse(interrupted);
        else
            refuse(other);
    }
    bool checkpoint(PhaseBase base, PhaseEdge edge, RawResult result, bool work = true) noexcept {
        phase_ = {base, edge};
        const auto next = ops_.checkpoint(phase_, result);
        if (next == Continuation::proceed)
            return true;
        if (next == Continuation::resource_refusal)
            refuse(resource_limit);
        else if (next == Continuation::interrupted) {
            if (work)
                refuse(interrupted);
        } else {
            refuse(validation_unavailable);
        }
        return false;
    }
    [[gnu::no_stack_protector]] void adopted(Slot slot) noexcept {
        had_resource_ = true;
        ops_.observe(
            {ObservationKind::adopt, phase_, descriptors_[slot], pid_, descriptors_[slot]});
    }
    [[gnu::no_stack_protector]] void close_bare(Slot slot) noexcept {
        if (descriptors_[slot] < 0)
            return;
        const int descriptor = descriptors_[slot];
        const auto result = ops_.template syscall<KernelCall::close>(word(descriptor));
        descriptors_[slot] = -1; // Actual Linux close precedes slot invalidation; NO retry.
        ops_.observe({ObservationKind::release, phase_, descriptor, pid_, result});
        if (result != 0)
            syscall_failure(result, validation_unavailable, false);
    }
    [[gnu::no_stack_protector]] bool close_work(Slot slot, PhaseBase base) noexcept {
        if (!checkpoint(base, PhaseEdge::before, -1))
            return false;
        const int descriptor = descriptors_[slot];
        const auto result = ops_.template syscall<KernelCall::close>(word(descriptor));
        descriptors_[slot] = -1;
        ops_.observe({ObservationKind::release, phase_, descriptor, pid_, result});
        if (result != 0) {
            syscall_failure(result, validation_unavailable);
            return false;
        }
        return checkpoint(base, PhaseEdge::after, result);
    }
    bool still_working(RawResult *remaining = nullptr) noexcept {
        const auto now = ops_.monotonic_ns();
        if (now < 0) {
            syscall_failure(now, had_resource_ ? validation_unavailable : environment_unavailable);
            return false;
        }
        if (now < last_clock_) {
            refuse(had_resource_ ? validation_unavailable : environment_unavailable);
            return false;
        }
        last_clock_ = now;
        if (now >= deadline_) {
            refuse(resource_limit);
            return false;
        }
        if (remaining)
            *remaining = deadline_ - now;
        return decision_ < 0;
    }
    bool config_name() noexcept {
        unsigned end = 0;
        while (scratch_.paths[1][end])
            ++end;
        unsigned slash = end;
        while (slash && scratch_.paths[1][slash - 1] != '/')
            --slash;
        if (slash < 5 || slash == end || scratch_.paths[1][slash - 5] != '/' ||
            scratch_.paths[1][slash - 4] != 'b' || scratch_.paths[1][slash - 3] != 'i' ||
            scratch_.paths[1][slash - 2] != 'n')
            return false;
        const unsigned root_bytes = slash - 5;
        if (root_bytes + sizeof(config_suffix) > sizeof(scratch_.config_path))
            return false;
        volatile unsigned char *out = scratch_.config_path;
        for (unsigned index = 0; index != root_bytes; ++index)
            out[index] = scratch_.paths[1][index];
        for (unsigned index = 0; index != sizeof(config_suffix); ++index)
            out[root_bytes + index] = static_cast<unsigned char>(config_suffix[index]);
        return true;
    }
    bool read_config() noexcept {
        if (!still_working() || !checkpoint(PhaseBase::config_open, PhaseEdge::before, -1))
            return false;
        const auto opened = ops_.template syscall<KernelCall::openat>(
            word(AT_FDCWD), address(scratch_.config_path),
            O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
        if (opened < 0) {
            syscall_failure(opened, environment_unavailable);
            return false;
        }
        descriptors_[config] = static_cast<int>(opened);
        adopted(config);
        if (!checkpoint(PhaseBase::config_open, PhaseEdge::after, opened))
            return false;
        auto result = ops_.template syscall<KernelCall::fstat>(word(descriptors_[config]),
                                                               address(&scratch_.initial));
        if (result != 0) {
            syscall_failure(result, environment_unavailable);
            return false;
        }
        if (!S_ISREG(scratch_.initial.st_mode) || scratch_.initial.st_size < 0 ||
            scratch_.initial.st_size > 4096) {
            refuse(environment_unavailable);
            return false;
        }
        unsigned used = 0;
        bool eof = false;
        while (used != sizeof(scratch_.config_bytes)) {
            if (!still_working() || !checkpoint(PhaseBase::config_read, PhaseEdge::before, -1))
                return false;
            result = ops_.template syscall<KernelCall::read>(word(descriptors_[config]),
                                                             address(scratch_.config_bytes + used),
                                                             sizeof(scratch_.config_bytes) - used);
            if (result < 0) {
                syscall_failure(result, environment_unavailable);
                return false;
            }
            if (result == 0) {
                eof = true;
                break;
            }
            used += static_cast<unsigned>(result);
            if (!checkpoint(PhaseBase::config_read, PhaseEdge::after, result))
                return false;
            if (used > 4096)
                break;
        }
        if (!eof || used != static_cast<unsigned long>(scratch_.initial.st_size)) {
            refuse(environment_unavailable);
            return false;
        }
        result = ops_.template syscall<KernelCall::fstat>(word(descriptors_[config]),
                                                          address(&scratch_.final));
        if (result == 0)
            result = ops_.template syscall<KernelCall::newfstatat>(
                word(AT_FDCWD), address(scratch_.config_path), address(&scratch_.named),
                AT_SYMLINK_NOFOLLOW);
        if (result != 0) {
            syscall_failure(result, environment_unavailable);
            return false;
        }
        if (!same_snapshot(scratch_.initial, scratch_.final) ||
            !same_snapshot(scratch_.initial, scratch_.named)) {
            refuse(environment_unavailable);
            return false;
        }
        return close_work(config, PhaseBase::config_close);
    }
    bool make_pipe(Slot read_end, Slot write_end, PhaseBase base) noexcept {
        if (!still_working() || !checkpoint(base, PhaseEdge::before, -1))
            return false;
        const auto result =
            ops_.template syscall<KernelCall::pipe2>(address(scratch_.pipe_result), O_CLOEXEC);
        if (result < 0) {
            syscall_failure(result, environment_unavailable);
            return false;
        }
        descriptors_[read_end] = scratch_.pipe_result[0];
        descriptors_[write_end] = scratch_.pipe_result[1];
        adopted(read_end);
        adopted(write_end);
        if (!checkpoint(base, PhaseEdge::after, result))
            return false;
        const auto flags =
            ops_.template syscall<KernelCall::fcntl>(word(descriptors_[read_end]), F_GETFL);
        if (flags < 0) {
            syscall_failure(flags, environment_unavailable);
            return false;
        }
        const auto changed = ops_.template syscall<KernelCall::fcntl>(
            word(descriptors_[read_end]), F_SETFL, static_cast<NativeWord>(flags) | O_NONBLOCK);
        if (changed != 0) {
            syscall_failure(changed, environment_unavailable);
            return false;
        }
        return true;
    }
    bool prepare_sources() noexcept {
        if (!make_pipe(activation_read, activation_write, PhaseBase::activation_pipe) ||
            !make_pipe(outcome_read, outcome_write, PhaseBase::outcome_pipe) || !still_working() ||
            !checkpoint(PhaseBase::null_open, PhaseEdge::before, -1))
            return false;
        const auto opened = ops_.template syscall<KernelCall::openat>(
            word(AT_FDCWD), address("/dev/null"), O_RDWR | O_CLOEXEC);
        if (opened < 0) {
            syscall_failure(opened, environment_unavailable);
            return false;
        }
        descriptors_[null_source] = static_cast<int>(opened);
        adopted(null_source);
        if (!checkpoint(PhaseBase::null_open, PhaseEdge::after, opened))
            return false;
        for (unsigned index = 0; index != 5; ++index) {
            if (!still_working() || !checkpoint(PhaseBase::source_duplicate, PhaseEdge::before, -1))
                return false;
            const auto duplicate = ops_.template syscall<KernelCall::fcntl>(
                word(descriptors_[index]), F_DUPFD_CLOEXEC, 5);
            if (duplicate < 0) {
                syscall_failure(duplicate, environment_unavailable);
                return false;
            }
            descriptors_[temporary] = static_cast<int>(duplicate);
            adopted(temporary);
            if (!checkpoint(PhaseBase::source_duplicate, PhaseEdge::after, duplicate))
                return false;
            close_bare(static_cast<Slot>(index));
            descriptors_[index] = descriptors_[temporary];
            descriptors_[temporary] = -1; // Move one owned identity, not another acquisition.
            if (decision_ >= 0)
                return false;
        }
        return true;
    }

    // Raw terminal path: normal child completion, or a fatal backend defect in
    // the caller. It cannot invoke a user SIGILL handler or fabricate a return.
    [[noreturn, gnu::no_stack_protector]] void child_exit(int status) noexcept {
        for (;;)
            ops_.template syscall<KernelCall::exit>(word(status));
    }
    [[noreturn, gnu::no_stack_protector]] void child_refusal(bool remapped) noexcept {
        const int output = remapped ? 4 : descriptors_[outcome_write];
        const int activation = remapped ? 3 : descriptors_[activation_write];
        // A single small blocking write; no activation byte or exception text.
        ops_.template syscall<KernelCall::write>(word(output), address(setup_refusal), 35);
        ops_.template syscall<KernelCall::close>(word(activation));
        child_exit(3); // Kernel exit closes every inherited duplicate, including output.
    }
    [[noreturn, gnu::noinline, gnu::no_stack_protector]] void child() noexcept {
        if (ops_.checkpoint({PhaseBase::child_signal, PhaseEdge::before}, -1) !=
            Continuation::proceed)
            child_refusal(false);
        RawResult result = 0;
        for (int signal = 1; signal <= 64; ++signal) {
            if (signal == SIGKILL || signal == SIGSTOP)
                continue;
            result = ops_.template syscall<KernelCall::rt_sigaction>(
                word(signal), address(&scratch_.defaults), 0, 8);
            if (result != 0)
                break;
        }
        if (result == 0)
            result = ops_.template syscall<KernelCall::rt_sigprocmask>(
                SIG_SETMASK, address(&scratch_.empty_mask), 0, 8);
        const auto signal_next =
            ops_.checkpoint({PhaseBase::child_signal, PhaseEdge::after}, result);
        if (result != 0 || signal_next != Continuation::proceed)
            child_refusal(false);
        for (int destination = 0; destination != 5; ++destination) {
            if (ops_.checkpoint({PhaseBase::child_dup, PhaseEdge::before}, -1) !=
                Continuation::proceed)
                child_refusal(false);
            const int source = destination < 3    ? descriptors_[null_source]
                               : destination == 3 ? descriptors_[activation_write]
                                                  : descriptors_[outcome_write];
            result = ops_.template syscall<KernelCall::dup3>(word(source), word(destination), 0);
            if (result < 0)
                child_refusal(false);
            if (ops_.checkpoint({PhaseBase::child_dup, PhaseEdge::after}, result) !=
                Continuation::proceed)
                child_refusal(false);
        }
        if (ops_.checkpoint({PhaseBase::child_close_range, PhaseEdge::before}, -1) !=
            Continuation::proceed)
            child_refusal(true);
        result = ops_.template syscall<KernelCall::close_range>(5, UINT_MAX, 0);
        if (result != 0 || ops_.checkpoint({PhaseBase::child_close_range, PhaseEdge::after},
                                           result) != Continuation::proceed)
            child_refusal(true);
        if (ops_.checkpoint({PhaseBase::child_exec, PhaseEdge::before}, -1) !=
            Continuation::proceed)
            child_refusal(true);
        result = ops_.template syscall<KernelCall::execve>(
            address(scratch_.paths[1]), address(scratch_.argv), address(scratch_.environment));
        ops_.checkpoint({PhaseBase::child_exec, PhaseEdge::after}, result);
        child_refusal(true); // exec failed; never return through a parent destructor.
    }

    // Stable raw clone/zero edge for independent generated-code inspection.
    [[gnu::noinline, gnu::no_stack_protector]] bool birth() noexcept {
        if (!still_working() || !checkpoint(PhaseBase::atomic_birth, PhaseEdge::before, -1))
            return false;
        scratch_.birth_pidfd = -1;
        scratch_.birth.flags = CLONE_PIDFD | CLONE_CLEAR_SIGHAND;
        scratch_.birth.pidfd = address(&scratch_.birth_pidfd);
        scratch_.birth.child_tid = 0;
        scratch_.birth.parent_tid = 0;
        scratch_.birth.exit_signal = SIGCHLD;
        scratch_.birth.stack = 0;
        scratch_.birth.stack_size = 0;
        scratch_.birth.tls = 0;
        scratch_.birth.set_tid = 0;
        scratch_.birth.set_tid_size = 0;
        scratch_.birth.cgroup = 0;
        const auto result = ops_.template syscall<KernelCall::clone3>(address(&scratch_.birth),
                                                                      sizeof(scratch_.birth));
        if (result == 0)
            child();
        if (result < 0) {
            // Even a changed failure output cell is NOT an installed owned FD.
            if (result == -EAGAIN)
                refuse(resource_limit); // Process-capacity EAGAIN is specific to birth.
            else
                syscall_failure(result, environment_unavailable);
            return false;
        }
        pid_ = result;
        descriptors_[pidfd] = scratch_.birth_pidfd;
        // Proven atomic kernel success commits both. Invalid successful output is
        // a fatal backend-contract defect, never permission to kill a guessed PID.
        if (result > INT_MAX || descriptors_[pidfd] < 0)
            child_exit(127);
        adopted(pidfd);
        return checkpoint(PhaseBase::atomic_birth, PhaseEdge::after, result);
    }

    bool read_channel(Slot slot, unsigned char *bytes, unsigned &used, unsigned capacity,
                      bool &eof) noexcept {
        while (!eof && used < capacity) {
            if (!still_working() || !checkpoint(PhaseBase::protocol_read, PhaseEdge::before, -1))
                return false;
            const auto result = ops_.template syscall<KernelCall::read>(
                word(descriptors_[slot]), address(bytes + used), capacity - used);
            if (result == -EAGAIN)
                return true;
            if (result < 0) {
                syscall_failure(result, validation_unavailable);
                return false;
            }
            if (result == 0) {
                eof = true;
                return true;
            }
            used += static_cast<unsigned>(result);
            if (!checkpoint(PhaseBase::protocol_read, PhaseEdge::after, result))
                return false;
            if ((slot == activation_read && (used != 1 || bytes[0] != 'A')) ||
                (slot == outcome_read && used > 256)) {
                refuse(validation_unavailable);
                return false;
            }
        }
        return true;
    }
    void reset_status() noexcept {
        scratch_.status.si_pid = 0;
        scratch_.status.si_code = 0;
        scratch_.status.si_status = 0;
    }
    [[gnu::no_stack_protector]] void reaped() noexcept {
        if (scratch_.status.si_pid != pid_)
            child_exit(127); // Admitted P_PIDFD wait must identify its atomic birth.
        child_code_ = scratch_.status.si_code;
        child_status_ = scratch_.status.si_status;
        quiescent_ = true;
        ops_.observe({ObservationKind::release,
                      {PhaseBase::wait_reap, PhaseEdge::before},
                      -1,
                      pid_,
                      child_status_});
    }
    bool wait_work() noexcept {
        if (quiescent_)
            return true;
        if (!still_working() || !checkpoint(PhaseBase::wait_reap, PhaseEdge::before, -1))
            return false;
        reset_status();
        const auto result = ops_.template syscall<KernelCall::waitid>(
            pidfd_selector, word(descriptors_[pidfd]), address(&scratch_.status),
            WEXITED | WNOHANG | same_thread, 0);
        if (result < 0) {
            syscall_failure(result, validation_unavailable);
            return false;
        }
        if (scratch_.status.si_pid > 0)
            reaped();
        return checkpoint(PhaseBase::wait_reap, PhaseEdge::after, result);
    }
    bool matching_line(const WireOutcome &candidate) noexcept {
        if (candidate.bytes != outcome_bytes_)
            return false;
        const volatile unsigned char *actual = scratch_.outcome;
        for (unsigned index = 0; index != candidate.bytes; ++index)
            if (actual[index] != static_cast<unsigned char>(candidate.line[index]))
                return false;
        return true;
    }
    void decode() noexcept {
        if (child_code_ != CLD_EXITED) {
            refuse(validation_unavailable);
            return;
        }
        for (const auto &candidate : outcomes) {
            if (!matching_line(candidate) || child_status_ != candidate.status)
                continue;
            if ((activation_bytes_ == 1 && scratch_.activation[0] == 'A') ||
                (activation_bytes_ == 0 && candidate.value == environment_unavailable)) {
                decision_ = candidate.value;
                return;
            }
        }
        refuse(validation_unavailable);
    }
    [[gnu::no_stack_protector]] void receive() noexcept {
        while (decision_ < 0) {
            RawResult remaining;
            if (!still_working(&remaining))
                return;
            scratch_.watched[0].fd = activation_eof_ ? -1 : descriptors_[activation_read];
            scratch_.watched[1].fd = outcome_eof_ ? -1 : descriptors_[outcome_read];
            scratch_.watched[2].fd = quiescent_ ? -1 : descriptors_[pidfd];
            for (auto &item : scratch_.watched) {
                item.events = POLLIN;
                item.revents = 0;
            }
            if (!checkpoint(PhaseBase::protocol_poll, PhaseEdge::before, -1))
                return;
            const auto milliseconds = remaining / 1000000 + (remaining % 1000000 != 0);
            const auto result = ops_.template syscall<KernelCall::poll>(
                address(scratch_.watched), 3, static_cast<NativeWord>(milliseconds));
            if (result < 0) {
                syscall_failure(result, validation_unavailable);
                return;
            }
            if (result == 0)
                continue; // Recheck the original absolute deadline, never restart it.
            if (!checkpoint(PhaseBase::protocol_poll, PhaseEdge::after, result))
                return;
            for (const auto &item : scratch_.watched)
                if (item.revents & (POLLERR | POLLNVAL)) {
                    refuse(validation_unavailable);
                    return;
                }
            if ((scratch_.watched[0].revents & (POLLIN | POLLHUP)) &&
                !read_channel(activation_read, scratch_.activation, activation_bytes_, 2,
                              activation_eof_))
                return;
            if ((scratch_.watched[1].revents & (POLLIN | POLLHUP)) &&
                !read_channel(outcome_read, scratch_.outcome, outcome_bytes_, 257, outcome_eof_))
                return;
            if ((scratch_.watched[2].revents & (POLLIN | POLLHUP)) && !wait_work())
                return;
            if (quiescent_ && activation_eof_ && outcome_eof_) {
                decode();
                return;
            }
        }
    }
    void terminate() noexcept {
        for (;;) {
            checkpoint(PhaseBase::pidfd_kill, PhaseEdge::before, -1, false);
            const auto result = ops_.template syscall<KernelCall::pidfd_send_signal>(
                word(descriptors_[pidfd]), SIGKILL, 0, 0);
            if (result == -EINTR)
                continue;
            if (result == 0)
                checkpoint(PhaseBase::pidfd_kill, PhaseEdge::after, result, false);
            else if (result != -ESRCH)
                syscall_failure(result, validation_unavailable, false);
            return; // ESRCH is NOT a quiescence/reap proof; wait below is mandatory.
        }
    }
    bool pinned_exit(int timeout) noexcept {
        auto &watched = scratch_.watched[0];
        watched.fd = descriptors_[pidfd];
        watched.events = POLLIN;
        watched.revents = 0;
        const auto result =
            ops_.template syscall<KernelCall::poll>(address(&watched), 1, word(timeout));
        return result == 1 && (watched.revents & (POLLIN | POLLHUP)) &&
               !(watched.revents & (POLLERR | POLLNVAL));
    }
    void wait_cleanup() noexcept {
        while (!quiescent_) {
            reset_status();
            const auto result = ops_.template syscall<KernelCall::waitid>(
                pidfd_selector, word(descriptors_[pidfd]), address(&scratch_.status),
                WEXITED | same_thread, 0);
            if (result == -EINTR)
                continue;
            if (result == 0 && scratch_.status.si_pid > 0) {
                reaped();
                return;
            }
            syscall_failure(result, validation_unavailable, false);
            if (result == -ECHILD && pinned_exit(0)) {
                reset_status();
                const auto fresh = ops_.template syscall<KernelCall::waitid>(
                    pidfd_selector, word(descriptors_[pidfd]), address(&scratch_.status),
                    WEXITED | WNOHANG | WNOWAIT | same_thread, 0);
                if (fresh == -ECHILD) {
                    quiescent_ = true;
                    return;
                }
            }
            // A syscall/policy failure is not a clean-child certificate. Keep the
            // identity and wait; non-real-time/fatal kernel failures cannot return.
            pinned_exit(-1);
        }
    }
    [[gnu::no_stack_protector]] void cleanup() noexcept {
        close_bare(config);
        close_bare(temporary);
        close_bare(activation_write);
        close_bare(outcome_write);
        close_bare(null_source);
        if (descriptors_[pidfd] >= 0 && !quiescent_) {
            terminate();
            wait_cleanup();
        }
        // Keep readers open through termination to avoid manufacturing SIGPIPE.
        close_bare(activation_read);
        close_bare(outcome_read);
        if (descriptors_[pidfd] >= 0) {
            checkpoint(PhaseBase::pidfd_close, PhaseEdge::before, -1, false);
            const int descriptor = descriptors_[pidfd];
            const auto result = ops_.template syscall<KernelCall::close>(word(descriptor));
            descriptors_[pidfd] = -1;
            ops_.observe({ObservationKind::release, phase_, descriptor, pid_, result});
            if (result != 0)
                syscall_failure(result, validation_unavailable, false);
            checkpoint(PhaseBase::pidfd_close, PhaseEdge::after, result, false);
        }
    }

  public:
    Owner(Ops &ops, Scratch &scratch, RawResult start) noexcept
        : ops_(ops), scratch_(scratch), deadline_(start + work_budget), last_clock_(start) {}
    Owner(const Owner &) = delete;
    Owner &operator=(const Owner &) = delete;
    ~Owner() noexcept {
        if (!finished_)
            cleanup();
    }

    [[gnu::no_stack_protector]] int run() noexcept {
        scratch_.argv[0] = reinterpret_cast<const char *>(scratch_.paths[1]);
        scratch_.argv[1] = "-I";
        scratch_.argv[2] = "-S";
        scratch_.argv[3] = reinterpret_cast<const char *>(scratch_.paths[2]);
        scratch_.argv[4] = "--manifest";
        scratch_.argv[5] = reinterpret_cast<const char *>(scratch_.paths[0]);
        scratch_.argv[6] = "--sdk-dir";
        scratch_.argv[7] = reinterpret_cast<const char *>(scratch_.paths[3]);
        scratch_.argv[8] = "--activation-fd";
        scratch_.argv[9] = "3";
        scratch_.argv[10] = "--outcome-fd";
        scratch_.argv[11] = "4";
        scratch_.argv[12] = nullptr;
        scratch_.environment[0] = "PATH=/usr/bin:/bin";
        scratch_.environment[1] = "LC_ALL=C.UTF-8";
        scratch_.environment[2] = "LANG=C.UTF-8";
        scratch_.environment[3] = nullptr;
        volatile KernelAction &defaults = scratch_.defaults;
        defaults.handler = 0;
        defaults.flags = 0;
        defaults.restorer = 0;
        defaults.mask = 0;
        scratch_.empty_mask = 0;
        const auto profile = ops_.template syscall<KernelCall::rt_sigaction>(
            SIGCHLD, 0, address(&scratch_.sigchld), 8);
        if (profile != 0)
            syscall_failure(profile, environment_unavailable);
        else if (scratch_.sigchld.handler != 0 || (scratch_.sigchld.flags & SA_NOCLDWAIT))
            refuse(environment_unavailable);
        else if (!config_name())
            refuse(environment_unavailable);
        else if (read_config() && prepare_sources() && birth() &&
                 close_work(activation_write, PhaseBase::parent_write_close) &&
                 close_work(outcome_write, PhaseBase::parent_write_close) &&
                 close_work(null_source, PhaseBase::parent_write_close))
            receive();
        if (decision_ < 0)
            refuse(validation_unavailable);
        cleanup();
        checkpoint(PhaseBase::final_commit, PhaseEdge::before, -1, false);
        const auto final_clock = ops_.monotonic_ns();
        if (final_clock < 0)
            syscall_failure(final_clock,
                            had_resource_ ? validation_unavailable : environment_unavailable,
                            false);
        else {
            if (final_clock < last_clock_)
                refuse(had_resource_ ? validation_unavailable : environment_unavailable);
            else if (final_clock >= deadline_)
                refuse(resource_limit);
            ops_.observe({ObservationKind::clock,
                          {PhaseBase::final_commit, PhaseEdge::after},
                          -1,
                          pid_,
                          final_clock});
        }
        checkpoint(PhaseBase::final_commit, PhaseEdge::after, final_clock, false);
        finished_ = true;
        ops_.observe({ObservationKind::final_commit,
                      {PhaseBase::final_commit, PhaseEdge::after},
                      -1,
                      pid_,
                      decision_});
        return decision_;
    }
};

} // namespace subathon::schema_validation::native::detail

namespace subathon::schema_validation::native {
template <TransportOps Ops>
[[gnu::no_stack_protector]] int run_transport_v1(const Request &request, Ops &ops) noexcept {
    // Foreign ABI shape refusals precede any clock/resource/owner observation.
    if (!detail::shape(request.manifest))
        return detail::invalid_path;
    const auto start = ops.monotonic_ns();
    detail::Scratch scratch;
    if (!detail::copy_path(request.manifest, scratch.paths[0]) ||
        detail::uri_scheme(scratch.paths[0], request.manifest.bytes))
        return detail::invalid_path;
    if (!detail::copy_path(request.interpreter, scratch.paths[1]) || scratch.paths[1][0] != '/' ||
        !detail::copy_path(request.worker, scratch.paths[2]) || scratch.paths[2][0] != '/' ||
        !detail::copy_path(request.sdk, scratch.paths[3]) || scratch.paths[3][0] != '/')
        return detail::environment_unavailable;
    if (start == -ENOMEM || start == -EMFILE || start == -ENFILE)
        return detail::resource_limit;
    if (start < 0 || start > INT64_MAX - detail::work_budget)
        return detail::environment_unavailable;
    detail::Owner<Ops> owner(ops, scratch, start);
    return owner.run();
}
} // namespace subathon::schema_validation::native

#endif // SUBATHON_SCHEMA_VALIDATION_NATIVE_TRANSPORT_CORE_V1_IMPL_H
