// SPDX-License-Identifier: GPL-3.0-or-later
// Nonvalidating fixture controls only. Both DSO owner entries remain literal31.
#include "native_fault_state.h"
#include "native_fault_v1.h"
#include "native_foundation_child.h"
#include <climits>
#include <cstdint>
#include <linux/futex.h>
#include <new>
#include <poll.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>

namespace {
using namespace subathon::schema_validation::native;
using schema_foundation::address;
using schema_foundation::raw;
using subathon::schema_validation::native::fault::Plan;
using subathon::schema_validation::native::fault::Writer;

constexpr std::size_t map_bytes = 12288;
constexpr unsigned capacities[3] = {64, 32, 32};
constexpr unsigned beginnings[3] = {0, 64, 96};
constexpr int own_thread = 0x20000000;
constexpr long pidfd_selector = 3;
constexpr std::int64_t second = 1000000000;
// A fixture watchdog is separate from, and longer than, the owner's 10s work cap.
constexpr std::int64_t control_budget = 15 * second;
constexpr int reserved_floor = 5;
constexpr unsigned guard_slot = 4, capture_slot = 5;

struct Shared {
    std::int64_t entries[128][8];
    std::uint32_t counts[3];
    std::uint32_t poison, sequence, stop, birth_request, birth_bound, capture_request,
        capture_bound;
    std::uint32_t child_ready, phase_requested, action_done, trigger_claimed;
    std::uint32_t watchdog_fired, controller_error;
    Plan plan;
    int descriptors[6];
    int core_descriptors[12];
    unsigned core_count;
    int parent_pid, parent_tid, controller_tid, birth_pid, birth_source, capture_source;
    std::int64_t epoch_ns, offset_ns;
    unsigned char capture[257];
    unsigned capture_count;
    bool capture_eof;
};
static_assert(sizeof(Shared) <= map_bytes);
static_assert(offsetof(Shared, counts) == 8192);
static_assert(__atomic_always_lock_free(4, nullptr));
static_assert(__atomic_always_lock_free(8, nullptr));

Shared *shared = nullptr;
bool ever_armed = false, controller_created = false;
pthread_t controller_handle{};
int last_disposal = 0;

struct ErrnoPlan {
    unsigned phase;
    int error;
};
constexpr ErrnoPlan selected_errors[] = {
    {2, EACCES},  {8, ENOSYS},  {10, ENFILE}, {12, EMFILE}, {14, EACCES}, {16, ENOSYS},
    {16, EAGAIN}, {16, EPERM},  {4, EINTR},   {7, EINTR},   {9, ENOMEM},  {11, ENOMEM},
    {13, ENOMEM}, {15, ENOMEM}, {17, EINTR},  {19, EIO},    {20, EPERM},  {22, ENOSYS},
    {24, ENOSYS}, {26, EACCES}, {28, EIO},    {29, EINTR},  {30, EIO},    {31, EINTR},
    {32, EIO},    {32, EINTR},  {34, EINTR},  {35, EIO},    {37, EIO},
};

template <class T> [[gnu::no_stack_protector]] T load(const T *value) noexcept {
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}
template <class T> [[gnu::no_stack_protector]] void store(T *value, T next) noexcept {
    __atomic_store_n(value, next, __ATOMIC_RELEASE);
}
[[gnu::no_stack_protector]] Shared *state() noexcept { return load(&shared); }
[[gnu::no_stack_protector]] void wake(Shared *value) noexcept {
    __atomic_add_fetch(&value->sequence, 1U, __ATOMIC_RELEASE);
    raw(SYS_futex, address(&value->sequence), FUTEX_WAKE, INT_MAX);
}
[[gnu::no_stack_protector]] void poison(Shared *value) noexcept {
    store(&value->poison, 1U);
    wake(value);
}
[[gnu::no_stack_protector]] std::int64_t real_clock() noexcept {
    timespec now;
    const long result = raw(SYS_clock_gettime, CLOCK_MONOTONIC, address(&now));
    if (result < 0)
        return result;
    if (now.tv_sec < 0 || now.tv_nsec < 0 || now.tv_nsec >= second ||
        now.tv_sec > (INT64_MAX - now.tv_nsec) / second)
        return -EOVERFLOW;
    return now.tv_sec * second + now.tv_nsec;
}
[[gnu::no_stack_protector]] unsigned encoded(Phase phase) noexcept {
    const unsigned base = static_cast<unsigned>(phase.base);
    const unsigned edge = static_cast<unsigned>(phase.edge);
    return base >= 1 && base <= 19 && edge <= 1 ? 2U * base + edge : 0;
}
[[gnu::no_stack_protector]] bool append(Shared *value, Writer writer, ObservationKind kind,
                                        unsigned phase, std::int64_t descriptor, std::int64_t pid,
                                        std::int64_t result) noexcept {
    const unsigned who = static_cast<unsigned>(writer);
    if (who > 2) {
        poison(value);
        return false;
    }
    const unsigned count = load(&value->counts[who]);
    if (count >= capacities[who]) {
        poison(value);
        return false;
    }
    const auto now = real_clock();
    if (now < 0) {
        poison(value);
        return false;
    }
    auto &entry = value->entries[beginnings[who] + count];
    entry[0] = (static_cast<std::int64_t>(who) << 32) | count;
    entry[1] = static_cast<unsigned>(kind);
    entry[2] = phase;
    entry[3] = descriptor;
    entry[4] = pid;
    entry[5] = result;
    entry[6] = kind == ObservationKind::syscall_return ? (result < 0 && result >= -4095 ? -result
                                                          : result >= 0                 ? 0
                                                                                        : -1)
                                                       : -1;
    entry[7] = now;
    store(&value->counts[who], count + 1);
    return true;
}

bool valid_plan(Plan plan) noexcept {
    if (plan.case_id >= 1 && plan.case_id <= 23)
        return plan.phase_id == 0 && plan.fault_errno == 0;
    if (plan.case_id == 24) {
        for (const auto &item : selected_errors)
            if (plan.phase_id == item.phase && plan.fault_errno == item.error)
                return true;
        return false;
    }
    if (plan.fault_errno != 0)
        return false;
    if (plan.case_id == 25)
        return plan.phase_id == 18 || plan.phase_id == 19;
    if (plan.case_id == 26)
        return plan.phase_id == 17;
    return (plan.case_id == 27 || plan.case_id == 28) && plan.phase_id == 0;
}

[[gnu::no_stack_protector]] long wait_change(Shared *value, std::uint32_t observed,
                                             std::int64_t until) noexcept {
    const timespec deadline{until / second, until % second};
    return raw(SYS_futex, address(&value->sequence), FUTEX_WAIT_BITSET, observed,
               address(&deadline), 0, FUTEX_BITSET_MATCH_ANY);
}
[[gnu::no_stack_protector]] bool await_flag(Shared *value, const std::uint32_t *flag) noexcept {
    const auto start = real_clock();
    if (start < 0 || start > INT64_MAX - control_budget) {
        poison(value);
        return false;
    }
    for (;;) {
        const auto sequence = load(&value->sequence);
        if (load(flag))
            return !load(&value->poison);
        if (load(&value->stop) || load(&value->poison))
            return false;
        const long result = wait_change(value, sequence, start + control_budget);
        if (result != 0 && result != -EINTR && result != -EAGAIN) {
            poison(value);
            return false;
        }
    }
}

long wait_pinned(int descriptor, siginfo_t &info, int options) noexcept {
    long result;
    do {
        info = {};
        result = raw(SYS_waitid, pidfd_selector, descriptor, address(&info), options, 0);
    } while (result == -EINTR);
    return result;
}
bool pinned_exit(int descriptor) noexcept {
    pollfd watched{descriptor, POLLIN, 0};
    long result;
    do {
        result = raw(SYS_poll, address(&watched), 1, 0);
    } while (result == -EINTR);
    return result == 1 && (watched.revents & (POLLIN | POLLHUP)) &&
           !(watched.revents & (POLLERR | POLLNVAL));
}
bool pinned_reaped(Shared *value) noexcept {
    if (!load(&value->birth_bound) || !pinned_exit(value->descriptors[guard_slot]) ||
        raw(SYS_gettid) != value->parent_tid)
        return false;
    siginfo_t info;
    return wait_pinned(value->descriptors[guard_slot], info,
                       WEXITED | WNOHANG | WNOWAIT | own_thread) == -ECHILD;
}
bool bind_reserved(Shared *value, int source, unsigned slot) noexcept {
    long result;
    do {
        result = raw(SYS_dup3, source, value->descriptors[slot], O_CLOEXEC);
    } while (result == -EINTR);
    append(value, Writer::controller, ObservationKind::syscall_return, 0, value->descriptors[slot],
           value->birth_pid, result);
    if (result != value->descriptors[slot]) {
        poison(value);
        return false;
    }
    return true;
}
void watchdog(Shared *value) noexcept {
    store(&value->watchdog_fired, 1U);
    poison(value);
    if (load(&value->birth_bound)) {
        long result;
        do {
            result = raw(SYS_pidfd_send_signal, value->descriptors[guard_slot], SIGKILL, 0, 0);
        } while (result == -EINTR);
        append(value, Writer::controller, ObservationKind::signal, 0,
               value->descriptors[guard_slot], value->birth_pid, result);
    }
    // A watchdog NEVER waits/reaps. The later before-fallback verdict already fails.
}

void *controller(void *argument) noexcept {
    auto *value = static_cast<Shared *>(argument);
    value->controller_tid = static_cast<int>(raw(SYS_gettid));
    const auto start = real_clock();
    if (start < 0 || start > INT64_MAX - control_budget) {
        store(&value->controller_error, 1U);
        poison(value);
        return nullptr;
    }
    for (;;) {
        const auto sequence = load(&value->sequence);
        if (load(&value->stop))
            return nullptr;
        if (load(&value->birth_request) && !load(&value->birth_bound)) {
            if (!bind_reserved(value, value->birth_source, guard_slot))
                return nullptr;
            store(&value->birth_bound, 1U);
            wake(value);
        }
        if (load(&value->capture_request) && !load(&value->capture_bound)) {
            if (!bind_reserved(value, value->capture_source, capture_slot))
                return nullptr;
            store(&value->capture_bound, 1U);
            wake(value);
        }
        const unsigned kind = value->plan.case_id;
        const bool ready = load(&value->birth_bound) && load(&value->child_ready);
        const bool requested = load(&value->phase_requested);
        if (!load(&value->action_done) &&
            ((kind == 23 && ready) || ((kind == 25 || kind == 26) && ready && requested))) {
            char token = 0;
            const long received = raw(SYS_read, value->descriptors[0], address(&token), 1);
            if (received != 1 || token != 'R') {
                poison(value);
                return nullptr;
            }
            append(value, Writer::controller, ObservationKind::readiness, value->plan.phase_id,
                   value->descriptors[0], value->birth_pid, received);
            if (kind == 23) {
                if (load(&value->epoch_ns) <= 0) {
                    poison(value);
                    return nullptr;
                }
                store(&value->offset_ns, 10 * second + 1);
                append(value, Writer::controller, ObservationKind::clock, 0, -1, value->birth_pid,
                       10 * second + 1);
            } else if (kind == 25) {
                const long result = raw(SYS_tgkill, value->parent_pid, value->parent_tid, SIGINT);
                append(value, Writer::controller, ObservationKind::signal, value->plan.phase_id, -1,
                       value->parent_pid, result);
                if (result != 0) {
                    poison(value);
                    return nullptr;
                }
            }
            store(&value->action_done, 1U);
            wake(value);
        }
        if (kind == 27 && requested && load(&value->birth_bound) && !load(&value->action_done)) {
            pollfd watched{value->descriptors[guard_slot], POLLIN, 0};
            long observed;
            do {
                const auto now = real_clock();
                if (now < 0) {
                    store(&value->controller_error, 1U);
                    poison(value);
                    return nullptr;
                }
                if (now >= start + control_budget) {
                    watchdog(value);
                    return nullptr;
                }
                const long milliseconds = (start + control_budget - now + 999999) / 1000000;
                observed = raw(SYS_poll, address(&watched), 1, milliseconds);
            } while (observed == -EINTR && !load(&value->stop));
            if (load(&value->stop))
                return nullptr;
            if (observed != 1 || !(watched.revents & (POLLIN | POLLHUP)) ||
                (watched.revents & (POLLERR | POLLNVAL))) {
                if (observed == 0)
                    watchdog(value);
                else {
                    store(&value->controller_error, 1U);
                    poison(value);
                }
                return nullptr;
            }
            siginfo_t info;
            // This is the EXPLICIT competing reaper, not watchdog/fallback/core.
            const long result =
                wait_pinned(value->descriptors[guard_slot], info, WEXITED | WNOHANG);
            append(value, Writer::controller, ObservationKind::syscall_return, 32,
                   value->descriptors[guard_slot],
                   result == 0 && info.si_pid > 0 ? info.si_pid : -1, result);
            if (result != 0 || info.si_pid != value->birth_pid) {
                poison(value);
                return nullptr;
            }
            append(value, Writer::controller, ObservationKind::release, 32, -1, info.si_pid,
                   info.si_status);
            store(&value->action_done, 1U);
            wake(value);
        }
        const long waited = wait_change(value, sequence, start + control_budget);
        if (waited == -ETIMEDOUT) {
            if (!load(&value->stop))
                watchdog(value);
            return nullptr;
        }
        if (waited != 0 && waited != -EINTR && waited != -EAGAIN) {
            store(&value->controller_error, 1U);
            poison(value);
            return nullptr;
        }
    }
}

bool prepare_descriptors(Shared *value) noexcept {
    for (unsigned pair = 0; pair != 2; ++pair) {
        int ends[2];
        if (raw(SYS_pipe2, address(ends), O_CLOEXEC) != 0)
            return false; // Failed pipe outputs are never adopted.
        for (unsigned end = 0; end != 2; ++end)
            value->descriptors[2 * pair + end] = ends[end];
        for (unsigned end = 0; end != 2; ++end)
            append(value, Writer::controller, ObservationKind::adopt, 0, ends[end],
                   value->parent_pid, ends[end]);
    }
    // All six slots stay occupied before owner entry: their numbers need not be
    // high. Child remapping replaces 0..4 copies and close_range removes >=5.
    // Only guard/capture must be >=5; later dup3 replaces these existing targets.
    for (unsigned index = guard_slot; index != 6; ++index) {
        const long next = raw(SYS_fcntl, value->descriptors[0], F_DUPFD_CLOEXEC, reserved_floor);
        if (next < 0)
            return false;
        value->descriptors[index] = static_cast<int>(next);
        append(value, Writer::controller, ObservationKind::adopt, 0, next, value->parent_pid, next);
    }
    const long flags = raw(SYS_fcntl, value->descriptors[0], F_GETFL);
    if (flags < 0 || raw(SYS_fcntl, value->descriptors[0], F_SETFL, flags | O_NONBLOCK) != 0)
        return false;
    if (value->plan.case_id == 28) {
        // The child's pre-birth placeholder copy is a genuine non-CLOEXEC
        // sentinel. Parent-only pidfd binding later restores CLOEXEC via dup3.
        const long changed = raw(SYS_fcntl, value->descriptors[guard_slot], F_SETFD, 0);
        append(value, Writer::controller, ObservationKind::syscall_return, 0,
               value->descriptors[guard_slot], value->parent_pid, changed);
        if (changed != 0)
            return false;
    }
    return !load(&value->poison);
}

bool dispose_child(Shared *value) noexcept {
    if (!load(&value->birth_bound))
        return value->birth_pid <= 0; // Never signal an unproved or placeholder FD.
    if (pinned_reaped(value))
        return true;
    poison(value); // Fallback cleanup can NEVER repair the pre-fallback verdict.
    long killed;
    do {
        killed = raw(SYS_pidfd_send_signal, value->descriptors[guard_slot], SIGKILL, 0, 0);
    } while (killed == -EINTR);
    if (killed != 0 && killed != -ESRCH)
        return false;
    siginfo_t info;
    const long reaped = wait_pinned(value->descriptors[guard_slot], info, WEXITED | own_thread);
    return (reaped == 0 && info.si_pid == value->birth_pid) ||
           (reaped == -ECHILD && pinned_reaped(value));
}

int copy_snapshot(std::int64_t *cells, const std::int64_t (&entries)[128][8],
                  const unsigned (&counts)[3]) noexcept {
    unsigned written = 0;
    for (unsigned who = 0; who != 3; ++who)
        for (unsigned index = 0; index != counts[who]; ++index) {
            for (unsigned cell = 0; cell != 8; ++cell)
                cells[8 * written + cell] = entries[beginnings[who] + index][cell];
            ++written;
        }
    for (unsigned index = 8 * written; index != 1024; ++index)
        cells[index] = -1;
    return static_cast<int>(written);
}
} // namespace

namespace subathon::schema_validation::native::fault {
Plan selected_plan() noexcept {
    const auto *value = state();
    return value ? value->plan : Plan{0, 0, 0};
}
[[gnu::no_stack_protector]] bool record(Writer writer, const Observation &observation) noexcept {
    auto *value = state();
    if (!value)
        return false;
    const long caller_pid = raw(SYS_getpid), caller_tid = raw(SYS_gettid);
    const unsigned kind = static_cast<unsigned>(observation.kind);
    if (kind < 1 || kind > 10 || encoded(observation.phase) == 0) {
        poison(value);
        return false;
    }
    if ((writer == Writer::parent &&
         (caller_pid != value->parent_pid || caller_tid != value->parent_tid)) ||
        (writer == Writer::child && caller_pid == value->parent_pid) ||
        writer == Writer::controller) {
        poison(value);
        return false; // Controller segment has its own single native writer.
    }
    if (writer == Writer::parent && observation.kind == ObservationKind::adopt &&
        observation.descriptor >= 0) {
        if (value->core_count == 12 || observation.descriptor > INT_MAX) {
            poison(value);
            return false;
        }
        value->core_descriptors[value->core_count++] = static_cast<int>(observation.descriptor);
    }
    const bool written = append(value, writer, observation.kind, encoded(observation.phase),
                                observation.descriptor, observation.pid, observation.result);
    if (observation.kind == ObservationKind::poison) {
        poison(value);
        return false;
    }
    return written;
}
[[gnu::no_stack_protector]] RawResult monotonic_ns() noexcept {
    auto *value = state();
    const auto now = real_clock();
    if (!value || now < 0)
        return now;
    std::int64_t empty = 0;
    if (load(&value->epoch_ns) == 0 && raw(SYS_getpid) != value->parent_pid) {
        poison(value);
        return -EPROTO;
    }
    if (__atomic_compare_exchange_n(&value->epoch_ns, &empty, now, false, __ATOMIC_RELEASE,
                                    __ATOMIC_RELAXED))
        append(value, Writer::parent, ObservationKind::clock, 0, -1, value->parent_pid, now);
    const auto offset = load(&value->offset_ns);
    if (offset < 0 || now > INT64_MAX - offset) {
        poison(value);
        return -EOVERFLOW;
    }
    return now + offset;
}
bool pin_birth(int pid, int pidfd) noexcept {
    auto *value = state();
    if (!value || pid <= 0 || pidfd < 0 || raw(SYS_gettid) != value->parent_tid ||
        load(&value->birth_request))
        return false;
    siginfo_t info;
    const long observed = wait_pinned(pidfd, info, WEXITED | WNOHANG | WNOWAIT | own_thread);
    if (observed != 0 || (info.si_pid != 0 && info.si_pid != pid)) {
        poison(value);
        return false;
    }
    value->birth_pid = pid;
    value->birth_source = pidfd;
    store(&value->birth_request, 1U);
    wake(value);
    return await_flag(value, &value->birth_bound);
}
bool pin_report_read(int descriptor) noexcept {
    auto *value = state();
    if (!value || value->plan.case_id != 28 || descriptor < 0 || load(&value->capture_request))
        return false;
    const long flags = raw(SYS_fcntl, descriptor, F_GETFL);
    if (flags < 0 || !(flags & O_NONBLOCK) || (flags & O_ACCMODE) != O_RDONLY) {
        poison(value);
        return false;
    }
    value->capture_source = descriptor;
    store(&value->capture_request, 1U);
    wake(value);
    return await_flag(value, &value->capture_bound);
}
bool capture_outcome(const unsigned char *bytes, std::size_t count) noexcept {
    auto *value = state();
    if (!value || value->plan.case_id != 28 || (!bytes && count) ||
        value->capture_count > sizeof(value->capture) ||
        count > sizeof(value->capture) - value->capture_count) {
        if (value)
            poison(value);
        return false;
    }
    for (std::size_t index = 0; index != count; ++index)
        value->capture[value->capture_count++] = bytes[index];
    return true;
}
[[gnu::no_stack_protector]] bool checkpoint(Phase phase, RawResult actual) noexcept {
    auto *value = state();
    if (!value)
        return false;
    const unsigned id = encoded(phase);
    if (id == 0) {
        poison(value);
        return false;
    }
    const auto plan = value->plan;
    const bool reaper = plan.case_id == 27 && id == 32;
    if (!reaper && (plan.phase_id == 0 || id != plan.phase_id))
        return false;
    std::uint32_t unused = 0;
    if (!__atomic_compare_exchange_n(&value->trigger_claimed, &unused, 1U, false, __ATOMIC_ACQ_REL,
                                     __ATOMIC_ACQUIRE))
        return false;
    const Writer writer = raw(SYS_getpid) == value->parent_pid ? Writer::parent : Writer::child;
    append(value, writer,
           phase.edge == PhaseEdge::before ? ObservationKind::syscall_attempt
                                           : ObservationKind::syscall_return,
           id, -1, raw(SYS_getpid), phase.edge == PhaseEdge::before ? -1 : actual);
    if (plan.case_id == 24)
        return !load(
            &value->poison); // Caller applies ONLY its reviewed negative errno/continuation.
    if (writer == Writer::child) {
        poison(value);
        return false;
    }
    store(&value->phase_requested, 1U);
    wake(value);
    return await_flag(value, &value->action_done);
}
[[gnu::no_stack_protector]] bool child_ready_and_hold() noexcept {
    auto *value = state();
    if (!value || raw(SYS_getpid) == value->parent_pid)
        return false;
    if (load(&value->child_ready)) {
        poison(value);
        return false;
    }
    const char token = 'R';
    if (raw(SYS_write, value->descriptors[1], address(&token), 1) != 1) {
        poison(value);
        return false;
    }
    append(value, Writer::child, ObservationKind::readiness, 21, value->descriptors[1],
           raw(SYS_getpid), 1);
    store(&value->child_ready, 1U);
    wake(value);
    char released = 0;
    long result;
    do {
        result = raw(SYS_read, value->descriptors[2], address(&released), 1);
    } while (result == -EINTR);
    return result == 1 && released == 'G';
}
bool release_child() noexcept {
    auto *value = state();
    const char token = 'G';
    return value && raw(SYS_write, value->descriptors[3], address(&token), 1) == 1;
}
const void *mapping_address() noexcept { return state(); }
} // namespace subathon::schema_validation::native::fault

extern "C" int subathon_schema_fault_arm_v1(unsigned int case_id, unsigned int phase_id,
                                            int fault_errno) noexcept {
    if (__atomic_exchange_n(&ever_armed, true, __ATOMIC_ACQ_REL))
        return -1;
    const Plan plan{case_id, phase_id, fault_errno};
    if (!valid_plan(plan))
        return -1;
    schema_foundation::KernelAction child_action;
    if (raw(SYS_rt_sigaction, SIGCHLD, 0, address(&child_action), 8) != 0 ||
        child_action.handler != 0 || (child_action.flags & SA_NOCLDWAIT))
        return -1;
    const long memory =
        raw(SYS_mmap, 0, map_bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (memory < 0)
        return -1;
    auto *value = ::new (reinterpret_cast<void *>(memory)) Shared{};
    value->plan = plan;
    value->parent_pid = static_cast<int>(raw(SYS_getpid));
    value->parent_tid = static_cast<int>(raw(SYS_gettid));
    value->birth_pid = value->birth_source = value->capture_source = value->controller_tid = -1;
    for (auto &descriptor : value->descriptors)
        descriptor = -1;
    for (auto &descriptor : value->core_descriptors)
        descriptor = -1;
    store(&shared, value);
    append(value, Writer::controller, ObservationKind::adopt, 0, -1, value->parent_pid, memory);
    if ((case_id == 25 && value->parent_pid != value->parent_tid) || !prepare_descriptors(value)) {
        poison(value);
        subathon_schema_fault_fallback_v1();
        return -1;
    }
    const int created = pthread_create(&controller_handle, nullptr, controller, value);
    if (created != 0) {
        poison(value);
        subathon_schema_fault_fallback_v1();
        return -1;
    }
    controller_created = true;
    return 0; // Never waits for entry, birth, readiness or an owner phase.
}

extern "C" int subathon_schema_fault_snapshot_v1(int64_t *cells, unsigned int cell_count) noexcept {
    if (!cells || cell_count != 1024)
        return -1;
    auto *value = state();
    if (!value)
        return -1; // The test saves observations BEFORE fallback destroys this map.
    if (load(&value->poison) || load(&value->watchdog_fired) || load(&value->controller_error))
        return -1;
    unsigned counts[3];
    for (unsigned who = 0; who != 3; ++who) {
        counts[who] = load(&value->counts[who]);
        if (counts[who] > capacities[who]) {
            poison(value);
            return -1;
        }
    }
    return copy_snapshot(cells, value->entries, counts);
}

extern "C" int subathon_schema_fault_report_v1(unsigned char *bytes,
                                               unsigned int byte_count) noexcept {
    auto *value = state();
    if (!value || !bytes || byte_count != 248 || value->plan.case_id != 28 ||
        !load(&value->capture_bound) || load(&value->poison))
        return -1;
    while (!value->capture_eof) {
        unsigned char suffix[258];
        const std::size_t room = 258 - value->capture_count;
        const long result = raw(SYS_read, value->descriptors[capture_slot], address(suffix), room);
        if (result == 0) {
            value->capture_eof = true;
        } else if (result > 0 && static_cast<unsigned long>(result) <=
                                     sizeof(value->capture) - value->capture_count) {
            for (long index = 0; index != result; ++index)
                value->capture[value->capture_count++] = suffix[index];
        } else if (result != -EINTR) {
            poison(value);
            return -1;
        }
    }
    if (value->capture_count != 248) {
        poison(value);
        return -1;
    }
    for (unsigned index = 0; index != 248; ++index)
        bytes[index] = value->capture[index];
    return 248;
}

extern "C" int subathon_schema_fault_fallback_v1(void) noexcept {
    auto *value = state();
    if (!value)
        return last_disposal;
    store(&value->stop, 1U);
    wake(value);
    // The caller's before-fallback observations are already complete. Kill any
    // leaked child now, also waking a controller polling its pinned identity.
    if (load(&value->birth_bound) && !pinned_reaped(value)) {
        long result;
        do {
            result = raw(SYS_pidfd_send_signal, value->descriptors[guard_slot], SIGKILL, 0, 0);
        } while (result == -EINTR);
        if (result != 0 && result != -ESRCH)
            poison(value);
    }
    if (controller_created) {
        if (pthread_join(controller_handle, nullptr) != 0) {
            poison(value);
            return -1; // Keep identities/map for a later real disposal attempt.
        }
        controller_created = false;
    }
    if (!dispose_child(value))
        return -1; // Never release the pinned identity while quiescence is unknown.
    for (unsigned index = 0; index != value->core_count; ++index) {
        const int descriptor = value->core_descriptors[index];
        bool seen = false;
        for (unsigned previous = 0; previous != index; ++previous)
            seen |= value->core_descriptors[previous] == descriptor;
        if (!seen && raw(SYS_fcntl, descriptor, F_GETFD) >= 0) {
            poison(value);
            raw(SYS_close, descriptor); // No free-FD creation occurred since owner return.
        }
    }
    for (auto &descriptor : value->descriptors) {
        if (descriptor >= 0) {
            const long result = raw(SYS_close, descriptor);
            append(value, Writer::controller, ObservationKind::release, 0, descriptor,
                   value->parent_pid, result);
            descriptor = -1; // Linux close is never retried on this number.
            if (result < 0)
                poison(value);
        }
    }
    const bool failed =
        load(&value->poison) || load(&value->watchdog_fired) || load(&value->controller_error);
    const long unmapped = raw(SYS_munmap, address(value), map_bytes);
    if (unmapped != 0)
        return -1;
    store(&shared, static_cast<Shared *>(nullptr));
    last_disposal = failed ? -1 : 0;
    return last_disposal; // Disposal never repairs a failed fixture/owner verdict.
}
