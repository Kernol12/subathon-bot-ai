// SPDX-License-Identifier: GPL-3.0-or-later
#include <QDate>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QUuid>
#include <memory>
#include <stdexcept>
#include <subathon/server/sqlite_writer.hpp>
#include <thread>

using namespace subathon::server;

// Independent fixture SQL never implements admission, transactions or writer decisions.
class FixtureDatabase final {
  public:
    explicit FixtureDatabase(const QString &path)
        : name_(QUuid::createUuid().toString()), db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
    }
    ~FixtureDatabase() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }
    bool open() { return db_.open(); }
    bool execute(const QString &sql) {
        QSqlQuery query(db_);
        return query.exec(sql);
    }
    QVariant scalar(const QString &sql) {
        QSqlQuery query(db_);
        if (!query.exec(sql) || !query.next())
            return {};
        return query.value(0);
    }

  private:
    QString name_;
    QSqlDatabase db_;
};

class SqliteWriterTests final : public QObject {
    Q_OBJECT
  private:
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<FixtureDatabase> fixture_;
    QString path_;

    static WriteBatch insert(qint64 id, qint64 amount) {
        return {{"INSERT INTO counters(id, amount) VALUES (?, ?)", {id, amount}}};
    }
    static QSet<QString> connection_names() {
        const auto names = QSqlDatabase::connectionNames();
        return {names.begin(), names.end()};
    }

  private slots:
    void initTestCase() {
        QVERIFY(QSqlDatabase::isDriverAvailable("QSQLITE"));
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }
    void init() {
        directory_ = std::make_unique<QTemporaryDir>();
        QVERIFY(directory_->isValid());
        path_ = directory_->filePath("state.sqlite");
        fixture_ = std::make_unique<FixtureDatabase>(path_);
        QVERIFY(fixture_->open());
        QVERIFY(fixture_->execute("CREATE TABLE counters(id INTEGER PRIMARY KEY, amount INTEGER)"));
        QVERIFY(fixture_->execute("CREATE TABLE parents(id INTEGER PRIMARY KEY)"));
        QVERIFY(fixture_->execute("CREATE TABLE children(id INTEGER PRIMARY KEY, parent_id INTEGER "
                                  "REFERENCES parents(id) DEFERRABLE INITIALLY DEFERRED)"));
    }
    void cleanup() {
        fixture_.reset();
        directory_.reset();
    }

    // §2 / DB-01: actual writer-connection PRAGMAs, not fixture connection defaults.
    void opensWalFullOnOwnerThread() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool completed = false;
        WriteResult result;
        QThread *completion_thread = nullptr;
        QCOMPARE(writer.enqueue({{"PRAGMA journal_mode", {}},
                                 {"PRAGMA synchronous", {}},
                                 {"PRAGMA foreign_keys", {}},
                                 {"PRAGMA busy_timeout", {}}},
                                [&](WriteResult value) {
                                    result = std::move(value);
                                    completion_thread = QThread::currentThread();
                                    completed = true;
                                }),
                 StorageError::none);
        QVERIFY(!completed);
        QTRY_VERIFY(completed);
        QCOMPARE(completion_thread, QThread::currentThread());
        QCOMPARE(result.error, StorageError::none);
        QCOMPARE(result.rows.size(), 4);
        QCOMPARE(result.rows[0], SqlRows({{QString("wal")}}));
        QCOMPARE(result.rows[1], SqlRows({{2}})); // SQLite FULL is literal2.
        QCOMPARE(result.rows[2], SqlRows({{1}}));
        QCOMPARE(result.rows[3], SqlRows({{0}})); // No hidden SQLite busy wait.
    }

    void commitsFifoBatchesBeforeCompletion() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QList<int> order;
        QList<qint64> observed_sums;
        StorageError first = StorageError::not_open, second = StorageError::not_open;
        QCOMPARE(writer.enqueue(
                     insert(1, 10),
                     [&](WriteResult result) {
                         first = result.error;
                         order.append(1);
                         observed_sums.append(
                             fixture_->scalar("SELECT SUM(amount) FROM counters").toLongLong());
                     }),
                 StorageError::none);
        QCOMPARE(writer.enqueue(
                     insert(2, 20),
                     [&](WriteResult result) {
                         second = result.error;
                         order.append(2);
                         observed_sums.append(
                             fixture_->scalar("SELECT SUM(amount) FROM counters").toLongLong());
                     }),
                 StorageError::none);
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM counters").toInt(), 0);
        QTRY_COMPARE(order.size(), 2);
        QCOMPARE(first, StorageError::none);
        QCOMPARE(second, StorageError::none);
        QCOMPARE(order, QList<int>({1, 2}));
        QCOMPARE(observed_sums, QList<qint64>({10, 30}));
    }

    void rejectsDuplicateWriterUntilShutdown() {
        SqliteWriter first, second;
        QCOMPARE(first.open(path_), StorageError::none);
        const QString equivalent = directory_->path() + "/./state.sqlite";
        QCOMPARE(second.open(equivalent), StorageError::writer_already_open);
        bool stopped = false;
        QCOMPARE(first.shutdown([&] { stopped = true; }), StorageError::none);
        QTRY_VERIFY(stopped);
        QCOMPARE(second.open(equivalent), StorageError::none);
    }

    void readerCannotWriteEvenAfterQueryOnlyOff() {
        SqliteReader reader;
        QCOMPARE(reader.open(path_), StorageError::none);
        QCOMPARE(reader.read({"PRAGMA query_only", {}}).rows, SqlRows({{1}}));
        QCOMPARE(reader.read({"INSERT INTO counters VALUES (1, 10)", {}}).error,
                 StorageError::sql_failure);
        QCOMPARE(reader.read({"PRAGMA query_only=OFF", {}}).error, StorageError::none);
        QCOMPARE(reader.read({"INSERT INTO counters VALUES (2, 20)", {}}).error,
                 StorageError::sql_failure);
        QCOMPARE(reader.read({"SELECT COUNT(*) FROM counters", {}}).rows, SqlRows({{0}}));
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM counters").toInt(), 0);
        QCOMPARE(reader.close(), StorageError::none);
    }

    void readerIsOwnedByItsOpeningThread() {
        SqliteReader reader;
        QCOMPARE(reader.open(path_), StorageError::none);
        StorageError read_error = StorageError::none, close_error = StorageError::none;
        std::thread foreign([&] {
            read_error = reader.read({"SELECT COUNT(*) FROM counters", {}}).error;
            close_error = reader.close();
        });
        foreign.join();
        QCOMPARE(read_error, StorageError::wrong_thread);
        QCOMPARE(close_error, StorageError::wrong_thread);
        QCOMPARE(reader.read({"SELECT COUNT(*) FROM counters", {}}).rows, SqlRows({{0}}));
        QCOMPARE(reader.close(), StorageError::none);
    }

    void writerRejectsForeignThreadBeforeAdmission() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        StorageError enqueue_error = StorageError::none, shutdown_error = StorageError::none;
        StorageError open_error = StorageError::none;
        bool invoked = false;
        std::thread foreign([&] {
            enqueue_error = writer.enqueue(insert(1, 10), [&](WriteResult) { invoked = true; });
            shutdown_error = writer.shutdown([&] { invoked = true; });
            open_error = writer.open(path_);
        });
        foreign.join();
        QCOMPARE(enqueue_error, StorageError::wrong_thread);
        QCOMPARE(shutdown_error, StorageError::wrong_thread);
        QCOMPARE(open_error, StorageError::wrong_thread);
        bool completed = false;
        QCOMPARE(writer.enqueue(insert(2, 20), [&](WriteResult) { completed = true; }),
                 StorageError::none);
        QTRY_VERIFY(completed);
        QVERIFY(!invoked);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM counters").toInt(), 20);
    }

    void validatesLimitsAndBatchBeforeAdmission() {
        SqliteWriter writer;
        auto limits = StorageLimits{};
        limits.pending_batches = 0;
        QCOMPARE(writer.open(path_, limits), StorageError::invalid_options);
        QCOMPARE(writer.open(""), StorageError::invalid_options);
        QCOMPARE(writer.open(":memory:"), StorageError::invalid_options);
        limits = {};
        limits.statements_per_batch = 1;
        limits.statement_utf8_bytes = 100;
        limits.bound_value_bytes = 16;
        QCOMPARE(writer.open(path_, limits), StorageError::none);
        bool invoked = false;
        auto callback = [&](WriteResult) { invoked = true; };
        QCOMPARE(writer.enqueue({}, callback), StorageError::invalid_batch);
        QCOMPARE(writer.enqueue(insert(1, 10), {}), StorageError::invalid_batch);
        QCOMPARE(writer.enqueue({{"SELECT 1", {}}, {"SELECT 2", {}}}, callback),
                 StorageError::invalid_batch);
        const QString exact_sql = QString("SELECT ?") + QString(92, ' '); // Exactly100 UTF-8 bytes.
        QCOMPARE(writer.enqueue({{exact_sql + " ", {QString("abcdefgh")}}}, callback),
                 StorageError::invalid_batch);
        QCOMPARE(writer.enqueue({{"SELECT ?", {QString::fromUtf8("ééééé")}}}, callback),
                 StorageError::invalid_batch); // Base8 plus UTF-8 payload10 exceeds16.
        QCOMPARE(writer.enqueue({{"SELECT ?", {QDate(2026, 11, 1)}}}, callback),
                 StorageError::invalid_batch);
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM counters").toInt(), 0);
        QVERIFY(!invoked);
        WriteResult result;
        QCOMPARE(writer.enqueue({{exact_sql, {QString("abcdefgh")}}},
                                [&](WriteResult value) {
                                    result = std::move(value);
                                    invoked = true;
                                }),
                 StorageError::none); // At SQL100 and value(8+8) boundaries, valid SQL is admitted.
        QTRY_VERIFY(invoked);
        QCOMPARE(result.error, StorageError::none);
        QCOMPARE(result.rows, QList<SqlRows>({{{QString("abcdefgh")}}}));
    }

    void pendingCapacityRejectsQueuedOverflow() {
        SqliteWriter writer;
        auto limits = StorageLimits{};
        limits.pending_batches = 2;
        QCOMPARE(writer.open(path_, limits), StorageError::none);
        int completions = 0;
        bool rejected_called = false;
        QCOMPARE(writer.enqueue(insert(1, 10), [&](WriteResult) { ++completions; }),
                 StorageError::none);
        QCOMPARE(writer.enqueue(insert(2, 20), [&](WriteResult) { ++completions; }),
                 StorageError::none);
        QCOMPARE(writer.enqueue(insert(3, 30), [&](WriteResult) { rejected_called = true; }),
                 StorageError::queue_full);
        QTRY_COMPARE(completions, 2);
        QVERIFY(!rejected_called);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM counters").toInt(), 30);
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM counters").toInt(), 2);
    }

    void rollsBackWholeBatchOnSqlFailure() {
        QVERIFY(fixture_->execute("INSERT INTO counters VALUES (1, 10)"));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool completed = false;
        WriteResult result;
        QCOMPARE(writer.enqueue({{"INSERT INTO counters VALUES (2, 20)", {}},
                                 {"INSERT INTO counters VALUES (1, 99)", {}}},
                                [&](WriteResult value) {
                                    result = std::move(value);
                                    completed = true;
                                }),
                 StorageError::none);
        QTRY_VERIFY(completed);
        QCOMPARE(result.error, StorageError::sql_failure);
        QVERIFY(result.rows.isEmpty());
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM counters").toInt(), 1);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM counters").toInt(), 10);
        completed = false;
        QCOMPARE(writer.enqueue(insert(3, 30),
                                [&](WriteResult value) {
                                    result = std::move(value);
                                    completed = true;
                                }),
                 StorageError::none);
        QTRY_VERIFY(completed);
        QCOMPARE(result.error, StorageError::none);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM counters").toInt(), 40);
    }

    void busyBeginFailsWithoutReplay() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QVERIFY(fixture_->execute("BEGIN IMMEDIATE"));
        int completions = 0;
        WriteResult result;
        QCOMPARE(writer.enqueue(insert(4, 40),
                                [&](WriteResult value) {
                                    result = std::move(value);
                                    ++completions;
                                }),
                 StorageError::none);
        QTRY_COMPARE(completions,
                     1); // Lock remains held; timeout/retry cannot hide behind release.
        QCOMPARE(result.error, StorageError::busy);
        QVERIFY(result.rows.isEmpty());
        QVERIFY(fixture_->execute("ROLLBACK"));
        bool stopped = false;
        QCOMPARE(writer.enqueue(insert(5, 50),
                                [&](WriteResult value) {
                                    result = std::move(value);
                                    ++completions;
                                }),
                 StorageError::none);
        QCOMPARE(writer.shutdown([&] { stopped = true; }), StorageError::none);
        QTRY_VERIFY(stopped);
        QCOMPARE(completions, 2);
        QCOMPARE(result.error, StorageError::none);
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM counters").toInt(), 1);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM counters").toInt(), 50);
    }

    void readerSnapshotDoesNotBlockWalCommit() {
        QVERIFY(fixture_->execute("INSERT INTO counters VALUES (1, 10)"));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        SqliteReader reader;
        QCOMPARE(reader.open(path_), StorageError::none);
        QCOMPARE(reader.read({"BEGIN", {}}).error, StorageError::none);
        QCOMPARE(reader.read({"SELECT SUM(amount) FROM counters", {}}).rows, SqlRows({{10}}));
        bool completed = false;
        WriteResult result;
        QCOMPARE(writer.enqueue(insert(2, 20),
                                [&](WriteResult value) {
                                    result = std::move(value);
                                    completed = true;
                                }),
                 StorageError::none);
        QTRY_VERIFY(completed);
        QCOMPARE(result.error, StorageError::none);
        QCOMPARE(reader.read({"SELECT SUM(amount) FROM counters", {}}).rows, SqlRows({{10}}));
        QCOMPARE(reader.read({"COMMIT", {}}).error, StorageError::none);
        QCOMPARE(reader.read({"SELECT SUM(amount) FROM counters", {}}).rows, SqlRows({{30}}));
        QCOMPARE(reader.close(), StorageError::none);
    }

    void shutdownDrainsAcceptedWorkAndRejectsNewWork() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QList<int> order;
        QList<StorageError> errors;
        QCOMPARE(writer.enqueue(insert(1, 10),
                                [&](WriteResult result) {
                                    errors.append(result.error);
                                    order.append(1);
                                }),
                 StorageError::none);
        QCOMPARE(writer.enqueue(insert(2, 20),
                                [&](WriteResult result) {
                                    errors.append(result.error);
                                    order.append(2);
                                }),
                 StorageError::none);
        QCOMPARE(writer.shutdown([&] { order.append(3); }), StorageError::none);
        bool rejected_called = false;
        QCOMPARE(writer.enqueue(insert(3, 30), [&](WriteResult) { rejected_called = true; }),
                 StorageError::shutting_down);
        QVERIFY(order.isEmpty());
        QTRY_COMPARE(order.size(), 3);
        QCOMPARE(order, QList<int>({1, 2, 3}));
        QCOMPARE(errors, QList<StorageError>({StorageError::none, StorageError::none}));
        QVERIFY(!rejected_called);
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM counters").toInt(), 2);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM counters").toInt(), 30);
    }

    void shutdownIsIdempotentAndReleasesConnection() {
        const auto initial_connections = connection_names();
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        int completions = 0;
        QCOMPARE(writer.shutdown([&] { ++completions; }), StorageError::none);
        QCOMPARE(writer.shutdown([&] { ++completions; }), StorageError::none);
        QCOMPARE(completions, 0);
        QTRY_COMPARE(completions, 2);
        QCOMPARE(connection_names(), initial_connections);
        QCOMPARE(writer.open(path_), StorageError::shutting_down);
        QCOMPARE(writer.shutdown([&] { ++completions; }), StorageError::none);
        QTRY_COMPARE(completions, 3);
        QCOMPARE(connection_names(), initial_connections);
    }

    void closedLifecycleIsExplicit() {
        SqliteWriter writer;
        QCOMPARE(writer.enqueue(insert(1, 10), [](WriteResult) {}), StorageError::not_open);
        QCOMPARE(writer.shutdown({}), StorageError::invalid_options);
        bool stopped = false;
        QCOMPARE(writer.shutdown([&] { stopped = true; }), StorageError::none);
        QVERIFY(!stopped);
        QTRY_VERIFY(stopped);
        QCOMPARE(writer.open(path_), StorageError::shutting_down);
        QCOMPARE(writer.enqueue(insert(1, 10), [](WriteResult) {}), StorageError::shutting_down);
        SqliteReader reader;
        QCOMPARE(reader.read({"SELECT 1", {}}).error, StorageError::not_open);
        QCOMPARE(reader.open(path_), StorageError::none);
        QCOMPARE(reader.open(path_), StorageError::writer_already_open);
        QCOMPARE(reader.close(), StorageError::none);
        QCOMPARE(reader.close(), StorageError::none);
        QCOMPARE(reader.open(path_), StorageError::none);
    }

    void readerMissingFileDoesNotCreateDatabase() {
        SqliteReader reader;
        const QString missing = directory_->filePath("missing.sqlite");
        QCOMPARE(reader.open(missing), StorageError::open_failed);
        QVERIFY(!QFileInfo::exists(missing));
        QCOMPARE(reader.open(path_), StorageError::none);
    }

    void failedOpenDoesNotPublishAWriter() {
        const auto initial_connections = connection_names();
        SqliteWriter writer;
        QCOMPARE(writer.open(directory_->path()), StorageError::open_failed);
        QCOMPARE(connection_names(), initial_connections);
        QCOMPARE(writer.open(path_), StorageError::none);
    }

    void completionCanEnqueueAfterCommit() {
        SqliteWriter writer;
        auto limits = StorageLimits{};
        limits.pending_batches = 1;
        QCOMPARE(writer.open(path_, limits), StorageError::none);
        int completed = 0;
        StorageError nested_admission = StorageError::not_open;
        StorageError first_error = StorageError::not_open, second_error = StorageError::not_open;
        QCOMPARE(writer.enqueue(insert(1, 10),
                                [&](WriteResult result) {
                                    first_error = result.error;
                                    ++completed;
                                    nested_admission =
                                        writer.enqueue(insert(2, 20), [&](WriteResult value) {
                                            second_error = value.error;
                                            ++completed;
                                        });
                                }),
                 StorageError::none);
        QTRY_COMPARE(completed, 2);
        QCOMPARE(nested_admission, StorageError::none);
        QCOMPARE(first_error, StorageError::none);
        QCOMPARE(second_error, StorageError::none);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM counters").toInt(), 30);
    }

    void rejectsTransactionControlStatements() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool invoked = false;
        for (const QString &sql :
             {QString("COMMIT"), QString("BEGIN"), QString("ROLLBACK"), QString("SAVEPOINT x"),
              QString("END"), QString("PRAGMA synchronous=OFF"),
              QString("ATTACH ':memory:' AS extra"),
              QString("INSERT INTO counters VALUES (2,20); COMMIT")}) {
            QCOMPARE(writer.enqueue({{"INSERT INTO counters VALUES (1,10)", {}}, {sql, {}}},
                                    [&](WriteResult) { invoked = true; }),
                     StorageError::invalid_batch);
        }
        QVERIFY(!invoked);
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM counters").toInt(), 0);
    }

    void completionExceptionDoesNotUndoCommitOrStopQueue() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        int first_calls = 0, second_calls = 0;
        StorageError second_error = StorageError::not_open;
        QCOMPARE(writer.enqueue(insert(1, 10),
                                [&](WriteResult) {
                                    ++first_calls;
                                    throw std::runtime_error(
                                        "synthetic completion failure after commit");
                                }),
                 StorageError::none);
        QCOMPARE(writer.enqueue(insert(2, 20),
                                [&](WriteResult result) {
                                    second_error = result.error;
                                    ++second_calls;
                                }),
                 StorageError::none);
        QTRY_COMPARE(second_calls, 1);
        QCOMPARE(first_calls, 1);
        QCOMPARE(second_error, StorageError::none);
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM counters").toInt(), 2);
        QCOMPARE(fixture_->scalar("SELECT SUM(amount) FROM counters").toInt(), 30);
    }

    void boundsResultRowsAndRollsBack() {
        SqliteWriter writer;
        auto limits = StorageLimits{};
        limits.result_rows = 1;
        QCOMPARE(writer.open(path_, limits), StorageError::none);
        bool completed = false;
        WriteResult result;
        QCOMPARE(writer.enqueue({{"INSERT INTO counters VALUES (1,10)", {}},
                                 {"SELECT 1 UNION ALL SELECT 2", {}}},
                                [&](WriteResult value) {
                                    result = std::move(value);
                                    completed = true;
                                }),
                 StorageError::none);
        QTRY_VERIFY(completed);
        QCOMPARE(result.error, StorageError::result_limit);
        QVERIFY(result.rows.isEmpty());
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM counters").toInt(), 0);
    }

    void boundsResultBytesAndRollsBack() {
        SqliteWriter writer;
        auto limits = StorageLimits{};
        limits.result_value_bytes = 16;
        QCOMPARE(writer.open(path_, limits), StorageError::none);
        bool completed = false;
        WriteResult result;
        QCOMPARE(writer.enqueue({{"INSERT INTO counters VALUES (1,10)", {}},
                                 {"SELECT ?", {QString::fromUtf8("ééééé")}}},
                                [&](WriteResult value) {
                                    result = std::move(value);
                                    completed = true;
                                }),
                 StorageError::none);
        QTRY_VERIFY(completed);
        QCOMPARE(result.error, StorageError::result_limit);
        QVERIFY(result.rows.isEmpty());
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM counters").toInt(), 0);
    }

    void failedCommitRollsBack() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool completed = false;
        WriteResult result;
        QCOMPARE(writer.enqueue({{"INSERT INTO children VALUES (1,99)", {}}},
                                [&](WriteResult value) {
                                    result = std::move(value);
                                    completed = true;
                                }),
                 StorageError::none);
        QTRY_VERIFY(completed);
        QCOMPARE(result.error, StorageError::sql_failure);
        QVERIFY(result.rows.isEmpty());
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM children").toInt(), 0);
        completed = false;
        QCOMPARE(writer.enqueue({{"INSERT INTO parents VALUES (99)", {}},
                                 {"INSERT INTO children VALUES (2,99)", {}}},
                                [&](WriteResult value) {
                                    result = std::move(value);
                                    completed = true;
                                }),
                 StorageError::none);
        QTRY_VERIFY(completed);
        QCOMPARE(result.error, StorageError::none);
        QCOMPARE(fixture_->scalar("SELECT COUNT(*) FROM children").toInt(), 1);
        QCOMPARE(fixture_->scalar("SELECT id FROM children").toInt(), 2);
    }
};

QTEST_GUILESS_MAIN(SqliteWriterTests)
#include "sqlite_writer_tests.moc"
