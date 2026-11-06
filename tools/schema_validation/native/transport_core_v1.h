// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SUBATHON_SCHEMA_VALIDATION_NATIVE_TRANSPORT_CORE_V1_H
#define SUBATHON_SCHEMA_VALIDATION_NATIVE_TRANSPORT_CORE_V1_H

#include <climits>
#include <concepts>
#include <cstdint>

namespace subathon::schema_validation::native {

static_assert(CHAR_BIT == 8);
static_assert(sizeof(int) == 4 && sizeof(unsigned int) == 4);
static_assert(sizeof(void *) == 8 && sizeof(std::uintptr_t) == 8);

// Private borrowed inputs only; no resource or result ownership crosses this type.
struct PathView {
    const unsigned char *data;
    unsigned int bytes;
};

struct Request {
    PathView manifest;
    PathView interpreter;
    PathView worker;
    PathView sdk;
};

using NativeWord = std::uintptr_t;
// Raw Linux result: nonnegative success or negative errno, never a domain outcome.
using RawResult = std::int64_t;

// Compile-time operation tags, not syscall numbers or a runtime dispatch interface.
enum class KernelCall : unsigned int {
    openat,
    fstat,
    newfstatat,
    read,
    write,
    close,
    fcntl,
    pipe2,
    clone3,
    dup3,
    close_range,
    execve,
    exit,
    clock_gettime,
    poll,
    rt_sigaction,
    rt_sigprocmask,
    pidfd_send_signal,
    waitid,
    getpid
};

enum class PhaseBase : unsigned int {
    config_open = 1,
    config_read = 2,
    config_close = 3,
    activation_pipe = 4,
    outcome_pipe = 5,
    source_duplicate = 6,
    null_open = 7,
    atomic_birth = 8,
    parent_write_close = 9,
    child_signal = 10,
    child_dup = 11,
    child_close_range = 12,
    child_exec = 13,
    protocol_read = 14,
    protocol_poll = 15,
    wait_reap = 16,
    pidfd_kill = 17,
    pidfd_close = 18,
    final_commit = 19
};

enum class PhaseEdge : unsigned int { before = 0, after = 1 };

// The private encoded phase ID is 2 * base + edge; no runtime helper is defined here.
struct Phase {
    PhaseBase base;
    PhaseEdge edge;
};

enum class Continuation : unsigned int { proceed, resource_refusal, interrupted };

enum class ObservationKind : unsigned int {
    syscall_attempt = 1,
    syscall_return = 2,
    adopt = 3,
    release = 4,
    clock = 5,
    signal = 6,
    readiness = 7,
    final_commit = 8,
    poison = 9,
    child_exec_handoff_intent = 10
};

// Native metadata only. The test adapter adds writer/sequence/clock journal fields.
struct Observation {
    ObservationKind kind;
    Phase phase;
    std::int64_t descriptor;
    std::int64_t pid;
    RawResult result;
};

// Unevaluated signature checks only; policy methods and the owner have NO body here.
// Each actually used KernelCall specialization must also compile in the later core.
template <class Ops>
concept TransportOps = requires(Ops &ops, NativeWord word, Phase phase, RawResult result,
                                const Observation &observation) {
    {
        ops.template syscall<KernelCall::read>(word, word, word, word, word, word)
    } noexcept -> std::same_as<RawResult>;
    { ops.monotonic_ns() } noexcept -> std::same_as<RawResult>;
    { ops.checkpoint(phase, result) } noexcept -> std::same_as<Continuation>;
    { ops.observe(observation) } noexcept -> std::same_as<void>;
};

// Declaration ONLY. The real shared owner awaits reviewed negative tests and red.
template <TransportOps Ops> int run_transport_v1(const Request &request, Ops &ops) noexcept;

} // namespace subathon::schema_validation::native

#endif // SUBATHON_SCHEMA_VALIDATION_NATIVE_TRANSPORT_CORE_V1_H
