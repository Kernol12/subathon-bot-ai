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
#include <subathon/server/sqlite_application_plans.hpp>
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
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class Operation> void expect_input(Operation operation, SqlitePlanInputError error) {
    try {
        operation();
    } catch (const SqlitePlanInputFailure &failure) {
        require(failure.error() == error, "wrong SqlitePlanInputFailure category");
        return;
    }
    throw std::runtime_error("expected SqlitePlanInputFailure missing");
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

// Independent exact-value construction for fixture/literal oracles, never the new codec.
boost::multiprecision::cpp_int reference_integer(const std::string &text) {
    require(!text.empty(), "empty reference integer");
    bool negative = text.front() == '-';
    boost::multiprecision::cpp_int result = 0;
    for (size_t i = negative ? 1 : 0; i < text.size(); ++i) {
        require(text[i] >= '0' && text[i] <= '9', "reference integer syntax");
        result = result * 10 + (text[i] - '0');
    }
    return negative ? -result : result;
}
Exact exact_value(const std::string &n, const std::string &d) {
    return Exact(reference_integer(n)) / Exact(reference_integer(d));
}
ContributionObservation plan_receipt() {
    auto value = confirmation(2);
    value.receipt.facts.schema_version = std::numeric_limits<std::uint32_t>::max();
    value.receipt.facts.source_timestamp_utc_ms = std::numeric_limits<std::int64_t>::min();
    value.receipt.arrival = {std::numeric_limits<std::int64_t>::max(), "connection-2", "cfg1",
                             "ctx1"};
    return value;
}
CapturedPlanRevision revision_one() {
    const auto source = plan_receipt();
    PlanEvent event{"event-2",
                    std::string(),
                    "root-7",
                    source.receipt.id,
                    source.receipt.identity,
                    source.receipt.facts,
                    source.receipt.arrival,
                    ContributionId{0}};
    HistoricalValuation quote{"resolution-1",
                              *event.facts.money,
                              "CHF",
                              "1.25000",
                              std::string("1.249900"),
                              "snapshot-1",
                              {},
                              {},
                              {},
                              {}};
    PlanCapture capture{event,
                        initial(support()),
                        {PlanValuationState::resolved, quote},
                        {{"time", "action-v1", "rule-a", "main", RequestedUnit::milliseconds, true,
                          Exact(3) / 2, 2, "1.5 half-even to2", "proportional"},
                         {"score", "action-v2", "rule-a", "score", RequestedUnit::micro_points,
                          false, Exact(-5) / 2, -2, "-2.5 half-even to-2", "signed"},
                         {"count", "action-v3", "rule-b", "count", RequestedUnit::whole_count,
                          false, Exact(0), 0, "zero retained", "whole-on-full"}}};
    return {1,
            {},
            capture,
            {{"rule-a", "rule-v1", true, "selected"},
             {"rule-b", "rule-v2", false, "excluded"},
             {"rule-a", "rule-v1", true, "duplicate retained"}},
            {{"double-v1", "instance-r1", "main", "bonus", "multiply", "multiply", "2.0000"},
             {"double-v1", "instance-r1", "main", "bonus", "multiply", "multiply", "2.0000"}},
            {}};
}
CapturedPlanRevision compact() {
    auto r = revision_one();
    r.rules.resize(1);
    r.modifiers.resize(1);
    r.captured.requested_effects.resize(1);
    return r;
}
CapturedPlanRevision next_revision(const CapturedPlanRevision &original, std::uint64_t n) {
    auto r = original;
    r.revision = n;
    r.parent_revision = n - 1;
    r.override_decision = PlanOverrideDecision{OverrideAuthority::authorized, "actor-7",
                                               "explicit override", "decision-7"};
    r.captured.contribution_state.conditions.review = ContributionReview::accepted;
    return r;
}
QList<QVariant> quote_values(const PlanValuation &value) {
    if (!value.historical) {
        QList<QVariant> result;
        for (int i = 0; i < 11; ++i)
            result.append(QVariant{});
        return result;
    }
    const auto &q = *value.historical;
    const auto optional = [](const std::optional<std::string> &v) {
        return v ? QVariant(blob(*v)) : QVariant{};
    };
    return {blob(q.resolution_ref),
            blob(q.original_money.original_decimal),
            blob(q.original_money.currency),
            blob(q.comparison_currency),
            blob(q.effective_rate_decimal),
            optional(q.fetched_rate_decimal),
            blob(q.snapshot_ref),
            optional(q.source_ref),
            q.rate_timestamp_utc_ms ? QVariant(qint64(*q.rate_timestamp_utc_ms)) : QVariant{},
            q.fetch_timestamp_utc_ms ? QVariant(qint64(*q.fetch_timestamp_utc_ms)) : QVariant{},
            optional(q.manual_decision_ref)};
}
const QStringList quote_columns{"historical_resolution_ref",
                                "historical_original_decimal",
                                "historical_currency",
                                "comparison_currency",
                                "effective_rate_decimal",
                                "fetched_rate_decimal",
                                "snapshot_ref",
                                "rate_source_ref",
                                "rate_timestamp_utc_ms",
                                "fetch_timestamp_utc_ms",
                                "manual_decision_ref"};

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

    bool plan_header(const std::string &ref, const PlanEvent &event, std::uint64_t head) {
        return insert(
            "subathon_plan_headers",
            {"plan_ref", "event_ref", "parent_event_ref", "root_event_ref", "receipt_id",
             "contribution_id", "head_revision"},
            {blob(ref), blob(event.event_ref),
             event.parent_event_ref ? QVariant(blob(*event.parent_event_ref)) : QVariant{},
             blob(event.root_event_ref), id_blob(event.receipt_id.value),
             event.contribution_id ? QVariant(id_blob(event.contribution_id->value)) : QVariant{},
             id_blob(head)});
    }
    bool plan_revision(const std::string &ref, const CapturedPlanRevision &r) {
        QStringList columns{"plan_ref", "revision", "parent_revision", "payment_evidence"};
        columns.append(condition_columns);
        columns.append("valuation_state");
        columns.append(quote_columns);
        columns.append({"override_authority", "override_actor_ref", "override_reason",
                        "override_decision_ref"});
        QList<QVariant> values{blob(ref), id_blob(r.revision),
                               r.parent_revision ? QVariant(id_blob(*r.parent_revision))
                                                 : QVariant{},
                               int(r.captured.contribution_state.payment_evidence)};
        values.append(conditions_values(r.captured.contribution_state.conditions));
        values.append(int(r.captured.valuation.state));
        values.append(quote_values(r.captured.valuation));
        if (r.override_decision)
            values.append({int(r.override_decision->authority),
                           blob(r.override_decision->actor_ref), blob(r.override_decision->reason),
                           blob(r.override_decision->decision_ref)});
        else
            for (int i = 0; i < 4; ++i)
                values.append(QVariant{});
        if (!insert("subathon_plan_revisions", columns, values))
            return false;
        qint64 ordinal = 0;
        for (const auto &rule : r.rules)
            if (!insert("subathon_plan_rules",
                        {"plan_ref", "revision", "ordinal", "rule_ref", "version_ref", "selected",
                         "explanation"},
                        {blob(ref), id_blob(r.revision), ++ordinal, blob(rule.rule_ref),
                         blob(rule.version_ref), int(rule.selected), blob(rule.explanation)}))
                return false;
        ordinal = 0;
        for (const auto &m : r.modifiers)
            if (!insert("subathon_plan_modifiers",
                        {"plan_ref", "revision", "ordinal", "definition_version_ref",
                         "instance_revision_ref", "target_ref", "group_ref", "group_operator",
                         "target_operator", "original_factor_decimal"},
                        {blob(ref), id_blob(r.revision), ++ordinal, blob(m.definition_version_ref),
                         blob(m.instance_revision_ref), blob(m.target_ref), blob(m.group_ref),
                         blob(m.group_operator), blob(m.target_operator),
                         blob(m.original_factor_decimal)}))
                return false;
        ordinal = 0;
        for (const auto &v : r.captured.requested_effects) {
            // Reference packing is only raw fixture setup. Codec expected components
            // come from independent JSON or fixed literals, never production output.
            const auto n = boost::multiprecision::numerator(v.exact_requested_units).str();
            const auto d = boost::multiprecision::denominator(v.exact_requested_units).str();
            if (!insert("subathon_plan_requests",
                        {"plan_ref", "revision", "ordinal", "action_ref", "action_version_ref",
                         "rule_ref", "target_ref", "unit", "requires_historical_valuation",
                         "exact_numerator", "exact_denominator", "requested_units",
                         "calculation_explanation", "reversal_behavior_ref"},
                        {blob(ref), id_blob(r.revision), ++ordinal, blob(v.action_ref),
                         blob(v.action_version_ref), blob(v.rule_ref), blob(v.target_ref),
                         int(v.unit), int(v.requires_historical_valuation), blob(n), blob(d),
                         qint64(v.requested_units), blob(v.calculation_explanation),
                         blob(v.reversal_behavior_ref)}))
                return false;
        }
        return true;
    }
    bool committed_plan(const std::string &ref, const std::vector<CapturedPlanRevision> &history,
                        std::uint64_t head) {
        if (history.empty() || !exec("BEGIN IMMEDIATE"))
            return false;
        bool good = plan_header(ref, history.front().captured.event, head);
        for (const auto &r : history)
            if (good)
                good = plan_revision(ref, r);
        if (!good) {
            (void)exec("ROLLBACK");
            return false;
        }
        return exec("COMMIT");
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
    std::optional<SqlitePlanInputError> invalid;
    std::optional<SqlitePlanStorageReason> storage;
    std::optional<WorkError> synchronous;
    QString unexpected;
    SqliteWorkResult result;
    QString problem() const {
        if (stub)
            return "SqliteApplicationPlansNotImplemented: explicit missing application-plan "
                   "behavior";
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
                              } catch (const SqliteApplicationPlansNotImplemented &) {
                                  state->stub = true;
                                  throw;
                              } catch (const SqlitePlanInputFailure &e) {
                                  state->invalid = e.error();
                                  throw;
                              } catch (const SqlitePlanStorageFailure &e) {
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
    QCOMPARE((r).storage, std::optional<SqlitePlanStorageReason>(reason));                         \
    QCOMPARE((r).result.transaction, TransactionOutcome::rolled_back);                             \
    QCOMPARE((r).result.error, outer);                                                             \
    QVERIFY(!(r).result.value)

class SqliteApplicationPlanTests final : public QObject {
    Q_OBJECT
  private:
    QJsonObject json_, m1_, m2_, m3_, m4_;
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<Fixture> fixture_;
    QString path_;
    MigrationManifest literals() const {
        MigrationManifest m;
        m.steps = {literal_step(m1_), literal_step(m2_), literal_step(m3_), literal_step(m4_)};
        m.supported_starting_versions = {0, 1, 2, 3, 4};
        return m;
    }
    bool install(const QString &path) {
        SqliteWriter writer;
        StorageLimits limits;
        limits.statements_per_batch = 2048;
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
    bool seed_sources(Fixture &f) {
        if (!f.exec("INSERT INTO main.subathon_retained_configurations VALUES "
                    "(X'63666731',X'63'),(X'63666732',X'64')") ||
            !f.exec("INSERT INTO main.subathon_retained_contexts VALUES "
                    "(X'63747831',X'74',X'63666732'),(X'63747832',X'75',NULL)"))
            return false;
        const auto s = support();
        const auto c = plan_receipt();
        return f.committed_receipt(s.receipt) && f.committed_receipt(c.receipt) &&
               f.seeded_contribution(id_blob(0), s, initial(s)) &&
               f.evidence({c, ContributionId{0}});
    }
    StorageError open(SqliteWriter &writer, StorageLimits limits = {}) {
        limits.statements_per_batch = 16384;
        return writer.open(path_, limits);
    }
    Run retain(SqliteWriter &writer, const std::string &ref, const CapturedPlanRevision &r,
               SqlitePlanLimits limits = {}) {
        return run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteApplicationPlanStore store(session, limits);
            const auto result = store.retain_initial(ref, r);
            require(result == SqlitePlanInitialDisposition::inserted, "initial inserted");
            session.commit();
            return int(result);
        });
    }
    Run append(SqliteWriter &writer, const std::string &ref, std::uint64_t expected,
               const CapturedPlanRevision &r, SqlitePlanLimits limits = {}) {
        return run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteApplicationPlanStore store(session, limits);
            const auto result = store.append(ref, expected, r);
            require(result == SqlitePlanAppendDisposition::appended, "next appended");
            session.commit();
            return int(result);
        });
    }
    Run read(SqliteWriter &writer, const std::string &ref, std::optional<ApplicationPlan> &out,
             SqlitePlanLimits limits = {}) {
        out.reset();
        return run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteApplicationPlanStore store(session, limits);
            out = store.find(ref);
            session.commit();
            return 1;
        });
    }
    qint64 count(const QString &table) {
        return fixture_->scalar("SELECT count(*) FROM main." + table).toLongLong();
    }
    Run refusal(SqliteWriter &writer, std::function<void(const SqliteApplicationPlanStore &)> body,
                SqlitePlanStorageReason reason, bool &caught, SqlitePlanLimits limits = {},
                Fixture *f = nullptr) {
        Fixture *observer = f ? f : fixture_.get();
        caught = false;
        return run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteApplicationPlanStore store(session, limits);
            (void)session.execute({"INSERT INTO main.subathon_retained_configurations "
                                   "VALUES(X'726F6C6C6261636B2D70726F6265',X'79')",
                                   {}});
            require(session.execute({"SELECT count(*) FROM main.subathon_retained_configurations "
                                     "WHERE reference=X'726F6C6C6261636B2D70726F6265'",
                                     {}})[0][0]
                            .toLongLong() == 1,
                    "actual earlier caller write before selected error");
            try {
                body(store);
            } catch (const SqlitePlanStorageFailure &e) {
                require(e.reason() == reason, "specific selected-data category");
                require(observer->released(), "known rollback inside catch while facade lives");
                caught = true;
                throw;
            }
            throw std::runtime_error("required storage failure absent");
        });
    }
    // Observe actual complete staged rows, including the published head key,
    // through the existing caller session before any injected failure.
    static void observe_staged(const SqliteWorkSession &session, const std::string &ref) {
        for (const auto *table :
             {"subathon_plan_headers", "subathon_plan_revisions", "subathon_plan_rules",
              "subathon_plan_modifiers", "subathon_plan_requests"})
            require(session.execute(
                               {"SELECT count(*) FROM main." + QString(table) + " WHERE plan_ref=?",
                                {blob(ref)}})[0][0]
                            .toLongLong() == 1,
                    "actual +1 staged row in every plan table");
        require(session.execute({"SELECT hex(head_revision) FROM main.subathon_plan_headers WHERE "
                                 "plan_ref=?",
                                 {blob(ref)}})[0][0]
                        .toString() == "0000000000000001",
                "actual staged initial head");
    }
  private slots:
    void initTestCase() {
        QVERIFY(QSqlDatabase::isDriverAvailable("QSQLITE"));
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
        for (const auto &pair : {std::pair{QString(SUBATHON_PLAN_VECTOR_PATH), &json_},
                                 std::pair{QString(SUBATHON_CONTRIBUTION_VECTOR_PATH), &m3_},
                                 std::pair{QString(SUBATHON_RECEIPT_VECTOR_PATH), &m2_},
                                 std::pair{QString(SUBATHON_RETAINED_VECTOR_PATH), &m1_}}) {
            QFile file(pair.first);
            QVERIFY(file.open(QIODevice::ReadOnly));
            QJsonParseError error;
            const auto j = QJsonDocument::fromJson(file.readAll(), &error);
            QCOMPARE(error.error, QJsonParseError::NoError);
            *pair.second = j.object();
        }
        m1_ = m1_["manifest"].toObject();
        m2_ = m2_["manifest"].toObject();
        m3_ = m3_["manifest"].toObject();
        m4_ = json_["manifest"].toObject();
        QCOMPARE(m4_["version"].toInt(), 4);
        QCOMPARE(m4_["payload_bytes"].toInt(), 29015);
        QCOMPARE(m4_["sha256_hex"].toString(),
                 QString("bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50"));
        QCOMPARE(m4_["expected_schema"].toArray().size(), 30);
    }
    void init() {
        directory_ = std::make_unique<QTemporaryDir>();
        QVERIFY(directory_->isValid());
        path_ = directory_->filePath("plans.sqlite");
        QVERIFY(install(path_));
        fixture_ = std::make_unique<Fixture>(path_);
        QVERIFY(fixture_->open());
        QVERIFY(seed_sources(*fixture_));
    }
    void cleanup() {
        fixture_.reset();
        directory_.reset();
    }

    void revisionCodecMatchesIndependentFullRange() {
        try {
            const auto vectors = json_["uint64_vectors"].toArray();
            QCOMPARE(vectors.size(), 6);
            const std::array<std::uint64_t, 6> values{0,
                                                      1,
                                                      9007199254740993ULL,
                                                      9223372036854775807ULL,
                                                      9223372036854775808ULL,
                                                      18446744073709551615ULL};
            for (qsizetype i = 0; i < vectors.size(); ++i) {
                const auto v = vectors[i].toObject();
                QVERIFY(v["decimal"].isString());
                QVERIFY(v["hex"].isString());
                QCOMPARE(v["decimal"].toString(), QString::number(qulonglong(values[size_t(i)])));
                const auto bytes = QByteArray::fromHex(v["hex"].toString().toLatin1());
                QCOMPARE(bytes.size(), 8);
                QCOMPARE(encode_plan_revision_uint64(values[size_t(i)]), bytes);
                QCOMPARE(decode_plan_revision_uint64(bytes), values[size_t(i)]);
            }
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void revisionCodecRejectsMalformedBuffers() {
        try {
            for (const auto &bytes : {QByteArray{}, QByteArray("", 0), QByteArray(7, '\0'),
                                      QByteArray(9, '\0'), QByteArray("18446744073709551615")})
                expect_input([&] { (void)decode_plan_revision_uint64(bytes); },
                             SqlitePlanInputError::invalid_codec);
            QCOMPARE(decode_plan_revision_uint64(QByteArray(8, '\0')), std::uint64_t(0));
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void exactCodecMatchesCanonicalLiteralVectorsAndLargeValue() {
        try {
            bool large = false, reduced = false;
            for (const auto &entry : json_["exact_vectors"].toArray()) {
                const auto v = entry.toObject();
                QVERIFY(v["numerator"].isString());
                QVERIFY(v["denominator"].isString());
                const SqlitePlanExactComponents expected{
                    QByteArray::fromHex(v["numerator_hex"].toString().toLatin1()),
                    QByteArray::fromHex(v["denominator_hex"].toString().toLatin1())};
                QCOMPARE(expected.numerator, v["numerator"].toString().toLatin1());
                QCOMPARE(expected.denominator, v["denominator"].toString().toLatin1());
                const auto value = exact_value(
                    v["input_numerator"].toString(v["numerator"].toString()).toStdString(),
                    v["input_denominator"].toString(v["denominator"].toString()).toStdString());
                QVERIFY(encode_plan_exact(value) == expected);
                QVERIFY(decode_plan_exact(expected) == value);
                const auto edge = std::max(expected.numerator.size(), expected.denominator.size());
                QVERIFY(encode_plan_exact(value, edge) == expected);
                QVERIFY(decode_plan_exact(expected, edge) == value);
                if (v["name"] == "above_4096_bits") {
                    QVERIFY(expected.numerator.size() > 1234);
                    QVERIFY(expected.denominator.size() > 1234);
                    large = true;
                }
                if (v["name"] == "reduced_half") {
                    QCOMPARE(expected.numerator, QByteArray("1"));
                    QCOMPARE(expected.denominator, QByteArray("2"));
                    reduced = true;
                }
            }
            QVERIFY(large);
            QVERIFY(reduced);
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void exactCodecRejectsInvalidAndOversizedComponents() {
        try {
            for (const auto &pair :
                 std::vector<SqlitePlanExactComponents>{{{}, "1"},
                                                        {"1", {}},
                                                        {"", "1"},
                                                        {"+1", "2"},
                                                        {"01", "2"},
                                                        {"-0", "1"},
                                                        {"1", "0"},
                                                        {"1", "-2"},
                                                        {"2", "4"},
                                                        {"0", "2"},
                                                        {"1 ", "2"},
                                                        {"1e2", "1"},
                                                        {QByteArray("1\0", 2), "2"}})
                expect_input([&] { (void)decode_plan_exact(pair); },
                             SqlitePlanInputError::invalid_exact);
            expect_input([&] { (void)decode_plan_exact({{}, {}}, 0); },
                         SqlitePlanInputError::invalid_options);
            expect_input([&] { (void)encode_plan_exact(Exact(1), -1); },
                         SqlitePlanInputError::invalid_options);
            expect_input([&] { (void)decode_plan_exact({"100", "3"}, 2); },
                         SqlitePlanInputError::exact_component_limit);
            expect_input([&] { (void)decode_plan_exact({"bad", "1"}, 2); },
                         SqlitePlanInputError::exact_component_limit);
            expect_input([&] { (void)encode_plan_exact(Exact(-100), 3); },
                         SqlitePlanInputError::exact_component_limit);
            QVERIFY(encode_plan_exact(Exact(-100), 4) == (SqlitePlanExactComponents{"-100", "1"}));
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void manifestPreservesPrefixesAndMatchesLiteralPlanStep() {
        try {
            const auto actual = application_plan_repository_manifest(), expected = literals();
            QCOMPARE(actual.supported_starting_versions, expected.supported_starting_versions);
            QCOMPARE(actual.steps.size(), 4);
            for (qsizetype i = 0; i < 4; ++i) {
                const auto &a = actual.steps[i];
                const auto &e = expected.steps[i];
                QCOMPARE(a.version, e.version);
                QCOMPARE(a.id, e.id);
                QCOMPARE(a.sql, e.sql);
                QCOMPARE(a.expected_schema.size(), e.expected_schema.size());
                for (qsizetype k = 0; k < e.expected_schema.size(); ++k) {
                    const auto &x = a.expected_schema[k], &y = e.expected_schema[k];
                    QCOMPARE(x.type, y.type);
                    QCOMPARE(x.name, y.name);
                    QCOMPARE(x.table_name, y.table_name);
                    QCOMPARE(x.sql, y.sql);
                }
            }
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void initialRoundTripPreservesCompleteOwnedSnapshot() {
        const auto original = revision_one();
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        const auto r = retain(writer, "plan-a", original);
        OK(r);
        std::optional<ApplicationPlan> result;
        const auto f = read(writer, "plan-a", result);
        OK(f);
        QVERIFY(result);
        QCOMPARE(result->plan_ref, std::string("plan-a"));
        QVERIFY(result->revisions == std::vector{original});
        QCOMPARE(fixture_->rows(
                     "SELECT "
                     "typeof(receipt_id),hex(receipt_id),typeof(contribution_id),hex(contribution_"
                     "id),typeof(head_revision),hex(head_revision),typeof(parent_event_ref),length("
                     "parent_event_ref) FROM main.subathon_plan_headers"),
                 (SqlRows{{"blob", "0000000000000002", "blob", "0000000000000000", "blob",
                           "0000000000000001", "blob", qint64(0)}}));
        QCOMPARE(
            fixture_->rows(
                "SELECT "
                "typeof(revision),hex(revision),typeof(parent_revision),typeof(override_authority),"
                "typeof(exact_numerator),hex(exact_numerator),hex(exact_denominator),typeof("
                "requested_units),requested_units FROM main.subathon_plan_revisions JOIN "
                "main.subathon_plan_requests USING(plan_ref,revision) ORDER BY ordinal"),
            (SqlRows{{"blob", "0000000000000001", "null", "null", "blob", "33", "32", "integer",
                      qint64(2)},
                     {"blob", "0000000000000001", "null", "null", "blob", "2D35", "32", "integer",
                      qint64(-2)},
                     {"blob", "0000000000000001", "null", "null", "blob", "30", "31", "integer",
                      qint64(0)}}));
        QCOMPARE(result->revisions[0].captured.event.facts.schema_version,
                 std::numeric_limits<std::uint32_t>::max());
        QCOMPARE(*result->revisions[0].captured.event.facts.source_timestamp_utc_ms,
                 std::numeric_limits<std::int64_t>::min());
        QCOMPARE(result->revisions[0].captured.event.arrival.server_timestamp_utc_ms,
                 std::numeric_limits<std::int64_t>::max());
    }
    void revisionsAppendContiguouslyWithoutRewritingInitial() {
        auto first = revision_one();
        const auto saved = first;
        auto second = next_revision(first, 2), third = next_revision(first, 3);
        second.captured.requested_effects[0].requested_units = 9;
        third.rules[0].explanation = "new choice";
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        auto r = retain(writer, "history", first);
        OK(r);
        const auto original = fixture_->rows("SELECT * FROM main.subathon_plan_revisions");
        r = append(writer, "history", 1, second);
        OK(r);
        r = append(writer, "history", 2, third);
        OK(r);
        first.rules.clear();
        first.captured.requested_effects.clear();
        first.captured.event.event_ref = "mutated input";
        auto later = plan_receipt().receipt;
        later.facts.actor_id = "later observation";
        later.arrival.connection_ref = "later connection";
        const ReceiptInput later_input{later.identity, later.facts, later.arrival};
        QVERIFY(fixture_->observation(id_blob(2), 2,
                                      attempt(later_input, ReceiptDisposition::conflict)));
        QVERIFY(fixture_->exec(
            "UPDATE main.subathon_contributions SET payment_evidence=2,review=2,application=1"));
        std::optional<ApplicationPlan> out;
        r = read(writer, "history", out);
        OK(r);
        QVERIFY(out);
        QVERIFY(out->revisions == std::vector({saved, second, third}));
        QCOMPARE(
            fixture_->rows(
                "SELECT * FROM main.subathon_plan_revisions WHERE revision=X'0000000000000001'"),
            original);
        auto stored_revisions = fixture_->rows(
            "SELECT hex(revision),hex(parent_revision),override_authority,"
            "typeof(override_authority),hex(override_actor_ref),hex(override_reason),"
            "hex(override_decision_ref) FROM main.subathon_plan_revisions ORDER BY revision");
        QCOMPARE(stored_revisions.size(), 3);
        for (const auto &row : stored_revisions)
            QCOMPARE(row.size(), 7);
        QVERIFY(stored_revisions[0][2].isNull());
        QCOMPARE(stored_revisions[1][2], QVariant(qint64(2)));
        QCOMPARE(stored_revisions[2][2], QVariant(qint64(2)));
        for (auto &row : stored_revisions)
            row.removeAt(2);
        QCOMPARE(stored_revisions,
                 (SqlRows{{"0000000000000001", "", "null", "", "", ""},
                          {"0000000000000002", "0000000000000001", "integer", "6163746F722D37",
                           "6578706C69636974206F76657272696465", "6465636973696F6E2D37"},
                          {"0000000000000003", "0000000000000002", "integer", "6163746F722D37",
                           "6578706C69636974206F76657272696465", "6465636973696F6E2D37"}}));
    }
    void quotesAndIndependentStatesRetainAllProvenanceAfterReopen() {
        auto first = revision_one();
        first.captured.valuation = {PlanValuationState::missing_historical, {}};
        for (auto &request : first.captured.requested_effects)
            request.requires_historical_valuation = false;
        auto second = next_revision(first, 2);
        second.captured.valuation = revision_one().captured.valuation;
        auto &q = *second.captured.valuation.historical;
        q.source_ref = std::string();
        q.manual_decision_ref = std::string();
        q.rate_timestamp_utc_ms = std::numeric_limits<std::int64_t>::min();
        q.fetch_timestamp_utc_ms = std::numeric_limits<std::int64_t>::max();
        auto &s = second.captured.contribution_state;
        s.payment_evidence = PaymentEvidence::unconfirmed;
        s.conditions.review = ContributionReview::rejected;
        s.conditions.application = ContributionApplication::applied;
        s.conditions.reversal = ContributionReversal::partial;
        s.conditions.settlement = SettlementGate::unresolved;
        s.conditions.evidence_admission.policy = EvidencePolicy::record_only;
        s.conditions.valuation.resolution_ref = std::string();
        auto third = next_revision(second, 3);
        third.captured.valuation.historical->fetched_rate_decimal.reset();
        third.captured.valuation.historical->source_ref.reset();
        third.captured.valuation.historical->rate_timestamp_utc_ms.reset();
        third.captured.valuation.historical->fetch_timestamp_utc_ms.reset();
        third.captured.valuation.historical->manual_decision_ref.reset();
        third.captured.contribution_state.payment_evidence = PaymentEvidence::not_applicable;
        third.captured.contribution_state.conditions.review = ContributionReview::not_required;
        third.captured.contribution_state.conditions.reversal = ContributionReversal::full;
        third.captured.contribution_state.conditions.evidence_admission = {
            EvidencePolicy::accept_provider_report, "", {}};
        auto fourth = next_revision(third, 4);
        fourth.captured.valuation = {PlanValuationState::not_applicable, {}};
        fourth.captured.contribution_state.payment_evidence = PaymentEvidence::confirmed;
        fourth.captured.contribution_state.conditions.valuation = {false, {}, {}, false};
        fourth.captured.contribution_state.conditions.review = ContributionReview::accepted;
        {
            SqliteWriter writer;
            QCOMPARE(open(writer), StorageError::none);
            auto r = retain(writer, "quote", first);
            OK(r);
            r = append(writer, "quote", 1, second);
            OK(r);
            r = append(writer, "quote", 2, third);
            OK(r);
            r = append(writer, "quote", 3, fourth);
            OK(r);
        }
        QCOMPARE(
            fixture_->rows(
                "SELECT "
                "valuation_state,typeof(historical_resolution_ref),hex(historical_resolution_ref),"
                "hex(historical_original_decimal),hex(historical_currency),hex(comparison_currency)"
                ",hex(effective_rate_decimal),typeof(fetched_rate_decimal),hex(fetched_rate_"
                "decimal),hex(snapshot_ref),typeof(rate_source_ref),length(rate_source_ref),typeof("
                "rate_timestamp_utc_ms),rate_timestamp_utc_ms,typeof(fetch_timestamp_utc_ms),fetch_"
                "timestamp_utc_ms,typeof(manual_decision_ref),length(manual_decision_ref) FROM "
                "main.subathon_plan_revisions WHERE revision=X'0000000000000002'"),
            (SqlRows{{qint64(2), "blob", "7265736F6C7574696F6E2D31", "31322E33343030", "455552",
                      "434846", "312E3235303030", "blob", "312E323439393030",
                      "736E617073686F742D31", "blob", qint64(0), "integer",
                      qint64(std::numeric_limits<std::int64_t>::min()), "integer",
                      qint64(std::numeric_limits<std::int64_t>::max()), "blob", qint64(0)}}));
        QCOMPARE(fixture_->rows(
                     "SELECT "
                     "typeof(fetched_rate_decimal),typeof(rate_source_ref),typeof(rate_timestamp_"
                     "utc_ms),typeof(fetch_timestamp_utc_ms),typeof(manual_decision_ref) FROM "
                     "main.subathon_plan_revisions WHERE revision=X'0000000000000003'"),
                 (SqlRows{{"null", "null", "null", "null", "null"}}));
        QCOMPARE(fixture_
                     ->scalar("SELECT count(*) FROM main.subathon_plan_revisions WHERE "
                              "revision=X'0000000000000001' AND valuation_state=1 AND "
                              "historical_resolution_ref IS NULL AND snapshot_ref IS NULL")
                     .toLongLong(),
                 qint64(1));
        SqliteWriter reopened;
        QCOMPARE(open(reopened), StorageError::none);
        std::optional<ApplicationPlan> result;
        const auto r = read(reopened, "quote", result);
        OK(r);
        QVERIFY(result);
        QVERIFY(result->revisions == std::vector({first, second, third, fourth}));
    }
    void orderedDuplicatesAndOpaqueBytesRemainExact() {
        auto first = revision_one();
        first.rules[0].explanation = std::string("A\0\xff", 3);
        first.rules[2] = first.rules[0];
        first.modifiers[0].group_operator = std::string("m\0\xff", 3);
        first.modifiers[1] = first.modifiers[0];
        first.captured.requested_effects[2] = first.captured.requested_effects[0];
        first.captured.requested_effects[2].calculation_explanation = std::string();
        first.captured.requested_effects[0].requested_units =
            std::numeric_limits<std::int64_t>::min();
        first.captured.requested_effects[1].requested_units =
            std::numeric_limits<std::int64_t>::max();
        const auto huge = json_["exact_vectors"].toArray().last().toObject();
        first.captured.requested_effects[0].exact_requested_units =
            exact_value(huge["numerator"].toString().toStdString(),
                        huge["denominator"].toString().toStdString());
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        auto r = retain(writer, std::string("p\0\xff", 3), first);
        OK(r);
        std::optional<ApplicationPlan> out;
        r = read(writer, std::string("p\0\xff", 3), out);
        OK(r);
        QVERIFY(out);
        QVERIFY(out->revisions == std::vector{first});
        QCOMPARE(fixture_->rows("SELECT ordinal,selected,typeof(explanation),hex(explanation) FROM "
                                "main.subathon_plan_rules ORDER BY ordinal"),
                 (SqlRows{{qint64(1), qint64(1), "blob", "4100FF"},
                          {qint64(2), qint64(0), "blob", "6578636C75646564"},
                          {qint64(3), qint64(1), "blob", "4100FF"}}));
        QCOMPARE(fixture_->rows("SELECT ordinal,hex(group_operator),hex(original_factor_decimal) "
                                "FROM main.subathon_plan_modifiers ORDER BY ordinal"),
                 (SqlRows{{qint64(1), "6D00FF", "322E30303030"},
                          {qint64(2), "6D00FF", "322E30303030"}}));
        QCOMPARE(fixture_->rows(
                     "SELECT "
                     "ordinal,unit,requires_historical_valuation,typeof(requested_units),requested_"
                     "units,typeof(calculation_explanation),hex(exact_numerator),hex(exact_"
                     "denominator) FROM main.subathon_plan_requests ORDER BY ordinal")[0],
                 (SqlRow{qint64(1), qint64(0), qint64(1), "integer",
                         qint64(std::numeric_limits<std::int64_t>::min()), "blob",
                         huge["numerator_hex"].toString().toUpper(),
                         huge["denominator_hex"].toString().toUpper()}));
        QCOMPARE(
            fixture_
                ->scalar("SELECT requested_units FROM main.subathon_plan_requests WHERE ordinal=2")
                .toLongLong(),
            qint64(std::numeric_limits<std::int64_t>::max()));
        QCOMPARE(fixture_
                     ->scalar("SELECT length(calculation_explanation) FROM "
                              "main.subathon_plan_requests WHERE ordinal=3")
                     .toLongLong(),
                 qint64(0));
    }
    void initialDuplicateAndConflictKeepEarlierCallerWrites() {
        const auto first = revision_one();
        QVERIFY(fixture_->committed_plan("duplicate", {first}, 1));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        StorageLimits comparison_budgets;
        comparison_budgets.result_rows = 20000;
        comparison_budgets.result_value_bytes = 8388608;
        comparison_budgets.bound_value_bytes = 8388608;
        QCOMPARE(open(writer, comparison_budgets), StorageError::none);
        const auto r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s);
            (void)s.execute(
                {"INSERT INTO main.subathon_retained_configurations VALUES(X'7072696F72',X'79')",
                 {}});
            require(store.retain_initial("duplicate", first) ==
                        SqlitePlanInitialDisposition::already_present,
                    "exact duplicate");
            auto different = first;
            different.captured.event.receipt_id = {999};
            different.captured.event.contribution_id = ContributionId{999};
            require(store.retain_initial("duplicate", different) ==
                        SqlitePlanInitialDisposition::conflict,
                    "conflict before proposed missing targets");
            require(s.execute({"SELECT count(*) FROM main.subathon_plan_revisions", {}})[0][0]
                            .toLongLong() == 1,
                    "no new revision");
            s.commit();
            return 1;
        });
        OK(r);
        QCOMPARE(count("subathon_plan_headers"), qint64(1));
        QCOMPARE(count("subathon_plan_revisions"), qint64(1));
        QCOMPARE(fixture_
                     ->scalar("SELECT count(*) FROM main.subathon_retained_configurations WHERE "
                              "reference=X'7072696F72'")
                     .toLongLong(),
                 qint64(1));
        QVERIFY(fixture_->exec(
            "DELETE FROM main.subathon_retained_configurations WHERE reference=X'7072696F72'"));
        QCOMPARE(fixture_->snapshot(), before);
    }
    void appendRefusalsDoNotWriteOrResolveProposedTargets() {
        const auto first = compact();
        QVERIFY(fixture_->committed_plan("refusals", {first}, 1));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        StorageLimits comparison_budgets;
        comparison_budgets.result_rows = 20000;
        comparison_budgets.result_value_bytes = 8388608;
        comparison_budgets.bound_value_bytes = 8388608;
        QCOMPARE(open(writer, comparison_budgets), StorageError::none);
        const auto r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s);
            (void)s.execute(
                {"INSERT INTO main.subathon_retained_configurations VALUES(X'7072696F72',X'79')",
                 {}});
            auto candidate = next_revision(first, 2);
            candidate.captured.event.receipt_id = {999};
            candidate.captured.event.contribution_id = ContributionId{999};
            require(store.append("absent", 1, candidate) ==
                        SqlitePlanAppendDisposition::missing_plan,
                    "missing before proposed target");
            require(store.append("refusals", 0, candidate) ==
                        SqlitePlanAppendDisposition::stale_head,
                    "expected0 stale");
            require(store.append("refusals", 9, candidate) ==
                        SqlitePlanAppendDisposition::stale_head,
                    "stale before event/target");
            auto third = next_revision(candidate, 3);
            require(store.append("refusals", 1, third) ==
                        SqlitePlanAppendDisposition::revision_conflict,
                    "local valid but next number conflict before event");
            require(store.append("refusals", 1, candidate) ==
                        SqlitePlanAppendDisposition::event_conflict,
                    "event mismatch before targets");
            // Every stable event member, including optional presence, conflicts
            // before resolving any proposed replacement receipt/cfg/context/CID.
            const std::vector<std::function<void(PlanEvent &)>> changes{
                [](auto &e) { e.event_ref = "another"; },
                [](auto &e) { e.parent_event_ref.reset(); },
                [](auto &e) { e.parent_event_ref = "another"; },
                [](auto &e) { e.root_event_ref = "another"; },
                [](auto &e) { e.receipt_id = {999}; },
                [](auto &e) { e.identity.origin.provider = "another"; },
                [](auto &e) { e.identity.origin.source_account = "another"; },
                [](auto &e) { e.identity.identifier_namespace = "another"; },
                [](auto &e) { e.identity.opaque_id = "another"; },
                [](auto &e) { e.facts.event_type = "another"; },
                [](auto &e) { e.facts.schema_version = 2; },
                [](auto &e) { e.facts.source_timestamp_utc_ms.reset(); },
                [](auto &e) { e.facts.source_timestamp_utc_ms = 0; },
                [](auto &e) { e.facts.actor_id.reset(); },
                [](auto &e) { e.facts.actor_display_name = std::string(); },
                [](auto &e) { e.facts.money.reset(); },
                [](auto &e) { e.facts.money->original_decimal = "99"; },
                [](auto &e) { e.facts.money->currency = "USD"; },
                [](auto &e) { e.facts.payment_reference.reset(); },
                [](auto &e) { e.facts.payment_reference->identifier_namespace = "another"; },
                [](auto &e) { e.facts.payment_reference->opaque_id = "another"; },
                [](auto &e) { e.facts.activity_reference.reset(); },
                [](auto &e) { e.facts.activity_reference->identifier_namespace = "another"; },
                [](auto &e) { e.facts.activity_reference->opaque_id = "another"; },
                [](auto &e) { e.arrival.server_timestamp_utc_ms = 0; },
                [](auto &e) { e.arrival.connection_ref = "another"; },
                [](auto &e) { e.arrival.configuration_ref = "missing"; },
                [](auto &e) { e.arrival.modifier_context_ref = "missing"; },
                [](auto &e) { e.contribution_id.reset(); },
                [](auto &e) { e.contribution_id = ContributionId{999}; }};
            for (const auto &change : changes) {
                auto changed = next_revision(first, 2);
                change(changed.captured.event);
                if (changed.captured.event.facts.money)
                    changed.captured.valuation.historical->original_money =
                        *changed.captured.event.facts.money;
                else {
                    changed.captured.valuation = {PlanValuationState::not_applicable, {}};
                    for (auto &request : changed.captured.requested_effects)
                        request.requires_historical_valuation = false;
                }
                require(store.append("refusals", 1, changed) ==
                            SqlitePlanAppendDisposition::event_conflict,
                        "complete stable event equality");
            }
            auto valid = next_revision(first, 2);
            require(store.append("refusals", 1, valid) == SqlitePlanAppendDisposition::appended,
                    "refusals leave active caller");
            require(store.append("refusals", 1, candidate) ==
                        SqlitePlanAppendDisposition::stale_head,
                    "duplicate append stale");
            s.commit();
            return 1;
        });
        OK(r);
        QCOMPARE(count("subathon_plan_revisions"), qint64(2));
        QCOMPARE(fixture_->scalar("SELECT hex(head_revision) FROM main.subathon_plan_headers")
                     .toString(),
                 QString("0000000000000002"));
        QCOMPARE(fixture_
                     ->scalar("SELECT count(*) FROM main.subathon_retained_configurations WHERE "
                              "reference=X'7072696F72'")
                     .toLongLong(),
                 qint64(1));
        QVERIFY(fixture_->exec(
            "DELETE FROM main.subathon_plan_rules WHERE revision=X'0000000000000002'"));
        QVERIFY(fixture_->exec(
            "DELETE FROM main.subathon_plan_modifiers WHERE revision=X'0000000000000002'"));
        QVERIFY(fixture_->exec(
            "DELETE FROM main.subathon_plan_requests WHERE revision=X'0000000000000002'"));
        QVERIFY(fixture_->exec(
            "DELETE FROM main.subathon_plan_revisions WHERE revision=X'0000000000000002'"));
        QVERIFY(fixture_->exec(
            "UPDATE main.subathon_plan_headers SET head_revision=X'0000000000000001'"));
        QVERIFY(fixture_->exec(
            "DELETE FROM main.subathon_retained_configurations WHERE reference=X'7072696F72'"));
        QCOMPARE(fixture_->snapshot(), before);
    }
    void queuedExpectedHeadHasOneWinner() {
        const auto first = compact(), next = next_revision(first, 2);
        QVERIFY(fixture_->committed_plan("race", {first}, 1));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        struct State {
            int bodies = 0, done = 0;
            std::vector<SqlitePlanAppendDisposition> dispositions;
            std::vector<SqliteWorkResult> results;
            bool stub = false;
        };
        auto state = std::make_shared<State>();
        for (int i = 0; i < 2; ++i)
            QCOMPARE(writer.enqueue_work(
                         [&, state](auto work) -> QVariant {
                             ++state->bodies;
                             auto s = work.begin();
                             SqliteApplicationPlanStore store(s);
                             try {
                                 state->dispositions.push_back(store.append("race", 1, next));
                             } catch (const SqliteApplicationPlansNotImplemented &) {
                                 state->stub = true;
                                 throw;
                             }
                             s.commit();
                             return 1;
                         },
                         [state](auto r) {
                             state->results.push_back(std::move(r));
                             ++state->done;
                         }),
                     StorageError::none);
        QVERIFY(QTest::qWaitFor([state] { return state->done == 2; }, 5000));
        QVERIFY2(!state->stub, "SqliteApplicationPlansNotImplemented: queued append missing");
        QCOMPARE(state->bodies, 2);
        QVERIFY(state->dispositions == std::vector({SqlitePlanAppendDisposition::appended,
                                                    SqlitePlanAppendDisposition::stale_head}));
        for (const auto &r : state->results) {
            QCOMPARE(r.transaction, TransactionOutcome::committed);
            QCOMPARE(r.error, WorkError::none);
            QVERIFY(r.value);
        }
        QCOMPARE(count("subathon_plan_revisions"), qint64(2));
        QCOMPARE(fixture_->scalar("SELECT hex(head_revision) FROM main.subathon_plan_headers")
                     .toString(),
                 QString("0000000000000002"));
    }
    void fullFindCapsNeverBecomeAppendQuotas() {
        const auto first = compact(), second = next_revision(first, 2),
                   third = next_revision(first, 3);
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        auto r = retain(writer, "caps", first);
        OK(r);
        r = append(writer, "caps", 1, second);
        OK(r);
        SqlitePlanLimits limits;
        limits.revisions_per_full_find = 2;
        limits.items_per_full_find = 6;
        limits.items_per_vector = 1;
        std::optional<ApplicationPlan> out;
        r = read(writer, "caps", out, limits);
        OK(r);
        QVERIFY(out);
        QCOMPARE(out->revisions.size(), size_t(2));
        r = append(writer, "caps", 2, third, limits);
        OK(r);
        bool caught = false;
        r = refusal(
            writer, [&](const auto &store) { (void)store.find("caps"); },
            SqlitePlanStorageReason::result_limit, caught, limits);
        KNOWN(r, SqlitePlanStorageReason::result_limit, WorkError::work_exception);
        QVERIFY(caught);
        r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s, limits);
            const auto selected = store.find_revision("caps", 3);
            require(selected && selected->revision == third, "targeted ignores full plan bounds");
            s.commit();
            return 1;
        });
        OK(r);
        limits.revisions_per_full_find = 3;
        limits.items_per_full_find = 9;
        r = read(writer, "caps", out, limits);
        OK(r);
        QVERIFY(out);
        QCOMPARE(out->revisions.size(), size_t(3));
        limits.items_per_full_find = 8;
        r = refusal(
            writer, [&](const auto &store) { (void)store.find("caps"); },
            SqlitePlanStorageReason::result_limit, caught, limits);
        KNOWN(r, SqlitePlanStorageReason::result_limit, WorkError::work_exception);
        QVERIFY(caught);
    }
    void targetedMaxIsExactWhileAggregateSparseHistoryRefuses() {
        const auto one = compact();
        auto parent = next_revision(one, std::numeric_limits<std::uint64_t>::max() - 1);
        parent.parent_revision = 1;
        auto last = next_revision(one, std::numeric_limits<std::uint64_t>::max());
        // Intentionally sparse and globally INVALID. Only selected local metadata
        // can be certified; these rows must never prove valid exhaustion.
        QVERIFY(fixture_->committed_plan("sparse", {one, parent, last},
                                         std::numeric_limits<std::uint64_t>::max()));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        auto r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s);
            const auto selected =
                store.find_revision("sparse", std::numeric_limits<std::uint64_t>::max());
            require(selected && selected->revision == last, "actual selected MAX/local parent");
            require(!store.find_revision("sparse", 2),
                    "targeted absent gap is absent not whole audit");
            s.commit();
            return 1;
        });
        OK(r);
        QCOMPARE(fixture_->rows("SELECT "
                                "typeof(head_revision),hex(head_revision),typeof(contribution_id),"
                                "hex(contribution_id) FROM main.subathon_plan_headers"),
                 (SqlRows{{"blob", "FFFFFFFFFFFFFFFF", "blob", "0000000000000000"}}));
        QCOMPARE(fixture_->rows(
                     "SELECT "
                     "typeof(revision),hex(revision),typeof(parent_revision),hex(parent_revision) "
                     "FROM main.subathon_plan_revisions ORDER BY revision DESC LIMIT 1"),
                 (SqlRows{{"blob", "FFFFFFFFFFFFFFFF", "blob", "FFFFFFFFFFFFFFFE"}}));
        bool caught = false;
        r = refusal(
            writer, [&](const auto &store) { (void)store.find("sparse"); },
            SqlitePlanStorageReason::invalid_stored_data, caught);
        KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(caught);
        r = refusal(
            writer,
            [&](const auto &store) {
                (void)store.append("sparse", std::numeric_limits<std::uint64_t>::max(),
                                   next_revision(one, 2));
            },
            SqlitePlanStorageReason::invalid_stored_data, caught);
        KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(caught);
        QVERIFY(fixture_->exec("UPDATE main.subathon_plan_headers SET contribution_id=NULL"));
        r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s);
            const auto value =
                store.find_revision("sparse", std::numeric_limits<std::uint64_t>::max());
            require(value && !value->revision.captured.event.contribution_id,
                    "SQLNULL distinct from present CID0");
            s.commit();
            return 1;
        });
        OK(r);
    }
    void targetedScopesDoNotCertifyUnselectedPayloads() {
        const auto first = compact(), second = next_revision(first, 2),
                   third = next_revision(first, 3);
        QVERIFY(fixture_->committed_plan("scope", {first, second, third}, 3));
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        QVERIFY(fixture_->exec("UPDATE main.subathon_plan_requests SET exact_numerator=X'3031' "
                               "WHERE revision=X'0000000000000001'"));
        QVERIFY(fixture_->exec("UPDATE main.subathon_plan_rules SET explanation=17 WHERE "
                               "revision=X'0000000000000002'"));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        auto r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s);
            const auto selected = store.find_revision("scope", 3);
            require(selected && selected->revision == third,
                    "unselected parent and anchor payloads not certified");
            require(!store.find_revision("scope", 99), "absent still only declared anchor scope");
            s.commit();
            return 1;
        });
        OK(r);
        bool caught = false;
        r = refusal(
            writer, [&](const auto &store) { (void)store.find("scope"); },
            SqlitePlanStorageReason::invalid_stored_data, caught);
        KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(caught);
        // Initial-anchor dependency metadata IS selected even for missing target.
        QVERIFY(fixture_->exec(
            "UPDATE main.subathon_plan_revisions SET "
            "valuation_state=1,historical_resolution_ref=NULL,historical_original_decimal=NULL,"
            "historical_currency=NULL,comparison_currency=NULL,effective_rate_decimal=NULL,fetched_"
            "rate_decimal=NULL,snapshot_ref=NULL,rate_source_ref=NULL,rate_timestamp_utc_ms=NULL,"
            "fetch_timestamp_utc_ms=NULL,manual_decision_ref=NULL WHERE "
            "revision=X'0000000000000001'"));
        r = refusal(
            writer, [&](const auto &store) { (void)store.find_revision("scope", 99); },
            SqlitePlanStorageReason::invalid_stored_data, caught);
        KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(caught);
        QVERIFY(
            fixture_->exec("UPDATE main.subathon_plan_requests SET requires_historical_valuation=0 "
                           "WHERE revision=X'0000000000000001'"));
        r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s);
            require(!store.find_revision("scope", 99),
                    "independent missing historical valid metadata");
            s.commit();
            return 1;
        });
        OK(r);
        QVERIFY(fixture_->exec(
            "UPDATE main.subathon_plan_revisions SET "
            "valuation_state=2,historical_resolution_ref=X'72',historical_original_decimal=X'3939',"
            "historical_currency=X'455552',comparison_currency=X'434846',effective_rate_decimal=X'"
            "31',snapshot_ref=X'73' WHERE revision=X'0000000000000001'"));
        r = refusal(
            writer, [&](const auto &store) { (void)store.find_revision("scope", 99); },
            SqlitePlanStorageReason::invalid_stored_data, caught);
        KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(caught);
        // Header absence never scans arbitrary orphan rows in another namespace.
        r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s);
            require(!store.find("absent") && !store.find_revision("absent", 1),
                    "absent header local scope");
            s.commit();
            return 1;
        });
        OK(r);
    }
    void selectedOriginalAndTypedTargetsAreRequired() {
        const auto original = compact();
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        bool caught = false;
        auto missing = original;
        missing.captured.event.receipt_id = {999};
        auto r = refusal(
            writer, [&](const auto &store) { (void)store.retain_initial("missing", missing); },
            SqlitePlanStorageReason::missing_receipt, caught);
        KNOWN(r, SqlitePlanStorageReason::missing_receipt, WorkError::work_exception);
        QVERIFY(caught);
        missing = original;
        missing.captured.event.contribution_id = ContributionId{999};
        r = refusal(
            writer, [&](const auto &store) { (void)store.retain_initial("missing", missing); },
            SqlitePlanStorageReason::missing_contribution, caught);
        KNOWN(r, SqlitePlanStorageReason::missing_contribution, WorkError::work_exception);
        QVERIFY(caught);
        auto different = original;
        different.captured.event.facts.actor_id = "changed";
        r = refusal(
            writer, [&](const auto &store) { (void)store.retain_initial("mismatch", different); },
            SqlitePlanStorageReason::receipt_mismatch, caught);
        KNOWN(r, SqlitePlanStorageReason::receipt_mismatch, WorkError::work_exception);
        QVERIFY(caught);
        QVERIFY(fixture_->committed_plan("target", {original}, 1));
        const std::vector<std::pair<QString, QString>> corruptions{
            {"UPDATE main.subathon_receipts SET configuration_ref=X'63747831' WHERE "
             "receipt_id=X'0000000000000002'",
             "UPDATE main.subathon_receipts SET configuration_ref=X'63666731' WHERE "
             "receipt_id=X'0000000000000002'"},
            {"UPDATE main.subathon_receipts SET modifier_context_ref=X'63666731' WHERE "
             "receipt_id=X'0000000000000002'",
             "UPDATE main.subathon_receipts SET modifier_context_ref=X'63747831' WHERE "
             "receipt_id=X'0000000000000002'"},
            {"UPDATE main.subathon_retained_contexts SET configuration_ref=X'6D697373' WHERE "
             "reference=X'63747831'",
             "UPDATE main.subathon_retained_contexts SET configuration_ref=X'63666732' WHERE "
             "reference=X'63747831'"},
            {"UPDATE main.subathon_contributions SET original_receipt_id=X'00000000000003E7'",
             "UPDATE main.subathon_contributions SET original_receipt_id=X'0000000000000001'"}};
        for (const auto &[bad, repair] : corruptions) {
            QVERIFY(fixture_->exec(bad));
            const auto before = fixture_->snapshot();
            r = refusal(
                writer, [&](const auto &store) { (void)store.find_revision("target", 1); },
                SqlitePlanStorageReason::invalid_stored_data, caught);
            KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
            QVERIFY(caught);
            QCOMPARE(fixture_->snapshot(), before);
            QVERIFY(fixture_->exec(repair));
        }
        std::optional<ApplicationPlan> out;
        r = read(writer, "target", out);
        OK(r);
        QVERIFY(out);
        QCOMPARE(out->revisions[0].captured.event.arrival.configuration_ref, std::string("cfg1"));
        QCOMPARE(out->revisions[0].captured.event.arrival.modifier_context_ref,
                 std::string("ctx1"));
        // Context known cfg2 differs from arrival cfg1 and must remain accepted.
        QCOMPARE(fixture_
                     ->scalar("SELECT hex(configuration_ref) FROM main.subathon_retained_contexts "
                              "WHERE reference=X'63747831'")
                     .toString(),
                 QString("63666732"));
        SqlitePlanLimits bounded;
        bounded.value_bytes = 64;
        QVERIFY(fixture_->exec("UPDATE main.subathon_retained_configurations SET "
                               "document=zeroblob(64) WHERE reference=X'63666732'"));
        r = read(writer, "target", out, bounded);
        OK(r);
        QVERIFY(out);
        QVERIFY(fixture_->exec("UPDATE main.subathon_retained_configurations SET "
                               "document=zeroblob(65) WHERE reference=X'63666732'"));
        r = refusal(
            writer,
            [&](const auto &store) { (void)store.retain_initial("proposed-mismatch", different); },
            SqlitePlanStorageReason::result_limit, caught, bounded);
        KNOWN(r, SqlitePlanStorageReason::result_limit, WorkError::work_exception);
        QVERIFY(caught);
        QVERIFY(fixture_->exec("UPDATE main.subathon_retained_configurations SET document=X'64' "
                               "WHERE reference=X'63666732'"));
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        QVERIFY(fixture_->exec("UPDATE main.subathon_receipts SET schema_version=4294967296 WHERE "
                               "receipt_id=X'0000000000000002'"));
        r = refusal(
            writer, [&](const auto &store) { (void)store.find("target"); },
            SqlitePlanStorageReason::invalid_stored_data, caught);
        KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(caught);
    }
    void selectedContributionIdentityDoesNotRequireSupportReceiptOrCurrentState() {
        const auto first = compact();
        QVERIFY(fixture_->committed_plan("identity", {first}, 1));
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        // These unselected STORAGE-014 payloads are deliberately corrupt. This
        // primitive selects physical CID/original-receipt identity, not an anchor
        // or the contribution's current state. Its target is support1, event is2.
        QVERIFY(fixture_->exec(
            "UPDATE main.subathon_contributions SET payment_evidence=91,policy_ref=15"));
        QVERIFY(fixture_->exec("DELETE FROM main.subathon_contribution_observations WHERE "
                               "receipt_id=X'0000000000000001'"));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        std::optional<ApplicationPlan> out;
        auto r = read(writer, "identity", out);
        OK(r);
        QVERIFY(out);
        QVERIFY(out->revisions == std::vector{first});
        QCOMPARE(out->revisions[0].captured.event.receipt_id.value, std::uint64_t(2));
        QCOMPARE(out->revisions[0].captured.event.contribution_id->value, std::uint64_t(0));
        QVERIFY(fixture_->exec(
            "UPDATE main.subathon_receipts SET provider=17 WHERE receipt_id=X'0000000000000001'"));
        bool caught = false;
        r = refusal(
            writer, [&](const auto &store) { (void)store.find_revision("identity", 1); },
            SqlitePlanStorageReason::invalid_stored_data, caught);
        KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(caught);
    }
    void inputPreflightPriorityAndRepresentability() {
        try {
            const auto good = compact();
            SqliteWorkSession absent;
            std::vector<SqlitePlanLimits> bad;
            for (int field = 0; field < 6; ++field) {
                SqlitePlanLimits l;
                switch (field) {
                case 0:
                    l.reference_bytes = 0;
                    break;
                case 1:
                    l.value_bytes = -1;
                    break;
                case 2:
                    l.component_bytes = 0;
                    break;
                case 3:
                    l.revisions_per_full_find = 0;
                    break;
                case 4:
                    l.items_per_vector = -1;
                    break;
                case 5:
                    l.items_per_full_find = 0;
                    break;
                }
                bad.push_back(l);
            }
            for (int field = 0; field < 3; ++field) {
                SqlitePlanLimits l;
                const auto m = std::numeric_limits<qsizetype>::max();
                if (field == 0)
                    l.revisions_per_full_find = m;
                if (field == 1)
                    l.items_per_vector = m;
                if (field == 2)
                    l.items_per_full_find = m;
                bad.push_back(l);
            }
            {
                SqlitePlanLimits l;
                l.revisions_per_full_find =
                    qsizetype(std::vector<CapturedPlanRevision>{}.max_size());
                bad.push_back(l);
            }
            for (const auto max : {std::vector<RuleSelection>{}.max_size(),
                                   std::vector<ModifierSelection>{}.max_size(),
                                   std::vector<RequestedEffect>{}.max_size()}) {
                SqlitePlanLimits l;
                l.items_per_vector = qsizetype(max);
                bad.push_back(l);
            }
            for (const auto &l : bad) {
                SqliteApplicationPlanStore store(absent, l);
                expect_input([&] { (void)store.find(""); }, SqlitePlanInputError::invalid_options);
            }
            SqliteApplicationPlanStore stale(absent);
            expect_input([&] { (void)stale.find(""); }, SqlitePlanInputError::invalid_plan_ref);
            expect_input([&] { (void)stale.find_revision("p", 0); },
                         SqlitePlanInputError::invalid_revision);
            expect_work([&] { (void)stale.find("p"); }, WorkError::stale_session);
            SqliteWriter writer;
            QCOMPARE(open(writer), StorageError::none);
            const auto r = run(writer, [&](auto work) -> QVariant {
                auto s = work.begin();
                (void)s.execute({"INSERT INTO main.subathon_retained_configurations "
                                 "VALUES(X'7072696F72',X'79')",
                                 {}});
                SqliteApplicationPlanStore store(s);
                auto v = good;
                v.revision = 0;
                v.captured.event.receipt_id = {0};
                expect_input([&] { (void)store.retain_initial("p", v); },
                             SqlitePlanInputError::invalid_revision);
                v = good;
                v.parent_revision = 1;
                expect_input([&] { (void)store.retain_initial("p", v); },
                             SqlitePlanInputError::invalid_revision);
                v = next_revision(good, 2);
                v.parent_revision = std::numeric_limits<std::uint64_t>::max();
                expect_input([&] { (void)store.append("p", 1, v); },
                             SqlitePlanInputError::invalid_revision);
                v = good;
                v.override_decision =
                    PlanOverrideDecision{OverrideAuthority::authorized, "a", "r", "d"};
                v.captured.event.receipt_id = {0};
                expect_input([&] { (void)store.retain_initial("p", v); },
                             SqlitePlanInputError::invalid_override);
                v = next_revision(good, 2);
                v.override_decision->authority = OverrideAuthority::denied;
                expect_input([&] { (void)store.append("p", 1, v); },
                             SqlitePlanInputError::invalid_override);
                v = good;
                v.captured.event.receipt_id = {0};
                v.captured.event.event_ref.clear();
                expect_input([&] { (void)store.retain_initial("p", v); },
                             SqlitePlanInputError::invalid_receipt_id);
                v = good;
                v.captured.event.event_ref.clear();
                v.captured.contribution_state.payment_evidence = PaymentEvidence(98);
                expect_input([&] { (void)store.retain_initial("p", v); },
                             SqlitePlanInputError::invalid_event);
                v = good;
                v.captured.contribution_state.payment_evidence = PaymentEvidence(98);
                v.captured.valuation.state = PlanValuationState(98);
                expect_input([&] { (void)store.retain_initial("p", v); },
                             SqlitePlanInputError::invalid_contribution_state);
                v = good;
                v.captured.valuation.state = PlanValuationState::missing_historical;
                v.captured.valuation.historical.reset();
                v.rules[0].rule_ref = std::string(4097, 'x');
                expect_input([&] { (void)store.retain_initial("p", v); },
                             SqlitePlanInputError::invalid_valuation);
                v = good;
                v.captured.valuation.historical->original_money.original_decimal = "99";
                expect_input([&] { (void)store.retain_initial("p", v); },
                             SqlitePlanInputError::invalid_valuation);
                v = good;
                v.rules[0].rule_ref = std::string(4097, 'x');
                v.modifiers[0].target_ref = std::string(4097, 'x');
                expect_input([&] { (void)store.retain_initial("p", v); },
                             SqlitePlanInputError::invalid_rule);
                v = good;
                v.modifiers[0].target_ref = std::string(4097, 'x');
                v.captured.requested_effects[0].unit = RequestedUnit(98);
                expect_input([&] { (void)store.retain_initial("p", v); },
                             SqlitePlanInputError::invalid_modifier);
                v = good;
                v.captured.requested_effects[0].unit = RequestedUnit(98);
                expect_input([&] { (void)store.retain_initial("p", v); },
                             SqlitePlanInputError::invalid_request);
                s.commit();
                return 1;
            });
            OK(r);
            QCOMPARE(fixture_
                         ->scalar("SELECT count(*) FROM main.subathon_retained_configurations "
                                  "WHERE reference=X'7072696F72'")
                         .toLongLong(),
                     qint64(1));
            QCOMPARE(count("subathon_plan_headers"), qint64(0));
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }
    void byteAndCollectionBoundsHaveExactEdges() {
        struct Field {
            const char *name;
            SqlitePlanInputError error;
            bool reference;
            std::function<void(CapturedPlanRevision &, const std::string &)> set;
        };
        using E = SqlitePlanInputError;
        const std::vector<Field> fields{
            {"event", E::invalid_event, true,
             [](auto &r, const auto &s) { r.captured.event.event_ref = s; }},
            {"root", E::invalid_event, true,
             [](auto &r, const auto &s) { r.captured.event.root_event_ref = s; }},
            {"parent", E::invalid_event, true,
             [](auto &r, const auto &s) { r.captured.event.parent_event_ref = s; }},
            {"provider", E::invalid_event, true,
             [](auto &r, const auto &s) { r.captured.event.identity.origin.provider = s; }},
            {"account", E::invalid_event, true,
             [](auto &r, const auto &s) { r.captured.event.identity.origin.source_account = s; }},
            {"namespace", E::invalid_event, true,
             [](auto &r, const auto &s) { r.captured.event.identity.identifier_namespace = s; }},
            {"opaque", E::invalid_event, true,
             [](auto &r, const auto &s) { r.captured.event.identity.opaque_id = s; }},
            {"payment ns", E::invalid_event, true,
             [](auto &r, const auto &s) {
                 r.captured.event.facts.payment_reference->identifier_namespace = s;
             }},
            {"payment id", E::invalid_event, true,
             [](auto &r, const auto &s) {
                 r.captured.event.facts.payment_reference->opaque_id = s;
             }},
            {"activity ns", E::invalid_event, true,
             [](auto &r, const auto &s) {
                 r.captured.event.facts.activity_reference->identifier_namespace = s;
             }},
            {"activity id", E::invalid_event, true,
             [](auto &r, const auto &s) {
                 r.captured.event.facts.activity_reference->opaque_id = s;
             }},
            {"connection", E::invalid_event, true,
             [](auto &r, const auto &s) { r.captured.event.arrival.connection_ref = s; }},
            {"cfg", E::invalid_event, true,
             [](auto &r, const auto &s) { r.captured.event.arrival.configuration_ref = s; }},
            {"ctx", E::invalid_event, true,
             [](auto &r, const auto &s) { r.captured.event.arrival.modifier_context_ref = s; }},
            {"type", E::invalid_event, false,
             [](auto &r, const auto &s) { r.captured.event.facts.event_type = s; }},
            {"actor", E::invalid_event, false,
             [](auto &r, const auto &s) { r.captured.event.facts.actor_id = s; }},
            {"display", E::invalid_event, false,
             [](auto &r, const auto &s) { r.captured.event.facts.actor_display_name = s; }},
            {"money", E::invalid_event, false,
             [](auto &r, const auto &s) { r.captured.event.facts.money->original_decimal = s; }},
            {"currency", E::invalid_event, false,
             [](auto &r, const auto &s) { r.captured.event.facts.money->currency = s; }},
            {"state resolution", E::invalid_contribution_state, true,
             [](auto &r, const auto &s) {
                 r.captured.contribution_state.conditions.valuation.resolution_ref = s;
             }},
            {"policy", E::invalid_contribution_state, true,
             [](auto &r, const auto &s) {
                 r.captured.contribution_state.conditions.evidence_admission.policy_ref = s;
             }},
            {"decision", E::invalid_contribution_state, true,
             [](auto &r, const auto &s) {
                 r.captured.contribution_state.conditions.evidence_admission.review_decision
                     ->decision_ref = s;
             }},
            {"decision actor", E::invalid_contribution_state, true,
             [](auto &r, const auto &s) {
                 r.captured.contribution_state.conditions.evidence_admission.review_decision
                     ->actor_ref = s;
             }},
            {"state money", E::invalid_contribution_state, false,
             [](auto &r, const auto &s) {
                 r.captured.contribution_state.conditions.valuation.original_money
                     ->original_decimal = s;
             }},
            {"state currency", E::invalid_contribution_state, false,
             [](auto &r, const auto &s) {
                 r.captured.contribution_state.conditions.valuation.original_money->currency = s;
             }},
            {"quote resolution", E::invalid_valuation, true,
             [](auto &r, const auto &s) { r.captured.valuation.historical->resolution_ref = s; }},
            {"snapshot", E::invalid_valuation, true,
             [](auto &r, const auto &s) { r.captured.valuation.historical->snapshot_ref = s; }},
            {"source", E::invalid_valuation, true,
             [](auto &r, const auto &s) { r.captured.valuation.historical->source_ref = s; }},
            {"manual", E::invalid_valuation, true,
             [](auto &r, const auto &s) {
                 r.captured.valuation.historical->manual_decision_ref = s;
             }},
            {"quote money", E::invalid_valuation, false,
             [](auto &r, const auto &s) {
                 r.captured.valuation.historical->original_money.original_decimal = s;
             }},
            {"quote currency", E::invalid_valuation, false,
             [](auto &r, const auto &s) {
                 r.captured.valuation.historical->original_money.currency = s;
             }},
            {"comparison", E::invalid_valuation, false,
             [](auto &r, const auto &s) {
                 r.captured.valuation.historical->comparison_currency = s;
             }},
            {"effective", E::invalid_valuation, false,
             [](auto &r, const auto &s) {
                 r.captured.valuation.historical->effective_rate_decimal = s;
             }},
            {"fetched", E::invalid_valuation, false,
             [](auto &r, const auto &s) {
                 r.captured.valuation.historical->fetched_rate_decimal = s;
             }},
            {"rule", E::invalid_rule, true,
             [](auto &r, const auto &s) { r.rules[0].rule_ref = s; }},
            {"rule version", E::invalid_rule, true,
             [](auto &r, const auto &s) { r.rules[0].version_ref = s; }},
            {"rule explanation", E::invalid_rule, false,
             [](auto &r, const auto &s) { r.rules[0].explanation = s; }},
            {"modifier def", E::invalid_modifier, true,
             [](auto &r, const auto &s) { r.modifiers[0].definition_version_ref = s; }},
            {"modifier instance", E::invalid_modifier, true,
             [](auto &r, const auto &s) { r.modifiers[0].instance_revision_ref = s; }},
            {"modifier target", E::invalid_modifier, true,
             [](auto &r, const auto &s) { r.modifiers[0].target_ref = s; }},
            {"modifier group", E::invalid_modifier, true,
             [](auto &r, const auto &s) { r.modifiers[0].group_ref = s; }},
            {"modifier group op", E::invalid_modifier, true,
             [](auto &r, const auto &s) { r.modifiers[0].group_operator = s; }},
            {"modifier target op", E::invalid_modifier, true,
             [](auto &r, const auto &s) { r.modifiers[0].target_operator = s; }},
            {"factor", E::invalid_modifier, false,
             [](auto &r, const auto &s) { r.modifiers[0].original_factor_decimal = s; }},
            {"action", E::invalid_request, true,
             [](auto &r, const auto &s) { r.captured.requested_effects[0].action_ref = s; }},
            {"action version", E::invalid_request, true,
             [](auto &r, const auto &s) {
                 r.captured.requested_effects[0].action_version_ref = s;
             }},
            {"request rule", E::invalid_request, true,
             [](auto &r, const auto &s) { r.captured.requested_effects[0].rule_ref = s; }},
            {"request target", E::invalid_request, true,
             [](auto &r, const auto &s) { r.captured.requested_effects[0].target_ref = s; }},
            {"reversal", E::invalid_request, true,
             [](auto &r, const auto &s) {
                 r.captured.requested_effects[0].reversal_behavior_ref = s;
             }},
            {"calculation", E::invalid_request, false, [](auto &r, const auto &s) {
                 r.captured.requested_effects[0].calculation_explanation = s;
             }}};
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        SqlitePlanLimits limits;
        limits.reference_bytes = 64;
        limits.value_bytes = 64;
        limits.items_per_vector = 1;
        auto r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s, limits);
            expect_input([&] { (void)store.find(std::string(65, 'p')); }, E::invalid_plan_ref);
            for (const auto &field : fields) {
                auto v = compact();
                field.set(v, std::string(65, 'x'));
                expect_input([&] { (void)store.retain_initial("bounds", v); }, field.error);
            }
            for (int vector = 0; vector < 3; ++vector) {
                auto v = compact();
                if (vector == 0)
                    v.rules.push_back(v.rules[0]);
                if (vector == 1)
                    v.modifiers.push_back(v.modifiers[0]);
                if (vector == 2)
                    v.captured.requested_effects.push_back(v.captured.requested_effects[0]);
                v.captured.event.receipt_id = {0};
                expect_input([&] { (void)store.retain_initial("bounds", v); }, E::collection_limit);
            }
            auto v = next_revision(compact(), 2);
            v.override_decision->actor_ref = std::string(65, 'x');
            expect_input([&] { (void)store.append("bounds", 1, v); }, E::invalid_override);
            v = next_revision(compact(), 2);
            v.override_decision->decision_ref = std::string(65, 'x');
            expect_input([&] { (void)store.append("bounds", 1, v); }, E::invalid_override);
            v = next_revision(compact(), 2);
            v.override_decision->reason = std::string(65, 'x');
            expect_input([&] { (void)store.append("bounds", 1, v); }, E::invalid_override);
            s.commit();
            return 1;
        });
        OK(r);
        auto edge = compact();
        edge.rules[0].rule_ref = std::string(64, 'r');
        edge.rules[0].explanation = std::string(64, 'e');
        edge.modifiers[0].original_factor_decimal = std::string(64, 'f');
        edge.captured.requested_effects[0].calculation_explanation = std::string(64, 'c');
        edge.captured.requested_effects[0].exact_requested_units = Exact(100) / 3;
        limits.component_bytes = 3;
        r = retain(writer, std::string(64, 'p'), edge, limits);
        OK(r);
        std::optional<ApplicationPlan> out;
        r = read(writer, std::string(64, 'p'), out, limits);
        OK(r);
        QVERIFY(out);
        QVERIFY(out->revisions == std::vector{edge});
        auto too = compact();
        too.captured.requested_effects[0].exact_requested_units = Exact(1000) / 3;
        r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s, limits);
            expect_input([&] { (void)store.retain_initial("too", too); }, E::exact_component_limit);
            s.commit();
            return 1;
        });
        OK(r);
        limits.items_per_full_find = 2;
        bool caught = false;
        r = refusal(
            writer, [&](const auto &store) { (void)store.find(std::string(64, 'p')); },
            SqlitePlanStorageReason::result_limit, caught, limits);
        KNOWN(r, SqlitePlanStorageReason::result_limit, WorkError::work_exception);
        QVERIFY(caught);
    }
    void selectedMalformedScalarAndChildDataRollback() {
        struct Fault {
            const char *table;
            const char *assignment;
        };
        const std::vector<Fault> faults{
            {"subathon_plan_headers", "head_revision='00000001'"},
            {"subathon_plan_headers", "head_revision=X'0000000000000000'"},
            {"subathon_plan_headers", "head_revision=X'00000000000001'"},
            {"subathon_plan_headers", "head_revision=X'000000000000000001'"},
            {"subathon_plan_headers", "contribution_id=X'00'"},
            {"subathon_plan_headers", "receipt_id=X'00'"},
            {"subathon_plan_headers", "event_ref=9"},
            {"subathon_plan_headers", "parent_event_ref=9"},
            {"subathon_plan_revisions", "payment_evidence=9"},
            {"subathon_plan_revisions", "monetary=2"},
            {"subathon_plan_revisions", "review=9"},
            {"subathon_plan_revisions", "evidence_policy=9"},
            {"subathon_plan_revisions", "application=9"},
            {"subathon_plan_revisions", "reversal=9"},
            {"subathon_plan_revisions", "settlement=9"},
            {"subathon_plan_revisions", "valuation_state=9"},
            {"subathon_plan_revisions", "original_decimal=NULL"},
            {"subathon_plan_revisions", "decision_actor_ref=NULL"},
            {"subathon_plan_revisions", "historical_currency=NULL"},
            {"subathon_plan_revisions", "rate_timestamp_utc_ms='bad'"},
            {"subathon_plan_revisions", "override_actor_ref=X'61'"},
            {"subathon_plan_rules", "selected=2"},
            {"subathon_plan_rules", "explanation=7"},
            {"subathon_plan_modifiers", "original_factor_decimal=1.5"},
            {"subathon_plan_requests", "unit=9"},
            {"subathon_plan_requests", "requires_historical_valuation=2"},
            {"subathon_plan_requests", "requested_units=1.5"},
            {"subathon_plan_requests", "calculation_explanation=17"}};
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        int index = 0;
        for (const auto &fault : faults) {
            const auto ref = "fault-" + std::to_string(index++);
            QVERIFY(fixture_->committed_plan(ref, {compact()}, 1));
            QVERIFY(fixture_->exec("UPDATE main." + QString(fault.table) + " SET " +
                                       fault.assignment + " WHERE plan_ref=?",
                                   {blob(ref)}));
            const auto before = fixture_->snapshot();
            bool caught = false;
            const auto r = refusal(
                writer, [&](const auto &store) { (void)store.find(ref); },
                SqlitePlanStorageReason::invalid_stored_data, caught);
            KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
            QVERIFY(caught);
            QCOMPARE(fixture_->snapshot(), before);
        }
    }
    void selectedExactSqlTypesGrammarAndBoundsStayDistinct() {
        struct Fault {
            const char *assignment;
            SqlitePlanStorageReason reason;
        };
        using R = SqlitePlanStorageReason;
        const std::vector<Fault> faults{
            {"exact_numerator='1'", R::invalid_stored_data},
            {"exact_denominator=2", R::invalid_stored_data},
            {"exact_numerator=1.5", R::invalid_stored_data},
            {"exact_numerator=X''", R::invalid_stored_data},
            {"exact_numerator=X'3031'", R::invalid_stored_data},
            {"exact_numerator=X'2B31'", R::invalid_stored_data},
            {"exact_numerator=X'2D30',exact_denominator=X'31'", R::invalid_stored_data},
            {"exact_numerator=X'32',exact_denominator=X'34'", R::invalid_stored_data},
            {"exact_numerator=X'30',exact_denominator=X'32'", R::invalid_stored_data},
            {"exact_denominator=X'30'", R::invalid_stored_data},
            {"exact_denominator=X'2D32'", R::invalid_stored_data},
            {"exact_numerator=X'3120'", R::invalid_stored_data},
            {"exact_numerator=X'3100'", R::invalid_stored_data},
            {"exact_numerator=X'31303030'", R::result_limit},
            {"exact_denominator=X'62616421'", R::result_limit}};
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        SqlitePlanLimits limits;
        limits.component_bytes = 3;
        int index = 0;
        for (const auto &fault : faults) {
            const auto ref = "exact-" + std::to_string(index++);
            QVERIFY(fixture_->committed_plan(ref, {compact()}, 1));
            QVERIFY(fixture_->exec("UPDATE main.subathon_plan_requests SET " +
                                       QString(fault.assignment) + " WHERE plan_ref=?",
                                   {blob(ref)}));
            const auto before = fixture_->snapshot();
            bool caught = false;
            const auto r = refusal(
                writer, [&](const auto &store) { (void)store.find_revision(ref, 1); }, fault.reason,
                caught, limits);
            KNOWN(r, fault.reason, WorkError::work_exception);
            QVERIFY(caught);
            QCOMPARE(fixture_->snapshot(), before);
        }
    }
    void selectedHistoryAndOrdinalCorruptionRefuses() {
        const auto one = compact(), two = next_revision(one, 2), three = next_revision(one, 3);
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        struct Fault {
            const char *table;
            const char *assignment;
            const char *extra;
        };
        const std::vector<Fault> faults{
            {"subathon_plan_headers", "head_revision=X'0000000000000002'", ""},
            {"subathon_plan_revisions", "parent_revision=X'0000000000000001'",
             " AND revision=X'0000000000000003'"},
            {"subathon_plan_revisions", "parent_revision=NULL",
             " AND revision=X'0000000000000002'"},
            {"subathon_plan_revisions", "revision=X'0000000000000000'",
             " AND revision=X'0000000000000001'"},
            {"subathon_plan_revisions", "revision='00000002'", " AND revision=X'0000000000000002'"},
            {"subathon_plan_revisions", "parent_revision=X'0000000000000000'",
             " AND revision=X'0000000000000002'"},
            {"subathon_plan_revisions", "override_authority=1",
             " AND revision=X'0000000000000002'"},
            {"subathon_plan_revisions", "override_reason=X''", " AND revision=X'0000000000000003'"},
            {"subathon_plan_rules", "ordinal=2", " AND revision=X'0000000000000001'"},
            {"subathon_plan_modifiers", "ordinal=0", " AND revision=X'0000000000000001'"},
            {"subathon_plan_requests", "ordinal='bad'", " AND revision=X'0000000000000001'"}};
        int index = 0;
        for (const auto &fault : faults) {
            const auto ref = "chain-" + std::to_string(index++);
            QVERIFY(fixture_->committed_plan(ref, {one, two, three}, 3));
            QVERIFY(fixture_->exec("UPDATE main." + QString(fault.table) + " SET " +
                                       fault.assignment + " WHERE plan_ref=?" + fault.extra,
                                   {blob(ref)}));
            bool caught = false;
            const auto r = refusal(
                writer, [&](const auto &store) { (void)store.find(ref); },
                SqlitePlanStorageReason::invalid_stored_data, caught);
            KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
            QVERIFY(caught);
        }
        QVERIFY(fixture_->committed_plan("gap", {one, two, three}, 3));
        QVERIFY(fixture_->exec("DELETE FROM main.subathon_plan_revisions WHERE plan_ref=X'676170' "
                               "AND revision=X'0000000000000002'"));
        SqlitePlanLimits cap;
        cap.revisions_per_full_find = 1;
        bool caught = false;
        const auto r = refusal(
            writer, [&](const auto &store) { (void)store.find("gap"); },
            SqlitePlanStorageReason::invalid_stored_data, caught, cap);
        KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(caught);
        QVERIFY(fixture_->committed_plan("anchor", {one, two, three}, 3));
        QVERIFY(fixture_->exec("DELETE FROM main.subathon_plan_revisions WHERE "
                               "plan_ref=X'616E63686F72' AND revision=X'0000000000000001'"));
        const auto a = refusal(
            writer, [&](const auto &store) { (void)store.find_revision("anchor", 99); },
            SqlitePlanStorageReason::invalid_stored_data, caught);
        KNOWN(a, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(caught);
    }
    void laterMetadataPagesCannotHideCorruptCapture() {
        const auto one = compact();
        std::vector<CapturedPlanRevision> history{one};
        for (std::uint64_t n = 2; n <= 65; ++n)
            history.push_back(next_revision(one, n));
        QVERIFY(fixture_->committed_plan("pages", history, 65));
        QVERIFY(fixture_->exec(
            "UPDATE main.subathon_plan_revisions SET "
            "valuation_state=1,historical_resolution_ref=NULL,historical_original_decimal=NULL,"
            "historical_currency=NULL,comparison_currency=NULL,effective_rate_decimal=NULL,fetched_"
            "rate_decimal=NULL,snapshot_ref=NULL,rate_source_ref=NULL,rate_timestamp_utc_ms=NULL,"
            "fetch_timestamp_utc_ms=NULL,manual_decision_ref=NULL WHERE "
            "revision=X'0000000000000041'"));
        // 65th row is structurally corrupt only through the dependent request flag.
        QCOMPARE(fixture_->scalar("SELECT count(*) FROM main.subathon_plan_revisions").toLongLong(),
                 qint64(65));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        StorageLimits budgets;
        budgets.result_rows = 20000;
        budgets.result_value_bytes = 8388608;
        budgets.bound_value_bytes = 8388608;
        QCOMPARE(open(writer, budgets), StorageError::none);
        SqlitePlanLimits tiny;
        tiny.revisions_per_full_find = 1;
        tiny.items_per_full_find = 1;
        bool staged = false, caught = false;
        const auto r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s, tiny);
            require(store.retain_initial("prior-staged", one) ==
                        SqlitePlanInitialDisposition::inserted,
                    "prior full staged plan");
            observe_staged(s, "prior-staged");
            staged = true;
            try {
                (void)store.append("pages", 0, next_revision(one, 66));
            } catch (const SqlitePlanStorageFailure &e) {
                require(e.reason() == SqlitePlanStorageReason::invalid_stored_data,
                        "later-page capture fault before stale/read cap");
                require(fixture_->released(), "inside-catch real rollback despite facade alive");
                caught = true;
                throw;
            }
            throw std::runtime_error("audit skipped later metadata page");
        });
        KNOWN(r, SqlitePlanStorageReason::invalid_stored_data, WorkError::work_exception);
        QVERIFY(staged);
        QVERIFY(caught);
        QCOMPARE(fixture_->snapshot(), before);
    }
    void stagedRowsAndHeadRollbackWithCallerUnit() {
        const auto one = compact(), two = next_revision(one, 2);
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        const auto before = fixture_->snapshot();
        bool initial_seen = false, head_seen = false, caught = false;
        const auto r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            (void)s.execute(
                {"INSERT INTO main.subathon_retained_configurations VALUES(X'7072696F72',X'79')",
                 {}});
            SqliteApplicationPlanStore store(s);
            require(store.retain_initial("rollback", one) == SqlitePlanInitialDisposition::inserted,
                    "staged initial");
            observe_staged(s, "rollback");
            initial_seen = true;
            require(store.append("rollback", 1, two) == SqlitePlanAppendDisposition::appended,
                    "staged append");
            require(s.execute({"SELECT hex(head_revision) FROM main.subathon_plan_headers WHERE "
                               "plan_ref=X'726F6C6C6261636B'",
                               {}})[0][0]
                            .toString() == "0000000000000002",
                    "actual head2 before fault");
            require(s.execute({"SELECT count(*) FROM main.subathon_plan_requests WHERE "
                               "plan_ref=X'726F6C6C6261636B'",
                               {}})[0][0]
                            .toLongLong() == 2,
                    "new child rows before fault");
            head_seen = true;
            try {
                (void)s.execute({"INSERT INTO main.no_such_plan_table VALUES(1)", {}});
            } catch (const SqliteWorkFailure &e) {
                require(e.error() == WorkError::sql_failure, "actual SQL failure preserved");
                require(fixture_->released(), "actual rollback inside body while store lives");
                caught = true;
                throw;
            }
            throw std::runtime_error("required SQL failure absent");
        });
        QVERIFY2(!r.stub, qPrintable(r.problem()));
        QVERIFY(initial_seen);
        QVERIFY(head_seen);
        QVERIFY(caught);
        QCOMPARE(r.synchronous, std::optional<WorkError>(WorkError::sql_failure));
        QCOMPARE(r.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(r.result.error, WorkError::sql_failure);
        QVERIFY(!r.result.value);
        QCOMPARE(fixture_->snapshot(), before);
        // Retry is an explicit caller decision after known rollback, from durable0.
        auto retry = retain(writer, "rollback", one);
        OK(retry);
        const auto durable_one = fixture_->snapshot();
        // Disposable fault DDL, NOT an accepted production schema/manifest. A
        // correct immediate-FK write orders scalar2 before request2; aborting its
        // child insertion therefore exercises a failure DURING append, after
        // preparing a new history row. The approved literal manifest is unchanged.
        QVERIFY(fixture_->exec(
            "CREATE TRIGGER storage015_append_fault BEFORE INSERT ON subathon_plan_requests "
            "WHEN NEW.plan_ref=X'726F6C6C6261636B' AND NEW.revision=X'0000000000000002' "
            "AND EXISTS(SELECT 1 FROM subathon_plan_revisions WHERE plan_ref=NEW.plan_ref AND "
            "revision=NEW.revision) "
            "BEGIN SELECT RAISE(ABORT,'synthetic plan request fault after scalar'); END"));
        QCOMPARE(fixture_
                     ->scalar("SELECT count(*) FROM main.sqlite_schema WHERE type='trigger' AND "
                              "name='storage015_append_fault'")
                     .toLongLong(),
                 qint64(1));
        const auto fault_snapshot = fixture_->snapshot();
        bool prior_staged = false, append_attempted = false, midcall_caught = false;
        const auto midcall = run(writer, [&](auto work) -> QVariant {
            auto session = work.begin();
            SqliteApplicationPlanStore store(session);
            (void)session.execute({"INSERT INTO main.subathon_retained_configurations "
                                   "VALUES(X'6D69642D7072696F72',X'79')",
                                   {}});
            require(store.retain_initial("midcall-prior", one) ==
                        SqlitePlanInitialDisposition::inserted,
                    "complete prior caller plan");
            observe_staged(session, "midcall-prior");
            prior_staged = true;
            try {
                append_attempted = true;
                (void)store.append("rollback", 1, two);
            } catch (const SqlitePlanStorageFailure &e) {
                require(e.reason() == SqlitePlanStorageReason::sql_failure,
                        "specific SQL port failure DURING append");
                require(fixture_->released(),
                        "actual midcall rollback inside catch while facade lives");
                midcall_caught = true;
                throw;
            }
            throw std::runtime_error("injected append child SQL failure absent");
        });
        KNOWN(midcall, SqlitePlanStorageReason::sql_failure, WorkError::sql_failure);
        QVERIFY(prior_staged);
        QVERIFY(append_attempted);
        QVERIFY(midcall_caught);
        QCOMPARE(fixture_->snapshot(), fault_snapshot);
        QCOMPARE(fixture_->scalar("SELECT hex(head_revision) FROM main.subathon_plan_headers")
                     .toString(),
                 QString("0000000000000001"));
        QCOMPARE(count("subathon_plan_revisions"), qint64(1));
        QVERIFY(fixture_->exec("DROP TRIGGER storage015_append_fault"));
        QCOMPARE(fixture_->snapshot(), durable_one);
        // Explicit retry recomputes from the actual unchanged durable head1.
        retry = append(writer, "rollback", 1, two);
        OK(retry);
        QCOMPARE(count("subathon_plan_revisions"), qint64(2));
    }
    void cumulativeResultsRollbackObservedWrites() {
        SqliteWriter writer;
        StorageLimits budgets;
        budgets.bound_value_bytes = 8388608;
        QCOMPARE(open(writer, budgets), StorageError::none);
        const auto before = fixture_->snapshot();
        bool staged = false, first_returned = false, second_attempted = false, caught = false;
        const auto r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            SqliteApplicationPlanStore store(s);
            require(store.retain_initial("budget", compact()) ==
                        SqlitePlanInitialDisposition::inserted,
                    "complete plan staged");
            observe_staged(s, "budget");
            staged = true;
            const auto first = s.execute({"SELECT zeroblob(600000)", {}});
            require(first.size() == 1 && first[0].size() == 1 &&
                        first[0][0].toByteArray().size() == 600000,
                    "first individually fitting result returned");
            first_returned = true;
            try {
                second_attempted = true;
                (void)s.execute({"SELECT zeroblob(600000)", {}});
            } catch (const SqliteWorkFailure &e) {
                require(e.error() == WorkError::result_limit,
                        "cumulative value budget not per-query reset");
                require(fixture_->released(), "known rollback inside body with facade alive");
                caught = true;
                throw;
            }
            throw std::runtime_error("cumulative budget not enforced");
        });
        QVERIFY2(!r.stub, qPrintable(r.problem()));
        QVERIFY(staged);
        QVERIFY(first_returned);
        QVERIFY(second_attempted);
        QVERIFY(caught);
        QCOMPARE(r.synchronous, std::optional<WorkError>(WorkError::result_limit));
        QCOMPARE(r.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(r.result.error, WorkError::result_limit);
        QVERIFY(!r.result.value);
        QCOMPARE(fixture_->snapshot(), before);
    }
    void inertStoreAndStagedResultsNeedCallerCommit() {
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        bool staged = false, after_destruction = false;
        auto r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            {
                const SqliteApplicationPlanStore store(s);
                const auto copy = store;
                require(copy.retain_initial("staged", compact()) ==
                            SqlitePlanInitialDisposition::inserted,
                        "return is staged only");
                observe_staged(s, "staged");
                staged = true;
                require(count("subathon_plan_headers") == 0,
                        "external reader cannot see uncommitted header");
            }
            require(s.execute({"SELECT count(*) FROM main.subathon_plan_headers", {}})[0][0]
                            .toLongLong() == 1,
                    "inert destructor leaves caller transaction active");
            require(!fixture_->released(), "destructor did not perform rollback");
            after_destruction = true;
            s.commit();
            return 1;
        });
        OK(r);
        QVERIFY(staged);
        QVERIFY(after_destruction);
        QCOMPARE(count("subathon_plan_headers"), qint64(1));
        const auto durable = fixture_->snapshot();
        staged = false;
        r = run(writer, [&](auto work) -> QVariant {
            auto s = work.begin();
            {
                SqliteApplicationPlanStore store(s);
                require(store.retain_initial("abandoned", compact()) ==
                            SqlitePlanInitialDisposition::inserted,
                        "staged abandoned row");
                observe_staged(s, "abandoned");
                staged = true;
            }
            require(s.execute({"SELECT count(*) FROM main.subathon_plan_headers", {}})[0][0]
                            .toLongLong() == 2,
                    "destructor no hidden commit/rollback");
            return 1;
        });
        QVERIFY2(!r.stub, qPrintable(r.problem()));
        QVERIFY(staged);
        QCOMPARE(r.result.transaction, TransactionOutcome::rolled_back);
        QCOMPARE(r.result.error, WorkError::uncommitted);
        QVERIFY(!r.result.value);
        QCOMPARE(fixture_->snapshot(), durable);
        QVERIFY(before != durable);
        // A facade with no live session can be copied/destroyed without BEGIN.
        {
            SqliteApplicationPlanStore inert(SqliteWorkSession{}, {0, 0, 0, 0, 0, 0});
            const auto copy = inert;
            (void)copy;
        }
        QVERIFY(fixture_->released());
    }
    void foreignStaleTerminalAndOwnedLifetimes() {
        const auto first = compact();
        QVERIFY(fixture_->committed_plan("life", {first}, 1));
        std::optional<ApplicationPlan> owned;
        std::unique_ptr<SqliteApplicationPlanStore> expired;
        {
            SqliteWriter writer;
            QCOMPARE(open(writer), StorageError::none);
            auto r = run(writer, [&](auto work) -> QVariant {
                auto s = work.begin();
                SqliteApplicationPlanStore store(s);
                SqlitePlanLimits bad;
                bad.reference_bytes = 0;
                SqliteApplicationPlanStore bad_options(s, bad);
                std::optional<WorkError> thread_error;
                std::optional<SqlitePlanInputError> pure_error, options_error;
                QString unexpected;
                bool stub = false;
                std::thread thread([&] {
                    try {
                        (void)store.find("life");
                    } catch (const SqliteApplicationPlansNotImplemented &) {
                        stub = true;
                    } catch (const SqliteWorkFailure &e) {
                        thread_error = e.error();
                    } catch (const std::exception &e) {
                        unexpected = e.what();
                    }
                    try {
                        (void)store.find_revision("life", 0);
                    } catch (const SqliteApplicationPlansNotImplemented &) {
                        stub = true;
                    } catch (const SqlitePlanInputFailure &e) {
                        pure_error = e.error();
                    } catch (const std::exception &e) {
                        unexpected = e.what();
                    }
                    try {
                        (void)bad_options.find("");
                    } catch (const SqliteApplicationPlansNotImplemented &) {
                        stub = true;
                    } catch (const SqlitePlanInputFailure &e) {
                        options_error = e.error();
                    } catch (const std::exception &e) {
                        unexpected = e.what();
                    }
                });
                thread.join();
                if (stub)
                    throw SqliteApplicationPlansNotImplemented();
                require(unexpected.isEmpty(), "foreign unexpected category");
                require(thread_error == WorkError::wrong_thread,
                        "actual weak capability thread guard");
                require(pure_error == SqlitePlanInputError::invalid_revision,
                        "pure revision before thread");
                require(options_error == SqlitePlanInputError::invalid_options,
                        "options before other checks");
                owned = store.find("life");
                require(owned && owned->revisions == std::vector{first}, "owner remains usable");
                expired = std::make_unique<SqliteApplicationPlanStore>(store);
                s.commit();
                expect_work([&] { (void)store.find("life"); }, WorkError::session_closed);
                return 1;
            });
            QVERIFY2(!r.stub && r.unexpected.isEmpty(), qPrintable(r.problem()));
            QCOMPARE(r.result.transaction, TransactionOutcome::committed);
            QCOMPARE(r.result.error, WorkError::session_closed);
            QVERIFY(!r.result.value);
            QVERIFY(owned);
            r = run(writer, [&](auto work) -> QVariant {
                auto s = work.begin();
                expect_work([&] { (void)expired->find("life"); }, WorkError::stale_session);
                SqliteApplicationPlanStore current(s);
                require(current.find("life").has_value(),
                        "fresh session unaffected by stale ticket");
                s.commit();
                return 1;
            });
            OK(r);
        }
        try {
            expect_work([&] { (void)expired->find("life"); }, WorkError::stale_session);
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
        QVERIFY(fixture_->exec("DELETE FROM main.subathon_plan_requests"));
        QVERIFY(fixture_->exec("DELETE FROM main.subathon_plan_rules"));
        QVERIFY(fixture_->exec("DELETE FROM main.subathon_plan_modifiers"));
        QVERIFY(fixture_->exec("DELETE FROM main.subathon_plan_revisions"));
        QVERIFY(fixture_->exec("DELETE FROM main.subathon_plan_headers"));
        QVERIFY(owned);
        QCOMPARE(owned->plan_ref, std::string("life"));
        QVERIFY(owned->revisions == std::vector{first});
    }
    void callerCommitAckLossRemainsUnknownWithPhysicalPlan() {
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_commit_ack),
                 StorageError::none);
        int bodies = 0;
        bool staged = false, after_commit = false;
        const auto r = run(writer, [&](auto work) -> QVariant {
            ++bodies;
            auto s = work.begin();
            SqliteApplicationPlanStore store(s);
            require(store.retain_initial("unknown", compact()) ==
                        SqlitePlanInitialDisposition::inserted,
                    "staged disposition");
            observe_staged(s, "unknown");
            staged = true;
            s.commit();
            after_commit = true;
            return 1;
        });
        QVERIFY2(!r.stub, qPrintable(r.problem()));
        QCOMPARE(bodies, 1);
        QVERIFY(staged);
        QVERIFY(!after_commit);
        QVERIFY(!r.storage);
        QCOMPARE(r.synchronous, std::optional<WorkError>(WorkError::uncertain_outcome));
        QCOMPARE(r.result.transaction, TransactionOutcome::outcome_unknown);
        QCOMPARE(r.result.error, WorkError::uncertain_outcome);
        QVERIFY(!r.result.value);
        QCOMPARE(count("subathon_plan_headers"), qint64(1));
        QCOMPARE(count("subathon_plan_revisions"), qint64(1));
        QCOMPARE(count("subathon_plan_rules"), qint64(1));
        QCOMPARE(count("subathon_plan_modifiers"), qint64(1));
        QCOMPARE(count("subathon_plan_requests"), qint64(1));
        QCOMPARE(fixture_->scalar("SELECT hex(head_revision) FROM main.subathon_plan_headers")
                     .toString(),
                 QString("0000000000000001"));
        QCOMPARE(writer.enqueue_work([](auto) { return QVariant(1); }, [](auto) {}),
                 StorageError::shutting_down);
        QCOMPARE(bodies, 1);
    }
    void cleanupAckLossOutranksSelectedFailure() {
        QVERIFY(fixture_->committed_plan("bad", {compact()}, 1));
        QVERIFY(fixture_->exec("PRAGMA ignore_check_constraints=ON"));
        QVERIFY(fixture_->exec("UPDATE main.subathon_plan_rules SET explanation='wrong-class'"));
        const auto before = fixture_->snapshot();
        SqliteWriter writer;
        QCOMPARE(open(writer), StorageError::none);
        QCOMPARE(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                     writer, WorkControlFault::lose_next_rollback_ack),
                 StorageError::none);
        int bodies = 0;
        bool staged = false;
        const auto r = run(writer, [&](auto work) -> QVariant {
            ++bodies;
            auto s = work.begin();
            SqliteApplicationPlanStore store(s);
            require(store.retain_initial("before-unknown", compact()) ==
                        SqlitePlanInitialDisposition::inserted,
                    "new plan before corrupt selection");
            observe_staged(s, "before-unknown");
            staged = true;
            (void)store.find("bad");
            return 1;
        });
        QVERIFY2(!r.stub, qPrintable(r.problem()));
        QVERIFY(staged);
        QCOMPARE(bodies, 1);
        QVERIFY(!r.storage);
        QCOMPARE(r.synchronous, std::optional<WorkError>(WorkError::uncertain_outcome));
        QCOMPARE(r.result.transaction, TransactionOutcome::outcome_unknown);
        QCOMPARE(r.result.error, WorkError::uncertain_outcome);
        QVERIFY(!r.result.value);
        QCOMPARE(fixture_->snapshot(), before);
        QCOMPARE(writer.enqueue_work([](auto) { return QVariant(1); }, [](auto) {}),
                 StorageError::shutting_down);
        QCOMPARE(bodies, 1);
    }
};
QTEST_GUILESS_MAIN(SqliteApplicationPlanTests)
#include "sqlite_application_plans_tests.moc"
