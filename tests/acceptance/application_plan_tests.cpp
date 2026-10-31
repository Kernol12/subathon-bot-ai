// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>
#include <subathon/core/application_plan.hpp>

namespace {
using namespace subathon::core;

// Synthetic resolved adapters: copy literal values and record inputs; no evaluation.
class Rules final : public RulePlanSource {
  public:
    std::vector<RuleSelection> values{{"normal", "rule-v2", true, "subscription matched"},
                                      {"bonus25", "rule-v5", false, "bonus50 wins group"},
                                      {"bonus50", "rule-v3", true, "highest applicable bonus"}};
    std::vector<PlanEvent> calls;
    std::vector<RuleSelection> capture(const PlanEvent &event) override {
        calls.push_back(event);
        return values;
    }
};
class Modifiers final : public ModifierPlanSource {
  public:
    std::vector<ModifierSelection> values{
        {"double-v1", "double-instance-r4", "main", "bonus", "multiply", "multiply", "2.0000"}};
    std::vector<PlanEvent> calls;
    std::vector<ModifierSelection> capture(const PlanEvent &event) override {
        calls.push_back(event);
        return values;
    }
};
PlanEvent event() {
    return {"event-41",
            "event-parent",
            "event-root",
            ReceiptId{41},
            {{"synthetic", "installation-a"}, "support-id", "opaque-41"},
            {"payment", 1, std::nullopt, std::nullopt, std::nullopt, ReceiptMoney{"20.0000", "EUR"},
             std::nullopt, std::nullopt},
            {1000, "connection-a", "configuration-v3", "modifier-context-v4"},
            ContributionId{9}};
}
PlanCapture input() {
    PlanCapture value;
    value.event = event();
    value.contribution_state.payment_evidence = PaymentEvidence::unknown;
    value.contribution_state.conditions.valuation.original_money = ReceiptMoney{"20.0000", "EUR"};
    value.valuation = {PlanValuationState::resolved, HistoricalValuation{"valuation-8",
                                                                         {"20.0000", "EUR"},
                                                                         "USD",
                                                                         "1.25000",
                                                                         "1.249900",
                                                                         "snapshot-7",
                                                                         std::nullopt,
                                                                         std::nullopt,
                                                                         900,
                                                                         std::nullopt}};
    // Already evaluated 60 seconds ×2 =120000ms; no cap/current timer in this input.
    value.requested_effects = {{"add-time", "action-v2", "normal", "main",
                                RequestedUnit::milliseconds, true, Exact(120000), 120000,
                                "60 seconds * 2; final half-even milliseconds",
                                "actual-proportional"}};
    return value;
}
ApplicationPlan history() {
    return {
        "plan-9",
        {{1,
          std::nullopt,
          input(),
          {{"normal", "rule-v2", true, "subscription matched"}},
          {{"double-v1", "double-instance-r4", "main", "bonus", "multiply", "multiply", "2.0000"}},
          std::nullopt}}};
}
PlanOverrideDecision authorized() {
    // Upstream permission result supplied explicitly; these strings are audit only.
    return {OverrideAuthority::authorized, "moderator-7", "explicit replacement calculation",
            "decision-12"};
}

// §§3.2–5, NUM-01/04: lossless original data, not invented provider facts.
TEST(ApplicationPlan, CapturesOriginalEnvelopeHistoricalQuoteAndIndependentHolds) {
    Rules rules;
    Modifiers modifiers;
    const auto supplied = input();
    const auto plan = capture_application_plan("plan-9", supplied, rules, modifiers);
    ASSERT_EQ(plan.revisions.size(), 1u);
    const auto &revision = plan.revisions.front();
    EXPECT_EQ(plan.plan_ref, "plan-9");
    EXPECT_EQ(revision.revision, 1u);
    EXPECT_FALSE(revision.parent_revision);
    EXPECT_FALSE(revision.override_decision);
    EXPECT_EQ(revision.captured, supplied);
    EXPECT_EQ(revision.captured.event.facts.money->original_decimal, "20.0000");
    EXPECT_EQ(revision.captured.valuation.historical->effective_rate_decimal, "1.25000");
    EXPECT_EQ(revision.captured.valuation.historical->fetched_rate_decimal, "1.249900");
    EXPECT_FALSE(revision.captured.valuation.historical->source_ref);
    EXPECT_FALSE(revision.captured.valuation.historical->rate_timestamp_utc_ms);
    EXPECT_FALSE(revision.captured.event.facts.source_timestamp_utc_ms);
    EXPECT_FALSE(revision.captured.event.facts.actor_id);
    EXPECT_EQ(revision.captured.contribution_state.payment_evidence, PaymentEvidence::unknown);
    EXPECT_EQ(revision.captured.contribution_state.conditions.review, ContributionReview::pending);
    EXPECT_EQ(revision.captured.contribution_state.conditions.settlement,
              SettlementGate::unresolved);
    EXPECT_EQ(revision.captured.contribution_state.conditions.application,
              ContributionApplication::not_applied);
}

// §§6/7: adapter outputs describe retained selections, not another rule evaluator.
TEST(ApplicationPlan, CapturesOrderedMatchesExclusionsAndTwoLevelModifierProvenance) {
    Rules rules;
    Modifiers modifiers;
    modifiers.values.push_back({"half-v2", "half-instance-r1", "score", "score-group",
                                "add_bonuses", "highest", "0.5000"});
    const auto supplied = input();
    const auto plan = capture_application_plan("plan-9", supplied, rules, modifiers);
    ASSERT_EQ(plan.revisions.size(), 1u);
    EXPECT_EQ(rules.calls, std::vector<PlanEvent>{supplied.event});
    EXPECT_EQ(modifiers.calls, std::vector<PlanEvent>{supplied.event});
    EXPECT_EQ(plan.revisions[0].rules, rules.values);
    EXPECT_EQ(plan.revisions[0].modifiers, modifiers.values);
    EXPECT_FALSE(plan.revisions[0].rules[1].selected);
    EXPECT_EQ(plan.revisions[0].rules[1].explanation, "bonus50 wins group");
}

// §§3.4/7: later expiry/settings changes cannot rewrite captured pending input.
TEST(ApplicationPlan, OwnsValuesAcrossCallerEditsModifierExpiryAndSourceReplacement) {
    Rules rules;
    Modifiers modifiers;
    auto supplied = input();
    const auto plan = capture_application_plan("plan-9", supplied, rules, modifiers);
    supplied.event.arrival.configuration_ref = "configuration-v99";
    supplied.event.arrival.modifier_context_ref = "modifier-context-expired";
    supplied.event.facts.money->original_decimal = "999";
    supplied.valuation.historical->effective_rate_decimal = "9";
    supplied.requested_effects[0].requested_units = 1;
    supplied.contribution_state.conditions.review = ContributionReview::accepted;
    rules.values.clear();
    modifiers.values.clear();
    ASSERT_EQ(plan.revisions.size(), 1u);
    EXPECT_EQ(plan.revisions[0].captured, input());
    ASSERT_EQ(plan.revisions[0].rules.size(), 3u);
    ASSERT_EQ(plan.revisions[0].modifiers.size(), 1u);
    EXPECT_EQ(plan.revisions[0].modifiers[0].instance_revision_ref, "double-instance-r4");
}

// NUM-01/04: copy exact final-request provenance, do not evaluate or reround.
TEST(ApplicationPlan, RetainsExactFractionSignedRequestsAndZeroWithActionProvenance) {
    Rules rules;
    Modifiers modifiers;
    auto supplied = input();
    supplied.requested_effects = {
        {"tiny-score", "a1", "normal", "score", RequestedUnit::micro_points, true, Exact(3) / 2, 2,
         "1.5 micro-points half-even to 2", "proportional"},
        {"correction", "a2", "normal", "main", RequestedUnit::milliseconds, false, Exact(-5) / 2,
         -2, "-2.5 milliseconds half-even to -2", "signed"},
        {"zero", "a3", "normal", "count", RequestedUnit::whole_count, false, Exact(0), 0,
         "zero permitted", "whole-on-full"}};
    const auto plan = capture_application_plan("plan-9", supplied, rules, modifiers);
    ASSERT_EQ(plan.revisions.size(), 1u);
    EXPECT_EQ(plan.revisions[0].captured.requested_effects, supplied.requested_effects);
    EXPECT_EQ(plan.revisions[0].captured.requested_effects[0].exact_requested_units, Exact(3) / 2);
    EXPECT_EQ(plan.revisions[0].captured.requested_effects[1].requested_units, -2);
}

// B03/B04: this output is requested120000 even when later capacity is30000 or0.
TEST(ApplicationPlan, CapturesRequestWithoutFreezingActualTimerCapacity) {
    Rules rules;
    Modifiers modifiers;
    const auto plan = capture_application_plan("plan-9", input(), rules, modifiers);
    ASSERT_EQ(plan.revisions.size(), 1u);
    ASSERT_EQ(plan.revisions[0].captured.requested_effects.size(), 1u);
    EXPECT_EQ(plan.revisions[0].captured.requested_effects[0].requested_units, 120000);
    EXPECT_EQ(check_plan_preview(plan, {1, "timer-at-55-min"}, "timer-at-58-min"),
              PlanPreviewValidity::stale_state);
    EXPECT_EQ(plan.revisions[0].captured.requested_effects[0].requested_units, 120000);
    EXPECT_EQ(plan.revisions[0].captured.contribution_state.conditions.application,
              ContributionApplication::not_applied);
}

TEST(ApplicationPlan, MissingHistoricalValuationRemainsExplicitAndKeepsOtherHolds) {
    Rules rules;
    Modifiers modifiers;
    auto supplied = input();
    supplied.valuation = {PlanValuationState::missing_historical, std::nullopt};
    supplied.requested_effects.clear();
    supplied.contribution_state.conditions.valuation.missing_rate = true;
    const auto plan = capture_application_plan("plan-9", supplied, rules, modifiers);
    ASSERT_EQ(plan.revisions.size(), 1u);
    EXPECT_EQ(plan.revisions[0].captured, supplied);
    EXPECT_FALSE(plan.revisions[0].captured.valuation.historical);
    EXPECT_EQ(plan.revisions[0].captured.contribution_state.payment_evidence,
              PaymentEvidence::unknown);
    EXPECT_EQ(plan.revisions[0].captured.contribution_state.conditions.review,
              ContributionReview::pending);
}

TEST(ApplicationPlan, MissingValuationAllowsIndependentNonmonetaryRequestWithoutResolvingHold) {
    Rules rules;
    Modifiers modifiers;
    auto supplied = input();
    supplied.valuation = {PlanValuationState::missing_historical, std::nullopt};
    supplied.requested_effects[0].requires_historical_valuation = false;
    const auto plan = capture_application_plan("plan-9", supplied, rules, modifiers);
    ASSERT_EQ(plan.revisions.size(), 1u);
    EXPECT_EQ(plan.revisions[0].captured, supplied);
}

TEST(ApplicationPlan, RejectsValuationDependentRequestWithoutHistoricalQuote) {
    Rules rules;
    Modifiers modifiers;
    auto supplied = input();
    supplied.valuation = {PlanValuationState::missing_historical, std::nullopt};
    EXPECT_THROW((void)capture_application_plan("plan-9", supplied, rules, modifiers),
                 std::invalid_argument);
}

TEST(ApplicationPlan, NonmonetaryNotApplicableDoesNotInventMoneyOrConfirmation) {
    Rules rules;
    Modifiers modifiers;
    auto supplied = input();
    supplied.event.facts.money.reset();
    supplied.contribution_state.conditions.valuation = {false, std::nullopt, std::nullopt, false};
    supplied.contribution_state.payment_evidence = PaymentEvidence::not_applicable;
    supplied.valuation = {PlanValuationState::not_applicable, std::nullopt};
    supplied.requested_effects[0].requires_historical_valuation = false;
    const auto plan = capture_application_plan("plan-9", supplied, rules, modifiers);
    ASSERT_EQ(plan.revisions.size(), 1u);
    EXPECT_EQ(plan.revisions[0].captured, supplied);
    EXPECT_FALSE(plan.revisions[0].captured.event.facts.money);
}

TEST(ApplicationPlan, RejectsInconsistentValuationShapeAndSubstitutedOriginalMoney) {
    Rules rules;
    Modifiers modifiers;
    auto supplied = input();
    supplied.valuation.historical.reset();
    EXPECT_THROW((void)capture_application_plan("plan-9", supplied, rules, modifiers),
                 std::invalid_argument);
    supplied = input();
    supplied.valuation.state = PlanValuationState::not_applicable;
    EXPECT_THROW((void)capture_application_plan("plan-9", supplied, rules, modifiers),
                 std::invalid_argument);
    supplied = input();
    supplied.valuation.historical->original_money.original_decimal = "20.00";
    EXPECT_THROW((void)capture_application_plan("plan-9", supplied, rules, modifiers),
                 std::invalid_argument);
}

TEST(ApplicationPlan, RejectsMissingLocalIdentityWithoutInferringProviderIdentity) {
    Rules rules;
    Modifiers modifiers;
    EXPECT_THROW((void)capture_application_plan("", input(), rules, modifiers),
                 std::invalid_argument);
    auto supplied = input();
    supplied.event.event_ref.clear();
    EXPECT_THROW((void)capture_application_plan("plan-9", supplied, rules, modifiers),
                 std::invalid_argument);
    supplied = input();
    supplied.event.root_event_ref.clear();
    EXPECT_THROW((void)capture_application_plan("plan-9", supplied, rules, modifiers),
                 std::invalid_argument);
    supplied = input();
    supplied.event.receipt_id.value = 0;
    EXPECT_THROW((void)capture_application_plan("plan-9", supplied, rules, modifiers),
                 std::invalid_argument);
}

// §§3.4/7, MOD-04: override appends instead of rewriting original arrival context.
TEST(ApplicationPlan, AuthorizedOverrideAppendsParentedRevisionAndPreservesOriginal) {
    Rules rules;
    Modifiers modifiers;
    const auto original = history();
    auto replacement = input();
    replacement.contribution_state.conditions.review = ContributionReview::accepted;
    replacement.requested_effects[0].exact_requested_units = 180000;
    replacement.requested_effects[0].requested_units = 180000;
    replacement.requested_effects[0].calculation_explanation = "explicit override to180 seconds";
    rules.values = {{"normal", "rule-v8", true, "explicit override"}};
    modifiers.values = {
        {"triple-v1", "instance-r9", "main", "bonus", "multiply", "multiply", "3.0"}};
    const auto revised =
        revise_application_plan(original, 1, replacement, authorized(), rules, modifiers);
    EXPECT_EQ(original, history());
    ASSERT_EQ(revised.revisions.size(), 2u);
    EXPECT_EQ(revised.plan_ref, "plan-9");
    EXPECT_EQ(revised.revisions[0], original.revisions[0]);
    EXPECT_EQ(revised.revisions[1].revision, 2u);
    EXPECT_EQ(revised.revisions[1].parent_revision, 1u);
    EXPECT_EQ(revised.revisions[1].override_decision, authorized());
    EXPECT_EQ(revised.revisions[1].captured, replacement);
    EXPECT_EQ(revised.revisions[1].rules, rules.values);
    EXPECT_EQ(revised.revisions[1].modifiers, modifiers.values);
    EXPECT_EQ(revised.revisions[0].captured.requested_effects[0].requested_units, 120000);
}

TEST(ApplicationPlan, ResolvingMissingRateAddsSourcedRevisionWithoutConfirmingOrAccepting) {
    Rules rules;
    Modifiers modifiers;
    auto original = history();
    original.revisions[0].captured.valuation = {PlanValuationState::missing_historical,
                                                std::nullopt};
    original.revisions[0].captured.requested_effects.clear();
    const auto before = original;
    auto replacement = input();
    replacement.valuation.historical->manual_decision_ref = "historical-manual-decision-6";
    replacement.valuation.historical->source_ref = "authorized-historical-input";
    const auto revised =
        revise_application_plan(original, 1, replacement, authorized(), rules, modifiers);
    EXPECT_EQ(original, before);
    ASSERT_EQ(revised.revisions.size(), 2u);
    EXPECT_EQ(revised.revisions[0], before.revisions[0]);
    EXPECT_EQ(revised.revisions[1].captured.valuation.historical->manual_decision_ref,
              "historical-manual-decision-6");
    EXPECT_EQ(revised.revisions[1].captured.contribution_state.payment_evidence,
              PaymentEvidence::unknown);
    EXPECT_EQ(revised.revisions[1].captured.contribution_state.conditions.review,
              ContributionReview::pending);
}

TEST(ApplicationPlan, AuditStringsCannotReplaceExplicitAuthorizedPermissionResult) {
    Rules rules;
    Modifiers modifiers;
    const auto original = history();
    for (auto authority : {OverrideAuthority::unknown, OverrideAuthority::denied}) {
        auto decision = authorized();
        decision.authority = authority;
        EXPECT_THROW(
            (void)revise_application_plan(original, 1, input(), decision, rules, modifiers),
            std::invalid_argument);
    }
    EXPECT_TRUE(rules.calls.empty());
    EXPECT_TRUE(modifiers.calls.empty());
    EXPECT_EQ(original, history());
}

TEST(ApplicationPlan, AuthorizedOverrideRequiresCompleteAuditDecision) {
    Rules rules;
    Modifiers modifiers;
    const auto original = history();
    auto decision = authorized();
    decision.actor_ref.clear();
    EXPECT_THROW((void)revise_application_plan(original, 1, input(), decision, rules, modifiers),
                 std::invalid_argument);
    decision = authorized();
    decision.reason.clear();
    EXPECT_THROW((void)revise_application_plan(original, 1, input(), decision, rules, modifiers),
                 std::invalid_argument);
    decision = authorized();
    decision.decision_ref.clear();
    EXPECT_THROW((void)revise_application_plan(original, 1, input(), decision, rules, modifiers),
                 std::invalid_argument);
    EXPECT_EQ(original, history());
}

TEST(ApplicationPlan, StaleOverrideRevisionFailsBeforeReplacementSourcesRun) {
    Rules rules;
    Modifiers modifiers;
    const auto original = history();
    EXPECT_THROW(
        (void)revise_application_plan(original, 0, input(), authorized(), rules, modifiers),
        std::runtime_error);
    EXPECT_TRUE(rules.calls.empty());
    EXPECT_TRUE(modifiers.calls.empty());
    EXPECT_EQ(original, history());
}

TEST(ApplicationPlan, OverrideCannotRewriteOriginalEventIdentityFactsArrivalOrContribution) {
    Rules rules;
    Modifiers modifiers;
    const auto original = history();
    auto replacement = input();
    replacement.event.event_ref = "different-event";
    EXPECT_THROW(
        (void)revise_application_plan(original, 1, replacement, authorized(), rules, modifiers),
        std::invalid_argument);
    replacement = input();
    replacement.event.facts.money->original_decimal = "99";
    EXPECT_THROW(
        (void)revise_application_plan(original, 1, replacement, authorized(), rules, modifiers),
        std::invalid_argument);
    replacement = input();
    replacement.event.arrival.configuration_ref = "current-config";
    EXPECT_THROW(
        (void)revise_application_plan(original, 1, replacement, authorized(), rules, modifiers),
        std::invalid_argument);
    replacement = input();
    replacement.event.contribution_id = ContributionId{99};
    EXPECT_THROW(
        (void)revise_application_plan(original, 1, replacement, authorized(), rules, modifiers),
        std::invalid_argument);
    replacement = input();
    replacement.event.parent_event_ref = "new-parent";
    EXPECT_THROW(
        (void)revise_application_plan(original, 1, replacement, authorized(), rules, modifiers),
        std::invalid_argument);
    EXPECT_EQ(original, history());
}

// Repeated overrides retain every earlier revision and invalidate the old preview.
TEST(ApplicationPlan, SecondOverrideExtendsExistingChainWithoutReplacingEarlierRevisions) {
    Rules rules;
    Modifiers modifiers;
    auto original = history();
    auto second = original.revisions.front();
    second.revision = 2;
    second.parent_revision = 1;
    second.override_decision = authorized();
    second.captured.requested_effects[0].requested_units = 180000;
    second.captured.requested_effects[0].exact_requested_units = 180000;
    original.revisions.push_back(second);
    const auto before = original;
    auto replacement = input();
    replacement.requested_effects[0].requested_units = 60000;
    replacement.requested_effects[0].exact_requested_units = 60000;
    auto decision = authorized();
    decision.decision_ref = "decision-13";
    const auto revised =
        revise_application_plan(original, 2, replacement, decision, rules, modifiers);
    ASSERT_EQ(revised.revisions.size(), 3u);
    EXPECT_EQ(original, before);
    EXPECT_EQ(revised.revisions[0], before.revisions[0]);
    EXPECT_EQ(revised.revisions[1], before.revisions[1]);
    EXPECT_EQ(revised.revisions[2].revision, 3u);
    EXPECT_EQ(revised.revisions[2].parent_revision, 2u);
    EXPECT_EQ(revised.revisions[2].override_decision, decision);
    EXPECT_EQ(revised.revisions[2].captured, replacement);
    EXPECT_EQ(check_plan_preview(revised, {2, "state-r10"}, "state-r10"),
              PlanPreviewValidity::stale_plan);
}

TEST(ApplicationPlan, CurrentPreviewOnlyReportsFreshnessAndDoesNotApplyContribution) {
    const auto plan = history();
    EXPECT_EQ(check_plan_preview(plan, {1, "state-r10"}, "state-r10"),
              PlanPreviewValidity::current);
    EXPECT_EQ(plan, history());
}
TEST(ApplicationPlan, PreviewReportsPlanStalenessBeforeStateStaleness) {
    const auto plan = history();
    EXPECT_EQ(check_plan_preview(plan, {0, "old-state"}, "state-r10"),
              PlanPreviewValidity::stale_plan);
    EXPECT_EQ(check_plan_preview(plan, {1, "old-state"}, "state-r10"),
              PlanPreviewValidity::stale_state);
}
TEST(ApplicationPlan, PreviewRejectsEmptyStateRevisionTokens) {
    const auto plan = history();
    EXPECT_THROW((void)check_plan_preview(plan, {1, ""}, "state-r10"), std::invalid_argument);
    EXPECT_THROW((void)check_plan_preview(plan, {1, "state-r10"}, ""), std::invalid_argument);
}
TEST(ApplicationPlan, RejectsEmptyOrMalformedRevisionHistory) {
    Rules rules;
    Modifiers modifiers;
    auto plan = history();
    plan.revisions.clear();
    EXPECT_THROW((void)check_plan_preview(plan, {1, "state-r10"}, "state-r10"),
                 std::invalid_argument);
    EXPECT_THROW((void)revise_application_plan(plan, 1, input(), authorized(), rules, modifiers),
                 std::invalid_argument);
    plan = history();
    plan.revisions[0].parent_revision = 0;
    EXPECT_THROW((void)check_plan_preview(plan, {1, "state-r10"}, "state-r10"),
                 std::invalid_argument);
    plan = history();
    plan.revisions[0].override_decision = authorized();
    EXPECT_THROW((void)check_plan_preview(plan, {1, "state-r10"}, "state-r10"),
                 std::invalid_argument);
    plan = history();
    plan.revisions.push_back(plan.revisions.front());
    plan.revisions[1].revision = 3;
    plan.revisions[1].parent_revision = 1;
    plan.revisions[1].override_decision = authorized();
    EXPECT_THROW((void)check_plan_preview(plan, {3, "state-r10"}, "state-r10"),
                 std::invalid_argument);
}
} // namespace
