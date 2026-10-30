// SPDX-License-Identifier: GPL-3.0-or-later
// Disposable feasibility experiment with synthetic transport, not a server implementation.
#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    if (!directory.isValid())
        return 1;
    int result = 1;
    {
        auto database = QSqlDatabase::addDatabase("QSQLITE", "spike-001-writer");
        database.setDatabaseName(directory.filePath("disposable.sqlite"));
        if (!database.open()) {
            qCritical() << database.lastError();
            return 1;
        }
        bool failed = false;
        auto check = [&](bool condition, const char *label) {
            if (!condition) {
                failed = true;
                qCritical() << label;
            }
            return condition;
        };
        auto sql = [&](const QString &statement) {
            QSqlQuery query(database);
            if (!query.exec(statement)) {
                check(false, "SQL operation failed");
                qCritical() << query.lastError();
            }
            return query;
        };
        {
            auto journal = sql("PRAGMA journal_mode=WAL");
            check(journal.next() && journal.value(0).toString() == "wal", "WAL unavailable");
            sql("PRAGMA synchronous=FULL");
            auto synchronous = sql("PRAGMA synchronous");
            check(synchronous.next() && synchronous.value(0).toInt() == 2, "FULL unavailable");
            sql("CREATE TABLE commands (id INTEGER PRIMARY KEY)");
            auto version = sql("SELECT sqlite_version()");
            if (version.next())
                std::cout << "Qt " << qVersion() << " SQLite runtime "
                          << version.value(0).toString().toStdString() << std::endl;
        }
        if (failed)
            return 1;

        QObject queue;
        std::mutex admission;
        bool accepting = true;
        int applied = 0;
        std::atomic<int> submitted{0};
        std::atomic<int> rejected{0};
        std::atomic<unsigned> producers{0};
        std::atomic<bool> queueFailure{false};
        auto execute = [&](int id) {
            if (!check(QThread::currentThread() == queue.thread(), "Queue owner mismatch") ||
                !check(database.thread() == QThread::currentThread(), "SQL owner mismatch"))
                return;
            if (!check(database.transaction(), "Transaction begin failed"))
                return;
            QSqlQuery query(database);
            query.prepare("INSERT INTO commands(id) VALUES (?)");
            query.addBindValue(id);
            if (!check(query.exec(), "Command insert failed") ||
                !check(database.commit(), "Transaction commit failed")) {
                database.rollback();
                return;
            }
            ++applied;
        };
        auto submit = [&](int id) {
            // Admission and posting share one lock, so close cannot overtake an admitted post.
            std::lock_guard lock(admission);
            if (!accepting) {
                ++rejected;
                return false;
            }
            if (!QMetaObject::invokeMethod(
                    &queue, [&, id] { execute(id); }, Qt::QueuedConnection)) {
                queueFailure = true;
                return false;
            }
            ++submitted;
            return true;
        };

        // A coordinator joins producers without blocking the authoritative Qt event loop.
        std::jthread coordinator([&] {
            std::vector<std::jthread> workers;
            for (int producer = 0; producer < 4; ++producer) {
                workers.emplace_back([&, producer] {
                    if (QThread::currentThread() == queue.thread())
                        queueFailure = true;
                    producers.fetch_or(1U << producer);
                    for (int index = 0; index < 16; ++index)
                        submit(producer * 16 + index);
                });
            }
            for (auto &worker : workers)
                worker.join();
            {
                std::lock_guard lock(admission);
                accepting = false;
            }
            submit(64); // Explicitly rejected after admission closes.
            if (!QMetaObject::invokeMethod(
                    &queue,
                    [&] {
                        check(!queueFailure, "Synthetic queue submission failed");
                        check(producers == 15 && submitted == 64 && rejected == 1,
                              "Admission counts differ from literal experiment oracle");
                        check(applied == 64, "Drain did not execute every admitted command");
                        auto rows = sql("SELECT COUNT(*), SUM(id) FROM commands");
                        check(rows.next() && rows.value(0).toInt() == 64 &&
                                  rows.value(1).toInt() == 2016,
                              "Persisted rows differ from independent 0..63 oracle");
                        std::cout << "synthetic producers=4 submitted=" << submitted.load()
                                  << " applied=" << applied << " rejected=" << rejected.load()
                                  << " single-owner WAL/FULL drain=" << (failed ? "FAIL" : "PASS")
                                  << std::endl;
                        app.exit(failed ? 1 : 0);
                    },
                    Qt::QueuedConnection))
                queueFailure = true;
        });
        QTimer watchdog;
        watchdog.setSingleShot(true);
        QObject::connect(&watchdog, &QTimer::timeout, &app, [&] {
            qCritical() << "Synthetic drain watchdog expired";
            app.exit(1);
        });
        watchdog.start(5000);
        result = app.exec();
        coordinator.join();
        database.close();
    }
    QSqlDatabase::removeDatabase("spike-001-writer");
    return result;
}
