// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <climits>
#include <cstring>
#include <drogon/BoundedHttp.h>
#include <drogon/HttpAppFramework.h>
#include <limits>
#include <mutex>
#include <new>
#include <optional>
#include <subathon/server/bounded_http.hpp>
#include <subathon/server/testing/bounded_http_observers.hpp>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <thread>
#include <trantor/net/Channel.h>
#include <trantor/net/EventLoop.h>
#include <unistd.h>

namespace subathon::server::bounded_http {
namespace sdk = drogon::bounded_http_sdk;
namespace io = trantor::bounded_io;
namespace {
constexpr U64 none = std::numeric_limits<U64>::max(), peer_bytes = 102424;
constexpr U64 metadata_stride = 10240; // 2048 first-party + 8192 typed SDK bytes.
// Native projection, poller cell and callback allowances share the same reservation.
static_assert(metadata_stride + 5120 + 128 + 320 <= 16384);
constexpr unsigned peer_bit = 1, request_bit = 2, reply_bit = 4, wake_bit = 8;
struct Core;
struct PeerRecord final : io::PeerStorage {
    Core *core{};
    U64 index{}, incarnation{}, sequence{1}, body_size{}, body_length{}, route{}, input{none},
        output{none};
    U64 accept_deadline{}, request_deadline{}, idle_deadline{}, output_deadline{},
        native_upgrade_deadline{}, ordinal{}, bytes_read{};
    U64 arena_used{}, field_count{}, method_size{}, target_size{}, header_size{}, reply_size{};
    std::byte *payload{}, *metadata{};
    io::Transport *transport{};
    void *context{};
    std::atomic<unsigned> states{0}, drops{0};
    std::atomic<U64> deletes{0}, request_drop{0}, reply_drop{0};
    std::atomic<bool> revoked{false};
    std::atomic<bool> committed{false}, retired_native{false};
    alignas(64) std::byte handles[4][128];
    bool parse_permit{}, live{}, attached_once{}, in_connected{}, wanted{}, reading{}, delivered{}, raw_live{},
        raw_revoked{}, ended{}, socket_gone{}, sdk_gone{}, removed{}, context_set{}, wake_issued{},
        close_reply{}, reserved{}, filled{}, wire_done{}, close_requested{}, quarantined{},
        native{}, native_established{}, native_open_notified{}, terminal_wiped{}, native_retired_seen{}, retiring_charge{},
        native_retiring_charge{}, control{}, request_started{}, has_end{};
    End end{End::cancelled};
    unsigned mailbox{}, executing{};
    int desired{-1};
    std::span<std::byte> receive_with_prefix() noexcept override { return {payload, 65544}; }
    std::span<std::byte> sdk_storage() noexcept override { return {metadata + 2048, 8192}; }
    std::span<std::byte> control_storage() noexcept override;
    bool wipe_receive(std::span<std::byte>, bool) noexcept override;
    void attached(io::Transport &) noexcept override;
    void read_witness(io::ReadWitness) noexcept override;
    void wire_released(void *) noexcept override;
    void socket_removed() noexcept override { socket_gone = true; transport = nullptr; reading = false; revoked.store(true, std::memory_order_release); }
    void native_retired() noexcept override;
    void sdk_objects_released() noexcept override { sdk_gone = true; }
    std::span<std::byte> arena() noexcept { return {payload + 65544, 32768}; }
    std::byte *method() noexcept { return payload + 98312; }
    std::byte *target() noexcept { return payload + 98328; }
    Field *fields() noexcept { return reinterpret_cast<Field *>(payload + 100376); }
    std::byte *control_data() noexcept { return payload + 101400; }
};
static_assert(sizeof(PeerRecord) <= 2048);
struct PoolSlot {
    PeerRecord *owner{};
};
static_assert(sizeof(PoolSlot) <= 128);
struct MetaDelete {
    void operator()(std::byte *p) const noexcept { ::operator delete(p, std::align_val_t(64)); }
};
struct ChannelDestroy {
    void operator()(trantor::Channel *p) const noexcept { if(p) p->~Channel(); }
};
struct Core final : io::Producer, sdk::Sink {
    Limits limits;
    Storage storage;
    std::shared_ptr<const native_ws::ElapsedSource> source;
    Callbacks callbacks;
    NativeBinding native;
    RawHooks hooks{};
    Observation observation{};
    std::array<Route, 8> routes{};
    std::array<std::array<char, 16>, 8> methods{};
    std::array<std::array<char, 2048>, 8> targets{};
    std::array<char, 128> native_target{};
    std::size_t route_count{};
    std::unique_ptr<std::byte, MetaDelete> metadata;
    std::byte *mapping{};
    U64 mapped{}, meta_bytes{};
    int event_fd{-1}, timer_fd{-1};
    // Physical wake-channel backing is allocated with Core before IO0 starts.
    alignas(trantor::Channel) std::byte event_backing[sizeof(trantor::Channel)];
    alignas(trantor::Channel) std::byte timer_backing[sizeof(trantor::Channel)];
    std::unique_ptr<trantor::Channel, ChannelDestroy> event_channel, timer_channel;
    std::atomic<trantor::EventLoop *> loop{nullptr};
    std::thread::id creator{std::this_thread::get_id()};
    std::mutex mutex;
    std::atomic<U64> handle_count{0}, delete_count{0};
    std::atomic<bool> stopping{false}, io_live{false};
    U64 stop_deadline{}, now_ns{}, prior{}, cursor{}, native_upgrade_ms{};
    std::atomic<bool> drained{false}, quarantine{false};
    bool prior_set{}, elapsed_bad{}, registered{}, finished{},
        hook_installed{}, turning{}, sdk_installed{};
    unsigned callbacks_active{}, visitors{}, turn_work[2]{};
    Core(Limits l, Storage s, std::shared_ptr<const native_ws::ElapsedSource> clock, Callbacks cb,
         NativeBinding nb)
        : limits(l), storage(s), source(std::move(clock)), callbacks(cb), native(nb) {
        observation.reserved_storage = s.reserved;
        observation.mapped_storage = s.mapped;
        observation.current.metadata_charge = 65536;
        peaks();
    }
    ~Core() {
        if (sdk_installed && !sdk::Runtime::discard(*this))
            std::terminate();
        std::lock_guard lock(mutex);
        if (mapping)
            ::munmap(mapping, static_cast<std::size_t>(mapped));
        if (event_fd >= 0)
            ::close(event_fd);
        if (timer_fd >= 0)
            ::close(timer_fd);
        if (metadata)
            for (U64 i = 0; i < limits.peers; ++i)
                record(i).~PeerRecord();
    }
    PeerRecord &record(U64 i) noexcept {
        return *reinterpret_cast<PeerRecord *>(metadata.get() + i * metadata_stride);
    }
    PoolSlot &input(U64 i) noexcept {
        return *reinterpret_cast<PoolSlot *>(metadata.get() + limits.peers * metadata_stride +
                                             i * 128);
    }
    PoolSlot &output(U64 i) noexcept {
        return *reinterpret_cast<PoolSlot *>(metadata.get() + limits.peers * metadata_stride +
                                             (limits.input_slots + i) * 128);
    }
    std::byte *input_data(U64 i) noexcept {
        return mapping + limits.peers * peer_bytes + i * limits.body_bytes;
    }
    std::byte *output_data(U64 i) noexcept {
        return mapping + limits.peers * peer_bytes + limits.input_slots * limits.body_bytes +
               i * (limits.reply_bytes + 128);
    }
    U64 peak_pending{}; // Protected by the same mutex as pending/executing records.
    void peaks() noexcept {
        observation.peak.peers=std::max(observation.peak.peers,observation.current.peers);
        observation.peak.http_peers=std::max(observation.peak.http_peers,observation.current.http_peers);
        observation.peak.native_peers=std::max(observation.peak.native_peers,observation.current.native_peers);
        observation.peak.retiring_peers=std::max(observation.peak.retiring_peers,observation.current.retiring_peers);
        observation.peak.input_slots=std::max(observation.peak.input_slots,observation.current.input_slots);
        observation.peak.output_slots=std::max(observation.peak.output_slots,observation.current.output_slots);
        observation.peak.input_capacity=std::max(observation.peak.input_capacity,observation.current.input_capacity);
        observation.peak.output_capacity=std::max(observation.peak.output_capacity,observation.current.output_capacity);
        observation.peak.control_capacity=std::max(observation.peak.control_capacity,observation.current.control_capacity);
        observation.peak.metadata_charge=std::max(observation.peak.metadata_charge,observation.current.metadata_charge);
        observation.peak.transient_fds=std::max(observation.peak.transient_fds,observation.current.transient_fds);
        observation.peak.constructed=std::max(observation.peak.constructed,observation.current.constructed);
        observation.peak.native_retiring=std::max(observation.peak.native_retiring,observation.current.native_retiring);
        observation.peak.sdk_retained_peers=std::max(observation.peak.sdk_retained_peers,observation.current.sdk_retained_peers);
        observation.peak.handles=std::max(observation.peak.handles,handle_count.load(std::memory_order_acquire));
    }
    bool is_io() const noexcept {
        auto *owner = loop.load(std::memory_order_acquire);
        return owner && owner == trantor::EventLoop::getEventLoopOfCurrentThread();
    }
    bool sample_now() noexcept {
        auto value = source->sample();
        auto *tick = std::get_if<Tick>(&value);
        if (!tick || (prior_set && tick->nanoseconds < prior)) {
            elapsed_bad = true;
            return false;
        }
        prior_set = true;
        prior = now_ns = tick->nanoseconds;
        return true;
    }
    bool anchor(U64 &deadline, U64 ms) noexcept {
        if (ms > (none - now_ns) / 1000000) {
            elapsed_bad = true;
            return false;
        }
        deadline = now_ns + ms * 1000000;
        return true;
    }
    void signal() noexcept {
        std::lock_guard lock(mutex);
        if (io_live.load(std::memory_order_acquire) && event_fd >= 0) {
            const U64 one = 1;
            ssize_t result;
            do { result=::write(event_fd, &one, sizeof(one)); } while(result<0 && errno==EINTR);
        }
    }
    template <class F> bool callback(F &&f) noexcept {
        ++callbacks_active;
        try {
            f();
            --callbacks_active;
            return true;
        } catch (...) {
            --callbacks_active;
            return false;
        }
    }
    void reading(PeerRecord &p, bool on) noexcept {
        if (!p.transport || p.socket_gone || p.reading == on) return;
        p.reading = on; // Set before a synchronous parser continuation can suspend it.
        p.transport->reading(on);
    }
    void choose(PeerRecord &p, End end) noexcept {
        if (!p.has_end) {
            p.end = end;
            p.has_end = true;
        }
    }
    void return_unqueued_output(PeerRecord &p) noexcept {
        // A reservation without an actual wire node has no SDK release callback.
        if (!p.committed && p.output != none) {
            output(p.output).owner = nullptr;
            p.output = none;
            --observation.current.output_slots;
            observation.current.output_capacity -= limits.reply_bytes + 128;
            observation.current.metadata_charge -= 128;
        }
    }
    void terminate(PeerRecord &p, End end) noexcept {
        choose(p, end);
        p.close_requested = true;
        p.wanted = false;
        return_unqueued_output(p);
        if (p.transport && !p.socket_gone)
            p.transport->close();
    }
    bool wipe(PeerRecord &p, RawKind kind, RawUse use, std::span<std::byte> bytes) noexcept {
        if (bytes.empty())
            return true;
        bool ok = true;
        if (hook_installed)
            ok = hooks.wipe(hooks.owner,
                            {p.context, use == RawUse::pending ? 0 : p.sequence, kind, use}, bytes);
        else
            std::fill(bytes.begin(), bytes.end(), std::byte{});
        if (!ok) {
            p.quarantined = true;
            quarantine = true;
            terminate(p, End::wipe_failed);
        }
        return ok;
    }
    // Anchor at the actual later terminal operation, before deferred notification.
    void anchor_idle(PeerRecord &p) noexcept {
        if (p.raw_live || !p.wire_done || p.idle_deadline || p.quarantined)
            return;
        if (!sample_now() || !anchor(p.idle_deadline, limits.idle_ms))
            terminate(p, End::elapsed_failed);
    }
    void release_raw(PeerRecord &p) noexcept {
        if (!p.raw_live)
            return;
        p.raw_live = false;
        if (!p.raw_revoked) {
            p.raw_revoked = true;
            if (hook_installed)
                hooks.revoked(hooks.owner, p.context, p.sequence, p.has_end ? p.end : End::drained);
        }
        wipe(p, RawKind::fields, RawUse::current,
             {reinterpret_cast<std::byte *>(p.fields()), 1024});
        wipe(p, RawKind::fields, RawUse::current, p.arena());
        wipe(p, RawKind::method, RawUse::current, {p.method(), 16});
        wipe(p, RawKind::target, RawUse::current, {p.target(), 2048});
        if (p.input != none) {
            wipe(p, RawKind::body, RawUse::current,
                 {input_data(p.input), static_cast<std::size_t>(limits.body_bytes)});
            if (!p.quarantined) {
                input(p.input).owner = nullptr;
                --observation.current.input_slots;
                observation.current.input_capacity -= limits.body_bytes;
                observation.current.metadata_charge -= 128;
                p.input = none;
            }
        }
        ++observation.raw_releases;
        anchor_idle(p);
    }
    void request_started(PeerRecord &p) noexcept {
        if (!p.request_started && guard(p)) {
            if (!anchor(p.request_deadline, limits.request_ms)) {
                terminate(p, End::elapsed_failed);
                return;
            }
            p.request_started = true;
            p.idle_deadline = 0;
        }
    }
    bool guard(PeerRecord &p, bool allow_committed = false) noexcept {
        if (!sample_now()) {
            terminate(p, End::elapsed_failed);
            return false;
        }
        if (stopping.load(std::memory_order_acquire) && !allow_committed) {
            terminate(p, End::stopped);
            return false;
        }
        if (p.native_established && !p.control)
            return !p.close_requested;
        if (p.native && !p.native_established && p.native_upgrade_deadline &&
            now_ns >= p.native_upgrade_deadline) {
            terminate(p, End::expired);
            return false;
        }
        const bool complete = p.delivered && p.wire_done && !p.raw_live;
        const U64 deadline = complete                     ? p.idle_deadline
                             : (p.committed || p.control) ? p.output_deadline
                             : p.request_started          ? p.request_deadline
                             : p.idle_deadline            ? p.idle_deadline
                                                          : p.accept_deadline;
        if (elapsed_bad || !deadline) {
            terminate(p, End::elapsed_failed);
            return false;
        }
        if (now_ns >= deadline) {
            terminate(p, End::expired);
            return false;
        }
        return !p.close_requested;
    }
    io::Sample sample() noexcept override {
        return sample_now() ? io::Sample{true, now_ns} : io::Sample{false, 0};
    }
    bool accepting(io::Role role) noexcept override {
        if (stopping.load(std::memory_order_acquire) || elapsed_bad)
            return false;
        auto &c = observation.current;
        return c.peers < limits.peers &&
               (role == io::Role::http ? c.http_peers < limits.http_peers
                                       : c.native_peers < limits.native_peers);
    }
    bool transient(bool take) noexcept override {
        if (take) {
            if (observation.current.transient_fds)
                return false;
            observation.current.transient_fds = 1;
            peaks();
            ++observation.accept_attempts;
        } else
            observation.current.transient_fds = 0;
        return true;
    }
    io::PeerStorage *acquire(io::Role role, io::Sample accepted) noexcept override {
        if (!accepted.available || !accepting(role))
            return nullptr;
        for (U64 i = 0; i < limits.peers; ++i) {
            auto &p = record(i);
            if (p.live)
                continue;
            if (p.incarnation == none) {
                ++observation.generation_exhaustions;
                continue;
            }
            ++p.incarnation;
            p.live = true;
            p.native = role == io::Role::native;
            p.sequence = 1;
            now_ns = accepted.ns;
            if (!anchor(p.accept_deadline, limits.accept_ms) ||
                (p.native && !anchor(p.native_upgrade_deadline, native_upgrade_ms))) {
                p.live = false;
                return nullptr;
            }
            ++observation.current.peers;
            if (p.native)
                ++observation.current.native_peers;
            else
                ++observation.current.http_peers;
            observation.current.metadata_charge += 16384;
            peaks();
            return &p;
        }
        return nullptr;
    }
    bool before_parse(io::PeerStorage &value) noexcept override {
        auto &p = static_cast<PeerRecord &>(value);
        p.reading = false; // Parsing/read guards can suspend the SDK channel.
        if (stopping.load(std::memory_order_acquire) || p.socket_gone || p.control ||
            !p.wanted || p.delivered || !guard(p))
            return false;
        request_started(p); // SDK invokes this only for actual buffered bytes.
        if (p.close_requested)
            return false;
        auto &work = turn_work[p.native ? 1 : 0];
        if (turning && !p.parse_permit) {
            if (work >= 8) return false;
            ++work;
            p.parse_permit = true;
        }
        return true;
    }
    bool before_read(io::PeerStorage &value, io::ReadWitness w) noexcept override {
        auto &p = static_cast<PeerRecord &>(value);
        p.reading = false; // Parsing/read guards can suspend the SDK channel.
        if (p.socket_gone || p.control || !p.wanted || p.delivered || !guard(p))
            return false;
        if (p.ordinal == none || observation.reads == none || w.offered > none - p.bytes_read ||
            w.offered > none - observation.bytes_read) {
            terminate(p, End::cancelled);
            return false;
        }
        if (!std::holds_alternative<Done>(check_read_window(w.writable))) {
            ++observation.read_guard_pauses;
            return false;
        }
        return true;
    }
    bool before_write(io::PeerStorage &value) noexcept override {
        auto &p = static_cast<PeerRecord &>(value);
        if (!guard(p, true))
            return false;
        if (stopping.load(std::memory_order_acquire) && now_ns >= stop_deadline) {
            terminate(p, End::stopped);
            return false;
        }
        auto &work = turn_work[p.native ? 1 : 0];
        if (turning && work >= 8)
            return false;
        if (turning)
            ++work;
        return !p.close_requested;
    }
    sdk::Decision head(io::PeerStorage &, sdk::Head) noexcept override;
    bool body(io::PeerStorage &value, std::span<const std::byte> bytes) noexcept override {
        auto &p = static_cast<PeerRecord &>(value);
        if (p.input == none || bytes.size() > p.body_length - p.body_size)
            return false;
        std::memcpy(input_data(p.input) + p.body_size, bytes.data(), bytes.size());
        p.body_size += bytes.size();
        return true;
    }
    void complete(io::PeerStorage &) noexcept override;
    void native_event(io::PeerStorage &, const std::shared_ptr<drogon::WebSocketConnection> &,
                      sdk::NativeEvent) noexcept override;
    void native_handoff(io::PeerStorage &, U64 capacity, bool accepted) noexcept override {
        observation.native_handoff_capacity = capacity;
        observation.native_handoff_accepted = accepted;
    }
    void owner_turn() noexcept override;
    void drop(PeerRecord &p, unsigned kind) noexcept {
        delete_count.fetch_add(1, std::memory_order_acq_rel);
        p.deletes.fetch_add(1, std::memory_order_acq_rel);
        if (kind == request_bit)
            p.request_drop.store(p.sequence, std::memory_order_release);
        if (kind == reply_bit)
            p.reply_drop.store(p.sequence, std::memory_order_release);
        p.drops.fetch_or(kind, std::memory_order_release);
        p.states.fetch_and(~kind, std::memory_order_release);
        signal();
        handle_count.fetch_sub(1, std::memory_order_release);
        p.deletes.fetch_sub(1, std::memory_order_release);
        delete_count.fetch_sub(1, std::memory_order_release);
    }
    bool take_input(PeerRecord &p) noexcept {
        if (p.input != none)
            return true;
        for (U64 i = 0; i < limits.input_slots; ++i)
            if (!input(i).owner) {
                input(i).owner = &p;
                p.input = i;
                p.raw_live = true;
                ++observation.current.input_slots;
                observation.current.input_capacity += limits.body_bytes;
                observation.current.metadata_charge += 128;
                peaks();
                return true;
            }
        return false;
    }
};
static_assert(((sizeof(Core) + 63) & ~std::size_t(63)) +
                      2 * ((sizeof(trantor::Channel) + 63) & ~std::size_t(63)) + 128 + 256 <=
                  sdk::first_party_global_metadata_bytes,
              "First-party globals exceed their share of the same G quota");
thread_local struct Descriptor {
    Core *core;
    PeerRecord *peer;
    unsigned kind;
    void *address;
} *allocation = nullptr;
thread_local Descriptor deletion{};
void *state_allocate(std::size_t n, unsigned kind) {
    if (!allocation || allocation->kind != kind || n > 128)
        throw std::bad_alloc();
    auto *p = allocation->address;
    allocation = nullptr;
    return p;
}
void state_delete() noexcept {
    auto d = deletion;
    deletion = {};
    if (!d.core)
        std::terminate();
    d.core->drop(*d.peer, d.kind);
}
struct StateRecord {
    Core *core;
    PeerRecord *peer;
    U64 incarnation, sequence;
    unsigned kind;
    StateRecord(Core &c, PeerRecord &p, unsigned k) noexcept
        : core(&c), peer(&p), incarnation(p.incarnation), sequence(p.sequence), kind(k) {}
    ~StateRecord() { deletion = {core, peer, kind, this}; }
};
bool ascii_equal(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        auto x = a[i], y = b[i];
        if (x >= 'A' && x <= 'Z')
            x = static_cast<char>(x + 32);
        if (y >= 'A' && y <= 'Z')
            y = static_cast<char>(y + 32);
        if (x != y)
            return false;
    }
    return true;
}
// Validate authority syntax before any native projection or ordinary callback.
bool valid_host(std::string_view value) noexcept {
    if (value.empty() || value.find_first_of(" \t@%/?#") != value.npos)
        return false;
    std::string_view name = value, port;
    if (value.front() == '[') {
        const auto end = value.find(']');
        if (end == value.npos || end == 1 || end - 1 >= INET6_ADDRSTRLEN)
            return false;
        std::array<char, INET6_ADDRSTRLEN> text{};
        std::memcpy(text.data(), value.data() + 1, end - 1);
        in6_addr address{};
        if (::inet_pton(AF_INET6, text.data(), &address) != 1)
            return false;
        if (end + 1 == value.size())
            return true;
        if (value[end + 1] != ':')
            return false;
        port = value.substr(end + 2);
    } else {
        const auto colon = value.find(':');
        if (colon != value.npos) {
            name = value.substr(0, colon);
            port = value.substr(colon + 1);
        }
        if (name.empty())
            return false;
        for (unsigned char c : name)
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                  c == '-' || c == '.'))
                return false;
        if (colon == value.npos)
            return true;
    }
    if (port.empty())
        return false;
    U64 number = 0;
    for (unsigned char c : port) {
        if (c < '0' || c > '9' || number > (65535 - (c - '0')) / 10)
            return false;
        number = number * 10 + c - '0';
    }
    return true;
}
// Validate trusted route targets before framework registration.
bool valid_target(std::string_view value) noexcept {
    if (value.empty() || value.front() != '/' || value.find_first_of("?#") != value.npos)
        return false;
    for (unsigned char c : value)
        if (c <= 32 || c == 127)
            return false;
    return true;
}
// Canonical base64 for exactly sixteen bytes has twenty-two digits and two '='.
bool valid_key(std::string_view value) noexcept {
    constexpr std::string_view alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (value.size() != 24 || value.substr(22) != "==")
        return false;
    for (std::size_t i = 0; i < 22; ++i)
        if (alphabet.find(value[i]) == alphabet.npos)
            return false;
    return alphabet.find(value[21]) % 16 == 0;
}
bool token(std::string_view s) noexcept {
    if (s.empty())
        return false;
    for (unsigned char c : s)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              std::string_view("!#$%&'*+-.^_`|~").find(static_cast<char>(c)) !=
                  std::string_view::npos))
            return false;
    return true;
}
} // namespace

#define HANDLE_STATE(Name, Kind)                                                                   \
    struct Name::State : StateRecord {                                                             \
        State(Core &c, PeerRecord &p) noexcept : StateRecord(c, p, Kind) {}                        \
        static void *operator new(std::size_t n) {                                                 \
            if (n != sizeof(State))                                                                \
                throw std::bad_alloc();                                                            \
            return state_allocate(n, Kind);                                                        \
        }                                                                                          \
        static void operator delete(void *) noexcept { state_delete(); }                           \
    };                                                                                             \
    Name::Name(std::unique_ptr<State> s) noexcept : state_(std::move(s)) {}                        \
    Name::Name(Name &&) noexcept = default;                                                        \
    Name &Name::operator=(Name &&) noexcept = default;                                             \
    Name::~Name() = default;
HANDLE_STATE(Peer, peer_bit)
HANDLE_STATE(Request, request_bit)
HANDLE_STATE(Reply, reply_bit)
HANDLE_STATE(Wake, wake_bit)
#undef HANDLE_STATE
struct Foundation::State {
    std::unique_ptr<Core> core;
    explicit State(std::unique_ptr<Core> c) : core(std::move(c)) {}
};
struct Factory {
    template <class H> static H issue(Core &c, PeerRecord &p, unsigned bit, unsigned slot) {
        if (p.states.fetch_or(bit, std::memory_order_acq_rel) & bit)
            throw std::bad_alloc();
        Descriptor descriptor{&c, &p, bit, p.handles[slot]};
        allocation = &descriptor;
        try {
            std::unique_ptr<typename H::State> s(new typename H::State(c, p));
            allocation = nullptr;
            c.handle_count.fetch_add(1, std::memory_order_release);
            c.peaks();
            return H(std::move(s));
        } catch (...) {
            allocation = nullptr;
            p.states.fetch_and(~bit, std::memory_order_release);
            throw;
        }
    }
    static Result<std::unique_ptr<Foundation>> make(std::unique_ptr<Core> c) noexcept {
        try {
            return std::unique_ptr<Foundation>(
                new Foundation(std::make_unique<Foundation::State>(std::move(c))));
        } catch (...) {
            return Error::allocation_failure;
        }
    }
    static Core &core(Foundation &f) noexcept { return *f.state_->core; }
    static StateRecord *state(const Peer &h) noexcept { return h.state_.get(); }
    static StateRecord *state(const Wake &h) noexcept { return h.state_.get(); }
};
namespace {
std::span<std::byte> PeerRecord::control_storage() noexcept {
    if (control || socket_gone || close_requested)
        return {};
    if (!core->guard(*this))
        return {};
    if (!core->anchor(output_deadline, core->limits.output_ms)) {
        core->terminate(*this, End::elapsed_failed);
        return {};
    }
    control = true;
    core->observation.current.control_capacity += 1024;
    core->peaks();
    return {control_data(), 1024};
}
bool PeerRecord::wipe_receive(std::span<std::byte> bytes, bool pending) noexcept {
    if (bytes.data() < payload || bytes.data() + bytes.size() > payload + 65544)
        return false;
    return core->wipe(*this, RawKind::receive, pending ? RawUse::pending : RawUse::current, bytes);
}
void PeerRecord::native_retired() noexcept {
    auto *c = core;
    c->delete_count.fetch_add(1, std::memory_order_acq_rel);
    deletes.fetch_add(1, std::memory_order_acq_rel);
    retired_native.store(true, std::memory_order_release);
    c->signal();
    deletes.fetch_sub(1, std::memory_order_release);
    c->delete_count.fetch_sub(1, std::memory_order_release); // Last core access.
}
void PeerRecord::attached(io::Transport &t) noexcept {
    transport = &t;
    attached_once = true;
    core->peaks();
    try {
        auto h = Factory::issue<Peer>(*core, *this, peer_bit, 0);
        in_connected = true;
        if (!core->callback(
                [&] { core->callbacks.connected(core->callbacks.owner, std::move(h)); }))
            core->terminate(*this, End::callback_failed);
        in_connected = false;
    } catch (...) {
        in_connected = false;
        core->terminate(*this, End::callback_failed);
    }
    // connected has returned: claim the input slot and arm the actual channel now.
    core->owner_turn();
}
void PeerRecord::read_witness(io::ReadWitness w) noexcept {
    auto &o = core->observation;
    const auto positive = w.returned > 0 ? static_cast<U64>(w.returned) : 0;
    if (ordinal == none || o.reads == none || positive > none - bytes_read ||
        positive > none - o.bytes_read) {
        core->terminate(*this, End::cancelled);
        return;
    }
    ++ordinal;
    ++o.reads;
    if (w.returned > 0) {
        bytes_read += static_cast<U64>(w.returned);
        o.bytes_read += static_cast<U64>(w.returned);
        core->request_started(*this);
    }
    if (w.iovcnt == 1)
        ++o.one_iovec_reads;
    o.max_iovcnt = std::max<U64>(o.max_iovcnt, w.iovcnt);
    o.read_min_writable = o.reads == 1 ? w.writable : std::min(o.read_min_writable,w.writable);
    o.last_read_head = w.head;
    o.last_read_tail = w.tail;
    o.last_read_writable = w.writable;
    o.last_read_offered = w.offered;
    o.last_read_returned = w.returned;
    o.last_read_error = w.error;
    if (core->callbacks.read_observed && !core->callback([&] {
            core->callbacks.read_observed(core->callbacks.owner, context,
                                          {ordinal, bytes_read, w.head, w.tail, w.writable,
                                           w.offered, w.returned, w.error, w.iovcnt});
        }))
        core->terminate(*this, End::callback_failed);
}
void PeerRecord::wire_released(void *cookie) noexcept {
    auto &o = core->observation;
    if (cookie == control_data()) {
        control = false;
        o.current.control_capacity -= 1024;
        close_requested = !native_established;
    } else if (output != none && cookie == core->output_data(output)) {
        core->output(output).owner = nullptr;
        output = none;
        --o.current.output_slots;
        o.current.output_capacity -= core->limits.reply_bytes + 128;
        o.current.metadata_charge -= 128;
        ++o.wire_releases;
        wire_done = true;
        core->choose(*this, socket_gone ? End::disconnected : close_requested ? End::cancelled : End::drained);
        core->anchor_idle(*this);
        if (close_reply)
            close_requested = true;
    }
}
sdk::Decision Core::head(io::PeerStorage &value, sdk::Head h) noexcept {
    auto &p = static_cast<PeerRecord &>(value);
    if (h.raw_bytes > 40960 || h.field_count > 64 || h.max_field_bytes > 8192 ||
        h.semantic_bytes > 32768)
        return {431, 0};
    if (h.line_bytes > 2048 || h.target.size() > 2048)
        return {414, 0};
    if (h.method.empty() || h.method.size() > 16)
        return {400, 0};
    U64 length = 0;
    bool seen_length = false, syntax = h.syntax_bad;
    unsigned hosts = 0, correlations = 0, keys = 0, versions = 0, upgrades = 0, connections = 0;
    bool upgrade_token = false, native_host_over = false;
    bool close = false;
    for (auto f : h.fields) {
        auto str = [&](std::uint32_t off, std::uint32_t n) {
            return std::string_view(reinterpret_cast<const char *>(h.raw.data() + off), n);
        };
        auto name = str(f.name, f.name_size), v = str(f.value, f.value_size);
        if (ascii_equal(name, "content-length")) {
            if (seen_length || v.empty())
                return {400, 0};
            seen_length = true;
            for (unsigned char c : v) {
                if (c < '0' || c > '9' || length > (none - (c - '0')) / 10)
                    return {400, 0};
                length = length * 10 + (c - '0');
            }
        }
        if (ascii_equal(name, "x-host-correlation") && ++correlations > 1)
            return {400, 0};
        if (ascii_equal(name, "transfer-encoding") || ascii_equal(name, "te") ||
            ascii_equal(name, "trailer") || ascii_equal(name, "expect") ||
            (!p.native && ascii_equal(name, "upgrade")) || ascii_equal(name, "proxy-connection"))
            return {400, 0};
        if (p.native) {
            if (ascii_equal(name, "sec-websocket-key") && (++keys != 1 || !valid_key(v)))
                syntax = true;
            if (ascii_equal(name, "sec-websocket-version") && (++versions != 1 || v != "13"))
                syntax = true;
            if (ascii_equal(name, "upgrade") && (++upgrades != 1 || !ascii_equal(v, "websocket")))
                syntax = true;
            if (ascii_equal(name, "sec-websocket-protocol") ||
                ascii_equal(name, "sec-websocket-extensions"))
                syntax = true;
        }
        if (ascii_equal(name, "connection")) {
            ++connections;
            std::size_t at = 0;
            while (at < v.size()) {
                auto end = v.find(',', at);
                if (end == v.npos)
                    end = v.size();
                auto item = v.substr(at, end - at);
                while (!item.empty() && (item.front() == ' ' || item.front() == '\t'))
                    item.remove_prefix(1);
                while (!item.empty() && (item.back() == ' ' || item.back() == '\t'))
                    item.remove_suffix(1);
                if (ascii_equal(item, "close"))
                    close = true;
                else if (p.native && ascii_equal(item, "upgrade"))
                    upgrade_token = true;
                else if (!ascii_equal(item, "keep-alive"))
                    return {400, 0};
                at = end + 1;
            }
        }
        if (ascii_equal(name, "host")) {
            ++hosts;
            if (p.native && v.size() > 256)
                native_host_over = true;
            if (!valid_host(v))
                syntax = true;
        }
        if (!token(name))
            syntax = true;
        for (unsigned char c : v)
            if (c == 127 || (c < 32 && c != '\t'))
                syntax = true;
    }
    if (p.native) {
        if (native_host_over)
            return {431, 0};
        if (syntax || hosts != 1 || keys != 1 || versions != 1 || upgrades != 1 ||
            connections != 1 || !upgrade_token || close || length || h.method != "GET" ||
            h.target != native.target)
            return {400, 0};
        return {0, 0};
    }
    U64 route_index = none;
    for (U64 i = 0; i < route_count; ++i)
        if (routes[i].method == h.method && routes[i].target == h.target) {
            route_index = i;
            break;
        }
    const U64 header = h.method.size() + h.target.size() + 12 + h.semantic_bytes;
    if (route_index != none && routes[route_index].header_semantic_ceiling &&
        header > routes[route_index].header_semantic_ceiling)
        return {431, 0};
    if (length > (route_index == none ? limits.body_bytes : routes[route_index].body_ceiling))
        return {413, 0};
    if (route_index != none && routes[route_index].total_semantic_ceiling &&
        (length > none - header || header + length > routes[route_index].total_semantic_ceiling))
        return {431, 0};
    if (syntax || hosts != 1 || !token(h.method) || h.target.empty() || h.target.front() != '/' ||
        h.target.find_first_of("?#") != h.target.npos || route_index == none || p.native)
        return {400, 0};
    for (unsigned char c : h.target)
        if (c <= 32 || c == 127)
            return {400, 0};
    p.route = route_index;
    p.body_length = length;
    p.body_size = 0;
    p.close_reply = close;
    p.method_size = h.method.size();
    p.target_size = h.target.size();
    p.field_count = h.fields.size();
    p.arena_used = 0;
    std::memcpy(p.method(), h.method.data(), h.method.size());
    std::memcpy(p.target(), h.target.data(), h.target.size());
    for (std::size_t i = 0; i < h.fields.size(); ++i) {
        auto f = h.fields[i];
        auto &d = p.fields()[i];
        d.name = static_cast<std::uint32_t>(p.arena_used);
        d.name_size = f.name_size;
        std::memcpy(p.arena().data() + p.arena_used, h.raw.data() + f.name, f.name_size);
        p.arena_used += f.name_size;
        d.value = static_cast<std::uint32_t>(p.arena_used);
        d.value_size = f.value_size;
        std::memcpy(p.arena().data() + p.arena_used, h.raw.data() + f.value, f.value_size);
        p.arena_used += f.value_size;
    }
    return {0, length};
}
void Core::native_event(io::PeerStorage &value,
                        const std::shared_ptr<drogon::WebSocketConnection> &connection,
                        sdk::NativeEvent event) noexcept {
    auto &p = static_cast<PeerRecord &>(value);
    if (event == sdk::NativeEvent::closed && !p.native_open_notified)
        return;
    if (event != sdk::NativeEvent::closed && !guard(p))
        return;
    if (event == sdk::NativeEvent::opened) {
        p.native_established = true;
        p.native_open_notified = native.opened != nullptr;

    }
    if (event == sdk::NativeEvent::closed) p.native_open_notified = false;
    auto function = event == sdk::NativeEvent::opened    ? native.opened
                    : event == sdk::NativeEvent::message ? native.message
                                                         : native.closed;
    if (function && !callback([&] { function(callbacks.owner, p.context, connection); }))
        terminate(p, End::callback_failed);
}
void Core::complete(io::PeerStorage &value) noexcept {
    auto &p = static_cast<PeerRecord &>(value);
    if (!guard(p))
        return;
    p.delivered = true;
    ++observation.requests;
    try {
        auto request = Factory::issue<Request>(*this, p, request_bit, 1);
        if (!callback([&] { callbacks.request(callbacks.owner, p.context, std::move(request)); }))
            terminate(p, End::callback_failed);
    } catch (...) {
        terminate(p, End::callback_failed);
    }
}
void Core::owner_turn() noexcept {
    if (turning)
        return;
    turning = true;
    turn_work[0] = turn_work[1] = 0;
    for (U64 i=0; i<limits.peers; ++i) record(i).parse_permit=false;
    if (!loop) {
        loop = trantor::EventLoop::getEventLoopOfCurrentThread();
        io_live.store(true, std::memory_order_release);
        if (!sdk::Runtime::bind_wake(*this, event_fd))
            std::terminate();
        event_channel.reset(new(event_backing) trantor::Channel(loop, event_fd));
        timer_channel.reset(new(timer_backing) trantor::Channel(loop, timer_fd));
        auto ready = [this] {
            U64 n;
            while (::read(event_fd, &n, sizeof(n)) > 0) {
            }
            while (::read(timer_fd, &n, sizeof(n)) > 0) {
            }
            owner_turn();
        };
        event_channel->setReadCallback(ready);
        timer_channel->setReadCallback(ready);
        event_channel->enableReading();
        timer_channel->enableReading();
    }
    (void)sample_now();
    for (U64 i = 0; i < limits.peers; ++i) {
        auto &p = record(i);
        if (!p.live || p.removed)
            continue;
        const auto drops = p.drops.exchange(0, std::memory_order_acq_rel);
        if (drops & peer_bit)
            terminate(p, End::cancelled);
        const bool current_request_drop =
            (drops & request_bit) && p.request_drop.load(std::memory_order_acquire) == p.sequence;
        const bool current_reply_drop =
            (drops & reply_bit) && p.reply_drop.load(std::memory_order_acquire) == p.sequence;
        if ((current_request_drop || current_reply_drop) && p.delivered) {
            if (!p.committed)
                terminate(p, End::cancelled);
            else if (current_request_drop)
                release_raw(p);
        }
        if (stopping.load(std::memory_order_acquire) &&
            (!p.committed || p.wire_done || now_ns >= stop_deadline))
            terminate(p, End::stopped);
        else if (elapsed_bad)
            terminate(p, End::elapsed_failed);
        else if (!p.close_requested && (!p.native_established || p.control))
            (void)guard(p, true);
        if (p.close_requested && p.transport && !p.socket_gone)
            p.transport->close();
    }
    const bool stop = stopping.load(std::memory_order_acquire);
    const bool acceptors_gone = stop && sdk::Runtime::stop_acceptors();
    sdk::Runtime::poll();
    for (U64 turn = 0; turn < limits.peers; ++turn) {
        auto &p = record((cursor + turn) % limits.peers);
        if (!p.live)
            continue;
        if (p.retired_native.exchange(false, std::memory_order_acq_rel)) {
            p.native_retired_seen = true;
            ++observation.native_retires;
        }
        if (p.socket_gone && !p.retiring_charge && !p.removed) {
            p.retiring_charge = true;
            ++observation.current.retiring_peers;
            peaks();
        }
        if (p.socket_gone && p.native && p.attached_once && !p.native_retired_seen &&
            !p.native_retiring_charge) {
            p.native_retiring_charge = true;
            ++observation.current.native_retiring;
            peaks();
        }
        if (p.native_retired_seen && p.native_retiring_charge) {
            p.native_retiring_charge = false;
            --observation.current.native_retiring;
        }
        if (p.socket_gone) {
            return_unqueued_output(p);
            if (!p.has_end)
                choose(p, End::disconnected);
            release_raw(p);
            // All SDK callbacks (including any wire-node return) completed in poll.
            // A reserved control slot may never have acquired a node.
            if (p.control) {
                p.control = false;
                observation.current.control_capacity -= 1024;
            }
            if (!p.terminal_wiped) {
                p.terminal_wiped = true;
                wipe(p, RawKind::receive, p.delivered ? RawUse::pending : RawUse::current,
                     p.receive_with_prefix());
                wipe(p, RawKind::receive, RawUse::owner_teardown, p.receive_with_prefix());
            }
        }
        if (p.delivered && !p.ended && !p.raw_live && (p.wire_done || p.socket_gone)) {
            p.ended = true;
            if (!p.has_end)
                choose(p, End::cancelled);
            if (!callback([&] { callbacks.ended(callbacks.owner, p.context, p.end); }))
                terminate(p, End::callback_failed);
            anchor_idle(p);
        }
        if (p.socket_gone && p.sdk_gone && !p.removed &&
            !p.deletes.load(std::memory_order_acquire) &&
            (!p.native || !p.attached_once || p.native_retired_seen)) {
            {
                std::lock_guard lock(mutex);
                p.removed = true;
                // A removed peer has no work to deliver, even while handles pin it.
                p.mailbox = p.executing = 0;
                p.desired = -1;
            }
            if (p.retiring_charge) {
                p.retiring_charge = false;
                --observation.current.retiring_peers;
            }
            p.revoked.store(true, std::memory_order_release);
            p.transport = nullptr;
            if (p.attached_once) {
                (void)callback([&] { callbacks.removed(callbacks.owner, p.context); });
            }
        }
        if (p.removed) {
            if (!p.states.load(std::memory_order_acquire) &&
                !p.deletes.load(std::memory_order_acquire) && !p.quarantined) {
                --observation.current.peers;
                if (p.native)
                    --observation.current.native_peers;
                else
                    --observation.current.http_peers;
                observation.current.metadata_charge -= 16384;
                const auto index = p.index, incarnation = p.incarnation;
                auto *data = p.payload, *meta = p.metadata;
                // Any-thread post scans these records under the same mutex.
                std::lock_guard lock(mutex);
                p.~PeerRecord();
                new (&p) PeerRecord;
                p.core = this;
                p.index = index;
                p.incarnation = incarnation;
                p.payload = data;
                p.metadata = meta;
            }
            continue;
        }
        unsigned intents = 0;
        int desired = -1;
        {
            std::lock_guard lock(mutex);
            intents = p.mailbox;
            if (!(intents & 2) && turn_work[p.native ? 1 : 0] >= 8) {
                intents &= ~1u; // Leave one coalesced ready record for the next turn.
                p.mailbox &= 1u;
            } else
                p.mailbox = 0;
            desired = p.desired;
            p.desired = -1;
            p.executing = intents | (desired >= 0 ? 4u : 0u);
        }
        if (intents & 2)
            terminate(p, End::cancelled);
        else {
            if (desired >= 0)
                p.wanted = desired != 0;
            if ((intents & 1) && !stop && !p.socket_gone && !p.close_requested && guard(p)) {
                ++turn_work[p.native ? 1 : 0];
                if (!callback([&] { callbacks.ready(callbacks.owner, p.context); }))
                    terminate(p, End::callback_failed);
            }
        }
        { std::lock_guard lock(mutex); p.executing = 0; }
        if (!stop && !p.socket_gone && !p.control && !p.close_requested && p.wanted) {
            if (p.ended &&
                !(p.states.load(std::memory_order_acquire) & (request_bit | reply_bit))) {
                if (p.sequence == none) {
                    ++observation.generation_exhaustions;
                    terminate(p, End::cancelled);
                    continue;
                }
                ++p.sequence;
                p.ended = p.delivered = p.reserved = p.filled = p.wire_done = p.has_end =
                    p.raw_revoked = p.request_started = false;
                p.committed.store(false, std::memory_order_release);
                p.request_deadline = p.output_deadline = 0;
            }
            if (p.transport && !p.native_established && !p.delivered && (p.native || take_input(p)) && guard(p))
                reading(p, true);
        } else if (p.transport && !p.socket_gone)
            reading(p, false);
    }
    cursor = (cursor + 1) % limits.peers;
    if (stop && acceptors_gone && !drained) {
        bool all = true;
        for (U64 i = 0; i < limits.peers; ++i) {
            auto &p = record(i);
            if (p.live && !p.removed)
                all = false;
        }
        if (all && !callbacks_active) {
            event_channel->disableAll();
            event_channel->remove();
            timer_channel->disableAll();
            timer_channel->remove();
            io_live.store(false, std::memory_order_release);
            drained = true;
            (void)callback([&] { callbacks.stop_drained(callbacks.owner); });
        }
    }
    turning = false;
}
Error state_guard(const StateRecord *s, bool raw = false) noexcept {
    if (!s)
        return Error::revoked;
    if (!s->core->is_io())
        return Error::wrong_thread;
    auto &p = *s->peer;
    if (!p.live || p.incarnation != s->incarnation ||
        ((s->kind & (request_bit | reply_bit)) && p.sequence != s->sequence) || p.removed || p.socket_gone ||
        (raw && !p.raw_live))
        return Error::revoked;
    return Error::unavailable;
}
} // namespace

Result<Done> Peer::context(void *value) noexcept {
    if (auto e = state_guard(state_.get()); e != Error::unavailable)
        return e;
    auto &p = *state_->peer;
    if (!value || p.context_set || !p.in_connected)
        return Error::invalid_state;
    p.context = value;
    p.context_set = true;
    return Done{};
}
Result<Wake> Peer::wake() noexcept {
    if (auto e = state_guard(state_.get()); e != Error::unavailable)
        return e;
    auto &p = *state_->peer;
    if (p.wake_issued || !p.in_connected)
        return Error::invalid_state;
    try {
        auto h = Factory::issue<Wake>(*state_->core, p, wake_bit, 3);
        p.wake_issued = true;
        return h;
    } catch (...) {
        return Error::allocation_failure;
    }
}
Result<Done> Request::visit(void *owner, void (*visitor)(void *, RequestView)) const noexcept {
    if (auto e = state_guard(state_.get(), true); e != Error::unavailable)
        return e;
    auto &c = *state_->core;
    auto &p = *state_->peer;
    if (c.visitors)
        return Error::reentrant;
    if (!visitor)
        return Error::invalid_config;
    if (!c.guard(p))
        return p.end == End::expired ? Error::expired : Error::revoked;
    ++c.visitors;
    const bool ok = c.callback([&] {
        visitor(
            owner,
            {static_cast<std::uint32_t>(p.route),
             {reinterpret_cast<const char *>(p.method()), static_cast<std::size_t>(p.method_size)},
             {reinterpret_cast<const char *>(p.target()), static_cast<std::size_t>(p.target_size)},
             {p.fields(), static_cast<std::size_t>(p.field_count)},
             p.arena().first(static_cast<std::size_t>(p.arena_used)),
             {c.input_data(p.input), static_cast<std::size_t>(p.body_size)}});
    });
    --c.visitors;
    if (!ok) {
        c.terminate(p, End::callback_failed);
        return Error::callback_failed;
    }
    return Done{};
}
Result<Reply> Request::reserve_reply(ReplySize size) noexcept {
    if (auto e = state_guard(state_.get()); e != Error::unavailable)
        return e;
    auto &c = *state_->core;
    auto &p = *state_->peer;
    if (c.visitors)
        return Error::reentrant;
    if (p.reserved)
        return Error::already_replied;
    if (size.status != 200 || size.bytes > c.limits.reply_bytes)
        return Error::invalid_size;
    if (!c.guard(p))
        return p.end == End::expired ? Error::expired : Error::revoked;
    for (U64 i = 0; i < c.limits.output_slots; ++i)
        if (!c.output(i).owner) {
            c.output(i).owner = &p;
            p.output = i;
            p.reply_size = size.bytes;
            p.close_reply = p.close_reply || size.close;
            ++c.observation.current.output_slots;
            c.observation.current.output_capacity += c.limits.reply_bytes + 128;
            c.observation.current.metadata_charge += 128;
            c.peaks();
            p.header_size = sdk::Runtime::render_reply(
                {c.output_data(i), static_cast<std::size_t>(c.limits.reply_bytes + 128)},
                size.status, size.bytes, p.close_reply);
            if (!p.header_size) {
                c.terminate(p, End::cancelled);
                return Error::invalid_state;
            }
            try {
                auto h = Factory::issue<Reply>(c, p, reply_bit, 2);
                p.reserved = true;
                return h;
            } catch (...) {
                c.terminate(p, End::cancelled);
                return Error::allocation_failure;
            }
        }
    return Error::resource_limit;
}
Result<Done> Reply::fill(void *owner, U64 (*visitor)(void *, std::span<std::byte>)) noexcept {
    if (auto e = state_guard(state_.get()); e != Error::unavailable)
        return e;
    auto &c = *state_->core;
    auto &p = *state_->peer;
    if (c.visitors)
        return Error::reentrant;
    if (p.filled || p.committed)
        return Error::invalid_state;
    if (!visitor)
        return Error::invalid_config;
    if (!c.guard(p))
        return p.end == End::expired ? Error::expired : Error::revoked;
    U64 n = none;
    ++c.visitors;
    const bool ok = c.callback([&] {
        n = visitor(owner, {c.output_data(p.output) + p.header_size,
                            static_cast<std::size_t>(p.reply_size)});
    });
    --c.visitors;
    if (!ok) {
        c.terminate(p, End::callback_failed);
        return Error::callback_failed;
    }
    if (n != p.reply_size) {
        c.terminate(p, End::cancelled);
        return Error::invalid_size;
    }
    p.filled = true;
    return Done{};
}
Result<Done> Reply::commit() noexcept {
    if (auto e = state_guard(state_.get()); e != Error::unavailable)
        return e;
    auto &c = *state_->core;
    auto &p = *state_->peer;
    if (c.visitors)
        return Error::reentrant;
    if (!p.filled || p.committed)
        return Error::invalid_state;
    if (!c.guard(p))
        return p.end == End::expired ? Error::expired : Error::revoked;
    if (!c.anchor(p.output_deadline, c.limits.output_ms))
        return Error::elapsed_unavailable;
    if (!p.transport || p.socket_gone)
        return Error::revoked;
    if (!p.transport->wire(
            {c.output_data(p.output), static_cast<std::size_t>(p.header_size + p.reply_size)},
            c.output_data(p.output)))
        return Error::unavailable;
    p.committed.store(true, std::memory_order_release);
    return Done{};
}
void Request::release_raw() noexcept {
    if (!state_)
        return;
    auto &c = *state_->core;
    if (!c.is_io())
        std::terminate();
    if (c.visitors) {
        c.terminate(*state_->peer, End::callback_failed);
        return;
    }
    c.release_raw(*state_->peer);
}
void Reply::cancel() noexcept {
    if (!state_)
        return;
    auto &c = *state_->core;
    if (!c.is_io())
        std::terminate();
    c.terminate(*state_->peer, End::cancelled);
}
Foundation::Foundation(std::unique_ptr<State> s) noexcept : state_(std::move(s)) {}
Foundation::~Foundation() {
    if (!state_)
        return;
    auto &c = *state_->core;
    if (std::this_thread::get_id() != c.creator ||
        (!c.finished && (c.loop || c.handle_count.load(std::memory_order_acquire) ||
                         c.delete_count.load(std::memory_order_acquire))))
        std::terminate();
    if (c.sdk_installed) {
        if (!sdk::Runtime::discard(c))
            std::terminate();
        c.sdk_installed = false;
    }
}
Result<Done> Foundation::admission(const Peer &h, Pause pause) noexcept {
    auto &c = *state_->core;
    if (!c.is_io())
        return Error::wrong_thread;
    if (!Factory::state(h) || Factory::state(h)->core != &c)
        return Error::revoked;
    auto &p = *Factory::state(h)->peer;
    if (p.removed || p.socket_gone)
        return Error::revoked;
    p.wanted = pause == Pause::admitted;
    if (!p.wanted && p.transport && !p.socket_gone)
        c.reading(p, false);
    if (!c.callbacks_active && !c.visitors)
        c.owner_turn();
    return Done{};
}
Result<Done> Foundation::close(const Peer &h) noexcept {
    auto &c = *state_->core;
    if (!c.is_io())
        return Error::wrong_thread;
    if (!Factory::state(h) || Factory::state(h)->core != &c || Factory::state(h)->peer->removed)
        return Error::revoked;
    c.terminate(*Factory::state(h)->peer, End::cancelled);
    return Done{};
}
Result<Observation> Foundation::inspect() const noexcept {
    auto &c = *state_->core;
    if (!c.is_io())
        return Error::wrong_thread;
    c.peaks();
    sdk::Facts facts{};
    if (!sdk::Runtime::observe(c,facts)) return Error::unavailable;
    c.observation.current.constructed=facts.constructed;
    c.observation.current.sdk_retained_peers=facts.sdk_retained_peers;
    c.observation.peak.constructed=facts.constructed;
    c.observation.peak.sdk_retained_peers=facts.peak_sdk_retained_peers;
    c.observation.stock_maps=facts.stock_maps;
    c.observation.stock_batches=facts.stock_batches;
    c.observation.stock_output_appends=facts.stock_output_appends;
    c.observation.native_projected_fields=facts.native_projected_fields;
    c.observation.native_projected_cookies=facts.native_projected_cookies;
    auto o = c.observation;
    o.held_application_wire = sdk::Runtime::wire_held() ? 1 : 0;
    o.current.handles = c.handle_count.load(std::memory_order_acquire);
    o.current.pending_wakes = 0;
    {
        std::lock_guard lock(c.mutex);
        for (U64 i = 0; i < c.limits.peers; ++i)
            if (c.record(i).mailbox || c.record(i).desired >= 0 || c.record(i).executing)
                ++o.current.pending_wakes;
        o.peak.pending_wakes=c.peak_pending;
    }
    return o;
}
Result<Post> Foundation::post(const Wake &h, Intent intent) noexcept {
    auto &c = *state_->core;
    if (!Factory::state(h) || Factory::state(h)->core != &c)
        return Error::revoked;
    std::unique_lock lock(c.mutex, std::defer_lock);
    try {
        lock.lock();
    } catch (...) {
        return Error::unavailable;
    }
    if (c.stopping.load(std::memory_order_acquire))
        return Error::stopping;
    if (!c.io_live.load(std::memory_order_acquire))
        return Error::revoked;
    auto &p = *Factory::state(h)->peer;
    if (p.revoked.load(std::memory_order_acquire))
        return Error::revoked;
    const bool existing = p.mailbox != 0 || p.desired >= 0;
    if (intent == Intent::close) {
        p.mailbox = 2;
        p.desired = -1;
    } else if (!(p.mailbox & 2)) {
        if (intent == Intent::owner_ready)
            p.mailbox |= 1;
        else
            p.desired = intent == Intent::admit ? 1 : 0;
    }
    U64 pending=0;
    for (U64 i=0;i<c.limits.peers;++i)
        if(c.record(i).mailbox || c.record(i).desired>=0 || c.record(i).executing) ++pending;
    c.peak_pending=std::max(c.peak_pending,pending);
    const U64 one = 1;
    const auto n = ::write(c.event_fd, &one, sizeof(one));
    const int e = errno;
    if (n != sizeof(one) && e != EAGAIN)
        return Post::wake_uncertain;
    return existing ? Post::coalesced : Post::queued;
}
Result<Post> Foundation::stop(Tick deadline) noexcept {
    auto &c = *state_->core;
    std::unique_lock lock(c.mutex, std::defer_lock);
    try {
        lock.lock();
    } catch (...) {
        return Error::unavailable;
    }
    if (c.stopping.load(std::memory_order_acquire))
        return Post::coalesced;
    c.stop_deadline = deadline.nanoseconds;
    c.stopping.store(true, std::memory_order_release);
    const U64 one = 1;
    const auto n = ::write(c.event_fd, &one, sizeof(one));
    return n == sizeof(one) || errno == EAGAIN ? Post::queued : Post::wake_uncertain;
}
Result<Done> Foundation::finish() noexcept {
    auto &c = *state_->core;
    if (std::this_thread::get_id() != c.creator)
        return Error::wrong_thread;
    std::lock_guard lock(c.mutex);
    if (c.quarantine)
        return Error::wipe_failed;
    if (!c.drained || c.handle_count.load(std::memory_order_acquire) ||
        c.delete_count.load(std::memory_order_acquire))
        return Error::incomplete_shutdown;
    // The mutex excludes active posts; zero handles excludes future valid posts.
    // discard uses acquire-published IO-join and final framework-cleanup evidence.
    if (c.sdk_installed) {
        if (!sdk::Runtime::discard(c))
            return Error::incomplete_shutdown;
        c.sdk_installed = false;
    }
    c.finished = true;
    return Done{};
}
Result<Done> register_listeners(Foundation &f, drogon::HttpAppFramework &app,
                                Endpoints e) noexcept {
    auto &c = Factory::core(f);
    if (!app.getLoop()->isInLoopThread())
        return Error::wrong_thread;
    if (c.registered || e.address.back() != 0 || (e.http_port && e.http_port == e.port))
        return Error::invalid_state;
    if (!sdk::Runtime::listener(app, {e.address, e.http_port}, io::Role::http) ||
        !sdk::Runtime::listener(app, {e.address, e.port}, io::Role::native))
        return Error::invalid_config;
    c.registered = true;
    return Done{};
}
Result<Done> install_raw_hooks(Foundation &f, RawHooks hooks) noexcept {
    auto &c = Factory::core(f);
    if (std::this_thread::get_id() != c.creator)
        return Error::wrong_thread;
    if (!hooks.owner || !hooks.wipe || !hooks.revoked)
        return Error::invalid_config;
    if (c.hook_installed || c.registered)
        return Error::invalid_state;
    c.hooks = hooks;
    c.hook_installed = true;
    return Done{};
}
Result<Done> testing::Access::arm_first_wire(Foundation &f) noexcept {
    auto &c = Factory::core(f);
    if (std::this_thread::get_id() != c.creator)
        return Error::wrong_thread;
    return !c.registered && sdk::Runtime::arm_first_wire() ? Result<Done>(Done{})
                                                           : Result<Done>(Error::invalid_state);
}
Result<Done> testing::Access::release_wire(Foundation &f) noexcept {
    auto &c = Factory::core(f);
    if (!c.is_io())
        return Error::wrong_thread;
    return sdk::Runtime::release_wire() ? Result<Done>(Done{}) : Result<Done>(Error::invalid_state);
}
Result<std::unique_ptr<Foundation>> create(Limits l, std::span<const Route> routes,
                                           NativeBinding native,
                                           std::shared_ptr<const native_ws::ElapsedSource> source,
                                           Callbacks cb) noexcept {
    auto projected = project_storage(l);
    if (auto *e = std::get_if<Error>(&projected))
        return *e;
    if (!std::holds_alternative<Storage>(projected))
        return Error::invalid_config;
    for (auto ms : {l.accept_ms, l.idle_ms, l.request_ms, l.output_ms, l.tick_ms})
        if (ms > none / 1000000)
            return Error::arithmetic_overflow;
    const auto plan = std::get<Storage>(projected);
    if (l.peers > static_cast<U64>(INT_MAX) - 8 || plan.mapped > static_cast<U64>(PTRDIFF_MAX) ||
        l.body_bytes > static_cast<U64>(SSIZE_MAX) || l.reply_bytes > static_cast<U64>(SSIZE_MAX))
        return Error::invalid_config;
    if (!source || routes.empty() || routes.size() > 8 || native.target.empty() ||
        native.target.size() > 128 || !valid_target(native.target) || !cb.connected ||
        !cb.request || !cb.ready || !cb.ended || !cb.removed || !cb.stop_drained)
        return Error::invalid_config;
    const auto *registration = sdk::Runtime::native_registration();
    if (!registration || registration->source_identity() != source.get())
        return Error::invalid_config;
    const auto &nl = registration->limits();
    if (nl.connections > l.native_peers || nl.incomplete_upgrades > none - nl.connections ||
        l.native_peers > nl.connections + nl.incomplete_upgrades)
        return Error::invalid_config;
    if (!std::holds_alternative<Tick>(source->sample()))
        return Error::elapsed_unavailable;
    for (auto r : routes)
        if (!token(r.method) || r.method.size() > 16 || !valid_target(r.target) ||
            r.target.size() > 2048 || r.body_ceiling > l.body_bytes)
            return Error::invalid_config;
    try {
        auto c = std::make_unique<Core>(l, plan, std::move(source), cb, native);
        c->mapped = plan.mapped;
        c->native_upgrade_ms = nl.upgrade_ms;
        void *mapping = ::mmap(nullptr, static_cast<std::size_t>(plan.mapped),
                               PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapping == MAP_FAILED)
            return Error::allocation_failure;
        ::new (mapping) std::byte[static_cast<std::size_t>(plan.payload)];
        c->mapping = static_cast<std::byte *>(mapping);
        c->meta_bytes = l.peers * metadata_stride + (l.input_slots + l.output_slots) * 128;
        auto *meta = static_cast<std::byte *>(::operator new(
            static_cast<std::size_t>(c->meta_bytes), std::align_val_t(64), std::nothrow));
        if (!meta)
            return Error::allocation_failure;
        c->metadata.reset(meta);
        for (U64 i = 0; i < l.peers; ++i) {
            auto *p = new (meta + i * metadata_stride) PeerRecord;
            p->core = c.get();
            p->index = i;
            p->metadata = meta + i * metadata_stride;
            p->payload = c->mapping + i * peer_bytes;
            ::new (p->fields()) Field[64];
        }
        for (U64 i = 0; i < l.input_slots + l.output_slots; ++i)
            new (meta + l.peers * metadata_stride + i * 128) PoolSlot;
        c->route_count = routes.size();
        for (std::size_t i = 0; i < routes.size(); ++i) {
            auto r = routes[i];
            std::memcpy(c->methods[i].data(), r.method.data(), r.method.size());
            std::memcpy(c->targets[i].data(), r.target.data(), r.target.size());
            c->routes[i] = {{c->methods[i].data(), r.method.size()},
                            {c->targets[i].data(), r.target.size()},
                            r.body_ceiling,
                            r.header_semantic_ceiling,
                            r.total_semantic_ceiling};
        }
        std::memcpy(c->native_target.data(), native.target.data(), native.target.size());
        c->native.target = {c->native_target.data(), native.target.size()};
        c->event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        c->timer_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if (c->event_fd < 0 || c->timer_fd < 0)
            return Error::allocation_failure;
        itimerspec scan{};
        scan.it_interval.tv_sec = static_cast<time_t>(l.tick_ms / 1000);
        scan.it_interval.tv_nsec = static_cast<long>((l.tick_ms % 1000) * 1000000);
        scan.it_value = scan.it_interval;
        if (::timerfd_settime(c->timer_fd, 0, &scan, nullptr))
            return Error::allocation_failure;
        if (!sdk::Runtime::install(*c, *c, l.peers)) {
            switch (sdk::Runtime::install_failure()) {
            case sdk::InstallFailure::allocation_failure:
                return Error::allocation_failure;
            case sdk::InstallFailure::invalid_config:
                return Error::invalid_config;
            default:
                return Error::invalid_state;
            }
        }
        c->sdk_installed = true;
        return Factory::make(std::move(c));
    } catch (...) {
        return Error::allocation_failure;
    }
}
} // namespace subathon::server::bounded_http
