// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/receipt_intake.hpp>

namespace subathon::server {
core::ReceiptIntakeResult ReceiptIntake::ingest(const core::ReceiptInput &) {
    throw ReceiptIntakeNotImplemented{};
}
} // namespace subathon::server
