// SPDX-License-Identifier: GPL-3.0-or-later
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <subathon/server/sqlite_contributions.hpp>
#include <subathon/server/sqlite_retained_context.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <thread>
using namespace subathon::server;
using namespace subathon::core;
using subathon::server::testing::SqliteWriterWorkAccess;
using subathon::server::testing::WorkControlFault;
namespace {
QByteArray blob(const std::string &s) {
    return s.empty() ? QByteArray("", 0) : QByteArray(s.data(), qsizetype(s.size()));
}
std::string opaque(const QByteArray &s) { return std::string(s.constData(), size_t(s.size())); }
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class Operation>
void expect_input(Operation operation, SqliteContributionInputError error) {
    try {
        operation();
    } catch (const SqliteContributionInputFailure &failure) {
        require(failure.error() == error, "wrong SqliteContributionInputFailure category");
        return;
    }
    throw std::runtime_error("expected SqliteContributionInputFailure missing");
}
template <class Operation> void expect_work(Operation operation, WorkError error) {
    try {
        operation();
    } catch (const SqliteWorkFailure &failure) {
        require(failure.error() == error, "wrong SqliteWorkFailure category");
        return;
    }
    throw std::runtime_error("expected SqliteWorkFailure missing");
}
ReceiptInput input(std::string id = "event-42") {
    return {{{"synthetic.provider-a", "source-account-7"}, "synthetic.event.v1", std::move(id)},
            {"synthetic.payment.received", 1, 1700000000000, "actor-3", "Synthetic Supporter",
             ReceiptMoney{"12.3400", "EUR"}, ReceiptLinkReference{"payment.v1", "payment-9"},
             ReceiptLinkReference{"activity.v1", "activity-2"}},
            {1700000000100, "connection-1", "cfg1", "ctx1"}};
}
ReceiptAttempt attempt(const ReceiptInput &value, ReceiptDisposition disposition) {
    return {value.arrival, value.facts, disposition};
}
const QStringList facts_columns = {
    "event_type",         "schema_version",         "source_timestamp_utc_ms", "actor_id",
    "actor_display_name", "money_original_decimal", "money_currency",          "payment_namespace",
    "payment_opaque_id",  "activity_namespace",     "activity_opaque_id"};
const QStringList arrival_columns = {"server_timestamp_utc_ms", "connection_ref",
                                     "configuration_ref", "modifier_context_ref"};
QList<QVariant> facts_values(const ReceiptFacts &v) {
    const auto optional_blob = [](const std::optional<std::string> &s) {
        return s ? QVariant(blob(*s)) : QVariant();
    };
    return {blob(v.event_type),
            qint64(v.schema_version),
            v.source_timestamp_utc_ms ? QVariant(qint64(*v.source_timestamp_utc_ms)) : QVariant(),
            optional_blob(v.actor_id),
            optional_blob(v.actor_display_name),
            v.money ? QVariant(blob(v.money->original_decimal)) : QVariant(),
            v.money ? QVariant(blob(v.money->currency)) : QVariant(),
            v.payment_reference ? QVariant(blob(v.payment_reference->identifier_namespace))
                                : QVariant(),
            v.payment_reference ? QVariant(blob(v.payment_reference->opaque_id)) : QVariant(),
            v.activity_reference ? QVariant(blob(v.activity_reference->identifier_namespace))
                                 : QVariant(),
            v.activity_reference ? QVariant(blob(v.activity_reference->opaque_id)) : QVariant()};
}
QList<QVariant> arrival_values(const ReceiptArrival &v) {
    return {qint64(v.server_timestamp_utc_ms), blob(v.connection_ref), blob(v.configuration_ref),
            blob(v.modifier_context_ref)};
}
QString quoted(QString name) {
    name.replace('"', "\"\"");
    return '"' + name + '"';
}

// Independent reference packing for disposable SQL seeds/observations only.
// Expected new-codec vectors still come from literal JSON, never this UUT.
QByteArray id_blob(std::uint64_t value) {
    QByteArray out(8, '\0');
    for (int i = 7; i >= 0; --i) {
        out[i] = char(value & 255U);
        value >>= 8;
    }
    return out;
}
ContributionConditions captured() {
    return {
        {true, ReceiptMoney{"12.3400", "EUR"}, {}, true},
        ContributionReview::pending,
        {EvidencePolicy::require_evidence_review, "policy1",
         EvidenceReviewDecision{EvidenceReviewChoice::await_confirmation, "decision1", "actor1"}},
        ContributionApplication::not_applied,
        ContributionReversal::none,
        SettlementGate::clear};
}
ContributionObservation support(std::uint64_t id = 1) {
    auto r = input("support-" + std::to_string(id));
    return {{{id}, r.identity, r.facts, r.arrival},
            ContributionObservationKind::support,
            PaymentEvidence::unknown,
            VerifiedActivityLink{{r.identity.origin, "activity.v1", "activity-9"}, "proof1"},
            captured()};
}
ContributionObservation confirmation(std::uint64_t id = 2) {
    auto s = support(id);
    s.receipt.identity.opaque_id = "confirmation-" + std::to_string(id);
    s.receipt.facts.event_type = "synthetic.payment.confirmed";
    s.receipt.arrival = {1700000001100, "connection-2", "cfg2", "ctx2"};
    s.kind = ContributionObservationKind::confirmation;
    s.evidence = PaymentEvidence::confirmed;
    s.support_conditions.reset();
    s.verified_activity->verification_ref = "proof2";
    return s;
}
ContributionState initial(const ContributionObservation &s) {
    return {s.evidence, *s.support_conditions};
}
QList<QVariant> conditions_values(const ContributionConditions &c) {
    const auto &v = c.valuation;
    const auto &a = c.evidence_admission;
    return {int(v.monetary),
            v.original_money ? QVariant(blob(v.original_money->original_decimal)) : QVariant(),
            v.original_money ? QVariant(blob(v.original_money->currency)) : QVariant(),
            v.resolution_ref ? QVariant(blob(*v.resolution_ref)) : QVariant(),
            int(v.missing_rate),
            int(c.review),
            int(a.policy),
            blob(a.policy_ref),
            a.review_decision ? QVariant(int(a.review_decision->choice)) : QVariant(),
            a.review_decision ? QVariant(blob(a.review_decision->decision_ref)) : QVariant(),
            a.review_decision ? QVariant(blob(a.review_decision->actor_ref)) : QVariant(),
            int(c.application),
            int(c.reversal),
            int(c.settlement)};
}
const QStringList condition_columns = {"monetary",        "original_decimal",   "currency",
                                       "resolution_ref",  "missing_rate",       "review",
                                       "evidence_policy", "policy_ref",         "decision_choice",
                                       "decision_ref",    "decision_actor_ref", "application",
                                       "reversal",        "settlement"};
QList<QVariant> activity_values(const std::optional<VerifiedActivityIdentity> &key) {
    if (!key)
        return {QVariant(), QVariant(), QVariant(), QVariant()};
    return {blob(key->origin.provider), blob(key->origin.source_account),
            blob(key->identifier_namespace), blob(key->opaque_id)};
}
const QStringList activity_columns = {"activity_provider", "activity_source_account",
                                      "activity_namespace", "activity_id"};

// Negative/raw fixtures are independent SQL, never new codec/factory/repository
// output. OFF/ignore-checks applies only to this fixture, not the product writer.
class Fixture final {
  public:
    explicit Fixture(const QString &path)
        : name_(QUuid::createUuid().toString()), db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
    }
    ~Fixture() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }
    bool open() { return db_.open() && exec("PRAGMA foreign_keys=OFF"); }
    bool exec(const QString &sql, const QList<QVariant> &values = {}) {
        QSqlQuery q(db_);
        if (!q.prepare(sql))
            return false;
        for (const auto &v : values)
            q.addBindValue(v);
        return q.exec();
    }
    SqlRows rows(const QString &sql) {
        QSqlQuery q(db_);
        if (!q.exec(sql)) {
            QTest::qFail(qPrintable(q.lastError().text()), __FILE__, __LINE__);
            return {};
        }
        SqlRows result;
        while (q.next()) {
            SqlRow row;
            for (int i = 0; i < q.record().count(); ++i)
                row.append(q.value(i));
            result.append(row);
        }
        if (q.lastError().isValid())
            QTest::qFail("fixture fetch failed", __FILE__, __LINE__);
        return result;
    }
    QVariant scalar(const QString &sql) {
        const auto r = rows(sql);
        if (r.size() != 1 || r[0].size() != 1) {
            QTest::qFail("fixture scalar shape", __FILE__, __LINE__);
            return {};
        }
        return r[0][0];
    }
    bool released() { return exec("BEGIN IMMEDIATE") && exec("ROLLBACK"); }
    QList<SqlRows> snapshot() {
        QList<SqlRows> result;
        const auto schema =
            rows("SELECT type,name,tbl_name,sql FROM main.sqlite_schema ORDER BY type,name");
        result.append(schema);
        for (const auto &object : schema) {
            if (object[0].toString() != "table")
                continue;
            const auto table = object[1].toString();
            QStringList projection;
            for (const auto &column : rows("PRAGMA main.table_info(" + quoted(table) + ")")) {
                const auto c = quoted(column[1].toString());
                projection.append("typeof(" + c + ")");
                projection.append("hex(CAST(" + c + " AS BLOB))");
            }
            result.append(rows("SELECT " + projection.join(',') + " FROM main." + quoted(table) +
                               " ORDER BY rowid"));
        }
        return result;
    }
    bool original(const QByteArray &id, const ReceiptInput &v) {
        QStringList columns = {"receipt_id", "provider", "source_account", "identifier_namespace",
                               "opaque_id"};
        columns.append(facts_columns);
        columns.append(arrival_columns);
        QList<QVariant> values = {
            id, blob(v.identity.origin.provider), blob(v.identity.origin.source_account),
            blob(v.identity.identifier_namespace), blob(v.identity.opaque_id)};
        values.append(facts_values(v.facts));
        values.append(arrival_values(v.arrival));
        return insert("subathon_receipts", columns, values);
    }
    bool observation(const QByteArray &id, qint64 ordinal, const ReceiptAttempt &v) {
        QStringList columns = {"receipt_id", "ordinal"};
        columns.append(facts_columns);
        columns.append(arrival_columns);
        columns.append("disposition");
        QList<QVariant> values = {id, ordinal};
        values.append(facts_values(v.observed_facts));
        values.append(arrival_values(v.arrival));
        int disposition = v.disposition == ReceiptDisposition::created ? 0
                          : v.disposition == ReceiptDisposition::retry ? 1
                                                                       : 2;
        values.append(disposition);
        return insert("subathon_receipt_attempts", columns, values);
    }
    bool seed(const QByteArray &id, const ReceiptInput &v) {
        return original(id, v) && observation(id, 1, attempt(v, ReceiptDisposition::created)) &&
               exec("UPDATE main.subathon_receipt_allocator SET last_allocated=? WHERE singleton=1",
                    {id});
    }

    // Explicit independent COMMIT establishes prior receipt existence, not same-unit visibility.
    bool committed_receipt(const ContributionReceipt &r) {
        const ReceiptInput v{r.identity, r.facts, r.arrival};
        if (!exec("BEGIN IMMEDIATE"))
            return false;
        if (!original(id_blob(r.id.value), v) ||
            !observation(id_blob(r.id.value), 1, attempt(v, ReceiptDisposition::created)) ||
            !exec("UPDATE main.subathon_receipt_allocator SET last_allocated=(SELECT "
                  "max(receipt_id) FROM main.subathon_receipts)")) {
            (void)exec("ROLLBACK");
            return false;
        }
        return exec("COMMIT");
    }
    bool contribution(const QByteArray &id, const ContributionObservation &s,
                      const ContributionState &state) {
        QStringList columns = {"contribution_id", "original_receipt_id"};
        columns.append(activity_columns);
        columns.append("payment_evidence");
        columns.append(condition_columns);
        QList<QVariant> values = {id, id_blob(s.receipt.id.value)};
        values.append(activity_values(s.verified_activity ? std::optional<VerifiedActivityIdentity>(
                                                                s.verified_activity->identity)
                                                          : std::nullopt));
        values.append(int(state.payment_evidence));
        values.append(conditions_values(state.conditions));
        return insert("subathon_contributions", columns, values);
    }
    bool evidence(const ContributionEvidenceRecord &r) {
        const auto &o = r.observation;
        QStringList columns = {"receipt_id", "contribution_id", "kind", "evidence"};
        columns.append(activity_columns);
        columns.append("verification_ref");
        for (const auto &c : condition_columns)
            columns.append("support_" + c);
        QList<QVariant> values = {id_blob(o.receipt.id.value),
                                  r.contribution_id ? QVariant(id_blob(r.contribution_id->value))
                                                    : QVariant(),
                                  int(o.kind), int(o.evidence)};
        values.append(activity_values(o.verified_activity ? std::optional<VerifiedActivityIdentity>(
                                                                o.verified_activity->identity)
                                                          : std::nullopt));
        values.append(o.verified_activity ? QVariant(blob(o.verified_activity->verification_ref))
                                          : QVariant());
        if (o.support_conditions)
            values.append(conditions_values(*o.support_conditions));
        else
            for (qsizetype i = 0; i < condition_columns.size(); ++i)
                values.append(QVariant());
        return insert("subathon_contribution_observations", columns, values);
    }
    bool seeded_contribution(const QByteArray &id, const ContributionObservation &s,
                             const ContributionState &state) {
        return contribution(id, s, state) && evidence({s, ContributionId{decode_seed(id)}}) &&
               exec("UPDATE main.subathon_contribution_allocator SET last_allocated=?", {id});
    }
    static std::uint64_t decode_seed(const QByteArray &id) {
        std::uint64_t value = 0;
        for (const auto c : id)
            value = (value << 8) | static_cast<unsigned char>(c);
        return value;
    }

  private:
    bool insert(const QString &table, const QStringList &columns, const QList<QVariant> &values) {
        QStringList marks;
        for (qsizetype i = 0; i < values.size(); ++i)
            marks.append("?");
        return exec("INSERT INTO main." + table + "(" + columns.join(',') + ") VALUES (" +
                        marks.join(',') + ")",
                    values);
    }
    QString name_;
    QSqlDatabase db_;
};
MigrationStep literal_step(const QJsonObject &o) {
    MigrationStep s;
    s.version = o["version"].toInt();
    s.id = o["id"].toString();
    for (const auto &sql : o["sql"].toArray())
        s.sql.append(sql.toString());
    for (const auto &entry : o["expected_schema"].toArray()) {
        const auto x = entry.toObject();
        s.expected_schema.append(
            {x["type"].toString(), x["name"].toString(), x["table_name"].toString(),
             x["sql"].isNull() ? std::nullopt : std::optional<QString>(x["sql"].toString())});
    }
    return s;
}
struct Run {
    bool admitted{false}, done{false}, stub{false};
    std::optional<SqliteContributionInputError> invalid;
    std::optional<SqliteContributionStorageReason> storage;
    std::optional<WorkError> synchronous;
    QString unexpected;
    SqliteWorkResult result;
    QString problem() const {
        if (stub)
            return "SqliteContributionsNotImplemented: explicit missing contribution behavior";
        if (!unexpected.isEmpty())
            return unexpected;
        return QString("input=%1 storage=%2 sync=%3 outer=%4")
            .arg(invalid ? int(*invalid) : -1)
            .arg(storage ? int(*storage) : -1)
            .arg(synchronous ? int(*synchronous) : -1)
            .arg(int(result.error));
    }
};
Run run(SqliteWriter &writer, std::function<QVariant(SqliteWorkContext)> body) {
    auto state = std::make_shared<Run>();
    state->admitted = writer.enqueue_work(
                          [state, body = std::move(body)](auto work) {
                              try {
                                  return body(work);
                              } catch (const SqliteContributionsNotImplemented &) {
                                  state->stub = true;
                                  throw;
                              } catch (const SqliteContributionInputFailure &e) {
                                  state->invalid = e.error();
                                  throw;
                              } catch (const SqliteContributionStorageFailure &e) {
                                  state->storage = e.reason();
                                  throw;
                              } catch (const SqliteWorkFailure &e) {
                                  state->synchronous = e.error();
                                  throw;
                              } catch (const std::exception &e) {
                                  state->unexpected = QString::fromUtf8(e.what());
                                  throw;
                              }
                          },
                          [state](SqliteWorkResult r) {
                              state->result = std::move(r);
                              state->done = true;
                          }) == StorageError::none;
    if (state->admitted && !QTest::qWaitFor([state] { return state->done; }, 5000))
        state->unexpected = "Timed out waiting for writer completion";
    return *state;
}
ContributionRecord contribution_found(ContributionTransaction &tx,
                                      const VerifiedActivityIdentity &key) {
    auto value = tx.find_activity(key);
    require(value.has_value(), "expected contribution missing");
    return *value;
}
ContributionEvidenceRecord observation_found(ContributionTransaction &tx, ReceiptId id) {
    auto value = tx.find_receipt(id);
    require(value.has_value(), "expected observation missing");
    return *value;
}
VerifiedActivityIdentity key(const ContributionObservation &o) {
    return o.verified_activity->identity;
}
} // namespace
#define OK(r)                                                                                      \
    QVERIFY((r).admitted && (r).done);                                                             \
    QVERIFY2(!(r).stub && !(r).invalid && !(r).storage && (r).unexpected.isEmpty(),                \
             qPrintable((r).problem()));                                                           \
    QCOMPARE((r).result.transaction, TransactionOutcome::committed);                               \
    QCOMPARE((r).result.error, WorkError::none);                                                   \
    QVERIFY((r).result.value)
#define KNOWN(r, reason, outer)                                                                    \
    QVERIFY((r).admitted && (r).done);                                                             \
    QVERIFY2(!(r).stub, qPrintable((r).problem()));                                                \
    QCOMPARE((r).storage, std::optional<SqliteContributionStorageReason>(reason));                 \
    QCOMPARE((r).result.transaction, TransactionOutcome::rolled_back);                             \
    QCOMPARE((r).result.error, outer);                                                             \
    QVERIFY(!(r).result.value)

// Policy-free decorator: real primitive, observed SQL delta, then real SQL fault.
class FailAfterChange final : public ContributionRepository {
  public:
    enum class Point { create, observation, link, state };
    FailAfterChange(ContributionRepository &inner, SqliteWorkSession session, Point point,
                    std::array<bool, 4> &stages, std::function<bool()> rollback_probe)
        : inner_(inner), session_(std::move(session)), point_(point), stages_(stages),
          probe_(std::move(rollback_probe)) {}
    std::unique_ptr<ContributionTransaction> begin() override {
        class Transaction final : public ContributionTransaction {
          public:
            Transaction(std::unique_ptr<ContributionTransaction> inner, SqliteWorkSession session,
                        Point point, std::array<bool, 4> &stages, std::function<bool()> probe)
                : inner_(std::move(inner)), session_(std::move(session)), point_(point),
                  stages_(stages), probe_(std::move(probe)) {}
            std::optional<ContributionEvidenceRecord> find_receipt(ReceiptId id) override {
                return inner_->find_receipt(id);
            }
            std::optional<ContributionRecord>
            find_activity(const VerifiedActivityIdentity &k) override {
                return inner_->find_activity(k);
            }
            std::vector<ContributionEvidenceRecord>
            unresolved_activity(const VerifiedActivityIdentity &k) override {
                return inner_->unresolved_activity(k);
            }
            ContributionId create(const ContributionReceipt &r,
                                  const std::optional<VerifiedActivityIdentity> &k,
                                  const ContributionState &state) override {
                const auto before = count("subathon_contributions");
                const auto id = inner_->create(r, k, state);
                require(count("subathon_contributions") == before + 1,
                        "new contribution row observed");
                stages_[0] = true;
                if (point_ == Point::create)
                    fail();
                return id;
            }
            void append_observation(const ContributionEvidenceRecord &r) override {
                const auto before = count("subathon_contribution_observations");
                inner_->append_observation(r);
                require(count("subathon_contribution_observations") == before + 1,
                        "new observation row observed");
                stages_[1] = true;
                if (point_ == Point::observation)
                    fail();
            }
            void link_observation(ReceiptId receipt, ContributionId id) override {
                const auto before =
                    session_.execute({"SELECT contribution_id,typeof(contribution_id) FROM "
                                      "main.subathon_contribution_observations WHERE receipt_id=?",
                                      {id_blob(receipt.value)}});
                require(before.size() == 1 && before[0].size() == 2 &&
                            before[0][1].toString() == "null" && before[0][0].isNull(),
                        "link starts SQLNULL");
                inner_->link_observation(receipt, id);
                const auto after =
                    session_.execute({"SELECT contribution_id,typeof(contribution_id) FROM "
                                      "main.subathon_contribution_observations WHERE receipt_id=?",
                                      {id_blob(receipt.value)}});
                require(after.size() == 1 && after[0].size() == 2 &&
                            after[0][1].toString() == "blob" &&
                            after[0][0].toByteArray() == id_blob(id.value),
                        "actual NULL to requested BLOB ID incl0");
                stages_[2] = true;
                if (point_ == Point::link)
                    fail();
            }
            void update_state(ContributionId id, const ContributionState &state) override {
                const auto before =
                    session_.execute({"SELECT payment_evidence FROM main.subathon_contributions "
                                      "WHERE contribution_id=?",
                                      {id_blob(id.value)}});
                inner_->update_state(id, state);
                const auto after =
                    session_.execute({"SELECT payment_evidence FROM main.subathon_contributions "
                                      "WHERE contribution_id=?",
                                      {id_blob(id.value)}});
                require(before.size() == 1 && after.size() == 1 && before != after &&
                            after[0][0].toInt() == int(state.payment_evidence),
                        "actual requested evidence delta before fault");
                stages_[3] = true;
                if (point_ == Point::state)
                    fail();
            }
            void commit() override { inner_->commit(); }

          private:
            qint64 count(const QString &table) {
                return session_.execute({"SELECT count(*) FROM main." + table, {}})[0][0]
                    .toLongLong();
            }
            [[noreturn]] void fail() {
                try {
                    (void)session_.execute(
                        {"INSERT INTO main.no_such_contribution_fault VALUES (1)", {}});
                } catch (const SqliteWorkFailure &e) {
                    if (e.error() == WorkError::sql_failure) {
                        require(probe_(), "known SQL rollback while wrapper alive");
                        throw SqliteContributionStorageFailure(
                            SqliteContributionStorageReason::sql_failure);
                    }
                    throw;
                }
                throw std::runtime_error("fault SQL unexpectedly accepted");
            }
            std::unique_ptr<ContributionTransaction> inner_;
            SqliteWorkSession session_;
            Point point_;
            std::array<bool, 4> &stages_;
            std::function<bool()> probe_;
        };
        return std::make_unique<Transaction>(inner_.begin(), session_, point_, stages_, probe_);
    }

  private:
    ContributionRepository &inner_;
    SqliteWorkSession session_;
    Point point_;
    std::array<bool, 4> &stages_;
    std::function<bool()> probe_;
};

class SqliteContributionTests final : public QObject {
    Q_OBJECT
  private:
    QJsonObject json_, m1_, m2_, m3_;
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<Fixture> fixture_;
    QString path_;
    MigrationManifest literals() const {
        MigrationManifest m;
        m.steps = {literal_step(m1_), literal_step(m2_), literal_step(m3_)};
        m.supported_starting_versions = {0, 1, 2, 3};
        return m;
    }
    bool install(const QString &path) {
        SqliteWriter writer;
        StorageLimits limits;
        limits.statements_per_batch = 512;
        if (writer.open(path, limits) != StorageError::none)
            return false;
        MigrationRequest request;
        request.mode = MigrationMode::advance;
        request.manifest = literals();
        struct State {
            bool done = false;
            MigrationResult result;
        };
        auto state = std::make_shared<State>();
        const auto a = SqliteMigrationRunner::enqueue(writer, std::move(request), [state](auto r) {
            state->result = std::move(r);
            state->done = true;
        });
        return a.error == StorageError::none &&
               QTest::qWaitFor([state] { return state->done; }, 5000) &&
               state->result.reason == MigrationReason::none &&
               state->result.work.transaction == TransactionOutcome::committed;
    }
    StorageError open(SqliteWriter &writer, StorageLimits limits = {}) {
        limits.statements_per_batch = 512;
        return writer.open(path_, limits);
    }
    qint64 count(const QString &table) {
        return fixture_->scalar("SELECT count(*) FROM main." + table).toLongLong();
    }
    bool commit_receipt(const ContributionObservation &o) {
        return fixture_->committed_receipt(o.receipt);
    }
    Run observe(SqliteWriter &writer, const ContributionObservation &o,
                std::optional<ContributionLinkResult> &result) {
        result.reset();
        return run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            ContributionEvidenceService service(repository);
            result = service.observe(o);
            return int(result->disposition);
        });
    }
    Run read(SqliteWriter &writer, const VerifiedActivityIdentity &identity,
             std::optional<ContributionRecord> &result, SqliteContributionLimits limits = {}) {
        result.reset();
        return run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work, limits);
            auto tx = repository.begin();
            result = tx->find_activity(identity);
            tx->commit();
            return 1;
        });
    }
  private slots:
    void initTestCase() {
        QVERIFY(QSqlDatabase::isDriverAvailable("QSQLITE"));
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
        for (const auto &pair : {std::pair{QString(SUBATHON_CONTRIBUTION_VECTOR_PATH), &json_},
                                 std::pair{QString(SUBATHON_RECEIPT_VECTOR_PATH), &m2_},
                                 std::pair{QString(SUBATHON_RETAINED_VECTOR_PATH), &m1_}}) {
            QFile file(pair.first);
            QVERIFY(file.open(QIODevice::ReadOnly));
            QJsonParseError e;
            const auto j = QJsonDocument::fromJson(file.readAll(), &e);
            QCOMPARE(e.error, QJsonParseError::NoError);
            *pair.second = j.object();
        }
        m1_ = m1_["manifest"].toObject();
        m2_ = m2_["manifest"].toObject();
        m3_ = json_["manifest"].toObject();
        QCOMPARE(m3_["version"].toInt(), 3);
        QCOMPARE(m3_["payload_bytes"].toInt(), 18126);
        QCOMPARE(m3_["sha256_hex"].toString(),
                 QString("b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d"));
    }
    void init() {
        directory_ = std::make_unique<QTemporaryDir>();
        QVERIFY(directory_->isValid());
        path_ = directory_->filePath("contributions.sqlite");
        QVERIFY(install(path_));
        fixture_ = std::make_unique<Fixture>(path_);
        QVERIFY(fixture_->open());
        QVERIFY(fixture_->exec("INSERT INTO main.subathon_retained_configurations VALUES "
                               "(X'63666731',X'63'),(X'63666732',X'64')"));
        QVERIFY(fixture_->exec("INSERT INTO main.subathon_retained_contexts VALUES "
                               "(X'63747831',X'74',X'63666731'),(X'63747832',X'75',NULL)"));
    }
    void cleanup() {
        fixture_.reset();
        directory_.reset();
    }

    void codecsPreserveZeroAndFullUnsignedRange() {
        constexpr std::array<std::uint64_t, 6> values = {0,
                                                         1,
                                                         9007199254740993ULL,
                                                         9223372036854775807ULL,
                                                         9223372036854775808ULL,
                                                         18446744073709551615ULL};
        const auto vectors = json_["codec_vectors"].toArray();
        QCOMPARE(vectors.size(), 6);
        try {
            for (qsizetype i = 0; i < 6; ++i) {
                const auto v = vectors[i].toObject();
                const auto expected = QByteArray::fromHex(v["hex"].toString().toLatin1());
                QCOMPARE(v["decimal"].toString(), QString::number(values[size_t(i)]));
                QCOMPARE(expected.size(), 8);
                QCOMPARE(encode_contribution_id({values[size_t(i)]}), expected);
                QCOMPARE(decode_contribution_id(expected).value, values[size_t(i)]);
            }
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void codecRejectsMalformedValues() {
        try {
            for (const auto &b :
                 {QByteArray(), QByteArray("", 0), QByteArray(7, '\0'), QByteArray(9, '\0')})
                expect_input([&] { (void)decode_contribution_id(b); },
                             SqliteContributionInputError::invalid_codec);
            QCOMPARE(decode_contribution_id(QByteArray(8, '\0')).value, std::uint64_t(0));
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void manifestPreservesReceiptPrefixAndMatchesLiteralStep() {
        try {
            const auto actual = contribution_repository_manifest();
            const auto expected = literals();
            QCOMPARE(actual.catalog_format, 1);
            QCOMPARE(actual.store_identity, QString("subathon.sqlite-store"));
            QCOMPARE(actual.supported_starting_versions, QList<qint32>({0, 1, 2, 3}));
            QCOMPARE(actual.steps.size(), 3);
            for (qsizetype i = 0; i < 3; ++i) {
                const auto &a = actual.steps[i], &e = expected.steps[i];
                QCOMPARE(a.version, e.version);
                QCOMPARE(a.id, e.id);
                QCOMPARE(a.sql, e.sql);
                QCOMPARE(a.expected_schema, e.expected_schema);
                const auto j = i == 0 ? m1_ : i == 1 ? m2_ : m3_;
                QCOMPARE(canonical_migration_payload(a),
                         QByteArray::fromHex(j["payload_hex"].toString().toLatin1()));
                QCOMPARE(migration_content_sha256(a),
                         QByteArray::fromHex(j["sha256_hex"].toString().toLatin1()));
            }
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void supportConfirmationAndDuplicatePreserveOriginals() {
        const auto s = support();
        const auto c = confirmation();
        auto delayed = support(3);
        delayed.receipt.arrival = c.receipt.arrival;
        delayed.evidence = PaymentEvidence::unconfirmed;
        delayed.support_conditions->valuation.resolution_ref = "later";
        delayed.support_conditions->valuation.missing_rate = false;
        delayed.support_conditions->review = ContributionReview::accepted;
        auto repeated = confirmation(4);
        repeated.verified_activity->verification_ref = "proof3";
        QVERIFY(commit_receipt(s));
        QVERIFY(commit_receipt(c));
        QVERIFY(commit_receipt(delayed));
        QVERIFY(commit_receipt(repeated));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        std::optional<ContributionLinkResult> result;
        auto a = observe(writer, s, result);
        OK(a);
        QVERIFY(result && result->contribution_id.has_value());
        QCOMPARE(result->contribution_id->value, std::uint64_t(0));
        QCOMPARE(result->disposition, ContributionLinkDisposition::created);
        auto duplicate = observe(writer, s, result);
        OK(duplicate);
        QVERIFY(result);
        QCOMPARE(result->disposition, ContributionLinkDisposition::duplicate);
        QVERIFY(result->contribution_id);
        QCOMPARE(result->contribution_id->value, std::uint64_t(0));
        auto b = observe(writer, c, result);
        OK(b);
        QVERIFY(result);
        QCOMPARE(result->disposition, ContributionLinkDisposition::linked);
        auto d = observe(writer, delayed, result);
        OK(d);
        auto r = observe(writer, repeated, result);
        OK(r);
        std::optional<ContributionRecord> record;
        auto q = read(writer, key(s), record);
        OK(q);
        QVERIFY(record);
        QCOMPARE(record->id.value, std::uint64_t(0));
        QCOMPARE(record->original_receipt_id, s.receipt.id);
        QCOMPARE(record->original_facts, s.receipt.facts);
        QCOMPARE(record->original_arrival, s.receipt.arrival);
        auto expected = initial(s);
        expected.payment_evidence = PaymentEvidence::confirmed;
        QCOMPARE(record->state, expected);
        auto audit = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            for (const auto &o : {s, c, delayed, repeated}) {
                const auto row = observation_found(*tx, o.receipt.id);
                require(row.observation == o, "each immutable observation exact");
                require(row.contribution_id.has_value() && row.contribution_id->value == 0,
                        "present0 association");
            }
            tx->commit();
            return 4;
        });
        OK(audit);
        QCOMPARE(count("subathon_contributions"), 1);
        QCOMPARE(count("subathon_contribution_observations"), 4);
        QCOMPARE(fixture_->rows(
                     "SELECT typeof(contribution_id),length(contribution_id),hex(contribution_id) "
                     "FROM main.subathon_contributions"),
                 SqlRows({{"blob", 8, "0000000000000000"}}));
        const auto before_reuse = fixture_->snapshot();
        auto contradictory = s;
        contradictory.support_conditions->evidence_admission.policy_ref = "changed-same-receipt";
        bool rejected = false;
        auto reuse = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            ContributionEvidenceService service(repository);
            try {
                (void)service.observe(contradictory);
            } catch (const SqliteContributionInputFailure &) {
                throw;
            } catch (const std::invalid_argument &) {
                rejected = true;
                throw;
            }
            return 1;
        });
        QVERIFY2(rejected, qPrintable(reuse.problem()));
        QVERIFY(!reuse.invalid && !reuse.storage);
        QCOMPARE(reuse.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(reuse.result.error, WorkError::work_exception);
        QVERIFY(!reuse.result.value);
        QCOMPARE(fixture_->snapshot(), before_reuse);
    }
    void pendingConfirmationLinksToNewZeroContribution() {
        const auto s = support();
        const auto c = confirmation();
        QVERIFY(commit_receipt(s));
        QVERIFY(commit_receipt(c));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        std::optional<ContributionLinkResult> result;
        auto pending = observe(writer, c, result);
        OK(pending);
        QVERIFY(result);
        QVERIFY(!result->contribution_id);
        QCOMPARE(result->disposition, ContributionLinkDisposition::unresolved);
        QCOMPARE(count("subathon_contributions"), 0);
        QCOMPARE(
            fixture_
                ->scalar(
                    "SELECT typeof(contribution_id) FROM main.subathon_contribution_observations")
                .toString(),
            QString("null"));
        auto before = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            require(tx->unresolved_activity(key(c)) ==
                        std::vector<ContributionEvidenceRecord>({{c, {}}}),
                    "complete pending confirmation");
            tx->commit();
            return 1;
        });
        OK(before);
        auto linked = observe(writer, s, result);
        OK(linked);
        QVERIFY(result && result->contribution_id);
        QCOMPARE(result->contribution_id->value, std::uint64_t(0));
        auto after = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            require(tx->unresolved_activity(key(c)).empty(), "BLOB0 is not unresolved NULL");
            const auto row = observation_found(*tx, c.receipt.id);
            require(row.observation == c && row.contribution_id.has_value() &&
                        row.contribution_id->value == 0,
                    "pending observation linked once to0");
            require(contribution_found(*tx, key(s)).state.payment_evidence ==
                        PaymentEvidence::confirmed,
                    "pending confirmation promotes via service");
            tx->commit();
            return 1;
        });
        OK(after);
        QCOMPARE(
            fixture_
                ->scalar(
                    "SELECT count(*) FROM main.subathon_contribution_observations WHERE "
                    "typeof(contribution_id)='blob' AND hex(contribution_id)='0000000000000000'")
                .toInt(),
            2);
        auto dup = observe(writer, c, result);
        OK(dup);
        QVERIFY(result);
        QCOMPARE(result->disposition, ContributionLinkDisposition::duplicate);
        QCOMPARE(count("subathon_contribution_observations"), 2);
    }
    void verifiedIdentityAndOpaqueProofBytesRemainExact() {
        const auto raw = opaque(QByteArray::fromHex("efbbbf6100ff"));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        for (int component = 0; component < 4; ++component) {
            auto s = support(std::uint64_t(component + 1));
            auto &k = s.verified_activity->identity;
            if (component == 0) {
                k.origin.provider = raw;
                s.receipt.identity.origin.provider = raw;
            }
            if (component == 1) {
                k.origin.source_account = raw;
                s.receipt.identity.origin.source_account = raw;
            }
            if (component == 2)
                k.identifier_namespace = raw;
            if (component == 3)
                k.opaque_id = raw;
            s.verified_activity->verification_ref = raw;
            QVERIFY(commit_receipt(s));
            std::optional<ContributionLinkResult> result;
            auto r = observe(writer, s, result);
            OK(r);
            QVERIFY(result && result->contribution_id);
            QCOMPARE(result->disposition, ContributionLinkDisposition::created);
            std::optional<ContributionRecord> record;
            auto q = read(writer, k, record);
            OK(q);
            QVERIFY(record);
            QCOMPARE(record->activity_identity, std::optional<VerifiedActivityIdentity>(k));
        }
        QCOMPARE(count("subathon_contributions"), 4);
        for (const auto &column : activity_columns)
            QCOMPARE(fixture_
                         ->scalar("SELECT count(*) FROM main.subathon_contributions WHERE typeof(" +
                                  column + ")='blob' AND hex(" + column + ")='EFBBBF6100FF'")
                         .toInt(),
                     1);
        QCOMPARE(
            fixture_
                ->scalar("SELECT count(*) FROM main.subathon_contribution_observations WHERE "
                         "typeof(verification_ref)='blob' AND hex(verification_ref)='EFBBBF6100FF'")
                .toInt(),
            4);
        auto s = support(10);
        s.verified_activity.reset();
        auto c = confirmation(11);
        c.verified_activity.reset();
        c.receipt.facts = s.receipt.facts;
        c.receipt.arrival = s.receipt.arrival;
        QVERIFY(commit_receipt(s));
        QVERIFY(commit_receipt(c));
        std::optional<ContributionLinkResult> result;
        auto a = observe(writer, s, result);
        OK(a);
        auto b = observe(writer, c, result);
        OK(b);
        QVERIFY(result);
        QVERIFY(!result->contribution_id);
        QCOMPARE(result->disposition, ContributionLinkDisposition::unresolved);
        QCOMPARE(count("subathon_contributions"), 5);
        QCOMPARE(count("subathon_contribution_observations"), 6);
    }
    void independentStateAndCapturedConditionsRoundTrip() {
        auto s = support();
        s.support_conditions->evidence_admission.policy_ref = "";
        s.support_conditions->evidence_admission.review_decision =
            EvidenceReviewDecision{EvidenceReviewChoice::accept_labelled_report, "", ""};
        s.support_conditions->valuation.resolution_ref = std::string();
        const auto c = confirmation();
        QVERIFY(commit_receipt(s));
        QVERIFY(commit_receipt(c));
        ContributionState expected;
        {
            SqliteWriter writer;
            QCOMPARE(open(writer), StorageError::none);
            std::optional<ContributionLinkResult> result;
            auto a = observe(writer, s, result);
            OK(a);
            expected = initial(s);
            expected.conditions.valuation = {
                false, ReceiptMoney{"9007199254740993.00000000000000000001", "EUR"}, std::string(),
                true};
            expected.conditions.review = ContributionReview::rejected;
            expected.conditions.application = ContributionApplication::applied;
            expected.conditions.reversal = ContributionReversal::full;
            expected.conditions.settlement = SettlementGate::unresolved;
            auto update = run(writer, [&](auto work) -> QVariant {
                SqliteContributionRepository repository(work);
                auto tx = repository.begin();
                tx->update_state({0}, expected);
                tx->commit();
                return 1;
            });
            OK(update);
            auto b = observe(writer, c, result);
            OK(b);
            expected.payment_evidence = PaymentEvidence::confirmed;
            auto check = run(writer, [&](auto work) -> QVariant {
                SqliteContributionRepository repository(work);
                auto tx = repository.begin();
                require(contribution_found(*tx, key(s)).state == expected,
                        "confirmation preserves independent supplied dimensions");
                require(observation_found(*tx, s.receipt.id).observation.support_conditions ==
                            s.support_conditions,
                        "immutable support conditions distinct from current state");
                tx->commit();
                return 1;
            });
            OK(check);
        }
        SqliteWriter reopened;
        QCOMPARE(open(reopened), StorageError::none);
        std::optional<ContributionRecord> owned;
        auto r = read(reopened, key(s), owned);
        OK(r);
        QVERIFY(owned);
        QCOMPARE(owned->state, expected);
        QCOMPARE(fixture_->rows(
                     "SELECT "
                     "typeof(policy_ref),length(policy_ref),typeof(decision_ref),length(decision_"
                     "ref),typeof(decision_actor_ref),length(decision_actor_ref),typeof(resolution_"
                     "ref),length(resolution_ref),missing_rate,monetary,review,application,"
                     "reversal,settlement FROM main.subathon_contributions"),
                 SqlRows({{"blob", 0, "blob", 0, "blob", 0, "blob", 0, 1, 0, 3, 1, 2, 0}}));
        auto defaults = support(3);
        defaults.verified_activity->identity.opaque_id = "defaults";
        defaults.support_conditions = ContributionConditions{};
        QVERIFY(commit_receipt(defaults));
        std::optional<ContributionLinkResult> result;
        auto d = observe(reopened, defaults, result);
        OK(d);
        std::optional<ContributionRecord> def;
        auto q = read(reopened, key(defaults), def);
        OK(q);
        QVERIFY(def);
        QCOMPARE(def->state, initial(defaults));
        QVERIFY(!def->state.conditions.valuation.original_money);
        QVERIFY(!def->state.conditions.valuation.resolution_ref);
    }
    void committedReceiptProjectionAndTargetsAreRequired() {
        const auto s = support();
        QVERIFY(commit_receipt(s));
        auto latest = s.receipt;
        latest.facts.money->original_decimal = "99.0000";
        latest.arrival = {1700000009000, "later", "cfg2", "ctx2"};
        QVERIFY(fixture_->observation(
            id_blob(1), 2, {latest.arrival, latest.facts, ReceiptDisposition::conflict}));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        for (const bool absent : {false, true}) {
            auto candidate = absent ? s.receipt : latest;
            if (absent)
                candidate.id = {99};
            const auto before = fixture_->snapshot();
            bool caught = false;
            const auto expected = absent ? SqliteContributionStorageReason::missing_receipt
                                         : SqliteContributionStorageReason::receipt_mismatch;
            auto r = run(writer, [&](auto work) -> QVariant {
                SqliteContributionRepository repository(work);
                auto tx = repository.begin();
                try {
                    (void)tx->create(candidate, key(s), initial(s));
                } catch (const SqliteContributionStorageFailure &e) {
                    require(e.reason() == expected, "actual original projection refusal");
                    require(fixture_->released(), "original refusal rollback while wrapper alive");
                    caught = true;
                    throw;
                }
                return 1;
            });
            QVERIFY2(caught, qPrintable(r.problem()));
            KNOWN(r, expected, WorkError::work_exception);
            QCOMPARE(fixture_->snapshot(), before);
        }
        auto lookup = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            require(!tx->find_receipt(s.receipt.id),
                    "receipt existence is not contribution observation existence");
            tx->commit();
            return 1;
        });
        OK(lookup);
        std::optional<ContributionLinkResult> result;
        auto correct = observe(writer, s, result);
        OK(correct);
        QVERIFY(result && result->contribution_id);
        QCOMPARE(result->contribution_id->value, std::uint64_t(0));
        std::optional<ContributionRecord> record;
        auto q = read(writer, key(s), record);
        OK(q);
        QVERIFY(record);
        QCOMPARE(record->original_facts, s.receipt.facts);
        QCOMPARE(record->original_arrival, s.receipt.arrival);
    }

    void allocatorCrossesSignedBoundaryAndExhausts() {
        auto s1 = support(1), s2 = support(2), s3 = support(3), s4 = support(4), s5 = support(5);
        s1.verified_activity->identity.opaque_id = "seed-signed";
        s2.verified_activity->identity.opaque_id = "above-signed";
        s3.verified_activity->identity.opaque_id = "seed-before-max";
        s4.verified_activity->identity.opaque_id = "at-max";
        s5.verified_activity->identity.opaque_id = "no-wrap";
        for (const auto &s : {s1, s2, s3, s4, s5})
            QVERIFY(commit_receipt(s));
        QCOMPARE(
            fixture_
                ->scalar("SELECT typeof(last_allocated) FROM main.subathon_contribution_allocator")
                .toString(),
            QString("null"));
        QVERIFY(fixture_->seeded_contribution(QByteArray::fromHex("7fffffffffffffff"), s1,
                                              initial(s1)));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        std::optional<ContributionLinkResult> result;
        auto a = observe(writer, s2, result);
        OK(a);
        QVERIFY(result && result->contribution_id);
        QCOMPARE(result->contribution_id->value, std::uint64_t(9223372036854775808ULL));
        QCOMPARE(
            fixture_->scalar("SELECT hex(last_allocated) FROM main.subathon_contribution_allocator")
                .toString(),
            QString("8000000000000000"));
        QVERIFY(fixture_->seeded_contribution(QByteArray::fromHex("fffffffffffffffe"), s3,
                                              initial(s3)));
        auto b = observe(writer, s4, result);
        OK(b);
        QVERIFY(result && result->contribution_id);
        QCOMPARE(result->contribution_id->value, std::numeric_limits<std::uint64_t>::max());
        const auto before = fixture_->snapshot();
        auto exhausted = observe(writer, s5, result);
        KNOWN(exhausted, SqliteContributionStorageReason::allocator_exhausted,
              WorkError::work_exception);
        QVERIFY(!result);
        QCOMPARE(fixture_->snapshot(), before);
        auto duplicate = observe(writer, s4, result);
        OK(duplicate);
        QVERIFY(result && result->contribution_id);
        QCOMPARE(result->contribution_id->value, std::numeric_limits<std::uint64_t>::max());
        QCOMPARE(result->disposition, ContributionLinkDisposition::duplicate);
    }
    void observationAssociationsRejectRetargetAndMissingRows() {
        const auto s = support();
        auto other = support(2);
        other.verified_activity->identity.opaque_id = "other";
        const auto c = confirmation(3);
        auto c2 = confirmation(4);
        c2.verified_activity = other.verified_activity;
        for (const auto &o : {s, other, c, c2})
            QVERIFY(commit_receipt(o));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        std::optional<ContributionLinkResult> result;
        auto a = observe(writer, s, result);
        OK(a);
        auto b = observe(writer, other, result);
        OK(b);
        auto append = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            tx->append_observation({c, {}});
            tx->append_observation({c2, {}});
            tx->link_observation(c.receipt.id, {0});
            tx->commit();
            return 1;
        });
        OK(append);
        const auto stable = fixture_->snapshot();
        auto same = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            tx->link_observation(c.receipt.id, {0});
            tx->commit();
            return 1;
        });
        OK(same);
        QCOMPARE(fixture_->snapshot(), stable);
        struct Bad {
            ReceiptId receipt;
            ContributionId target;
            SqliteContributionStorageReason reason;
        };
        for (const auto bad :
             {Bad{c.receipt.id, {1}, SqliteContributionStorageReason::association_conflict},
              Bad{{99}, {0}, SqliteContributionStorageReason::missing_observation},
              Bad{c.receipt.id, {99}, SqliteContributionStorageReason::missing_contribution},
              Bad{c2.receipt.id, {0}, SqliteContributionStorageReason::association_conflict}}) {
            const auto before = fixture_->snapshot();
            bool caught = false;
            auto r = run(writer, [&](auto work) -> QVariant {
                SqliteContributionRepository repository(work);
                auto tx = repository.begin();
                try {
                    tx->link_observation(bad.receipt, bad.target);
                } catch (const SqliteContributionStorageFailure &e) {
                    require(e.reason() == bad.reason, "association refusal category");
                    require(fixture_->released(),
                            "association refusal rollback while wrapper alive");
                    caught = true;
                    throw;
                }
                return 1;
            });
            QVERIFY2(caught, qPrintable(r.problem()));
            KNOWN(r, bad.reason, WorkError::work_exception);
            QCOMPARE(fixture_->snapshot(), before);
        }
        auto unkeyed_support = support(5);
        auto unkeyed_confirmation = confirmation(6);
        unkeyed_support.verified_activity.reset();
        unkeyed_confirmation.verified_activity.reset();
        QVERIFY(commit_receipt(unkeyed_support));
        QVERIFY(commit_receipt(unkeyed_confirmation));
        auto own = observe(writer, unkeyed_support, result);
        OK(own);
        QVERIFY(result && result->contribution_id);
        const auto own_id = *result->contribution_id;
        auto unresolved = observe(writer, unkeyed_confirmation, result);
        OK(unresolved);
        QVERIFY(result && !result->contribution_id);
        const auto no_key_before = fixture_->snapshot();
        bool refused = false;
        auto no_key = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            try {
                tx->link_observation(unkeyed_confirmation.receipt.id, own_id);
            } catch (const SqliteContributionStorageFailure &e) {
                require(e.reason() == SqliteContributionStorageReason::association_conflict,
                        "unkeyed resemblance cannot attach a different receipt");
                require(fixture_->released(), "no-key association rollback while wrapper alive");
                refused = true;
                throw;
            }
            return 1;
        });
        QVERIFY2(refused, qPrintable(no_key.problem()));
        KNOWN(no_key, SqliteContributionStorageReason::association_conflict,
              WorkError::work_exception);
        QCOMPARE(fixture_->snapshot(), no_key_before);
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        QVERIFY(fixture_->exec("UPDATE main.subathon_contributions SET policy_ref='text' WHERE "
                               "contribution_id=X'0000000000000000'"));
        const auto corrupt_before = fixture_->snapshot();
        auto corrupted = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            tx->link_observation(c.receipt.id, {0});
            return 1;
        });
        KNOWN(corrupted, SqliteContributionStorageReason::invalid_stored_data,
              WorkError::work_exception);
        QCOMPARE(fixture_->snapshot(), corrupt_before);
    }
    void primitiveUniquenessAndAmbiguityAreExplicit() {
        const auto s = support();
        auto other = support(2);
        const auto c = confirmation(3);
        for (const auto &o : {s, other, c})
            QVERIFY(commit_receipt(o));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        for (bool same_original : {false, true}) {
            const auto before = fixture_->snapshot();
            bool staged = false;
            auto r = run(writer, [&](auto work) -> QVariant {
                auto session = work.begin();
                SqliteContributionRepository repository(adopt_active_contribution_session, session);
                auto tx = repository.begin();
                (void)tx->create(s.receipt, key(s), initial(s));
                require(
                    session.execute({"SELECT count(*) FROM main.subathon_contributions", {}})[0][0]
                            .toInt() == 1,
                    "first row genuinely staged");
                staged = true;
                auto second_key = key(s);
                if (same_original)
                    second_key.opaque_id = "other";
                (void)tx->create(same_original ? s.receipt : other.receipt, second_key, initial(s));
                return 1;
            });
            QVERIFY2(staged, qPrintable(r.problem()));
            KNOWN(r, SqliteContributionStorageReason::sql_failure, WorkError::sql_failure);
            QCOMPARE(fixture_->snapshot(), before);
        }
        bool appended = false;
        const auto empty = fixture_->snapshot();
        auto repeated = run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteContributionRepository repository(adopt_active_contribution_session, session);
            auto tx = repository.begin();
            tx->append_observation({c, {}});
            require(session.execute({"SELECT count(*) FROM main.subathon_contribution_observations",
                                     {}})[0][0]
                            .toInt() == 1,
                    "first observation staged");
            appended = true;
            tx->append_observation({c, {}});
            return 1;
        });
        QVERIFY2(appended, qPrintable(repeated.problem()));
        KNOWN(repeated, SqliteContributionStorageReason::sql_failure, WorkError::sql_failure);
        QCOMPARE(fixture_->snapshot(), empty);
        QVERIFY(fixture_->seeded_contribution(id_blob(0), s, initial(s)));
        // Deliberately malformed-schema diagnostic bypasses UNIQUE only in fixture;
        // this is not a supported startup/catalog or import capability.
        QVERIFY(fixture_->exec(
            "ALTER TABLE main.subathon_contributions RENAME TO fixture_original_contributions"));
        QVERIFY(fixture_->exec(
            "CREATE TABLE subathon_contributions AS SELECT * FROM fixture_original_contributions"));
        QVERIFY(fixture_->exec("INSERT INTO main.subathon_contributions SELECT * FROM "
                               "fixture_original_contributions"));
        const auto before = fixture_->snapshot();
        std::optional<ContributionRecord> record;
        auto ambiguous = read(writer, key(s), record);
        KNOWN(ambiguous, SqliteContributionStorageReason::invalid_stored_data,
              WorkError::work_exception);
        QVERIFY(!record);
        QCOMPARE(fixture_->snapshot(), before);
    }
    void createAppendLinkAndUpdateFailuresRollBackAtomically() {
        const auto s = support();
        const auto c = confirmation();
        QVERIFY(commit_receipt(s));
        QVERIFY(commit_receipt(c));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        for (const auto point :
             {FailAfterChange::Point::create, FailAfterChange::Point::observation}) {
            const auto before = fixture_->snapshot();
            std::array<bool, 4> stages{};
            auto r = run(writer, [&](auto work) -> QVariant {
                auto session = work.begin();
                SqliteContributionRepository real(adopt_active_contribution_session, session);
                FailAfterChange fault(real, session, point, stages,
                                      [&] { return fixture_->released(); });
                ContributionEvidenceService service(fault);
                (void)service.observe(s);
                return 1;
            });
            QVERIFY2(stages[0], qPrintable(r.problem()));
            QCOMPARE(stages[1], point == FailAfterChange::Point::observation);
            KNOWN(r, SqliteContributionStorageReason::sql_failure, WorkError::sql_failure);
            QCOMPARE(fixture_->snapshot(), before);
        }
        std::optional<ContributionLinkResult> result;
        auto first = observe(writer, s, result);
        OK(first);
        for (const auto point :
             {FailAfterChange::Point::observation, FailAfterChange::Point::state}) {
            const auto before = fixture_->snapshot();
            std::array<bool, 4> stages{};
            auto r = run(writer, [&](auto work) -> QVariant {
                auto session = work.begin();
                SqliteContributionRepository real(adopt_active_contribution_session, session);
                FailAfterChange fault(real, session, point, stages,
                                      [&] { return fixture_->released(); });
                ContributionEvidenceService service(fault);
                (void)service.observe(c);
                return 1;
            });
            QVERIFY2(stages[1], qPrintable(r.problem()));
            QCOMPARE(stages[3], point == FailAfterChange::Point::state);
            KNOWN(r, SqliteContributionStorageReason::sql_failure, WorkError::sql_failure);
            QCOMPARE(fixture_->snapshot(), before);
        }
        auto pending_support = support(3);
        pending_support.verified_activity->identity.opaque_id = "pending-fault";
        auto pending_confirmation = confirmation(4);
        pending_confirmation.verified_activity->identity =
            pending_support.verified_activity->identity;
        QVERIFY(commit_receipt(pending_support));
        QVERIFY(commit_receipt(pending_confirmation));
        auto pending = observe(writer, pending_confirmation, result);
        OK(pending);
        QVERIFY(result && !result->contribution_id);
        const auto before = fixture_->snapshot();
        std::array<bool, 4> stages{};
        auto link = run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteContributionRepository real(adopt_active_contribution_session, session);
            FailAfterChange fault(real, session, FailAfterChange::Point::link, stages,
                                  [&] { return fixture_->released(); });
            ContributionEvidenceService service(fault);
            (void)service.observe(pending_support);
            return 1;
        });
        QVERIFY2(stages[0] && stages[1] && stages[2], qPrintable(link.problem()));
        KNOWN(link, SqliteContributionStorageReason::sql_failure, WorkError::sql_failure);
        QCOMPARE(fixture_->snapshot(), before);
        auto retry = observe(writer, pending_support, result);
        OK(retry);
        QVERIFY(result && result->contribution_id);
        QCOMPARE(result->disposition, ContributionLinkDisposition::created);
    }
    void adoptedSessionCommitsEarlierWorkBeforeObserveReturns() {
        const auto s = support();
        QVERIFY(commit_receipt(s));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        std::optional<ContributionLinkResult> result;
        auto r = run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteRetainedContextStore retained(session);
            require(retained.retain({{RetainedKind::configuration, "extra-valid-work"},
                                     QByteArray::fromHex("00ff"),
                                     {}}) == RetainDisposition::inserted,
                    "earlier valid retained row staged");
            require(
                count("subathon_contributions") == 0 &&
                    fixture_->scalar("SELECT count(*) FROM main.subathon_retained_configurations "
                                     "WHERE reference=X'65787472612D76616C69642D776F726B'")
                            .toInt() == 0,
                "observer sees none before commit");
            SqliteContributionRepository repository(adopt_active_contribution_session, session);
            ContributionEvidenceService service(repository);
            result = service.observe(s);
            require(count("subathon_contributions") == 1 &&
                        count("subathon_contribution_observations") == 1,
                    "actual commit before observe returns");
            require(fixture_->scalar("SELECT count(*) FROM main.subathon_retained_configurations "
                                     "WHERE reference=X'65787472612D76616C69642D776F726B'")
                            .toInt() == 1,
                    "same actual commit includes earlier valid work");
            return 1;
        });
        OK(r);
        QVERIFY(result && result->contribution_id);
        QCOMPARE(result->contribution_id->value, std::uint64_t(0));
    }
    void ownNewAnchorReadsAndIncompleteCommitAreHonest() {
        const auto s = support();
        auto next = support(2);
        next.verified_activity->identity.opaque_id = "next";
        for (const auto &o : {s, next})
            QVERIFY(commit_receipt(o));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        auto complete = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            const auto id = tx->create(s.receipt, key(s), initial(s));
            require(id.value == 0, "first pending0");
            require(contribution_found(*tx, key(s)).id == id,
                    "own new anchor read before observation");
            require(!tx->find_receipt(s.receipt.id),
                    "no observation fabricated for pending anchor");
            auto changed = initial(s);
            changed.conditions.review = ContributionReview::rejected;
            tx->update_state(id, changed);
            tx->append_observation({s, id});
            tx->commit();
            return 1;
        });
        OK(complete);
        const auto before = fixture_->snapshot();
        bool staged = false, caught = false;
        auto incomplete = run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteContributionRepository repository(adopt_active_contribution_session, session);
            auto tx = repository.begin();
            const auto prior =
                session.execute({"SELECT count(*) FROM main.subathon_contributions", {}})[0][0]
                    .toLongLong();
            (void)tx->create(next.receipt, key(next), initial(next));
            require(session.execute({"SELECT count(*) FROM main.subathon_contributions", {}})[0][0]
                            .toLongLong() == prior + 1,
                    "incomplete anchor staged");
            staged = true;
            try {
                tx->commit();
            } catch (const SqliteContributionStorageFailure &e) {
                require(e.reason() == SqliteContributionStorageReason::incomplete_contribution,
                        "missing own support incomplete");
                require(fixture_->released(), "incomplete rollback before throw");
                caught = true;
                throw;
            }
            return 1;
        });
        QVERIFY2(staged && caught, qPrintable(incomplete.problem()));
        KNOWN(incomplete, SqliteContributionStorageReason::incomplete_contribution,
              WorkError::work_exception);
        QCOMPARE(fixture_->snapshot(), before);
        std::unique_ptr<ContributionTransaction> expired;
        bool pending = false;
        auto abandoned = run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteContributionRepository repository(adopt_active_contribution_session, session);
            expired = repository.begin();
            const auto count_before =
                session.execute({"SELECT count(*) FROM main.subathon_contributions", {}})[0][0]
                    .toLongLong();
            (void)expired->create(next.receipt, key(next), initial(next));
            require(session.execute({"SELECT count(*) FROM main.subathon_contributions", {}})[0][0]
                            .toLongLong() == count_before + 1,
                    "actual pending row before owner-finalizer abandonment");
            pending = true;
            return 1;
        });
        QVERIFY2(pending, qPrintable(abandoned.problem()));
        QCOMPARE(abandoned.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(abandoned.result.error, WorkError::uncommitted);
        QVERIFY(!abandoned.result.value);
        QCOMPARE(fixture_->snapshot(), before);
        try {
            expect_work([&] { expired->commit(); }, WorkError::stale_session);
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
        expired.reset();
        QVERIFY(fixture_->contribution(id_blob(1), next, initial(next)));
        QVERIFY(fixture_->exec(
            "UPDATE main.subathon_contribution_allocator SET last_allocated=X'0000000000000001'"));
        const auto persisted = fixture_->snapshot();
        std::optional<ContributionRecord> record;
        auto bad = read(writer, key(next), record);
        KNOWN(bad, SqliteContributionStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(!record);
        QCOMPARE(fixture_->snapshot(), persisted);
    }

    void inputPreflightPriorityAndExactOptions() {
        const auto s = support();
        QVERIFY(commit_receipt(s));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        const auto maximum =
            std::min({std::uintmax_t(std::numeric_limits<qsizetype>::max()),
                      std::uintmax_t(std::numeric_limits<qint64>::max()),
                      std::uintmax_t(std::vector<ContributionEvidenceRecord>{}.max_size())});
        QVERIFY(maximum > 1024);
        auto r = run(writer, [&](auto work) -> QVariant {
            for (const auto limits :
                 {SqliteContributionLimits{0, 4, 4}, SqliteContributionLimits{4, 0, 4},
                  SqliteContributionLimits{4, 4, 0}, SqliteContributionLimits{-1, 4, 4},
                  SqliteContributionLimits{4, -1, 4}, SqliteContributionLimits{4, 4, -1},
                  SqliteContributionLimits{4, 4, qsizetype(maximum)},
                  SqliteContributionLimits{4, 4, std::numeric_limits<qsizetype>::max()}}) {
                SqliteContributionRepository bad(SqliteWorkContext{}, limits);
                expect_input([&] { (void)bad.begin(); },
                             SqliteContributionInputError::invalid_options);
            }
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            const auto id = tx->create(s.receipt, key(s), initial(s));
            require(id.value == 0, "zero accepted create");
            tx->append_observation({s, id});
            auto bad_receipt = s.receipt;
            bad_receipt.id = {0};
            bad_receipt.identity.origin.provider = "";
            auto bad_key = key(s);
            bad_key.opaque_id = "";
            auto bad_state = initial(s);
            bad_state.payment_evidence = static_cast<PaymentEvidence>(99);
            expect_input([&] { (void)tx->find_receipt({0}); },
                         SqliteContributionInputError::invalid_receipt_id);
            expect_input([&] { (void)tx->create(bad_receipt, bad_key, bad_state); },
                         SqliteContributionInputError::invalid_receipt_id);
            bad_receipt.id = s.receipt.id;
            expect_input([&] { (void)tx->create(bad_receipt, bad_key, bad_state); },
                         SqliteContributionInputError::invalid_receipt);
            expect_input([&] { (void)tx->create(s.receipt, bad_key, bad_state); },
                         SqliteContributionInputError::invalid_activity);
            auto wrong_origin = key(s);
            wrong_origin.origin.source_account = "wrong";
            expect_input([&] { (void)tx->create(s.receipt, wrong_origin, initial(s)); },
                         SqliteContributionInputError::invalid_activity);
            expect_input([&] { (void)tx->create(s.receipt, key(s), bad_state); },
                         SqliteContributionInputError::invalid_state);
            expect_input([&] { (void)tx->find_activity(bad_key); },
                         SqliteContributionInputError::invalid_activity);
            expect_input([&] { (void)tx->unresolved_activity(bad_key); },
                         SqliteContributionInputError::invalid_activity);
            auto bad = s;
            bad.receipt.id = {0};
            bad.kind = static_cast<ContributionObservationKind>(99);
            expect_input([&] { tx->append_observation({bad, ContributionId{0}}); },
                         SqliteContributionInputError::invalid_receipt_id);
            bad = s;
            bad.receipt.identity.origin.provider = "";
            bad.kind = static_cast<ContributionObservationKind>(99);
            expect_input([&] { tx->append_observation({bad, ContributionId{0}}); },
                         SqliteContributionInputError::invalid_receipt);
            bad = s;
            bad.kind = static_cast<ContributionObservationKind>(99);
            bad.verified_activity->verification_ref = "";
            expect_input([&] { tx->append_observation({bad, ContributionId{0}}); },
                         SqliteContributionInputError::invalid_observation);
            bad = s;
            bad.support_conditions.reset();
            expect_input([&] { tx->append_observation({bad, ContributionId{0}}); },
                         SqliteContributionInputError::invalid_observation);
            bad = confirmation();
            bad.support_conditions = captured();
            expect_input([&] { tx->append_observation({bad, ContributionId{0}}); },
                         SqliteContributionInputError::invalid_observation);
            bad = confirmation();
            bad.evidence = PaymentEvidence::unknown;
            expect_input([&] { tx->append_observation({bad, ContributionId{0}}); },
                         SqliteContributionInputError::invalid_observation);
            bad = s;
            bad.evidence = static_cast<PaymentEvidence>(99);
            expect_input([&] { tx->append_observation({bad, ContributionId{0}}); },
                         SqliteContributionInputError::invalid_observation);
            bad = s;
            bad.verified_activity->verification_ref = "";
            bad.support_conditions->review = static_cast<ContributionReview>(99);
            expect_input([&] { tx->append_observation({bad, ContributionId{0}}); },
                         SqliteContributionInputError::invalid_activity);
            bad = s;
            bad.support_conditions->review = static_cast<ContributionReview>(99);
            expect_input([&] { tx->append_observation({bad, ContributionId{0}}); },
                         SqliteContributionInputError::invalid_state);
            for (int field = 0; field < 7; ++field) {
                auto state = initial(s);
                if (field == 0)
                    state.conditions.review = static_cast<ContributionReview>(99);
                if (field == 1)
                    state.conditions.evidence_admission.policy = static_cast<EvidencePolicy>(99);
                if (field == 2)
                    state.conditions.evidence_admission.review_decision->choice =
                        static_cast<EvidenceReviewChoice>(99);
                if (field == 3)
                    state.conditions.application = static_cast<ContributionApplication>(99);
                if (field == 4)
                    state.conditions.reversal = static_cast<ContributionReversal>(99);
                if (field == 5)
                    state.conditions.settlement = static_cast<SettlementGate>(99);
                if (field == 6)
                    state.conditions.evidence_admission.policy_ref = std::string(4097, 'p');
                expect_input([&] { tx->update_state({0}, state); },
                             SqliteContributionInputError::invalid_state);
            }
            expect_input([&] { tx->link_observation({0}, {0}); },
                         SqliteContributionInputError::invalid_receipt_id);
            tx->update_state({0}, initial(s));
            require(contribution_found(*tx, key(s)).id.value == 0,
                    "caught invalid input preserves earlier valid0 anchor");
            tx->commit();
            return 1;
        });
        OK(r);
        auto edge = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work, {4096, 262144, qsizetype(maximum - 1)});
            auto tx = repository.begin();
            tx->commit();
            return 1;
        });
        OK(edge);
    }

    void byteBoundsAndSelectedTargetLimitsAreExact() {
        auto tiny = support();
        tiny.receipt.identity = {{"prov", "acct"}, "name", "id01"};
        tiny.receipt.facts = {"evt", 0, {}, std::string(), {}, ReceiptMoney{"1.0", "EUR"}, {}, {}};
        tiny.receipt.arrival = {0, "conn", "cfg1", "ctx1"};
        tiny.verified_activity =
            VerifiedActivityLink{{tiny.receipt.identity.origin, "act", "key"}, "prf4"};
        tiny.support_conditions = ContributionConditions{
            {true, ReceiptMoney{"1.0", "EUR"}, std::string(), false},
            ContributionReview::pending,
            {EvidencePolicy::require_evidence_review, "pol4",
             EvidenceReviewDecision{EvidenceReviewChoice::await_confirmation, "dec4", "act4"}},
            ContributionApplication::not_applied,
            ContributionReversal::none,
            SettlementGate::clear};
        QVERIFY(commit_receipt(tiny));
        SqliteWriter writer;
        StorageLimits limits;
        limits.bound_value_bytes = 33554432;
        limits.result_value_bytes = 33554432;
        QCOMPARE(open(writer, limits), StorageError::none);
        auto exact = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work, {4, 3, 4});
            auto tx = repository.begin();
            const auto id = tx->create(tiny.receipt, key(tiny), initial(tiny));
            tx->append_observation({tiny, id});
            auto bad_key = key(tiny);
            bad_key.opaque_id = "12345";
            expect_input([&] { (void)tx->find_activity(bad_key); },
                         SqliteContributionInputError::invalid_activity);
            auto bad = tiny;
            bad.receipt.facts.actor_display_name = "four";
            expect_input([&] { (void)tx->create(bad.receipt, key(bad), initial(bad)); },
                         SqliteContributionInputError::invalid_receipt);
            bad = tiny;
            bad.verified_activity->verification_ref = "12345";
            expect_input([&] { tx->append_observation({bad, id}); },
                         SqliteContributionInputError::invalid_activity);
            auto state = initial(tiny);
            state.conditions.evidence_admission.policy_ref = "12345";
            expect_input([&] { tx->update_state(id, state); },
                         SqliteContributionInputError::invalid_state);
            tx->commit();
            return 1;
        });
        OK(exact);
        auto full = support(2);
        full.receipt.identity.opaque_id = std::string(4096, 'r');
        full.receipt.facts.event_type = std::string(262144, 'e');
        full.receipt.facts.money = ReceiptMoney{std::string(262144, '1'), "EUR"};
        full.verified_activity->identity.opaque_id = std::string(4096, 'k');
        full.support_conditions->valuation.original_money = full.receipt.facts.money;
        QVERIFY(commit_receipt(full));
        auto boundary = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            const auto id = tx->create(full.receipt, key(full), initial(full));
            tx->append_observation({full, id});
            auto bad_key = key(full);
            bad_key.opaque_id.push_back('k');
            expect_input([&] { (void)tx->find_activity(bad_key); },
                         SqliteContributionInputError::invalid_activity);
            auto bad_receipt = full.receipt;
            bad_receipt.facts.event_type.push_back('e');
            expect_input([&] { (void)tx->create(bad_receipt, key(full), initial(full)); },
                         SqliteContributionInputError::invalid_receipt);
            auto state = initial(full);
            state.conditions.valuation.original_money->original_decimal.push_back('1');
            expect_input([&] { tx->update_state(id, state); },
                         SqliteContributionInputError::invalid_state);
            tx->commit();
            return 1;
        });
        OK(boundary);
        QCOMPARE(fixture_
                     ->scalar("SELECT count(*) FROM main.subathon_contributions WHERE "
                              "length(activity_id)=4096 AND length(original_decimal)=262144")
                     .toInt(),
                 1);
        for (const bool malformed : {true, false}) {
            const auto path =
                directory_->filePath(malformed ? "target-text.sqlite" : "target-limit.sqlite");
            QVERIFY(install(path));
            Fixture f(path);
            QVERIFY(f.open());
            QVERIFY(f.exec("INSERT INTO main.subathon_retained_configurations VALUES "
                           "(X'63666731',X'01020304')"));
            QVERIFY(f.exec("INSERT INTO main.subathon_retained_contexts VALUES "
                           "(X'63747831',X'74',X'63666731')"));
            auto short_value = tiny;
            short_value.verified_activity->verification_ref = "prf";
            short_value.support_conditions->evidence_admission.policy_ref = "pol";
            short_value.support_conditions->evidence_admission.review_decision =
                EvidenceReviewDecision{EvidenceReviewChoice::await_confirmation, "dec", "act"};
            QVERIFY(f.committed_receipt(short_value.receipt));
            QVERIFY(f.seeded_contribution(id_blob(0), short_value, initial(short_value)));
            if (malformed) {
                QVERIFY(f.exec("PRAGMA ignore_check_constraints=ON"));
                QVERIFY(f.exec("UPDATE main.subathon_retained_configurations SET document='bad'"));
            }
            QCOMPARE(f.rows("SELECT typeof(document),length(document) FROM "
                            "main.subathon_retained_configurations"),
                     SqlRows({{malformed ? "text" : "blob", malformed ? 3 : 4}}));
            const auto before = f.snapshot();
            SqliteWriter target;
            StorageLimits capacity;
            capacity.statements_per_batch = 512;
            QCOMPARE(target.open(path, capacity), StorageError::none);
            bool caught = false;
            std::optional<ContributionRecord> record;
            const auto reason = malformed ? SqliteContributionStorageReason::invalid_stored_data
                                          : SqliteContributionStorageReason::result_limit;
            auto failure = run(target, [&](auto work) -> QVariant {
                SqliteContributionRepository repository(work, {4096, 3, 1024});
                auto tx = repository.begin();
                try {
                    record = tx->find_activity(key(short_value));
                } catch (const SqliteContributionStorageFailure &e) {
                    require(e.reason() == reason, "isolated retained target error mapping");
                    require(f.released(), "target cleanup before port error");
                    caught = true;
                    throw;
                }
                return 1;
            });
            QVERIFY2(caught, qPrintable(failure.problem()));
            KNOWN(failure, reason, WorkError::work_exception);
            QVERIFY(!record);
            QCOMPARE(f.snapshot(), before);
        }
    }

    void unresolvedCollectionLimitNeverTruncatesOrLimitsWrites() {
        const auto s = support();
        auto c2 = confirmation(2), c7 = confirmation(7), c10 = confirmation(10);
        c7.receipt.arrival = c2.receipt.arrival;
        c10.receipt.arrival = c2.receipt.arrival;
        for (const auto &o : {s, c10, c2, c7})
            QVERIFY(commit_receipt(o));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        std::optional<ContributionLinkResult> result;
        for (const auto &o : {c10, c2, c7}) {
            auto r = observe(writer, o, result);
            OK(r);
            QVERIFY(result && !result->contribution_id);
        }
        const auto before = fixture_->snapshot();
        bool caught = false;
        std::vector<ContributionEvidenceRecord> rows;
        auto limited = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work, {4096, 262144, 2});
            auto tx = repository.begin();
            try {
                rows = tx->unresolved_activity(key(s));
            } catch (const SqliteContributionStorageFailure &e) {
                require(e.reason() == SqliteContributionStorageReason::result_limit,
                        "whole unresolved result limit");
                require(fixture_->released(), "unresolved limit rollback before throw");
                caught = true;
                throw;
            }
            return 1;
        });
        QVERIFY2(caught, qPrintable(limited.problem()));
        KNOWN(limited, SqliteContributionStorageReason::result_limit, WorkError::work_exception);
        QVERIFY(rows.empty());
        QCOMPARE(fixture_->snapshot(), before);
        auto ordered = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work, {4096, 262144, 3});
            auto tx = repository.begin();
            require(tx->unresolved_activity(key(s)) ==
                        std::vector<ContributionEvidenceRecord>({{c2, {}}, {c7, {}}, {c10, {}}}),
                    "canonical ReceiptId order with equal UTC");
            tx->commit();
            return 1;
        });
        OK(ordered);
        auto writes = run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteContributionRepository repository(adopt_active_contribution_session, session,
                                                    {4096, 262144, 1});
            auto tx = repository.begin();
            const auto id = tx->create(s.receipt, key(s), initial(s));
            require(id.value == 0, "write target0");
            tx->append_observation({s, id});
            for (const auto &o : {c2, c7, c10}) {
                const auto null_before =
                    session.execute({"SELECT typeof(contribution_id) FROM "
                                     "main.subathon_contribution_observations WHERE receipt_id=?",
                                     {id_blob(o.receipt.id.value)}});
                require(null_before == SqlRows({{"null"}}), "unresolved before link");
                tx->link_observation(o.receipt.id, id);
                const auto row = observation_found(*tx, o.receipt.id);
                require(row.contribution_id.has_value() && row.contribution_id->value == 0,
                        "targeted link not limited by read quota");
            }
            auto state = initial(s);
            state.payment_evidence = PaymentEvidence::confirmed;
            tx->update_state(id, state);
            tx->commit();
            return 1;
        });
        OK(writes);
        QCOMPARE(
            fixture_
                ->scalar(
                    "SELECT count(*) FROM main.subathon_contribution_observations WHERE "
                    "typeof(contribution_id)='blob' AND hex(contribution_id)='0000000000000000'")
                .toInt(),
            4);
    }

    void selectedMalformedRowsAndGroupsRequireRollback() {
        struct Corruption {
            QString table, assignment;
            bool observation_lookup;
        };
        const QList<Corruption> cases = {
            {"subathon_contributions", "contribution_id=X'00000000000000'", false},
            {"subathon_contributions", "contribution_id=X'000000000000000100'", false},
            {"subathon_contributions", "contribution_id=CAST(X'0000000000000002' AS TEXT)", false},
            {"subathon_contributions", "contribution_id=3", false},
            {"subathon_contributions", "original_decimal='text'", false},
            {"subathon_contributions", "currency=NULL", false},
            {"subathon_contributions", "decision_choice=NULL", false},
            {"subathon_contributions", "payment_evidence=99", false},
            {"subathon_contributions", "monetary=2", false},
            {"subathon_contributions", "policy_ref='text'", false},
            {"subathon_contributions", "activity_namespace=NULL", true},
            {"delete_anchor", "", false},
            {"subathon_contribution_observations", "verification_ref='text'", true},
            {"subathon_contribution_observations", "support_currency=NULL", true},
            {"subathon_contribution_observations", "support_review=99", true},
            {"subathon_contribution_observations",
             "contribution_id=CAST(X'000000000000000f' AS TEXT)", true},
            {"subathon_receipts", "actor_id='text'", false},
            {"subathon_contribution_observations", "contribution_id=NULL", false},
            {"subathon_contributions", "contribution_id=18.5", false}};
        QList<ContributionObservation> values;
        for (qsizetype i = 0; i < cases.size(); ++i) {
            auto s = support(std::uint64_t(i + 1));
            s.verified_activity->identity.opaque_id = "bad-" + std::to_string(i);
            QVERIFY(commit_receipt(s));
            QVERIFY(fixture_->seeded_contribution(id_blob(std::uint64_t(i)), s, initial(s)));
            values.append(s);
        }
        auto healthy = support(100);
        healthy.verified_activity->identity.opaque_id = "healthy";
        QVERIFY(commit_receipt(healthy));
        QVERIFY(fixture_->seeded_contribution(id_blob(50), healthy, initial(healthy)));
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        for (qsizetype i = 0; i < cases.size(); ++i) {
            const auto &c = cases[i];
            if (c.table == "delete_anchor") {
                QVERIFY(fixture_->exec(
                    "DELETE FROM main.subathon_contribution_observations WHERE receipt_id=?",
                    {id_blob(values[i].receipt.id.value)}));
                continue;
            }
            const auto where = c.table == "subathon_contributions" ? QString("activity_id=?")
                                                                   : QString("receipt_id=?");
            const auto binding = c.table == "subathon_contributions"
                                     ? blob(key(values[i]).opaque_id)
                                     : id_blob(values[i].receipt.id.value);
            QVERIFY(fixture_->exec(
                "UPDATE main." + c.table + " SET " + c.assignment + " WHERE " + where, {binding}));
        }
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        for (qsizetype i = 0; i < cases.size(); ++i) {
            bool caught = false;
            auto r = run(writer, [&](auto work) -> QVariant {
                SqliteContributionRepository repository(work);
                auto tx = repository.begin();
                try {
                    if (cases[i].observation_lookup)
                        (void)tx->find_receipt(values[i].receipt.id);
                    else
                        (void)tx->find_activity(key(values[i]));
                } catch (const SqliteContributionStorageFailure &e) {
                    require(e.reason() == SqliteContributionStorageReason::invalid_stored_data,
                            "selected strict type/group/anchor failure");
                    require(fixture_->released(), "selected cleanup while wrapper alive");
                    caught = true;
                    throw;
                }
                return 1;
            });
            QVERIFY2(caught, qPrintable(r.problem()));
            KNOWN(r, SqliteContributionStorageReason::invalid_stored_data,
                  WorkError::work_exception);
            QCOMPARE(fixture_->snapshot(), before);
        }
        std::optional<ContributionRecord> record;
        auto good = read(writer, key(healthy), record);
        OK(good);
        QVERIFY(record);
        QCOMPARE(record->original_receipt_id, healthy.receipt.id);
    }

    void allocatorTypeAndOptionalZeroCannotBeConflated() {
        const auto s = support();
        auto next = support(2);
        next.verified_activity->identity.opaque_id = "next";
        QVERIFY(commit_receipt(s));
        QVERIFY(commit_receipt(next));
        QVERIFY(fixture_->seeded_contribution(QByteArray(8, '\0'), s, initial(s)));
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        for (const auto &value :
             {QVariant(), QVariant(QByteArray(7, '\0')), QVariant(QByteArray(9, '\0')), QVariant(0),
              QVariant(0.5), QVariant(QString("00000000")), QVariant(id_blob(1))}) {
            QVERIFY(fixture_->exec(
                "UPDATE main.subathon_contribution_allocator SET last_allocated=?", {value}));
            const auto before = fixture_->snapshot();
            std::optional<ContributionLinkResult> result;
            auto r = observe(writer, next, result);
            KNOWN(r, SqliteContributionStorageReason::invalid_stored_data,
                  WorkError::work_exception);
            QVERIFY(!result);
            QCOMPARE(fixture_->snapshot(), before);
        }
        QVERIFY(fixture_->exec("UPDATE main.subathon_contribution_allocator SET "
                               "last_allocated=CAST(X'0000000000000000' AS TEXT)"));
        QCOMPARE(fixture_->rows("SELECT typeof(last_allocated),hex(CAST(last_allocated AS BLOB)) "
                                "FROM main.subathon_contribution_allocator"),
                 SqlRows({{"text", "0000000000000000"}}));
        const auto text_before = fixture_->snapshot();
        std::optional<ContributionLinkResult> result;
        auto text = observe(writer, next, result);
        KNOWN(text, SqliteContributionStorageReason::invalid_stored_data,
              WorkError::work_exception);
        QVERIFY(!result);
        QCOMPARE(fixture_->snapshot(), text_before);
        QVERIFY(fixture_->exec("DELETE FROM main.subathon_contribution_allocator"));
        auto absent = observe(writer, next, result);
        KNOWN(absent, SqliteContributionStorageReason::invalid_stored_data,
              WorkError::work_exception);
        QVERIFY(fixture_->exec(
            "INSERT INTO main.subathon_contribution_allocator VALUES (1,X'0000000000000000')"));
        auto valid = observe(writer, next, result);
        OK(valid);
        QVERIFY(result && result->contribution_id);
        QCOMPARE(result->contribution_id->value, std::uint64_t(1));
        auto zero = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            const auto row = observation_found(*tx, s.receipt.id);
            require(row.contribution_id.has_value() && row.contribution_id->value == 0,
                    "optional0 survives allocatorlast0");
            require(tx->unresolved_activity(key(s)).empty(),
                    "associated0 excluded from unresolved");
            tx->commit();
            return 1;
        });
        OK(zero);
    }

    void cumulativeResultBudgetRollsBackProvedWrites() {
        const auto s = support();
        QVERIFY(commit_receipt(s));
        SqliteWriter writer;
        StorageLimits limits;
        limits.bound_value_bytes = 8388608;
        QCOMPARE(open(writer, limits), StorageError::none);
        const auto before = fixture_->snapshot();
        bool created = false, appended = false, first_returned = false, second_attempted = false,
             caught = false;
        auto r = run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteContributionRepository repository(adopt_active_contribution_session, session);
            auto tx = repository.begin();
            const auto c0 =
                session.execute({"SELECT count(*) FROM main.subathon_contributions", {}})[0][0]
                    .toLongLong();
            const auto id = tx->create(s.receipt, key(s), initial(s));
            require(session.execute({"SELECT count(*) FROM main.subathon_contributions", {}})[0][0]
                            .toLongLong() == c0 + 1,
                    "actual contribution delta before budget");
            created = true;
            const auto o0 =
                session
                    .execute(
                        {"SELECT count(*) FROM main.subathon_contribution_observations", {}})[0][0]
                    .toLongLong();
            tx->append_observation({s, id});
            require(session.execute({"SELECT count(*) FROM main.subathon_contribution_observations",
                                     {}})[0][0]
                            .toLongLong() == o0 + 1,
                    "actual observation delta before budget");
            appended = true;
            const auto first = session.execute({"SELECT zeroblob(600000)", {}});
            require(first.size() == 1 && first[0].size() == 1 &&
                        first[0][0].toByteArray().size() == 600000,
                    "first individually fitting result returned");
            first_returned = true;
            try {
                second_attempted = true;
                (void)session.execute({"SELECT zeroblob(600000)", {}});
            } catch (const SqliteWorkFailure &e) {
                require(e.error() == WorkError::result_limit,
                        "two individually fitting values exceed cumulative1MiB");
                require(fixture_->released(), "cumulative known rollback while wrapper alive");
                caught = true;
                throw;
            }
            throw std::runtime_error("per-call budget reset accepted both results");
        });
        QVERIFY2(created && appended && first_returned && second_attempted && caught,
                 qPrintable(r.problem()));
        QCOMPARE(r.synchronous, std::optional<WorkError>(WorkError::result_limit));
        QCOMPARE(r.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(r.result.error, WorkError::result_limit);
        QVERIFY(!r.result.value);
        QCOMPARE(fixture_->snapshot(), before);
    }

    void localSecondBeginAndBusyPreserveTransactionTruth() {
        const auto s = support();
        QVERIFY(commit_receipt(s));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        auto r = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            auto tx = repository.begin();
            expect_input([&] { (void)repository.begin(); },
                         SqliteContributionInputError::already_begun);
            const auto id = tx->create(s.receipt, key(s), initial(s));
            tx->append_observation({s, id});
            tx->commit();
            return 1;
        });
        OK(r);
        QVERIFY(fixture_->exec("BEGIN IMMEDIATE"));
        bool caught = false;
        auto busy = run(writer, [&](auto work) -> QVariant {
            SqliteContributionRepository repository(work);
            try {
                (void)repository.begin();
            } catch (const SqliteContributionStorageFailure &e) {
                require(e.reason() == SqliteContributionStorageReason::busy, "actual busy BEGIN");
                caught = true;
                expect_input([&] { (void)repository.begin(); },
                             SqliteContributionInputError::already_begun);
                throw;
            }
            return 1;
        });
        QVERIFY(fixture_->exec("ROLLBACK"));
        QVERIFY2(caught, qPrintable(busy.problem()));
        QCOMPARE(busy.storage, std::optional<SqliteContributionStorageReason>(
                                   SqliteContributionStorageReason::busy));
        QCOMPARE(busy.result.transaction, TransactionOutcome::not_started);
        QCOMPARE(busy.result.error, WorkError::busy);
        QVERIFY(!busy.result.value);
        std::optional<ContributionLinkResult> result;
        auto next = observe(writer, s, result);
        OK(next);
        QVERIFY(result);
        QCOMPARE(result->disposition, ContributionLinkDisposition::duplicate);
    }

    void foreignStaleTerminalAndOwnedResultsRemainSafe() {
        const auto s = support();
        QVERIFY(commit_receipt(s));
        QVERIFY(fixture_->seeded_contribution(id_blob(0), s, initial(s)));
        std::optional<ContributionRecord> owned;
        std::unique_ptr<ContributionTransaction> expired;
        SqliteWorkSession old_session;
        {
            SqliteWriter writer;
            QCOMPARE(open(writer), StorageError::none);
            auto foreign = run(writer, [&](auto work) -> QVariant {
                SqliteContributionRepository repository(work), invalid_options(work, {0, 1, 1});
                std::optional<WorkError> work_error;
                std::optional<SqliteContributionInputError> input_error;
                QString unexpected;
                bool worker_stub = false;
                std::thread worker([&] {
                    try {
                        (void)repository.begin();
                    } catch (const SqliteContributionsNotImplemented &) {
                        worker_stub = true;
                    } catch (const SqliteWorkFailure &e) {
                        work_error = e.error();
                    } catch (const std::exception &e) {
                        unexpected = QString::fromUtf8(e.what());
                    }
                    try {
                        (void)invalid_options.begin();
                    } catch (const SqliteContributionsNotImplemented &) {
                        worker_stub = true;
                    } catch (const SqliteContributionInputFailure &e) {
                        input_error = e.error();
                    } catch (const std::exception &e) {
                        unexpected = QString::fromUtf8(e.what());
                    }
                });
                worker.join();
                if (worker_stub)
                    throw SqliteContributionsNotImplemented();
                require(unexpected.isEmpty(), "unexpected foreign begin category");
                require(work_error == WorkError::wrong_thread,
                        "construction owner checked before mutable guard");
                require(input_error == SqliteContributionInputError::invalid_options,
                        "pure options before owner");
                auto tx = repository.begin();
                require(tx->find_receipt(s.receipt.id).has_value(),
                        "foreign reject leaves owner first begin usable");
                tx->commit();
                return 1;
            });
            OK(foreign);
            auto terminal = run(writer, [&](auto work) -> QVariant {
                auto session = work.begin();
                old_session = session;
                {
                    SqliteContributionRepository repository(adopt_active_contribution_session,
                                                            session);
                    expired = repository.begin();
                }
                owned = expired->find_activity(key(s));
                std::optional<WorkError> work_error;
                std::optional<SqliteContributionInputError> input_error;
                QString unexpected;
                bool worker_stub = false;
                std::thread worker([&] {
                    try {
                        (void)expired->find_receipt(s.receipt.id);
                    } catch (const SqliteContributionsNotImplemented &) {
                        worker_stub = true;
                    } catch (const SqliteWorkFailure &e) {
                        work_error = e.error();
                    } catch (const std::exception &e) {
                        unexpected = QString::fromUtf8(e.what());
                    }
                    try {
                        expired->link_observation({0}, {0});
                    } catch (const SqliteContributionsNotImplemented &) {
                        worker_stub = true;
                    } catch (const SqliteContributionInputFailure &e) {
                        input_error = e.error();
                    } catch (const std::exception &e) {
                        unexpected = QString::fromUtf8(e.what());
                    }
                });
                worker.join();
                if (worker_stub)
                    throw SqliteContributionsNotImplemented();
                require(unexpected.isEmpty(), "unexpected foreign primitive category");
                require(work_error == WorkError::wrong_thread, "actual primitive thread rejection");
                require(input_error == SqliteContributionInputError::invalid_receipt_id,
                        "pure id before owner");
                expired->commit();
                expect_work([&] { (void)expired->find_activity(key(s)); },
                            WorkError::session_closed);
                return 1;
            });
            QVERIFY(terminal.admitted && terminal.done);
            QVERIFY2(!terminal.stub && terminal.unexpected.isEmpty(),
                     qPrintable(terminal.problem()));
            QCOMPARE(terminal.result.transaction, TransactionOutcome::committed);
            QCOMPARE(terminal.result.error, WorkError::session_closed);
            QVERIFY(!terminal.result.value);
            // Misconstructed facade diagnostic: local owner is worker, actual weak
            // Session is main-owner. It cannot claim real phase/owner from a tag.
            auto supplied = run(writer, [&](auto work) -> QVariant {
                auto session = work.begin();
                bool wrapper_returned = false, worker_stub = false;
                std::optional<WorkError> category;
                QString unexpected;
                std::thread worker([&] {
                    try {
                        SqliteContributionRepository repository(adopt_active_contribution_session,
                                                                session);
                        auto tx = repository.begin();
                        wrapper_returned = true;
                        (void)tx->find_receipt(s.receipt.id);
                    } catch (const SqliteContributionsNotImplemented &) {
                        worker_stub = true;
                    } catch (const SqliteWorkFailure &e) {
                        category = e.error();
                    } catch (const std::exception &e) {
                        unexpected = QString::fromUtf8(e.what());
                    }
                });
                worker.join();
                if (worker_stub)
                    throw SqliteContributionsNotImplemented();
                require(unexpected.isEmpty() && wrapper_returned,
                        "adoption does not fake true-ticket validation");
                require(category == WorkError::wrong_thread,
                        "first actual operation checks supplied ticket owner");
                session.commit();
                return 1;
            });
            OK(supplied);
        }
        QVERIFY(owned);
        QCOMPARE(owned->id.value, std::uint64_t(0));
        QCOMPARE(owned->original_facts, s.receipt.facts);
        QCOMPARE(owned->original_arrival, s.receipt.arrival);
        QCOMPARE(owned->state, initial(s));
        try {
            expect_work([&] { expired->commit(); }, WorkError::stale_session);
            auto invalid = key(s);
            invalid.opaque_id.clear();
            expect_input([&] { (void)expired->find_activity(invalid); },
                         SqliteContributionInputError::invalid_activity);
            SqliteContributionRepository stale(adopt_active_contribution_session, old_session);
            auto no_pending = stale.begin();
            expect_work([&] { no_pending->commit(); }, WorkError::stale_session);
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
        expired.reset();
    }

    void lostCommitAckReportsUnknownWithPhysicalContribution() {
        const auto s = support();
        QVERIFY(commit_receipt(s));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_commit_ack),
                 StorageError::none);
        int body_calls = 0;
        std::optional<ContributionLinkResult> returned;
        auto r = run(writer, [&](auto work) -> QVariant {
            ++body_calls;
            SqliteContributionRepository repository(work);
            ContributionEvidenceService service(repository);
            returned = service.observe(s);
            return 1;
        });
        QVERIFY2(!r.stub, qPrintable(r.problem()));
        QCOMPARE(body_calls, 1);
        QVERIFY(!returned);
        QVERIFY(!r.storage);
        QCOMPARE(r.synchronous, std::optional<WorkError>(WorkError::uncertain_outcome));
        QCOMPARE(r.result.transaction, TransactionOutcome::outcome_unknown);
        QCOMPARE(r.result.error, WorkError::uncertain_outcome);
        QVERIFY(!r.result.value);
        QCOMPARE(count("subathon_contributions"), 1);
        QCOMPARE(count("subathon_contribution_observations"), 1);
        QCOMPARE(fixture_->rows("SELECT typeof(contribution_id),hex(contribution_id) FROM "
                                "main.subathon_contributions"),
                 SqlRows({{"blob", "0000000000000000"}}));
        QCOMPARE(fixture_->rows("SELECT typeof(contribution_id),hex(contribution_id) FROM "
                                "main.subathon_contribution_observations"),
                 SqlRows({{"blob", "0000000000000000"}}));
        QCOMPARE(fixture_->rows("SELECT typeof(last_allocated),hex(last_allocated) FROM "
                                "main.subathon_contribution_allocator"),
                 SqlRows({{"blob", "0000000000000000"}}));
        QCOMPARE(writer.enqueue_work([](auto) { return QVariant(1); }, [](auto) {}),
                 StorageError::shutting_down);
    }

    void lostCleanupAckOutranksLocalFailure() {
        const auto s = support();
        auto bad = support(2);
        bad.verified_activity->identity.opaque_id = "corrupt";
        QVERIFY(commit_receipt(s));
        QVERIFY(commit_receipt(bad));
        QVERIFY(fixture_->seeded_contribution(id_blob(0), bad, initial(bad)));
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        QVERIFY(fixture_->exec("UPDATE main.subathon_contributions SET policy_ref='text'"));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_rollback_ack),
                 StorageError::none);
        bool created = false, appended = false;
        int body_calls = 0;
        auto r = run(writer, [&](auto work) -> QVariant {
            ++body_calls;
            auto session = work.begin();
            SqliteContributionRepository repository(adopt_active_contribution_session, session);
            auto tx = repository.begin();
            const auto before_c =
                session.execute({"SELECT count(*) FROM main.subathon_contributions", {}})[0][0]
                    .toLongLong();
            const auto id = tx->create(s.receipt, key(s), initial(s));
            require(session.execute({"SELECT count(*) FROM main.subathon_contributions", {}})[0][0]
                            .toLongLong() == before_c + 1,
                    "actual created row before corrupt read");
            created = true;
            const auto before_o =
                session
                    .execute(
                        {"SELECT count(*) FROM main.subathon_contribution_observations", {}})[0][0]
                    .toLongLong();
            tx->append_observation({s, id});
            require(session.execute({"SELECT count(*) FROM main.subathon_contribution_observations",
                                     {}})[0][0]
                            .toLongLong() == before_o + 1,
                    "actual observation before corrupt read");
            appended = true;
            (void)tx->find_activity(key(bad));
            return 1;
        });
        QVERIFY2(created && appended, qPrintable(r.problem()));
        QCOMPARE(body_calls, 1);
        QVERIFY(!r.storage);
        QCOMPARE(r.synchronous, std::optional<WorkError>(WorkError::uncertain_outcome));
        QCOMPARE(r.result.transaction, TransactionOutcome::outcome_unknown);
        QCOMPARE(r.result.error, WorkError::uncertain_outcome);
        QVERIFY(!r.result.value);
        QCOMPARE(fixture_->snapshot(), before);
        QCOMPARE(writer.enqueue_work([](auto) { return QVariant(1); }, [](auto) {}),
                 StorageError::shutting_down);
    }
};
QTEST_GUILESS_MAIN(SqliteContributionTests)
#include "sqlite_contributions_tests.moc"
