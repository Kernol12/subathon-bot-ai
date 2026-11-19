// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_application_holds.hpp>

namespace subathon::server {

// Called exclusively after the financial boundary checks the complete mixed set.
// The shipped hold mutation stores the ordinary projection, including its link.
[[nodiscard]] ApplicationHoldMutationResult
stage_checked_ordinary_resolution(const SqliteWorkSession &,
                                  const ApplicationHoldResolutionBatch &);

} // namespace subathon::server
