// SPDX-License-Identifier: GPL-3.0-or-later
// Test-stage placeholder compiled only into the W2–W4 fixture owner variant.
// The adopted owner has no arm yet: nothing is installed and nothing is observed.
// The implementation replaces this file with real definitions in the owner TU.
#include "../../../server/src/sqlite_clock_checkpoint_fixture.hpp"

namespace subathon::server::detail {

StorageError arm_clock_checkpoint_fixture(SqliteWriter &, ClockFixtureArm,
                                          const ApplicationOperationKey &) {
    return StorageError::feature_not_implemented;
}

std::optional<ClockFixtureArmReceipt> take_clock_checkpoint_fixture_receipt(SqliteWriter &) {
    return std::nullopt;
}

} // namespace subathon::server::detail
