// SPDX-License-Identifier: GPL-3.0-or-later
#include "v23_inspection_fixture.hpp"

namespace subathon::server::detail::testing {
namespace {
constexpr qint64 fixture_store_extent = 16 * 1024 * 1024;
constexpr StorageLimits fixture_inspection_profile{
    .statements_per_batch = 64,
    .statement_utf8_bytes = 64 * 1024,
    .result_rows = 1024,
    .result_value_bytes = 1024 * 1024,
};
}

std::variant<V23PreOpenObservation, MigrationReason>
V23InspectionFixture::inspect(const QString &path) {
    return V23InspectionAccess::inspect_with_limits(path, fixture_store_extent,
                                                    fixture_inspection_profile);
}

} // namespace subathon::server::detail::testing
