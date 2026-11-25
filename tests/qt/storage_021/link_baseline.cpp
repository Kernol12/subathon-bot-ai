// SPDX-License-Identifier: GPL-3.0-or-later
#include "fake_ops02_update_hold.hpp"

namespace subathon::server::detail::testing {

// Force the link to resolve the witness, reservation, test issuer and inert entry.
// Stage 1 fixtures will invoke these seams after source review.
MigrationAdmission link_v23_baseline(SqliteWriter &writer, const QString &path,
                                     SqliteMigrationRunner::Completion done) {
    auto inspection = inspect_v22_pre_open(path, {});
    if (const auto *reason = std::get_if<MigrationReason>(&inspection))
        return {StorageError::invalid_options, *reason};
    return enqueue_v23_rebuild(std::get<V22PreOpenWitness>(std::move(inspection)),
                               FakeOps02UpdateHoldIssuer::issue(),
                               V23RebuildAccess::acquire(writer), {}, {}, std::move(done));
}

} // namespace subathon::server::detail::testing

int main() { return 0; }
