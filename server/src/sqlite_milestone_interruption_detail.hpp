// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_milestone_edit_detail.hpp"
#include <subathon/server/sqlite_milestone_interruption.hpp>

namespace subathon::server::milestone_interruption::detail {

using milestone_edit_sql::unique;
using milestone_sql::blob;
using milestone_sql::corrupt;
using milestone_sql::guarded;
using milestone_sql::integer;
using milestone_sql::reference;

inline constexpr qint64 member_limit = 4096;
inline constexpr qsizetype document_limit = milestone_edit_sql::document_limit;
inline constexpr qsizetype automation_limit = 262144;

inline bool ref(QByteArrayView value) { return value.size() > 0 && value.size() <= 4096; }
inline QByteArray verified_ref(const QVariant &cell) { return blob(reference(cell)); }
inline Decision answer(Disposition disposition) { return {disposition, {}, {}, {}, {}, {}}; }
inline QList<QVariant> scope(const MilestoneTrackKey &track) {
    return {blob(track.installation_ref), blob(track.track_ref)};
}
inline const FixtureStorageBytes *fixture(const AdmissionProfile *profile) {
    return profile ? profile->fixture_bytes : nullptr;
}

struct Origin {
    int kind;
    QByteArray scope, operation, occurrence;

    QList<QVariant> at(const MilestoneTrackKey &track) const {
        auto args = detail::scope(track);
        args.append({kind, scope, operation, occurrence});
        return args;
    }
};

bool valid(const MilestoneTrackKey &, const Occurrence &);
Origin origin(const Occurrence &, const AdmissionProfile *);
bool same(const Occurrence &, const Occurrence &);
void require_catalog(const SqliteWorkSession &);
QByteArray allocate(const QByteArray &fixture_value = {});
QByteArray numeric_payload(qint64);

struct Target {
    qint64 order;
    bool released, skipped;
    QByteArray definition, prefix, stop;
    bool deferred;
    qsizetype reviews;
};
std::optional<Target> target(const SqliteWorkSession &, const Occurrence &, const Origin &,
                             bool include_terminal = false);

struct Slot {
    MilestoneActionKind kind;
    QByteArray version;
    QVariant revision, payload_size, route_size, sender_size;
    bool applied;
};
std::optional<Slot> slot(const SqliteWorkSession &, const Occurrence &, const Origin &, qint64);
bool started(const Slot &);
std::optional<qint64> attempt_state(const SqliteWorkSession &, const Occurrence &, const Origin &,
                                    qint64 ordinal, const QByteArray &version, qint64 revision,
                                    bool playback);

struct PlaybackRecord {
    PlaybackInstance observation;
    QByteArray instance, attempt;
};
std::vector<PlaybackRecord> initial_playback(const SqliteWorkSession &, const Occurrence &,
                                             const Origin &, qint64 ordinal, const Slot &,
                                             qint64 state, const CommandAuthority::Facts &,
                                             const AdmissionProfile *);
qint64 playback_capacity(const Origin &, const MilestoneTrackKey &, const QByteArray &stop,
                         const QByteArray &decision, const std::vector<PlaybackRecord> &,
                         const AdmissionProfile *);
void save_playback(const SqliteWorkSession &, const MilestoneTrackKey &, const QByteArray &stop,
                   const PlaybackRecord &);

} // namespace subathon::server::milestone_interruption::detail
