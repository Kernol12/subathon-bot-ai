// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_application_bundle_values.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <subathon/server/sqlite_contributions.hpp>
#include <subathon/server/sqlite_retained_context.hpp>
#include <tuple>
#include <utility>
namespace subathon::server {
namespace {
using namespace core;
using Reason = SqliteBundleStorageReason;
using SD = SqliteBundleStageDisposition;
using CK = ApplicationComponentKind;
using AR = ApplicationArithmeticRole;
const QStringList receipt_columns{"receipt_id",
                                  "provider",
                                  "source_account",
                                  "identifier_namespace",
                                  "opaque_id",
                                  "event_type",
                                  "schema_version",
                                  "source_timestamp_utc_ms",
                                  "actor_id",
                                  "actor_display_name",
                                  "money_original_decimal",
                                  "money_currency",
                                  "payment_namespace",
                                  "payment_opaque_id",
                                  "activity_namespace",
                                  "activity_opaque_id",
                                  "server_timestamp_utc_ms",
                                  "connection_ref",
                                  "configuration_ref",
                                  "modifier_context_ref"};
const QStringList condition_columns{"monetary",        "original_decimal",   "currency",
                                    "resolution_ref",  "missing_rate",       "review",
                                    "evidence_policy", "policy_ref",         "decision_choice",
                                    "decision_ref",    "decision_actor_ref", "application",
                                    "reversal",        "settlement"};
const QStringList header_columns{"plan_ref",       "event_ref",  "parent_event_ref",
                                 "root_event_ref", "receipt_id", "contribution_id",
                                 "head_revision"};
const QStringList revision_columns{"plan_ref",
                                   "revision",
                                   "parent_revision",
                                   "payment_evidence",
                                   "monetary",
                                   "original_decimal",
                                   "currency",
                                   "resolution_ref",
                                   "missing_rate",
                                   "review",
                                   "evidence_policy",
                                   "policy_ref",
                                   "decision_choice",
                                   "decision_ref",
                                   "decision_actor_ref",
                                   "application",
                                   "reversal",
                                   "settlement",
                                   "valuation_state",
                                   "historical_resolution_ref",
                                   "historical_original_decimal",
                                   "historical_currency",
                                   "comparison_currency",
                                   "effective_rate_decimal",
                                   "fetched_rate_decimal",
                                   "snapshot_ref",
                                   "rate_source_ref",
                                   "rate_timestamp_utc_ms",
                                   "fetch_timestamp_utc_ms",
                                   "manual_decision_ref",
                                   "override_authority",
                                   "override_actor_ref",
                                   "override_reason",
                                   "override_decision_ref"};
const QStringList rule_columns{"plan_ref",    "revision", "ordinal",    "rule_ref",
                               "version_ref", "selected", "explanation"};
const QStringList modifier_columns{"plan_ref",
                                   "revision",
                                   "ordinal",
                                   "definition_version_ref",
                                   "instance_revision_ref",
                                   "target_ref",
                                   "group_ref",
                                   "group_operator",
                                   "target_operator",
                                   "original_factor_decimal"};
const QStringList request_columns{"plan_ref",
                                  "revision",
                                  "ordinal",
                                  "action_ref",
                                  "action_version_ref",
                                  "rule_ref",
                                  "target_ref",
                                  "unit",
                                  "requires_historical_valuation",
                                  "exact_numerator",
                                  "exact_denominator",
                                  "requested_units",
                                  "calculation_explanation",
                                  "reversal_behavior_ref"};
QString projection(const QStringList &columns) {
    QStringList fields;
    for (const auto &column : columns) {
        fields.append(column);
        fields.append("typeof(" + column + ")");
        fields.append("length(" + column + ")");
    }
    return fields.join(',');
}
SqlStatement insertion(const QString &table, const QStringList &columns, QList<QVariant> values) {
    QStringList marks;
    for (qsizetype i = 0; i < columns.size(); ++i)
        marks.append("?");
    return {"INSERT INTO main." + table + "(" + columns.join(',') + ") VALUES (" + marks.join(',') +
                ")",
            std::move(values)};
}
QByteArray bytes(const std::string &value) {
    return value.empty() ? QByteArray("", 0) : QByteArray(value.data(), qsizetype(value.size()));
}
bool bounded(const std::string &v, qsizetype n, bool required = false) {
    return (!required || !v.empty()) && v.size() <= size_t(n);
}
template <class E> bool enum_in(E v, int maximum) { return int(v) >= 0 && int(v) <= maximum; }
bool valid_valuation(const PlanValuation &v, const PlanEvent &event, bool dependent,
                     SqliteBundleLimits l) {
    if (!enum_in(v.state, 2) ||
        (v.state == PlanValuationState::resolved) != v.historical.has_value() ||
        (v.state == PlanValuationState::missing_historical && dependent))
        return false;
    if (!v.historical)
        return true;
    const auto &q = *v.historical;
    return event.facts.money && q.original_money == *event.facts.money &&
           bounded(q.resolution_ref, l.reference_bytes) &&
           bounded(q.original_money.original_decimal, l.value_bytes) &&
           bounded(q.original_money.currency, l.value_bytes) &&
           bounded(q.comparison_currency, l.value_bytes) &&
           bounded(q.effective_rate_decimal, l.value_bytes) &&
           (!q.fetched_rate_decimal || bounded(*q.fetched_rate_decimal, l.value_bytes)) &&
           bounded(q.snapshot_ref, l.reference_bytes) &&
           (!q.source_ref || bounded(*q.source_ref, l.reference_bytes)) &&
           (!q.manual_decision_ref || bounded(*q.manual_decision_ref, l.reference_bytes));
}
QList<QVariant> condition_values(const core::ContributionConditions &c) {
    const auto &v = c.valuation;
    const auto &a = c.evidence_admission;
    return {int(v.monetary),
            v.original_money ? QVariant(bytes(v.original_money->original_decimal)) : QVariant{},
            v.original_money ? QVariant(bytes(v.original_money->currency)) : QVariant{},
            v.resolution_ref ? QVariant(bytes(*v.resolution_ref)) : QVariant{},
            int(v.missing_rate),
            int(c.review),
            int(a.policy),
            bytes(a.policy_ref),
            a.review_decision ? QVariant(int(a.review_decision->choice)) : QVariant{},
            a.review_decision ? QVariant(bytes(a.review_decision->decision_ref)) : QVariant{},
            a.review_decision ? QVariant(bytes(a.review_decision->actor_ref)) : QVariant{},
            int(c.application),
            int(c.reversal),
            int(c.settlement)};
}
const QStringList activity_columns{"activity_provider", "activity_source_account",
                                   "activity_namespace", "activity_id"};
QStringList contribution_columns() {
    QStringList columns{"contribution_id", "original_receipt_id"};
    columns.append(activity_columns);
    columns.append("payment_evidence");
    columns.append(condition_columns);
    return columns;
}
QStringList observation_columns() {
    QStringList columns{"receipt_id", "contribution_id", "kind", "evidence"};
    columns.append(activity_columns);
    columns.append("verification_ref");
    for (const auto &column : condition_columns)
        columns.append("support_" + column);
    return columns;
}

[[noreturn]] void map_work_failure(const SqliteWorkFailure &e) {
    switch (e.error()) {
    case WorkError::busy:
        throw SqliteBundleStorageFailure(Reason::busy);
    case WorkError::sql_failure:
        throw SqliteBundleStorageFailure(Reason::sql_failure);
    case WorkError::resource_limit:
        throw SqliteBundleStorageFailure(Reason::resource_limit);
    case WorkError::result_limit:
        throw SqliteBundleStorageFailure(Reason::result_limit);
    default:
        throw;
    }
}
const QStringList app_states_columns{"token",
                                     "format_ref",
                                     "document",
                                     "configuration_ref",
                                     "lifecycle",
                                     "phase",
                                     "main_remaining_ms",
                                     "grace_remaining_ms",
                                     "awaiting_confirmation",
                                     "time_debt_ms",
                                     "balance_count"};
const QStringList app_balances_columns{"token", "balance_kind", "target_ref", "units"};
const QStringList app_head_columns{"singleton", "token"};
const QStringList app_operations_columns{
    "operation_scope",    "operation_ref",    "operation_kind", "subject_kind",
    "receipt_id",         "contribution_id",  "plan_ref",       "plan_revision",
    "head_token",         "before_token",     "after_token",    "advance_explanation",
    "choices_format_ref", "choices_document", "reversal_ref"};
const QStringList app_contributions_columns{
    "operation_scope", "operation_ref",    "contribution_id",    "payment_evidence",
    "monetary",        "original_decimal", "currency",           "resolution_ref",
    "missing_rate",    "review",           "evidence_policy",    "policy_ref",
    "decision_choice", "decision_ref",     "decision_actor_ref", "application",
    "reversal",        "settlement",       "application_after"};
const QStringList app_claims_columns{"subject_kind", "subject_id", "operation_scope",
                                     "operation_ref"};
const QStringList app_effects_columns{"operation_scope",
                                      "operation_ref",
                                      "request_ordinal",
                                      "plan_ref",
                                      "plan_revision",
                                      "arithmetic_role",
                                      "unit",
                                      "exact_numerator",
                                      "exact_denominator",
                                      "requested_units",
                                      "residual_numerator",
                                      "residual_denominator",
                                      "maximum_main_ms",
                                      "deficit_policy",
                                      "deficit_disposition",
                                      "application_explanation"};
const QStringList app_components_columns{"operation_scope", "operation_ref", "request_ordinal",
                                         "component_kind",  "units",         "before_token",
                                         "target_ref"};
const QStringList app_transitions_columns{"operation_scope", "operation_ref", "ordinal",
                                          "transition"};
const QStringList app_jobs_columns{
    "operation_scope", "operation_ref", "ordinal",    "job_ref", "source_request_ordinal",
    "group_ref",       "group_ordinal", "format_ref", "document"};
const QStringList app_results_columns{"operation_scope", "operation_ref",    "format_ref",
                                      "document",        "effect_count",     "component_count",
                                      "job_count",       "transition_count", "reversal_count"};
const QStringList app_audit_columns{"operation_scope", "operation_ref", "actor_ref", "decision_ref",
                                    "reason",          "format_ref",    "document"};
const QStringList app_reversals_columns{"operation_scope",         "operation_ref",
                                        "request_ordinal",         "original_scope",
                                        "original_operation",      "original_request_ordinal",
                                        "original_component_kind", "basis_position",
                                        "fraction_numerator",      "fraction_denominator",
                                        "entitlement_numerator",   "entitlement_denominator",
                                        "residual_numerator",      "residual_denominator",
                                        "previous_cumulative",     "cumulative",
                                        "requested_increment",     "full_refund"};
const QStringList app_reversal_heads_columns{"original_scope",
                                             "original_operation",
                                             "original_request_ordinal",
                                             "original_component_kind",
                                             "basis_position",
                                             "cumulative",
                                             "last_scope",
                                             "last_operation",
                                             "last_request_ordinal"};
// One ephemeral decoder borrows the active caller unit. It never commits/adopts
// or provides destructor cleanup. All known failures establish actual rollback.
class Access {
  public:
    SqliteWorkSession session_;
    SqliteBundleLimits limits_;
    bool wrote_{false};
    Access(SqliteWorkSession s, SqliteBundleLimits l) : session_(std::move(s)), limits_(l) {}
    template <class F> auto guarded(F &&f) -> decltype(f()) {
        try {
            return f();
        } catch (const SqliteBundleStorageFailure &) {
            throw;
        } catch (const SqliteWorkFailure &) {
            throw;
        } catch (...) {
            if (wrote_)
                session_.rollback();
            throw;
        }
    }
    SqlRows execute(const SqlStatement &s) {
        try {
            return session_.execute(s);
        } catch (const SqliteWorkFailure &e) {
            switch (e.error()) {
            case WorkError::busy:
            case WorkError::sql_failure:
            case WorkError::resource_limit:
            case WorkError::result_limit:
                wrote_ = false;
                break;
            default:
                break;
            }
            map_work_failure(e);
        }
    }
    [[noreturn]] void fail(Reason reason) {
        session_.rollback();
        wrote_ = false;
        throw SqliteBundleStorageFailure(reason);
    }
    void shape(const SqlRow &row, qsizetype count) {
        if (row.size() != count * 3)
            fail(Reason::invalid_stored_data);
    }
    bool type(const SqlRow &row, qsizetype index, const char *expected) const {
        return row[index * 3 + 1].metaType().id() == QMetaType::QString &&
               row[index * 3 + 1].toString() == expected;
    }
    bool null(const SqlRow &row, qsizetype index) {
        if (!type(row, index, "null"))
            return false;
        if (!row[index * 3].isNull() || !row[index * 3 + 2].isNull())
            fail(Reason::invalid_stored_data);
        return true;
    }
    qint64 integer(const SqlRow &row, qsizetype index) {
        const auto &value = row[index * 3];
        if (!type(row, index, "integer") || (value.metaType().id() != QMetaType::Int &&
                                             value.metaType().id() != QMetaType::LongLong))
            fail(Reason::invalid_stored_data);
        return value.toLongLong();
    }
    qint64 enumeration(const SqlRow &row, qsizetype index, qint64 maximum) {
        const auto value = integer(row, index);
        if (value < 0 || value > maximum)
            fail(Reason::invalid_stored_data);
        return value;
    }
    std::string string(const SqlRow &row, qsizetype index, qsizetype bound, bool required = false) {
        const auto &value = row[index * 3], &length = row[index * 3 + 2];
        if (!type(row, index, "blob") ||
            (length.metaType().id() != QMetaType::Int &&
             length.metaType().id() != QMetaType::LongLong) ||
            length.toLongLong() < 0)
            fail(Reason::invalid_stored_data);
        const auto size = length.toLongLong();
        if (!(size == 0 && value.isNull()) &&
            (value.metaType().id() != QMetaType::QByteArray || value.toByteArray().size() != size))
            fail(Reason::invalid_stored_data);
        if (required && size == 0)
            fail(Reason::invalid_stored_data);
        if (size > bound)
            fail(Reason::result_limit);
        const auto data = value.toByteArray();
        return size == 0 ? std::string{} : std::string(data.constData(), size_t(data.size()));
    }
    std::uint64_t id(const SqlRow &row, qsizetype index, bool receipt = false) {
        if (!type(row, index, "blob") || row[index * 3 + 2].toLongLong() != 8)
            fail(Reason::invalid_stored_data);
        const auto value = decode_plan_revision_uint64(bytes(string(row, index, 8, true)));
        if (receipt && value == 0)
            fail(Reason::invalid_stored_data);
        return value;
    }
    std::optional<std::string> optional_string(const SqlRow &row, qsizetype index,
                                               qsizetype bound) {
        return null(row, index) ? std::nullopt
                                : std::optional<std::string>(string(row, index, bound));
    }
    std::optional<std::pair<std::string, std::string>> group(const SqlRow &row, qsizetype index,
                                                             qsizetype bound) {
        const bool a = null(row, index), b = null(row, index + 1);
        if (a != b)
            fail(Reason::invalid_stored_data);
        if (a)
            return std::nullopt;
        return std::pair{string(row, index, bound), string(row, index + 1, bound)};
    }
    core::ContributionConditions read_conditions(const SqlRow &row, qsizetype offset) {
        core::ContributionConditions c;
        c.valuation.monetary = enumeration(row, offset, 1) != 0;
        if (auto money = group(row, offset + 1, limits_.value_bytes))
            c.valuation.original_money =
                core::ReceiptMoney{std::move(money->first), std::move(money->second)};
        c.valuation.resolution_ref = optional_string(row, offset + 3, limits_.reference_bytes);
        c.valuation.missing_rate = enumeration(row, offset + 4, 1) != 0;
        c.review = static_cast<core::ContributionReview>(enumeration(row, offset + 5, 3));
        c.evidence_admission.policy =
            static_cast<core::EvidencePolicy>(enumeration(row, offset + 6, 2));
        c.evidence_admission.policy_ref = string(row, offset + 7, limits_.reference_bytes);
        const bool a = null(row, offset + 8), b = null(row, offset + 9), d = null(row, offset + 10);
        if (a != b || a != d)
            fail(Reason::invalid_stored_data);
        if (!a)
            c.evidence_admission.review_decision = core::EvidenceReviewDecision{
                static_cast<core::EvidenceReviewChoice>(enumeration(row, offset + 8, 1)),
                string(row, offset + 9, limits_.reference_bytes),
                string(row, offset + 10, limits_.reference_bytes)};
        c.application =
            static_cast<core::ContributionApplication>(enumeration(row, offset + 11, 1));
        c.reversal = static_cast<core::ContributionReversal>(enumeration(row, offset + 12, 2));
        c.settlement = static_cast<core::SettlementGate>(enumeration(row, offset + 13, 1));
        return c;
    }
    void resolve(const core::ReceiptArrival &arrival) {
        try {
            const SqliteRetainedContextStore retained(
                session_, {limits_.reference_bytes, limits_.value_bytes});
            if (!retained.find({RetainedKind::configuration, arrival.configuration_ref}) ||
                !retained.find({RetainedKind::arrival_context, arrival.modifier_context_ref}))
                fail(Reason::invalid_stored_data);
        } catch (const RetentionFailure &e) {
            // The retained primitive already established rollback; do not replace
            // its outcome with a second control operation on the closed unit.
            if (e.error() == RetentionError::invalid_stored_data)
                throw SqliteBundleStorageFailure(Reason::invalid_stored_data);
            if (e.error() == RetentionError::result_limit)
                throw SqliteBundleStorageFailure(Reason::result_limit);
            throw;
        } catch (const SqliteWorkFailure &e) {
            map_work_failure(e);
        }
    }
    std::optional<ContributionReceipt> original(core::ReceiptId receipt) {
        const auto rows = execute({"SELECT " + projection(receipt_columns) +
                                       " FROM main.subathon_receipts WHERE receipt_id=? LIMIT 2",
                                   {encode_plan_revision_uint64(receipt.value)}});
        if (rows.isEmpty())
            return std::nullopt;
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        const auto &row = rows.front();
        shape(row, 20);
        if (id(row, 0, true) != receipt.value)
            fail(Reason::invalid_stored_data);
        core::ReceiptIdentity identity{{string(row, 1, limits_.reference_bytes, true),
                                        string(row, 2, limits_.reference_bytes, true)},
                                       string(row, 3, limits_.reference_bytes, true),
                                       string(row, 4, limits_.reference_bytes, true)};
        core::ReceiptFacts facts;
        facts.event_type = string(row, 5, limits_.value_bytes);
        const auto schema = integer(row, 6);
        if (schema < 0 || std::uint64_t(schema) > std::numeric_limits<std::uint32_t>::max())
            fail(Reason::invalid_stored_data);
        facts.schema_version = std::uint32_t(schema);
        if (!null(row, 7))
            facts.source_timestamp_utc_ms = integer(row, 7);
        facts.actor_id = optional_string(row, 8, limits_.value_bytes);
        facts.actor_display_name = optional_string(row, 9, limits_.value_bytes);
        if (auto money = group(row, 10, limits_.value_bytes))
            facts.money = core::ReceiptMoney{std::move(money->first), std::move(money->second)};
        if (auto payment = group(row, 12, limits_.reference_bytes))
            facts.payment_reference =
                core::ReceiptLinkReference{std::move(payment->first), std::move(payment->second)};
        if (auto activity = group(row, 14, limits_.reference_bytes))
            facts.activity_reference =
                core::ReceiptLinkReference{std::move(activity->first), std::move(activity->second)};
        core::ReceiptArrival arrival{integer(row, 16), string(row, 17, limits_.reference_bytes),
                                     string(row, 18, limits_.reference_bytes, true),
                                     string(row, 19, limits_.reference_bytes, true)};
        resolve(arrival);
        return ContributionReceipt{receipt, std::move(identity), std::move(facts),
                                   std::move(arrival)};
    }
    void contribution_identity(ContributionId wanted, Reason missing) {
        const auto rows =
            execute({"SELECT " + projection({"contribution_id", "original_receipt_id"}) +
                         " FROM main.subathon_contributions WHERE contribution_id=? LIMIT 2",
                     {encode_plan_revision_uint64(wanted.value)}});
        if (rows.isEmpty())
            fail(missing);
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        shape(rows.front(), 2);
        if (id(rows.front(), 0) != wanted.value)
            fail(Reason::invalid_stored_data);
        const auto receipt = id(rows.front(), 1, true);
        const auto identities =
            execute({"SELECT " +
                         projection({"receipt_id", "provider", "source_account",
                                     "identifier_namespace", "opaque_id"}) +
                         " FROM main.subathon_receipts WHERE receipt_id=? LIMIT 2",
                     {encode_plan_revision_uint64(receipt)}});
        if (identities.size() != 1)
            fail(Reason::invalid_stored_data);
        const auto &row = identities.front();
        shape(row, 5);
        if (id(row, 0, true) != receipt)
            fail(Reason::invalid_stored_data);
        for (qsizetype i = 1; i < 5; ++i)
            (void)string(row, i, limits_.reference_bytes, true);
    }
    void source(const PlanEvent &event) {
        const auto actual = original(event.receipt_id);
        if (!actual)
            fail(Reason::missing_receipt);
        if (event.contribution_id)
            contribution_identity(*event.contribution_id, Reason::missing_contribution);
        if (actual->identity != event.identity || actual->facts != event.facts ||
            actual->arrival != event.arrival)
            fail(Reason::source_mismatch);
    }
    struct Header {
        PlanEvent event;
        std::uint64_t head;
    };
    std::optional<Header> header(const std::string &ref) {
        const auto rows = execute({"SELECT " + projection(header_columns) +
                                       " FROM main.subathon_plan_headers WHERE plan_ref=? LIMIT 2",
                                   {bytes(ref)}});
        if (rows.isEmpty())
            return std::nullopt;
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        const auto &row = rows.front();
        shape(row, 7);
        if (string(row, 0, limits_.reference_bytes, true) != ref)
            fail(Reason::invalid_stored_data);
        PlanEvent e;
        e.event_ref = string(row, 1, limits_.reference_bytes, true);
        e.parent_event_ref = optional_string(row, 2, limits_.reference_bytes);
        e.root_event_ref = string(row, 3, limits_.reference_bytes, true);
        e.receipt_id = {id(row, 4, true)};
        if (!null(row, 5))
            e.contribution_id = ContributionId{id(row, 5)};
        const auto head = id(row, 6, true);
        const auto receipt = original(e.receipt_id);
        if (!receipt)
            fail(Reason::invalid_stored_data);
        e.identity = receipt->identity;
        e.facts = receipt->facts;
        e.arrival = receipt->arrival;
        if (e.contribution_id)
            contribution_identity(*e.contribution_id, Reason::invalid_stored_data);
        return Header{std::move(e), head};
    }
    void key(const SqlRow &row, const std::string &ref, std::uint64_t revision) {
        if (string(row, 0, limits_.reference_bytes, true) != ref || id(row, 1, true) != revision)
            fail(Reason::invalid_stored_data);
    }
    // Aggregate validates every key's type/domain before comparing count/head.
    // No full-read quota is applied to this health check.
    void summary(const std::string &ref, const Header &h) {
        const QString bad =
            "coalesce(sum(CASE WHEN typeof(revision)<>'blob' OR length(revision)<>8 OR "
            "revision=X'0000000000000000' THEN 1 ELSE 0 END),0)";
        const auto rows =
            execute({"SELECT " + projection({"count(*)", "min(revision)", "max(revision)", bad}) +
                         " FROM main.subathon_plan_revisions WHERE plan_ref=?",
                     {bytes(ref)}});
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        const auto &row = rows.front();
        shape(row, 4);
        const auto n = integer(row, 0);
        if (n <= 0 || integer(row, 3) != 0 || id(row, 1, true) != 1 || id(row, 2, true) != h.head ||
            std::uint64_t(n) != h.head)
            fail(Reason::invalid_stored_data);
    }
    void selected_max(const std::string &ref, const Header &h) {
        const auto rows = execute({"SELECT " + projection({"plan_ref", "revision"}) +
                                       " FROM main.subathon_plan_revisions WHERE plan_ref=? ORDER "
                                       "BY revision DESC LIMIT 1",
                                   {bytes(ref)}});
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        shape(rows.front(), 2);
        key(rows.front(), ref, h.head);
    }
    bool dependent(const std::string &ref, std::uint64_t revision) {
        const QString bad =
            "coalesce(sum(CASE WHEN typeof(requires_historical_valuation)<>'integer' OR "
            "requires_historical_valuation NOT IN (0,1) THEN 1 ELSE 0 END),0)";
        const auto rows = execute(
            {"SELECT " +
                 projection({"count(*)", bad, "coalesce(max(requires_historical_valuation),0)"}) +
                 " FROM main.subathon_plan_requests WHERE plan_ref=? AND revision=?",
             {bytes(ref), encode_plan_revision_uint64(revision)}});
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        const auto &row = rows.front();
        shape(row, 3);
        if (integer(row, 0) < 0 || integer(row, 1) != 0)
            fail(Reason::invalid_stored_data);
        return enumeration(row, 2, 1) != 0;
    }
    CapturedPlanRevision scalar(const SqlRow &row, const std::string &ref, const Header &h) {
        shape(row, 34);
        CapturedPlanRevision r;
        r.revision = id(row, 1, true);
        key(row, ref, r.revision);
        if (!null(row, 2))
            r.parent_revision = id(row, 2, true);
        r.captured.event = h.event;
        r.captured.contribution_state = {static_cast<PaymentEvidence>(enumeration(row, 3, 3)),
                                         read_conditions(row, 4)};
        auto &v = r.captured.valuation;
        v.state = static_cast<PlanValuationState>(enumeration(row, 18, 2));
        if (v.state == PlanValuationState::resolved) {
            HistoricalValuation q{
                string(row, 19, limits_.reference_bytes),
                {string(row, 20, limits_.value_bytes), string(row, 21, limits_.value_bytes)},
                string(row, 22, limits_.value_bytes),
                string(row, 23, limits_.value_bytes),
                optional_string(row, 24, limits_.value_bytes),
                string(row, 25, limits_.reference_bytes),
                optional_string(row, 26, limits_.reference_bytes),
                {},
                {},
                optional_string(row, 29, limits_.reference_bytes)};
            if (!null(row, 27))
                q.rate_timestamp_utc_ms = integer(row, 27);
            if (!null(row, 28))
                q.fetch_timestamp_utc_ms = integer(row, 28);
            v.historical = std::move(q);
        } else
            for (qsizetype i = 19; i < 30; ++i)
                if (!null(row, i))
                    fail(Reason::invalid_stored_data);
        if (r.revision == 1) {
            if (r.parent_revision)
                fail(Reason::invalid_stored_data);
            for (qsizetype i = 30; i < 34; ++i)
                if (!null(row, i))
                    fail(Reason::invalid_stored_data);
        } else {
            if (!r.parent_revision || *r.parent_revision != r.revision - 1 ||
                enumeration(row, 30, 2) != 2)
                fail(Reason::invalid_stored_data);
            r.override_decision = PlanOverrideDecision{
                OverrideAuthority::authorized, string(row, 31, limits_.reference_bytes, true),
                string(row, 32, limits_.value_bytes, true),
                string(row, 33, limits_.reference_bytes, true)};
        }
        if (!valid_valuation(v, h.event, dependent(ref, r.revision), limits_))
            fail(Reason::invalid_stored_data);
        return r;
    }
    std::optional<CapturedPlanRevision> selected_scalar(const std::string &ref,
                                                        std::uint64_t revision, const Header &h) {
        const auto rows = execute(
            {"SELECT " + projection(revision_columns) +
                 " FROM main.subathon_plan_revisions WHERE plan_ref=? AND revision=? LIMIT 2",
             {bytes(ref), encode_plan_revision_uint64(revision)}});
        if (rows.isEmpty())
            return std::nullopt;
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        auto r = scalar(rows.front(), ref, h);
        if (r.revision != revision)
            fail(Reason::invalid_stored_data);
        return r;
    }
    void parent_target(const std::string &ref, const CapturedPlanRevision &r) {
        if (!r.parent_revision)
            return;
        const auto rows = execute(
            {"SELECT " + projection({"plan_ref", "revision"}) +
                 " FROM main.subathon_plan_revisions WHERE plan_ref=? AND revision=? LIMIT 2",
             {bytes(ref), encode_plan_revision_uint64(*r.parent_revision)}});
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        shape(rows.front(), 2);
        key(rows.front(), ref, *r.parent_revision);
    }
    // Bounded keyset pages audit old structural metadata only. They do not load
    // or certify historical child strings/ordinals/rationals, nor impose read caps.
    void audit(const std::string &ref, const Header &h) {
        summary(ref, h);
        std::uint64_t previous = 0;
        for (;;) {
            const auto rows = execute({"SELECT " + projection(revision_columns) +
                                           " FROM main.subathon_plan_revisions WHERE plan_ref=? "
                                           "AND revision>? ORDER BY revision LIMIT 65",
                                       {bytes(ref), encode_plan_revision_uint64(previous)}});
            if (rows.isEmpty())
                break;
            for (qsizetype index = 0; index < std::min<qsizetype>(64, rows.size()); ++index) {
                const auto &row = rows[index];
                auto r = scalar(row, ref, h);
                if (previous == std::numeric_limits<std::uint64_t>::max() ||
                    r.revision != previous + 1)
                    fail(Reason::invalid_stored_data);
                previous = r.revision;
            }
            if (rows.size() <= 64)
                break;
        }
        if (previous != h.head)
            fail(Reason::invalid_stored_data);
    }
    SqlRows children(const QString &table, const QStringList &columns, const std::string &ref,
                     std::uint64_t revision) {
        auto rows = execute({"SELECT " + projection(columns) + " FROM main." + table +
                                 " WHERE plan_ref=? AND revision=? ORDER BY ordinal LIMIT ?",
                             {bytes(ref), encode_plan_revision_uint64(revision),
                              qint64(limits_.plan_items_per_vector + 1)}});
        if (rows.size() > limits_.plan_items_per_vector)
            fail(Reason::result_limit);
        qint64 ordinal = 0;
        for (const auto &row : rows) {
            shape(row, columns.size());
            key(row, ref, revision);
            if (integer(row, 2) != ++ordinal)
                fail(Reason::invalid_stored_data);
        }
        return rows;
    }
    // Validate BOTH SQL component shapes before their common size check, then
    // invoke the canonical mathematical decoder; no repair/normalization on read.
    Exact exact(const SqlRow &row, qsizetype n, qsizetype d) {
        for (const auto i : {n, d}) {
            const auto &length = row[i * 3 + 2], &value = row[i * 3];
            if (!type(row, i, "blob") ||
                (length.metaType().id() != QMetaType::Int &&
                 length.metaType().id() != QMetaType::LongLong) ||
                length.toLongLong() <= 0 || value.metaType().id() != QMetaType::QByteArray ||
                value.toByteArray().size() != length.toLongLong())
                fail(Reason::invalid_stored_data);
        }
        if (row[n * 3 + 2].toLongLong() > limits_.component_bytes ||
            row[d * 3 + 2].toLongLong() > limits_.component_bytes)
            fail(Reason::result_limit);
        try {
            return decode_plan_exact({row[n * 3].toByteArray(), row[d * 3].toByteArray()},
                                     limits_.component_bytes);
        } catch (const SqlitePlanInputFailure &) {
            fail(Reason::invalid_stored_data);
        }
    }
    void body(const std::string &ref, CapturedPlanRevision &r) {
        const auto rules = children("subathon_plan_rules", rule_columns, ref, r.revision);
        r.rules.reserve(size_t(rules.size()));
        for (const auto &row : rules)
            r.rules.push_back({string(row, 3, limits_.reference_bytes),
                               string(row, 4, limits_.reference_bytes), enumeration(row, 5, 1) != 0,
                               string(row, 6, limits_.value_bytes)});
        const auto modifiers =
            children("subathon_plan_modifiers", modifier_columns, ref, r.revision);
        r.modifiers.reserve(size_t(modifiers.size()));
        for (const auto &row : modifiers)
            r.modifiers.push_back(
                {string(row, 3, limits_.reference_bytes), string(row, 4, limits_.reference_bytes),
                 string(row, 5, limits_.reference_bytes), string(row, 6, limits_.reference_bytes),
                 string(row, 7, limits_.reference_bytes), string(row, 8, limits_.reference_bytes),
                 string(row, 9, limits_.value_bytes)});
        const auto requests = children("subathon_plan_requests", request_columns, ref, r.revision);
        r.captured.requested_effects.reserve(size_t(requests.size()));
        for (const auto &row : requests)
            r.captured.requested_effects.push_back(
                {string(row, 3, limits_.reference_bytes), string(row, 4, limits_.reference_bytes),
                 string(row, 5, limits_.reference_bytes), string(row, 6, limits_.reference_bytes),
                 static_cast<RequestedUnit>(enumeration(row, 7, 2)), enumeration(row, 8, 1) != 0,
                 exact(row, 9, 10), integer(row, 11), string(row, 12, limits_.value_bytes),
                 string(row, 13, limits_.reference_bytes)});
    }
    std::optional<VerifiedActivityIdentity> read_activity(const SqlRow &row, qsizetype offset) {
        bool absent = null(row, offset);
        for (qsizetype i = 1; i < 4; ++i)
            if (null(row, offset + i) != absent)
                fail(Reason::invalid_stored_data);
        if (absent)
            return std::nullopt;
        return VerifiedActivityIdentity{{string(row, offset, limits_.reference_bytes, true),
                                         string(row, offset + 1, limits_.reference_bytes, true)},
                                        string(row, offset + 2, limits_.reference_bytes, true),
                                        string(row, offset + 3, limits_.reference_bytes, true)};
    }
    ContributionEvidenceRecord read_observation(const SqlRow &row) {
        shape(row, 23);
        const core::ReceiptId receipt{id(row, 0, true)};
        auto source = original(receipt);
        if (!source)
            fail(Reason::invalid_stored_data);
        std::optional<core::ContributionId> associated;
        if (!null(row, 1))
            associated = core::ContributionId{id(row, 1)};
        const auto kind = static_cast<ContributionObservationKind>(enumeration(row, 2, 1));
        const auto evidence = static_cast<core::PaymentEvidence>(enumeration(row, 3, 3));
        auto activity = read_activity(row, 4);
        std::optional<VerifiedActivityLink> verified;
        if (activity) {
            if (activity->origin != source->identity.origin)
                fail(Reason::invalid_stored_data);
            verified = VerifiedActivityLink{std::move(*activity),
                                            string(row, 8, limits_.reference_bytes, true)};
        } else if (!null(row, 8))
            fail(Reason::invalid_stored_data);
        std::optional<core::ContributionConditions> conditions;
        if (kind == ContributionObservationKind::support)
            conditions = read_conditions(row, 9);
        else {
            if (evidence != core::PaymentEvidence::confirmed)
                fail(Reason::invalid_stored_data);
            for (qsizetype i = 9; i < 23; ++i)
                if (!null(row, i))
                    fail(Reason::invalid_stored_data);
        }
        return {{std::move(*source), kind, evidence, std::move(verified), std::move(conditions)},
                associated};
    }
    bool compatible(const ContributionObservation &o, const ContributionRecord &c) const {
        if (o.verified_activity)
            return c.activity_identity ==
                   std::optional<VerifiedActivityIdentity>(o.verified_activity->identity);
        return !c.activity_identity && o.kind == ContributionObservationKind::support &&
               o.receipt.id == c.original_receipt_id;
    }
    bool anchor_matches(const ContributionRecord &c,
                        const std::optional<ContributionEvidenceRecord> &o) const {
        return o && o->contribution_id == std::optional<core::ContributionId>(c.id) &&
               o->observation.kind == ContributionObservationKind::support &&
               o->observation.support_conditions && compatible(o->observation, c) &&
               o->observation.receipt.id == c.original_receipt_id &&
               o->observation.receipt.facts == c.original_facts &&
               o->observation.receipt.arrival == c.original_arrival;
    }
    std::optional<ContributionRecord> contribution(const QString &where, QList<QVariant> bindings,
                                                   bool anchor = true) {
        const auto rows =
            execute({"SELECT " + projection(contribution_columns()) +
                         " FROM main.subathon_contributions WHERE " + where + " LIMIT 2",
                     std::move(bindings)});
        if (rows.isEmpty())
            return std::nullopt;
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        const auto &row = rows.front();
        shape(row, 21);
        const core::ContributionId cid{id(row, 0)};
        const core::ReceiptId rid{id(row, 1, true)};
        auto key = read_activity(row, 2);
        auto receipt = original(rid);
        if (!receipt || (key && key->origin != receipt->identity.origin))
            fail(Reason::invalid_stored_data);
        core::ContributionState state{static_cast<core::PaymentEvidence>(enumeration(row, 6, 3)),
                                      read_conditions(row, 7)};
        ContributionRecord result{cid,
                                  std::move(key),
                                  rid,
                                  std::move(receipt->facts),
                                  std::move(receipt->arrival),
                                  std::move(state)};
        if (anchor) {
            const auto support = observation(rid, false);
            {
                if (!anchor_matches(result, support))
                    fail(Reason::invalid_stored_data);
            }
        }
        return result;
    }
    std::optional<ContributionRecord> contribution_by_id(core::ContributionId cid) {
        auto result = contribution("contribution_id=?", {encode_contribution_id(cid)});
        if (result && result->id != cid)
            fail(Reason::invalid_stored_data);
        return result;
    }
    std::optional<ContributionEvidenceRecord> observation(core::ReceiptId rid,
                                                          bool validate_association) {
        const auto rows =
            execute({"SELECT " + projection(observation_columns()) +
                         " FROM main.subathon_contribution_observations WHERE receipt_id=? LIMIT 2",
                     {encode_plan_revision_uint64(rid.value)}});
        if (rows.isEmpty())
            return std::nullopt;
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        auto result = read_observation(rows.front());
        if (result.observation.receipt.id != rid)
            fail(Reason::invalid_stored_data);
        if (validate_association && result.contribution_id) {
            const auto target = contribution_by_id(*result.contribution_id);
            if (!target || !compatible(result.observation, *target))
                fail(Reason::invalid_stored_data);
        }
        return result;
    }
    using Values = QList<QVariant>;
    std::optional<SqlRow> one(const QString &table, const QStringList &cols, const QString &where,
                              Values values, bool required = false) {
        auto rows = execute(
            {"SELECT " + projection(cols) + " FROM main." + table + " WHERE " + where + " LIMIT 2",
             std::move(values)});
        if (rows.isEmpty()) {
            if (required)
                fail(Reason::invalid_stored_data);
            return std::nullopt;
        }
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        shape(rows.front(), cols.size());
        return rows.front();
    }
    static Values operation_values(const ApplicationOperationKey &k) {
        return {bytes(k.operation_scope), bytes(k.operation_ref)};
    }
    static QString operation_where() { return "operation_scope=? AND operation_ref=?"; }
    void operation_key(const SqlRow &row, const ApplicationOperationKey &key_) {
        if (string(row, 0, limits_.reference_bytes, true) != key_.operation_scope ||
            string(row, 1, limits_.reference_bytes, true) != key_.operation_ref)
            fail(Reason::invalid_stored_data);
    }
    void config(const std::string &ref) {
        try {
            if (!SqliteRetainedContextStore(session_,
                                            {limits_.reference_bytes, limits_.value_bytes})
                     .find({RetainedKind::configuration, ref}))
                fail(Reason::invalid_stored_data);
        } catch (const RetentionFailure &e) {
            if (e.error() == RetentionError::result_limit)
                throw SqliteBundleStorageFailure(Reason::result_limit);
            if (e.error() == RetentionError::invalid_stored_data)
                throw SqliteBundleStorageFailure(Reason::invalid_stored_data);
            throw;
        } catch (const SqliteWorkFailure &e) {
            map_work_failure(e);
        }
    }
    void result_size(size_t items, size_t total_bytes) {
        if (items > size_t(limits_.result_items) || total_bytes > size_t(limits_.owned_bytes))
            fail(Reason::result_limit);
    }
    template <class F> void result_bytes(size_t items, F f) {
        if (items > size_t(limits_.result_items))
            fail(Reason::result_limit);
        try {
            bundle_detail::ByteCount n(size_t(limits_.owned_bytes));
            f(n);
        } catch (const SqliteBundleInputFailure &) {
            fail(Reason::result_limit);
        }
    }
    size_t add_result_items(size_t a, size_t b) {
        if (b > std::numeric_limits<size_t>::max() - a)
            fail(Reason::result_limit);
        return a + b;
    }
    size_t plan_items(const CapturedPlanRevision &p) {
        return add_result_items(add_result_items(p.rules.size(), p.modifiers.size()),
                                p.captured.requested_effects.size());
    }
    struct Plan {
        CapturedPlanRevision selected;
        std::uint64_t head;
    };
    std::optional<Plan> plan(const ApplicationPlanKey &key_, bool current_only = false) {
        auto h = header(key_.plan_ref);
        if (!h)
            return std::nullopt;
        audit(key_.plan_ref, *h);
        if (current_only && h->head != key_.revision)
            return Plan{{}, h->head};
        auto r = selected_scalar(key_.plan_ref, key_.revision, *h);
        if (!r)
            return std::nullopt;
        body(key_.plan_ref, *r);
        return Plan{std::move(*r), h->head};
    }
    ApplicationEnvelope envelope(const SqlRow &r, qsizetype pos) {
        return {string(r, pos, limits_.reference_bytes, true),
                bytes(string(r, pos + 1, limits_.value_bytes))};
    }
    ApplicationStateSnapshot state(const std::string &token, bool metadata = false) {
        const auto rows =
            one("subathon_app_states", app_states_columns, "token=?", {bytes(token)}, true);
        const auto &r = *rows;
        ApplicationStateSnapshot s;
        s.token = string(r, 0, limits_.reference_bytes, true);
        if (s.token != token)
            fail(Reason::invalid_stored_data);
        s.envelope = envelope(r, 1);
        s.configuration_ref = string(r, 3, limits_.reference_bytes, true);
        s.timer = {static_cast<TimerLifecycle>(enumeration(r, 4, 2)),
                   static_cast<CountdownPhase>(enumeration(r, 5, 1)), integer(r, 6), integer(r, 7),
                   enumeration(r, 8, 1) != 0};
        s.time_debt_ms = integer(r, 9);
        const auto count = integer(r, 10);
        if (count < 0 || s.timer.main_remaining_ms < 0 || s.timer.grace_remaining_ms < 0 ||
            s.time_debt_ms < 0 ||
            (s.timer.phase == CountdownPhase::Grace &&
             (s.timer.main_remaining_ms != 0 || s.timer.awaiting_confirmation)) ||
            (s.timer.awaiting_confirmation &&
             (s.timer.lifecycle != TimerLifecycle::Active ||
              s.timer.phase != CountdownPhase::Main || s.timer.main_remaining_ms != 0)))
            fail(Reason::invalid_stored_data);
        config(s.configuration_ref);
        if (!metadata && count > limits_.balances_per_state)
            fail(Reason::result_limit);
        qint64 seen = 0;
        qint64 previous_kind = 1;
        std::string previous;
        for (;;) {
            auto rows_ = execute(
                {"SELECT " + projection(app_balances_columns) +
                     " FROM main.subathon_app_balances WHERE token=? AND (balance_kind>? OR "
                     "(balance_kind=? AND target_ref>?)) ORDER BY balance_kind,target_ref LIMIT 65",
                 {bytes(token), previous_kind, previous_kind, bytes(previous)}});
            const auto n = std::min<qsizetype>(64, rows_.size());
            for (qsizetype i = 0; i < n; ++i) {
                const auto &v = rows_[i];
                shape(v, 4);
                if (string(v, 0, limits_.reference_bytes, true) != token)
                    fail(Reason::invalid_stored_data);
                const auto kind = enumeration(v, 1, 3);
                if (kind < 2)
                    fail(Reason::invalid_stored_data);
                auto target = string(v, 2, limits_.reference_bytes, true);
                if (kind < previous_kind ||
                    (kind == previous_kind && bytes(target) <= bytes(previous)))
                    fail(Reason::invalid_stored_data);
                previous_kind = kind;
                previous = target;
                if (seen == std::numeric_limits<qint64>::max())
                    fail(Reason::invalid_stored_data);
                ++seen;
                if (!metadata)
                    s.balances.push_back({static_cast<ApplicationBalanceKind>(kind),
                                          std::move(target), integer(v, 3)});
            }
            if (rows_.size() <= 64)
                break;
        }
        auto actual_balances = execute(
            {"SELECT count(*) FROM main.subathon_app_balances WHERE token=?", {bytes(token)}});
        if (seen != count || actual_balances.size() != 1 ||
            actual_balances.front().front().toLongLong() != seen)
            fail(Reason::invalid_stored_data);
        return s;
    }
    std::optional<ApplicationStateSnapshot> current() {
        auto rows = execute(
            {"SELECT " + projection(app_head_columns) + " FROM main.subathon_app_head LIMIT 2",
             {}});
        if (rows.isEmpty()) {
            auto count = execute({"SELECT count(*) FROM main.subathon_app_states", {}});
            if (count.size() != 1 || count.front().size() != 1 ||
                count.front().front().toLongLong() != 0)
                fail(Reason::invalid_stored_data);
            return std::nullopt;
        }
        if (rows.size() != 1)
            fail(Reason::invalid_stored_data);
        shape(rows.front(), 2);
        if (integer(rows.front(), 0) != 1)
            fail(Reason::invalid_stored_data);
        return state(string(rows.front(), 1, limits_.reference_bytes, true));
    }
    void write(std::vector<SqlStatement> prepared) {
        wrote_ = true;
        for (const auto &step : prepared)
            (void)execute(step);
    }
    void prepare_state(std::vector<SqlStatement> &out, const ApplicationStateSnapshot &s) {
        out.push_back(
            insertion("subathon_app_states", app_states_columns,
                      {bytes(s.token), bytes(s.envelope.format_ref), s.envelope.document,
                       bytes(s.configuration_ref), int(s.timer.lifecycle), int(s.timer.phase),
                       qint64(s.timer.main_remaining_ms), qint64(s.timer.grace_remaining_ms),
                       int(s.timer.awaiting_confirmation), qint64(s.time_debt_ms),
                       qint64(s.balances.size())}));
        for (const auto &v : s.balances)
            out.push_back(
                insertion("subathon_app_balances", app_balances_columns,
                          {bytes(s.token), int(v.kind), bytes(v.target_ref), qint64(v.units)}));
    }
    SqliteBundleInitialDisposition initial(const ApplicationStateSnapshot &s) {
        auto old = current();
        if (old)
            return bundle_detail::same_state(*old, s)
                       ? SqliteBundleInitialDisposition::already_present
                       : SqliteBundleInitialDisposition::conflict;
        config(s.configuration_ref);
        std::vector<SqlStatement> out;
        prepare_state(out, s);
        out.push_back(insertion("subathon_app_head", app_head_columns, {1, bytes(s.token)}));
        write(std::move(out));
        return SqliteBundleInitialDisposition::inserted;
    }
    ApplicationBundleInputResult inputs(const ApplicationPlanKey &key_,
                                        ApplicationSubjectKind subject) {
        auto p = plan(key_, true);
        if (!p)
            return {};
        if (p->head != key_.revision)
            return {SqliteBundleInputDisposition::stale_plan, std::nullopt};
        const auto &e = p->selected.captured.event;
        if ((subject == ApplicationSubjectKind::contribution) != bool(e.contribution_id))
            fail(Reason::source_mismatch);
        auto s = current();
        if (!s)
            fail(Reason::missing_state);
        ApplicationBundleInputs out{subject,       key_, p->head, std::move(p->selected),
                                    std::move(*s), {}};
        if (e.contribution_id) {
            auto c = contribution_by_id(*e.contribution_id);
            if (!c)
                fail(Reason::missing_contribution);
            auto o = observation(c->original_receipt_id, false);
            if (!o || !anchor_matches(*c, o))
                fail(Reason::invalid_stored_data);
            out.current_contribution = ApplicationContributionInput{std::move(*c), std::move(*o)};
        }
        result_bytes(
            add_result_items(plan_items(out.selected_plan), out.current_state.balances.size()),
            [&](auto &n) {
                n.add(1 + 8);
                bundle_detail::count_bytes(n, out.plan);
                bundle_detail::count_bytes(n, out.selected_plan);
                bundle_detail::count_bytes(n, out.current_state);
                if (out.current_contribution)
                    bundle_detail::count_bytes(n, *out.current_contribution);
            });
        return {SqliteBundleInputDisposition::ready, std::move(out)};
    }
    struct Fixed {
        RecordedApplicationBundle record;
        Plan source;
        std::array<qint64, 5> counts{};
    };
    std::optional<Fixed> fixed(const ApplicationOperationKey &k, bool metadata) {
        auto op = one("subathon_app_operations", app_operations_columns, operation_where(),
                      operation_values(k));
        if (!op)
            return std::nullopt;
        const auto &r = *op;
        operation_key(r, k);
        Fixed f;
        auto &v = f.record;
        auto &i = v.intent;
        i.operation = k;
        i.kind = static_cast<ApplicationOperationKind>(enumeration(r, 2, 1));
        i.subject_kind = static_cast<ApplicationSubjectKind>(enumeration(r, 3, 1));
        i.receipt_id = {id(r, 4, true)};
        if (!null(r, 5))
            i.contribution_id = ContributionId{id(r, 5)};
        if ((i.subject_kind == ApplicationSubjectKind::contribution) != bool(i.contribution_id))
            fail(Reason::invalid_stored_data);
        i.plan = {string(r, 6, limits_.reference_bytes, true), id(r, 7, true)};
        i.expected_state_token = string(r, 8, limits_.reference_bytes, true);
        const auto before = string(r, 9, limits_.reference_bytes, true),
                   after = string(r, 10, limits_.reference_bytes, true);
        if (after == before || after == i.expected_state_token)
            fail(Reason::invalid_stored_data);
        v.advance_explanation = string(r, 11, limits_.value_bytes);
        i.choices = envelope(r, 12);
        i.reversal_ref = optional_string(r, 14, limits_.reference_bytes);
        if ((i.kind == ApplicationOperationKind::reversal) != bool(i.reversal_ref) ||
            (i.reversal_ref && i.reversal_ref->empty()))
            fail(Reason::invalid_stored_data);
        v.persisted_head = state(i.expected_state_token, metadata);
        v.advanced_before = state(before, metadata);
        v.after = state(after, metadata);
        auto p = plan(i.plan);
        if (!p)
            fail(Reason::invalid_stored_data);
        f.source = std::move(*p);
        const auto &event = f.source.selected.captured.event;
        if (event.receipt_id != i.receipt_id || event.contribution_id != i.contribution_id)
            fail(Reason::invalid_stored_data);
        auto cs = one("subathon_app_contributions", app_contributions_columns, operation_where(),
                      operation_values(k));
        if (bool(cs) != bool(i.contribution_id))
            fail(Reason::invalid_stored_data);
        if (cs) {
            operation_key(*cs, k);
            if (id(*cs, 2) != i.contribution_id->value)
                fail(Reason::invalid_stored_data);
            ContributionState c{static_cast<PaymentEvidence>(enumeration(*cs, 3, 3)),
                                read_conditions(*cs, 4)};
            i.expected_contribution_state = c;
            v.contribution = ApplicationContributionSnapshot{
                *i.contribution_id, std::move(c),
                static_cast<ContributionApplication>(enumeration(*cs, 18, 1))};
            if (v.contribution->application_after !=
                (i.kind == ApplicationOperationKind::original
                     ? ContributionApplication::applied
                     : v.contribution->before.conditions.application))
                fail(Reason::invalid_stored_data);
            contribution_identity(*i.contribution_id, Reason::invalid_stored_data);
        }
        auto claim = one("subathon_app_claims", app_claims_columns,
                         "operation_scope=? AND operation_ref=?", operation_values(k));
        if (i.kind == ApplicationOperationKind::original) {
            if (!claim)
                fail(Reason::invalid_stored_data);
            if (enumeration(*claim, 0, 1) != int(i.subject_kind) ||
                id(*claim, 1, i.subject_kind == ApplicationSubjectKind::receipt) !=
                    (i.contribution_id ? i.contribution_id->value : i.receipt_id.value) ||
                string(*claim, 2, limits_.reference_bytes, true) != k.operation_scope ||
                string(*claim, 3, limits_.reference_bytes, true) != k.operation_ref)
                fail(Reason::invalid_stored_data);
        } else if (claim)
            fail(Reason::invalid_stored_data);
        auto result = one("subathon_app_results", app_results_columns, operation_where(),
                          operation_values(k), true);
        operation_key(*result, k);
        v.result = envelope(*result, 2);
        for (qsizetype n = 0; n < 5; ++n) {
            f.counts[size_t(n)] = integer(*result, 4 + n);
            if (f.counts[size_t(n)] < 0)
                fail(Reason::invalid_stored_data);
        }
        auto audit_ = one("subathon_app_audit", app_audit_columns, operation_where(),
                          operation_values(k), true);
        operation_key(*audit_, k);
        v.audit = {optional_string(*audit_, 2, limits_.reference_bytes),
                   optional_string(*audit_, 3, limits_.reference_bytes),
                   optional_string(*audit_, 4, limits_.value_bytes), envelope(*audit_, 5)};
        // A narrow source projection validates all state target identities without
        // materializing unrelated historical balance amounts.
        if (metadata) {
            if (v.persisted_head.configuration_ref != v.advanced_before.configuration_ref ||
                v.after.configuration_ref != v.advanced_before.configuration_ref ||
                v.persisted_head.envelope.format_ref != v.advanced_before.envelope.format_ref ||
                v.after.envelope.format_ref != v.advanced_before.envelope.format_ref)
                fail(Reason::invalid_stored_data);
            for (const auto &token : {v.persisted_head.token, v.after.token}) {
                auto mismatch = execute(
                    {"SELECT count(*) FROM (SELECT balance_kind,target_ref FROM "
                     "main.subathon_app_balances WHERE token=? EXCEPT SELECT "
                     "balance_kind,target_ref FROM main.subathon_app_balances WHERE token=?)",
                     {bytes(token), bytes(before)}});
                auto reverse = execute(
                    {"SELECT count(*) FROM (SELECT balance_kind,target_ref FROM "
                     "main.subathon_app_balances WHERE token=? EXCEPT SELECT "
                     "balance_kind,target_ref FROM main.subathon_app_balances WHERE token=?)",
                     {bytes(before), bytes(token)}});
                if (mismatch.front().front().toLongLong() != 0 ||
                    reverse.front().front().toLongLong() != 0)
                    fail(Reason::invalid_stored_data);
            }
        }
        return f;
    }
    template <class F>
    qint64 ordinal_rows(const QString &table, const QStringList &columns,
                        const ApplicationOperationKey &k, F fn) {
        qint64 previous = 0;
        for (;;) {
            auto values = operation_values(k);
            values.append(previous);
            auto rows = execute({"SELECT " + projection(columns) + " FROM main." + table +
                                     " WHERE " + operation_where() + " AND " + columns[2] +
                                     ">? ORDER BY " + columns[2] + " LIMIT 65",
                                 std::move(values)});
            const auto n = std::min<qsizetype>(64, rows.size());
            for (qsizetype x = 0; x < n; ++x) {
                const auto &r = rows[x];
                shape(r, columns.size());
                operation_key(r, k);
                if (previous == std::numeric_limits<qint64>::max() || integer(r, 2) != previous + 1)
                    fail(Reason::invalid_stored_data);
                ++previous;
                fn(r);
            }
            if (rows.size() <= 64)
                break;
        }
        auto actual = execute({"SELECT count(*) FROM main." + table + " WHERE " + operation_where(),
                               operation_values(k)});
        if (actual.size() != 1 || actual.front().front().toLongLong() != previous)
            fail(Reason::invalid_stored_data);
        return previous;
    }
    ApplicationEffect effect(const SqlRow &r, const ApplicationPlanKey &p) {
        if (string(r, 3, limits_.reference_bytes, true) != p.plan_ref ||
            id(r, 4, true) != p.revision)
            fail(Reason::invalid_stored_data);
        ApplicationEffect e;
        e.request_ordinal = integer(r, 2);
        e.arithmetic_role = static_cast<AR>(enumeration(r, 5, 2));
        e.unit = static_cast<RequestedUnit>(enumeration(r, 6, 2));
        e.exact_requested_units = exact(r, 7, 8);
        e.requested_units = integer(r, 9);
        e.rounding_residual = exact(r, 10, 11);
        if (!null(r, 12))
            e.maximum_main_ms = integer(r, 12);
        if (!null(r, 13))
            e.deficit_policy = static_cast<RefundDeficitPolicy>(enumeration(r, 13, 2));
        if (!null(r, 14))
            e.deficit_disposition = static_cast<TimeReversalDisposition>(enumeration(r, 14, 2));
        e.application_explanation = string(r, 15, limits_.value_bytes);
        return e;
    }
    std::vector<ApplicationComponent> components(const ApplicationOperationKey &k,
                                                 ApplicationEffect &e, const std::string &before,
                                                 bool full) {
        auto values = operation_values(k);
        values.append(qint64(e.request_ordinal));
        auto cols = app_components_columns;
        if (!full)
            cols = {"operation_scope", "operation_ref", "request_ordinal",
                    "component_kind",  "before_token",  "target_ref"};
        auto rows = execute({"SELECT " + projection(cols) +
                                 " FROM main.subathon_app_components WHERE " + operation_where() +
                                 " AND request_ordinal=? ORDER BY component_kind LIMIT 4",
                             std::move(values)});
        const std::vector<int> expected =
            e.arithmetic_role == AR::direct
                ? std::vector<int>{e.unit == RequestedUnit::micro_points ? 2 : 3}
            : e.arithmetic_role == AR::time_addition ? std::vector<int>{0, 1, 5}
                                                     : std::vector<int>{0, 1, 4};
        if (rows.size() != qsizetype(expected.size()))
            fail(Reason::invalid_stored_data);
        std::vector<ApplicationComponent> out;
        for (qsizetype n = 0; n < rows.size(); ++n) {
            const auto &r = rows[n];
            shape(r, cols.size());
            operation_key(r, k);
            if (integer(r, 2) != e.request_ordinal || enumeration(r, 3, 5) != expected[size_t(n)] ||
                string(r, full ? 5 : 4, limits_.reference_bytes, true) != before)
                fail(Reason::invalid_stored_data);
            ApplicationComponent c{static_cast<CK>(expected[size_t(n)]), full ? integer(r, 4) : 0,
                                   optional_string(r, full ? 6 : 5, limits_.reference_bytes)};
            if ((int(c.kind) == 2 || int(c.kind) == 3) != bool(c.target_ref) ||
                (c.target_ref && c.target_ref->empty()))
                fail(Reason::invalid_stored_data);
            if (c.target_ref) {
                auto target = one("subathon_app_balances", app_balances_columns,
                                  "token=? AND balance_kind=? AND target_ref=?",
                                  {bytes(before), int(c.kind), bytes(*c.target_ref)}, true);
                if (string(*target, 0, limits_.reference_bytes, true) != before ||
                    enumeration(*target, 1, 3) != int(c.kind) ||
                    string(*target, 2, limits_.reference_bytes, true) != *c.target_ref)
                    fail(Reason::invalid_stored_data);
            }
            out.push_back(std::move(c));
        }
        return out;
    }
    void children(Fixed &f, std::optional<qint64> selected = {}) {
        auto &v = f.record;
        const auto &k = v.intent.operation;
        const bool full = !selected;
        const auto &reqs = f.source.selected.captured.requested_effects;
        if (full &&
            (f.counts[0] > limits_.effects_per_bundle || f.counts[2] > limits_.jobs_per_bundle ||
             f.counts[3] > limits_.transitions_per_bundle ||
             f.counts[4] > limits_.links_per_bundle))
            fail(Reason::result_limit);
        qint64 component_count = 0;
        const QStringList meta_effects{
            "operation_scope", "operation_ref", "request_ordinal", "plan_ref", "plan_revision",
            "arithmetic_role", "unit"};
        auto count = ordinal_rows("subathon_app_effects", meta_effects, k, [&](const SqlRow &r) {
            const auto pos = integer(r, 2);
            if (string(r, 3, limits_.reference_bytes, true) != v.intent.plan.plan_ref ||
                id(r, 4, true) != v.intent.plan.revision || pos < 1 || size_t(pos) > reqs.size())
                fail(Reason::invalid_stored_data);
            ApplicationEffect e;
            e.request_ordinal = pos;
            e.arithmetic_role = static_cast<AR>(enumeration(r, 5, 2));
            e.unit = static_cast<RequestedUnit>(enumeration(r, 6, 2));
            if ((e.arithmetic_role == AR::direct) == (e.unit == RequestedUnit::milliseconds))
                fail(Reason::invalid_stored_data);
            const bool take = full || *selected == pos;
            if (take) {
                auto b = operation_values(k);
                b.append(pos);
                auto row = one("subathon_app_effects", app_effects_columns,
                               operation_where() + " AND request_ordinal=?", std::move(b), true);
                e = effect(*row, v.intent.plan);
            }
            e.components = components(k, e, v.advanced_before.token, take);
            if (e.components.size() >
                std::uintmax_t(std::numeric_limits<qint64>::max() - component_count))
                fail(Reason::invalid_stored_data);
            component_count += qint64(e.components.size());
            if (take) {
                const auto &q = reqs[size_t(pos - 1)];
                if (e.unit != q.unit || e.exact_requested_units != q.exact_requested_units ||
                    e.requested_units != q.requested_units)
                    fail(Reason::invalid_stored_data);
                for (const auto &c : e.components)
                    if (c.target_ref && *c.target_ref != q.target_ref)
                        fail(Reason::invalid_stored_data);
                v.effects.push_back(std::move(e));
            }
        });
        auto actual_components =
            execute({"SELECT count(*) FROM main.subathon_app_components WHERE " + operation_where(),
                     operation_values(k)});
        if (count != f.counts[0] || size_t(count) != reqs.size() ||
            component_count != f.counts[1] || actual_components.size() != 1 ||
            actual_components.front().front().toLongLong() != component_count)
            fail(Reason::invalid_stored_data);
        const QStringList meta_jobs{"operation_scope", "operation_ref",          "ordinal",
                                    "job_ref",         "source_request_ordinal", "group_ref",
                                    "group_ordinal"};
        const auto jobs = ordinal_rows(
            "subathon_app_jobs", full ? app_jobs_columns : meta_jobs, k, [&](const SqlRow &r) {
                ApplicationJobIntent j;
                j.job_ref = string(r, 3, limits_.reference_bytes, true);
                if (!null(r, 4)) {
                    auto n = integer(r, 4);
                    if (n < 1 || n > count)
                        fail(Reason::invalid_stored_data);
                    j.source_request_ordinal = n;
                }
                if (null(r, 5) != null(r, 6))
                    fail(Reason::invalid_stored_data);
                if (!null(r, 5)) {
                    j.group_ref = string(r, 5, limits_.reference_bytes, true);
                    j.group_ordinal = integer(r, 6);
                    if (*j.group_ordinal < 1)
                        fail(Reason::invalid_stored_data);
                    // Group order is checked by SQL ordinal count/min/max below, not an
                    // unbounded materialization vector or a delivery policy.
                }
                if (full) {
                    j.envelope = envelope(r, 7);
                    v.jobs.push_back(std::move(j));
                }
            });
        if (jobs != f.counts[2])
            fail(Reason::invalid_stored_data);
        auto grouped = execute(
            {"SELECT count(*) FROM (SELECT group_ref FROM main.subathon_app_jobs WHERE " +
                 operation_where() +
                 " AND group_ref IS NOT NULL GROUP BY group_ref HAVING min(group_ordinal)<>1 OR "
                 "max(group_ordinal)<>count(*) OR count(DISTINCT group_ordinal)<>count(*))",
             operation_values(k)});
        auto unique_jobs =
            execute({"SELECT count(DISTINCT job_ref) FROM main.subathon_app_jobs WHERE " +
                         operation_where(),
                     operation_values(k)});
        if (grouped.front().front().toLongLong() != 0 ||
            unique_jobs.front().front().toLongLong() != jobs)
            fail(Reason::invalid_stored_data);
        auto transitions = ordinal_rows(
            "subathon_app_transitions", app_transitions_columns, k, [&](const SqlRow &r) {
                auto t = static_cast<TimerTransition>(enumeration(r, 3, 5));
                if (full)
                    v.transitions.push_back(t);
            });
        if (transitions != f.counts[3])
            fail(Reason::invalid_stored_data);
        const auto reverse_count =
            execute({"SELECT count(*) FROM main.subathon_app_reversals WHERE " + operation_where(),
                     operation_values(k)});
        if (reverse_count.size() != 1 ||
            reverse_count.front().front().toLongLong() != f.counts[4] ||
            (v.intent.kind == ApplicationOperationKind::original && f.counts[4] != 0))
            fail(Reason::invalid_stored_data);
    }
    static Values basis_values(const ApplicationOriginalComponent &o) {
        auto v = operation_values(o.operation);
        v.append(qint64(o.request_ordinal));
        v.append(int(o.component_kind));
        return v;
    }
    static QString basis_where() {
        return "original_scope=? AND original_operation=? AND original_request_ordinal=? AND "
               "original_component_kind=?";
    }
    ApplicationReversalOccurrence occurrence(const SqlRow &r, const ApplicationOperationKey &k) {
        operation_key(r, k);
        ApplicationReversalOccurrence o;
        o.request_ordinal = integer(r, 2);
        o.original = {{string(r, 3, limits_.reference_bytes, true),
                       string(r, 4, limits_.reference_bytes, true)},
                      integer(r, 5),
                      static_cast<CK>(enumeration(r, 6, 3))};
        o.basis_position = integer(r, 7);
        o.cumulative_fraction = exact(r, 8, 9);
        o.exact_entitlement = exact(r, 10, 11);
        o.rounding_residual = exact(r, 12, 13);
        o.previous_cumulative = integer(r, 14);
        o.cumulative = integer(r, 15);
        o.requested_increment = integer(r, 16);
        o.full_refund = enumeration(r, 17, 1) != 0;
        if (o.request_ordinal < 1 || o.original.request_ordinal < 1 ||
            o.original.component_kind == CK::debt_delta || o.basis_position < 1)
            fail(Reason::invalid_stored_data);
        return o;
    }
    void reversal_algebra(const ApplicationReversalOccurrence &o, std::int64_t actual) {
        if (o.cumulative_fraction < 0 || o.cumulative_fraction > 1 || o.previous_cumulative < 0 ||
            o.cumulative < o.previous_cumulative || o.cumulative > actual ||
            o.exact_entitlement != Exact(actual) * o.cumulative_fraction ||
            o.rounding_residual != o.exact_entitlement - Exact(o.cumulative) ||
            Exact(o.requested_increment) != Exact(o.previous_cumulative) - Exact(o.cumulative) ||
            (o.full_refund && (o.cumulative_fraction != 1 || o.cumulative != actual)))
            fail(Reason::invalid_stored_data);
    }
    ApplicationReversalProgress progress(const ApplicationOriginalComponent &o,
                                         std::int64_t actual) {
        auto h = one("subathon_app_reversal_heads", app_reversal_heads_columns, basis_where(),
                     basis_values(o));
        qint64 position = 0, cumulative = 0;
        std::optional<ApplicationReversalOccurrenceKey> last;
        const QStringList cols{"operation_scope",         "operation_ref",
                               "request_ordinal",         "original_scope",
                               "original_operation",      "original_request_ordinal",
                               "original_component_kind", "basis_position",
                               "previous_cumulative",     "cumulative"};
        for (;;) {
            auto b = basis_values(o);
            b.append(position);
            auto rows = execute({"SELECT " + projection(cols) +
                                     " FROM main.subathon_app_reversals WHERE " + basis_where() +
                                     " AND basis_position>? ORDER BY basis_position LIMIT 65",
                                 std::move(b)});
            for (qsizetype n = 0; n < std::min<qsizetype>(64, rows.size()); ++n) {
                const auto &r = rows[n];
                shape(r, cols.size());
                ApplicationOperationKey key_{string(r, 0, limits_.reference_bytes, true),
                                             string(r, 1, limits_.reference_bytes, true)};
                const auto ordinal = integer(r, 2);
                if (ordinal < 1 ||
                    string(r, 3, limits_.reference_bytes, true) != o.operation.operation_scope ||
                    string(r, 4, limits_.reference_bytes, true) != o.operation.operation_ref ||
                    integer(r, 5) != o.request_ordinal ||
                    enumeration(r, 6, 3) != int(o.component_kind) ||
                    position == std::numeric_limits<qint64>::max() ||
                    integer(r, 7) != position + 1 || integer(r, 8) != cumulative)
                    fail(Reason::invalid_stored_data);
                const auto next = integer(r, 9);
                if (next < cumulative || next > actual)
                    fail(Reason::invalid_stored_data);
                ++position;
                cumulative = next;
                last = ApplicationReversalOccurrenceKey{std::move(key_), ordinal, o};
            }
            if (rows.size() <= 64)
                break;
        }
        auto actual_occurrences =
            execute({"SELECT count(*) FROM main.subathon_app_reversals WHERE " + basis_where(),
                     basis_values(o)});
        if (actual_occurrences.size() != 1 ||
            actual_occurrences.front().front().toLongLong() != position)
            fail(Reason::invalid_stored_data);
        if (!h) {
            if (position)
                fail(Reason::invalid_stored_data);
            return {};
        }
        if (position == 0 ||
            string(*h, 0, limits_.reference_bytes, true) != o.operation.operation_scope ||
            string(*h, 1, limits_.reference_bytes, true) != o.operation.operation_ref ||
            integer(*h, 2) != o.request_ordinal || enumeration(*h, 3, 3) != int(o.component_kind) ||
            integer(*h, 4) != position || integer(*h, 5) != cumulative ||
            string(*h, 6, limits_.reference_bytes, true) != last->operation.operation_scope ||
            string(*h, 7, limits_.reference_bytes, true) != last->operation.operation_ref ||
            integer(*h, 8) != last->request_ordinal)
            fail(Reason::invalid_stored_data);
        auto b = operation_values(last->operation);
        b.append(qint64(last->request_ordinal));
        b.append(basis_values(o));
        auto terminal = one("subathon_app_reversals", app_reversals_columns,
                            operation_where() + " AND request_ordinal=? AND " + basis_where(),
                            std::move(b), true);
        auto selected = occurrence(*terminal, last->operation);
        if (selected.basis_position != position || selected.cumulative != cumulative)
            fail(Reason::invalid_stored_data);
        reversal_algebra(selected, actual);
        return {position, cumulative, selected.cumulative_fraction, selected.full_refund,
                std::move(last)};
    }
    ApplicationReversalBasisResult basis(const ApplicationOriginalComponent &o,
                                         bool account = true) {
        auto f = fixed(o.operation, true);
        if (!f)
            fail(Reason::missing_reference);
        if (f->record.intent.kind != ApplicationOperationKind::original)
            fail(Reason::source_mismatch);
        children(*f, o.request_ordinal);
        if (f->record.effects.size() != 1)
            fail(Reason::missing_reference);
        auto &e = f->record.effects.front();
        auto selected = std::find_if(e.components.begin(), e.components.end(),
                                     [&](const auto &v) { return v.kind == o.component_kind; });
        if (selected == e.components.end())
            fail(Reason::missing_reference);
        ApplicationReversalBasis out;
        out.original = o;
        out.subject_kind = f->record.intent.subject_kind;
        out.plan = f->record.intent.plan;
        out.selected_plan = std::move(f->source.selected);
        out.application_before_token = f->record.advanced_before.token;
        out.application_configuration_ref = f->record.advanced_before.configuration_ref;
        if (selected->target_ref) {
            auto row = one("subathon_app_balances", app_balances_columns,
                           "token=? AND balance_kind=? AND target_ref=?",
                           {bytes(out.application_before_token), int(selected->kind),
                            bytes(*selected->target_ref)},
                           true);
            out.target =
                ApplicationBalance{static_cast<ApplicationBalanceKind>(int(selected->kind)),
                                   *selected->target_ref, integer(*row, 3)};
        }
        // Validate the selected effect against its actual target; unrelated source
        // balances and sibling amounts are intentionally outside this projection.
        auto before = f->record.advanced_before;
        if (out.target)
            before.balances = {*out.target};
        try {
            bundle_detail::validate_effect(e, e.request_ordinal, before, limits_);
        } catch (const SqliteBundleInputFailure &x) {
            if (x.error() == SqliteBundleInputError::exact_component_limit)
                fail(Reason::result_limit);
            fail(Reason::invalid_stored_data);
        }
        if (f->record.intent.contribution_id) {
            auto rows =
                one("subathon_contributions", {"contribution_id", "original_receipt_id"},
                    "contribution_id=?",
                    {encode_plan_revision_uint64(f->record.intent.contribution_id->value)}, true);
            out.contribution_original_receipt_id = ReceiptId{id(*rows, 1, true)};
        }
        const bool credit =
            selected->units >= 0 &&
            ((o.component_kind == CK::main_delta && e.arithmetic_role == AR::time_addition &&
              e.components[1].units == 0) ||
             (o.component_kind == CK::score && e.arithmetic_role == AR::direct &&
              e.unit == RequestedUnit::micro_points) ||
             (o.component_kind == CK::whole_count && e.arithmetic_role == AR::direct &&
              e.unit == RequestedUnit::whole_count));
        if (!credit)
            return {};
        out.actual_credit_units = selected->units;
        out.effect = std::move(e);
        out.progress = progress(o, out.actual_credit_units);
        if (account)
            result_bytes(add_result_items(add_result_items(plan_items(out.selected_plan),
                                                           out.effect.components.size()),
                                          size_t(bool(out.target))),
                         [&](auto &n) {
                             bundle_detail::count_bytes(n, out.original);
                             n.add(1 + 8);
                             bundle_detail::count_bytes(n, out.plan);
                             bundle_detail::count_bytes(n, out.selected_plan);
                             bundle_detail::count_bytes(n, out.effect);
                             if (out.target)
                                 bundle_detail::count_bytes(n, *out.target);
                             bundle_detail::count_bytes(n, out.application_before_token);
                             bundle_detail::count_bytes(n, out.application_configuration_ref);
                             if (out.contribution_original_receipt_id)
                                 n.add(8);
                             n.add(8 + 8 + 1);
                             bundle_detail::count_bytes(n, out.progress.cumulative_fraction);
                             if (out.progress.last) {
                                 bundle_detail::count_bytes(n, out.progress.last->operation);
                                 n.add(8);
                                 bundle_detail::count_bytes(n, out.progress.last->original);
                             }
                         });
        return {SqliteBundleBasisDisposition::ready, std::move(out)};
    }
    std::optional<RecordedApplicationBundle> lookup(const ApplicationOperationKey &k) {
        auto f = fixed(k, false);
        if (!f)
            return std::nullopt;
        children(*f);
        auto &v = f->record;
        // Materialize current-operation links in the declared composite key order;
        // vector count is bounded independently from each basis's lifetime history.
        auto rows = execute({"SELECT " + projection(app_reversals_columns) +
                                 " FROM main.subathon_app_reversals WHERE " + operation_where() +
                                 " ORDER BY "
                                 "request_ordinal,original_scope,original_operation,original_"
                                 "request_ordinal,original_component_kind LIMIT ?",
                             [&] {
                                 auto x = operation_values(k);
                                 x.append(qint64(limits_.links_per_bundle + 1));
                                 return x;
                             }()});
        if (rows.size() > limits_.links_per_bundle)
            fail(Reason::result_limit);
        for (const auto &r : rows) {
            shape(r, app_reversals_columns.size());
            auto o = occurrence(r, k);
            auto source_ = basis(o.original, false);
            if (!source_.basis)
                fail(Reason::invalid_stored_data);
            reversal_algebra(o, source_.basis->actual_credit_units);
            v.reversals.push_back(std::move(o));
        }
        if (qint64(v.reversals.size()) != f->counts[4])
            fail(Reason::invalid_stored_data);
        PreparedApplicationBundle prepared{v.intent,      v.advanced_before,
                                           v.after,       v.advance_explanation,
                                           v.effects,     v.jobs,
                                           v.transitions, {},
                                           v.result,      v.audit};
        for (const auto &o : v.reversals)
            prepared.reversals.push_back({o.request_ordinal, o.original, o.basis_position - 1,
                                          o.previous_cumulative, o.cumulative,
                                          o.requested_increment, o.cumulative_fraction,
                                          o.exact_entitlement, o.rounding_residual, o.full_refund});
        try {
            auto stored_limits = limits_;
            stored_limits.input_items = std::numeric_limits<qsizetype>::max() - 1;
            stored_limits.owned_bytes = std::numeric_limits<qsizetype>::max();
            bundle_detail::validate_bundle(prepared, stored_limits);
        } catch (const SqliteBundleInputFailure &e) {
            if (e.error() == SqliteBundleInputError::collection_limit ||
                e.error() == SqliteBundleInputError::exact_component_limit ||
                e.error() == SqliteBundleInputError::owned_byte_limit)
                fail(Reason::result_limit);
            fail(Reason::invalid_stored_data);
        }
        if (!head_relation(v.persisted_head, v.advanced_before, v.advance_explanation))
            fail(Reason::invalid_stored_data);
        size_t n = 0;
        for (const auto count :
             {v.persisted_head.balances.size(), v.advanced_before.balances.size(),
              v.after.balances.size(), v.effects.size(), v.jobs.size(), v.transitions.size(),
              v.reversals.size()})
            n = add_result_items(n, count);
        for (const auto &e : v.effects)
            n = add_result_items(n, e.components.size());
        result_bytes(n, [&](auto &count) {
            bundle_detail::count_bytes(count, v.intent);
            bundle_detail::count_bytes(count, v.persisted_head);
            bundle_detail::count_bytes(count, v.advanced_before);
            bundle_detail::count_bytes(count, v.after);
            bundle_detail::count_bytes(count, v.advance_explanation);
            if (v.contribution) {
                count.add(8 + 1);
                bundle_detail::count_bytes(count, v.contribution->before);
            }
            for (const auto &e : v.effects)
                bundle_detail::count_bytes(count, e);
            for (const auto &j : v.jobs)
                bundle_detail::count_bytes(count, j);
            count.add(v.transitions.size());
            for (const auto &r : v.reversals)
                bundle_detail::count_bytes(count, r);
            bundle_detail::count_bytes(count, v.result);
            bundle_detail::count_bytes(count, v.audit);
        });
        return std::move(v);
    }
    bool head_relation(const ApplicationStateSnapshot &head, const ApplicationStateSnapshot &before,
                       const std::string &explanation) {
        if (head.token == before.token)
            return bundle_detail::same_state(head, before);
        if (explanation.empty() || head.configuration_ref != before.configuration_ref ||
            head.envelope.format_ref != before.envelope.format_ref ||
            head.time_debt_ms != before.time_debt_ms ||
            head.balances.size() != before.balances.size())
            return false;
        for (size_t n = 0; n < head.balances.size(); ++n) {
            const auto &a = head.balances[n], &b = before.balances[n];
            if (a.kind != b.kind || a.target_ref != b.target_ref || a.units != b.units)
                return false;
        }
        return true;
    }
    ApplicationBundleStageResult stage(const PreparedApplicationBundle &b) {
        const auto &i = b.intent;
        if (auto old = lookup(i.operation)) {
            if (bundle_detail::same_intent(old->intent, i))
                return {SD::already_present, std::move(old)};
            return {SD::operation_conflict, {}};
        }
        const auto claim_result = [&](const SqlRow &row, SD outcome, qsizetype offset = 0) {
            const ApplicationOperationKey k{string(row, offset, limits_.reference_bytes, true),
                                            string(row, offset + 1, limits_.reference_bytes, true)};
            auto old = lookup(k);
            if (!old)
                fail(Reason::invalid_stored_data);
            return ApplicationBundleStageResult{outcome, std::move(old)};
        };
        if (i.kind == ApplicationOperationKind::original) {
            auto claim =
                one("subathon_app_claims", {"operation_scope", "operation_ref"},
                    "subject_kind=? AND subject_id=?",
                    {int(i.subject_kind),
                     encode_plan_revision_uint64(i.contribution_id ? i.contribution_id->value
                                                                   : i.receipt_id.value)});
            if (claim)
                return claim_result(*claim, SD::subject_claimed);
        }
        auto receipt_claim = one("subathon_app_operations", {"operation_scope", "operation_ref"},
                                 "operation_kind=? AND receipt_id=?",
                                 {int(i.kind), encode_plan_revision_uint64(i.receipt_id.value)});
        if (receipt_claim)
            return claim_result(*receipt_claim, SD::source_claimed);
        if (i.reversal_ref) {
            auto reversal = one("subathon_app_operations", {"operation_scope", "operation_ref"},
                                "operation_scope=? AND reversal_ref=?",
                                {bytes(i.operation.operation_scope), bytes(*i.reversal_ref)});
            if (reversal) {
                (void)claim_result(*reversal, SD::reversal_identity_conflict);
                return {SD::reversal_identity_conflict, {}};
            }
        }
        auto head = current();
        if (!head)
            fail(Reason::missing_state);
        if (head->token != i.expected_state_token)
            return {SD::stale_state, {}};
        std::optional<ContributionRecord> current_contribution;
        if (i.contribution_id) {
            current_contribution = contribution_by_id(*i.contribution_id);
            if (!current_contribution)
                fail(Reason::missing_contribution);
            if (current_contribution->state != *i.expected_contribution_state)
                return {SD::stale_contribution, {}};
            if (i.kind == ApplicationOperationKind::original &&
                current_contribution->state.conditions.application ==
                    ContributionApplication::applied)
                return {SD::subject_unavailable, {}};
        }
        auto p = plan(i.plan, true);
        if (!p)
            fail(Reason::missing_plan);
        if (p->head != i.plan.revision)
            return {SD::stale_plan, {}};
        const auto &event = p->selected.captured.event;
        if (event.receipt_id != i.receipt_id || event.contribution_id != i.contribution_id ||
            ((i.subject_kind == ApplicationSubjectKind::contribution) !=
             bool(event.contribution_id)))
            fail(Reason::source_mismatch);
        for (const auto &token : {b.advanced_before.token, b.after.token})
            if (token != head->token || token == b.after.token) {
                auto row = one("subathon_app_states", {"token"}, "token=?", {bytes(token)});
                if (row) {
                    (void)state(token);
                    return {SD::state_token_conflict, {}};
                }
            }
        if (!head_relation(*head, b.advanced_before, b.advance_explanation))
            fail(Reason::source_mismatch);
        const auto &requests = p->selected.captured.requested_effects;
        if (requests.size() != b.effects.size())
            fail(Reason::source_mismatch);
        for (size_t n = 0; n < requests.size(); ++n) {
            const auto &q = requests[n];
            const auto &e = b.effects[n];
            if (q.unit != e.unit || q.exact_requested_units != e.exact_requested_units ||
                q.requested_units != e.requested_units)
                fail(Reason::source_mismatch);
            for (const auto &c : e.components)
                if (c.target_ref && *c.target_ref != q.target_ref)
                    fail(Reason::source_mismatch);
        }
        std::map<std::tuple<std::string, std::string, qint64, int>, ApplicationReversalProgress>
            proposed;
        std::vector<ApplicationReversalOccurrence> occurrences;
        occurrences.reserve(b.reversals.size());
        for (const auto &link : b.reversals) {
            const auto key_ = std::make_tuple(
                link.original.operation.operation_scope, link.original.operation.operation_ref,
                qint64(link.original.request_ordinal), int(link.original.component_kind));
            auto source_ = basis(link.original, false);
            if (!source_.basis)
                return {SD::reversal_conflict, {}};
            const auto &source = *source_.basis;
            auto [it, inserted] = proposed.try_emplace(key_, source.progress);
            (void)inserted;
            auto &previous = it->second;
            if (previous.position != link.expected_prior_position ||
                previous.cumulative != link.expected_prior_cumulative)
                return {SD::stale_reversal, {}};
            if (link.cumulative_fraction < previous.cumulative_fraction || link.cumulative < 0 ||
                link.cumulative > source.actual_credit_units ||
                link.exact_entitlement !=
                    Exact(source.actual_credit_units) * link.cumulative_fraction ||
                (link.full_refund &&
                 (link.cumulative_fraction != 1 || link.cumulative != source.actual_credit_units)))
                return {SD::reversal_conflict, {}};
            const auto &e = b.effects[size_t(link.request_ordinal - 1)];
            if (e.unit != source.effect.unit)
                return {SD::reversal_conflict, {}};
            if (source.target) {
                const auto &request = requests[size_t(link.request_ordinal - 1)];
                if (request.target_ref != source.target->target_ref)
                    return {SD::reversal_conflict, {}};
            }
            if (previous.position == std::numeric_limits<qint64>::max())
                fail(Reason::sequence_exhausted);
            occurrences.push_back({link.request_ordinal, link.original, previous.position + 1,
                                   previous.cumulative, link.cumulative, link.requested_increment,
                                   link.cumulative_fraction, link.exact_entitlement,
                                   link.rounding_residual, link.full_refund});
            previous = {
                previous.position + 1, link.cumulative, link.cumulative_fraction, link.full_refund,
                ApplicationReversalOccurrenceKey{i.operation, link.request_ordinal, link.original}};
        }
        // Every statement, binding and returned disposition is allocated before
        // writing; the outer guard still covers allocations inside the real writer.
        std::vector<SqlStatement> out;
        if (b.advanced_before.token != head->token)
            prepare_state(out, b.advanced_before);
        prepare_state(out, b.after);
        out.push_back(insertion(
            "subathon_app_operations", app_operations_columns,
            {bytes(i.operation.operation_scope), bytes(i.operation.operation_ref), int(i.kind),
             int(i.subject_kind), encode_plan_revision_uint64(i.receipt_id.value),
             i.contribution_id ? QVariant(encode_plan_revision_uint64(i.contribution_id->value))
                               : QVariant{},
             bytes(i.plan.plan_ref), encode_plan_revision_uint64(i.plan.revision),
             bytes(head->token), bytes(b.advanced_before.token), bytes(b.after.token),
             bytes(b.advance_explanation), bytes(i.choices.format_ref), i.choices.document,
             i.reversal_ref ? QVariant(bytes(*i.reversal_ref)) : QVariant{}}));
        if (i.contribution_id) {
            auto values = operation_values(i.operation);
            values.append(encode_plan_revision_uint64(i.contribution_id->value));
            values.append(int(i.expected_contribution_state->payment_evidence));
            values.append(condition_values(i.expected_contribution_state->conditions));
            values.append(int(i.kind == ApplicationOperationKind::original
                                  ? ContributionApplication::applied
                                  : i.expected_contribution_state->conditions.application));
            out.push_back(insertion("subathon_app_contributions", app_contributions_columns,
                                    std::move(values)));
        }
        if (i.kind == ApplicationOperationKind::original)
            out.push_back(
                insertion("subathon_app_claims", app_claims_columns,
                          {int(i.subject_kind),
                           encode_plan_revision_uint64(i.contribution_id ? i.contribution_id->value
                                                                         : i.receipt_id.value),
                           bytes(i.operation.operation_scope), bytes(i.operation.operation_ref)}));
        qint64 component_count = 0;
        for (const auto &e : b.effects) {
            auto a = encode_plan_exact(e.exact_requested_units, limits_.component_bytes),
                 r = encode_plan_exact(e.rounding_residual, limits_.component_bytes);
            auto values = operation_values(i.operation);
            values.append(
                {qint64(e.request_ordinal), bytes(i.plan.plan_ref),
                 encode_plan_revision_uint64(i.plan.revision), int(e.arithmetic_role), int(e.unit),
                 a.numerator, a.denominator, qint64(e.requested_units), r.numerator, r.denominator,
                 e.maximum_main_ms ? QVariant(qint64(*e.maximum_main_ms)) : QVariant{},
                 e.deficit_policy ? QVariant(int(*e.deficit_policy)) : QVariant{},
                 e.deficit_disposition ? QVariant(int(*e.deficit_disposition)) : QVariant{},
                 bytes(e.application_explanation)});
            out.push_back(
                insertion("subathon_app_effects", app_effects_columns, std::move(values)));
            for (const auto &c : e.components) {
                auto val = operation_values(i.operation);
                val.append({qint64(e.request_ordinal), int(c.kind), qint64(c.units),
                            bytes(b.advanced_before.token),
                            c.target_ref ? QVariant(bytes(*c.target_ref)) : QVariant{}});
                out.push_back(
                    insertion("subathon_app_components", app_components_columns, std::move(val)));
                ++component_count;
            }
        }
        qint64 ordinal = 0;
        for (const auto &t : b.transitions) {
            auto v = operation_values(i.operation);
            v.append({++ordinal, int(t)});
            out.push_back(
                insertion("subathon_app_transitions", app_transitions_columns, std::move(v)));
        }
        if (i.kind == ApplicationOperationKind::original && i.contribution_id)
            out.push_back(
                {"UPDATE main.subathon_contributions SET application=1 WHERE contribution_id=?",
                 {encode_plan_revision_uint64(i.contribution_id->value)}});
        ordinal = 0;
        for (const auto &j : b.jobs) {
            auto v = operation_values(i.operation);
            v.append({++ordinal, bytes(j.job_ref),
                      j.source_request_ordinal ? QVariant(qint64(*j.source_request_ordinal))
                                               : QVariant{},
                      j.group_ref ? QVariant(bytes(*j.group_ref)) : QVariant{},
                      j.group_ordinal ? QVariant(qint64(*j.group_ordinal)) : QVariant{},
                      bytes(j.envelope.format_ref), j.envelope.document});
            out.push_back(insertion("subathon_app_jobs", app_jobs_columns, std::move(v)));
        }
        {
            auto v = operation_values(i.operation);
            v.append({bytes(b.result.format_ref), b.result.document, qint64(b.effects.size()),
                      component_count, qint64(b.jobs.size()), qint64(b.transitions.size()),
                      qint64(b.reversals.size())});
            out.push_back(insertion("subathon_app_results", app_results_columns, std::move(v)));
        }
        {
            auto v = operation_values(i.operation);
            v.append({b.audit.actor_ref ? QVariant(bytes(*b.audit.actor_ref)) : QVariant{},
                      b.audit.decision_ref ? QVariant(bytes(*b.audit.decision_ref)) : QVariant{},
                      b.audit.reason ? QVariant(bytes(*b.audit.reason)) : QVariant{},
                      bytes(b.audit.envelope.format_ref), b.audit.envelope.document});
            out.push_back(insertion("subathon_app_audit", app_audit_columns, std::move(v)));
        }
        for (const auto &o : occurrences) {
            auto f = encode_plan_exact(o.cumulative_fraction, limits_.component_bytes),
                 e = encode_plan_exact(o.exact_entitlement, limits_.component_bytes),
                 r = encode_plan_exact(o.rounding_residual, limits_.component_bytes);
            auto v = operation_values(i.operation);
            v.append(qint64(o.request_ordinal));
            v.append(basis_values(o.original));
            v.append({qint64(o.basis_position), f.numerator, f.denominator, e.numerator,
                      e.denominator, r.numerator, r.denominator, qint64(o.previous_cumulative),
                      qint64(o.cumulative), qint64(o.requested_increment), int(o.full_refund)});
            out.push_back(insertion("subathon_app_reversals", app_reversals_columns, std::move(v)));
        }
        for (const auto &[key_, value] : proposed) {
            auto v = Values{bytes(std::get<0>(key_)),
                            bytes(std::get<1>(key_)),
                            std::get<2>(key_),
                            std::get<3>(key_),
                            qint64(value.position),
                            qint64(value.cumulative),
                            bytes(i.operation.operation_scope),
                            bytes(i.operation.operation_ref),
                            qint64(value.last->request_ordinal)};
            auto sql =
                insertion("subathon_app_reversal_heads", app_reversal_heads_columns, std::move(v));
            sql.sql += " ON "
                       "CONFLICT(original_scope,original_operation,original_request_ordinal,"
                       "original_component_kind) DO UPDATE SET "
                       "basis_position=excluded.basis_position,cumulative=excluded.cumulative,last_"
                       "scope=excluded.last_scope,last_operation=excluded.last_operation,last_"
                       "request_ordinal=excluded.last_request_ordinal";
            out.push_back(std::move(sql));
        }
        out.push_back({"UPDATE main.subathon_app_head SET token=? WHERE singleton=1 AND token=?",
                       {bytes(b.after.token), bytes(head->token)}});
        const SqlStatement changed_query{"SELECT changes()", {}};
        write(std::move(out));
        auto changed = execute(changed_query);
        if (changed.size() != 1 || changed.front().size() != 1 ||
            changed.front().front().toLongLong() != 1)
            fail(Reason::invalid_stored_data);
        return {SD::staged, {}};
    }
};
} // namespace
MigrationManifest application_bundle_repository_manifest() {
    return bundle_detail::make_manifest();
}
SqliteApplicationBundleStore::SqliteApplicationBundleStore(SqliteWorkSession session,
                                                           SqliteBundleLimits limits)
    : session_(std::move(session)), limits_(limits) {}
SqliteBundleInitialDisposition
SqliteApplicationBundleStore::retain_initial_state(const ApplicationStateSnapshot &s) const {
    bundle_detail::validate_initial(s, limits_);
    Access a(session_, limits_);
    return a.guarded([&] { return a.initial(s); });
}
std::optional<ApplicationStateSnapshot> SqliteApplicationBundleStore::read_state() const {
    bundle_detail::validate_options(limits_);
    Access a(session_, limits_);
    return a.guarded([&] {
        auto s = a.current();
        if (s)
            a.result_size(s->balances.size(), bundle_detail::logical_bytes(*s));
        return s;
    });
}
ApplicationBundleInputResult
SqliteApplicationBundleStore::read_inputs(const ApplicationPlanKey &p,
                                          ApplicationSubjectKind k) const {
    bundle_detail::validate_options(limits_);
    bundle_detail::validate_plan_key(p, limits_);
    if (int(k) < 0 || int(k) > 1)
        throw SqliteBundleInputFailure(SqliteBundleInputError::invalid_source);
    Access a(session_, limits_);
    return a.guarded([&] { return a.inputs(p, k); });
}
std::optional<RecordedApplicationBundle>
SqliteApplicationBundleStore::lookup(const ApplicationOperationKey &k) const {
    bundle_detail::validate_options(limits_);
    bundle_detail::validate_key(k, limits_);
    Access a(session_, limits_);
    return a.guarded([&] { return a.lookup(k); });
}
ApplicationReversalBasisResult
SqliteApplicationBundleStore::read_reversal_basis(const ApplicationOriginalComponent &o) const {
    bundle_detail::validate_options(limits_);
    bundle_detail::validate_original(o, limits_);
    Access a(session_, limits_);
    return a.guarded([&] { return a.basis(o); });
}
ApplicationBundleStageResult
SqliteApplicationBundleStore::stage(const PreparedApplicationBundle &b) const {
    bundle_detail::validate_bundle(b, limits_);
    Access a(session_, limits_);
    return a.guarded([&] { return a.stage(b); });
}
} // namespace subathon::server
