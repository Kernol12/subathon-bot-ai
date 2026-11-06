// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SUBATHON_SCHEMA_VALIDATION_TEST_NATIVE_FAULT_OPS_H
#define SUBATHON_SCHEMA_VALIDATION_TEST_NATIVE_FAULT_OPS_H

#include "native_fault_state.h"
#include "native_foundation_child.h"
#include <linux/sched.h>
#include <sys/wait.h>

// The root build binds ONE reviewed negative executable, never a package/env path.
#ifndef SUBATHON_NATIVE_FAULT_CHILD_PATH
#error "Bind the reviewed native_fault_child absolute target path for the fault artifact"
#endif
static_assert(sizeof(SUBATHON_NATIVE_FAULT_CHILD_PATH) <= 4097 &&
              SUBATHON_NATIVE_FAULT_CHILD_PATH[0] == '/');

namespace subathon::schema_validation::native::fault {

// Test-only syscall policy. This contains no path admission, owner, protocol decoder,
// launch/read/poll/wait loop or ABI trampoline. The common algorithm stays absent.
class [[gnu::visibility("hidden")]] FaultOps {
    struct Handle {
        int descriptor = -1;
        bool live = false, adopted = false, released = false;
        bool outcome_read = false, normalized = false;
        RawResult close_result = -1;
    };

    const Request request_;
    const Plan plan_;
    const int parent_pid_;
    Handle handles_[12];
    unsigned count_ = 0, adopt_pending_ = 0, release_pending_ = 0;
    unsigned clock_records_ = 1;
    int pid_ = -1, pidfd_ = -1, capture_fd_ = -1;
    int reaped_pid_ = -1, reaped_status_ = -1;
    bool child_ = false, failed_ = false, triggered_ = false, before_pending_ = false;
    bool deferred_failure_ = false;
    bool selected_retry_ = false, reap_pending_ = false;
    bool signal_entered_ = false, signal_finished_ = false, sources_closed_ = false;
    bool exec_entered_ = false, final_observed_ = false;
    bool destinations_[5] = {false, false, false, false, false};
    Phase phase_{PhaseBase::config_open, PhaseEdge::before};
    KernelCall last_call_ = KernelCall::getpid;
    NativeWord last_operation_ = 0;
    RawResult last_result_ = -1, last_clock_ = -1;
    char parent_text_[11]{};
    char mode_text_[3]{};
    const char *fixed_argv_[6]{};

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

    [[gnu::no_stack_protector]] Writer writer() const noexcept {
        return child_ ? Writer::child : Writer::parent;
    }
    [[gnu::no_stack_protector]] void emit(ObservationKind kind, int descriptor, RawResult result,
                                          int pid = -1) noexcept {
        if (!record(writer(), {kind, phase_, descriptor,
                               pid < 0 ? (pid_ > 0 ? pid_ : parent_pid_) : pid, result}))
            failed_ = true;
    }
    [[gnu::no_stack_protector]] void fail() noexcept {
        if (!failed_) {
            failed_ = true;
            // Persistent shared poison makes an unrelated refusal fail the fixture.
            record(writer(), {ObservationKind::poison, phase_, -1, pid_, -1});
        }
    }
    [[gnu::no_stack_protector]] Handle *live(int descriptor) noexcept {
        for (unsigned index = count_; index != 0; --index)
            if (handles_[index - 1].live && handles_[index - 1].descriptor == descriptor)
                return &handles_[index - 1];
        return nullptr;
    }
    [[gnu::no_stack_protector]] void acquired(RawResult result, bool outcome = false,
                                              bool normalized = false) noexcept {
        // Before return to the owner this is ONLY nonfallible scalar cache work.
        // Journal clocks/controller hooks wait for its immediate slot-adopt ACK.
        if (result < 0 || result > INT_MAX || count_ == 12 || live(static_cast<int>(result))) {
            deferred_failure_ = true;
            return;
        }
        auto &entry = handles_[count_++];
        entry.descriptor = static_cast<int>(result);
        entry.live = true;
        entry.outcome_read = outcome;
        entry.normalized = normalized;
        ++adopt_pending_;
    }
    [[gnu::no_stack_protector]] void closed(int descriptor, RawResult result) noexcept {
        auto *entry = live(descriptor);
        if (!entry || result == -EBADF) {
            deferred_failure_ = true;
            return;
        }
        // Linux releases this number even on EINTR/EIO. Never retry/relabel it.
        entry->live = false;
        entry->close_result = result;
        ++release_pending_;
    }
    [[gnu::no_stack_protector]] static bool target(PhaseBase base, KernelCall call,
                                                   NativeWord operation) noexcept {
        switch (base) {
        case PhaseBase::config_open:
        case PhaseBase::null_open:
            return call == KernelCall::openat;
        case PhaseBase::config_read:
        case PhaseBase::protocol_read:
            return call == KernelCall::read;
        case PhaseBase::config_close:
        case PhaseBase::parent_write_close:
        case PhaseBase::pidfd_close:
            return call == KernelCall::close;
        case PhaseBase::activation_pipe:
        case PhaseBase::outcome_pipe:
            return call == KernelCall::pipe2;
        case PhaseBase::source_duplicate:
            return call == KernelCall::fcntl && operation == F_DUPFD_CLOEXEC;
        case PhaseBase::atomic_birth:
            return call == KernelCall::clone3;
        case PhaseBase::child_signal:
            return call == KernelCall::rt_sigaction || call == KernelCall::rt_sigprocmask;
        case PhaseBase::child_dup:
            return call == KernelCall::dup3;
        case PhaseBase::child_close_range:
            return call == KernelCall::close_range;
        case PhaseBase::child_exec:
            return call == KernelCall::execve;
        case PhaseBase::protocol_poll:
            return call == KernelCall::poll;
        case PhaseBase::wait_reap:
            return call == KernelCall::waitid;
        case PhaseBase::pidfd_kill:
            return call == KernelCall::pidfd_send_signal;
        case PhaseBase::final_commit:
            return false;
        }
        return false;
    }

    [[gnu::no_stack_protector]] void entered_child() noexcept {
        child_ = true;
        pid_ = static_cast<int>(schema_foundation::raw(SYS_getpid));
        pidfd_ = -1;
        adopt_pending_ = release_pending_ = 0;
        unsigned kept = 0;
        phase_ = {PhaseBase::child_signal, PhaseEdge::before};
        // The common owner must have normalized exactly five live sources >=5.
        for (unsigned index = 0; index != count_; ++index) {
            if (!handles_[index].live)
                continue;
            if (handles_[index].descriptor < 5 || kept == 5) {
                fail();
                continue;
            }
            // Fixed scalar copies only; no child container/copy helper.
            const int descriptor = handles_[index].descriptor;
            handles_[kept].descriptor = descriptor;
            handles_[kept].live = true;
            handles_[kept].adopted = true;
            handles_[kept].released = false;
            handles_[kept].outcome_read = handles_[index].outcome_read;
            handles_[kept].normalized = handles_[index].normalized;
            handles_[kept].close_result = -1;
            ++kept;
            emit(ObservationKind::adopt, descriptor, descriptor);
        }
        count_ = kept;
        if (kept != 5)
            fail();
    }
    [[gnu::no_stack_protector]] bool signals_are_default() noexcept {
        schema_foundation::KernelAction action;
        constexpr int signals[] = {SIGINT, SIGPIPE, SIGCHLD, SIGTERM};
        for (int signal : signals)
            if (schema_foundation::raw(SYS_rt_sigaction, signal, 0,
                                       schema_foundation::address(&action), 8) != 0 ||
                action.handler != 0)
                return false;
        std::uint64_t mask = UINT64_MAX;
        return schema_foundation::raw(SYS_rt_sigprocmask, SIG_SETMASK, 0,
                                      schema_foundation::address(&mask), 8) == 0 &&
               mask == 0;
    }
    [[gnu::no_stack_protector]] static void decimal(unsigned value, char *bytes,
                                                    unsigned capacity) noexcept {
        unsigned digits = 1, scale = 1;
        for (unsigned copy = value; copy >= 10; copy /= 10) {
            ++digits;
            scale *= 10;
        }
        if (digits >= capacity) {
            bytes[0] = 0;
            return;
        }
        for (unsigned index = 0; index != digits; ++index) {
            bytes[index] = static_cast<char>('0' + value / scale);
            value %= scale;
            scale /= 10;
        }
        bytes[digits] = 0;
    }
    template <std::size_t Count>
    [[gnu::no_stack_protector]] static bool literal(const char *value,
                                                    const char (&expected)[Count]) noexcept {
        if (!value)
            return false;
        const volatile char *actual = value;
        for (std::size_t index = 0; index != Count; ++index)
            if (actual[index] != expected[index])
                return false;
        return true;
    }
    [[gnu::no_stack_protector]] static bool path(const char *value, PathView expected) noexcept {
        if (!value || !expected.data || expected.bytes == 0 || expected.bytes > 4096)
            return false;
        const volatile unsigned char *actual =
            reinterpret_cast<const volatile unsigned char *>(value);
        const volatile unsigned char *original = expected.data;
        for (unsigned index = 0; index != expected.bytes; ++index)
            if (actual[index] != original[index])
                return false;
        return actual[expected.bytes] == 0;
    }
    [[gnu::no_stack_protector]] bool normal_exec(NativeWord file, NativeWord arguments,
                                                 NativeWord environment) noexcept {
        const auto *argv = reinterpret_cast<const char *const *>(arguments);
        const auto *envp = reinterpret_cast<const char *const *>(environment);
        return child_ && argv && envp &&
               path(reinterpret_cast<const char *>(file), request_.interpreter) &&
               path(argv[0], request_.interpreter) && literal(argv[1], "-I") &&
               literal(argv[2], "-S") && path(argv[3], request_.worker) &&
               literal(argv[4], "--manifest") && path(argv[5], request_.manifest) &&
               literal(argv[6], "--sdk-dir") && path(argv[7], request_.sdk) &&
               literal(argv[8], "--activation-fd") && literal(argv[9], "3") &&
               literal(argv[10], "--outcome-fd") && literal(argv[11], "4") && !argv[12] &&
               literal(envp[0], "PATH=/usr/bin:/bin") && literal(envp[1], "LC_ALL=C.UTF-8") &&
               literal(envp[2], "LANG=C.UTF-8") && !envp[3];
    }
    [[gnu::no_stack_protector]] unsigned negative_mode() const noexcept {
        if (plan_.case_id >= 1 && plan_.case_id <= 22)
            return plan_.case_id;
        if (plan_.case_id == 28)
            return 28;
        if (plan_.case_id == 24 && (plan_.phase_id == 34 || plan_.phase_id == 35))
            return 24;
        return 0; // In particular late work/C3 and case27 use the REAL worker.
    }
    [[gnu::no_stack_protector]] RawResult exec(NativeWord file, NativeWord arguments,
                                               NativeWord environment) noexcept {
        if (exec_entered_ || !normal_exec(file, arguments, environment)) {
            fail();
            schema_foundation::finish(125);
        }
        exec_entered_ = true;
        for (unsigned descriptor = 0; descriptor != 5; ++descriptor) {
            const long flags = schema_foundation::raw(SYS_fcntl, descriptor, F_GETFD);
            if (!destinations_[descriptor] || flags < 0 || (flags & FD_CLOEXEC))
                fail();
            emit(ObservationKind::child_exec_handoff_intent, static_cast<int>(descriptor), flags);
        }
        if (failed_)
            schema_foundation::finish(125);
        const unsigned mode = negative_mode();
        if (mode == 0)
            return schema_foundation::raw(SYS_execve, static_cast<long>(file),
                                          static_cast<long>(arguments),
                                          static_cast<long>(environment));
        // All strings were prepared BEFORE birth. Only this closed negative map
        // changes exec; no runtime executable/validation factory is admitted.
        return schema_foundation::raw(
            SYS_execve, schema_foundation::address(SUBATHON_NATIVE_FAULT_CHILD_PATH),
            schema_foundation::address(fixed_argv_), static_cast<long>(environment));
    }

  public:
    FaultOps(const FaultOps &) = delete;
    FaultOps &operator=(const FaultOps &) = delete;
    FaultOps(FaultOps &&) = delete;
    FaultOps &operator=(FaultOps &&) = delete;

    explicit FaultOps(const Request &request) noexcept
        : request_(request), plan_(selected_plan()),
          parent_pid_(static_cast<int>(schema_foundation::raw(SYS_getpid))) {
        decimal(static_cast<unsigned>(parent_pid_), parent_text_, sizeof(parent_text_));
        decimal(negative_mode(), mode_text_, sizeof(mode_text_));
        fixed_argv_[0] = SUBATHON_NATIVE_FAULT_CHILD_PATH;
        fixed_argv_[1] = "--case";
        fixed_argv_[2] = mode_text_;
        fixed_argv_[3] = "--parent";
        fixed_argv_[4] = parent_text_;
        fixed_argv_[5] = nullptr;
        if (plan_.case_id == 0 || parent_pid_ <= 0)
            fail();
    }

    template <KernelCall Call>
    [[gnu::no_stack_protector]] RawResult syscall(NativeWord a = 0, NativeWord b = 0,
                                                  NativeWord c = 0, NativeWord d = 0,
                                                  NativeWord e = 0, NativeWord f = 0) noexcept {
        if (before_pending_) {
            before_pending_ = false;
            if (!target(phase_.base, Call, b)) {
                fail();
                return -EPROTO;
            }
            selected_retry_ = phase_.base == PhaseBase::pidfd_kill;
            return -static_cast<RawResult>(plan_.fault_errno); // No kernel return is fabricated.
        }
        RawResult result;
        if constexpr (Call == KernelCall::execve)
            result = exec(a, b, c);
        else
            result = schema_foundation::raw(
                number<Call>(), static_cast<long>(a), static_cast<long>(b), static_cast<long>(c),
                static_cast<long>(d), static_cast<long>(e), static_cast<long>(f));
        last_call_ = Call;
        last_operation_ = b;
        last_result_ = result;

        if constexpr (Call == KernelCall::clone3) {
            if (result == 0) {
                entered_child();
            } else if (result > 0) {
                const auto *args = reinterpret_cast<const clone_args *>(a);
                // Read the output ONLY on genuine successful parent birth.
                const auto *output = reinterpret_cast<const int *>(args->pidfd);
                pid_ = static_cast<int>(result);
                pidfd_ = *output;
                acquired(pidfd_);
            }
        } else if constexpr (Call == KernelCall::pipe2) {
            if (result == 0) {
                const auto *ends = reinterpret_cast<const int *>(a);
                acquired(ends[0], phase_.base == PhaseBase::outcome_pipe);
                acquired(ends[1]);
            }
        } else if constexpr (Call == KernelCall::openat) {
            if (result >= 0)
                acquired(result);
        } else if constexpr (Call == KernelCall::fcntl) {
            if (b == F_DUPFD_CLOEXEC && result >= 0) {
                const auto *source = live(static_cast<int>(a));
                if (!source)
                    deferred_failure_ = true;
                acquired(result, source && source->outcome_read, true);
            }
        } else if constexpr (Call == KernelCall::close) {
            if (!child_)
                closed(static_cast<int>(a), result);
        } else if constexpr (Call == KernelCall::dup3) {
            if (!child_ || result < 0 || result > 4 || result != static_cast<RawResult>(b)) {
                if (result >= 0)
                    fail();
            } else {
                auto &destination = destinations_[static_cast<unsigned>(result)];
                if (destination)
                    fail();
                else {
                    destination = true;
                    emit(ObservationKind::syscall_return, static_cast<int>(result), result);
                }
            }
        } else if constexpr (Call == KernelCall::close_range) {
            if (result == 0 && child_) {
                if (a != 5 || b != UINT_MAX || c != 0 || sources_closed_)
                    fail();
                if (!sources_closed_) {
                    sources_closed_ = true;
                    for (unsigned index = 0; index != count_; ++index) {
                        auto &entry = handles_[index];
                        const long flags =
                            schema_foundation::raw(SYS_fcntl, entry.descriptor, F_GETFD);
                        if (flags != -EBADF)
                            fail();
                        entry.live = false;
                        emit(ObservationKind::release, entry.descriptor, flags);
                    }
                }
            }
        } else if constexpr (Call == KernelCall::read) {
            if (result > 0 && !child_ && plan_.case_id == 28) {
                const auto *source = live(static_cast<int>(a));
                if (source && source->outcome_read &&
                    !capture_outcome(reinterpret_cast<const unsigned char *>(b),
                                     static_cast<std::size_t>(result)))
                    fail();
            }
        } else if constexpr (Call == KernelCall::waitid) {
            if (result == 0 && c != 0) {
                const auto *info = reinterpret_cast<const siginfo_t *>(c);
                if (info->si_pid > 0) {
                    if (info->si_code == CLD_EXITED && info->si_status == 125)
                        fail(); // Fixed-child failure must NEVER become an ordinary17 pass.
                    if (!(d & WNOWAIT)) {
                        reaped_pid_ = info->si_pid;
                        reaped_status_ = info->si_status;
                        reap_pending_ = true;
                        const Phase saved = phase_;
                        phase_ = {PhaseBase::wait_reap, PhaseEdge::before};
                        emit(ObservationKind::release, -1, info->si_status, info->si_pid);
                        phase_ = saved;
                    }
                }
            }
        }
        if (selected_retry_ && Call == KernelCall::pidfd_send_signal && result == 0) {
            selected_retry_ = false;
            const Phase saved = phase_;
            phase_ = {PhaseBase::pidfd_kill, PhaseEdge::after};
            emit(ObservationKind::syscall_return, static_cast<int>(a), result);
            phase_ = saved;
        }
        if constexpr (Call == KernelCall::execve) {
            emit(ObservationKind::syscall_return, -1,
                 result); // Real exec failed; no handoff proof.
            // Real prerequisites must work; BEFORE injection bypasses this path.
            fail();
        }
        return result; // EVERY real return stays intact, including acquisition and close.
    }

    [[gnu::no_stack_protector]] RawResult monotonic_ns() noexcept {
        last_clock_ = fault::monotonic_ns();
        return last_clock_;
    }

    [[gnu::no_stack_protector]] Continuation checkpoint(Phase phase, RawResult actual) noexcept {
        phase_ = phase;
        const unsigned base = static_cast<unsigned>(phase.base);
        const unsigned edge = static_cast<unsigned>(phase.edge);
        const unsigned encoded = 2 * base + edge;
        if (base < 1 || base > 19 || edge > 1 || before_pending_ || deferred_failure_ ||
            (!child_ && (adopt_pending_ || release_pending_))) {
            fail();
            return Continuation::transport_refusal;
        }
        if (child_ && phase.base == PhaseBase::child_signal) {
            const bool valid = phase.edge == PhaseEdge::before
                                   ? !signal_entered_
                                   : signal_entered_ && !signal_finished_;
            if (!valid)
                fail();
            else {
                if (phase.edge == PhaseEdge::before)
                    signal_entered_ = true;
                else
                    signal_finished_ = true;
                emit(ObservationKind::signal, -1, phase.edge == PhaseEdge::before ? -1 : actual);
            }
            if (phase.edge == PhaseEdge::after && actual == 0 &&
                (plan_.case_id == 23 || plan_.case_id == 25 || plan_.case_id == 26)) {
                if (!signals_are_default() || !child_ready_and_hold())
                    fail();
            }
        }
        const bool selected = !triggered_ && ((plan_.phase_id != 0 && encoded == plan_.phase_id) ||
                                              (plan_.case_id == 27 && encoded == 32));
        if (!selected)
            return failed_ ? Continuation::transport_refusal : Continuation::proceed;
        // AFTER is usable only after the genuine successful operation, with all
        // owner-slot acknowledgements already complete. No EAGAIN/idle success.
        if (phase.edge == PhaseEdge::after &&
            (actual < 0 ||
             ((phase.base == PhaseBase::protocol_read || phase.base == PhaseBase::protocol_poll) &&
              actual == 0) ||
             actual != last_result_ || !target(phase.base, last_call_, last_operation_))) {
            fail();
            return Continuation::transport_refusal;
        }
        if (!fault::checkpoint(phase, actual)) {
            fail();
            return Continuation::transport_refusal;
        }
        triggered_ = true;
        if (plan_.case_id == 25)
            return Continuation::interrupted; // Controller delivered the REAL tgkill SIGINT.
        if (plan_.case_id == 26)
            return Continuation::resource_refusal;
        if (plan_.case_id != 24)
            return Continuation::proceed; // Competing reap is observed by the next REAL waitid.
        if (phase.edge == PhaseEdge::before) {
            before_pending_ = true;
            return Continuation::proceed;
        }
        if (plan_.fault_errno == ENOMEM)
            return Continuation::resource_refusal;
        if (plan_.fault_errno == EINTR)
            return Continuation::interrupted;
        return Continuation::transport_refusal; // EIO AFTER still preserves actual real success.
    }

    // Owner resource ACKs verify cached RAW facts before those facts are emitted.
    // They never supply identities; selected clocks/final decisions are observed.
    [[gnu::no_stack_protector]] void observe(const Observation &value) noexcept {
        if (deferred_failure_)
            fail();
        if (child_) {
            fail();
            return;
        }
        if (value.kind == ObservationKind::adopt && value.descriptor >= 0 &&
            value.descriptor <= INT_MAX) {
            auto *entry = live(static_cast<int>(value.descriptor));
            if (!entry || entry->adopted || value.result != entry->descriptor ||
                (entry->descriptor == pidfd_ && value.pid != pid_)) {
                fail();
                return;
            }
            entry->adopted = true;
            --adopt_pending_;
            emit(ObservationKind::adopt, entry->descriptor, entry->descriptor);
            // Raw pipe-end ACK precedes its fallible F_SETFL. Bind ONLY the
            // normalized read duplicate, after the already-owned original has
            // become nonblocking. Write ends remain blocking from pipe2 onward.
            if (entry->outcome_read && entry->normalized && plan_.case_id == 28 &&
                capture_fd_ < 0) {
                capture_fd_ = entry->descriptor;
                if (!pin_report_read(capture_fd_))
                    fail();
            }
            if (entry->descriptor == pidfd_ && !pin_birth(pid_, pidfd_))
                fail();
            return;
        }
        if (value.kind == ObservationKind::release && value.descriptor >= 0) {
            for (unsigned index = count_; index != 0; --index) {
                auto &entry = handles_[index - 1];
                if (entry.descriptor == value.descriptor && !entry.live && !entry.released &&
                    entry.close_result == value.result) {
                    entry.released = true;
                    --release_pending_;
                    emit(ObservationKind::release, entry.descriptor, entry.close_result);
                    return;
                }
            }
            fail();
            return;
        }
        if (value.kind == ObservationKind::release && value.descriptor == -1 && reap_pending_ &&
            value.pid == reaped_pid_ && value.result == reaped_status_) {
            reap_pending_ = false;
            return;
        }
        if (value.kind == ObservationKind::clock && value.result == last_clock_ &&
            clock_records_ < 4) {
            ++clock_records_;
            if (!record(Writer::parent, value))
                failed_ = true;
            return;
        }
        if (value.kind == ObservationKind::final_commit) {
            if (final_observed_)
                fail();
            final_observed_ = true;
            for (unsigned index = 0; index != count_; ++index)
                if (handles_[index].live || !handles_[index].adopted || !handles_[index].released)
                    fail();
            // Child trigger state is private after clone: independent tests must
            // inspect the child's committed shared journal, not this parent copy.
            const bool child_phase =
                plan_.case_id == 24 && plan_.phase_id >= 20 && plan_.phase_id <= 27;
            if (before_pending_ || selected_retry_ || adopt_pending_ || release_pending_ ||
                reap_pending_ ||
                ((plan_.phase_id != 0 || plan_.case_id == 27) && !child_phase && !triggered_) ||
                value.result < 0 || value.result > 18)
                fail();
            // This is the actual owner decision after cleanup, NEVER a factory.
            if (!record(Writer::parent, value))
                failed_ = true;
            return;
        }
        fail();
    }
};

static_assert(TransportOps<FaultOps>);
static_assert(sizeof(FaultOps) <= 2048);

} // namespace subathon::schema_validation::native::fault

#endif // SUBATHON_SCHEMA_VALIDATION_TEST_NATIVE_FAULT_OPS_H
