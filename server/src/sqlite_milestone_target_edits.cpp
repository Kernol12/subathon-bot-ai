// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_milestone_target_edits.hpp>

namespace subathon::server {
namespace {

[[noreturn]] void missing(const char *entry) {
    throw std::logic_error(std::string("MILESTONE-002 NI: ") + entry);
}

} // namespace

MigrationManifest milestone_target_edits_repository_manifest() {
    missing("milestone_target_edits_repository_manifest");
}

bool initialize_milestone_track(const SqliteWorkSession &, const MilestoneTrackSeed &) {
    missing("initialize_milestone_track");
}

MilestoneEditPreview preview_milestone_target_edit(const SqliteWorkSession &,
                                                   MilestoneEditAuthority &,
                                                   const MilestoneEditActor &,
                                                   const MilestoneEditIntent &) {
    missing("preview_milestone_target_edit");
}

MilestoneEditReply confirm_milestone_target_edit(const SqliteWorkSession &,
                                                 MilestoneEditAuthority &,
                                                 const MilestoneEditActor &,
                                                 const MilestoneEditIntent &,
                                                 const MilestoneEditBinding &) {
    missing("confirm_milestone_target_edit");
}

std::optional<MilestoneEditResult> read_milestone_target_edit(const SqliteWorkSession &,
                                                              MilestoneEditAuthority &,
                                                              const MilestoneEditActor &,
                                                              const MilestoneEditIntent &) {
    missing("read_milestone_target_edit");
}

StorageError release_milestone_head(SqliteWriter &, MilestoneOccurrenceKey,
                                    std::shared_ptr<MilestoneEditReleaseAuthority>,
                                    MilestoneReleaseCallback) {
    missing("release_milestone_head (edit origin)");
}

std::optional<MilestoneEditJob> lookup_milestone_edit_job(const SqliteWorkSession &,
                                                          const MilestoneEditJobKey &) {
    missing("lookup_milestone_edit_job");
}

MilestoneEditJobResult claim_milestone_edit_job(const SqliteWorkSession &,
                                                const MilestoneEditJobKey &, std::int64_t,
                                                MilestoneEditDispatchAuthority &) {
    missing("claim_milestone_edit_job");
}

MilestoneEditJobResult finish_milestone_edit_job(const SqliteWorkSession &,
                                                 const MilestoneEditJobKey &, std::int64_t,
                                                 std::int64_t, DeliveryEvidence,
                                                 const std::string &) {
    missing("finish_milestone_edit_job");
}

MilestoneEditJobResult retry_milestone_edit_job(const SqliteWorkSession &,
                                                const MilestoneEditJobKey &, std::int64_t,
                                                MilestoneEditDispatchAuthority &) {
    missing("retry_milestone_edit_job");
}

MilestoneStrandedPage stranded_milestone_delivery(const SqliteWorkSession &,
                                                  const std::optional<MilestoneDeliveryKey> &,
                                                  std::size_t) {
    missing("stranded_milestone_delivery");
}

MilestoneEditJobResult recover_milestone_edit_job(const SqliteWorkSession &,
                                                  const MilestoneEditJobKey &, std::int64_t) {
    missing("recover_milestone_edit_job");
}

} // namespace subathon::server
