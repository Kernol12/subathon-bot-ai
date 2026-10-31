// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <memory>
#include <optional>
#include <subathon/server/receipt_intake.hpp>
#include <utility>
#include <vector>

namespace subathon::tests {
// A disposable primitive store. It never classifies retries/conflicts or deduplicates.
// No SQL uniqueness, disk durability, thread ownership or concurrency is simulated.
class FakeReceiptRepository final : public server::ReceiptRepository {
  public:
    enum class Failure { none, after_create, after_append, before_commit };
    Failure fail_next{Failure::none};

    [[nodiscard]] const std::vector<core::ReceiptRecord> &records() const { return records_; }
    [[nodiscard]] std::unique_ptr<server::ReceiptTransaction> begin() override {
        return std::make_unique<Transaction>(*this);
    }

  private:
    class Transaction final : public server::ReceiptTransaction {
      public:
        explicit Transaction(FakeReceiptRepository &owner)
            : owner_(owner), staged_(owner.records_), next_id_(owner.next_id_) {}

        [[nodiscard]] std::optional<core::ReceiptRecord>
        find(const core::ReceiptIdentity &identity) override {
            const auto record = std::find_if(staged_.begin(), staged_.end(), [&](const auto &row) {
                return row.identity == identity;
            });
            if (record == staged_.end()) {
                return std::nullopt;
            }
            return *record;
        }

        [[nodiscard]] core::ReceiptId create(const core::ReceiptIdentity &identity,
                                             const core::ReceiptFacts &facts,
                                             const core::ReceiptArrival &arrival) override {
            const core::ReceiptId id{next_id_++};
            staged_.push_back({id, identity, facts, arrival, {}});
            fail_if(Failure::after_create);
            return id;
        }

        void append_attempt(core::ReceiptId id, const core::ReceiptAttempt &attempt) override {
            const auto record = std::find_if(staged_.begin(), staged_.end(),
                                             [&](const auto &row) { return row.id == id; });
            if (record == staged_.end()) {
                throw server::ReceiptStorageError("Fake append references an absent receipt");
            }
            record->attempts.push_back(attempt);
            fail_if(Failure::after_append);
        }

        void commit() override {
            fail_if(Failure::before_commit);
            owner_.records_ = std::move(staged_);
            owner_.next_id_ = next_id_;
        }

      private:
        void fail_if(Failure point) {
            if (owner_.fail_next == point) {
                owner_.fail_next = Failure::none;
                throw server::ReceiptStorageError("Injected disposable-store transaction failure");
            }
        }
        FakeReceiptRepository &owner_;
        std::vector<core::ReceiptRecord> staged_;
        std::uint64_t next_id_;
    };

    std::vector<core::ReceiptRecord> records_;
    std::uint64_t next_id_{1};
};
} // namespace subathon::tests
