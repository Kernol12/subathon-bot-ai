// SPDX-License-Identifier: GPL-3.0-or-later
// Specification-derived bundle baseline; fixture/setup uses only frozen literals and old writer.
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <subathon/server/sqlite_application_bundle.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <thread>
#include <tuple>
using namespace subathon::server;
using namespace subathon::core;
using subathon::server::testing::SqliteWriterWorkAccess;
using subathon::server::testing::WorkControlFault;
namespace {
using CK = ApplicationComponentKind;
using AR = ApplicationArithmeticRole;
using SD = SqliteBundleStageDisposition;
using IE = SqliteBundleInputError;
using SR = SqliteBundleStorageReason;
using Cells = QMap<QString, QVariant>;
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}
QByteArray blob(const std::string &value) {
    return value.empty() ? QByteArray("", 0) : QByteArray(value.data(), qsizetype(value.size()));
}
std::string bytes(const QVariant &value) {
    require(!value.isNull() && value.metaType().id() == QMetaType::QByteArray,
            "literal expected BLOB, not text/default");
    const auto array = value.toByteArray();
    return {array.constData(), size_t(array.size())};
}
std::optional<std::string> optional_bytes(const QVariant &value) {
    return value.isNull() ? std::nullopt : std::optional(bytes(value));
}
QByteArray id_blob(std::uint64_t value) {
    QByteArray result(8, '\0');
    for (int position = 7; position >= 0; --position) {
        result[position] = char(value & 255U);
        value >>= 8;
    }
    return result;
}
std::uint64_t reference_id(const QVariant &value) {
    const auto array = value.toByteArray();
    require(!value.isNull() && array.size() == 8, "literal uint64 shape");
    std::uint64_t result = 0;
    for (unsigned char octet : array)
        result = (result << 8) | octet;
    return result;
}
boost::multiprecision::cpp_int reference_integer(const std::string &value) {
    require(!value.empty(), "literal integer missing");
    const bool negative = value.front() == '-';
    boost::multiprecision::cpp_int result = 0;
    for (size_t index = negative ? 1 : 0; index < value.size(); ++index) {
        require(value[index] >= '0' && value[index] <= '9', "literal integer syntax");
        result = result * 10 + (value[index] - '0');
    }
    return negative ? -result : result;
}
Exact exact(const std::string &numerator, const std::string &denominator = "1") {
    return Exact(reference_integer(numerator)) / Exact(reference_integer(denominator));
}
QVariant literal_value(const QJsonValue &entry) {
    const auto object = entry.toObject();
    const auto type = object["type"].toString();
    if (type == "null")
        return {};
    if (type == "blob") {
        const auto value = QByteArray::fromHex(object["hex"].toString().toLatin1());
        return value.isEmpty() ? QByteArray("", 0) : value;
    }
    if (type == "text")
        return object["value"].toString();
    bool ok = false;
    const auto value = object["decimal"].toString().toLongLong(&ok);
    require(type == "integer" && ok, "literal signed64 decimal");
    return value;
}
SqlStatement statement(const QJsonValue &entry) {
    const auto object = entry.toObject();
    QList<QVariant> bindings;
    for (const auto &value : object["bindings"].toArray())
        bindings.append(literal_value(value));
    return {object["sql"].toString(), bindings};
}
QString quoted(QString value) {
    value.replace('"', "\"\"");
    return '"' + value + '"';
}
StorageLimits ample() {
    StorageLimits limits;
    limits.statements_per_batch = 16384;
    limits.statement_utf8_bytes = 4194304;
    limits.bound_value_bytes = 33554432;
    limits.result_rows = 65536;
    limits.result_value_bytes = 33554432;
    return limits;
}
struct Run {
    bool done{false}, stub{false};
    QString problem;
    std::optional<IE> input;
    std::optional<SR> storage;
    std::optional<WorkError> synchronous;
    SqliteWorkResult result;
};
Run run(SqliteWriter &writer, SqliteWriter::Work body) {
    auto result = std::make_shared<Run>();
    const auto admitted = writer.enqueue_work(
        [body = std::move(body), result](auto context) -> QVariant {
            try {
                return body(context);
            } catch (const SqliteApplicationBundleNotImplemented &error) {
                result->stub = true;
                result->problem = QString::fromUtf8(error.what());
                throw;
            } catch (const SqliteBundleInputFailure &error) {
                result->input = error.error();
                throw;
            } catch (const SqliteBundleStorageFailure &error) {
                result->storage = error.reason();
                throw;
            } catch (const SqliteWorkFailure &error) {
                result->synchronous = error.error();
                throw;
            } catch (const std::exception &error) {
                result->problem = QString::fromUtf8(error.what());
                throw;
            }
        },
        [result](auto value) {
            result->result = std::move(value);
            result->done = true;
        });
    require(admitted == StorageError::none, "old writer work admission");
    require(QTest::qWaitFor([result] { return result->done; }, 10000), "old writer completion");
    return *result;
}
void no_stub(const Run &result) {
    if (result.stub)
        throw SqliteApplicationBundleNotImplemented();
}
void ok(const Run &result) {
    no_stub(result);
    require(result.done && result.result.transaction == TransactionOutcome::committed &&
                result.result.error == WorkError::none && result.result.value && !result.input &&
                !result.storage && !result.synchronous,
            "required committed normal work: " + result.problem.toStdString());
}
void clean_rollback(const Run &result, WorkError expected = WorkError::not_committed) {
    no_stub(result);
    require(result.done && result.result.transaction == TransactionOutcome::rolled_back &&
                result.result.error == expected && !result.result.value && !result.input &&
                !result.storage && !result.synchronous && result.problem.isEmpty(),
            "only a completed deliberate caller rollback may satisfy abandonment");
}
void known(const Run &result, SR reason, WorkError work = WorkError::work_exception) {
    no_stub(result);
    require(result.storage == reason &&
                result.result.transaction == TransactionOutcome::rolled_back &&
                result.result.error == work && !result.result.value,
            "required established whole-unit rollback and typed bundle storage failure");
}
template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const SqliteApplicationBundleNotImplemented &error) {
        QTest::qFail(error.what(), __FILE__, __LINE__);
    } catch (const std::exception &error) {
        QTest::qFail(error.what(), __FILE__, __LINE__);
    }
}
template <class Body> void input_failure(Body body, IE expected) {
    try {
        body();
    } catch (const SqliteBundleInputFailure &error) {
        require(error.error() == expected, "specific pure input error category");
        return;
    }
    throw std::runtime_error("required pure input failure absent");
}
template <class Body> void work_failure(Body body, WorkError expected) {
    try {
        body();
    } catch (const SqliteWorkFailure &error) {
        require(error.error() == expected, "specific actual work capability/control category");
        return;
    }
    throw std::runtime_error("required actual work failure absent");
}

// Expectations are independently frozen INSERT literals, never rows returned by the new port.
class Literals final {
  public:
    QJsonObject root;
    QList<QJsonObject> manifests;
    struct Row {
        QString table;
        Cells cells;
    };
    QList<Row> rows;
    static QJsonObject read(const char *path) {
        QFile file(QString::fromUtf8(path));
        require(file.open(QIODevice::ReadOnly), "frozen independent fixture exists");
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(file.readAll(), &error);
        require(error.error == QJsonParseError::NoError && document.isObject(),
                "fixture JSON parse");
        return document.object();
    }
    Literals() {
        for (const auto *path : {SUBATHON_RETAINED_VECTOR_PATH, SUBATHON_RECEIPT_VECTOR_PATH,
                                 SUBATHON_CONTRIBUTION_VECTOR_PATH, SUBATHON_PLAN_VECTOR_PATH})
            manifests.append(read(path)["manifest"].toObject());
        root = read(SUBATHON_BUNDLE_VECTOR_PATH);
        manifests.append(root["manifest"].toObject());
        consume(root["literal_setup"].toArray());
        const auto sequences = root["scenarios"].toObject();
        for (const auto &name : {"original_prepare", "original_finish", "reversal_one",
                                 "reversal_two", "score_reversal"})
            consume(sequences[name].toArray());
    }
    void consume(const QJsonArray &steps) {
        const QRegularExpression insert("^INSERT INTO main\\.([a-z0-9_]+)\\(([^)]+)\\) VALUES ");
        for (const auto &entry : steps) {
            const auto value = statement(entry);
            const auto match = insert.match(value.sql);
            if (!match.hasMatch())
                continue;
            const auto fields = match.captured(2).split(',');
            require(fields.size() == value.bindings.size(), "literal INSERT width");
            Cells cells;
            for (qsizetype index = 0; index < fields.size(); ++index)
                cells.insert(fields[index], value.bindings[index]);
            rows.append({match.captured(1), cells});
        }
    }
    QList<Cells> all(const QString &table, const Cells &key = {}) const {
        QList<Cells> result;
        for (const auto &row : rows) {
            if (row.table != table)
                continue;
            bool match = true;
            for (auto it = key.begin(); it != key.end(); ++it)
                match = match && row.cells.value(it.key()) == it.value();
            if (match)
                result.append(row.cells);
        }
        return result;
    }
    Cells one(const QString &table, const Cells &key) const {
        const auto result = all(table, key);
        require(result.size() == 1, "one independently literal row: " + table.toStdString());
        return result[0];
    }
    static ContributionState contribution(const Cells &r, const QString &prefix = {}) {
        const auto v = [&](const char *name) { return r.value(prefix + name); };
        ContributionConditions c;
        c.valuation.monetary = v("monetary").toBool();
        if (!v("original_decimal").isNull())
            c.valuation.original_money =
                ReceiptMoney{bytes(v("original_decimal")), bytes(v("currency"))};
        c.valuation.resolution_ref = optional_bytes(v("resolution_ref"));
        c.valuation.missing_rate = v("missing_rate").toBool();
        c.review = ContributionReview(v("review").toInt());
        c.evidence_admission.policy = EvidencePolicy(v("evidence_policy").toInt());
        c.evidence_admission.policy_ref = bytes(v("policy_ref"));
        if (!v("decision_choice").isNull())
            c.evidence_admission.review_decision =
                EvidenceReviewDecision{EvidenceReviewChoice(v("decision_choice").toInt()),
                                       bytes(v("decision_ref")), bytes(v("decision_actor_ref"))};
        c.application = ContributionApplication(v("application").toInt());
        c.reversal = ContributionReversal(v("reversal").toInt());
        c.settlement = SettlementGate(v("settlement").toInt());
        return {PaymentEvidence(r.value("payment_evidence").toInt()), c};
    }
    ContributionReceipt receipt(std::uint64_t id) const {
        const auto r = one("subathon_receipts", {{"receipt_id", id_blob(id)}});
        ReceiptFacts f;
        f.event_type = bytes(r["event_type"]);
        f.schema_version = std::uint32_t(r["schema_version"].toULongLong());
        if (!r["source_timestamp_utc_ms"].isNull())
            f.source_timestamp_utc_ms = r["source_timestamp_utc_ms"].toLongLong();
        f.actor_id = optional_bytes(r["actor_id"]);
        f.actor_display_name = optional_bytes(r["actor_display_name"]);
        if (!r["money_original_decimal"].isNull())
            f.money = ReceiptMoney{bytes(r["money_original_decimal"]), bytes(r["money_currency"])};
        if (!r["payment_namespace"].isNull())
            f.payment_reference =
                ReceiptLinkReference{bytes(r["payment_namespace"]), bytes(r["payment_opaque_id"])};
        if (!r["activity_namespace"].isNull())
            f.activity_reference = ReceiptLinkReference{bytes(r["activity_namespace"]),
                                                        bytes(r["activity_opaque_id"])};
        return {{id},
                {{bytes(r["provider"]), bytes(r["source_account"])},
                 bytes(r["identifier_namespace"]),
                 bytes(r["opaque_id"])},
                f,
                {r["server_timestamp_utc_ms"].toLongLong(), bytes(r["connection_ref"]),
                 bytes(r["configuration_ref"]), bytes(r["modifier_context_ref"])}};
    }
    ApplicationStateSnapshot state(const std::string &token) const {
        const auto r = one("subathon_app_states", {{"token", blob(token)}});
        ApplicationStateSnapshot result{
            token,
            {bytes(r["format_ref"]), r["document"].toByteArray()},
            bytes(r["configuration_ref"]),
            {TimerLifecycle(r["lifecycle"].toInt()), CountdownPhase(r["phase"].toInt()),
             r["main_remaining_ms"].toLongLong(), r["grace_remaining_ms"].toLongLong(),
             r["awaiting_confirmation"].toBool()},
            r["time_debt_ms"].toLongLong(),
            {}};
        for (const auto &balance : all("subathon_app_balances", {{"token", blob(token)}}))
            result.balances.push_back({ApplicationBalanceKind(balance["balance_kind"].toInt()),
                                       bytes(balance["target_ref"]),
                                       balance["units"].toLongLong()});
        return result;
    }
    CapturedPlanRevision capture(const std::string &plan) const {
        const auto h = one("subathon_plan_headers", {{"plan_ref", blob(plan)}});
        const auto r = one("subathon_plan_revisions", {{"plan_ref", blob(plan)}});
        const auto source = receipt(reference_id(h["receipt_id"]));
        CapturedPlanRevision result;
        result.revision = 1;
        result.captured.event = {
            bytes(h["event_ref"]),
            optional_bytes(h["parent_event_ref"]),
            bytes(h["root_event_ref"]),
            source.id,
            source.identity,
            source.facts,
            source.arrival,
            h["contribution_id"].isNull()
                ? std::nullopt
                : std::optional(ContributionId{reference_id(h["contribution_id"])})};
        result.captured.contribution_state = contribution(r);
        result.captured.valuation.state = PlanValuationState(r["valuation_state"].toInt());
        if (result.captured.valuation.state == PlanValuationState::resolved) {
            HistoricalValuation quote{
                bytes(r["historical_resolution_ref"]),
                {bytes(r["historical_original_decimal"]), bytes(r["historical_currency"])},
                bytes(r["comparison_currency"]),
                bytes(r["effective_rate_decimal"]),
                optional_bytes(r["fetched_rate_decimal"]),
                bytes(r["snapshot_ref"]),
                optional_bytes(r["rate_source_ref"]),
                {},
                {},
                optional_bytes(r["manual_decision_ref"])};
            if (!r["rate_timestamp_utc_ms"].isNull())
                quote.rate_timestamp_utc_ms = r["rate_timestamp_utc_ms"].toLongLong();
            if (!r["fetch_timestamp_utc_ms"].isNull())
                quote.fetch_timestamp_utc_ms = r["fetch_timestamp_utc_ms"].toLongLong();
            result.captured.valuation.historical = quote;
        }
        for (const auto &row : all("subathon_plan_requests", {{"plan_ref", blob(plan)}}))
            result.captured.requested_effects.push_back(
                {bytes(row["action_ref"]), bytes(row["action_version_ref"]), bytes(row["rule_ref"]),
                 bytes(row["target_ref"]), RequestedUnit(row["unit"].toInt()),
                 row["requires_historical_valuation"].toBool(),
                 exact(bytes(row["exact_numerator"]), bytes(row["exact_denominator"])),
                 row["requested_units"].toLongLong(), bytes(row["calculation_explanation"]),
                 bytes(row["reversal_behavior_ref"])});
        for (const auto &row : all("subathon_plan_rules", {{"plan_ref", blob(plan)}}))
            result.rules.push_back({bytes(row["rule_ref"]), bytes(row["version_ref"]),
                                    row["selected"].toBool(), bytes(row["explanation"])});
        for (const auto &row : all("subathon_plan_modifiers", {{"plan_ref", blob(plan)}}))
            result.modifiers.push_back(
                {bytes(row["definition_version_ref"]), bytes(row["instance_revision_ref"]),
                 bytes(row["target_ref"]), bytes(row["group_ref"]), bytes(row["group_operator"]),
                 bytes(row["target_operator"]), bytes(row["original_factor_decimal"])});
        return result;
    }
    PreparedApplicationBundle prepared(const std::string &op = "original") const {
        const auto r = one("subathon_app_operations", {{"operation_ref", blob(op)}});
        PreparedApplicationBundle result;
        auto &v = result.intent;
        v.operation = {bytes(r["operation_scope"]), op};
        v.kind = ApplicationOperationKind(r["operation_kind"].toInt());
        v.subject_kind = ApplicationSubjectKind(r["subject_kind"].toInt());
        v.receipt_id = {reference_id(r["receipt_id"])};
        if (!r["contribution_id"].isNull())
            v.contribution_id = ContributionId{reference_id(r["contribution_id"])};
        v.plan = {bytes(r["plan_ref"]), reference_id(r["plan_revision"])};
        v.expected_state_token = bytes(r["head_token"]);
        if (v.contribution_id)
            v.expected_contribution_state =
                contribution(one("subathon_app_contributions", {{"operation_ref", blob(op)}}));
        v.choices = {bytes(r["choices_format_ref"]), r["choices_document"].toByteArray()};
        v.reversal_ref = optional_bytes(r["reversal_ref"]);
        result.advanced_before = state(bytes(r["before_token"]));
        result.after = state(bytes(r["after_token"]));
        result.advance_explanation = bytes(r["advance_explanation"]);
        for (const auto &e : all("subathon_app_effects", {{"operation_ref", blob(op)}})) {
            ApplicationEffect effect;
            effect.request_ordinal = e["request_ordinal"].toLongLong();
            effect.arithmetic_role = AR(e["arithmetic_role"].toInt());
            effect.unit = RequestedUnit(e["unit"].toInt());
            effect.exact_requested_units =
                exact(bytes(e["exact_numerator"]), bytes(e["exact_denominator"]));
            effect.requested_units = e["requested_units"].toLongLong();
            effect.rounding_residual =
                exact(bytes(e["residual_numerator"]), bytes(e["residual_denominator"]));
            if (!e["maximum_main_ms"].isNull())
                effect.maximum_main_ms = e["maximum_main_ms"].toLongLong();
            if (!e["deficit_policy"].isNull())
                effect.deficit_policy = RefundDeficitPolicy(e["deficit_policy"].toInt());
            if (!e["deficit_disposition"].isNull())
                effect.deficit_disposition =
                    TimeReversalDisposition(e["deficit_disposition"].toInt());
            effect.application_explanation = bytes(e["application_explanation"]);
            for (const auto &c : all("subathon_app_components",
                                     {{"operation_ref", blob(op)},
                                      {"request_ordinal", qint64(effect.request_ordinal)}}))
                effect.components.push_back({CK(c["component_kind"].toInt()),
                                             c["units"].toLongLong(),
                                             optional_bytes(c["target_ref"])});
            result.effects.push_back(effect);
        }
        for (const auto &j : all("subathon_app_jobs", {{"operation_ref", blob(op)}})) {
            ApplicationJobIntent job{bytes(j["job_ref"]),
                                     {},
                                     optional_bytes(j["group_ref"]),
                                     {},
                                     {bytes(j["format_ref"]), j["document"].toByteArray()}};
            if (!j["source_request_ordinal"].isNull())
                job.source_request_ordinal = j["source_request_ordinal"].toLongLong();
            if (!j["group_ordinal"].isNull())
                job.group_ordinal = j["group_ordinal"].toLongLong();
            result.jobs.push_back(job);
        }
        for (const auto &link : all("subathon_app_reversals", {{"operation_ref", blob(op)}}))
            result.reversals.push_back(
                {link["request_ordinal"].toLongLong(),
                 {{bytes(link["original_scope"]), bytes(link["original_operation"])},
                  link["original_request_ordinal"].toLongLong(),
                  CK(link["original_component_kind"].toInt())},
                 link["basis_position"].toLongLong() - 1,
                 link["previous_cumulative"].toLongLong(),
                 link["cumulative"].toLongLong(),
                 link["requested_increment"].toLongLong(),
                 exact(bytes(link["fraction_numerator"]), bytes(link["fraction_denominator"])),
                 exact(bytes(link["entitlement_numerator"]),
                       bytes(link["entitlement_denominator"])),
                 exact(bytes(link["residual_numerator"]), bytes(link["residual_denominator"])),
                 link["full_refund"].toBool()});
        const auto output = one("subathon_app_results", {{"operation_ref", blob(op)}});
        result.result = {bytes(output["format_ref"]), output["document"].toByteArray()};
        const auto audit = one("subathon_app_audit", {{"operation_ref", blob(op)}});
        result.audit = {optional_bytes(audit["actor_ref"]),
                        optional_bytes(audit["decision_ref"]),
                        optional_bytes(audit["reason"]),
                        {bytes(audit["format_ref"]), audit["document"].toByteArray()}};
        return result;
    }
};

bool same(const ApplicationOperationKey &a, const ApplicationOperationKey &b) {
    return std::tie(a.operation_scope, a.operation_ref) ==
           std::tie(b.operation_scope, b.operation_ref);
}
bool same(const ApplicationPlanKey &a, const ApplicationPlanKey &b) {
    return a.plan_ref == b.plan_ref && a.revision == b.revision;
}
bool same(const ApplicationEnvelope &a, const ApplicationEnvelope &b) {
    return a.format_ref == b.format_ref && a.document == b.document &&
           a.document.isNull() == b.document.isNull();
}
bool same(const ApplicationBalance &a, const ApplicationBalance &b) {
    return std::tie(a.kind, a.target_ref, a.units) == std::tie(b.kind, b.target_ref, b.units);
}
template <class T, class Compare>
bool same_vector(const std::vector<T> &a, const std::vector<T> &b, Compare compare) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), compare);
}
bool same(const ApplicationStateSnapshot &a, const ApplicationStateSnapshot &b) {
    return a.token == b.token && same(a.envelope, b.envelope) &&
           a.configuration_ref == b.configuration_ref &&
           std::tie(a.timer.lifecycle, a.timer.phase, a.timer.main_remaining_ms,
                    a.timer.grace_remaining_ms, a.timer.awaiting_confirmation, a.time_debt_ms) ==
               std::tie(b.timer.lifecycle, b.timer.phase, b.timer.main_remaining_ms,
                        b.timer.grace_remaining_ms, b.timer.awaiting_confirmation,
                        b.time_debt_ms) &&
           same_vector(a.balances, b.balances, [](auto &x, auto &y) { return same(x, y); });
}
bool same(const ApplicationIntent &a, const ApplicationIntent &b) {
    return same(a.operation, b.operation) && a.kind == b.kind && a.subject_kind == b.subject_kind &&
           a.receipt_id == b.receipt_id && a.contribution_id == b.contribution_id &&
           same(a.plan, b.plan) && a.expected_state_token == b.expected_state_token &&
           a.expected_contribution_state == b.expected_contribution_state &&
           same(a.choices, b.choices) && a.reversal_ref == b.reversal_ref;
}
bool same(const ApplicationComponent &a, const ApplicationComponent &b) {
    return std::tie(a.kind, a.units, a.target_ref) == std::tie(b.kind, b.units, b.target_ref);
}
bool same(const ApplicationEffect &a, const ApplicationEffect &b) {
    return a.request_ordinal == b.request_ordinal && a.arithmetic_role == b.arithmetic_role &&
           a.unit == b.unit && a.exact_requested_units == b.exact_requested_units &&
           a.requested_units == b.requested_units && a.rounding_residual == b.rounding_residual &&
           a.maximum_main_ms == b.maximum_main_ms && a.deficit_policy == b.deficit_policy &&
           a.deficit_disposition == b.deficit_disposition &&
           a.application_explanation == b.application_explanation &&
           same_vector(a.components, b.components, [](auto &x, auto &y) { return same(x, y); });
}
bool same(const ApplicationJobIntent &a, const ApplicationJobIntent &b) {
    return std::tie(a.job_ref, a.source_request_ordinal, a.group_ref, a.group_ordinal) ==
               std::tie(b.job_ref, b.source_request_ordinal, b.group_ref, b.group_ordinal) &&
           same(a.envelope, b.envelope);
}
bool same(const ApplicationAudit &a, const ApplicationAudit &b) {
    return std::tie(a.actor_ref, a.decision_ref, a.reason) ==
               std::tie(b.actor_ref, b.decision_ref, b.reason) &&
           same(a.envelope, b.envelope);
}
bool same(const ApplicationOriginalComponent &a, const ApplicationOriginalComponent &b) {
    return same(a.operation, b.operation) && a.request_ordinal == b.request_ordinal &&
           a.component_kind == b.component_kind;
}
bool same(const ApplicationReversalOccurrence &a, const ApplicationReversalOccurrence &b) {
    return a.request_ordinal == b.request_ordinal && same(a.original, b.original) &&
           std::tie(a.basis_position, a.previous_cumulative, a.cumulative, a.requested_increment,
                    a.cumulative_fraction, a.exact_entitlement, a.rounding_residual,
                    a.full_refund) == std::tie(b.basis_position, b.previous_cumulative,
                                               b.cumulative, b.requested_increment,
                                               b.cumulative_fraction, b.exact_entitlement,
                                               b.rounding_residual, b.full_refund);
}
void matches(const RecordedApplicationBundle &actual, const PreparedApplicationBundle &expected,
             const ApplicationStateSnapshot &head) {
    require(same(actual.intent, expected.intent) && same(actual.persisted_head, head) &&
                same(actual.advanced_before, expected.advanced_before) &&
                same(actual.after, expected.after) &&
                actual.advance_explanation == expected.advance_explanation,
            "complete original intent/three snapshots/advance explanation");
    require(
        same_vector(actual.effects, expected.effects,
                    [](auto &x, auto &y) { return same(x, y); }) &&
            same_vector(actual.jobs, expected.jobs, [](auto &x, auto &y) { return same(x, y); }) &&
            actual.transitions == expected.transitions && same(actual.result, expected.result) &&
            same(actual.audit, expected.audit),
        "all ordered effects/components/jobs/trace/result/audit");
    require(actual.contribution.has_value() == expected.intent.contribution_id.has_value(),
            "historical contribution presence");
    if (actual.contribution)
        require(actual.contribution->contribution_id == *expected.intent.contribution_id &&
                    actual.contribution->before == *expected.intent.expected_contribution_state &&
                    actual.contribution->application_after ==
                        (expected.intent.kind == ApplicationOperationKind::original
                             ? ContributionApplication::applied
                             : expected.intent.expected_contribution_state->conditions.application),
                "complete historical CURRENT snapshot/application-only outcome");
    require(actual.reversals.size() == expected.reversals.size(),
            "all many-basis reversal occurrences");
    for (size_t index = 0; index < expected.reversals.size(); ++index) {
        const auto &e = expected.reversals[index];
        const auto &a = actual.reversals[index];
        require(e.expected_prior_position >= 0 &&
                    e.expected_prior_position < std::numeric_limits<std::int64_t>::max(),
                "finite independent expected occurrence position");
        const ApplicationReversalOccurrence expected_occurrence{e.request_ordinal,
                                                                e.original,
                                                                e.expected_prior_position + 1,
                                                                e.expected_prior_cumulative,
                                                                e.cumulative,
                                                                e.requested_increment,
                                                                e.cumulative_fraction,
                                                                e.exact_entitlement,
                                                                e.rounding_residual,
                                                                e.full_refund};
        require(same(a, expected_occurrence),
                "exact per-original occurrence/progress/source provenance");
    }
}

class Fixture final {
  public:
    QString path;
    const Literals &literals;
    Fixture(QString file, const Literals &values)
        : path(std::move(file)), literals(values), name_(QUuid::createUuid().toString()),
          db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        SqliteWriter writer;
        require(writer.open(path, ample()) == StorageError::none, "literal setup writer open");
        ok(run(writer, [&](auto context) -> QVariant {
            auto s = context.begin();
            for (const auto &entry : literals.manifests[0]["expected_schema"].toArray()) {
                const auto o = entry.toObject();
                if (o["name"] == "subathon_catalog" || o["name"] == "subathon_migration_history")
                    (void)s.execute({o["sql"].toString(), {}});
            }
            for (const auto &m : literals.manifests)
                for (const auto &sql : m["sql"].toArray())
                    (void)s.execute({sql.toString(), {}});
            (void)s.execute(
                {"INSERT INTO subathon_catalog VALUES(1,1,'subathon.sqlite-store')", {}});
            for (const auto &m : literals.manifests)
                (void)s.execute({"INSERT INTO subathon_migration_history VALUES(?,?,?)",
                                 {m["version"].toInt(), m["id"].toString(),
                                  QByteArray::fromHex(m["sha256_hex"].toString().toLatin1())}});
            for (const auto &e : literals.root["literal_setup"].toArray())
                (void)s.execute(statement(e));
            s.commit();
            return 1;
        }));
        db_.setDatabaseName(path);
        db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
        require(db_.open(), "independent fixture observer open");
        require(exec("PRAGMA foreign_keys=OFF"),
                "fixture-only corruption connection; controlled writer keeps FK enforcement");
    }
    ~Fixture() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }
    bool exec(const QString &sql, const QList<QVariant> &values = {}) {
        QSqlQuery q(db_);
        if (!q.prepare(sql))
            return false;
        for (const auto &v : values)
            q.addBindValue(v);
        return q.exec();
    }
    SqlRows rows(const QString &sql, const QList<QVariant> &values = {}) const {
        QSqlQuery q(db_);
        require(q.prepare(sql), "observer prepare: " + q.lastError().text().toStdString());
        for (const auto &v : values)
            q.addBindValue(v);
        require(q.exec(), "observer query: " + q.lastError().text().toStdString());
        SqlRows result;
        while (q.next()) {
            SqlRow row;
            for (int i = 0; i < q.record().count(); ++i)
                row.append(q.value(i));
            result.append(row);
        }
        require(!q.lastError().isValid(), "observer fetch");
        return result;
    }
    QVariant scalar(const QString &sql, const QList<QVariant> &values = {}) const {
        auto r = rows(sql, values);
        require(r.size() == 1 && r[0].size() == 1, "observer scalar shape");
        return r[0][0];
    }
    bool released() { return exec("BEGIN IMMEDIATE") && exec("ROLLBACK"); }
    QList<SqlRows> snapshot(bool prefix_only = false) const {
        QList<SqlRows> result;
        const auto schema =
            rows("SELECT type,name,tbl_name,sql FROM sqlite_schema ORDER BY type,name");
        SqlRows selected_schema;
        for (const auto &object : schema)
            if (!prefix_only || !object[2].toString().startsWith("subathon_app_"))
                selected_schema.append(object);
        result.append(selected_schema);
        for (const auto &object : selected_schema) {
            if (object[0].toString() != "table")
                continue;
            const auto table = object[1].toString();
            QStringList fields;
            for (const auto &column : rows("PRAGMA table_info(" + quoted(table) + ")")) {
                const auto name = column[1].toString();
                auto expression = quoted(name);
                // The separately asserted CID0 application0→1 is the ONLY allowed prefix delta.
                if (prefix_only && table == "subathon_contributions" && name == "application")
                    expression =
                        "CASE WHEN contribution_id=X'0000000000000000' THEN 0 ELSE application END";
                fields.append("typeof(" + expression + ")");
                fields.append("hex(CAST(" + expression + " AS BLOB))");
            }
            result.append(
                rows("SELECT " + fields.join(',') + " FROM " + quoted(table) + " ORDER BY rowid"));
        }
        return result;
    }
    static SqlStatement insert(const QString &table, const Cells &cells) {
        QStringList columns, placeholders;
        QList<QVariant> values;
        for (auto it = cells.begin(); it != cells.end(); ++it) {
            columns.append(quoted(it.key()));
            placeholders.append("?");
            values.append(it.value());
        }
        return {"INSERT INTO " + quoted(table) + "(" + columns.join(',') + ") VALUES(" +
                    placeholders.join(',') + ")",
                values};
    }
    void apply(const QList<SqlStatement> &steps) {
        SqliteWriter writer;
        require(writer.open(path, ample()) == StorageError::none,
                "independent committed fixture unit");
        ok(run(writer, [&](auto context) -> QVariant {
            auto s = context.begin();
            for (const auto &step : steps)
                (void)s.execute(step);
            s.commit();
            return 1;
        }));
    }
    void seed(const QStringList &names) {
        QList<SqlStatement> steps;
        for (const auto &name : names)
            for (const auto &e : literals.root["scenarios"].toObject()[name].toArray())
                steps.append(statement(e));
        apply(steps);
    }
    void insert_row(const QString &table, const Cells &row) { apply({insert(table, row)}); }
    void empty_states() {
        apply({{"DELETE FROM subathon_app_head", {}},
               {"DELETE FROM subathon_app_balances", {}},
               {"DELETE FROM subathon_app_states", {}}});
    }
    void replace_requests(const std::string &plan, const std::vector<RequestedEffect> &requests) {
        QList<SqlStatement> steps{
            {"DELETE FROM subathon_plan_requests WHERE plan_ref=?", {blob(plan)}}};
        qint64 ordinal = 0;
        for (const auto &r : requests) {
            ++ordinal;
            Cells row{{"plan_ref", blob(plan)},
                      {"revision", id_blob(1)},
                      {"ordinal", ordinal},
                      {"action_ref", blob(r.action_ref)},
                      {"action_version_ref", blob(r.action_version_ref)},
                      {"rule_ref", blob(r.rule_ref)},
                      {"target_ref", blob(r.target_ref)},
                      {"unit", int(r.unit)},
                      {"requires_historical_valuation", int(r.requires_historical_valuation)},
                      {"exact_numerator", blob(numerator(r.exact_requested_units).str())},
                      {"exact_denominator", blob(denominator(r.exact_requested_units).str())},
                      {"requested_units", qint64(r.requested_units)},
                      {"calculation_explanation", blob(r.calculation_explanation)},
                      {"reversal_behavior_ref", blob(r.reversal_behavior_ref)}};
            steps.append(insert("subathon_plan_requests", row));
        }
        apply(steps);
    }
    void set_initial(const ApplicationStateSnapshot &value) {
        QList<SqlStatement> steps{
            {"UPDATE subathon_app_states SET "
             "main_remaining_ms=?,time_debt_ms=?,lifecycle=?,phase=?,grace_remaining_ms=?,awaiting_"
             "confirmation=?,document=?,format_ref=?,configuration_ref=? WHERE token=X'7374617274'",
             {qint64(value.timer.main_remaining_ms), qint64(value.time_debt_ms),
              int(value.timer.lifecycle), int(value.timer.phase),
              qint64(value.timer.grace_remaining_ms), int(value.timer.awaiting_confirmation),
              value.envelope.document, blob(value.envelope.format_ref),
              blob(value.configuration_ref)}}};
        for (const auto &b : value.balances)
            steps.append({"UPDATE subathon_app_balances SET units=? WHERE token=X'7374617274' AND "
                          "balance_kind=? AND target_ref=?",
                          {qint64(b.units), int(b.kind), blob(b.target_ref)}});
        apply(steps);
    }
    void add_state(ApplicationStateSnapshot value) {
        Cells row{{"token", blob(value.token)},
                  {"format_ref", blob(value.envelope.format_ref)},
                  {"document", value.envelope.document},
                  {"configuration_ref", blob(value.configuration_ref)},
                  {"lifecycle", int(value.timer.lifecycle)},
                  {"phase", int(value.timer.phase)},
                  {"main_remaining_ms", qint64(value.timer.main_remaining_ms)},
                  {"grace_remaining_ms", qint64(value.timer.grace_remaining_ms)},
                  {"awaiting_confirmation", int(value.timer.awaiting_confirmation)},
                  {"time_debt_ms", qint64(value.time_debt_ms)},
                  {"balance_count", qint64(value.balances.size())}};
        QList<SqlStatement> steps{insert("subathon_app_states", row)};
        for (const auto &b : value.balances)
            steps.append(insert("subathon_app_balances", {{"token", blob(value.token)},
                                                          {"balance_kind", int(b.kind)},
                                                          {"target_ref", blob(b.target_ref)},
                                                          {"units", qint64(b.units)}}));
        apply(steps);
    }
    void append_plan_metadata(std::int64_t through) {
        QList<SqlStatement> steps;
        auto r = literals.one("subathon_plan_revisions", {{"plan_ref", blob("plan-P")}});
        for (std::int64_t k = 2; k <= through; ++k) {
            r["revision"] = id_blob(std::uint64_t(k));
            r["parent_revision"] = id_blob(std::uint64_t(k - 1));
            r["override_authority"] = 2;
            r["override_actor_ref"] = blob("actor");
            r["override_reason"] = blob("explicit override");
            r["override_decision_ref"] = blob("decision");
            steps.append(insert("subathon_plan_revisions", r));
        }
        steps.append({"UPDATE subathon_plan_headers SET head_revision=? WHERE plan_ref=?",
                      {id_blob(std::uint64_t(through)), blob("plan-P")}});
        apply(steps);
    }
    void add_jobs(std::int64_t through) {
        QList<SqlStatement> steps;
        auto r = literals.one("subathon_app_jobs",
                              {{"operation_ref", blob("original")}, {"ordinal", qint64(1)}});
        for (std::int64_t k = 3; k <= through; ++k) {
            r["ordinal"] = qint64(k);
            r["job_ref"] = blob("job-" + std::to_string(k));
            r["group_ref"] = {};
            r["group_ordinal"] = {};
            steps.append(insert("subathon_app_jobs", r));
        }
        steps.append({"UPDATE subathon_app_results SET job_count=? WHERE operation_ref=?",
                      {qint64(through), blob("original")}});
        apply(steps);
    }
    // Independent historical fixture: zero-correction sources commit BEFORE their
    // complete raw bundles. Positions are actual contiguous rows, not mocked MAX.
    void extend_zero_progress(std::int64_t through) {
        QList<SqlStatement> sources, applications;
        for (std::int64_t position = 3; position <= through; ++position) {
            const auto suffix = std::to_string(position);
            const auto receipt_id = std::uint64_t(1000 + position);
            const auto operation = "progress-" + suffix, plan = "progress-plan-" + suffix,
                       after = "progress-state-" + suffix;
            const auto before = position == 3 ? std::string("refund2-after")
                                              : "progress-state-" + std::to_string(position - 1);
            auto receipt = literals.one("subathon_receipts", {{"receipt_id", id_blob(9)}});
            receipt["receipt_id"] = id_blob(receipt_id);
            receipt["opaque_id"] = blob("zero-correction-" + suffix);
            receipt["event_type"] = blob("synthetic.zero-correction");
            receipt["money_original_decimal"] = blob("0.0000");
            sources.append(insert("subathon_receipts", receipt));
            auto attempt = literals.one("subathon_receipt_attempts", {{"receipt_id", id_blob(9)}});
            for (auto it = attempt.begin(); it != attempt.end(); ++it)
                if (receipt.contains(it.key()))
                    it.value() = receipt[it.key()];
            sources.append(insert("subathon_receipt_attempts", attempt));
            auto observation =
                literals.one("subathon_contribution_observations", {{"receipt_id", id_blob(9)}});
            observation["receipt_id"] = id_blob(receipt_id);
            sources.append(insert("subathon_contribution_observations", observation));
            auto header = literals.one("subathon_plan_headers", {{"plan_ref", blob("plan-R2")}});
            header["plan_ref"] = blob(plan);
            header["receipt_id"] = id_blob(receipt_id);
            header["event_ref"] = blob("event-" + operation);
            sources.append(insert("subathon_plan_headers", header));
            auto revision =
                literals.one("subathon_plan_revisions", {{"plan_ref", blob("plan-R2")}});
            revision["plan_ref"] = blob(plan);
            revision["historical_original_decimal"] = blob("0.0000");
            sources.append(insert("subathon_plan_revisions", revision));
            for (const auto *table :
                 {"subathon_plan_requests", "subathon_plan_rules", "subathon_plan_modifiers"})
                for (auto row : literals.all(table, {{"plan_ref", blob("plan-R2")}})) {
                    row["plan_ref"] = blob(plan);
                    if (QString(table) == "subathon_plan_requests") {
                        row["exact_numerator"] = blob("0");
                        row["requested_units"] = qint64(0);
                    }
                    sources.append(insert(table, row));
                }
            auto state = literals.one("subathon_app_states", {{"token", blob("refund2-after")}});
            state["token"] = blob(after);
            applications.append(insert("subathon_app_states", state));
            for (auto balance :
                 literals.all("subathon_app_balances", {{"token", blob("refund2-after")}})) {
                balance["token"] = blob(after);
                applications.append(insert("subathon_app_balances", balance));
            }
            auto op = literals.one("subathon_app_operations", {{"operation_ref", blob("refund2")}});
            op["operation_ref"] = blob(operation);
            op["receipt_id"] = id_blob(receipt_id);
            op["plan_ref"] = blob(plan);
            op["head_token"] = blob(before);
            op["before_token"] = blob(before);
            op["after_token"] = blob(after);
            op["reversal_ref"] = blob("zero-correction-" + suffix);
            applications.append(insert("subathon_app_operations", op));
            auto current =
                literals.one("subathon_app_contributions", {{"operation_ref", blob("refund2")}});
            current["operation_ref"] = blob(operation);
            applications.append(insert("subathon_app_contributions", current));
            auto effect =
                literals.one("subathon_app_effects", {{"operation_ref", blob("refund2")}});
            effect["operation_ref"] = blob(operation);
            effect["plan_ref"] = blob(plan);
            effect["exact_numerator"] = blob("0");
            effect["requested_units"] = qint64(0);
            applications.append(insert("subathon_app_effects", effect));
            for (auto component :
                 literals.all("subathon_app_components", {{"operation_ref", blob("refund2")}})) {
                component["operation_ref"] = blob(operation);
                component["before_token"] = blob(before);
                component["units"] = qint64(0);
                applications.append(insert("subathon_app_components", component));
            }
            auto link =
                literals.one("subathon_app_reversals", {{"operation_ref", blob("refund2")},
                                                        {"original_request_ordinal", qint64(1)}});
            link["operation_ref"] = blob(operation);
            link["basis_position"] = qint64(position);
            link["previous_cumulative"] = qint64(100);
            link["cumulative"] = qint64(100);
            link["requested_increment"] = qint64(0);
            applications.append(insert("subathon_app_reversals", link));
            applications.append(
                {"UPDATE subathon_app_reversal_heads SET basis_position=?,last_operation=? WHERE "
                 "original_request_ordinal=1 AND original_component_kind=0",
                 {qint64(position), blob(operation)}});
            auto result =
                literals.one("subathon_app_results", {{"operation_ref", blob("refund2")}});
            result["operation_ref"] = blob(operation);
            result["reversal_count"] = qint64(1);
            applications.append(insert("subathon_app_results", result));
            auto audit = literals.one("subathon_app_audit", {{"operation_ref", blob("refund2")}});
            audit["operation_ref"] = blob(operation);
            applications.append(insert("subathon_app_audit", audit));
            applications.append(
                {"UPDATE subathon_app_head SET token=? WHERE singleton=1 AND token=?",
                 {blob(after), blob(before)}});
        }
        apply(sources);
        apply(applications);
    }
    void clone_plan(const std::string &plan, std::uint64_t receipt_id,
                    std::optional<std::uint64_t> cid, const std::vector<RequestedEffect> &requests,
                    bool non_support = false) {
        auto h = literals.one("subathon_plan_headers", {{"plan_ref", blob("plan-P")}});
        h["plan_ref"] = blob(plan);
        h["receipt_id"] = id_blob(receipt_id);
        h["contribution_id"] = cid ? QVariant(id_blob(*cid)) : QVariant{};
        h["event_ref"] = blob("event-" + plan);
        auto r = literals.one("subathon_plan_revisions", {{"plan_ref", blob("plan-P")}});
        r["plan_ref"] = blob(plan);
        if (!non_support) {
            const auto receipt = literals.receipt(receipt_id);
            require(receipt.facts.money.has_value(), "synthetic selected event money");
            r["historical_original_decimal"] = blob(receipt.facts.money->original_decimal);
            r["historical_currency"] = blob(receipt.facts.money->currency);
        }
        if (non_support) {
            r["payment_evidence"] = 3;
            r["monetary"] = 0;
            r["original_decimal"] = {};
            r["currency"] = {};
            r["resolution_ref"] = {};
            r["missing_rate"] = 0;
            r["review"] = 1;
            r["evidence_policy"] = 2;
            r["decision_choice"] = {};
            r["decision_ref"] = {};
            r["decision_actor_ref"] = {};
            r["valuation_state"] = 0;
            for (const auto *field :
                 {"historical_resolution_ref", "historical_original_decimal", "historical_currency",
                  "comparison_currency", "effective_rate_decimal", "fetched_rate_decimal",
                  "snapshot_ref", "rate_source_ref", "rate_timestamp_utc_ms",
                  "fetch_timestamp_utc_ms", "manual_decision_ref"})
                r[field] = {};
        }
        QList<SqlStatement> steps{insert("subathon_plan_headers", h),
                                  insert("subathon_plan_revisions", r)};
        for (const auto *table : {"subathon_plan_rules", "subathon_plan_modifiers"})
            for (auto row : literals.all(table, {{"plan_ref", blob("plan-P")}})) {
                row["plan_ref"] = blob(plan);
                steps.append(insert(table, row));
            }
        apply(steps);
        replace_requests(plan, requests);
    }
    void non_support_source() {
        auto r = literals.one("subathon_receipts", {{"receipt_id", id_blob(7)}});
        r["receipt_id"] = id_blob(12);
        r["opaque_id"] = blob("explicit-non-support");
        r["event_type"] = blob("synthetic.non-support");
        for (const auto *field : {"money_original_decimal", "money_currency", "payment_namespace",
                                  "payment_opaque_id", "activity_namespace", "activity_opaque_id"})
            r[field] = {};
        auto a = literals.one("subathon_receipt_attempts", {{"receipt_id", id_blob(7)}});
        a["receipt_id"] = id_blob(12);
        for (auto it = a.begin(); it != a.end(); ++it)
            if (r.contains(it.key()))
                it.value() = r[it.key()];
        apply({insert("subathon_receipts", r), insert("subathon_receipt_attempts", a)});
        clone_plan("non-support", 12, std::nullopt, {}, true);
    }

  private:
    QString name_;
    QSqlDatabase db_;
};
ApplicationOriginalComponent main_basis(std::int64_t ordinal = 1) {
    return {{"scope", "original"}, ordinal, CK::main_delta};
}
ApplicationOperationKey key(const std::string &name = "original") { return {"scope", name}; }
Run stage(SqliteWriter &w, const PreparedApplicationBundle &input, SqliteBundleLimits limits = {}) {
    return run(w, [&](auto context) -> QVariant {
        auto s = context.begin();
        SqliteApplicationBundleStore store(s, limits);
        const auto r = store.stage(input);
        require(r.disposition == SD::staged && !r.existing, "new complete staged bundle");
        s.commit();
        return 1;
    });
}
Run lookup(SqliteWriter &w, const ApplicationOperationKey &k,
           std::optional<RecordedApplicationBundle> &output, SqliteBundleLimits limits = {}) {
    output.reset();
    return run(w, [&](auto context) -> QVariant {
        auto s = context.begin();
        SqliteApplicationBundleStore store(s, limits);
        output = store.lookup(k);
        s.commit();
        return 1;
    });
}
Run refusal(SqliteWriter &w, Fixture &f,
            const std::function<void(SqliteApplicationBundleStore &)> &body, SR reason,
            bool &caught, SqliteBundleLimits limits = {}) {
    caught = false;
    return run(w, [&](auto context) -> QVariant {
        auto s = context.begin();
        SqliteApplicationBundleStore store(s, limits);
        (void)s.execute({"INSERT INTO subathon_retained_configurations "
                         "VALUES(X'7072696F722D63616C6C6572',X'79')",
                         {}});
        require(s.execute({"SELECT count(*) FROM subathon_retained_configurations WHERE "
                           "reference=X'7072696F722D63616C6C6572'",
                           {}})[0][0]
                        .toLongLong() == 1,
                "observed earlier caller write");
        try {
            body(store);
        } catch (const SqliteBundleStorageFailure &e) {
            require(e.reason() == reason, "exact selected error reason");
            require(f.released(), "real independent BEGIN inside catch while facade lives");
            caught = true;
            throw;
        }
        throw std::runtime_error("required selected storage failure missing");
        return 0;
    });
}
void disposition(const Run &r, SD expected, const ApplicationBundleStageResult &actual) {
    ok(r);
    require(actual.disposition == expected, "exact no-own-write disposition");
}
RequestedEffect request_like(const RequestedEffect &base, std::int64_t units,
                             RequestedUnit unit = RequestedUnit::milliseconds, Exact value = 0) {
    auto r = base;
    r.unit = unit;
    r.target_ref = unit == RequestedUnit::milliseconds   ? "timer"
                   : unit == RequestedUnit::micro_points ? "score"
                                                         : "count";
    r.requires_historical_valuation = false;
    r.exact_requested_units = value == 0 ? Exact(units) : value;
    r.requested_units = units;
    return r;
}
PreparedApplicationBundle single(const Literals &l, const RequestedEffect &r,
                                 std::int64_t main_before, std::int64_t debt_before,
                                 std::int64_t main_after, std::int64_t debt_after, AR role,
                                 const std::vector<ApplicationComponent> &components) {
    auto p = l.prepared();
    p.advanced_before = l.state("start");
    p.advanced_before.timer.main_remaining_ms = main_before;
    p.advanced_before.time_debt_ms = debt_before;
    p.after = p.advanced_before;
    p.after.token = "single-after";
    p.after.timer.main_remaining_ms = main_after;
    p.after.time_debt_ms = debt_after;
    p.advance_explanation.clear();
    p.effects = {ApplicationEffect{1,
                                   role,
                                   r.unit,
                                   r.exact_requested_units,
                                   r.requested_units,
                                   r.exact_requested_units - r.requested_units,
                                   {},
                                   {},
                                   {},
                                   "independent supplied outcome",
                                   components}};
    p.jobs.clear();
    p.reversals.clear();
    return p;
}
} // namespace

class SqliteApplicationBundleTests final : public QObject {
    Q_OBJECT
  private:
    std::unique_ptr<Literals> l_;
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<Fixture> f_;
    std::unique_ptr<Fixture> another(const QString &name) {
        return std::make_unique<Fixture>(directory_->filePath(name + ".sqlite"), *l_);
    }
    static void open(SqliteWriter &w, const Fixture &f, StorageLimits limits = ample()) {
        require(w.open(f.path, limits) == StorageError::none, "test controlled writer open");
    }
    Run domain(SqliteWriter &w, const PreparedApplicationBundle &p,
               ApplicationBundleStageResult &result, bool add_marker = false) {
        return run(w, [&](auto context) -> QVariant {
            auto s = context.begin();
            SqliteApplicationBundleStore store(s);
            if (add_marker)
                (void)s.execute({"INSERT INTO subathon_retained_configurations "
                                 "VALUES(X'646F6D61696E2D7072696F72',X'79')",
                                 {}});
            result = store.stage(p);
            require(s.execute({"SELECT count(*) FROM subathon_app_head", {}})[0][0].toLongLong() ==
                        1,
                    "healthy no-own-write session remains usable");
            s.commit();
            return 1;
        });
    }
  private slots:
    void initTestCase() {
        l_ = std::make_unique<Literals>();
        require(l_->root["manifest"].toObject()["sha256_hex"].toString() ==
                    "c9ba98fa4fe2a42cc6ed9776baa011fc2b93cf879e7374b53e9a6128f7d7afbb",
                "frozen independent step5 identity");
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }
    void init() {
        directory_ = std::make_unique<QTemporaryDir>();
        QVERIFY(directory_->isValid());
        f_ = another("primary");
    }
    void cleanup() {
        f_.reset();
        directory_.reset();
    }

    void manifestMatchesFrozenStepFiveAndPrefixes() {
        scenario([&] {
            const auto manifest = application_bundle_repository_manifest();
            require(manifest.catalog_format == 1 &&
                        manifest.store_identity == "subathon.sqlite-store" &&
                        manifest.supported_starting_versions == QList<qint32>({0, 1, 2, 3, 4, 5}) &&
                        manifest.steps.size() == 5,
                    "exact manifest/version support");
            for (qsizetype index = 0; index < 5; ++index) {
                const auto expected = l_->manifests[index];
                const auto &actual = manifest.steps[index];
                require(actual.version == index + 1 && actual.id == expected["id"].toString(),
                        "immutable prefix migration identity");
                QList<QString> sql;
                for (const auto &v : expected["sql"].toArray())
                    sql.append(v.toString());
                require(actual.sql == sql, "all exact reviewed DDL literals");
                QList<CatalogObject> objects;
                for (const auto &e : expected["expected_schema"].toArray()) {
                    const auto o = e.toObject();
                    objects.append(
                        {o["type"].toString(), o["name"].toString(), o["table_name"].toString(),
                         o["sql"].isNull() ? std::nullopt : std::optional(o["sql"].toString())});
                }
                require(actual.expected_schema == objects,
                        "manual expected full catalog including NULL indexes");
                require(canonical_migration_payload(actual) ==
                            QByteArray::fromHex(expected["payload_hex"].toString().toLatin1()),
                        "independently frozen complete framed bytes");
            }
        });
    }
    void initialStateAndEmptyAbsenceUseRealCallerUnit() {
        scenario([&] {
            f_->empty_states();
            SqliteWriter w;
            open(w, *f_);
            const auto first = l_->state("start");
            const auto before = f_->snapshot();
            auto r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                require(!store.read_state(), "empty means absent, no invented state");
                require(store.retain_initial_state(first) ==
                            SqliteBundleInitialDisposition::inserted,
                        "staged initial");
                auto found = store.read_state();
                require(found && same(*found, first), "own staged complete initial");
                {
                    auto copy = store;
                    require(copy.retain_initial_state(first) ==
                                SqliteBundleInitialDisposition::already_present,
                            "exact initial duplicate");
                }
                auto changed = first;
                changed.envelope.document.append('x');
                require(store.retain_initial_state(changed) ==
                            SqliteBundleInitialDisposition::conflict,
                        "same token cannot rebind");
                require(f_->scalar("SELECT count(*) FROM subathon_app_states").toLongLong() == 0,
                        "outside still empty");
                s.rollback();
                return 1;
            });
            clean_rollback(r);
            require(f_->snapshot() == before, "inert facade did not commit initial");
            r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                require(store.retain_initial_state(first) ==
                            SqliteBundleInitialDisposition::inserted,
                        "known rollback permits explicit new unit");
                s.commit();
                return 1;
            });
            ok(r);
            require(f_->scalar("SELECT count(*) FROM subathon_app_states").toLongLong() == 1,
                    "initial whole unit durable");
        });
    }
    void readInputsResolvesCurrentAndHistoricalProvenance() {
        scenario([&] {
            SqliteWriter w;
            open(w, *f_);
            ApplicationBundleInputResult out;
            auto r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                out = store.read_inputs({"plan-P", 1}, ApplicationSubjectKind::contribution);
                s.commit();
                return 1;
            });
            ok(r);
            require(out.disposition == SqliteBundleInputDisposition::ready && out.inputs,
                    "owned input result presence");
            const auto &v = *out.inputs;
            require(v.subject_kind == ApplicationSubjectKind::contribution &&
                        same(v.plan, {"plan-P", 1}) && v.actual_plan_head == 1 &&
                        v.selected_plan == l_->capture("plan-P") &&
                        same(v.current_state, l_->state("start")),
                    "full selected event/quote/ordered captures/current snapshot");
            require(v.current_contribution &&
                        v.current_contribution->contribution.id == ContributionId{0},
                    "present CID0");
            const auto &c = v.current_contribution->contribution;
            const auto source = l_->receipt(1);
            require(c.original_receipt_id == ReceiptId{1} && c.original_facts == source.facts &&
                        c.original_arrival == source.arrival &&
                        c.state == *l_->prepared().intent.expected_contribution_state,
                    "CURRENT dimensions and immutable support source");
            const auto &a = v.current_contribution->support_anchor;
            require(a.contribution_id == ContributionId{0} && a.observation.receipt == source &&
                        a.observation.kind == ContributionObservationKind::support &&
                        a.observation.evidence == PaymentEvidence::unknown &&
                        a.observation.support_conditions ==
                            l_->capture("plan-P").captured.contribution_state.conditions,
                    "bounded support anchor, no growing observation history");
            const auto current_row =
                l_->one("subathon_contributions", {{"contribution_id", id_blob(0)}});
            const VerifiedActivityIdentity expected_activity{
                {bytes(current_row["activity_provider"]),
                 bytes(current_row["activity_source_account"])},
                bytes(current_row["activity_namespace"]),
                bytes(current_row["activity_id"])};
            const ContributionRecord complete_current{
                ContributionId{0}, expected_activity,
                ReceiptId{1},      source.facts,
                source.arrival,    *l_->prepared().intent.expected_contribution_state};
            require(c == complete_current,
                    "all current owned identity/activity/source/state members");
            const auto anchor_row =
                l_->one("subathon_contribution_observations", {{"receipt_id", id_blob(1)}});
            const VerifiedActivityLink expected_link{
                {{bytes(anchor_row["activity_provider"]),
                  bytes(anchor_row["activity_source_account"])},
                 bytes(anchor_row["activity_namespace"]),
                 bytes(anchor_row["activity_id"])},
                bytes(anchor_row["verification_ref"])};
            const ContributionEvidenceRecord complete_anchor{
                {source, ContributionObservationKind::support,
                 PaymentEvidence(anchor_row["evidence"].toInt()), expected_link,
                 Literals::contribution(anchor_row, "support_").conditions},
                ContributionId{0}};
            require(a == complete_anchor, "all trusted support activity+verification provenance "
                                          "retained, not provider proof");
            require(v.selected_plan.captured.event.receipt_id == ReceiptId{7} &&
                        c.original_receipt_id != v.selected_plan.captured.event.receipt_id,
                    "confirmation event is not required to equal support");
            require(v.selected_plan.captured.valuation.historical->effective_rate_decimal ==
                            "1.25000" &&
                        v.selected_plan.captured.valuation.historical->fetched_rate_decimal ==
                            "1.249900" &&
                        v.selected_plan.captured.valuation.historical->source_ref == std::string(),
                    "independent historical spellings/present empty provenance");
            r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                const auto missing =
                    store.read_inputs({"absent", 1}, ApplicationSubjectKind::contribution);
                require(missing.disposition == SqliteBundleInputDisposition::missing_plan &&
                            !missing.inputs,
                        "missing is explicit optional data");
                require(!store.lookup(key("absent")),
                        "missing operation does not fabricate result");
                s.commit();
                return 1;
            });
            ok(r);
        });
    }
    void stateAndRecordedValuesOwnBytesAfterReopen() {
        scenario([&] {
            f_->seed({"original_prepare", "original_finish"});
            std::optional<RecordedApplicationBundle> out;
            std::optional<ApplicationStateSnapshot> state;
            {
                SqliteWriter w;
                open(w, *f_);
                auto r = run(w, [&](auto context) -> QVariant {
                    auto s = context.begin();
                    SqliteApplicationBundleStore store(s);
                    out = store.lookup(key());
                    state = store.read_state();
                    s.commit();
                    return 1;
                });
                ok(r);
            }
            require(out && state && same(*state, l_->state("original-after")),
                    "owned current state after original file close");
            matches(*out, l_->prepared(), l_->state("start"));
            require(f_->exec("UPDATE subathon_app_head SET token=X'7374617274'"),
                    "later current head change fixture");
            require(f_->exec("UPDATE subathon_contributions SET review=3,payment_evidence=1"),
                    "later current dimensions differ");
            {
                SqliteWriter reopened;
                open(reopened, *f_);
                std::optional<RecordedApplicationBundle> historical;
                ok(lookup(reopened, key(), historical));
                require(historical.has_value(),
                        "historical lookup survives later head/current changes");
                matches(*historical, l_->prepared(), l_->state("start"));
            }
            require(f_->exec("DELETE FROM subathon_app_jobs"), "owned-result lifetime probe");
            matches(*out, l_->prepared(), l_->state("start"));
            require(same(*state, l_->state("original-after")),
                    "read bytes owned independently of later source edits");
        });
    }
    void explicitReceiptSubjectAndContributionZeroStayDistinct() {
        scenario([&] {
            const auto maximum = std::numeric_limits<std::uint64_t>::max();
            const auto high_source = l_->receipt(maximum);
            f_->apply({{"UPDATE subathon_contributions SET original_decimal=?,currency=? WHERE "
                        "contribution_id=?",
                        {blob(high_source.facts.money->original_decimal),
                         blob(high_source.facts.money->currency), id_blob(maximum)}},
                       {"UPDATE subathon_contribution_observations SET "
                        "support_original_decimal=?,support_currency=? WHERE receipt_id=?",
                        {blob(high_source.facts.money->original_decimal),
                         blob(high_source.facts.money->currency), id_blob(maximum)}}});
            f_->clone_plan("healthy-high", maximum, maximum,
                           l_->capture("plan-P").captured.requested_effects);
            f_->non_support_source();
            auto p = l_->prepared();
            p.intent.operation.operation_ref = "high";
            p.intent.receipt_id = {maximum};
            p.intent.contribution_id = ContributionId{maximum};
            p.intent.plan = {"healthy-high", 1};
            p.intent.expected_contribution_state = Literals::contribution(
                l_->one("subathon_contributions", {{"contribution_id", id_blob(maximum)}}));
            p.intent.expected_contribution_state->conditions.valuation.original_money =
                *high_source.facts.money;
            p.advanced_before.token = "high-before";
            p.after.token = "high-after";
            SqliteWriter w;
            open(w, *f_);
            ok(stage(w, p));
            std::optional<RecordedApplicationBundle> out;
            ok(lookup(w, key("high"), out));
            require(out && out->intent.receipt_id == ReceiptId{maximum} &&
                        out->intent.contribution_id == ContributionId{maximum} &&
                        out->intent.plan.revision == 1,
                    "healthy revision1 with actual full unsigned source IDs");
            require(f_->rows("SELECT "
                             "typeof(receipt_id),hex(receipt_id),typeof(contribution_id),hex("
                             "contribution_id),hex(plan_revision) FROM subathon_app_operations "
                             "WHERE operation_ref=X'68696768'") ==
                        SqlRows({{"blob", "FFFFFFFFFFFFFFFF", "blob", "FFFFFFFFFFFFFFFF",
                                  "0000000000000001"}}),
                    "actual FK field classes and bytes");
            matches(*out, p, l_->state("start"));
            require(
                f_->rows("SELECT hex(contribution_id),application FROM subathon_contributions "
                         "ORDER BY contribution_id") ==
                    SqlRows({{"0000000000000000", qint64(0)}, {"FFFFFFFFFFFFFFFF", qint64(1)}}),
                "full-range application patch addresses actualCIDMAX and leaves CID0 untouched");
            auto non = p;
            non.intent.operation.operation_ref = "non-support";
            non.intent.subject_kind = ApplicationSubjectKind::receipt;
            non.intent.receipt_id = {12};
            non.intent.contribution_id.reset();
            non.intent.expected_contribution_state.reset();
            non.intent.plan = {"non-support", 1};
            non.intent.expected_state_token = p.after.token;
            non.advanced_before = p.after;
            non.after = non.advanced_before;
            non.after.token = "non-support-after";
            non.advance_explanation.clear();
            non.effects.clear();
            non.jobs.clear();
            ok(stage(w, non));
            out.reset();
            ok(lookup(w, key("non-support"), out));
            require(out && !out->intent.contribution_id && !out->contribution &&
                        out->intent.subject_kind == ApplicationSubjectKind::receipt,
                    "explicit non-support receipt subject retains SQLNULL");
            require(f_->rows("SELECT subject_kind,hex(subject_id) FROM subathon_app_claims ORDER "
                             "BY subject_kind") ==
                        SqlRows({{0, "FFFFFFFFFFFFFFFF"}, {1, "000000000000000C"}}),
                    "tagged namespaces remain distinct");
        });
    }
    void originalStageCommitsCompleteBundleAndPatchesOnlyApplication() {
        scenario([&] {
            const auto prefix = f_->snapshot(true);
            const auto schema_before =
                f_->rows("SELECT type,name,tbl_name,sql FROM sqlite_schema ORDER BY type,name");
            const auto expected = l_->prepared();
            SqliteWriter w;
            open(w, *f_);
            ok(stage(w, expected));
            std::optional<RecordedApplicationBundle> out;
            ok(lookup(w, key(), out));
            require(out.has_value(), "complete result retrievable");
            matches(*out, expected, l_->state("start"));
            require(f_->rows("SELECT typeof(application),application FROM subathon_contributions "
                             "WHERE contribution_id=X'0000000000000000'") ==
                        SqlRows({{"integer", qint64(1)}}),
                    "actual original status INTEGER1 patched");
            const auto preservation = l_->root["queries"].toObject()["preservation"].toObject();
            const auto actual = f_->rows(preservation["sql"].toString());
            const auto wanted = preservation["expected"].toArray();
            require(actual.size() == 1, "preserved current row");
            for (qsizetype column = 0; column < wanted[0].toArray().size(); ++column)
                require(actual[0][column] == literal_value(wanted[0].toArray()[column]),
                        "every non-application current field preserved");
            for (const auto &pair :
                 std::array<std::pair<const char *, qint64>, 7>{{{"subathon_app_operations", 1},
                                                                 {"subathon_app_claims", 1},
                                                                 {"subathon_app_effects", 4},
                                                                 {"subathon_app_components", 8},
                                                                 {"subathon_app_jobs", 2},
                                                                 {"subathon_app_results", 1},
                                                                 {"subathon_app_audit", 1}}})
                require(f_->scalar("SELECT count(*) FROM " + QString(pair.first)).toLongLong() ==
                            pair.second,
                        "exact one whole bundle cardinality");
            // The sole permitted prefix mutation is application on CID0; compare all other
            // source tables through their immutable explicit projections above and below.
            require(
                f_->rows(
                    "SELECT hex(original_receipt_id),payment_evidence,review,reversal,settlement "
                    "FROM subathon_contributions WHERE contribution_id=X'0000000000000000'") ==
                    SqlRows({{"0000000000000001", 2, 2, 0, 1}}),
                "source/evidence/review/reversal/settlement stay independent");
            require(f_->snapshot(true) == prefix,
                    "ALL typed immutable prefix "
                    "schema/config/context/receipts/attempts/contributions/plans preserved except "
                    "independently asserted CID0 application patch");
            require(
                f_->rows("SELECT type,name,tbl_name,sql FROM sqlite_schema ORDER BY type,name") ==
                    schema_before,
                "stage has no migration/schema mutation authority");
        });
    }
    void capDebtAndDeficitOutcomesPreserveSuppliedConservation() {
        scenario([&] {
            int index = 0;
            for (const auto &entry : l_->root["arithmetic_vectors"].toArray()) {
                const auto v = entry.toObject();
                if (!v.contains("before_main"))
                    continue;
                auto f = another("arithmetic-" + QString::number(index++));
                const auto number = [&](const char *k) {
                    bool valid = false;
                    const auto n = v[k].toString().toLongLong(&valid);
                    require(valid, "independent signed literal");
                    return std::int64_t(n);
                };
                const auto r = request_like(l_->capture("plan-P").captured.requested_effects[0],
                                            number("request"));
                f->replace_requests("plan-P", {r});
                const bool addition = v.contains("cap_limited");
                std::vector<ApplicationComponent> c{
                    {CK::main_delta, number("main_delta"), {}},
                    {CK::debt_delta, number("debt_delta"), {}},
                    {addition ? CK::cap_limited_time : CK::unapplied_time,
                     number(addition ? "cap_limited" : "unapplied"),
                     {}}};
                auto p = single(*l_, r, number("before_main"), number("before_debt"),
                                number("after_main"), number("after_debt"),
                                addition ? AR::time_addition : AR::time_reversal, c);
                if (addition)
                    p.effects[0].maximum_main_ms = number("cap");
                else {
                    const auto name = v["id"].toString();
                    p.effects[0].deficit_policy =
                        name == "AT09-review" ? RefundDeficitPolicy::RequireReview
                        : name == "AT09-stop" ? RefundDeficitPolicy::StopAtZero
                                              : RefundDeficitPolicy::CarryTimeDebt;
                    p.effects[0].deficit_disposition = name == "AT09-review"
                                                           ? TimeReversalDisposition::RequiresReview
                                                           : TimeReversalDisposition::Applied;
                }
                f->set_initial(p.advanced_before);
                SqliteWriter w;
                open(w, *f);
                ok(stage(w, p));
                std::optional<RecordedApplicationBundle> out;
                ok(lookup(w, key(), out));
                require(out && same(out->effects[0], p.effects[0]) && same(out->after, p.after),
                        "literal exact signed partition/debt/Main/cap/held result retained; no "
                        "evaluator invoked");
            }
            require(index == 7, "all supplied cap/debt/deficit/MIN examples selected");
        });
    }
    void sameUnitReadDuplicateAndClaimRollbackOrCommit() {
        scenario([&] {
            for (bool commit : {false, true}) {
                auto f = another(commit ? "provisional-commit" : "provisional-abandon");
                const auto before = f->snapshot();
                const auto p = l_->prepared();
                SqliteWriter w;
                open(w, *f);
                auto r = run(w, [&](auto context) -> QVariant {
                    auto s = context.begin();
                    SqliteApplicationBundleStore store(s);
                    require(store.stage(p).disposition == SD::staged,
                            "own first staged application");
                    auto found = store.lookup(key());
                    require(found.has_value(), "own provisional lookup");
                    matches(*found, p, l_->state("start"));
                    auto state = store.read_state();
                    require(state && same(*state, p.after), "own provisional state");
                    const auto duplicate = store.stage(p);
                    require(duplicate.disposition == SD::already_present && duplicate.existing,
                            "own complete same-intent duplicate");
                    matches(*duplicate.existing, p, l_->state("start"));
                    auto other = p;
                    other.intent.operation.operation_ref = "op-B";
                    other.intent.expected_state_token = p.after.token;
                    other.intent.expected_contribution_state->conditions.application =
                        ContributionApplication::applied;
                    other.advanced_before = p.after;
                    other.after = other.advanced_before;
                    other.after.token = "unused-B";
                    other.after.timer.main_remaining_ms += 300;
                    other.after.balances[0].units += 100;
                    other.after.balances[1].units += 1;
                    other.advance_explanation.clear();
                    const auto claim = store.stage(other);
                    require(claim.disposition == SD::subject_claimed && claim.existing,
                            "fresh state-b reaches staged subject claim");
                    matches(*claim.existing, p, l_->state("start"));
                    require(s.execute({"SELECT count(*) FROM subathon_app_operations", {}})[0][0]
                                        .toLongLong() == 1 &&
                                s.execute({"SELECT count(*) FROM subathon_app_effects", {}})[0][0]
                                        .toLongLong() == 4,
                            "no duplicate/claim extra rows");
                    require(f->scalar("SELECT application FROM subathon_contributions WHERE "
                                      "contribution_id=X'0000000000000000'")
                                        .toInt() == 0 &&
                                f->scalar("SELECT count(*) FROM subathon_app_operations").toInt() ==
                                    0 &&
                                f->scalar("SELECT hex(token) FROM subathon_app_head").toString() ==
                                    "7374617274",
                            "outside retains earlier committed pending/state-a");
                    if (commit)
                        s.commit();
                    else
                        s.rollback();
                    return 1;
                });
                no_stub(r);
                if (commit) {
                    ok(r);
                    require(f->scalar("SELECT count(*) FROM subathon_app_operations").toInt() == 1,
                            "one application becomes durable only on actual commit");
                } else {
                    clean_rollback(r);
                    require(f->snapshot() == before,
                            "abandon removes all staged result/duplicate/claim evidence");
                }
            }
        });
    }
    void existingIntentComparisonIgnoresFreshComputedOutcome() {
        scenario([&] {
            f_->seed({"original_prepare", "original_finish"});
            auto p = l_->prepared();
            p.advanced_before.token = "new-observation";
            p.after.token = "new-computed";
            p.after.timer.main_remaining_ms = 0;
            for (size_t k = 0; k < 2; ++k) {
                p.effects[k].maximum_main_ms = 0;
                p.effects[k].components[0].units = 0;
                p.effects[k].components[2].units = p.effects[k].requested_units;
            }
            const auto before = f_->snapshot();
            SqliteWriter w;
            open(w, *f_);
            ApplicationBundleStageResult result;
            auto r = domain(w, p, result);
            disposition(r, SD::already_present, result);
            require(result.existing.has_value(),
                    "same complete Intent returns original immutable outcome");
            matches(*result.existing, l_->prepared(), l_->state("start"));
            require(f_->snapshot() == before,
                    "valid alternative computed outcome never overwrites/re-evaluates");
            p.intent.choices.document = QByteArray("changed explicit choice");
            r = domain(w, p, result);
            disposition(r, SD::operation_conflict, result);
            require(!result.existing && f_->snapshot() == before,
                    "different complete Intent conflicts before fresh outcome/targets");
        });
    }
    void operationSourceAndReversalIdentityConflictsDoNotWrite() {
        scenario([&] {
            f_->seed({"original_prepare", "original_finish", "reversal_one"});
            SqliteWriter w;
            open(w, *f_);
            const auto before = f_->snapshot();
            auto p = l_->prepared();
            p.intent.operation.operation_ref = "other-source";
            p.intent.subject_kind = ApplicationSubjectKind::receipt;
            p.intent.contribution_id.reset();
            p.intent.expected_contribution_state.reset();
            ApplicationBundleStageResult result;
            auto r = domain(w, p, result);
            disposition(r, SD::source_claimed, result);
            require(result.existing && same(result.existing->intent, l_->prepared().intent),
                    "same-kind actual receipt source cannot be awarded through another role/key");
            auto refund = l_->prepared("refund2");
            refund.intent.operation.operation_ref = "another-refund";
            refund.intent.reversal_ref = "refund-1";
            r = domain(w, refund, result);
            disposition(r, SD::reversal_identity_conflict, result);
            require(!result.existing && f_->snapshot() == before,
                    "same scoped financial identity refuses without write/alias/cleanup");
        });
    }
    void staleStateContributionPlanAndTokenRebindRefuse() {
        scenario([&] {
            const auto base = l_->prepared();
            int index = 0;
            const std::vector<std::pair<QString, QVariant>> changes{
                {"payment_evidence", 1},
                {"monetary", 0},
                {"original_decimal", blob("20.0001")},
                {"currency", blob("usd")},
                {"resolution_ref", blob("")},
                {"missing_rate", 1},
                {"review", 3},
                {"evidence_policy", 2},
                {"policy_ref", blob("new-policy")},
                {"decision_choice", 0},
                {"decision_ref", blob("")},
                {"decision_actor_ref", blob("new-actor")},
                {"application", 1},
                {"reversal", 1},
                {"settlement", 0}};
            for (const auto &change : changes) {
                auto f = another("dimension-" + QString::number(index++));
                f->apply({{"UPDATE subathon_contributions SET " + change.first +
                               "=? WHERE contribution_id=X'0000000000000000'",
                           {change.second}}});
                const auto before = f->snapshot();
                SqliteWriter w;
                open(w, *f);
                ApplicationBundleStageResult result;
                auto r = domain(w, base, result);
                disposition(r, SD::stale_contribution, result);
                require(!result.existing && f->snapshot() == before,
                        "ALL15 current cells compare independently with unchanged state token");
            }
            auto p = base;
            p.intent.expected_state_token = "old-token";
            p.intent.plan.plan_ref = "missing-new-target";
            SqliteWriter w;
            open(w, *f_);
            ApplicationBundleStageResult result;
            auto r = domain(w, p, result);
            disposition(r, SD::stale_state, result);
            require(!result.existing, "stale state before proposed plan lookup");
            auto f = another("plan-stale");
            f->append_plan_metadata(2);
            SqliteWriter plan_writer;
            open(plan_writer, *f);
            r = domain(plan_writer, base, result);
            disposition(r, SD::stale_plan, result);
            require(!result.existing, "healthy later head rejects old selected revision");
            auto token = another("old-token");
            auto retired = base.after;
            retired.token = "retired-token";
            token->add_state(retired);
            auto reused = base;
            reused.after = retired;
            SqliteWriter token_writer;
            open(token_writer, *token);
            r = domain(token_writer, reused, result);
            disposition(r, SD::state_token_conflict, result);
            require(!result.existing,
                    "previously present identical fresh-token payload still cannot reuse/rebind");
            auto applied = another("unavailable");
            applied->apply({{"UPDATE subathon_contributions SET application=1 WHERE "
                             "contribution_id=X'0000000000000000'",
                             {}}});
            auto unavailable = base;
            unavailable.intent.expected_contribution_state->conditions.application =
                ContributionApplication::applied;
            SqliteWriter applied_writer;
            open(applied_writer, *applied);
            r = domain(applied_writer, unavailable, result);
            disposition(r, SD::subject_unavailable, result);
            require(!result.existing, "old14 applied state without bundle claim does not fabricate "
                                      "an old bundle or award again");
        });
    }
    void clockAdvancedBeforeIsDistinctFromPersistedHead() {
        scenario([&] {
            const auto p = l_->prepared();
            require(p.intent.expected_state_token == "start" &&
                        p.advanced_before.token != "start" &&
                        p.after.token != p.advanced_before.token,
                    "three independent binding tokens");
            SqliteWriter w;
            open(w, *f_);
            ok(stage(w, p));
            std::optional<RecordedApplicationBundle> out;
            ok(lookup(w, key(), out));
            require(out && out->persisted_head.timer.main_remaining_ms == 400 &&
                        out->advanced_before.timer.main_remaining_ms == 0 &&
                        out->after.timer.main_remaining_ms == 300,
                    "clock advance is not accounting delta");
            require(out->effects[0].components[0].units == 100 &&
                        out->effects[1].components[0].units == 200 &&
                        out->advance_explanation == "synthetic elapsed400ms before application",
                    "supplied observation annotation and actual components survive separately");
            auto f = another("clock-source-mismatch");
            auto malformed = p;
            malformed.advanced_before.time_debt_ms = 1;
            malformed.after.time_debt_ms = 1;
            const auto before = f->snapshot();
            SqliteWriter bad_writer;
            open(bad_writer, *f);
            bool caught = false;
            auto failure = refusal(
                bad_writer, *f, [&](auto &store) { (void)store.stage(malformed); },
                SR::source_mismatch, caught);
            known(failure, SR::source_mismatch);
            require(caught && f->snapshot() == before,
                    "approved80f8 clock-only debt violation rolls whole caller unit back after "
                    "actual head comparison");
        });
    }

    void sourceAndSelectedClosureFailuresRollbackBeforeCatch() {
        scenario([&] {
            const std::vector<QString> faults{
                "UPDATE subathon_receipts SET configuration_ref=X'637478' WHERE "
                "receipt_id=X'0000000000000007'",
                "UPDATE subathon_retained_contexts SET configuration_ref=X'6D697373696E67' WHERE "
                "reference=X'637478'",
                "DELETE FROM subathon_contribution_observations WHERE "
                "receipt_id=X'0000000000000001'",
                "UPDATE subathon_contributions SET original_receipt_id=X'0000000000000063' WHERE "
                "contribution_id=X'0000000000000000'"};
            int index = 0;
            for (const auto &sql : faults) {
                auto f = another("source-" + QString::number(index++));
                require(f->exec(sql), "deliberate source fixture corruption");
                const auto before = f->snapshot();
                SqliteWriter w;
                open(w, *f);
                bool caught = false;
                auto r = refusal(
                    w, *f,
                    [](auto &store) {
                        (void)store.read_inputs({"plan-P", 1},
                                                ApplicationSubjectKind::contribution);
                    },
                    SR::invalid_stored_data, caught);
                known(r, SR::invalid_stored_data);
                require(caught && f->snapshot() == before,
                        "selected closure fails without repair and rolls earlier work back "
                        "synchronously");
            }
            SqliteWriter w;
            open(w, *f_);
            bool caught = false;
            const auto before = f_->snapshot();
            auto r = refusal(
                w, *f_,
                [](auto &store) {
                    (void)store.read_reversal_basis({{"scope", "missing"}, 1, CK::main_delta});
                },
                SR::missing_reference, caught);
            known(r, SR::missing_reference);
            require(caught && f_->snapshot() == before,
                    "missing actual original is not a zero-credit basis");
            auto wrong = l_->prepared();
            wrong.effects[2].unit = RequestedUnit::whole_count;
            wrong.effects[2].components[0] = {CK::whole_count, 100, std::string("count")};
            wrong.after.balances[0].units = 0;
            wrong.after.balances[1].units = 101;
            auto mismatch = another("copied-source-unit");
            SqliteWriter mismatch_writer;
            open(mismatch_writer, *mismatch);
            auto mismatch_result = refusal(
                mismatch_writer, *mismatch, [&](auto &store) { (void)store.stage(wrong); },
                SR::source_mismatch, caught);
            known(mismatch_result, SR::source_mismatch);
            require(caught,
                    "valid local direct math cannot rewrite actual selected request unit/target");
        });
    }
    void fullLookupRejectsMissingCountsGroupsAndCorruptSelectedBodies() {
        scenario([&] {
            const std::vector<QString> faults{
                "DELETE FROM subathon_app_results WHERE operation_ref=X'6F726967696E616C'",
                "DELETE FROM subathon_app_audit WHERE operation_ref=X'6F726967696E616C'",
                "DELETE FROM subathon_app_claims",
                "DELETE FROM subathon_app_contributions WHERE operation_ref=X'6F726967696E616C'",
                "DELETE FROM subathon_app_jobs WHERE ordinal=2",
                "DELETE FROM subathon_app_components WHERE request_ordinal=1 AND component_kind=5",
                "UPDATE subathon_app_results SET effect_count=5",
                "UPDATE subathon_app_states SET balance_count=3 WHERE token=X'7374617274'",
                "UPDATE subathon_app_jobs SET group_ordinal=3 WHERE ordinal=2",
                "UPDATE subathon_app_audit SET document='wrong-class'"};
            int index = 0;
            for (const auto &sql : faults) {
                auto f = another("closure-" + QString::number(index++));
                f->seed({"original_prepare", "original_finish"});
                require(f->exec("PRAGMA ignore_check_constraints=ON") && f->exec(sql),
                        "deliberate full selected closure fault");
                const auto before = f->snapshot();
                SqliteWriter w;
                open(w, *f);
                bool caught = false;
                auto r = refusal(
                    w, *f, [](auto &store) { (void)store.lookup(key()); }, SR::invalid_stored_data,
                    caught);
                known(r, SR::invalid_stored_data);
                require(caught && f->snapshot() == before,
                        "no incomplete result synthesis/replay/repair, exact snapshot preserved");
            }
            const auto original = l_->prepared();
            require(original.effects[2].request_ordinal == 3 &&
                        original.effects[2].exact_requested_units == 100 &&
                        original.effects[2].requested_units == 100,
                    "selected score request independently fixes canonical100/1");
            for (const auto &effect : original.effects)
                require(numerator(effect.exact_requested_units).str().size() <= 3 &&
                            denominator(effect.exact_requested_units).str().size() <= 3 &&
                            numerator(effect.rounding_residual).str().size() <= 3 &&
                            denominator(effect.rounding_residual).str().size() <= 3,
                        "all untouched literal selected effect Exact components fit three bytes");
            for (const auto &request : l_->capture("plan-P").captured.requested_effects)
                require(numerator(request.exact_requested_units).str().size() <= 3 &&
                            denominator(request.exact_requested_units).str().size() <= 3,
                        "all literal selected plan Exact components fit three bytes");
            struct ExactFault {
                QString name;
                QVariant numerator;
                QVariant denominator;
                SR reason;
            };
            const std::array<ExactFault, 3> exact_faults{
                {{"equivalent-noncanonical", blob("200"), blob("2"), SR::invalid_stored_data},
                 {"size-before-grammar", QByteArray::fromHex("FF000080"), blob("1"),
                  SR::result_limit},
                 {"class-before-size", QByteArray::fromHex("FF000080"), QString("1"),
                  SR::invalid_stored_data}}};
            for (const auto &fault : exact_faults) {
                auto f = another("selected-exact-" + fault.name);
                f->seed({"original_prepare", "original_finish"});
                require(f->exec("PRAGMA ignore_check_constraints=ON") &&
                            f->exec("UPDATE subathon_app_effects SET exact_numerator=?,"
                                    "exact_denominator=? WHERE operation_scope=? AND "
                                    "operation_ref=? AND request_ordinal=3",
                                    {fault.numerator, fault.denominator, blob("scope"),
                                     blob("original")}),
                        "feasible independent selected Exact corruption");
                const auto before = f->snapshot();
                SqliteWriter w;
                open(w, *f);
                SqliteBundleLimits limits;
                limits.component_bytes = 3;
                bool caught = false;
                auto r = refusal(
                    w, *f, [](auto &store) { (void)store.lookup(key()); }, fault.reason, caught,
                    limits);
                known(r, fault.reason);
                require(caught && f->snapshot() == before,
                        "both component classes then both sizes then canonical grammar/gcd; "
                        "equivalent value is never normalized and prior work rolls back");
            }
        });
    }
    void typedReferenceClassesNullsAndSparseMaxAreStrict() {
        scenario([&] {
            const std::vector<QVariant> ids{QString("00000007"), qint64(7), QByteArray(7, 'x'),
                                            QByteArray(9, 'x'), id_blob(0)};
            int index = 0;
            for (const auto &id : ids) {
                auto f = another("typed-" + QString::number(index++));
                require(f->exec("PRAGMA ignore_check_constraints=ON") &&
                            f->exec("UPDATE subathon_plan_headers SET receipt_id=? WHERE "
                                    "plan_ref=X'706C616E2D50'",
                                    {id}),
                        "wrong stored type/domain seed");
                const auto before = f->snapshot();
                SqliteWriter w;
                open(w, *f);
                bool caught = false;
                auto r = refusal(
                    w, *f,
                    [](auto &store) {
                        (void)store.read_inputs({"plan-P", 1},
                                                ApplicationSubjectKind::contribution);
                    },
                    SR::invalid_stored_data, caught);
                known(r, SR::invalid_stored_data);
                require(caught && f->snapshot() == before,
                        "no equivalent numeric TEXT/INTEGER/NULL coercion");
            }
            {
                auto nullable = another("nullable-quote");
                require(nullable->exec("PRAGMA ignore_check_constraints=ON") &&
                            nullable->exec(
                                "UPDATE subathon_plan_revisions SET historical_resolution_ref=NULL "
                                "WHERE plan_ref=X'706C616E2D50'"),
                        "nullable required quote member is a feasible grouped corruption");
                const auto before = nullable->snapshot();
                SqliteWriter writer;
                open(writer, *nullable);
                bool caught = false;
                auto failure = refusal(
                    writer, *nullable,
                    [](auto &store) {
                        (void)store.read_inputs({"plan-P", 1},
                                                ApplicationSubjectKind::contribution);
                    },
                    SR::invalid_stored_data, caught);
                known(failure, SR::invalid_stored_data);
                require(caught && nullable->snapshot() == before,
                        "SQLNULL is observed in a nullable inconsistent selected group, never an "
                        "impossible NOTNULL seed");
            }
            f_->seed({"boundary_raw_only"});
            SqliteWriter w;
            open(w, *f_);
            bool caught = false;
            const auto before = f_->snapshot();
            auto r = refusal(
                w, *f_,
                [](auto &store) {
                    (void)store.read_inputs({"raw-MAX", std::numeric_limits<std::uint64_t>::max()},
                                            ApplicationSubjectKind::contribution);
                },
                SR::invalid_stored_data, caught);
            known(r, SR::invalid_stored_data);
            require(caught && f_->snapshot() == before,
                    "count3/headMAX is INVALID, never a healthy application/exhaustion proof");
        });
    }
    void returnedAndInputBoundsHaveExactOneOverCategories() {
        scenario([&] {
            auto p = l_->prepared();
            SqliteWorkSession expired;
            for (const auto &entry :
                 std::array<std::pair<int, int>, 4>{{{0, 4}, {1, 2}, {2, 2}, {3, 18}}}) {
                SqliteBundleLimits limit;
                const auto set = [&](int value) {
                    if (entry.first == 0)
                        limit.effects_per_bundle = value;
                    if (entry.first == 1)
                        limit.jobs_per_bundle = value;
                    if (entry.first == 2)
                        limit.balances_per_state = value;
                    if (entry.first == 3)
                        limit.input_items = value;
                };
                set(entry.second);
                SqliteApplicationBundleStore fitting(expired, limit);
                work_failure([&] { (void)fitting.stage(p); }, WorkError::stale_session);
                set(entry.second - 1);
                SqliteApplicationBundleStore over(expired, limit);
                input_failure([&] { (void)over.stage(p); }, IE::collection_limit);
            }
            auto reference = p;
            reference.intent.operation.operation_ref = std::string(4096, 'k');
            SqliteApplicationBundleStore defaults(expired);
            work_failure([&] { (void)defaults.stage(reference); }, WorkError::stale_session);
            reference.intent.operation.operation_ref.push_back('k');
            input_failure([&] { (void)defaults.stage(reference); }, IE::invalid_key);
            auto value = p;
            value.result.document = QByteArray(64, 'x');
            SqliteBundleLimits value_limit;
            value_limit.value_bytes = 64;
            SqliteApplicationBundleStore values(expired, value_limit);
            work_failure([&] { (void)values.stage(value); }, WorkError::stale_session);
            value.result.document.append('x');
            input_failure([&] { (void)values.stage(value); }, IE::invalid_result);
            // Independent logical-byte table:145+122+121+41+216+116+36+41=838.
            SqliteBundleLimits owned_input;
            owned_input.owned_bytes = 838;
            SqliteApplicationBundleStore fitting_owned(expired, owned_input);
            work_failure([&] { (void)fitting_owned.stage(p); }, WorkError::stale_session);
            owned_input.owned_bytes = 837;
            SqliteApplicationBundleStore over_owned(expired, owned_input);
            input_failure([&] { (void)over_owned.stage(p); }, IE::owned_byte_limit);
            auto refund = l_->prepared("refund1");
            SqliteBundleLimits links;
            links.links_per_bundle = 2;
            SqliteApplicationBundleStore fitting_links(expired, links);
            work_failure([&] { (void)fitting_links.stage(refund); }, WorkError::stale_session);
            links.links_per_bundle = 1;
            SqliteApplicationBundleStore over_links(expired, links);
            input_failure([&] { (void)over_links.stage(refund); }, IE::collection_limit);
            auto trace = p;
            trace.advanced_before.timer.phase = CountdownPhase::Grace;
            trace.transitions = {TimerTransition::MainExpired, TimerTransition::GraceStarted};
            SqliteBundleLimits transitions;
            transitions.transitions_per_bundle = 2;
            SqliteApplicationBundleStore fitting_trace(expired, transitions);
            work_failure([&] { (void)fitting_trace.stage(trace); }, WorkError::stale_session);
            transitions.transitions_per_bundle = 1;
            SqliteApplicationBundleStore over_trace(expired, transitions);
            input_failure([&] { (void)over_trace.stage(trace); }, IE::collection_limit);
            f_->seed({"original_prepare", "original_finish"});
            SqliteWriter w;
            open(w, *f_);
            SqliteBundleLimits limit;
            limit.result_items = 20;
            std::optional<RecordedApplicationBundle> out;
            ok(lookup(w, key(), out, limit));
            require(out.has_value(), "lookup counts actual returned20, no hidden plan DTO quota");
            limit.result_items = 19;
            bool caught = false;
            auto r = refusal(
                w, *f_, [](auto &store) { (void)store.lookup(key()); }, SR::result_limit, caught,
                limit);
            known(r, SR::result_limit);
            require(caught, "complete read refuses one-over, never truncates");
            limit = {};
            limit.result_items = 9;
            r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s, limit);
                require(store.read_inputs({"plan-P", 1}, ApplicationSubjectKind::contribution)
                            .inputs.has_value(),
                        "inputs count actual9");
                s.commit();
                return 1;
            });
            ok(r);
            limit.result_items = 8;
            r = refusal(
                w, *f_,
                [](auto &store) {
                    (void)store.read_inputs({"plan-P", 1}, ApplicationSubjectKind::contribution);
                },
                SR::result_limit, caught, limit);
            known(r, SR::result_limit);
            limit = {};
            limit.result_items = 10;
            r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s, limit);
                require(store.read_reversal_basis(main_basis()).basis.has_value(),
                        "Main basis count actual10, selected Effect is scalar");
                s.commit();
                return 1;
            });
            ok(r);
            limit.result_items = 9;
            r = refusal(
                w, *f_, [](auto &store) { (void)store.read_reversal_basis(main_basis()); },
                SR::result_limit, caught, limit);
            known(r, SR::result_limit);
            limit = {};
            limit.plan_items_per_vector = 4;
            r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s, limit);
                require(store.read_inputs({"plan-P", 1}, ApplicationSubjectKind::contribution)
                            .inputs.has_value(),
                        "selected plan vector exactly4 fits");
                s.commit();
                return 1;
            });
            ok(r);
            limit.plan_items_per_vector = 3;
            r = refusal(
                w, *f_,
                [](auto &store) {
                    (void)store.read_inputs({"plan-P", 1}, ApplicationSubjectKind::contribution);
                },
                SR::result_limit, caught, limit);
            known(r, SR::result_limit);
            auto owned = another("owned-state-boundary");
            SqliteWriter owned_writer;
            open(owned_writer, *owned);
            SqliteBundleLimits state_bytes;
            state_bytes.owned_bytes = 112;
            r = run(owned_writer, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s, state_bytes);
                auto selected_state = store.read_state();
                require(
                    selected_state && same(*selected_state, l_->state("start")),
                    "returned state literal112=57identity/envelope/config+27timer/debt+28balances");
                s.commit();
                return 1;
            });
            ok(r);
            state_bytes.owned_bytes = 111;
            r = refusal(
                owned_writer, *owned, [](auto &store) { (void)store.read_state(); },
                SR::result_limit, caught, state_bytes);
            known(r, SR::result_limit);
        });
    }
    void invalidInputPriorityPreservesHealthyUnit() {
        scenario([&] {
            const auto good = l_->prepared();
            SqliteWorkSession expired;
            SqliteBundleLimits invalid;
            invalid.reference_bytes = 0;
            SqliteApplicationBundleStore bad(expired, invalid);
            input_failure([&] { (void)bad.lookup({"", ""}); }, IE::invalid_options);
            input_failure([&] { (void)bad.read_state(); }, IE::invalid_options);
            SqliteApplicationBundleStore store(expired);
            input_failure([&] { (void)store.read_reversal_basis({{"", ""}, 0, CK::debt_delta}); },
                          IE::invalid_key);
            input_failure([&] { (void)store.read_reversal_basis({key(), 0, CK::debt_delta}); },
                          IE::invalid_reversal);
            auto p = good;
            p.intent.operation.operation_ref.clear();
            p.intent.receipt_id = {0};
            input_failure([&] { (void)store.stage(p); }, IE::invalid_key);
            p = good;
            p.intent.receipt_id = {0};
            input_failure([&] { (void)store.stage(p); }, IE::invalid_source);
            p = good;
            p.after.timer.main_remaining_ms = -1;
            SqliteBundleLimits small;
            small.effects_per_bundle = 3;
            SqliteApplicationBundleStore counts(expired, small);
            input_failure([&] { (void)counts.stage(p); }, IE::collection_limit);
            p = good;
            p.after.token.clear();
            p.intent.expected_contribution_state.reset();
            input_failure([&] { (void)store.stage(p); }, IE::invalid_state);
            p = good;
            p.intent.expected_contribution_state->payment_evidence = PaymentEvidence(99);
            input_failure([&] { (void)store.stage(p); }, IE::invalid_contribution);
            p = good;
            p.effects[0].request_ordinal = 0;
            input_failure([&] { (void)store.stage(p); }, IE::invalid_effect);
            p = good;
            p.effects[0].components.pop_back();
            input_failure([&] { (void)store.stage(p); }, IE::invalid_component);
            p = good;
            p.jobs[0].job_ref.clear();
            input_failure([&] { (void)store.stage(p); }, IE::invalid_job);
            p = good;
            p.transitions = {TimerTransition(99)};
            input_failure([&] { (void)store.stage(p); }, IE::invalid_transition);
            p = good;
            p.result.format_ref.clear();
            input_failure([&] { (void)store.stage(p); }, IE::invalid_result);
            p = good;
            p.audit.envelope.format_ref.clear();
            input_failure([&] { (void)store.stage(p); }, IE::invalid_audit);
            p = l_->prepared("refund1");
            p.reversals[0].request_ordinal = 0;
            input_failure([&] { (void)store.stage(p); }, IE::invalid_reversal);
            const std::array<qsizetype SqliteBundleLimits::*, 12> positive_options{
                &SqliteBundleLimits::reference_bytes,
                &SqliteBundleLimits::value_bytes,
                &SqliteBundleLimits::component_bytes,
                &SqliteBundleLimits::balances_per_state,
                &SqliteBundleLimits::plan_items_per_vector,
                &SqliteBundleLimits::effects_per_bundle,
                &SqliteBundleLimits::jobs_per_bundle,
                &SqliteBundleLimits::transitions_per_bundle,
                &SqliteBundleLimits::links_per_bundle,
                &SqliteBundleLimits::input_items,
                &SqliteBundleLimits::result_items,
                &SqliteBundleLimits::owned_bytes};
            for (auto member : positive_options) {
                SqliteBundleLimits option;
                option.*member = 0;
                SqliteApplicationBundleStore invalid_option(expired, option);
                input_failure([&] { (void)invalid_option.stage(good); }, IE::invalid_options);
            }
            const std::array<qsizetype SqliteBundleLimits::*, 8> count_options{
                &SqliteBundleLimits::balances_per_state,
                &SqliteBundleLimits::plan_items_per_vector,
                &SqliteBundleLimits::effects_per_bundle,
                &SqliteBundleLimits::jobs_per_bundle,
                &SqliteBundleLimits::transitions_per_bundle,
                &SqliteBundleLimits::links_per_bundle,
                &SqliteBundleLimits::input_items,
                &SqliteBundleLimits::result_items};
            for (auto member : count_options) {
                SqliteBundleLimits option;
                option.*member = std::numeric_limits<qsizetype>::max();
                SqliteApplicationBundleStore invalid_option(expired, option);
                input_failure([&] { (void)invalid_option.stage(good); }, IE::invalid_options);
            }
            SqliteWriter w;
            open(w, *f_);
            const auto r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore active(s);
                (void)s.execute({"INSERT INTO subathon_retained_configurations "
                                 "VALUES(X'707572652D7072696F72',X'79')",
                                 {}});
                auto malformed = good;
                malformed.result.format_ref.clear();
                input_failure([&] { (void)active.stage(malformed); }, IE::invalid_result);
                require(s.execute({"SELECT count(*) FROM subathon_retained_configurations WHERE "
                                   "reference=X'707572652D7072696F72'",
                                   {}})[0][0]
                                .toInt() == 1,
                        "caught pure input retains healthy earlier staging");
                s.commit();
                return 1;
            });
            ok(r);
            require(f_->scalar("SELECT count(*) FROM subathon_retained_configurations WHERE "
                               "reference=X'707572652D7072696F72'")
                            .toInt() == 1,
                    "pure failure performs no cleanup");
        });
    }
    void signedExactResidualAndAggregateConservationAreExact() {
        scenario([&] {
            SqliteApplicationBundleStore expired(SqliteWorkSession{});
            auto p = l_->prepared();
            p.after.timer.main_remaining_ms = 301;
            input_failure([&] { (void)expired.stage(p); }, IE::invalid_state);
            p = l_->prepared();
            p.effects[2].rounding_residual = 1;
            input_failure([&] { (void)expired.stage(p); }, IE::invalid_effect);
            for (int index = 0; index < 2; ++index) {
                auto f = another("half-even-" + QString::number(index));
                const auto value = index == 0 ? exact("3", "2") : exact("5", "2");
                auto request = request_like(l_->capture("plan-P").captured.requested_effects[2], 2,
                                            RequestedUnit::micro_points, value);
                f->replace_requests("plan-P", {request});
                auto input = single(*l_, request, 400, 0, 400, 0, AR::direct,
                                    {{CK::score, 2, std::string("score")}});
                input.after.balances[0].units = 2;
                f->set_initial(input.advanced_before);
                SqliteWriter w;
                open(w, *f);
                ok(stage(w, input));
                std::optional<RecordedApplicationBundle> out;
                ok(lookup(w, key(), out));
                require(out && out->effects[0].rounding_residual ==
                                   (index == 0 ? exact("-1", "2") : exact("1", "2")),
                        "independent literal half-even residual, no rerounding");
            }
            auto f = another("large-exact");
            const auto vector = l_->root["exact_vectors"].toArray().last().toObject();
            const auto huge = exact(vector["numerator"].toString().toStdString(),
                                    vector["denominator"].toString().toStdString());
            auto request = request_like(l_->capture("plan-P").captured.requested_effects[2], 0,
                                        RequestedUnit::micro_points, huge);
            f->replace_requests("plan-P", {request});
            auto input = single(*l_, request, 400, 0, 400, 0, AR::direct,
                                {{CK::score, 0, std::string("score")}});
            SqliteBundleLimits limits;
            limits.component_bytes = 1265;
            SqliteWriter w;
            open(w, *f);
            ok(stage(w, input, limits));
            std::optional<RecordedApplicationBundle> out;
            ok(lookup(w, key(), out, limits));
            require(out && out->effects[0].exact_requested_units == huge &&
                        out->effects[0].rounding_residual == huge,
                    "fitting >4096bit canonical VALUE retained as storage-only data, not finalizer "
                    "acceptance");
            limits.component_bytes = 1264;
            SqliteApplicationBundleStore bounded(SqliteWorkSession{}, limits);
            input_failure([&] { (void)bounded.stage(input); }, IE::exact_component_limit);
            auto overflow = l_->prepared();
            overflow.advanced_before.balances[0].units = std::numeric_limits<std::int64_t>::max();
            overflow.after.balances[0].units = std::numeric_limits<std::int64_t>::min();
            input_failure([&] { (void)expired.stage(overflow); }, IE::invalid_state);
        });
    }

    void multiBasisRefundsUseOneCurrentRequestAndIndependentProgress() {
        scenario([&] {
            f_->seed({"original_prepare", "original_finish"});
            const auto original =
                f_->rows("SELECT * FROM subathon_app_effects WHERE "
                         "operation_ref=X'6F726967696E616C' ORDER BY request_ordinal");
            SqliteWriter w;
            open(w, *f_);
            for (const auto &name : {"refund1", "refund2"}) {
                const auto p = l_->prepared(name);
                require(p.effects.size() == 1 && p.effects[0].request_ordinal == 1 &&
                            p.reversals.size() == 2,
                        "ONE immutable request, MANY original bases");
                ok(stage(w, p));
                std::optional<RecordedApplicationBundle> out;
                ok(lookup(w, key(name), out));
                require(out.has_value(), "refund result exists");
                matches(*out, p, l_->state(p.intent.expected_state_token));
                require(out->reversals[0].original.request_ordinal == 1 &&
                            out->reversals[1].original.request_ordinal == 2,
                        "distinct original occurrence locators");
            }
            require(
                f_->rows(
                    "SELECT hex(operation_ref),request_ordinal,count(*),sum(requested_increment) "
                    "FROM subathon_app_reversals GROUP BY operation_ref,request_ordinal ORDER BY "
                    "operation_ref") == SqlRows({{"726566756E6431", 1, 2, qint64(-75)},
                                                 {"726566756E6432", 1, 2, qint64(-225)}}),
                "independent literal25+50,75+150 sums; no request splitting");
            require(f_->rows("SELECT * FROM subathon_app_effects WHERE "
                             "operation_ref=X'6F726967696E616C' ORDER BY request_ordinal") ==
                        original,
                    "original credited effects immutable after full refund");
            require(f_->scalar("SELECT application FROM subathon_contributions WHERE "
                               "contribution_id=X'0000000000000000'")
                            .toInt() == 1,
                    "fully reversed original remains originally applied");
            auto f = another("SB13");
            f->seed({"original_prepare", "original_finish"});
            f->apply(
                {{"UPDATE subathon_plan_requests SET requested_units=50,exact_numerator=X'3530' "
                  "WHERE plan_ref=X'706C616E2D50' AND ordinal=2",
                  {}},
                 {"UPDATE subathon_app_effects SET requested_units=50,exact_numerator=X'3530' "
                  "WHERE operation_ref=X'6F726967696E616C' AND request_ordinal=2",
                  {}},
                 {"UPDATE subathon_app_components SET units=50 WHERE "
                  "operation_ref=X'6F726967696E616C' AND request_ordinal=2 AND component_kind=0",
                  {}},
                 {"UPDATE subathon_app_states SET main_remaining_ms=150 WHERE "
                  "token=X'6F726967696E616C2D6166746572'",
                  {}}});
            auto request = request_like(l_->capture("plan-R1").captured.requested_effects[0], -30);
            f->replace_requests("plan-R1", {request});
            auto p = l_->prepared("refund1");
            p.advanced_before.timer.main_remaining_ms = 150;
            p.after.timer.main_remaining_ms = 120;
            p.effects[0].exact_requested_units = -30;
            p.effects[0].requested_units = -30;
            p.effects[0].components[0].units = -30;
            for (size_t k = 0; k < 2; ++k) {
                auto &link = p.reversals[k];
                link.cumulative = k == 0 ? 20 : 10;
                link.requested_increment = -link.cumulative;
                link.cumulative_fraction = exact("1", "5");
                link.exact_entitlement = link.cumulative;
                link.rounding_residual = 0;
            }
            SqliteWriter writer;
            open(writer, *f);
            ok(stage(writer, p));
            std::optional<RecordedApplicationBundle> out;
            ok(lookup(writer, key("refund1"), out));
            require(out && out->effects.size() == 1 && out->reversals.size() == 2 &&
                        out->effects[0].requested_units == -30,
                    "SB13 actual100/50 bases and -20/-10 links retained as ONE -30 request");
            auto repeated = another("same-basis-two-current-requests");
            repeated->seed({"original_prepare", "original_finish"});
            auto first_request =
                request_like(l_->capture("plan-R1").captured.requested_effects[0], -10);
            auto second_request = request_like(first_request, -15);
            repeated->replace_requests("plan-R1", {first_request, second_request});
            auto split = l_->prepared("refund1");
            split.after.timer.main_remaining_ms = 275;
            split.effects.resize(2, split.effects[0]);
            for (size_t index = 0; index < 2; ++index) {
                auto &e = split.effects[index];
                e.request_ordinal = std::int64_t(index + 1);
                e.requested_units = index == 0 ? -10 : -15;
                e.exact_requested_units = e.requested_units;
                e.components[0].units = e.requested_units;
            }
            split.reversals.resize(2, split.reversals[0]);
            for (size_t index = 0; index < 2; ++index) {
                auto &link = split.reversals[index];
                link.request_ordinal = std::int64_t(index + 1);
                link.original = main_basis();
                link.expected_prior_position = std::int64_t(index);
                link.expected_prior_cumulative = index == 0 ? 0 : 10;
                link.cumulative = index == 0 ? 10 : 25;
                link.requested_increment = index == 0 ? -10 : -15;
                link.cumulative_fraction = index == 0 ? exact("1", "10") : exact("1", "4");
                link.exact_entitlement = link.cumulative;
                link.rounding_residual = 0;
            }
            SqliteWriter repeated_writer;
            open(repeated_writer, *repeated);
            ok(stage(repeated_writer, split));
            std::optional<RecordedApplicationBundle> repeated_out;
            ok(lookup(repeated_writer, key("refund1"), repeated_out));
            require(repeated_out.has_value(), "same basis can advance twice inside ONE bundle");
            matches(*repeated_out, split, l_->state("original-after"));
            require(
                repeated->rows("SELECT basis_position,cumulative,last_request_ordinal FROM "
                               "subathon_app_reversal_heads WHERE original_request_ordinal=1 AND "
                               "original_component_kind=0") == SqlRows({{2, 25, 2}}),
                "proposed chain uses previous proposed link, last head points current request2");
        });
    }
    void zeroIncrementFullRefundAndStaleProgressStayDistinct() {
        scenario([&] {
            auto f = another("zero-progress");
            f->seed({"original_prepare", "original_finish"});
            f->apply({{"UPDATE subathon_plan_requests SET requested_units=1,exact_numerator=X'31' "
                       "WHERE plan_ref=X'706C616E2D50' AND ordinal=3",
                       {}},
                      {"UPDATE subathon_app_effects SET requested_units=1,exact_numerator=X'31' "
                       "WHERE operation_ref=X'6F726967696E616C' AND request_ordinal=3",
                       {}},
                      {"UPDATE subathon_app_components SET units=1 WHERE "
                       "operation_ref=X'6F726967696E616C' AND request_ordinal=3",
                       {}},
                      {"UPDATE subathon_app_balances SET units=1 WHERE "
                       "token=X'6F726967696E616C2D6166746572' AND balance_kind=2",
                       {}}});
            const std::array<std::int64_t, 3> increments{0, -1, 0}, cumulative{0, 1, 1},
                previous{0, 0, 1};
            std::array<PreparedApplicationBundle, 3> proposals;
            auto current = l_->state("original-after");
            current.balances[0].units = 1;
            for (size_t k = 0; k < 3; ++k) {
                const auto request =
                    request_like(l_->capture("plan-S").captured.requested_effects[0], increments[k],
                                 RequestedUnit::micro_points);
                const auto plan = "zero-plan-" + std::to_string(k);
                f->clone_plan(plan, std::uint64_t(8 + k), 0, {request});
                auto p = l_->prepared("score-refund");
                p.intent.operation.operation_ref = "zero-" + std::to_string(k);
                p.intent.receipt_id = {std::uint64_t(8 + k)};
                p.intent.plan = {plan, 1};
                p.intent.reversal_ref = "zero-refund-" + std::to_string(k);
                p.intent.expected_state_token = current.token;
                p.advanced_before = current;
                p.after = current;
                p.after.token = "zero-after-" + std::to_string(k);
                p.after.balances[0].units += increments[k];
                p.advance_explanation.clear();
                p.effects[0].exact_requested_units = increments[k];
                p.effects[0].requested_units = increments[k];
                p.effects[0].rounding_residual = 0;
                p.effects[0].components[0].units = increments[k];
                auto &link = p.reversals[0];
                link.original = main_basis(3);
                link.original.component_kind = CK::score;
                link.expected_prior_position = std::int64_t(k);
                link.expected_prior_cumulative = previous[k];
                link.cumulative = cumulative[k];
                link.requested_increment = increments[k];
                link.cumulative_fraction = k == 2 ? Exact(1) : exact(std::to_string(k + 1), "3");
                link.exact_entitlement = link.cumulative_fraction;
                link.rounding_residual = link.exact_entitlement - link.cumulative;
                link.full_refund = k == 2;
                proposals[k] = p;
                current = p.after;
            }
            SqliteWriter w;
            open(w, *f);
            ok(stage(w, proposals[0]));
            auto stale = proposals[1];
            stale.reversals[0].expected_prior_position = 0;
            ApplicationBundleStageResult result;
            auto r = domain(w, stale, result);
            disposition(r, SD::stale_reversal, result);
            require(!result.existing, "stale position is normal refusal");
            auto stale_cumulative = proposals[2];
            stale_cumulative.intent.operation.operation_ref = "stale-cumulative";
            stale_cumulative.intent.expected_state_token = proposals[0].after.token;
            stale_cumulative.advanced_before = proposals[0].after;
            stale_cumulative.after = stale_cumulative.advanced_before;
            stale_cumulative.after.token = "stale-cumulative-after";
            stale_cumulative.reversals[0].expected_prior_position = 1;
            stale_cumulative.reversals[0].expected_prior_cumulative = 1;
            r = domain(w, stale_cumulative, result);
            disposition(r, SD::stale_reversal, result);
            require(!result.existing, "matching position but wrong prior cumulative also refuses");
            ok(stage(w, proposals[1]));
            ok(stage(w, proposals[2]));
            require(f->rows("SELECT "
                            "basis_position,previous_cumulative,cumulative,requested_increment,hex("
                            "residual_numerator),hex(residual_denominator),full_refund FROM "
                            "subathon_app_reversals WHERE original_component_kind=2 ORDER BY "
                            "basis_position") == SqlRows({{1, 0, 0, 0, "31", "33", 0},
                                                          {2, 0, 1, -1, "2D31", "33", 0},
                                                          {3, 1, 1, 0, "30", "31", 1}}),
                    "SB05 independent cumulative0/1/1, increments0/-1/0, residual1/3,-1/3,0 and "
                    "final-zero provenance");
            ApplicationReversalBasisResult basis;
            r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                basis = store.read_reversal_basis({key(), 3, CK::score});
                s.commit();
                return 1;
            });
            ok(r);
            require(basis.basis && basis.basis->actual_credit_units == 1 &&
                        basis.basis->progress.position == 3 &&
                        basis.basis->progress.cumulative == 1 &&
                        basis.basis->progress.full_refund && basis.basis->progress.last &&
                        same(basis.basis->progress.last->operation, key("zero-2")),
                    "read actual terminal full-refund occurrence including zero");
            require(f->scalar(
                         "SELECT units FROM subathon_app_balances WHERE token=? AND balance_kind=3",
                         {blob(proposals[2].after.token)})
                            .toLongLong() == 1,
                    "unaddressed whole-count original remains recorded; full-count refund policy "
                    "is upstream");
        });
    }
    void reversalBasisReadReturnsActualCreditAndTerminalKey() {
        scenario([&] {
            f_->seed({"original_prepare", "original_finish"});
            SqliteWriter w;
            open(w, *f_);
            ApplicationReversalBasisResult out;
            auto r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                out = store.read_reversal_basis(main_basis());
                s.commit();
                return 1;
            });
            ok(r);
            require(out.disposition == SqliteBundleBasisDisposition::ready && out.basis,
                    "valid original with no history gives real basis, not missing-zero conflation");
            const auto &b = *out.basis;
            require(same(b.original, main_basis()) &&
                        b.subject_kind == ApplicationSubjectKind::contribution &&
                        same(b.plan, {"plan-P", 1}) && b.selected_plan == l_->capture("plan-P") &&
                        same(b.effect, l_->prepared().effects[0]) && b.actual_credit_units == 100 &&
                        !b.target && b.application_before_token == "advanced-before" &&
                        b.application_configuration_ref == "cfg-arrival" &&
                        b.contribution_original_receipt_id == ReceiptId{1},
                    "all selected actual-credit/source/quote/effect/component provenance returned");
            require(b.progress.position == 0 && b.progress.cumulative == 0 &&
                        b.progress.cumulative_fraction == 0 && !b.progress.full_refund &&
                        !b.progress.last,
                    "exact valid no-occurrence progress");
            auto f = another("basis-terminal");
            f->seed({"original_prepare", "original_finish", "reversal_one", "reversal_two"});
            SqliteWriter writer;
            open(writer, *f);
            r = run(writer, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                auto v = store.read_reversal_basis(main_basis());
                require(v.basis && v.basis->actual_credit_units == 100 &&
                            v.basis->progress.position == 2 &&
                            v.basis->progress.cumulative == 100 &&
                            v.basis->progress.cumulative_fraction == 1 &&
                            v.basis->progress.full_refund && v.basis->progress.last &&
                            same(v.basis->progress.last->operation, key("refund2")) &&
                            v.basis->progress.last->request_ordinal == 1 &&
                            same(v.basis->progress.last->original, main_basis()),
                        "complete actual terminal key/current fraction/full flag");
                auto score = store.read_reversal_basis({key(), 3, CK::score});
                require(score.basis && score.basis->actual_credit_units == 100 &&
                            score.basis->target &&
                            same(*score.basis->target,
                                 ApplicationBalance{ApplicationBalanceKind::score, "score", 0}),
                        "selected BEFORE target, not today's score");
                s.commit();
                return 1;
            });
            ok(r);
            bool caught = false;
            r = refusal(
                writer, *f,
                [](auto &store) {
                    (void)store.read_reversal_basis({key("refund1"), 1, CK::main_delta});
                },
                SR::source_mismatch, caught);
            known(r, SR::source_mismatch);
            require(caught, "no reversal-of-reversal source masquerading as original");
            auto debt = another("unsupported-debt-basis");
            debt->seed({"original_prepare", "original_finish"});
            debt->apply(
                {{"UPDATE subathon_app_states SET time_debt_ms=10 WHERE token IN "
                  "(X'7374617274',X'616476616E6365642D6265666F7265')",
                  {}},
                 {"UPDATE subathon_app_states SET main_remaining_ms=290 WHERE "
                  "token=X'6F726967696E616C2D6166746572'",
                  {}},
                 {"UPDATE subathon_app_components SET units=90 WHERE "
                  "operation_ref=X'6F726967696E616C' AND request_ordinal=1 AND component_kind=0",
                  {}},
                 {"UPDATE subathon_app_components SET units=-10 WHERE "
                  "operation_ref=X'6F726967696E616C' AND request_ordinal=1 AND component_kind=1",
                  {}}});
            SqliteWriter debt_writer;
            open(debt_writer, *debt);
            r = run(debt_writer, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                auto v = store.read_reversal_basis(main_basis());
                require(v.disposition == SqliteBundleBasisDisposition::reversal_conflict &&
                            !v.basis,
                        "valid credited Main with prior debt repayment is not an invented enabled "
                        "refund allocation");
                (void)s.execute({"SELECT 1", {}});
                s.commit();
                return 1;
            });
            ok(r);
        });
    }
    void selectedSourceMetadataDoesNotCertifyUnselectedBodies() {
        scenario([&] {
            for (int kind = 0; kind < 2; ++kind) {
                auto f = another("unselected-body-" + QString::number(kind));
                f->seed({"original_prepare", "original_finish"});
                require(f->exec("PRAGMA ignore_check_constraints=ON"),
                        "external diagnostic corruption");
                require(
                    f->exec(
                        kind == 0
                            ? "UPDATE subathon_app_jobs SET document='wrong-class' WHERE ordinal=2"
                            : "UPDATE subathon_app_effects SET exact_numerator=X'626164' WHERE "
                              "operation_ref=X'6F726967696E616C' AND request_ordinal=2"),
                    "unselected payload fault");
                SqliteWriter w;
                open(w, *f);
                auto r = run(w, [&](auto context) -> QVariant {
                    auto s = context.begin();
                    SqliteBundleLimits limits;
                    limits.jobs_per_bundle = 1;
                    limits.effects_per_bundle = 1;
                    SqliteApplicationBundleStore store(s, limits);
                    auto v = store.read_reversal_basis(main_basis());
                    require(v.basis && same(v.basis->effect, l_->prepared().effects[0]),
                            "narrow selected projection certifies no unselected job/Exact body");
                    s.commit();
                    return 1;
                });
                ok(r);
                bool caught = false;
                r = refusal(
                    w, *f, [](auto &store) { (void)store.lookup(key()); }, SR::invalid_stored_data,
                    caught);
                known(r, SR::invalid_stored_data);
                require(caught, "full lookup DOES validate every selected body");
            }
            auto f = another("missing-sibling");
            f->seed({"original_prepare", "original_finish"});
            require(f->exec("DELETE FROM subathon_app_jobs WHERE ordinal=2"),
                    "missing sibling fixture");
            SqliteWriter w;
            open(w, *f);
            bool caught = false;
            auto r = refusal(
                w, *f, [](auto &store) { (void)store.read_reversal_basis(main_basis()); },
                SR::invalid_stored_data, caught);
            known(r, SR::invalid_stored_data);
            require(caught, "selected-source count metadata cannot trust declared sibling count or "
                            "ignore a missing row");
        });
    }
    void pagedMetadataBeyondSixtyFourHasNoMaterializationQuota() {
        scenario([&] {
            f_->seed({"original_prepare", "original_finish"});
            f_->add_jobs(65);
            f_->append_plan_metadata(65);
            require(f_->scalar("SELECT count(*) FROM subathon_app_jobs").toInt() == 65 &&
                        f_->scalar("SELECT count(*) FROM subathon_plan_revisions WHERE "
                                   "plan_ref=X'706C616E2D50'")
                                .toInt() == 65,
                    "actual65 contiguous metadata rows, not mocked counts");
            SqliteBundleLimits limits;
            limits.jobs_per_bundle = 1;
            limits.effects_per_bundle = 1;
            limits.result_items = 10;
            limits.plan_items_per_vector = 4;
            SqliteWriter w;
            open(w, *f_);
            auto r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s, limits);
                auto basis = store.read_reversal_basis(main_basis());
                require(basis.basis && basis.basis->actual_credit_units == 100 &&
                            basis.basis->selected_plan.revision == 1,
                        "old selected body plus65 job/history metadata fits selected-only return "
                        "bounds");
                auto stale = store.read_inputs({"plan-P", 1}, ApplicationSubjectKind::contribution);
                require(stale.disposition == SqliteBundleInputDisposition::stale_plan &&
                            !stale.inputs,
                        "complete65 audit before healthy stale classification");
                s.commit();
                return 1;
            });
            ok(r);
            require(f_->exec("UPDATE subathon_app_jobs SET ordinal=66 WHERE ordinal=65"),
                    "last-page ordinal gap, actualcount still65");
            const auto before = f_->snapshot();
            bool caught = false;
            r = refusal(
                w, *f_, [](auto &store) { (void)store.read_reversal_basis(main_basis()); },
                SR::invalid_stored_data, caught, limits);
            known(r, SR::invalid_stored_data);
            require(caught && f_->snapshot() == before,
                    "64row page is not permission to stop before65th malformed metadata");
            auto f = another("plan-page65");
            f->seed({"original_prepare", "original_finish"});
            f->append_plan_metadata(65);
            require(f->exec("UPDATE subathon_plan_revisions SET parent_revision=? WHERE plan_ref=? "
                            "AND revision=?",
                            {id_blob(63), blob("plan-P"), id_blob(65)}),
                    "last-page non-immediate parent remains a valid FK");
            SqliteWriter plan_writer;
            open(plan_writer, *f);
            r = refusal(
                plan_writer, *f,
                [](auto &store) {
                    (void)store.read_inputs({"plan-P", 1}, ApplicationSubjectKind::contribution);
                },
                SR::invalid_stored_data, caught);
            known(r, SR::invalid_stored_data);
            require(caught, "late structural corruption wins before stale/read caps");
            auto progress = another("progress-page65");
            progress->seed({"original_prepare", "original_finish", "reversal_one", "reversal_two"});
            progress->extend_zero_progress(65);
            require(progress->scalar("SELECT count(*) FROM subathon_app_reversals WHERE "
                                     "original_request_ordinal=1 AND original_component_kind=0")
                            .toInt() == 65,
                    "actual contiguous65 per-basis occurrences, not fake MAX count");
            SqliteWriter progress_writer;
            open(progress_writer, *progress);
            r = run(progress_writer, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s, limits);
                auto v = store.read_reversal_basis(main_basis());
                require(v.basis && v.basis->progress.position == 65 &&
                            v.basis->progress.cumulative == 100 && v.basis->progress.last &&
                            same(v.basis->progress.last->operation, key("progress-65")),
                        "per-basis65metadata audit independent of full-vector limits");
                s.commit();
                return 1;
            });
            ok(r);
            require(progress->exec("UPDATE subathon_app_reversals SET previous_cumulative=99 WHERE "
                                   "operation_ref=X'70726F67726573732D3635'"),
                    "typed last-page cumulative chain defect");
            r = refusal(
                progress_writer, *progress,
                [](auto &store) { (void)store.read_reversal_basis(main_basis()); },
                SR::invalid_stored_data, caught, limits);
            known(r, SR::invalid_stored_data);
            require(caught, "65th previous-cumulative mismatch cannot hide behind first64rows");
        });
    }
    void progressHeadPathsAndSparseSequenceAreValidated() {
        scenario([&] {
            const std::vector<QString> faults{
                "UPDATE subathon_app_reversal_heads SET cumulative=99 WHERE "
                "original_request_ordinal=1 AND original_component_kind=0",
                "UPDATE subathon_app_reversal_heads SET last_operation=X'726566756E6431' WHERE "
                "original_request_ordinal=1 AND original_component_kind=0",
                "DELETE FROM subathon_app_reversal_heads WHERE original_request_ordinal=1 AND "
                "original_component_kind=0"};
            int index = 0;
            for (const auto &fault : faults) {
                auto f = another("progress-" + QString::number(index++));
                f->seed({"original_prepare", "original_finish", "reversal_one", "reversal_two"});
                require(f->exec(fault),
                        "feasible independently dangling/mismatched progress paths");
                const auto before = f->snapshot();
                SqliteWriter w;
                open(w, *f);
                bool caught = false;
                auto r = refusal(
                    w, *f, [](auto &store) { (void)store.read_reversal_basis(main_basis()); },
                    SR::invalid_stored_data, caught);
                known(r, SR::invalid_stored_data);
                require(caught && f->snapshot() == before,
                        "both FK paths/cumulative/history must agree on SAME terminal occurrence");
            }
            f_->seed({"original_prepare", "original_finish", "reversal_one", "reversal_two"});
            const auto maximum = std::numeric_limits<std::int64_t>::max();
            require(f_->exec("UPDATE subathon_app_reversals SET basis_position=? WHERE "
                             "operation_ref=? AND original_request_ordinal=1",
                             {qint64(maximum), blob("refund2")}) &&
                        f_->exec("UPDATE subathon_app_reversal_heads SET basis_position=? WHERE "
                                 "original_request_ordinal=1 AND original_component_kind=0",
                                 {qint64(maximum)}),
                    "raw sparse position1/MAX diagnostic");
            SqliteWriter w;
            open(w, *f_);
            bool caught = false;
            auto r = refusal(
                w, *f_, [](auto &store) { (void)store.read_reversal_basis(main_basis()); },
                SR::invalid_stored_data, caught);
            known(r, SR::invalid_stored_data);
            require(caught, "sparse two-row signedMAX history is INVALID before exhaustion, no "
                            "fake healthy MAX sequence");
        });
    }
    void directAmountMustMatchCopiedRequestEvenWhenDeltaBalances() {
        scenario([&] {
            auto p = l_->prepared("score-refund");
            p.effects[0].components[0].units = -1000;
            p.after.balances[0].units = -900;
            SqliteApplicationBundleStore expired(SqliteWorkSession{});
            input_failure([&] { (void)expired.stage(p); }, IE::invalid_component);
            f_->seed({"original_prepare", "original_finish", "reversal_one", "reversal_two",
                      "score_reversal"});
            require(f_->exec("UPDATE subathon_app_components SET units=-1000 WHERE "
                             "operation_ref=X'73636F72652D726566756E64'") &&
                        f_->exec("UPDATE subathon_app_balances SET units=-900 WHERE "
                                 "token=X'73636F72652D6166746572' AND balance_kind=2"),
                    "SQL-admitted SB14 corruption");
            const auto before = f_->snapshot();
            SqliteWriter w;
            open(w, *f_);
            bool caught = false;
            auto r = refusal(
                w, *f_, [](auto &store) { (void)store.lookup(key("score-refund")); },
                SR::invalid_stored_data, caught);
            known(r, SR::invalid_stored_data);
            require(caught && f_->snapshot() == before,
                    "100-1000=-900 aggregate identity does not excuse immutable request-20/direct "
                    "component-1000 mismatch");
        });
    }

    void callerCommitAckLossKeepsUnknownWithPhysicalWholeBundle() {
        scenario([&] {
            SqliteWriter w;
            open(w, *f_);
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        w, WorkControlFault::lose_next_commit_ack) == StorageError::none,
                    "existing actual COMMIT acknowledgement-loss seam");
            int bodies = 0;
            bool staged = false, after_commit = false;
            const auto p = l_->prepared();
            const auto r = run(w, [&](auto context) -> QVariant {
                ++bodies;
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                require(store.stage(p).disposition == SD::staged,
                        "whole speculative bundle prepared");
                require(
                    s.execute({"SELECT count(*) FROM subathon_app_effects", {}})[0][0].toInt() ==
                            4 &&
                        s.execute({"SELECT hex(token) FROM subathon_app_head", {}})[0][0]
                                .toString() == "6F726967696E616C2D6166746572",
                    "actual whole staged records/head before real COMMIT");
                staged = true;
                s.commit();
                after_commit = true;
                return 1;
            });
            no_stub(r);
            require(
                bodies == 1 && staged && !after_commit && !r.storage &&
                    r.synchronous == WorkError::uncertain_outcome &&
                    r.result.transaction == TransactionOutcome::outcome_unknown &&
                    r.result.error == WorkError::uncertain_outcome && !r.result.value,
                "lost real acknowledgement remains unknown, never a known port rollback/success");
            for (const auto &entry :
                 std::array<std::pair<const char *, int>, 7>{{{"subathon_app_operations", 1},
                                                              {"subathon_app_effects", 4},
                                                              {"subathon_app_components", 8},
                                                              {"subathon_app_jobs", 2},
                                                              {"subathon_app_results", 1},
                                                              {"subathon_app_audit", 1},
                                                              {"subathon_app_claims", 1}}})
                require(f_->scalar("SELECT count(*) FROM " + QString(entry.first)).toInt() ==
                            entry.second,
                        "physical whole bundle after deliberately successful SQL COMMIT");
            require(f_->scalar("SELECT application FROM subathon_contributions WHERE "
                               "contribution_id=X'0000000000000000'")
                            .toInt() == 1,
                    "unknown does not imply pending; physical status observed separately");
            require(w.enqueue_work([](auto) -> QVariant { return 1; }, [](auto) {}) ==
                            StorageError::shutting_down &&
                        bodies == 1,
                    "fail closed, one body, no blind replay");
        });
    }
    void cleanupAckLossOutranksSelectedFailure() {
        scenario([&] {
            f_->seed({"original_prepare", "original_finish"});
            require(
                f_->exec("PRAGMA ignore_check_constraints=ON") &&
                    f_->exec("UPDATE subathon_app_jobs SET document='wrong-class' WHERE ordinal=2"),
                "selected malformed body");
            const auto before = f_->snapshot();
            SqliteWriter w;
            open(w, *f_);
            require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        w, WorkControlFault::lose_next_rollback_ack) == StorageError::none,
                    "actual rollback acknowledgement loss");
            int bodies = 0;
            bool prior_seen = false;
            const auto r = run(w, [&](auto context) -> QVariant {
                ++bodies;
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                (void)s.execute({"INSERT INTO subathon_retained_configurations "
                                 "VALUES(X'756E6B6E6F776E2D7072696F72',X'79')",
                                 {}});
                prior_seen = s.execute({"SELECT count(*) FROM subathon_retained_configurations "
                                        "WHERE reference=X'756E6B6E6F776E2D7072696F72'",
                                        {}})[0][0]
                                 .toInt() == 1;
                (void)store.lookup(key());
                return 1;
            });
            no_stub(r);
            require(
                bodies == 1 && prior_seen && !r.storage &&
                    r.synchronous == WorkError::uncertain_outcome &&
                    r.result.transaction == TransactionOutcome::outcome_unknown &&
                    r.result.error == WorkError::uncertain_outcome && !r.result.value,
                "unknown cleanup outranks local selected reason; inactive cleanup is not proof");
            require(f_->snapshot() == before,
                    "physical rollback observed separately from uncertain reported durability");
            require(w.enqueue_work([](auto) -> QVariant { return 1; }, [](auto) {}) ==
                        StorageError::shutting_down,
                    "no replay after cleanup uncertainty");
        });
    }
    void postCommitBodyFailurePreservesAccountingWithoutPayload() {
        scenario([&] {
            SqliteWriter w;
            open(w, *f_);
            bool commit_seen = false;
            const auto p = l_->prepared();
            const auto r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                require(store.stage(p).disposition == SD::staged, "stage before physical COMMIT");
                s.commit();
                commit_seen = true;
                throw std::runtime_error("synthetic body failure after acknowledged COMMIT");
                return 1;
            });
            no_stub(r);
            require(commit_seen && r.result.transaction == TransactionOutcome::committed &&
                        r.result.error == WorkError::work_exception && !r.result.value &&
                        !r.storage && !r.synchronous,
                    "committed accounting but body failed before normal completion, no normal work "
                    "payload");
            std::optional<RecordedApplicationBundle> out;
            ok(lookup(w, key(), out));
            require(out.has_value(),
                    "durable operation result still retrievable without reapplication");
            matches(*out, p, l_->state("start"));
            require(f_->scalar("SELECT count(*) FROM subathon_app_operations").toInt() == 1,
                    "no replaying accounting on result retrieval");
            const auto completed = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                auto result = store.lookup(key());
                require(result.has_value(), "record found");
                s.commit();
                return QByteArray("already-produced normal work payload");
            });
            ok(completed);
            try {
                throw std::runtime_error("later response publication failed");
            } catch (const std::runtime_error &) {
            }
            require(completed.result.value &&
                        completed.result.value->toByteArray() ==
                            "already-produced normal work payload" &&
                        f_->scalar("SELECT count(*) FROM subathon_app_operations").toInt() == 1,
                    "later publication failure cannot erase completed payload/durable accounting");
        });
    }
    void genuineMidstageJobFaultRollsBackWholeCallerUnit() {
        scenario([&] {
            const auto prior = f_->snapshot();
            const auto fault = l_->root["fault"].toObject();
            require(f_->released() && f_->exec(fault["create_trigger"].toString()),
                    "install reviewed compound test trigger on IDLE independent observer, not "
                    "restricted writer");
            require(f_->scalar("SELECT count(*) FROM sqlite_schema WHERE type='trigger' AND "
                               "name='storage002_job_fault'")
                            .toInt() == 1,
                    "instrumentation exists");
            const auto with_trigger = f_->snapshot();
            SqliteWriter w;
            open(w, *f_);
            bool caller_seen = false, attempted = false, caught = false;
            const auto p = l_->prepared();
            const auto r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                (void)s.execute({"INSERT INTO subathon_retained_configurations "
                                 "VALUES(X'6230382D7072696F72',X'79')",
                                 {}});
                caller_seen = s.execute({"SELECT count(*) FROM subathon_retained_configurations "
                                         "WHERE reference=X'6230382D7072696F72'",
                                         {}})[0][0]
                                  .toInt() == 1;
                require(
                    f_->scalar("SELECT application FROM subathon_contributions WHERE "
                               "contribution_id=X'0000000000000000'")
                                .toInt() == 0 &&
                        f_->scalar("SELECT hex(token) FROM subathon_app_head").toString() ==
                            "7374617274",
                    "earlier actual committed pending0/head is independently visible before stage");
                try {
                    attempted = true;
                    (void)store.stage(p);
                } catch (const SqliteBundleStorageFailure &e) {
                    require(e.reason() == SR::sql_failure, "actual SQL port error DURING stage");
                    require(f_->released() && f_->snapshot() == with_trigger,
                            "real rollback inside catch while facade alive includes earlier caller "
                            "work");
                    caught = true;
                    throw;
                }
                throw std::runtime_error("conditional job trigger must abort during stage");
                return 1;
            });
            known(r, SR::sql_failure, WorkError::sql_failure);
            require(caller_seen && attempted && caught && f_->snapshot() == with_trigger,
                    "actual conditional engine prerequisite proof and complete typed rollback, not "
                    "a later caller exception");
            require(f_->released() && f_->exec(fault["drop_trigger"].toString()) &&
                        f_->snapshot() == prior,
                    "remove only disposable instrumentation before identical retry");
            ok(stage(w, p));
            std::optional<RecordedApplicationBundle> out;
            ok(lookup(w, key(), out));
            require(out.has_value(), "fault-off exact same preparation commits once");
            matches(*out, p, l_->state("start"));
        });
    }
    void cumulativeWriterBudgetsAbortObservedStaging() {
        scenario([&] {
            f_->seed({"original_prepare", "original_finish"});
            f_->apply(
                {{"UPDATE subathon_app_jobs SET document=? WHERE operation_ref=? AND ordinal=1",
                  {QByteArray(600000, 'x'), blob("original")}}});
            const auto before = f_->snapshot();
            StorageLimits budget = ample();
            budget.result_value_bytes = 1048576;
            SqliteBundleLimits limits;
            limits.value_bytes = 700000;
            SqliteWriter w;
            open(w, *f_, budget);
            bool staged = false, consumed = false, caught = false;
            const auto refund = l_->prepared("refund1");
            const auto r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s, limits);
                require(
                    store.stage(refund).disposition == SD::staged,
                    "small new refund uses selected-source metadata, not 600k unselected job body");
                require(
                    s.execute({"SELECT count(*) FROM subathon_app_operations", {}})[0][0].toInt() ==
                        2,
                    "actual +1 staged operation before budget failure");
                staged = true;
                const auto selected_job =
                    s.execute({"SELECT typeof(document),document FROM subathon_app_jobs WHERE "
                               "operation_scope=? AND operation_ref=? AND ordinal=1",
                               {blob("scope"), blob("original")}});
                require(selected_job.size() == 1 && selected_job[0].size() == 2 &&
                            selected_job[0][0].toString() == "blob" &&
                            bytes(selected_job[0][1]) == std::string(600000, 'x'),
                        "one explicit old-session read consumes at least600000 result bytes");
                consumed = true;
                try {
                    (void)store.lookup(key());
                } catch (const SqliteBundleStorageFailure &e) {
                    require(e.reason() == SR::result_limit && f_->released(),
                            "required600000-byte body exceeds at most448576 remaining cumulative "
                            "result bytes and rolls unit back inside catch");
                    caught = true;
                    throw;
                }
                throw std::runtime_error("required cumulative result budget failure absent");
                return 1;
            });
            known(r, SR::result_limit, WorkError::result_limit);
            require(staged && consumed && caught && f_->snapshot() == before,
                    "no reset/truncate/partial application, prior durable original and large body "
                    "unchanged");
        });
    }
    void borrowedStoreLifetimeAndCapabilityErrorsStayTruthful() {
        scenario([&] {
            SqliteApplicationBundleStore expired(SqliteWorkSession{});
            work_failure([&] { (void)expired.read_state(); }, WorkError::stale_session);
            const auto p = l_->prepared();
            auto f = another("borrowed-dtor");
            const auto before = f->snapshot();
            SqliteWriter w;
            open(w, *f);
            auto r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                {
                    SqliteApplicationBundleStore store(s);
                    auto copy = store;
                    require(copy.stage(p).disposition == SD::staged,
                            "copy grants no independent commit authority");
                }
                require(
                    s.execute({"SELECT count(*) FROM subathon_app_operations", {}})[0][0].toInt() ==
                            1 &&
                        !f->released(),
                    "facade destruction does not clean still-active caller staging");
                return 1;
            });
            clean_rollback(r, WorkError::uncommitted);
            require(f->snapshot() == before,
                    "actual work owner abandonment rolls all speculative rows back");
            std::optional<SqliteApplicationBundleStore> saved;
            std::optional<WorkError> foreign;
            std::optional<IE> pure;
            bool thread_stub = false;
            std::string thread_problem;
            r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                saved.emplace(store);
                std::thread thread([&] {
                    try {
                        (void)store.read_state();
                    } catch (const SqliteApplicationBundleNotImplemented &) {
                        thread_stub = true;
                    } catch (const SqliteWorkFailure &e) {
                        foreign = e.error();
                    } catch (const std::exception &error) {
                        thread_problem = error.what();
                    }
                    try {
                        (void)store.lookup({"", ""});
                    } catch (const SqliteApplicationBundleNotImplemented &) {
                        thread_stub = true;
                    } catch (const SqliteBundleInputFailure &e) {
                        pure = e.error();
                    } catch (const std::exception &error) {
                        thread_problem = error.what();
                    }
                });
                thread.join();
                require(thread_problem.empty(),
                        "unexpected foreign-thread failure: " + thread_problem);
                if (thread_stub)
                    throw SqliteApplicationBundleNotImplemented();
                require(foreign == WorkError::wrong_thread && pure == IE::invalid_key,
                        "actual foreign owner truth, pure input wins before ticket access");
                s.rollback();
                work_failure([&] { (void)store.read_state(); }, WorkError::session_closed);
                return 1;
            });
            clean_rollback(r);
            require(saved.has_value(), "copied inert ticket retained for expiry test");
            work_failure([&] { (void)saved->read_state(); }, WorkError::stale_session);
        });
    }
    void presentEmptyAndOpaqueEnvelopeFieldsAreLossless() {
        scenario([&] {
            const std::string opaque("\xEF\xBB\xBF"
                                     "a\0\xFF",
                                     6);
            auto p = l_->prepared();
            auto head = l_->state("start");
            head.token = opaque + "-head";
            f_->add_state(head);
            f_->apply(
                {{"UPDATE subathon_app_head SET token=? WHERE singleton=1", {blob(head.token)}}});
            require(f_->scalar("SELECT count(*) FROM subathon_app_states WHERE "
                               "token=X'7374617274'")
                                .toInt() == 1 &&
                        f_->scalar("SELECT count(*) FROM subathon_app_balances WHERE "
                                   "token=X'7374617274'")
                                .toInt() == 2,
                    "prior immutable start snapshot and balances remain while head moves");
            p.intent.operation = {opaque, opaque + "-operation"};
            p.intent.expected_state_token = head.token;
            p.advanced_before.token = opaque + "-before";
            p.after.token = opaque + "-after";
            p.advance_explanation = opaque;
            p.intent.choices.document = QByteArray("", 0);
            p.result.document = blob(opaque);
            p.audit.actor_ref = std::string();
            p.audit.decision_ref.reset();
            p.audit.reason = opaque;
            p.jobs[0].group_ref = opaque;
            p.jobs[1].group_ref = opaque;
            p.jobs[0].job_ref = opaque + "-job1";
            p.jobs[1].job_ref = opaque + "-job2";
            p.jobs[1].envelope.document = QByteArray("", 0);
            const auto expected = p;
            SqliteWriter w;
            open(w, *f_);
            std::optional<RecordedApplicationBundle> out;
            auto r = run(w, [&](auto context) -> QVariant {
                auto s = context.begin();
                SqliteApplicationBundleStore store(s);
                require(store.stage(p).disposition == SD::staged,
                        "opaque keys and present-empty documents stage");
                out = store.lookup(p.intent.operation);
                require(out.has_value(), "own provisional result owns bytes");
                p.result.document.fill('x');
                p.audit.reason = std::string("mutated");
                p.jobs.clear();
                s.commit();
                return 1;
            });
            ok(r);
            matches(*out, expected, head);
            require(out->audit.actor_ref == std::string() && !out->audit.decision_ref &&
                        out->audit.reason == opaque && out->jobs[1].envelope.document.isEmpty() &&
                        !out->jobs[1].envelope.document.isNull(),
                    "present empty BLOB/string differs from SQLNULL absence");
            require(f_->rows("SELECT "
                             "typeof(operation_scope),hex(operation_scope),typeof(choices_document)"
                             ",length(choices_document) FROM subathon_app_operations") ==
                        SqlRows({{"blob", QString::fromLatin1(blob(opaque).toHex().toUpper()),
                                  "blob", qint64(0)}}),
                    "actual binary classes preserve BOM/NUL/FF without UTF8 normalization");
        });
    }
};
QTEST_GUILESS_MAIN(SqliteApplicationBundleTests)
#include "sqlite_application_bundle_tests.moc"
