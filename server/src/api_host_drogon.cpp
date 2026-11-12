// SPDX-License-Identifier: GPL-3.0-or-later
#include "api_host_private.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QObject>
#include <QTemporaryDir>
#include <QTimer>
#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <atomic>
#include <cstring>
#include <drogon/WebSocketConnection.h>
#include <drogon/WebSocketController.h>
#include <drogon/drogon.h>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <subathon/server/api_host_drogon.hpp>
#include <thread>
#include <trantor/net/EventLoop.h>
#include <trantor/net/TcpConnection.h>

namespace subathon::server::api_host {
namespace {
using Callback = std::function<void(const drogon::HttpResponsePtr &)>;
using Tcp = trantor::TcpConnectionPtr;
using Ws = native_ws::Connection;
class DrogonTransport;
class Controller final : public drogon::WebSocketController<Controller, false> {
    DrogonTransport &owner_;

  public:
    explicit Controller(DrogonTransport &o) : owner_(o) {}
    void handleNewConnection(const drogon::HttpRequestPtr &, const Ws &) override;
    void handleNewMessage(const Ws &, std::string &&,
                          const drogon::WebSocketMessageType &) override;
    void handleConnectionClosed(const Ws &) override;
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/api-host/ws");
    WS_PATH_LIST_END
};
// Fixed transport tables are allocated once from the trusted host profile. Only
// the one IO loop accesses peers/pending HTTP records. Qt publishes handoffs into
// a finite mailbox, with a coalesced loop wake instead of per-reply closures.
class DrogonTransport final : public NetworkTransport {
    struct Pending {
        bool used = false;
        std::uint64_t serial = 0, deadline = 0;
        Callback callback;
    };
    struct Peer {
        Tcp tcp;
        Ws ws;
        std::optional<PeerLease> lease;
        std::uint64_t serial = 0;
        std::unique_ptr<Pending[]> requests;
        std::array<CopyCredit, 3> http_output;
        bool responding = false;
        std::optional<std::uint64_t> output_deadline;
    };
    struct Mail {
        std::optional<ResponseLease> lease;
        std::uint64_t peer = 0, command = 0;
    };
    static_assert(sizeof(Pending) <= 128);
    static_assert(sizeof(Peer) <= 512);
    static_assert(sizeof(Mail) <= 128);
    DrogonOptions options_;
    Limits limits_;
    Ingress *ingress_ = nullptr;
    std::unique_ptr<native_ws::Profile> profile_;
    std::shared_ptr<const native_ws::ElapsedSource> elapsed_;
    std::shared_ptr<ResponseBudget> budget_;
    std::unique_ptr<QTemporaryDir> upload_directory_;
    std::string upload_path_;
    std::optional<std::uint64_t> last_;
    StartupObserver ready_;
    NetworkBarrierObserver barrier_;
    QObject owner_context_;
    QTimer finish_timer_;
    std::atomic<bool> framework_done_{false}, framework_clean_{false};
    std::thread framework_;
    std::unique_ptr<Peer[]> peers_;
    std::unique_ptr<Mail[]> mail_;
    std::mutex mutex_;
    trantor::EventLoop *io_ = nullptr;
    bool mail_wake_ = false, started_ = false, joined_ = false;
    bool closing_ = false, quitting_ = false;
    std::atomic<bool> stop_requested_{false};
    std::atomic<std::uint64_t> stop_deadline_{0};
    trantor::TimerId tick_ = trantor::InvalidTimerId;
    bool time_ok(std::uint64_t *value = nullptr) {
        auto sample = elapsed_->sample();
        auto *now = std::get_if<native_ws::ElapsedInstant>(&sample);
        if (!now || (last_ && now->nanoseconds < *last_))
            return false;
        last_ = now->nanoseconds;
        if (value)
            *value = now->nanoseconds;
        return true;
    }
    Peer *find(const Tcp &tcp) {
        for (std::uint64_t i = 0; i < limits_.peers; ++i)
            if (peers_[i].tcp == tcp)
                return &peers_[i];
        return nullptr;
    }
    Peer *find(const Ws &ws) {
        for (std::uint64_t i = 0; i < limits_.peers; ++i)
            if (peers_[i].ws == ws)
                return &peers_[i];
        return nullptr;
    }
    Peer *find(std::uint64_t serial) {
        for (std::uint64_t i = 0; i < limits_.peers; ++i)
            if (peers_[i].tcp && peers_[i].serial == serial)
                return &peers_[i];
        return nullptr;
    }
    void revoke(Peer &p) {
        if (p.lease)
            ingress_->disconnected(*p.lease);
        for (std::uint64_t i = 0; i < limits_.per_peer; ++i) {
            p.requests[i].callback = {};
            p.requests[i].used = false;
        }
        p.lease.reset();
    }
    void force(Peer &p) {
        revoke(p);
        if (p.tcp)
            p.tcp->forceClose();
    }
    void resource_close(Peer &p) {
        if (!p.ws) {
            force(p);
            return;
        }
        native_ws::CloseWithStatus status{};
        status.code = 1009;
        const auto closed = profile_->start_close(p.ws, status);
        // Profile controls use their separately reserved fixed output storage.
        // Keep earlier accepted association truth; only their transport lease may
        // subsequently be revoked by the actual Close/disconnect.
        if (!std::holds_alternative<native_ws::CloseDisposition>(closed))
            force(p);
    }
    void connection(const Tcp &tcp) {
        if (!tcp->connected()) {
            if (auto *p = find(tcp)) {
                revoke(*p);
                p->ws.reset();
                p->tcp.reset();
                p->serial = 0;
                p->responding = false;
                p->output_deadline.reset();
                for (auto &credit : p->http_output)
                    credit.reset();
            }
            return;
        }
        if (stop_requested_ || !time_ok()) {
            tcp->forceClose();
            return;
        }
        for (std::uint64_t i = 0; i < limits_.peers; ++i) {
            auto &p = peers_[i];
            if (p.tcp)
                continue;
            auto admitted = ingress_->connected();
            if (!std::holds_alternative<PeerLease>(admitted)) {
                tcp->forceClose();
                return;
            }
            p.tcp = tcp;
            p.serial = connected_peer_serial;
            p.lease.emplace(std::get<PeerLease>(std::move(admitted)));
            return;
        }
        tcp->forceClose();
    }
    static int hex(unsigned char c) {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        return -1;
    }
    void http(const drogon::HttpRequestPtr &request, Callback callback, RouteId route) {
        auto tcp = request->getConnectionPtr().lock();
        auto *p = find(tcp);
        if (!p || !p->lease || p->responding || stop_requested_ || !time_ok()) {
            if (tcp)
                tcp->forceClose();
            return;
        }
        const auto &header = request->getHeader("x-host-correlation");
        if (header.empty() || header.size() > 512 || header.size() % 2) {
            force(*p);
            return;
        }
        std::array<std::byte, 256> correlation{};
        for (std::size_t i = 0; i < header.size(); i += 2) {
            int a = hex(header[i]), b = hex(header[i + 1]);
            if (a < 0 || b < 0) {
                force(*p);
                return;
            }
            correlation[i / 2] = std::byte(a * 16 + b);
        }
        Pending *slot = nullptr;
        for (std::uint64_t i = 0; i < limits_.per_peer; ++i) {
            auto &r = p->requests[i];
            if (!r.used && !slot)
                slot = &r;
        }
        if (!slot) {
            force(*p);
            return;
        }
        auto body = request->body();
        auto result =
            ingress_->try_submit(*p->lease, {correlation.data(), header.size() / 2}, route,
                                 {reinterpret_cast<const std::byte *>(body.data()), body.size()});
        if (!std::holds_alternative<Admitted>(result)) {
            force(*p);
            return;
        }
        std::uint64_t now;
        const auto duration = limits_.queue_ms * 1000000;
        if (!time_ok(&now) || now > UINT64_MAX - duration) {
            force(*p);
            return;
        }
        slot->deadline = now + duration;
        slot->used = true;
        slot->serial = submitted_command_serial;
        slot->callback = std::move(callback);
    }
    void drain_mail() {
        for (;;) {
            std::optional<ResponseLease> lease;
            std::uint64_t serial = 0, command = 0;
            {
                std::lock_guard lock(mutex_);
                for (std::uint64_t i = 0; i < limits_.associations; ++i) {
                    if (!mail_[i].lease)
                        continue;
                    lease = std::move(mail_[i].lease);
                    mail_[i].lease.reset();
                    serial = mail_[i].peer;
                    command = mail_[i].command;
                    break;
                }
                if (!lease) {
                    mail_wake_ = false;
                    break;
                }
            }
            auto *p = find(serial);
            if (!p || stop_requested_)
                continue;
            if (!time_ok()) {
                force(*p);
                continue;
            }
            auto view = lease->take();
            auto *v = std::get_if<CommandView>(&view);
            if (!v) {
                force(*p);
                continue;
            }
            if (p->ws) {
                if (v->correlation.size() != 1 || v->bytes.size() > limits_.response_bytes) {
                    force(*p);
                    continue;
                }
                // No startup payload reservation: equal single/pool capacities
                // remain valid. Each actual projection competes for another copy.
                CopyCredit credit;
                if (!credit.reserve(budget_, v->bytes.size() + 1)) {
                    budget_->resource_refused();
                    resource_close(*p);
                    continue;
                }
                try {
                    auto output = std::make_unique<std::byte[]>(v->bytes.size() + 1);
                    output[0] = v->correlation[0];
                    std::copy(v->bytes.begin(), v->bytes.end(), output.get() + 1);
                    auto sent = profile_->try_send(p->ws, native_ws::MessageKind::binary,
                                                   {output.get(), v->bytes.size() + 1});
                    if (!std::holds_alternative<native_ws::Admitted>(sent))
                        resource_close(*p);
                } catch (...) {
                    budget_->resource_refused();
                    resource_close(*p);
                }
            } else {
                Pending *slot = nullptr;
                for (std::uint64_t i = 0; i < limits_.per_peer; ++i) {
                    auto &r = p->requests[i];
                    if (r.used && r.serial == command) {
                        slot = &r;
                        break;
                    }
                }
                if (!slot) {
                    force(*p);
                    continue;
                }
                if (p->responding) {
                    force(*p);
                    continue;
                }
                const auto body_capacity = std::max<std::uint64_t>(15, v->bytes.size());
                // Selected HTTP response/body serialization and TCP MsgBuffer each
                // retain a separate copy. Reserve conservative complete capacities
                // before invoking Drogon; one response closes this HTTP connection.
                if (!p->http_output[0].reserve(budget_, body_capacity) ||
                    !p->http_output[1].reserve(budget_, 2 * (body_capacity + 1024)) ||
                    !p->http_output[2].reserve(budget_, 2 * (body_capacity + 1024))) {
                    budget_->resource_refused();
                    force(*p);
                    continue;
                }
                p->responding = true;
                std::uint64_t now;
                if (!time_ok(&now) || now > UINT64_MAX - limits_.handoff_ms * 1000000) {
                    force(*p);
                    continue;
                }
                p->output_deadline = now + limits_.handoff_ms * 1000000;
                auto callback = std::move(slot->callback);
                slot->used = false;
                try {
                    auto reply = drogon::HttpResponse::newHttpResponse();
                    reply->setContentTypeCode(drogon::CT_APPLICATION_OCTET_STREAM);
                    reply->setCloseConnection(true);
                    reply->setBody(std::string(reinterpret_cast<const char *>(v->bytes.data()),
                                               v->bytes.size()));
                    callback(reply);
                } catch (...) {
                    budget_->resource_refused();
                    force(*p);
                }
            }
        }
    }
    void begin_stop() {
        if (closing_)
            return;
        closing_ = true;
        // Refuse new/partial HTTP immediately. Established WS gets its one bounded
        // native Close; the original host drain deadline is never refreshed.
        drain_mail();
        for (std::uint64_t i = 0; i < limits_.peers; ++i) {
            auto &p = peers_[i];
            if (!p.tcp)
                continue;
            revoke(p);
            if (p.ws) {
                auto close = profile_->start_close(p.ws, native_ws::CloseWithoutStatus{});
                if (!std::holds_alternative<native_ws::CloseDisposition>(close))
                    p.tcp->forceClose();
            } else
                p.tcp->forceClose();
        }
        turn();
    }
    void turn() {
        if (quitting_)
            return;
        if (!stop_requested_.load(std::memory_order_acquire))
            drain_mail();
        std::uint64_t now = 0;
        const bool valid = time_ok(&now);
        if (stop_requested_ && !closing_) {
            begin_stop();
            return;
        }
        bool any = false;
        for (std::uint64_t i = 0; i < limits_.peers; ++i) {
            auto &p = peers_[i];
            if (!p.tcp)
                continue;
            any = true;
            bool expired_request = false;
            if (valid && !closing_)
                for (std::uint64_t n = 0; n < limits_.per_peer; ++n)
                    if (p.requests[n].used && now >= p.requests[n].deadline)
                        expired_request = true;
            if (expired_request || !valid || (closing_ && now >= stop_deadline_) ||
                (!closing_ && p.lease && !budget_->peer_live(p.serial)) ||
                (p.output_deadline && now >= *p.output_deadline))
                force(p);
            else if (p.ws)
                (void)profile_->recheck_deadlines(p.ws);
        }
        if (closing_ && !any) {
            const auto snapshot = profile_->inspect_global();
            const auto *native = std::get_if<native_ws::GlobalSnapshot>(&snapshot);
            if (!native || native->owners_live_or_retiring || native->upgrade_owners ||
                native->callback_leases)
                return;
            quitting_ = true;
            if (tick_ != trantor::InvalidTimerId) {
                io_->invalidateTimer(tick_);
                tick_ = trantor::InvalidTimerId;
            }
            // A final queued IO turn lets TCP close callbacks and native retirement
            // run before framework quit removes listeners and joins the IO pool.
            io_->queueInLoop([this] {
                for (std::uint64_t i = 0; i < limits_.peers; ++i) {
                    revoke(peers_[i]);
                    peers_[i].ws.reset();
                }
                profile_.reset();
                drogon::app().quit();
            });
        }
    }

  public:
    explicit DrogonTransport(DrogonOptions options) : options_(std::move(options)) {
        QObject::connect(&finish_timer_, &QTimer::timeout, &owner_context_, [this] {
            if (!framework_done_.load(std::memory_order_acquire) || joined_)
                return;
            framework_.join();
            joined_ = true;
            finish_timer_.stop();
            if (!stop_requested_.load(std::memory_order_acquire))
                ready_(Failure::unavailable);
            if (barrier_) {
                auto done = std::move(barrier_);
                const bool clean = framework_clean_.load(std::memory_order_acquire);
                done({clean, clean, true, clean});
            }
        });
    }
    ~DrogonTransport() override {
        if (framework_.joinable()) {
            // Host must retain this owner through the explicit joined acknowledgment.
            if (!joined_)
                std::terminate();
            framework_.join();
        }
    }
    void configure(const Limits &limits, std::shared_ptr<const native_ws::ElapsedSource> elapsed,
                   std::shared_ptr<ResponseBudget> budget, std::string_view store_path) override {
        limits_ = limits;
        elapsed_ = std::move(elapsed);
        budget_ = std::move(budget);
        peers_ = std::make_unique<Peer[]>(limits_.peers);
        for (std::uint64_t i = 0; i < limits_.peers; ++i)
            peers_[i].requests = std::make_unique<Pending[]>(limits_.per_peer);
        mail_ = std::make_unique<Mail[]>(limits_.associations);
        const auto parent =
            QFileInfo(QString::fromUtf8(store_path.data(), qsizetype(store_path.size())))
                .absolutePath();
        upload_directory_ = std::make_unique<QTemporaryDir>(parent + "/.api-host-upload-XXXXXX");
        if (!upload_directory_->isValid())
            throw std::runtime_error("Host upload directory unavailable");
        upload_path_ = upload_directory_->path().toStdString();
    }
    Result<StartAccepted> start(Ingress &ingress, std::unique_ptr<native_ws::Profile> profile,
                                StartupObserver observer) override {
        if (started_)
            return Failure::already_started;
        if (!elapsed_ || !profile || !observer)
            return Failure::invalid_options;
        started_ = true;
        ingress_ = &ingress;
        profile_ = std::move(profile);
        ready_ = std::move(observer);
        finish_timer_.start(10);
        try {
            framework_ = std::thread([this] {
                try {
                    auto &app = drogon::app();
                    app.setLogLevel(trantor::Logger::kFatal)
                        .setUploadPath(upload_path_)
                        .disableSigtermHandling()
                        .enableGzip(false)
                        .enableBrotli(false)
                        .enableDateHeader(false)
                        .enableServerHeader(false)
                        .setPipeliningRequestsNumber(limits_.per_peer)
                        .setThreadNum(1)
                        .setIdleConnectionTimeout(0)
                        .setMaxConnectionNum(limits_.peers)
                        .setClientMaxBodySize(limits_.envelope_bytes)
                        .setClientMaxMemoryBodySize(limits_.envelope_bytes);
                    // B1 remains unresolved: this admitted SDK listener refuses HTTP.
                    // No ordinary listener or alternate routing is introduced here.
                    app.addListener(options_.address, options_.port, false, "", "", false, {},
                                    true);
                    app.setConnectionCallback([this](const Tcp &tcp) { connection(tcp); });
                    for (auto [path, route] : {std::pair{"diagnostic", RouteId::diagnostic},
                                               {"installation", RouteId::installation},
                                               {"credential", RouteId::credential},
                                               {"account", RouteId::account}}) {
                        app.registerHandler(
                            std::string("/api-host/") + path,
                            [this, route](const drogon::HttpRequestPtr &request, Callback &&cb) {
                                http(request, std::move(cb), route);
                            },
                            {drogon::Post});
                    }
                    app.registerController(std::make_shared<Controller>(*this));
                    app.registerBeginningAdvice([this] {
                        // Beginning advice itself precedes the actual listen turn.
                        drogon::app().getLoop()->queueInLoop([this] {
                            auto listeners = drogon::app().getListeners();
                            if (listeners.size() != 1 || listeners.front().toPort() == 0) {
                                ready_(Failure::unavailable);
                                drogon::app().quit();
                                return;
                            }
                            auto *loop = drogon::app().getIOLoop(0);
                            loop->queueInLoop([this, loop] {
                                {
                                    std::lock_guard lock(mutex_);
                                    io_ = loop;
                                }
                                tick_ = loop->runEvery(0.01, [this] { turn(); });
                                if (stop_requested_)
                                    begin_stop();
                                else
                                    ready_(StartAccepted{});
                            });
                        });
                    });
                    app.run(); // quit joins the selected IO pool before returning
                    framework_clean_.store(true, std::memory_order_release);
                } catch (...) {
                    // A partial SDK startup/run exception has no public cleanup
                    // proof. Report failure and retain dependencies; do not forge
                    // an IO/socket barrier or destruct an unknown framework state.
                    try {
                        ready_(Failure::resource_limit);
                    } catch (...) {
                    }
                }
                // No allocation or Qt invocation on the finishing framework
                // thread; the prearmed Qt timer observes actual run completion.
                framework_done_.store(true, std::memory_order_release);
            });
        } catch (...) {
            started_ = false;
            finish_timer_.stop();
            return Failure::resource_limit;
        }
        return StartAccepted{};
    }
    void handoff(ResponseLease lease) override {
        lease.reset();
    } // only private routed handoff is supported
    void handoff_to(std::uint64_t peer, std::uint64_t command, ResponseLease lease) override {
        std::lock_guard lock(mutex_);
        if (!io_ || stop_requested_)
            return;
        for (std::uint64_t i = 0; i < limits_.associations; ++i) {
            auto &slot = mail_[i];
            if (slot.lease)
                continue;
            slot.peer = peer;
            slot.command = command;
            slot.lease.emplace(std::move(lease));
            if (!mail_wake_) {
                mail_wake_ = true;
                try {
                    io_->queueInLoop([this] { drain_mail(); });
                } catch (...) {
                    mail_wake_ = false;
                } // fixed IO tick retains/drains the mailbox
            }
            return;
        }
        // At most one lease per live association. Full mailbox refuses by dropping
        // the bounded lease, never retaining an overflow callback or response.
    }
    void stop(StopDeadline deadline, NetworkBarrierObserver done) override {
        std::lock_guard lock(mutex_);
        if (stop_requested_.load(std::memory_order_relaxed))
            return;
        stop_deadline_.store(deadline.nanoseconds, std::memory_order_relaxed);
        barrier_ = std::move(done);
        // IO must acquire the immutable deadline before observing stop.
        stop_requested_.store(true, std::memory_order_release);
        if (io_) {
            try {
                io_->queueInLoop([this] { begin_stop(); });
            } catch (...) {
            } // fixed IO tick consumes the published stop/deadline
        } else if (!framework_.joinable()) {
            profile_.reset();
            joined_ = true;
            auto complete = std::move(barrier_);
            const bool clean = !started_ || framework_clean_.load(std::memory_order_acquire);
            complete({clean, clean, true, clean});
        }
    }
    void opened(const drogon::HttpRequestPtr &request, const Ws &ws) {
        auto *p = find(request->getConnectionPtr().lock());
        if (!p || stop_requested_ || !time_ok()) {
            ws->forceClose();
            return;
        }
        p->ws = ws;
        ws->disablePing();
    }
    void message(const Ws &ws, const drogon::WebSocketMessageType &type) {
        auto *p = find(ws);
        if (!p || !p->lease || stop_requested_ || !time_ok()) {
            if (p)
                force(*p);
            return;
        }
        if (type != drogon::WebSocketMessageType::Binary)
            return;
        auto accepted = profile_->accept_callback(ws);
        auto *lease = std::get_if<native_ws::MessageLease>(&accepted.value);
        if (!lease) {
            force(*p);
            return;
        }
        const auto message = lease->view();
        if (!message || message->bytes.size() < 2 ||
            std::to_integer<unsigned>(message->bytes[1]) > 3) {
            force(*p);
            return;
        }
        auto result =
            ingress_->try_submit(*p->lease, message->bytes.first(1),
                                 static_cast<RouteId>(std::to_integer<unsigned>(message->bytes[1])),
                                 message->bytes.subspan(2));
        if (const auto *f = std::get_if<Failure>(&result)) {
            if (*f == Failure::resource_limit)
                resource_close(*p);
            else
                force(*p);
        }
    }
    void closed(const Ws &ws) {
        if (auto *p = find(ws)) {
            revoke(*p);
            p->ws.reset();
        }
    }
};
void Controller::handleNewConnection(const drogon::HttpRequestPtr &r, const Ws &w) {
    owner_.opened(r, w);
}
void Controller::handleNewMessage(const Ws &w, std::string &&,
                                  const drogon::WebSocketMessageType &t) {
    owner_.message(w, t);
}
void Controller::handleConnectionClosed(const Ws &w) { owner_.closed(w); }
} // namespace
Result<std::unique_ptr<Transport>> make_drogon_transport(DrogonOptions options) {
    if (options.address.empty() || options.address.size() > 255 ||
        options.address.find('\0') != std::string::npos)
        return Failure::invalid_options;
    in_addr ipv4{};
    in6_addr ipv6{};
    if (::inet_pton(AF_INET, options.address.c_str(), &ipv4) != 1 &&
        ::inet_pton(AF_INET6, options.address.c_str(), &ipv6) != 1)
        return Failure::invalid_options;
    try {
        return std::unique_ptr<Transport>(std::make_unique<DrogonTransport>(std::move(options)));
    } catch (...) {
        return Failure::resource_limit;
    }
}
} // namespace subathon::server::api_host
