// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sqlite_test_support.hpp"
// Frozen raw foundation plus OLD bundle lookup only. The new claim reader is never invoked here.
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTest>
#include <QUuid>
#include <limits>
#include <memory>
#include <subathon/server/sqlite_application_bundle.hpp>

namespace original_claim_fixture {
namespace core = subathon::core;
using namespace subathon::server;
using qt_test_support::require;
inline QByteArray blob(const std::string &value) {
    return value.empty() ? QByteArray("", 0) : QByteArray(value.data(), qsizetype(value.size()));
}
inline QJsonObject load(const char *path) {
    QFile file(QString::fromUtf8(path));
    require(file.open(QIODevice::ReadOnly), "Frozen fixture exists");
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);
    require(error.error == QJsonParseError::NoError && document.isObject(), "Frozen fixture JSON");
    return document.object();
}
inline QVariant value(const QJsonValue &entry) {
    const auto v = entry.toObject();
    const auto type = v["type"].toString();
    if (type == "null")
        return {};
    if (type == "blob") {
        const auto hex = v["hex"].toString().toLatin1();
        auto bytes = QByteArray::fromHex(hex);
        require(bytes.toHex() == hex.toLower(), "Exact fixture hex");
        return bytes.isEmpty() ? QByteArray("", 0) : bytes;
    }
    if (type == "text")
        return v["value"].toString();
    bool ok = false;
    const auto result = v["decimal"].toString().toLongLong(&ok);
    require(type == "integer" && ok, "Literal signed64 decimal");
    return result;
}
inline SqlStatement statement(const QJsonValue &entry) {
    const auto v = entry.toObject();
    QList<QVariant> values;
    for (const auto &binding : v["bindings"].toArray())
        values.append(value(binding));
    return {v["sql"].toString(), values};
}
inline void execute(const SqliteWorkSession &session, const QJsonArray &steps) {
    for (const auto &entry : steps)
        (void)session.execute(statement(entry));
}
inline void check_rows(const SqlRows &actual, const QJsonObject &query) {
    const auto expected = query["expected"].toArray();
    const auto label = query["name"].toString();
    require(actual.size() == expected.size(), label + ": row count");
    for (qsizetype row = 0; row < actual.size(); ++row) {
        const auto cells = expected[row].toArray();
        require(actual[row].size() == cells.size(), label + ": column count");
        for (qsizetype column = 0; column < cells.size(); ++column) {
            const auto type = cells[column].toObject()["type"].toString();
            const auto &seen = actual[row][column];
            const auto wanted = value(cells[column]);
            const bool same = type == "null"   ? seen.isNull()
                              : seen.isNull()  ? false
                              : type == "blob" ? seen.metaType().id() == QMetaType::QByteArray &&
                                                     seen.toByteArray() == wanted.toByteArray()
                              : type == "text" ? seen.metaType().id() == QMetaType::QString &&
                                                     seen.toString() == wanted.toString()
                                               : (seen.metaType().id() == QMetaType::LongLong ||
                                                  seen.metaType().id() == QMetaType::Int) &&
                                                     seen.toLongLong() == wanted.toLongLong();
            require(same, label + QString(": cell%1/%2").arg(row).arg(column));
        }
    }
}
struct Data {
    QJsonObject minimal{load(SUBATHON_ORIGINAL_CLAIM_VECTOR_PATH)};
    QJsonObject nonempty{load(SUBATHON_BUNDLE_VECTOR_PATH)};
    QList<QJsonObject> manifests;
    Data() {
        for (const auto *path : {SUBATHON_RETAINED_VECTOR_PATH, SUBATHON_RECEIPT_VECTOR_PATH,
                                 SUBATHON_CONTRIBUTION_VECTOR_PATH, SUBATHON_PLAN_VECTOR_PATH,
                                 SUBATHON_BUNDLE_VECTOR_PATH})
            manifests.append(load(path)["manifest"].toObject());
    }
    void schema(const SqliteWorkSession &session) const {
        for (const auto &entry : manifests.front()["expected_schema"].toArray()) {
            const auto object = entry.toObject();
            if (object["name"] == "subathon_catalog" ||
                object["name"] == "subathon_migration_history")
                (void)session.execute({object["sql"].toString(), {}});
        }
        for (const auto &manifest : manifests) {
            for (const auto &sql : manifest["sql"].toArray())
                (void)session.execute({sql.toString(), {}});
            (void)session.execute(
                {"INSERT INTO subathon_migration_history VALUES(?,?,?)",
                 {manifest["version"].toInt(), manifest["id"].toString(),
                  QByteArray::fromHex(manifest["sha256_hex"].toString().toLatin1())}});
        }
        (void)session.execute(
            {"INSERT INTO subathon_catalog VALUES(1,1,'subathon.sqlite-store')", {}});
    }
    void populate(const SqliteWorkSession &session, bool use_nonempty = false) const {
        schema(session);
        if (!use_nonempty) {
            execute(session, minimal["setup"].toArray());
        } else {
            execute(session, nonempty["literal_setup"].toArray());
            for (const auto *name : {"original_prepare", "original_finish"})
                execute(session, nonempty["scenarios"].toObject()[name].toArray());
        }
    }
};
inline StorageLimits limits() {
    StorageLimits l;
    l.statements_per_batch = 16384;
    l.statement_utf8_bytes = 4194304;
    l.bound_value_bytes = 33554432;
    l.result_rows = 65536;
    l.result_value_bytes = 33554432;
    return l;
}
inline SqliteWorkResult run(SqliteWriter &writer, SqliteWriter::Work body) {
    struct State {
        QString problem;
        SqliteWorkResult result;
    };
    auto state = std::make_shared<State>();
    state->result = qt_test_support::run_work(
        writer,
        [state, body = std::move(body)](auto context) {
            try {
                return body(context);
            } catch (const std::exception &e) {
                state->problem = QString::fromUtf8(e.what());
                throw;
            }
        },
        10000);
    require(state->problem.isEmpty(), "Fixture work: " + state->problem);
    require(state->result.error == WorkError::none &&
                state->result.transaction == TransactionOutcome::committed && state->result.value,
            "Actual committed normal old-writer completion");
    return state->result;
}
class Observer final {
  public:
    explicit Observer(const QString &path)
        : name_(QUuid::createUuid().toString()), db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
        require(db_.open(), "Independent fixture observer opens");
    }
    ~Observer() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }
    SqlRows rows(const QString &sql) {
        QSqlQuery query(db_);
        require(query.exec(sql), "Observer query: " + query.lastError().text());
        SqlRows result;
        while (query.next()) {
            SqlRow row;
            for (int i = 0; i < query.record().count(); ++i)
                row.append(query.value(i));
            result.append(std::move(row));
        }
        require(!query.lastError().isValid(), "Observer fetch");
        return result;
    }

  private:
    QString name_;
    QSqlDatabase db_;
};
inline core::ContributionState expected_contribution() {
    core::ContributionState s;
    s.payment_evidence = core::PaymentEvidence::not_applicable;
    s.conditions.valuation.monetary = false;
    s.conditions.review = core::ContributionReview::not_required;
    s.conditions.evidence_admission.policy = core::EvidencePolicy::accept_provider_report;
    s.conditions.evidence_admission.policy_ref = "p";
    s.conditions.settlement = core::SettlementGate::clear;
    return s;
}
inline ApplicationStateSnapshot expected_state(const std::string &token) {
    return {token,
            {"s", blob("")},
            "cfg",
            {core::TimerLifecycle::Active, core::CountdownPhase::Main, token == "d" ? 7 : 10,
             120000, false},
            0,
            {}};
}
inline RecordedApplicationBundle expected_record(char tag) {
    require(tag == 'A' || tag == 'B' || tag == 'C', "Known literal record");
    const bool a = tag == 'A', c = tag == 'C';
    const auto max = std::numeric_limits<std::uint64_t>::max();
    RecordedApplicationBundle r;
    r.intent.operation = {a ? "alpha" : c ? "gamma" : "beta", std::string(1, tag)};
    r.intent.subject_kind =
        c ? ApplicationSubjectKind::receipt : ApplicationSubjectKind::contribution;
    r.intent.receipt_id = {a ? 7 : c ? 8 : max};
    if (!c) {
        r.intent.contribution_id = core::ContributionId{a ? 0 : max};
        r.intent.expected_contribution_state = expected_contribution();
        r.contribution =
            ApplicationContributionSnapshot{*r.intent.contribution_id, expected_contribution(),
                                            core::ContributionApplication::applied};
    }
    r.intent.plan = {a ? "PA" : c ? "PC" : "PB", 1};
    r.intent.expected_state_token = a ? "a" : c ? "c" : "b";
    r.intent.choices = {"c", blob("")};
    r.persisted_head = expected_state(r.intent.expected_state_token);
    r.advanced_before = r.persisted_head;
    r.after = expected_state(a ? "b" : c ? "d" : "c");
    r.result = {"r", blob("")};
    r.audit.envelope = {"u", blob("")};
    if (c) {
        ApplicationEffect effect;
        effect.request_ordinal = 1;
        effect.arithmetic_role = ApplicationArithmeticRole::time_reversal;
        effect.exact_requested_units = -3;
        effect.requested_units = -3;
        effect.deficit_policy = core::RefundDeficitPolicy::StopAtZero;
        effect.deficit_disposition = core::TimeReversalDisposition::Applied;
        effect.application_explanation = "synthetic correction";
        effect.components = {{ApplicationComponentKind::main_delta, -3, {}},
                             {ApplicationComponentKind::debt_delta, 0, {}},
                             {ApplicationComponentKind::unapplied_time, 0, {}}};
        r.effects.push_back(std::move(effect));
    }
    return r;
}
inline void same_envelope(const ApplicationEnvelope &actual, const ApplicationEnvelope &expected) {
    require(actual.format_ref == expected.format_ref && actual.document == expected.document &&
                actual.document.isNull() == expected.document.isNull(),
            "Complete owned envelope");
}
inline void same_state(const ApplicationStateSnapshot &a, const ApplicationStateSnapshot &b) {
    same_envelope(a.envelope, b.envelope);
    require(a.token == b.token && a.configuration_ref == b.configuration_ref &&
                a.timer.lifecycle == b.timer.lifecycle && a.timer.phase == b.timer.phase &&
                a.timer.main_remaining_ms == b.timer.main_remaining_ms &&
                a.timer.grace_remaining_ms == b.timer.grace_remaining_ms &&
                a.timer.awaiting_confirmation == b.timer.awaiting_confirmation &&
                a.time_debt_ms == b.time_debt_ms && a.balances.empty() && b.balances.empty(),
            "Complete literal snapshot");
}
inline void same_record(const RecordedApplicationBundle &a, char tag) {
    const auto b = expected_record(tag);
    const auto &x = a.intent, &y = b.intent;
    require(x.operation.operation_scope == y.operation.operation_scope &&
                x.operation.operation_ref == y.operation.operation_ref && x.kind == y.kind &&
                x.subject_kind == y.subject_kind && x.receipt_id == y.receipt_id &&
                x.contribution_id == y.contribution_id && x.plan.plan_ref == y.plan.plan_ref &&
                x.plan.revision == y.plan.revision &&
                x.expected_state_token == y.expected_state_token &&
                x.expected_contribution_state == y.expected_contribution_state &&
                x.reversal_ref == y.reversal_ref,
            "Complete retained intent");
    same_envelope(x.choices, y.choices);
    same_state(a.persisted_head, b.persisted_head);
    same_state(a.advanced_before, b.advanced_before);
    same_state(a.after, b.after);
    require(a.advance_explanation == b.advance_explanation &&
                a.contribution.has_value() == b.contribution.has_value(),
            "Historical relation");
    if (a.contribution)
        require(a.contribution->contribution_id == b.contribution->contribution_id &&
                    a.contribution->before == b.contribution->before &&
                    a.contribution->application_after == b.contribution->application_after,
                "Complete historical contribution snapshot");
    require(a.effects.size() == b.effects.size() && a.jobs.empty() && a.transitions.empty() &&
                a.reversals.empty(),
            "Complete literal child sets");
    for (size_t i = 0; i < a.effects.size(); ++i) {
        const auto &e = a.effects[i], &f = b.effects[i];
        require(e.request_ordinal == f.request_ordinal && e.arithmetic_role == f.arithmetic_role &&
                    e.unit == f.unit && e.exact_requested_units == f.exact_requested_units &&
                    e.requested_units == f.requested_units &&
                    e.rounding_residual == f.rounding_residual &&
                    e.maximum_main_ms == f.maximum_main_ms &&
                    e.deficit_policy == f.deficit_policy &&
                    e.deficit_disposition == f.deficit_disposition &&
                    e.application_explanation == f.application_explanation &&
                    e.components.size() == f.components.size(),
                "Complete literal effect");
        for (size_t j = 0; j < e.components.size(); ++j)
            require(e.components[j].kind == f.components[j].kind &&
                        e.components[j].units == f.components[j].units &&
                        e.components[j].target_ref == f.components[j].target_ref,
                    "Complete component");
    }
    same_envelope(a.result, b.result);
    same_envelope(a.audit.envelope, b.audit.envelope);
    require(a.audit.actor_ref == b.audit.actor_ref &&
                a.audit.decision_ref == b.audit.decision_ref && a.audit.reason == b.audit.reason,
            "Complete optional audit fields");
}
} // namespace original_claim_fixture
