// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_receipts.hpp>
#include <subathon/server/sqlite_retained_context.hpp>

#include <QStringList>
#include <QThread>
#include <algorithm>
#include <array>
#include <limits>
#include <utility>
#include <vector>

namespace subathon::server {
namespace {
// Exact approved migration2 values; prefix1 is supplied by its immutable factory.
const std::array<QString, 4> migration_sql = {
    "CREATE TABLE subathon_receipt_allocator(singleton INTEGER PRIMARY KEY CHECK(typeof(singlet"
    "on)='integer' AND singleton=1), last_allocated BLOB CHECK(last_allocated IS NULL OR (typeo"
    "f(last_allocated)='blob' AND length(last_allocated)=8 AND last_allocated<>X'00000000000000"
    "00')))",
    "CREATE TABLE subathon_receipts(receipt_id BLOB NOT NULL PRIMARY KEY CHECK(typeof(receipt_i"
    "d)='blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000'), provider BLOB NOT"
    " NULL CHECK(typeof(provider)='blob' AND length(provider)>0), source_account BLOB NOT NULL "
    "CHECK(typeof(source_account)='blob' AND length(source_account)>0), identifier_namespace BL"
    "OB NOT NULL CHECK(typeof(identifier_namespace)='blob' AND length(identifier_namespace)>0),"
    " opaque_id BLOB NOT NULL CHECK(typeof(opaque_id)='blob' AND length(opaque_id)>0), event_ty"
    "pe BLOB NOT NULL CHECK(typeof(event_type)='blob'), schema_version INTEGER NOT NULL CHECK(t"
    "ypeof(schema_version)='integer' AND schema_version BETWEEN 0 AND 4294967295), source_times"
    "tamp_utc_ms INTEGER CHECK(source_timestamp_utc_ms IS NULL OR typeof(source_timestamp_utc_m"
    "s)='integer'), actor_id BLOB CHECK(actor_id IS NULL OR typeof(actor_id)='blob'), actor_dis"
    "play_name BLOB CHECK(actor_display_name IS NULL OR typeof(actor_display_name)='blob'), mon"
    "ey_original_decimal BLOB, money_currency BLOB CHECK((money_original_decimal IS NULL AND mo"
    "ney_currency IS NULL) OR (typeof(money_original_decimal)='blob' AND typeof(money_currency)"
    "='blob')), payment_namespace BLOB, payment_opaque_id BLOB CHECK((payment_namespace IS NULL"
    " AND payment_opaque_id IS NULL) OR (typeof(payment_namespace)='blob' AND typeof(payment_op"
    "aque_id)='blob')), activity_namespace BLOB, activity_opaque_id BLOB CHECK((activity_namesp"
    "ace IS NULL AND activity_opaque_id IS NULL) OR (typeof(activity_namespace)='blob' AND type"
    "of(activity_opaque_id)='blob')), server_timestamp_utc_ms INTEGER NOT NULL CHECK(typeof(ser"
    "ver_timestamp_utc_ms)='integer'), connection_ref BLOB NOT NULL CHECK(typeof(connection_ref"
    ")='blob'), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND leng"
    "th(configuration_ref)>0) REFERENCES subathon_retained_configurations(reference), modifier_"
    "context_ref BLOB NOT NULL CHECK(typeof(modifier_context_ref)='blob' AND length(modifier_co"
    "ntext_ref)>0) REFERENCES subathon_retained_contexts(reference), UNIQUE(provider,source_acc"
    "ount,identifier_namespace,opaque_id))",
    "CREATE TABLE subathon_receipt_attempts(receipt_id BLOB NOT NULL CHECK(typeof(receipt_id)='"
    "blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000') REFERENCES subathon_re"
    "ceipts(receipt_id), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0"
    "), event_type BLOB NOT NULL CHECK(typeof(event_type)='blob'), schema_version INTEGER NOT N"
    "ULL CHECK(typeof(schema_version)='integer' AND schema_version BETWEEN 0 AND 4294967295), s"
    "ource_timestamp_utc_ms INTEGER CHECK(source_timestamp_utc_ms IS NULL OR typeof(source_time"
    "stamp_utc_ms)='integer'), actor_id BLOB CHECK(actor_id IS NULL OR typeof(actor_id)='blob')"
    ", actor_display_name BLOB CHECK(actor_display_name IS NULL OR typeof(actor_display_name)='"
    "blob'), money_original_decimal BLOB, money_currency BLOB CHECK((money_original_decimal IS "
    "NULL AND money_currency IS NULL) OR (typeof(money_original_decimal)='blob' AND typeof(mone"
    "y_currency)='blob')), payment_namespace BLOB, payment_opaque_id BLOB CHECK((payment_namesp"
    "ace IS NULL AND payment_opaque_id IS NULL) OR (typeof(payment_namespace)='blob' AND typeof"
    "(payment_opaque_id)='blob')), activity_namespace BLOB, activity_opaque_id BLOB CHECK((acti"
    "vity_namespace IS NULL AND activity_opaque_id IS NULL) OR (typeof(activity_namespace)='blo"
    "b' AND typeof(activity_opaque_id)='blob')), server_timestamp_utc_ms INTEGER NOT NULL CHECK"
    "(typeof(server_timestamp_utc_ms)='integer'), connection_ref BLOB NOT NULL CHECK(typeof(con"
    "nection_ref)='blob'), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blo"
    "b' AND length(configuration_ref)>0) REFERENCES subathon_retained_configurations(reference)"
    ", modifier_context_ref BLOB NOT NULL CHECK(typeof(modifier_context_ref)='blob' AND length("
    "modifier_context_ref)>0) REFERENCES subathon_retained_contexts(reference), disposition INT"
    "EGER NOT NULL CHECK(typeof(disposition)='integer' AND disposition IN (0,1,2)), PRIMARY KEY"
    "(receipt_id,ordinal))",
    "INSERT INTO subathon_receipt_allocator(singleton,last_allocated) VALUES (1,NULL)",
};
const QStringList identity_columns{"provider", "source_account", "identifier_namespace",
                                   "opaque_id"};
const QStringList facts_columns{
    "event_type",         "schema_version",         "source_timestamp_utc_ms", "actor_id",
    "actor_display_name", "money_original_decimal", "money_currency",          "payment_namespace",
    "payment_opaque_id",  "activity_namespace",     "activity_opaque_id"};
const QStringList arrival_columns{"server_timestamp_utc_ms", "connection_ref", "configuration_ref",
                                  "modifier_context_ref"};
QStringList original_columns() {
    QStringList columns{"receipt_id"};
    columns.append(identity_columns);
    columns.append(facts_columns);
    columns.append(arrival_columns);
    return columns;
}
QStringList attempt_columns() {
    QStringList columns{"receipt_id", "ordinal"};
    columns.append(facts_columns);
    columns.append(arrival_columns);
    columns.append("disposition");
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
[[noreturn]] void invalid(SqliteReceiptInputError error) { throw SqliteReceiptInputFailure(error); }
void validate_options(SqliteReceiptLimits limits) {
    if (limits.reference_bytes <= 0 || limits.value_bytes <= 0 || limits.attempts_per_find <= 0 ||
        limits.attempts_per_find == std::numeric_limits<qsizetype>::max() ||
        std::uintmax_t(limits.attempts_per_find) >=
            std::uintmax_t(std::numeric_limits<qint64>::max()) ||
        std::uintmax_t(limits.attempts_per_find) >= std::vector<core::ReceiptAttempt>{}.max_size())
        invalid(SqliteReceiptInputError::invalid_options);
}
bool bounded(const std::string &value, qsizetype limit, bool required = false) {
    return (!required || !value.empty()) && value.size() <= size_t(limit);
}
void validate_identity(const core::ReceiptIdentity &identity, SqliteReceiptLimits limits) {
    if (!bounded(identity.origin.provider, limits.reference_bytes, true) ||
        !bounded(identity.origin.source_account, limits.reference_bytes, true) ||
        !bounded(identity.identifier_namespace, limits.reference_bytes, true) ||
        !bounded(identity.opaque_id, limits.reference_bytes, true))
        invalid(SqliteReceiptInputError::invalid_identity);
}
void validate_facts(const core::ReceiptFacts &facts, SqliteReceiptLimits limits) {
    const auto optional = [&](const std::optional<std::string> &value) {
        return !value || bounded(*value, limits.value_bytes);
    };
    const auto link = [&](const std::optional<core::ReceiptLinkReference> &value) {
        return !value || (bounded(value->identifier_namespace, limits.reference_bytes) &&
                          bounded(value->opaque_id, limits.reference_bytes));
    };
    if (!bounded(facts.event_type, limits.value_bytes) || !optional(facts.actor_id) ||
        !optional(facts.actor_display_name) ||
        (facts.money && (!bounded(facts.money->original_decimal, limits.value_bytes) ||
                         !bounded(facts.money->currency, limits.value_bytes))) ||
        !link(facts.payment_reference) || !link(facts.activity_reference))
        invalid(SqliteReceiptInputError::invalid_facts);
}
void validate_arrival(const core::ReceiptArrival &arrival, SqliteReceiptLimits limits) {
    if (!bounded(arrival.connection_ref, limits.reference_bytes) ||
        !bounded(arrival.configuration_ref, limits.reference_bytes, true) ||
        !bounded(arrival.modifier_context_ref, limits.reference_bytes, true))
        invalid(SqliteReceiptInputError::invalid_arrival);
}
QByteArray bytes(const std::string &value) {
    return value.empty() ? QByteArray("", 0) : QByteArray(value.data(), qsizetype(value.size()));
}
QList<QVariant> identity_values(const core::ReceiptIdentity &identity) {
    return {bytes(identity.origin.provider), bytes(identity.origin.source_account),
            bytes(identity.identifier_namespace), bytes(identity.opaque_id)};
}
QList<QVariant> facts_values(const core::ReceiptFacts &facts) {
    const auto optional = [](const std::optional<std::string> &value) {
        return value ? QVariant(bytes(*value)) : QVariant{};
    };
    return {
        bytes(facts.event_type),
        qint64(facts.schema_version),
        facts.source_timestamp_utc_ms ? QVariant(qint64(*facts.source_timestamp_utc_ms))
                                      : QVariant{},
        optional(facts.actor_id),
        optional(facts.actor_display_name),
        facts.money ? QVariant(bytes(facts.money->original_decimal)) : QVariant{},
        facts.money ? QVariant(bytes(facts.money->currency)) : QVariant{},
        facts.payment_reference ? QVariant(bytes(facts.payment_reference->identifier_namespace))
                                : QVariant{},
        facts.payment_reference ? QVariant(bytes(facts.payment_reference->opaque_id)) : QVariant{},
        facts.activity_reference ? QVariant(bytes(facts.activity_reference->identifier_namespace))
                                 : QVariant{},
        facts.activity_reference ? QVariant(bytes(facts.activity_reference->opaque_id))
                                 : QVariant{}};
}
QList<QVariant> arrival_values(const core::ReceiptArrival &arrival) {
    return {qint64(arrival.server_timestamp_utc_ms), bytes(arrival.connection_ref),
            bytes(arrival.configuration_ref), bytes(arrival.modifier_context_ref)};
}
void check_owner(QThread *owner) {
    if (QThread::currentThread() != owner)
        throw SqliteWorkFailure(WorkError::wrong_thread);
}
[[noreturn]] void map_work_failure(const SqliteWorkFailure &failure) {
    switch (failure.error()) {
    case WorkError::busy:
        throw SqliteReceiptStorageFailure(SqliteReceiptStorageReason::busy);
    case WorkError::sql_failure:
        throw SqliteReceiptStorageFailure(SqliteReceiptStorageReason::sql_failure);
    case WorkError::resource_limit:
        throw SqliteReceiptStorageFailure(SqliteReceiptStorageReason::resource_limit);
    case WorkError::result_limit:
        throw SqliteReceiptStorageFailure(SqliteReceiptStorageReason::result_limit);
    default:
        throw; // Actual capability/protocol/unknown categories are never manufactured or converted.
    }
}

class SqliteReceiptTransaction final : public ReceiptTransaction {
  public:
    SqliteReceiptTransaction(SqliteReceiptLimits limits, QThread *owner)
        : limits_(limits), owner_(owner) {}
    ~SqliteReceiptTransaction() override {
        if (owns_session_)
            (void)session_.cleanup();
    }
    void adopt(SqliteWorkSession session) noexcept {
        session_ = std::move(session);
        owns_session_ = true;
    }
    std::optional<core::ReceiptRecord> find(const core::ReceiptIdentity &identity) override {
        validate_identity(identity, limits_);
        check_owner(owner_);
        auto record =
            original("provider=? AND source_account=? AND identifier_namespace=? AND opaque_id=?",
                     identity_values(identity));
        if (!record)
            return std::nullopt;
        if (record->identity != identity)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        const auto rows = execute(
            {"SELECT " + projection(attempt_columns()) +
                 " FROM main.subathon_receipt_attempts WHERE receipt_id=? ORDER BY ordinal LIMIT ?",
             {encode_receipt_uint64(record->id.value), qint64(limits_.attempts_per_find + 1)}});
        if (rows.size() > limits_.attempts_per_find)
            fail(SqliteReceiptStorageReason::result_limit);
        if (rows.isEmpty() && !pending(record->id.value))
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        record->attempts.reserve(size_t(rows.size()));
        qint64 ordinal = 0;
        for (const auto &row : rows) {
            shape(row, 18);
            if (id(row, 0) != record->id.value || integer(row, 1) != ++ordinal)
                fail(SqliteReceiptStorageReason::invalid_stored_data);
            auto facts = read_facts(row, 2);
            auto arrival = read_arrival(row, 13);
            const auto value = integer(row, 17);
            core::ReceiptDisposition disposition;
            switch (value) {
            case 0:
                disposition = core::ReceiptDisposition::created;
                break;
            case 1:
                disposition = core::ReceiptDisposition::retry;
                break;
            case 2:
                disposition = core::ReceiptDisposition::conflict;
                break;
            default:
                fail(SqliteReceiptStorageReason::invalid_stored_data);
            }
            record->attempts.push_back({std::move(arrival), std::move(facts), disposition});
        }
        return record;
    }
    core::ReceiptId create(const core::ReceiptIdentity &identity, const core::ReceiptFacts &facts,
                           const core::ReceiptArrival &arrival) override {
        validate_identity(identity, limits_);
        validate_facts(facts, limits_);
        validate_arrival(arrival, limits_);
        check_owner(owner_);
        resolve(arrival);
        const auto candidate = next_id();
        if (pending_.size() == pending_.max_size())
            throw std::length_error("Too many pending receipt originals");
        pending_.reserve(pending_.size() + 1);
        const auto encoded = encode_receipt_uint64(candidate);
        QList<QVariant> values{encoded};
        values.append(identity_values(identity));
        values.append(facts_values(facts));
        values.append(arrival_values(arrival));
        const auto insert = insertion("subathon_receipts", original_columns(), std::move(values));
        const SqlStatement update{
            "UPDATE main.subathon_receipt_allocator SET last_allocated=? WHERE singleton=1",
            {encoded}};
        // Reserve tracking and prepare BOTH statements before the first write.
        // After SQL, uint64 push into reserved storage cannot allocate. The guard
        // also covers exceptions crossing actual execute calls after a first write.
        mutate([&] {
            (void)execute(insert);
            (void)execute(update);
            pending_.push_back(candidate);
        });
        return {candidate};
    }
    void append_attempt(core::ReceiptId receipt, const core::ReceiptAttempt &attempt) override {
        if (receipt.value == 0)
            invalid(SqliteReceiptInputError::invalid_receipt_id);
        validate_facts(attempt.observed_facts, limits_);
        validate_arrival(attempt.arrival, limits_);
        int disposition;
        switch (attempt.disposition) {
        case core::ReceiptDisposition::created:
            disposition = 0;
            break;
        case core::ReceiptDisposition::retry:
            disposition = 1;
            break;
        case core::ReceiptDisposition::conflict:
            disposition = 2;
            break;
        default:
            invalid(SqliteReceiptInputError::invalid_disposition);
        }
        check_owner(owner_);
        const auto encoded = encode_receipt_uint64(receipt.value);
        const auto stored = original("receipt_id=?", {encoded});
        if (!stored)
            fail(SqliteReceiptStorageReason::missing_reference);
        if (stored->id != receipt)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        const auto ordinal = next_ordinal(receipt.value, encoded);
        resolve(attempt.arrival);
        QList<QVariant> values{encoded, ordinal};
        values.append(facts_values(attempt.observed_facts));
        values.append(arrival_values(attempt.arrival));
        values.append(disposition);
        const auto insert =
            insertion("subathon_receipt_attempts", attempt_columns(), std::move(values));
        mutate([&] {
            (void)execute(insert);
            std::erase(pending_, receipt.value); // Trivial erase cannot allocate.
        });
    }
    void commit() override {
        check_owner(owner_);
        if (!pending_.empty()) {
            // A retained wrapper may outlive its work. Use actual rollback here,
            // so stale/terminal/unknown capabilities cannot become a false known
            // incomplete-receipt claim through cleanup's inactive no-op.
            session_.rollback();
            pending_.clear();
            throw SqliteReceiptStorageFailure(SqliteReceiptStorageReason::incomplete_receipt);
        }
        try {
            session_.commit();
        } catch (const SqliteWorkFailure &failure) {
            map_work_failure(failure);
        }
    }

  private:
    SqliteWorkSession session_;
    const SqliteReceiptLimits limits_;
    QThread *const owner_;
    bool owns_session_{false};
    std::vector<std::uint64_t> pending_;

    void cleanup() {
        const auto error = session_.cleanup();
        if (error != WorkError::none)
            throw SqliteWorkFailure(error);
        pending_.clear();
    }
    [[noreturn]] void fail(SqliteReceiptStorageReason reason) {
        cleanup();
        throw SqliteReceiptStorageFailure(reason);
    }
    template <class Operation> void mutate(Operation &&operation) {
        try {
            operation();
        } catch (const SqliteWorkFailure &) {
            throw;
        } catch (const SqliteReceiptStorageFailure &) {
            throw;
        } catch (...) {
            cleanup(); // A caught allocation failure must not leave an untracked write.
            throw;
        }
    }
    SqlRows execute(const SqlStatement &statement) {
        try {
            return session_.execute(statement);
        } catch (const SqliteWorkFailure &failure) {
            // These four known failures already established rollback in the writer;
            // no second control operation is needed to preserve its sticky outcome.
            switch (failure.error()) {
            case WorkError::busy:
            case WorkError::sql_failure:
            case WorkError::resource_limit:
            case WorkError::result_limit:
                pending_.clear();
                break;
            default:
                break;
            }
            map_work_failure(failure);
        }
    }
    bool pending(std::uint64_t value) const {
        return std::find(pending_.begin(), pending_.end(), value) != pending_.end();
    }
    void shape(const SqlRow &row, qsizetype fields) {
        if (row.size() != fields * 3)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
    }
    bool type(const SqlRow &row, qsizetype index, const char *expected) const {
        return row[index * 3 + 1].metaType().id() == QMetaType::QString &&
               row[index * 3 + 1].toString() == expected;
    }
    bool null(const SqlRow &row, qsizetype index) {
        if (!type(row, index, "null"))
            return false;
        if (!row[index * 3].isNull() || !row[index * 3 + 2].isNull())
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        return true;
    }
    qint64 integer(const SqlRow &row, qsizetype index) {
        const auto &value = row[index * 3];
        if (!type(row, index, "integer") || (value.metaType().id() != QMetaType::Int &&
                                             value.metaType().id() != QMetaType::LongLong))
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        return value.toLongLong();
    }
    std::string string(const SqlRow &row, qsizetype index, qsizetype bound, bool required = false) {
        const auto &value = row[index * 3];
        const auto &length = row[index * 3 + 2];
        if (!type(row, index, "blob") ||
            (length.metaType().id() != QMetaType::Int &&
             length.metaType().id() != QMetaType::LongLong) ||
            length.toLongLong() < 0)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        const auto size = length.toLongLong();
        if (!(size == 0 && value.isNull()) &&
            (value.metaType().id() != QMetaType::QByteArray || value.toByteArray().size() != size))
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        if (required && size == 0)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        if (size > bound)
            fail(SqliteReceiptStorageReason::result_limit);
        const auto data = value.toByteArray();
        return size == 0 ? std::string{} : std::string(data.constData(), size_t(data.size()));
    }
    std::uint64_t id(const SqlRow &row, qsizetype index) {
        // Wrong ID length is corruption, not a configured value bound.
        if (!type(row, index, "blob") || row[index * 3 + 2].toLongLong() != 8)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        const auto raw = string(row, index, 8, true);
        const auto decoded = decode_receipt_uint64(bytes(raw));
        if (decoded == 0)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        return decoded;
    }
    std::optional<std::string> optional_string(const SqlRow &row, qsizetype index,
                                               qsizetype bound) {
        return null(row, index) ? std::nullopt
                                : std::optional<std::string>(string(row, index, bound));
    }
    std::optional<std::pair<std::string, std::string>> group(const SqlRow &row, qsizetype index,
                                                             qsizetype bound) {
        const bool first = null(row, index), second = null(row, index + 1);
        if (first != second)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        if (first)
            return std::nullopt;
        return std::pair{string(row, index, bound), string(row, index + 1, bound)};
    }
    core::ReceiptFacts read_facts(const SqlRow &row, qsizetype offset) {
        core::ReceiptFacts facts;
        facts.event_type = string(row, offset, limits_.value_bytes);
        const auto schema = integer(row, offset + 1);
        if (schema < 0 || std::uint64_t(schema) > std::numeric_limits<std::uint32_t>::max())
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        facts.schema_version = std::uint32_t(schema);
        if (!null(row, offset + 2))
            facts.source_timestamp_utc_ms = integer(row, offset + 2);
        facts.actor_id = optional_string(row, offset + 3, limits_.value_bytes);
        facts.actor_display_name = optional_string(row, offset + 4, limits_.value_bytes);
        if (auto money = group(row, offset + 5, limits_.value_bytes))
            facts.money = core::ReceiptMoney{std::move(money->first), std::move(money->second)};
        if (auto payment = group(row, offset + 7, limits_.reference_bytes))
            facts.payment_reference =
                core::ReceiptLinkReference{std::move(payment->first), std::move(payment->second)};
        if (auto activity = group(row, offset + 9, limits_.reference_bytes))
            facts.activity_reference =
                core::ReceiptLinkReference{std::move(activity->first), std::move(activity->second)};
        return facts;
    }
    core::ReceiptArrival read_arrival(const SqlRow &row, qsizetype offset) {
        core::ReceiptArrival arrival{integer(row, offset),
                                     string(row, offset + 1, limits_.reference_bytes),
                                     string(row, offset + 2, limits_.reference_bytes, true),
                                     string(row, offset + 3, limits_.reference_bytes, true)};
        resolve(arrival, true);
        return arrival;
    }
    void resolve(const core::ReceiptArrival &arrival, bool stored = false) {
        try {
            const SqliteRetainedContextStore retained(
                session_, {limits_.reference_bytes, limits_.value_bytes});
            if (!retained.find({RetainedKind::configuration, arrival.configuration_ref}) ||
                !retained.find({RetainedKind::arrival_context, arrival.modifier_context_ref}))
                fail(stored ? SqliteReceiptStorageReason::invalid_stored_data
                            : SqliteReceiptStorageReason::missing_reference);
        } catch (const RetentionFailure &failure) {
            if (failure.error() == RetentionError::invalid_stored_data)
                throw SqliteReceiptStorageFailure(SqliteReceiptStorageReason::invalid_stored_data);
            if (failure.error() == RetentionError::result_limit)
                throw SqliteReceiptStorageFailure(SqliteReceiptStorageReason::result_limit);
            throw;
        } catch (const SqliteWorkFailure &failure) {
            map_work_failure(failure);
        }
    }
    std::optional<core::ReceiptRecord> original(const QString &where, QList<QVariant> bindings) {
        const auto rows = execute({"SELECT " + projection(original_columns()) +
                                       " FROM main.subathon_receipts WHERE " + where,
                                   std::move(bindings)});
        if (rows.isEmpty())
            return std::nullopt;
        if (rows.size() != 1)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        const auto &row = rows.front();
        shape(row, 20);
        const core::ReceiptId receipt{id(row, 0)};
        core::ReceiptIdentity identity{{string(row, 1, limits_.reference_bytes, true),
                                        string(row, 2, limits_.reference_bytes, true)},
                                       string(row, 3, limits_.reference_bytes, true),
                                       string(row, 4, limits_.reference_bytes, true)};
        auto facts = read_facts(row, 5);
        auto arrival = read_arrival(row, 16);
        return core::ReceiptRecord{
            receipt, std::move(identity), std::move(facts), std::move(arrival), {}};
    }
    std::uint64_t next_id() {
        const auto rows = execute({"SELECT " + projection({"singleton", "last_allocated"}) +
                                       " FROM main.subathon_receipt_allocator",
                                   {}});
        if (rows.size() != 1)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        shape(rows.front(), 2);
        if (integer(rows.front(), 0) != 1)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        const bool absent = null(rows.front(), 1);
        const auto last = absent ? std::uint64_t(0) : id(rows.front(), 1);
        const auto greatest =
            execute({"SELECT " + projection({"receipt_id"}) +
                         " FROM main.subathon_receipts ORDER BY receipt_id DESC LIMIT 1",
                     {}});
        if (greatest.size() > 1 || absent != greatest.isEmpty())
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        if (!greatest.isEmpty()) {
            shape(greatest.front(), 1);
            if (id(greatest.front(), 0) != last)
                fail(SqliteReceiptStorageReason::invalid_stored_data);
        }
        if (last == std::numeric_limits<std::uint64_t>::max())
            fail(SqliteReceiptStorageReason::allocator_exhausted);
        return last + 1;
    }
    qint64 next_ordinal(std::uint64_t receipt, const QByteArray &encoded) {
        const QStringList columns{
            "COUNT(*)", "MIN(ordinal)", "MAX(ordinal)",
            "COUNT(CASE WHEN typeof(ordinal)<>'integer' OR ordinal<=0 THEN 1 END)"};
        const auto rows = execute({"SELECT " + projection(columns) +
                                       " FROM main.subathon_receipt_attempts WHERE receipt_id=?",
                                   {encoded}});
        if (rows.size() != 1)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        const auto &row = rows.front();
        shape(row, 4);
        const auto count = integer(row, 0);
        if (count < 0 || integer(row, 3) != 0)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        if (count == 0) {
            if (!null(row, 1) || !null(row, 2) || !pending(receipt))
                fail(SqliteReceiptStorageReason::invalid_stored_data);
            return 1;
        }
        const auto minimum = integer(row, 1), maximum = integer(row, 2);
        if (minimum != 1 || maximum != count)
            fail(SqliteReceiptStorageReason::invalid_stored_data);
        if (maximum == std::numeric_limits<qint64>::max())
            fail(SqliteReceiptStorageReason::attempt_order_exhausted);
        return maximum + 1;
    }
};
} // namespace

QByteArray encode_receipt_uint64(std::uint64_t value) {
    QByteArray encoded(8, '\0');
    for (unsigned i = 0; i < 8; ++i)
        encoded[qsizetype(i)] = char((value >> (8 * (7 - i))) & 0xff);
    return encoded;
}
std::uint64_t decode_receipt_uint64(const QByteArray &encoded) {
    if (encoded.isNull() || encoded.size() != 8)
        invalid(SqliteReceiptInputError::invalid_codec);
    std::uint64_t value = 0;
    for (const auto byte : encoded)
        value = (value << 8) | static_cast<unsigned char>(byte);
    return value;
}
MigrationManifest receipt_repository_manifest() {
    auto manifest = retained_context_manifest();
    MigrationStep step;
    step.version = 2;
    step.id = "identified-receipts-v1";
    for (const auto &sql : migration_sql)
        step.sql.append(sql);
    step.expected_schema = manifest.steps.front().expected_schema;
    step.expected_schema.append(
        {"table", "subathon_receipt_allocator", "subathon_receipt_allocator", migration_sql[0]});
    step.expected_schema.append(
        {"table", "subathon_receipts", "subathon_receipts", migration_sql[1]});
    step.expected_schema.append(
        {"table", "subathon_receipt_attempts", "subathon_receipt_attempts", migration_sql[2]});
    step.expected_schema.append({"index", "sqlite_autoindex_subathon_receipt_attempts_1",
                                 "subathon_receipt_attempts", std::nullopt});
    step.expected_schema.append(
        {"index", "sqlite_autoindex_subathon_receipts_1", "subathon_receipts", std::nullopt});
    step.expected_schema.append(
        {"index", "sqlite_autoindex_subathon_receipts_2", "subathon_receipts", std::nullopt});
    // These fixed approved object names are ASCII and unique by type/name.
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return a.type < b.type || (a.type == b.type && a.name < b.name);
              });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions = {0, 1, 2};
    return manifest;
}
SqliteReceiptRepository::SqliteReceiptRepository(SqliteWorkContext context,
                                                 SqliteReceiptLimits limits)
    : capability_(std::move(context)), construction_thread_(QThread::currentThread()),
      limits_(limits) {}
SqliteReceiptRepository::SqliteReceiptRepository(AdoptActiveReceiptSessionTag,
                                                 SqliteWorkSession session,
                                                 SqliteReceiptLimits limits)
    : capability_(std::move(session)), construction_thread_(QThread::currentThread()),
      limits_(limits) {}
std::unique_ptr<ReceiptTransaction> SqliteReceiptRepository::begin() {
    validate_options(limits_);
    check_owner(construction_thread_);
    if (begun_)
        invalid(SqliteReceiptInputError::already_begun);
    // Allocation cannot strand a begun session or take adoption authority before
    // the wrapper exists. Attachment and the unique_ptr conversion cannot allocate.
    auto transaction = std::make_unique<SqliteReceiptTransaction>(limits_, construction_thread_);
    if (const auto *context = std::get_if<SqliteWorkContext>(&capability_)) {
        begun_ = true;
        try {
            transaction->adopt(context->begin());
        } catch (const SqliteWorkFailure &failure) {
            map_work_failure(failure);
        }
    } else {
        transaction->adopt(std::get<SqliteWorkSession>(capability_));
        begun_ = true;
    }
    return transaction;
}
} // namespace subathon::server
