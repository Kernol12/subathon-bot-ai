// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_receipts.hpp>

namespace subathon::server {
// Read the immutable receipt and its creating attempt on a borrowed writer
// session. Stored-data errors may settle the active unit through writer cleanup.
[[nodiscard]] std::optional<core::ReceiptRecord> read_receipt(const SqliteWorkSession &,
                                                              core::ReceiptId);
struct ReceiptObservation {
    core::ReceiptRecord original;
    core::ReceiptAttempt attempt;
};

[[nodiscard]] std::optional<ReceiptObservation>
read_receipt_observation(const SqliteWorkSession &, core::ReceiptId, qint64 ordinal);

enum class ManagedReceiptStatus { committed, ordinal_limit, rolled_back };
struct ManagedReceiptWrite {
    ManagedReceiptStatus status;
    std::optional<core::ReceiptId> receipt{};
    qint64 ordinal{};
};

// Extension runs inside the adopted receipt transaction. True authorizes its
// commit; false lets the owning wrapper settle the transaction by rollback.
using ReceiptPreparation = std::function<bool(const SqliteWorkSession &)>;
using ReceiptExtension = std::function<bool(const SqliteWorkSession &, core::ReceiptId, qint64,
                                            core::ReceiptDisposition)>;
[[nodiscard]] ManagedReceiptWrite write_managed_receipt(const SqliteWorkSession &,
                                                        const core::ReceiptInput &,
                                                        const ReceiptPreparation &,
                                                        const ReceiptExtension &);

} // namespace subathon::server
