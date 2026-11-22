// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

#include <limits>

namespace {
using namespace rules_005;

RuleCondition gift_count(std::uint64_t count) {
    return {RulePredicate{RuleComparison<RuleCount, RuleOrder>{{RuleCountField::gift_count, count},
                                                               RuleOrder::at_least}}};
}

TEST(Rules005, S1RankedGiftBundle) {
    const auto cfg = validate_rule_configuration(
        {"gift-config",
         {},
         {rule("normal", Kind::gift_bundle, {time("normal-time", "1000")}),
          rule("25", Kind::gift_bundle, {time("25-time", "1500")}, gift_count(25), 25, "G"),
          rule("50", Kind::gift_bundle, {time("50-time", "2000")}, gift_count(50), 50, "G",
               {"tail"}),
          rule("tie", Kind::gift_bundle, {time("tie-time", "4000")}, gift_count(50), 50, "G"),
          rule("tail", Kind::gift_bundle, {time("tail-time", "3000")})}});
    const auto r = root("gift");
    auto in = inputs(10'000);
    add_root(in, r);
    in.captures.push_back(retain(r.event, cfg, Kind::gift_bundle, "gift_bundle",
                                 {RuleCount{RuleCountField::gift_count, 50}}));
    turn(in, "gift-admitted", event(r.event));
    const auto scenario = replay("S1", std::move(in));

    const auto result = inspect_simulation(scenario, limits);
    expect_complete(result, scenario);
    ASSERT_EQ(result.trace.size(), 1U);
    const auto &trace = result.trace.front();
    ASSERT_TRUE(trace.selection);
    const auto &selected = trace.selection->selection;
    EXPECT_EQ(selected.configuration_ref, "gift-config");
    EXPECT_EQ(selected.event_ref, "gift");
    EXPECT_EQ(selected.decisions,
              (std::vector<RuleSelectionDecision>{{Disposition::selected, {}},
                                                  {Disposition::exclusive_group, 2},
                                                  {Disposition::selected, {}},
                                                  {Disposition::exclusive_group, 2},
                                                  {Disposition::excluded, 2}}));
    EXPECT_EQ(selected.priority_ties, (std::vector<RulePriorityTie>{{{2, 3}}}));
    EXPECT_EQ(selected.selected_rule_indices, (std::vector<std::size_t>{2, 0}));
    ASSERT_TRUE(trace.configuration);
    const auto &retained = trace.configuration->data().rules;
    ASSERT_EQ(retained.size(), 5U);
    for (const auto &[index, action] :
         {std::pair{1U, "25-time"}, {3U, "tie-time"}, {4U, "tail-time"}}) {
        ASSERT_EQ(retained[index].actions.size(), 1U);
        EXPECT_EQ(retained[index].actions.front().action_ref, action);
    }
    ASSERT_EQ(trace.actions.size(), 2U);
    expect_actual_time(trace.actions[0], key("50", "50-time"), {2000, 2000, 0, 0, 12'000});
    expect_actual_time(trace.actions[1], key("normal", "normal-time"), {1000, 1000, 0, 0, 13'000});
    EXPECT_EQ(result.final_state.timer.main_remaining_ms, 13'000);
}

EffectiveRateSnapshot usd_quote(std::string snapshot, std::string source, std::string rate,
                                std::string numerator, std::string denominator,
                                std::optional<std::int64_t> published) {
    return make_effective_rate_snapshot(
        {std::move(snapshot),
         {},
         {},
         {{{source,
            "synthetic-rate-source",
            "rate-config",
            "rate-request",
            "rate-evidence",
            {},
            {},
            {},
            {"publication", published}},
           {{"EUR", "USD", std::move(rate)}}}},
         {},
         {FetchedRateConstraint{source, 1}},
         {{{{"EUR", "1", "1"}, {"USD", std::move(numerator), std::move(denominator)}}}}});
}

TEST(Rules005, S3MissingAndOldQuote) {
    const auto cfg = validate_rule_configuration(
        {"currency-config",
         {},
         {rule("foreign", Kind::payment_confirmed, {time("qualified", "1000")},
               {RulePredicate{RuleComparison<RuleMoney, RuleOrder>{
                   {RuleMoneyField::source_amount, {"25", "USD"}}, RuleOrder::at_least}}})}});
    const ReceiptMoney money{"20.00", "EUR"};
    struct Example {
        const char *name;
        bool quote;
        std::optional<std::int64_t> publication;
        std::int64_t observation;
        std::optional<std::int64_t> age;
    };
    for (const auto &example :
         {Example{"missing-quote", false, {}, 3000, {}},
          Example{"old-quote", true, 1000, 3000, 2000},
          Example{"threshold-quote", true, 1000, 2500, 1500},
          Example{"missing-time", true, {}, 3000, {}},
          Example{"reversed-time", true, 1000, 999, {}},
          Example{"age-overflow", true, std::numeric_limits<std::int64_t>::min(), 0, {}}}) {
        SCOPED_TRACE(example.name);
        const auto r = root(example.name);
        auto in = inputs(10'000);
        add_root(in, r);
        const auto valuation = example.quote
                                   ? std::optional{HistoricalValuation{"retained-valuation",
                                                                       money,
                                                                       "USD",
                                                                       "1.5",
                                                                       "1.5",
                                                                       "retained",
                                                                       "retained-source",
                                                                       {},
                                                                       {},
                                                                       {}}}
                                   : std::nullopt;
        auto capture =
            retain(r.event, cfg, Kind::payment_confirmed, "payment_confirmed",
                   {RuleMoney{RuleMoneyField::source_amount, money}, PaymentEvidence::confirmed},
                   valuation, money);
        if (example.quote)
            capture.rates.emplace(
                usd_quote("retained", "retained-source", "1.5", "3", "2", example.publication));
        capture.quote_observation_utc_ms = example.observation;
        capture.quote_age_threshold_ms = 1500;
        in.captures.push_back(std::move(capture));
        turn(in, "inspect-money", event(r.event), {example.observation, 0ms});
        auto scenario = replay(example.name, std::move(in));
        scenario.current.rates.emplace(usd_quote("current", "current-source", "2", "2", "1", 2000));

        const auto result = inspect_simulation(scenario, limits);
        expect_complete(result, scenario);
        ASSERT_EQ(result.trace.size(), 1U);
        const auto &trace = result.trace.front();
        ASSERT_TRUE(trace.selection);
        ASSERT_EQ(trace.selection->selection.decisions.size(), 1U);
        if (!example.quote) {
            EXPECT_EQ(trace.selection->selection.decisions[0].disposition,
                      Disposition::condition_unknown);
            EXPECT_FALSE(trace.quote);
            EXPECT_TRUE(trace.actions.empty());
            if (trace.plan) {
                EXPECT_TRUE(trace.plan->inspected.captured.requested_effects.empty());
            }
            EXPECT_EQ(result.final_state.timer.main_remaining_ms, 10'000);
            continue;
        }
        EXPECT_EQ(trace.selection->selection.decisions[0].disposition, Disposition::selected);
        ASSERT_TRUE(trace.quote);
        EXPECT_EQ(trace.quote->provenance, valuation);
        EXPECT_EQ(trace.quote->rate, Exact(3) / 2);
        EXPECT_EQ(trace.quote->converted, Exact(30));
        EXPECT_EQ(trace.quote->age_ms, example.age);
        EXPECT_EQ(trace.quote->stale,
                  example.age ? std::optional{*example.age > 1500} : std::nullopt);
        ASSERT_EQ(trace.actions.size(), 1U);
        expect_actual_time(trace.actions.front(), key("foreign", "qualified"),
                           {1000, 1000, 0, 0, 11'000});
        EXPECT_EQ(result.final_state.timer.main_remaining_ms, 11'000);
    }
}

struct PlanRules final : RulePlanSource {
    std::vector<RuleSelection> capture(const PlanEvent &) override {
        return {{"prepared-rule", "v1", true, "matched"}};
    }
};

struct PlanModifiers final : ModifierPlanSource {
    std::vector<ModifierSelection> capture(const PlanEvent &) override { return {}; }
};

TEST(Rules005, S4StalePlanPrecedesStaleState) {
    const auto cfg = validate_rule_configuration(
        {"prepared-config",
         {},
         {named("prepared-rule", "prepared", {time("prepared-time", "1000")})}});
    const auto r = root("prepared");
    const auto retained = retain(r.event, cfg, Kind::named_event, "named_event",
                                 {RuleText{RuleTextField::event_name, "prepared"}});
    const PlanCapture original{
        retained.receipt,
        retained.eligibility,
        retained.valuation,
        {{"prepared-time", "v1", "prepared-rule", "Main", RequestedUnit::milliseconds, false,
          Exact(1000), 1000, "captured fixed request", "reverse-actual"}}};
    PlanRules rules;
    PlanModifiers modifiers;
    const auto initial = capture_application_plan("P", original, rules, modifiers);
    auto replacement = original;
    replacement.requested_effects.front().requested_units = 2000;
    replacement.requested_effects.front().exact_requested_units = Exact(2000);
    const auto revised = revise_application_plan(
        initial, 1, replacement,
        {OverrideAuthority::authorized, "author", "revised fixed request", "override-P"}, rules,
        modifiers);
    for (const bool new_head : {true, false}) {
        SCOPED_TRACE(new_head);
        auto in = inputs(10'000);
        add_root(in, r);
        in.captures.push_back(retained);
        in.initial.state_revision = "S2";
        auto attempt = event(r.event);
        attempt.prepared = PreparedPlan{new_head ? revised : initial, {1, "S1"}};
        turn(in, "try-selected-plan", std::move(attempt));
        const auto scenario = replay(new_head ? "new-plan-and-state" : "new-state", std::move(in));

        for (int repeat = 0; repeat != 2; ++repeat) {
            const auto result = inspect_simulation(scenario, limits);
            expect_complete(result, scenario);
            ASSERT_EQ(result.trace.size(), 1U);
            const auto &trace = result.trace.front();
            ASSERT_TRUE(trace.plan);
            EXPECT_EQ(trace.plan->preview, new_head ? PlanPreviewValidity::stale_plan
                                                    : PlanPreviewValidity::stale_state);
            EXPECT_EQ(trace.plan->inspected.revision, 1U);
            EXPECT_EQ(trace.plan->inspected.captured, original);
            EXPECT_EQ(trace.plan->head, new_head ? 2U : 1U);
            ASSERT_EQ(trace.plan->head_requests.size(), 1U);
            EXPECT_EQ(trace.plan->head_requests.front().requested_units, new_head ? 2000 : 1000);
            for (const auto &action : trace.actions) {
                EXPECT_FALSE(action.submitted);
                EXPECT_FALSE(action.accepted);
            }
            EXPECT_EQ(result.final_state.timer.main_remaining_ms, 10'000);
            EXPECT_EQ(result.final_state.state_revision, "S2");
        }
    }
}

TEST(Rules005, S12SubscriptionClassificationCanBeUnknown) {
    const auto cfg = validate_rule_configuration(
        {"subscription-config",
         {},
         {rule("N", Kind::subscription, {time("subscription-time", "60000")},
               {RuleAll{{1, 2}}, text(RuleTextField::subscription_tier, "tier1"),
                text(RuleTextField::subscription_classification, "new")}),
          rule("G", Kind::gift_bundle, {time("gift-time", "30000")})}});
    for (const bool classified : {true, false}) {
        SCOPED_TRACE(classified);
        auto in = inputs(120'000);
        const auto r = root(classified ? "subscription-new" : "subscription-unclassified");
        add_root(in, r);
        std::vector<RuleCapturedFact> facts{RuleText{RuleTextField::subscription_tier, "tier1"}};
        if (classified)
            facts.emplace_back(RuleText{RuleTextField::subscription_classification, "new"});
        in.captures.push_back(
            retain(r.event, cfg, Kind::subscription, "subscription", std::move(facts)));
        turn(in, "subscription-admitted", event(r.event));
        const auto scenario = replay(r.event.event, std::move(in));

        const auto result = inspect_simulation(scenario, limits);
        expect_complete(result, scenario);
        ASSERT_EQ(result.trace.size(), 1U);
        const auto &trace = result.trace.front();
        ASSERT_TRUE(trace.selection);
        EXPECT_EQ(trace.selection->selection.decisions,
                  (std::vector<RuleSelectionDecision>{
                      {classified ? Disposition::selected : Disposition::condition_unknown, {}},
                      {Disposition::condition_no_match, {}}}));
        ASSERT_EQ(trace.actions.size(), classified ? 1U : 0U);
        if (classified)
            expect_actual_time(trace.actions.front(), key("N", "subscription-time"),
                               {60'000, 60'000, 0, 0, 180'000});
        EXPECT_EQ(result.final_state.timer.main_remaining_ms, classified ? 180'000 : 120'000);
    }
}
} // namespace
