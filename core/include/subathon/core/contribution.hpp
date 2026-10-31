// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <subathon/core/receipt.hpp>

namespace subathon::core {
struct ContributionId {
    std::uint64_t value;
    bool operator==(const ContributionId &) const = default;
};

enum class PaymentEvidence { confirmed, unconfirmed, unknown, not_applicable };
enum class ContributionReview { not_required, pending, accepted, rejected };
enum class ContributionApplication { not_applied, applied };
enum class ContributionReversal { none, partial, full };
enum class SettlementGate { clear, unresolved };

enum class EvidencePolicy { accept_provider_report, require_evidence_review, record_only };
enum class EvidenceReviewChoice { accept_labelled_report, await_confirmation };

struct EvidenceReviewDecision {
    EvidenceReviewChoice choice;
    std::string decision_ref;
    std::string actor_ref;
    bool operator==(const EvidenceReviewDecision &) const = default;
};

struct EvidenceAdmission {
    EvidencePolicy policy;
    std::string policy_ref;
    std::optional<EvidenceReviewDecision> review_decision;
    bool operator==(const EvidenceAdmission &) const = default;
};

// Monetary status is explicit: absence of source money never means a non-money input.
struct ContributionValuation {
    bool monetary;
    std::optional<ReceiptMoney> original_money;
    std::optional<std::string> resolution_ref;
    bool missing_rate;
    bool operator==(const ContributionValuation &) const = default;
};

// These are independent dimensions, not a single accepted/eligible status.
struct ContributionConditions {
    ContributionValuation valuation;
    ContributionReview review;
    EvidenceAdmission evidence_admission;
    ContributionApplication application;
    ContributionReversal reversal;
    SettlementGate settlement;
    bool operator==(const ContributionConditions &) const = default;
};

struct ContributionState {
    PaymentEvidence payment_evidence;
    ContributionConditions conditions;
    bool operator==(const ContributionState &) const = default;
};

enum class ContributionBlocker {
    payment_evidence_required,
    evidence_review_required,
    record_only_policy,
    missing_source_amount,
    missing_rate,
    valuation_unresolved,
    review_pending,
    review_rejected,
    already_applied,
    settlement_review_required
};

struct ContributionEligibility {
    // Empty permits only further planning within this projection; it cannot authorize effects.
    std::set<ContributionBlocker> blockers;
};

class ContributionEligibilityNotImplemented final : public std::logic_error {
  public:
    ContributionEligibilityNotImplemented()
        : std::logic_error(
              "Contribution eligibility is intentionally unimplemented; review tests first") {}
};

// Revision 1: docs/contracts/contribution-eligibility-v1.md. No state mutation or effects.
[[nodiscard]] ContributionEligibility
assess_contribution_eligibility(const ContributionState &state);
} // namespace subathon::core
