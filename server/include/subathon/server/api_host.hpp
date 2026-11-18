// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <subathon/server/native_websocket_profile.hpp>
#include <subathon/server/sqlite_account_sessions.hpp>
#include <variant>
#include <vector>

namespace subathon::server::session_clock::detail {
class HostBindingAccess;
}
namespace subathon::server::api_host {
namespace testing {
class HostAccess;
}
// Baseline markers never represent an admitted operation or an actual SQL outcome.
struct NotImplemented final : std::exception {
    const char *what() const noexcept override {
        return "APITEST-001 production host unimplemented";
    }
};
enum class State { cold, starting, serving, stopping, stopped, failed };
// start: invalid_options/wrong_thread/already_started; ingress: stopping,
// unavailable/resource_limit/expired/disconnected/elapsed_unavailable/wrong_thread;
// lease take/copy: those plus invalid_completion; handle calls: see CommandHandle.
// cancelled is owner cancellation; incomplete_shutdown never certifies stopped.
// Every unimplemented Result operation returns NotImplemented, never success.
enum class Failure {
    invalid_options,
    wrong_thread,
    already_started,
    stopping,
    unavailable,
    resource_limit,
    expired,
    disconnected,
    cancelled,
    invalid_completion,
    elapsed_unavailable,
    incomplete_shutdown
};
template <class T> using Result = std::variant<T, Failure, NotImplemented>;
struct StartAccepted {};
struct Admitted {};
struct Connected {};
struct StopDeadline {
    std::uint64_t nanoseconds;
};
struct NetworkBarrier {
    bool callbacks_revoked, sockets_removed, framework_joined, io_joined;
};
using StartupObserver = std::function<void(Result<StartAccepted>)>;
using NetworkBarrierObserver = std::function<void(NetworkBarrier)>;
// Finite trusted table. The last two routes require unavailable downstream owners;
// their IDs cannot establish account authority or trusted publication UTC.
enum class RouteId { diagnostic, installation, credential, account };
struct CommandView {
    RouteId route;
    std::span<const std::byte> correlation, bytes;
};
struct OwnedReply {
    std::vector<std::byte> bytes;
};
// Counts/capacities/durations are nonzero and fit the selected finite profile;
// per_peer <= associations, envelope_bytes <= input_capacity,
// response_bytes <= response_capacity, writer pending_batches >= associations.
// Millisecond conversion and each absolute deadline addition are checked.
struct Limits {
    std::uint64_t associations = 128, per_peer = 8, peers = 128;
    std::uint64_t envelope_bytes = 1048576, input_capacity = 8388608;
    std::uint64_t response_bytes = 1048576, response_capacity = 8388608;
    std::uint64_t input_slots = 128, response_slots = 128;
    std::uint64_t queue_ms = 5000, handoff_ms = 5000, drain_ms = 5000;
};
struct Options {
    std::string store_path, installation = "I";
    // Host associations need128 pending slots; the standalone writer stays64.
    StorageLimits writer_limits = [] {
        StorageLimits limits;
        limits.pending_batches = 128;
        return limits;
    }();
    AccountSessionStoreLimits store_limits;
    Limits limits;
    native_ws::Limits native{32,      16,       1048576, 1048576, 4096,    2,
                             2097152, 16777216, 1048590, 1048576, 1048590, 254,
                             64,      5000,     10000,   30000,   1000};
};
struct PoolUsage {
    std::uint64_t capacity = 0, copies = 0, metadata = 0, peak_charge = 0;
};
struct Inspection {
    State state = State::cold;
    bool storage_active = false, writer_barrier = false, network_barrier = false;
    bool owner_cleanup = false;
    std::uint64_t associations = 0, association_charge = 0;
    std::uint64_t control_charge = 0, stop_fences = 0;
    std::uint64_t queued_diagnostics = 0, accepted_units = 0;
    PoolUsage input, response;
    std::optional<std::uint64_t> drain_deadline;
};
class Host;
class Ingress;
class CommandHandle;
// Opaque, retainable, revocable association; no public constructor/identity setter.
class PeerLease final {
  public:
    PeerLease(PeerLease &&) noexcept;
    PeerLease &operator=(PeerLease &&) noexcept;
    ~PeerLease();
    PeerLease(const PeerLease &) = delete;
    PeerLease &operator=(const PeerLease &) = delete;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit PeerLease(std::unique_ptr<Impl>);
    friend class Ingress;
};
class ResponseLease final {
  public:
    ResponseLease(ResponseLease &&) noexcept;
    ResponseLease &operator=(ResponseLease &&) noexcept;
    ~ResponseLease();
    ResponseLease(const ResponseLease &) = delete;
    ResponseLease &operator=(const ResponseLease &) = delete;
    // IO-owner check, expiry recheck and once-only consumption precede the borrow.
    Result<CommandView> take();
    // A payload copy pays another reservation, but shares once-only response authority.
    Result<ResponseLease> try_copy() const;
    void reset() noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit ResponseLease(std::unique_ptr<Impl>);
    friend class Host;
};
// Host refusals have no writer completion. StorageError is only the actual
// private writer's enqueue_work result (none means accepted); never predicted.
using ForwardResult = std::variant<StorageError, Failure, NotImplemented>;

// One host-issued move-only handle for one entered command. It grants no raw
// writer, account/current-installation facade, generation or session clock.
// The dispatcher retains it across asynchronous units; destruction without finish
// abandons the command, draining all accepted bodies/completions before release.
class CommandHandle final {
  public:
    CommandHandle(CommandHandle &&) noexcept;
    CommandHandle &operator=(CommandHandle &&) noexcept;
    ~CommandHandle();
    CommandHandle(const CommandHandle &) = delete;
    CommandHandle &operator=(const CommandHandle &) = delete;

    // Owner-only checked borrow of the admitted, charged host-owned input.
    // Use only during execute or accepted bodies/completions. Storage survives
    // all accepted units through drain, including after forward/reply revocation;
    // this borrow may be retained by a unit accepted before the current callback
    // returns. No borrow escapes final cleanup and no uncharged payload copy is
    // authorized. An accepted callback may reacquire input even after stop/end.
    Result<CommandView> input() const;

    // Only execute and supplied unit completions may forward. Execute may
    // PREQUEUE multiple units (including the three-unit fail-close example).
    // Forward from a running body refuses invalid_completion and ends without
    // reply; accepted units still drain. No event pumping or nested dispatch.
    // Work/completion captures retain the existing intake resource rules; there
    // is no new capture-size limit, storage API or compile-time charging rule.
    // Host bookkeeping is bounded by pending_batches, with no secondary queue.
    // Results pass unchanged to completion on Qt, after ticket revocation and
    // writer_completed observation. Callback exceptions end without reply/replay.
    ForwardResult forward(SqliteWriter::Work, SqliteWriter::WorkCompletion);

    // Once-only end: nullopt means no reply. Finish while any unit is pending,
    // or from a running body, refuses invalid_completion and ends without reply.
    // The last unit's supplied completion may finish; handoff/next entry waits
    // until it returns normally. Finish never preempts accepted-unit drain.
    // Unknown outcome or abnormal commit latches withholding; reply then refuses
    // invalid_completion and ends without reply. Known noncommits may be replied
    // to; the host does not inspect reply content. No-unit commands may reply.
    Result<Admitted> finish(std::optional<OwnedReply> = std::nullopt);

    // For input/forward/finish: wrong_thread refuses before host/writer access
    // or command mutation. Reentrant forward/finish refuses invalid_completion
    // without state change. Further forward/finish on a terminal handle or a
    // second finish refuses likewise; input remains borrowed for accepted drain.
    // No call changes a previous finish/reply or revives authority.
    // There is no active-command timeout. Disconnect/elapsed failure revoke
    // response authority only; later forward remains allowed until stop/end.
    // Before forward/finish consume stop INCLUDING pending native signal.
    // Stopped forward or finish(reply) refuses stopping and ends without reply;
    // finish(nullopt) may end without reply, subject to the pending-unit rule.
    // Finish(reply) after disconnect/elapsed failure refuses with that response
    // failure and ends without reply. A refused writer admission ends without
    // reply; no body/completion runs for it or fabricated result is emitted.
    // Returning from execute/completion unfinished with no accepted unit pending
    // ends without reply that turn. Retained inert handles cannot pin payloads.
    // Foreign-thread destruction only records bounded lifetime-safe abandonment;
    // owner cleanup drains/releases it. Handles outliving Host are inert.

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit CommandHandle(std::unique_ptr<Impl>);
    friend class Host;
    friend class subathon::server::session_clock::detail::HostBindingAccess;
};
class CommandDispatcher {
  public:
    virtual ~CommandDispatcher() = default;
    // Synchronous bounded data-only callback outside the writer FIFO.
    virtual OwnedReply diagnostic(CommandView) = 0;
    // One command at a time, including diagnostics between protected commands.
    // CommandView borrows only for this call; later units reacquire input().
    virtual void execute(CommandHandle, CommandView) = 0;
};
class Transport {
  public:
    virtual ~Transport() = default;
    virtual Result<StartAccepted> start(Ingress &, std::unique_ptr<native_ws::Profile>,
                                        StartupObserver) = 0;
    virtual void handoff(ResponseLease) = 0;
    virtual void stop(StopDeadline, NetworkBarrierObserver) = 0;
};
class Ingress final {
  public:
    // Called only for actual connect/close events on the transport's owner.
    Result<PeerLease> connected();
    Result<Admitted> try_submit(const PeerLease &, std::span<const std::byte> correlation, RouteId,
                                std::span<const std::byte>);
    void disconnected(const PeerLease &) noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit Ingress(std::unique_ptr<Impl>);
    ~Ingress();
    friend class Host;
};
class Host final {
  public:
    Host(Options, std::unique_ptr<Transport>, std::unique_ptr<CommandDispatcher>);
    ~Host();
    Host(const Host &) = delete;
    Host &operator=(const Host &) = delete;
    Result<StartAccepted> start(StartupObserver);
    void request_stop() noexcept;
    Inspection inspect() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend class testing::HostAccess;
    friend class subathon::server::session_clock::detail::HostBindingAccess;
};
} // namespace subathon::server::api_host
