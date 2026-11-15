// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/reference_intake.hpp>

namespace subathon::dev {
// Development-only composition DATA and provisioning, compiled into the executable only.
server::SessionLookupRequest owner_session();
server::ReferenceConfigurationFacts configuration(std::int64_t add_ms, std::int64_t cap_ms);
void provision(const server::SqliteWorkSession &, server::CurrentSessionClock &,
               const server::ReferenceConfiguration &);

class DevelopmentSessionClock final : public server::CurrentSessionClock {
  public:
    explicit DevelopmentSessionClock(bool controlled) : controlled_(controlled) {}
    server::SessionClockResolution sample_current() override;

  private:
    bool controlled_;
};
class ControlledReferenceClock final : public server::ReferenceClockSampler {
  public:
    server::ReferenceClockSampleResult sample() override;
};
} // namespace subathon::dev
