// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <limits>
#include <subathon/server/sqlite_application_plans.hpp>
#include <subathon/server/sqlite_contributions.hpp>
#include <subathon/server/sqlite_retained_context.hpp>
#include <utility>

namespace subathon::server {
namespace {
using namespace core;
using Input = SqlitePlanInputError;
using Reason = SqlitePlanStorageReason;
// Exact approved physical proposal; prefixes are obtained from their pure factory.
const std::array<QString, 5> migration_sql{
    "CREATE TABLE subathon_plan_headers(plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AN"
    "D length(plan_ref)>0) PRIMARY KEY, event_ref BLOB NOT NULL CHECK(typeof(event_ref)='blob' "
    "AND length(event_ref)>0), parent_event_ref BLOB CHECK(parent_event_ref IS NULL OR (typeof("
    "parent_event_ref)='blob')), root_event_ref BLOB NOT NULL CHECK(typeof(root_event_ref)='blo"
    "b' AND length(root_event_ref)>0), receipt_id BLOB NOT NULL CHECK(typeof(receipt_id)='blob'"
    " AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000') REFERENCES subathon_receipt"
    "s(receipt_id), contribution_id BLOB CHECK(contribution_id IS NULL OR (typeof(contribution_"
    "id)='blob' AND length(contribution_id)=8)) REFERENCES subathon_contributions(contribution_"
    "id), head_revision BLOB NOT NULL CHECK(typeof(head_revision)='blob' AND length(head_revisi"
    "on)=8 AND head_revision<>X'0000000000000000'))",
    "CREATE TABLE subathon_plan_revisions(plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' "
    "AND length(plan_ref)>0) REFERENCES subathon_plan_headers(plan_ref), revision BLOB NOT NULL"
    " CHECK(typeof(revision)='blob' AND length(revision)=8 AND revision<>X'0000000000000000'), "
    "parent_revision BLOB CHECK(parent_revision IS NULL OR (typeof(parent_revision)='blob' AND "
    "length(parent_revision)=8 AND parent_revision<>X'0000000000000000')), payment_evidence INT"
    "EGER NOT NULL CHECK(typeof(payment_evidence)='integer' AND payment_evidence BETWEEN 0 AND "
    "3), monetary INTEGER NOT NULL CHECK(typeof(monetary)='integer' AND monetary BETWEEN 0 AND "
    "1), original_decimal BLOB, currency BLOB, resolution_ref BLOB CHECK(resolution_ref IS NULL"
    " OR (typeof(resolution_ref)='blob')), missing_rate INTEGER NOT NULL CHECK(typeof(missing_r"
    "ate)='integer' AND missing_rate BETWEEN 0 AND 1), review INTEGER NOT NULL CHECK(typeof(rev"
    "iew)='integer' AND review BETWEEN 0 AND 3), evidence_policy INTEGER NOT NULL CHECK(typeof("
    "evidence_policy)='integer' AND evidence_policy BETWEEN 0 AND 2), policy_ref BLOB NOT NULL "
    "CHECK(typeof(policy_ref)='blob'), decision_choice INTEGER, decision_ref BLOB, decision_act"
    "or_ref BLOB, application INTEGER NOT NULL CHECK(typeof(application)='integer' AND applicat"
    "ion BETWEEN 0 AND 1), reversal INTEGER NOT NULL CHECK(typeof(reversal)='integer' AND rever"
    "sal BETWEEN 0 AND 2), settlement INTEGER NOT NULL CHECK(typeof(settlement)='integer' AND s"
    "ettlement BETWEEN 0 AND 1), valuation_state INTEGER NOT NULL CHECK(typeof(valuation_state)"
    "='integer' AND valuation_state BETWEEN 0 AND 2), historical_resolution_ref BLOB, historica"
    "l_original_decimal BLOB, historical_currency BLOB, comparison_currency BLOB, effective_rat"
    "e_decimal BLOB, fetched_rate_decimal BLOB, snapshot_ref BLOB, rate_source_ref BLOB, rate_t"
    "imestamp_utc_ms INTEGER, fetch_timestamp_utc_ms INTEGER, manual_decision_ref BLOB, overrid"
    "e_authority INTEGER, override_actor_ref BLOB, override_reason BLOB, override_decision_ref "
    "BLOB, PRIMARY KEY(plan_ref,revision), FOREIGN KEY(plan_ref,parent_revision) REFERENCES sub"
    "athon_plan_revisions(plan_ref,revision), CHECK((original_decimal IS NULL AND currency IS N"
    "ULL) OR (typeof(original_decimal)='blob' AND typeof(currency)='blob')), CHECK((decision_ch"
    "oice IS NULL AND decision_ref IS NULL AND decision_actor_ref IS NULL) OR (typeof(decision_"
    "choice)='integer' AND decision_choice IN (0,1) AND typeof(decision_ref)='blob' AND typeof("
    "decision_actor_ref)='blob')), CHECK((valuation_state IN (0,1) AND historical_resolution_re"
    "f IS NULL AND historical_original_decimal IS NULL AND historical_currency IS NULL AND comp"
    "arison_currency IS NULL AND effective_rate_decimal IS NULL AND fetched_rate_decimal IS NUL"
    "L AND snapshot_ref IS NULL AND rate_source_ref IS NULL AND rate_timestamp_utc_ms IS NULL A"
    "ND fetch_timestamp_utc_ms IS NULL AND manual_decision_ref IS NULL) OR (valuation_state=2 A"
    "ND typeof(historical_resolution_ref)='blob' AND typeof(historical_original_decimal)='blob'"
    " AND typeof(historical_currency)='blob' AND typeof(comparison_currency)='blob' AND typeof("
    "effective_rate_decimal)='blob' AND typeof(snapshot_ref)='blob' AND (fetched_rate_decimal I"
    "S NULL OR typeof(fetched_rate_decimal)='blob') AND (rate_source_ref IS NULL OR typeof(rate"
    "_source_ref)='blob') AND (manual_decision_ref IS NULL OR typeof(manual_decision_ref)='blob"
    "') AND (rate_timestamp_utc_ms IS NULL OR typeof(rate_timestamp_utc_ms)='integer') AND (fet"
    "ch_timestamp_utc_ms IS NULL OR typeof(fetch_timestamp_utc_ms)='integer'))), CHECK((revisio"
    "n=X'0000000000000001' AND parent_revision IS NULL AND override_authority IS NULL AND overr"
    "ide_actor_ref IS NULL AND override_reason IS NULL AND override_decision_ref IS NULL) OR (r"
    "evision<>X'0000000000000001' AND typeof(parent_revision)='blob' AND length(parent_revision"
    ")=8 AND parent_revision<>X'0000000000000000' AND parent_revision<revision AND typeof(overr"
    "ide_authority)='integer' AND override_authority=2 AND typeof(override_actor_ref)='blob' AN"
    "D length(override_actor_ref)>0 AND typeof(override_reason)='blob' AND length(override_reas"
    "on)>0 AND typeof(override_decision_ref)='blob' AND length(override_decision_ref)>0)))",
    "CREATE TABLE subathon_plan_rules(plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND "
    "length(plan_ref)>0), revision BLOB NOT NULL CHECK(typeof(revision)='blob' AND length(revis"
    "ion)=8 AND revision<>X'0000000000000000'), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)="
    "'integer' AND ordinal>0), rule_ref BLOB NOT NULL CHECK(typeof(rule_ref)='blob'), version_r"
    "ef BLOB NOT NULL CHECK(typeof(version_ref)='blob'), selected INTEGER NOT NULL CHECK(typeof"
    "(selected)='integer' AND selected BETWEEN 0 AND 1), explanation BLOB NOT NULL CHECK(typeof"
    "(explanation)='blob'), PRIMARY KEY(plan_ref,revision,ordinal), FOREIGN KEY(plan_ref,revisi"
    "on) REFERENCES subathon_plan_revisions(plan_ref,revision))",
    "CREATE TABLE subathon_plan_modifiers(plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' "
    "AND length(plan_ref)>0), revision BLOB NOT NULL CHECK(typeof(revision)='blob' AND length(r"
    "evision)=8 AND revision<>X'0000000000000000'), ordinal INTEGER NOT NULL CHECK(typeof(ordin"
    "al)='integer' AND ordinal>0), definition_version_ref BLOB NOT NULL CHECK(typeof(definition"
    "_version_ref)='blob'), instance_revision_ref BLOB NOT NULL CHECK(typeof(instance_revision_"
    "ref)='blob'), target_ref BLOB NOT NULL CHECK(typeof(target_ref)='blob'), group_ref BLOB NO"
    "T NULL CHECK(typeof(group_ref)='blob'), group_operator BLOB NOT NULL CHECK(typeof(group_op"
    "erator)='blob'), target_operator BLOB NOT NULL CHECK(typeof(target_operator)='blob'), orig"
    "inal_factor_decimal BLOB NOT NULL CHECK(typeof(original_factor_decimal)='blob'), PRIMARY K"
    "EY(plan_ref,revision,ordinal), FOREIGN KEY(plan_ref,revision) REFERENCES subathon_plan_rev"
    "isions(plan_ref,revision))",
    "CREATE TABLE subathon_plan_requests(plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' A"
    "ND length(plan_ref)>0), revision BLOB NOT NULL CHECK(typeof(revision)='blob' AND length(re"
    "vision)=8 AND revision<>X'0000000000000000'), ordinal INTEGER NOT NULL CHECK(typeof(ordina"
    "l)='integer' AND ordinal>0), action_ref BLOB NOT NULL CHECK(typeof(action_ref)='blob'), ac"
    "tion_version_ref BLOB NOT NULL CHECK(typeof(action_version_ref)='blob'), rule_ref BLOB NOT"
    " NULL CHECK(typeof(rule_ref)='blob'), target_ref BLOB NOT NULL CHECK(typeof(target_ref)='b"
    "lob'), unit INTEGER NOT NULL CHECK(typeof(unit)='integer' AND unit BETWEEN 0 AND 2), requi"
    "res_historical_valuation INTEGER NOT NULL CHECK(typeof(requires_historical_valuation)='int"
    "eger' AND requires_historical_valuation BETWEEN 0 AND 1), exact_numerator BLOB NOT NULL CH"
    "ECK(typeof(exact_numerator)='blob' AND length(exact_numerator)>0), exact_denominator BLOB "
    "NOT NULL CHECK(typeof(exact_denominator)='blob' AND length(exact_denominator)>0), requeste"
    "d_units INTEGER NOT NULL CHECK(typeof(requested_units)='integer'), calculation_explanation"
    " BLOB NOT NULL CHECK(typeof(calculation_explanation)='blob'), reversal_behavior_ref BLOB N"
    "OT NULL CHECK(typeof(reversal_behavior_ref)='blob'), PRIMARY KEY(plan_ref,revision,ordinal"
    "), FOREIGN KEY(plan_ref,revision) REFERENCES subathon_plan_revisions(plan_ref,revision))",
};
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
[[noreturn]] void invalid(Input error) { throw SqlitePlanInputFailure(error); }
bool bounded(const std::string &v, qsizetype n, bool required = false) {
    return (!required || !v.empty()) && v.size() <= size_t(n);
}
template <class E> bool enum_in(E v, int maximum) { return int(v) >= 0 && int(v) <= maximum; }
void validate_options(SqlitePlanLimits l) {
    const auto count = [](qsizetype n) {
        return n > 0 && n < std::numeric_limits<qsizetype>::max() &&
               std::uintmax_t(n) < std::uintmax_t(std::numeric_limits<qint64>::max());
    };
    if (l.reference_bytes <= 0 || l.value_bytes <= 0 || l.component_bytes <= 0 ||
        !count(l.revisions_per_full_find) || !count(l.items_per_vector) ||
        !count(l.items_per_full_find) ||
        std::uintmax_t(l.revisions_per_full_find) >=
            std::vector<CapturedPlanRevision>{}.max_size() ||
        std::uintmax_t(l.items_per_vector) >= std::vector<RuleSelection>{}.max_size() ||
        std::uintmax_t(l.items_per_vector) >= std::vector<ModifierSelection>{}.max_size() ||
        std::uintmax_t(l.items_per_vector) >= std::vector<RequestedEffect>{}.max_size() ||
        std::uintmax_t(l.items_per_full_find) >= std::numeric_limits<size_t>::max())
        invalid(Input::invalid_options);
}
void preflight(const std::string &ref, SqlitePlanLimits l) {
    validate_options(l);
    if (!bounded(ref, l.reference_bytes, true))
        invalid(Input::invalid_plan_ref);
}
void validate_receipt(const ContributionReceipt &receipt, SqlitePlanLimits limits) {
    const auto &k = receipt.identity;
    const auto &f = receipt.facts;
    const auto &a = receipt.arrival;
    const auto optional = [&](const std::optional<std::string> &v) {
        return !v || bounded(*v, limits.value_bytes);
    };
    const auto link = [&](const std::optional<core::ReceiptLinkReference> &v) {
        return !v || (bounded(v->identifier_namespace, limits.reference_bytes) &&
                      bounded(v->opaque_id, limits.reference_bytes));
    };
    if (!bounded(k.origin.provider, limits.reference_bytes, true) ||
        !bounded(k.origin.source_account, limits.reference_bytes, true) ||
        !bounded(k.identifier_namespace, limits.reference_bytes, true) ||
        !bounded(k.opaque_id, limits.reference_bytes, true) ||
        !bounded(f.event_type, limits.value_bytes) || !optional(f.actor_id) ||
        !optional(f.actor_display_name) ||
        (f.money && (!bounded(f.money->original_decimal, limits.value_bytes) ||
                     !bounded(f.money->currency, limits.value_bytes))) ||
        !link(f.payment_reference) || !link(f.activity_reference) ||
        !bounded(a.connection_ref, limits.reference_bytes) ||
        !bounded(a.configuration_ref, limits.reference_bytes, true) ||
        !bounded(a.modifier_context_ref, limits.reference_bytes, true))
        invalid(Input::invalid_event);
}
void validate_conditions(const core::ContributionConditions &c, SqlitePlanLimits limits) {
    const auto &v = c.valuation;
    const auto &a = c.evidence_admission;
    if (!enum_in(c.review, 3) || !enum_in(a.policy, 2) || !enum_in(c.application, 1) ||
        !enum_in(c.reversal, 2) || !enum_in(c.settlement, 1) ||
        (v.original_money && (!bounded(v.original_money->original_decimal, limits.value_bytes) ||
                              !bounded(v.original_money->currency, limits.value_bytes))) ||
        (v.resolution_ref && !bounded(*v.resolution_ref, limits.reference_bytes)) ||
        !bounded(a.policy_ref, limits.reference_bytes) ||
        (a.review_decision && (!enum_in(a.review_decision->choice, 1) ||
                               !bounded(a.review_decision->decision_ref, limits.reference_bytes) ||
                               !bounded(a.review_decision->actor_ref, limits.reference_bytes))))
        invalid(Input::invalid_contribution_state);
}
void validate_state(const core::ContributionState &state, SqlitePlanLimits limits) {
    if (!enum_in(state.payment_evidence, 3))
        invalid(Input::invalid_contribution_state);
    validate_conditions(state.conditions, limits);
}
bool valid_valuation(const PlanValuation &v, const PlanEvent &event, bool dependent,
                     SqlitePlanLimits l) {
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
void validate_revision(const CapturedPlanRevision &r, bool initial, SqlitePlanLimits l) {
    if (initial ? (r.revision != 1 || r.parent_revision.has_value())
                : (r.revision <= 1 || !r.parent_revision || *r.parent_revision == 0 ||
                   *r.parent_revision == std::numeric_limits<std::uint64_t>::max() ||
                   r.revision != *r.parent_revision + 1))
        invalid(Input::invalid_revision);
    if (r.rules.size() > size_t(l.items_per_vector) ||
        r.modifiers.size() > size_t(l.items_per_vector) ||
        r.captured.requested_effects.size() > size_t(l.items_per_vector))
        invalid(Input::collection_limit);
    if (initial ? r.override_decision.has_value()
                : (!r.override_decision ||
                   r.override_decision->authority != OverrideAuthority::authorized ||
                   !bounded(r.override_decision->actor_ref, l.reference_bytes, true) ||
                   !bounded(r.override_decision->reason, l.value_bytes, true) ||
                   !bounded(r.override_decision->decision_ref, l.reference_bytes, true)))
        invalid(Input::invalid_override);
    const auto &e = r.captured.event;
    if (e.receipt_id.value == 0)
        invalid(Input::invalid_receipt_id);
    if (!bounded(e.event_ref, l.reference_bytes, true) ||
        !bounded(e.root_event_ref, l.reference_bytes, true) ||
        (e.parent_event_ref && !bounded(*e.parent_event_ref, l.reference_bytes)))
        invalid(Input::invalid_event);
    validate_receipt({e.receipt_id, e.identity, e.facts, e.arrival}, l);
    validate_state(r.captured.contribution_state, l);
    const bool dependent =
        std::any_of(r.captured.requested_effects.begin(), r.captured.requested_effects.end(),
                    [](const auto &q) { return q.requires_historical_valuation; });
    if (!valid_valuation(r.captured.valuation, e, dependent, l))
        invalid(Input::invalid_valuation);
    for (const auto &v : r.rules)
        if (!bounded(v.rule_ref, l.reference_bytes) || !bounded(v.version_ref, l.reference_bytes) ||
            !bounded(v.explanation, l.value_bytes))
            invalid(Input::invalid_rule);
    for (const auto &v : r.modifiers)
        if (!bounded(v.definition_version_ref, l.reference_bytes) ||
            !bounded(v.instance_revision_ref, l.reference_bytes) ||
            !bounded(v.target_ref, l.reference_bytes) || !bounded(v.group_ref, l.reference_bytes) ||
            !bounded(v.group_operator, l.reference_bytes) ||
            !bounded(v.target_operator, l.reference_bytes) ||
            !bounded(v.original_factor_decimal, l.value_bytes))
            invalid(Input::invalid_modifier);
    for (const auto &v : r.captured.requested_effects) {
        if (!enum_in(v.unit, 2) || !bounded(v.action_ref, l.reference_bytes) ||
            !bounded(v.action_version_ref, l.reference_bytes) ||
            !bounded(v.rule_ref, l.reference_bytes) || !bounded(v.target_ref, l.reference_bytes) ||
            !bounded(v.calculation_explanation, l.value_bytes) ||
            !bounded(v.reversal_behavior_ref, l.reference_bytes))
            invalid(Input::invalid_request);
        (void)encode_plan_exact(v.exact_requested_units, l.component_bytes);
    }
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
[[noreturn]] void map_work_failure(const SqliteWorkFailure &e) {
    switch (e.error()) {
    case WorkError::busy:
        throw SqlitePlanStorageFailure(Reason::busy);
    case WorkError::sql_failure:
        throw SqlitePlanStorageFailure(Reason::sql_failure);
    case WorkError::resource_limit:
        throw SqlitePlanStorageFailure(Reason::resource_limit);
    case WorkError::result_limit:
        throw SqlitePlanStorageFailure(Reason::result_limit);
    default:
        throw;
    }
}
// Each operation uses a private ephemeral decoder. It owns no transaction and
// never inspects mutable receipt attempts/current contribution payloads.
class Access final {
  public:
    Access(SqliteWorkSession session, SqlitePlanLimits limits)
        : session_(std::move(session)), limits_(limits) {}
    template <class F> auto guarded(F &&f) -> decltype(f()) {
        try {
            return f();
        } catch (const SqlitePlanStorageFailure &) {
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
            // These writer failures already performed the actual rollback. Clear
            // own-write tracking BEFORE constructing the mapped exception, which
            // can itself allocate; never replace that allocation failure with a
            // second control operation on an already terminal session.
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
        throw SqlitePlanStorageFailure(reason);
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
                throw SqlitePlanStorageFailure(Reason::invalid_stored_data);
            if (e.error() == RetentionError::result_limit)
                throw SqlitePlanStorageFailure(Reason::result_limit);
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
            fail(Reason::receipt_mismatch);
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
                                           "AND revision>? ORDER BY revision LIMIT 64",
                                       {bytes(ref), encode_plan_revision_uint64(previous)}});
            if (rows.isEmpty())
                break;
            for (const auto &row : rows) {
                auto r = scalar(row, ref, h);
                if (previous == std::numeric_limits<std::uint64_t>::max() ||
                    r.revision != previous + 1)
                    fail(Reason::invalid_stored_data);
                previous = r.revision;
            }
            if (rows.size() < 64)
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
                              qint64(limits_.items_per_vector + 1)}});
        if (rows.size() > limits_.items_per_vector)
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
    std::optional<ApplicationPlan> find(const std::string &ref) {
        const auto h = header(ref);
        if (!h)
            return std::nullopt;
        summary(ref, *h);
        if (h->head > std::uint64_t(limits_.revisions_per_full_find))
            fail(Reason::result_limit);
        ApplicationPlan result{ref, {}};
        result.revisions.reserve(size_t(h->head));
        std::uintmax_t total = 0;
        for (std::uint64_t n = 1;; ++n) {
            auto r = selected_scalar(ref, n, *h);
            if (!r)
                fail(Reason::invalid_stored_data);
            body(ref, *r);
            for (const auto size :
                 {r->rules.size(), r->modifiers.size(), r->captured.requested_effects.size()}) {
                if (std::uintmax_t(size) > std::uintmax_t(limits_.items_per_full_find) - total)
                    fail(Reason::result_limit);
                total += size;
            }
            result.revisions.push_back(std::move(*r));
            if (n == h->head)
                break;
        }
        return result;
    }
    std::optional<SqlitePlanRevisionSnapshot> targeted(const std::string &ref,
                                                       std::uint64_t revision) {
        const auto h = header(ref);
        if (!h)
            return std::nullopt;
        selected_max(ref, *h);
        const auto initial = selected_scalar(ref, 1, *h);
        if (!initial)
            fail(Reason::invalid_stored_data);
        auto r = revision == 1 ? initial : selected_scalar(ref, revision, *h);
        if (!r)
            return std::nullopt;
        parent_target(ref, *r);
        body(ref, *r);
        return SqlitePlanRevisionSnapshot{ref, std::move(*r)};
    }
    SqlitePlanInitialDisposition retain(const std::string &ref, const CapturedPlanRevision &r);
    SqlitePlanAppendDisposition append(const std::string &ref, std::uint64_t expected,
                                       const CapturedPlanRevision &r);

  private:
    SqliteWorkSession session_;
    SqlitePlanLimits limits_;
    bool wrote_{false};
    void write(const std::vector<SqlStatement> &prepared) {
        wrote_ = true;
        for (const auto &statement : prepared)
            (void)execute(statement);
    }
};

// Build every allocating SQL/value buffer before the first own write. The
// operation guard still covers writer-side allocations and the entire caller unit.
std::vector<SqlStatement> prepared_revision(const std::string &ref, const CapturedPlanRevision &r,
                                            bool initial, SqlitePlanLimits limits) {
    std::vector<SqlStatement> statements;
    size_t count = 2;
    for (const auto n : {r.rules.size(), r.modifiers.size(), r.captured.requested_effects.size()}) {
        if (n > statements.max_size() - count)
            throw std::length_error("Plan statement capacity exceeded");
        count += n;
    }
    statements.reserve(count);
    const auto plan = bytes(ref), revision = encode_plan_revision_uint64(r.revision);
    const auto &e = r.captured.event;
    if (initial)
        statements.push_back(insertion(
            "subathon_plan_headers", header_columns,
            {plan, bytes(e.event_ref),
             e.parent_event_ref ? QVariant(bytes(*e.parent_event_ref)) : QVariant{},
             bytes(e.root_event_ref), encode_plan_revision_uint64(e.receipt_id.value),
             e.contribution_id ? QVariant(encode_plan_revision_uint64(e.contribution_id->value))
                               : QVariant{},
             revision}));
    QList<QVariant> values{
        plan, revision,
        r.parent_revision ? QVariant(encode_plan_revision_uint64(*r.parent_revision)) : QVariant{},
        int(r.captured.contribution_state.payment_evidence)};
    values.append(condition_values(r.captured.contribution_state.conditions));
    values.append(int(r.captured.valuation.state));
    if (const auto &h = r.captured.valuation.historical) {
        const auto &q = *h;
        values.append(
            {bytes(q.resolution_ref), bytes(q.original_money.original_decimal),
             bytes(q.original_money.currency), bytes(q.comparison_currency),
             bytes(q.effective_rate_decimal),
             q.fetched_rate_decimal ? QVariant(bytes(*q.fetched_rate_decimal)) : QVariant{},
             bytes(q.snapshot_ref), q.source_ref ? QVariant(bytes(*q.source_ref)) : QVariant{},
             q.rate_timestamp_utc_ms ? QVariant(qint64(*q.rate_timestamp_utc_ms)) : QVariant{},
             q.fetch_timestamp_utc_ms ? QVariant(qint64(*q.fetch_timestamp_utc_ms)) : QVariant{},
             q.manual_decision_ref ? QVariant(bytes(*q.manual_decision_ref)) : QVariant{}});
    } else
        for (int i = 0; i < 11; ++i)
            values.append(QVariant{});
    if (const auto &decision = r.override_decision)
        values.append({int(decision->authority), bytes(decision->actor_ref),
                       bytes(decision->reason), bytes(decision->decision_ref)});
    else
        for (int i = 0; i < 4; ++i)
            values.append(QVariant{});
    statements.push_back(insertion("subathon_plan_revisions", revision_columns, std::move(values)));
    qint64 ordinal = 0;
    for (const auto &v : r.rules)
        statements.push_back(
            insertion("subathon_plan_rules", rule_columns,
                      {plan, revision, ++ordinal, bytes(v.rule_ref), bytes(v.version_ref),
                       int(v.selected), bytes(v.explanation)}));
    ordinal = 0;
    for (const auto &v : r.modifiers)
        statements.push_back(insertion(
            "subathon_plan_modifiers", modifier_columns,
            {plan, revision, ++ordinal, bytes(v.definition_version_ref),
             bytes(v.instance_revision_ref), bytes(v.target_ref), bytes(v.group_ref),
             bytes(v.group_operator), bytes(v.target_operator), bytes(v.original_factor_decimal)}));
    ordinal = 0;
    for (const auto &v : r.captured.requested_effects) {
        const auto components = encode_plan_exact(v.exact_requested_units, limits.component_bytes);
        statements.push_back(
            insertion("subathon_plan_requests", request_columns,
                      {plan, revision, ++ordinal, bytes(v.action_ref), bytes(v.action_version_ref),
                       bytes(v.rule_ref), bytes(v.target_ref), int(v.unit),
                       int(v.requires_historical_valuation), components.numerator,
                       components.denominator, qint64(v.requested_units),
                       bytes(v.calculation_explanation), bytes(v.reversal_behavior_ref)}));
    }
    if (!initial)
        statements.push_back(
            {"UPDATE main.subathon_plan_headers SET head_revision=? WHERE plan_ref=?",
             {revision, plan}});
    return statements;
}
SqlitePlanInitialDisposition Access::retain(const std::string &ref, const CapturedPlanRevision &r) {
    if (const auto h = header(ref)) {
        audit(ref, *h);
        auto old = selected_scalar(ref, 1, *h);
        if (!old)
            fail(Reason::invalid_stored_data);
        body(ref, *old);
        return *old == r ? SqlitePlanInitialDisposition::already_present
                         : SqlitePlanInitialDisposition::conflict;
    }
    source(r.captured.event);
    const auto statements = prepared_revision(ref, r, true, limits_);
    write(statements);
    return SqlitePlanInitialDisposition::inserted;
}
SqlitePlanAppendDisposition Access::append(const std::string &ref, std::uint64_t expected,
                                           const CapturedPlanRevision &r) {
    const auto h = header(ref);
    if (!h)
        return SqlitePlanAppendDisposition::missing_plan;
    audit(ref, *h);
    if (expected != h->head)
        return SqlitePlanAppendDisposition::stale_head;
    if (h->head == std::numeric_limits<std::uint64_t>::max())
        fail(Reason::revision_exhausted);
    if (r.revision != h->head + 1 || r.parent_revision != h->head)
        return SqlitePlanAppendDisposition::revision_conflict;
    if (r.captured.event != h->event)
        return SqlitePlanAppendDisposition::event_conflict;
    const auto statements = prepared_revision(ref, r, false, limits_);
    write(statements);
    return SqlitePlanAppendDisposition::appended;
}

boost::multiprecision::cpp_int integer_value(const QByteArray &text) {
    using boost::multiprecision::cpp_int;
    const bool negative = text.front() == '-';
    const qsizetype start = negative ? 1 : 0;
    if (start == text.size() || (text[start] == '0' && text.size() - start != 1) ||
        (negative && text[start] == '0'))
        invalid(Input::invalid_exact);
    cpp_int value = 0;
    for (qsizetype i = start; i < text.size(); ++i) {
        const auto ch = text[i];
        if (ch < '0' || ch > '9')
            invalid(Input::invalid_exact);
        value *= 10;
        value += ch - '0';
    }
    return negative ? -value : value;
}
} // namespace

QByteArray encode_plan_revision_uint64(std::uint64_t value) {
    QByteArray encoded(8, '\0');
    for (int i = 7; i >= 0; --i) {
        encoded[i] = char(value & 255U);
        value >>= 8;
    }
    return encoded;
}
std::uint64_t decode_plan_revision_uint64(const QByteArray &encoded) {
    if (encoded.isNull() || encoded.size() != 8)
        invalid(Input::invalid_codec);
    std::uint64_t value = 0;
    for (const auto byte : encoded)
        value = (value << 8) | static_cast<unsigned char>(byte);
    return value;
}
SqlitePlanExactComponents encode_plan_exact(const core::Exact &value, qsizetype limit) {
    if (limit <= 0)
        invalid(Input::invalid_options);
    const auto n = boost::multiprecision::numerator(value).str(),
               d = boost::multiprecision::denominator(value).str();
    if (n.size() > size_t(limit) || d.size() > size_t(limit))
        invalid(Input::exact_component_limit);
    return {bytes(n), bytes(d)};
}
core::Exact decode_plan_exact(const SqlitePlanExactComponents &components, qsizetype limit) {
    if (limit <= 0)
        invalid(Input::invalid_options);
    if (components.numerator.isEmpty() || components.denominator.isEmpty())
        invalid(Input::invalid_exact);
    if (components.numerator.size() > limit || components.denominator.size() > limit)
        invalid(Input::exact_component_limit);
    using boost::multiprecision::cpp_int;
    const cpp_int n = integer_value(components.numerator),
                  d = integer_value(components.denominator);
    if (d <= 0 || (n == 0 && d != 1))
        invalid(Input::invalid_exact);
    cpp_int a = n < 0 ? -n : n, b = d;
    while (b != 0) {
        cpp_int remainder = a % b;
        a = std::move(b);
        b = std::move(remainder);
    }
    if (a != 1)
        invalid(Input::invalid_exact);
    return core::Exact(n) / core::Exact(d);
}
MigrationManifest application_plan_repository_manifest() {
    auto manifest = contribution_repository_manifest();
    MigrationStep step;
    step.version = 4;
    step.id = "captured-application-plans-v1";
    for (const auto &sql : migration_sql)
        step.sql.append(sql);
    step.expected_schema = manifest.steps.back().expected_schema;
    const QStringList tables{"subathon_plan_headers", "subathon_plan_revisions",
                             "subathon_plan_rules", "subathon_plan_modifiers",
                             "subathon_plan_requests"};
    for (qsizetype i = 0; i < tables.size(); ++i) {
        step.expected_schema.append({"table", tables[i], tables[i], migration_sql[size_t(i)]});
        step.expected_schema.append(
            {"index", "sqlite_autoindex_" + tables[i] + "_1", tables[i], std::nullopt});
    }
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return a.type < b.type || (a.type == b.type && a.name < b.name);
              });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions = {0, 1, 2, 3, 4};
    return manifest;
}
SqliteApplicationPlanStore::SqliteApplicationPlanStore(SqliteWorkSession session,
                                                       SqlitePlanLimits limits)
    : session_(std::move(session)), limits_(limits) {}
SqlitePlanInitialDisposition
SqliteApplicationPlanStore::retain_initial(const std::string &ref,
                                           const core::CapturedPlanRevision &r) const {
    preflight(ref, limits_);
    validate_revision(r, true, limits_);
    Access access(session_, limits_);
    return access.guarded([&] { return access.retain(ref, r); });
}
std::optional<core::ApplicationPlan>
SqliteApplicationPlanStore::find(const std::string &ref) const {
    preflight(ref, limits_);
    Access access(session_, limits_);
    return access.guarded([&] { return access.find(ref); });
}
std::optional<SqlitePlanRevisionSnapshot>
SqliteApplicationPlanStore::find_revision(const std::string &ref, std::uint64_t revision) const {
    preflight(ref, limits_);
    if (revision == 0)
        invalid(Input::invalid_revision);
    Access access(session_, limits_);
    return access.guarded([&] { return access.targeted(ref, revision); });
}
SqlitePlanAppendDisposition
SqliteApplicationPlanStore::append(const std::string &ref, std::uint64_t expected,
                                   const core::CapturedPlanRevision &r) const {
    preflight(ref, limits_);
    validate_revision(r, false, limits_);
    Access access(session_, limits_);
    return access.guarded([&] { return access.append(ref, expected, r); });
}
} // namespace subathon::server
