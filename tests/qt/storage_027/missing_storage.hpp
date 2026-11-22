// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_milestone_interruption.hpp>

namespace storage_027_fixture {
using namespace subathon::server;
using namespace subathon::server::milestone_interruption;

struct ManifestMissing {};
inline std::variant<ManifestMissing, MigrationManifest> compiled_successor() {
    return milestone_slot_interruption_repository_manifest();
}

} // namespace storage_027_fixture
