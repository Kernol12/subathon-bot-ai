// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/api_host.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <thread>

namespace subathon::server::api_host::testing {
// Bounded observation only. No payload, SQL, credentials, facade or writable permit.
enum class EventKind {
    inspection_enqueued,
    inspection_completed,
    installation_read_enqueued,
    installation_read_completed,
    activated,
    listener_ready,
    diagnostic_queued,
    diagnostic_cancelled,
    diagnostic_entered,
    wrapper_accepted,
    wrapper_dispatched,
    execution_entered,
    begin_attempted,
    precommit_read,
    actual_commit,
    execution_revoked,
    writer_completed, // accepted command wrappers only; excludes startup work
    publication_entered,
    publication_revoked,
    local_handoff,
    stop_consumed,
    callbacks_revoked,
    sockets_removed,
    network_ack,
    writer_removed,
    writer_ack,
    framework_joined,
    io_joined,
    source_destroyed,
    dispatcher_destroyed,
    owner_cleanup,
    stopped,
    ingress_refused
};
struct WorkObservation {
    TransactionOutcome transaction;
    WorkError error;
    bool has_value, ticket_revoked;
};
struct Event {
    EventKind kind;
    std::uint64_t ordinal = 0, generation = 0;
    std::thread::id thread;
    // Present only for an ACTUAL writer completion; diagnostics have none.
    std::optional<WorkObservation> work;
    std::optional<Failure> refusal;
};
// Observe synchronously on the emitting thread, with NO host lock held.
// Bounded blocking is allowed. No event pumping or host re-entry on the
// emitting thread except request_stop(). Independent IO ingress may progress;
// never wait for a turn on the thread currently emitting.
struct Hooks {
    std::shared_ptr<const native_ws::ElapsedSource> elapsed;
    std::shared_ptr<CurrentSessionClock> raw_session_sampler;
    std::function<void(const Event &)> observe;
};
struct ElapsedIdentity {
    const native_ws::ElapsedSource *host = nullptr, *profile = nullptr;
};
class HostAccess final {
  public:
    static std::unique_ptr<Host> create(Options, std::unique_ptr<Transport>,
                                        std::unique_ptr<CommandDispatcher>, Hooks);
    static Result<Connected>
    inject_next_control_ack_loss(Host &, subathon::server::testing::WorkControlFault);
    // Idle-only, one-shot: corrupt only the next actual wrapper's private
    // envelope match. Never changes SQL/WorkResult truth or forges a match.
    static Result<Connected> inject_next_envelope_mismatch(Host &);
    static ElapsedIdentity elapsed_identity(const Host &);
};
} // namespace subathon::server::api_host::testing
