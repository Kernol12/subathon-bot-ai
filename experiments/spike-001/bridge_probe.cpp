// SPDX-License-Identifier: GPL-3.0-or-later
// Disposable Linux loopback experiment. This is not a production server or product API.
#include <QCoreApplication>
#include <QMetaObject>
#include <QSocketNotifier>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <cerrno>
#include <csignal>
#include <drogon/WebSocketController.h>
#include <drogon/drogon.h>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <thread>
#include <trantor/net/EventLoop.h>
#include <trantor/net/TcpConnection.h>
#include <unistd.h>
#include <vector>

namespace {
volatile sig_atomic_t signalFd = -1;

// Installed before starting Drogon, removed before closing either pipe descriptor.
class TerminationPipe {
  public:
    TerminationPipe() {
        if (pipe2(descriptors_, O_NONBLOCK | O_CLOEXEC) != 0)
            throw std::runtime_error("Cannot create signal pipe");
        signalFd = descriptors_[1];
        struct sigaction action{};
        action.sa_handler = [](int) {
            const int saved = errno;
            const char byte = 's';
            if (signalFd >= 0) {
                const auto ignored = write(signalFd, &byte, 1);
                (void)ignored;
            }
            errno = saved;
        };
        sigemptyset(&action.sa_mask);
        if (sigaction(SIGTERM, &action, &previous_) != 0) {
            signalFd = -1;
            close(descriptors_[0]);
            close(descriptors_[1]);
            throw std::runtime_error("Cannot install SIGTERM handoff");
        }
    }
    ~TerminationPipe() {
        sigaction(SIGTERM, &previous_, nullptr);
        signalFd = -1;
        close(descriptors_[0]);
        close(descriptors_[1]);
    }
    int reader() const { return descriptors_[0]; }

  private:
    int descriptors_[2]{};
    struct sigaction previous_{};
};

Json::Value observation(const char *name) {
    Json::Value value;
    value["event"] = name;
    return value;
}
void output(const Json::Value &value) {
    // All observations are emitted on the authoritative Qt owner.
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::cout << Json::writeString(writer, value) << std::endl;
}
Json::Value parse(const std::string &text) {
    Json::Value value;
    Json::CharReaderBuilder builder;
    std::string errors;
    std::istringstream stream(text);
    if (!Json::parseFromStream(builder, stream, &value, &errors))
        throw std::runtime_error("Invalid probe JSON");
    return value;
}

struct Reply {
    trantor::EventLoop *loop;
    std::function<void(const Json::Value &, drogon::HttpStatusCode)> deliver;
};
struct Command {
    int id;
    bool requestOffOwner;
    Reply reply;
};

// QObject and the one SQL connection stay on the QCoreApplication thread.
class Bridge final : public QObject {
  public:
    Bridge(const std::string &databasePath, bool hold)
        : owner_(QThread::currentThread()), hold_(hold) {
        database_ = QSqlDatabase::addDatabase("QSQLITE", "bridge-writer");
        database_.setDatabaseName(QString::fromStdString(databasePath));
        if (!database_.open())
            throw std::runtime_error(database_.lastError().text().toStdString());
        QSqlQuery query(database_);
        if (!query.exec("PRAGMA journal_mode=WAL") || !query.next())
            throw std::runtime_error("Cannot obtain journal mode");
        journalMode_ = query.value(0).toString().toStdString();
        if (!query.exec("PRAGMA synchronous=FULL") || !query.exec("PRAGMA synchronous") ||
            !query.next())
            throw std::runtime_error("Cannot obtain synchronous mode");
        synchronous_ = query.value(0).toInt();
        if (journalMode_ != "wal" || synchronous_ != 2 ||
            !query.exec("CREATE TABLE commands (id INTEGER PRIMARY KEY)"))
            throw std::runtime_error("Probe requires an empty database with WAL/FULL");
    }

    bool offOwner() const { return QThread::currentThread() != owner_; }

    void ready(uint16_t port) {
        requireOwner();
        networkReady_ = true;
        auto value = observation("ready");
        value["port"] = port;
        value["qt_version"] = qVersion();
        value["drogon_version"] = drogon::getVersion();
        value["journal_mode"] = journalMode_;
        value["synchronous"] = synchronous_;
        output(value);
        finishIfDrained();
    }

    // Network callbacks only post immutable command data and response ownership.
    void post(int id, const char *transport, Reply reply) {
        const bool requestOffOwner = offOwner();
        QMetaObject::invokeMethod(
            this,
            [this, id, transport, requestOffOwner, reply = std::move(reply)]() mutable {
                requireOwner();
                if (!requestOffOwner)
                    ++violations_;
                if (!accepting_) {
                    Json::Value error;
                    error["error"] = "admission_closed";
                    respond(std::move(reply), error, drogon::k503ServiceUnavailable);
                    return;
                }
                commands_.push_back({id, requestOffOwner, std::move(reply)});
                auto value = observation("admitted");
                value["id"] = id;
                value["transport"] = transport;
                output(
                    value); // The command is now owned by this queue before its barrier is visible.
                if (!hold_)
                    applyCommands();
            },
            Qt::QueuedConnection);
    }

    void connection(const trantor::TcpConnectionPtr &connection) {
        const auto key = reinterpret_cast<uintptr_t>(connection.get());
        const bool connected = connection->connected();
        const std::weak_ptr<trantor::TcpConnection> weak = connection;
        QMetaObject::invokeMethod(
            this,
            [this, key, connected, weak] {
                requireOwner();
                if (connected)
                    connections_[key] = {weak, false};
                else
                    connections_.erase(key);
                finishIfDrained();
            },
            Qt::QueuedConnection);
    }

    void closeAdmission() {
        requireOwner();
        if (accepting_) {
            accepting_ = false;
            output(observation("admission_closed"));
        }
    }
    void release() {
        requireOwner();
        hold_ = false;
        applyCommands();
        output(observation("released"));
    }
    void stop() {
        requireOwner();
        if (stopping_)
            return;
        closeAdmission();
        stopping_ = true;
        output(observation("stopping"));
        hold_ = false;
        applyCommands();
        finishIfDrained();
    }
    bool closeWriter() {
        requireOwner();
        if (database_.thread() != owner_)
            ++violations_;
        database_.close();
        const bool closed = !database_.isOpen();
        database_ = QSqlDatabase();
        QSqlDatabase::removeDatabase("bridge-writer");
        return closed && !QSqlDatabase::contains("bridge-writer");
    }
    int violations() const { return violations_; }
    bool failed() const { return failed_; }

  private:
    void requireOwner() {
        if (QThread::currentThread() != owner_)
            throw std::logic_error("Probe accessed outside Qt owner");
    }
    void respond(Reply reply, Json::Value body, drogon::HttpStatusCode status) {
        ++responses_;
        auto *loop = reply.loop;
        loop->queueInLoop([this, reply = std::move(reply), body = std::move(body), status, loop] {
            const bool correctLoop = loop->isInLoopThread();
            reply.deliver(body, status);
            QMetaObject::invokeMethod(
                this,
                [this, correctLoop] {
                    if (!correctLoop)
                        ++violations_;
                    --responses_;
                    finishIfDrained();
                },
                Qt::QueuedConnection);
        });
    }
    void applyCommands() {
        requireOwner();
        // Move the current batch: replies return asynchronously to their originating IO loops.
        auto commands = std::move(commands_);
        commands_.clear();
        for (auto &command : commands) {
            const bool qtOwner = QThread::currentThread() == thread();
            const bool sqlOwner = database_.thread() == QThread::currentThread();
            if (!qtOwner || !sqlOwner)
                ++violations_;
            bool committed = false;
            if (qtOwner && sqlOwner && database_.transaction()) {
                QSqlQuery query(database_);
                query.prepare("INSERT INTO commands(id) VALUES (?)");
                query.addBindValue(command.id);
                committed = query.exec() && database_.commit();
                if (!committed)
                    database_.rollback();
            }
            Json::Value body;
            if (committed) {
                body["id"] = command.id;
                body["qt_owner"] = qtOwner;
                body["sql_owner"] = sqlOwner;
                body["request_off_owner"] = command.requestOffOwner;
            } else {
                failed_ = true;
                body["error"] = "probe_sql_failure";
            }
            respond(std::move(command.reply), std::move(body),
                    committed ? drogon::k200OK : drogon::k500InternalServerError);
        }
        finishIfDrained();
    }
    void finishIfDrained() {
        requireOwner();
        if (!stopping_ || !networkReady_ || !commands_.empty() || responses_ != 0 || quitting_)
            return;
        // quit() force-closes sockets. First flush replies with TCP shutdown, then wait for
        // peer close/disconnection observations; only then stop the framework event loops.
        for (auto &[key, connection] : connections_) {
            (void)key;
            if (!connection.closing) {
                connection.closing = true;
                if (auto peer = connection.weak.lock())
                    peer->getLoop()->queueInLoop([peer] { peer->shutdown(); });
            }
        }
        if (connections_.empty()) {
            quitting_ = true;
            drogon::app().quit();
        }
    }

    struct Connection {
        std::weak_ptr<trantor::TcpConnection> weak;
        bool closing;
    };
    QThread *const owner_;
    QSqlDatabase database_;
    std::string journalMode_;
    int synchronous_ = 0;
    bool hold_;
    bool accepting_ = true;
    bool stopping_ = false;
    bool networkReady_ = false;
    bool quitting_ = false;
    bool failed_ = false;
    int responses_ = 0;
    int violations_ = 0;
    std::vector<Command> commands_;
    std::map<uintptr_t, Connection> connections_;
};

Bridge *bridge = nullptr; // Installed before run(); cleared only after Drogon has joined.
class ProbeSocket : public drogon::WebSocketController<ProbeSocket> {
  public:
    void handleNewMessage(const drogon::WebSocketConnectionPtr &connection, std::string &&message,
                          const drogon::WebSocketMessageType &type) override {
        if (type != drogon::WebSocketMessageType::Text)
            return;
        auto value = parse(message);
        if (!value["id"].isInt()) {
            Json::Value error;
            error["error"] = "invalid_probe_id";
            connection->sendJson(error);
            return;
        }
        const std::weak_ptr<drogon::WebSocketConnection> weak = connection;
        Reply reply{trantor::EventLoop::getEventLoopOfCurrentThread(),
                    [weak](const Json::Value &body, drogon::HttpStatusCode) {
                        if (auto peer = weak.lock(); peer && peer->connected()) {
                            peer->sendJson(body);
                            // One command per probe connection: initiate a valid close frame before
                            // the pinned framework half-closes TCP on a client close frame.
                            peer->shutdown();
                        }
                    }};
        bridge->post(value["id"].asInt(), "websocket", std::move(reply));
    }
    void handleNewConnection(const drogon::HttpRequestPtr &,
                             const drogon::WebSocketConnectionPtr &) override {}
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &) override {}
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws");
    WS_PATH_LIST_END
};

int run(int argc, char **argv) {
    QCoreApplication application(argc, argv);
    int port = 0;
    bool hold = false;
    std::string databasePath;
    for (int i = 1; i < argc; ++i) {
        const std::string argument(argv[i]);
        if (argument == "--port" && i + 1 < argc)
            port = std::stoi(argv[++i]);
        else if (argument == "--database" && i + 1 < argc)
            databasePath = argv[++i];
        else if (argument == "--hold")
            hold = true;
        else
            return 2;
    }
    if (databasePath.empty() || port < 0 || port > 65535)
        return 2;
    Bridge owner(databasePath, hold);
    bridge = &owner;
    TerminationPipe signalPipe;
    auto &framework = drogon::app();
    framework.setLogLevel(trantor::Logger::kWarn)
        .disableSigtermHandling()
        .setThreadNum(1)
        .addListener("127.0.0.1", static_cast<uint16_t>(port));
    framework.setConnectionCallback(
        [&](const trantor::TcpConnectionPtr &connection) { owner.connection(connection); });
    framework.registerHandler(
        "/command",
        [&](const drogon::HttpRequestPtr &request,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            const auto value = request->getJsonObject();
            if (!value || !(*value)["id"].isInt()) {
                auto response = drogon::HttpResponse::newHttpResponse();
                response->setStatusCode(drogon::k400BadRequest);
                response->setCloseConnection(true);
                callback(response);
                return;
            }
            Reply reply{trantor::EventLoop::getEventLoopOfCurrentThread(),
                        [callback = std::move(callback)](const Json::Value &body,
                                                         drogon::HttpStatusCode status) {
                            auto response = drogon::HttpResponse::newHttpJsonResponse(body);
                            response->setStatusCode(status);
                            response->setCloseConnection(true);
                            callback(response);
                        }};
            owner.post((*value)["id"].asInt(), "http", std::move(reply));
        },
        {drogon::Post});
    framework.registerBeginningAdvice([&] {
        // In the pinned source, beginning advice precedes startListening(). This second
        // main-loop post then an IO-loop barrier orders ready AFTER the actual listen call.
        framework.getLoop()->queueInLoop([&] {
            const auto listenerPort = framework.getListeners().front().toPort();
            framework.getIOLoop(0)->queueInLoop([&, listenerPort] {
                QMetaObject::invokeMethod(
                    &owner, [&, listenerPort] { owner.ready(listenerPort); }, Qt::QueuedConnection);
            });
        });
    });
    QSocketNotifier signalNotifier(signalPipe.reader(), QSocketNotifier::Read);
    QObject::connect(&signalNotifier, &QSocketNotifier::activated, &application, [&] {
        char data[32];
        while (read(signalPipe.reader(), data, sizeof(data)) > 0) {
        }
        owner.stop();
    });
    QSocketNotifier inputNotifier(STDIN_FILENO, QSocketNotifier::Read);
    std::string input;
    QObject::connect(&inputNotifier, &QSocketNotifier::activated, &application, [&] {
        char data[256];
        const auto count = read(STDIN_FILENO, data, sizeof(data));
        if (count <= 0) {
            inputNotifier.setEnabled(false);
            owner.stop();
            return;
        }
        input.append(data, static_cast<size_t>(count));
        for (auto end = input.find('\n'); end != std::string::npos; end = input.find('\n')) {
            auto command = input.substr(0, end);
            input.erase(0, end + 1);
            if (command == "stop")
                owner.stop();
            else if (command == "close")
                owner.closeAdmission();
            else if (command == "release")
                owner.release();
        }
    });
    std::jthread server([&] {
        framework.run();
        QMetaObject::invokeMethod(&application, &QCoreApplication::quit, Qt::QueuedConnection);
    });
    const int result = application.exec();
    server.join();
    bridge = nullptr;
    auto stopped = observation("stopped");
    stopped["writer_closed"] = owner.closeWriter();
    stopped["drogon_joined"] = !server.joinable();
    stopped["owner_violations"] = owner.violations();
    output(stopped);
    return result || owner.failed() || owner.violations() || !stopped["writer_closed"].asBool();
}
} // namespace
int main(int argc, char **argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception &error) {
        std::cerr << "Probe error: " << error.what() << '\n';
        return 2;
    }
}
