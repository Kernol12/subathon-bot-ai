// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_v23_rebuild.hpp"

namespace subathon::server::detail::testing {

// Test double for the inert baseline. Real OPS-02 issuance belongs to #855.
class FakeOps02UpdateHoldIssuer final {
  public:
    [[nodiscard]] static Ops02UpdateHold issue();
};

} // namespace subathon::server::detail::testing
