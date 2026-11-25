// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_catalog_inspection.hpp"

#include <QSet>
#include <algorithm>
#include <limits>
#include <utility>

namespace subathon::server::detail {
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

static bool byte_less(const QByteArray &left, const QByteArray &right) {
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

static QList<CatalogObject> sorted_objects(const QList<CatalogObject> &objects) {
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

static bool text_value(const QVariant &value) {
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

namespace {
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
class CatalogInspection final {
    using Query = detail::ClockProbeQuery;

  public:
    CatalogInspection(const MigrationManifest &manifest, const QList<QByteArray> &hashes,
                      CatalogQuery execute)
        : manifest_(manifest), hashes_(hashes), execute_(execute) {}

    qint32 inspect_store(bool legacy_prefix) {
        // The encoding gate must precede every raw-text/schema/catalog observation.
        verify_encoding();
        if (legacy_prefix) {
            // A recognized legacy prefix keeps its normal admission. The
            // caller's migration manifest still owns its structural checks.
            try {
                const auto head = read_history_head();
                if (head < manifest_.steps.size())
                    return head;
            } catch (const MigrationFailure &error) {
                if (error.reason() != MigrationReason::catalog_mismatch &&
                    error.reason() != MigrationReason::unsupported_version)
                    throw;
                // Retain the normal v22 comparison order and diagnostic.
            }
        }
        const auto schema = read_catalog_schema(execute_);
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
        const auto head = read_history_head();
        if (!manifest_.supported_starting_versions.contains(head))
            throw MigrationFailure(MigrationReason::unsupported_version);
        verify_catalog_schema(schema, manifest_.steps[head - 1].expected_schema);
        verify_integrity();
        return head;
    }

  private:
    const MigrationManifest &manifest_;
    const QList<QByteArray> &hashes_;
    CatalogQuery execute_;

    qint32 read_history_head() {
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
        return head;
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

QList<CatalogObject> read_catalog_schema(CatalogQuery execute) {
    const auto rows = execute(
        ClockProbeQuery::schema,
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

qint32 inspect_store(const MigrationManifest &manifest, const QList<QByteArray> &hashes,
                     CatalogQuery execute, bool legacy_prefix) {
    return CatalogInspection(manifest, hashes, execute).inspect_store(legacy_prefix);
}

} // namespace subathon::server::detail
