// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/receipt_intake.hpp>

namespace subathon::server {
core::ReceiptIntakeResult ReceiptIntake::ingest(const core::ReceiptInput &input) {
    auto transaction = repository_.begin();
    const auto original = transaction->find(input.identity);
    const auto receipt_id =
        original ? original->id : transaction->create(input.identity, input.facts, input.arrival);
    auto disposition = core::ReceiptDisposition::created;
    if (original) {
        disposition = original->original_facts == input.facts ? core::ReceiptDisposition::retry
                                                              : core::ReceiptDisposition::conflict;
    }

    transaction->append_attempt(receipt_id, {input.arrival, input.facts, disposition});
    transaction->commit();
    return {receipt_id, disposition};
}
} // namespace subathon::server
