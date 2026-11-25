// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_v23_rebuild.hpp"

#include "sqlite_v22_catalog.hpp"

#include <QThread>

namespace subathon::server::detail {

MigrationAdmission enqueue_v23_rebuild(V22PreOpenWitness witness, Ops02UpdateHold hold,
                                       V23StoppedWriterLease lease, V23RebuildBudget budget,
                                       V23Cancellation,
                                       SqliteMigrationRunner::Completion completion) {
    return V23RebuildAccess::enqueue(std::move(witness), std::move(hold), std::move(lease), budget,
                                     std::move(completion));
}

MigrationAdmission V23RebuildAccess::enqueue(V22PreOpenWitness witness, Ops02UpdateHold hold,
                                             V23StoppedWriterLease lease, V23RebuildBudget budget,
                                             SqliteMigrationRunner::Completion completion) {
    const MigrationAdmission invalid{StorageError::invalid_options,
                                     MigrationReason::invalid_manifest};
    const auto &identity = sqlite_v22_catalog();
    // Check the owner thread before locking the generation so a foreign call
    // cannot keep the marker alive while its writer is being destroyed.
    if (!lease.reservation_ || QThread::currentThread() != lease.owner_)
        return invalid;
    const auto generation = lease.generation_.lock();
    if (!hold.fixture_hold_ || !generation || generation->writer->thread() != generation->owner ||
        witness.version != static_cast<qint32>(identity.steps.size()) ||
        witness.projection_objects != identity.projection_objects || witness.prefix.size() != 32 ||
        witness.prefix.toHex() !=
            QByteArrayView(identity.prefix_sha256_hex.data(), identity.prefix_sha256_hex.size()))
        return invalid;
    if (!completion)
        return {StorageError::invalid_work, MigrationReason::none};
    if (budget.disk_bytes.value_or(v23_fixture_disk_bytes) > v23_fixture_disk_bytes ||
        budget.memory_bytes.value_or(v23_fixture_memory_bytes) > v23_fixture_memory_bytes)
        return invalid;

    return {StorageError::feature_not_implemented, MigrationReason::feature_not_implemented};
}

} // namespace subathon::server::detail
