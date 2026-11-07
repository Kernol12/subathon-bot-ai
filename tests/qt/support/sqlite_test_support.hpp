// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QRegularExpression>
#include <QString>
#include <QTest>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <string>
#include <subathon/server/sqlite_writer.hpp>
#include <utility>

namespace qt_test_support {

// Work callbacks must unwind on assertion failure; returning from a Qt macro
// would return only from the helper/callback and could let later work commit.
inline void require(bool condition, const QString &message,
                    const std::source_location location = std::source_location::current()) {
    if (!condition)
        throw std::runtime_error(QString("%1:%2: %3")
                                     .arg(QString::fromUtf8(location.file_name()))
                                     .arg(location.line())
                                     .arg(message)
                                     .toStdString());
}
inline void require(bool condition, const char *message,
                    const std::source_location location = std::source_location::current()) {
    if (!condition)
        require(false, QString::fromUtf8(message), location);
}
inline void require(bool condition, const std::string &message,
                    const std::source_location location = std::source_location::current()) {
    if (!condition)
        require(false, QString::fromStdString(message), location);
}

// Preserve the actual writer outcome, including negative and unknown outcomes.
// The shared callback state remains alive if the condition wait times out.
inline subathon::server::SqliteWorkResult
run_work(subathon::server::SqliteWriter &writer, subathon::server::SqliteWriter::Work body,
         int timeout, const std::source_location location = std::source_location::current()) {
    struct State {
        bool done{false};
        subathon::server::SqliteWorkResult result;
    };
    const auto state = std::make_shared<State>();
    require(writer.enqueue_work(std::move(body),
                                [state](auto result) {
                                    state->result = std::move(result);
                                    state->done = true;
                                }) == subathon::server::StorageError::none,
            "Writer admits test work", location);
    require(QTest::qWaitFor([state] { return state->done; }, timeout), "Writer completes test work",
            location);
    return std::move(state->result);
}

// Qt clears these patterns after each test function: install from init() too.
inline void fail_on_sql_warnings() {
    QTest::failOnWarning(
        QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
}

} // namespace qt_test_support
