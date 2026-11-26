// SPDX-License-Identifier: GPL-3.0-or-later
#include "../account_capture/native_fixture.hpp"

#include <cinttypes>
#include <cstdio>

namespace {
namespace fixture = account_capture_test;
namespace ac = subathon::server::account_capture;
namespace bh = subathon::server::bounded_http;
namespace ws = subathon::server::native_ws;
using namespace std::chrono_literals;

constexpr std::string_view loopback = "127.0.0.1";
constexpr std::string_view target = "/api/reference/v1/timer";

// The pinned Go writer emits Host separately and sorts other header names.
// Exact cross-name order here describes these selected controls, not a policy rule.
constexpr fixture::Field via{"Via", "1.1 Caddy"};
constexpr fixture::Field forwarded_for{"X-Forwarded-For", loopback};
constexpr fixture::Field forwarded_host{"X-Forwarded-Host", "timer.example"};
constexpr fixture::Field forwarded_proto{"X-Forwarded-Proto", "https"};
constexpr std::array desktop{
    fixture::fields_desktop[0],
    fixture::fields_desktop[1],
    via,
    forwarded_for,
    forwarded_host,
    forwarded_proto,
};
constexpr std::array browser{
    fixture::fields_desktop[0],
    fixture::Field{"Cookie", fixture::cookie},
    via,
    forwarded_for,
    forwarded_host,
    forwarded_proto,
    fixture::Field{"X-Subathon-Csrf", "1"},
};

struct Expected {
    const char *selector;
    std::span<const fixture::Field> fields;
    std::size_t projection;
};

bool equals(std::span<const std::byte> arena, std::size_t offset, std::size_t size,
            std::string_view expected) noexcept {
    return size == expected.size() && offset <= arena.size() && size <= arena.size() - offset &&
           std::memcmp(arena.data() + offset, expected.data(), size) == 0;
}

std::optional<std::size_t> projection(bh::RequestView view, const Expected &expected) noexcept {
    if (view.route_index != 0 || view.method != "GET" || view.escaped_target != target ||
        !view.body.empty() || view.fields.size() != expected.fields.size())
        return {};
    std::size_t total = view.method.size() + view.escaped_target.size() + 12 + 2;
    for (std::size_t i = 0; i < view.fields.size(); ++i) {
        const auto field = view.fields[i];
        const auto &oracle = expected.fields[i];
        if (!equals(view.arena, field.name, field.name_size, oracle.name) ||
            !equals(view.arena, field.value, field.value_size, oracle.value))
            return {};
        total += field.name_size + field.value_size + 4;
    }
    if (total != expected.projection)
        return {};
    return total;
}

class SteadyClock final : public ws::ElapsedSource {
  public:
    std::variant<ws::ElapsedInstant, ws::ElapsedUnavailable> sample() const noexcept override {
        const auto elapsed = std::chrono::steady_clock::now() - epoch_;
        return ws::ElapsedInstant{
            1 + static_cast<bh::U64>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count())};
    }

  private:
    const std::chrono::steady_clock::time_point epoch_ = std::chrono::steady_clock::now();
};

struct Endpoints {
    std::uint16_t http, native;
};

struct Report {
    bh::U64 listener, connection, sequence;
    std::uint16_t peer_port;
    std::size_t fields, projection;
};

class Control {
  public:
    explicit Control(Expected expected) : expected_(expected) {}
    ~Control() { stop(); }

    void start() {
        // The default log registration remains live until process exit. This
        // existing scanner observes only its own INFO canary and channel.
        static auto *const logs = new fixture::LogObservation;
        logs_ = logs;
        fixture::bind_checked_log(*logs);
        scratch_.acquire();
        scratch_.initial_directories();

        if (ws::linked_backend() != ws::Backend::admitted_corrected)
            throw std::runtime_error("FIXTURE: expected corrected native backend");
        auto profile = ws::create_profile(
            {1, 1, 64, 64, 16, 2, 128, 256, 256, 64, 256, 254, 8, 50, 50, 100, 50}, clock_);
        native_ = take<std::unique_ptr<ws::Profile>>(profile);
        constexpr std::array routes{bh::Route{"GET", target, 4096, 4096, 4096}};
        auto foundation =
            bh::create({2, 1, 1, 1, 1, 4096, 4, 5000, 15000, 5000, 5000, 10}, routes,
                       {"/foundation/ws", nullptr, nullptr, nullptr}, clock_,
                       {this, connected, request, ready, ended, removed, drained, nullptr});
        foundation_ = take<std::unique_ptr<bh::Foundation>>(foundation);
        require(ac::install(*foundation_, {this, capture}));

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
                fixture::terminal_fixture();
        });
        bh::Endpoints bind{};
        std::copy(loopback.begin(), loopback.end(), bind.address.begin());
        require(bh::register_listeners(*foundation_, app, bind));
        app.registerBeginningAdvice([this, &app] {
            app.getLoop()->queueInLoop([this, &app] {
                const auto endpoints = app.getListeners();
                if (endpoints.size() != 2 || endpoints[0].toIp() != loopback ||
                    endpoints[1].toIp() != loopback || !endpoints[0].toPort() ||
                    !endpoints[1].toPort() || endpoints[0].toPort() == endpoints[1].toPort()) {
                    fail();
                    return;
                }
                app.getIOLoop(0)->queueInLoop(
                    [this, ports = Endpoints{endpoints[0].toPort(), endpoints[1].toPort()}] {
                        publish(listening_, ports);
                    });
            });
        });
        framework_ = std::thread([this, &app] {
            try {
                app.run();
            } catch (...) {
                fail();
            }
            publish(returned_, true);
        });
    }

    Endpoints listeners() { return wait(listening_, 7s); }
    Report observed_request() { return wait(reported_, 12s); }

    void stop() noexcept {
        if (framework_.joinable()) {
            const auto now = std::get<ws::ElapsedInstant>(clock_->sample()).nanoseconds;
            if (!std::holds_alternative<bh::Post>(foundation_->stop({now + 5'000'000'000})))
                fixture::terminal_fixture();
            std::unique_lock lock(mutex_);
            if (!changed_.wait_for(lock, 7s, [this] { return returned_ && drained_; }))
                fixture::terminal_fixture();
            lock.unlock();
            framework_.join();
            if (!std::holds_alternative<bh::Done>(foundation_->finish()))
                fixture::terminal_fixture();
            foundation_.reset();
        }
        if (!scratch_.remove(listening_.has_value()))
            fixture::terminal_fixture();
    }

    bool complete() const {
        // Only after stop and join. No input-derived strings enter the report.
        return !failed_ && phase_ == Phase::removed && ended_ && observed_ &&
               observed_->requests == 1 && observed_->raw_releases == 1 &&
               observed_->wire_releases == 1 && observed_->bytes_read == expected_.projection &&
               logs_->clean();
    }

  private:
    // Phase, associated Peer, and drain snapshot are IO0-owned until join.
    enum class Phase { cold, connected, captured, replied, removed };
    fixture::Deadline watchdog_;
    fixture::OwnedScratch scratch_;
    Expected expected_;
    std::shared_ptr<SteadyClock> clock_ = std::make_shared<SteadyClock>();
    std::unique_ptr<ws::Profile> native_;
    std::unique_ptr<bh::Foundation> foundation_;
    std::thread framework_;
    fixture::LogObservation *logs_ = nullptr;
    Phase phase_ = Phase::cold;
    std::optional<bh::Peer> peer_;
    std::optional<Report> captured_;
    std::optional<bh::Observation> observed_;
    bool ended_ = false;

    std::mutex mutex_;
    std::condition_variable changed_;
    std::optional<Endpoints> listening_;
    std::optional<Report> reported_;
    bool failed_ = false, returned_ = false, drained_ = false;

    template <class T, class V> void publish(T &destination, V value) {
        {
            std::lock_guard lock(mutex_);
            destination = value;
        }
        changed_.notify_all();
    }

    template <class T> T wait(const std::optional<T> &value, std::chrono::seconds timeout) {
        std::unique_lock lock(mutex_);
        if (!changed_.wait_for(lock, timeout, [&] { return value || failed_ || returned_; }) ||
            failed_ || returned_)
            throw std::runtime_error("FIXTURE: capture control failed or timed out");
        return *value;
    }

    void fail() { publish(failed_, true); }

    template <class T, class Result> static T take(Result &result) {
        if (auto *value = std::get_if<T>(&result))
            return std::move(*value);
        throw std::runtime_error("FIXTURE: native control result");
    }

    static void require(bh::Result<bh::Done> result) { (void)take<bh::Done>(result); }
    static Control &self(void *owner) { return *static_cast<Control *>(owner); }

    static void connected(void *owner, bh::Peer peer) {
        auto &s = self(owner);
        if (s.phase_ != Phase::cold || !std::holds_alternative<bh::Done>(peer.context(&s)) ||
            !std::holds_alternative<bh::Done>(
                s.foundation_->admission(peer, bh::Pause::admitted))) {
            s.fail();
            (void)s.foundation_->close(peer);
            return;
        }
        s.peer_.emplace(std::move(peer));
        s.phase_ = Phase::connected;
    }

    static void capture(void *owner, ac::CaptureLease lease) {
        auto &s = self(owner);
        if (s.phase_ != Phase::connected) {
            s.fail();
            throw std::runtime_error("FIXTURE: unexpected capture");
        }
        auto result = lease.with_capture(owner, [](void *p, ac::CaptureView view) {
            auto &control = self(p);
            const auto &facts = view.native;
            const auto size = projection(view.request, control.expected_);
            if (!size || facts.listener != 1 || !facts.connection || facts.request_sequence != 1 ||
                !facts.peer_port ||
                !std::equal(loopback.begin(), loopback.end(), facts.peer_address.begin()) ||
                facts.peer_address[loopback.size()] != '\0' ||
                facts.artifact_sha256 != fixture::selected_identity::artifact ||
                facts.profile_sha256 != fixture::selected_identity::profile)
                return;
            control.captured_ =
                Report{facts.listener,  facts.connection,           facts.request_sequence,
                       facts.peer_port, view.request.fields.size(), *size};
        });
        if (!std::holds_alternative<ac::Done>(result) || !s.captured_) {
            s.fail();
            throw std::runtime_error("FIXTURE: ordered capture or native association mismatch");
        }
        s.phase_ = Phase::captured;
    }

    static void request(void *owner, void *context, bh::Request request) {
        auto &s = self(owner);
        if (context != owner || s.phase_ != Phase::captured) {
            s.fail();
            return;
        }
        try {
            auto reserved = request.reserve_reply({200, 4, false});
            auto reply = take<bh::Reply>(reserved);
            require(reply.fill(nullptr, [](void *, std::span<std::byte> bytes) -> bh::U64 {
                if (bytes.size() != 4)
                    return 0;
                std::memcpy(bytes.data(), "diag", 4);
                return 4;
            }));
            require(reply.commit());
            request.release_raw();
            s.phase_ = Phase::replied;
            s.publish(s.reported_, *s.captured_);
        } catch (...) {
            s.fail();
            throw;
        }
    }

    static void ready(void *, void *) {}
    static void ended(void *owner, void *context, bh::End end) {
        auto &s = self(owner);
        if (context != owner || s.phase_ != Phase::replied || s.ended_ || end != bh::End::drained)
            s.fail();
        s.ended_ = true;
    }

    static void removed(void *owner, void *context) {
        auto &s = self(owner);
        if (context != owner) {
            s.fail();
            return;
        }
        if (s.phase_ != Phase::replied || !s.ended_)
            s.fail();
        s.phase_ = Phase::removed;
        s.peer_.reset();
    }

    static void drained(void *owner) {
        auto &s = self(owner);
        auto observation = s.foundation_->inspect();
        if (auto *value = std::get_if<bh::Observation>(&observation))
            s.observed_ = *value;
        else
            s.fail();
        s.publish(s.drained_, true);
        drogon::app().quit();
    }
};

bool completion() {
    pollfd input{STDIN_FILENO, POLLIN, 0};
    char command = 0;
    return ::poll(&input, 1, 5000) == 1 && (input.revents & POLLIN) &&
           ::read(STDIN_FILENO, &command, 1) == 1 && command == 'S';
}

void run(Expected expected) {
    // Both the SDK and the existing log fixture have one lifetime per process.
    ASSERT_TRUE(GTEST_FLAG_GET(filter) == expected.selector && GTEST_FLAG_GET(repeat) == 1);
    Control control(expected);
    control.start();
    const auto port = control.listeners();
    ASSERT_GT(std::printf("SESSION010_READY %s http=%s:%u native=%s:%u\n", expected.selector,
                          loopback.data(), unsigned(port.http), loopback.data(),
                          unsigned(port.native)),
              0);
    ASSERT_EQ(std::fflush(stdout), 0);
    const auto capture = control.observed_request();
    ASSERT_GT(std::printf("SESSION010_CAPTURE %s listener=%" PRIu64 " connection=%" PRIu64
                          " sequence=%" PRIu64 " peer=%s:%u fields=%zu projection=%zu ordered=1\n",
                          expected.selector, capture.listener, capture.connection, capture.sequence,
                          loopback.data(), unsigned(capture.peer_port), capture.fields,
                          capture.projection),
              0);
    ASSERT_EQ(std::fflush(stdout), 0);
    ASSERT_TRUE(completion()) << "FIXTURE: completion byte missing";
    control.stop();
    ASSERT_TRUE(control.complete()) << "FIXTURE: expected one drained capture/request";
    ASSERT_GT(std::printf("SESSION010_FINISHED %s\n", expected.selector), 0);
    ASSERT_EQ(std::fflush(stdout), 0);
}

TEST(Session010EdgeCapture, Desktop252) { run({"Session010EdgeCapture.Desktop252", desktop, 252}); }
TEST(Session010EdgeCapture, Browser282) { run({"Session010EdgeCapture.Browser282", browser, 282}); }
} // namespace
