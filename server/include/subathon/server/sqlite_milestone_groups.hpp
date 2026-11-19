// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <functional>
#include <subathon/server/delivery_jobs.hpp>
#include <subathon/server/milestone_timeline.hpp>
#include <subathon/server/sqlite_migrations.hpp>

namespace subathon::server {

// Revision 1. See docs/contracts/sqlite-milestone-groups-v1.md.
class MilestoneGroupsNotImplemented final : public std::logic_error {
  public:
    explicit MilestoneGroupsNotImplemented(const char *entry)
        : std::logic_error(std::string("MILESTONE-009 milestone groups NI: ") + entry) {}
};

[[nodiscard]] MigrationManifest milestone_groups_repository_manifest();

struct MilestoneJobMembership {
    std::int64_t action_ordinal;
    DeliveryJobKey job;
};

struct CapturedMilestoneGroup {
    CommittedMilestoneGroup group;
    std::vector<MilestoneJobMembership> jobs;
};

struct MilestoneBonusKey {
    MilestoneOccurrenceKey occurrence;
    std::int64_t action_ordinal;
};

enum class MilestoneBonusDisposition { applied, already_applied, refused };

class MilestoneBonusAuthority {
  public:
    virtual ~MilestoneBonusAuthority() = default;
    // Trusted bounded owner-thread decision from current authority for this exact capture.
    // The port must not call the writer or pump events.
    [[nodiscard]] virtual bool allows(const MilestoneBonusKey &, const MilestoneAction &) = 0;
};

class MilestoneBonusEffect {
  public:
    virtual ~MilestoneBonusEffect() = default;
    // Applies one actual effect and returns its durable reference. Trusted
    // synchronous implementation using only the supplied active session.
    // No commit, rollback, external I/O or event pumping by the effect port.
    [[nodiscard]] virtual std::string apply(const SqliteWorkSession &, const MilestoneBonusKey &,
                                            const MilestoneAction &) = 0;
};

class SqliteMilestoneGroupStore final {
  public:
    explicit SqliteMilestoneGroupStore(SqliteWorkSession session) : session_(std::move(session)) {}

    // Call in newly staged originating application work, after staging its
    // bundle and delivery captures. Occurrence keys and action positions are
    // immutable. Invalid membership fails the unit. All results are provisional.
    void stage(const ApplicationOperationKey &origin,
               const std::vector<CapturedMilestoneGroup> &groups) const;

    // Ordered unreleased captures. Restore only after normal work completion.
    [[nodiscard]] std::vector<CapturedMilestoneGroup> pending() const;

    // Explicit selection only. The exact captured position must be a bonus
    // under a committed release. Apply and record its receipt on this session.
    // No effect call on refusal or existing receipt. An effect/receipt failure
    // rolls back the unit. The caller owns commit and normal publication.
    [[nodiscard]] MilestoneBonusDisposition apply_bonus(const MilestoneBonusKey &key,
                                                        MilestoneBonusAuthority &authority,
                                                        MilestoneBonusEffect &effect) const;

  private:
    SqliteWorkSession session_;
};

enum class MilestoneReleaseDisposition { released, refused };
struct MilestoneReleaseCompletion {
    SqliteWorkResult work;
    // Present only on normal acknowledged committed completion.
    std::optional<MilestoneReleaseDisposition> disposition;
};

using MilestoneReleaseCallback = std::function<void(MilestoneReleaseCompletion)>;
[[nodiscard]] StorageError release_milestone_head(SqliteWriter &writer, MilestoneOccurrenceKey key,
                                                  MilestoneReleaseCallback completion);

} // namespace subathon::server
