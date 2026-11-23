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

struct ReplacementAttempt {
    qint64 revision;
    DeliveryJobState state;
    QByteArray evidence;
};

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
