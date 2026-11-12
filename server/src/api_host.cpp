// SPDX-License-Identifier: GPL-3.0-or-later
#include "api_host_private.hpp"
#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <limits>
#include <mutex>
#include <subathon/server/api_host.hpp>
#include <subathon/server/api_host_elapsed.hpp>
#include <subathon/server/testing/api_host_observers.hpp>
#include <thread>
#include <utility>

namespace subathon::server::api_host::private_detail {
struct Completion {
    const void *host = nullptr, *record = nullptr;
    std::uint64_t serial = 0, generation = 0;
};
} // namespace subathon::server::api_host::private_detail
Q_DECLARE_METATYPE(subathon::server::api_host::private_detail::Completion)

namespace subathon::server::api_host {
namespace {
namespace ht = testing;
using Kind = ht::EventKind;
struct Depth {
    unsigned &value;
    explicit Depth(unsigned &v) : value(v) { ++value; }
    ~Depth() { --value; }
    Depth(const Depth &) = delete;
    Depth &operator=(const Depth &) = delete;
};
constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
bool deadline(std::uint64_t now, std::uint64_t ms, std::uint64_t &out) {
    if (ms > maximum / 1000000 || now > maximum - ms * 1000000)
        return false;
    out = now + ms * 1000000;
    return true;
}
struct Peer {
    std::uint64_t serial = 0, associations = 0;
    std::thread::id thread;
    bool live = false, leased = false, control_reserved = false;
    std::size_t time_owner = 0;
    std::array<std::byte, 1024> control{};
};
struct TimeOwner {
    std::thread::id thread;
    std::optional<std::uint64_t> last;
};
struct Association {
    std::array<std::byte, 256> correlation{};
    std::uint64_t serial = 0, generation = 0, queue_deadline = 0, response_deadline = 0;
    std::size_t peer = 0, correlation_size = 0, input_size = 0, response_copies = 0;
    RouteId route = RouteId::diagnostic;
    std::unique_ptr<std::byte[]> input;
    bool used = false, processed = false, accepted = false, active = false, finished = false;
    bool consumed = false;
    std::optional<Failure> cancelled;
};
static_assert(sizeof(Association) <= 512);
struct Shared : ResponseBudget {
    mutable std::mutex mutex;
    Options options;
    Inspection status;
    std::unique_ptr<Peer[]> peers;
    std::unique_ptr<TimeOwner[]> time_owners;
    std::unique_ptr<Association[]> associations;
    std::shared_ptr<const native_ws::ElapsedSource> elapsed;
    ht::Hooks hooks;
    std::thread::id owner = std::this_thread::get_id();
    QObject *context = nullptr;
    std::atomic<bool> stop{false};
    std::atomic<std::uint64_t> ordinal{0};
    bool wake_pending = false, notify_pending = false, alive = true;
    unsigned owner_depth = 0, observer_depth = 0; // Qt owner only
    std::function<void()> owner_wake;
    std::optional<Result<StartAccepted>> ready_mail;
    std::optional<NetworkBarrier> network_mail;
    bool notify_locked() noexcept {
        if (notify_pending)
            return true;
        if (!context)
            return false;
        notify_pending = true;
        try {
            if (QMetaObject::invokeMethod(context, owner_wake, Qt::QueuedConnection))
                return true;
        } catch (...) {
        }
        notify_pending = false;
        return false;
    }
    std::uint64_t serial = 0;
    std::optional<StoredInstallationFacts> current;
    void event(Kind k, std::optional<ht::WorkObservation> work = {},
               std::optional<Failure> f = {}) noexcept {
        // A failing or reentrant test observer cannot strand actual cleanup.
        static thread_local bool observing = false;
        if (!hooks.observe || observing)
            return;
        const bool on_owner = std::this_thread::get_id() == owner;
        observing = true;
        if (on_owner)
            ++observer_depth;
        try {
            std::uint64_t generation;
            {
                std::lock_guard lock(mutex);
                generation = status.generation;
            }
            hooks.observe({k, ++ordinal, generation, std::this_thread::get_id(), work, f});
        } catch (...) {
        }
        if (on_owner)
            --observer_depth;
        observing = false;
    }
    std::optional<std::uint64_t> sample(std::optional<std::uint64_t> &last) const {
        if (!elapsed)
            return {};
        const auto observation = elapsed->sample();
        const auto *n = std::get_if<native_ws::ElapsedInstant>(&observation);
        if (!n || (last && n->nanoseconds < *last))
            return {};
        last = n->nanoseconds;
        return n->nanoseconds;
    }
    void drop_input(Association &a) {
        if (!a.input)
            return;
        status.input.capacity -= a.input_size;
        --status.input.copies;
        status.input.metadata -= 128;
        a.input.reset();
        a.input_size = 0;
    }
    void release(Association &a) {
        if (!a.used)
            return;
        drop_input(a);
        --peers[a.peer].associations;
        --status.associations;
        status.association_charge -= 512;
        a.used = false;
    }
    void cancel_peer(std::size_t index, Failure f) {
        auto &p = peers[index];
        p.live = false;
        if (p.control_reserved) {
            p.control_reserved = false;
            status.control_charge -= 1024;
        }
        if (!associations)
            return; // partial startup allocation owns no commands
        for (std::uint64_t i = 0; i < options.limits.associations; ++i) {
            auto &a = associations[i];
            if (!a.used || a.peer != index)
                continue;
            if (!a.cancelled)
                a.cancelled = f;
            if (!a.active)
                drop_input(a);
            if (!a.processed && !a.accepted) {
                if (a.route == RouteId::diagnostic)
                    --status.queued_diagnostics;
                release(a);
            } else if (a.finished && a.response_copies == 0)
                release(a);
        }
    }
    bool acquire(std::uint64_t bytes) override {
        std::lock_guard lock(mutex);
        return alive && reserve(status.response, bytes, true);
    }
    void release(std::uint64_t bytes) noexcept override {
        std::lock_guard lock(mutex);
        status.response.capacity -= bytes;
        --status.response.copies;
        status.response.metadata -= 128;
    }
    void resource_refused() noexcept override {
        event(Kind::operation_refused, {}, Failure::resource_limit);
    }
    bool peer_live(std::uint64_t serial_value) const override {
        std::lock_guard lock(mutex);
        for (std::uint64_t i = 0; peers && i < options.limits.peers; ++i)
            if (peers[i].serial == serial_value)
                return peers[i].live;
        return false;
    }
    bool reserve(PoolUsage &pool, std::uint64_t bytes, bool response) {
        const auto &l = options.limits;
        const auto cap = response ? l.response_capacity : l.input_capacity;
        const auto copy_slots = response ? l.response_slots : l.input_slots;
        if (pool.copies >= copy_slots || bytes > cap - pool.capacity)
            return false;
        pool.capacity += bytes;
        ++pool.copies;
        pool.metadata += 128;
        pool.peak_charge = std::max(pool.peak_charge, pool.capacity + pool.metadata);
        return true;
    }
};
// Retainable sampler owns only revocation state and a weak host. No dead scope
// pointer is dereferenced, including a foreign or recursively entered call.
struct Membership {
    std::weak_ptr<Shared> host;
    std::atomic<bool> live{true};
    bool sampling = false;
    std::uint64_t generation = 0;
};
class ScopedClock final : public CurrentSessionClock {
    std::shared_ptr<Membership> scope_;

  public:
    explicit ScopedClock(std::shared_ptr<Membership> s) : scope_(std::move(s)) {}
    SessionClockResolution sample_current() override {
        auto h = scope_->host.lock();
        if (!h || std::this_thread::get_id() != h->owner || !scope_->live || scope_->sampling ||
            h->observer_depth || h->owner_depth != 1)
            return SessionSourceUnavailable{};
        {
            std::lock_guard lock(h->mutex);
            if (!h->alive || h->status.state != State::serving || !h->status.storage_active ||
                h->status.generation != scope_->generation || !h->current || h->stop)
                return SessionSourceUnavailable{};
        }
        if (!h->hooks.raw_session_sampler)
            return SessionSourceUnavailable{};
        scope_->sampling = true;
        try {
            auto result = h->hooks.raw_session_sampler->sample_current();
            scope_->sampling = false;
            return result;
        } catch (...) {
            scope_->sampling = false;
            return SessionSourceUnavailable{};
        }
    }
};
Result<CurrentInstallationView> installation(const std::shared_ptr<Membership> &m) {
    auto h = m->host.lock();
    if (!h || std::this_thread::get_id() != h->owner || !m->live || m->sampling ||
        h->observer_depth || h->owner_depth != 1)
        return Failure::unavailable;
    std::lock_guard lock(h->mutex);
    if (!h->alive || !h->current || !h->status.storage_active || h->stop ||
        h->status.state != State::serving || h->status.generation != m->generation)
        return Failure::unavailable;
    const auto &f = *h->current;
    return CurrentInstallationView{f.current.installation_ref, f.current.session_epoch_ref,
                                   f.revision_ref};
}
struct TransactionState {
    std::shared_ptr<Membership> membership;
    SqliteWorkContext context;
    SqliteWorkSession session;
    std::unique_ptr<SqliteAccountSessionUnit> unit;
    std::optional<StoredInstallationFacts> candidate;
    bool begun = false, terminal = false, busy = false;
    void guard() {
        const auto h = membership->host.lock();
        if (!h || std::this_thread::get_id() != h->owner)
            throw SqliteWorkFailure(WorkError::wrong_thread);
        if (!membership->live || membership->sampling || h->observer_depth || h->owner_depth != 1)
            throw SqliteWorkFailure(WorkError::stale_session);
        if (!begun || terminal || busy)
            throw SqliteWorkFailure(WorkError::session_closed);
    }
};
struct Busy {
    bool &value;
    explicit Busy(bool &b) : value(b) { value = true; }
    ~Busy() { value = false; }
};
ht::WorkObservation observed(const SqliteWorkResult &r) {
    // Writer finish revokes its work state before calling this completion.
    return {r.transaction, r.error, r.value.has_value(), true};
}
bool valid_options(const Options &o) {
    const auto &l = o.limits;
    const std::uint64_t counts[]{l.associations, l.per_peer, l.peers, l.input_slots,
                                 l.response_slots};
    for (auto n : counts)
        if (!n || n > std::size_t(-1) / 2048)
            return false;
    const std::uint64_t sizes[]{l.envelope_bytes, l.input_capacity, l.response_bytes,
                                l.response_capacity};
    for (auto n : sizes)
        if (!n || n > std::size_t(-1) / 4)
            return false;
    // Includes the transport's peer × per-peer fixed request table. Reject
    // unrepresentable aggregate table bounds before any startup allocation.
    if (l.peers > std::size_t(-1) / l.per_peer || l.peers * l.per_peer > std::size_t(-1) / 128)
        return false;
    std::uint64_t unused;
    for (auto n : {l.queue_ms, l.handoff_ms, l.drain_ms})
        if (!n || !deadline(0, n, unused))
            return false;
    const auto &w = o.writer_limits;
    if (w.statements_per_batch <= 0 || w.statement_utf8_bytes <= 0 || w.bound_value_bytes <= 0 ||
        w.result_rows <= 0 || w.result_value_bytes <= 0)
        return false;
    const auto &store = o.store_limits;
    if (!store.reference_bytes || !store.selected_bytes || !store.mutation_bytes ||
        !store.page_rows || store.reference_bytes > std::size_t(-1) / 4 ||
        store.selected_bytes > std::size_t(-1) / 4 || store.mutation_bytes > std::size_t(-1) / 4 ||
        store.page_rows > std::size_t(-1) / 2048)
        return false;
    return l.per_peer <= l.associations && l.envelope_bytes <= l.input_capacity &&
           l.response_bytes <= l.response_capacity && o.writer_limits.pending_batches > 0 &&
           std::uint64_t(o.writer_limits.pending_batches) >= l.associations &&
           !o.store_path.empty() && o.store_path.size() <= 4096 &&
           o.store_path.find('\0') == std::string::npos && !o.installation.empty() &&
           o.installation.size() <= o.store_limits.reference_bytes;
}
} // namespace
struct PeerLease::Impl {
    std::shared_ptr<Shared> shared;
    std::size_t index;
    std::uint64_t serial;
    ~Impl() {
        std::lock_guard lock(shared->mutex);
        auto &p = shared->peers[index];
        if (p.serial == serial) {
            shared->cancel_peer(index, Failure::disconnected);
            p.leased = false;
        }
    }
};
struct Ingress::Impl {
    std::shared_ptr<Shared> shared;
};
struct ResponseLease::Impl {
    std::shared_ptr<Shared> shared;
    std::size_t index;
    std::uint64_t serial;
    std::vector<std::byte> bytes;
    std::size_t size = 0, capacity = 0;
    ~Impl() {
        std::lock_guard lock(shared->mutex);
        // Destroy payload ownership while the reservation still excludes other
        // allocators. The vector member's later destructor is then inert.
        std::vector<std::byte>().swap(bytes);
        auto &p = shared->status.response;
        p.capacity -= capacity;
        --p.copies;
        p.metadata -= 128;
        auto &a = shared->associations[index];
        if (a.used && a.serial == serial) {
            --a.response_copies;
            if (a.finished && !a.response_copies)
                shared->release(a);
        }
    }
};

PeerLease::PeerLease(std::unique_ptr<Impl> p) : impl_(std::move(p)) {}
PeerLease::PeerLease(PeerLease &&) noexcept = default;
PeerLease &PeerLease::operator=(PeerLease &&) noexcept = default;
PeerLease::~PeerLease() = default;
ResponseLease::ResponseLease(std::unique_ptr<Impl> p) : impl_(std::move(p)) {
    static_assert(sizeof(Impl) <= 128);
}
ResponseLease::ResponseLease(ResponseLease &&) noexcept = default;
ResponseLease &ResponseLease::operator=(ResponseLease &&) noexcept = default;
ResponseLease::~ResponseLease() = default;
void ResponseLease::reset() noexcept { impl_.reset(); }
Result<CommandView> ResponseLease::take() {
    if (!impl_)
        return Failure::invalid_completion;
    auto &s = *impl_->shared;
    std::unique_lock lock(s.mutex);
    auto &a = s.associations[impl_->index];
    if (!a.used || a.serial != impl_->serial)
        return Failure::invalid_completion;
    auto &p = s.peers[a.peer];
    if (std::this_thread::get_id() != p.thread)
        return Failure::wrong_thread;
    if (!p.live)
        return Failure::disconnected;
    if (a.consumed)
        return Failure::invalid_completion;
    if (a.cancelled)
        return *a.cancelled;
    const auto now = s.sample(s.time_owners[p.time_owner].last);
    if (!now) {
        s.cancel_peer(a.peer, Failure::elapsed_unavailable);
        return Failure::elapsed_unavailable;
    }
    if (*now >= a.response_deadline) {
        a.cancelled = Failure::expired;
        return Failure::expired;
    }
    if (!s.alive || s.status.state != State::serving || s.stop)
        return Failure::stopping;
    a.consumed = true;
    const CommandView view{
        a.route, {a.correlation.data(), a.correlation_size}, {impl_->bytes.data(), impl_->size}};
    lock.unlock();
    s.event(Kind::local_handoff);
    return view;
}
Result<ResponseLease> ResponseLease::try_copy() const {
    if (!impl_)
        return Failure::invalid_completion;
    auto s = impl_->shared;
    std::lock_guard lock(s->mutex);
    auto &a = s->associations[impl_->index];
    if (!a.used || a.serial != impl_->serial || a.consumed)
        return Failure::invalid_completion;
    auto &p = s->peers[a.peer];
    if (p.thread != std::this_thread::get_id())
        return Failure::wrong_thread;
    if (!p.live)
        return Failure::disconnected;
    const auto now = s->sample(s->time_owners[p.time_owner].last);
    if (!now) {
        s->cancel_peer(a.peer, Failure::elapsed_unavailable);
        return Failure::elapsed_unavailable;
    }
    if (*now >= a.response_deadline)
        return Failure::expired;
    if (!s->reserve(s->status.response, impl_->capacity, true))
        return Failure::resource_limit;
    try {
        std::vector<std::byte> data(impl_->capacity);
        std::copy(impl_->bytes.begin(), impl_->bytes.end(), data.begin());
        data.resize(impl_->size);
        auto copy = std::make_unique<Impl>();
        copy->shared = s;
        copy->index = impl_->index;
        copy->serial = impl_->serial;
        copy->bytes = std::move(data);
        copy->size = impl_->size;
        copy->capacity = impl_->capacity;
        ++a.response_copies;
        return ResponseLease(std::move(copy));
    } catch (...) {
        auto &pool = s->status.response;
        pool.capacity -= impl_->capacity;
        --pool.copies;
        pool.metadata -= 128;
        return Failure::resource_limit;
    }
}
Ingress::Ingress(std::unique_ptr<Impl> p) : impl_(std::move(p)) {}
Ingress::~Ingress() = default;
Result<PeerLease> Ingress::connected() {
    auto s = impl_->shared;
    std::lock_guard lock(s->mutex);
    if (!s->alive || s->stop || s->status.state == State::stopping ||
        s->status.state == State::stopped || s->status.state == State::failed)
        return Failure::stopping;
    if (s->status.state != State::serving)
        return Failure::unavailable;
    for (std::uint64_t i = 0; i < s->options.limits.peers; ++i) {
        auto &p = s->peers[i];
        if (p.leased || p.associations)
            continue;
        if (s->serial == maximum)
            return Failure::unavailable;
        p = {};
        p.thread = std::this_thread::get_id();
        p.serial = ++s->serial;
        bool found_owner = false;
        for (std::uint64_t n = 0; n < s->options.limits.peers; ++n) {
            auto &time = s->time_owners[n];
            if (time.thread == p.thread) {
                p.time_owner = n;
                found_owner = true;
                break;
            }
        }
        if (!found_owner)
            for (std::uint64_t n = 0; n < s->options.limits.peers; ++n) {
                auto &time = s->time_owners[n];
                if (time.thread == std::thread::id{}) {
                    time.thread = p.thread;
                    p.time_owner = n;
                    found_owner = true;
                    break;
                }
            }
        if (!found_owner)
            return Failure::resource_limit;
        p.live = true;
        p.leased = true;
        p.control_reserved = true;
        s->status.control_charge += 1024;
        try {
            auto impl = std::make_unique<PeerLease::Impl>();
            impl->shared = s;
            impl->index = i;
            impl->serial = p.serial;
            auto lease = PeerLease(std::move(impl));
            connected_peer_serial = p.serial;
            return lease;
        } catch (...) {
            p.live = p.leased = p.control_reserved = false;
            s->status.control_charge -= 1024;
            return Failure::resource_limit;
        }
    }
    return Failure::resource_limit;
}
Result<Admitted> Ingress::try_submit(const PeerLease &lease, std::span<const std::byte> correlation,
                                     RouteId route, std::span<const std::byte> bytes) {
    auto s = impl_->shared;
    std::unique_lock lock(s->mutex);
    auto refuse = [&](Failure f) -> Result<Admitted> {
        lock.unlock();
        s->event(Kind::ingress_refused, {}, f);
        return f;
    };
    if (!lease.impl_ || lease.impl_->shared != s)
        return refuse(Failure::disconnected);
    auto &p = s->peers[lease.impl_->index];
    if (p.serial != lease.impl_->serial || !p.live)
        return refuse(Failure::disconnected);
    if (p.thread != std::this_thread::get_id())
        return refuse(Failure::wrong_thread);
    if (!s->alive || s->status.state != State::serving || s->stop)
        return refuse(Failure::stopping);
    if (route != RouteId::diagnostic &&
        (route != RouteId::installation || !s->status.storage_active))
        return refuse(Failure::unavailable);
    if (correlation.empty() || correlation.size() > 256 ||
        bytes.size() > s->options.limits.envelope_bytes)
        return refuse(Failure::resource_limit);
    auto now = s->sample(s->time_owners[p.time_owner].last);
    std::uint64_t end;
    if (!now || !deadline(*now, s->options.limits.queue_ms, end)) {
        s->cancel_peer(lease.impl_->index, Failure::elapsed_unavailable);
        return refuse(Failure::elapsed_unavailable);
    }
    if (p.associations >= s->options.limits.per_peer ||
        s->status.associations >= s->options.limits.associations || s->serial == maximum)
        return refuse(Failure::resource_limit);
    if (!s->reserve(s->status.input, bytes.size(), false))
        return refuse(Failure::resource_limit);
    std::unique_ptr<std::byte[]> owned;
    try {
        owned = std::make_unique<std::byte[]>(bytes.size());
    } catch (...) {
        s->status.input.capacity -= bytes.size();
        --s->status.input.copies;
        s->status.input.metadata -= 128;
        return refuse(Failure::resource_limit);
    }
    std::memcpy(owned.get(), bytes.data(), bytes.size());
    for (std::uint64_t i = 0; i < s->options.limits.associations; ++i) {
        auto &a = s->associations[i];
        if (a.used)
            continue;
        a = {};
        a.used = true;
        a.serial = ++s->serial;
        a.generation = s->status.generation;
        a.peer = lease.impl_->index;
        a.route = route;
        a.queue_deadline = end;
        a.input = std::move(owned);
        a.input_size = bytes.size();
        std::copy(correlation.begin(), correlation.end(), a.correlation.begin());
        a.correlation_size = correlation.size();
        ++p.associations;
        ++s->status.associations;
        s->status.association_charge += 512;
        if (route == RouteId::diagnostic)
            ++s->status.queued_diagnostics;
        // One coalesced notification, with no input capture or per-request callback.
        if (!s->wake_pending) {
            s->wake_pending = true;
            if (!s->notify_locked()) {
                s->wake_pending = false;
                if (route == RouteId::diagnostic)
                    --s->status.queued_diagnostics;
                s->release(a);
                return refuse(Failure::resource_limit);
            }
        }
        submitted_command_serial = a.serial;
        return Admitted{};
    }
    std::terminate(); // count/free-slot invariant
}
void Ingress::disconnected(const PeerLease &lease) noexcept {
    auto s = impl_->shared;
    std::lock_guard lock(s->mutex);
    if (!lease.impl_ || lease.impl_->shared != s)
        return;
    auto &p = s->peers[lease.impl_->index];
    if (p.serial != lease.impl_->serial || p.thread != std::this_thread::get_id())
        return;
    s->cancel_peer(lease.impl_->index, Failure::disconnected);
}

struct AccountSessions::Impl {
    std::shared_ptr<TransactionState> state;
};
struct Transaction::Impl {
    std::shared_ptr<TransactionState> state;
    std::unique_ptr<AccountSessions> accounts;
};
struct Execution::Impl {
    std::shared_ptr<Membership> membership;
    std::shared_ptr<TransactionState> state;
    std::unique_ptr<Transaction> transaction;
    std::shared_ptr<CurrentSessionClock> clock;
};
struct Publication::Impl {
    std::shared_ptr<Membership> membership;
    std::shared_ptr<CurrentSessionClock> clock;
};
AccountSessions::AccountSessions() = default;
AccountSessions::~AccountSessions() = default;
AccountSessionRead<StoredInstallationFacts>
AccountSessions::read_installation(std::string_view key) {
    auto &t = *impl_->state;
    t.guard();
    Busy busy(t.busy);
    return t.unit->read_installation(key);
}
LifecycleWriteResult AccountSessions::replace_installation_epoch(const StoredInstallationFacts &f,
                                                                 std::string_view epoch,
                                                                 std::string_view revision) {
    auto &t = *impl_->state;
    t.guard();
    Busy busy(t.busy);
    return t.unit->replace_installation_epoch(f, epoch, revision);
}
Transaction::Transaction() : impl_(std::make_unique<Impl>()) {
    impl_->accounts = std::unique_ptr<AccountSessions>(new AccountSessions);
    impl_->accounts->impl_ = std::make_unique<AccountSessions::Impl>();
}
Transaction::~Transaction() = default;
AccountSessions &Transaction::accounts() {
    impl_->state->guard();
    impl_->accounts->impl_->state = impl_->state;
    return *impl_->accounts;
}
void Transaction::commit() {
    auto &t = *impl_->state;
    t.guard();
    Busy busy(t.busy);
    t.terminal = true;
    t.candidate.reset();
    try {
        auto h = t.membership->host.lock();
        h->event(Kind::precommit_read);
        auto read = t.unit->read_installation(h->options.installation);
        auto *f = std::get_if<StoredInstallationFacts>(&read);
        if (!f)
            throw AccountSessionStorageFailure(std::get<AccountSessionReadFailure>(read));
        t.candidate = std::move(*f);
        // No callbacks, repository operation or yielding between this final read and COMMIT.
        t.session.commit();
        h->event(Kind::actual_commit);
    } catch (...) {
        t.candidate.reset();
        (void)t.session.cleanup();
        throw;
    }
}
void Transaction::rollback() {
    auto &t = *impl_->state;
    t.guard();
    Busy busy(t.busy);
    t.terminal = true;
    t.candidate.reset();
    try {
        t.session.rollback();
    } catch (...) {
        (void)t.session.cleanup();
        throw;
    }
}
Execution::Execution() = default;
Execution::~Execution() = default;
Transaction &Execution::begin_transaction() {
    auto &t = *impl_->state;
    auto h = t.membership->host.lock();
    if (!h || std::this_thread::get_id() != h->owner)
        throw SqliteWorkFailure(WorkError::wrong_thread);
    if (!t.membership->live || t.membership->sampling || t.busy || h->observer_depth ||
        h->owner_depth != 1)
        throw SqliteWorkFailure(WorkError::stale_session);
    Busy busy(t.busy);
    h->event(Kind::begin_attempted);
    // A second call deliberately reaches the unchanged writer protocol gate.
    t.session = t.context.begin();
    t.begun = true;
    t.unit = std::make_unique<SqliteAccountSessionUnit>(t.session, AccountSessionReadScope{{1}},
                                                        h->options.store_limits);
    auto tx = std::unique_ptr<Transaction>(new Transaction);
    tx->impl_->state = impl_->state;
    impl_->transaction = std::move(tx);
    return *impl_->transaction;
}
std::shared_ptr<CurrentSessionClock> Execution::clock() const { return impl_->clock; }
Result<CurrentInstallationView> Execution::current_installation() const {
    return installation(impl_->membership);
}
Publication::Publication() = default;
Publication::~Publication() = default;
std::shared_ptr<CurrentSessionClock> Publication::clock() const { return impl_->clock; }
Result<CurrentInstallationView> Publication::current_installation() const {
    return installation(impl_->membership);
}

struct Host::Impl {
    Host &host;
    std::shared_ptr<Shared> s = std::make_shared<Shared>();
    QObject context;
    QTimer sweep;
    std::unique_ptr<Transport> transport;
    std::unique_ptr<CommandDispatcher> dispatcher;
    struct IngressDelete {
        void operator()(Ingress *p) const { delete p; }
    };
    std::unique_ptr<Ingress, IngressDelete> ingress;
    std::unique_ptr<SqliteWriter> writer;
    StartupObserver startup;
    std::optional<std::uint64_t> last;
    std::optional<subathon::server::testing::WorkControlFault> fault;
    bool attempted = false, transport_started = false, shutting_writer = false;
    bool failed = false, mismatch = false;
    bool writer_ack_pending = false, processing_pending = false;
    // An enqueue/post exception may follow callback retention in the unchanged
    // writer. Quarantine its owners; neither rejection nor safe cleanup is known.
    bool writer_ownership_uncertain = false;
    std::optional<MigrationResult> inspection_mail;
    std::optional<SqliteWorkResult> installation_mail;
    std::optional<StoredInstallationFacts> startup_candidate;
    // Fixed per-association command completion records, separate from the data
    // envelope. Writer closures capture slot+serial only, never unbounded data.
    struct WorkRecord {
        std::uint64_t serial = 0, generation = 0;
        std::shared_ptr<TransactionState> transaction;
        std::optional<OwnedDomainResult> result;
        std::uint64_t result_capacity = 0;
        bool normal = false, mismatched = false;
        std::optional<Failure> host_failure;
    };
    static_assert(sizeof(WorkRecord) <= 128);
    std::unique_ptr<WorkRecord[]> records;
    // Accepted writer completions may arrive during an illegal nested event pump.
    // Fixed slots retain their actual outcomes until the outer owner turn returns.
    std::unique_ptr<std::optional<SqliteWorkResult>[]> completion_mail;
    Impl(Host &h, Options o, std::unique_ptr<Transport> t, std::unique_ptr<CommandDispatcher> d)
        : host(h), transport(std::move(t)), dispatcher(std::move(d)) {
        s->options = std::move(o);
        s->context = &context;
        s->owner_wake = [this] {
            {
                std::lock_guard lock(s->mutex);
                s->notify_pending = false;
            }
            poll();
        };
        QObject::connect(&sweep, &QTimer::timeout, &context, [this] { poll(); });
        // Already armed before any ingress/stop notification can allocate. This
        // owner wake is the no-allocation fallback when invokeMethod refuses.
        sweep.start(10);
    }
    bool owner() const { return std::this_thread::get_id() == s->owner; }
    void event(Kind k, std::optional<ht::WorkObservation> w = {}, std::optional<Failure> f = {}) {
        s->event(k, w, f);
    }
    bool occupied() const { return s->owner_depth || s->observer_depth; }
    template <class F> bool on_owner(F &&body) {
        if (occupied())
            return false;
        Depth turn(s->owner_depth);
        body();
        return true;
    }
    void quarantine_writer() {
        if (writer_ownership_uncertain)
            return;
        writer_ownership_uncertain = true;
        failed = true;
        s->stop = true;
        event(Kind::operation_refused, {}, Failure::incomplete_shutdown);
    }
    void report_startup(Result<StartAccepted> result) noexcept {
        if (!startup)
            return;
        auto callback = std::move(startup);
        try {
            callback(std::move(result));
        } catch (...) {
            failed = true;
            s->stop = true;
        }
    }
    void receive_work(std::size_t index, std::uint64_t serial, SqliteWorkResult result) {
        if (!on_owner([&] {
                try {
                    completed(index, serial, std::move(result));
                } catch (...) {
                    clear_record(records[index]);
                    refuse(index, serial, Failure::unavailable);
                    consume_stop();
                }
            })) {
            completion_mail[index] = std::move(result);
            return;
        }
        poll();
    }
    void poll() noexcept {
        try {
            poll_owned();
        } catch (...) {
            failed = true;
            s->stop = true;
            event(Kind::operation_refused, {}, Failure::resource_limit);
        }
    }
    void poll_owned() {
        if (!on_owner([&] {
                consume_stop();
                std::optional<Result<StartAccepted>> ready_result;
                std::optional<NetworkBarrier> network_result;
                {
                    std::lock_guard lock(s->mutex);
                    ready_result = std::exchange(s->ready_mail, {});
                    network_result = std::exchange(s->network_mail, {});
                }
                if (ready_result)
                    ready(std::move(*ready_result));
                if (network_result)
                    network_done(*network_result);
                if (inspection_mail)
                    inspection_done(std::move(*std::exchange(inspection_mail, {})));
                if (installation_mail)
                    installation_done(std::move(*std::exchange(installation_mail, {})));
                for (;;) {
                    std::size_t index = 0;
                    std::uint64_t serial = maximum;
                    for (std::uint64_t n = 0;
                         records && completion_mail && n < s->options.limits.associations; ++n)
                        if (completion_mail[n] && records[n].serial < serial) {
                            index = n;
                            serial = records[n].serial;
                        }
                    if (serial == maximum)
                        break;
                    auto result = std::move(*completion_mail[index]);
                    completion_mail[index].reset();
                    try {
                        completed(index, serial, std::move(result));
                    } catch (...) {
                        clear_record(records[index]);
                        refuse(index, serial, Failure::unavailable);
                    }
                }
                if (writer_ack_pending) {
                    writer_ack_pending = false;
                    event(Kind::writer_removed);
                    {
                        std::lock_guard lock(s->mutex);
                        s->status.writer_barrier = true;
                    }
                    event(Kind::writer_ack);
                }
                process_pending();
                sweep_queued();
                if (s->stop && writer && !shutting_writer)
                    request_writer_shutdown();
            }))
            return;
        finish_stop();
    }
    void consume_stop() {
        consume_stop_signal(host);
        if (!s->stop)
            return;
        bool first = false;
        {
            std::lock_guard lock(s->mutex);
            if (s->status.state != State::stopping && s->status.state != State::stopped &&
                s->status.state != State::failed) {
                s->status.state = State::stopping;
                ++s->status.stop_fences;
                s->status.storage_active = false;
                first = true;
                for (std::uint64_t i = 0; i < s->options.limits.associations && s->associations;
                     ++i) {
                    auto &a = s->associations[i];
                    if (a.used && !a.active && !a.cancelled)
                        a.cancelled = Failure::stopping;
                }
            }
        }
        if (!first)
            return;
        auto now = s->sample(last);
        std::uint64_t end = 0;
        if (now && deadline(*now, s->options.limits.drain_ms, end)) {
            std::lock_guard lock(s->mutex);
            s->status.drain_deadline = end;
        }
        // An unmeasurable drain requests immediate network teardown, never an unlimited wait.
        event(Kind::stop_consumed);
        // A consumed stop cancels readiness notification, preserving the frozen
        // startup-stop consumer. Actual start failures report through their own
        // failure path before initiating this same cleanup sequence.
        startup = {};
        if (transport_started) {
            transport->stop({end}, [this](NetworkBarrier b) {
                std::lock_guard lock(s->mutex);
                s->network_mail = b;
                (void)s->notify_locked();
            });
        } else {
            network_done({true, true, true, true});
        }
        process_pending();
        request_writer_shutdown();
    }
    void request_writer_shutdown() {
        if (writer && !shutting_writer) {
            StorageError admitted = StorageError::invalid_work;
            // Registration may precede a throwing dispatch post. Never register
            // another callback after an exception or assume scheduling recovered.
            shutting_writer = true;
            try {
                admitted = writer->shutdown([this] {
                    writer_ack_pending = true;
                    poll();
                });
            } catch (...) {
                quarantine_writer();
                return;
            }
            shutting_writer = admitted == StorageError::none;
            if (!shutting_writer)
                event(Kind::operation_refused, {}, Failure::incomplete_shutdown);
        } else if (!writer) {
            {
                std::lock_guard lock(s->mutex);
                s->status.writer_barrier = true;
            }
            event(Kind::writer_removed);
            event(Kind::writer_ack);
            finish_stop();
        }
    }
    void network_done(NetworkBarrier b) {
        if (!(b.callbacks_revoked && b.sockets_removed && b.framework_joined && b.io_joined)) {
            event(Kind::operation_refused, {}, Failure::incomplete_shutdown);
            return;
        }
        {
            std::lock_guard lock(s->mutex);
            if (s->status.network_barrier)
                return;
            s->status.network_barrier = true;
            if (s->peers)
                for (std::uint64_t i = 0; i < s->options.limits.peers; ++i)
                    s->cancel_peer(i, Failure::stopping);
        }
        event(Kind::callbacks_revoked);
        event(Kind::sockets_removed);
        event(Kind::framework_joined);
        event(Kind::io_joined);
        event(Kind::network_ack);
        finish_stop();
    }
    void finish_stop() {
        if (occupied())
            return;
        Depth cleanup(s->owner_depth);
        {
            std::lock_guard lock(s->mutex);
            if (writer_ownership_uncertain || !s->status.network_barrier ||
                !s->status.writer_barrier || s->status.owner_cleanup)
                return;
            for (std::uint64_t i = 0; s->associations && i < s->options.limits.associations; ++i) {
                auto &a = s->associations[i];
                if (!a.used)
                    continue;
                // A dishonest transport cannot acknowledge while retaining leases.
                if (a.response_copies || (a.accepted && !a.finished))
                    return;
                s->release(a);
            }
            s->current.reset();
            s->status.storage_active = false;
        }
        sweep.stop();
        writer.reset();
        startup_candidate.reset();
        transport.reset();
        s->elapsed.reset();
        s->hooks.elapsed.reset();
        s->hooks.raw_session_sampler.reset();
        event(Kind::source_destroyed);
        dispatcher.reset();
        event(Kind::dispatcher_destroyed);
        {
            std::lock_guard lock(s->mutex);
            s->status.owner_cleanup = true;
        }
        event(Kind::owner_cleanup);
        {
            std::lock_guard lock(s->mutex);
            s->status.state = failed ? State::failed : State::stopped;
        }
        event(Kind::stopped);
    }
    void ready(Result<StartAccepted> r) {
        consume_stop();
        {
            std::lock_guard lock(s->mutex);
            if (s->status.state != State::starting)
                return;
            if (!std::holds_alternative<StartAccepted>(r))
                failed = true;
            else
                s->status.state = State::serving;
        }
        if (failed) {
            report_startup(std::holds_alternative<Failure>(r)
                               ? Result<StartAccepted>(std::get<Failure>(r))
                               : Result<StartAccepted>(Failure::unavailable));
            s->stop = true;
            consume_stop();
            return;
        }
        event(Kind::listener_ready);
        report_startup(StartAccepted{});
        consume_stop();
    }
    void inspection_done(MigrationResult r) {
        consume_stop();
        const bool valid = r.work.transaction == TransactionOutcome::committed &&
                           r.work.error == WorkError::none && r.reason == MigrationReason::none &&
                           r.report && r.report->state == CatalogState::current &&
                           r.report->version == 8 && r.report->target_version == 8 &&
                           r.report->applied == 0 && !s->stop && !writer_ownership_uncertain;
        event(Kind::inspection_completed, observed(r.work),
              valid ? std::optional<Failure>{} : Failure::unavailable);
        consume_stop();
        if (valid && !s->stop)
            read_initial();
    }
    void inspect_store() {
        MigrationRequest request;
        request.mode = MigrationMode::inspect;
        request.manifest = account_session_repository_manifest();
        request.cancel = [this](const MigrationProgress &) {
            consume_stop_signal(host);
            return s->stop.load() || occupied();
        };
        try {
            const auto admission = SqliteMigrationRunner::enqueue(
                *writer, std::move(request), [this](MigrationResult r) {
                    if (!on_owner([&] { inspection_done(std::move(r)); }))
                        inspection_mail = std::move(r);
                    else
                        poll();
                });
            if (admission.error == StorageError::none)
                event(Kind::inspection_enqueued);
            else
                event(Kind::inspection_completed, {}, Failure::unavailable);
        } catch (...) {
            quarantine_writer();
            // Preserve the synchronous start failure notification.
            throw;
        }
        consume_stop();
    }
    void installation_done(SqliteWorkResult r) {
        consume_stop();
        const bool valid = !s->stop && !writer_ownership_uncertain &&
                           r.transaction == TransactionOutcome::committed &&
                           r.error == WorkError::none && r.value && startup_candidate;
        event(Kind::installation_read_completed, observed(r),
              valid ? std::optional<Failure>{} : Failure::unavailable);
        consume_stop();
        if (!valid || s->stop)
            return;
        {
            std::lock_guard lock(s->mutex);
            s->current = std::move(*startup_candidate);
            s->status.storage_active = true;
            s->status.generation = 1;
        }
        event(Kind::activated);
    }
    void read_initial() {
        // Startup has its own fixed record; no command can access the private writer
        // or source before this acknowledged chain activates it.
        startup_candidate.reset();
        try {
            const auto admission = writer->enqueue_work(
                [this](SqliteWorkContext c) -> QVariant {
                    if (occupied())
                        return {};
                    Depth turn(s->owner_depth);
                    consume_stop();
                    if (s->stop)
                        return {};
                    auto session = c.begin();
                    try {
                        SqliteAccountSessionUnit unit(session, AccountSessionReadScope{{1}},
                                                      s->options.store_limits);
                        auto read = unit.read_installation(s->options.installation);
                        if (!std::holds_alternative<StoredInstallationFacts>(read))
                            throw AccountSessionStorageFailure(
                                std::get<AccountSessionReadFailure>(read));
                        startup_candidate = std::get<StoredInstallationFacts>(std::move(read));
                        session.commit();
                        return QVariant(true);
                    } catch (...) {
                        (void)session.cleanup();
                        throw;
                    }
                },
                [this](SqliteWorkResult r) {
                    if (!on_owner([&] { installation_done(std::move(r)); }))
                        installation_mail = std::move(r);
                    else
                        poll();
                });
            if (admission == StorageError::none)
                event(Kind::installation_read_enqueued);
            else
                event(Kind::installation_read_completed, {}, Failure::unavailable);
        } catch (...) {
            // Writer completion containment cannot hide this admission uncertainty.
            quarantine_writer();
        }
        consume_stop();
    }
    // Gate is checked on the authoritative owner immediately before entry. It
    // never manufactures a writer result for a never-accepted item.
    std::optional<Failure> entry(std::size_t i, std::uint64_t serial) {
        consume_stop();
        std::lock_guard lock(s->mutex);
        auto &a = s->associations[i];
        if (!a.used || a.serial != serial)
            return Failure::cancelled;
        if (a.cancelled)
            return a.cancelled;
        if (s->stop || s->status.state != State::serving)
            return Failure::stopping;
        if (!s->peers[a.peer].live)
            return Failure::disconnected;
        auto now = s->sample(last);
        if (!now) {
            s->cancel_peer(a.peer, Failure::elapsed_unavailable);
            return Failure::elapsed_unavailable;
        }
        if (*now >= a.queue_deadline)
            return Failure::expired;
        if (a.route != RouteId::diagnostic &&
            (!s->status.storage_active || !s->current || a.generation != s->status.generation))
            return Failure::unavailable;
        a.active = true;
        return {};
    }
    CommandView view(std::size_t i) {
        std::lock_guard lock(s->mutex);
        auto &a = s->associations[i];
        return {a.route, {a.correlation.data(), a.correlation_size}, {a.input.get(), a.input_size}};
    }
    void settle(std::size_t i, std::uint64_t serial) {
        std::lock_guard lock(s->mutex);
        auto &a = s->associations[i];
        if (!a.used || a.serial != serial)
            return;
        a.active = false;
        a.finished = true;
        s->drop_input(a);
        if (!a.response_copies)
            s->release(a);
    }
    void refuse(std::size_t i, std::uint64_t serial, Failure f) {
        event(Kind::operation_refused, {}, f);
        settle(i, serial);
    }
    void send(std::size_t i, std::uint64_t serial, OwnedReply reply) {
        consume_stop();
        std::unique_ptr<ResponseLease::Impl> lease;
        std::optional<Failure> refusal;
        {
            std::lock_guard lock(s->mutex);
            auto &a = s->associations[i];
            if (!a.used || a.serial != serial)
                return;
            auto now = s->sample(last);
            std::uint64_t end;
            if (!now || !deadline(*now, s->options.limits.handoff_ms, end)) {
                s->cancel_peer(a.peer, Failure::elapsed_unavailable);
                refusal = Failure::elapsed_unavailable;
            } else if (a.cancelled)
                refusal = a.cancelled;
            else if (s->stop || s->status.state != State::serving)
                refusal = Failure::stopping;
            else if (!s->peers[a.peer].live)
                refusal = Failure::disconnected;
            else if (reply.bytes.capacity() > s->options.limits.response_bytes ||
                     !s->reserve(s->status.response, reply.bytes.capacity(), true))
                refusal = Failure::resource_limit;
            else {
                const auto capacity = reply.bytes.capacity();
                try {
                    lease = std::make_unique<ResponseLease::Impl>();
                    lease->shared = s;
                    lease->index = i;
                    lease->serial = serial;
                    lease->size = reply.bytes.size();
                    lease->capacity = capacity;
                    lease->bytes = std::move(reply.bytes);
                    // OwnedReply may have unused capacity; the lease keeps the full
                    // capacity charged, while its view must expose only the payload.
                    ++a.response_copies;
                    a.response_deadline = end;
                } catch (...) {
                    auto &pool = s->status.response;
                    pool.capacity -= capacity;
                    --pool.copies;
                    pool.metadata -= 128;
                    refusal = Failure::resource_limit;
                }
            }
        }
        if (refusal) {
            // No ordinary response allocation on refusal. Revoke this transport
            // peer; its preallocated control allowance is never borrowed for data.
            {
                std::lock_guard lock(s->mutex);
                auto &a = s->associations[i];
                if (a.used && a.serial == serial)
                    s->cancel_peer(a.peer, *refusal);
            }
            refuse(i, serial, *refusal);
            return;
        }
        try {
            if (auto *network = dynamic_cast<NetworkTransport *>(transport.get())) {
                std::uint64_t peer;
                {
                    std::lock_guard lock(s->mutex);
                    peer = s->peers[s->associations[i].peer].serial;
                }
                network->handoff_to(peer, serial, ResponseLease(std::move(lease)));
            } else
                transport->handoff(ResponseLease(std::move(lease)));
        } catch (...) {
            event(Kind::operation_refused, {}, Failure::unavailable);
        }
        settle(i, serial);
    }
    void diagnostic(std::size_t i, std::uint64_t serial) {
        if (auto f = entry(i, serial)) {
            event(Kind::diagnostic_cancelled, {}, *f);
            settle(i, serial);
            return;
        }
        event(Kind::diagnostic_entered);
        try {
            auto reply = dispatcher->diagnostic(view(i));
            send(i, serial, std::move(reply));
        } catch (...) {
            refuse(i, serial, Failure::unavailable);
        }
        consume_stop();
        finish_stop();
    }
    QVariant execute(std::size_t i, std::uint64_t serial, SqliteWorkContext context_value) {
        event(Kind::wrapper_dispatched);
        if (occupied()) {
            event(Kind::operation_refused, {}, Failure::unavailable);
            return {}; // actual writer reports not_started/missing_transaction
        }
        Depth turn(s->owner_depth);
        if (auto f = entry(i, serial)) {
            event(Kind::operation_refused, {}, *f);
            return {};
        }
        auto &record = records[i];
        auto membership = std::make_shared<Membership>();
        membership->host = s;
        {
            std::lock_guard lock(s->mutex);
            membership->generation = s->status.generation;
        }
        auto tx = std::make_shared<TransactionState>();
        tx->membership = membership;
        tx->context = context_value;
        record.transaction = tx;
        record.generation = membership->generation;
        Execution e;
        e.impl_ = std::make_unique<Execution::Impl>();
        e.impl_->membership = membership;
        e.impl_->state = tx;
        e.impl_->clock = std::make_shared<ScopedClock>(membership);
        event(Kind::execution_entered);
        auto revoke = [&] {
            membership->live = false;
            tx->unit.reset();
            (void)tx->session.cleanup();
            {
                std::lock_guard lock(s->mutex);
                auto &a = s->associations[i];
                a.active = false;
                s->drop_input(a);
            }
            event(Kind::execution_revoked);
        };
        try {
            auto result = dispatcher->execute(e, view(i));
            // Result ownership is admitted BEFORE retaining it beyond this call.
            if (auto *r = std::get_if<OwnedReply>(&result)) {
                const auto capacity = r->bytes.capacity();
                {
                    std::lock_guard lock(s->mutex);
                    if (capacity > s->options.limits.response_bytes ||
                        !s->reserve(s->status.response, capacity, true)) {
                        record.host_failure = Failure::resource_limit;
                        throw SqliteWorkFailure(WorkError::resource_limit);
                    }
                }
                record.result_capacity = capacity;
            }
            record.result = std::move(result);
            record.normal = true;
            revoke();
            // Private identity is a slot serial, never client data. The actual
            // completion must also match the record, normal return and work truth.
            return QVariant::fromValue(private_detail::Completion{
                s.get(), &record, record.mismatched ? 0 : serial, record.generation});
        } catch (...) {
            revoke();
            throw;
        }
    }
    void bind_candidate(const SqliteWorkResult &r, WorkRecord &record) {
        std::lock_guard lock(s->mutex);
        if (r.transaction == TransactionOutcome::outcome_unknown ||
            (r.transaction == TransactionOutcome::committed &&
             (!record.transaction || !record.transaction->candidate))) {
            s->status.storage_active = false;
            s->current.reset();
            return;
        }
        if (r.transaction != TransactionOutcome::committed)
            return;
        auto &candidate = *record.transaction->candidate;
        bool changed = !s->current ||
                       s->current->current.installation_ref != candidate.current.installation_ref ||
                       s->current->current.session_epoch_ref != candidate.current.session_epoch_ref;
        if (changed) {
            if (s->status.generation == maximum) {
                s->current.reset();
                s->status.storage_active = false;
                s->stop = true;
                return;
            }
            ++s->status.generation;
            for (std::uint64_t i = 0; i < s->options.limits.associations; ++i) {
                auto &a = s->associations[i];
                if (a.used && a.generation != s->status.generation && !a.cancelled)
                    a.cancelled = Failure::unavailable;
            }
        }
        s->current = std::move(candidate);
    }
    void clear_record(WorkRecord &r) {
        if (r.result && std::holds_alternative<OwnedReply>(*r.result)) {
            std::lock_guard lock(s->mutex);
            r.result.reset();
            auto &pool = s->status.response;
            pool.capacity -= r.result_capacity;
            --pool.copies;
            pool.metadata -= 128;
        }
        r = {};
    }
    void completed(std::size_t i, std::uint64_t serial, SqliteWorkResult r) {
        // Includes allocation exceptions before an execution facade could escape.
        {
            std::lock_guard lock(s->mutex);
            auto &a = s->associations[i];
            a.active = false;
            s->drop_input(a);
        }
        auto &record = records[i];
        bind_candidate(r, record);
        event(Kind::writer_completed, observed(r));
        consume_stop();
        const bool match =
            record.serial == serial && record.normal &&
            r.transaction == TransactionOutcome::committed && r.error == WorkError::none &&
            r.value && r.value->metaType() == QMetaType::fromType<private_detail::Completion>() &&
            r.value->value<private_detail::Completion>().host == s.get() &&
            r.value->value<private_detail::Completion>().record == &record &&
            r.value->value<private_detail::Completion>().serial == serial &&
            r.value->value<private_detail::Completion>().generation == record.generation &&
            record.result;
        if (!match) {
            Failure reason = record.host_failure.value_or(Failure::invalid_completion);
            {
                std::lock_guard lock(s->mutex);
                const auto &a = s->associations[i];
                if (a.cancelled && !record.host_failure)
                    reason = *a.cancelled;
                else if (r.error != WorkError::none && !record.host_failure)
                    reason = Failure::unavailable;
            }
            if (record.normal && r.transaction == TransactionOutcome::committed &&
                r.error == WorkError::none) {
                std::lock_guard lock(s->mutex);
                s->current.reset();
                s->status.storage_active = false;
            }
            if (reason == Failure::resource_limit) {
                std::lock_guard lock(s->mutex);
                const auto &a = s->associations[i];
                if (a.used && a.serial == serial)
                    s->cancel_peer(a.peer, reason);
            }
            clear_record(record);
            refuse(i, serial, reason);
            finish_stop();
            return;
        }
        bool available;
        std::optional<Failure> publication_failure;
        {
            std::lock_guard lock(s->mutex);
            auto &a = s->associations[i];
            available = a.used && a.serial == serial && !a.cancelled && !s->stop &&
                        s->status.state == State::serving && s->status.storage_active &&
                        s->current && a.generation == s->status.generation && s->peers[a.peer].live;
            if (available && !s->sample(last)) {
                s->cancel_peer(a.peer, Failure::elapsed_unavailable);
                publication_failure = Failure::elapsed_unavailable;
                available = false;
            }
        }
        if (!available) {
            clear_record(record);
            refuse(
                i, serial,
                publication_failure.value_or(s->stop ? Failure::stopping : Failure::unavailable));
            finish_stop();
            return;
        }
        auto membership = std::make_shared<Membership>();
        membership->host = s;
        {
            std::lock_guard lock(s->mutex);
            membership->generation = s->status.generation;
        }
        Publication p;
        p.impl_ = std::make_unique<Publication::Impl>();
        p.impl_->membership = membership;
        p.impl_->clock = std::make_shared<ScopedClock>(membership);
        event(Kind::publication_entered);
        std::optional<OwnedReply> reply;
        try {
            consume_stop();
            if (!s->stop)
                reply = dispatcher->complete(p, *record.result);
            else
                event(Kind::operation_refused, {}, Failure::stopping);
        } catch (...) {
            event(Kind::operation_refused, {}, Failure::unavailable);
        }
        membership->live = false;
        event(Kind::publication_revoked);
        clear_record(record);
        if (reply)
            send(i, serial, std::move(*reply));
        else
            settle(i, serial);
        consume_stop();
        finish_stop();
    }
    void process_pending() {
        if (processing_pending)
            return;
        struct Processing {
            bool &flag;
            explicit Processing(bool &f) : flag(f) { flag = true; }
            ~Processing() { flag = false; }
        } processing(processing_pending);
        // Admission sequence, rather than array order, preserves actual FIFO when
        // released slots are reused. The table is finite; observers hold no lock.
        for (;;) {
            std::size_t index = 0;
            std::uint64_t serial = maximum;
            RouteId route{};
            {
                std::lock_guard lock(s->mutex);
                s->wake_pending = false;
                for (std::uint64_t i = 0; s->associations && i < s->options.limits.associations;
                     ++i) {
                    auto &a = s->associations[i];
                    if (a.used && !a.processed && a.serial < serial) {
                        index = i;
                        serial = a.serial;
                        route = a.route;
                    }
                }
                if (serial == maximum)
                    break;
                s->associations[index].processed = true;
            }
            if (route == RouteId::diagnostic) {
                event(Kind::diagnostic_queued);
                bool posted = false;
                try {
                    posted = QMetaObject::invokeMethod(
                        &context,
                        [this, index, serial] {
                            {
                                std::lock_guard lock(s->mutex);
                                auto &a = s->associations[index];
                                if (!a.used || a.serial != serial)
                                    return;
                                --s->status.queued_diagnostics;
                            }
                            if (!on_owner([&] { diagnostic(index, serial); })) {
                                event(Kind::diagnostic_cancelled, {}, Failure::unavailable);
                                settle(index, serial);
                            }
                            poll();
                        },
                        Qt::QueuedConnection);
                } catch (...) {
                }
                if (!posted) {
                    {
                        std::lock_guard lock(s->mutex);
                        --s->status.queued_diagnostics;
                    }
                    event(Kind::diagnostic_cancelled, {}, Failure::resource_limit);
                    settle(index, serial);
                }
            } else {
                auto &record = records[index];
                if (writer_ownership_uncertain) {
                    // This item was never offered to the quarantined writer.
                    refuse(index, serial, Failure::unavailable);
                    continue;
                }
                record.serial = serial;
                StorageError admission = StorageError::invalid_work;
                bool ownership_unknown = false;
                try {
                    admission = writer->enqueue_work(
                        [this, index, serial](auto c) { return execute(index, serial, c); },
                        [this, index, serial](auto r) {
                            receive_work(index, serial, std::move(r));
                        });
                } catch (...) {
                    ownership_unknown = true;
                    {
                        std::lock_guard lock(s->mutex);
                        // Pin the association as potentially retained, WITHOUT
                        // claiming an acknowledged wrapper or a SQL outcome.
                        s->associations[index].accepted = true;
                    }
                    quarantine_writer();
                }
                if (ownership_unknown)
                    continue;
                if (admission == StorageError::none) {
                    record.mismatched = std::exchange(mismatch, false);
                    {
                        std::lock_guard lock(s->mutex);
                        s->associations[index].accepted = true;
                        ++s->status.accepted_wrappers;
                    }
                    event(Kind::wrapper_accepted);
                } else {
                    clear_record(record);
                    refuse(index, serial, Failure::unavailable);
                }
            }
            // Keep accepting arrivals made during observer gates, without pumping
            // the writer. Stop will be consumed before every actual entry.
        }
    }
    void sweep_queued() {
        consume_stop();
        std::lock_guard lock(s->mutex);
        auto now = s->sample(last);
        if (!s->associations)
            return;
        for (std::uint64_t i = 0; i < s->options.limits.associations; ++i) {
            auto &a = s->associations[i];
            if (!a.used || a.active || a.finished)
                continue;
            if (!now) {
                s->cancel_peer(a.peer, Failure::elapsed_unavailable);
            } else if (*now >= a.queue_deadline && !a.cancelled) {
                a.cancelled = Failure::expired;
                s->drop_input(a);
            }
        }
    }
};
Host::Host(Options o, std::unique_ptr<Transport> t, std::unique_ptr<CommandDispatcher> d)
    : impl_(std::make_unique<Impl>(*this, std::move(o), std::move(t), std::move(d))) {}
Host::~Host() {
    auto &i = *impl_;
    if (!i.owner() || i.occupied())
        std::terminate();
    // Destruction is never a substitute for asynchronous stop/join. Refuse to
    // free callback dependencies if an owner barrier has not actually passed.
    if (i.attempted && (!i.s->status.owner_cleanup))
        std::terminate();
    std::lock_guard lock(i.s->mutex);
    i.s->alive = false;
    i.s->context = nullptr;
    i.s->owner_wake = {};
}
Result<StartAccepted> Host::start(StartupObserver done) {
    auto &i = *impl_;
    auto s = i.s;
    if (!i.owner() || !QCoreApplication::instance() ||
        QThread::currentThread() != QCoreApplication::instance()->thread())
        return Failure::wrong_thread;
    if (i.occupied())
        return Failure::unavailable;
    if (i.attempted || s->status.state != State::cold)
        return Failure::already_started;
    Depth turn(s->owner_depth);
    i.attempted = true;
    i.startup = std::move(done);
    {
        std::lock_guard lock(s->mutex);
        s->status.state = State::starting;
    }
    auto fail = [&](Failure reason) -> Result<StartAccepted> {
        i.failed = true;
        i.report_startup(reason);
        s->stop = true;
        i.consume_stop();
        return reason;
    };
    if (!valid_options(s->options) || !i.transport || !i.dispatcher)
        return fail(Failure::invalid_options);
    try {
        s->peers = std::make_unique<Peer[]>(s->options.limits.peers);
        s->time_owners = std::make_unique<TimeOwner[]>(s->options.limits.peers);
        s->associations = std::make_unique<Association[]>(s->options.limits.associations);
        i.records = std::make_unique<Impl::WorkRecord[]>(s->options.limits.associations);
        i.completion_mail =
            std::make_unique<std::optional<SqliteWorkResult>[]>(s->options.limits.associations);
        if (s->hooks.elapsed)
            s->elapsed = s->hooks.elapsed;
        else {
            auto clock = make_linux_monotonic_elapsed();
            if (!std::holds_alternative<std::shared_ptr<const native_ws::ElapsedSource>>(clock))
                return fail(Failure::elapsed_unavailable);
            s->elapsed =
                std::get<std::shared_ptr<const native_ws::ElapsedSource>>(std::move(clock));
        }
        auto profile = native_ws::create_profile(s->options.native, s->elapsed);
        if (!std::holds_alternative<std::unique_ptr<native_ws::Profile>>(profile))
            return fail(Failure::invalid_options);
        i.ingress.reset(new Ingress(std::make_unique<Ingress::Impl>()));
        i.ingress->impl_->shared = s;
        if (s->stop) {
            i.consume_stop();
            return StartAccepted{};
        }
        i.writer = std::make_unique<SqliteWriter>();
        if (i.writer->open(QString::fromStdString(s->options.store_path),
                           s->options.writer_limits) == StorageError::none) {
            if (i.fault) {
                (void)
                    subathon::server::testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *i.writer, *i.fault);
                i.fault.reset();
            }
            i.inspect_store();
        } else
            i.event(Kind::inspection_completed, {}, Failure::unavailable);
        if (!s->stop) {
            if (auto *network = dynamic_cast<NetworkTransport *>(i.transport.get()))
                network->configure(s->options.limits, s->elapsed, s, s->options.store_path);
            i.transport_started = true;
            auto result = i.transport->start(
                *i.ingress, std::get<std::unique_ptr<native_ws::Profile>>(std::move(profile)),
                [&i](auto r) {
                    std::lock_guard lock(i.s->mutex);
                    if (!i.s->ready_mail)
                        i.s->ready_mail = std::move(r);
                    (void)i.s->notify_locked();
                });
            if (!std::holds_alternative<StartAccepted>(result)) {
                return fail(std::holds_alternative<Failure>(result) ? std::get<Failure>(result)
                                                                    : Failure::unavailable);
            }
        }
        i.consume_stop();
        return StartAccepted{};
    } catch (...) {
        return fail(Failure::resource_limit);
    }
}

void Host::request_stop() noexcept {
    auto &i = *impl_;
    auto s = i.s;
    if (s->stop.exchange(true))
        return;
    std::lock_guard lock(s->mutex);
    if (s->alive && s->context)
        (void)s->notify_locked();
}
Inspection Host::inspect() const {
    auto &i = *impl_;
    if (!i.owner())
        return {};
    std::lock_guard lock(i.s->mutex);
    return i.s->status;
}
std::unique_ptr<Host> testing::HostAccess::create(Options o, std::unique_ptr<Transport> t,
                                                  std::unique_ptr<CommandDispatcher> d,
                                                  Hooks hooks) {
    auto h = std::make_unique<Host>(std::move(o), std::move(t), std::move(d));
    h->impl_->s->hooks = std::move(hooks);
    return h;
}
Result<Connected>
testing::HostAccess::inject_next_control_ack_loss(Host &h,
                                                  subathon::server::testing::WorkControlFault f) {
    auto &i = *h.impl_;
    if (!i.owner())
        return Failure::wrong_thread;
    if (i.occupied())
        return Failure::unavailable;
    if (f != subathon::server::testing::WorkControlFault::lose_next_commit_ack &&
        f != subathon::server::testing::WorkControlFault::lose_next_rollback_ack)
        return Failure::invalid_options;
    std::lock_guard lock(i.s->mutex);
    if (i.s->status.associations || i.fault)
        return Failure::unavailable;
    if (!i.writer) {
        if (i.attempted)
            return Failure::unavailable;
        i.fault = f;
        return Connected{};
    }
    auto r = subathon::server::testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
        *i.writer, f);
    return r == StorageError::none ? Result<Connected>(Connected{})
                                   : Result<Connected>(Failure::unavailable);
}
Result<Connected> testing::HostAccess::inject_next_envelope_mismatch(Host &h) {
    auto &i = *h.impl_;
    if (!i.owner())
        return Failure::wrong_thread;
    if (i.occupied())
        return Failure::unavailable;
    std::lock_guard lock(i.s->mutex);
    if (i.s->status.associations || i.mismatch)
        return Failure::unavailable;
    i.mismatch = true;
    return Connected{};
}
testing::ElapsedIdentity testing::HostAccess::elapsed_identity(const Host &h) {
    auto *p = h.impl_->s->elapsed.get();
    return {p, h.impl_->transport_started ? p : nullptr};
}
} // namespace subathon::server::api_host
