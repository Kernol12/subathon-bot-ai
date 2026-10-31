// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <memory>
#include <optional>
#include <subathon/server/contribution_evidence.hpp>
#include <utility>
#include <vector>

namespace subathon::tests {
// Primitive disposable storage: no identity deduplication, evidence merge or eligibility policy.
class FakeContributionRepository final : public server::ContributionRepository {
  public:
    enum class Failure {
        none,
        after_create,
        after_observation,
        after_link,
        after_update,
        before_commit
    };
    Failure fail_next{Failure::none};

    [[nodiscard]] const std::vector<server::ContributionRecord> &contributions() const {
        return contributions_;
    }
    [[nodiscard]] const std::vector<server::ContributionEvidenceRecord> &observations() const {
        return observations_;
    }
    // Represents a separately committed application/review/valuation/settlement update.
    void seed_state(core::ContributionId id, const core::ContributionState &state) {
        for (auto &record : contributions_) {
            if (record.id == id) {
                record.state = state;
                return;
            }
        }
        throw server::ContributionStorageError("Cannot seed state for an absent contribution");
    }
    [[nodiscard]] std::unique_ptr<server::ContributionTransaction> begin() override {
        return std::make_unique<Transaction>(*this);
    }

  private:
    class Transaction final : public server::ContributionTransaction {
      public:
        explicit Transaction(FakeContributionRepository &owner)
            : owner_(owner), contributions_(owner.contributions_),
              observations_(owner.observations_), next_id_(owner.next_id_) {}

        [[nodiscard]] std::optional<server::ContributionEvidenceRecord>
        find_receipt(core::ReceiptId id) override {
            const auto found =
                std::find_if(observations_.begin(), observations_.end(),
                             [&](const auto &row) { return row.observation.receipt.id == id; });
            return found == observations_.end()
                       ? std::nullopt
                       : std::optional<server::ContributionEvidenceRecord>{*found};
        }
        [[nodiscard]] std::optional<server::ContributionRecord>
        find_activity(const server::VerifiedActivityIdentity &identity) override {
            const auto found =
                std::find_if(contributions_.begin(), contributions_.end(),
                             [&](const auto &row) { return row.activity_identity == identity; });
            return found == contributions_.end()
                       ? std::nullopt
                       : std::optional<server::ContributionRecord>{*found};
        }
        [[nodiscard]] std::vector<server::ContributionEvidenceRecord>
        unresolved_activity(const server::VerifiedActivityIdentity &identity) override {
            std::vector<server::ContributionEvidenceRecord> found;
            for (const auto &row : observations_) {
                if (!row.contribution_id && row.observation.verified_activity &&
                    row.observation.verified_activity->identity == identity) {
                    found.push_back(row);
                }
            }
            return found;
        }
        [[nodiscard]] core::ContributionId
        create(const server::ContributionReceipt &support,
               const std::optional<server::VerifiedActivityIdentity> &identity,
               const core::ContributionState &state) override {
            const core::ContributionId id{next_id_++};
            contributions_.push_back(
                {id, identity, support.id, support.facts, support.arrival, state});
            fail_if(Failure::after_create);
            return id;
        }
        void append_observation(const server::ContributionEvidenceRecord &observation) override {
            observations_.push_back(observation);
            fail_if(Failure::after_observation);
        }
        void link_observation(core::ReceiptId receipt_id,
                              core::ContributionId contribution_id) override {
            const auto row =
                std::find_if(observations_.begin(), observations_.end(), [&](const auto &value) {
                    return value.observation.receipt.id == receipt_id;
                });
            if (row == observations_.end()) {
                throw server::ContributionStorageError("Cannot link an absent observation");
            }
            row->contribution_id = contribution_id;
            fail_if(Failure::after_link);
        }
        void update_state(core::ContributionId id, const core::ContributionState &state) override {
            const auto row = std::find_if(contributions_.begin(), contributions_.end(),
                                          [&](const auto &value) { return value.id == id; });
            if (row == contributions_.end()) {
                throw server::ContributionStorageError("Cannot update an absent contribution");
            }
            row->state = state;
            fail_if(Failure::after_update);
        }
        void commit() override {
            fail_if(Failure::before_commit);
            owner_.contributions_ = std::move(contributions_);
            owner_.observations_ = std::move(observations_);
            owner_.next_id_ = next_id_;
        }

      private:
        void fail_if(Failure point) {
            if (owner_.fail_next == point) {
                owner_.fail_next = Failure::none;
                throw server::ContributionStorageError(
                    "Injected disposable-store transaction failure");
            }
        }
        FakeContributionRepository &owner_;
        std::vector<server::ContributionRecord> contributions_;
        std::vector<server::ContributionEvidenceRecord> observations_;
        std::uint64_t next_id_;
    };
    std::vector<server::ContributionRecord> contributions_;
    std::vector<server::ContributionEvidenceRecord> observations_;
    std::uint64_t next_id_{1};
};
} // namespace subathon::tests
