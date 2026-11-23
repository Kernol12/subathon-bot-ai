// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/milestone_group_decisions.hpp>
#include <subathon/server/sqlite_milestone_target_edits.hpp>

#include <array>
#include <variant>

namespace subathon::server {

// Explicit runner input for the full milestone interruption storage catalog.
[[nodiscard]] MigrationManifest milestone_slot_interruption_repository_manifest();

namespace milestone_interruption {

struct Occurrence {
    MilestoneTrackKey track;
    MilestoneOccurrenceKey occurrence;
    std::variant<ApplicationOperationKey, MilestoneEditKey> origin;
};

enum class QueueChoice { hold, continue_queue };
struct StopIntent {
    Occurrence target;
    std::optional<QueueChoice> choice;
};
struct Replacement {
    qint64 ordinal;
    MilestoneAction action;
    bool operator==(const Replacement &) const = default;
};
struct AuthorizedCapture {
    Replacement selection;
    QByteArray payload;
    std::string route, qualified_sender;
    std::optional<qint64> bonus_ms{};
};
struct ReplaceIntent {
    Occurrence target;
    std::vector<Replacement> patch;
};
struct ScopedCause {
    MilestoneTrackKey track;
    QByteArray ref;
    bool operator==(const ScopedCause &) const = default;
};

struct ResumeIntent {
    std::vector<ScopedCause> causes;
};
using Intent = std::variant<StopIntent, ReplaceIntent, ResumeIntent>;

struct Command {
    MilestoneTrackKey track;
    QByteArray operation;
    MilestoneEditActor actor;
    Intent intent;
};

enum class Disposition {
    not_implemented,
    committed,
    replay,
    conflict,
    denied,
    refused,
    unavailable,
    resource_limit
};
struct SelectedVersion {
    qint64 ordinal;
    QByteArray version;
    bool operator==(const SelectedVersion &) const = default;
};
struct Decision {
    Disposition disposition;
    std::vector<SelectedVersion> versions, cancellations;
    std::vector<QByteArray> added_causes, resolved_causes;
    std::vector<QByteArray> playback_intents{};
    bool operator==(const Decision &) const = default;
};

enum class StopCapability { supported, unsupported, unknown };
struct PlaybackInstance {
    Occurrence target;
    qint64 ordinal;
    QByteArray version;
    qint64 started_attempt;
    QByteArray adapter, activation, runtime_generation;
    StopCapability capability;
};

struct PresentationObservation {
    Occurrence target;
    QByteArray presentation, bound_generation, current_generation, owner_revision;
    qint64 started_ms, expiry_ms, observed_ms;
    QByteArray current_owner_revision;
    qint64 last_accepted_observation_ms;
};

class CommandAuthority {
  public:
    struct Facts {
        bool can_command, can_disclose;
        QByteArray grant_revision, configuration_revision, decision;
        std::vector<AuthorizedCapture> captures;
        std::vector<PlaybackInstance> observed_playback;
        QByteArray capability_revision{"capability-1"};
        std::optional<PresentationObservation> presentation{};
    };
    virtual ~CommandAuthority() = default;
    virtual std::optional<Facts> select(const SqliteWorkSession &, const Command &) = 0;
    // Where actor/session mismatch is known, report denial even when select()
    // cannot return command facts. Unknown identity evidence stays unavailable.
    virtual bool denies_actor(const SqliteWorkSession &, const Command &) { return false; }
    // The owner may qualify a retained decision beyond the current scoped grant.
    virtual bool discloses_retained(const SqliteWorkSession &, const Command &, const Facts &facts,
                                    const QByteArray &retained_decision) {
        return facts.can_disclose && facts.decision == retained_decision;
    }
    // The selected original action's version_ref identifies its immutable
    // definition. Missing history supplies no substitute amount.
    virtual std::optional<qint64>
    historical_bonus_amount(const SqliteWorkSession &, const Occurrence &, const Replacement &) = 0;
};

// Optional inputs at the Stage 1 fixture's allocation and encoding points.
// Normal authority, selection, metering and persistence still apply. These
// values impose no width or representation on production.
struct FixtureStorageBytes {
    QByteArray edit_origin_scope;
    QByteArray interruption_cause;
    std::array<QByteArray, 3> replacement_versions; // Positive fixture ordinals 1 through 3.
    QByteArray playback_attempt;                    // Exact fixture's original committed attempt 2.
    struct PlaybackRef {
        QByteArray activation, instance;
    };
    std::array<PlaybackRef, 3> playback_refs;
    QByteArray handoff;
};

// Playback admission is charged separately and retained with Stop for later activations.
struct AdmissionProfile {
    StorageLimits work;
    std::optional<StorageLimits> playback;
    const FixtureStorageBytes *fixture_bytes{};
};

class ReplacePreview;
using Preview = std::variant<Disposition, std::shared_ptr<const ReplacePreview>>;

Preview preview_replace(const SqliteWorkSession &, CommandAuthority &, const Command &,
                        const AdmissionProfile * = nullptr);
Preview preview_milestone_replacement(const SqliteWorkSession &, CommandAuthority &,
                                      const Command &);
Decision confirm_milestone_replacement(const SqliteWorkSession &, CommandAuthority &,
                                       const Command &, const ReplacePreview &);

using OriginalMilestoneAutomation = std::variant<DeliveryJobKey, MilestoneEditJobKey>;
struct MilestoneReplacementProfile {
    // The authority's revisions must change whenever compatibility or
    // sender-selection mode changes, including a return to an old value.
    std::function<std::optional<bool>(const CommandAuthority::Facts &, const Replacement &,
                                      const OriginalMilestoneAutomation &, std::string_view)>
        compatible;
    std::function<std::optional<DeliverySenderSelection>(const CommandAuthority::Facts &,
                                                         const Replacement &)>
        sender_selection;
};
struct MilestoneSlotDisclosure {
    qint64 ordinal;
    QByteArray selected_version;
    AuthorizedCapture old_capture;
    AuthorizedCapture new_capture;
    std::string inherited_format;
    std::optional<DeliverySenderSelection> proposed_sender_selection;
};
struct MilestoneReplacementDisclosure {
    Occurrence target;
    bool released;
    std::vector<MilestoneHoldKey> holds;
    std::vector<MilestoneSlotDisclosure> ordered;
    std::shared_ptr<const ReplacePreview> confirmation;
};
using MilestoneReplacementDisclosureReply =
    std::variant<Disposition, MilestoneReplacementDisclosure>;

MilestoneReplacementDisclosureReply
preview_milestone_replacement_disclosure(const SqliteWorkSession &, CommandAuthority &,
                                         const MilestoneReplacementProfile &, const Command &);

struct MilestoneDefinitionEdit {
    MilestoneTrackKey track;
    MilestoneEditActor actor;
    QByteArray operation;
    std::string expected_revision;
    std::string proposed_configuration;
};
struct MilestoneDefinitionBinding {
    MilestoneTrackKey track;
    std::string revision;
    std::string configuration;
};
struct MilestoneDefinitionAdmission {
    MilestoneDefinitionBinding binding;
    std::vector<core::MilestoneTarget> targets;
};
using MilestoneDefinitionAuthority = std::function<std::optional<MilestoneDefinitionAdmission>(
    const SqliteWorkSession &, const MilestoneDefinitionEdit &)>;

Disposition publish_milestone_definitions(const SqliteWorkSession &,
                                          const MilestoneDefinitionAuthority &,
                                          const MilestoneDefinitionEdit &);

struct PublishedMilestoneAutomation {
    MilestoneAction action;
    ApplicationEnvelope envelope;
    DeliverySenderSelection sender_selection;
    std::string route;
    std::string qualified_sender;
};
using PublishedMilestoneConfiguration = std::function<std::optional<PublishedMilestoneAutomation>(
    const SqliteWorkSession &, const MilestoneDefinitionBinding &, const core::MilestoneTarget &)>;

// The trusted application owner supplies the already admitted receipt plan
// and exact score operands. This is an internal capture turn for one newly
// earned occurrence, with no additional application effects.
Disposition capture_published_milestone_group(const SqliteWorkSession &,
                                              const PublishedMilestoneConfiguration &,
                                              const ContributionReceipt &,
                                              const ApplicationPlanKey &, const Occurrence &,
                                              qint64 score_before, qint64 score_after);

// An unused Replace key requires a current preview. A committed key uses the
// journal's bound preview after authority and exact typed target/patch checks.
Decision stage_command(const SqliteWorkSession &, CommandAuthority &, const Command &,
                       const ReplacePreview * = nullptr, const AdmissionProfile * = nullptr);

Decision read_command(const SqliteWorkSession &, CommandAuthority &, const Command &,
                      const AdmissionProfile * = nullptr);

// A cold, exact owner projection. Stop selection retains the original release
// bit and never reconstructs a live presentation or playback instance.
struct OccurrenceSelection {
    Occurrence target;
    qint64 queue_order;
    bool released, skipped;
    QByteArray stop;
    std::vector<SelectedVersion> selected_versions, cancellations;
    std::vector<MilestoneHoldKey> independent_holds;
    std::vector<QByteArray> interruption_holds;
};

enum class MilestoneStopCompletionSource { command, readback };
enum class MilestoneQueueAdoption { warm, cold };
struct AcknowledgedStop {
    Occurrence target;
    QByteArray operation;
    std::optional<ScopedCause> hold;
    MilestoneStopCompletionSource source{MilestoneStopCompletionSource::command};
    std::optional<PresentationObservation> presentation{};
};
struct PendingStopReadback {
    Occurrence target;
    QByteArray operation;
};
struct AcknowledgedResume {
    std::vector<ScopedCause> resolved;
};
using TimelineUpdate = std::variant<AcknowledgedStop, PendingStopReadback, AcknowledgedResume>;

namespace detail {
inline MilestoneTimelineOccurrence timeline_target(const Occurrence &target) {
    if (const auto *edit = std::get_if<MilestoneEditKey>(&target.origin))
        return {target.track, target.occurrence, *edit, {}};
    const auto &application = std::get<ApplicationOperationKey>(target.origin);
    return {target.track,
            target.occurrence,
            {},
            MilestoneApplicationOrigin{application.operation_scope, application.operation_ref}};
}

inline MilestoneTimelineCause timeline_cause(const ScopedCause &cause) {
    return {cause.track, cause.ref.toStdString()};
}

} // namespace detail

inline void adopt_milestone_timeline_update(MilestoneTimeline &timeline,
                                            std::chrono::milliseconds now,
                                            const TimelineUpdate &update) {
    if (const auto *stop = std::get_if<AcknowledgedStop>(&update)) {
        const auto target = detail::timeline_target(stop->target);
        if (stop->source != MilestoneStopCompletionSource::command &&
            stop->source != MilestoneStopCompletionSource::readback)
            return;
        if (stop->presentation && detail::timeline_target(stop->presentation->target) != target)
            return;
        timeline.adopt_stop(now, target, stop->operation.toStdString(),
                            stop->hold ? std::optional{detail::timeline_cause(*stop->hold)}
                                       : std::nullopt);
    } else if (const auto *pending = std::get_if<PendingStopReadback>(&update)) {
        timeline.observe_stop_completion(now, detail::timeline_target(pending->target),
                                         pending->operation.toStdString(), true, false);
    } else {
        const auto &resume = std::get<AcknowledgedResume>(update);
        std::vector<MilestoneTimelineCause> resolved;
        resolved.reserve(resume.resolved.size());
        for (const auto &cause : resume.resolved)
            resolved.push_back(detail::timeline_cause(cause));
        timeline.adopt_resume(now, resolved);
    }
}

void adopt_milestone_stop_completion(MilestoneTimeline &, std::chrono::milliseconds,
                                     const Command &, MilestoneStopCompletionSource,
                                     const SqliteWorkResult &, const std::optional<Decision> &);
void adopt_ordered_milestone_selection(MilestoneTimeline &, std::chrono::milliseconds,
                                       MilestoneQueueAdoption, const OccurrenceSelection &,
                                       const std::vector<CommittedMilestoneGroup> &pending);

std::optional<OccurrenceSelection> read_occurrence_selection(const SqliteWorkSession &,
                                                             CommandAuthority &,
                                                             const Command &stop_selector);

struct LatePlayback {
    MilestoneTrackKey track;
    QByteArray stop;
    PlaybackInstance instance;
};

struct PlaybackEvidence;

struct PlaybackCurrent {};
struct PlaybackProvenObsolete {
    QByteArray observed_generation;
};
struct PlaybackCurrentnessUnknown {};
using PlaybackCurrentness =
    std::variant<PlaybackCurrentnessUnknown, PlaybackCurrent, PlaybackProvenObsolete>;

class PlaybackAuthority {
  public:
    virtual ~PlaybackAuthority() = default;
    virtual bool proves_current(const SqliteWorkSession &, const LatePlayback &) = 0;
    // Only an affirmative observation for the exact instance can prove a new
    // generation. Legacy ports can prove currentness or leave it unknown.
    virtual PlaybackCurrentness currentness(const SqliteWorkSession &s, const LatePlayback &playback) {
        if (proves_current(s, playback))
            return PlaybackCurrent{};
        return PlaybackCurrentnessUnknown{};
    }
    virtual bool owns_evidence(const SqliteWorkSession &, const PlaybackEvidence &);
};

Disposition stage_late_playback(const SqliteWorkSession &, PlaybackAuthority &,
                                const LatePlayback &, const AdmissionProfile * = nullptr);

struct PlaybackHandoff {
    LatePlayback selected;
    QByteArray intent;
};

enum class PlaybackOutcome {
    requested,
    confirmed_stopped,
    failed,
    obsolete,
    unsupported,
    uncertain
};
struct PlaybackEvidence {
    PlaybackHandoff handoff;
    PlaybackOutcome outcome;
    QByteArray evidence;
};

Disposition stage_playback_handoff(const SqliteWorkSession &, PlaybackAuthority &,
                                   const PlaybackHandoff &, const AdmissionProfile * = nullptr);

Disposition stage_playback_outcome(const SqliteWorkSession &, PlaybackAuthority &,
                                   const PlaybackEvidence &, const AdmissionProfile * = nullptr);

struct PlaybackOutcomeRecord {
    PlaybackOutcome outcome;
    QByteArray evidence;
};

// Cold evidence reads require the exact authorized Stop but no live instance.
std::optional<PlaybackOutcomeRecord>
lookup_playback_evidence(const SqliteWorkSession &, CommandAuthority &, const Command &stop,
                         const PlaybackHandoff &, qint64 ordinal);

struct ReplacementJob {
    Occurrence target;
    SelectedVersion version;
    QByteArray payload;
    std::string route, qualified_sender;
};

struct EffectiveMilestoneRelease {
    CommittedMilestoneGroup group;
    std::vector<SelectedVersion> versions;
    std::vector<ReplacementJob> replacement_jobs;
};

// Re-read the selected versions in the writer; cached action definitions can
// lag replacement, but the occurrence and its immutable ordinal layout cannot.
std::optional<EffectiveMilestoneRelease>
select_effective_milestone_release(SqliteWriter &, const CommittedMilestoneGroup &,
                                   const Occurrence &);

// Consume only the completion of the exact queued -> sending turn. This is
// not the publication path for retained sending rows after recovery.
void publish_acknowledged_replacement_claim(const SqliteWorkResult &, Disposition,
                                            std::optional<ReplacementJob>,
                                            std::vector<ReplacementJob> &);

struct ReplacementAttempt {
    qint64 revision;
    DeliveryJobState state;
    QByteArray evidence;
};

struct StrandedReplacementAttempt {
    Occurrence target;
    SelectedVersion version;
    qint64 sending_revision;
};
struct StrandedReplacementPage {
    std::vector<StrandedReplacementAttempt> attempts;
    std::optional<QByteArray> next_after;
};

// Use a quiescent owner session during recovery. Continuation follows the
// last examined key, including when none of the examined versions are sending.
StrandedReplacementPage stranded_replacement_attempts(const SqliteWorkSession &,
                                                      const MilestoneTrackKey &,
                                                      const std::optional<QByteArray> &after,
                                                      std::size_t limit);

// Exact retained reads and settlement do not require the version to remain
// eligible to start. The caller owns the active writer session.
std::optional<ReplacementAttempt> lookup_replacement_attempt(const SqliteWorkSession &,
                                                             const Occurrence &,
                                                             const SelectedVersion &,
                                                             qint64 revision);

Disposition finish_replacement(const SqliteWorkSession &, const Occurrence &,
                               const SelectedVersion &, qint64 sending_revision, DeliveryEvidence,
                               const QByteArray &evidence);

class ReplacementDispatchAuthority {
  public:
    virtual ~ReplacementDispatchAuthority() = default;
    virtual bool allows(const ReplacementJob &) = 0;
    virtual std::optional<QByteArray> retry_decision(const ReplacementJob &) { return {}; }
};

class ReplacementDeliveryAdapter {
  public:
    virtual ~ReplacementDeliveryAdapter() = default;
    // Owned capture on the serial owner after normal claim acknowledgement.
    // The attempt revision is the sending event's exact revision.
    virtual void start(ReplacementJob, qint64 attempt_revision) = 0;
};
struct ReplacementDispatchCompletion {
    SqliteWorkResult work;
    std::optional<Disposition> disposition;
    bool adapter_threw{false};
};
using ReplacementDispatchCallback = std::function<void(ReplacementDispatchCompletion)>;

[[nodiscard]] StorageError dispatch_replacement_job(SqliteWriter &, Occurrence, SelectedVersion,
                                                    qint64 queued_revision,
                                                    std::shared_ptr<ReplacementDispatchAuthority>,
                                                    std::shared_ptr<ReplacementDeliveryAdapter>,
                                                    ReplacementDispatchCallback);

Disposition claim_replacement(const SqliteWorkSession &, const Occurrence &,
                              const SelectedVersion &, qint64, ReplacementDispatchAuthority &);

Disposition retry_replacement(const SqliteWorkSession &, const Occurrence &,
                              const SelectedVersion &, qint64 failed_revision,
                              ReplacementDispatchAuthority &);

class ReplacementBonusEffect {
  public:
    virtual ~ReplacementBonusEffect() = default;
    struct Actual {
        std::string effect_ref;
        qint64 milliseconds;
    };
    virtual Actual apply(const SqliteWorkSession &, const Occurrence &, const SelectedVersion &,
                         qint64 requested_ms) = 0;
};

Disposition apply_replacement_bonus(const SqliteWorkSession &, CommandAuthority &,
                                    ReplacementBonusEffect &, const Command &,
                                    const SelectedVersion &);

} // namespace milestone_interruption

} // namespace subathon::server
