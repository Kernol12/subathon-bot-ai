// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_milestone_interruption.hpp>

namespace subathon::server::milestone_interruption {

// Renderer/connector adapter boundary for one exact playback instance.
// The milestone owner rechecks authority and currentness, then calls this port
// outside SQL after normal acknowledgement of the one-way handoff marker.
// Returning does not establish that playback stopped. Exact outcome evidence
// returns separately through the owning adapter and the playback evidence path.
class PlaybackStopAdapter {
  public:
    virtual ~PlaybackStopAdapter() = default;
    virtual void request_stop(PlaybackHandoff handoff) = 0;
};

enum class MilestonePlaybackTurn { handoff, cold_restore, late_activation };

struct MilestonePlaybackCompletion {
    SqliteWorkResult work;
    std::optional<Disposition> disposition;
    std::optional<QByteArray> intent;
    bool adapter_threw{false};
};

// Called on the serial owner. Late activation only records the exact instance.
// Cold readback never calls the adapter. A handoff uses a separate acknowledged
// writer turn and calls the adapter only after the session has been revoked.
MilestonePlaybackCompletion process_milestone_playback_turn(SqliteWriter &, MilestonePlaybackTurn,
                                                            const PlaybackHandoff &,
                                                            PlaybackAuthority &,
                                                            PlaybackStopAdapter &);

struct RestoredMilestoneQueue {
    std::vector<ScopedCause> holds;
    std::vector<SelectedVersion> selected;
    std::optional<ActiveMilestonePresentation> presentation;
    std::optional<PlaybackOutcome> prior_handoff;
};

struct RestoredMilestoneQueueCompletion {
    SqliteWorkResult work;
    std::optional<RestoredMilestoneQueue> queue;
    std::optional<Disposition> disposition;
    std::optional<std::vector<ScopedCause>> durable_holds;
    bool authoritative_absence{false};
};

// Acknowledged Stop readback can recover its active Holds independently of the
// queue projection. Keep scheduling fenced when authority or writer acknowledgement
// is unavailable, until a fresh authoritative readback succeeds.
[[nodiscard]] RestoredMilestoneQueueCompletion
restore_interrupted_milestone_queue(SqliteWriter &, CommandAuthority &, const Command &stop,
                                    const std::optional<Command> &next,
                                    const std::optional<PlaybackHandoff> &);

} // namespace subathon::server::milestone_interruption
