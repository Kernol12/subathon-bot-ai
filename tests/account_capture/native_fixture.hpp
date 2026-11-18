// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Ordinary typed GoogleTest fixture. SOURCE ONLY: no registration/admission implied.
#include "capture_assertions.hpp"
#include "erasure_observation.hpp"
#include "log_observation.hpp"
#include "runtime_config.hpp"
#include "selected_identity.hpp" // configured independent expected DATA, never producer output
#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <drogon/drogon.h>
#include <fcntl.h>
#include <filesystem>
#include <mutex>
#include <optional>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <trantor/net/EventLoop.h>
#include <unistd.h>
#include <utility>

namespace account_capture_test {
namespace bh = subathon::server::bounded_http;
namespace ws = subathon::server::native_ws;
using namespace std::chrono_literals;

// All fixture faults are fixed nonsecret text. A failed bounded drain/join is a
// FIXTURE exit90, never account behavior red or fabricated graceful cleanup.
[[noreturn]] inline void terminal_fixture() noexcept {
    std::_Exit(90); // distinguished terminal fixture status; no blocking diagnostic write
}
class Deadline {
    std::mutex mutex_;
    std::condition_variable changed_;
    bool done_ = false;
    std::thread thread_;

  public:
    Deadline()
        : thread_([this] {
              std::unique_lock lock(mutex_);
              if (!changed_.wait_for(lock, 30s, [this] { return done_; }))
                  terminal_fixture();
          }) {}
    ~Deadline() {
        {
            std::lock_guard lock(mutex_);
            done_ = true;
        }
        changed_.notify_one();
        thread_.join();
    }
};
class Socket {
    int fd_ = -1;
    SecretScanner wire_scanner_;

  public:
    Socket() = default;
    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;
    ~Socket() { reset(); }
    void reset() noexcept {
        if (fd_ >= 0)
            ::close(std::exchange(fd_, -1));
    }
    int get() const { return fd_; }
    std::uint16_t connect(std::uint16_t port) {
        if (fd_ >= 0)
            throw std::runtime_error("FIXTURE: socket already owned");
        fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (fd_ < 0)
            throw std::runtime_error("FIXTURE: client socket");
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        const auto rc = ::connect(fd_, reinterpret_cast<sockaddr *>(&address), sizeof(address));
        if (rc && errno != EINPROGRESS)
            throw std::runtime_error("FIXTURE: connect");
        pollfd p{fd_, POLLOUT, 0};
        if (::poll(&p, 1, 7000) != 1)
            throw std::runtime_error("FIXTURE: connect deadline");
        int error = 0;
        socklen_t n = sizeof(error);
        if (::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &n) || error)
            throw std::runtime_error("FIXTURE: connect status");
        n = sizeof(address);
        if (::getsockname(fd_, reinterpret_cast<sockaddr *>(&address), &n))
            throw std::runtime_error("FIXTURE: actual client endpoint");
        return ntohs(address.sin_port);
    }
    void send(std::string_view bytes) {
        if (bytes.size() > 65536)
            throw std::runtime_error("FIXTURE: send capacity");
        const auto until = std::chrono::steady_clock::now() + 7s;
        for (unsigned turns = 0; !bytes.empty() && turns < 8192; ++turns) {
            if (std::chrono::steady_clock::now() >= until)
                break;
            auto n =
                ::send(fd_, bytes.data(), std::min<std::size_t>(8192, bytes.size()), MSG_NOSIGNAL);
            if (n > 0)
                bytes.remove_prefix(n);
            else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                pollfd p{fd_, POLLOUT, 0};
                (void)::poll(&p, 1, 10);
            } else
                throw std::runtime_error("FIXTURE: send status");
        }
        if (!bytes.empty())
            throw std::runtime_error("FIXTURE: send deadline/turns");
    }
    std::size_t read(std::span<char> output, bool through_eof = false) {
        if (output.size() > 65664)
            throw std::runtime_error("FIXTURE: read capacity");
        std::size_t used = 0;
        const auto until = std::chrono::steady_clock::now() + 7s;
        for (unsigned turns = 0; turns < 8192 && std::chrono::steady_clock::now() < until;
             ++turns) {
            if (used == output.size())
                return used;
            auto n = ::recv(fd_, output.data() + used,
                            std::min<std::size_t>(8192, output.size() - used), 0);
            if (n > 0) {
                wire_scanner_.feed({output.data() + used, static_cast<std::size_t>(n)});
                used += n;
                if (!through_eof && used == output.size())
                    return used;
            } else if (n == 0)
                return used;
            else if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                pollfd p{fd_, POLLIN, 0};
                (void)::poll(&p, 1, 10);
            } else
                throw std::runtime_error("FIXTURE: receive status");
        }
        throw std::runtime_error("FIXTURE: receive deadline/turns");
    }
    bool finish_wire() {
        if (fd_ < 0)
            return wire_scanner_.clean(); // explicitly closed peer cannot be read back
        std::array<char, 65664> remaining{};
        auto bytes = read(remaining, true); // after actual server stop/join, require EOF
        if (bytes == remaining.size())
            throw std::runtime_error("FIXTURE: residual wire ceiling");
        EXPECT_EQ(bytes, 0U) << "Unexpected bytes after exact response completion";
        reset();
        return wire_scanner_.clean();
    }
    void eof() {
        if (::shutdown(fd_, SHUT_WR))
            throw std::runtime_error("FIXTURE: shutdown");
    }
};
class Clock final : public ws::ElapsedSource {
  public:
    std::atomic<std::uint64_t> ns{1000000000};
    std::variant<ws::ElapsedInstant, ws::ElapsedUnavailable> sample() const noexcept override {
        return ws::ElapsedInstant{ns.load()};
    }
};
enum class LeaseAction {
    exact,
    lifetime,
    nested,
    move,
    foreign,
    null_visitor,
    retain,
    visitor_throw,
    callback_throw
};
struct Expectation {
    std::span<const Field> fields;
    std::string_view method = "GET", target = "/api/reference/v1/timer";
    std::span<const std::byte> body{};
    LeaseAction action = LeaseAction::exact;
};
struct Context {
    std::optional<bh::Peer> peer;
    std::optional<bh::Wake> wake;
    std::optional<bh::Request> request;
    std::optional<bh::Reply> reply;
    unsigned ends = 0, removes = 0;
    std::uint64_t previous_ordinal = 0;
    bh::End end{};
};
static_assert(sizeof(Context) <= 128);
// Dedicated foreign-hook conflict oracle. This ordinary generic eraser is NOT
// account/sodium evidence, and is installed ONLY before a refused account install.
struct ForeignHook {
    unsigned wipes = 0, revocations = 0;
    bool failed = false;
    static bool wipe(void *owner, bh::RawTag tag, std::span<std::byte> bytes) noexcept {
        auto &self = *static_cast<ForeignHook *>(owner);
        if (self.wipes == 8192 || ((tag.use == bh::RawUse::pending) != (tag.sequence == 0))) {
            self.failed = true;
            return false;
        }
        ++self.wipes;
        std::fill(bytes.begin(), bytes.end(), std::byte{}); // generic test behavior only
        return true;
    }
    static void revoked(void *owner, void *, bh::U64 sequence, bh::End) noexcept {
        auto &self = *static_cast<ForeignHook *>(owner);
        if (!sequence || self.revocations == 2)
            self.failed = true;
        else
            ++self.revocations;
    }
    bh::RawHooks binding() noexcept { return {this, wipe, revoked}; }
};
static_assert(sizeof(ForeignHook) <= 16);
// Exactly one process-retained registration owner. No raw byte/history allocations.
// Callback fields are IO0-only unless atomic or protected by mutex below.
struct Rig {
    std::shared_ptr<Clock> clock = std::make_shared<Clock>();
    std::unique_ptr<ws::Profile> native;
    std::unique_ptr<bh::Foundation> foundation;
    std::array<Context, 2> contexts;
    std::optional<ac::CaptureLease> retained;
    Expectation expected{};
    ErasureObservation erasures;
    ForeignHook foreign_hook;
    LogObservation logs;
    bool require_erasure = false;
    std::atomic<unsigned> connected{0}, captures{0}, requests{0}, removed{0}, ends{0};
    std::atomic<unsigned> native_opened{0}, native_messages{0}, native_closed{0};
    std::atomic<bool> ready{false}, drained{false}, returned{false}, fault{false};
    std::atomic<bool> hold{false}, cancel{false}, initial_admit{true};
    std::atomic<unsigned> turns{0};
    std::atomic<std::uint16_t> client_port{0};
    std::uint16_t http_port = 0, native_port = 0;
    trantor::EventLoop *io = nullptr;
    bh::Observation observation{};
    std::array<bh::ReadSample, 2> read_latches{};
    unsigned latch_count = 0, read_callbacks = 0;
    std::atomic<std::uint64_t> read_bytes{0};
    std::uint64_t last_sequence = 0, connection_id = 0, listener_id = 0;
    std::atomic<bool> generic_body_check{false};
    std::mutex mutex;
    std::condition_variable changed;
    static Rig &self(void *p) { return *static_cast<Rig *>(p); }
    void notify() {
        std::lock_guard lock(mutex);
        changed.notify_all();
    }
    bool done(bh::Result<bh::Done> result) {
        if (std::holds_alternative<bh::Done>(result))
            return true;
        fault = true;
        notify();
        return false;
    }
    template <class Predicate> void wait(Predicate predicate) {
        // Predicate reads only atomics or publication protected by this mutex.
        std::unique_lock lock(mutex);
        if (!changed.wait_for(lock, 7s, [&] { return predicate() || fault.load(); }) || fault)
            throw std::runtime_error("FIXTURE: callback/deadline");
    }
    static void connected_cb(void *o, bh::Peer handle) {
        auto &r = self(o);
        auto i = r.connected.load();
        if (i == r.contexts.size()) {
            r.fault = true;
            r.notify();
            return;
        }
        auto &c = r.contexts[i];
        c.peer.emplace(std::move(handle));
        if (!r.done(c.peer->context(&c)))
            return;
        auto w = c.peer->wake();
        if (!std::holds_alternative<bh::Wake>(w)) {
            r.fault = true;
            r.notify();
            return;
        }
        c.wake.emplace(std::move(std::get<bh::Wake>(w)));
        r.connected = i + 1;
        if (r.initial_admit)
            r.done(r.foundation->admission(*c.peer, bh::Pause::admitted));
        r.notify();
    }
    void reply(Context &c) {
        if (!c.request) {
            fault = true;
            notify();
            return;
        }
        auto result = c.request->reserve_reply({200, 4, false});
        if (!std::holds_alternative<bh::Reply>(result)) {
            fault = true;
            notify();
            return;
        }
        c.reply.emplace(std::move(std::get<bh::Reply>(result)));
        if (!done(c.reply->fill(nullptr, [](void *, std::span<std::byte> bytes) -> bh::U64 {
                if (bytes.size() != 4)
                    return 0;
                std::memcpy(bytes.data(), "diag", 4);
                return 4;
            })))
            return;
        done(c.reply->commit());
        c.reply.reset();
        c.request->release_raw();
        c.request.reset();
    }
    static void request_cb(void *o, void *p, bh::Request request) {
        auto &r = self(o);
        auto &c = *static_cast<Context *>(p);
        c.request.emplace(std::move(request));
        if (r.generic_body_check) {
            r.done(c.request->visit(nullptr, [](void *, bh::RequestView view) {
                EXPECT_TRUE(view.method == "POST" && view.escaped_target == "/f");
                constexpr std::array<std::byte, 4> body{std::byte{'d'}, std::byte{'a'},
                                                        std::byte{'t'}, std::byte{'a'}};
                EXPECT_EQ(view.fields.size(), 2U);
                EXPECT_EQ(view.body.size(), body.size());
                EXPECT_TRUE(
                    std::equal(view.body.begin(), view.body.end(), body.begin(), body.end()));
            }));
        }
        if (r.requests.fetch_add(1) >= 2) {
            ADD_FAILURE() << "Unexpected third request";
            r.fault = true;
            r.notify();
            return;
        }
        if (!r.hold)
            r.reply(c);
        r.notify();
    }
    static void ready_cb(void *o, void *p) {
        auto &r = self(o);
        auto &c = *static_cast<Context *>(p);
        if (r.retained)
            second_or_expired_visit(*r.retained);
        if (r.cancel) {
            c.reply.reset();
            c.request.reset();
        } else if (c.request && !r.hold)
            r.reply(c);
        auto inspected = r.foundation->inspect();
        if (!std::holds_alternative<bh::Observation>(inspected)) {
            r.fault = true;
            r.notify();
            return;
        }
        {
            std::lock_guard lock(r.mutex);
            r.observation = std::get<bh::Observation>(inspected);
        }
        ++r.turns;
        r.notify();
    }
    static void ended_cb(void *o, void *p, bh::End end) {
        auto &r = self(o);
        auto &c = *static_cast<Context *>(p);
        if (++c.ends > 2) {
            r.fault = true;
            r.notify();
            return;
        }
        c.end = end;
        r.ends.fetch_add(1);
        r.notify();
    }
    static void removed_cb(void *o, void *p) {
        auto &r = self(o);
        auto &c = *static_cast<Context *>(p);
        if (++c.removes != 1)
            r.fault = true;
        // Request/Reply may be inert; destroy before the final peer metadata return.
        {
            std::lock_guard lock(r.mutex);
            c.reply.reset();
            c.request.reset();
            c.wake.reset();
            c.peer.reset();
        }
        r.removed.fetch_add(1);
        r.notify();
    }
    static void drained_cb(void *o) {
        auto &r = self(o);
        auto inspected = r.foundation->inspect();
        if (auto actual = std::get_if<bh::Observation>(&inspected)) {
            std::lock_guard lock(r.mutex);
            r.observation = *actual;
        } else {
            ADD_FAILURE() << "FIXTURE: actual drained resource snapshot unavailable";
            r.fault = true;
        }
        r.drained = true;
        r.notify();
        drogon::app().quit();
    }
    static void read_cb(void *o, void *p, bh::ReadSample sample) {
        auto &r = self(o);
        auto &c = *static_cast<Context *>(p);
        if (++r.read_callbacks > 8192) {
            ADD_FAILURE() << "FIXTURE: read callback inventory exhausted";
            r.fault = true;
            r.notify();
            return;
        }
        EXPECT_GT(sample.ordinal, c.previous_ordinal);
        c.previous_ordinal = sample.ordinal;
        EXPECT_EQ(sample.iovcnt, 1U);
        EXPECT_GE(sample.writable, 8192U);
        EXPECT_LE(sample.offered, sample.writable);
        EXPECT_LE(sample.offered, 65536U);
        if (sample.returned > 0 && r.latch_count < 2)
            r.read_latches[r.latch_count++] = sample;
        r.read_bytes = sample.bytes_read;
        r.notify();
    }
    static void opened_cb(void *o, void *, const ws::Connection &) {
        auto &r = self(o);
        ++r.native_opened;
        r.notify();
    }
    static void message_cb(void *o, void *, const ws::Connection &conn) {
        auto &r = self(o);
        auto accepted = r.native->accept_callback(conn);
        if (!std::holds_alternative<ws::MessageLease>(accepted.value)) {
            r.fault = true;
            r.notify();
            return;
        }
        auto lease = std::move(std::get<ws::MessageLease>(accepted.value));
        auto view = lease.view();
        EXPECT_TRUE(view && view->bytes.size() == 1 && view->bytes[0] == std::byte{0x41});
        ++r.native_messages;
        r.notify(); // real lease is released before callback return
    }
    static void closed_cb(void *o, void *, const ws::Connection &) {
        auto &r = self(o);
        ++r.native_closed;
        r.notify();
    }
    static void capture_cb(void *o, ac::CaptureLease lease) {
        auto &r = self(o);
        if (r.captures.load() >= 2) {
            ADD_FAILURE() << "Unexpected third capture";
            r.fault = true;
            r.notify();
            return;
        }
        auto action = r.expected.action;
        if (action == LeaseAction::retain) {
            // Retain an UNVISITED real lease, so the later revoked result proves
            // scope completion rather than merely the once-visit guard.
            r.retained.emplace(std::move(lease));
            ++r.captures;
            r.notify();
            return;
        }
        if (action == LeaseAction::lifetime) {
            stable_after_visit(lease, r.expected.fields);
            ++r.captures;
            r.notify();
            return;
        }
        if (action == LeaseAction::callback_throw)
            throw std::runtime_error("synthetic capture exception");
        if (action == LeaseAction::foreign) {
            // No event pumping/client work in parallel; exclusive lease access,
            // joined fixed worker. Watchdog bounds an impossible join.
            std::thread foreign([&] { refused_visit(lease, ac::Error::wrong_thread); });
            foreign.join();
        }
        if (action == LeaseAction::move) {
            auto moved = std::move(lease);
            refused_visit(lease, ac::Error::revoked);
            lease = std::move(moved);
        }
        if (action == LeaseAction::null_visitor) {
            unsigned null_visits = 0;
            const auto refused = lease.with_capture(&null_visits, nullptr);
            EXPECT_TRUE(std::holds_alternative<ac::Error>(refused));
            if (auto error = std::get_if<ac::Error>(&refused))
                EXPECT_EQ(*error, ac::Error::invalid_config);
            EXPECT_EQ(null_visits, 0U);
        }
        struct Visit {
            Rig *r;
            ac::CaptureLease *lease;
            unsigned calls = 0;
        } visit{&r, &lease};
        const auto result = lease.with_capture(&visit, [](void *p, ac::CaptureView view) {
            auto &v = *static_cast<Visit *>(p);
            ++v.calls;
            auto &r = *v.r;
            if (r.expected.action == LeaseAction::visitor_throw)
                throw std::runtime_error("synthetic visitor exception");
            exact_capture(view, r.expected.fields, r.expected.method, r.expected.target,
                          r.expected.body);
            EXPECT_EQ(view.native.peer_port, r.client_port.load());
            constexpr std::string_view peer = "127.0.0.1";
            EXPECT_TRUE(std::equal(peer.begin(), peer.end(), view.native.peer_address.begin()));
            EXPECT_EQ(view.native.peer_address[peer.size()], '\0');
            EXPECT_NE(view.native.connection, 0U);
            EXPECT_NE(view.native.listener, 0U);
            EXPECT_EQ(view.native.request_sequence, r.last_sequence + 1);
            if (r.last_sequence) {
                EXPECT_EQ(view.native.connection, r.connection_id);
                EXPECT_EQ(view.native.listener, r.listener_id);
            }
            r.last_sequence = view.native.request_sequence;
            r.connection_id = view.native.connection;
            r.listener_id = view.native.listener;
            EXPECT_TRUE(view.native.artifact_sha256 == selected_identity::artifact);
            EXPECT_TRUE(view.native.profile_sha256 == selected_identity::profile);
            if (r.expected.action == LeaseAction::nested)
                nested_visit(*v.lease);
        });
        EXPECT_EQ(visit.calls, 1U); // includes positive reach after a refused null visitor
        if (action == LeaseAction::visitor_throw) {
            EXPECT_TRUE(std::holds_alternative<ac::Error>(result));
            if (auto error = std::get_if<ac::Error>(&result))
                EXPECT_EQ(*error, ac::Error::callback_failed);
        } else
            EXPECT_TRUE(std::holds_alternative<ac::Done>(result));
        if (action != LeaseAction::visitor_throw)
            second_or_expired_visit(lease);
        r.captures.fetch_add(1);
        r.notify();
    }
};
static_assert(sizeof(Rig) <= 2048); // metadata only, caller fixture reservation
inline Rig *retained_rig = nullptr; // one normal GTest body per process; no reinstall

struct ObserverPolicy {
    bool reject_before = false, reject_after = false;
    unsigned limit = ErasureObservation::ceiling;
};
// Private ordinary typed fixture setup expectations, never a CLI/SDK mode.
enum class SetupExpectation { ordinary, late_account, late_observer, foreign_hook, observer_first };
class Live {
    Deadline watchdog_; // destroyed AFTER socket shutdown, stop/drain/join and scratch cleanup
    std::thread framework_;
    std::array<char, 534> scratch_{};
    bool started_ = false, scratch_parent_created_ = false;
    static void refusal(ac::Result<ac::Done> result, ac::Error expected) {
        if (!std::holds_alternative<ac::Error>(result)) {
            ADD_FAILURE() << "Refusal boundary missing/incorrect result (NI is not success)";
            throw std::runtime_error("refusal boundary unreached or wrong result");
        }
        EXPECT_EQ(std::get<ac::Error>(result), expected);
        if (std::get<ac::Error>(result) != expected)
            throw std::runtime_error("wrong refusal result");
    }
    void scratch() {
        const auto path = std::filesystem::path(runtime_config::scratch_parent);
        if (!path.is_absolute() || path.lexically_normal() != path ||
            path.filename() != "account-capture-runtime")
            throw std::runtime_error("FIXTURE: configured scratch confinement");
        struct stat parent{};
        if (::lstat(path.parent_path().c_str(), &parent) || !S_ISDIR(parent.st_mode) ||
            parent.st_uid != ::geteuid())
            throw std::runtime_error("FIXTURE: caller build directory");
        if (::mkdir(path.c_str(), 0700) == 0)
            scratch_parent_created_ = true;
        else if (errno != EEXIST)
            throw std::runtime_error("FIXTURE: scratch parent");
        struct stat info{};
        if (::lstat(path.c_str(), &info) || !S_ISDIR(info.st_mode) || info.st_uid != ::geteuid())
            throw std::runtime_error("FIXTURE: scratch ownership/symlink");
        auto pattern = (path / "capture-XXXXXX").string();
        if (pattern.size() >= scratch_.size())
            throw std::runtime_error("FIXTURE: scratch length");
        std::array<char, 534> candidate{};
        std::memcpy(candidate.data(), pattern.c_str(), pattern.size() + 1);
        if (!::mkdtemp(candidate.data()))
            throw std::runtime_error("FIXTURE: scratch creation");
        scratch_ = candidate; // cleanup owns a leaf only after successful creation
        for (auto part : {"empty", "upload"})
            if (::mkdir((std::filesystem::path(scratch_.data()) / part).c_str(), 0700))
                throw std::runtime_error("FIXTURE: scratch subdirectory");
    }

  public:
    Rig &r;
    bool expect_wipe_failure = false; // creator-only; failed finish retains quarantine to exit
    bool foreign_inert_drop = false;  // creator-only; used after all IO callbacks/join
    Socket client, native_client;     // two clients maximum; no caller-owned client thread
    explicit Live(bool account, Expectation expected = {}, bool budget = false, bool observe = true,
                  ObserverPolicy policy = {}, SetupExpectation setup = SetupExpectation::ordinary)
        : r(*([] {
              if (retained_rig)
                  throw std::runtime_error("FIXTURE: one fixture/process");
              return retained_rig = new Rig;
          })()) {
        try {
            r.expected = expected;
            if (!policy.limit || policy.limit > ErasureObservation::ceiling)
                throw std::runtime_error("FIXTURE: observer capacity");
            r.erasures.limit = policy.limit;
            r.erasures.reject_before = policy.reject_before;
            r.erasures.reject_after = policy.reject_after;
            bind_checked_log(r.logs); // BEFORE profile, Foundation or framework/listener work
            scratch();
            const auto backend = ws::linked_backend();
            EXPECT_EQ(backend, ws::Backend::admitted_corrected);
            if (backend != ws::Backend::admitted_corrected)
                throw std::runtime_error("FIXTURE: corrected backend required");
            auto p = ws::create_profile(
                {1, 1, 64, 64, 16, 2, 128, 256, 256, 64, 256, 254, 8, 50, 50, 100, 50}, r.clock);
            if (!std::holds_alternative<std::unique_ptr<ws::Profile>>(p))
                throw std::runtime_error("FIXTURE: real native profile");
            r.native = std::move(std::get<std::unique_ptr<ws::Profile>>(p));
            // Private finite TEST DATA. No production route/AccountIngressSource.
            const std::array routes{
                bh::Route{"GET", "/api/reference/v1/timer", 4096, 4096, 4096},
                bh::Route{"POST", "/f", 4, 0, 0}, bh::Route{"POST", "/p", 4096, 4096, 4096},
                bh::Route{"POST", "/api/reference/v1/negotiate", 4096, 4096, 4096}};
            auto f = bh::create(
                {2, 1, 1, 1, 1, budget ? 8192ULL : 4096ULL, 4, 5000, 5000, 5000, 5000, 10}, routes,
                {"/foundation/ws", Rig::opened_cb, Rig::message_cb, Rig::closed_cb}, r.clock,
                {&r, Rig::connected_cb, Rig::request_cb, Rig::ready_cb, Rig::ended_cb,
                 Rig::removed_cb, Rig::drained_cb, Rig::read_cb});
            if (!std::holds_alternative<std::unique_ptr<bh::Foundation>>(f))
                throw std::runtime_error("FIXTURE: real foundation");
            r.foundation = std::move(std::get<std::unique_ptr<bh::Foundation>>(f));
            if (setup == SetupExpectation::foreign_hook) {
                if (account)
                    throw std::runtime_error("FIXTURE: conflict requires generic fixture");
                if (!r.done(bh::install_raw_hooks(*r.foundation, r.foreign_hook.binding())))
                    throw std::runtime_error("FIXTURE: foreign hook setup");
                refusal(ac::install(*r.foundation, {&r, Rig::capture_cb}),
                        ac::Error::invalid_state);
                // The one slot is still occupied; do not replace or chain it.
                refusal(bh::install_raw_hooks(*r.foundation, r.foreign_hook.binding()),
                        ac::Error::invalid_state);
            }
            if (setup == SetupExpectation::observer_first) {
                if (!account || !observe)
                    throw std::runtime_error("FIXTURE: observer order configuration");
                auto result = eo::install_erasure_observer(
                    *r.foundation, {&r.erasures, ErasureObservation::callback});
                if (!std::holds_alternative<ac::Done>(result)) {
                    ADD_FAILURE()
                        << "Observer-before-account boundary NI/refusal; later phases unreached";
                    throw std::runtime_error("private observer-before-account unavailable");
                }
                r.require_erasure = true;
            }
            if ((setup == SetupExpectation::late_account ||
                 setup == SetupExpectation::late_observer) &&
                account)
                throw std::runtime_error("FIXTURE: late refusal needs no earlier account install");
            if (account) {
                auto result = ac::install(*r.foundation, {&r, Rig::capture_cb});
                if (!std::holds_alternative<ac::Done>(result)) {
                    if (std::holds_alternative<ac::NI>(result))
                        ADD_FAILURE() << "MISSING account capture producer at cold install; later "
                                         "phases unreached";
                    else
                        ADD_FAILURE()
                            << "Account capture installation refused; classify independently";
                    throw std::runtime_error("account capture NI/refusal; no live-case evidence");
                }
            }
            if (account && observe && setup != SetupExpectation::observer_first) {
                auto result = eo::install_erasure_observer(
                    *r.foundation, {&r.erasures, ErasureObservation::callback});
                if (!std::holds_alternative<ac::Done>(result)) {
                    ADD_FAILURE()
                        << "MISSING/private erasure observer binding; later phases unreached";
                    throw std::runtime_error("observer NI/refusal; no erasure evidence");
                }
                r.require_erasure = true;
            }
            if (setup == SetupExpectation::observer_first)
                refusal(eo::install_erasure_observer(*r.foundation,
                                                     {&r.erasures, ErasureObservation::callback}),
                        ac::Error::invalid_state);
            auto &app = drogon::app();
            app.disableSigtermHandling()
                .setThreadNum(1)
                .setIdleConnectionTimeout(0)
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
                .setDocumentRoot((std::filesystem::path(scratch_.data()) / "empty").string())
                .setUploadPath((std::filesystem::path(scratch_.data()) / "upload").string())
                .setClientMaxBodySize(4096)
                .setClientMaxMemoryBodySize(4096);
            app.enableReusePort(true);
            app.setBeforeListenSockOptCallback([](int fd) {
                int zero = 0, reuse = 0, address = 0;
                socklen_t n = sizeof(int);
                if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &zero, sizeof(zero)) ||
                    ::getsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse, &n) || reuse ||
                    ::getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &address, &n) || address != 1)
                    terminal_fixture();
            });
            bh::Endpoints endpoints{};
            std::memcpy(endpoints.address.data(), "127.0.0.1", 10);
            if (!r.done(bh::register_listeners(*r.foundation, app, endpoints)))
                throw std::runtime_error("FIXTURE: register listeners");
            if (setup == SetupExpectation::late_account)
                refusal(ac::install(*r.foundation, {&r, Rig::capture_cb}),
                        ac::Error::invalid_state);
            if (setup == SetupExpectation::late_observer)
                refusal(eo::install_erasure_observer(*r.foundation,
                                                     {&r.erasures, ErasureObservation::callback}),
                        ac::Error::invalid_state);
            app.registerBeginningAdvice([&] {
                app.getLoop()->queueInLoop([&] {
                    auto endpoints = app.getListeners();
                    if (endpoints.size() != 2) {
                        r.fault = true;
                        r.notify();
                        return;
                    }
                    auto http = endpoints[0].toPort(), native = endpoints[1].toPort();
                    r.io = app.getIOLoop(0);
                    r.io->queueInLoop([&, http, native] {
                        {
                            std::lock_guard lock(r.mutex);
                            r.http_port = http;
                            r.native_port = native;
                        }
                        if (!http || !native || http == native)
                            r.fault = true;
                        r.ready = true;
                        r.notify();
                    });
                });
            });
            framework_ = std::thread([&] {
                app.run();
                r.returned = true;
                r.notify();
            });
            started_ = true;
            r.wait([&] { return r.ready.load(); });
        } catch (...) {
            cleanup();
            throw;
        }
    }
    void connect() { r.client_port = client.connect(r.http_port); }
    void post(bh::Intent intent = bh::Intent::owner_ready) {
        // Wake is stable from connected publication through client closure/removal.
        // Do not call after EOF/close/removal or during cleanup.
        std::lock_guard lock(r.mutex);
        auto &c = r.contexts[0];
        if (!c.wake)
            throw std::runtime_error("FIXTURE: missing issued wake");
        auto result = r.foundation->post(*c.wake, intent);
        if (!std::holds_alternative<bh::Post>(result))
            throw std::runtime_error("FIXTURE: post");
    }
    void cleanup() noexcept {
        try {
            // Stop server admission/transport first; keep client FDs to scan any
            // unread response tail after the actual drain/join, then close them.
            if (r.foundation && started_) {
                (void)r.foundation->stop({r.clock->ns.load() + 5000000000ULL});
                // No predicate ignores fixture faults during teardown; wait for the
                // real barrier even after an earlier assertion/socket failure.
                std::unique_lock lock(r.mutex);
                if (!r.changed.wait_for(lock, 7s,
                                        [&] { return r.returned.load() && r.drained.load(); }))
                    terminal_fixture();
                lock.unlock();
                if (framework_.joinable())
                    framework_.join();
                // IO is joined; only inert handle destruction is legal here.
                if (foreign_inert_drop) {
                    std::thread foreign([&] { r.retained.reset(); });
                    foreign.join(); // no IO/native access, only inert state destruction
                } else
                    r.retained.reset();
                for (auto &c : r.contexts) {
                    c.reply.reset();
                    c.request.reset();
                    c.wake.reset();
                    c.peer.reset();
                }
                const auto finished = r.foundation->finish();
                if (expect_wipe_failure) {
                    EXPECT_TRUE(std::holds_alternative<bh::Error>(finished));
                    if (auto error = std::get_if<bh::Error>(&finished))
                        EXPECT_EQ(*error, bh::Error::wipe_failed);
                    // Keep actual charged quarantined core in process-retained Rig.
                    // No destructor/Runtime discard or successful memory cleanup claimed.
                } else if (!std::holds_alternative<bh::Done>(finished))
                    terminal_fixture();
                if (r.require_erasure && !expect_wipe_failure) {
                    EXPECT_TRUE(r.erasures.clean());
                    EXPECT_GT(r.erasures.calls, 0U); // no all-unavailable/all-zero observation pass
                    EXPECT_TRUE(r.erasures.observed(eo::Region::receive, bh::RawUse::owner_teardown,
                                                    false));
                } else if (!r.require_erasure)
                    EXPECT_EQ(r.erasures.calls, 0U);
            }
            EXPECT_TRUE(client.finish_wire());
            EXPECT_TRUE(native_client.finish_wire());
            if (!expect_wipe_failure)
                r.foundation.reset();
            // Callback target and scanner stay alive through process teardown.
            EXPECT_TRUE(r.logs.clean());
            r.expected = {}; // no dangling expected-byte borrow in retained native registration
            started_ = false;
            if (scratch_[0]) {
                // No recursive traversal/deletion: only this owned fixed three-dir tree.
                if (::rmdir((std::filesystem::path(scratch_.data()) / "upload").c_str()) &&
                    errno != ENOENT)
                    terminal_fixture();
                if (::rmdir((std::filesystem::path(scratch_.data()) / "empty").c_str()) &&
                    errno != ENOENT)
                    terminal_fixture();
                if (::rmdir(scratch_.data()) && errno != ENOENT)
                    terminal_fixture();
                scratch_[0] = 0;
            }
            if (scratch_parent_created_) {
                if (::rmdir(std::filesystem::path(runtime_config::scratch_parent).c_str()) &&
                    errno != ENOENT && errno != ENOTEMPTY)
                    terminal_fixture();
                scratch_parent_created_ = false;
            }
        } catch (...) {
            terminal_fixture();
        }
    }
    ~Live() { cleanup(); }
};
} // namespace account_capture_test
