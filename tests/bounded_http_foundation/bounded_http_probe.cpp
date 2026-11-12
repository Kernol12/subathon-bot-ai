// SPDX-License-Identifier: GPL-3.0-or-later
// Test consumer only. Fixture allocations/queues are finite and NOT foundation evidence.
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <drogon/WebSocketConnection.h>
#include <drogon/drogon.h>
#include <fcntl.h>
#include <fstream>
#include <json/json.h>
#include <link.h>
#include <mutex>
#include <openssl/sha.h>
#include <optional>
#include <poll.h>
#include <stdexcept>
#include <subathon/server/bounded_http.hpp>
#include <subathon/server/testing/bounded_http_observers.hpp>
#include <sys/resource.h>
#include <sys/socket.h>
#include <thread>
#include <trantor/net/EventLoop.h>
#include <unistd.h>

namespace bh = subathon::server::bounded_http;
namespace ws = subathon::server::native_ws;
using namespace std::chrono_literals;
namespace {
constexpr unsigned maxEvents = 2048, maxLine = 65536, maxLog = 2097152;
[[noreturn]] void fixture(const char *why) {
    ::dprintf(2, "FIXTURE:%s\n", why);
    std::_Exit(90);
}
class Output {
    struct Record {
        unsigned offset = 0, size = 0;
    };
    std::array<char, maxLog> arena_{};
    std::array<Record, maxEvents> records_{};
    unsigned bytes_ = 0, count_ = 0, read_ = 0, written_ = 0;
    std::mutex mutex_;
    std::condition_variable changed_;
    bool done_ = false;
    std::thread writer_;

  public:
    Output() : writer_([this] { run(); }) {}
    ~Output() {
        {
            std::lock_guard lock(mutex_);
            done_ = true;
        }
        changed_.notify_one();
        writer_.join();
    }
    void emit(Json::Value v) {
        Json::StreamWriterBuilder b;
        b["indentation"] = "";
        auto line = Json::writeString(b, v) + '\n';
        std::lock_guard lock(mutex_);
        if (count_ == maxEvents || line.size() > maxLine || line.size() > maxLog - bytes_)
            fixture("event bounds");
        records_[count_++] = {bytes_, static_cast<unsigned>(line.size())};
        std::memcpy(arena_.data() + bytes_, line.data(), line.size());
        bytes_ += line.size();
        changed_.notify_one();
    }
    void flush() {
        std::unique_lock lock(mutex_);
        if (!changed_.wait_for(lock, 3s, [&] { return written_ == count_; }))
            fixture("fatal event flush bound");
    }
    void run() {
        for (unsigned record = 0; record < maxEvents; ++record) {
            Record r;
            {
                std::unique_lock lock(mutex_);
                changed_.wait(lock, [&] { return done_ || read_ < count_; });
                if (read_ == count_)
                    return;
                r = records_[read_++];
            }
            unsigned at = 0;
            for (unsigned turn = 0; turn < 256 && at < r.size; ++turn) {
                auto n = ::write(1, arena_.data() + r.offset + at, r.size - at);
                if (n > 0)
                    at += static_cast<unsigned>(n);
                else if (n < 0 && errno == EINTR)
                    continue;
                else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    pollfd fd{1, POLLOUT, 0};
                    ::poll(&fd, 1, 10);
                } else
                    fixture("event pipe");
            }
            if (at != r.size)
                fixture("event writer bound");
            {
                std::lock_guard lock(mutex_);
                ++written_;
            }
            changed_.notify_all();
        }
    }
};
static_assert(sizeof(Output) <= 2113792); // arena + records + <=256 synchronization bytes
Output *output = nullptr;
void emit(Json::Value v) { output->emit(std::move(v)); }
Json::Value event(const char *name) {
    Json::Value v;
    v["event"] = name;
    return v;
}
void must(bool ok, const char *oracle) {
    if (ok)
        return;
    auto v = event("assertion");
    v["oracle"] = oracle;
    emit(v);
    output->flush();
    std::_Exit(42);
}
template <class T> T required(bh::Result<T> r, const char *boundary) {
    if (auto p = std::get_if<T>(&r))
        return std::move(*p);
    auto v = event(std::holds_alternative<bh::NI>(r) ? "missing_producer" : "assertion");
    v["boundary"] = boundary;
    if (auto e = std::get_if<bh::Error>(&r))
        v["error"] = static_cast<unsigned>(*e);
    emit(v);
    output->flush();
    std::_Exit(std::holds_alternative<bh::NI>(r) ? 40 : 42);
}
template <class T> void error(bh::Result<T> r, bh::Error expected, const char *oracle) {
    if (std::holds_alternative<bh::NI>(r)) {
        required(std::move(r), oracle);
        return;
    }
    must(std::get_if<bh::Error>(&r) && std::get<bh::Error>(r) == expected, oracle);
}
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> d{};
    SHA256(reinterpret_cast<const unsigned char *>(bytes.data()), bytes.size(), d.data());
    std::array<char, 65> out{};
    for (unsigned i = 0; i < 32; ++i)
        std::snprintf(out.data() + i * 2, 3, "%02x", d[i]);
    return out.data();
}
void maps(const char *phase) {
    // Complete Linux file-backed map inventory, including repeated mappings, bounded separately.
    std::ifstream f("/proc/self/maps");
    std::string line;
    unsigned n = 0;
    while (std::getline(f, line)) {
        if (++n > 256 || line.size() > 4096)
            fixture("maps bounds");
        auto v = event("map");
        v["phase"] = phase;
        v["line"] = line;
        emit(v);
    }
    if (!f.eof())
        fixture("maps read");
    auto v = event("maps_end");
    v["phase"] = phase;
    v["count"] = n;
    emit(v);
}
struct Clock final : ws::ElapsedSource {
    std::atomic<bh::U64> ns{1000000000};
    std::atomic<bool> available{true};
    std::variant<ws::ElapsedInstant, ws::ElapsedUnavailable> sample() const noexcept override {
        if (!available.load())
            return ws::ElapsedUnavailable{};
        return ws::ElapsedInstant{ns.load()};
    }
};
bh::Limits limits(std::string_view name) {
    if (name == "reference" || name == "method" || name == "budget")
        return {128,     80,   48,   8,    8,    name == "budget" ? 8192ULL : 1048576ULL,
                1048576, 5000, 5000, 5000, 5000, 10};
    if (name == "p2")
        return {2, 1, 1, 1, 1, 4, 4, 50, 50, 50, 50, 1};
    return {4, 2, 2, 2, name == "pressure" ? 1ULL : 2ULL, 4, 4, 50, 50, 50, 50, 1};
}
ws::Limits nativeLimits(bool ref) {
    if (ref)
        return {32,      16,      1048576, 1048576, 4096, 2,     2097152, 16777216, 1048590,
                1048576, 1048590, 254,     64,      5000, 10000, 30000,   1000};
    return {1, 1, 64, 64, 16, 2, 128, 256, 256, 64, 256, 254, 8, 50, 50, 100, 50};
}
struct Peer {
    std::optional<bh::Peer> peer;
    std::optional<bh::Wake> wake;
    std::optional<bh::Request> request;
    std::optional<bh::Reply> reply;
    ws::Connection strong;
    std::weak_ptr<drogon::WebSocketConnection> weak;
    std::optional<ws::MessageLease> lease;
    std::array<bh::ReadSample, 2> latches{}; // exactly two caller-precharged 64-byte slots
    std::array<bool, 2> latched{};
    unsigned index = 0, requests = 0, ends = 0, removes = 0, ready = 0;
    bh::U64 cumulative = 0, ordinal = 0;
};
static_assert(sizeof(Peer) <= 512); // finite caller record, separate from producer State slots
static_assert(sizeof(bh::ReadSample) <= 64);
static_assert(!std::is_default_constructible_v<bh::Peer> &&
              !std::is_copy_constructible_v<bh::Peer>);
static_assert(!std::is_default_constructible_v<bh::Wake> &&
              !std::is_copy_constructible_v<bh::Wake>);
static_assert(!std::is_default_constructible_v<bh::Request> &&
              !std::is_copy_constructible_v<bh::Request>);
static_assert(!std::is_default_constructible_v<bh::Reply> &&
              !std::is_copy_constructible_v<bh::Reply>);
struct Rig {
    std::array<Peer, 128> peers{};
    std::shared_ptr<Clock> clock = std::make_shared<Clock>();
    std::unique_ptr<ws::Profile> profile;
    std::unique_ptr<bh::Foundation> foundation;
    unsigned connected = 0, requests = 0, legacy = 0;
    std::atomic<bool> drained{false}, finished{false}, fullReceiveRelease{false};
    std::string action, profileName;
    std::atomic<trantor::EventLoop *> io{nullptr};
    bool nullContext = false, wipeFails = false;
    unsigned wipes = 0, revocations = 0;
    static Rig &self(void *p) { return *static_cast<Rig *>(p); }
    Peer &context(void *c) {
        // Actual callback context only: never resolve a client/Connection/public token as
        // authority.
        must(!nullContext && c, "context present");
        auto *p = static_cast<Peer *>(c);
        must(p >= peers.data() && p < peers.data() + connected, "context actual owner table");
        return *p;
    }
    void peerEvent(const char *name, Peer &p) {
        auto v = event(name);
        v["peer"] = p.index;
        emit(v);
    }
    static void onConnected(void *o, bh::Peer handle) {
        auto &r = self(o);
        must(r.connected < 128, "fixture peer table");
        auto &p = r.peers[r.connected];
        p.index = r.connected++;
        p.peer.emplace(std::move(handle));
        if (!r.nullContext)
            required(p.peer->context(&p), "context");
        if (r.action != "no-wake")
            p.wake.emplace(required(p.peer->wake(), "wake"));
        if (!r.nullContext)
            error(p.peer->context(&p), bh::Error::invalid_state, "context once");
        if (r.action != "no-wake")
            error(p.peer->wake(), bh::Error::invalid_state, "wake once");
        if (r.action == "full-receive-block") {
            r.peerEvent("connected_held", p);
            for (unsigned i = 0; i < 5000 && !r.fullReceiveRelease.load(); ++i)
                std::this_thread::sleep_for(1ms);
            if (!r.fullReceiveRelease.load())
                fixture("connected fixture gate deadline");
        }
        required(r.foundation->admission(*p.peer, bh::Pause::admitted), "initial admission");
        r.peerEvent("connected", p);
    }
    static void onRequest(void *o, void *c, bh::Request h) {
        auto &r = self(o);
        // null context has its own single-peer positive; all other correlations use actual context.
        auto &p = r.nullContext ? r.peers[0] : r.context(c);
        must(!r.nullContext || !c, "null context unchanged");
        must(!p.request, "no second current request");
        p.request.emplace(std::move(h));
        ++p.requests;
        ++r.requests;
        required(
            p.request->visit(
                &p,
                [](void *q, bh::RequestView v) {
                    auto &p = *static_cast<Peer *>(q);
                    auto e = event("request");
                    e["peer"] = p.index;
                    e["ordinal"] = p.requests;
                    e["route"] = v.route_index;
                    e["method"] = std::string(v.method);
                    e["target"] = std::string(v.escaped_target);
                    e["body_size"] = Json::UInt64(v.body.size());
                    e["body_sha256"] = digest(v.body);
                    Json::Value fields(Json::arrayValue);
                    must(v.fields.size() <= 64 && v.arena.size() <= 32768, "view envelope");
                    for (auto f : v.fields) {
                        must(f.name <= v.arena.size() && f.name_size <= v.arena.size() - f.name &&
                                 f.value <= v.arena.size() &&
                                 f.value_size <= v.arena.size() - f.value,
                             "view offsets");
                        Json::Value a(Json::arrayValue);
                        a.append(std::string(
                            reinterpret_cast<const char *>(v.arena.data() + f.name), f.name_size));
                        a.append(
                            std::string(reinterpret_cast<const char *>(v.arena.data() + f.value),
                                        f.value_size));
                        fields.append(a);
                    }
                    e["fields"] = fields;
                    emit(e);
                }),
            "visit");
        if (r.action == "throw-request")
            throw std::runtime_error("synthetic callback fault");
        if (r.action == "pressure") {
            if (r.requests == 2) {
                auto &first = r.peers[0];
                r.reply(first);
                auto x = required(r.foundation->inspect(), "pressure held inspect");
                must(x.current.output_slots == 1 && x.current.output_capacity == 132 &&
                         x.held_application_wire == 1,
                     "actual held W/node");
                emit(event("pressure_held"));
                error(p.request->reserve_reply({200, 4, false}), bh::Error::resource_limit,
                      "second output before fill");
                emit(event("pressure_refused"));
                if (!std::holds_alternative<bh::Done>(
                        bh::testing::Access::release_wire(*r.foundation)))
                    fixture("gate release unavailable");
            }
            return;
        }
        if (r.action == "hold" || r.action == "deadline" || r.action == "full-receive-block")
            return;
        if (r.action == "drop-request") {
            p.request.reset();
            return;
        }
        if (r.action == "wrong-thread-raw") {
            std::thread t([&] { p.request->release_raw(); });
            t.join();
            fixture("void raw returned");
        }
        if (r.action == "wrong-thread-result") {
            std::thread t([&] {
                error(p.request->visit(nullptr, [](void *, bh::RequestView) {}),
                      bh::Error::wrong_thread, "foreign visit");
            });
            t.join();
        }
        if (r.action == "reentry")
            required(p.request->visit(&p,
                                      [](void *q, bh::RequestView) {
                                          auto &p = *static_cast<Peer *>(q);
                                          error(p.request->visit(nullptr,
                                                                 [](void *, bh::RequestView) {}),
                                                bh::Error::reentrant, "nested visit");
                                      }),
                     "outer visit");
        if (r.action == "moves") {
            auto before = required(r.foundation->inspect(), "move before");
            std::thread t([&] {
                auto tmp = std::move(*p.request);
                p.request.reset();
                p.request.emplace(std::move(tmp));
            });
            t.join();
            auto after = required(r.foundation->inspect(), "move after");
            must(before.current.handles == after.current.handles &&
                     before.current.input_capacity == after.current.input_capacity,
                 "exclusive foreign moves add zero");
        }
        r.reply(p);
    }
    void reply(Peer &p) {
        const bh::U64 length = action == "zero-reply" ? 0 : action == "large-output" ? 1048576 : 4;
        p.reply.emplace(required(p.request->reserve_reply({200, length, action == "reply-close"}),
                                 "reserve reply"));
        if (action == "once") {
            error(p.request->reserve_reply({200, 4, false}), bh::Error::already_replied,
                  "reserve once");
            error(p.reply->commit(), bh::Error::invalid_state, "fill before commit");
        }
        if (action == "drop-reply") {
            p.reply.reset();
            return;
        }
        if (action == "cancel-before") {
            p.reply->cancel();
            return;
        }
        if (action == "wrong-thread-cancel") {
            std::thread t([&] { p.reply->cancel(); });
            t.join();
            fixture("void cancel returned");
        }
        auto fill = p.reply->fill(this, [](void *q, std::span<std::byte> b) -> bh::U64 {
            if (self(q).action == "large-output") {
                must(b.size() == 1048576, "large direct subspan");
                std::fill(b.begin(), b.end(), std::byte{'z'});
                return 1048576;
            }
            const auto zero = self(q).action == "zero-reply";
            must(b.size() == (zero ? 0 : 4), "direct declared body subspan");
            if (zero)
                return 0;
            std::memcpy(b.data(), "diag", 4);
            return self(q).action == "bad-fill" ? 3 : 4;
        });
        if (action == "bad-fill") {
            error(std::move(fill), bh::Error::invalid_size, "fill size");
            return;
        }
        required(std::move(fill), "fill");
        if (action == "once")
            error(p.reply->fill(nullptr,
                                [](void *, std::span<std::byte>) -> bh::U64 {
                                    fixture("second fill called");
                                }),
                  bh::Error::invalid_state, "fill once");
        if (action == "commit-equality")
            clock->ns = 6100000000ULL;
        if (action == "commit-equality") {
            error(p.reply->commit(), bh::Error::expired, "commit equality");
            return;
        }
        required(p.reply->commit(), "commit");
        if (action == "once")
            error(p.reply->commit(), bh::Error::invalid_state, "commit once");
        p.request->release_raw();
        if (action == "revoked")
            error(p.request->visit(nullptr,
                                   [](void *, bh::RequestView) { fixture("revoked visitor"); }),
                  bh::Error::revoked, "raw revoked");
        if (action == "cancel-committed")
            p.reply->cancel();
        if (action != "retain-inert" && action != "gate-cancel") {
            p.reply.reset();
            p.request.reset();
        }
    }
    static void onReady(void *o, void *c) {
        auto &r = self(o);
        auto &p = r.context(c);
        r.peerEvent("ready", p);
        if (r.action == "ready-repost" && ++p.ready == 1) {
            std::thread t([&] {
                required(r.foundation->post(*p.wake, bh::Intent::owner_ready),
                         "post during actual ready");
            });
            t.join();
        }
    }
    static void onEnd(void *o, void *c, bh::End end) {
        auto &r = self(o);
        auto &p = r.nullContext ? r.peers[0] : r.context(c);
        must(++p.ends == p.requests, "one ended per request");
        auto v = event("ended");
        v["peer"] = p.index;
        v["reason"] = static_cast<unsigned>(end);
        emit(v);
        if (r.action == "gate-stop") {
            auto x = required(r.foundation->inspect(), "gate stop real release");
            must(end == bh::End::stopped && x.wire_releases == 1 && !x.current.output_slots,
                 "stop overrides held node once");
        }
        if (r.action == "pressure" && p.index == 0) {
            auto x = required(r.foundation->inspect(), "pressure real release inspect");
            if (end != bh::End::drained)
                fixture("pressure deadline/terminal during local sequence");
            must(!x.current.output_slots && !x.current.output_capacity && x.wire_releases == 1,
                 "actual node returned exactly once");
            emit(event("pressure_released"));
            r.reply(r.peers[1]);
        }
    }
    static void onRemoved(void *o, void *c) {
        auto &r = self(o);
        auto &p = r.nullContext ? r.peers[0] : r.context(c);
        must(++p.removes == 1 && p.ends == p.requests,
             "ended before removed, native no invented end");
        r.peerEvent("removed", p);
        if (r.action != "retain-inert") {
            p.request.reset();
            p.reply.reset();
            p.peer.reset();
            p.wake.reset();
        }
    }
    static void onDrained(void *o) {
        auto &r = self(o);
        must(!r.drained.exchange(true), "stop_drained once");
        auto x = required(r.foundation->inspect(), "drain inspect");
        for (unsigned i = 0; i < r.connected; ++i)
            must(r.peers[i].removes == 1, "all actual removed before drain");
        must(!x.current.native_retiring && !x.current.sdk_retained_peers &&
                 !x.current.output_slots && !x.current.transient_fds && !x.current.pending_wakes,
             "actual network drain");
        emit(event("stop_drained"));
        drogon::app().quit();
    }
    static void onRead(void *o, void *c, bh::ReadSample s) {
        auto &r = self(o);
        auto &p = r.nullContext ? r.peers[0] : r.context(c);
        must(s.ordinal == p.ordinal + 1 &&
                 s.bytes_read == p.cumulative + (s.returned > 0 ? s.returned : 0),
             "signed per-peer read ordinal/cumulative");
        auto prior = p.cumulative;
        p.ordinal = s.ordinal;
        p.cumulative = s.bytes_read;
        if (s.returned > 0) {
            must(s.writable >= 8192 && s.writable <= 65536 && s.offered == s.writable &&
                     s.iovcnt == 1,
                 "actual one-iovec pre-read guard");
            unsigned slot = prior == 0 ? 0 : prior == 32 ? 1 : 2;
            if (slot < 2 && !p.latched[slot]) {
                p.latched[slot] = true;
                p.latches[slot] = s;
                auto v = event("latch");
                v["peer"] = p.index;
                v["slot"] = slot;
                v["head"] = Json::UInt64(s.head);
                v["tail"] = Json::UInt64(s.tail);
                v["writable"] = Json::UInt64(s.writable);
                v["offered"] = Json::UInt64(s.offered);
                v["iovcnt"] = s.iovcnt;
                v["returned"] = Json::Int64(s.returned);
                emit(v);
            }
        }
        if (r.action == "throw-read")
            throw std::runtime_error("synthetic read observer");
    }
    static void opened(void *o, void *c, const ws::Connection &conn) {
        auto &r = self(o);
        auto &p = r.context(c);
        if (r.action == "strong" || r.action == "facade-negative")
            p.strong = conn;
        if (r.action == "weak")
            p.weak = conn;
        r.peerEvent("opened", p);
    }
    static void message(void *o, void *c, const ws::Connection &conn) {
        auto &r = self(o);
        auto &p = r.context(c);
        auto lease = r.profile->accept_callback(conn);
        must(std::holds_alternative<ws::MessageLease>(lease.value), "actual native lease");
        auto h = std::move(std::get<ws::MessageLease>(lease.value));
        auto view = h.view();
        must(view && view->bytes.size() == 1 && view->bytes[0] == std::byte{0x41},
             "native literal binary41");
        if (r.action == "lease")
            p.lease.emplace(std::move(h));
        r.peerEvent("message", p);
    }
    static void closed(void *o, void *c, const ws::Connection &) {
        auto &r = self(o);
        r.peerEvent("closed", r.context(c));
    }
    static bool wipe(void *o, bh::RawTag tag, std::span<std::byte> bytes) noexcept {
        auto &r = self(o);
        if (tag.use == bh::RawUse::pending && tag.sequence != 0)
            fixture("pending attribution");
        ++r.wipes;
        std::fill(bytes.begin(), bytes.end(), std::byte{});
        return !r.wipeFails;
    }
    static void revoked(void *o, void *, bh::U64 sequence, bh::End) noexcept {
        auto &r = self(o);
        must(sequence != 0, "revoked actual current sequence");
        ++r.revocations;
    }
    void inspect() {
        auto s = required(foundation->inspect(), "inspect");
        auto v = event("inspection");
#define C(n) v[#n] = Json::UInt64(s.current.n)
        C(peers);
        C(http_peers);
        C(native_peers);
        C(retiring_peers);
        C(handles);
        C(input_slots);
        C(output_slots);
        C(input_capacity);
        C(output_capacity);
        C(control_capacity);
        C(metadata_charge);
        C(pending_wakes);
        C(transient_fds);
        C(constructed);
        C(native_retiring);
        C(sdk_retained_peers);
#undef C
#define O(n) v[#n] = Json::UInt64(s.n)
        O(reserved_storage);
        O(mapped_storage);
        O(reads);
        O(one_iovec_reads);
        O(bytes_read);
        O(requests);
        O(stock_maps);
        O(stock_batches);
        O(stock_output_appends);
        O(accept_attempts);
        O(raw_releases);
        O(wire_releases);
        O(native_retires);
        O(generation_exhaustions);
        O(native_handoff_capacity);
        O(native_handoff_accepted);
        O(native_projected_fields);
        O(native_projected_cookies);
        O(held_application_wire);
#undef O
        v["peak_transient_fds"] = Json::UInt64(s.peak.transient_fds);
        v["fixture_wipes"] = wipes;
        v["fixture_revocations"] = revocations;
        emit(v);
    }
    void command(const Json::Value &v) {
        const auto op = v["op"].asString();
        const auto i = v.get("peer", 0).asUInt();
        must(i < 128, "command peer table");
        auto &p = peers[i];
        if (op == "inspect")
            inspect();
        else if (op == "admit")
            required(foundation->admission(*p.peer, bh::Pause::admitted), "admit");
        else if (op == "suspend")
            required(foundation->admission(*p.peer, bh::Pause::suspended), "suspend");
        else if (op == "reply")
            reply(p);
        else if (op == "reserve-held") {
            p.reply.emplace(required(p.request->reserve_reply({200, 4, false}), "held reserve"));
            required(p.reply->fill(nullptr,
                                   [](void *, std::span<std::byte> b) -> bh::U64 {
                                       std::memcpy(b.data(), "diag", 4);
                                       return 4;
                                   }),
                     "held fill");
        } else if (op == "commit-held")
            error(p.reply->commit(), bh::Error::expired, "held commit exact deadline");
        else if (op == "post-pair") {
            bh::Post a{}, b{};
            std::thread t([&] {
                a = required(foundation->post(*p.wake, bh::Intent::owner_ready),
                             "first recorded post");
                b = required(foundation->post(*p.wake, bh::Intent::owner_ready),
                             "second recorded post");
            });
            t.join();
            must((a == bh::Post::queued || a == bh::Post::wake_uncertain) &&
                     (b == bh::Post::coalesced || b == bh::Post::wake_uncertain),
                 "coalesced is recording not delivery");
        } else if (op == "post-close") {
            std::thread t([&] {
                required(foundation->post(*p.wake, bh::Intent::suspend), "post suspend");
                required(foundation->post(*p.wake, bh::Intent::admit), "post admit last");
                required(foundation->post(*p.wake, bh::Intent::close), "post close absorbs");
            });
            t.join();
        } else if (op == "pressure-cancel") {
            p.reply->cancel();
        } else if (op == "release") {
            p.request->release_raw();
            p.request.reset();
            p.reply.reset();
        } else if (op == "close")
            required(foundation->close(*p.peer), "close");
        else if (op == "drop") {
            p.request.reset();
            p.reply.reset();
            p.peer.reset();
            p.wake.reset();
        } else if (op == "gate-release") {
            if (!std::holds_alternative<bh::Done>(bh::testing::Access::release_wire(*foundation)))
                fixture("gate release unavailable");
        } else if (op == "post") {
            bh::Post result{};
            std::thread t([&] {
                result = required(foundation->post(*p.wake, bh::Intent::owner_ready),
                                  "ordinary-thread post");
            });
            t.join();
            auto e = event("post");
            e["result"] = static_cast<unsigned>(result);
            emit(e);
        } else if (op == "clock") {
            clock->ns = v["ns"].asUInt64();
            clock->available = v.get("available", true).asBool();
        } else if (op == "stop")
            required(foundation->stop({v["ns"].asUInt64()}), "stop");
        else if (op == "post-stopping")
            error(foundation->post(*p.wake, bh::Intent::owner_ready), bh::Error::stopping,
                  "post after stop refuses before work");
        else if (op == "foreign-release") {
            // Actual final owner/weak reset on a non-IO thread; no fabricated fence observation.
            std::thread t([&] {
                p.strong.reset();
                p.weak.reset();
                p.lease.reset();
            });
            t.join();
        } else if (op == "facade-negative") {
            must(bool(p.strong), "real retained facade");
            auto before = required(foundation->inspect(), "facade before");
            std::thread t([&] {
                p.strong->send("synthetic");
                p.strong->shutdown();
                p.strong->forceClose();
            });
            t.join();
            auto after = required(foundation->inspect(), "facade after");
            must(before.stock_output_appends == after.stock_output_appends &&
                     before.current.metadata_charge == after.current.metadata_charge,
                 "offIO facade no enqueue/allocation source gate also required");
        } else
            fixture("unknown command");
        auto e = event("ack");
        e["op"] = op;
        emit(e);
    }
};
static_assert(sizeof(Rig) <= 66560); // 128 caller records x512 +1024 global fixture bytes
// SDK retains native registration globally. Keep the fixture and its callback targets alive until
// process teardown, even if a child fails; do not pretend Profile destruction unregisters it.
Rig *retained = nullptr; // One intentionally process-retained registration owner; see receipt.
void diagnostics(const char *, const std::uint64_t) { fixture("unexpected SDK diagnostic"); }
void setup(drogon::HttpAppFramework &app) {
    trantor::Logger::setOutputFunction(diagnostics, [] {});
    app.disableSigtermHandling()
        .setThreadNum(1)
        .setIdleConnectionTimeout(0)
        .setLogLevel(trantor::Logger::kFatal)
        .setFileTypes({})
        .setHomePage("")
        .setImplicitPageEnable(false)
        .enableGzip(false)
        .enableBrotli(false)
        .enableDateHeader(false)
        .enableServerHeader(false)
        .disableSession()
        .enableRequestStream(false)
        .enableCompressedRequest(false)
        .setDocumentRoot("empty")
        .setUploadPath("upload")
        .setClientMaxBodySize(1048576)
        .setClientMaxMemoryBodySize(1048576);
    app.enableReusePort(true); // Public probe bypass: no /tmp/drogon.lock.
    app.setBeforeListenSockOptCallback([](int fd) {
        int zero = 0, reuse = 0, address = 0;
        socklen_t n = sizeof(int);
        if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &zero, sizeof(zero)) ||
            ::getsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse, &n) || reuse ||
            ::getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &address, &n) || address != 1)
            fixture("socket options");
    });
}
Json::Value parse(std::string_view text) {
    Json::CharReaderBuilder b;
    std::unique_ptr<Json::CharReader> reader(b.newCharReader());
    Json::Value v;
    std::string err;
    if (text.size() > 512 || !reader->parse(text.data(), text.data() + text.size(), &v, &err) ||
        !v.isObject())
        fixture("command parse");
    return v;
}
void arithmetic() {
    constexpr std::array<std::array<bh::U64, 6>, 4> expected{
        {{29888512, 3213312, 33101824, 29888512, 0, 33101824},
         {409968, 164352, 574320, 413696, 3728, 578048},
         {204984, 114944, 319928, 208896, 3912, 323840},
         {409836, 164224, 574060, 413696, 3860, 577920}}};
    constexpr std::array<const char *, 4> names{"reference", "tiny", "p2", "pressure"};
    for (unsigned i = 0; i < 4; ++i) {
        auto s = required(bh::project_storage(limits(names[i])), "project_storage");
        const auto &e = expected[i];
        must(s.payload == e[0] && s.external == e[1] && s.reserved == e[2] && s.mapped == e[3] &&
                 s.tail == e[4] && s.total == e[5],
             "literal storage plan");
    }
    auto l = limits("p2");
    l.peers = 145249953336295ULL;
    l.http_peers = l.peers - 1;
    auto s = required(bh::project_storage(l), "large literal plan");
    must(s.payload == 14877081220516679216ULL && s.external == 3569662853192851712ULL &&
             s.reserved == 18446744073709530928ULL && s.mapped == 14877081220516679680ULL &&
             s.tail == 464 && s.total == 18446744073709531392ULL,
         "L+X exact fits");
    ++l.peers;
    ++l.http_peers;
    error(bh::project_storage(l), bh::Error::arithmetic_overflow, "L+X overflow");
    l = limits("tiny");
    l.reply_bytes = UINT64_MAX;
    error(bh::project_storage(l), bh::Error::arithmetic_overflow, "W overflow");
    l = limits("tiny");
    l.peers = 0;
    error(bh::project_storage(l), bh::Error::invalid_config, "zero peers");
    l = limits("tiny");
    l.input_slots = 5;
    error(bh::project_storage(l), bh::Error::invalid_config, "I greater P");
    required(bh::check_read_window(8192), "8192 guard");
    required(bh::check_read_window(65536), "65536 guard");
    error(bh::check_read_window(8191), bh::Error::resource_limit, "8191 guard");
    error(bh::check_read_window(65537), bh::Error::resource_limit, "65537 guard");
    emit(event("arithmetic_pass"));
}
} // namespace
int main(int argc, char **argv) {
    rlimit noFiles{512, 512};
    if (::setrlimit(RLIMIT_NOFILE, &noFiles))
        fixture("FD limit");
    rlimit noCore{0, 0};
    if (::setrlimit(RLIMIT_CORE, &noCore))
        fixture("core limit");
    if (argc != 2 || std::strlen(argv[1]) > 512)
        fixture("arguments");
    int flags = ::fcntl(1, F_GETFL);
    if (flags < 0 || ::fcntl(1, F_SETFL, flags | O_NONBLOCK) < 0)
        fixture("pipe flags");
    Output writer;
    output = &writer;
    const auto config = parse(argv[1]);
    // Child-wide watchdog is fixture failure, never intended product red.
    std::atomic<bool> done = false;
    std::thread watchdog([&] {
        for (unsigned i = 0; i < 300 && !done; ++i)
            std::this_thread::sleep_for(100ms);
        if (!done)
            fixture("30s child watchdog");
    });
    struct Join {
        std::atomic<bool> &done;
        std::thread &t;
        ~Join() {
            done = true;
            t.join();
        }
    } join{done, watchdog};
    maps("initial");
    if (config["mode"] == "identity") {
        auto e = event("identity");
        e["cplusplus"] = Json::UInt64(__cplusplus);
        e["read_sample_size"] = Json::UInt64(sizeof(bh::ReadSample));
        e["native_backend"] = ws::linked_backend() == ws::Backend::admitted_corrected
                                  ? "admitted_corrected"
                                  : "stock";
        emit(e);
        maps("cleanup");
        return 0;
    }
    if (config["mode"] == "arithmetic") {
        arithmetic();
        maps("cleanup");
        return 0;
    }
    retained = new Rig;
    auto &r = *retained;
    r.action = config.get("action", "auto").asString();
    r.profileName = config.get("profile", "reference").asString();
    r.nullContext = r.action == "null-context";
    r.wipeFails = r.action == "wipe-failure";
    if (ws::linked_backend() != ws::Backend::admitted_corrected)
        fixture("requires independently admitted candidate4/successor");
    auto n =
        ws::create_profile(nativeLimits(r.profileName == "reference" || r.profileName == "method" ||
                                        r.profileName == "budget"),
                           r.clock);
    if (!std::holds_alternative<std::unique_ptr<ws::Profile>>(n))
        fixture("legacy profile unavailable");
    r.profile = std::move(std::get<std::unique_ptr<ws::Profile>>(n));
    bool control = config["mode"] == "legacy";
    std::string longTarget = "/" + std::string(2032, 'a');
    std::array<bh::Route, 4> routes{{{"POST", "/f", limits(r.profileName).body_bytes, 0, 0},
                                     {r.profileName == "method" ? "ABCDEFGHIJKLMNOP" : "GET", "/f",
                                      limits(r.profileName).body_bytes, 0, 0},
                                     {"GET", longTarget, limits(r.profileName).body_bytes, 0, 0},
                                     {"POST", "/p", 4096, 4096, 4096}}};
    if (!control) {
        auto selected = limits(r.profileName);
        auto source = r.clock;
        if (config["mode"] == "config") {
            auto name = config["case"].asString();
            bh::Error expected = bh::Error::invalid_config;
            if (name == "zero-duration")
                selected.accept_ms = 0;
            else if (name == "roles")
                selected.http_peers = 79;
            else if (name == "output-count")
                selected.output_slots = 129;
            else if (name == "duration-over") {
                selected.accept_ms = 18446744073710ULL;
                expected = bh::Error::arithmetic_overflow;
            } else if (name == "epoll-narrow") {
                selected = {
                    145249953336295ULL, 145249953336294ULL, 1, 1, 1, 4, 4, 50, 50, 50, 50, 1};
            } else if (name == "missing-source")
                source.reset();
            else if (name == "duration-bound")
                selected.accept_ms = 18446744073709ULL;
            else
                fixture("config selector");
            for (unsigned i = 0; i < 3; ++i)
                routes[i].body_ceiling = selected.body_bytes;
            auto result =
                bh::create(selected, std::span(routes.data(), 3),
                           {"/foundation/ws", Rig::opened, Rig::message, Rig::closed}, source,
                           {&r, Rig::onConnected, Rig::onRequest, Rig::onReady, Rig::onEnd,
                            Rig::onRemoved, Rig::onDrained, Rig::onRead});
            if (name == "duration-bound")
                r.foundation = required(std::move(result), "foundation.create.duration_bound");
            else if (name == "missing-source") {
                if (std::holds_alternative<bh::NI>(result))
                    required(std::move(result), "foundation.create.config_before_listeners_run");
                must(std::holds_alternative<bh::Error>(result),
                     "null source refuses without constructor");
            } else
                error(std::move(result), expected, "foundation.create.config_before_listeners_run");
            emit(event("config_pass"));
            maps("cleanup");
            return 0;
        }
        r.foundation = required(
            bh::create(selected, std::span(routes.data(), r.profileName == "budget" ? 4 : 3),
                       {"/foundation/ws", Rig::opened, Rig::message, Rig::closed}, r.clock,
                       {&r, Rig::onConnected, Rig::onRequest, Rig::onReady, Rig::onEnd,
                        Rig::onRemoved, Rig::onDrained,
                        r.action == "null-read" ? nullptr : Rig::onRead}),
            "foundation.create.before_listeners_run");
        required(bh::install_raw_hooks(*r.foundation, {&r, Rig::wipe, Rig::revoked}), "raw hooks");
        if (r.action == "owner-nonregression")
            r.clock->ns = 999999999;
        if (r.action == "pressure" || r.action == "output-expiry" || r.action == "gate-cancel" ||
            r.action == "gate-stop") {
            if (!std::holds_alternative<bh::Done>(
                    bh::testing::Access::arm_first_wire(*r.foundation)))
                fixture("actual-node gate unavailable");
        }
    }
    auto &app = drogon::app();
    setup(app);
    if (r.action == "large-output")
        app.setAfterAcceptSockOptCallback([](int fd) {
            int n = 4096;
            if (::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n)))
                fixture("send buffer setup");
        });
    if (control) {
        app.addListener("127.0.0.1", 0, false, "", "", false, {}, false);
        app.registerHandler("/legacy-positive",
                            [&r](const drogon::HttpRequestPtr &,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
                                must(++r.legacy == 1, "legacy exactly one callback");
                                auto response = drogon::HttpResponse::newHttpResponse();
                                response->setBody("legacy-positive");
                                cb(response);
                                emit(event("legacy_request"));
                            },
                            {drogon::Get});
    } else {
        bh::Endpoints e{};
        std::memcpy(e.address.data(), "127.0.0.1", 10);
        required(bh::register_listeners(*r.foundation, app, e), "register actual two listeners");
    }
    app.registerBeginningAdvice([&r, control] {
        drogon::app().getLoop()->queueInLoop([&r, control] {
            auto endpoints = drogon::app().getListeners();
            must(endpoints.size() == (control ? 1 : 2), "actual endpoint count");
            auto http = endpoints[0].toPort();
            auto native = control ? 0 : endpoints[1].toPort();
            must(http && (control || (native && native != http)), "actual distinct OS ports");
            r.io = drogon::app().getIOLoop(0);
            r.io.load()->queueInLoop([&r, http, native] {
                maps("serving");
                auto e = event("listeners");
                e["http"] = http;
                e["native"] = native;
                emit(e);
            });
        });
    });
    std::atomic<bool> ran = false;
    std::thread server([&] {
        app.run();
        ran = true;
    });
    // One outstanding bounded fixture command at a time, owned copies <=256*512 total.
    std::array<char, 513> buf{};
    unsigned size = 0, commands = 0;
    for (unsigned turns = 0; turns < 6000 && !ran; ++turns) {
        pollfd fd{0, POLLIN, 0};
        auto status = ::poll(&fd, 1, 5);
        if (status < 0 && errno == EINTR)
            continue;
        if (status < 0)
            fixture("command poll");
        if (status == 0)
            continue;
        std::array<char, 512> incoming{};
        auto nread = ::read(0, incoming.data(), incoming.size());
        if (nread <= 0)
            fixture("command EOF before cleanup");
        for (ssize_t j = 0; j < nread; ++j) {
            char c = incoming[j];
            if (c != '\n') {
                if (size == 512)
                    fixture("command cap");
                buf[size++] = c;
                continue;
            }
            if (++commands > 256 || !r.io.load())
                fixture("command count/order");
            auto v = parse({buf.data(), size});
            size = 0;
            if (v["op"] == "release-connected") {
                r.fullReceiveRelease = true;
                auto e = event("ack");
                e["op"] = "release-connected";
                emit(e);
                continue;
            }
            if (control) {
                if (v["op"] != "stop")
                    fixture("legacy command");
                r.io.load()->queueInLoop([] { drogon::app().quit(); });
            } else
                r.io.load()->queueInLoop([&r, v] { r.command(v); });
        }
    }
    if (!ran)
        fixture("command/read turns exhausted");
    server.join();
    emit(event("framework_joined"));
    if (!control) {
        must(r.drained, "drain before framework join");
        if (r.action == "retain-inert") {
            error(r.foundation->finish(), bh::Error::incomplete_shutdown,
                  "inert handles pin finish");
            std::thread t([&] {
                for (auto &p : r.peers) {
                    p.request.reset();
                    p.reply.reset();
                    p.peer.reset();
                    p.wake.reset();
                }
            });
            t.join();
            emit(event("inert_foreign_destroyed_after_join"));
        }
        for (auto &p : r.peers) {
            p.request.reset();
            p.reply.reset();
            p.peer.reset();
            p.wake.reset();
        }
        if (r.wipeFails)
            error(r.foundation->finish(), bh::Error::wipe_failed, "quarantine finish");
        else
            required(r.foundation->finish(), "finish after joins/handle quiescence");
        r.finished = true;
    }
    maps("cleanup");
    auto final = event("cleanup_complete");
    final["native_registration_retained_until_process_exit"] = true;
    emit(final);
    return 0;
}
