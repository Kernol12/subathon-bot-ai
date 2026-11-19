// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_receipts.hpp>

namespace subathon::server {
[[nodiscard]] std::optional<core::ReceiptRecord> read_receipt(const SqliteWorkSession &,
                                                              core::ReceiptId);
}
