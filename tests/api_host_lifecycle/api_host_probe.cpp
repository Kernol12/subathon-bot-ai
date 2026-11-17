// SPDX-License-Identifier: GPL-3.0-or-later
// NEW consumer: ships no listener/signal/clock/host implementation. All those
// symbols below resolve to first-party production translation units.
#include "support/api_host_fixture.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSocketNotifier>
#include <QTimer>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <link.h>
#include <poll.h>
#include <subathon/server/api_host_drogon.hpp>
#include <subathon/server/api_host_signal.hpp>
#include <unistd.h>

namespace {
namespace h = subathon::server::api_host;
namespace t = h::testing;
using namespace subathon::server;
using api_host_fixture::require;
constexpr std::size_t command_bytes = 4096, command_count = 64, event_count = 256;
constexpr std::size_t log_bytes = 1048576, identity_bytes = 131072;
// Separate inherited protocol pipe. Fixed queue slots and a 1MiB cumulative
// byte budget; a single writer owns all writes. Host callbacks never wait for IO.
int protocol_fd = -1;
unsigned commands = 0; // includes the bounded release gates, all consumed on Qt owner
int remaining_ms(std::chrono::steady_clock::time_point deadline) {
    const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
                          deadline - std::chrono::steady_clock::now())
                          .count();
    if (left <= 0)
        ::_exit(70);
    return int(std::min<std::int64_t>(left, 7000));
}
class EventWriter {
  public:
    EventWriter() : worker_([this] { run(); }) {}
    ~EventWriter() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        cv_.notify_one();
        worker_.join();
    }
    void add(QJsonObject v) {
        auto data = QJsonDocument(v).toJson(QJsonDocument::Compact);
        data.append('\n');
        {
            std::lock_guard lock(mutex_);
            if (closed_ || ++events_ > event_count || data.size() > qsizetype(identity_bytes) ||
                bytes_ + std::size_t(data.size()) > log_bytes || count_ == queue_.size())
                ::_exit(70);
            auto &slot = queue_[(head_ + count_) % queue_.size()];
            slot = {bytes_, std::size_t(data.size())};
            std::memcpy(storage_.data() + bytes_, data.data(), slot.size);
            bytes_ += slot.size;
            ++count_;
        }
        cv_.notify_one();
    }
    void flush() {
        std::unique_lock lock(mutex_);
        if (!cv_.wait_for(lock, std::chrono::seconds(7), [&] { return count_ == 0 && !writing_; }))
            ::_exit(70);
    }

  private:
    struct Record {
        std::size_t offset = 0, size = 0;
    };
    void run() {
        for (;;) {
            Record data;
            {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, [&] { return closed_ || count_; });
                if (!count_)
                    return;
                data = queue_[head_];
                head_ = (head_ + 1) % queue_.size();
                --count_;
                writing_ = true;
            }
            std::size_t sent = 0;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(7);
            while (sent < data.size) {
                pollfd ready{protocol_fd, POLLOUT, 0};
                const auto result = ::poll(&ready, 1, remaining_ms(deadline));
                if (result < 0 && errno == EINTR)
                    continue;
                if (result <= 0 || !(ready.revents & POLLOUT))
                    ::_exit(70);
                const auto n =
                    ::write(protocol_fd, storage_.data() + data.offset + sent, data.size - sent);
                if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
                    continue;
                if (n <= 0)
                    ::_exit(70);
                sent += std::size_t(n);
            }
            {
                std::lock_guard lock(mutex_);
                writing_ = false;
            }
            cv_.notify_all();
        }
    }
    // Each byte is written once, immutable after enqueue; cumulative1MiB cap
    // means the arena never wraps.256 descriptors add only4096 bytes on x86-64.
    std::array<char, log_bytes> storage_{};
    std::array<Record, event_count> queue_{};
    std::mutex mutex_;
    std::condition_variable cv_;
    std::size_t head_ = 0, count_ = 0, events_ = 0, bytes_ = 0;
    bool closed_ = false, writing_ = false;
    std::thread worker_;
};
std::unique_ptr<EventWriter> output;
void emit_event(QJsonObject v) { output->add(std::move(v)); }
[[noreturn]] void terminal(const char *kind, const char *reason, int code) {
    emit_event({{"event", kind}, {"reason", reason}});
    output->flush();
    ::_exit(code);
}
void fixture_require(bool ok, const char *message) {
    if (!ok)
        terminal("fixture_error", message, 70);
}
void product_require(bool ok, const char *message) {
    if (!ok)
        terminal("assertion", message, 43);
}
// No pumping/re-entry, byte-exact release, EINTR-safe with one absolute bound.
void release_gate() {
    fixture_require(++commands <= command_count, "Release command count");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(7);
    constexpr std::string_view expected = "{\"op\":\"release\"}\n";
    std::array<char, 32> release{};
    std::size_t used = 0;
    while (used < expected.size()) {
        pollfd command{STDIN_FILENO, POLLIN, 0};
        const auto result = ::poll(&command, 1, remaining_ms(deadline));
        if (result < 0 && errno == EINTR)
            continue;
        fixture_require(result == 1 && (command.revents & POLLIN), "Release watchdog/EOF");
        const auto n = ::read(STDIN_FILENO, release.data() + used, expected.size() - used);
        if (n < 0 && (errno == EINTR || errno == EAGAIN))
            continue;
        fixture_require(n > 0, "Release read/EOF");
        used += n;
    }
    fixture_require(std::string_view(release.data(), used) == expected, "Exact bounded release");
}
QString thread_token(std::thread::id id) {
    // Small per-process stable identity table; never a hash collision oracle.
    static std::mutex mutex;
    static std::array<std::thread::id, 8> ids{};
    static std::size_t used = 0;
    std::lock_guard lock(mutex);
    for (std::size_t i = 0; i < used; ++i)
        if (ids[i] == id)
            return QString::number(i + 1);
    fixture_require(used < ids.size(), "Thread token cap");
    ids[used++] = id;
    return QString::number(used);
}
QJsonObject identity(const char *event) {
    QJsonObject value{
        {"event", event},
        {"consumer_cplusplus", qint64(__cplusplus)},
        {"qt_runtime", qVersion()},
        {"qt_compiled", QT_VERSION_STR},
        {"backend", native_ws::linked_backend() == native_ws::Backend::admitted_corrected
                        ? "admitted_corrected"
                        : "stock_upstream"}};
    struct Inventory {
        QJsonArray objects;
        bool complete = true;
    } inventory;
    const auto result = ::dl_iterate_phdr(
        [](dl_phdr_info *info, std::size_t, void *context) {
            auto &v = *static_cast<Inventory *>(context);
            if (v.objects.size() == 128 || !info->dlpi_name) {
                v.complete = false;
                return 1;
            }
            const auto size = ::strnlen(info->dlpi_name, 513);
            if (size > 512) {
                v.complete = false;
                return 1;
            }
            QByteArray path(info->dlpi_name, qsizetype(size));
            QString kind = "file";
            if (path.isEmpty()) {
                std::array<char, 513> out{};
                const auto n = ::readlink("/proc/self/exe", out.data(), out.size());
                if (n <= 0 || n >= ssize_t(out.size())) {
                    v.complete = false;
                    return 1;
                }
                path = QByteArray(out.data(), qsizetype(n));
                kind = "main";
            } else if (path == "linux-vdso.so.1" || path == "linux-gate.so.1")
                kind = "virtual";
            else if (!path.startsWith('/')) {
                v.complete = false;
                return 1;
            }
            v.objects.append(
                QJsonObject{{"path", QString::fromUtf8(path)},
                            {"kind", kind},
                            {"load_address", QString::number(qulonglong(info->dlpi_addr))}});
            return 0;
        },
        &inventory);
    value["loaded_objects"] = inventory.objects;
    value["complete"] = inventory.complete && result == 0;
    value["incomplete_reason"] =
        inventory.complete && result == 0 ? "none" : "bounded_inventory_incomplete";
    return value;
}
template <class T> T implemented(h::Result<T> result, const char *piece) {
    if (std::holds_alternative<h::NotImplemented>(result)) {
        emit_event({{"event", "unimplemented"}, {"piece", piece}});
        output->flush();
        ::_exit(42); // attributable assertion failure, never socket/setup timeout
    }
    if (!std::holds_alternative<T>(result)) {
        emit_event({{"event", "host_failure"},
                    {"piece", piece},
                    {"reason", int(std::get<h::Failure>(result))}});
        terminal("assertion", "Unexpected production refusal", 43);
    }
    return std::get<T>(std::move(result));
}
QJsonObject snapshot(const h::Host &host) {
    const auto s = host.inspect();
    const auto identity = t::HostAccess::elapsed_identity(host);
    return {{"elapsed_source_shared", identity.host && identity.host == identity.profile},
            {"state", int(s.state)},
            {"storage_active", s.storage_active},
            {"associations", QString::number(s.associations)},
            {"queued_diagnostics", QString::number(s.queued_diagnostics)},
            {"input_capacity", QString::number(s.input.capacity)},
            {"response_capacity", QString::number(s.response.capacity)},
            {"writer_barrier", s.writer_barrier},
            {"network_barrier", s.network_barrier},
            {"owner_cleanup", s.owner_cleanup},
            {"stop_fences", QString::number(s.stop_fences)},
            {"drain_deadline", s.drain_deadline ? QJsonValue(QString::number(*s.drain_deadline))
                                                : QJsonValue(QJsonValue::Null)}};
}
class Dispatch final : public h::CommandDispatcher {
  public:
    explicit Dispatch(bool block, bool gate, bool diagnostic_throw, bool forward_after_signal)
        : block_(block), gate_(gate), diagnostic_throw_(diagnostic_throw),
          forward_after_signal_(forward_after_signal), owner_(std::this_thread::get_id()) {}
    ~Dispatch() override {
        emit_event({{"event", "dispatch_destructor"},
                    {"owner_thread", std::this_thread::get_id() == owner_}});
    }
    h::OwnedReply diagnostic(h::CommandView) override {
        product_require(std::this_thread::get_id() == owner_, "Diagnostic on Qt owner");
        emit_event({{"event", "diagnostic"}});
        if (diagnostic_throw_) {
            emit_event({{"event", "diagnostic_exception"}});
            throw std::runtime_error("deliberate diagnostic refusal");
        }
        return api_host_fixture::reply("diag");
    }
    void execute(h::CommandHandle handle, h::CommandView) override {
        product_require(std::this_thread::get_id() == owner_, "Execution on Qt main");
        command_.emplace(std::move(handle));
        api_host_fixture::admitted(command_->forward(
            [this](auto context) -> QVariant {
                product_require(std::this_thread::get_id() == owner_, "Unit body on Qt main");
                const auto session = context.begin();
                account_sessions_fixture::Cleanup cleanup{session};
                SqliteAccountSessionUnit unit(session, {});
                const auto f = unit.read_installation("I");
                product_require(std::holds_alternative<StoredInstallationFacts>(f),
                                "Actual store read");
                const auto &facts = std::get<StoredInstallationFacts>(f);
                product_require(facts.current.installation_ref == "I" &&
                                    facts.current.session_epoch_ref == "E" &&
                                    facts.revision_ref == "J",
                                "I/E/J literal");
                if (block_) {
                    emit_event({{"event", "nonreturning_body"}});
                    for (;;)
                        ::pause(); // supervisor-only terminal path, no cleanup claim
                }
                if (gate_) {
                    gate_ = false;
                    emit_event({{"event", "executing_gate"}});
                    release_gate();
                }
                session.commit();
                return 1;
            },
            [this](auto result) {
                product_require(std::this_thread::get_id() == owner_,
                                "Supplied completion on Qt main");
                emit_event({{"event", "unit_completion"},
                            {"transaction", int(result.transaction)},
                            {"error", int(result.error)},
                            {"value", result.value.has_value()}});
                if (forward_after_signal_) {
                    const auto refusal =
                        command_->forward(api_host_fixture::read_installation, [](auto) {
                            product_require(false, "Refused unit cannot complete");
                        });
                    product_require(std::holds_alternative<h::Failure>(refusal) &&
                                        std::get<h::Failure>(refusal) == h::Failure::stopping,
                                    "Pending native signal cuts forwarding");
                    emit_event({{"event", "signal_forward_refused"}});
                    return;
                }
                if (result.transaction == TransactionOutcome::committed &&
                    result.error == WorkError::none && result.value) {
                    auto end = command_->finish(api_host_fixture::reply("done"));
                    if (std::holds_alternative<h::Failure>(end))
                        product_require(
                            std::get<h::Failure>(end) == h::Failure::stopping ||
                                std::get<h::Failure>(end) == h::Failure::disconnected ||
                                std::get<h::Failure>(end) == h::Failure::resource_limit ||
                                std::get<h::Failure>(end) == h::Failure::elapsed_unavailable,
                            "Actual terminal reply gate");
                    else
                        implemented(std::move(end), "command_finish");
                } else
                    implemented(command_->finish(), "command_end");
            }));
    }

  private:
    std::optional<h::CommandHandle> command_;
    bool block_, gate_, diagnostic_throw_, forward_after_signal_;
    std::thread::id owner_;
};
int child(int argc, char **argv, const QJsonObject &config) {
    QCoreApplication app(argc, argv); // only Qt application, actual process main
    const auto store_kind = config.value("store").toString("current").toStdString();
    api_host_fixture::Store store(store_kind, config.value("fixture_root").toString());
    const auto catalog_before = store.catalog();
    const auto history_before = store_kind == "fresh" ? SqlRows{} : store.history();
    const bool physical = store_kind != "fresh" && store_kind != "old";
    const auto facts_before = physical ? store.facts() : SqlRows{};
    const auto sessions_before = physical ? store.sessions() : SqlRows{};
    emit_event(identity("runtime_identity")); // includes actually loaded SQL plugin
    h::Options options;
    options.store_path = store.path.toStdString();
    options.writer_limits = account_sessions_fixture::limits();
    options.limits.associations = 2;
    options.limits.per_peer = 2;
    options.installation = config.value("installation").toString("I").toStdString();
    const auto response_cap = config.value("response_cap").toInt();
    fixture_require(response_cap == 0 || response_cap == 3 || response_cap == 4,
                    "Finite response profile");
    if (response_cap)
        options.limits.response_bytes = response_cap;
    if (response_cap == 4)
        options.limits.response_capacity = 4;
    const auto peer_cap = config.value("peer_cap").toInt();
    fixture_require(peer_cap == 0 || peer_cap == 2, "Finite peer profile");
    if (peer_cap)
        options.limits.peers = peer_cap;
    const auto body_cap = config.value("body_cap").toInt();
    fixture_require(body_cap == 0 || body_cap == 4, "Finite body profile");
    if (body_cap)
        options.limits.envelope_bytes = body_cap;
    const auto frame_ms = config.value("frame_ms").toInt();
    fixture_require(frame_ms == 0 || frame_ms == 200, "Finite fixture frame duration selection");
    if (frame_ms)
        options.native.frame_ms = std::uint64_t(frame_ms);
    const bool observer_throw = config.value("observer_throw").toBool();
    const auto owner = std::this_thread::get_id();
    bool endpoints_seen = false;
    h::DrogonOptions network;
    network.port = std::uint16_t(config.value("port").toInt());
    network.http_port = std::uint16_t(config.value("http_port").toInt());
    network.scratch_parent = config.value("fixture_root").toString().toStdString();
    auto transport = implemented(
        h::make_drogon_transport(
            network,
            [&](const auto &e) {
                product_require(std::this_thread::get_id() == owner && !endpoints_seen,
                                "Once-only Qt endpoint observer");
                endpoints_seen = true;
                const auto address_size = ::strnlen(e.address.data(), e.address.size());
                const auto root_size = ::strnlen(e.scratch_root.data(), e.scratch_root.size());
                product_require(address_size <= 45 && root_size <= 533 && e.port && e.http_port &&
                                    e.port != e.http_port,
                                "Actual complete distinct endpoints");
                // Fixed fields bound this one serialized record below4096 before output.
                emit_event({{"event", "listeners"},
                            {"address", QString::fromUtf8(e.address.data(), address_size)},
                            {"port", e.port},
                            {"http_port", e.http_port},
                            {"scratch_root", QString::fromUtf8(e.scratch_root.data(), root_size)},
                            {"native_socket_options", e.native_socket_options},
                            {"http_socket_options", e.http_socket_options}});
                if (observer_throw)
                    throw std::runtime_error("endpoint observer exception");
            }),
        "drogon_factory");
    auto dispatcher = std::make_unique<Dispatch>(
        config.value("block_body").toBool(), config.value("gate_body").toBool(),
        config.value("diagnostic_throw").toBool(), config.value("forward_after_signal").toBool());
    // Native path uses NO elapsed/session testing injection. Host must invoke
    // the real monotonic factory itself; identity observation cannot establish it.
    const auto owner_thread = std::this_thread::get_id();
    std::atomic<bool> startup_gate_used{false};
    const bool hold_inspection = config.value("hold_inspection").toBool();
    const bool hold_wrapper = config.value("hold_wrapper").toBool();
    const bool hold_diagnostic = config.value("hold_diagnostic").toBool();

    auto host = t::HostAccess::create(
        std::move(options), std::move(transport), std::move(dispatcher),
        {{}, [&](const t::Event &e) {
             QJsonObject event{{"event", "host_event"},
                               {"kind", int(e.kind)},
                               {"ordinal", QString::number(e.ordinal)},
                               {"command", QString::number(e.command)},
                               {"unit", QString::number(e.unit)},
                               {"owner_thread", e.thread == owner_thread},
                               {"thread_token", thread_token(e.thread)}};
             if (e.work)
                 event["work"] = QJsonObject{{"transaction", int(e.work->transaction)},
                                             {"error", int(e.work->error)},
                                             {"value", e.work->has_value},
                                             {"ticket_revoked", e.work->ticket_revoked}};
             if (e.admission)
                 event["admission"] = int(*e.admission);
             if (e.end)
                 event["end"] = int(*e.end);
             if (e.refusal)
                 event["refusal"] = int(*e.refusal);
             emit_event(std::move(event));
             if (((hold_inspection && e.kind == t::EventKind::inspection_enqueued) ||
                  (hold_wrapper && e.kind == t::EventKind::command_admitted) ||
                  (hold_diagnostic && e.kind == t::EventKind::diagnostic_queued)) &&
                 !startup_gate_used.exchange(true)) {
                 product_require(e.thread == owner_thread &&
                                     std::this_thread::get_id() == owner_thread,
                                 "Pre-entry observation gate on actual Qt owner");
                 emit_event({{"event", hold_inspection   ? "startup_gate"
                                       : hold_diagnostic ? "diagnostic_gate"
                                                         : "wrapper_gate"}});
                 // No execution/publication/work scope exists at these stages.
                 // diagnostic_queued proves actual retained data before callback;
                 // signal during this gate must win at next owner entry.
                 // This bounded fixture handshake observes the actual stage;
                 // it never calls Qt events or a signal handler/host algorithm.
                 release_gate();
             }
         }});
    h::StopSignalBridge signal_bridge(*host);
    implemented(signal_bridge.install(), "signal_install");
    emit_event({{"event", "signal_installed"}}); // before start/framework threads
    // First command chooses when to start; stop/signal before start is observable,
    // never a fixture state setter. Commands cannot select readiness or invocation.
    QByteArray partial;
    bool started = false, stopped_reported = false, exit_requested = false;
    QSocketNotifier control(STDIN_FILENO, QSocketNotifier::Read);
    QObject::connect(&control, &QSocketNotifier::activated, &app, [&] {
        std::array<char, 4096> block{};
        const auto n = ::read(STDIN_FILENO, block.data(), block.size());
        if (n < 0 && (errno == EINTR || errno == EAGAIN))
            return;
        if (n <= 0) {
            fixture_require(n == 0, "Control read");
            control.setEnabled(false);
            exit_requested = true;
            host->request_stop();
            if (stopped_reported)
                app.quit();
            return;
        }
        partial.append(block.data(), qsizetype(n));
        if (partial.size() > qsizetype(command_bytes))
            terminal("fixture_error", "Control protocol bound/shape", 70);
        const auto end = partial.indexOf('\n');
        if (end < 0)
            return;
        if (end != partial.size() - 1 || ++commands > command_count)
            terminal("fixture_error", "Control protocol bound/shape", 70);
        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(partial, &error);
        partial.clear();
        if (error.error != QJsonParseError::NoError || !doc.isObject())
            terminal("fixture_error", "Control protocol bound/shape", 70);
        const auto cmd = doc.object();
        const auto op = cmd.value("op").toString();
        if (op == "start" && !started) {
            started = true;
            implemented(host->start([&](auto r) {
                if (observer_throw) {
                    product_require(std::holds_alternative<h::Failure>(r) &&
                                        std::get<h::Failure>(r) == h::Failure::unavailable,
                                    "Observer exception startup refusal");
                    emit_event(
                        {{"event", "startup_refused"}, {"reason", int(h::Failure::unavailable)}});
                } else {
                    product_require(endpoints_seen,
                                    "Actual endpoint pair precedes startup success");
                    implemented(std::move(r), "startup_completion");
                }
            }),
                        "host_start");
        } else if (op == "stop")
            host->request_stop();
        else if (op == "exit" && stopped_reported) {
            exit_requested = true;
            app.quit();
        } else if (op != "inspect")
            terminal("fixture_error", "Control protocol bound/shape", 70);
        auto response = snapshot(*host);
        response["event"] = "result";
        response["request"] = cmd.value("request");
        emit_event(response);
    });
    QTimer observations;
    bool serving_reported = false;
    // Observation wake hints do not establish listener readiness or a deadline.
    QObject::connect(&observations, &QTimer::timeout, &app, [&] {
        try {
            const auto s = host->inspect();
            if (s.state == h::State::serving && !serving_reported) {
                serving_reported = true;
                emit_event(identity("serving_identity")); // after serving/lazy framework loads
                emit_event({{"event", "serving"}, {"snapshot", snapshot(*host)}});
            }
            if ((s.state == h::State::stopped || (observer_throw && s.state == h::State::failed)) &&
                s.writer_barrier && s.network_barrier && s.owner_cleanup && !stopped_reported) {
                stopped_reported = true;
                emit_event(identity("stopped_identity")); // after lazy runtime/teardown paths
                product_require(s.writer_barrier && s.network_barrier && s.owner_cleanup,
                                "Actual full shutdown barrier");
                product_require(store.catalog() == catalog_before, "Startup never changes schema");
                if (store_kind != "fresh")
                    product_require(store.history() == history_before,
                                    "Startup never advances/repairs history");
                if (physical)
                    product_require(store.facts() == facts_before &&
                                        store.sessions() == sessions_before,
                                    "Startup never rotates/changes account-session facts");
                if (store_kind == "current") {
                    product_require(store.facts() ==
                                        SqlRows{{QString("49"), QString("45"), QString("4A")}},
                                    "Native I/E/J preserved");
                    product_require(store.sessions() == sessions_before,
                                    "Native session facts preserved");
                }
                emit_event({{"event", "stopped"}, {"snapshot", snapshot(*host)}});
                if (exit_requested)
                    app.quit(); // bridge remains installed until explicit exit/EOF
            }
        } catch (const std::exception &) {
            terminal("fixture_error", "Observation SQL/fixture exception", 70);
        }
    });
    observations.start(5);
    // Body watchdog is external too; this timer cannot interrupt blocked work.
    QTimer::singleShot(20000, &app, [] { terminal("fixture_error", "child watchdog", 70); });
    return app.exec(); // framework threads belong ONLY to production transport
}
} // namespace
int main(int argc, char **argv) {
    const char *fd = ::getenv("APITEST_PROTOCOL_FD");
    if (!fd || !*fd)
        return 70;
    char *end = nullptr;
    const auto parsed = ::strtol(fd, &end, 10);
    if (*end || parsed < 3 || parsed > 1048576)
        return 70;
    protocol_fd = int(parsed);
    const auto flags = ::fcntl(protocol_fd, F_GETFL);
    if (flags < 0 || ::fcntl(protocol_fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return 70;
    output = std::make_unique<EventWriter>();
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--inventory") {
            emit_event({{"event", "inventory"},
                        {"protocol", 1},
                        {"commands", QJsonArray{"start", "inspect", "stop", "exit", "release"}},
                        {"production_pieces", QJsonArray{"host", "drogon", "signal", "elapsed"}},
                        {"command_bytes", 4096},
                        {"commands_max", 64},
                        {"events_max", 256},
                        {"log_bytes", 1048576},
                        {"line_bytes", 131072},
                        {"loaded_objects_max", 128},
                        {"writer_arena_bytes", 1048576},
                        {"writer_descriptors_max", 256}});
            output->flush();
            return 0; // NO QCoreApplication, setup, factory, host or test body
        }
        emit_event(identity("identity"));
        if (argc == 2 && std::string_view(argv[1]) == "--identity") {
            output->flush();
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--elapsed-factory") {
            auto clock = implemented(h::make_linux_monotonic_elapsed(), "elapsed_factory");
            const auto a = clock->sample(), b = clock->sample();
            product_require(std::holds_alternative<native_ws::ElapsedInstant>(a) &&
                                std::holds_alternative<native_ws::ElapsedInstant>(b),
                            "Actual monotonic observations");
            product_require(std::get<native_ws::ElapsedInstant>(a).nanoseconds <=
                                std::get<native_ws::ElapsedInstant>(b).nanoseconds,
                            "Native nonregression");
            emit_event({{"event", "elapsed_factory_ok"}});
            output->flush();
            return 0;
        }
        fixture_require(argc == 2 && std::strlen(argv[1]) <= 4096, "Bounded child configuration");
        QJsonParseError error;
        const auto config = QJsonDocument::fromJson(argv[1], &error);
        fixture_require(error.error == QJsonParseError::NoError && config.isObject(),
                        "Configuration object");
        if (config.object().contains("validate_options")) {
            h::DrogonOptions network;
            const auto c = config.object().value("validate_options").toObject();
            network.address = c.value("address").toString("127.0.0.1").toStdString();
            network.port = std::uint16_t(c.value("port").toInt());
            network.http_port = std::uint16_t(c.value("http_port").toInt());
            network.scratch_parent = c.value("scratch_parent").toString("/tmp").toStdString();
            const auto result = h::make_drogon_transport(network);
            if (std::holds_alternative<h::NotImplemented>(result)) {
                emit_event({{"event", "unimplemented"}, {"piece", "drogon_factory"}});
                output->flush();
                ::_exit(42);
            }
            product_require(std::holds_alternative<h::Failure>(result) &&
                                std::get<h::Failure>(result) == h::Failure::invalid_options,
                            "Exact factory invalid_options");
            emit_event({{"event", "factory_options_refused"}});
            output->flush();
            return 0;
        }
        const auto result = child(argc, argv, config.object());
        output->flush();
        return result;
    } catch (const h::NotImplemented &) {
        emit_event({{"event", "unimplemented"}, {"piece", "production_facade"}});
        output->flush();
        return 42;
    } catch (const api_host_fixture::AssertionFailure &e) {
        terminal("fixture_error", e.what(), 70);
    } catch (const std::exception &) {
        emit_event({{"event", "fixture_error"}, {"reason", "assertion_or_setup_failure"}});
        output->flush();
        return 70;
    }
}
