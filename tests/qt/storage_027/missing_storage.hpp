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
};
struct ReplaceIntent {
    Occurrence target;
    std::vector<Replacement> patch;
};
struct ResumeIntent {
    std::vector<QByteArray> causes;
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

class CommandAuthority {
  public:
    struct Facts {
        bool can_command, can_disclose;
        QByteArray grant_revision, configuration_revision, decision;
        std::vector<AuthorizedCapture> captures;
        std::vector<PlaybackInstance> observed_playback;
    };
    virtual ~CommandAuthority() = default;
    virtual std::optional<Facts> select(const SqliteWorkSession &, const Command &) = 0;
};

class ReplacePreview;
using Preview = std::variant<Disposition, std::shared_ptr<const ReplacePreview>>;

inline Preview preview_replace(const SqliteWorkSession &, CommandAuthority &, const Command &) {
    return Disposition::not_implemented;
}

inline Decision stage_command(const SqliteWorkSession &, CommandAuthority &, const Command &,
                              const ReplacePreview * = nullptr) {
    return {Disposition::not_implemented, {}, {}, {}, {}};
}

inline Decision read_command(const SqliteWorkSession &, CommandAuthority &, const Command &) {
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
                                       const LatePlayback &) {
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

} // namespace storage_027_fixture
