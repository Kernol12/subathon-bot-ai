// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sqlite_reference_authority_fixture.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QMetaObject>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <stdexcept>
#include <subathon/server/api_host.hpp>
#include <subathon/server/api_host_elapsed.hpp>
#include <subathon/server/testing/api_host_observers.hpp>
#include <thread>

namespace api_host_fixture {
namespace host = subathon::server::api_host;
namespace hooks = host::testing;
using namespace subathon::server;
using account_sessions_fixture::scenario;
// Distinguish fixture assertions from deliberately thrown domain/emitter errors.
struct AssertionFailure : std::runtime_error {
    using std::runtime_error::runtime_error;
};
inline void require(bool condition, const char *message) {
    if (!condition)
        throw AssertionFailure(message);
}
struct CallbackFailure {
    mutable std::mutex mutex;
    std::exception_ptr first;
    void capture() {
        std::lock_guard lock(mutex);
        if (!first)
            first = std::current_exception();
    }
    template <class F> auto call(F f) -> decltype(f()) {
        try {
            return f();
        } catch (const AssertionFailure &) {
            capture();
            throw;
        } catch (const host::NotImplemented &) {
            capture();
            throw;
        }
    }
    void check() const {
        std::exception_ptr error;
        {
            std::lock_guard lock(mutex);
            error = first;
        }
        if (error)
            std::rethrow_exception(error);
    }
};
// Owned fixture watchdogs are wall-time supervision, never product elapsed time.
inline constexpr int wait_ms = 10000;
template <class T> T success(host::Result<T> result, const char *where) {
    if (std::holds_alternative<host::NotImplemented>(result))
        throw AssertionFailure(std::string("UNIMPLEMENTED production api-host: ") + where);
    require(std::holds_alternative<T>(result), where);
    return std::get<T>(std::move(result));
}
template <class T> void failure(const host::Result<T> &result, host::Failure expected) {
    require(!std::holds_alternative<host::NotImplemented>(result),
            "UNIMPLEMENTED production api-host");
    require(std::holds_alternative<host::Failure>(result), "Expected explicit host refusal");
    require(std::get<host::Failure>(result) == expected, "Exact host refusal");
}
inline std::span<const std::byte> bytes(std::string_view s) {
    return {reinterpret_cast<const std::byte *>(s.data()), s.size()};
}
inline host::OwnedReply reply(std::string_view s) { return {{bytes(s).begin(), bytes(s).end()}}; }
class Elapsed final : public native_ws::ElapsedSource {
  public:
    std::atomic<std::uint64_t> now{0}, reads{0};
    mutable std::atomic<std::uint64_t> owner_reads{0}, foreign_reads{0};
    mutable std::atomic<std::uint64_t> owner_last{0}, foreign_last{0};
    std::atomic<bool> available{true}, foreign_override{false};
    std::atomic<std::uint64_t> foreign_now{0};
    const std::thread::id owner = std::this_thread::get_id();
    std::variant<native_ws::ElapsedInstant, native_ws::ElapsedUnavailable>
    sample() const noexcept override {
        const_cast<Elapsed *>(this)->reads.fetch_add(1);
        if (!available.load())
            return native_ws::ElapsedUnavailable{};
        const bool on_owner = std::this_thread::get_id() == owner;
        const auto value = foreign_override.load() && !on_owner ? foreign_now.load() : now.load();
        (on_owner ? owner_last : foreign_last).store(value);
        (on_owner ? owner_reads : foreign_reads).fetch_add(1);
        return native_ws::ElapsedInstant{value};
    }
};
struct Log {
    mutable std::mutex mutex;
    std::array<hooks::Event, 4096> events{};
    std::size_t used = 0;
    bool overflow = false;
    std::function<void(const hooks::Event &)> observe;
    CallbackFailure failures;
    std::condition_variable idle;
    unsigned active_callbacks = 0;
    void add(const hooks::Event &e) {
        std::function<void(const hooks::Event &)> callback;
        {
            std::lock_guard lock(mutex);
            callback = observe;
            if (callback)
                ++active_callbacks;
            if (used == events.size()) {
                overflow = true;
                if (callback)
                    --active_callbacks;
                idle.notify_all();
                return;
            }
            events[used++] = e;
        }
        if (callback) {
            try {
                failures.call([&] { callback(e); });
            } catch (...) {
                std::lock_guard lock(mutex);
                --active_callbacks;
                idle.notify_all();
                throw;
            }
            std::lock_guard lock(mutex);
            --active_callbacks;
            idle.notify_all();
        }
    }
    void set_observer(std::function<void(const hooks::Event &)> callback) {
        std::unique_lock lock(mutex);
        observe = std::move(callback);
        // In-flight copies must finish before captured test locals can disappear.
        if (!idle.wait_for(lock, std::chrono::milliseconds(wait_ms),
                           [&] { return active_callbacks == 0; }))
            std::terminate();
    }
    std::vector<hooks::Event> matching(hooks::EventKind kind) const {
        std::lock_guard lock(mutex);
        std::vector<hooks::Event> out;
        for (std::size_t i = 0; i < used; ++i)
            if (events[i].kind == kind)
                out.push_back(events[i]);
        return out;
    }
    bool full() const {
        std::lock_guard lock(mutex);
        return overflow;
    }
    std::size_t refusal_count() const {
        std::lock_guard lock(mutex);
        return std::count_if(events.begin(), events.begin() + used,
                             [](const auto &e) { return e.refusal.has_value(); });
    }
    bool has_refusal(host::Failure refusal) const {
        std::lock_guard lock(mutex);
        return std::any_of(events.begin(), events.begin() + used,
                           [refusal](const auto &e) { return e.refusal == refusal; });
    }
    bool terminal_refusal() const {
        std::lock_guard lock(mutex);
        return std::any_of(events.begin(), events.begin() + used, [](const auto &e) {
            return (e.kind == hooks::EventKind::inspection_completed ||
                    e.kind == hooks::EventKind::installation_read_completed) &&
                   e.refusal.has_value();
        });
    }
    std::size_t count(hooks::EventKind kind) const {
        std::lock_guard lock(mutex);
        return std::count_if(events.begin(), events.begin() + used,
                             [kind](auto &e) { return e.kind == kind; });
    }
    std::size_t position(hooks::EventKind kind) const {
        std::lock_guard lock(mutex);
        for (std::size_t i = 0; i < used; ++i)
            if (events[i].kind == kind)
                return i;
        throw std::runtime_error("Missing actual host event");
    }
    hooks::Event last(hooks::EventKind kind) const {
        std::lock_guard lock(mutex);
        for (auto i = used; i > 0; --i)
            if (events[i - 1].kind == kind)
                return events[i - 1];
        throw std::runtime_error("Missing actual host event");
    }
};
// Single fixture IO owner, 256 queued tasks, 8 retained response leases. This fake
// only transports events/data; it never creates a host scope/transaction/readiness.
class FakeTransport final : public host::Transport {
  public:
    FakeTransport() : worker_([this] { loop(); }) {}
    ~FakeTransport() override {
        {
            std::lock_guard lock(mutex_);
            quitting_ = true;
        }
        cv_.notify_one();
        if (worker_.joinable())
            worker_.join(); // tasks terminate; suite external watchdog remains mandatory
    }
    host::Ingress *ingress = nullptr;
    std::unique_ptr<native_ws::Profile> profile;
    host::StartupObserver startup;
    host::NetworkBarrierObserver barrier;
    std::optional<host::StopDeadline> first_deadline;
    bool delay_ready = false, hold_network = false, retain_replies = false;
    std::atomic<unsigned> handoffs{0}, takes{0};
    std::array<std::optional<host::ResponseLease>, 8> replies;
    std::array<std::vector<std::byte>, 8> correlations;
    std::thread::id io_thread;
    host::Result<host::StartAccepted> start(host::Ingress &i, std::unique_ptr<native_ws::Profile> p,
                                            host::StartupObserver ready) override {
        ingress = &i;
        profile = std::move(p);
        startup = std::move(ready);
        if (!delay_ready)
            fire_ready();
        return host::StartAccepted{};
    }
    void fire_ready() {
        post([this] { startup(host::StartAccepted{}); });
    }
    void handoff(host::ResponseLease lease) override {
        auto owned = std::make_shared<host::ResponseLease>(std::move(lease));
        post([this, owned] {
            const auto count = handoffs.fetch_add(1);
            require(!retain_replies || count < replies.size(), "Fixture response slots");
            const auto index = count % replies.size();
            replies[index].emplace(std::move(*owned));
            if (!retain_replies) {
                const auto view = success(replies[index]->take(), "IO response lease");
                correlations[index].assign(view.correlation.begin(), view.correlation.end());
                ++takes;
                replies[index].reset();
            }
        });
    }
    void stop(host::StopDeadline deadline, host::NetworkBarrierObserver done) override {
        if (!first_deadline)
            first_deadline = deadline;
        barrier = std::move(done);
        if (!hold_network)
            acknowledge();
    }
    void acknowledge() {
        if (network_scheduled_)
            return;
        network_scheduled_ = true;
        post([this] {
            for (auto &r : replies)
                r.reset();
            profile.reset();
            {
                std::lock_guard lock(mutex_);
                quitting_ = true;
            }
            cv_.notify_one();
            QMetaObject::invokeMethod(
                &context_,
                [this] {
                    // Socket/lease callbacks are already gone; this is cleanup join,
                    // never domain work waiting for an IO response.
                    worker_.join();
                    joined_ = true;
                    barrier({true, true, true, true});
                },
                Qt::QueuedConnection);
        });
    }
    template <class F> auto on_io(F f) -> decltype(f()) {
        using R = decltype(f());
        const auto task = std::make_shared<std::packaged_task<R()>>(std::move(f));
        auto result = task->get_future();
        post([task] { (*task)(); });
        // A timed-out task may retain caller stack borrows: never unwind past it.
        if (result.wait_for(std::chrono::milliseconds(wait_ms)) != std::future_status::ready)
            std::terminate();
        return result.get();
    }
    bool joined() const { return joined_; }
    void check() {
        if (!joined_)
            on_io([] {});
        if (problem_)
            std::rethrow_exception(problem_);
    }

  private:
    void post(std::function<void()> f) {
        {
            std::lock_guard lock(mutex_);
            require(tasks_.size() < 256, "Fixture task cap");
            tasks_.push_back(std::move(f));
        }
        cv_.notify_one();
    }
    void loop() {
        io_thread = std::this_thread::get_id();
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, [this] { return quitting_ || !tasks_.empty(); });
                if (tasks_.empty() && quitting_)
                    return;
                task = std::move(tasks_.front());
                tasks_.pop_front();
            }
            try {
                task();
            } catch (...) {
                problem_ = std::current_exception();
            }
        }
    }
    QObject context_; // cancels queued join callback before this transport is destroyed
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> tasks_;
    bool quitting_ = false, joined_ = false, network_scheduled_ = false;
    std::exception_ptr problem_;
    std::thread worker_;
};
// These are test dispatch units, not a replacement writer or host bridge. Every
// transaction is run by the actual private writer through CommandHandle::forward.
inline void admitted(host::ForwardResult result) {
    require(!std::holds_alternative<host::NotImplemented>(result), "UNIMPLEMENTED forward");
    require(std::holds_alternative<StorageError>(result) &&
                std::get<StorageError>(result) == StorageError::none,
            "Actual private writer admission");
}
inline QVariant read_installation(SqliteWorkContext context) {
    const auto session = context.begin();
    account_sessions_fixture::Cleanup cleanup{session};
    SqliteAccountSessionUnit unit(session, {});
    const auto facts = unit.read_installation("I");
    require(std::holds_alternative<StoredInstallationFacts>(facts), "Actual I/E/J read");
    const auto &f = std::get<StoredInstallationFacts>(facts);
    require(f.current.installation_ref == "I" && f.current.session_epoch_ref == "E" &&
                f.revision_ref == "J",
            "Independent I/E/J literals");
    session.commit();
    return 1;
}
class Dispatcher final : public host::CommandDispatcher {
  public:
    std::function<host::OwnedReply(host::CommandView)> diagnostic_body;
    SqliteWriter::Work body;
    std::function<void(host::CommandHandle &, const SqliteWorkResult &)> completion;
    std::function<void(host::CommandHandle &, host::CommandView)> execution;
    std::optional<host::CommandHandle> command;
    CallbackFailure failures;
    unsigned diagnostics = 0, executions = 0, completions = 0;
    std::thread::id body_thread, completion_thread;
    host::OwnedReply diagnostic(host::CommandView v) override {
        ++diagnostics;
        return failures.call([&] { return diagnostic_body ? diagnostic_body(v) : reply("diag"); });
    }
    void execute(host::CommandHandle handle, host::CommandView view) override {
        ++executions;
        command.emplace(std::move(handle));
        failures.call([&] {
            if (execution) {
                execution(*command, view);
                return;
            }
            admitted(command->forward(
                [this](auto context) {
                    body_thread = std::this_thread::get_id();
                    return failures.call(
                        [&] { return body ? body(context) : read_installation(context); });
                },
                [this](auto result) {
                    completion_thread = std::this_thread::get_id();
                    ++completions;
                    failures.call([&] {
                        if (completion) {
                            completion(*command, result);
                            return;
                        }
                        // The fixture chooses no reply on known noncommits; separate
                        // cases explicitly exercise the dispatcher's right to reply.
                        if (result.transaction == TransactionOutcome::committed &&
                            result.error == WorkError::none && result.value) {
                            auto end = command->finish(reply("done"));
                            if (std::holds_alternative<host::Failure>(end))
                                require(std::get<host::Failure>(end) == host::Failure::stopping ||
                                            std::get<host::Failure>(end) ==
                                                host::Failure::disconnected ||
                                            std::get<host::Failure>(end) ==
                                                host::Failure::elapsed_unavailable,
                                        "Only revoked reply authority may refuse normal finish");
                            else
                                success(std::move(end), "finish");
                        } else
                            success(command->finish(), "No-reply end");
                    });
                }));
        });
    }
};
// Host owns these proxies. The fixture separately retains only fake transport
// observation state/dispatcher counters; host cleanup cannot dangle test pointers.
class TransportOwner final : public host::Transport {
  public:
    explicit TransportOwner(std::shared_ptr<FakeTransport> state) : state_(std::move(state)) {}
    host::Result<host::StartAccepted> start(host::Ingress &i, std::unique_ptr<native_ws::Profile> p,
                                            host::StartupObserver o) override {
        return state_->start(i, std::move(p), std::move(o));
    }
    void handoff(host::ResponseLease r) override { state_->handoff(std::move(r)); }
    void stop(host::StopDeadline d, host::NetworkBarrierObserver o) override {
        state_->stop(d, std::move(o));
    }

  private:
    std::shared_ptr<FakeTransport> state_;
};
class DispatcherOwner final : public host::CommandDispatcher {
  public:
    explicit DispatcherOwner(std::shared_ptr<Dispatcher> state) : state_(std::move(state)) {}
    host::OwnedReply diagnostic(host::CommandView v) override { return state_->diagnostic(v); }
    void execute(host::CommandHandle h, host::CommandView v) override {
        state_->execute(std::move(h), v);
    }

  private:
    std::shared_ptr<Dispatcher> state_;
};
// Dispose setup writer before host acquisition. Independent SQL reads are never
// used to activate the host, and never keep an execution scope alive by pumping.
struct Store {
    QTemporaryDir directory;
    QString path = directory.filePath("host.sqlite");
    account_sessions_fixture::Data data;
    explicit Store(std::string_view variant = "current", const QString &root = {})
        : directory(root.isEmpty() ? QDir::tempPath() + "/api-host-XXXXXX"
                                   : root + "/api-host-XXXXXX") {
        require(directory.isValid(), "Disposable host store");
        SqliteWriter setup;
        require(setup.open(path, account_sessions_fixture::limits()) == StorageError::none,
                "Setup writer open");
        if (variant != "fresh") {
            auto manifest = data.manifest(variant != "old");
            if (variant != "old" && variant != "previous_head") {
                manifest.steps.append(permission_graph_fixture::step9(manifest));
                manifest.supported_starting_versions.append(9);
                manifest.steps.append(reference_authority_fixture::step10(manifest));
                manifest.supported_starting_versions.append(10);
            }
            const auto m = permission_graph_fixture::migrate(setup, std::move(manifest));
            require(m.work.transaction == TransactionOutcome::committed &&
                        m.reason == MigrationReason::none,
                    "Existing literal schema foundation");
            if (variant != "old")
                (void)account_sessions_fixture::run(setup, [&](auto c) -> QVariant {
                    auto s = c.begin();
                    account_sessions_fixture::Cleanup cleanup{s};
                    data.populate(s);
                    s.commit();
                    return 1;
                });
        }
        bool done = false;
        require(setup.shutdown([&] { done = true; }) == StorageError::none,
                "Setup writer shutdown");
        require(QTest::qWaitFor([&] { return done; }, wait_ms), "Setup writer barrier");
        if (variant == "future")
            account_sessions_fixture::corrupt(
                path,
                {"INSERT INTO subathon_migration_history VALUES(11,'future11',zeroblob(32))"});
        if (variant == "wrong")
            account_sessions_fixture::corrupt(
                path, {"UPDATE subathon_catalog SET store_identity='foreign'"});
        if (variant == "malformed_catalog")
            account_sessions_fixture::corrupt(
                path,
                {"UPDATE subathon_migration_history SET content_sha256=X'01' WHERE version=1"});
        if (variant == "missing")
            account_sessions_fixture::corrupt(path, {"DELETE FROM subathon_account_installation"});
        if (variant == "malformed")
            account_sessions_fixture::corrupt(
                path, {"UPDATE subathon_account_installation SET session_epoch_ref=X''"});
    }
    SqlRows catalog() const {
        account_sessions_fixture::Observer r(path);
        return r.rows("SELECT type,name,tbl_name,sql FROM sqlite_schema ORDER BY type,name");
    }
    SqlRows history() const {
        account_sessions_fixture::Observer r(path);
        return r.rows("SELECT version,migration_id,hex(content_sha256) FROM "
                      "subathon_migration_history ORDER BY version");
    }
    SqlRows facts() const {
        account_sessions_fixture::Observer r(path);
        return r.rows("SELECT hex(installation_ref),hex(session_epoch_ref),hex(revision_ref) FROM "
                      "subathon_account_installation");
    }
    SqlRows sessions() const {
        account_sessions_fixture::Observer r(path);
        return r.rows(
            "SELECT "
            "hex(installation_ref),hex(account_ref),hex(session_ref),hex(installation_epoch_ref),"
            "hex(account_epoch_ref),hex(record_revision_ref),hex(digest),state,retention,lifetime_"
            "started_utc_ms,authenticated_utc_ms,expires_utc_ms FROM subathon_account_sessions");
    }
};
struct Rig {
    Store store;
    std::shared_ptr<Elapsed> elapsed = std::make_shared<Elapsed>();
    std::shared_ptr<Log> log = std::make_shared<Log>();
    std::shared_ptr<FakeTransport> transport = std::make_shared<FakeTransport>();
    std::shared_ptr<Dispatcher> dispatch = std::make_shared<Dispatcher>();
    std::unique_ptr<host::Host> instance;
    explicit Rig(std::string_view variant = "current",
                 std::function<void(host::Options &)> configure = {})
        : store(variant) {
        auto t = std::make_unique<TransportOwner>(transport);
        auto d = std::make_unique<DispatcherOwner>(dispatch);
        host::Options o;
        o.store_path = store.path.toStdString();
        o.writer_limits = account_sessions_fixture::limits();
        o.writer_limits.pending_batches = 128;
        if (variant == "wrong_selector")
            o.installation = "X";
        if (configure)
            configure(o);
        instance = hooks::HostAccess::create(std::move(o), std::move(t), std::move(d),
                                             {elapsed, [l = log](const auto &e) { l->add(e); }});
    }
    ~Rig() {
        log->set_observer({}); // test locals may already have unwound
        if (!instance)
            return;
        const auto state = instance->inspect().state;
        if (state == host::State::cold)
            return;
        if (state == host::State::stopped || state == host::State::failed) {
            const auto s = instance->inspect();
            if (s.writer_barrier && s.network_barrier && s.owner_cleanup)
                return;
        }
        instance->request_stop();
        if (transport->hold_network && transport->barrier && !transport->joined())
            transport->acknowledge();
        if (!QTest::qWaitFor(
                [&] {
                    if (transport->hold_network && transport->barrier && !transport->joined())
                        transport->acknowledge();
                    const auto s = instance->inspect();
                    return (s.state == host::State::stopped || s.state == host::State::failed) &&
                           s.writer_barrier && s.network_barrier && s.owner_cleanup;
                },
                wait_ms)) {
            QTest::qFail("Fixture cleanup barrier missing; process-terminal failure", __FILE__,
                         __LINE__);
            std::terminate();
        }
    }
    void start(bool active = true) {
        success(instance->start([this](auto r) {
            dispatch->failures.call([&] { success(std::move(r), "Startup completion"); });
        }),
                "Host::start");
        require(QTest::qWaitFor([&] { return instance->inspect().state == host::State::serving; },
                                wait_ms),
                "Actual serving");
        if (active)
            require(QTest::qWaitFor([&] { return instance->inspect().storage_active; }, wait_ms),
                    "Actual host startup chain");
    }
    host::PeerLease peer() {
        return transport->on_io(
            [&] { return success(transport->ingress->connected(), "Peer connect"); });
    }
    host::Result<host::Admitted> submit(const host::PeerLease &p, std::string_view id = "A",
                                        host::RouteId route = host::RouteId::installation,
                                        std::string_view body = "data") {
        return transport->on_io(
            [&] { return transport->ingress->try_submit(p, bytes(id), route, bytes(body)); });
    }
    void disconnect(const host::PeerLease &p) {
        transport->on_io([&] { transport->ingress->disconnected(p); });
    }
    void check_callbacks() const {
        { // Bounded fixed event storage, no fabricated SQL or writer admission.
            std::lock_guard lock(log->mutex);
            for (std::size_t i = 0; i != log->used; ++i) {
                const auto &e = log->events[i];
                const bool writer = e.kind == hooks::EventKind::writer_completed ||
                                    e.kind == hooks::EventKind::inspection_completed ||
                                    e.kind == hooks::EventKind::installation_read_completed;
                require(writer || !e.work, "Non-writer observation has no SQL result");
                require(e.kind != hooks::EventKind::writer_completed ||
                            (e.work && e.work->ticket_revoked),
                        "Actual command result after ticket revocation");
            }
        }
        dispatch->failures.check();
        log->failures.check();
    }
    void refused_activation() {
        require(QTest::qWaitFor([&] { return log->terminal_refusal(); }, wait_ms),
                "Terminal actual startup refusal");
        require(log->count(hooks::EventKind::activated) == 0, "No activation on refusal");
    }
    void completed(unsigned n = 1) {
        require(QTest::qWaitFor(
                    [&] {
                        check_callbacks();
                        return dispatch->completions >= n;
                    },
                    wait_ms),
                "Supplied unit completions finish");
        transport->check();
        check_callbacks();
        const auto results = log->matching(hooks::EventKind::writer_completed);
        require(results.size() >= n, "Actual writer completions, not dispatcher counters alone");
        for (std::size_t i = 0; i != n; ++i)
            require(results[i].work &&
                        results[i].work->transaction == TransactionOutcome::committed &&
                        results[i].work->error == WorkError::none && results[i].work->has_value &&
                        results[i].work->ticket_revoked,
                    "Normal component positive requires actual committed truth");
        require(!log->full(), "Bounded observer log");
    }
    void stop(bool allow_failed = false) {
        instance->request_stop();
        require(QTest::qWaitFor(
                    [&] {
                        const auto s = instance->inspect();
                        return (s.state == host::State::stopped ||
                                (allow_failed && s.state == host::State::failed)) &&
                               s.writer_barrier && s.network_barrier && s.owner_cleanup;
                    },
                    wait_ms),
                "Actual stop barriers");
        transport->check();
        check_callbacks();
        const auto s = instance->inspect();
        require(s.writer_barrier && s.network_barrier && s.owner_cleanup,
                "No fabricated stop from deadline");
        require(s.associations == 0 && s.input.capacity == 0 && s.input.copies == 0 &&
                    s.response.capacity == 0 && s.response.copies == 0,
                "Every reservation released once");
    }
};
} // namespace api_host_fixture
