// SPDX-License-Identifier: GPL-3.0-or-later
#include "fake_ops02_update_hold.hpp"

namespace subathon::server::detail::testing {

Ops02UpdateHold FakeOps02UpdateHoldIssuer::issue() {
    Ops02UpdateHold hold;
    hold.fixture_hold_ = std::make_unique<Ops02UpdateHold::FixtureHold>();
    return hold;
}

} // namespace subathon::server::detail::testing
