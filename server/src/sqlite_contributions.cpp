// SPDX-License-Identifier: GPL-3.0-or-later
#include <QStringList>
#include <QThread>
#include <algorithm>
#include <array>
#include <limits>
#include <subathon/server/sqlite_contributions.hpp>
#include <subathon/server/sqlite_receipts.hpp>
#include <subathon/server/sqlite_retained_context.hpp>
#include <utility>

namespace subathon::server {
namespace {
// Exact approved migration3 literals. Earlier factories retain immutable prefixes1/2.
const std::array<QString, 4> migration_sql = {
    "CREATE TABLE subathon_contribution_allocator(singleton INTEGER PRIMARY KEY CHECK(typeof(si"
    "ngleton)='integer' AND singleton=1), last_allocated BLOB CHECK(last_allocated IS NULL OR ("
    "typeof(last_allocated)='blob' AND length(last_allocated)=8)))",
    "CREATE TABLE subathon_contributions(contribution_id BLOB NOT NULL PRIMARY KEY CHECK(typeof"
    "(contribution_id)='blob' AND length(contribution_id)=8), original_receipt_id BLOB NOT NULL"
    " UNIQUE CHECK(typeof(original_receipt_id)='blob' AND length(original_receipt_id)=8 AND ori"
    "ginal_receipt_id<>X'0000000000000000') REFERENCES subathon_receipts(receipt_id), activity_"
    "provider BLOB, activity_source_account BLOB, activity_namespace BLOB, activity_id BLOB CHE"
    "CK((activity_provider IS NULL AND activity_source_account IS NULL AND activity_namespace I"
    "S NULL AND activity_id IS NULL) OR (typeof(activity_provider)='blob' AND length(activity_p"
    "rovider)>0 AND typeof(activity_source_account)='blob' AND length(activity_source_account)>"
    "0 AND typeof(activity_namespace)='blob' AND length(activity_namespace)>0 AND typeof(activi"
    "ty_id)='blob' AND length(activity_id)>0)), payment_evidence INTEGER NOT NULL CHECK(typeof("
    "payment_evidence)='integer' AND payment_evidence IN (0,1,2,3)), monetary INTEGER NOT NULL "
    "CHECK(typeof(monetary)='integer' AND monetary IN (0,1)), original_decimal BLOB, currency B"
    "LOB CHECK((original_decimal IS NULL AND currency IS NULL) OR (typeof(original_decimal)='bl"
    "ob' AND typeof(currency)='blob')), resolution_ref BLOB CHECK(resolution_ref IS NULL OR typ"
    "eof(resolution_ref)='blob'), missing_rate INTEGER NOT NULL CHECK(typeof(missing_rate)='int"
    "eger' AND missing_rate IN (0,1)), review INTEGER NOT NULL CHECK(typeof(review)='integer' A"
    "ND review IN (0,1,2,3)), evidence_policy INTEGER NOT NULL CHECK(typeof(evidence_policy)='i"
    "nteger' AND evidence_policy IN (0,1,2)), policy_ref BLOB NOT NULL CHECK(typeof(policy_ref)"
    "='blob'), decision_choice INTEGER, decision_ref BLOB, decision_actor_ref BLOB CHECK((decis"
    "ion_choice IS NULL AND decision_ref IS NULL AND decision_actor_ref IS NULL) OR (typeof(dec"
    "ision_choice)='integer' AND decision_choice IN (0,1) AND typeof(decision_ref)='blob' AND t"
    "ypeof(decision_actor_ref)='blob')), application INTEGER NOT NULL CHECK(typeof(application)"
    "='integer' AND application IN (0,1)), reversal INTEGER NOT NULL CHECK(typeof(reversal)='in"
    "teger' AND reversal IN (0,1,2)), settlement INTEGER NOT NULL CHECK(typeof(settlement)='int"
    "eger' AND settlement IN (0,1)), UNIQUE(activity_provider,activity_source_account,activity_"
    "namespace,activity_id))",
    "CREATE TABLE subathon_contribution_observations(receipt_id BLOB NOT NULL PRIMARY KEY CHECK"
    "(typeof(receipt_id)='blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000') R"
    "EFERENCES subathon_receipts(receipt_id), contribution_id BLOB CHECK(contribution_id IS NUL"
    "L OR (typeof(contribution_id)='blob' AND length(contribution_id)=8)) REFERENCES subathon_c"
    "ontributions(contribution_id), kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind"
    " IN (0,1)), evidence INTEGER NOT NULL CHECK(typeof(evidence)='integer' AND evidence IN (0,"
    "1,2,3)), activity_provider BLOB, activity_source_account BLOB, activity_namespace BLOB, ac"
    "tivity_id BLOB, verification_ref BLOB CHECK((activity_provider IS NULL AND activity_source"
    "_account IS NULL AND activity_namespace IS NULL AND activity_id IS NULL AND verification_r"
    "ef IS NULL) OR (typeof(activity_provider)='blob' AND length(activity_provider)>0 AND typeo"
    "f(activity_source_account)='blob' AND length(activity_source_account)>0 AND typeof(activit"
    "y_namespace)='blob' AND length(activity_namespace)>0 AND typeof(activity_id)='blob' AND le"
    "ngth(activity_id)>0 AND typeof(verification_ref)='blob' AND length(verification_ref)>0)), "
    "support_monetary INTEGER, support_original_decimal BLOB, support_currency BLOB, support_re"
    "solution_ref BLOB, support_missing_rate INTEGER, support_review INTEGER, support_evidence_"
    "policy INTEGER, support_policy_ref BLOB, support_decision_choice INTEGER, support_decision"
    "_ref BLOB, support_decision_actor_ref BLOB, support_application INTEGER, support_reversal "
    "INTEGER, support_settlement INTEGER, CHECK((kind=0 AND typeof(support_monetary)='integer' "
    "AND support_monetary IN (0,1) AND typeof(support_missing_rate)='integer' AND support_missi"
    "ng_rate IN (0,1) AND typeof(support_review)='integer' AND support_review IN (0,1,2,3) AND "
    "typeof(support_evidence_policy)='integer' AND support_evidence_policy IN (0,1,2) AND typeo"
    "f(support_application)='integer' AND support_application IN (0,1) AND typeof(support_rever"
    "sal)='integer' AND support_reversal IN (0,1,2) AND typeof(support_settlement)='integer' AN"
    "D support_settlement IN (0,1) AND ((support_original_decimal IS NULL AND support_currency "
    "IS NULL) OR (typeof(support_original_decimal)='blob' AND typeof(support_currency)='blob'))"
    " AND (support_resolution_ref IS NULL OR typeof(support_resolution_ref)='blob') AND typeof("
    "support_policy_ref)='blob' AND ((support_decision_choice IS NULL AND support_decision_ref "
    "IS NULL AND support_decision_actor_ref IS NULL) OR (typeof(support_decision_choice)='integ"
    "er' AND support_decision_choice IN (0,1) AND typeof(support_decision_ref)='blob' AND typeo"
    "f(support_decision_actor_ref)='blob'))) OR (kind=1 AND evidence=2 AND (support_monetary IS"
    " NULL AND support_original_decimal IS NULL AND support_currency IS NULL AND support_resolu"
    "tion_ref IS NULL AND support_missing_rate IS NULL AND support_review IS NULL AND support_e"
    "vidence_policy IS NULL AND support_policy_ref IS NULL AND support_decision_choice IS NULL "
    "AND support_decision_ref IS NULL AND support_decision_actor_ref IS NULL AND support_applic"
    "ation IS NULL AND support_reversal IS NULL AND support_settlement IS NULL))))",
    "INSERT INTO subathon_contribution_allocator(singleton,last_allocated) VALUES (1,NULL)",
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
const QStringList activity_columns{"activity_provider", "activity_source_account",
                                   "activity_namespace", "activity_id"};
const QStringList condition_columns{"monetary",        "original_decimal",   "currency",
                                    "resolution_ref",  "missing_rate",       "review",
                                    "evidence_policy", "policy_ref",         "decision_choice",
                                    "decision_ref",    "decision_actor_ref", "application",
                                    "reversal",        "settlement"};
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
[[noreturn]] void invalid(SqliteContributionInputError error) {
    throw SqliteContributionInputFailure(error);
}
bool bounded(const std::string &value, qsizetype limit, bool required = false) {
    return (!required || !value.empty()) && value.size() <= size_t(limit);
}
void validate_options(SqliteContributionLimits limits) {
    if (limits.reference_bytes <= 0 || limits.value_bytes <= 0 ||
        limits.unresolved_per_lookup <= 0 ||
        std::uintmax_t(limits.unresolved_per_lookup) >=
            std::uintmax_t(std::numeric_limits<qsizetype>::max()) ||
        std::uintmax_t(limits.unresolved_per_lookup) >=
            std::uintmax_t(std::numeric_limits<qint64>::max()) ||
        std::uintmax_t(limits.unresolved_per_lookup) >=
            std::vector<ContributionEvidenceRecord>{}.max_size())
        invalid(SqliteContributionInputError::invalid_options);
}
bool valid_activity(const VerifiedActivityIdentity &key, SqliteContributionLimits limits) {
    return bounded(key.origin.provider, limits.reference_bytes, true) &&
           bounded(key.origin.source_account, limits.reference_bytes, true) &&
           bounded(key.identifier_namespace, limits.reference_bytes, true) &&
           bounded(key.opaque_id, limits.reference_bytes, true);
}
void validate_activity(const VerifiedActivityIdentity &key, SqliteContributionLimits limits) {
    if (!valid_activity(key, limits))
        invalid(SqliteContributionInputError::invalid_activity);
}
void validate_receipt_id(core::ReceiptId id) {
    if (id.value == 0)
        invalid(SqliteContributionInputError::invalid_receipt_id);
}
void validate_receipt(const ContributionReceipt &receipt, SqliteContributionLimits limits) {
    validate_receipt_id(receipt.id);
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
        invalid(SqliteContributionInputError::invalid_receipt);
}
template <class Enum> bool enum_in(Enum value, int maximum) {
    return int(value) >= 0 && int(value) <= maximum;
}
void validate_conditions(const core::ContributionConditions &c, SqliteContributionLimits limits) {
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
        invalid(SqliteContributionInputError::invalid_state);
}
void validate_state(const core::ContributionState &state, SqliteContributionLimits limits) {
    if (!enum_in(state.payment_evidence, 3))
        invalid(SqliteContributionInputError::invalid_state);
    validate_conditions(state.conditions, limits);
}
void validate_observation(const ContributionObservation &o, SqliteContributionLimits limits) {
    validate_receipt(o.receipt, limits);
    if (!enum_in(o.kind, 1) || !enum_in(o.evidence, 3) ||
        (o.kind == ContributionObservationKind::support && !o.support_conditions) ||
        (o.kind == ContributionObservationKind::confirmation &&
         (o.support_conditions || o.evidence != core::PaymentEvidence::confirmed)))
        invalid(SqliteContributionInputError::invalid_observation);
    if (o.verified_activity &&
        (!valid_activity(o.verified_activity->identity, limits) ||
         o.verified_activity->identity.origin != o.receipt.identity.origin ||
         !bounded(o.verified_activity->verification_ref, limits.reference_bytes, true)))
        invalid(SqliteContributionInputError::invalid_activity);
    if (o.support_conditions)
        validate_conditions(*o.support_conditions, limits);
}
QList<QVariant> activity_values(const std::optional<VerifiedActivityIdentity> &key) {
    if (!key)
        return {QVariant{}, QVariant{}, QVariant{}, QVariant{}};
    return {bytes(key->origin.provider), bytes(key->origin.source_account),
            bytes(key->identifier_namespace), bytes(key->opaque_id)};
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
void check_owner(QThread *owner) {
    if (QThread::currentThread() != owner)
        throw SqliteWorkFailure(WorkError::wrong_thread);
}
[[noreturn]] void map_work_failure(const SqliteWorkFailure &failure) {
    switch (failure.error()) {
    case WorkError::busy:
        throw SqliteContributionStorageFailure(SqliteContributionStorageReason::busy);
    case WorkError::sql_failure:
        throw SqliteContributionStorageFailure(SqliteContributionStorageReason::sql_failure);
    case WorkError::resource_limit:
        throw SqliteContributionStorageFailure(SqliteContributionStorageReason::resource_limit);
    case WorkError::result_limit:
        throw SqliteContributionStorageFailure(SqliteContributionStorageReason::result_limit);
    default:
        throw;
    }
}

class SqliteContributionTransaction final : public ContributionTransaction {
  public:
    SqliteContributionTransaction(SqliteContributionLimits limits, QThread *owner)
        : limits_(limits), owner_(owner) {}
    ~SqliteContributionTransaction() override {
        if (owns_session_)
            (void)session_.cleanup();
    }
    void adopt(SqliteWorkSession session) noexcept {
        session_ = std::move(session);
        owns_session_ = true;
    }
    std::optional<ContributionEvidenceRecord> find_receipt(core::ReceiptId receipt) override {
        validate_receipt_id(receipt);
        check_owner(owner_);
        return guarded([&] { return observation(receipt, true); });
    }
    std::optional<ContributionRecord> find_activity(const VerifiedActivityIdentity &key) override {
        validate_activity(key, limits_);
        check_owner(owner_);
        return guarded([&] {
            auto result = contribution("activity_provider=? AND activity_source_account=? AND "
                                       "activity_namespace=? AND activity_id=?",
                                       activity_values(key));
            if (result && result->activity_identity != std::optional<VerifiedActivityIdentity>(key))
                fail(SqliteContributionStorageReason::invalid_stored_data);
            return result;
        });
    }
    std::vector<ContributionEvidenceRecord>
    unresolved_activity(const VerifiedActivityIdentity &key) override {
        validate_activity(key, limits_);
        check_owner(owner_);
        return guarded([&] {
            auto bindings = activity_values(key);
            bindings.append(qint64(limits_.unresolved_per_lookup + 1));
            const auto rows = execute(
                {"SELECT " + projection(observation_columns()) +
                     " FROM main.subathon_contribution_observations WHERE activity_provider=? AND "
                     "activity_source_account=? AND activity_namespace=? AND activity_id=? AND "
                     "contribution_id IS NULL ORDER BY receipt_id LIMIT ?",
                 std::move(bindings)});
            if (rows.size() > limits_.unresolved_per_lookup)
                fail(SqliteContributionStorageReason::result_limit);
            std::vector<ContributionEvidenceRecord> result;
            result.reserve(size_t(rows.size()));
            std::optional<std::uint64_t> previous;
            for (const auto &row : rows) {
                auto record = read_observation(row);
                if (record.contribution_id || !record.observation.verified_activity ||
                    record.observation.verified_activity->identity != key ||
                    (previous && record.observation.receipt.id.value <= *previous))
                    fail(SqliteContributionStorageReason::invalid_stored_data);
                previous = record.observation.receipt.id.value;
                result.push_back(std::move(record));
            }
            return result;
        });
    }
    core::ContributionId create(const ContributionReceipt &support,
                                const std::optional<VerifiedActivityIdentity> &key,
                                const core::ContributionState &state) override {
        validate_receipt(support, limits_);
        if (key && (!valid_activity(*key, limits_) || key->origin != support.identity.origin))
            invalid(SqliteContributionInputError::invalid_activity);
        validate_state(state, limits_);
        check_owner(owner_);
        return guarded([&] {
            require_receipt(support);
            const core::ContributionId candidate{next_id()};
            if (pending_.size() == pending_.max_size())
                throw std::length_error("Too many pending contribution anchors");
            pending_.reserve(pending_.size() + 1);
            const auto encoded = encode_contribution_id(candidate);
            QList<QVariant> values{encoded, encode_receipt_uint64(support.id.value)};
            values.append(activity_values(key));
            values.append(int(state.payment_evidence));
            values.append(condition_values(state.conditions));
            const auto insert =
                insertion("subathon_contributions", contribution_columns(), std::move(values));
            const SqlStatement update{"UPDATE main.subathon_contribution_allocator SET "
                                      "last_allocated=? WHERE singleton=1",
                                      {encoded}};
            wrote_ = true;
            (void)execute(insert);
            (void)execute(update);
            pending_.push_back(candidate.value);
            return candidate;
        });
    }
    void append_observation(const ContributionEvidenceRecord &record) override {
        validate_observation(record.observation, limits_);
        check_owner(owner_);
        guarded([&] {
            require_receipt(record.observation.receipt);
            if (record.contribution_id) {
                auto target = contribution_by_id(*record.contribution_id);
                if (!target)
                    fail(SqliteContributionStorageReason::missing_contribution);
                if (!compatible(record.observation, *target))
                    fail(SqliteContributionStorageReason::association_conflict);
            }
            const auto &o = record.observation;
            QList<QVariant> values{encode_receipt_uint64(o.receipt.id.value),
                                   record.contribution_id
                                       ? QVariant(encode_contribution_id(*record.contribution_id))
                                       : QVariant{},
                                   int(o.kind), int(o.evidence)};
            values.append(activity_values(
                o.verified_activity
                    ? std::optional<VerifiedActivityIdentity>(o.verified_activity->identity)
                    : std::nullopt));
            values.append(o.verified_activity
                              ? QVariant(bytes(o.verified_activity->verification_ref))
                              : QVariant{});
            if (o.support_conditions)
                values.append(condition_values(*o.support_conditions));
            else
                for (qsizetype i = 0; i < condition_columns.size(); ++i)
                    values.append(QVariant{});
            const auto insert = insertion("subathon_contribution_observations",
                                          observation_columns(), std::move(values));
            wrote_ = true;
            (void)execute(insert);
        });
    }
    void link_observation(core::ReceiptId receipt, core::ContributionId target_id) override {
        validate_receipt_id(receipt);
        check_owner(owner_);
        guarded([&] {
            auto old = observation(receipt, true);
            if (!old)
                fail(SqliteContributionStorageReason::missing_observation);
            auto target = contribution_by_id(target_id);
            if (!target)
                fail(SqliteContributionStorageReason::missing_contribution);
            if ((old->contribution_id && *old->contribution_id != target_id) ||
                !compatible(old->observation, *target))
                fail(SqliteContributionStorageReason::association_conflict);
            if (old->contribution_id)
                return;
            const SqlStatement update{
                "UPDATE main.subathon_contribution_observations SET contribution_id=? WHERE "
                "receipt_id=?",
                {encode_contribution_id(target_id), encode_receipt_uint64(receipt.value)}};
            wrote_ = true;
            (void)execute(update);
        });
    }
    void update_state(core::ContributionId id, const core::ContributionState &state) override {
        validate_state(state, limits_);
        check_owner(owner_);
        guarded([&] {
            if (!contribution_by_id(id))
                fail(SqliteContributionStorageReason::missing_contribution);
            QStringList set{"payment_evidence=?"};
            for (const auto &column : condition_columns)
                set.append(column + "=?");
            QList<QVariant> values{int(state.payment_evidence)};
            values.append(condition_values(state.conditions));
            values.append(encode_contribution_id(id));
            const SqlStatement update{"UPDATE main.subathon_contributions SET " + set.join(',') +
                                          " WHERE contribution_id=?",
                                      std::move(values)};
            wrote_ = true;
            (void)execute(update);
        });
    }
    void commit() override {
        check_owner(owner_);
        guarded([&] {
            for (const auto value : pending_) {
                const auto record =
                    contribution("contribution_id=?", {encode_contribution_id({value})}, false);
                if (!record ||
                    !anchor_matches(*record, observation(record->original_receipt_id, false)))
                    fail(SqliteContributionStorageReason::incomplete_contribution);
            }
            try {
                session_.commit();
            } catch (const SqliteWorkFailure &e) {
                map_work_failure(e);
            }
            pending_.clear();
        });
    }

  private:
    SqliteWorkSession session_;
    const SqliteContributionLimits limits_;
    QThread *const owner_;
    bool owns_session_{false}, wrote_{false};
    std::vector<std::uint64_t> pending_;
    void cleanup() {
        const auto error = session_.cleanup();
        if (error != WorkError::none)
            throw SqliteWorkFailure(error);
        pending_.clear();
    }
    [[noreturn]] void fail(SqliteContributionStorageReason reason) {
        // Every local refusal follows a successful active-session query. Real
        // rollback, rather than inactive cleanup, preserves capability truth.
        session_.rollback();
        pending_.clear();
        throw SqliteContributionStorageFailure(reason);
    }
    template <class Operation> auto guarded(Operation &&operation) -> decltype(operation()) {
        try {
            return operation();
        } catch (const SqliteWorkFailure &) {
            throw;
        } catch (const SqliteContributionStorageFailure &) {
            throw;
        } catch (...) {
            if (wrote_)
                cleanup();
            throw;
        }
    }
    SqlRows execute(const SqlStatement &statement) {
        try {
            return session_.execute(statement);
        } catch (const SqliteWorkFailure &e) {
            map_work_failure(e);
        }
    }
    bool pending(std::uint64_t value) const {
        return std::find(pending_.begin(), pending_.end(), value) != pending_.end();
    }
    void shape(const SqlRow &row, qsizetype count) {
        if (row.size() != count * 3)
            fail(SqliteContributionStorageReason::invalid_stored_data);
    }
    bool type(const SqlRow &row, qsizetype index, const char *expected) const {
        return row[index * 3 + 1].metaType().id() == QMetaType::QString &&
               row[index * 3 + 1].toString() == expected;
    }
    bool null(const SqlRow &row, qsizetype index) {
        if (!type(row, index, "null"))
            return false;
        if (!row[index * 3].isNull() || !row[index * 3 + 2].isNull())
            fail(SqliteContributionStorageReason::invalid_stored_data);
        return true;
    }
    qint64 integer(const SqlRow &row, qsizetype index) {
        const auto &value = row[index * 3];
        if (!type(row, index, "integer") || (value.metaType().id() != QMetaType::Int &&
                                             value.metaType().id() != QMetaType::LongLong))
            fail(SqliteContributionStorageReason::invalid_stored_data);
        return value.toLongLong();
    }
    qint64 enumeration(const SqlRow &row, qsizetype index, qint64 maximum) {
        const auto value = integer(row, index);
        if (value < 0 || value > maximum)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        return value;
    }
    std::string string(const SqlRow &row, qsizetype index, qsizetype bound, bool required = false) {
        const auto &value = row[index * 3], &length = row[index * 3 + 2];
        if (!type(row, index, "blob") ||
            (length.metaType().id() != QMetaType::Int &&
             length.metaType().id() != QMetaType::LongLong) ||
            length.toLongLong() < 0)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        const auto size = length.toLongLong();
        if (!(size == 0 && value.isNull()) &&
            (value.metaType().id() != QMetaType::QByteArray || value.toByteArray().size() != size))
            fail(SqliteContributionStorageReason::invalid_stored_data);
        if (required && size == 0)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        if (size > bound)
            fail(SqliteContributionStorageReason::result_limit);
        const auto data = value.toByteArray();
        return size == 0 ? std::string{} : std::string(data.constData(), size_t(data.size()));
    }
    std::uint64_t id(const SqlRow &row, qsizetype index, bool receipt = false) {
        if (!type(row, index, "blob") || row[index * 3 + 2].toLongLong() != 8)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        const auto value = decode_contribution_id(bytes(string(row, index, 8, true))).value;
        if (receipt && value == 0)
            fail(SqliteContributionStorageReason::invalid_stored_data);
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
            fail(SqliteContributionStorageReason::invalid_stored_data);
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
            fail(SqliteContributionStorageReason::invalid_stored_data);
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
    std::optional<VerifiedActivityIdentity> read_activity(const SqlRow &row, qsizetype offset) {
        bool absent = null(row, offset);
        for (qsizetype i = 1; i < 4; ++i)
            if (null(row, offset + i) != absent)
                fail(SqliteContributionStorageReason::invalid_stored_data);
        if (absent)
            return std::nullopt;
        return VerifiedActivityIdentity{{string(row, offset, limits_.reference_bytes, true),
                                         string(row, offset + 1, limits_.reference_bytes, true)},
                                        string(row, offset + 2, limits_.reference_bytes, true),
                                        string(row, offset + 3, limits_.reference_bytes, true)};
    }
    void resolve(const core::ReceiptArrival &arrival) {
        try {
            const SqliteRetainedContextStore retained(
                session_, {limits_.reference_bytes, limits_.value_bytes});
            if (!retained.find({RetainedKind::configuration, arrival.configuration_ref}) ||
                !retained.find({RetainedKind::arrival_context, arrival.modifier_context_ref}))
                fail(SqliteContributionStorageReason::invalid_stored_data);
        } catch (const RetentionFailure &e) {
            // The retained primitive already established rollback; do not replace
            // its outcome with a second control operation on the closed unit.
            if (e.error() == RetentionError::invalid_stored_data)
                throw SqliteContributionStorageFailure(
                    SqliteContributionStorageReason::invalid_stored_data);
            if (e.error() == RetentionError::result_limit)
                throw SqliteContributionStorageFailure(
                    SqliteContributionStorageReason::result_limit);
            throw;
        } catch (const SqliteWorkFailure &e) {
            map_work_failure(e);
        }
    }
    std::optional<ContributionReceipt> original(core::ReceiptId receipt) {
        const auto rows = execute({"SELECT " + projection(receipt_columns) +
                                       " FROM main.subathon_receipts WHERE receipt_id=? LIMIT 2",
                                   {encode_receipt_uint64(receipt.value)}});
        if (rows.isEmpty())
            return std::nullopt;
        if (rows.size() != 1)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        const auto &row = rows.front();
        shape(row, 20);
        if (id(row, 0, true) != receipt.value)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        core::ReceiptIdentity identity{{string(row, 1, limits_.reference_bytes, true),
                                        string(row, 2, limits_.reference_bytes, true)},
                                       string(row, 3, limits_.reference_bytes, true),
                                       string(row, 4, limits_.reference_bytes, true)};
        core::ReceiptFacts facts;
        facts.event_type = string(row, 5, limits_.value_bytes);
        const auto schema = integer(row, 6);
        if (schema < 0 || std::uint64_t(schema) > std::numeric_limits<std::uint32_t>::max())
            fail(SqliteContributionStorageReason::invalid_stored_data);
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
    void require_receipt(const ContributionReceipt &supplied) {
        const auto stored = original(supplied.id);
        if (!stored)
            fail(SqliteContributionStorageReason::missing_receipt);
        if (*stored != supplied)
            fail(SqliteContributionStorageReason::receipt_mismatch);
    }
    ContributionEvidenceRecord read_observation(const SqlRow &row) {
        shape(row, 23);
        const core::ReceiptId receipt{id(row, 0, true)};
        auto source = original(receipt);
        if (!source)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        std::optional<core::ContributionId> associated;
        if (!null(row, 1))
            associated = core::ContributionId{id(row, 1)};
        const auto kind = static_cast<ContributionObservationKind>(enumeration(row, 2, 1));
        const auto evidence = static_cast<core::PaymentEvidence>(enumeration(row, 3, 3));
        auto activity = read_activity(row, 4);
        std::optional<VerifiedActivityLink> verified;
        if (activity) {
            if (activity->origin != source->identity.origin)
                fail(SqliteContributionStorageReason::invalid_stored_data);
            verified = VerifiedActivityLink{std::move(*activity),
                                            string(row, 8, limits_.reference_bytes, true)};
        } else if (!null(row, 8))
            fail(SqliteContributionStorageReason::invalid_stored_data);
        std::optional<core::ContributionConditions> conditions;
        if (kind == ContributionObservationKind::support)
            conditions = read_conditions(row, 9);
        else {
            if (evidence != core::PaymentEvidence::confirmed)
                fail(SqliteContributionStorageReason::invalid_stored_data);
            for (qsizetype i = 9; i < 23; ++i)
                if (!null(row, i))
                    fail(SqliteContributionStorageReason::invalid_stored_data);
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
            fail(SqliteContributionStorageReason::invalid_stored_data);
        const auto &row = rows.front();
        shape(row, 21);
        const core::ContributionId cid{id(row, 0)};
        const core::ReceiptId rid{id(row, 1, true)};
        auto key = read_activity(row, 2);
        auto receipt = original(rid);
        if (!receipt || (key && key->origin != receipt->identity.origin))
            fail(SqliteContributionStorageReason::invalid_stored_data);
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
            if (support || !pending(cid.value)) {
                if (!anchor_matches(result, support))
                    fail(SqliteContributionStorageReason::invalid_stored_data);
            }
        }
        return result;
    }
    std::optional<ContributionRecord> contribution_by_id(core::ContributionId cid) {
        auto result = contribution("contribution_id=?", {encode_contribution_id(cid)});
        if (result && result->id != cid)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        return result;
    }
    std::optional<ContributionEvidenceRecord> observation(core::ReceiptId rid,
                                                          bool validate_association) {
        const auto rows =
            execute({"SELECT " + projection(observation_columns()) +
                         " FROM main.subathon_contribution_observations WHERE receipt_id=? LIMIT 2",
                     {encode_receipt_uint64(rid.value)}});
        if (rows.isEmpty())
            return std::nullopt;
        if (rows.size() != 1)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        auto result = read_observation(rows.front());
        if (result.observation.receipt.id != rid)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        if (validate_association && result.contribution_id) {
            const auto target = contribution_by_id(*result.contribution_id);
            if (!target || !compatible(result.observation, *target))
                fail(SqliteContributionStorageReason::invalid_stored_data);
        }
        return result;
    }
    std::uint64_t next_id() {
        const auto rows = execute({"SELECT " + projection({"singleton", "last_allocated"}) +
                                       " FROM main.subathon_contribution_allocator",
                                   {}});
        if (rows.size() != 1)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        shape(rows.front(), 2);
        if (integer(rows.front(), 0) != 1)
            fail(SqliteContributionStorageReason::invalid_stored_data);
        const bool absent = null(rows.front(), 1);
        const auto last = absent ? std::uint64_t(0) : id(rows.front(), 1);
        const auto greatest =
            execute({"SELECT " + projection({"contribution_id"}) +
                         " FROM main.subathon_contributions ORDER BY contribution_id DESC LIMIT 1",
                     {}});
        if (greatest.size() > 1 || absent != greatest.isEmpty())
            fail(SqliteContributionStorageReason::invalid_stored_data);
        if (!greatest.isEmpty()) {
            shape(greatest.front(), 1);
            if (id(greatest.front(), 0) != last)
                fail(SqliteContributionStorageReason::invalid_stored_data);
        }
        if (!absent && last == std::numeric_limits<std::uint64_t>::max())
            fail(SqliteContributionStorageReason::allocator_exhausted);
        return absent ? 0 : last + 1;
    }
};
} // namespace
QByteArray encode_contribution_id(core::ContributionId id) {
    return encode_receipt_uint64(id.value);
}
core::ContributionId decode_contribution_id(const QByteArray &encoded) {
    if (encoded.isNull() || encoded.size() != 8)
        invalid(SqliteContributionInputError::invalid_codec);
    std::uint64_t value = 0;
    for (const auto byte : encoded)
        value = (value << 8) | static_cast<unsigned char>(byte);
    return {value};
}
MigrationManifest contribution_repository_manifest() {
    auto manifest = receipt_repository_manifest();
    MigrationStep step;
    step.version = 3;
    step.id = "contribution-evidence-v1";
    for (const auto &sql : migration_sql)
        step.sql.append(sql);
    step.expected_schema = manifest.steps.back().expected_schema;
    const QStringList tables{"subathon_contribution_allocator", "subathon_contributions",
                             "subathon_contribution_observations"};
    for (qsizetype i = 0; i < 3; ++i)
        step.expected_schema.append({"table", tables[i], tables[i], migration_sql[size_t(i)]});
    step.expected_schema.append({"index", "sqlite_autoindex_subathon_contribution_observations_1",
                                 "subathon_contribution_observations", std::nullopt});
    for (int i = 1; i <= 3; ++i)
        step.expected_schema.append(
            {"index", "sqlite_autoindex_subathon_contributions_" + QString::number(i),
             "subathon_contributions", std::nullopt});
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return a.type < b.type || (a.type == b.type && a.name < b.name);
              });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions = {0, 1, 2, 3};
    return manifest;
}
SqliteContributionRepository::SqliteContributionRepository(SqliteWorkContext context,
                                                           SqliteContributionLimits limits)
    : capability_(std::move(context)), construction_thread_(QThread::currentThread()),
      limits_(limits) {}
SqliteContributionRepository::SqliteContributionRepository(AdoptActiveContributionSessionTag,
                                                           SqliteWorkSession session,
                                                           SqliteContributionLimits limits)
    : capability_(std::move(session)), construction_thread_(QThread::currentThread()),
      limits_(limits) {}
std::unique_ptr<ContributionTransaction> SqliteContributionRepository::begin() {
    validate_options(limits_);
    check_owner(construction_thread_);
    if (begun_)
        invalid(SqliteContributionInputError::already_begun);
    auto transaction =
        std::make_unique<SqliteContributionTransaction>(limits_, construction_thread_);
    if (const auto *context = std::get_if<SqliteWorkContext>(&capability_)) {
        begun_ = true;
        try {
            transaction->adopt(context->begin());
        } catch (const SqliteWorkFailure &e) {
            map_work_failure(e);
        }
    } else {
        transaction->adopt(std::get<SqliteWorkSession>(capability_));
        begun_ = true;
    }
    return transaction;
}
} // namespace subathon::server
