// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_application_bundle.hpp>
#include <utility>

namespace subathon::server {
MigrationManifest application_bundle_repository_manifest() {
    throw SqliteApplicationBundleNotImplemented();
}
SqliteApplicationBundleStore::SqliteApplicationBundleStore(SqliteWorkSession session,
                                                           SqliteBundleLimits limits)
    : session_(std::move(session)), limits_(limits) {}
SqliteBundleInitialDisposition
SqliteApplicationBundleStore::retain_initial_state(const ApplicationStateSnapshot &) const {
    throw SqliteApplicationBundleNotImplemented();
}
std::optional<ApplicationStateSnapshot> SqliteApplicationBundleStore::read_state() const {
    throw SqliteApplicationBundleNotImplemented();
}
ApplicationBundleInputResult
SqliteApplicationBundleStore::read_inputs(const ApplicationPlanKey &,
                                          ApplicationSubjectKind) const {
    throw SqliteApplicationBundleNotImplemented();
}
std::optional<RecordedApplicationBundle>
SqliteApplicationBundleStore::lookup(const ApplicationOperationKey &) const {
    throw SqliteApplicationBundleNotImplemented();
}
ApplicationReversalBasisResult
SqliteApplicationBundleStore::read_reversal_basis(const ApplicationOriginalComponent &) const {
    throw SqliteApplicationBundleNotImplemented();
}
ApplicationBundleStageResult
SqliteApplicationBundleStore::stage(const PreparedApplicationBundle &) const {
    throw SqliteApplicationBundleNotImplemented();
}
} // namespace subathon::server
