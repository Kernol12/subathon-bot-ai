// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_migrations.hpp>

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

struct RequestState {
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

// This executor owns no connection. Every observation and change is charged to
// the existing session, including raw/type verification and repeated final checks.
class MigrationExecution final {
  public:
    MigrationExecution(const MigrationRequest &request, const QList<QByteArray> &hashes,
                       RequestState &state)
        : request_(request), hashes_(hashes), state_(state) {}

    QVariant run(SqliteWorkContext context) {
        try {
            probe(MigrationCheckpoint::before_begin, 0, 0);
            session_ = context.begin();
            const auto old_version = inspect_store();
            qint32 final_version = old_version;
            qsizetype applied = 0;
            if (request_.mode == MigrationMode::advance) {
                if (old_version == 0)
                    bootstrap();
                for (qsizetype i = old_version; i < request_.manifest.steps.size(); ++i) {
                    const auto &step = request_.manifest.steps[i];
                    for (qsizetype statement = 0; statement < step.sql.size(); ++statement) {
                        probe(MigrationCheckpoint::before_statement, step.version, statement + 1);
                        (void)execute(step.sql[statement], {}, MigrationReason::sql_failure);
                    }
                    probe(MigrationCheckpoint::before_history, step.version, step.sql.size());
                    verify_schema(read_schema(), step.expected_schema);
                    (void)execute("INSERT INTO main.subathon_migration_history"
                                  "(version,migration_id,content_sha256) VALUES (?,?,?)",
                                  {step.version, step.id, hashes_[i]},
                                  MigrationReason::sql_failure);
                    ++applied;
                }
                final_version = static_cast<qint32>(request_.manifest.steps.size());
            }
            if (inspect_store() != final_version)
                refuse(MigrationReason::catalog_mismatch);
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

    [[noreturn]] void refuse(MigrationReason reason) {
        state_.remember(reason);
        (void)session_.cleanup();
        throw MigrationFailure(reason);
    }
    void probe(MigrationCheckpoint checkpoint, qint32 version, qsizetype statement) {
        if (!request_.cancel)
            return;
        bool cancelled;
        try {
            cancelled = request_.cancel({checkpoint, version, statement});
        } catch (...) {
            refuse(MigrationReason::callback_failure);
        }
        if (cancelled)
            refuse(MigrationReason::cancelled);
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
    QString stored_text(const QVariant &value, const QVariant &type, const QVariant &raw,
                        MigrationReason reason) {
        if (!text_value(type) || type.toString() != "text" || !text_value(value) ||
            raw.metaType().id() != QMetaType::QByteArray)
            refuse(reason);
        try {
            if (strict_utf8(value.toString()) == raw.toByteArray())
                return value.toString();
        } catch (const MigrationFailure &) {
            // Invalid encoded database text is a catalog/schema refusal, not a
            // change to the already admitted manifest's validity.
        }
        refuse(reason);
    }
    qint64 stored_integer(const QVariant &value, const QVariant &type) {
        if (!text_value(type) || type.toString() != "integer" || !integer_value(value))
            refuse(MigrationReason::catalog_mismatch);
        return value.toLongLong();
    }
    void verify_encoding() {
        const auto rows =
            execute("SELECT * FROM pragma_encoding()", {}, MigrationReason::inspection_failed);
        if (rows.size() != 1 || rows[0].size() != 1 || !text_value(rows[0][0]) ||
            rows[0][0].toString() != "UTF-8")
            refuse(MigrationReason::unsupported_encoding);
    }
    void verify_integrity() {
        const auto rows = execute("SELECT * FROM pragma_integrity_check() LIMIT 2", {},
                                  MigrationReason::inspection_failed);
        if (rows.size() != 1 || rows[0].size() != 1 || !text_value(rows[0][0]) ||
            rows[0][0].toString() != "ok")
            refuse(MigrationReason::integrity_failed);
        if (!execute("SELECT * FROM pragma_foreign_key_check() LIMIT 1", {},
                     MigrationReason::inspection_failed)
                 .isEmpty())
            refuse(MigrationReason::integrity_failed);
    }
    QList<CatalogObject> read_schema() {
        const auto rows = execute(
            "SELECT type,name,tbl_name,sql,typeof(type),typeof(name),typeof(tbl_name),typeof(sql),"
            "CAST(type AS BLOB),CAST(name AS BLOB),CAST(tbl_name AS BLOB),CAST(sql AS BLOB) "
            "FROM main.sqlite_schema",
            {}, MigrationReason::inspection_failed);
        QList<CatalogObject> objects;
        for (const auto &row : rows) {
            if (row.size() != 12)
                refuse(MigrationReason::schema_mismatch);
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
    void verify_schema(const QList<CatalogObject> &actual, const QList<CatalogObject> &expected) {
        if (actual != expected)
            refuse(MigrationReason::schema_mismatch);
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
            refuse(MigrationReason::foreign_store);
        if (!header || !history)
            refuse(MigrationReason::catalog_mismatch);
        for (const auto &required : reserved_objects())
            if (!schema.contains(required))
                refuse(MigrationReason::schema_mismatch);
        const auto headers =
            execute("SELECT singleton,catalog_format,store_identity,typeof(singleton),"
                    "typeof(catalog_format),typeof(store_identity),CAST(store_identity AS BLOB) "
                    "FROM main.subathon_catalog",
                    {}, MigrationReason::inspection_failed);
        if (headers.size() != 1 || headers[0].size() != 7)
            refuse(MigrationReason::catalog_mismatch);
        const auto &row = headers[0];
        if (stored_integer(row[0], row[3]) != 1)
            refuse(MigrationReason::catalog_mismatch);
        if (stored_integer(row[1], row[4]) != 1)
            refuse(MigrationReason::unsupported_catalog_format);
        if (stored_text(row[2], row[5], row[6], MigrationReason::catalog_mismatch) !=
            store_identity)
            refuse(MigrationReason::foreign_store);
        const auto histories = execute(
            "SELECT version,migration_id,content_sha256,typeof(version),typeof(migration_id),"
            "typeof(content_sha256),CAST(migration_id AS BLOB) "
            "FROM main.subathon_migration_history ORDER BY version",
            {}, MigrationReason::inspection_failed);
        if (histories.isEmpty())
            refuse(MigrationReason::catalog_mismatch);
        QSet<QString> ids;
        qint32 head = 0;
        for (const auto &entry : histories) {
            if (entry.size() != 7)
                refuse(MigrationReason::catalog_mismatch);
            const auto version = stored_integer(entry[0], entry[3]);
            const auto id =
                stored_text(entry[1], entry[4], entry[6], MigrationReason::catalog_mismatch);
            if (version < 1 || version > std::numeric_limits<qint32>::max() ||
                version != static_cast<qint64>(head) + 1 || !valid_id(id, 128) ||
                ids.contains(id) || !text_value(entry[5]) || entry[5].toString() != "blob" ||
                entry[2].metaType().id() != QMetaType::QByteArray ||
                entry[2].toByteArray().size() != 32)
                refuse(MigrationReason::catalog_mismatch);
            if (version > request_.manifest.steps.size())
                refuse(MigrationReason::unsupported_version);
            const auto &step = request_.manifest.steps[version - 1];
            if (id != step.id || entry[2].toByteArray() != hashes_[version - 1])
                refuse(MigrationReason::catalog_mismatch);
            ids.insert(id);
            head = static_cast<qint32>(version);
        }
        if (!request_.manifest.supported_starting_versions.contains(head))
            refuse(MigrationReason::unsupported_version);
        verify_schema(schema, request_.manifest.steps[head - 1].expected_schema);
        verify_integrity();
        return head;
    }
    void bootstrap() {
        (void)execute(header_ddl, {}, MigrationReason::sql_failure);
        (void)execute(history_ddl, {}, MigrationReason::sql_failure);
        (void)execute("INSERT INTO main.subathon_catalog"
                      "(singleton,catalog_format,store_identity) VALUES (?,?,?)",
                      {1, 1, store_identity}, MigrationReason::sql_failure);
    }
};
} // namespace

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
MigrationAdmission SqliteMigrationRunner::enqueue(SqliteWriter &writer, MigrationRequest request,
                                                  Completion completion) {
    QList<QByteArray> hashes;
    try {
        hashes = preflight(request);
    } catch (const MigrationFailure &) {
        return {StorageError::invalid_options, MigrationReason::invalid_manifest};
    }
    if (!completion)
        return {StorageError::invalid_work, MigrationReason::none};
    auto state = std::make_shared<RequestState>();
    auto error = writer.enqueue_work(
        [request = std::move(request), hashes = std::move(hashes),
         state](SqliteWorkContext context) {
            return MigrationExecution(request, hashes, *state).run(context);
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
} // namespace subathon::server
