// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QTest>
#include <QUuid>
#include <QVariant>
#include <subathon/server/sqlite_contributions.hpp>
#include <subathon/server/sqlite_receipts.hpp>
#include <utility>

namespace qt_test_support::sqlite_fixture {
using namespace subathon::server;
using namespace subathon::core;

inline QByteArray blob(const std::string &s) {
    return s.empty() ? QByteArray("", 0) : QByteArray(s.data(), qsizetype(s.size()));
}
inline std::string opaque(const QByteArray &s) {
    return std::string(s.constData(), size_t(s.size()));
}

inline ReceiptInput input(std::string id = "event-42") {
    return {{{"synthetic.provider-a", "source-account-7"}, "synthetic.event.v1", std::move(id)},
            {"synthetic.payment.received", 1, 1700000000000, "actor-3", "Synthetic Supporter",
             ReceiptMoney{"12.3400", "EUR"}, ReceiptLinkReference{"payment.v1", "payment-9"},
             ReceiptLinkReference{"activity.v1", "activity-2"}},
            {1700000000100, "connection-1", "cfg1", "ctx1"}};
}
inline ReceiptAttempt attempt(const ReceiptInput &value, ReceiptDisposition disposition) {
    return {value.arrival, value.facts, disposition};
}
inline const QStringList facts_columns = {
    "event_type",         "schema_version",         "source_timestamp_utc_ms", "actor_id",
    "actor_display_name", "money_original_decimal", "money_currency",          "payment_namespace",
    "payment_opaque_id",  "activity_namespace",     "activity_opaque_id"};
inline const QStringList arrival_columns = {"server_timestamp_utc_ms", "connection_ref",
                                            "configuration_ref", "modifier_context_ref"};
inline QList<QVariant> facts_values(const ReceiptFacts &v) {
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
inline QList<QVariant> arrival_values(const ReceiptArrival &v) {
    return {qint64(v.server_timestamp_utc_ms), blob(v.connection_ref), blob(v.configuration_ref),
            blob(v.modifier_context_ref)};
}
inline QString quoted(QString name) {
    name.replace('"', "\"\"");
    return '"' + name + '"';
}

// Independent reference packing for disposable SQL seeds/observations only.
// Expected new-codec vectors still come from literal JSON, never this UUT.
inline QByteArray id_blob(std::uint64_t value) {
    QByteArray out(8, '\0');
    for (int i = 7; i >= 0; --i) {
        out[i] = char(value & 255U);
        value >>= 8;
    }
    return out;
}
inline ContributionConditions captured() {
    return {
        {true, ReceiptMoney{"12.3400", "EUR"}, {}, true},
        ContributionReview::pending,
        {EvidencePolicy::require_evidence_review, "policy1",
         EvidenceReviewDecision{EvidenceReviewChoice::await_confirmation, "decision1", "actor1"}},
        ContributionApplication::not_applied,
        ContributionReversal::none,
        SettlementGate::clear};
}
inline ContributionObservation support(std::uint64_t id = 1) {
    auto r = input("support-" + std::to_string(id));
    return {{{id}, r.identity, r.facts, r.arrival},
            ContributionObservationKind::support,
            PaymentEvidence::unknown,
            VerifiedActivityLink{{r.identity.origin, "activity.v1", "activity-9"}, "proof1"},
            captured()};
}
inline ContributionObservation confirmation(std::uint64_t id = 2) {
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
inline ContributionState initial(const ContributionObservation &s) {
    return {s.evidence, *s.support_conditions};
}
inline QList<QVariant> conditions_values(const ContributionConditions &c) {
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
inline const QStringList condition_columns = {
    "monetary",        "original_decimal", "currency",           "resolution_ref",
    "missing_rate",    "review",           "evidence_policy",    "policy_ref",
    "decision_choice", "decision_ref",     "decision_actor_ref", "application",
    "reversal",        "settlement"};
inline QList<QVariant> activity_values(const std::optional<VerifiedActivityIdentity> &key) {
    if (!key)
        return {QVariant(), QVariant(), QVariant(), QVariant()};
    return {blob(key->origin.provider), blob(key->origin.source_account),
            blob(key->identifier_namespace), blob(key->opaque_id)};
}
inline const QStringList activity_columns = {"activity_provider", "activity_source_account",
                                             "activity_namespace", "activity_id"};

// Negative/raw fixtures are independent SQL, never new codec/factory/repository
// output. OFF/ignore-checks applies only to this fixture, not the product writer.
class ReceiptFixture {
  public:
    explicit ReceiptFixture(const QString &path)
        : name_(QUuid::createUuid().toString()), db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
    }
    ~ReceiptFixture() {
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
    qint64 count(const QString &table) {
        return scalar("SELECT count(*) FROM main." + table).toLongLong();
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

  protected:
    bool insert(const QString &table, const QStringList &columns, const QList<QVariant> &values) {
        QStringList marks;
        for (qsizetype i = 0; i < values.size(); ++i)
            marks.append("?");
        return exec("INSERT INTO main." + table + "(" + columns.join(',') + ") VALUES (" +
                        marks.join(',') + ")",
                    values);
    }

  private:
    QString name_;
    QSqlDatabase db_;
};

class ContributionFixture : public ReceiptFixture {
  public:
    using ReceiptFixture::ReceiptFixture;
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
};

} // namespace qt_test_support::sqlite_fixture
