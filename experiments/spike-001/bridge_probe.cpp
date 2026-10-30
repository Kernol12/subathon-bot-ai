// SPDX-License-Identifier: GPL-3.0-or-later
// Running test-first harness: every credit/queue operation is explicitly unimplemented.
#include <QCoreApplication>
#include <QMetaObject>
#include <QSocketNotifier>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <cerrno>
#include <drogon/WebSocketController.h>
#include <drogon/drogon.h>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <signal.h>
#include <sstream>
#include <thread>
#include <trantor/net/EventLoop.h>
#include <unistd.h>

namespace {
std::mutex outputMutex;
int signalFd = -1;
void emitEvent(Json::Value value) {
    std::lock_guard lock(outputMutex);
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
void onSignal(int) {
    const int saved = errno;
    const char byte = 's';
    if (signalFd >= 0) {
        const auto ignored = write(signalFd, &byte, 1);
        (void)ignored;
    }
    errno = saved;
}
Json::Value missing(const Json::Value &input, const char *transport) {
    Json::Value event;
    event["event"] = "admitted";
    event["id"] = input["id"];
    event["transport"] = transport;
    emitEvent(event);
    Json::Value response;
    response["error"] = "FeatureNotImplemented";
    return response;
}
class ProbeSocket : public drogon::WebSocketController<ProbeSocket> {
  public:
    void handleNewMessage(const drogon::WebSocketConnectionPtr &connection, std::string &&message,
                          const drogon::WebSocketMessageType &type) override {
        if (type == drogon::WebSocketMessageType::Text)
            connection->sendJson(missing(parse(message), "websocket"));
    }
    void handleNewConnection(const drogon::HttpRequestPtr &,
                             const drogon::WebSocketConnectionPtr &) override {}
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &) override {}
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws");
    WS_PATH_LIST_END
};
} // namespace
int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);
    int port = 0;
    std::string databasePath;
    for (int i = 1; i < argc; ++i) {
        const std::string argument(argv[i]);
        if (argument == "--port" && i + 1 < argc)
            port = std::stoi(argv[++i]);
        else if (argument == "--database" && i + 1 < argc)
            databasePath = argv[++i];
        else if (argument != "--hold")
            return 2;
    }
    if (databasePath.empty())
        return 2;
    auto database = QSqlDatabase::addDatabase("QSQLITE", "bridge-writer");
    database.setDatabaseName(QString::fromStdString(databasePath));
    if (!database.open())
        return 2;
    QSqlQuery query(database);
    if (!query.exec("PRAGMA journal_mode=WAL") || !query.next() ||
        query.value(0).toString() != "wal")
        return 2;
    if (!query.exec("PRAGMA synchronous=FULL") ||
        !query.exec("CREATE TABLE commands (id INTEGER PRIMARY KEY)"))
        return 2;
    auto &framework = drogon::app();
    framework.setLogLevel(trantor::Logger::kWarn)
        .disableSigtermHandling()
        .setThreadNum(1)
        .addListener("127.0.0.1", static_cast<uint16_t>(port));
    framework.registerHandler("/command",
                              [](const drogon::HttpRequestPtr &request,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
                                  auto response = drogon::HttpResponse::newHttpJsonResponse(
                                      missing(*request->getJsonObject(), "http"));
                                  response->setStatusCode(drogon::k501NotImplemented);
                                  response->setCloseConnection(true);
                                  callback(response);
                              },
                              {drogon::Post});
    framework.registerBeginningAdvice([&] {
        framework.getLoop()->queueInLoop([&] {
            framework.getIOLoop(0)->queueInLoop([&] {
                QMetaObject::invokeMethod(
                    &application,
                    [&] {
                        Json::Value ready;
                        ready["event"] = "ready";
                        ready["port"] = framework.getListeners().front().toPort();
                        ready["qt_version"] = qVersion();
                        ready["drogon_version"] = "1.9.14";
                        ready["journal_mode"] = "wal";
                        ready["synchronous"] = 2;
                        emitEvent(ready);
                    },
                    Qt::QueuedConnection);
            });
        });
    });
    int signalPipe[2];
    if (pipe2(signalPipe, O_NONBLOCK | O_CLOEXEC))
        return 2;
    signalFd = signalPipe[1];
    struct sigaction action{};
    action.sa_handler = onSignal;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGTERM, &action, nullptr))
        return 2;
    bool stopping = false;
    auto stop = [&] {
        if (!stopping) {
            stopping = true;
            Json::Value event;
            event["event"] = "stopping";
            emitEvent(event);
            framework.quit();
        }
    };
    QSocketNotifier signalNotifier(signalPipe[0], QSocketNotifier::Read);
    QObject::connect(&signalNotifier, &QSocketNotifier::activated, &application, [&] {
        char data[32];
        while (read(signalPipe[0], data, sizeof(data)) > 0) {
        }
        stop();
    });
    QSocketNotifier inputNotifier(STDIN_FILENO, QSocketNotifier::Read);
    std::string input;
    QObject::connect(&inputNotifier, &QSocketNotifier::activated, &application, [&] {
        char data[256];
        const auto count = read(STDIN_FILENO, data, sizeof(data));
        if (count <= 0) {
            inputNotifier.setEnabled(false);
            stop();
            return;
        }
        input.append(data, static_cast<size_t>(count));
        for (auto end = input.find('\n'); end != std::string::npos; end = input.find('\n')) {
            auto command = input.substr(0, end);
            input.erase(0, end + 1);
            Json::Value event;
            if (command == "stop")
                stop();
            else {
                event["event"] = command == "close" ? "admission_closed" : "released";
                emitEvent(event);
            }
        }
    });
    std::jthread server([&] {
        framework.run();
        QMetaObject::invokeMethod(&application, &QCoreApplication::quit, Qt::QueuedConnection);
    });
    const int result = application.exec();
    server.join();
    query.clear();
    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase("bridge-writer");
    close(signalPipe[0]);
    close(signalPipe[1]);
    signalFd = -1;
    Json::Value event;
    event["event"] = "stopped";
    event["writer_closed"] = true;
    event["drogon_joined"] = true;
    event["owner_violations"] = 0;
    emitEvent(event);
    return result;
}
