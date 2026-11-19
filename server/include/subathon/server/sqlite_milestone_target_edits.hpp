// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/core/milestone_target_edits.hpp>
#include <subathon/server/milestone_group_decisions.hpp>

namespace subathon::server {

struct MilestoneTrackKey {
    std::string installation_ref;
    std::string track_ref;
    bool operator==(const MilestoneTrackKey &) const = default;
};

struct MilestoneEditKey {
    MilestoneTrackKey track;
    std::string operation_ref;
    bool operator==(const MilestoneEditKey &) const = default;
};

struct MilestoneEditJobKey {
    MilestoneEditKey operation;
    MilestoneOccurrenceKey occurrence;
    std::int64_t action_ordinal;
    bool operator==(const MilestoneEditJobKey &) const = default;
};

using MilestoneDeliveryKey = std::variant<DeliveryJobKey, MilestoneEditJobKey>;

struct MilestoneTrackSeed {
    MilestoneTrackKey track;
    std::string configuration_ref;
    std::string definition_revision;
    std::string prefix_revision;
    std::vector<core::MilestoneTarget> targets;
    struct Member {
        core::MilestoneCompletion completion;
        ApplicationOperationKey application;
        MilestoneOccurrenceKey occurrence;
    };
    // Absent evidence is distinct from an explicitly known empty prefix.
    std::optional<std::vector<Member>> completion_prefix;
};

struct MilestoneEditLifecycle {
    std::string revision;
    core::TimerState timer;
    std::int64_t debt_ms{};
    std::optional<std::int64_t> deadline_utc_ms;
    std::vector<std::string> pause_reasons;
    std::string reopen_pending_action_revision;
    bool post_finish_progression{};
    bool post_finish_automation{};
};

struct MilestoneEditActor {
    std::string actor_ref;
    std::string session_lineage;
};

struct MilestoneEditDeliveryCapture {
    std::string route_ref;
    DeliverySenderSelection selection;
    std::string sender_ref;
    std::string action_ref;
    std::string action_version_ref;
};

struct MilestoneEditCapture {
    std::string milestone_ref;
    MilestoneOccurrenceKey occurrence;
    std::vector<MilestoneAction> actions;
    MilestoneTimingOverride timing;
    struct Job {
        std::int64_t ordinal;
        ApplicationEnvelope envelope;
        MilestoneEditDeliveryCapture capture;
    };
    std::vector<Job> jobs;
};

struct MilestoneEditIntent {
    MilestoneEditKey key;
    MilestoneDecisionEvidence evidence;
    std::vector<core::MilestoneTarget> patch;
    std::vector<MilestoneAcceptanceChoice> choices;
};

struct MilestoneEditFacts {
    std::string configuration_ref;
    std::string capture_revision;
    std::string grant_revision;
    MilestoneEditLifecycle lifecycle;
    bool unfinished_grant{};
    bool correction_grant{};
    bool disclosure_grant{};
    std::string candidate_definition_revision;
    std::string candidate_prefix_revision;
    // Selected configuration supplies candidate captures by milestone identity.
    std::vector<MilestoneEditCapture> captures;
};

// The owner supplies current session-scoped facts inside the writer unit.
// These facts must remain valid through commit under the owner's serialization.
class MilestoneEditAuthority {
  public:
    virtual ~MilestoneEditAuthority() = default;
    [[nodiscard]] virtual std::optional<MilestoneEditFacts>
    select(const SqliteWorkSession &, const MilestoneEditActor &, const MilestoneEditIntent &) = 0;
};

class MilestoneEditBinding;

struct MilestoneEditPreview {
    core::MilestoneEditPlan plan;
    std::string prior_definition_revision;
    std::string next_definition_revision;
    std::string score_head;
    std::int64_t score{};
    std::vector<core::MilestoneTarget> previous_targets;
    MilestoneEditLifecycle lifecycle;
    std::vector<MilestoneEditCapture> achievements;
    std::shared_ptr<const MilestoneEditBinding> confirmation;
};

struct MilestoneEditAchievement {
    CommittedMilestoneGroup group;
    std::int64_t order;
    MilestoneActionChoice choice;
};

struct MilestoneEditResult {
    std::string definition_revision;
    std::string prefix_revision;
    std::vector<MilestoneEditAchievement> achievements;
};

struct MilestoneEditReply {
    core::MilestoneEditStatus status;
    std::optional<MilestoneEditResult> result;
};

[[nodiscard]] MigrationManifest milestone_target_edits_repository_manifest();

// All results are provisional until the caller receives normal committed work.
[[nodiscard]] bool initialize_milestone_track(const SqliteWorkSession &,
                                              const MilestoneTrackSeed &);
[[nodiscard]] MilestoneEditPreview preview_milestone_target_edit(const SqliteWorkSession &,
                                                                 MilestoneEditAuthority &,
                                                                 const MilestoneEditActor &,
                                                                 const MilestoneEditIntent &);
[[nodiscard]] MilestoneEditReply confirm_milestone_target_edit(const SqliteWorkSession &,
                                                               MilestoneEditAuthority &,
                                                               const MilestoneEditActor &,
                                                               const MilestoneEditIntent &,
                                                               const MilestoneEditBinding &);
[[nodiscard]] std::optional<MilestoneEditResult>
read_milestone_target_edit(const SqliteWorkSession &, MilestoneEditAuthority &,
                           const MilestoneEditActor &, const MilestoneEditIntent &);

// Separate release unit reads current authorization and lifecycle gates for
// the exact committed edit head. Application releases keep their owning path.
class MilestoneEditReleaseAuthority {
  public:
    virtual ~MilestoneEditReleaseAuthority() = default;
    [[nodiscard]] virtual bool allows(const SqliteWorkSession &, const MilestoneTrackKey &,
                                      const MilestoneOccurrenceKey &) = 0;
};

[[nodiscard]] StorageError release_milestone_head(SqliteWriter &, MilestoneOccurrenceKey,
                                                  std::shared_ptr<MilestoneEditReleaseAuthority>,
                                                  MilestoneReleaseCallback);

// Typed extension of the existing delivery boundary. A selected application job
// uses the original application lane. Edits never supply an application job key.
struct MilestoneEditJob {
    MilestoneEditJobKey key;
    MilestoneEditDeliveryCapture capture;
    QByteArray action_document;
    DeliveryJobState state;
    std::int64_t revision;
    std::optional<std::int64_t> attempt;
};

struct MilestoneEditJobResult {
    DeliveryDisposition disposition;
    std::optional<MilestoneEditJob> job;
};

class MilestoneEditDispatchAuthority {
  public:
    virtual ~MilestoneEditDispatchAuthority() = default;
    [[nodiscard]] virtual bool allows(const MilestoneEditJob &) = 0;
    [[nodiscard]] virtual std::optional<std::string> retry_policy(const MilestoneEditJob &) = 0;
};

struct MilestoneStrandedPage {
    struct Attempt {
        MilestoneDeliveryKey key;
        std::int64_t revision;
    };
    std::vector<Attempt> attempts;
    std::optional<MilestoneDeliveryKey> next_after;
};

[[nodiscard]] std::optional<MilestoneEditJob>
lookup_milestone_edit_job(const SqliteWorkSession &, const MilestoneEditJobKey &);
[[nodiscard]] MilestoneEditJobResult claim_milestone_edit_job(const SqliteWorkSession &,
                                                              const MilestoneEditJobKey &,
                                                              std::int64_t,
                                                              MilestoneEditDispatchAuthority &);
[[nodiscard]] MilestoneEditJobResult
finish_milestone_edit_job(const SqliteWorkSession &, const MilestoneEditJobKey &, std::int64_t,
                          std::int64_t, DeliveryEvidence, const std::string &);
[[nodiscard]] MilestoneEditJobResult retry_milestone_edit_job(const SqliteWorkSession &,
                                                              const MilestoneEditJobKey &,
                                                              std::int64_t,
                                                              MilestoneEditDispatchAuthority &);
[[nodiscard]] MilestoneStrandedPage
stranded_milestone_delivery(const SqliteWorkSession &, const std::optional<MilestoneDeliveryKey> &,
                            std::size_t);
[[nodiscard]] MilestoneEditJobResult
recover_milestone_edit_job(const SqliteWorkSession &, const MilestoneEditJobKey &, std::int64_t);

} // namespace subathon::server
