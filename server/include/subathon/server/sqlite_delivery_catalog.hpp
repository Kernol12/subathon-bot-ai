// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_migrations.hpp>

namespace subathon::server {
// Additive delivery catalog. Host startup still selects the version 10 catalog.
[[nodiscard]] MigrationManifest delivery_jobs_repository_manifest();
} // namespace subathon::server
