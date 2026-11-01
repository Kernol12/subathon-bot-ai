// SPDX-License-Identifier: GPL-3.0-or-later
#include <QThread>
#include <subathon/server/sqlite_contributions.hpp>
#include <utility>

namespace subathon::server {
QByteArray encode_contribution_id(core::ContributionId) {
    throw SqliteContributionsNotImplemented();
}
core::ContributionId decode_contribution_id(const QByteArray &) {
    throw SqliteContributionsNotImplemented();
}
MigrationManifest contribution_repository_manifest() { throw SqliteContributionsNotImplemented(); }

SqliteContributionRepository::SqliteContributionRepository(SqliteWorkContext context,
                                                           SqliteContributionLimits limits)
    : capability_(std::move(context)), construction_thread_(QThread::currentThread()),
      limits_(limits) {}
SqliteContributionRepository::SqliteContributionRepository(AdoptActiveContributionSessionTag,
                                                           SqliteWorkSession session,
                                                           SqliteContributionLimits limits)
    : capability_(std::move(session)), construction_thread_(QThread::currentThread()),
      limits_(limits) {}

std::unique_ptr<ContributionTransaction> SqliteContributionRepository::begin() {
    throw SqliteContributionsNotImplemented();
}
} // namespace subathon::server
