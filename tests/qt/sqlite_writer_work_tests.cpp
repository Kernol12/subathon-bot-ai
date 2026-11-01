// SPDX-License-Identifier: GPL-3.0-or-later
#include <QDate>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <memory>
#include <stdexcept>
#include <subathon/server/receipt_intake.hpp>
#include <subathon/server/sqlite_writer.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <thread>
#include <type_traits>

using namespace subathon::server;
using subathon::server::testing::SqliteWriterWorkAccess;
using subathon::server::testing::WorkControlFault;

namespace {
// Independent fixture SQL only: never executes the work/session algorithm.
class Observer final {
  public:
    explicit Observer(const QString &path)
        : name_(QUuid::createUuid().toString()), db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
    }
    ~Observer() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }
    bool open() { return db_.open(); }
    bool exec(const QString &sql) {
        QSqlQuery query(db_);
        return query.exec(sql);
    }
    // A zero-wait independent write-lock probe proves rollback completed before
    // a synchronous failure escaped; invisibility of uncommitted rows cannot.
    bool write_lock_is_released() { return exec("BEGIN IMMEDIATE") && exec("ROLLBACK"); }
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

SqlStatement put(qint64 id, qint64 amount) {
    return {"INSERT INTO counters(id, amount) VALUES (?, ?)", {id, amount}};
}

template <typename Operation> WorkError failure(Operation operation) {
    try {
        operation();
        return WorkError::none;
    } catch (const SqliteWorkFailure &error) {
        return error.error();
    }
}

void check_result(const SqliteWorkResult &result, TransactionOutcome transaction, WorkError error,
                  bool value = false) {
    QCOMPARE(result.transaction, transaction);
    QCOMPARE(result.error, error);
    QCOMPARE(result.value.has_value(), value);
}

// A deliberately tiny synthetic ReceiptRepository projection. The REAL intake
// service supplies its call ordering; the REAL session performs all transactions.
// These two generic rows do not purport to encode receipt facts or a final schema.
class PrimitiveReceiptTransaction final : public ReceiptTransaction {
  public:
    explicit PrimitiveReceiptTransaction(SqliteWorkSession session)
        : session_(std::move(session)) {}
    ~PrimitiveReceiptTransaction() override { (void)session_.cleanup(); }
    std::optional<subathon::core::ReceiptRecord>
    find(const subathon::core::ReceiptIdentity &) override {
        (void)known([&] { return session_.execute({"SELECT id FROM counters WHERE id=101", {}}); });
        return std::nullopt; // This fixture has exactly one fresh service invocation.
    }
    subathon::core::ReceiptId create(const subathon::core::ReceiptIdentity &,
                                     const subathon::core::ReceiptFacts &,
                                     const subathon::core::ReceiptArrival &) override {
        (void)known([&] { return session_.execute(put(101, 10)); });
        return {101};
    }
    void append_attempt(subathon::core::ReceiptId,
                        const subathon::core::ReceiptAttempt &) override {
        (void)known([&] { return session_.execute(put(102, 20)); });
    }
    void commit() override {
        known([&] {
            session_.commit();
            return 0;
        });
    }

  private:
    // Only failures whose session guarantee establishes known rollback translate
    // to the existing port category. Unknown outcomes cross the port distinctly.
    template <typename Operation> static auto known(Operation operation) -> decltype(operation()) {
        try {
            return operation();
        } catch (const SqliteWorkFailure &error) {
            switch (error.error()) {
            case WorkError::busy:
            case WorkError::sql_failure:
            case WorkError::resource_limit:
            case WorkError::result_limit:
            case WorkError::invalid_statement:
                throw ReceiptStorageError("synthetic primitive: known uncommitted");
            default:
                throw;
            }
        }
    }
    SqliteWorkSession session_;
};

class PrimitiveReceiptRepository final : public ReceiptRepository {
  public:
    explicit PrimitiveReceiptRepository(SqliteWorkContext context) : context_(std::move(context)) {}
    std::unique_ptr<ReceiptTransaction> begin() override {
        return std::make_unique<PrimitiveReceiptTransaction>(context_.begin());
    }

  private:
    SqliteWorkContext context_;
};

subathon::core::ReceiptInput fresh_receipt() {
    return {{{"synthetic", "test-account"}, "test-delivery", "one"},
            {"test", 1, {}, {}, {}, {}, {}, {}},
            {1000, "test-connection", "config-v1", "modifiers-v1"}};
}
} // namespace

class SqliteWriterWorkTests final : public QObject {
    Q_OBJECT
  private:
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<Observer> observer_;
    QString path_;

  private slots:
    void initTestCase() {
        QVERIFY(QSqlDatabase::isDriverAvailable("QSQLITE"));
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }
    void init() {
        directory_ = std::make_unique<QTemporaryDir>();
        QVERIFY(directory_->isValid());
        path_ = directory_->filePath("work.sqlite");
        observer_ = std::make_unique<Observer>(path_);
        QVERIFY(observer_->open());
        QVERIFY(observer_->exec("CREATE TABLE counters(id INTEGER PRIMARY KEY, amount INTEGER)"));
        QVERIFY(observer_->exec("CREATE TABLE parents(id INTEGER PRIMARY KEY)"));
        QVERIFY(observer_->exec("CREATE TABLE children(id INTEGER PRIMARY KEY, parent_id INTEGER "
                                "REFERENCES parents(id) DEFERRABLE INITIALLY DEFERRED)"));
    }
    void cleanup() {
        observer_.reset();
        directory_.reset();
    }

    // WT01: one FIFO includes both existing batches and dynamic work.
    void mixedBatchAndWorkAreFifo() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QList<int> order;
        SqliteWorkResult work;
        WriteResult first, last;
        qint64 inside = -1;
        QCOMPARE(writer.enqueue({put(1, 10)},
                                [&](WriteResult r) {
                                    first = std::move(r);
                                    order.append(1);
                                }),
                 StorageError::none);
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         auto session = context.begin();
                         const auto rows =
                             session.execute({"SELECT SUM(amount) FROM counters", {}});
                         inside = rows.at(0).at(0).toLongLong();
                         (void)session.execute(put(2, 20));
                         session.commit();
                         if (order != QList<int>{1})
                             throw std::runtime_error("next item entered before body returned");
                         return QVariant(30);
                     },
                     [&](SqliteWorkResult r) {
                         work = std::move(r);
                         order.append(2);
                     }),
                 StorageError::none);
        QCOMPARE(writer.enqueue({{"SELECT SUM(amount) FROM counters", {}}},
                                [&](WriteResult r) {
                                    last = std::move(r);
                                    order.append(3);
                                }),
                 StorageError::none);
        QVERIFY(order.isEmpty());
        QTRY_COMPARE(order.size(), 3);
        QCOMPARE(order, QList<int>({1, 2, 3}));
        QCOMPARE(first.error, StorageError::none);
        QCOMPARE(last.error, StorageError::none);
        QCOMPARE(inside, 10);
        QCOMPARE(last.rows, QList<SqlRows>({{{30}}}));
        check_result(work, TransactionOutcome::committed, WorkError::none, true);
    }

    // WT02: the branch depends on an actual in-transaction read, not fixture data.
    void dynamicReadDecideWriteIsAtomic() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool done = false;
        qint64 before_commit = -1, after_commit = -1;
        SqlRows rows;
        SqliteWorkResult result;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         auto session = context.begin();
                         (void)session.execute(put(1, 10));
                         rows = session.execute({"SELECT amount FROM counters WHERE id=1", {}});
                         if (rows == SqlRows{{10}})
                             (void)session.execute(put(2, 20));
                         before_commit =
                             observer_->scalar("SELECT COUNT(*) FROM counters").toLongLong();
                         session.commit();
                         after_commit =
                             observer_->scalar("SELECT COUNT(*) FROM counters").toLongLong();
                         return QVariant(30);
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         done = true;
                     }),
                 StorageError::none);
        QTRY_VERIFY(done);
        check_result(result, TransactionOutcome::committed, WorkError::none, true);
        QCOMPARE(rows, SqlRows({{10}}));
        QCOMPARE(before_commit, 0);
        QCOMPARE(after_commit, 2);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 30);
    }

    // WT03: actual synchronous ReceiptIntake::ingest and primitive port commit.
    void serviceCommitIsVisibleBeforeBodyReturns() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool done = false, service_returned = false, completion_was_pending = false;
        qint64 visible_inside = -1;
        SqliteWorkResult result;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         PrimitiveReceiptRepository repository(context);
                         ReceiptIntake intake(repository);
                         const auto receipt = intake.ingest(fresh_receipt());
                         service_returned =
                             receipt.receipt_id.value == 101 &&
                             receipt.disposition == subathon::core::ReceiptDisposition::created;
                         visible_inside =
                             observer_->scalar("SELECT SUM(amount) FROM counters").toLongLong();
                         completion_was_pending = !done;
                         return QVariant("service-returned");
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         done = true;
                     }),
                 StorageError::none);
        QTRY_VERIFY(done);
        QVERIFY(service_returned);
        QVERIFY(completion_was_pending);
        QCOMPARE(visible_inside, 30);
        check_result(result, TransactionOutcome::committed, WorkError::none, true);
    }

    void returnWithoutCommitRollsBack() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        for (const bool explicit_rollback : {false, true}) {
            bool done = false;
            SqliteWorkResult result;
            QCOMPARE(writer.enqueue_work(
                         [explicit_rollback](SqliteWorkContext context) {
                             auto session = context.begin();
                             (void)session.execute(put(1, 10));
                             if (explicit_rollback)
                                 session.rollback();
                             return QVariant("uncommitted");
                         },
                         [&](SqliteWorkResult r) {
                             result = std::move(r);
                             done = true;
                         }),
                     StorageError::none);
            QTRY_VERIFY(done);
            check_result(result, TransactionOutcome::rolled_back,
                         explicit_rollback ? WorkError::not_committed : WorkError::uncommitted);
            QCOMPARE(observer_->scalar("SELECT COUNT(*) FROM counters").toInt(), 0);
        }
    }

    void exceptionBeforeCommitRollsBackAndQueueContinues() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QList<SqliteWorkResult> results;
        int calls = 0;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) -> QVariant {
                         ++calls;
                         auto session = context.begin();
                         (void)session.execute(put(1, 10));
                         throw std::runtime_error("synthetic body failure before commit");
                     },
                     [&](SqliteWorkResult r) { results.append(std::move(r)); }),
                 StorageError::none);
        QCOMPARE(writer.enqueue_work(
                     [](SqliteWorkContext context) {
                         auto session = context.begin();
                         (void)session.execute(put(2, 20));
                         session.commit();
                         return QVariant(20);
                     },
                     [&](SqliteWorkResult r) { results.append(std::move(r)); }),
                 StorageError::none);
        QTRY_COMPARE(results.size(), 2);
        check_result(results[0], TransactionOutcome::rolled_back, WorkError::work_exception);
        check_result(results[1], TransactionOutcome::committed, WorkError::none, true);
        QCOMPARE(calls, 1);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 20);
    }

    // WT05: a caught failure cannot turn a rolled-back transaction into success.
    void caughtSqlErrorStaysStickyAndCannotCommit() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool done = false;
        WorkError sql = WorkError::none, commit = WorkError::none, begin = WorkError::none;
        qint64 count_at_throw = -1;
        bool rollback_released_lock = false;
        SqliteWorkResult result;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) -> QVariant {
                         auto session = context.begin();
                         (void)session.execute(put(1, 10));
                         sql = failure([&] { (void)session.execute(put(1, 999)); });
                         count_at_throw =
                             observer_->scalar("SELECT COUNT(*) FROM counters").toLongLong();
                         rollback_released_lock = observer_->write_lock_is_released();
                         commit = failure([&] { session.commit(); });
                         begin = failure([&] { (void)context.begin(); });
                         throw std::runtime_error("later body error must not replace SQL error");
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         done = true;
                     }),
                 StorageError::none);
        QTRY_VERIFY(done);
        QCOMPARE(sql, WorkError::sql_failure);
        QCOMPARE(commit, WorkError::session_closed);
        QCOMPARE(begin, WorkError::protocol_error);
        QCOMPARE(count_at_throw, 0);
        QVERIFY(rollback_released_lock);
        check_result(result, TransactionOutcome::rolled_back, WorkError::sql_failure);
    }

    // WT06: completion happens while the external lock is still held.
    void busyBeginDoesNotReplay() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QVERIFY(observer_->exec("BEGIN IMMEDIATE"));
        int calls = 0;
        bool done = false, next_done = false;
        SqliteWorkResult first, next;
        WorkError busy = WorkError::none, second_begin = WorkError::none;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         ++calls;
                         busy = failure([&] { (void)context.begin(); });
                         second_begin = failure([&] { (void)context.begin(); });
                         return QVariant("busy attempt caught");
                     },
                     [&](SqliteWorkResult r) {
                         first = std::move(r);
                         done = true;
                     }),
                 StorageError::none);
        QTRY_VERIFY(done);
        check_result(first, TransactionOutcome::not_started, WorkError::busy);
        QCOMPARE(busy, WorkError::busy);
        QCOMPARE(second_begin, WorkError::protocol_error);
        QCOMPARE(calls, 1);
        QVERIFY(observer_->exec("ROLLBACK"));
        QCOMPARE(writer.enqueue_work(
                     [](SqliteWorkContext context) {
                         auto session = context.begin();
                         (void)session.execute(put(2, 20));
                         session.commit();
                         return QVariant(20);
                     },
                     [&](SqliteWorkResult r) {
                         next = std::move(r);
                         next_done = true;
                     }),
                 StorageError::none);
        QTRY_VERIFY(next_done);
        QCOMPARE(calls, 1);
        check_result(next, TransactionOutcome::committed, WorkError::none, true);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 20);
    }

    void failedCommitRollsBackBeforeThrowing() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool done = false, commit_returned = false;
        WorkError error = WorkError::none;
        qint64 visible_at_throw = -1;
        bool rollback_released_lock = false;
        SqliteWorkResult result;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         auto session = context.begin();
                         (void)session.execute({"INSERT INTO children VALUES (1, 404)", {}});
                         error = failure([&] {
                             session.commit();
                             commit_returned = true;
                         });
                         visible_at_throw =
                             observer_->scalar("SELECT COUNT(*) FROM children").toLongLong();
                         rollback_released_lock = observer_->write_lock_is_released();
                         return QVariant("caught failed commit");
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         done = true;
                     }),
                 StorageError::none);
        QTRY_VERIFY(done);
        QVERIFY(!commit_returned);
        QCOMPARE(error, WorkError::sql_failure);
        QCOMPARE(visible_at_throw, 0);
        QVERIFY(rollback_released_lock);
        check_result(result, TransactionOutcome::rolled_back, WorkError::sql_failure);
        bool healthy_done = false;
        QCOMPARE(writer.enqueue({put(2, 20)},
                                [&](WriteResult r) {
                                    QCOMPARE(r.error, StorageError::none);
                                    healthy_done = true;
                                }),
                 StorageError::none);
        QTRY_VERIFY(healthy_done);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 20);
    }

    void exceptionAfterCommitPreservesCommittedOutcome() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool done = false, next_done = false;
        int calls = 0;
        SqliteWorkResult result;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) -> QVariant {
                         ++calls;
                         auto session = context.begin();
                         (void)session.execute(put(1, 10));
                         session.commit();
                         throw std::runtime_error("postcommit failure");
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         done = true;
                     }),
                 StorageError::none);
        QCOMPARE(writer.enqueue({put(2, 20)},
                                [&](WriteResult r) {
                                    QCOMPARE(r.error, StorageError::none);
                                    next_done = true;
                                }),
                 StorageError::none);
        QTRY_VERIFY(done && next_done);
        check_result(result, TransactionOutcome::committed, WorkError::work_exception);
        QCOMPARE(calls, 1);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 30);
    }

    // WT09: physical persistence is checked independently of unknown classification.
    void lostCommitAcknowledgmentFailsClosed() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_commit_ack),
                 StorageError::none);
        QList<int> order;
        int body_calls = 0, queued_body_calls = 0;
        WriteResult before, after;
        SqliteWorkResult result, queued;
        QCOMPARE(writer.enqueue({put(1, 10)},
                                [&](WriteResult r) {
                                    before = std::move(r);
                                    order.append(1);
                                }),
                 StorageError::none);
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         ++body_calls;
                         auto session = context.begin();
                         (void)session.execute(put(2, 20));
                         session.commit();
                         return QVariant(20);
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         order.append(2);
                     }),
                 StorageError::none);
        QCOMPARE(writer.enqueue({put(3, 30)},
                                [&](WriteResult r) {
                                    after = std::move(r);
                                    order.append(3);
                                }),
                 StorageError::none);
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext) {
                         ++queued_body_calls;
                         return QVariant(99);
                     },
                     [&](SqliteWorkResult r) {
                         queued = std::move(r);
                         order.append(4);
                     }),
                 StorageError::none);
        QTRY_COMPARE(order.size(), 4);
        QCOMPARE(order, QList<int>({1, 2, 3, 4}));
        QCOMPARE(before.error, StorageError::none);
        QCOMPARE(after.error, StorageError::sql_failure);
        QVERIFY(after.rows.isEmpty());
        check_result(result, TransactionOutcome::outcome_unknown, WorkError::uncertain_outcome);
        check_result(queued, TransactionOutcome::not_started, WorkError::writer_unavailable);
        QCOMPARE(body_calls, 1);
        QCOMPARE(queued_body_calls, 0);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 30);
        QCOMPARE(writer.enqueue({put(4, 40)}, [](WriteResult) {}), StorageError::shutting_down);
        QCOMPARE(writer.enqueue_work([](SqliteWorkContext) { return QVariant(); },
                                     [](SqliteWorkResult) {}),
                 StorageError::shutting_down);
        QCOMPARE(writer.open(path_), StorageError::shutting_down);
    }

    void lostRollbackAcknowledgmentFailsClosed() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_rollback_ack),
                 StorageError::none);
        QList<int> order;
        SqliteWorkResult result, queued;
        WriteResult batch;
        int queued_calls = 0;
        WorkError at_rollback = WorkError::none;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         auto session = context.begin();
                         (void)session.execute(put(1, 10));
                         at_rollback = failure([&] { session.rollback(); });
                         return QVariant("caught uncertainty");
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         order.append(1);
                     }),
                 StorageError::none);
        QCOMPARE(writer.enqueue({put(2, 20)},
                                [&](WriteResult r) {
                                    batch = std::move(r);
                                    order.append(2);
                                }),
                 StorageError::none);
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext) {
                         ++queued_calls;
                         return QVariant();
                     },
                     [&](SqliteWorkResult r) {
                         queued = std::move(r);
                         order.append(3);
                     }),
                 StorageError::none);
        QTRY_COMPARE(order.size(), 3);
        QCOMPARE(order, QList<int>({1, 2, 3}));
        QCOMPARE(at_rollback, WorkError::uncertain_outcome);
        check_result(result, TransactionOutcome::outcome_unknown, WorkError::uncertain_outcome);
        check_result(queued, TransactionOutcome::not_started, WorkError::writer_unavailable);
        QCOMPARE(batch.error, StorageError::sql_failure);
        QVERIFY(batch.rows.isEmpty());
        QCOMPARE(queued_calls, 0);
        QCOMPARE(observer_->scalar("SELECT COUNT(*) FROM counters").toInt(), 0);
        QCOMPARE(writer.enqueue({put(3, 30)}, [](WriteResult) {}), StorageError::shutting_down);
    }

    void retainedCapabilitiesCannotReachLaterWork() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        SqliteWorkContext retained_context;
        SqliteWorkSession retained_session;
        QList<SqliteWorkResult> results;
        WorkError closed = WorkError::none, stale_begin = WorkError::none,
                  stale_sql = WorkError::none;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         retained_context = context;
                         retained_session = context.begin();
                         (void)retained_session.execute(put(1, 10));
                         retained_session.commit();
                         closed = failure([&] { (void)retained_session.execute(put(9, 900)); });
                         return QVariant(10);
                     },
                     [&](SqliteWorkResult r) { results.append(std::move(r)); }),
                 StorageError::none);
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         auto session = context.begin();
                         (void)session.execute(put(2, 20));
                         stale_begin = failure([&] { (void)retained_context.begin(); });
                         stale_sql = failure([&] { (void)retained_session.execute(put(8, 800)); });
                         session.commit();
                         return QVariant(20);
                     },
                     [&](SqliteWorkResult r) { results.append(std::move(r)); }),
                 StorageError::none);
        QTRY_COMPARE(results.size(), 2);
        QCOMPARE(closed, WorkError::session_closed);
        QCOMPARE(stale_begin, WorkError::stale_session);
        QCOMPARE(stale_sql, WorkError::stale_session);
        check_result(results[0], TransactionOutcome::committed, WorkError::session_closed);
        check_result(results[1], TransactionOutcome::committed, WorkError::none, true);
        QCOMPARE(retained_session.cleanup(), WorkError::none);
        QCOMPARE(failure([&] { retained_session.commit(); }), WorkError::stale_session);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 30);
    }

    void capabilitiesAfterWriterDestructionAreRevoked() {
        SqliteWorkContext context_copy;
        SqliteWorkSession session_copy;
        {
            auto writer = std::make_unique<SqliteWriter>();
            QCOMPARE(writer->open(path_), StorageError::none);
            bool done = false;
            SqliteWorkResult result;
            QCOMPARE(writer->enqueue_work(
                         [&](SqliteWorkContext context) {
                             context_copy = context;
                             session_copy = context.begin();
                             (void)session_copy.execute(put(1, 10));
                             session_copy.commit();
                             return QVariant(10);
                         },
                         [&](SqliteWorkResult r) {
                             result = std::move(r);
                             done = true;
                         }),
                     StorageError::none);
            QTRY_VERIFY(done);
            check_result(result, TransactionOutcome::committed, WorkError::none, true);
        }
        QCOMPARE(failure([&] { (void)context_copy.begin(); }), WorkError::stale_session);
        QCOMPARE(failure([&] { (void)session_copy.execute(put(2, 20)); }),
                 WorkError::stale_session);
        QCOMPARE(failure([&] { session_copy.commit(); }), WorkError::stale_session);
        QCOMPARE(failure([&] { session_copy.rollback(); }), WorkError::stale_session);
        QCOMPARE(session_copy.cleanup(), WorkError::none);
        SqliteWriter replacement;
        QCOMPARE(replacement.open(path_), StorageError::none);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 10);
    }

    void foreignCapabilitiesDoNotPoisonOwner() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool done = false;
        QList<WorkError> foreign;
        SqliteWorkResult result;
        QCOMPARE(
            writer.enqueue_work(
                [&](SqliteWorkContext context) {
                    auto session = context.begin();
                    (void)session.execute(put(1, 10));
                    std::thread other([context, session, &foreign] {
                        foreign.append(failure([&] { (void)context.begin(); }));
                        foreign.append(failure([&] { (void)session.execute(put(9, 900)); }));
                        foreign.append(failure([&] { session.commit(); }));
                        foreign.append(failure([&] { session.rollback(); }));
                        foreign.append(session.cleanup());
                    });
                    other.join(); // No event pumping or owner SQL access on the foreign thread.
                    (void)session.execute(put(2, 20));
                    session.commit();
                    return QVariant(30);
                },
                [&](SqliteWorkResult r) {
                    result = std::move(r);
                    done = true;
                }),
            StorageError::none);
        QTRY_VERIFY(done);
        QCOMPARE(foreign, QList<WorkError>(5, WorkError::wrong_thread));
        check_result(result, TransactionOutcome::committed, WorkError::none, true);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 30);
    }

    void secondBeginAndSqlControlAreRejected() {
        const QList<QString> prohibited = {"COMMIT",
                                           "BEGIN IMMEDIATE",
                                           "ROLLBACK",
                                           "SAVEPOINT x",
                                           "ATTACH DATABASE ':memory:' AS other",
                                           "DETACH DATABASE other",
                                           "VACUUM",
                                           "PRAGMA foreign_keys=OFF",
                                           "PRAGMA database_list",
                                           "SELECT 1; SELECT 2"};
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        for (const auto &sql : prohibited) {
            bool done = false;
            WorkError caught = WorkError::none;
            SqliteWorkResult result;
            QCOMPARE(writer.enqueue_work(
                         [&](SqliteWorkContext context) {
                             auto session = context.begin();
                             (void)session.execute(put(1, 10));
                             caught = failure([&] { (void)session.execute({sql, {}}); });
                             return QVariant("rejected");
                         },
                         [&](SqliteWorkResult r) {
                             result = std::move(r);
                             done = true;
                         }),
                     StorageError::none);
            QTRY_VERIFY(done);
            QCOMPARE(caught, WorkError::invalid_statement);
            check_result(result, TransactionOutcome::rolled_back, WorkError::invalid_statement);
            QCOMPARE(observer_->scalar("SELECT COUNT(*) FROM counters").toInt(), 0);
        }
        // Second begin is forbidden even after explicit rollback, not just while
        // active or after commit. Each phase is independently observable on disk.
        for (const int terminal_phase : {0, 1, 2}) {
            QVERIFY(observer_->exec("DELETE FROM counters"));
            bool done = false;
            WorkError caught = WorkError::none;
            SqliteWorkResult result;
            QCOMPARE(writer.enqueue_work(
                         [&](SqliteWorkContext context) {
                             auto session = context.begin();
                             (void)session.execute(put(1, 10));
                             if (terminal_phase == 1)
                                 session.commit();
                             if (terminal_phase == 2)
                                 session.rollback();
                             caught = failure([&] { (void)context.begin(); });
                             return QVariant("second begin");
                         },
                         [&](SqliteWorkResult r) {
                             result = std::move(r);
                             done = true;
                         }),
                     StorageError::none);
            QTRY_VERIFY(done);
            QCOMPARE(caught, WorkError::protocol_error);
            check_result(result,
                         terminal_phase == 1 ? TransactionOutcome::committed
                                             : TransactionOutcome::rolled_back,
                         WorkError::protocol_error);
            QCOMPARE(observer_->scalar("SELECT COUNT(*) FROM counters").toInt(),
                     terminal_phase == 1 ? 1 : 0);
        }
    }

    // WT14: each overage occurs after an earlier speculative write. Arithmetic is
    // literal: 8+UTF8(é)=10, 8+blob(7)=15, scalar/null=8; 10+15=25.
    void cumulativeBudgetsHaveExactBoundaries() {
        enum class Budget { statements, sql_bytes, bindings, rows, result_bytes, scalars };
        for (const auto budget : {Budget::statements, Budget::sql_bytes, Budget::bindings,
                                  Budget::rows, Budget::result_bytes, Budget::scalars}) {
            for (const bool over : {false, true}) {
                QVERIFY(observer_->exec("DELETE FROM counters"));
                StorageLimits limits;
                switch (budget) {
                case Budget::statements:
                    limits.statements_per_batch = 3;
                    break;
                case Budget::sql_bytes:
                    limits.statement_utf8_bytes = 48;
                    break;
                case Budget::bindings:
                    limits.bound_value_bytes = 25;
                    break;
                case Budget::rows:
                    limits.result_rows = 2;
                    break;
                case Budget::result_bytes:
                    limits.result_value_bytes = 25;
                    break;
                case Budget::scalars:
                    limits.bound_value_bytes = 16;
                    break;
                }
                SqliteWriter writer;
                QCOMPARE(writer.open(path_, limits), StorageError::none);
                bool done = false, rollback_released_lock = false;
                WorkError caught = WorkError::none;
                SqliteWorkResult result;
                QCOMPARE(writer.enqueue_work(
                             [&](SqliteWorkContext context) {
                                 auto session = context.begin();
                                 // No bound/result values; one statement, below48 ASCII bytes.
                                 (void)session.execute({"INSERT INTO counters VALUES (1, 10)", {}});
                                 caught = failure([&] {
                                     switch (budget) {
                                     case Budget::statements:
                                         (void)session.execute({"SELECT 1", {}});
                                         (void)session.execute({"SELECT 2", {}});
                                         if (over)
                                             (void)session.execute({"SELECT 3", {}});
                                         break;
                                     case Budget::sql_bytes:
                                         // SELECT 'é' is 11 UTF-8 bytes, including quotes.
                                         (void)session.execute({QString::fromUtf8("SELECT 'é'") +
                                                                    QString(over ? 38 : 37, ' '),
                                                                {}});
                                         break;
                                     case Budget::bindings:
                                         (void)session.execute(
                                             {"SELECT ?", {QString::fromUtf8("é")}});
                                         (void)session.execute(
                                             {"SELECT ?", {QByteArray(over ? 8 : 7, 'x')}});
                                         break;
                                     case Budget::rows:
                                         (void)session.execute({"SELECT 1", {}});
                                         (void)session.execute({"SELECT 2", {}});
                                         if (over)
                                             (void)session.execute({"SELECT 3", {}});
                                         break;
                                     case Budget::result_bytes:
                                         (void)session.execute({"SELECT 'é'", {}});
                                         (void)session.execute({over ? "SELECT X'0102030405060708'"
                                                                     : "SELECT X'01020304050607'",
                                                                {}});
                                         break;
                                     case Budget::scalars:
                                         (void)session.execute({"SELECT ?", {QVariant()}});
                                         (void)session.execute({"SELECT ?", {42}});
                                         if (over)
                                             (void)session.execute({"SELECT ?", {false}});
                                         break;
                                     }
                                 });
                                 if (caught != WorkError::none)
                                     rollback_released_lock = observer_->write_lock_is_released();
                                 if (caught == WorkError::none)
                                     session.commit();
                                 return QVariant("boundary");
                             },
                             [&](SqliteWorkResult r) {
                                 result = std::move(r);
                                 done = true;
                             }),
                         StorageError::none);
                QTRY_VERIFY(done);
                const auto expected_error =
                    !over ? WorkError::none
                          : (budget == Budget::rows || budget == Budget::result_bytes
                                 ? WorkError::result_limit
                                 : WorkError::resource_limit);
                QCOMPARE(caught, expected_error);
                if (over)
                    QVERIFY(rollback_released_lock);
                check_result(result,
                             over ? TransactionOutcome::rolled_back : TransactionOutcome::committed,
                             expected_error, !over);
                QCOMPARE(observer_->scalar("SELECT COUNT(*) FROM counters").toInt(), over ? 0 : 1);
            }
        }
        // Supported-type checks are distinct from byte exhaustion.
        QVERIFY(observer_->exec("DELETE FROM counters"));
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool done = false;
        SqliteWorkResult result;
        QCOMPARE(writer.enqueue_work(
                     [](SqliteWorkContext context) {
                         auto session = context.begin();
                         (void)session.execute(put(1, 10));
                         (void)session.execute({"SELECT ?", {QDate(2026, 1, 1)}});
                         session.commit();
                         return QVariant();
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         done = true;
                     }),
                 StorageError::none);
        QTRY_VERIFY(done);
        check_result(result, TransactionOutcome::rolled_back, WorkError::invalid_statement);
        QCOMPARE(observer_->scalar("SELECT COUNT(*) FROM counters").toInt(), 0);
    }

    void activeCapacityLastsUntilBodyReturns() {
        SqliteWriter writer;
        StorageLimits limits;
        limits.pending_batches = 2;
        QCOMPARE(writer.open(path_, limits), StorageError::none);
        QList<int> order;
        StorageError before = StorageError::none, after = StorageError::none,
                     reentrant = StorageError::not_open;
        int rejected_calls = 0;
        SqliteWorkResult result;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         auto session = context.begin();
                         before =
                             writer.enqueue({put(9, 900)}, [&](WriteResult) { ++rejected_calls; });
                         (void)session.execute(put(1, 10));
                         session.commit();
                         after = writer.enqueue_work([](SqliteWorkContext) { return QVariant(); },
                                                     [&](SqliteWorkResult) { ++rejected_calls; });
                         return QVariant(10);
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         order.append(1);
                         reentrant = writer.enqueue({put(3, 30)}, [&](WriteResult v) {
                             QCOMPARE(v.error, StorageError::none);
                             order.append(3);
                         });
                     }),
                 StorageError::none);
        QCOMPARE(writer.enqueue({put(2, 20)},
                                [&](WriteResult r) {
                                    QCOMPARE(r.error, StorageError::none);
                                    order.append(2);
                                }),
                 StorageError::none);
        QTRY_COMPARE(order.size(), 3);
        QCOMPARE(before, StorageError::queue_full);
        QCOMPARE(after, StorageError::queue_full);
        QCOMPARE(reentrant, StorageError::none);
        QCOMPARE(rejected_calls, 0);
        QCOMPARE(order, QList<int>({1, 2, 3}));
        check_result(result, TransactionOutcome::committed, WorkError::none, true);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 60);
    }

    void shutdownDrainsWorkAndBatches() {
        SqliteWriter writer, replacement;
        QCOMPARE(writer.open(path_), StorageError::none);
        QList<int> order;
        StorageError stop = StorageError::not_open, rejected = StorageError::none,
                     reopened = StorageError::not_open;
        SqliteWorkResult first, second;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         auto session = context.begin();
                         (void)session.execute(put(1, 10));
                         stop = writer.shutdown([&] {
                             reopened = replacement.open(path_);
                             order.append(4);
                         });
                         rejected = writer.enqueue({put(9, 900)}, [](WriteResult) {});
                         session.commit();
                         return QVariant(10);
                     },
                     [&](SqliteWorkResult r) {
                         first = std::move(r);
                         order.append(1);
                     }),
                 StorageError::none);
        QCOMPARE(writer.enqueue({put(2, 20)},
                                [&](WriteResult r) {
                                    QCOMPARE(r.error, StorageError::none);
                                    order.append(2);
                                }),
                 StorageError::none);
        QCOMPARE(writer.enqueue_work(
                     [](SqliteWorkContext context) {
                         auto session = context.begin();
                         (void)session.execute(put(3, 30));
                         session.commit();
                         return QVariant(30);
                     },
                     [&](SqliteWorkResult r) {
                         second = std::move(r);
                         order.append(3);
                     }),
                 StorageError::none);
        QTRY_COMPARE(order.size(), 4);
        QCOMPARE(stop, StorageError::none);
        QCOMPARE(rejected, StorageError::shutting_down);
        QCOMPARE(reopened, StorageError::none);
        QCOMPARE(order, QList<int>({1, 2, 3, 4}));
        check_result(first, TransactionOutcome::committed, WorkError::none, true);
        check_result(second, TransactionOutcome::committed, WorkError::none, true);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 60);
    }

    void throwingOrDestroyingCompletionIsContained() {
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            bool threw = false, next = false;
            SqliteWorkResult result;
            QCOMPARE(writer.enqueue_work(
                         [](SqliteWorkContext context) {
                             auto session = context.begin();
                             (void)session.execute(put(1, 10));
                             session.commit();
                             return QVariant(10);
                         },
                         [&](SqliteWorkResult r) {
                             result = std::move(r);
                             threw = true;
                             throw std::runtime_error("completion");
                         }),
                     StorageError::none);
            QCOMPARE(writer.enqueue({put(2, 20)},
                                    [&](WriteResult r) {
                                        QCOMPARE(r.error, StorageError::none);
                                        next = true;
                                    }),
                     StorageError::none);
            QTRY_VERIFY(threw && next);
            check_result(result, TransactionOutcome::committed, WorkError::none, true);
        }
        auto writer = std::make_unique<SqliteWriter>();
        QCOMPARE(writer->open(path_), StorageError::none);
        bool destroyed = false;
        SqliteWorkResult result;
        QCOMPARE(writer->enqueue_work(
                     [](SqliteWorkContext context) {
                         auto session = context.begin();
                         (void)session.execute(put(3, 30));
                         session.commit();
                         return QVariant(30);
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         writer.reset();
                         destroyed = true;
                     }),
                 StorageError::none);
        QTRY_VERIFY(destroyed);
        QVERIFY(!writer);
        check_result(result, TransactionOutcome::committed, WorkError::none, true);
        SqliteWriter replacement;
        QCOMPARE(replacement.open(path_), StorageError::none);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 60);
    }

    void ownedRequestAndResultOutliveCallerAndSession() {
        SqlRows rows;
        SqliteWorkResult result;
        {
            SqliteWriter writer;
            QCOMPARE(writer.open(path_), StorageError::none);
            bool done = false;
            QString request = QString::fromUtf8("Grüße");
            QByteArray blob("a\0b", 3);
            // Explicit owning captures are a caller precondition, not introspection.
            QCOMPARE(writer.enqueue_work(
                         [text = request, bytes = blob, &rows](SqliteWorkContext context) {
                             auto session = context.begin();
                             SqlStatement statement{"SELECT ?, ?", {text, bytes}};
                             rows = session.execute(statement);
                             statement.sql.clear();
                             statement.bindings.clear();
                             session.commit();
                             return QVariant(text);
                         },
                         [&](SqliteWorkResult r) {
                             result = std::move(r);
                             done = true;
                         }),
                     StorageError::none);
            request.fill('x');
            blob.fill('z');
            request.clear();
            blob.clear();
            QTRY_VERIFY(done);
            check_result(result, TransactionOutcome::committed, WorkError::none, true);
        }
        QCOMPARE(rows, SqlRows({{QString::fromUtf8("Grüße"), QByteArray("a\0b", 3)}}));
        QVERIFY(result.value.has_value());
        QCOMPARE(result.value->toString(), QString::fromUtf8("Grüße"));
    }

    void unknownOutcomeIsNotKnownRepositoryFailure() {
        static_assert(!std::is_base_of_v<ReceiptStorageError, SqliteWorkFailure>);
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        // First exercise the existing known-rollback category through the real
        // service: append conflicts after create, so its earlier write must undo.
        QVERIFY(observer_->exec("INSERT INTO counters VALUES (102, 99)"));
        bool known_done = false, known_caught = false, known_returned = false,
             known_released_lock = false;
        SqliteWorkResult known_result;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         PrimitiveReceiptRepository repository(context);
                         ReceiptIntake intake(repository);
                         try {
                             (void)intake.ingest(fresh_receipt());
                             known_returned = true;
                         } catch (const ReceiptStorageError &) {
                             known_caught = true;
                             known_released_lock = observer_->write_lock_is_released();
                         }
                         return QVariant("caught known rollback");
                     },
                     [&](SqliteWorkResult r) {
                         known_result = std::move(r);
                         known_done = true;
                     }),
                 StorageError::none);
        QTRY_VERIFY(known_done);
        QVERIFY(!known_returned);
        QVERIFY(known_caught);
        QVERIFY(known_released_lock);
        check_result(known_result, TransactionOutcome::rolled_back, WorkError::sql_failure);
        QCOMPARE(observer_->scalar("SELECT COUNT(*) FROM counters").toInt(), 1);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 99);
        QVERIFY(observer_->exec("DELETE FROM counters"));
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_commit_ack),
                 StorageError::none);
        bool done = false, returned = false, known_rollback = false;
        WorkError observed = WorkError::none;
        SqliteWorkResult result;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         PrimitiveReceiptRepository repository(context);
                         ReceiptIntake intake(repository);
                         try {
                             (void)intake.ingest(fresh_receipt());
                             returned = true;
                         } catch (const ReceiptStorageError &) {
                             known_rollback = true;
                         } catch (const SqliteWorkFailure &error) {
                             observed = error.error();
                         }
                         return QVariant("caught service failure");
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         done = true;
                     }),
                 StorageError::none);
        QTRY_VERIFY(done);
        QVERIFY(!returned);
        QVERIFY(!known_rollback);
        QCOMPARE(observed, WorkError::uncertain_outcome);
        check_result(result, TransactionOutcome::outcome_unknown, WorkError::uncertain_outcome);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 30);
    }

    void returnWithoutBeginHasNoSuccessPayload() {
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        bool done = false;
        SqliteWorkResult result;
        QCOMPARE(writer.enqueue_work([](SqliteWorkContext) { return QVariant("not committed"); },
                                     [&](SqliteWorkResult r) {
                                         result = std::move(r);
                                         done = true;
                                     }),
                 StorageError::none);
        QTRY_VERIFY(done);
        check_result(result, TransactionOutcome::not_started, WorkError::missing_transaction);
        QCOMPARE(observer_->scalar("SELECT COUNT(*) FROM counters").toInt(), 0);
        done = false;
        QCOMPARE(writer.enqueue_work(
                     [](SqliteWorkContext) -> QVariant {
                         throw std::runtime_error("synthetic validation before BEGIN");
                     },
                     [&](SqliteWorkResult r) {
                         result = std::move(r);
                         done = true;
                     }),
                 StorageError::none);
        QTRY_VERIFY(done);
        check_result(result, TransactionOutcome::not_started, WorkError::work_exception);
    }

    void admissionErrorsDoNotInvokeWork() {
        SqliteWriter writer;
        int bodies = 0, completions = 0;
        auto work = [&](SqliteWorkContext) {
            ++bodies;
            return QVariant();
        };
        auto complete = [&](SqliteWorkResult) { ++completions; };
        QCOMPARE(writer.enqueue_work(work, complete), StorageError::not_open);
        StorageLimits limits;
        limits.pending_batches = 1;
        QCOMPARE(writer.open(path_, limits), StorageError::none);
        QCOMPARE(writer.enqueue_work({}, complete), StorageError::invalid_work);
        QCOMPARE(writer.enqueue_work(work, {}), StorageError::invalid_work);
        StorageError foreign = StorageError::none;
        std::thread other([&] { foreign = writer.enqueue_work(work, complete); });
        other.join();
        QCOMPARE(foreign, StorageError::wrong_thread);
        bool first_done = false, stopped = false;
        QCOMPARE(writer.enqueue_work(
                     [](SqliteWorkContext context) {
                         auto session = context.begin();
                         session.commit();
                         return QVariant(1);
                     },
                     [&](SqliteWorkResult r) {
                         check_result(r, TransactionOutcome::committed, WorkError::none, true);
                         first_done = true;
                     }),
                 StorageError::none);
        QCOMPARE(writer.enqueue_work(work, complete), StorageError::queue_full);
        QCOMPARE(writer.shutdown([&] { stopped = true; }), StorageError::none);
        QCOMPARE(writer.enqueue_work(work, complete), StorageError::shutting_down);
        QTRY_VERIFY(first_done && stopped);
        QCOMPARE(bodies, 0);
        QCOMPARE(completions, 0);
    }

    void cleanupIsNonthrowingAndDoesNotRepeatRollback() {
        static_assert(noexcept(std::declval<SqliteWorkSession>().cleanup()));
        SqliteWorkSession empty;
        QCOMPARE(empty.cleanup(), WorkError::none);
        QCOMPARE(failure([&] { (void)SqliteWorkContext().begin(); }), WorkError::stale_session);
        SqliteWriter writer;
        QCOMPARE(writer.open(path_), StorageError::none);
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_rollback_ack),
                 StorageError::none);
        QList<SqliteWorkResult> results;
        WorkError first_cleanup = WorkError::feature_not_implemented,
                  repeated = WorkError::feature_not_implemented, rollback_cleanup = WorkError::none;
        QCOMPARE(writer.enqueue_work(
                     [&](SqliteWorkContext context) {
                         auto session = context.begin();
                         (void)session.execute(put(1, 10));
                         session.commit();
                         first_cleanup = session.cleanup();
                         repeated = session.cleanup();
                         return QVariant(10);
                     },
                     [&](SqliteWorkResult r) { results.append(std::move(r)); }),
                 StorageError::none);
        QCOMPARE(
            writer.enqueue_work(
                [&](SqliteWorkContext context) {
                    auto session = context.begin();
                    (void)session.execute(put(2, 20));
                    rollback_cleanup = session.cleanup();
                    (void)session.cleanup(); // Still nonthrowing after uncertain terminal cleanup.
                    return QVariant("rolled back physically, acknowledgement lost");
                },
                [&](SqliteWorkResult r) { results.append(std::move(r)); }),
            StorageError::none);
        QTRY_COMPARE(results.size(), 2);
        QCOMPARE(first_cleanup, WorkError::none);
        QCOMPARE(repeated, WorkError::none);
        QCOMPARE(rollback_cleanup, WorkError::uncertain_outcome);
        check_result(results[0], TransactionOutcome::committed, WorkError::none, true);
        check_result(results[1], TransactionOutcome::outcome_unknown, WorkError::uncertain_outcome);
        QCOMPARE(observer_->scalar("SELECT SUM(amount) FROM counters").toInt(), 10);
    }
};

QTEST_GUILESS_MAIN(SqliteWriterWorkTests)
#include "sqlite_writer_work_tests.moc"
