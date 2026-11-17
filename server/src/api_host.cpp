// SPDX-License-Identifier: GPL-3.0-or-later
#include "api_host_internal.hpp"
#include <QCoreApplication>
#include <QThread>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <limits>
#include <mutex>
#include <subathon/server/api_host.hpp>
#include <subathon/server/api_host_elapsed.hpp>
#include <subathon/server/sqlite_reference_authority.hpp>
#include <subathon/server/testing/api_host_observers.hpp>
#include <thread>

namespace subathon::server::api_host {
namespace {
using U64 = std::uint64_t;
namespace obs = testing;
using K = obs::EventKind;
using End = obs::CommandEnd;
constexpr U64 max = std::numeric_limits<U64>::max();
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
    std::vector<std::byte> input;
    std::optional<OwnedReply> reply;
    bool announced = false, entered = false, terminal = false, withheld = false;
    bool input_charged = true, reply_charged = false, responding = false;
    bool body = false, callback = false, operation = false, ended = false;
    std::atomic<bool> abandoned{false};
    End end = End::idle;
    CommandView view() const { return {route, {correlation.data(), correlation_size}, input}; }
};
static_assert(sizeof(Record) <= 512);
struct Response {
    std::shared_ptr<Record> record;
    U64 deadline;
    bool consumed = false;
};
struct Engine : std::enable_shared_from_this<Engine> {
    QObject context;
    const std::thread::id owner = std::this_thread::get_id();
    Options options;
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
    std::atomic<bool> stop_requested{false}, wake_pending{false};
    std::atomic<U64> ordinal{0};
    U64 next_id = 0;
    std::optional<U64> owner_time, io_time;
    std::optional<subathon::server::testing::WorkControlFault> cold_fault;
    bool started = false, transport_started = false, writer_stopping = false, failed = false;
    bool dispatching = false;
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
    bool stopping() const {
        return stop_requested.load(std::memory_order_acquire) || detail::signal_pending();
    }
    void wake() {
        if (!wake_pending.exchange(true)) {
            QMetaObject::invokeMethod(
                &context,
                [this] {
                    wake_pending.store(false);
                    turn();
                },
                Qt::QueuedConnection);
        }
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
        release(snapshot.input, r->input.capacity());
        std::vector<std::byte>().swap(r->input);
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
        r->terminal = true;
        r->end = reason;
        if (r->reply_charged) {
            std::lock_guard lock(mutex);
            release(snapshot.response, r->reply->bytes.capacity());
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
        if (!first)
            return;
        const auto now = sample();
        const auto until = now ? deadline(*now, options.limits.drain_ms) : std::nullopt;
        {
            std::lock_guard lock(mutex);
            snapshot.drain_deadline = until;
        }
        event(K::stop_consumed);
        if (active && !active->terminal)
            end(active, End::stopping);
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
    void cleanup() {
        if (!stopping() || active || dispatching)
            return;
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
            ready = snapshot.network_barrier && snapshot.writer_barrier && !snapshot.owner_cleanup;
        }
        if (!ready)
            return;
        dispatcher.reset();
        event(K::dispatcher_destroyed);
        elapsed.reset();
        hooks.elapsed.reset();
        event(K::source_destroyed);
        {
            std::lock_guard lock(mutex);
            for (auto &p : peers) {
                if (p)
                    p->live.store(false);
                p.reset();
            }
            snapshot.control_charge = 0;
            snapshot.owner_cleanup = true;
            snapshot.state = failed ? State::failed : State::stopped;
        }
        event(K::owner_cleanup);
        event(K::stopped);
    }
    void after_callback(const std::shared_ptr<Record> &r) {
        if (r->abandoned.load() && !r->terminal)
            end(r, End::abandoned);
        if (!r->terminal && !r->pending)
            end(r, End::idle);
        if (r->pending || r->callback || r->body)
            return;
        // A chosen reply becomes transport-owned only after normal callback return.
        if (r->reply) {
            if (stopping())
                end(r, End::stopping);
            else
                deliver(r);
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
        // Announce all retained admissions on Qt, outside the lock, before entry.
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
        if (stopping())
            denied = Failure::stopping;
        else if (!r->peer->live.load())
            denied = Failure::disconnected;
        else {
            const auto now = sample();
            if (!now) {
                denied = Failure::elapsed_unavailable;
                r->peer->live.store(false);
            } else if (*now >= r->deadline)
                denied = Failure::expired;
            else if (r->route != RouteId::diagnostic) {
                std::lock_guard lock(mutex);
                if (!snapshot.storage_active || r->route != RouteId::installation)
                    denied = Failure::unavailable;
            }
        }
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
        {
            std::lock_guard lock(mutex);
            r->entered = true;
        }
        r->callback = true;
        dispatching = true;
        if (r->route == RouteId::diagnostic) {
            {
                std::lock_guard lock(mutex);
                --snapshot.queued_diagnostics;
            }
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
    std::optional<Failure> response_gate(const std::shared_ptr<Record> &r) {
        if (stopping())
            return Failure::stopping;
        if (!r->peer->live.load())
            return Failure::disconnected;
        if (!sample()) {
            r->peer->live.store(false);
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
            const auto size = reply.bytes.capacity();
            if (reply.bytes.size() > options.limits.response_bytes ||
                size > options.limits.response_capacity - snapshot.response.capacity ||
                snapshot.response.copies >= options.limits.response_slots)
                return Failure::resource_limit;
            charge(snapshot.response, size);
            r->reply_charged = true;
        }
        r->reply = std::move(reply);
        r->terminal = true;
        r->end = End::finished;
        return {};
    }
    ForwardResult forward(const std::shared_ptr<Record> &r, SqliteWriter::Work body,
                          SqliteWriter::WorkCompletion done) {
        if (r->operation || r->terminal)
            return Failure::invalid_completion;
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
                try {
                    auto value = body(c);
                    r->body = false;
                    return value;
                } catch (...) {
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
                try {
                    done(std::move(result));
                } catch (...) {
                    end(r, End::callback_exception);
                }
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
    bool charged = true;
    ~Impl() {
        if (charged)
            if (auto e = engine.lock()) {
                {
                    std::lock_guard lock(e->mutex);
                    Engine::release(e->snapshot.response, bytes.capacity());
                    if (response.use_count() == 1)
                        e->release_record_locked(response->record);
                }
                e->wake();
            }
    }
};
struct Host::Impl {
    std::shared_ptr<Engine> engine;
    Ingress ingress;
    Impl(Options o, std::unique_ptr<Transport> t, std::unique_ptr<CommandDispatcher> d)
        : engine(std::make_shared<Engine>(std::move(o), std::move(t), std::move(d))),
          ingress(std::make_unique<Ingress::Impl>(Ingress::Impl{engine})) {
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
                r->peer->live.store(false);
                e.event(K::operation_refused, r->id, 0, Failure::elapsed_unavailable);
                e.end(r, End::elapsed_unavailable);
                return;
            }
            auto lease = std::make_unique<ResponseLease::Impl>();
            lease->engine = engine;
            lease->response = std::make_shared<Response>(Response{r, *until});
            lease->bytes = std::move(r->reply->bytes);
            {
                std::lock_guard lock(e.mutex);
                r->responding = true;
                r->reply_charged = false;
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
Host::~Host() = default;
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
    e.elapsed = e.hooks.elapsed;
    if (!e.elapsed) {
        auto source = make_linux_monotonic_elapsed();
        if (auto *p = std::get_if<std::shared_ptr<const native_ws::ElapsedSource>>(&source))
            e.elapsed = *p;
        else
            return Failure::elapsed_unavailable;
    }
    auto profile = native_ws::create_profile(e.options.native, e.elapsed);
    if (!std::holds_alternative<std::unique_ptr<native_ws::Profile>>(profile))
        return Failure::invalid_options;
    e.peers.resize(std::size_t(l.peers));
    e.records.resize(std::size_t(l.associations));
    e.startup = std::move(observer);
    {
        std::lock_guard lock(e.mutex);
        e.snapshot.state = State::starting;
    }
    if (auto *composition = dynamic_cast<detail::TransportComposition *>(e.transport.get()))
        composition->configure(e.elapsed, l);
    if (e.stopping()) {
        e.wake();
        return StartAccepted{};
    }
    e.writer = std::make_unique<SqliteWriter>();
    if (e.writer->open(QString::fromStdString(e.options.store_path), e.options.writer_limits) !=
        StorageError::none) {
        e.failed = true;
        e.request_stop();
        return Failure::unavailable;
    }
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
    if (!std::holds_alternative<StartAccepted>(accepted)) {
        e.failed = true;
        e.request_stop();
        return Failure::unavailable;
    }
    const auto manifest = reference_authority_repository_manifest();
    const auto head = manifest.steps.back().version;
    auto admission = SqliteMigrationRunner::enqueue(
        *e.writer, {MigrationMode::inspect, manifest, {}, {}}, [&e, head](MigrationResult r) {
            const bool current = r.work.transaction == TransactionOutcome::committed &&
                                 r.work.error == WorkError::none && r.work.value &&
                                 r.reason == MigrationReason::none && r.report &&
                                 r.report->state == CatalogState::current &&
                                 r.report->version == head && r.report->target_version == head &&
                                 r.report->applied == 0;
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
                        result.error == WorkError::none && result.value && result.value->toBool();
                    e.event(K::installation_read_completed, 0, 0,
                            installation_ok ? std::nullopt : std::optional{Failure::unavailable},
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
    {
        std::lock_guard lock(e.mutex);
        if (e.snapshot.state != State::serving)
            return Failure::unavailable;
        auto it = std::find(e.peers.begin(), e.peers.end(), nullptr);
        if (it == e.peers.end())
            return Failure::resource_limit;
        peer = std::make_shared<Peer>();
        peer->owner = std::this_thread::get_id();
        *it = peer;
        e.snapshot.control_charge += 1024;
    }
    if (auto *composition = dynamic_cast<detail::TransportComposition *>(e.transport.get()))
        composition->peer_connected(peer.get());
    return PeerLease(
        std::make_unique<PeerLease::Impl>(PeerLease::Impl{e.weak_from_this(), std::move(peer)}));
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
            peer->live.store(false);
        }
    }
    if (!refusal) {
        std::lock_guard lock(e.mutex);
        const auto &l = e.options.limits;
        if (e.snapshot.state != State::serving ||
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
                r->input = std::vector<std::byte>(bytes.begin(), bytes.end());
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
    peer->live.store(false);
    {
        std::lock_guard lock(e.mutex);
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
        r.record->peer->live.store(false);
        return Failure::elapsed_unavailable;
    }
    if (*now >= r.deadline)
        return Failure::expired;
    r.consumed = true;
    e->event(K::local_handoff, r.record->id);
    return CommandView{
        r.record->route, {r.record->correlation.data(), r.record->correlation_size}, impl_->bytes};
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
        r.record->peer->live.store(false);
        return Failure::elapsed_unavailable;
    }
    if (*now >= r.deadline)
        return Failure::expired;
    auto p = std::make_unique<Impl>();
    p->charged = false;
    p->engine = e;
    p->response = impl_->response;
    {
        std::lock_guard lock(e->mutex);
        auto &pool = e->snapshot.response;
        if (impl_->bytes.size() > e->options.limits.response_capacity - pool.capacity ||
            pool.copies >= e->options.limits.response_slots)
            return Failure::resource_limit;
        Engine::charge(pool, impl_->bytes.size());
        p->charged = true;
    }
    p->bytes = impl_->bytes;
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
testing::ElapsedIdentity testing::HostAccess::elapsed_identity(const Host &h) {
    const auto *source = h.impl_->engine->elapsed.get();
    return {source, source};
}
} // namespace subathon::server::api_host
