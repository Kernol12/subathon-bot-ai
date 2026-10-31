// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/contribution.hpp>

namespace subathon::core {
ContributionEligibility assess_contribution_eligibility(const ContributionState &state) {
    ContributionEligibility result;
    auto &blockers = result.blockers;
    const auto &conditions = state.conditions;
    const auto &admission = conditions.evidence_admission;

    if (admission.policy == EvidencePolicy::record_only) {
        blockers.insert(ContributionBlocker::record_only_policy);
    } else if (admission.policy == EvidencePolicy::require_evidence_review &&
               (state.payment_evidence == PaymentEvidence::unknown ||
                state.payment_evidence == PaymentEvidence::unconfirmed)) {
        if (!admission.review_decision) {
            blockers.insert(ContributionBlocker::evidence_review_required);
        } else if (admission.review_decision->choice == EvidenceReviewChoice::await_confirmation) {
            blockers.insert(ContributionBlocker::payment_evidence_required);
        }
    }

    const auto &valuation = conditions.valuation;
    if (valuation.monetary) {
        if (!valuation.original_money) {
            blockers.insert(ContributionBlocker::missing_source_amount);
        }
        if (valuation.missing_rate) {
            blockers.insert(ContributionBlocker::missing_rate);
        } else if (!valuation.resolution_ref) {
            blockers.insert(ContributionBlocker::valuation_unresolved);
        }
    }

    if (conditions.review == ContributionReview::pending) {
        blockers.insert(ContributionBlocker::review_pending);
    } else if (conditions.review == ContributionReview::rejected) {
        blockers.insert(ContributionBlocker::review_rejected);
    }
    if (conditions.application == ContributionApplication::applied) {
        blockers.insert(ContributionBlocker::already_applied);
    }
    if (conditions.settlement == SettlementGate::unresolved ||
        (conditions.application == ContributionApplication::not_applied &&
         conditions.reversal != ContributionReversal::none)) {
        blockers.insert(ContributionBlocker::settlement_review_required);
    }
    return result;
}
} // namespace subathon::core
