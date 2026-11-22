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

} // namespace subathon::server::milestone_interruption
