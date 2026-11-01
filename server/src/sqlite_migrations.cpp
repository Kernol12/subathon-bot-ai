// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_migrations.hpp>

// Explicit STORAGETEST-011 baseline stubs only. No validation, digest, catalog,
// migration SQL, queueing, result fabrication or callback behavior is implemented.
// STORAGE-011 must satisfy the separately reviewed tests after intended red.
namespace subathon::server {
QByteArray canonical_migration_payload(const MigrationStep &) {
    throw MigrationFailure(MigrationReason::feature_not_implemented);
}
QByteArray migration_content_sha256(const MigrationStep &) {
    throw MigrationFailure(MigrationReason::feature_not_implemented);
}
MigrationAdmission SqliteMigrationRunner::enqueue(SqliteWriter &, MigrationRequest, Completion) {
    return {StorageError::feature_not_implemented, MigrationReason::feature_not_implemented};
}
} // namespace subathon::server
