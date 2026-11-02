// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_application_plans.hpp>
#include <utility>

namespace subathon::server {
QByteArray encode_plan_revision_uint64(std::uint64_t) {
    throw SqliteApplicationPlansNotImplemented();
}
std::uint64_t decode_plan_revision_uint64(const QByteArray &) {
    throw SqliteApplicationPlansNotImplemented();
}
SqlitePlanExactComponents encode_plan_exact(const core::Exact &, qsizetype) {
    throw SqliteApplicationPlansNotImplemented();
}
core::Exact decode_plan_exact(const SqlitePlanExactComponents &, qsizetype) {
    throw SqliteApplicationPlansNotImplemented();
}
MigrationManifest application_plan_repository_manifest() {
    throw SqliteApplicationPlansNotImplemented();
}
SqliteApplicationPlanStore::SqliteApplicationPlanStore(SqliteWorkSession session,
                                                       SqlitePlanLimits limits)
    : session_(std::move(session)), limits_(limits) {}
SqlitePlanInitialDisposition
SqliteApplicationPlanStore::retain_initial(const std::string &,
                                           const core::CapturedPlanRevision &) const {
    throw SqliteApplicationPlansNotImplemented();
}
std::optional<core::ApplicationPlan> SqliteApplicationPlanStore::find(const std::string &) const {
    throw SqliteApplicationPlansNotImplemented();
}
std::optional<SqlitePlanRevisionSnapshot>
SqliteApplicationPlanStore::find_revision(const std::string &, std::uint64_t) const {
    throw SqliteApplicationPlansNotImplemented();
}
SqlitePlanAppendDisposition
SqliteApplicationPlanStore::append(const std::string &, std::uint64_t,
                                   const core::CapturedPlanRevision &) const {
    throw SqliteApplicationPlansNotImplemented();
}
} // namespace subathon::server
