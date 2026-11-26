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
    diagnostic_queued, // Qt owner: actual retained callback/data, before entry, no live scope
    diagnostic_cancelled,
    diagnostic_entered,
    // Synchronous on Qt after actual retained admission, before entry checks.
    // No execution/writer scope or host lock; a bounded pause leaves IO ingress live.
    command_admitted,
    command_entered,
    unit_entered,     // host emits immediately before calling an accepted unit body
    writer_completed, // actual accepted command units only; excludes startup work
    command_ended,
    command_cancelled,
    forward_refused,
    finish_refused,
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
    ingress_refused,
    operation_refused // actual owner terminal refusal after admission, no fabricated ingress phase
};
// A command ending is not a SqliteWorkResult or an invented writer completion.
enum class CommandEnd {
    finished,
    abandoned,
    idle,
    callback_exception,
    writer_refused,
    premature_finish,
    reply_withheld,
    stopping,
    disconnected,
    elapsed_unavailable,
    cancelled
};
struct WorkObservation {
    TransactionOutcome transaction;
    WorkError error;
    bool has_value, ticket_revoked;
};
struct Event {
    EventKind kind;
    std::uint64_t ordinal = 0, command = 0, unit = 0;
    std::thread::id thread;
    // Present only for an ACTUAL writer completion (including inspection/read
    // completion events at startup); diagnostics/refusal-only events have none.
    std::optional<WorkObservation> work;
    std::optional<Failure> refusal;
    // Only a real private-writer admission attempt may supply this result.
    std::optional<StorageError> admission;
    std::optional<CommandEnd> end;
};
// Observe synchronously on the emitting thread, with NO host lock held.
// Bounded blocking is allowed. No event pumping or host re-entry on the
// emitting thread except request_stop(). Independent IO ingress may progress;
// never wait for a turn on the thread currently emitting.
struct Hooks {
    std::shared_ptr<const native_ws::ElapsedSource> elapsed;
    std::function<void(const Event &)> observe;
};
// API-023 Stage 3 test seams. An injected fault replaces Engine::wake's queued
// owner-turn post: refuse returns false, raise throws InjectedPostFault.
enum class PostFault { refuse, raise };
struct InjectedPostFault final : std::exception {
    const char *what() const noexcept override { return "API-023 injected owner-turn post fault"; }
};
struct OwnerTimerObservation {
    bool active = false;
    unsigned registrations = 0;
};
struct ElapsedIdentity {
    const native_ws::ElapsedSource *host = nullptr, *profile = nullptr;
};
class HostAccess final {
  public:
    static std::unique_ptr<Host> create(Options, std::unique_ptr<Transport>,
                                        std::unique_ptr<CommandDispatcher>, Hooks);
    // Owner-thread, one-shot. Cold arming is forwarded once to the actual writer
    // after open while idle, BEFORE inspection enqueue; otherwise idle-only.
    // Never assigns a work outcome. Startup completions expose actual work truth.
    static Result<Connected>
    inject_next_control_ack_loss(Host &, subathon::server::testing::WorkControlFault);
    static ElapsedIdentity elapsed_identity(const Host &);
    // Calls the Host's private fatal relay binding, the one TransportComposition
    // receives, as an already-published Foundation fact. Any thread, store only.
    static void publish_fatal(Host &, bounded_http::FatalElapsed) noexcept;
    // Any thread: the next `count` owner-turn posts fail as given.
    static void inject_post_faults(Host &, PostFault, unsigned count) noexcept;
    // Any thread: injected faults not yet met by a post.
    static unsigned pending_post_faults(const Host &) noexcept;
    // Owner, before start: stop the cold owner timer; later registrations fail.
    static Result<Connected> refuse_timer_registration(Host &);
    // Owner, before start, once: runs after startup preparation, before the
    // early-stop check and writer open.
    static Result<Connected> before_writer_open(Host &, std::function<void()>);
    // Owner: whether the owner timer is registered and how often it was.
    static OwnerTimerObservation owner_timer(const Host &);
};
} // namespace subathon::server::api_host::testing
