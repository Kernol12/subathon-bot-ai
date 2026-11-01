// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_retained_context.hpp>

namespace subathon::server {
namespace {
const QString configuration_ddl =
    "CREATE TABLE subathon_retained_configurations(reference BLOB NOT NULL PRIMARY KEY "
    "CHECK(typeof(reference)='blob' AND length(reference)>0), document BLOB NOT NULL "
    "CHECK(typeof(document)='blob'))";
const QString context_ddl =
    "CREATE TABLE subathon_retained_contexts(reference BLOB NOT NULL PRIMARY KEY "
    "CHECK(typeof(reference)='blob' AND length(reference)>0), document BLOB NOT NULL "
    "CHECK(typeof(document)='blob'), configuration_ref BLOB CHECK(configuration_ref IS NULL OR "
    "(typeof(configuration_ref)='blob' AND length(configuration_ref)>0)) REFERENCES "
    "subathon_retained_configurations(reference))";

void validate_key(const RetainedKey &key, RetainedLimits limits) {
    if (limits.reference_bytes <= 0 || limits.document_bytes <= 0)
        throw RetentionFailure(RetentionError::invalid_options);
    if (key.kind != RetainedKind::configuration && key.kind != RetainedKind::arrival_context)
        throw RetentionFailure(RetentionError::invalid_kind);
    if (key.reference.empty() || key.reference.size() > size_t(limits.reference_bytes))
        throw RetentionFailure(RetentionError::invalid_reference);
}

void validate_document(const RetainedDocument &document, RetainedLimits limits) {
    validate_key(document.key, limits);
    if (document.document.isNull() || document.document.size() > limits.document_bytes)
        throw RetentionFailure(RetentionError::invalid_document);
    if (document.configuration &&
        (document.key.kind == RetainedKind::configuration ||
         document.configuration->reference.empty() ||
         document.configuration->reference.size() > size_t(limits.reference_bytes)))
        throw RetentionFailure(RetentionError::invalid_link);
}

// References have already passed the positive qsizetype limit, so this checked
// size conversion preserves every opaque byte rather than invoking a text codec.
QByteArray reference_blob(const std::string &reference) {
    return QByteArray(reference.data(), static_cast<qsizetype>(reference.size()));
}
QByteArray owned_blob(const QByteArray &bytes) {
    return bytes.isEmpty() ? QByteArray("", 0) : QByteArray(bytes.constData(), bytes.size());
}
std::string reference_string(const QByteArray &bytes) {
    return std::string(bytes.constData(), size_t(bytes.size()));
}

[[noreturn]] void stored_failure(const SqliteWorkSession &session, RetentionError error) {
    // This is exceptional selected-data failure, not a domain refusal. Establish
    // known rollback before exposing it; uncertain cleanup outranks the local error.
    const auto cleanup = session.cleanup();
    if (cleanup != WorkError::none)
        throw SqliteWorkFailure(cleanup);
    throw RetentionFailure(error);
}

bool sql_type(const QVariant &value, const char *type) {
    return value.metaType().id() == QMetaType::QString && value.toString() == type;
}

QByteArray selected_blob(const SqliteWorkSession &session, const QVariant &value,
                         const QVariant &type, const QVariant &length) {
    if (!sql_type(type, "blob") ||
        (length.metaType().id() != QMetaType::Int &&
         length.metaType().id() != QMetaType::LongLong) ||
        length.toLongLong() < 0)
        stored_failure(session, RetentionError::invalid_stored_data);
    const auto size = length.toLongLong();
    if (size == 0 && value.isNull())
        return QByteArray("", 0);
    if (value.metaType().id() != QMetaType::QByteArray || value.toByteArray().size() != size)
        stored_failure(session, RetentionError::invalid_stored_data);
    // SQL typeof/length establish present empty BLOB even if the driver buffer is
    // null. Every returned payload is owned and can be retained again unchanged.
    return owned_blob(value.toByteArray());
}

std::optional<RetainedDocument> select_record(const SqliteWorkSession &session,
                                              RetainedLimits limits, const RetainedKey &key) {
    const bool configuration = key.kind == RetainedKind::configuration;
    const QString sql =
        configuration
            ? "SELECT reference,document,NULL,typeof(reference),length(reference),typeof(document),"
              "length(document),'null',NULL FROM main.subathon_retained_configurations WHERE "
              "reference=?"
            : "SELECT reference,document,configuration_ref,typeof(reference),length(reference),"
              "typeof(document),length(document),typeof(configuration_ref),length(configuration_"
              "ref) "
              "FROM main.subathon_retained_contexts WHERE reference=?";
    const auto reference = reference_blob(key.reference);
    const auto rows = session.execute({sql, {reference}});
    if (rows.isEmpty())
        return std::nullopt;
    if (rows.size() != 1 || rows[0].size() != 9)
        stored_failure(session, RetentionError::invalid_stored_data);
    const auto &row = rows[0];
    auto stored_reference = selected_blob(session, row[0], row[3], row[4]);
    auto payload = selected_blob(session, row[1], row[5], row[6]);
    if (stored_reference.isEmpty() || stored_reference != reference)
        stored_failure(session, RetentionError::invalid_stored_data);
    std::optional<ConfigurationReference> link;
    if (sql_type(row[7], "null") && row[2].isNull() && row[8].isNull()) {
        link = std::nullopt;
    } else {
        const auto bytes = selected_blob(session, row[2], row[7], row[8]);
        if (configuration || bytes.isEmpty())
            stored_failure(session, RetentionError::invalid_stored_data);
        link = ConfigurationReference{reference_string(bytes)};
    }
    if (stored_reference.size() > limits.reference_bytes ||
        payload.size() > limits.document_bytes ||
        (link && link->reference.size() > size_t(limits.reference_bytes)))
        stored_failure(session, RetentionError::result_limit);
    if (link && !select_record(session, limits, {RetainedKind::configuration, link->reference}))
        stored_failure(session, RetentionError::invalid_stored_data);
    return RetainedDocument{
        {key.kind, reference_string(stored_reference)}, std::move(payload), std::move(link)};
}
} // namespace

MigrationManifest retained_context_manifest() {
    // Approved immutable application migration 1. This factory performs no SQL
    // and grants no maintenance/update authority; later slices append versions.
    MigrationStep step;
    step.version = 1;
    step.id = "retain-config-context-v1";
    step.sql = {configuration_ddl, context_ddl};
    step.expected_schema = {
        {"index", "sqlite_autoindex_subathon_migration_history_1", "subathon_migration_history",
         std::nullopt},
        {"index", "sqlite_autoindex_subathon_retained_configurations_1",
         "subathon_retained_configurations", std::nullopt},
        {"index", "sqlite_autoindex_subathon_retained_contexts_1", "subathon_retained_contexts",
         std::nullopt},
        {"table", "subathon_catalog", "subathon_catalog",
         "CREATE TABLE subathon_catalog(singleton INTEGER PRIMARY KEY, catalog_format INTEGER "
         "NOT NULL, store_identity TEXT NOT NULL)"},
        {"table", "subathon_migration_history", "subathon_migration_history",
         "CREATE TABLE subathon_migration_history(version INTEGER PRIMARY KEY, migration_id TEXT "
         "NOT NULL UNIQUE, content_sha256 BLOB NOT NULL)"},
        {"table", "subathon_retained_configurations", "subathon_retained_configurations",
         configuration_ddl},
        {"table", "subathon_retained_contexts", "subathon_retained_contexts", context_ddl}};
    MigrationManifest manifest;
    manifest.steps = {std::move(step)};
    manifest.supported_starting_versions = {0, 1};
    return manifest;
}

std::optional<RetainedDocument> SqliteRetainedContextStore::find(const RetainedKey &key) const {
    validate_key(key, limits_);
    return select_record(session_, limits_, key);
}
RetainDisposition SqliteRetainedContextStore::retain(const RetainedDocument &document) const {
    validate_document(document, limits_);
    // Validate the selected stored row and its old known target before comparing.
    // An immutable conflict precedes any lookup of a newly proposed target.
    if (const auto old = select_record(session_, limits_, document.key))
        return *old == document ? RetainDisposition::already_present : RetainDisposition::conflict;
    if (document.configuration &&
        !select_record(session_, limits_,
                       {RetainedKind::configuration, document.configuration->reference}))
        return RetainDisposition::missing_reference;
    const auto reference = reference_blob(document.key.reference);
    const auto payload = owned_blob(document.document);
    if (document.key.kind == RetainedKind::configuration) {
        (void)session_.execute({"INSERT INTO main.subathon_retained_configurations"
                                "(reference,document) VALUES (?,?)",
                                {reference, payload}});
    } else {
        const QVariant link = document.configuration
                                  ? QVariant(reference_blob(document.configuration->reference))
                                  : QVariant{};
        (void)session_.execute({"INSERT INTO main.subathon_retained_contexts"
                                "(reference,document,configuration_ref) VALUES (?,?,?)",
                                {reference, payload, link}});
    }
    return RetainDisposition::inserted;
}
} // namespace subathon::server
