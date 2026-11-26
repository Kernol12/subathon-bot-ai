// SPDX-License-Identifier: GPL-3.0-or-later
#include "api_host_internal.hpp"
#include <QMetaObject>
#include <QObject>
#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <drogon/BoundedHttp.h>
#include <drogon/drogon.h>
#include <filesystem>
#include <mutex>
#include <subathon/server/api_host_drogon.hpp>
#include <subathon/server/bounded_http.hpp>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <trantor/net/EventLoop.h>
#include <unistd.h>

namespace subathon::server::api_host {
namespace {
namespace bh = bounded_http;
namespace fs = std::filesystem;
template <class T> bool ok(const bh::Result<T> &r) { return std::holds_alternative<T>(r); }
struct ConnectionRecord {
    const void *identity = nullptr;
    std::optional<PeerLease> host;
    std::optional<bh::Peer> peer;
    std::optional<bh::Wake> wake;
    std::optional<bh::Request> request;
    native_ws::Connection native;
    std::vector<std::optional<ResponseLease>> mail;
    bool live = false;
};
class DrogonTransport final : public Transport, public detail::TransportComposition {
    QObject owner_;
    DrogonOptions options_;
    ListenersObserver observer_;
    Limits limits_;
    std::shared_ptr<const native_ws::ElapsedSource> elapsed_;
    detail::FatalRelay fatal_relay_{};
    Ingress *ingress_ = nullptr;
    std::unique_ptr<native_ws::Profile> profile_;
    std::unique_ptr<bh::Foundation> foundation_;
    std::unique_ptr<ConnectionRecord[]> peers_;
    std::size_t peer_count_ = 0;
    std::vector<std::byte> wire_; // one separately bounded IO serialization scratch
    ConnectionRecord *connecting_ = nullptr;
    std::mutex mutex_;
    StartupObserver startup_;
    NetworkBarrierObserver barrier_;
    std::thread framework_;
    std::atomic<bool> stopping_{false}, drained_{false};
    std::atomic<unsigned> native_live_{0};
    std::atomic<trantor::EventLoop *> native_loop_{nullptr};
    trantor::TimerId native_close_timer_ = trantor::InvalidTimerId; // IO0 only
    StopDeadline native_drain_{};
    std::optional<std::uint64_t> native_close_time_;
    ListenerEndpoints endpoints_{};
    std::atomic<unsigned> socket_count_{0};
    bool native_first_ = false;
    struct Life {
        std::atomic<DrogonTransport *> target{nullptr};
    };
    std::shared_ptr<Life> life_ = std::make_shared<Life>();
    std::string scratch_;
    bool scratch_created_ = false, framework_launched_ = false;
    static DrogonTransport &self(void *p) { return *static_cast<DrogonTransport *>(p); }
    static ConnectionRecord &peer(void *p) { return *static_cast<ConnectionRecord *>(p); }
    void close(ConnectionRecord &p) {
        if (p.peer)
            (void)foundation_->close(*p.peer);
    }
    static void connected(void *owner, bh::Peer value) {
        auto &s = self(owner);
        ConnectionRecord *p = nullptr;
        {
            std::lock_guard lock(s.mutex_);
            for (std::size_t i = 0; i < s.peer_count_; ++i)
                if (!s.peers_[i].live) {
                    p = &s.peers_[i];
                    p->live = true;
                    break;
                }
        }
        if (!p)
            return; // Dropping the issued peer closes the unadmitted connection.
        p->peer.emplace(std::move(value));
        if (!ok(p->peer->context(p))) {
            s.close(*p);
            return;
        }
        auto wake = p->peer->wake();
        if (!std::holds_alternative<bh::Wake>(wake)) {
            s.close(*p);
            return;
        }
        {
            std::lock_guard lock(s.mutex_);
            p->wake.emplace(std::move(std::get<bh::Wake>(wake)));
        }
        s.connecting_ = p;
        auto host = s.ingress_->connected();
        s.connecting_ = nullptr;
        if (!std::holds_alternative<PeerLease>(host)) {
            s.close(*p);
            return;
        }
        p->host.emplace(std::move(std::get<PeerLease>(host)));
        if (!ok(s.foundation_->admission(*p->peer, bh::Pause::admitted)))
            s.close(*p);
    }
    static void request(void *owner, void *context, bh::Request request) {
        auto &s = self(owner);
        auto &p = peer(context);
        p.request.emplace(std::move(request));
        struct Visit {
            DrogonTransport &s;
            ConnectionRecord &p;
            bool accepted = false;
        } visit{s, p};
        const auto result = p.request->visit(&visit, [](void *data, bh::RequestView v) {
            auto &x = *static_cast<Visit *>(data);
            std::array<std::byte, 256> correlation{};
            std::size_t count = 0;
            for (auto field : v.fields) {
                std::string_view name(reinterpret_cast<const char *>(v.arena.data()) + field.name,
                                      field.name_size);
                constexpr std::string_view key = "x-host-correlation";
                if (name.size() != key.size() ||
                    !std::equal(name.begin(), name.end(), key.begin(),
                                [](unsigned char a, unsigned char b) {
                                    return (a >= 'A' && a <= 'Z' ? a + 32 : a) == b;
                                }))
                    continue;
                if (field.value_size == 0 || field.value_size % 2 || field.value_size > 512)
                    return;
                auto hex = [](char c) {
                    return c >= '0' && c <= '9'   ? c - '0'
                           : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                                  : -1;
                };
                const char *value = reinterpret_cast<const char *>(v.arena.data()) + field.value;
                for (unsigned i = 0; i < field.value_size; i += 2) {
                    int a = hex(value[i]), b = hex(value[i + 1]);
                    if (a < 0 || b < 0)
                        return;
                    correlation[count++] = std::byte(a * 16 + b);
                }
            }
            if (!count || !x.p.host || v.route_index > 3)
                return;
            x.accepted = std::holds_alternative<Admitted>(
                x.s.ingress_->try_submit(*x.p.host, {correlation.data(), count},
                                         static_cast<RouteId>(v.route_index), v.body));
        });
        p.request->release_raw();
        if (!ok(result) || !visit.accepted)
            s.close(p);
    }
    static void ready(void *owner, void *context) {
        auto &s = self(owner);
        auto &p = peer(context);
        // Each mailbox slot was reserved at startup. Moving a lease transfers its
        // host charge; the response borrow is used only on this IO owner.
        for (std::size_t i = 0; i < p.mail.size(); ++i) {
            std::optional<ResponseLease> response;
            {
                std::lock_guard lock(s.mutex_);
                if (p.mail[i]) {
                    response.emplace(std::move(*p.mail[i]));
                    p.mail[i].reset();
                }
            }
            if (!response)
                continue;
            auto result = response->take();
            if (auto *v = std::get_if<CommandView>(&result)) {
                if (p.native) {
                    if (v->correlation.size() != 1 || v->bytes.size() + 1 > s.wire_.size()) {
                        s.close(p);
                        continue;
                    }
                    s.wire_[0] = v->correlation[0];
                    std::copy(v->bytes.begin(), v->bytes.end(), s.wire_.begin() + 1);
                    auto sent = s.profile_->try_send(p.native, native_ws::MessageKind::binary,
                                                     {s.wire_.data(), v->bytes.size() + 1});
                    if (!std::holds_alternative<native_ws::Admitted>(sent))
                        s.close(p);
                } else if (p.request) {
                    auto reserved = p.request->reserve_reply({200, v->bytes.size(), false});
                    if (!std::holds_alternative<bh::Reply>(reserved)) {
                        s.close(p);
                        continue;
                    }
                    auto reply = std::move(std::get<bh::Reply>(reserved));
                    struct Bytes {
                        std::span<const std::byte> value;
                    } data{v->bytes};
                    auto filled =
                        reply.fill(&data, [](void *d, std::span<std::byte> into) -> bh::U64 {
                            auto bytes = static_cast<Bytes *>(d)->value;
                            if (into.size() != bytes.size())
                                return 0;
                            std::copy(bytes.begin(), bytes.end(), into.begin());
                            return bytes.size();
                        });
                    if (!ok(filled) || !ok(reply.commit()))
                        s.close(p);
                } else
                    s.close(p);
            } else
                s.close(p);
        }
    }
    static void ended(void *, void *context, bh::End) { peer(context).request.reset(); }
    void retire_native(ConnectionRecord &p) {
        if (p.native) {
            p.native.reset();
            native_live_.fetch_sub(1, std::memory_order_release);
        }
    }
    static void removed(void *owner, void *context) {
        auto &s = self(owner);
        auto &p = peer(context);
        if (p.host)
            s.ingress_->disconnected(*p.host);
        p.host.reset();
        p.request.reset();
        s.retire_native(p);
        p.peer.reset();
        {
            std::lock_guard lock(s.mutex_);
            p.wake.reset();
            for (auto &r : p.mail)
                r.reset();
            p.identity = nullptr;
            p.live = false;
        }
    }
    static void drained(void *owner) {
        auto &s = self(owner);
        s.drained_.store(true, std::memory_order_release);
        drogon::app().quit(); // after actual socket/acceptor/callback drain, never signal handler
    }
    static void opened(void *owner, void *context, const native_ws::Connection &c) {
        auto &s = self(owner);
        peer(context).native = c;
        s.native_loop_.store(drogon::app().getIOLoop(0), std::memory_order_release);
        s.native_live_.fetch_add(1, std::memory_order_release);
    }
    static void message(void *owner, void *context, const native_ws::Connection &c) {
        auto &s = self(owner);
        auto &p = peer(context);
        auto accepted = s.profile_->accept_callback(c);
        if (!std::holds_alternative<native_ws::MessageLease>(accepted.value)) {
            s.close(p);
            return;
        }
        auto lease = std::move(std::get<native_ws::MessageLease>(accepted.value));
        const auto view = lease.view();
        if (!view || view->kind != native_ws::MessageKind::binary || view->bytes.size() < 2 ||
            !p.host) {
            s.close(p);
            return;
        }
        const auto route = std::to_integer<unsigned>(view->bytes[1]);
        if (route > 3) {
            s.close(p);
            return;
        }
        const auto result = s.ingress_->try_submit(
            *p.host, view->bytes.first(1), static_cast<RouteId>(route), view->bytes.subspan(2));
        // Pressure refuses only this command. Closing the peer would revoke
        // replies belonging to commands already accepted on this connection.
        if (!std::holds_alternative<Admitted>(result) &&
            !(std::get_if<Failure>(&result) &&
              std::get<Failure>(result) == Failure::resource_limit))
            s.close(p);
    }
    static void closed(void *owner, void *context, const native_ws::Connection &) {
        self(owner).retire_native(peer(context));
    }
    // Established native peers keep their unchanged profile Close bound. The
    // foundation must not force-close them before that handshake can drain.
    // This IO-only hint checks the original host deadline; it never extends it.
    void poll_native_close() {
        const auto observation = elapsed_->sample();
        const auto *now = std::get_if<native_ws::ElapsedInstant>(&observation);
        const bool bad = !now || (native_close_time_ && now->nanoseconds < *native_close_time_);
        if (!bad)
            native_close_time_ = now->nanoseconds;
        if (native_live_.load(std::memory_order_acquire) && !bad &&
            now->nanoseconds < native_drain_.nanoseconds)
            return;
        if (native_close_timer_ != trantor::InvalidTimerId) {
            native_loop_.load()->invalidateTimer(native_close_timer_);
            native_close_timer_ = trantor::InvalidTimerId;
        }
        (void)foundation_->stop({native_drain_.nanoseconds});
    }
    void start_native_close(StopDeadline deadline) {
        native_drain_ = deadline;
        for (std::size_t i = 0; i < peer_count_; ++i) {
            auto &p = peers_[i];
            if (p.native && !std::holds_alternative<native_ws::CloseDisposition>(
                                profile_->start_close(p.native, native_ws::CloseWithoutStatus{})))
                close(p);
        }
        native_close_timer_ = native_loop_.load()->runEvery(0.005, [this] { poll_native_close(); });
        poll_native_close();
    }
    void configure_app() {
        auto &app = drogon::app();
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
            .setDocumentRoot(scratch_ + "/empty")
            .setUploadPath(scratch_ + "/upload")
            .setClientMaxBodySize(limits_.envelope_bytes)
            .setClientMaxMemoryBodySize(limits_.envelope_bytes);
        app.enableReusePort(true);
        app.setDefaultHandler([](const drogon::HttpRequestPtr &,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&done) {
            auto response = drogon::HttpResponse::newHttpResponse();
            response->setStatusCode(drogon::k400BadRequest);
            response->addHeader("Connection", "close");
            done(response);
        });
        auto life = life_;
        const auto refusal = drogon::HttpResponse::newHttpResponse();
        refusal->setStatusCode(drogon::k400BadRequest);
        refusal->addHeader("Connection", "close");
        app.registerSyncAdvice(
            [life, refusal](const drogon::HttpRequestPtr &request) -> drogon::HttpResponsePtr {
                auto *transport = life->target.load();
                if (!transport || transport->stopping_.load())
                    return refusal;
                const auto &path = request->getOriginalPath();
                if (path != request->path() || !request->query().empty())
                    return refusal;
                const bool native = request->localAddr().toPort() == transport->endpoints_.port;
                if (native)
                    return request->method() == drogon::Get && path == "/api-host/ws" ? nullptr
                                                                                      : refusal;
                if (request->localAddr().toPort() != transport->endpoints_.http_port ||
                    request->method() != drogon::Post)
                    return refusal;
                if (path != "/api-host/diagnostic" && path != "/api-host/installation" &&
                    path != "/api-host/credential" && path != "/api-host/account")
                    return refusal;
                for (const auto *name : {"upgrade", "content-encoding", "transfer-encoding", "te",
                                         "trailer", "expect"})
                    if (request->headers().find(name) != request->headers().end())
                        return refusal;
                return nullptr;
            });
        app.setBeforeListenSockOptCallback([life](int fd) {
            auto *s = life->target.load(std::memory_order_acquire);
            if (!s)
                std::_Exit(1);
            int zero = 0, reuse = 0, address = 0;
            socklen_t length = sizeof(int);
            if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &zero, sizeof(zero)) ||
                ::getsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse, &length) || reuse ||
                ::getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &address, &length) || address != 1)
                std::_Exit(1);
            const auto index = s->socket_count_.fetch_add(1);
            if (index >= 2)
                std::_Exit(1);
            const bool native = s->native_first_ ? index == 0 : index == 1;
            (native ? s->endpoints_.native_socket_options : s->endpoints_.http_socket_options) = 1;
        });
        // Existing SDK registration supplies the same bounded two-listener path;
        // order fixed endpoints first so an ephemeral bind cannot consume one.
        namespace sdk = drogon::bounded_http_sdk;
        namespace io = trantor::bounded_io;
        sdk::Endpoint native_endpoint{endpoints_.address, options_.port},
            http_endpoint{endpoints_.address, options_.http_port};
        native_first_ = options_.port && !options_.http_port;
        // API-024 Correction 1: seal the cold arm windows before the first SDK call.
        if (!ok(bh::seal_listener_registration(*foundation_)))
            throw std::runtime_error("Host listener registration refused");
        const bool registered =
            native_first_ ? (sdk::Runtime::listener(app, native_endpoint, io::Role::native) &&
                             sdk::Runtime::listener(app, http_endpoint, io::Role::http))
                          : (sdk::Runtime::listener(app, http_endpoint, io::Role::http) &&
                             sdk::Runtime::listener(app, native_endpoint, io::Role::native));
        if (!registered)
            throw std::runtime_error("Host listener registration refused");
        app.registerBeginningAdvice([life] {
            if (life->target.load())
                drogon::app().getLoop()->queueInLoop([life] {
                    auto *s = life->target.load();
                    if (!s)
                        return;
                    auto actual = drogon::app().getListeners();
                    if (actual.size() != 2)
                        std::_Exit(1);
                    const auto native = actual[s->native_first_ ? 0 : 1].toPort();
                    const auto http = actual[s->native_first_ ? 1 : 0].toPort();
                    if (!native || !http || native == http ||
                        (s->options_.port && s->options_.port != native) ||
                        (s->options_.http_port && s->options_.http_port != http))
                        std::_Exit(1);
                    drogon::app().getIOLoop(0)->queueInLoop([life, native, http] {
                        auto *ready_owner = life->target.load();
                        if (!ready_owner)
                            return;
                        ready_owner->endpoints_.port = native;
                        ready_owner->endpoints_.http_port = http;
                        QMetaObject::invokeMethod(
                            &ready_owner->owner_,
                            [ready_owner] {
                                if (ready_owner->stopping_.load())
                                    return;
                                try {
                                    if (ready_owner->observer_)
                                        ready_owner->observer_(ready_owner->endpoints_);
                                } catch (...) {
                                    ready_owner->startup_(Failure::unavailable);
                                    return;
                                }
                                ready_owner->startup_(StartAccepted{});
                            },
                            Qt::QueuedConnection);
                    });
                });
        });
    }

  public:
    DrogonTransport(DrogonOptions o, ListenersObserver observer)
        : options_(std::move(o)), observer_(std::move(observer)) {
        life_->target.store(this);
    }
    ~DrogonTransport() override {
        life_->target.store(nullptr);
        if (framework_.joinable())
            std::terminate(); // no fabricated shutdown for an unreturned owner
    }
    void configure(std::shared_ptr<const native_ws::ElapsedSource> source,
                   const Limits &l) override {
        elapsed_ = std::move(source);
        limits_ = l;
    }
    // Kept for the IO0 Foundation relay, which lands with the producer slice.
    void bind_fatal(detail::FatalRelay relay) noexcept override { fatal_relay_ = relay; }
    void peer_connected(const void *id) override {
        std::lock_guard lock(mutex_);
        if (connecting_)
            connecting_->identity = id;
    }
    Result<StartAccepted> start(Ingress &ingress, std::unique_ptr<native_ws::Profile> profile,
                                StartupObserver startup) override {
        try {
            ingress_ = &ingress;
            profile_ = std::move(profile);
            startup_ = std::move(startup);
            auto *registration = drogon::bounded_http_sdk::Runtime::native_registration();
            if (!registration || !elapsed_)
                return Failure::unavailable;
            const auto &nl = registration->limits();
            peer_count_ = std::size_t(limits_.peers + nl.connections + nl.incomplete_upgrades);
            peers_ = std::make_unique<ConnectionRecord[]>(peer_count_);
            for (std::size_t i = 0; i < peer_count_; ++i)
                peers_[i].mail.resize(std::size_t(limits_.per_peer));
            wire_.resize(std::size_t(limits_.response_bytes + 1));
            bh::Limits limits{peer_count_,
                              limits_.peers,
                              nl.connections + nl.incomplete_upgrades,
                              std::min<std::uint64_t>(limits_.input_slots, peer_count_),
                              std::min<std::uint64_t>(limits_.response_slots, peer_count_),
                              limits_.envelope_bytes,
                              limits_.response_bytes,
                              5000,
                              5000,
                              5000,
                              5000,
                              10};
            const std::array<bh::Route, 4> routes{
                {{"POST", "/api-host/diagnostic", limits_.envelope_bytes, 0, 0},
                 {"POST", "/api-host/installation", limits_.envelope_bytes, 0, 0},
                 {"POST", "/api-host/credential", limits_.envelope_bytes, 0, 0},
                 {"POST", "/api-host/account", limits_.envelope_bytes, 0, 0}}};
            auto foundation =
                bh::create(limits, routes, {"/api-host/ws", opened, message, closed}, elapsed_,
                           {this, connected, request, ready, ended, removed, drained, nullptr});
            if (!std::holds_alternative<std::unique_ptr<bh::Foundation>>(foundation))
                return Failure::unavailable;
            foundation_ = std::move(std::get<std::unique_ptr<bh::Foundation>>(foundation));
            scratch_ = options_.scratch_parent + "/subathon-host-XXXXXX";
            if (!::mkdtemp(scratch_.data()))
                return Failure::unavailable;
            scratch_created_ = true;
            fs::create_directory(scratch_ + "/empty");
            fs::create_directory(scratch_ + "/upload");
            std::copy(options_.address.begin(), options_.address.end(), endpoints_.address.begin());
            std::copy(scratch_.begin(), scratch_.end(), endpoints_.scratch_root.begin());
            configure_app();
            framework_ = std::thread([this] {
                bool returned_normally = true;
                try {
                    drogon::app().run();
                } catch (...) {
                    returned_normally = false;
                }
                QMetaObject::invokeMethod(
                    &owner_,
                    [this, returned_normally] {
                        framework_.join();
                        if (!returned_normally) {
                            // A native-framework exception is a failed startup, never
                            // evidence that its partially created owners joined/drained.
                            startup_(Failure::unavailable);
                            return;
                        }
                        if (!drained_.load() || !ok(foundation_->finish()))
                            return;
                        foundation_.reset();
                        profile_.reset();
                        elapsed_.reset();
                        std::error_code error;
                        fs::remove_all(scratch_, error);
                        if (error)
                            return;
                        auto done = std::move(barrier_);
                        if (done)
                            done({true, true, true, true});
                    },
                    Qt::QueuedConnection);
            });
            framework_launched_ = true;
            return StartAccepted{};
        } catch (...) {
            // The Host consumes failure and calls stop on this creating thread.
            return Failure::unavailable;
        }
    }
    void close_peer(const void *id) override {
        std::lock_guard lock(mutex_);
        for (std::size_t i = 0; i < peer_count_; ++i) {
            auto &p = peers_[i];
            if (p.live && p.identity == id && p.wake) {
                (void)foundation_->post(*p.wake, bh::Intent::close);
                return;
            }
        }
    }
    void handoff_peer(const void *id, ResponseLease response) override {
        std::lock_guard lock(mutex_);
        for (std::size_t i = 0; i < peer_count_; ++i) {
            auto &p = peers_[i];
            if (!p.live || p.identity != id || !p.wake)
                continue;
            for (auto &slot : p.mail)
                if (!slot) {
                    slot.emplace(std::move(response));
                    (void)foundation_->post(*p.wake, bh::Intent::owner_ready);
                    return;
                }
            return;
        }
    }
    void handoff(ResponseLease) override { /* Real host uses the private issued-peer binding. */ }
    void stop(StopDeadline deadline, NetworkBarrierObserver barrier) override {
        if (stopping_.exchange(true))
            return;
        barrier_ = std::move(barrier);
        if (!framework_launched_) {
            // No run() means no sockets, IO callbacks or joins were created.
            // The existing foundation destructor verifies creator ownership and
            // discards the unstarted SDK binding (including listener records).
            foundation_.reset();
            profile_.reset();
            elapsed_.reset();
            peers_.reset();
            peer_count_ = 0;
            if (scratch_created_) {
                std::error_code error;
                fs::remove_all(scratch_, error);
                if (error)
                    return; // An incomplete cleanup is not a barrier.
                scratch_created_ = false;
            }
            life_->target.store(nullptr, std::memory_order_release);
            auto done = std::move(barrier_);
            done({true, true, true, true});
            return;
        }
        if (foundation_) {
            if (native_live_.load(std::memory_order_acquire)) {
                auto *loop = native_loop_.load(std::memory_order_acquire);
                loop->queueInLoop([this, deadline] { start_native_close(deadline); });
            } else
                (void)foundation_->stop({deadline.nanoseconds});
        } else {
            auto done = std::move(barrier_);
            done({true, true, true, true});
        }
    }
};
bool valid_parent(const std::string &path) {
    if (path.empty() || path.size() > 512 || path.find('\0') != std::string::npos ||
        path.front() != '/')
        return false;
    fs::path parsed(path);
    if (parsed.lexically_normal().string() != path)
        return false;
    fs::path prefix;
    for (const auto &part : parsed) {
        if (part == "." || part == "..")
            return false;
        prefix /= part;
        struct stat status{};
        if (::lstat(prefix.c_str(), &status) || !S_ISDIR(status.st_mode) || S_ISLNK(status.st_mode))
            return false;
    }
    struct stat status{};
    if (::stat(path.c_str(), &status))
        return false;
    return path == "/tmp" ? status.st_uid == 0 && (status.st_mode & S_ISVTX)
                          : status.st_uid == ::geteuid() && (status.st_mode & 0077) == 0;
}
} // namespace
Result<std::unique_ptr<Transport>> make_drogon_transport(DrogonOptions options,
                                                         ListenersObserver observer) {
    if (options.address.empty() || options.address.size() > 45 ||
        options.address.find('\0') != std::string::npos ||
        (options.port && options.port == options.http_port) ||
        !valid_parent(options.scratch_parent))
        return Failure::invalid_options;
    std::array<unsigned char, 16> bytes{};
    std::array<char, 46> canonical{};
    int family = options.address.find(':') == std::string::npos ? AF_INET : AF_INET6;
    if (::inet_pton(family, options.address.c_str(), bytes.data()) != 1 ||
        !::inet_ntop(family, bytes.data(), canonical.data(), canonical.size()))
        return Failure::invalid_options;
    options.address = canonical.data();
    return std::unique_ptr<Transport>(
        std::make_unique<DrogonTransport>(std::move(options), std::move(observer)));
}
} // namespace subathon::server::api_host
