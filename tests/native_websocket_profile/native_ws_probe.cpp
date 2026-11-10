// SPDX-License-Identifier: GPL-3.0-or-later
// Bounded native observer. Protocol enforcement belongs to the selected SDK/profile.
#include <subathon/server/native_websocket_profile.hpp>

#include <drogon/WebSocketConnection.h>
#include <drogon/WebSocketController.h>
#include <drogon/drogon.h>
#include <openssl/evp.h>
#include <trantor/net/EventLoop.h>
#include <trantor/utils/Logger.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <fcntl.h>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <link.h>
#include <mutex>
#include <optional>
#include <poll.h>
#include <stdexcept>
#include <string_view>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>

namespace ws = subathon::server::native_ws;
namespace {
constexpr std::size_t maxEvent = 65536, maxEvents = 128, maxEventBytes = 2 * 1024 * 1024;
constexpr std::size_t maxCommand = 4096, maxCommands = 512, maxPeers = 64, maxLeases = 64;
constexpr std::size_t maxPayload = 1048576;
using namespace std::chrono_literals;

std::atomic<std::uint64_t> sdkDiagnosticBytes{0};

// The SDK owns only this captureless callback, never an Events/Harness reference.
// Nonblocking diagnostic transport failure is fixture failure, not native behavior red.
void sdkDiagnosticOutput(const char *message, std::uint64_t length) noexcept {
    if (length > maxEvent || (length != 0 && message == nullptr))
        std::_Exit(91);
    auto used = sdkDiagnosticBytes.load(std::memory_order_relaxed);
    bool reserved = false;
    for (unsigned attempt = 0; attempt < 8; ++attempt) {
        if (used > maxEventBytes || length > maxEventBytes - used)
            std::_Exit(91);
        // The checked sum is at most 2 MiB, so neither addition nor the counter can wrap.
        if (sdkDiagnosticBytes.compare_exchange_strong(used, used + length,
                                                       std::memory_order_relaxed)) {
            reserved = true;
            break;
        }
    }
    if (!reserved)
        std::_Exit(91);
    std::uint64_t offset = 0;
    for (unsigned attempt = 0; offset < length && attempt < 64; ++attempt) {
        const auto written = ::write(STDERR_FILENO, message + static_cast<std::size_t>(offset),
                                     static_cast<std::size_t>(length - offset));
        if (written > 0)
            offset += static_cast<std::uint64_t>(written);
        else if (written < 0 && errno == EINTR)
            continue;
        else
            std::_Exit(91); // Includes EAGAIN: no blocking wait, truncation or stdout fallback.
    }
    if (offset != length)
        std::_Exit(91);
}

void installSdkDiagnosticRoute() {
    const auto flags = ::fcntl(STDERR_FILENO, F_GETFL);
    if (flags < 0 || ::fcntl(STDERR_FILENO, F_SETFL, flags | O_NONBLOCK) < 0)
        std::_Exit(91);
    trantor::Logger::setOutputFunction(sdkDiagnosticOutput,
                                       [] {}); // Direct write has no user buffer.
}

Json::Value event(const char *name) {
    Json::Value v(Json::objectValue);
    v["event"] = name;
    return v;
}

// Only this writer thread writes stdout. IO callbacks never block on pipe output.
class Events {
  public:
    Events() {
        const int flags = ::fcntl(STDOUT_FILENO, F_GETFL);
        if (flags < 0 || ::fcntl(STDOUT_FILENO, F_SETFL, flags | O_NONBLOCK) < 0)
            throw std::runtime_error("stdout flags");
        writer_ = std::thread([this] { writeLoop(); });
    }
    ~Events() {
        {
            std::lock_guard lock(mutex_);
            done_ = true;
        }
        wake_.notify_one();
        writer_.join();
    }
    void emit(const Json::Value &v) {
        Json::StreamWriterBuilder b;
        b["indentation"] = "";
        std::string line = Json::writeString(b, v) + '\n';
        std::lock_guard lock(mutex_);
        if (line.size() > maxEvent || queue_.size() == maxEvents ||
            line.size() > maxEventBytes - total_)
            std::_Exit(70); // Fixture overflow is failure, never native resource evidence.
        total_ += line.size();
        queue_.push_back(std::move(line));
        wake_.notify_one();
    }

  private:
    void writeLoop() {
        for (;;) {
            std::string line;
            {
                std::unique_lock lock(mutex_);
                wake_.wait(lock, [this] { return done_ || !queue_.empty(); });
                if (queue_.empty())
                    return;
                line = std::move(queue_.front());
                queue_.pop_front();
            }
            const auto deadline = std::chrono::steady_clock::now() + 5s;
            std::size_t offset = 0;
            while (offset < line.size()) {
                const auto n = ::write(STDOUT_FILENO, line.data() + offset, line.size() - offset);
                if (n > 0) {
                    offset += static_cast<std::size_t>(n);
                    continue;
                }
                if (n < 0 && errno == EINTR)
                    continue;
                if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) &&
                    std::chrono::steady_clock::now() < deadline) {
                    pollfd fd{STDOUT_FILENO, POLLOUT, 0};
                    ::poll(&fd, 1, 100);
                    continue;
                }
                std::_Exit(71);
            }
        }
    }
    std::mutex mutex_;
    std::condition_variable wake_;
    std::deque<std::string> queue_;
    std::size_t total_ = 0;
    bool done_ = false;
    std::thread writer_;
};

std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (auto byte : bytes) {
        const auto n = std::to_integer<unsigned>(byte);
        out.push_back(digits[n >> 4]);
        out.push_back(digits[n & 15]);
    }
    return out;
}

std::span<const std::byte> bytes(std::string_view s) {
    return {reinterpret_cast<const std::byte *>(s.data()), s.size()};
}

Json::Value payload(std::span<const std::byte> data) {
    if (data.size() > maxPayload)
        throw std::runtime_error("fixture payload cap");
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned length = 0;
    const char empty = 0;
    const auto *input = data.empty() ? &empty : reinterpret_cast<const char *>(data.data());
    if (EVP_Digest(input, data.size(), digest.data(), &length, EVP_sha256(), nullptr) != 1 ||
        length != 32)
        throw std::runtime_error("digest");
    Json::Value v(Json::objectValue);
    v["length"] = Json::UInt64(data.size());
    v["sha256"] = hex({reinterpret_cast<const std::byte *>(digest.data()), length});
    if (data.size() <= 32)
        v["hex"] = hex(data);
    return v;
}

const char *failure(ws::Failure f) {
    switch (f) {
#define F(name)                                                                                    \
    case ws::Failure::name:                                                                        \
        return #name
        F(null_connection);
        F(not_owned);
        F(wrong_owner);
        F(closed);
        F(resource_limit);
        F(allocation_failure);
        F(allocation_bound_unavailable);
        F(outside_delivery);
        F(delivery_mismatch);
        F(delivery_claimed);
        F(elapsed_unavailable);
        F(elapsed_regression);
        F(deadline_expired);
        F(invalid_input);
#undef F
    }
    return "invalid_enum";
}

const char *configuration(ws::ConfigurationFailure f) {
    switch (f) {
#define F(name)                                                                                    \
    case ws::ConfigurationFailure::name:                                                           \
        return #name
        F(zero_limit);
        F(inconsistent_limits);
        F(arithmetic_overflow);
        F(unsupported_platform);
        F(missing_elapsed_source);
        F(already_installed);
        F(native_started);
#undef F
    }
    return "invalid_enum";
}

template <class T> Json::Value status(const ws::Result<T> &result) {
    Json::Value v(Json::objectValue);
    if (std::holds_alternative<T>(result))
        v["status"] = "ok";
    else if (std::holds_alternative<ws::Unimplemented>(result))
        v["status"] = "unimplemented";
    else {
        v["status"] = "unavailable";
        v["reason"] = failure(std::get<ws::Unavailable>(result).reason);
    }
    return v;
}

Json::Value counter(const ws::ByteCounters &c) {
    Json::Value v(Json::objectValue);
#define C(name) v[#name] = Json::UInt64(c.name)
    C(raw_capacity);
    C(assembly_capacity);
    C(fixed_native_storage);
    C(application_output);
    C(control_output);
    C(callback_payload_connection);
    C(callback_payload_global);
#undef C
    return v;
}

Json::Value global(const ws::GlobalSnapshot &s) {
    Json::Value v(Json::objectValue);
#define C(name) v[#name] = Json::UInt64(s.name)
    C(owners_live_or_retiring);
    C(owners_peak);
    C(upgrade_owners);
    C(upgrade_owners_peak);
    C(callback_leases);
    C(callback_leases_peak);
    C(callback_payload_bytes);
    C(callback_payload_bytes_peak);
#undef C
    return v;
}

Json::Value snapshot(const ws::Snapshot &s) {
    Json::Value v(Json::objectValue);
    v["current"] = counter(s.current);
    v["peak"] = counter(s.peak);
#define C(name) v[#name] = Json::UInt64(s.name)
    C(callback_leases_connection);
    C(callback_leases_global);
    C(owners_live_or_retiring);
    C(owners_peak);
    C(upgrade_owners);
    C(upgrade_owners_peak);
    C(callback_leases_connection_peak);
    C(callback_leases_global_peak);
    C(ws_bytes_read);
    C(ws_bytes_consumed);
    C(frames_consumed);
    C(frames_in_last_turn);
#undef C
    v["continuation_pending"] = s.continuation_pending;
    v["close_sent"] = s.close_sent;
    v["transport_closed"] = s.transport_closed;
    auto &close = v["first_peer_close"];
    if (std::holds_alternative<ws::NoCloseFrame>(s.first_peer_close))
        close["kind"] = "no_close_frame";
    else if (std::holds_alternative<ws::CloseWithoutStatus>(s.first_peer_close))
        close["kind"] = "close_without_status";
    else {
        const auto &body = std::get<ws::CloseWithStatus>(s.first_peer_close);
        if (body.reason_size > body.reason.size())
            throw std::runtime_error("native reason_size");
        close["kind"] = "close_with_status";
        close["code"] = body.code;
        close["reason_hex"] = hex({body.reason.data(), body.reason_size});
    }
    const auto instant = [](const std::optional<ws::ElapsedInstant> &time) {
        return time ? Json::Value(Json::UInt64(time->nanoseconds)) : Json::Value();
    };
    v["frame_deadline"] = instant(s.frame_deadline);
    v["message_deadline"] = instant(s.message_deadline);
    v["close_deadline"] = instant(s.close_deadline);
    return v;
}

thread_local std::uint64_t callerSourceSamples = 0;

struct ClockAudit {
    std::atomic<std::uint64_t> samples{0}, destroyed{0};
};
// Manual mode is explicitly simulated. Native mode is fixture-only MONOTONIC, not session UTC.
class Clock final : public ws::ElapsedSource {
  public:
    Clock(bool native, std::shared_ptr<ClockAudit> audit)
        : native_(native), audit_(std::move(audit)) {}
    ~Clock() override { audit_->destroyed.fetch_add(1); }
    std::variant<ws::ElapsedInstant, ws::ElapsedUnavailable> sample() const noexcept override {
        if (callerSourceSamples == std::numeric_limits<std::uint64_t>::max())
            std::_Exit(85);
        ++callerSourceSamples;
        audit_->samples.fetch_add(1);
        if (!available_.load())
            return ws::ElapsedUnavailable{};
        if (!native_)
            return ws::ElapsedInstant{now_.load()};
        timespec t{};
        if (::clock_gettime(CLOCK_MONOTONIC, &t) != 0 || t.tv_sec < 0 || t.tv_nsec < 0 ||
            t.tv_nsec >= 1000000000)
            return ws::ElapsedUnavailable{};
        constexpr auto max = std::numeric_limits<std::uint64_t>::max();
        const auto seconds = static_cast<std::uint64_t>(t.tv_sec);
        const auto ns = static_cast<std::uint64_t>(t.tv_nsec);
        if (seconds > (max - ns) / 1000000000)
            return ws::ElapsedUnavailable{};
        return ws::ElapsedInstant{seconds * 1000000000 + ns};
    }
    void set(std::uint64_t now, bool available) {
        now_.store(now);
        available_.store(available);
    }

  private:
    bool native_;
    std::atomic<std::uint64_t> now_{1000000000};
    std::shared_ptr<ClockAudit> audit_;
    std::atomic<bool> available_{true};
};

std::atomic<unsigned> foreignCalls{0};
class Foreign final : public drogon::WebSocketConnection {
  public:
    void send(const char *, uint64_t, drogon::WebSocketMessageType) override { ++foreignCalls; }
    void send(std::string_view, drogon::WebSocketMessageType) override { ++foreignCalls; }
    void sendJson(const Json::Value &, drogon::WebSocketMessageType) override { ++foreignCalls; }
    const trantor::InetAddress &localAddr() const override {
        ++foreignCalls;
        return address_;
    }
    const trantor::InetAddress &peerAddr() const override {
        ++foreignCalls;
        return address_;
    }
    bool connected() const override {
        ++foreignCalls;
        return false;
    }
    bool disconnected() const override {
        ++foreignCalls;
        return true;
    }
    void shutdown(drogon::CloseCode, const std::string &) override { ++foreignCalls; }
    void forceClose() override { ++foreignCalls; }
    void setPingMessage(const std::string &, const std::chrono::duration<double> &) override {
        ++foreignCalls;
    }
    void disablePing() override { ++foreignCalls; }

  private:
    trantor::InetAddress address_{0, true};
};

ws::Limits limits(const Json::Value &j) {
    ws::Limits l{32,      16,      1048576, 1048576, 4096, 2,     2097152, 16777216, 1048590,
                 1048576, 1048590, 254,     64,      5000, 10000, 30000,   1000};
    if (!j.isObject())
        throw std::runtime_error("limits object");
#define L(name)                                                                                    \
    if (j.isMember(#name)) {                                                                       \
        if (!j[#name].isUInt64())                                                                  \
            throw std::runtime_error("limit");                                                     \
        l.name = j[#name].asUInt64();                                                              \
    }
    L(connections);
    L(incomplete_upgrades);
    L(frame_bytes);
    L(message_bytes);
    L(frames_per_message);
    L(callback_leases_per_connection);
    L(callback_bytes_per_connection);
    L(callback_bytes_global);
    L(raw_capacity_bytes);
    L(assembly_capacity_bytes);
    L(application_output_bytes);
    L(control_output_bytes);
    L(frames_per_turn);
    L(upgrade_ms);
    L(frame_ms);
    L(message_ms);
    L(close_ms);
#undef L
    return l;
}

Json::Value parse(std::string_view text) {
    if (text.size() > maxCommand)
        throw std::runtime_error("json cap");
    Json::CharReaderBuilder b;
    b["collectComments"] = false;
    std::unique_ptr<Json::CharReader> reader(b.newCharReader());
    Json::Value value;
    std::string error;
    if (!reader->parse(text.data(), text.data() + text.size(), &value, &error) || !value.isObject())
        throw std::runtime_error("json object");
    return value;
}

Json::Value identity() {
    auto v = event("identity");
    v["consumer_cplusplus"] = Json::UInt64(__cplusplus);
    v["pid"] = Json::UInt64(::getpid());
    v["loaded_objects"] = Json::Value(Json::arrayValue);
    struct Inventory {
        Json::Value *objects;
        bool complete = true;
        const char *reason = "none";
    } inventory{&v["loaded_objects"]};
    const auto stopped = ::dl_iterate_phdr(
        [](dl_phdr_info *info, std::size_t, void *context) {
            auto &state = *static_cast<Inventory *>(context);
            if (state.objects->size() == 64 || info->dlpi_name == nullptr) {
                state.complete = false;
                state.reason = "object_count_or_name_missing";
                return 1;
            }
            const auto size = ::strnlen(info->dlpi_name, 513);
            if (size > 512) {
                state.complete = false;
                state.reason = "object_name_capacity";
                return 1;
            }
            std::string name(info->dlpi_name, size);
            Json::Value item(Json::objectValue);
            item["load_address"] = Json::UInt64(info->dlpi_addr);
            if (name.empty()) {
                std::array<char, 513> path{};
                const auto n = ::readlink("/proc/self/exe", path.data(), path.size());
                if (n <= 0 || n >= static_cast<ssize_t>(path.size())) {
                    state.complete = false;
                    state.reason = "main_path_unresolved";
                    item["kind"] = "unresolved_main";
                } else {
                    name.assign(path.data(), static_cast<std::size_t>(n));
                    item["kind"] = "main";
                }
            } else if (name == "linux-vdso.so.1" || name == "linux-gate.so.1")
                item["kind"] = "virtual";
            else if (name.front() == '/')
                item["kind"] = "file";
            else {
                item["kind"] = "unresolved";
                state.complete = false;
                state.reason = "relative_object_path_unresolved";
            }
            if (name.size() > 512) {
                state.complete = false;
                state.reason = "resolved_name_capacity";
                return 1;
            }
            item["path"] = name;
            state.objects->append(std::move(item));
            return 0;
        },
        &inventory);
    v["complete"] = inventory.complete && stopped == 0;
    v["incomplete_reason"] = inventory.reason;
    // Complete means this bounded CURRENT loader inventory, not future dlopen/ABI proof.
    v["sdk_initialization"] = "selected_DSO_initializers_have_run";
    return v;
}

class Harness;
Harness *active = nullptr;
class Controller final : public drogon::WebSocketController<Controller, false> {
  public:
    void handleNewConnection(const drogon::HttpRequestPtr &, const ws::Connection &) override;
    void handleNewMessage(const ws::Connection &, std::string &&,
                          const drogon::WebSocketMessageType &) override;
    void handleConnectionClosed(const ws::Connection &) override;
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/native-profile");
    WS_PATH_LIST_END
};

struct Peer {
    ws::Connection connection;
    trantor::EventLoop *loop = nullptr;
};
struct Held {
    std::optional<ws::MessageLease> lease;
    unsigned id = 0, peer = 0;
};

class Harness {
  public:
    Harness(Events &events, Json::Value settings)
        : events_(events), settings_(std::move(settings)), limits_(limits(settings_["limits"])),
          audit_(std::make_shared<ClockAudit>()),
          clock_(std::make_shared<Clock>(settings_["clock"].asString() == "native", audit_)) {
        if (settings_.isMember("now"))
            clock_->set(settings_["now"].asUInt64(), true);
    }
    ~Harness() {
        finishReader_.store(true);
        if (reader_.joinable())
            reader_.join();
    }
    bool create(bool repeat = false) {
        std::shared_ptr<const ws::ElapsedSource> source =
            settings_["null_source"].asBool() ? nullptr : clock_;
        auto result = ws::create_profile(limits_, std::move(source));
        auto v = event("factory");
        v["repeat"] = repeat;
        v["clock_calls"] = Json::UInt64(audit_->samples.load());
        if (auto *p = std::get_if<std::unique_ptr<ws::Profile>>(&result)) {
            if (!*p)
                throw std::runtime_error("null profile success");
            v["status"] = "created";
            profile_ = std::move(*p);
        } else if (std::holds_alternative<ws::Unimplemented>(result))
            v["status"] = "unimplemented";
        else if (auto *invalid = std::get_if<ws::InvalidConfiguration>(&result)) {
            v["status"] = "invalid_configuration";
            v["reason"] = configuration(invalid->reason);
        } else {
            v["status"] = "unavailable";
            v["reason"] = failure(std::get<ws::Unavailable>(result).reason);
        }
        events_.emit(v);
        return profile_ != nullptr;
    }
    int run() {
        const auto backend = ws::linked_backend();
        auto v = identity();
        v["backend"] =
            backend == ws::Backend::stock_upstream ? "stock_upstream" : "admitted_corrected";
        v["clock"] = settings_["clock"].asString() == "native" ? "fixture_monotonic" : "simulated";
        events_.emit(v);
        const auto mode = settings_["mode"].asString();
        const bool after = settings_["factory_action"].asString() == "after_start";
        const bool repeatAfter = settings_["factory_action"].asString() == "repeat_after_start";
        const bool created = after ? false : create();
        if (settings_["factory_action"].asString() == "repeat")
            create(true);
        if (mode == "factory" && !after && !repeatAfter)
            return 0;
        if (!after && !created && (mode == "profile" || backend != ws::Backend::stock_upstream))
            return backend == ws::Backend::stock_upstream ? 0 : 72;
        if (created && settings_["mutate_limits"].asBool())
            limits_ = ws::Limits{}; // Mutation of the caller copy must not change installed policy.
        active = this;
        auto &app = drogon::app();
        app.setLogLevel(trantor::Logger::kFatal)
            .disableSigtermHandling()
            .setThreadNum(1)
            .setIdleConnectionTimeout(0);
        if (settings_["idle_observer"].asBool())
            app.setIdleConnectionTimeout(2); // Selected TimingWheel requires timeout > 1s.
#if defined(DROGON_SUBATHON_NATIVE_PROFILE_V1)
        // Startup-factory refusal cases must still start an ordinary native framework.
        if (created)
            app.addListener("127.0.0.1", 0, false, "", "", false, {}, true);
        else
            app.addListener("127.0.0.1", 0);
#else
        if (backend == ws::Backend::admitted_corrected)
            return 94; // SETUP: corrected SDK lacks explicit listener-selection wiring.
        app.addListener("127.0.0.1", 0);
#endif
        if (settings_["ordinary_listener"].asBool()) {
            app.addListener("127.0.0.1", 0); // Default policy, in the SAME Drogon process.
            app.registerHandler(
                "/ordinary-probe",
                [this](const drogon::HttpRequestPtr &,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
                    if (++ordinaryRequests_ > 16)
                        std::_Exit(92);
                    auto observed = event("ordinary_request");
                    observed["ordinal"] = ordinaryRequests_;
                    events_.emit(observed);
                    auto response = drogon::HttpResponse::newHttpResponse();
                    response->setContentTypeCode(drogon::CT_TEXT_PLAIN);
                    response->setBody("ordinary-ok\n");
                    response->addHeader("X-Probe-Request", std::to_string(ordinaryRequests_));
                    callback(response);
                },
                {drogon::Get});
        }
        app.registerController(std::make_shared<Controller>());
        if (backend == ws::Backend::stock_upstream)
            app.setClientMaxWebSocketMessageSize(1048576).setMaxConnectionNum(64);
        if (settings_["small_send_buffer"].asBool()) {
            app.setAfterAcceptSockOptCallback([](int fd) {
                const int n = 4096;
                if (::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n)) != 0)
                    std::_Exit(73);
            });
        }
        app.registerBeginningAdvice([this] {
            drogon::app().getLoop()->queueInLoop([this] {
                const auto listeners = drogon::app().getListeners();
                const bool ordinary = settings_["ordinary_listener"].asBool();
                if (listeners.size() != (ordinary ? 2u : 1u))
                    std::_Exit(74);
                // Selected Linux ListenerManager returns actual bound addresses in registration
                // order for this ONE IO loop. This labels ports only; SDK admission uses the bool.
                const auto port = listeners.front().toPort();
                const auto ordinaryPort = ordinary ? listeners.back().toPort() : 0;
                if (port == 0 || (ordinary && (ordinaryPort == 0 || ordinaryPort == port)))
                    std::_Exit(74);
                drogon::app().getIOLoop(0)->queueInLoop([this, port, ordinaryPort] {
                    const auto action = settings_["factory_action"].asString();
                    if (action == "after_start" || action == "repeat_after_start") {
                        create(action == "repeat_after_start");
                        drogon::app().quit();
                        return;
                    }
                    controlOwner_ = trantor::EventLoop::getEventLoopOfCurrentThread();
                    if (controlOwner_ == nullptr)
                        std::_Exit(87);
                    if (ordinaryPort != 0) {
                        if (!profile_)
                            std::_Exit(95);
                        // Read-only observations let native timers/cleanup make progress.
                        // A wake itself proves nothing; only the actual snapshot is reported.
                        retirementObserver_ = controlOwner_->runEvery(0.01, [this] {
                            if (retirementObserverTicks_ == 64)
                                std::_Exit(95);
                            if (++retirementObserverTicks_ == 64) {
                                controlOwner_->invalidateTimer(retirementObserver_);
                                retirementObserver_ = trantor::InvalidTimerId;
                            }
                            auto observed = event("global_observed");
                            const auto value = profile_->inspect_global();
                            observed["inspection"] = status(value);
                            if (const auto *s = std::get_if<ws::GlobalSnapshot>(&value))
                                observed["inspection"]["snapshot"] = global(*s);
                            events_.emit(observed);
                        });
                    }
                    reader_ = std::thread([this] { readCommands(); });
                    auto ready = event("ready");
                    ready["port"] = port;
                    ready["profile"] = profile_ != nullptr;
                    if (ordinaryPort != 0)
                        ready["ordinary_port"] = ordinaryPort;
                    events_.emit(ready);
                });
            });
        });
        app.run();
        finishReader_.store(true);
        if (reader_.joinable())
            reader_.join();
        auto stopped = event("fixture_stopped");
        stopped["failed"] = failed_.load();
        events_.emit(stopped);
        active = nullptr;
        return failed_.load() ? 75 : 0;
    }
    void opened(const ws::Connection &connection) {
        unsigned id;
        {
            std::lock_guard lock(peersMutex_);
            if (peerCount_ == maxPeers)
                std::_Exit(76);
            id = ++peerCount_;
            peers_[id - 1] = {connection, trantor::EventLoop::getEventLoopOfCurrentThread()};
        }
        connection->disablePing();
        auto v = event("opened");
        v["connection"] = id;
        events_.emit(v);
        if (settings_["idle_observer"].asBool()) {
            if (!profile_ || id != 1 || idleObserver_ != trantor::InvalidTimerId)
                std::_Exit(93); // This optional observer permits exactly one WS peer/timer.
            const auto began = std::chrono::steady_clock::now();
            idleObserverOwner_ = trantor::EventLoop::getEventLoopOfCurrentThread();
            idleObserver_ = idleObserverOwner_->runEvery(
                0.5s, [this, weak = std::weak_ptr<drogon::WebSocketConnection>(connection), id, began] {
                    if (idleObserverTicks_ == 7)
                        std::_Exit(93);
                    if (++idleObserverTicks_ == 7) {
                        idleObserverOwner_->invalidateTimer(idleObserver_);
                        idleObserver_ = trantor::InvalidTimerId;
                    }
                    auto observed = event("idle_observed");
                    observed["connection"] = id;
                    observed["ordinal"] = idleObserverTicks_;
                    observed["elapsed_ns"] = Json::UInt64(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::steady_clock::now() - began)
                            .count());
                    // Observation only: no deadline recheck, send, close or state mutation.
                    const auto connection = weak.lock();
                    if (!connection)
                        std::_Exit(93);
                    const auto result = profile_->inspect(connection);
                    observed["inspection"] = status(result);
                    if (const auto *s = std::get_if<ws::Snapshot>(&result))
                        observed["inspection"]["snapshot"] = snapshot(*s);
                    events_.emit(observed);
                });
        }
    }
    void closed(const ws::Connection &connection) {
        const auto id = find(connection);
        auto v = event("closed");
        v["connection"] = id;
        if (profile_) {
            const auto result = profile_->inspect(connection);
            v["inspection"] = status(result);
            if (auto *s = std::get_if<ws::Snapshot>(&result))
                v["inspection"]["snapshot"] = snapshot(*s);
        }
        events_.emit(v);
    }
    void message(const ws::Connection &connection, std::string &&text,
                 drogon::WebSocketMessageType type) {
        const auto id = find(connection);
        const bool data = type == drogon::WebSocketMessageType::Text ||
                          type == drogon::WebSocketMessageType::Binary;
        if (!data || !profile_) {
            auto v = event(data ? "message" : "control");
            v["connection"] = id;
            v["origin"] = profile_ ? "native_control" : "stock_raw";
            v["kind"] = kind(type);
            v["payload"] = payload(bytes(text));
            events_.emit(v);
            if (profile_ && type == drogon::WebSocketMessageType::Ping &&
                settings_["quota_observer"].asBool() && !quotaObserverQueued_) {
                quotaObserverQueued_ = true;
                auto *owner = trantor::EventLoop::getEventLoopOfCurrentThread();
                // Posted during the real control callback, before native quota dispatch returns
                // and queues its continuation. FIFO runs this observer first, then continuation
                // without another poll. Source review must exclude an interposed timer guard.
                owner->queueInLoop([this, connection, id] {
                    auto observed = event("quota_observed");
                    observed["connection"] = id;
                    const auto native = profile_->inspect(connection);
                    observed["inspection"] = status(native);
                    bool valid = false;
                    if (const auto *s = std::get_if<ws::Snapshot>(&native)) {
                        observed["snapshot"] = snapshot(*s);
                        valid = s->continuation_pending && s->frames_in_last_turn == 1 &&
                                s->frames_consumed == 1 && s->ws_bytes_read >= 7 &&
                                s->ws_bytes_consumed == 6 && s->frame_deadline &&
                                s->frame_deadline->nanoseconds == 1010000000;
                    }
                    if (valid && clock_) {
                        // Test input only: no recheck/parser/close/counter mutation here.
                        clock_->set(1010000000, true);
                        observed["status"] = "observed_and_advanced";
                        observed["advanced_to"] = Json::UInt64(1010000000);
                    } else
                        observed["status"] = "fixture_barrier_not_established";
                    events_.emit(observed);
                });
            }
            return;
        }
        // The corrected source binds the current validated native delivery. The fixture
        // cannot substitute text/type or construct that private binding.
        if (settings_["claim_other"].isUInt() && settings_["claim_other"].asUInt() != id) {
            const auto other = peer(settings_["claim_other"].asUInt()).connection;
            auto r = profile_->accept_callback(other);
            auto v = event("claim_other");
            v["connection"] = id;
            v["result"] = status(r.value);
            events_.emit(v);
        }
        auto result = profile_->accept_callback(connection);
        if (auto *lease = std::get_if<ws::MessageLease>(&result.value)) {
            auto v = event("message");
            v["connection"] = id;
            v["origin"] = "profile_lease";
            const auto view = lease->view();
            if (!view)
                throw std::runtime_error("empty successful lease");
            v["kind"] = view->kind == ws::MessageKind::text ? "text" : "binary";
            v["payload"] = payload(view->bytes);
            v["reserved_payload_bytes"] = Json::UInt64(lease->reserved_payload_bytes());
            if (settings_["hold"].asBool())
                v["lease"] = hold(id, std::move(*lease));
            if (settings_["claim_twice"].asBool()) {
                const auto second = profile_->accept_callback(connection);
                v["second_claim"] = status(second.value);
            }
            events_.emit(v);
        } else {
            auto v = event("callback_refused");
            v["connection"] = id;
            v["result"] = status(result.value);
            events_.emit(v);
        }
    }

  private:
    static const char *kind(drogon::WebSocketMessageType t) {
        switch (t) {
        case drogon::WebSocketMessageType::Text:
            return "text";
        case drogon::WebSocketMessageType::Binary:
            return "binary";
        case drogon::WebSocketMessageType::Ping:
            return "ping";
        case drogon::WebSocketMessageType::Pong:
            return "pong";
        case drogon::WebSocketMessageType::Close:
            return "close";
        default:
            return "unknown";
        }
    }
    unsigned find(const ws::Connection &connection) {
        std::lock_guard lock(peersMutex_);
        for (unsigned i = 0; i < peerCount_; ++i)
            if (peers_[i].connection == connection)
                return i + 1;
        throw std::runtime_error("unregistered callback");
    }
    Peer peer(unsigned id) {
        std::lock_guard lock(peersMutex_);
        if (id == 0 || id > peerCount_)
            throw std::runtime_error("peer id");
        return peers_[id - 1];
    }
    unsigned hold(unsigned peerId, ws::MessageLease &&lease) {
        for (auto &slot : held_)
            if (!slot.lease) {
                slot.peer = peerId;
                slot.id = ++leaseSequence_;
                slot.lease.emplace(std::move(lease));
                return slot.id;
            }
        throw std::runtime_error("fixture lease cap");
    }
    Held &held(unsigned id) {
        for (auto &slot : held_)
            if (slot.id == id && slot.lease)
                return slot;
        throw std::runtime_error("lease id");
    }
    void requestStop() {
        if (stopRequested_.exchange(true))
            return;
        if (controlOwner_ == nullptr)
            std::_Exit(88); // Reader starts only after actual owner initialization.
        controlOwner_->queueInLoop([this] { stop(); });
    }
    void stop() {
        if (stopping_)
            return;
        stopping_ = true;
        if (retirementObserver_ != trantor::InvalidTimerId) {
            controlOwner_->invalidateTimer(retirementObserver_);
            retirementObserver_ = trantor::InvalidTimerId;
        }
        if (idleObserver_ != trantor::InvalidTimerId) {
            idleObserverOwner_->invalidateTimer(idleObserver_);
            idleObserver_ = trantor::InvalidTimerId;
        }
        // All teardown is on the initialized IO owner, including wrong-owner failures.
        // Cleanup is deliberately explicit and cannot count as tested native teardown.
        for (auto &slot : held_)
            slot.lease.reset();
        unsigned count;
        {
            std::lock_guard lock(peersMutex_);
            count = peerCount_;
        }
        for (unsigned i = 0; i < count; ++i)
            if (peers_[i].connection)
                peers_[i].connection->forceClose();
        drogon::app().getIOLoop(0)->queueInLoop([this, count] {
            std::array<ws::Connection, maxPeers> releasing{};
            {
                std::lock_guard lock(peersMutex_);
                for (unsigned i = 0; i < count; ++i)
                    releasing[i] = std::move(peers_[i].connection);
            }
            // Destroy references on the real owner, outside the observer registry lock.
            for (auto &connection : releasing)
                connection.reset();
            drogon::app().quit();
        });
    }
    void command(Json::Value c) {
        try {
            const auto op = c["op"].asString();
            auto v = event("result");
            v["request"] = c["request"];
            if (op == "stop") {
                v["status"] = "ok";
                pending_.store(false);
                events_.emit(v);
                requestStop();
                return;
            }
            ws::Connection connection;
            if (c["target"].asString() == "foreign")
                connection = std::make_shared<Foreign>();
            else if (c["target"].asString() != "null" && c["connection"].isUInt())
                connection = peer(c["connection"].asUInt()).connection;
            const auto before = callerSourceSamples;
            if (op == "barrier")
                v["status"] = "ok";
            else if (op == "drop_clock") {
                clock_.reset();
                v["status"] = "ok";
            } else if (op == "clock") {
                if (!clock_)
                    throw std::runtime_error("caller clock already released");
                clock_->set(c["now"].asUInt64(), !c["unavailable"].asBool());
                v["status"] = "ok";
            } else if (op == "native_send" || op == "send") {
                const auto n = c["length"].asUInt64();
                const auto octet = c["octet"].asUInt();
                if (!connection || n > maxPayload || octet > 255)
                    throw std::runtime_error("send input");
                const std::string body(static_cast<std::size_t>(n), static_cast<char>(octet));
                const bool binary = c["binary"].asBool();
                if (op == "native_send") {
                    connection->send(body, binary ? drogon::WebSocketMessageType::Binary
                                                  : drogon::WebSocketMessageType::Text);
                    v["status"] = "public_native_call";
                } else if (profile_)
                    v = result(v, status(profile_->try_send(connection,
                                                            binary ? ws::MessageKind::binary
                                                                   : ws::MessageKind::text,
                                                            bytes(body))));
                else
                    v["status"] = "unimplemented";
            } else if (op == "native_close") {
                if (!connection)
                    throw std::runtime_error("native close connection");
                connection->shutdown(static_cast<drogon::CloseCode>(c["code"].asUInt()),
                                     c["reason"].asString());
                v["status"] = "public_native_call";
            } else if (!profile_)
                v["status"] = "unimplemented";
            else if (op == "inspect") {
                const auto r = profile_->inspect(connection);
                v = result(v, status(r));
                if (const auto *s = std::get_if<ws::Snapshot>(&r))
                    v["snapshot"] = snapshot(*s);
            } else if (op == "global") {
                const auto r = profile_->inspect_global();
                v = result(v, status(r));
                if (const auto *s = std::get_if<ws::GlobalSnapshot>(&r))
                    v["snapshot"] = global(*s);
            } else if (op == "recheck")
                v = result(v, status(profile_->recheck_deadlines(connection)));
            else if (op == "claim") {
                const std::string notNative =
                    "A"; // Cannot be passed as native delivery to the API.
                (void)notNative;
                const auto r = profile_->accept_callback(connection);
                v = result(v, status(r.value));
            } else if (op == "close") {
                ws::LocalClose body = ws::CloseWithoutStatus{};
                if (c.isMember("code")) {
                    const auto reason = c["reason"].asString();
                    if (reason.size() > 123 || c["code"].asUInt() > 65535)
                        throw std::runtime_error("local close input");
                    ws::CloseWithStatus s{};
                    s.code = static_cast<std::uint16_t>(c["code"].asUInt());
                    s.reason_size = static_cast<std::uint8_t>(reason.size());
                    std::memcpy(s.reason.data(), reason.data(), reason.size());
                    body = s;
                }
                const auto r = profile_->start_close(connection, body);
                v = result(v, status(r));
                if (const auto *d = std::get_if<ws::CloseDisposition>(&r))
                    v["disposition"] =
                        *d == ws::CloseDisposition::started ? "started" : "already_closing";
            } else if (op == "lease_consumer") {
                auto &source = held(c["lease"].asUInt());
                ws::MessageLease transferred(std::move(*source.lease));
                source.lease.reset();
                std::promise<Json::Value> completion;
                auto ready = completion.get_future();
                std::thread consumer(
                    [lease = std::move(transferred), &completion, owner = controlOwner_]() mutable {
                        Json::Value outcome(Json::objectValue);
                        try {
                            outcome["off_owner"] =
                                trantor::EventLoop::getEventLoopOfCurrentThread() != owner;
                            const auto view = lease.view();
                            outcome["original_present"] = view.has_value();
                            if (view)
                                outcome["payload"] = payload(view->bytes);
                            auto copy = lease.try_copy();
                            outcome["copy"] = status(copy.value);
                            if (auto *cloned = std::get_if<ws::MessageLease>(&copy.value)) {
                                cloned->reset();
                                outcome["copy_reset_absent"] = !cloned->view().has_value();
                                outcome["copy_reset_reserved"] =
                                    Json::UInt64(cloned->reserved_payload_bytes());
                            }
                            lease.reset();
                            outcome["original_reset_absent"] = !lease.view().has_value();
                            outcome["original_reset_reserved"] =
                                Json::UInt64(lease.reserved_payload_bytes());
                            outcome["status"] = "ok";
                        } catch (...) {
                            outcome["status"] = "fixture_consumer_error";
                        }
                        completion.set_value(std::move(outcome));
                    });
                // IO cannot run a resumption while waiting here. Consumer release/copy must
                // complete independently; the watchdog is failure, never simulated success.
                if (ready.wait_for(2s) != std::future_status::ready)
                    std::_Exit(90);
                consumer.join();
                v = result(v, ready.get());
            } else if (op == "lease_copy") {
                auto &source = held(c["lease"].asUInt());
                auto r = source.lease->try_copy();
                v = result(v, status(r.value));
                if (auto *copy = std::get_if<ws::MessageLease>(&r.value))
                    v["lease"] = hold(source.peer, std::move(*copy));
            } else if (op == "lease_move") {
                auto &source = held(c["lease"].asUInt());
                v["lease"] = hold(source.peer, std::move(*source.lease));
                v["moved_from_present"] = source.lease->view().has_value();
                v["moved_from_reserved"] = Json::UInt64(source.lease->reserved_payload_bytes());
                source.lease.reset();
                v["status"] = "ok";
            } else if (op == "lease_release") {
                held(c["lease"].asUInt()).lease.reset();
                v["status"] = "ok";
            } else if (op == "lease_view") {
                const auto &lease = *held(c["lease"].asUInt()).lease;
                const auto view = lease.view();
                v["present"] = view.has_value();
                v["reserved_payload_bytes"] = Json::UInt64(lease.reserved_payload_bytes());
                if (view)
                    v["payload"] = payload(view->bytes);
                v["status"] = "ok";
            } else
                throw std::runtime_error("unknown operation");
            v["clock_audit_scope"] = "current_caller_thread";
            v["clock_calls_before"] = Json::UInt64(before);
            v["clock_calls_after"] = Json::UInt64(callerSourceSamples);
            v["foreign_virtual_calls"] = foreignCalls.load();
            v["source_destroyed"] = Json::UInt64(audit_->destroyed.load());
            pending_.store(
                false); // Publish terminal result only after releasing command ownership.
            events_.emit(v);
        } catch (...) {
            failed_.store(true);
            pending_.store(false);
            auto v = event("fixture_error");
            v["reason"] = "command";
            events_.emit(v);
            requestStop();
        }
    }
    static Json::Value result(Json::Value v, const Json::Value &fields) {
        for (const auto &key : fields.getMemberNames())
            v[key] = fields[key];
        return v;
    }
    void readCommands() {
        try {
            std::string line;
            unsigned count = 0;
            std::array<char, 256> buffer{};
            while (!finishReader_.load()) {
                pollfd fd{STDIN_FILENO, POLLIN, 0};
                const auto ready = ::poll(&fd, 1, 100);
                if (ready < 0 && errno == EINTR)
                    continue;
                if (ready < 0)
                    throw std::runtime_error("stdin poll");
                if (ready == 0)
                    continue;
                const auto n = ::read(STDIN_FILENO, buffer.data(), buffer.size());
                if (n < 0) {
                    if (errno == EINTR)
                        continue;
                    throw std::runtime_error("stdin read error");
                }
                if (n == 0) {
                    if (!line.empty())
                        throw std::runtime_error("truncated stdin command");
                    requestStop();
                    return;
                }
                for (ssize_t i = 0; i < n; ++i) {
                    if (buffer[static_cast<std::size_t>(i)] != '\n') {
                        if (line.size() == maxCommand)
                            throw std::runtime_error("stdin cap");
                        line += buffer[static_cast<std::size_t>(i)];
                        continue;
                    }
                    if (++count > maxCommands || pending_.exchange(true))
                        throw std::runtime_error("command quota");
                    auto c = parse(line);
                    line.clear();
                    auto *owner = drogon::app().getIOLoop(0);
                    if (c["wrong_owner"].asBool())
                        owner = drogon::app().getLoop();
                    else if (c["connection"].isUInt())
                        owner = peer(c["connection"].asUInt()).loop;
                    owner->queueInLoop(
                        [this, c = std::move(c)]() mutable { command(std::move(c)); });
                }
            }
        } catch (...) {
            failed_.store(true);
            auto v = event("fixture_error");
            v["reason"] = "stdin";
            pending_.store(false);
            events_.emit(v);
            requestStop();
        }
    }
    Events &events_;
    Json::Value settings_;
    ws::Limits limits_;
    std::shared_ptr<ClockAudit> audit_;
    std::shared_ptr<Clock> clock_;
    std::unique_ptr<ws::Profile> profile_;
    std::mutex peersMutex_;
    std::array<Peer, maxPeers> peers_{};
    unsigned peerCount_ = 0;
    unsigned ordinaryRequests_ = 0; // Optional synthetic route, on the sole IO owner.
    std::array<Held, maxLeases> held_{};
    unsigned leaseSequence_ = 0;
    std::atomic<bool> finishReader_{false}, pending_{false}, failed_{false}, stopRequested_{false};
    trantor::EventLoop *controlOwner_ = nullptr;
    trantor::TimerId idleObserver_ = trantor::InvalidTimerId;
    trantor::EventLoop *idleObserverOwner_ = nullptr;
    unsigned idleObserverTicks_ = 0; // At most seven read-only observations on one IO owner.
    trantor::TimerId retirementObserver_ = trantor::InvalidTimerId;
    unsigned retirementObserverTicks_ = 0; // At most 64 snapshots, sibling-listener cases only.
    bool stopping_ = false, quotaObserverQueued_ = false;
    std::thread reader_;
};

void Controller::handleNewConnection(const drogon::HttpRequestPtr &, const ws::Connection &c) {
    active->opened(c);
}
void Controller::handleNewMessage(const ws::Connection &c, std::string &&s,
                                  const drogon::WebSocketMessageType &t) {
    active->message(c, std::move(s), t);
}
void Controller::handleConnectionClosed(const ws::Connection &c) { active->closed(c); }
} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 2 || std::strlen(argv[1]) > maxCommand)
            return 64;
        installSdkDiagnosticRoute();
        Events events;
        if (std::string_view(argv[1]) == "--identity") {
            auto v = identity();
            v["backend"] = ws::linked_backend() == ws::Backend::stock_upstream
                               ? "stock_upstream"
                               : "admitted_corrected";
            const bool complete = v["complete"].asBool();
            events.emit(v);
            return complete ? 0 : 89;
        }
        Harness harness(events, parse(argv[1]));
        return harness.run();
    } catch (...) {
        return 65;
    }
}
