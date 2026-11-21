// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_milestone_target_edits.hpp>

namespace storage_027_fixture {
using namespace subathon::server;

struct ManifestMissing {};
inline std::variant<ManifestMissing, MigrationManifest> compiled_successor() {
    return ManifestMissing{};
}

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
};

// Disposable admission profile for the Stage 1 seam. Playback admission is
// charged separately and retained with Stop for later exact activations.
struct AdmissionProfile {
    StorageLimits work;
    std::optional<StorageLimits> playback;
};

class ReplacePreview;
using Preview = std::variant<Disposition, std::shared_ptr<const ReplacePreview>>;

inline Preview preview_replace(const SqliteWorkSession &, CommandAuthority &, const Command &,
                               const AdmissionProfile * = nullptr) {
    return Disposition::not_implemented;
}

inline Decision stage_command(const SqliteWorkSession &, CommandAuthority &, const Command &,
                              const ReplacePreview * = nullptr,
                              const AdmissionProfile * = nullptr) {
    return {Disposition::not_implemented, {}, {}, {}, {}};
}

inline Decision read_command(const SqliteWorkSession &, CommandAuthority &, const Command &,
                             const AdmissionProfile * = nullptr) {
    return {Disposition::not_implemented, {}, {}, {}, {}};
}

struct LatePlayback {
    MilestoneTrackKey track;
    QByteArray stop;
    PlaybackInstance instance;
};

class PlaybackAuthority {
  public:
    virtual ~PlaybackAuthority() = default;
    virtual bool proves_current(const SqliteWorkSession &, const LatePlayback &) = 0;
};

inline Disposition stage_late_playback(const SqliteWorkSession &, PlaybackAuthority &,
                                       const LatePlayback &, const AdmissionProfile * = nullptr) {
    return Disposition::not_implemented;
}

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

inline Disposition stage_playback_handoff(const SqliteWorkSession &, PlaybackAuthority &,
                                          const PlaybackHandoff &,
                                          const AdmissionProfile * = nullptr) {
    return Disposition::not_implemented;
}

inline Disposition stage_playback_outcome(const SqliteWorkSession &, PlaybackAuthority &,
                                          const PlaybackEvidence &,
                                          const AdmissionProfile * = nullptr) {
    return Disposition::not_implemented;
}

struct ReplacementJob {
    Occurrence target;
    SelectedVersion version;
    QByteArray payload;
    std::string route, qualified_sender;
};

class ReplacementDispatchAuthority {
  public:
    virtual ~ReplacementDispatchAuthority() = default;
    virtual bool allows(const ReplacementJob &) = 0;
};

inline Disposition claim_replacement(const SqliteWorkSession &, const Occurrence &,
                                     const SelectedVersion &, qint64,
                                     ReplacementDispatchAuthority &) {
    return Disposition::not_implemented;
}

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

inline Disposition apply_replacement_bonus(const SqliteWorkSession &, CommandAuthority &,
                                           ReplacementBonusEffect &, const Command &,
                                           const SelectedVersion &) {
    return Disposition::not_implemented;
}

} // namespace storage_027_fixture
