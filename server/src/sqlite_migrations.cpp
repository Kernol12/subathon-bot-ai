// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_migration_inspection.hpp"

#include <QCryptographicHash>
#include <QSet>
#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <utility>

namespace subathon::server {
namespace {
const QString header_ddl = "CREATE TABLE subathon_catalog(singleton INTEGER PRIMARY KEY, "
                           "catalog_format INTEGER NOT NULL, store_identity TEXT NOT NULL)";
const QString history_ddl = "CREATE TABLE subathon_migration_history(version INTEGER PRIMARY KEY, "
                            "migration_id TEXT NOT NULL UNIQUE, content_sha256 BLOB NOT NULL)";
const QString store_identity = "subathon.sqlite-store";
const QString post_empty =
    "SELECT CASE WHEN NOT EXISTS(SELECT 1 FROM main.subathon_clock_checkpoint) AND "
    "NOT EXISTS(SELECT 1 FROM main.subathon_clock_evidence) THEN 1 ELSE 0 END";

const QString legacy_guard =
    "WITH checked AS (SELECT CASE WHEN h.singleton IS NULL THEN CASE WHEN EXISTS(SELECT 1 FROM "
    "main.subathon_app_states) THEN 4 ELSE 0 END WHEN typeof(h.singleton)='integer' AND "
    "h.singleton=1 AND typeof(h.token)='blob' AND length(h.token) BETWEEN 1 AND 4096 AND "
    "typeof(s.format_ref)='blob' AND length(s.format_ref) BETWEEN 1 AND 4096 AND "
    "typeof(s.document)='blob' AND length(s.document)<=262144 AND "
    "typeof(s.configuration_ref)='blob' AND length(s.configuration_ref) BETWEEN 1 AND 4096 AND "
    "typeof(c.reference)='blob' AND typeof(c.document)='blob' AND typeof(s.lifecycle)='integer' "
    "AND "
    "s.lifecycle BETWEEN 0 AND 2 AND typeof(s.phase)='integer' AND s.phase IN (0,1) AND "
    "typeof(s.main_remaining_ms)='integer' AND s.main_remaining_ms>=0 AND "
    "typeof(s.grace_remaining_ms)='integer' AND s.grace_remaining_ms>=0 AND "
    "typeof(s.awaiting_confirmation)='integer' AND s.awaiting_confirmation IN (0,1) AND "
    "(s.phase<>1 "
    "OR (s.main_remaining_ms=0 AND s.awaiting_confirmation=0)) AND (s.awaiting_confirmation=0 OR "
    "(s.lifecycle=1 AND s.phase=0 AND s.main_remaining_ms=0)) AND typeof(s.time_debt_ms)='integer' "
    "AND s.time_debt_ms>=0 AND typeof(s.balance_count)='integer' AND s.balance_count>=0 THEN "
    "s.lifecycle+1 ELSE 4 END AS status,h.token,s.configuration_ref FROM (SELECT 1 AS anchor) LEFT "
    "JOIN main.subathon_app_head h ON 1=1 LEFT JOIN main.subathon_app_states s ON s.token=h.token "
    "LEFT JOIN main.subathon_retained_configurations c ON c.reference=s.configuration_ref) SELECT "
    "status,CASE WHEN status BETWEEN 1 AND 3 THEN token END,CASE WHEN status BETWEEN 1 AND 3 THEN "
    "configuration_ref END FROM checked LIMIT 2";

[[noreturn]] void invalid_manifest() { throw MigrationFailure(MigrationReason::invalid_manifest); }

void add_size(qsizetype &total, qsizetype amount) {
    if (amount < 0 || total > std::numeric_limits<qsizetype>::max() - amount)
        invalid_manifest();
    total += amount;
}

// Measure without allocating an encoded buffer. Reject repair by QString's UTF-8
// encoder, embedded NUL and unpaired UTF-16 surrogates. Genuine U+FFFD is valid.
qsizetype utf8_size(const QString &text) {
    qsizetype size = 0;
    for (qsizetype i = 0; i < text.size(); ++i) {
        const auto unit = text[i].unicode();
        if (unit == 0)
            invalid_manifest();
        if (QChar::isHighSurrogate(unit)) {
            if (++i == text.size() || !text[i].isLowSurrogate())
                invalid_manifest();
            add_size(size, 4);
        } else if (QChar::isLowSurrogate(unit)) {
            invalid_manifest();
        } else {
            add_size(size, unit < 0x80 ? 1 : unit < 0x800 ? 2 : 3);
        }
    }
    return size;
}

QByteArray strict_utf8(const QString &text) {
    const auto size = utf8_size(text);
    const auto bytes = text.toUtf8();
    // utf8_size already establishes valid Unicode scalars. A decoder round trip
    // would consume an initial BOM, which is content in these framed fields.
    if (bytes.size() != size)
        invalid_manifest();
    return bytes;
}

void add_frame_size(qsizetype &total, qsizetype bytes) {
    qsizetype digits = 1;
    for (auto length = bytes; length >= 10; length /= 10)
        ++digits;
    add_size(total, digits);
    add_size(total, 1);
    add_size(total, bytes);
}

struct EncodedObject {
    CatalogObject object;
    std::array<QByteArray, 5> fields;
};

bool byte_less(const QByteArray &left, const QByteArray &right) {
    return std::lexicographical_compare(
        left.begin(), left.end(), right.begin(), right.end(), [](char a, char b) {
            return static_cast<unsigned char>(a) < static_cast<unsigned char>(b);
        });
}

QList<EncodedObject> encoded_objects(const QList<CatalogObject> &objects) {
    QList<EncodedObject> result;
    result.reserve(objects.size());
    for (const auto &object : objects)
        result.append(
            {object,
             {strict_utf8(object.type), strict_utf8(object.name), strict_utf8(object.table_name),
              object.sql ? "1" : "0", object.sql ? strict_utf8(*object.sql) : QByteArray{}}});
    std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
        for (size_t i = 0; i < left.fields.size(); ++i) {
            if (left.fields[i] != right.fields[i])
                return byte_less(left.fields[i], right.fields[i]);
        }
        return false;
    });
    return result;
}

QList<CatalogObject> sorted_objects(const QList<CatalogObject> &objects) {
    QList<CatalogObject> result;
    for (const auto &entry : encoded_objects(objects))
        result.append(entry.object);
    return result;
}

QList<CatalogObject> reserved_objects() {
    return {{"table", "subathon_catalog", "subathon_catalog", header_ddl},
            {"table", "subathon_migration_history", "subathon_migration_history", history_ddl},
            {"index", "sqlite_autoindex_subathon_migration_history_1", "subathon_migration_history",
             std::nullopt}};
}

bool valid_id(const QString &id, qsizetype limit) {
    if (id.isEmpty() || id.size() > std::min<qsizetype>(128, limit))
        return false;
    for (const auto character : id)
        if (character.unicode() < 0x21 || character.unicode() > 0x7e)
            return false;
    return true;
}

// Preflight is pure and precedes completion/writer admission checks. Hashes and
// sorted expectations are derived from the immutable owned request, never storage.
QList<QByteArray> preflight(MigrationRequest &request) {
    const auto &limits = request.limits;
    auto &manifest = request.manifest;
    const auto count = manifest.steps.size();
    if ((request.mode != MigrationMode::inspect && request.mode != MigrationMode::advance) ||
        limits.versions <= 0 || limits.id_bytes <= 0 || limits.schema_objects <= 0 ||
        limits.manifest_utf8_bytes <= 0 || manifest.catalog_format != 1 ||
        manifest.store_identity != store_identity || count < 1 || count > limits.versions ||
        count > std::numeric_limits<qint32>::max())
        invalid_manifest();
    if (manifest.supported_starting_versions.isEmpty() ||
        manifest.supported_starting_versions.front() != 0 ||
        manifest.supported_starting_versions.back() != count)
        invalid_manifest();
    qint32 previous = -1;
    for (const auto version : manifest.supported_starting_versions) {
        if (version <= previous || version < 0 || version > count)
            invalid_manifest();
        previous = version;
    }
    QSet<QString> ids;
    QList<QByteArray> hashes;
    qsizetype payload_bytes = 0;
    const auto reserved = reserved_objects();
    for (qsizetype i = 0; i < count; ++i) {
        auto &step = manifest.steps[i];
        if (step.version != i + 1 || !valid_id(step.id, limits.id_bytes) || ids.contains(step.id) ||
            step.expected_schema.size() > limits.schema_objects)
            invalid_manifest();
        ids.insert(step.id);
        for (const auto &sql : step.sql)
            add_size(payload_bytes, utf8_size(sql));
        for (const auto &object : step.expected_schema) {
            add_size(payload_bytes, utf8_size(object.type));
            add_size(payload_bytes, utf8_size(object.name));
            add_size(payload_bytes, utf8_size(object.table_name));
            if (object.sql)
                add_size(payload_bytes, utf8_size(*object.sql));
        }
        if (payload_bytes > limits.manifest_utf8_bytes)
            invalid_manifest();
        const auto objects = encoded_objects(step.expected_schema);
        step.expected_schema.clear();
        for (const auto &entry : objects) {
            if (!step.expected_schema.isEmpty() && step.expected_schema.back() == entry.object)
                invalid_manifest();
            step.expected_schema.append(entry.object);
            for (const auto &required : reserved)
                if (entry.object.name == required.name && entry.object != required)
                    invalid_manifest();
        }
        for (const auto &required : reserved)
            if (!step.expected_schema.contains(required))
                invalid_manifest();
        hashes.append(migration_content_sha256(step));
    }
    return hashes;
}

struct GuardedAdvance {
    detail::ClockPreOpenWitness witness;
    const MigrationManifest compiled_manifest;
    detail::ClockAdvanceRefusal refusal{detail::ClockAdvanceRefusal::none};
    std::optional<detail::ClockProbeQuery> terminal{};
    detail::ClockStatementLedger ledger{};
};

struct RequestState {
    std::optional<GuardedAdvance> guard;
    MigrationReason reason{MigrationReason::none};
    std::optional<MigrationReport> report;
    void remember(MigrationReason value) noexcept {
        if (reason == MigrationReason::none)
            reason = value;
    }
};

MigrationReason work_reason(WorkError error, MigrationReason sql_reason) {
    switch (error) {
    case WorkError::none:
        return MigrationReason::none;
    case WorkError::busy:
        return MigrationReason::busy;
    case WorkError::resource_limit:
    case WorkError::result_limit:
        return MigrationReason::resource_limit;
    case WorkError::uncertain_outcome:
        return MigrationReason::uncertain_outcome;
    case WorkError::writer_unavailable:
        return MigrationReason::writer_unavailable;
    default:
        return sql_reason;
    }
}

bool text_value(const QVariant &value) {
    return value.metaType().id() == QMetaType::QString && !value.isNull();
}
bool integer_value(const QVariant &value) {
    return value.metaType().id() == QMetaType::Int || value.metaType().id() == QMetaType::LongLong;
}

void verify_catalog_schema(const QList<CatalogObject> &actual,
                           const QList<CatalogObject> &expected) {
    if (actual != expected)
        throw MigrationFailure(MigrationReason::schema_mismatch);
}

template <typename Execute> class CatalogInspection final {
    using Query = detail::ClockProbeQuery;

  public:
    CatalogInspection(const MigrationManifest &manifest, const QList<QByteArray> &hashes,
                      Execute execute)
        : manifest_(manifest), hashes_(hashes), execute_(std::move(execute)) {}

    QList<CatalogObject> read_schema() {
        const auto rows = execute_(
            Query::schema,
            "SELECT type,name,tbl_name,sql,typeof(type),typeof(name),typeof(tbl_name),typeof(sql),"
            "CAST(type AS BLOB),CAST(name AS BLOB),CAST(tbl_name AS BLOB),CAST(sql AS BLOB) "
            "FROM main.sqlite_schema");
        QList<CatalogObject> objects;
        for (const auto &row : rows) {
            if (row.size() != 12)
                throw MigrationFailure(MigrationReason::schema_mismatch);
            CatalogObject object;
            object.type = stored_text(row[0], row[4], row[8], MigrationReason::schema_mismatch);
            object.name = stored_text(row[1], row[5], row[9], MigrationReason::schema_mismatch);
            object.table_name =
                stored_text(row[2], row[6], row[10], MigrationReason::schema_mismatch);
            if (text_value(row[7]) && row[7].toString() == "null" && row[3].isNull() &&
                row[11].isNull())
                object.sql = std::nullopt;
            else
                object.sql = stored_text(row[3], row[7], row[11], MigrationReason::schema_mismatch);
            objects.append(std::move(object));
        }
        return sorted_objects(objects);
    }

    qint32 inspect_store() {
        // The encoding gate must precede every raw-text/schema/catalog observation.
        verify_encoding();
        const auto schema = read_schema();
        if (schema.isEmpty()) {
            verify_integrity();
            return 0;
        }
        bool header = false, history = false;
        for (const auto &object : schema) {
            header = header || object.name == "subathon_catalog";
            history = history || object.name == "subathon_migration_history";
        }
        if (!header && !history)
            throw MigrationFailure(MigrationReason::foreign_store);
        if (!header || !history)
            throw MigrationFailure(MigrationReason::catalog_mismatch);
        for (const auto &required : reserved_objects())
            if (!schema.contains(required))
                throw MigrationFailure(MigrationReason::schema_mismatch);
        const auto headers =
            execute_(Query::header,
                     "SELECT singleton,catalog_format,store_identity,typeof(singleton),"
                     "typeof(catalog_format),typeof(store_identity),CAST(store_identity AS BLOB) "
                     "FROM main.subathon_catalog");
        if (headers.size() != 1 || headers[0].size() != 7)
            throw MigrationFailure(MigrationReason::catalog_mismatch);
        const auto &row = headers[0];
        if (stored_integer(row[0], row[3]) != 1)
            throw MigrationFailure(MigrationReason::catalog_mismatch);
        if (stored_integer(row[1], row[4]) != 1)
            throw MigrationFailure(MigrationReason::unsupported_catalog_format);
        if (stored_text(row[2], row[5], row[6], MigrationReason::catalog_mismatch) !=
            store_identity)
            throw MigrationFailure(MigrationReason::foreign_store);
        const auto histories = execute_(
            Query::history,
            "SELECT version,migration_id,content_sha256,typeof(version),typeof(migration_id),"
            "typeof(content_sha256),CAST(migration_id AS BLOB) "
            "FROM main.subathon_migration_history ORDER BY version");
        if (histories.isEmpty())
            throw MigrationFailure(MigrationReason::catalog_mismatch);
        QSet<QString> ids;
        qint32 head = 0;
        for (const auto &entry : histories) {
            if (entry.size() != 7)
                throw MigrationFailure(MigrationReason::catalog_mismatch);
            const auto version = stored_integer(entry[0], entry[3]);
            const auto id =
                stored_text(entry[1], entry[4], entry[6], MigrationReason::catalog_mismatch);
            if (version < 1 || version > std::numeric_limits<qint32>::max() ||
                version != static_cast<qint64>(head) + 1 || !valid_id(id, 128) ||
                ids.contains(id) || !text_value(entry[5]) || entry[5].toString() != "blob" ||
                entry[2].metaType().id() != QMetaType::QByteArray ||
                entry[2].toByteArray().size() != 32)
                throw MigrationFailure(MigrationReason::catalog_mismatch);
            if (version > manifest_.steps.size())
                throw MigrationFailure(MigrationReason::unsupported_version);
            const auto &step = manifest_.steps[version - 1];
            if (id != step.id || entry[2].toByteArray() != hashes_[version - 1])
                throw MigrationFailure(MigrationReason::catalog_mismatch);
            ids.insert(id);
            head = static_cast<qint32>(version);
        }
        if (!manifest_.supported_starting_versions.contains(head))
            throw MigrationFailure(MigrationReason::unsupported_version);
        verify_catalog_schema(schema, manifest_.steps[head - 1].expected_schema);
        verify_integrity();
        return head;
    }

  private:
    const MigrationManifest &manifest_;
    const QList<QByteArray> &hashes_;
    Execute execute_;

    QString stored_text(const QVariant &value, const QVariant &type, const QVariant &raw,
                        MigrationReason reason) {
        if (!text_value(type) || type.toString() != "text" || !text_value(value) ||
            raw.metaType().id() != QMetaType::QByteArray)
            throw MigrationFailure(reason);
        try {
            if (strict_utf8(value.toString()) == raw.toByteArray())
                return value.toString();
        } catch (const MigrationFailure &) {
            // Invalid encoded database text is a catalog/schema refusal, not a
            // change to the already admitted manifest's validity.
        }
        throw MigrationFailure(reason);
    }
    qint64 stored_integer(const QVariant &value, const QVariant &type) {
        if (!text_value(type) || type.toString() != "integer" || !integer_value(value))
            throw MigrationFailure(MigrationReason::catalog_mismatch);
        return value.toLongLong();
    }
    void verify_encoding() {
        const auto rows = execute_(Query::encoding, "SELECT * FROM pragma_encoding()");
        if (rows.size() != 1 || rows[0].size() != 1 || !text_value(rows[0][0]) ||
            rows[0][0].toString() != "UTF-8")
            throw MigrationFailure(MigrationReason::unsupported_encoding);
    }
    void verify_integrity() {
        const auto rows =
            execute_(Query::integrity, "SELECT * FROM pragma_integrity_check() LIMIT 2");
        if (rows.size() != 1 || rows[0].size() != 1 || !text_value(rows[0][0]) ||
            rows[0][0].toString() != "ok")
            throw MigrationFailure(MigrationReason::integrity_failed);
        if (!execute_(Query::foreign_keys, "SELECT * FROM pragma_foreign_key_check() LIMIT 1")
                 .isEmpty())
            throw MigrationFailure(MigrationReason::integrity_failed);
    }
};

} // namespace

namespace detail {
// Only the migration executor can construct the key required by the writer bridge.
class CompiledMigrationSqlKey final {
    friend class MigrationExecution;

    CompiledMigrationSqlKey() = default;
    CompiledMigrationSqlKey(const CompiledMigrationSqlKey &) = delete;
};

SqlRows execute_compiled_migration_sql(const CompiledMigrationSqlKey &,
                                       const std::weak_ptr<SqliteWorkState> &ticket,
                                       const QString &sql, const QString &compiled);

// This executor owns no connection. Every observation and change is charged to
// the existing session, including raw/type verification and repeated final checks.
class MigrationExecution final {
    auto catalog_inspection() {
        return CatalogInspection(request_.manifest, hashes_,
                                 [this](detail::ClockProbeQuery, const QString &sql) {
                                     return execute(sql, {}, MigrationReason::inspection_failed);
                                 });
    }

  public:
    MigrationExecution(const MigrationRequest &request, const QList<QByteArray> &hashes,
                       RequestState &state)
        : request_(request), hashes_(hashes), state_(state) {}

    QVariant run(SqliteWorkContext context) {
        try {
            probe(MigrationCheckpoint::before_begin, 0, 0);
            session_ = context.begin();
            const auto old_version = state_.guard ? inspect_guarded_predecessor(*state_.guard)
                                                  : catalog_inspection().inspect_store();
            qint32 final_version = old_version;
            qsizetype applied = 0;
            if (request_.mode == MigrationMode::advance) {
                const auto *compiled = state_.guard ? &state_.guard->compiled_manifest : nullptr;
                if (old_version == 0)
                    bootstrap();
                for (qsizetype i = old_version; i < request_.manifest.steps.size(); ++i) {
                    const auto &step = request_.manifest.steps[i];
                    for (qsizetype statement = 0; statement < step.sql.size(); ++statement) {
                        probe(MigrationCheckpoint::before_statement, step.version, statement + 1);
                        const auto &sql = step.sql[statement];
                        execute_migration_sql(sql,
                                              compiled ? compiled->steps[i].sql[statement] : sql);
                    }
                    probe(MigrationCheckpoint::before_history, step.version, step.sql.size());
                    verify_catalog_schema(catalog_inspection().read_schema(), step.expected_schema);
                    (void)execute("INSERT INTO main.subathon_migration_history"
                                  "(version,migration_id,content_sha256) VALUES (?,?,?)",
                                  {step.version, step.id, hashes_[i]},
                                  MigrationReason::sql_failure);
                    ++applied;
                }
                final_version = static_cast<qint32>(request_.manifest.steps.size());
            }
            if (catalog_inspection().inspect_store() != final_version)
                throw MigrationFailure(MigrationReason::catalog_mismatch);
            if (state_.guard && applied != 0)
                verify_empty_checkpoint();
            probe(MigrationCheckpoint::before_commit, final_version, 0);
            session_.commit();
            const auto target = static_cast<qint32>(request_.manifest.steps.size());
            MigrationReport report{final_version == 0       ? CatalogState::fresh
                                   : final_version < target ? CatalogState::upgrade_required
                                                            : CatalogState::current,
                                   final_version, target, applied};
            state_.report = report;
            return QVariantList{static_cast<int>(report.state), report.version,
                                report.target_version, static_cast<qlonglong>(report.applied)};
        } catch (const MigrationFailure &error) {
            state_.remember(error.reason());
            (void)session_.cleanup();
            throw;
        } catch (const SqliteWorkFailure &error) {
            state_.remember(work_reason(error.error(), MigrationReason::sql_failure));
            (void)session_.cleanup();
            throw;
        } catch (...) {
            (void)session_.cleanup();
            throw;
        }
    }

  private:
    const MigrationRequest &request_;
    const QList<QByteArray> &hashes_;
    RequestState &state_;
    SqliteWorkSession session_;

    qint32 inspect_guarded_predecessor(GuardedAdvance &guard) {
        const ClockInspectionQuery inspect = [this, &guard](ClockProbeQuery kind, const QString &sql) {
            guard.terminal = kind;
            try {
                return session_.execute_clock_preflight({sql, {}}, guard.ledger);
            } catch (const SqliteWorkFailure &error) {
                state_.remember(work_reason(error.error(), MigrationReason::inspection_failed));
                throw;
            }
        };
        const auto version =
            CatalogInspection(request_.manifest, hashes_, std::cref(inspect)).inspect_store();

        // A verified successor is already beyond the admitted predecessor range.
        // It does not use the legacy projection.
        if (version > clock_predecessor_version) {
            guard.refusal = ClockAdvanceRefusal::changed_basis;
        } else {
            const auto current = read_clock_predecessor(version, inspect);
            if (current.predecessor == ClockPredecessor::active)
                guard.refusal = ClockAdvanceRefusal::active;
            else if (current != guard.witness)
                guard.refusal = ClockAdvanceRefusal::changed_basis;
        }
        if (guard.refusal != ClockAdvanceRefusal::none)
            throw MigrationFailure(MigrationReason::catalog_mismatch);
        return version;
    }

    void probe(MigrationCheckpoint checkpoint, qint32 version, qsizetype statement) {
        if (!request_.cancel)
            return;
        bool cancelled;
        try {
            cancelled = request_.cancel({checkpoint, version, statement});
        } catch (...) {
            throw MigrationFailure(MigrationReason::callback_failure);
        }
        if (cancelled)
            throw MigrationFailure(MigrationReason::cancelled);
    }
    SqlRows execute(const QString &sql, const QList<QVariant> &bindings,
                    MigrationReason sql_reason) {
        try {
            return session_.execute({sql, bindings});
        } catch (const SqliteWorkFailure &error) {
            state_.remember(work_reason(error.error(), sql_reason));
            throw;
        }
    }
    void execute_migration_sql(const QString &sql, const QString &compiled) {
        if (state_.guard)
            (void)execute_compiled_migration_sql(CompiledMigrationSqlKey{}, session_.state_, sql,
                                                 compiled);
        else
            (void)execute(sql, {}, MigrationReason::sql_failure);
    }
    void verify_empty_checkpoint() {
        const auto rows = execute(post_empty, {}, MigrationReason::inspection_failed);
        if (rows.size() != 1 || rows.front().size() != 1)
            throw MigrationFailure(MigrationReason::catalog_mismatch);
        const auto &empty = rows.front().front();
        if (empty.isNull() || !integer_value(empty) || empty.toLongLong() != 1)
            throw MigrationFailure(MigrationReason::catalog_mismatch);
    }
    void bootstrap() {
        execute_migration_sql(header_ddl, header_ddl);
        execute_migration_sql(history_ddl, history_ddl);
        (void)execute("INSERT INTO main.subathon_catalog"
                      "(singleton,catalog_format,store_identity) VALUES (?,?,?)",
                      {1, 1, store_identity}, MigrationReason::sql_failure);
    }
};
} // namespace detail

detail::ClockPreOpenWitness detail::read_clock_predecessor(qint32 version,
                                                           const ClockInspectionQuery &execute) {
    // The verified prefixes before version 5 have no legacy application head.
    if (version < 5)
        return {version, ClockPredecessor::blank, {}, {}};
    if (version > clock_predecessor_version)
        throw MigrationFailure(MigrationReason::unsupported_version);

    const auto rows = execute(ClockProbeQuery::legacy_guard, legacy_guard);
    if (rows.size() != 1 || rows.front().size() != 3)
        throw MigrationFailure(MigrationReason::catalog_mismatch);
    const auto &row = rows.front();
    const auto type = row[0].metaType().id();
    if (row[0].isNull() || (type != QMetaType::Int && type != QMetaType::LongLong))
        throw MigrationFailure(MigrationReason::catalog_mismatch);
    const auto status = row[0].toLongLong();
    if (status == 0 && row[1].isNull() && row[2].isNull())
        return {version, ClockPredecessor::no_head, {}, {}};
    constexpr ClockPredecessor lifecycle[]{ClockPredecessor::ready, ClockPredecessor::active,
                                           ClockPredecessor::finished};
    if (status < 1 || status > 3 || row[1].metaType().id() != QMetaType::QByteArray ||
        row[2].metaType().id() != QMetaType::QByteArray)
        throw MigrationFailure(MigrationReason::catalog_mismatch);
    auto token = row[1].toByteArray();
    auto configuration = row[2].toByteArray();
    if (token.isEmpty() || token.size() > 4096 || configuration.isEmpty() ||
        configuration.size() > 4096)
        throw MigrationFailure(MigrationReason::catalog_mismatch);
    return {version, lifecycle[status - 1], std::move(token), std::move(configuration)};
}

qint32 detail::inspect_clock_predecessor_catalog(MigrationManifest manifest,
                                                 const ClockInspectionQuery &execute) {
    if (manifest.steps.isEmpty())
        invalid_manifest();
    const auto schema_objects = manifest.steps.back().expected_schema.size();
    MigrationRequest request{MigrationMode::inspect, std::move(manifest),
                             {.schema_objects = schema_objects}, {}};
    const auto hashes = preflight(request);
    return CatalogInspection(request.manifest, hashes, std::cref(execute)).inspect_store();
}

QByteArray canonical_migration_payload(const MigrationStep &step) {
    if (step.version < 1)
        invalid_manifest();
    // Validate all text and calculate the complete frame before allocating its
    // UTF-8 fields, sorting keys or payload. No length multiplication can wrap.
    qsizetype size = 0;
    add_frame_size(size, sizeof("migration-catalog-v1") - 1);
    add_frame_size(size, QByteArray::number(step.version).size());
    add_frame_size(size, utf8_size(step.id));
    add_frame_size(size, QByteArray::number(step.sql.size()).size());
    for (const auto &sql : step.sql)
        add_frame_size(size, utf8_size(sql));
    add_frame_size(size, QByteArray::number(step.expected_schema.size()).size());
    for (const auto &object : step.expected_schema) {
        add_frame_size(size, utf8_size(object.type));
        add_frame_size(size, utf8_size(object.name));
        add_frame_size(size, utf8_size(object.table_name));
        add_frame_size(size, 1);
        if (object.sql)
            add_frame_size(size, utf8_size(*object.sql));
    }
    QList<QByteArray> fields{"migration-catalog-v1", QByteArray::number(step.version),
                             strict_utf8(step.id), QByteArray::number(step.sql.size())};
    for (const auto &sql : step.sql)
        fields.append(strict_utf8(sql));
    fields.append(QByteArray::number(step.expected_schema.size()));
    for (const auto &entry : encoded_objects(step.expected_schema)) {
        for (size_t i = 0; i < 4; ++i)
            fields.append(entry.fields[i]);
        if (entry.object.sql)
            fields.append(entry.fields[4]);
    }
    QByteArray payload;
    payload.reserve(size);
    for (const auto &field : fields) {
        payload.append(QByteArray::number(field.size()));
        payload.append(':');
        payload.append(field);
    }
    return payload;
}
QByteArray migration_content_sha256(const MigrationStep &step) {
    return QCryptographicHash::hash(canonical_migration_payload(step), QCryptographicHash::Sha256);
}

namespace {
MigrationAdmission enqueue_migration(SqliteWriter &writer, MigrationRequest request,
                                     SqliteMigrationRunner::Completion completion,
                                     std::shared_ptr<RequestState> state = {}) {
    QList<QByteArray> hashes;
    try {
        hashes = preflight(request);
    } catch (const MigrationFailure &) {
        return {StorageError::invalid_options, MigrationReason::invalid_manifest};
    }
    if (!completion)
        return {StorageError::invalid_work, MigrationReason::none};
    if (!state)
        state = std::make_shared<RequestState>();
    auto error = writer.enqueue_work(
        [request = std::move(request), hashes = std::move(hashes),
         state](SqliteWorkContext context) {
            return detail::MigrationExecution(request, hashes, *state).run(context);
        },
        [state, completion = std::move(completion)](SqliteWorkResult work) {
            auto reason = state->reason;
            if (work.transaction == TransactionOutcome::outcome_unknown ||
                work.error == WorkError::uncertain_outcome)
                reason = MigrationReason::uncertain_outcome;
            else if (reason == MigrationReason::none && work.error != WorkError::none)
                reason = work_reason(work.error, MigrationReason::sql_failure);
            std::optional<MigrationReport> report;
            if (reason == MigrationReason::none &&
                work.transaction == TransactionOutcome::committed &&
                work.error == WorkError::none && work.value)
                report = state->report;
            completion({std::move(work), reason, std::move(report)});
        });
    return {error, MigrationReason::none};
}
} // namespace

MigrationAdmission SqliteMigrationRunner::enqueue(SqliteWriter &writer, MigrationRequest request,
                                                  Completion completion) {
    return enqueue_migration(writer, std::move(request), std::move(completion));
}

MigrationAdmission detail::ClockAdvanceExecution::enqueue(SqliteWriter &writer,
                                                          MigrationRequest request,
                                                          ClockPreOpenWitness witness,
                                                          ClockAdvanceCompletion completion) {
    if (!completion)
        return enqueue_migration(writer, std::move(request), {});
    auto state = std::make_shared<RequestState>();
    // The binding has replaced the supplied manifest with the exact compiled
    // values. Keep these for per-ordinal comparison after runner preflight.
    state->guard.emplace(GuardedAdvance{std::move(witness), request.manifest});
    auto finish = [state, completion = std::move(completion)](MigrationResult result) {
        const auto &guard = *state->guard;
        completion({std::move(result), guard.refusal, guard.terminal,
                    guard.terminal ? std::optional{guard.ledger} : std::nullopt});
    };
    return enqueue_migration(writer, std::move(request), std::move(finish), std::move(state));
}

} // namespace subathon::server
