// SPDX-License-Identifier: GPL-3.0-or-later
// Specification v0.45 §§3.2–4,5,14,18; contribution-eligibility-v1.md revision 1.
// Synthetic verified-key inputs are caller evidence, not live provider capability claims.
#include "../fakes/contribution_repository.hpp"
#include <array>
#include <functional>
#include <gtest/gtest.h>
#include <set>
#include <string>
#include <subathon/core/contribution.hpp>
#include <subathon/server/contribution_evidence.hpp>
#include <utility>

namespace {
using namespace subathon::core;
using namespace subathon::server;
using subathon::tests::FakeContributionRepository;

ContributionState ready_state() {
    return {PaymentEvidence::confirmed,
            {{true, ReceiptMoney{"20.0000", "EUR"}, "valuation-r4", false},
             ContributionReview::not_required,
             {EvidencePolicy::require_evidence_review, "evidence-policy-r2", {}},
             ContributionApplication::not_applied,
             ContributionReversal::none,
             SettlementGate::clear}};
}

ContributionObservation support_observation() {
    auto state = ready_state();
    state.conditions.valuation.resolution_ref.reset();
    state.conditions.valuation.missing_rate = true;
    state.conditions.review = ContributionReview::pending;
    state.conditions.evidence_admission.review_decision = EvidenceReviewDecision{
        EvidenceReviewChoice::await_confirmation, "evidence-decision-r3", "actor-owner"};
    return {{{101},
             {{"synthetic.provider-a", "source-7"}, "synthetic.event.v1", "received-42"},
             {"synthetic.payment.received",
              1,
              1'700'000'000'000,
              {},
              "Synthetic Supporter",
              ReceiptMoney{"20.0000", "EUR"},
              ReceiptLinkReference{"synthetic.payment.v1", "payment-9"},
              {}},
             {1'700'000'000'100, "connection-1", "configuration-r5", "modifiers-r8"}},
            ContributionObservationKind::support,
            PaymentEvidence::unknown,
            VerifiedActivityLink{
                {{"synthetic.provider-a", "source-7"}, "synthetic.activity.v1", "activity-9"},
                "synthetic-capability-proof-r1"},
            state.conditions};
}

ContributionObservation confirmation_observation() {
    auto observation = support_observation();
    observation.receipt.id = ReceiptId{102};
    observation.receipt.identity.opaque_id = "confirmed-43";
    observation.receipt.facts.event_type = "synthetic.payment.confirmed";
    observation.receipt.facts.source_timestamp_utc_ms = 1'700'000'001'000;
    observation.receipt.arrival = {1'700'000'001'100, "connection-2", "configuration-r6",
                                   "modifiers-r9"};
    observation.kind = ContributionObservationKind::confirmation;
    observation.evidence = PaymentEvidence::confirmed;
    observation.support_conditions.reset();
    return observation;
}

// §§3.3,5: ordinary acceptance resolves only its independent review dimension.
TEST(ContributionEligibilityAcceptance, MultipleHoldsRemainAfterPausedInputAcceptance) {
    auto state = ready_state();
    state.payment_evidence = PaymentEvidence::unknown;
    state.conditions.valuation.resolution_ref.reset();
    state.conditions.valuation.missing_rate = true;
    state.conditions.review = ContributionReview::pending;
    state.conditions.evidence_admission.review_decision = EvidenceReviewDecision{
        EvidenceReviewChoice::await_confirmation, "decision-r3", "actor-owner"};
    EXPECT_EQ(assess_contribution_eligibility(state).blockers,
              (std::set{ContributionBlocker::payment_evidence_required,
                        ContributionBlocker::missing_rate, ContributionBlocker::review_pending}));
    state.conditions.review = ContributionReview::accepted;
    const auto before = state;
    EXPECT_EQ(assess_contribution_eligibility(state).blockers,
              (std::set{ContributionBlocker::payment_evidence_required,
                        ContributionBlocker::missing_rate}));
    EXPECT_EQ(state, before);
    EXPECT_EQ(state.payment_evidence, PaymentEvidence::unknown);
    EXPECT_FALSE(state.conditions.valuation.resolution_ref);
}

TEST(ContributionEligibilityAcceptance, RecordedValuationResolutionDoesNotResolveEvidenceOrReview) {
    auto state = ready_state();
    state.payment_evidence = PaymentEvidence::unconfirmed;
    state.conditions.review = ContributionReview::rejected;
    state.conditions.valuation.resolution_ref = "explicit-later-manual-valuation-r9";
    state.conditions.evidence_admission.review_decision = EvidenceReviewDecision{
        EvidenceReviewChoice::await_confirmation, "decision-r3", "actor-owner"};
    const auto before = state;
    EXPECT_EQ(assess_contribution_eligibility(state).blockers,
              (std::set{ContributionBlocker::payment_evidence_required,
                        ContributionBlocker::review_rejected}));
    EXPECT_EQ(state, before);
    state.conditions.valuation.resolution_ref.reset();
    EXPECT_EQ(assess_contribution_eligibility(state).blockers,
              (std::set{ContributionBlocker::payment_evidence_required,
                        ContributionBlocker::valuation_unresolved,
                        ContributionBlocker::review_rejected}));
}

// §14 permits progression from a provider report without relabeling it as confirmed.
TEST(ContributionEligibilityAcceptance, ProviderReportPolicyPreservesUnknownAndUnconfirmedLabels) {
    for (const auto evidence : {PaymentEvidence::unknown, PaymentEvidence::unconfirmed}) {
        auto state = ready_state();
        state.payment_evidence = evidence;
        state.conditions.evidence_admission = {
            EvidencePolicy::accept_provider_report, "policy-report-r2", {}};
        const auto before = state;
        EXPECT_TRUE(assess_contribution_eligibility(state).blockers.empty());
        EXPECT_EQ(state, before);
        EXPECT_EQ(state.payment_evidence, evidence);
    }
}

TEST(ContributionEligibilityAcceptance, EvidenceReviewDecisionIsSeparateFromOrdinaryAcceptance) {
    auto state = ready_state();
    state.payment_evidence = PaymentEvidence::unknown;
    state.conditions.review = ContributionReview::accepted;
    EXPECT_EQ(assess_contribution_eligibility(state).blockers,
              (std::set{ContributionBlocker::evidence_review_required}));
    state.conditions.evidence_admission.review_decision = EvidenceReviewDecision{
        EvidenceReviewChoice::await_confirmation, "evidence-wait-r4", "actor-owner"};
    EXPECT_EQ(assess_contribution_eligibility(state).blockers,
              (std::set{ContributionBlocker::payment_evidence_required}));
    state.conditions.evidence_admission.review_decision = EvidenceReviewDecision{
        EvidenceReviewChoice::accept_labelled_report, "evidence-accept-r5", "actor-owner"};
    const auto before = state;
    EXPECT_TRUE(assess_contribution_eligibility(state).blockers.empty());
    EXPECT_EQ(state, before);
    EXPECT_EQ(state.payment_evidence, PaymentEvidence::unknown);
}

TEST(ContributionEligibilityAcceptance, CapturedRecordOnlyPolicyStillBlocksConfirmedAcceptedInput) {
    auto state = ready_state();
    state.conditions.review = ContributionReview::accepted;
    state.conditions.evidence_admission = {EvidencePolicy::record_only, "policy-record-r2", {}};
    const auto before = state;
    EXPECT_EQ(assess_contribution_eligibility(state).blockers,
              (std::set{ContributionBlocker::record_only_policy}));
    EXPECT_EQ(state, before);
}

TEST(ContributionEligibilityAcceptance,
     HiddenSourceAmountCannotBecomeZeroOrReadyThroughAReference) {
    auto state = ready_state();
    state.conditions.valuation.original_money.reset();
    state.conditions.valuation.missing_rate = true;
    state.conditions.valuation.resolution_ref = "reference-does-not-supply-missing-source-money";
    const auto before = state;
    EXPECT_EQ(
        assess_contribution_eligibility(state).blockers,
        (std::set{ContributionBlocker::missing_source_amount, ContributionBlocker::missing_rate}));
    EXPECT_EQ(state, before);
    EXPECT_FALSE(state.conditions.valuation.original_money);
}

TEST(ContributionEligibilityAcceptance, NonMonetaryNotApplicableEvidenceDoesNotBypassReview) {
    auto state = ready_state();
    state.payment_evidence = PaymentEvidence::not_applicable;
    state.conditions.valuation = {false, {}, {}, false};
    state.conditions.review = ContributionReview::pending;
    EXPECT_EQ(assess_contribution_eligibility(state).blockers,
              (std::set{ContributionBlocker::review_pending}));
    state.conditions.review = ContributionReview::accepted;
    EXPECT_TRUE(assess_contribution_eligibility(state).blockers.empty());
    EXPECT_EQ(state.payment_evidence, PaymentEvidence::not_applicable);
}

// ACCT-004 applies effects; ACCT-006 resolves refund-before-apply/settlement policy.
TEST(ContributionEligibilityAcceptance,
     ApplicationReversalAndSettlementCannotSilentlyReopenPlanning) {
    auto state = ready_state();
    state.conditions.application = ContributionApplication::applied;
    for (const auto reversal :
         {ContributionReversal::none, ContributionReversal::partial, ContributionReversal::full}) {
        state.conditions.reversal = reversal;
        const auto before = state;
        EXPECT_EQ(assess_contribution_eligibility(state).blockers,
                  (std::set{ContributionBlocker::already_applied}));
        EXPECT_EQ(state, before);
    }
    state.conditions.application = ContributionApplication::not_applied;
    for (const auto reversal : {ContributionReversal::partial, ContributionReversal::full}) {
        state.conditions.reversal = reversal;
        EXPECT_EQ(assess_contribution_eligibility(state).blockers,
                  (std::set{ContributionBlocker::settlement_review_required}));
    }
    state.conditions.reversal = ContributionReversal::none;
    state.conditions.settlement = SettlementGate::unresolved;
    EXPECT_EQ(assess_contribution_eligibility(state).blockers,
              (std::set{ContributionBlocker::settlement_review_required}));
}

class ContributionEvidenceAcceptance : public ::testing::Test {
  protected:
    FakeContributionRepository repository;
    ContributionEvidenceService service{repository};
};

TEST_F(ContributionEvidenceAcceptance,
       FirstSupportOwnsItsOriginalFactsConditionsAndDecisionReferences) {
    auto input = support_observation();
    const auto result = service.observe(input);
    input.receipt.facts.money->original_decimal = "999";
    input.receipt.arrival.configuration_ref = "caller-reused-buffer";
    input.support_conditions->evidence_admission.policy_ref = "caller-reused-policy";

    EXPECT_EQ(result.disposition, ContributionLinkDisposition::created);
    ASSERT_TRUE(result.contribution_id);
    ASSERT_EQ(repository.contributions().size(), 1U);
    const auto &record = repository.contributions().front();
    EXPECT_EQ(record.id, *result.contribution_id);
    EXPECT_EQ(record.original_receipt_id, ReceiptId{101});
    EXPECT_EQ(record.original_facts.money, (ReceiptMoney{"20.0000", "EUR"}));
    EXPECT_EQ(record.original_arrival.server_timestamp_utc_ms, 1'700'000'000'100);
    EXPECT_EQ(record.original_arrival.configuration_ref, "configuration-r5");
    EXPECT_EQ(record.original_arrival.modifier_context_ref, "modifiers-r8");
    EXPECT_EQ(record.state.payment_evidence, PaymentEvidence::unknown);
    EXPECT_EQ(record.state.conditions.evidence_admission.policy_ref, "evidence-policy-r2");
    ASSERT_TRUE(record.state.conditions.evidence_admission.review_decision);
    EXPECT_EQ(record.state.conditions.evidence_admission.review_decision->decision_ref,
              "evidence-decision-r3");
    EXPECT_EQ(record.state.conditions.evidence_admission.review_decision->actor_ref, "actor-owner");
    ASSERT_EQ(repository.observations().size(), 1U);
    EXPECT_EQ(repository.observations()[0].contribution_id, result.contribution_id);
    ASSERT_TRUE(repository.observations()[0].observation.verified_activity);
    EXPECT_EQ(repository.observations()[0].observation.verified_activity->verification_ref,
              "synthetic-capability-proof-r1");
}

TEST_F(ContributionEvidenceAcceptance,
       LaterConfirmationLinksOriginalWithoutChangingOtherDimensions) {
    const auto support = support_observation();
    const auto first = service.observe(support);
    const auto confirmed = service.observe(confirmation_observation());
    EXPECT_EQ(confirmed.disposition, ContributionLinkDisposition::linked);
    EXPECT_EQ(confirmed.contribution_id, first.contribution_id);
    ASSERT_EQ(repository.contributions().size(), 1U);
    const auto &record = repository.contributions().front();
    EXPECT_EQ(record.state.payment_evidence, PaymentEvidence::confirmed);
    EXPECT_EQ(record.state.conditions, *support.support_conditions);
    EXPECT_EQ(record.original_receipt_id, ReceiptId{101});
    EXPECT_EQ(record.original_facts, support.receipt.facts);
    EXPECT_EQ(record.original_arrival, support.receipt.arrival);
    EXPECT_EQ(assess_contribution_eligibility(record.state).blockers,
              (std::set{ContributionBlocker::missing_rate, ContributionBlocker::review_pending}));
    ASSERT_EQ(repository.observations().size(), 2U);
    EXPECT_EQ(repository.observations()[1].contribution_id, first.contribution_id);
    EXPECT_EQ(repository.observations()[1].observation.receipt.arrival.connection_ref,
              "connection-2");
}

TEST_F(ContributionEvidenceAcceptance,
       RepeatedReceiptsAreIdempotentAndDistinctConfirmationsRemainAudited) {
    const auto support = support_observation();
    const auto first = service.observe(support);
    EXPECT_EQ(service.observe(support).disposition, ContributionLinkDisposition::duplicate);
    auto confirmation = confirmation_observation();
    EXPECT_EQ(service.observe(confirmation).contribution_id, first.contribution_id);
    ContributionEvidenceService another_service{repository};
    const auto duplicate = another_service.observe(confirmation);
    EXPECT_EQ(duplicate.disposition, ContributionLinkDisposition::duplicate);
    EXPECT_EQ(duplicate.contribution_id, first.contribution_id);
    ASSERT_EQ(repository.observations().size(), 2U);
    confirmation.receipt.id = ReceiptId{103};
    confirmation.receipt.identity.opaque_id = "confirmed-44";
    confirmation.verified_activity->verification_ref = "synthetic-capability-proof-r2";
    const auto repeated = service.observe(confirmation);
    EXPECT_EQ(repeated.disposition, ContributionLinkDisposition::linked);
    EXPECT_EQ(repeated.contribution_id, first.contribution_id);
    ASSERT_EQ(repository.contributions().size(), 1U);
    ASSERT_EQ(repository.observations().size(), 3U);
    EXPECT_EQ(repository.observations()[2].observation.receipt.id, ReceiptId{103});
    EXPECT_EQ(repository.observations()[2].observation.verified_activity->verification_ref,
              "synthetic-capability-proof-r2");
}

TEST_F(ContributionEvidenceAcceptance, ConfirmationBeforeSupportRemainsUnresolvedThenLinksOnce) {
    const auto confirmation = confirmation_observation();
    const auto pending = service.observe(confirmation);
    EXPECT_EQ(pending.disposition, ContributionLinkDisposition::unresolved);
    EXPECT_FALSE(pending.contribution_id);
    EXPECT_TRUE(repository.contributions().empty());
    ASSERT_EQ(repository.observations().size(), 1U);
    EXPECT_FALSE(repository.observations()[0].contribution_id);
    auto support = support_observation();
    support.receipt.arrival.server_timestamp_utc_ms = 1'700'000'005'000;
    const auto original = service.observe(support);

    EXPECT_EQ(original.disposition, ContributionLinkDisposition::created);
    ASSERT_TRUE(original.contribution_id);
    ASSERT_EQ(repository.contributions().size(), 1U);
    const auto &record = repository.contributions().front();
    EXPECT_EQ(record.state.payment_evidence, PaymentEvidence::confirmed);
    EXPECT_EQ(record.original_arrival, support.receipt.arrival);
    EXPECT_EQ(record.original_receipt_id, ReceiptId{101});
    EXPECT_EQ(record.state.conditions, *support.support_conditions);
    ASSERT_EQ(repository.observations().size(), 2U);
    EXPECT_EQ(repository.observations()[0].contribution_id, original.contribution_id);
    EXPECT_EQ(repository.observations()[0].observation, confirmation);
    EXPECT_EQ(service.observe(confirmation).contribution_id, original.contribution_id);
    EXPECT_EQ(repository.contributions().size(), 1U);
    EXPECT_EQ(repository.observations().size(), 2U);
}

TEST_F(ContributionEvidenceAcceptance,
       DelayedUnconfirmedSupportCannotEraseConfirmationOrFirstContext) {
    const auto support = support_observation();
    const auto first = service.observe(support);
    static_cast<void>(service.observe(confirmation_observation()));
    auto delayed = support;
    delayed.receipt.id = ReceiptId{104};
    delayed.receipt.identity.opaque_id = "delayed-support-45";
    delayed.receipt.arrival = {1'700'000'006'000, "connection-3", "configuration-r7",
                               "modifiers-r10"};
    delayed.evidence = PaymentEvidence::unconfirmed;
    const auto linked = service.observe(delayed);
    EXPECT_EQ(linked.disposition, ContributionLinkDisposition::linked);
    EXPECT_EQ(linked.contribution_id, first.contribution_id);
    ASSERT_EQ(repository.contributions().size(), 1U);
    const auto &record = repository.contributions().front();
    EXPECT_EQ(record.state.payment_evidence, PaymentEvidence::confirmed);
    EXPECT_EQ(record.state.conditions, *support.support_conditions);
    EXPECT_EQ(record.original_facts, support.receipt.facts);
    EXPECT_EQ(record.original_arrival, support.receipt.arrival);
    ASSERT_EQ(repository.observations().size(), 3U);
    EXPECT_EQ(repository.observations()[2].observation.evidence, PaymentEvidence::unconfirmed);
}

TEST_F(ContributionEvidenceAcceptance,
       RawPaymentReferenceAndLookalikeFactsCannotReplaceVerifiedCorrelation) {
    auto support = support_observation();
    support.verified_activity.reset();
    const auto first = service.observe(support);
    ASSERT_TRUE(first.contribution_id);
    auto confirmation = confirmation_observation();
    confirmation.verified_activity.reset();
    confirmation.receipt.facts = support.receipt.facts;
    confirmation.receipt.arrival = support.receipt.arrival;
    const auto unresolved = service.observe(confirmation);
    EXPECT_EQ(unresolved.disposition, ContributionLinkDisposition::unresolved);
    EXPECT_FALSE(unresolved.contribution_id);
    ASSERT_EQ(repository.contributions().size(), 1U);
    EXPECT_EQ(repository.contributions()[0].id, *first.contribution_id);
    EXPECT_EQ(repository.contributions()[0].state.payment_evidence, PaymentEvidence::unknown);
    ASSERT_EQ(repository.observations().size(), 2U);
    EXPECT_FALSE(repository.observations()[1].contribution_id);
}

TEST_F(ContributionEvidenceAcceptance,
       VerifiedActivityIdentityRetainsProviderAccountNamespaceAndOpaqueScope) {
    using Mutation = std::pair<const char *, std::function<void(ContributionObservation &)>>;
    const std::array<Mutation, 4> mutations{{
        {"provider",
         [](auto &input) {
             input.receipt.identity.origin.provider = "synthetic.provider-b";
             input.verified_activity->identity.origin.provider = "synthetic.provider-b";
         }},
        {"account",
         [](auto &input) {
             input.receipt.identity.origin.source_account = "source-8";
             input.verified_activity->identity.origin.source_account = "source-8";
         }},
        {"namespace",
         [](auto &input) {
             input.verified_activity->identity.identifier_namespace = "synthetic.other-activity.v1";
         }},
        {"opaque", [](auto &input) { input.verified_activity->identity.opaque_id = "ACTIVITY-9"; }},
    }};
    const auto first = service.observe(support_observation());
    ASSERT_TRUE(first.contribution_id);
    std::uint64_t next_receipt_id = 103;
    for (const auto &[name, mutate] : mutations) {
        SCOPED_TRACE(name);
        auto confirmation = confirmation_observation();
        confirmation.receipt.id = ReceiptId{next_receipt_id++};
        confirmation.receipt.identity.opaque_id += name;
        mutate(confirmation);
        const auto pending = service.observe(confirmation);
        EXPECT_EQ(pending.disposition, ContributionLinkDisposition::unresolved);
        EXPECT_FALSE(pending.contribution_id);
    }
    ASSERT_EQ(repository.contributions().size(), 1U);
    EXPECT_EQ(repository.contributions()[0].id, *first.contribution_id);
    EXPECT_EQ(repository.contributions()[0].state.payment_evidence, PaymentEvidence::unknown);
    EXPECT_EQ(repository.observations().size(), 5U);
}

TEST_F(ContributionEvidenceAcceptance,
       ConfirmationPreservesRejectedAppliedReversedAndUnsettledMarkers) {
    const auto first = service.observe(support_observation());
    ASSERT_TRUE(first.contribution_id);
    auto state = ready_state();
    state.payment_evidence = PaymentEvidence::unknown;
    state.conditions.review = ContributionReview::rejected;
    state.conditions.application = ContributionApplication::applied;
    state.conditions.reversal = ContributionReversal::full;
    state.conditions.settlement = SettlementGate::unresolved;
    repository.seed_state(*first.contribution_id, state);
    const auto result = service.observe(confirmation_observation());
    EXPECT_EQ(result.contribution_id, first.contribution_id);
    ASSERT_EQ(repository.contributions().size(), 1U);
    const auto &updated = repository.contributions()[0].state;
    EXPECT_EQ(updated.payment_evidence, PaymentEvidence::confirmed);
    EXPECT_EQ(updated.conditions, state.conditions);
    EXPECT_EQ(assess_contribution_eligibility(updated).blockers,
              (std::set{ContributionBlocker::review_rejected, ContributionBlocker::already_applied,
                        ContributionBlocker::settlement_review_required}));
}

TEST_F(ContributionEvidenceAcceptance,
       FailedInitialSupportLeavesNoPartialContributionOrObservation) {
    for (const auto failure : {FakeContributionRepository::Failure::after_create,
                               FakeContributionRepository::Failure::after_observation,
                               FakeContributionRepository::Failure::before_commit}) {
        SCOPED_TRACE(static_cast<int>(failure));
        FakeContributionRepository store;
        ContributionEvidenceService intake{store};
        store.fail_next = failure;
        EXPECT_THROW(static_cast<void>(intake.observe(support_observation())),
                     ContributionStorageError);
        EXPECT_TRUE(store.contributions().empty());
        EXPECT_TRUE(store.observations().empty());
        EXPECT_EQ(intake.observe(support_observation()).disposition,
                  ContributionLinkDisposition::created);
        EXPECT_EQ(store.contributions().size(), 1U);
        EXPECT_EQ(store.observations().size(), 1U);
    }
}

TEST_F(ContributionEvidenceAcceptance,
       FailedConfirmationLeavesOriginalAndEvidenceUnchangedUntilRetry) {
    for (const auto failure : {FakeContributionRepository::Failure::after_observation,
                               FakeContributionRepository::Failure::after_update,
                               FakeContributionRepository::Failure::before_commit}) {
        SCOPED_TRACE(static_cast<int>(failure));
        FakeContributionRepository store;
        ContributionEvidenceService intake{store};
        const auto first = intake.observe(support_observation());
        const auto contributions_before = store.contributions();
        const auto observations_before = store.observations();
        store.fail_next = failure;
        EXPECT_THROW(static_cast<void>(intake.observe(confirmation_observation())),
                     ContributionStorageError);
        EXPECT_EQ(store.contributions(), contributions_before);
        EXPECT_EQ(store.observations(), observations_before);
        EXPECT_EQ(intake.observe(confirmation_observation()).contribution_id,
                  first.contribution_id);
        ASSERT_EQ(store.contributions().size(), 1U);
        EXPECT_EQ(store.contributions()[0].state.payment_evidence, PaymentEvidence::confirmed);
        EXPECT_EQ(store.observations().size(), 2U);
    }
}

TEST_F(ContributionEvidenceAcceptance,
       FailedPendingLinkDoesNotLoseUnresolvedConfirmationOrCreatePartialSupport) {
    for (const auto failure : {FakeContributionRepository::Failure::after_link,
                               FakeContributionRepository::Failure::before_commit}) {
        SCOPED_TRACE(static_cast<int>(failure));
        FakeContributionRepository store;
        ContributionEvidenceService intake{store};
        static_cast<void>(intake.observe(confirmation_observation()));
        const auto before = store.observations();
        store.fail_next = failure;
        EXPECT_THROW(static_cast<void>(intake.observe(support_observation())),
                     ContributionStorageError);
        EXPECT_TRUE(store.contributions().empty());
        EXPECT_EQ(store.observations(), before);
        const auto support = intake.observe(support_observation());
        ASSERT_TRUE(support.contribution_id);
        ASSERT_EQ(store.contributions().size(), 1U);
        EXPECT_EQ(store.contributions()[0].state.payment_evidence, PaymentEvidence::confirmed);
        ASSERT_EQ(store.observations().size(), 2U);
        EXPECT_EQ(store.observations()[0].contribution_id, support.contribution_id);
    }
}

TEST_F(ContributionEvidenceAcceptance,
       InvalidEvidenceAndContradictoryReceiptReuseCannotMutateHistory) {
    auto unknown_confirmation = confirmation_observation();
    unknown_confirmation.evidence = PaymentEvidence::unknown;
    EXPECT_THROW(static_cast<void>(service.observe(unknown_confirmation)), std::invalid_argument);
    auto wrong_origin = confirmation_observation();
    wrong_origin.verified_activity->identity.origin.source_account = "other-source";
    EXPECT_THROW(static_cast<void>(service.observe(wrong_origin)), std::invalid_argument);
    auto unproved_key = confirmation_observation();
    unproved_key.verified_activity->verification_ref.clear();
    EXPECT_THROW(static_cast<void>(service.observe(unproved_key)), std::invalid_argument);
    EXPECT_TRUE(repository.contributions().empty());
    EXPECT_TRUE(repository.observations().empty());

    auto support = support_observation();
    static_cast<void>(service.observe(support));
    const auto contributions_before = repository.contributions();
    const auto observations_before = repository.observations();
    support.support_conditions->evidence_admission.policy_ref =
        "replacement-policy-without-audited-override";
    EXPECT_THROW(static_cast<void>(service.observe(support)), std::invalid_argument);
    EXPECT_EQ(repository.contributions(), contributions_before);
    EXPECT_EQ(repository.observations(), observations_before);
}
} // namespace
