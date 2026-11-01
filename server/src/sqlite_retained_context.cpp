// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_retained_context.hpp>

// Explicit STORAGETEST-012 baseline stubs only. No manifest generation, input
// validation, SQL, byte codec, lookup, retention or transaction behavior exists.
namespace subathon::server {
MigrationManifest retained_context_manifest() {
    throw RetentionFailure(RetentionError::feature_not_implemented);
}
std::optional<RetainedDocument> SqliteRetainedContextStore::find(const RetainedKey &) const {
    throw RetentionFailure(RetentionError::feature_not_implemented);
}
RetainDisposition SqliteRetainedContextStore::retain(const RetainedDocument &) const {
    throw RetentionFailure(RetentionError::feature_not_implemented);
}
} // namespace subathon::server
