// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/api_host.hpp>
#include <subathon/server/testing/api_host_observers.hpp>
#include <utility>

namespace subathon::server::api_host {
// EXPLICIT UNIMPLEMENTED BASELINE: no writer, source, queue, profile or readiness.
struct Host::Impl {
    std::unique_ptr<Transport> transport;
    std::unique_ptr<CommandDispatcher> dispatcher;
};
struct Ingress::Impl {};
struct PeerLease::Impl {};
struct ResponseLease::Impl {};
Host::Host(Options, std::unique_ptr<Transport> t, std::unique_ptr<CommandDispatcher> d)
    : impl_(std::make_unique<Impl>(Impl{std::move(t), std::move(d)})) {}
Host::~Host() = default;
Result<StartAccepted> Host::start(StartupObserver) { return NotImplemented{}; }
void Host::request_stop() noexcept {}
Inspection Host::inspect() const { return {}; }
Ingress::Ingress(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
Ingress::~Ingress() = default;
Result<PeerLease> Ingress::connected() { return NotImplemented{}; }
Result<Admitted> Ingress::try_submit(const PeerLease &, std::span<const std::byte>, RouteId,
                                     std::span<const std::byte>) {
    return NotImplemented{};
}
void Ingress::disconnected(const PeerLease &) noexcept {}
PeerLease::PeerLease(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
PeerLease::PeerLease(PeerLease &&) noexcept = default;
PeerLease &PeerLease::operator=(PeerLease &&) noexcept = default;
PeerLease::~PeerLease() = default;
ResponseLease::ResponseLease(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
ResponseLease::ResponseLease(ResponseLease &&) noexcept = default;
ResponseLease &ResponseLease::operator=(ResponseLease &&) noexcept = default;
ResponseLease::~ResponseLease() = default;
Result<CommandView> ResponseLease::take() { return NotImplemented{}; }
Result<ResponseLease> ResponseLease::try_copy() const { return NotImplemented{}; }
void ResponseLease::reset() noexcept {}
struct AccountSessions::Impl {};
AccountSessions::AccountSessions() = default;
AccountSessions::~AccountSessions() = default;
AccountSessionRead<StoredInstallationFacts> AccountSessions::read_installation(std::string_view) {
    throw NotImplemented{};
}
LifecycleWriteResult AccountSessions::replace_installation_epoch(const StoredInstallationFacts &,
                                                                 std::string_view,
                                                                 std::string_view) {
    throw NotImplemented{};
}
struct Transaction::Impl {};
Transaction::Transaction() = default;
Transaction::~Transaction() = default;
AccountSessions &Transaction::accounts() { throw NotImplemented{}; }
void Transaction::commit() { throw NotImplemented{}; }
void Transaction::rollback() { throw NotImplemented{}; }
struct Execution::Impl {};
Execution::Execution() = default;
Execution::~Execution() = default;
Transaction &Execution::begin_transaction() { throw NotImplemented{}; }
std::shared_ptr<CurrentSessionClock> Execution::clock() const { throw NotImplemented{}; }
Result<CurrentInstallationView> Execution::current_installation() const { return NotImplemented{}; }
struct Publication::Impl {};
Publication::Publication() = default;
Publication::~Publication() = default;
std::shared_ptr<CurrentSessionClock> Publication::clock() const { throw NotImplemented{}; }
Result<CurrentInstallationView> Publication::current_installation() const {
    return NotImplemented{};
}
std::unique_ptr<Host> testing::HostAccess::create(Options o, std::unique_ptr<Transport> t,
                                                  std::unique_ptr<CommandDispatcher> d, Hooks) {
    return std::make_unique<Host>(std::move(o), std::move(t), std::move(d));
}
Result<Connected>
testing::HostAccess::inject_next_control_ack_loss(Host &,
                                                  subathon::server::testing::WorkControlFault) {
    return NotImplemented{};
}
Result<Connected> testing::HostAccess::inject_next_envelope_mismatch(Host &) {
    return NotImplemented{};
}
testing::ElapsedIdentity testing::HostAccess::elapsed_identity(const Host &) { return {}; }
} // namespace subathon::server::api_host
