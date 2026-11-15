// SPDX-License-Identifier: GPL-3.0-or-later
// Oracles: SLICE-001 #642 owner decision, contract revision 2, examples 1–8;
// CONTRACT-014 §6 RP14-B supplies the 4096-byte reference limit.
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

#include <limits>

namespace {
struct Run {
    QJsonObject json;
    qint64 wallMs = 0;
};

QStringList arguments(const QString &database) {
    return {"--database", database, "--start-ms",        "600000", "--cap-ms", "3600000",
            "--add-ms",   "300000", "--controlled-clock"};
}

// Process/setup failures are distinct from the exit-code assertion at the stub boundary.
void invoke(const QStringList &args, int expectedExit, Run &result, const QString &directory) {
    QProcess process;
    process.setWorkingDirectory(directory);
    QElapsedTimer wall;
    wall.start();
    process.start(QString::fromUtf8(SUBATHON_DEV_SERVER_PATH), args);
    QVERIFY2(process.waitForStarted(5000), qPrintable(process.errorString()));
    QVERIFY2(process.waitForFinished(10000), qPrintable(process.errorString()));
    // Round the wall-time bound upwards using integer arithmetic only.
    result.wallMs = (wall.nsecsElapsed() + 999999) / 1000000;
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    const auto output = process.readAllStandardOutput();
    if (process.exitCode() != 0)
        QVERIFY2(output.isEmpty(), "Every nonzero exit must have empty stdout");
    QCOMPARE(process.exitCode(), expectedExit);
    if (expectedExit != 0)
        return;
    QVERIFY2(output.endsWith('\n') && output.count('\n') == 1,
             "Success must print exactly one JSON line");
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(output, &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QVERIFY(document.isObject());
    result.json = document.object();
}

void integer(const QJsonObject &object, const QString &key, qint64 expected) {
    const auto value = object.value(key);
    QVERIFY2(value.isDouble(), qPrintable(key + " must be a JSON number"));
    // Qt's integer accessor rejects fractional/out-of-range numbers; never use toDouble().
    QCOMPARE(value.toInteger(std::numeric_limits<qint64>::min()), expected);
}

void summary(const Run &run, qint64 timer, qint64 applications, qsizetype events) {
    QCOMPARE(run.json.value("clock").toString(), QString("controlled"));
    integer(run.json, "timer_ms", timer);
    integer(run.json, "elapsed_ms", 0);
    integer(run.json, "applications", applications);
    QVERIFY(run.json.value("events").isArray());
    QCOMPARE(run.json.value("events").toArray().size(), events);
}

void checkEvent(const QJsonValue &value, qint64 requested, qint64 actual, bool duplicate,
                const QString &id = "e1") {
    QVERIFY(value.isObject());
    const auto object = value.toObject();
    QCOMPARE(object.value("event_id").toString(), id);
    QVERIFY(object.contains("receipt_id"));
    QVERIFY(!object.value("receipt_id").isNull());
    QVERIFY(object.value("duplicate").isBool());
    QCOMPARE(object.value("duplicate").toBool(), duplicate);
    integer(object, "requested_ms", requested);
    integer(object, "actual_ms", actual);
}
} // namespace

class DevelopmentServerTests : public QObject {
    Q_OBJECT
  private slots:
    void example1_addition() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        Run run;
        invoke(arguments(directory.filePath("state.sqlite")) << "--event" << "e1", 0, run,
               directory.path());
        if (QTest::currentTestFailed())
            return;
        summary(run, 900000, 1, 1);
        checkEvent(run.json.value("events").toArray().at(0), 300000, 300000, false);
    }

    void example2_cap() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        auto args = arguments(directory.filePath("state.sqlite"));
        args[args.indexOf("--start-ms") + 1] = "3300000";
        args[args.indexOf("--add-ms") + 1] = "600000";
        Run run;
        invoke(args << "--event" << "e1", 0, run, directory.path());
        if (QTest::currentTestFailed())
            return;
        summary(run, 3600000, 1, 1);
        checkEvent(run.json.value("events").toArray().at(0), 600000, 300000, false);
    }

    void example3_duplicate() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        Run run;
        invoke(arguments(directory.filePath("state.sqlite")) << "--event" << "e1" << "--event"
                                                             << "e1",
               0, run, directory.path());
        if (QTest::currentTestFailed())
            return;
        summary(run, 900000, 1, 2);
        const auto events = run.json.value("events").toArray();
        checkEvent(events.at(0), 300000, 300000, false);
        checkEvent(events.at(1), 300000, 300000, true);
        QCOMPARE(events.at(0).toObject().value("receipt_id"),
                 events.at(1).toObject().value("receipt_id"));
    }

    void example4_restartRead() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto args = arguments(directory.filePath("state.sqlite"));
        Run initial;
        invoke(QStringList(args) << "--event" << "e1", 0, initial, directory.path());
        if (QTest::currentTestFailed())
            return;
        summary(initial, 900000, 1, 1);
        if (QTest::currentTestFailed())
            return;
        Run restarted;
        invoke(args, 0, restarted, directory.path());
        if (QTest::currentTestFailed())
            return;
        summary(restarted, 900000, 0, 0);
    }

    void example5_emptyEvent() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto database = directory.filePath("state.sqlite");
        Run run;
        invoke(arguments(database) << "--event" << "", 2, run, directory.path());
        QVERIFY(!QFileInfo::exists(database));
    }

    void example6_restartRefusal() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto args = arguments(directory.filePath("state.sqlite"));
        Run initial;
        invoke(QStringList(args) << "--event" << "e1", 0, initial, directory.path());
        if (QTest::currentTestFailed())
            return;
        summary(initial, 900000, 1, 1);
        if (QTest::currentTestFailed())
            return;
        Run refused;
        invoke(QStringList(args) << "--event" << "e2", 3, refused, directory.path());
        if (QTest::currentTestFailed())
            return;
        Run reread;
        invoke(args, 0, reread, directory.path());
        if (QTest::currentTestFailed())
            return;
        summary(reread, 900000, 0, 0);
    }

    void example7_systemClock() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        auto args = arguments(directory.filePath("state.sqlite"));
        args.removeAll("--controlled-clock");
        Run run;
        invoke(args << "--event" << "e1", 0, run, directory.path());
        if (QTest::currentTestFailed())
            return;
        QCOMPARE(run.json.value("clock").toString(), QString("system"));
        const auto value = run.json.value("elapsed_ms");
        QVERIFY(value.isDouble());
        const auto elapsed = value.toInteger(-1);
        QVERIFY(elapsed >= 0);
        QVERIFY(elapsed <= run.wallMs);
        integer(run.json, "timer_ms", 900000 - elapsed);
        integer(run.json, "applications", 1);
        QVERIFY(run.json.value("events").isArray());
        const auto events = run.json.value("events").toArray();
        QCOMPARE(events.size(), 1);
        checkEvent(events.at(0), 300000, 300000, false);
    }

    void example8_twoEvents() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        Run run;
        invoke(arguments(directory.filePath("state.sqlite")) << "--event" << "e1" << "--event"
                                                             << "e2",
               0, run, directory.path());
        if (QTest::currentTestFailed())
            return;
        summary(run, 1200000, 2, 2);
        const auto events = run.json.value("events").toArray();
        checkEvent(events.at(0), 300000, 300000, false);
        checkEvent(events.at(1), 300000, 300000, false, "e2");
        QVERIFY(events.at(0).toObject().value("receipt_id") !=
                events.at(1).toObject().value("receipt_id"));
    }

    void invalidArguments_data() {
        QTest::addColumn<QString>("option");
        QTest::addColumn<QString>("kind");
        QTest::addColumn<QString>("value");
        for (const auto &option : {"--database", "--start-ms", "--cap-ms", "--add-ms"}) {
            QTest::newRow(qPrintable(QString("missing-option-") + option))
                << QString(option) << QString("omit") << QString();
            QTest::newRow(qPrintable(QString("missing-value-") + option))
                << QString(option) << QString("missing") << QString();
        }
        for (const auto &option : {"--start-ms", "--cap-ms", "--add-ms"}) {
            QTest::newRow(qPrintable(QString("negative-") + option))
                << QString(option) << QString("replace") << QString("-1");
            QTest::newRow(qPrintable(QString("non-numeric-") + option))
                << QString(option) << QString("replace") << QString("abc");
        }
        QTest::newRow("fractional---add-ms")
            << QString("--add-ms") << QString("replace") << QString("300000.5");
        QTest::newRow("cap-below-start")
            << QString("--cap-ms") << QString("replace") << QString("599999");
        QTest::newRow("event-4097-bytes")
            << QString("--event") << QString("append") << QString(4097, QLatin1Char('x'));
    }

    void invalidArguments() {
        QFETCH(QString, option);
        QFETCH(QString, kind);
        QFETCH(QString, value);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto database = directory.filePath("state.sqlite");
        auto args = arguments(database);
        if (kind == "append") {
            args << option << value;
        } else {
            const auto position = args.indexOf(option);
            QVERIFY(position >= 0);
            if (kind == "replace") {
                args[position + 1] = value;
            } else {
                args.removeAt(position + 1);
                args.removeAt(position);
                if (kind == "missing")
                    args << option;
            }
        }
        Run run;
        invoke(args, 2, run, directory.path());
        QVERIFY(!QFileInfo::exists(database));
        QVERIFY(
            QDir(directory.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty());
    }
};

QTEST_GUILESS_MAIN(DevelopmentServerTests)
#include "dev_server_tests.moc"
