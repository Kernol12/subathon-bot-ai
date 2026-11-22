// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_clock_checkpoint.hpp"

namespace subathon::server::detail {

std::variant<MigrationManifest, MigrationReason> clock_checkpoint_manifest() {
    return MigrationReason::feature_not_implemented;
}

ClockPreOpenResult probe_clock_checkpoint_predecessor(const QString &, StorageLimits) {
    return {ClockPreOpenResult::Disposition::unavailable, MigrationReason::feature_not_implemented,
            std::nullopt, std::nullopt, std::nullopt};
}

#ifdef SUBATHON_STORAGE_003_CONSTRAINT_TEST_ACCESS
std::optional<ClockFixtureWriterTrace> clock_checkpoint_fixture_trace(const SqliteWriter &) {
    return std::nullopt;
}
#endif

MigrationAdmission enqueue_clock_checkpoint_advance(SqliteWriter &, MigrationRequest,
                                                    const ClockPreOpenWitness &,
                                                    ClockAdvanceCompletion) {
    return {StorageError::feature_not_implemented, MigrationReason::feature_not_implemented};
}

std::variant<SqliteBundleInitialDisposition, WorkError>
stage_clock_checkpoint_initial(const SqliteWorkSession &, const RetainedDocument &,
                               const ApplicationStateSnapshot &, const ClockInitialCheckpoint &) {
    return WorkError::feature_not_implemented;
}

} // namespace subathon::server::detail
