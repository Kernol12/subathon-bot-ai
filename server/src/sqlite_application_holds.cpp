// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_application_holds.hpp>
#include <utility>

namespace subathon::server {
MigrationManifest application_hold_repository_manifest() {
    throw SqliteApplicationHoldNotImplemented();
}
SqliteApplicationHoldStore::SqliteApplicationHoldStore(SqliteWorkSession session,
                                                       SqliteHoldLimits limits)
    : session_(std::move(session)), limits_(limits) {}
std::optional<ApplicationHoldView> SqliteApplicationHoldStore::find(const std::string &) const {
    throw SqliteApplicationHoldNotImplemented();
}
ApplicationHoldActiveSet SqliteApplicationHoldStore::active(const ApplicationHoldSubject &) const {
    throw SqliteApplicationHoldNotImplemented();
}
ApplicationHoldHistoryResult
SqliteApplicationHoldStore::history(const ApplicationHoldHistoryQuery &) const {
    throw SqliteApplicationHoldNotImplemented();
}
ApplicationHoldMutationResult
SqliteApplicationHoldStore::record_detection(const ApplicationHoldDetection &) const {
    throw SqliteApplicationHoldNotImplemented();
}
ApplicationHoldMutationResult
SqliteApplicationHoldStore::stage_resolution(const ApplicationHoldResolutionBatch &) const {
    throw SqliteApplicationHoldNotImplemented();
}
} // namespace subathon::server
