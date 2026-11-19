// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_milestone_groups.hpp>

namespace subathon::server {

MigrationManifest milestone_groups_repository_manifest() {
    throw MilestoneGroupsNotImplemented("milestone_groups_repository_manifest");
}

void SqliteMilestoneGroupStore::stage(const ApplicationOperationKey &,
                                      const std::vector<CapturedMilestoneGroup> &) const {
    throw MilestoneGroupsNotImplemented("stage");
}

std::vector<CapturedMilestoneGroup> SqliteMilestoneGroupStore::pending() const {
    throw MilestoneGroupsNotImplemented("pending");
}

MilestoneBonusDisposition SqliteMilestoneGroupStore::apply_bonus(const MilestoneBonusKey &,
                                                                 MilestoneBonusAuthority &,
                                                                 MilestoneBonusEffect &) const {
    throw MilestoneGroupsNotImplemented("apply_bonus");
}

StorageError release_milestone_head(SqliteWriter &, MilestoneOccurrenceKey,
                                    MilestoneReleaseCallback) {
    throw MilestoneGroupsNotImplemented("release_milestone_head");
}

} // namespace subathon::server
