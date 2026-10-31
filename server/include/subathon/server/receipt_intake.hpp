// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <memory>
#include <optional>
#include <stdexcept>
#include <subathon/core/receipt.hpp>

namespace subathon::server {
class ReceiptStorageError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

// One writer-owned transaction. Destruction without successful commit rolls back.
// Known uncommitted failures throw ReceiptStorageError without exposing partial writes.
// Indeterminate durable commit outcomes need adapter resolution; see STORAGE-002.
class ReceiptTransaction {
  public:
    virtual ~ReceiptTransaction() = default;
    [[nodiscard]] virtual std::optional<core::ReceiptRecord>
    find(const core::ReceiptIdentity &identity) = 0;
    [[nodiscard]] virtual core::ReceiptId create(const core::ReceiptIdentity &identity,
                                                 const core::ReceiptFacts &facts,
                                                 const core::ReceiptArrival &arrival) = 0;
    virtual void append_attempt(core::ReceiptId id, const core::ReceiptAttempt &attempt) = 0;
    virtual void commit() = 0;
};

class ReceiptRepository {
  public:
    virtual ~ReceiptRepository() = default;
    [[nodiscard]] virtual std::unique_ptr<ReceiptTransaction> begin() = 0;
};

class ReceiptIntakeNotImplemented final : public std::logic_error {
  public:
    ReceiptIntakeNotImplemented()
        : std::logic_error("Receipt intake is intentionally unimplemented; review tests first") {}
};

// Called by the authoritative writer with owned, already-validated normalized values.
// Revision 1: docs/contracts/receipt-intake-v1.md. Returns only after atomic commit.
class ReceiptIntake final {
  public:
    explicit ReceiptIntake(ReceiptRepository &repository) : repository_(repository) {}
    [[nodiscard]] core::ReceiptIntakeResult ingest(const core::ReceiptInput &input);

  private:
    ReceiptRepository &repository_;
};
} // namespace subathon::server
