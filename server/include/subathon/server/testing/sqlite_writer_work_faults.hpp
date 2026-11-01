// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/sqlite_writer.hpp>

namespace subathon::server::testing {
enum class WorkControlFault { lose_next_commit_ack, lose_next_rollback_ack };

// Acceptance-only seam, owner-thread/idle writer only. Installs one pending fault;
// a second installation is invalid_options. Only the targeted real successful SQL
// COMMIT/ROLLBACK in a new work item consumes it, immediately after SQL succeeds
// and before the normal control acknowledgement reaches outcome classification.
// Failed controls, cleanup of inactive sessions and old WriteBatch controls do not
// consume it. The seam must never assign WorkResult or fabricate SQL execution.
// This models lost acknowledgement, not power-loss/disk-failure verification.
class SqliteWriterWorkAccess final {
  public:
    [[nodiscard]] static StorageError inject_next_control_ack_loss(SqliteWriter &writer,
                                                                   WorkControlFault fault);
};
} // namespace subathon::server::testing
