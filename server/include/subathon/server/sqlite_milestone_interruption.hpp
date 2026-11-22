// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_migrations.hpp>

namespace subathon::server {

// Explicit runner input for the full milestone interruption storage catalog.
[[nodiscard]] MigrationManifest milestone_slot_interruption_repository_manifest();

} // namespace subathon::server
