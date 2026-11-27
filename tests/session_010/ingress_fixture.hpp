// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// SESSION-010 S3 rig: the actual #591 Foundation and capture lane on loopback, the
// S3 binding under test, and the real S2 policy owner run on the test thread.
// Include from exactly ONE translation unit per binary (it defines the crypto wrap).
#include "../account_capture/bounded_literals.hpp"
#include "../account_capture/native_fixture.hpp"
#include "session_ingress_candidate.hpp"

#include <vector>

namespace session_ingress_test {
namespace fx = account_capture_test;
namespace bh = subathon::server::bounded_http;
namespace ws = subathon::server::native_ws;
namespace sp = subathon::server::session_policy;
namespace eo = subathon::server::account_capture::testing;
namespace si = subathon::server::session_ingress_candidate;
using namespace std::literals;
using subathon::server::AccountTransportFailure;
using subathon::server::AccountTransportKind;
using subathon::server::AccountTransportPurpose;
using subathon::server::PreparedTransportSession;

// Linker-wrapped BLAKE2b calls: the only digest primitive of the unchanged supplier.
inline std::atomic<unsigned> crypto_calls{0};
} // namespace session_ingress_test
extern "C" {
int __real_crypto_generichash(unsigned char *, std::size_t, const unsigned char *,
                              unsigned long long, const unsigned char *, std::size_t);
int __wrap_crypto_generichash(unsigned char *out, std::size_t out_size, const unsigned char *in,
                              unsigned long long in_size, const unsigned char *key,
                              std::size_t key_size) {
    session_ingress_test::crypto_calls.fetch_add(1);
    return __real_crypto_generichash(out, out_size, in, in_size, key, key_size);
}
}

namespace session_ingress_test {
constexpr auto target = "/api/reference/v1/timer"sv;
// r2 line 103: BLAKE2b-256("subathon.account-session.v1" NUL, 32 zero octets).
constexpr std::array<std::uint8_t, 32> digest_z{
    0xa2, 0x26, 0xd5, 0x44, 0xff, 0x92, 0xf3, 0x24, 0x7d, 0xc7, 0x84, 0xe9, 0x49, 0xc8, 0x46, 0x99,
    0x9f, 0xde, 0x80, 0x1e, 0x45, 0x81, 0xc6, 0xbb, 0x21, 0x9f, 0xde, 0x07, 0x05, 0x91, 0xd3, 0xea};
// The exact single-hop vector the #888 stock edge adds: forwarding 87 + Via 16.
constexpr auto forwarding = "Via: 1.1 Caddy\r\nX-Forwarded-For: 127.0.0.1\r\n"
                            "X-Forwarded-Host: timer.example\r\nX-Forwarded-Proto: https\r\n"sv;
static_assert(forwarding.size() == 103);
constexpr auto bad = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"sv;
constexpr auto large_body =
    "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"sv;
constexpr auto large_head =
    "HTTP/1.1 431 Request Header Fields Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"sv;

// The manufactured downstream projection of a direct literal: the vector before the
// terminator. Order relative to earlier fields is not a trust rule (trace Q3).
inline std::string upstream(std::string_view direct) {
    std::string out(direct.substr(0, direct.size() - 2));
    return out.append(forwarding).append("\r\n");
}

// Admitted reference snapshot facts: R2 reference limits, the one GET route, the
// Foundation-local listener 1, loopback peer and the proxied single-hop mode.
inline sp::Facts reference_facts() {
    sp::Facts f;
    f.installation = 1;
    f.origin = "https://timer.example";
    f.routes = {{std::string(target), "GET", AccountTransportPurpose::protected_access}};
    f.peers = {"127.0.0.1"};
    f.limits = {64, 4096, 4096, 64, 1024, 4096, 4096};
    f.foundation = f.native_abi = f.native_artifact = f.profile = 1;
    f.listener = f.edge_artifact = f.edge_config = f.logging = 1;
    f.isolation = f.forwarding_grammar = f.normalization = 1;
    f.configuration = "configuration";
    f.address = "127.0.0.1";
    f.mode = sp::Facts::Mode::proxied;
    f.forwarded_metadata = true;
    return f;
}

// Synthetic admitted registry (reading 893.3): identity 1 resolves to the running
// artifact/profile SHA-256, 3 to a hash nothing runs; 2 is unknown.
inline si::AdmittedRegistry admitted() {
    return {{{1, fx::selected_identity::artifact}, {3, si::Sha256{}}},
            {{1, fx::selected_identity::profile}, {3, si::Sha256{}}}};
}

class Readiness final : public sp::ReadinessProducer {
  public:
    sp::ReadinessState query(const sp::Readiness &) noexcept override {
        return sp::ReadinessState::live;
    }
};

class Ingress {
  public:
    // `fail_target_wipe`: #591's observer refuses the target wipe (release_raw only).
    explicit Ingress(const sp::Facts &facts = reference_facts(), bool publish_first = true,
                     bool fail_target_wipe = false)
        : expect_wipe_failure_(fail_target_wipe) {
        static auto *const logs = new fx::LogObservation; // process-retained sink target
        logs_ = logs;
        fx::bind_checked_log(*logs);
        scratch_.acquire();
        scratch_.initial_directories();
        if (ws::linked_backend() != ws::Backend::admitted_corrected)
            throw std::runtime_error("FIXTURE: corrected native backend");
        auto profile = ws::create_profile(
            {1, 1, 64, 64, 16, 2, 128, 256, 256, 64, 256, 254, 8, 50, 50, 100, 50}, clock_);
        native_ = take<std::unique_ptr<ws::Profile>>(profile);
        auto created = bh::create({2, 1, 1, 1, 1, 8192, 4, 5000, 5000, 5000, 5000, 10}, routes_,
                                  {"/foundation/ws", nullptr, nullptr, nullptr}, clock_,
                                  {this, connected, request, ready, ended, removed, drained, read});
        foundation_ = take<std::unique_ptr<bh::Foundation>>(created);
        binding_ = si::Binding::install(*foundation_, routes_, admitted());
        if (!binding_)
            throw std::runtime_error("FIXTURE: binding installation refused");
        if (fail_target_wipe)
            require(eo::install_erasure_observer(*foundation_, {this, refuse_target}));
        if (publish_first)
            binding_->publish(next(facts));
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
            .setDocumentRoot((std::filesystem::path(scratch_.path()) / "empty").string())
            .setUploadPath((std::filesystem::path(scratch_.path()) / "upload").string())
            .setClientMaxBodySize(4096)
            .setClientMaxMemoryBodySize(4096);
        app.enableReusePort(true);
        app.setBeforeListenSockOptCallback([](int fd) {
            int zero = 0, reuse = 0, address = 0;
            socklen_t size = sizeof(int);
            if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &zero, sizeof(zero)) ||
                ::getsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse, &size) || reuse ||
                ::getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &address, &size) || address != 1)
                fx::terminal_fixture();
        });
        bh::Endpoints bind{};
        std::memcpy(bind.address.data(), "127.0.0.1", 10);
        require(bh::register_listeners(*foundation_, app, bind));
        app.registerBeginningAdvice([this, &app] {
            app.getLoop()->queueInLoop([this, &app] {
                const auto endpoints = app.getListeners();
                const bool valid = endpoints.size() == 2 && endpoints[0].toPort() &&
                                   endpoints[1].toPort() != endpoints[0].toPort();
                app.getIOLoop(0)->queueInLoop(
                    [this, valid, http = endpoints[0].toPort(), native = endpoints[1].toPort()] {
                        update([&] {
                            http_port = valid ? http : 0;
                            native_port = valid ? native : 0;
                            failed_ |= !valid;
                        });
                    });
            });
        });
        framework_ = std::thread([this, &app] {
            app.run();
            update([&] { returned_ = true; });
        });
        if (!wait([&] { return http_port != 0; }))
            throw std::runtime_error("FIXTURE: listener readiness");
    }
    ~Ingress() { stop(); }

    // Owner thread (this test thread): one real S2 publication; returns its pin. A
    // drain-requiring change names the synthetic lifecycle owner's channels (trace Q14).
    sp::Pin next(const sp::Facts &facts, std::vector<sp::Identity> drained = {}) {
        const auto attempt = policy_->suspend();
        const auto candidate = attempt ? policy_->validate(*attempt, facts, drained) : std::nullopt;
        auto pin = candidate ? policy_->publish(candidate->attempt,
                                                {*candidate, ++ack_, facts.profile, facts.listener,
                                                 facts.configuration, facts.address},
                                                drained.empty()
                                                    ? std::nullopt
                                                    : std::optional(sp::Drain{*candidate, drained}))
                             : nullptr;
        if (!pin)
            throw std::runtime_error("FIXTURE: S2 policy publication");
        return pin;
    }
    si::Binding &binding() { return *binding_; }
    fx::Socket &client(unsigned i) { return clients_.at(i); }
    void connect(unsigned i) { (void)clients_.at(i).connect(http_port); }
    // Read one connection through EOF (a native refusal closes it).
    std::string through_eof(unsigned i) {
        std::array<char, 4096> bytes{};
        return {bytes.data(), clients_.at(i).read(bytes, true)};
    }

    // Bounded wait for IO0 publications; false on timeout or a fixture fault.
    template <class Predicate> bool wait(Predicate predicate) {
        std::unique_lock lock(mutex_);
        return changed_.wait_for(lock, 7s, [&] { return predicate() || failed_; }) && !failed_;
    }
    template <class F> void update(F f) {
        {
            std::lock_guard lock(mutex_);
            f();
        }
        changed_.notify_all();
    }

    // Stop, drain, join and finish; then drain and scan every client wire.
    void stop() noexcept {
        if (!framework_.joinable())
            return;
        (void)foundation_->stop({clock_->ns.load() + 5'000'000'000ULL});
        if (!wait([&] { return returned_ && drained_; }))
            fx::terminal_fixture();
        framework_.join();
        for (auto &peer : peers_)
            peer.reset();
        const auto finished = foundation_->finish();
        if (expect_wipe_failure_) {
            const auto *error = std::get_if<bh::Error>(&finished);
            quarantined = error && *error == bh::Error::wipe_failed;
            // #591 precedent: the quarantined core stays charged until process exit.
            (void)binding_.release();
            (void)foundation_.release();
        } else if (!std::holds_alternative<bh::Done>(finished))
            fx::terminal_fixture();
        binding_.reset();
        foundation_.reset();
        for (auto &c : clients_) {
            if (c.get() >= 0) {
                std::array<char, 65664> rest{};
                (void)c.read(rest, true);
                c.reset();
            }
            wire_clean &= c.finish_wire();
        }
        logs_clean = logs_->clean();
        if (!scratch_.remove(true))
            fx::terminal_fixture();
    }

    // IO0-published observations; read under wait() or after stop().
    std::vector<si::Handoff> handoffs;
    unsigned requests = 0, removed_count = 0;
    std::uint64_t read_bytes = 0;
    std::uint16_t http_port = 0, native_port = 0;
    std::function<void()> during_request; // IO0, once, before the first reply
    bool wire_clean = true, logs_clean = false, quarantined = false;
    // How the request callback ends: commit its reply, cancel it, or stop first (I20).
    enum class Ending { commit, cancel, stop } ending = Ending::commit;

  private:
    fx::Deadline watchdog_;
    fx::OwnedScratch scratch_;
    std::shared_ptr<fx::Clock> clock_ = std::make_shared<fx::Clock>();
    std::shared_ptr<Readiness> readiness_ = std::make_shared<Readiness>();
    std::unique_ptr<sp::Policy> policy_ = sp::Policy::construct({11, 1}, {4, 4, 3}, readiness_);
    sp::Identity ack_ = 20;
    std::array<bh::Route, 1> routes_{bh::Route{"GET", target, 4096, 4096, 4096}};
    std::unique_ptr<ws::Profile> native_;
    std::unique_ptr<bh::Foundation> foundation_;
    std::unique_ptr<si::Binding> binding_;
    std::array<std::optional<bh::Peer>, 2> peers_;
    std::array<fx::Socket, 2> clients_;
    unsigned connected_ = 0;
    std::thread framework_;
    fx::LogObservation *logs_ = nullptr;
    std::mutex mutex_;
    std::condition_variable changed_;
    bool failed_ = false, returned_ = false, drained_ = false;
    bool expect_wipe_failure_;

    template <class T, class Result> static T take(Result &result) {
        if (auto *value = std::get_if<T>(&result))
            return std::move(*value);
        throw std::runtime_error("FIXTURE: native control result");
    }
    static void require(bh::Result<bh::Done> result) { (void)take<bh::Done>(result); }
    static Ingress &self(void *owner) { return *static_cast<Ingress *>(owner); }

    static void connected(void *owner, bh::Peer peer) {
        auto &s = self(owner);
        if (s.connected_ == s.peers_.size() ||
            !std::holds_alternative<bh::Done>(peer.context(&s.peers_[s.connected_])) ||
            !std::holds_alternative<bh::Done>(s.foundation_->admission(peer, bh::Pause::admitted)))
            return s.update([&] { s.failed_ = true; });
        s.peers_[s.connected_++].emplace(std::move(peer));
    }
    static void request(void *owner, void *, bh::Request request) {
        auto &s = self(owner);
        if (auto action = std::exchange(s.during_request, {}))
            action();
        if (s.ending == Ending::stop) // from an ordinary thread, as the host issues it
            std::thread([&s] {
                (void)s.foundation_->stop({s.clock_->ns.load() + 5'000'000'000ULL});
            }).join();
        auto reserved = request.reserve_reply({200, 4, false});
        bool replied = false;
        if (auto *reply = std::get_if<bh::Reply>(&reserved); reply && s.ending == Ending::cancel) {
            reply->cancel();
            replied = true;
        } else if (reply) {
            replied = std::holds_alternative<bh::Done>(
                          reply->fill(nullptr,
                                      [](void *, std::span<std::byte> bytes) -> bh::U64 {
                                          if (bytes.size() != 4)
                                              return 0;
                                          std::memcpy(bytes.data(), "diag", 4);
                                          return 4;
                                      })) &&
                      std::holds_alternative<bh::Done>(reply->commit());
        }
        auto handoff = s.binding_->release(request);
        s.update([&] {
            s.failed_ |= !replied && s.ending != Ending::stop;
            if (handoff)
                s.handoffs.push_back(std::move(*handoff));
            ++s.requests;
        });
    }
    static bool refuse_target(void *, eo::ErasureEvent event, std::span<const std::byte>) noexcept {
        return event.region != eo::Region::target || event.phase != eo::Phase::before_sodium;
    }
    static void ready(void *, void *) {}
    static void ended(void *, void *, bh::End) {}
    static void removed(void *owner, void *context) {
        auto &s = self(owner);
        static_cast<std::optional<bh::Peer> *>(context)->reset();
        s.update([&] { ++s.removed_count; });
    }
    static void drained(void *owner) {
        self(owner).update([&] { self(owner).drained_ = true; });
        drogon::app().quit();
    }
    static void read(void *owner, void *, bh::ReadSample sample) {
        self(owner).update([&] { self(owner).read_bytes = sample.bytes_read; });
    }
};

// Typed accessors for assertions; a wrong alternative yields nullptr.
inline const PreparedTransportSession *prepared(const si::Handoff &h) {
    auto *result = std::get_if<subathon::server::AccountTransportResult>(&h.outcome);
    return result ? std::get_if<PreparedTransportSession>(result) : nullptr;
}
inline std::optional<AccountTransportFailure> failure(const si::Handoff &h) {
    auto *result = std::get_if<subathon::server::AccountTransportResult>(&h.outcome);
    auto *value = result ? std::get_if<AccountTransportFailure>(result) : nullptr;
    return value ? std::optional(*value) : std::nullopt;
}
} // namespace session_ingress_test
