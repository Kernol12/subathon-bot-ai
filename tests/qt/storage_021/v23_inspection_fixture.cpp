// SPDX-License-Identifier: GPL-3.0-or-later
#include "v23_inspection_fixture.hpp"

namespace subathon::server::detail::testing {
namespace {
constexpr StorageLimits fixture_inspection_profile{64, 4096, 65536, 1048576, 8192, 16777216};
}

std::variant<V23PreOpenObservation, MigrationReason>
V23InspectionFixture::inspect(const QString &path) {
    return V23InspectionAccess::inspect_with_limits(path, fixture_inspection_profile);
}

} // namespace subathon::server::detail::testing
