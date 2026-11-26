// SPDX-License-Identifier: GPL-3.0-or-later
#include "api_host_internal.hpp"
#include "session_clock_host_internal.hpp"
#include <QAbstractEventDispatcher>
#include <QBasicTimer>
#include <QCoreApplication>
#include <QThread>
#include <QTimerEvent>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <limits>
#include <mutex>
#include <subathon/server/api_host.hpp>
#include <subathon/server/api_host_elapsed.hpp>
#include <subathon/server/sqlite_reference_authority.hpp>
#include <subathon/server/testing/api_host_observers.hpp>
#include <thread>
#include <utility>

namespace subathon::server::api_host {
namespace {
using U64 = std::uint64_t;
namespace obs = testing;
using K = obs::EventKind;
using End = obs::CommandEnd;
constexpr U64 max = std::numeric_limits<U64>::max();
namespace bh = bounded_http;
// Exact enumerator spellings for the fixed diagnostic line (API-023 r4 §2).
constexpr std::array<std::string_view, 3> fatal_reasons{"sample_unavailable", "sample_regressed",
                                                        "deadline_overflow"};
constexpr std::array<std::string_view, 11> fatal_sites{"scan",
                                                       "accept_sample",
                                                       "peer_guard",
                                                       "idle_sample",
                                                       "accept_deadline",
                                                       "upgrade_deadline",
                                                       "request_deadline",
                                                       "idle_deadline",
                                                       "reply_deadline",
                                                       "control_deadline",
                                                       "listener_hold_sample"};
static_assert(fatal_reasons.size() == std::size_t(bh::FatalElapsedReason::deadline_overflow) + 1);
static_assert(fatal_sites.size() == std::size_t(bh::FatalElapsedSite::listener_hold_sample) + 1);
constexpr std::string_view fatal_prefix = "api_host.foundation_elapsed_fatal/";
constexpr std::size_t longest(const auto &names) {
    std::size_t n = 0;
    for (auto name : names)
        n = std::max(n, name.size());
    return n;
}
constexpr std::size_t fatal_line_max =
    fatal_prefix.size() + longest(fatal_reasons) + 1 + longest(fatal_sites) + 1;
static_assert(fatal_line_max == 74); // r4: at most 74 bytes including LF, no NUL
// Present bit, reason and site in one word: publication is one release store.
constexpr std::uint32_t fatal_present = 0x10000;
struct Peer {
    std::atomic<bool> live{true};
    std::thread::id owner;
    U64 associations = 0;
    std::array<std::byte, 1024> control{};
};
struct Record {
    U64 id = 0, deadline = 0, unit = 0, pending = 0;
    std::shared_ptr<Peer> peer;
    RouteId route{};
    std::array<std::byte, 256> correlation{};
    std::size_t correlation_size = 0;
    std::unique_ptr<std::byte[]> input;
    U64 input_size = 0, input_charge = 0, reply_charge = 0;
    std::optional<OwnedReply> reply;
    bool announced = false, entered = false, terminal = false, withheld = false;
    bool input_charged = true, reply_charged = false, responding = false;
    bool body = false, callback = false, operation = false, ended = false;
    std::atomic<bool> abandoned{false};
    End end = End::idle;
    CommandView view() const {
        return {
            route, {correlation.data(), correlation_size}, {input.get(), std::size_t(input_size)}};
    }
};
static_assert(sizeof(Record) <= 512);
// Shared by every charged lease of one handed-off reply. `owners` counts those
// leases under Engine::mutex; the release that brings it to zero retires the
// association exactly once, independent of when each lease's pointer drops.
struct Response {
    std::shared_ptr<Record> record;
    U64 deadline;
    bool consumed = false;
    U64 owners = 1;
};
struct Engine : std::enable_shared_from_this<Engine> {
    QObject context;
    const std::thread::id owner = std::this_thread::get_id();
    Options options;
    // Declared before transport so the relay target outlives transport threads.
    std::atomic<std::uint32_t> fatal_word{0};
    std::unique_ptr<Transport> transport;
    std::unique_ptr<CommandDispatcher> dispatcher;
    std::unique_ptr<SqliteWriter> writer;
    obs::Hooks hooks;
    std::shared_ptr<const native_ws::ElapsedSource> elapsed;
    mutable std::mutex mutex;
    Inspection snapshot;
    std::vector<std::shared_ptr<Peer>> peers;
    std::vector<std::shared_ptr<Record>> records;
    std::shared_ptr<Record> active;
    std::shared_ptr<session_clock::detail::BindingState> clock_state;
    bool clock_claimed = false;
    U64 clock_unit = 0;
    std::atomic<bool> stop_requested{false}, wake_pending{false};
    std::atomic<U64> ordinal{0};
    U64 next_id = 0;
    std::optional<U64> owner_time, io_time;
    std::optional<std::thread::id> io_owner;
    const native_ws::ElapsedSource *profile_source = nullptr;
    std::optional<subathon::server::testing::WorkControlFault> cold_fault;
    bool started = false, transport_started = false, writer_stopping = false, failed = false;
    bool dispatching = false, writer_open = false;
    bool fatal_consumed = false; // Qt owner only
    std::function<void(std::shared_ptr<Record>)> execute, deliver;
    StartupObserver startup;
    Engine(Options o, std::unique_ptr<Transport> t, std::unique_ptr<CommandDispatcher> d)
        : options(std::move(o)), transport(std::move(t)), dispatcher(std::move(d)) {}
    void event(K kind, U64 command = 0, U64 unit = 0, std::optional<Failure> refusal = {},
               std::optional<SqliteWorkResult> work = {},
               std::optional<StorageError> admission = {}, std::optional<End> end = {}) {
        if (!hooks.observe)
            return;
        obs::Event e{kind,
                     ordinal.fetch_add(1) + 1,
                     command,
                     unit,
                     std::this_thread::get_id(),
                     {},
                     refusal,
                     admission,
                     end};
        // SqliteWriter revokes its ticket before invoking ANY completion. This
        // flag describes that structural callback invariant, not a new SQL probe.
        if (work)
            e.work =
                obs::WorkObservation{work->transaction, work->error, work->value.has_value(), true};
        hooks.observe(e);
    }
    std::optional<U64> sample(bool io = false) {
        if (!elapsed)
            return {};
        auto value = elapsed->sample();
        auto *n = std::get_if<native_ws::ElapsedInstant>(&value);
        auto &last = io ? io_time : owner_time;
        if (!n || (last && n->nanoseconds < *last))
            return {};
        last = n->nanoseconds;
        return *last;
    }
    static std::optional<U64> deadline(U64 now, U64 ms) {
        if (ms > max / 1000000 || now > max - ms * 1000000)
            return {};
        return now + ms * 1000000;
    }
    // A published fatal is a stop at every existing final gate (Correction 2).
    bool stopping() const {
        return stop_requested.load(std::memory_order_acquire) || detail::signal_pending() ||
               (fatal_word.load(std::memory_order_acquire) & fatal_present);
    }
    // The relay binding: any thread, first valid value wins, store only.
    static void publish_fatal(void *owner, bh::FatalElapsed fact) noexcept {
        const auto reason = std::uint32_t(fact.reason), site = std::uint32_t(fact.site);
        if (reason >= fatal_reasons.size() || site >= fatal_sites.size())
            return; // closed sets; no arbitrary fact can be published
        std::uint32_t none = 0;
        static_cast<Engine *>(owner)->fatal_word.compare_exchange_strong(
            none, fatal_present | reason << 8 | site, std::memory_order_release,
            std::memory_order_relaxed);
    }
    detail::FatalRelay fatal_relay() noexcept { return {this, &Engine::publish_fatal}; }
    std::optional<bh::FatalElapsed> published_fatal() const noexcept {
        const auto word = fatal_word.load(std::memory_order_acquire);
        if (!(word & fatal_present))
            return {};
        return bh::FatalElapsed{bh::FatalElapsedReason((word >> 8) & 0xff),
                                bh::FatalElapsedSite(word & 0xff)};
    }
    // Once, on Qt, outside Host locks. A refusal or absent sink keeps inspection.
    void report_fatal(bh::FatalElapsed fact) noexcept {
        const auto &sink = options.fatal_diagnostic;
        if (!sink || !sink->write)
            return;
        std::array<char, 96> line{};
        static_assert(fatal_line_max <= line.size());
        std::size_t size = 0;
        for (auto part :
             {fatal_prefix, fatal_reasons[std::size_t(fact.reason)], std::string_view("/"),
              fatal_sites[std::size_t(fact.site)], std::string_view("\n")}) {
            std::copy(part.begin(), part.end(), line.begin() + size);
            size += part.size();
        }
        (void)sink->write(sink->context, {line.data(), size});
    }
    // Record a published fatal once after the stop fence, then report it.
    void consume_fatal() {
        if (fatal_consumed)
            return;
        const auto fact = published_fatal();
        if (!fact)
            return;
        {
            std::lock_guard lock(mutex);
            if (snapshot.owner_cleanup)
                return;
            snapshot.fatal_elapsed = *fact;
        }
        fatal_consumed = true;
        failed = true;
        report_fatal(*fact);
    }
    // Test-only seams (testing::HostAccess); production never sets them.
    std::atomic<unsigned> post_faults{0};
    std::atomic<bool> post_fault_raises{false};
    std::function<void()> before_writer_open; // owner, one-shot
    bool registration_refused = false;        // owner
    unsigned timer_registrations = 0;         // owner
    // The queued owner-turn post. An injected fault refuses or throws instead.
    bool post_turn() {
        auto faults = post_faults.load();
        while (faults && !post_faults.compare_exchange_weak(faults, faults - 1)) {
        }
        if (faults) {
            if (post_fault_raises.load())
                throw testing::InjectedPostFault{};
            return false;
        }
        return QMetaObject::invokeMethod(
            &context,
            [this] {
                wake_pending.store(false);
                turn();
            },
            Qt::QueuedConnection);
    }
    void wake() {
        if (!wake_pending.exchange(true))
            (void)post_turn();
    }
    void request_stop() noexcept {
        stop_requested.store(true, std::memory_order_release);
        wake();
    }
    static void charge(PoolUsage &p, U64 bytes) {
        p.capacity += bytes;
        ++p.copies;
        p.metadata += 128;
        p.peak_charge = std::max(p.peak_charge, p.capacity + p.metadata);
    }
    static void release(PoolUsage &p, U64 bytes) {
        p.capacity -= bytes;
        --p.copies;
        p.metadata -= 128;
    }
    void release_input_locked(const std::shared_ptr<Record> &r) {
        if (!r->input_charged)
            return;
        release(snapshot.input, r->input_charge);
        r->input.reset();
        r->input_size = r->input_charge = 0;
        r->input_charged = false;
    }
    void release_record_locked(const std::shared_ptr<Record> &r) {
        auto it = std::find(records.begin(), records.end(), r);
        if (it == records.end())
            return;
        release_input_locked(r);
        --snapshot.associations;
        snapshot.association_charge -= 512;
        --r->peer->associations;
        if (!r->entered && r->route == RouteId::diagnostic)
            --snapshot.queued_diagnostics;
        it->reset();
    }
    void end(const std::shared_ptr<Record> &r, End reason) {
        if (clock_state && active == r)
            session_clock::detail::HostBindingAccess::leave_scope(*clock_state);
        r->terminal = true;
        r->end = reason;
        if (r->reply_charged) {
            std::lock_guard lock(mutex);
            release(snapshot.response, r->reply_charge);
            r->reply_charge = 0;
            r->reply_charged = false;
        }
        r->reply.reset();
    }
    void consume_stop() {
        if (!stopping())
            return;
        bool first = false;
        {
            std::lock_guard lock(mutex);
            if (snapshot.state != State::stopping && !snapshot.owner_cleanup) {
                snapshot.state = State::stopping;
                snapshot.storage_active = false;
                ++snapshot.stop_fences;
                first = true;
            }
        }
        if (first)
            fence_stop();
        consume_fatal();
    }
    void fence_stop() {
        if (clock_state)
            session_clock::detail::HostBindingAccess::revoke(*clock_state);
        started = true; // A consumed cold stop also consumes this process's attempt.
        // Fatal first: immediate closure, no elapsed read and no deadline (r4 §2).
        const auto now = published_fatal() ? std::nullopt : sample();
        const auto until = now ? deadline(*now, options.limits.drain_ms) : std::nullopt;
        {
            std::lock_guard lock(mutex);
            snapshot.drain_deadline = until;
        }
        event(K::stop_consumed);
        // A stop revokes forwarding/reply authority, not the handle's once-only
        // end. Accepted completions may still finish(nullopt) after this turn.
        if (transport_started) {
            transport->stop({until.value_or(0)}, [weak = weak_from_this()](NetworkBarrier b) {
                if (auto self = weak.lock())
                    QMetaObject::invokeMethod(
                        &self->context,
                        [self, b] {
                            if (!(b.callbacks_revoked && b.sockets_removed && b.framework_joined &&
                                  b.io_joined))
                                return;
                            self->event(K::callbacks_revoked);
                            self->event(K::sockets_removed);
                            self->event(K::framework_joined);
                            self->event(K::io_joined);
                            {
                                std::lock_guard lock(self->mutex);
                                self->snapshot.network_barrier = true;
                            }
                            self->event(K::network_ack);
                            self->wake();
                        },
                        Qt::QueuedConnection);
            });
        } else {
            std::lock_guard lock(mutex);
            snapshot.network_barrier = true;
        }
    }
    Failure fail_start(Failure failure = Failure::unavailable) {
        failed = true;
        stop_requested.store(true, std::memory_order_release);
        consume_stop();
        cleanup();
        return failure;
    }
    void cleanup() {
        if (!stopping() || active || dispatching)
            return;
        {
            std::lock_guard lock(mutex);
            if (std::any_of(records.begin(), records.end(),
                            [](const auto &r) { return r && !r->entered; }))
                return;
        }
        if (writer && !writer_open)
            writer.reset();
        if (writer && !writer_stopping) {
            writer_stopping = true;
            const auto accepted = writer->shutdown([this] {
                writer.reset();
                {
                    std::lock_guard lock(mutex);
                    snapshot.writer_barrier = true;
                }
                event(K::writer_removed);
                event(K::writer_ack);
                wake();
            });
            if (accepted != StorageError::none)
                failed = true;
        } else if (!writer) {
            std::lock_guard lock(mutex);
            snapshot.writer_barrier = true;
        }
        bool ready;
        {
            std::lock_guard lock(mutex);
            ready = snapshot.network_barrier && snapshot.writer_barrier &&
                    !snapshot.owner_cleanup && snapshot.associations == 0;
        }
        if (!ready)
            return;
        // r4 §3.3: Qt consumes any published fatal before terminal cleanup.
        consume_fatal();
        dispatcher.reset();
        event(K::dispatcher_destroyed);
        profile_source = nullptr;
        elapsed.reset();
        hooks.elapsed.reset();
        event(K::source_destroyed);
        std::optional<bh::FatalElapsed> late;
        {
            std::lock_guard lock(mutex);
            for (auto &p : peers) {
                if (p)
                    p->live.store(false);
                p.reset();
            }
            // A fatal published after the check above still decides the state here.
            if (!fatal_consumed && (late = published_fatal())) {
                snapshot.fatal_elapsed = *late;
                fatal_consumed = true;
                failed = true;
            }
            snapshot.control_charge = 0;
            snapshot.owner_cleanup = true;
            snapshot.state = failed ? State::failed : State::stopped;
        }
        if (late)
            report_fatal(*late);
        event(K::owner_cleanup);
        event(K::stopped);
    }
    void after_callback(const std::shared_ptr<Record> &r) {
        if (r->abandoned.load() && !r->terminal)
            end(r, End::abandoned);
        if (!r->terminal && !r->pending)
            end(r, stopping() ? End::stopping : End::idle);
        if (r->pending || r->callback || r->body)
            return;
        // A chosen reply becomes transport-owned only after normal callback return.
        if (r->reply) {
            if (stopping())
                end(r, End::stopping);
            else {
                try {
                    deliver(r);
                } catch (...) {
                    end(r, End::callback_exception);
                }
            }
        }
        if (!r->ended) {
            r->ended = true;
            event(K::command_ended, r->id, 0, {}, {}, {}, r->end);
        }
        {
            std::lock_guard lock(mutex);
            release_input_locked(r);
            if (!r->responding)
                release_record_locked(r);
        }
        if (active == r)
            active.reset();
        wake();
    }
    void turn() {
        consume_stop();
        // Announce retained admissions on Qt outside the lock, before entry.
        // Slot traversal is not a FIFO-order promise; dispatch selects by id.
        for (auto &slot : records) {
            std::shared_ptr<Record> r;
            {
                std::lock_guard lock(mutex);
                r = slot;
                if (!r || r->announced)
                    continue;
                r->announced = true;
            }
            event(r->route == RouteId::diagnostic ? K::diagnostic_queued : K::command_admitted,
                  r->id);
        }
        if (active) {
            if (!active->callback && !active->body)
                after_callback(active);
            cleanup();
            return;
        }
        std::shared_ptr<Record> r;
        {
            std::lock_guard lock(mutex);
            for (auto &candidate : records)
                if (candidate && !candidate->entered && (!r || candidate->id < r->id))
                    r = candidate;
        }
        if (!r) {
            cleanup();
            return;
        }
        consume_stop();
        std::optional<Failure> denied;
        const auto now = stopping() ? std::nullopt : sample();
        bool elapsed_failed = false;
        {
            // This is the entry linearization point, shared with IO disconnect.
            // Sampling/callbacks happen outside this lock; all current gates are
            // rechecked before committing entry and diagnostic counter transfer.
            std::lock_guard lock(mutex);
            const bool retained = std::find(records.begin(), records.end(), r) != records.end();
            if (stopping())
                denied = Failure::stopping;
            else if (!retained || !r->peer->live.load())
                denied = Failure::disconnected;
            else if (!now) {
                denied = Failure::elapsed_unavailable;
                elapsed_failed = true;
                r->peer->live.store(false);
            } else if (*now >= r->deadline)
                denied = Failure::expired;
            else if (r->route != RouteId::diagnostic &&
                     (!snapshot.storage_active || r->route != RouteId::installation))
                denied = Failure::unavailable;
            if (!denied) {
                r->entered = true;
                if (r->route == RouteId::diagnostic)
                    --snapshot.queued_diagnostics;
            }
        }
        if (elapsed_failed)
            revoke_peer(r->peer);
        if (denied) {
            event(r->route == RouteId::diagnostic ? K::diagnostic_cancelled : K::command_cancelled,
                  r->id, 0, denied);
            {
                std::lock_guard lock(mutex);
                release_record_locked(r);
            }
            wake();
            return;
        }
        active = r;
        r->callback = true;
        dispatching = true;
        if (r->route == RouteId::diagnostic) {
            event(K::diagnostic_entered, r->id);
            try {
                auto reply = dispatcher->diagnostic(r->view());
                if (auto f = choose_reply(r, std::move(reply)))
                    event(K::operation_refused, r->id, 0, f);
            } catch (...) {
                end(r, End::callback_exception);
            }
        } else {
            event(K::command_entered, r->id);
            try {
                execute(r);
            } catch (...) {
                end(r, End::callback_exception);
            }
        }
        r->callback = false;
        dispatching = false;
        consume_stop();
        after_callback(r);
        cleanup();
    }
    void revoke_peer(const std::shared_ptr<Peer> &peer) {
        {
            std::lock_guard lock(mutex);
            peer->live.store(false);
        }
        if (auto *composition = dynamic_cast<detail::TransportComposition *>(transport.get()))
            composition->close_peer(peer.get());
    }
    std::optional<Failure> response_gate(const std::shared_ptr<Record> &r) {
        if (stopping())
            return Failure::stopping;
        if (!r->peer->live.load())
            return Failure::disconnected;
        if (!sample()) {
            revoke_peer(r->peer);
            return Failure::elapsed_unavailable;
        }
        return {};
    }
    std::optional<Failure> choose_reply(const std::shared_ptr<Record> &r, OwnedReply reply) {
        if (auto f = response_gate(r))
            return f;
        if (r->withheld)
            return Failure::invalid_completion;
        {
            std::lock_guard lock(mutex);
            if (stopping())
                return Failure::stopping;
            if (!r->peer->live.load())
                return Failure::disconnected;
            const auto size = reply.bytes.capacity();
            if (reply.bytes.size() > options.limits.response_bytes ||
                size > options.limits.response_capacity - snapshot.response.capacity ||
                snapshot.response.copies >= options.limits.response_slots)
                return Failure::resource_limit;
            charge(snapshot.response, size);
            r->reply_charge = size;
            r->reply_charged = true;
        }
        r->reply = std::move(reply);
        r->terminal = true;
        r->end = End::finished;
        return {};
    }
    ForwardResult forward(const std::shared_ptr<Record> &r, SqliteWriter::Work body,
                          SqliteWriter::WorkCompletion done) {
        if (r->operation)
            return Failure::invalid_completion;
        if (r->terminal) {
            event(K::forward_refused, r->id, 0, Failure::invalid_completion);
            return Failure::invalid_completion;
        }
        if (r->body) {
            end(r, End::premature_finish);
            event(K::forward_refused, r->id, 0, Failure::invalid_completion);
            return Failure::invalid_completion;
        }
        consume_stop();
        if (stopping()) {
            end(r, End::stopping);
            event(K::forward_refused, r->id, 0, Failure::stopping);
            return Failure::stopping;
        }
        if (!r->callback)
            return Failure::invalid_completion;
        r->operation = true;
        const U64 unit = ++r->unit;
        const auto error = writer->enqueue_work(
            [this, r, unit, body = std::move(body)](auto c) {
                r->body = true;
                event(K::unit_entered, r->id, unit);
                clock_unit = unit;
                if (clock_state)
                    session_clock::detail::HostBindingAccess::enter_body(*clock_state, r.get(),
                                                                         unit, c);
                try {
                    auto value = body(c);
                    if (clock_state)
                        session_clock::detail::HostBindingAccess::leave_scope(*clock_state);
                    clock_unit = 0;
                    r->body = false;
                    return value;
                } catch (...) {
                    if (clock_state)
                        session_clock::detail::HostBindingAccess::leave_scope(*clock_state);
                    clock_unit = 0;
                    r->body = false;
                    throw;
                }
            },
            [this, r, unit, done = std::move(done)](SqliteWorkResult result) {
                --r->pending;
                {
                    std::lock_guard lock(mutex);
                    --snapshot.accepted_units;
                    if (result.transaction == TransactionOutcome::outcome_unknown)
                        snapshot.storage_active = false;
                }
                if (result.transaction == TransactionOutcome::outcome_unknown ||
                    (result.transaction == TransactionOutcome::committed &&
                     (result.error != WorkError::none || !result.value)))
                    r->withheld = true;
                r->callback = true;
                event(K::writer_completed, r->id, unit, {}, result);
                clock_unit = unit;
                if (clock_state)
                    session_clock::detail::HostBindingAccess::enter_completion(
                        *clock_state, r.get(), unit, result);
                try {
                    done(std::move(result));
                } catch (...) {
                    end(r, End::callback_exception);
                }
                if (clock_state)
                    session_clock::detail::HostBindingAccess::leave_scope(*clock_state);
                clock_unit = 0;
                r->callback = false;
                consume_stop();
                after_callback(r);
                cleanup();
            });
        r->operation = false;
        if (error == StorageError::none) {
            ++r->pending;
            std::lock_guard lock(mutex);
            ++snapshot.accepted_units;
        } else {
            end(r, End::writer_refused);
            event(K::forward_refused, r->id, unit, {}, {}, error);
        }
        return error;
    }
    Result<Admitted> finish(const std::shared_ptr<Record> &r, std::optional<OwnedReply> reply) {
        if (r->operation)
            return Failure::invalid_completion;
        if (r->terminal) {
            event(K::finish_refused, r->id, 0, Failure::invalid_completion);
            return Failure::invalid_completion;
        }
        if (r->body || r->pending) {
            end(r, End::premature_finish);
            event(K::finish_refused, r->id, 0, Failure::invalid_completion);
            return Failure::invalid_completion;
        }
        r->operation = true;
        struct Leave {
            bool &flag;
            ~Leave() { flag = false; }
        } leave{r->operation};
        consume_stop();
        if (reply) {
            if (auto f = choose_reply(r, std::move(*reply))) {
                end(r, End::reply_withheld);
                event(K::finish_refused, r->id, 0, f);
                return *f;
            }
        } else {
            r->terminal = true;
            r->end = End::finished;
        }
        if (clock_state && active == r)
            session_clock::detail::HostBindingAccess::leave_scope(*clock_state);
        return Admitted{};
    }
};
} // namespace
struct PeerLease::Impl {
    std::weak_ptr<Engine> engine;
    std::shared_ptr<Peer> peer;
};
struct Ingress::Impl {
    std::shared_ptr<Engine> engine;
};
struct CommandHandle::Impl {
    std::weak_ptr<Engine> engine;
    std::weak_ptr<Record> record;
    std::thread::id owner;
    ~Impl() {
        if (auto r = record.lock())
            r->abandoned.store(true);
    }
};
struct ResponseLease::Impl {
    std::weak_ptr<Engine> engine;
    std::shared_ptr<Response> response;
    std::vector<std::byte> bytes;
    std::unique_ptr<std::byte[]> copy;
    U64 copy_size = 0, byte_charge = 0;
    bool charged = false;
    std::span<const std::byte> payload() const {
        return copy ? std::span<const std::byte>(copy.get(), std::size_t(copy_size))
                    : std::span<const std::byte>(bytes);
    }
    ~Impl() {
        if (charged)
            if (auto e = engine.lock()) {
                {
                    std::lock_guard lock(e->mutex);
                    Engine::release(e->snapshot.response, byte_charge);
                    if (--response->owners == 0)
                        e->release_record_locked(response->record);
                }
                e->wake();
            }
    }
};
// Qt owner timer (r4 §2 "Delivery and wake"). The fatal relay never posts, so one
// repeating 10 ms tick drives the existing owner turn while a published fatal
// persists; other ticks do no lifecycle work. Host::Impl owns it so it is armed,
// stopped and destroyed on the owner, before the Engine it borrows.
struct OwnerTimer final : QObject {
    Engine &engine;
    QBasicTimer timer;
    explicit OwnerTimer(Engine &e) : engine(e) {}
    bool arm() {
        if (!timer.isActive() && !engine.registration_refused &&
            QAbstractEventDispatcher::instance()) {
            timer.start(std::chrono::milliseconds(10), this);
            if (timer.isActive())
                ++engine.timer_registrations;
        }
        return timer.isActive();
    }
    void timerEvent(QTimerEvent *event) override {
        if (event->id() != timer.id())
            return QObject::timerEvent(event);
        bool cleaned;
        {
            std::lock_guard lock(engine.mutex);
            cleaned = engine.snapshot.owner_cleanup;
        }
        if (cleaned)
            timer.stop();
        else if (engine.published_fatal())
            engine.turn();
    }
};
struct Host::Impl {
    std::shared_ptr<Engine> engine;
    Ingress ingress;
    OwnerTimer timer{*engine};
    Impl(Options o, std::unique_ptr<Transport> t, std::unique_ptr<CommandDispatcher> d)
        : engine(std::make_shared<Engine>(std::move(o), std::move(t), std::move(d))),
          ingress(std::make_unique<Ingress::Impl>(Ingress::Impl{engine})) {
        timer.arm(); // cold setup; start() refuses if it is still unarmed
        engine->execute = [this](auto r) {
            auto h = std::make_unique<CommandHandle::Impl>();
            h->engine = engine;
            h->record = r;
            h->owner = engine->owner;
            engine->dispatcher->execute(CommandHandle(std::move(h)), r->view());
        };
        engine->deliver = [this](auto r) {
            auto &e = *engine;
            if (auto f = e.response_gate(r)) {
                e.event(K::operation_refused, r->id, 0, f);
                e.end(r, End::reply_withheld);
                return;
            }
            auto now = e.sample();
            auto until = now ? Engine::deadline(*now, e.options.limits.handoff_ms) : std::nullopt;
            if (!until) {
                e.revoke_peer(r->peer);
                e.event(K::operation_refused, r->id, 0, Failure::elapsed_unavailable);
                e.end(r, End::elapsed_unavailable);
                return;
            }
            auto lease = std::make_unique<ResponseLease::Impl>();
            lease->engine = engine;
            lease->response = std::make_shared<Response>(Response{r, *until});
            lease->bytes = std::move(r->reply->bytes);
            lease->byte_charge = r->reply_charge;
            lease->charged = true;
            {
                std::lock_guard lock(e.mutex);
                r->responding = true;
                r->reply_charged = false;
                r->reply_charge = 0;
            }
            r->reply.reset();
            if (auto *composition = dynamic_cast<detail::TransportComposition *>(e.transport.get()))
                composition->handoff_peer(r->peer.get(), ResponseLease(std::move(lease)));
            else
                e.transport->handoff(ResponseLease(std::move(lease)));
        };
    }
};
Host::Host(Options o, std::unique_ptr<Transport> t, std::unique_ptr<CommandDispatcher> d)
    : impl_(std::make_unique<Impl>(std::move(o), std::move(t), std::move(d))) {}
Host::~Host() {
    if (impl_->engine->clock_state)
        session_clock::detail::HostBindingAccess::revoke(*impl_->engine->clock_state);
}
Result<StartAccepted> Host::start(StartupObserver observer) {
    auto &e = *impl_->engine;
    if (e.owner != std::this_thread::get_id())
        return Failure::wrong_thread;
    if (e.started)
        return Failure::already_started;
    e.started = true;
    const auto &l = e.options.limits;
    bool valid = e.transport && e.dispatcher && !e.options.store_path.empty() &&
                 !e.options.installation.empty();
    for (auto n : {l.associations, l.per_peer, l.peers, l.envelope_bytes, l.input_capacity,
                   l.response_bytes, l.response_capacity, l.input_slots, l.response_slots,
                   l.queue_ms, l.handoff_ms, l.drain_ms})
        valid = valid && n && n <= U64(PTRDIFF_MAX) / 2048;
    valid = valid && l.per_peer <= l.associations && l.envelope_bytes <= l.input_capacity &&
            l.response_bytes <= l.response_capacity &&
            e.options.writer_limits.pending_batches > 0 &&
            U64(e.options.writer_limits.pending_batches) >= l.associations;
    if (!valid)
        return Failure::invalid_options;
    if (!impl_->timer.arm())
        return e.fail_start();
    try {
        e.elapsed = e.hooks.elapsed;
        if (!e.elapsed) {
            auto source = make_linux_monotonic_elapsed();
            if (auto *p = std::get_if<std::shared_ptr<const native_ws::ElapsedSource>>(&source))
                e.elapsed = *p;
            else
                return e.fail_start(Failure::elapsed_unavailable);
        }
        auto profile = native_ws::create_profile(e.options.native, e.elapsed);
        if (!std::holds_alternative<std::unique_ptr<native_ws::Profile>>(profile))
            return e.fail_start(std::holds_alternative<native_ws::InvalidConfiguration>(profile)
                                    ? Failure::invalid_options
                                    : Failure::unavailable);
        e.profile_source = e.elapsed.get();
        e.peers.resize(std::size_t(l.peers));
        e.records.resize(std::size_t(l.associations));
        e.startup = std::move(observer);
        {
            std::lock_guard lock(e.mutex);
            e.snapshot.state = State::starting;
        }
        if (auto *composition = dynamic_cast<detail::TransportComposition *>(e.transport.get())) {
            composition->configure(e.elapsed, l);
            composition->bind_fatal(e.fatal_relay());
        }
        if (auto hook = std::exchange(e.before_writer_open, {}))
            hook();
        if (e.stopping()) {
            e.wake();
            return StartAccepted{};
        }
        e.writer = std::make_unique<SqliteWriter>();
        if (e.writer->open(QString::fromStdString(e.options.store_path), e.options.writer_limits) !=
            StorageError::none) {
            return e.fail_start();
        }
        e.writer_open = true;
        if (e.cold_fault) {
            (void)subathon::server::testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                *e.writer, *e.cold_fault);
            e.cold_fault.reset();
        }
        e.transport_started = true;
        auto accepted = e.transport->start(
            impl_->ingress, std::move(std::get<std::unique_ptr<native_ws::Profile>>(profile)),
            [weak = e.weak_from_this()](auto result) {
                if (auto s = weak.lock())
                    QMetaObject::invokeMethod(
                        &s->context,
                        [s, result] {
                            s->consume_stop();
                            if (s->stopping())
                                return;
                            if (!std::holds_alternative<StartAccepted>(result)) {
                                try {
                                    if (s->startup)
                                        s->startup(Failure::unavailable);
                                } catch (...) {
                                }
                                s->failed = true;
                                s->request_stop();
                                return;
                            }
                            s->event(K::listener_ready);
                            try {
                                if (s->startup)
                                    s->startup(StartAccepted{});
                            } catch (...) {
                                s->failed = true;
                                s->request_stop();
                                return;
                            }
                            if (!s->stopping()) {
                                std::lock_guard lock(s->mutex);
                                s->snapshot.state = State::serving;
                            }
                        },
                        Qt::QueuedConnection);
            });
        if (!std::holds_alternative<StartAccepted>(accepted))
            return e.fail_start();
        const auto manifest = reference_authority_repository_manifest();
        const auto head = manifest.steps.back().version;
        auto admission = SqliteMigrationRunner::enqueue(
            *e.writer, {MigrationMode::inspect, manifest, {}, {}}, [&e, head](MigrationResult r) {
                const bool current = r.work.transaction == TransactionOutcome::committed &&
                                     r.work.error == WorkError::none && r.work.value &&
                                     r.reason == MigrationReason::none && r.report &&
                                     r.report->state == CatalogState::current &&
                                     r.report->version == head &&
                                     r.report->target_version == head && r.report->applied == 0;
                e.event(K::inspection_completed, 0, 0,
                        current ? std::nullopt : std::optional{Failure::unavailable}, r.work);
                e.consume_stop();
                if (e.stopping())
                    return;
                if (!current) {
                    e.event(K::operation_refused, 0, 0, Failure::unavailable);
                    return;
                }
                const auto a = e.writer->enqueue_work(
                    [&e](auto context) -> QVariant {
                        auto session = context.begin();
                        try {
                            SqliteAccountSessionUnit unit(session, {}, e.options.store_limits);
                            auto facts = unit.read_installation(e.options.installation);
                            if (!std::holds_alternative<StoredInstallationFacts>(facts)) {
                                session.rollback();
                                return false;
                            }
                            const auto &f = std::get<StoredInstallationFacts>(facts);
                            if (f.current.installation_ref != e.options.installation ||
                                f.current.session_epoch_ref.empty() || f.revision_ref.empty()) {
                                session.rollback();
                                return false;
                            }
                            session.commit();
                            return true;
                        } catch (...) {
                            (void)session.cleanup();
                            throw;
                        }
                    },
                    [&e](SqliteWorkResult result) {
                        const bool installation_ok =
                            result.transaction == TransactionOutcome::committed &&
                            result.error == WorkError::none && result.value &&
                            result.value->toBool();
                        e.event(K::installation_read_completed, 0, 0,
                                installation_ok ? std::nullopt
                                                : std::optional{Failure::unavailable},
                                result);
                        e.consume_stop();
                        if (!e.stopping() && installation_ok) {
                            {
                                std::lock_guard lock(e.mutex);
                                e.snapshot.storage_active = true;
                            }
                            e.event(K::activated);
                        } else if (!e.stopping())
                            e.event(K::operation_refused, 0, 0, Failure::unavailable);
                    });
                if (a == StorageError::none)
                    e.event(K::installation_read_enqueued);
                else
                    e.event(K::operation_refused, 0, 0, Failure::unavailable);
            });
        if (admission.error == StorageError::none)
            e.event(K::inspection_enqueued);
        else
            e.event(K::operation_refused, 0, 0, Failure::unavailable);
        return StartAccepted{};
    } catch (...) {
        return e.fail_start();
    }
}
void Host::request_stop() noexcept { impl_->engine->request_stop(); }
Inspection Host::inspect() const {
    auto &e = *impl_->engine;
    std::lock_guard lock(e.mutex);
    return e.snapshot;
}
Ingress::Ingress(std::unique_ptr<Impl> p) : impl_(std::move(p)) {}
Ingress::~Ingress() = default;
Result<PeerLease> Ingress::connected() {
    auto &e = *impl_->engine;
    if (e.stopping())
        return Failure::stopping;
    std::shared_ptr<Peer> peer;
    std::unique_ptr<PeerLease::Impl> lease;
    {
        std::lock_guard lock(e.mutex);
        if (e.io_owner && *e.io_owner != std::this_thread::get_id())
            return Failure::wrong_thread;
        if (e.stopping())
            return Failure::stopping;
        if (e.snapshot.state != State::serving)
            return Failure::unavailable;
        auto it = std::find(e.peers.begin(), e.peers.end(), nullptr);
        if (it == e.peers.end())
            return Failure::resource_limit;
        try {
            peer = std::make_shared<Peer>();
            peer->owner = std::this_thread::get_id();
            lease = std::make_unique<PeerLease::Impl>(PeerLease::Impl{e.weak_from_this(), peer});
        } catch (...) {
            return Failure::resource_limit;
        }
        e.io_owner = peer->owner;
        *it = peer;
        e.snapshot.control_charge += 1024;
    }
    if (auto *composition = dynamic_cast<detail::TransportComposition *>(e.transport.get()))
        composition->peer_connected(peer.get());
    return PeerLease(std::move(lease));
}
Result<Admitted> Ingress::try_submit(const PeerLease &p, std::span<const std::byte> correlation,
                                     RouteId route, std::span<const std::byte> bytes) {
    auto &e = *impl_->engine;
    if (!p.impl_ || p.impl_->peer->owner != std::this_thread::get_id())
        return Failure::wrong_thread;
    if (p.impl_->engine.lock().get() != &e)
        return Failure::unavailable;
    auto peer = p.impl_->peer;
    std::optional<Failure> refusal;
    if (e.stopping())
        refusal = Failure::stopping;
    else if (!peer->live.load())
        refusal = Failure::disconnected;
    std::optional<U64> until;
    if (!refusal) {
        auto now = e.sample(true);
        until = now ? Engine::deadline(*now, e.options.limits.queue_ms) : std::nullopt;
        if (!until) {
            refusal = Failure::elapsed_unavailable;
            e.revoke_peer(peer);
        }
    }
    if (!refusal) {
        std::lock_guard lock(e.mutex);
        const auto &l = e.options.limits;
        if (e.stopping())
            refusal = Failure::stopping;
        else if (!peer->live.load())
            refusal = Failure::disconnected;
        else if (e.snapshot.state != State::serving ||
                 (route != RouteId::diagnostic &&
                  (!e.snapshot.storage_active || route != RouteId::installation)))
            refusal = Failure::unavailable;
        else if (correlation.size() > 256 || bytes.size() > l.envelope_bytes ||
                 bytes.size() > l.input_capacity - e.snapshot.input.capacity ||
                 e.snapshot.input.copies >= l.input_slots ||
                 e.snapshot.associations >= l.associations || peer->associations >= l.per_peer)
            refusal = Failure::resource_limit;
        else {
            auto r = std::make_shared<Record>();
            r->id = ++e.next_id;
            r->deadline = *until;
            r->peer = peer;
            r->route = route;
            r->correlation_size = correlation.size();
            std::copy(correlation.begin(), correlation.end(), r->correlation.begin());
            Engine::charge(e.snapshot.input, bytes.size());
            try {
                r->input_charge = r->input_size = bytes.size();
                r->input = std::make_unique<std::byte[]>(bytes.size());
                std::copy(bytes.begin(), bytes.end(), r->input.get());
            } catch (...) {
                Engine::release(e.snapshot.input, bytes.size());
                return Failure::resource_limit;
            }
            auto it = std::find(e.records.begin(), e.records.end(), nullptr);
            *it = std::move(r);
            ++e.snapshot.associations;
            e.snapshot.association_charge += 512;
            ++peer->associations;
            if (route == RouteId::diagnostic)
                ++e.snapshot.queued_diagnostics;
        }
    }
    if (refusal) {
        e.event(K::ingress_refused, 0, 0, refusal);
        return *refusal;
    }
    e.wake();
    return Admitted{};
}
void Ingress::disconnected(const PeerLease &p) noexcept {
    if (!p.impl_ || p.impl_->peer->owner != std::this_thread::get_id())
        return;
    auto &e = *impl_->engine;
    const auto peer = p.impl_->peer;
    if (p.impl_->engine.lock().get() != &e)
        return;
    {
        std::lock_guard lock(e.mutex);
        peer->live.store(false);
        for (auto &slot : e.records) {
            auto r = slot;
            if (r && r->peer == peer && !r->entered)
                e.release_record_locked(r);
        }
        for (auto &slot : e.peers)
            if (slot == peer) {
                slot.reset();
                e.snapshot.control_charge -= 1024;
            }
    }
    e.wake();
}
PeerLease::PeerLease(std::unique_ptr<Impl> p) : impl_(std::move(p)) {}
PeerLease::PeerLease(PeerLease &&) noexcept = default;
PeerLease &PeerLease::operator=(PeerLease &&) noexcept = default;
PeerLease::~PeerLease() = default;
CommandHandle::CommandHandle(std::unique_ptr<Impl> p) : impl_(std::move(p)) {}
CommandHandle::CommandHandle(CommandHandle &&) noexcept = default;
CommandHandle &CommandHandle::operator=(CommandHandle &&) noexcept = default;
CommandHandle::~CommandHandle() = default;
Result<CommandView> CommandHandle::input() const {
    if (!impl_)
        return Failure::invalid_completion;
    if (impl_->owner != std::this_thread::get_id())
        return Failure::wrong_thread;
    auto r = impl_->record.lock();
    auto e = impl_->engine.lock();
    if (!r || !e || (!r->callback && !r->body))
        return Failure::invalid_completion;
    return r->view();
}
ForwardResult CommandHandle::forward(SqliteWriter::Work w, SqliteWriter::WorkCompletion c) {
    if (!impl_)
        return Failure::invalid_completion;
    if (impl_->owner != std::this_thread::get_id())
        return Failure::wrong_thread;
    auto r = impl_->record.lock();
    auto e = impl_->engine.lock();
    if (!r || !e)
        return Failure::invalid_completion;
    return e->forward(r, std::move(w), std::move(c));
}
Result<Admitted> CommandHandle::finish(std::optional<OwnedReply> reply) {
    if (!impl_)
        return Failure::invalid_completion;
    if (impl_->owner != std::this_thread::get_id())
        return Failure::wrong_thread;
    auto r = impl_->record.lock();
    auto e = impl_->engine.lock();
    if (!r || !e)
        return Failure::invalid_completion;
    return e->finish(r, std::move(reply));
}
ResponseLease::ResponseLease(std::unique_ptr<Impl> p) : impl_(std::move(p)) {}
ResponseLease::ResponseLease(ResponseLease &&) noexcept = default;
ResponseLease &ResponseLease::operator=(ResponseLease &&) noexcept = default;
ResponseLease::~ResponseLease() = default;
Result<CommandView> ResponseLease::take() {
    if (!impl_)
        return Failure::invalid_completion;
    auto &r = *impl_->response;
    auto e = impl_->engine.lock();
    if (r.record->peer->owner != std::this_thread::get_id())
        return Failure::wrong_thread;
    if (!e)
        return Failure::unavailable;
    if (r.consumed)
        return Failure::invalid_completion;
    if (!r.record->peer->live.load())
        return Failure::disconnected;
    auto now = e->sample(true);
    if (!now) {
        e->revoke_peer(r.record->peer);
        return Failure::elapsed_unavailable;
    }
    if (*now >= r.deadline)
        return Failure::expired;
    {
        std::lock_guard lock(e->mutex);
        if (r.consumed)
            return Failure::invalid_completion;
        if (!r.record->peer->live.load())
            return Failure::disconnected;
        r.consumed = true;
    }
    e->event(K::local_handoff, r.record->id);
    return CommandView{r.record->route,
                       {r.record->correlation.data(), r.record->correlation_size},
                       impl_->payload()};
}
Result<ResponseLease> ResponseLease::try_copy() const {
    if (!impl_)
        return Failure::invalid_completion;
    auto e = impl_->engine.lock();
    auto &r = *impl_->response;
    if (r.record->peer->owner != std::this_thread::get_id())
        return Failure::wrong_thread;
    if (!e || r.consumed)
        return Failure::invalid_completion;
    if (!r.record->peer->live.load())
        return Failure::disconnected;
    auto now = e->sample(true);
    if (!now) {
        e->revoke_peer(r.record->peer);
        return Failure::elapsed_unavailable;
    }
    if (*now >= r.deadline)
        return Failure::expired;
    const auto payload = impl_->payload();
    const auto byte_charge = U64(payload.size());
    {
        std::lock_guard lock(e->mutex);
        auto &pool = e->snapshot.response;
        if (!r.record->peer->live.load())
            return Failure::disconnected;
        if (r.consumed)
            return Failure::invalid_completion;
        if (byte_charge > e->options.limits.response_capacity - pool.capacity ||
            pool.copies >= e->options.limits.response_slots)
            return Failure::resource_limit;
        Engine::charge(pool, byte_charge);
        ++r.owners;
    }
    std::unique_ptr<Impl> p;
    try {
        p = std::make_unique<Impl>();
        p->engine = e;
        p->response = impl_->response;
        p->byte_charge = p->copy_size = byte_charge;
        p->charged = true;
        p->copy = std::make_unique<std::byte[]>(std::size_t(byte_charge));
        std::copy(payload.begin(), payload.end(), p->copy.get());
    } catch (...) {
        if (!p) {
            // The live source still owns the association, so this never retires.
            std::lock_guard lock(e->mutex);
            Engine::release(e->snapshot.response, byte_charge);
            --r.owners;
        }
        return Failure::resource_limit;
    }
    return ResponseLease(std::move(p));
}
void ResponseLease::reset() noexcept { impl_.reset(); }
std::unique_ptr<Host> testing::HostAccess::create(Options o, std::unique_ptr<Transport> t,
                                                  std::unique_ptr<CommandDispatcher> d, Hooks h) {
    auto p = std::make_unique<Host>(std::move(o), std::move(t), std::move(d));
    p->impl_->engine->hooks = std::move(h);
    return p;
}
Result<Connected>
testing::HostAccess::inject_next_control_ack_loss(Host &h,
                                                  subathon::server::testing::WorkControlFault f) {
    auto &e = *h.impl_->engine;
    if (e.owner != std::this_thread::get_id())
        return Failure::wrong_thread;
    if (!e.started) {
        if (e.cold_fault)
            return Failure::invalid_completion;
        e.cold_fault = f;
        return Connected{};
    }
    if (!e.writer || e.active)
        return Failure::unavailable;
    return subathon::server::testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
               *e.writer, f) == StorageError::none
               ? Result<Connected>{Connected{}}
               : Result<Connected>{Failure::invalid_completion};
}
void testing::HostAccess::inject_post_faults(Host &h, PostFault fault, unsigned count) noexcept {
    auto &e = *h.impl_->engine;
    e.post_fault_raises.store(fault == PostFault::raise);
    e.post_faults.store(count);
}
Result<Connected> testing::HostAccess::refuse_timer_registration(Host &h) {
    auto &e = *h.impl_->engine;
    if (e.owner != std::this_thread::get_id())
        return Failure::wrong_thread;
    if (e.started)
        return Failure::already_started;
    h.impl_->timer.timer.stop();
    e.registration_refused = true;
    return Connected{};
}
Result<Connected> testing::HostAccess::before_writer_open(Host &h, std::function<void()> hook) {
    auto &e = *h.impl_->engine;
    if (e.owner != std::this_thread::get_id())
        return Failure::wrong_thread;
    if (e.started)
        return Failure::already_started;
    if (e.before_writer_open || !hook)
        return Failure::invalid_completion;
    e.before_writer_open = std::move(hook);
    return Connected{};
}
testing::OwnerTimerObservation testing::HostAccess::owner_timer(const Host &h) {
    return {h.impl_->timer.timer.isActive(), h.impl_->engine->timer_registrations};
}
void testing::HostAccess::publish_fatal(Host &h, bh::FatalElapsed fact) noexcept {
    const auto relay = h.impl_->engine->fatal_relay();
    relay.publish(relay.owner, fact);
}
testing::ElapsedIdentity testing::HostAccess::elapsed_identity(const Host &h) {
    const auto *source = h.impl_->engine->elapsed.get();
    // Profile identity is the actual create_profile argument recorded at construction,
    // not a sampled clock observation or a query of a fabricated profile facade.
    return {source, h.impl_->engine->profile_source};
}
} // namespace subathon::server::api_host

namespace subathon::server::session_clock::detail {
bool HostBindingAccess::attach(api_host::Host &host, const std::shared_ptr<BindingState> &state) {
    auto engine = host.impl_->engine;
    if (engine->owner != std::this_thread::get_id() || !state || state->owner != engine->owner ||
        engine->clock_claimed || engine->stopping())
        return false;
    // No native factory/monitor is constructed here, including for a second service.
    // The lambda borrows no Host and cannot keep Engine alive after destruction.
    state->eligible = [weak = std::weak_ptr(engine)](const void *record, std::uint64_t unit,
                                                     ScopeKind kind) {
        auto e = weak.lock();
        if (!e || e->owner != std::this_thread::get_id() || e->stopping() ||
            !e->snapshot.storage_active || !e->active || e->active.get() != record ||
            e->clock_unit != unit)
            return false;
        const auto &r = *e->active;
        if (r.terminal || r.abandoned.load())
            return false;
        return kind == ScopeKind::body
                   ? r.body && r.pending > 0
                   : r.callback && !r.body && r.pending == 0 && unit == r.unit && !r.withheld;
    };
    engine->clock_claimed = true;
    engine->clock_state = state;
    return true;
}
void HostBindingAccess::leave_scope(BindingState &state) noexcept {
    if (state.scope)
        state.scope->live.store(false);
    state.scope.reset();
}
void HostBindingAccess::revoke(BindingState &state) noexcept {
    state.alive.store(false);
    leave_scope(state);
    state.source.reset();
}
void HostBindingAccess::enter_body(BindingState &state, const void *record, std::uint64_t unit,
                                   const SqliteWorkContext &context) {
    leave_scope(state);
    try {
        if (state.owner != std::this_thread::get_id() || !state.alive.load() ||
            !state.eligible(record, unit, ScopeKind::body) || context.state_.expired())
            return;
        auto scope = std::make_shared<ScopeState>(ScopeKind::body, record, unit);
        scope->work = context.state_;
        state.scope = std::move(scope);
    } catch (...) { /* Clock resource failure cannot change the writer's outcome. */
    }
}
void HostBindingAccess::enter_completion(BindingState &state, const void *record,
                                         std::uint64_t unit, const SqliteWorkResult &result) {
    leave_scope(state);
    try {
        if (state.owner != std::this_thread::get_id() || !state.alive.load() ||
            result.transaction != TransactionOutcome::committed ||
            result.error != WorkError::none || !result.value ||
            !state.eligible(record, unit, ScopeKind::completion))
            return;
        state.scope = std::make_shared<ScopeState>(ScopeKind::completion, record, unit);
    } catch (...) { /* No publication authority on failed allocation. */
    }
}
std::weak_ptr<ScopeState> HostBindingAccess::body(const BindingState &state,
                                                  const SqliteWorkContext &context) {
    if (state.owner != std::this_thread::get_id() || !state.alive.load() || state.sampling)
        return {};
    const auto &scope = state.scope;
    if (!scope || !scope->live.load() || scope->kind != ScopeKind::body ||
        context.state_.expired() || scope->work.expired() ||
        context.state_.owner_before(scope->work) || scope->work.owner_before(context.state_) ||
        !state.eligible(scope->record, scope->unit, scope->kind))
        return {};
    return scope;
}
std::weak_ptr<ScopeState> HostBindingAccess::completion(const BindingState &state,
                                                        const api_host::CommandHandle &command) {
    if (state.owner != std::this_thread::get_id() || !state.alive.load() || state.sampling)
        return {};
    const auto &scope = state.scope;
    if (!scope || !scope->live.load() || scope->kind != ScopeKind::completion || !command.impl_ ||
        command.impl_->owner != state.owner)
        return {};
    const auto record = command.impl_->record.lock();
    const auto engine = command.impl_->engine.lock();
    if (!record || !engine || record.get() != scope->record ||
        engine->clock_state.get() != &state ||
        !state.eligible(scope->record, scope->unit, scope->kind))
        return {};
    return scope;
}
} // namespace subathon::server::session_clock::detail
