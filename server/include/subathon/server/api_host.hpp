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
// lease take/copy: those plus invalid_completion; current source: unavailable.
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
using OwnedDomainResult = std::variant<OwnedReply, Failure>;
struct CurrentInstallationView {
    std::string_view installation, epoch, revision;
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
    // Cold=0, first activation=1; installation/epoch rebind adds one.
    // Normal unchanged commits and revision-only rebind leave it unchanged.
    std::uint64_t generation = 0, associations = 0, association_charge = 0;
    std::uint64_t control_charge = 0, stop_fences = 0;
    std::uint64_t queued_diagnostics = 0, accepted_wrappers = 0;
    PoolUsage input, response;
    std::optional<std::uint64_t> drain_deadline;
};
class Host;
class Ingress;
class Execution;
class Publication;
class Transaction;
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
class CommandDispatcher {
  public:
    virtual ~CommandDispatcher() = default;
    virtual OwnedReply diagnostic(CommandView) = 0;
    virtual OwnedDomainResult execute(Execution &, CommandView) = 0;
    virtual OwnedReply complete(Publication &, const OwnedDomainResult &) = 0;
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
class AccountSessions final {
  public:
    ~AccountSessions();
    AccountSessionRead<StoredInstallationFacts> read_installation(std::string_view);
    LifecycleWriteResult replace_installation_epoch(const StoredInstallationFacts &,
                                                    std::string_view, std::string_view);
    AccountSessions(const AccountSessions &) = delete;
    AccountSessions &operator=(const AccountSessions &) = delete;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    AccountSessions();
    friend class Transaction;
};
class Transaction final {
  public:
    ~Transaction();
    AccountSessions &accounts();
    // Final read through the same real store immediately precedes actual COMMIT.
    void commit();
    void rollback();
    Transaction(const Transaction &) = delete;
    Transaction &operator=(const Transaction &) = delete;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    Transaction();
    friend class Execution;
};
class Execution final {
  public:
    ~Execution();
    Transaction &begin_transaction();
    std::shared_ptr<CurrentSessionClock> clock() const;
    Result<CurrentInstallationView> current_installation() const;
    Execution(const Execution &) = delete;
    Execution &operator=(const Execution &) = delete;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    Execution();
    friend class Host;
};
class Publication final {
  public:
    ~Publication();
    std::shared_ptr<CurrentSessionClock> clock() const;
    Result<CurrentInstallationView> current_installation() const;
    Publication(const Publication &) = delete;
    Publication &operator=(const Publication &) = delete;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    Publication();
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
};
} // namespace subathon::server::api_host
