// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_application_bundle.hpp>
#include <subathon/server/sqlite_writer.hpp>

#include <optional>

namespace subathon::server::detail {

// Acceptance-only #834 W2–W4 arms of the adopted checkpoint owner. Only the
// private W2–W4 fixture owner variant defines these symbols; no ordinary
// object declares or links them. Each arm is a closed one-shot capability
// for one idle writer and the exact operation key it names. The adopted owner
// consumes it at its real named stage inside that operation's own work unit.
// There is no SQL, callback, connection or simulated outcome input.
enum class ClockFixtureArm {
    // Stage the #312 closure, head CAS and root append, but no slot CAS.
    omit_slot_follow,
    // After both checked CAS witnesses, execute the identical root INSERT again.
    repeat_root_append
};
enum class ClockFixtureFailureSite { commit, repeated_root };

// One real same-unit statement result and the value bytes the writer metered.
struct ClockFixtureWitness {
    SqlRows rows;
    qsizetype value_bytes{0};
};

// Detached receipt of the consumed arm. It is available once after the work
// completes, including after an unknown outcome revoked the writer.
struct ClockFixtureArmReceipt {
    ClockFixtureArm consumed{ClockFixtureArm::omit_slot_follow};
    ApplicationOperationKey operation;
    ClockFixtureWitness head_changes;
    // Absent when the slot follow-up was omitted.
    std::optional<ClockFixtureWitness> slot_changes;
    // The joined head/slot/root read after the omitted follow-up.
    std::optional<ClockFixtureWitness> joined;
    ClockFixtureFailureSite failure_site{ClockFixtureFailureSite::commit};
    // The mapped storage error of the failed COMMIT control or root statement.
    StorageError failure{StorageError::none};
    bool rollback_acknowledged{false};
};

// Installs the arm on an idle, open writer. Any second installation, busy
// writer or unknown arm is invalid_options.
[[nodiscard]] StorageError arm_clock_checkpoint_fixture(SqliteWriter &writer, ClockFixtureArm arm,
                                                        const ApplicationOperationKey &operation);
[[nodiscard]] std::optional<ClockFixtureArmReceipt>
take_clock_checkpoint_fixture_receipt(SqliteWriter &writer);

} // namespace subathon::server::detail
