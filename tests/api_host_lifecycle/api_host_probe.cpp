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
constexpr std::size_t log_bytes = 1048576, identity_bytes = 65536;
std::size_t events = 0, output_bytes = 0;
std::mutex output_mutex;
// Output is nonblocking and finite; overflow/EAGAIN is fixture failure70, never
// feature red. At most16 writes per event, no owner waits for pipe consumption.
void emit_event(QJsonObject v) {
    std::unique_lock lock(output_mutex, std::try_to_lock);
    if (!lock.owns_lock())
        ::_exit(70);
    auto data = QJsonDocument(v).toJson(QJsonDocument::Compact);
    data.append('\n');
    if (++events > event_count || data.size() > qsizetype(identity_bytes) ||
        output_bytes + std::size_t(data.size()) > log_bytes)
        ::_exit(70);
    output_bytes += data.size();
    qsizetype sent = 0;
    for (int n = 0; n < 16 && sent < data.size(); ++n) {
        const auto wrote =
            ::write(STDOUT_FILENO, data.constData() + sent, std::size_t(data.size() - sent));
        if (wrote < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) && data.size() > 4096) {
            pollfd ready{STDOUT_FILENO, POLLOUT, 0};
            if (::poll(&ready, 1, 100) <= 0)
                ::_exit(70);
            continue; // only large identity record, no host/domain callback
        }
        if (wrote <= 0)
            ::_exit(70);
        sent += wrote;
    }
    if (sent != data.size())
        ::_exit(70);
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
            if (v.objects.size() == 64 || !info->dlpi_name) {
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
        ::_exit(42); // attributable assertion failure, never socket/setup timeout
    }
    if (!std::holds_alternative<T>(result)) {
        emit_event({{"event", "host_failure"},
                    {"piece", piece},
                    {"reason", int(std::get<h::Failure>(result))}});
        throw std::runtime_error("Production operation refused");
    }
    return std::get<T>(std::move(result));
}
QJsonObject snapshot(const h::Host &host) {
    const auto s = host.inspect();
    const auto identity = t::HostAccess::elapsed_identity(host);
    return {{"elapsed_source_shared", identity.host && identity.host == identity.profile},
            {"state", int(s.state)},
            {"storage_active", s.storage_active},
            {"generation", QString::number(s.generation)},
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
    explicit Dispatch(bool block) : block_(block), owner_(std::this_thread::get_id()) {}
    h::OwnedReply diagnostic(h::CommandView) override {
        require(std::this_thread::get_id() == owner_, "Diagnostic on Qt owner");
        emit_event({{"event", "diagnostic"}});
        return api_host_fixture::reply("diag");
    }
    h::OwnedDomainResult execute(h::Execution &e, h::CommandView) override {
        require(std::this_thread::get_id() == owner_, "Actual execution on Qt main");
        auto &tx = e.begin_transaction();
        const auto f = tx.accounts().read_installation("I");
        require(std::holds_alternative<StoredInstallationFacts>(f), "Actual guarded store read");
        const auto &facts = std::get<StoredInstallationFacts>(f);
        require(facts.current.installation_ref == "I" && facts.current.session_epoch_ref == "E" &&
                    facts.revision_ref == "J",
                "I/E/J literal");
        if (block_) {
            emit_event({{"event", "nonreturning_body"}});
            // Process-terminal ONLY. Blocked trusted body cannot service Qt stop or
            // certify stopped. External supervisor kills/reaps, no cleanup claim.
            for (;;)
                ::pause();
        }
        tx.commit();
        emit_event({{"event", "domain_committed"}});
        return api_host_fixture::reply("done");
    }
    h::OwnedReply complete(h::Publication &p, const h::OwnedDomainResult &) override {
        require(std::this_thread::get_id() == owner_, "Publication on Qt main");
        const auto f = implemented(p.current_installation(), "publication_source");
        require(f.installation == "I" && f.epoch == "E" && f.revision == "J",
                "Fresh publication I/E/J");
        require(std::holds_alternative<SessionSourceUnavailable>(p.clock()->sample_current()),
                "No SESSION009 native UTC binding");
        emit_event({{"event", "publication"}});
        return api_host_fixture::reply("done");
    }

  private:
    bool block_;
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
    auto transport = implemented(
        h::make_drogon_transport({"127.0.0.1", std::uint16_t(config.value("port").toInt())}),
        "drogon_factory");
    auto dispatcher = std::make_unique<Dispatch>(config.value("block_body").toBool());
    // Native path uses NO elapsed/session testing injection. Host must invoke
    // the real monotonic factory itself; identity observation cannot establish it.
    const auto owner_thread = std::this_thread::get_id();
    bool startup_gate_used = false;
    const bool hold_inspection = config.value("hold_inspection").toBool();
    const bool hold_wrapper = config.value("hold_wrapper").toBool();

    auto host = t::HostAccess::create(
        std::move(options), std::move(transport), std::move(dispatcher),
        {{}, {}, [&](const t::Event &e) {
             QJsonObject event{{"event", "host_event"},
                               {"kind", int(e.kind)},
                               {"ordinal", QString::number(e.ordinal)},
                               {"generation", QString::number(e.generation)},
                               {"owner_thread", e.thread == owner_thread}};
             if (e.work)
                 event["work"] = QJsonObject{{"transaction", int(e.work->transaction)},
                                             {"error", int(e.work->error)},
                                             {"value", e.work->has_value},
                                             {"ticket_revoked", e.work->ticket_revoked}};
             if (e.refusal)
                 event["refusal"] = int(*e.refusal);
             emit_event(std::move(event));
             if (!startup_gate_used &&
                 ((hold_inspection && e.kind == t::EventKind::inspection_enqueued) ||
                  (hold_wrapper && e.kind == t::EventKind::wrapper_accepted))) {
                 startup_gate_used = true;
                 emit_event({{"event", hold_inspection ? "startup_gate" : "wrapper_gate"}});
                 // No execution/publication/work scope exists at acceptance.
                 // This bounded fixture handshake observes the actual stage;
                 // it never calls Qt events or a signal handler/host algorithm.
                 pollfd command{STDIN_FILENO, POLLIN, 0};
                 require(::poll(&command, 1, 7000) == 1, "Startup fixture release watchdog");
                 std::array<char, 32> release{};
                 const auto n = ::read(STDIN_FILENO, release.data(), release.size());
                 require(n > 0 && std::string_view(release.data(), std::size_t(n)) ==
                                      "{\"op\":\"release\"}\n",
                         "Exact bounded fixture release");
             }
         }});
    h::StopSignalBridge signals(*host);
    implemented(signals.install(), "signal_install");
    emit_event({{"event", "signal_installed"}}); // before start/framework threads
    // First command chooses when to start; stop/signal before start is observable,
    // never a fixture state setter. Commands cannot select generation/readiness.
    unsigned commands = 0;
    QByteArray partial;
    bool started = false;
    QSocketNotifier control(STDIN_FILENO, QSocketNotifier::Read);
    QObject::connect(&control, &QSocketNotifier::activated, &app, [&] {
        std::array<char, 4096> block{};
        const auto n = ::read(STDIN_FILENO, block.data(), block.size());
        if (n <= 0) {
            if (n == 0)
                host->request_stop();
            return;
        }
        partial.append(block.data(), qsizetype(n));
        if (partial.size() > qsizetype(command_bytes))
            ::_exit(70);
        const auto end = partial.indexOf('\n');
        if (end < 0)
            return;
        if (end != partial.size() - 1 || ++commands > command_count)
            ::_exit(70);
        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(partial, &error);
        partial.clear();
        if (error.error != QJsonParseError::NoError || !doc.isObject())
            ::_exit(70);
        const auto cmd = doc.object();
        const auto op = cmd.value("op").toString();
        if (op == "start" && !started) {
            started = true;
            implemented(
                host->start([](auto r) { implemented(std::move(r), "startup_completion"); }),
                "host_start");
        } else if (op == "stop")
            host->request_stop();
        else if (op != "inspect")
            ::_exit(70);
        auto response = snapshot(*host);
        response["event"] = "result";
        response["request"] = cmd.value("request");
        emit_event(response);
    });
    QTimer observations;
    bool serving_reported = false;
    // Observation wake hints do not establish listener readiness or a deadline.
    QObject::connect(&observations, &QTimer::timeout, &app, [&] {
        const auto s = host->inspect();
        if (s.state == h::State::serving && !serving_reported) {
            serving_reported = true;
            emit_event({{"event", "serving"}, {"snapshot", snapshot(*host)}});
        }
        if (s.state == h::State::stopped) {
            require(s.writer_barrier && s.network_barrier && s.owner_cleanup,
                    "Actual full shutdown barrier");
            require(store.catalog() == catalog_before, "Startup never changes schema");
            if (store_kind != "fresh")
                require(store.history() == history_before,
                        "Startup never advances/repairs history");
            if (physical)
                require(store.facts() == facts_before && store.sessions() == sessions_before,
                        "Startup never rotates/changes account-session facts");
            if (store_kind == "current") {
                require(store.facts() == SqlRows{{QString("49"), QString("45"), QString("4A")}},
                        "Native I/E/J preserved");
                require(store.sessions() == sessions_before, "Native session facts preserved");
            }
            emit_event({{"event", "stopped"}, {"snapshot", snapshot(*host)}});
            app.quit();
        }
    });
    observations.start(5);
    // Body watchdog is external too; this timer cannot interrupt blocked work.
    QTimer::singleShot(20000, &app, [] {
        emit_event({{"event", "fixture_error"}, {"reason", "child watchdog"}});
        ::_exit(70);
    });
    return app.exec(); // framework threads belong ONLY to production transport
}
} // namespace
int main(int argc, char **argv) {
    const auto flags = ::fcntl(STDOUT_FILENO, F_GETFL);
    if (flags < 0 || ::fcntl(STDOUT_FILENO, F_SETFL, flags | O_NONBLOCK) < 0)
        return 70;
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--inventory") {
            emit_event({{"event", "inventory"},
                        {"protocol", 1},
                        {"commands", QJsonArray{"start", "inspect", "stop"}},
                        {"production_pieces", QJsonArray{"host", "drogon", "signal", "elapsed"}},
                        {"command_bytes", 4096},
                        {"commands_max", 64},
                        {"events_max", 256},
                        {"log_bytes", 1048576}});
            return 0; // NO QCoreApplication, setup, factory, host or test body
        }
        emit_event(identity("identity"));
        if (argc == 2 && std::string_view(argv[1]) == "--identity")
            return 0;
        if (argc == 2 && std::string_view(argv[1]) == "--elapsed-factory") {
            auto clock = implemented(h::make_linux_monotonic_elapsed(), "elapsed_factory");
            const auto a = clock->sample(), b = clock->sample();
            require(std::holds_alternative<native_ws::ElapsedInstant>(a) &&
                        std::holds_alternative<native_ws::ElapsedInstant>(b),
                    "Actual monotonic observations");
            require(std::get<native_ws::ElapsedInstant>(a).nanoseconds <=
                        std::get<native_ws::ElapsedInstant>(b).nanoseconds,
                    "Native nonregression");
            emit_event({{"event", "elapsed_factory_ok"}});
            return 0;
        }
        require(argc == 2 && std::strlen(argv[1]) <= 4096, "Bounded child configuration");
        QJsonParseError error;
        const auto config = QJsonDocument::fromJson(argv[1], &error);
        require(error.error == QJsonParseError::NoError && config.isObject(),
                "Configuration object");
        return child(argc, argv, config.object());
    } catch (const h::NotImplemented &) {
        emit_event({{"event", "unimplemented"}, {"piece", "production_facade"}});
        return 42;
    } catch (const std::exception &) {
        emit_event({{"event", "fixture_error"}, {"reason", "assertion_or_setup_failure"}});
        return 70;
    }
}
