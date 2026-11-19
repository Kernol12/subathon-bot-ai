// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_migrations.hpp>

namespace subathon::server {

// Explicit provider credential catalog for the controlled migration runner.
[[nodiscard]] MigrationManifest provider_credentials_repository_manifest();

} // namespace subathon::server
