// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_receipts.hpp>

namespace subathon::server {
// Read the immutable receipt and its creating attempt on a borrowed writer
// session. Stored-data errors may settle the active unit through writer cleanup.
[[nodiscard]] std::optional<core::ReceiptRecord> read_receipt(const SqliteWorkSession &,
                                                              core::ReceiptId);
} // namespace subathon::server
