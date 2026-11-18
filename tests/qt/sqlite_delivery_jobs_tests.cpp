// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/application_service_fixture.hpp"

#include <QTemporaryDir>
#include <QTest>
#include <subathon/server/delivery_jobs.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace {
using namespace subathon::server;
using application_service_fixture::Data;
using qt_test_support::require;
using testing::SqliteWriterWorkAccess;
using testing::WorkControlFault;

// Independent proposed schema fixture. The coordinator owns the production
// catalog and its separate exact-prefix/advance checks.
const QString capture_ddl =
    "CREATE TABLE subathon_delivery_captures(operation_scope BLOB NOT NULL "
    "CHECK(typeof(operation_scope)='blob' AND length(operation_scope) BETWEEN 1 AND 4096), "
    "operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND "
    "length(operation_ref) BETWEEN 1 AND 4096), job_ref BLOB NOT NULL CHECK(typeof(job_ref)='blob' "
    "AND length(job_ref) BETWEEN 1 AND 4096), route_ref BLOB NOT NULL "
    "CHECK(typeof(route_ref)='blob' "
    "AND length(route_ref) BETWEEN 1 AND 4096), selection INTEGER NOT NULL "
    "CHECK(typeof(selection)='integer' AND selection IN (0,1)), sender_ref BLOB NOT NULL "
    "CHECK(typeof(sender_ref)='blob' AND length(sender_ref) BETWEEN 1 AND 4096), action_ref BLOB "
    "NOT NULL CHECK(typeof(action_ref)='blob' AND length(action_ref) BETWEEN 1 AND 4096), "
    "action_version_ref BLOB NOT NULL CHECK(typeof(action_version_ref)='blob' AND "
    "length(action_version_ref) BETWEEN 1 AND 4096), PRIMARY KEY(operation_scope,operation_ref,"
    "job_ref), FOREIGN KEY(operation_scope,operation_ref,job_ref) "
    "REFERENCES subathon_app_jobs(operation_scope,operation_ref,job_ref))";
const QString events_ddl =
    "CREATE TABLE subathon_delivery_events(operation_scope BLOB NOT NULL, operation_ref BLOB "
    "NOT NULL, job_ref BLOB NOT NULL, revision INTEGER NOT NULL "
    "CHECK(typeof(revision)='integer' AND revision>=1), state INTEGER NOT NULL "
    "CHECK(typeof(state)='integer' AND state BETWEEN 0 AND 4), attempt INTEGER "
    "CHECK(attempt IS NULL OR (typeof(attempt)='integer' AND attempt>=2 AND attempt<=revision)), "
    "evidence_ref BLOB CHECK(evidence_ref IS NULL OR (typeof(evidence_ref)='blob' AND "
    "length(evidence_ref) BETWEEN 1 AND 4096)), CHECK((state=0 AND attempt IS NULL) OR "
    "(state=1 AND attempt IS NOT NULL AND attempt=revision) OR "
    "(state BETWEEN 2 AND 4 AND attempt IS NOT NULL AND attempt<revision)), "
    "CHECK((state=1 OR revision=1)=(evidence_ref IS NULL)), "
    "CHECK(revision<>1 OR state=0), PRIMARY KEY(operation_scope,operation_ref,job_ref,revision), "
    "FOREIGN KEY(operation_scope,operation_ref,job_ref) "
    "REFERENCES subathon_delivery_captures(operation_scope,operation_ref,job_ref))";

const DeliveryJobKey key{{"svc", "add10"}, "job"};
const DeliveryCapture capture{
    "job",           "test/channel-A", DeliverySenderSelection::explicit_identity,
    "broadcaster-A", "announce",       "v7"};
const QByteArray document = QByteArray::fromHex("6561726e656400ff");
constexpr auto format = "subathon.internal.delivery-job.v1";
const QString event_query =
    "SELECT revision,state,attempt,evidence_ref FROM main.subathon_delivery_events WHERE "
    "operation_scope=X'737663' AND operation_ref=X'6164643130' AND job_ref=X'6a6f62' "
    "ORDER BY revision";
constexpr int timeout = 10000;

SqlRow event(qint64 revision, qint64 state, QVariant attempt = {}, QVariant evidence = {}) {
    return {revision, state, std::move(attempt), std::move(evidence)};
}

void normal(const SqliteWorkResult &work) {
    require(work.transaction == TransactionOutcome::committed && work.error == WorkError::none &&
                work.value,
            "Actual writer completion must be committed and normal");
}

void check_capture(const DeliveryJob &job) {
    require(job.key.operation.operation_scope == "svc" &&
                job.key.operation.operation_ref == "add10" && job.key.job_ref == "job" &&
                job.capture.job_ref == "job" && job.capture.route_ref == "test/channel-A" &&
                job.capture.selection == DeliverySenderSelection::explicit_identity &&
                job.capture.sender_ref == "broadcaster-A" && job.capture.action_ref == "announce" &&
                job.capture.action_version_ref == "v7" && job.action_document == document,
            "Complete captured action and resolved sender, including binary body");
}

class File {
  public:
    explicit File(bool reject_sending_insert = false) {
        require(directory.isValid(), "Disposable database path");
        reopen();
        application_service_fixture::retain_case(*writer, data, "add10");
        normal(work([reject_sending_insert](auto context) -> QVariant {
            auto s = context.begin();
            (void)s.execute({capture_ddl, {}});
            auto ddl = events_ddl;
            if (reject_sending_insert)
                ddl.insert(ddl.indexOf("CHECK(revision<>1"), "CHECK(state<>1), ");
            (void)s.execute({ddl, {}});
            s.commit();
            return 1;
        }));
    }

    void reopen() {
        writer.reset();
        writer = std::make_unique<SqliteWriter>();
        if (!opened) {
            application_service_fixture::open_sources(*writer, path, data);
            opened = true;
        } else {
            require(writer->open(path, application_service_fixture::limits()) == StorageError::none,
                    "Fresh writer opens same disposable store");
        }
    }

    SqliteWorkResult work(SqliteWriter::Work body) {
        const auto ni = std::make_shared<bool>(false);
        auto result = qt_test_support::run_work(
            *writer,
            [body = std::move(body), ni](auto context) {
                try {
                    return body(context);
                } catch (const DeliveryJobsNotImplemented &) {
                    *ni = true;
                    throw;
                }
            },
            timeout);
        if (*ni)
            throw DeliveryJobsNotImplemented();
        return result;
    }

    template <class Call> auto committed(Call call) {
        using Value = decltype(call(std::declval<SqliteDeliveryJobStore &>()));
        const auto value = std::make_shared<std::optional<Value>>();
        normal(work([call = std::move(call), value](auto context) mutable -> QVariant {
            auto s = context.begin();
            SqliteDeliveryJobStore store(s);
            value->emplace(call(store));
            s.commit();
            return 1;
        }));
        return std::move(value->value());
    }

    PreparedApplicationBundle bundle(bool marked = true) const {
        auto b = application_service_fixture::prepared(data.case_data("add10")["expected_record"]);
        if (marked)
            b.jobs[0].envelope = {format, document};
        // The v1 service result embeds SAJ1. Use a storage-only result for a
        // marked bundle so the fixture never creates contradictory service history.
        if (marked)
            b.result = {"subathon.storage.synthetic-result.v1", QByteArray("delivery-example")};
        return b;
    }

    // Seed independent durable inputs through the old bundle port and literal
    // SQL. An NI in enrollment must not mask NI in dispatch, finish or recovery.
    void seed(int state = 0, bool marked = true) {
        normal(work([b = bundle(marked), state, marked](auto context) -> QVariant {
            auto s = context.begin();
            require(SqliteApplicationBundleStore(s).stage(b).disposition ==
                        SqliteBundleStageDisposition::staged,
                    "Frozen application fixture stages using the existing port");
            if (marked) {
                (void)s.execute({"INSERT INTO main.subathon_delivery_captures VALUES "
                                 "(X'737663',X'6164643130',X'6a6f62',?,1,?,?,?)",
                                 {QByteArray("test/channel-A"), QByteArray("broadcaster-A"),
                                  QByteArray("announce"), QByteArray("v7")}});
                (void)s.execute({"INSERT INTO main.subathon_delivery_events VALUES "
                                 "(X'737663',X'6164643130',X'6a6f62',1,0,NULL,NULL)",
                                 {}});
                if (state != 0)
                    (void)s.execute({"INSERT INTO main.subathon_delivery_events VALUES "
                                     "(X'737663',X'6164643130',X'6a6f62',2,1,2,NULL)",
                                     {}});
                if (state == 3)
                    (void)s.execute(
                        {"INSERT INTO main.subathon_delivery_events VALUES "
                         "(X'737663',X'6164643130',X'6a6f62',3,3,2,X'6e6f742d73656e74')",
                         {}});
            }
            s.commit();
            return 1;
        }));
    }

    SqlRows read(const QString &sql) const {
        SqliteReader reader;
        require(reader.open(path) == StorageError::none, "Independent read-only connection opens");
        auto result = reader.read({sql, {}});
        require(result.error == StorageError::none, "Independent SQL read succeeds");
        return std::move(result.rows);
    }

    QTemporaryDir directory;
    const QString path{directory.filePath("delivery.sqlite")};
    Data data;
    std::unique_ptr<SqliteWriter> writer;
    bool opened{false};
};

class Authority final : public DeliveryDispatchAuthority {
  public:
    bool allows(const DeliveryJob &job) override {
        check_capture(job);
        ++calls;
        return permit;
    }
    bool permit{true};
    int calls{0};
};

class RetryPolicy final : public DeliveryRetryPolicy {
  public:
    std::optional<std::string> allows(const DeliveryJob &job) override {
        check_capture(job);
        require(job.state == DeliveryJobState::failed && job.attempt == 2 &&
                    job.evidence_ref == "not-sent",
                "Policy sees exact durable evidence and attempt");
        ++calls;
        return decision;
    }
    std::optional<std::string> decision;
    int calls{0};
};

class Adapter final : public DeliveryAdapter {
  public:
    explicit Adapter(QString path) : path_(std::move(path)) {}
    void start(DeliveryJob claim) override {
        claims.push_back(std::move(claim));
        SqliteReader reader;
        require(reader.open(path_) == StorageError::none, "Adapter observer opens");
        const auto result = reader.read({event_query, {}});
        require(result.error == StorageError::none, "Independent look inside fake adapter");
        observed = result.rows;
    }

    std::vector<DeliveryJob> claims;
    SqlRows observed;

  private:
    QString path_;
};

DeliveryDispatchCompletion dispatch(File &f, std::int64_t revision,
                                    const std::shared_ptr<Authority> &authority,
                                    const std::shared_ptr<Adapter> &adapter,
                                    DeliveryJobKey selected = key) {
    const auto response = std::make_shared<std::optional<DeliveryDispatchCompletion>>();
    auto admission =
        dispatch_delivery_job(*f.writer, std::move(selected), revision, authority, adapter,
                              [response](auto result) { response->emplace(std::move(result)); });
    if (admission == StorageError::feature_not_implemented)
        throw DeliveryJobsNotImplemented();
    require(admission == StorageError::none, "Dispatch work admitted");
    require(QTest::qWaitFor([response] { return response->has_value(); }, timeout),
            "Dispatch completes");
    return std::move(response->value());
}

void expect(const DeliveryJobResult &result, DeliveryDisposition disposition) {
    require(result.disposition == disposition &&
                result.job.has_value() == (disposition == DeliveryDisposition::changed),
            "Exact disposition and payload presence");
}

void expect(const DeliveryDispatchCompletion &completion, DeliveryDisposition disposition) {
    normal(completion.work);
    require(completion.result && !completion.adapter_threw, "Normal typed dispatch result");
    expect(*completion.result, disposition);
}

template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}
} // namespace

class SqliteDeliveryJobsTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void bundleAndCaptureCommitTogether() {
        scenario([] {
            File f;
            const auto b = f.bundle();
            const auto staged = f.committed([&](auto &store) { return store.stage(b, {capture}); });
            require(staged.disposition == SqliteBundleStageDisposition::staged && !staged.existing,
                    "First application stages one bundle");
            f.reopen();
            require(f.read(event_query) == SqlRows{event(1, 0)}, "Queued is a stored state");
            require(f.read("SELECT token FROM main.subathon_app_head") ==
                        SqlRows{{QByteArray("c")}},
                    "Actual application head advanced with enrollment");
            const auto retained = f.committed([](auto &store) { return store.lookup(key); });
            require(retained && retained->state == DeliveryJobState::queued &&
                        retained->revision == 1 && !retained->attempt && !retained->evidence_ref,
                    "Read retained queued job after reopen");
            check_capture(*retained);
            const auto repeat = f.committed([&](auto &store) { return store.stage(b, {capture}); });
            require(repeat.disposition == SqliteBundleStageDisposition::already_present &&
                        repeat.existing && f.read(event_query) == SqlRows{event(1, 0)},
                    "Same application cannot add another enrollment or transition");
            auto edited = capture;
            edited.sender_ref = "bot-B";
            const auto conflict =
                f.committed([&](auto &store) { return store.stage(b, {edited}); });
            require(conflict.disposition == SqliteBundleStageDisposition::operation_conflict &&
                        !conflict.existing,
                    "Duplicate application cannot replace captured sender");
            const auto unchanged = f.committed([](auto &store) { return store.lookup(key); });
            require(unchanged.has_value(), "The original enrollment remains");
            check_capture(*unchanged);
        });
    }

    void abandonBundleAndCapture() {
        scenario([] {
            File f;
            const auto b = f.bundle();
            const auto before = f.read("SELECT token FROM main.subathon_app_head");
            const auto result = f.work([b](auto context) -> QVariant {
                auto s = context.begin();
                require(SqliteDeliveryJobStore(s).stage(b, {capture}).disposition ==
                            SqliteBundleStageDisposition::staged,
                        "Bundle and capture actually staged before abandonment");
                require(s.execute({event_query, {}}) == SqlRows{event(1, 0)},
                        "Sending is still ineligible while queue entry is provisional");
                return 1;
            });
            require(result.transaction == TransactionOutcome::rolled_back &&
                        result.error == WorkError::uncommitted && !result.value,
                    "Existing writer reports actual rollback of an abandoned unit");
            require(f.read(event_query).isEmpty() &&
                        f.read("SELECT * FROM main.subathon_delivery_captures").isEmpty() &&
                        f.read("SELECT * FROM main.subathon_app_jobs").isEmpty() &&
                        f.read("SELECT * FROM main.subathon_app_effects").isEmpty() &&
                        f.read("SELECT token FROM main.subathon_app_head") == before,
                    "No staged effects, envelope, state or capture survived");
        });
    }

    void legacyIsIneligible() {
        scenario([] {
            File f;
            f.seed(0, false);
            const auto authority = std::make_shared<Authority>();
            const auto adapter = std::make_shared<Adapter>(f.path);
            require(!f.committed([](auto &store) { return store.lookup(key); }),
                    "SAJ1 is only a historical storage envelope");
            expect(dispatch(f, 1, authority, adapter), DeliveryDisposition::missing);
            require(adapter->claims.empty() && authority->calls == 0 &&
                        f.read(event_query).isEmpty(),
                    "Legacy yields no authority check, attempt or send");
        });
    }

    void dispatchFencesBeforeAdapter() {
        scenario([] {
            File f;
            f.seed();
            f.reopen();
            const auto authority = std::make_shared<Authority>();
            const auto adapter = std::make_shared<Adapter>(f.path);
            const SqlRows expected{event(1, 0), event(2, 1, qint64(2))};
            auto other = key;
            other.operation.operation_scope = "svc-other";
            expect(dispatch(f, 1, authority, adapter, other), DeliveryDisposition::missing);
            expect(dispatch(f, 1, authority, adapter), DeliveryDisposition::changed);
            expect(dispatch(f, 1, authority, adapter), DeliveryDisposition::stale);
            expect(dispatch(f, 2, authority, adapter), DeliveryDisposition::refused);
            require(authority->calls == 1 && adapter->claims.size() == 1 &&
                        adapter->observed == expected && f.read(event_query) == expected,
                    "Exactly one adapter call observed durable sending before starting");
            const auto &claim = adapter->claims.front();
            check_capture(claim);
            require(claim.state == DeliveryJobState::sending && claim.revision == 2 &&
                        claim.attempt == 2 && !claim.evidence_ref,
                    "Exact dispatch claim");
        });
    }

    void deniedDispatchKeepsQueued() {
        scenario([] {
            File f;
            f.seed();
            const auto authority = std::make_shared<Authority>();
            authority->permit = false;
            const auto adapter = std::make_shared<Adapter>(f.path);
            expect(dispatch(f, 1, authority, adapter), DeliveryDisposition::refused);
            require(authority->calls == 1 && adapter->claims.empty() &&
                        f.read(event_query) == SqlRows{event(1, 0)},
                    "Denied sender cannot be substituted or started");
        });
    }

    void knownClaimRollbackDoesNotSend() {
        scenario([] {
            File f(true);
            f.seed();
            const auto authority = std::make_shared<Authority>();
            const auto adapter = std::make_shared<Adapter>(f.path);
            const auto result = dispatch(f, 1, authority, adapter);
            require(result.work.transaction == TransactionOutcome::rolled_back &&
                        result.work.error == WorkError::sql_failure && !result.work.value &&
                        !result.result && !result.adapter_threw && adapter->claims.empty() &&
                        authority->calls == 1 && f.read(event_query) == SqlRows{event(1, 0)},
                    "Actual SQL failure rolls back the claim and cannot start the adapter");
        });
    }

    void lostClaimAcknowledgmentDoesNotSend() {
        scenario([] {
            File f;
            f.seed();
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *f.writer, WorkControlFault::lose_next_commit_ack) == StorageError::none,
                    "Real claim COMMIT loses acknowledgment");
            const auto authority = std::make_shared<Authority>();
            const auto adapter = std::make_shared<Adapter>(f.path);
            const auto result = dispatch(f, 1, authority, adapter);
            require(result.work.transaction == TransactionOutcome::outcome_unknown &&
                        result.work.error == WorkError::uncertain_outcome && !result.work.value &&
                        !result.result && !result.adapter_threw && adapter->claims.empty(),
                    "Unknown claim COMMIT gives no normal result or send");
            require(f.read(event_query) == SqlRows{event(1, 0), event(2, 1, qint64(2))},
                    "Claim was physically committed by the existing fault seam");
        });
    }

    void settleEvidence_data() {
        QTest::addColumn<int>("evidence");
        QTest::addColumn<int>("state");
        QTest::newRow("delivered") << 0 << 2;
        QTest::newRow("known-undelivered") << 1 << 3;
        QTest::newRow("indeterminate") << 2 << 4;
    }
    void settleEvidence() {
        QFETCH(int, evidence);
        QFETCH(int, state);
        scenario([&] {
            File f;
            f.seed(1);
            const auto before = f.read("SELECT * FROM main.subathon_app_effects");
            const auto finish = [&](auto &store, std::int64_t revision, std::int64_t attempt) {
                return store.finish(key, revision, attempt, DeliveryEvidence(evidence), "report-A");
            };
            expect(f.committed([&](auto &store) { return finish(store, 2, 1); }),
                   DeliveryDisposition::stale);
            const auto result = f.committed([&](auto &store) { return finish(store, 2, 2); });
            expect(result, DeliveryDisposition::changed);
            check_capture(*result.job);
            require(int(result.job->state) == state && result.job->revision == 3 &&
                        result.job->attempt == 2 && result.job->evidence_ref == "report-A",
                    "Typed transition matches independent evidence oracle");
            expect(f.committed([&](auto &store) { return finish(store, 2, 2); }),
                   DeliveryDisposition::stale);
            f.reopen();
            require(f.read(event_query) ==
                            SqlRows{event(1, 0), event(2, 1, qint64(2)),
                                    event(3, state, qint64(2), QByteArray("report-A"))} &&
                        f.read("SELECT * FROM main.subathon_app_effects") == before,
                    "Exactly one terminal outcome survives without rewriting effects");
        });
    }

    void interruptedSendingRequiresReview_data() {
        QTest::addColumn<bool>("started");
        QTest::newRow("before-adapter") << false;
        QTest::newRow("after-adapter-before-evidence") << true;
    }
    void interruptedSendingRequiresReview() {
        QFETCH(bool, started);
        scenario([&] {
            File f;
            const auto authority = std::make_shared<Authority>();
            const auto adapter = std::make_shared<Adapter>(f.path);
            f.seed(started ? 0 : 1);
            // For the before-send cut, the independent SQL fixture stops just
            // after the durable claim. For the other cut, the fake start runs.
            if (started)
                expect(dispatch(f, 1, authority, adapter), DeliveryDisposition::changed);
            f.reopen();
            const auto recovered = f.committed([](auto &store) { return store.recover(key, 2); });
            expect(recovered, DeliveryDisposition::changed);
            require(recovered.job->state == DeliveryJobState::outcome_unknown &&
                        recovered.job->revision == 3 && recovered.job->attempt == 2 &&
                        recovered.job->evidence_ref == "interrupted-sending",
                    "Restart never derives known non-delivery from an interrupted attempt");
            RetryPolicy policy;
            policy.decision = "synthetic-retry";
            expect(f.committed([&](auto &store) { return store.retry(key, 3, policy); }),
                   DeliveryDisposition::refused);
            expect(f.committed([](auto &store) {
                return store.finish(key, 3, 2, DeliveryEvidence::delivered, "late-report");
            }),
                   DeliveryDisposition::refused);
            expect(dispatch(f, 3, authority, adapter), DeliveryDisposition::refused);
            require(f.read(event_query) ==
                            SqlRows{event(1, 0), event(2, 1, qint64(2)),
                                    event(3, 4, qint64(2), QByteArray("interrupted-sending"))} &&
                        adapter->claims.size() == (started ? 1U : 0U) && policy.calls == 0,
                    "Recovery commits one review state and no blind replay");
        });
    }

    void lostFinishAcknowledgmentDoesNotReplay() {
        scenario([] {
            File f;
            f.seed();
            const auto authority = std::make_shared<Authority>();
            const auto adapter = std::make_shared<Adapter>(f.path);
            expect(dispatch(f, 1, authority, adapter), DeliveryDisposition::changed);
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *f.writer, WorkControlFault::lose_next_commit_ack) == StorageError::none,
                    "Inject loss only after actual finish COMMIT");
            const auto result = f.work([](auto context) -> QVariant {
                auto s = context.begin();
                expect(SqliteDeliveryJobStore(s).finish(key, 2, 2, DeliveryEvidence::delivered,
                                                        "sent-A"),
                       DeliveryDisposition::changed);
                s.commit();
                return 1;
            });
            require(result.transaction == TransactionOutcome::outcome_unknown &&
                        result.error == WorkError::uncertain_outcome && !result.value,
                    "Lost SQL acknowledgment cannot report a normal finish");
            f.reopen();
            expect(f.committed([](auto &store) { return store.recover(key, 3); }),
                   DeliveryDisposition::refused);
            expect(dispatch(f, 3, authority, adapter), DeliveryDisposition::refused);
            require(adapter->claims.size() == 1 && authority->calls == 1 &&
                        f.read(event_query) ==
                            SqlRows{event(1, 0), event(2, 1, qint64(2)),
                                    event(3, 2, qint64(2), QByteArray("sent-A"))},
                    "Durable delivered survives lost local acknowledgment and restart");
        });
    }

    void retryRequiresKnownNonDeliveryAndPolicy() {
        scenario([] {
            File f;
            f.seed(3);
            RetryPolicy policy;
            const auto retry = [&policy](auto &store) { return store.retry(key, 3, policy); };
            expect(f.committed(retry), DeliveryDisposition::refused);
            policy.decision = "synthetic-retry";
            expect(f.committed(retry), DeliveryDisposition::changed);
            expect(f.committed(retry), DeliveryDisposition::stale);
            const auto authority = std::make_shared<Authority>();
            const auto adapter = std::make_shared<Adapter>(f.path);
            expect(dispatch(f, 4, authority, adapter), DeliveryDisposition::changed);
            expect(f.committed([](auto &store) {
                return store.finish(key, 5, 2, DeliveryEvidence::delivered, "old-report");
            }),
                   DeliveryDisposition::stale);
            require(policy.calls == 2 && adapter->claims.size() == 1 &&
                        adapter->claims.front().attempt == 5 &&
                        f.read(event_query) ==
                            SqlRows{event(1, 0), event(2, 1, qint64(2)),
                                    event(3, 3, qint64(2), QByteArray("not-sent")),
                                    event(4, 0, {}, QByteArray("synthetic-retry")),
                                    event(5, 1, qint64(5))},
                    "Retry preserves attempt history and the original sender/action capture");
            check_capture(adapter->claims.front());
        });
    }
};

QTEST_GUILESS_MAIN(SqliteDeliveryJobsTests)
#include "sqlite_delivery_jobs_tests.moc"
