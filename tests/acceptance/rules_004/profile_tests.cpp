// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

namespace {
using namespace rules_004;

TEST(Rules004, G9ActivationValidatesLimitsAndKeepsSupportedProfile) {
    Owner owner;
    auto runtime = profile;
    runtime.maximum = {40, 512};
    owner.define("defaults", {32, 256}, {});
    owner.define("small", {2, 3}, {});
    owner.define("deep", {41, 3}, {});
    owner.define("wide", {2, 513}, {});
    EXPECT_EQ(owner.activate("defaults", runtime), PolicyCheck::supported);
    EXPECT_EQ(owner.active, "defaults");
    EXPECT_EQ(owner.activate("small", runtime), PolicyCheck::supported);
    EXPECT_EQ(owner.active, "small");
    for (const std::string id : {"deep", "wide"}) {
        EXPECT_EQ(owner.activate(id, runtime), PolicyCheck::exceeds_profile);
        EXPECT_EQ(owner.active, "small");
    }
    for (const RuleGenerationLimits limits : {RuleGenerationLimits{0, 3}, {2, 0}}) {
        try {
            static_cast<void>(owner.define("zero", limits, {}));
            ADD_FAILURE() << "zero must be rejected by the shipped configuration validator";
        } catch (const RuleDefinitionFailure &failure) {
            EXPECT_EQ(failure.code(), RuleDefinitionError::invalid_generation_limits);
        }
        EXPECT_EQ(owner.active, "small");
    }

    auto admitted_profile = runtime;
    admitted_profile.maximum = {64, 512};
    EXPECT_EQ(owner.activate("deep", admitted_profile), PolicyCheck::supported);
    // Compare-and-change at the modeled owner. The proposed profile is checked
    // against the still-active validated configuration before it can take effect.
    const auto check = check_generation_policy(owner.configurations.at(owner.active), runtime);
    if (check == PolicyCheck::supported)
        admitted_profile = runtime;
    EXPECT_EQ(check, PolicyCheck::exceeds_profile);
    EXPECT_EQ(admitted_profile.maximum.maximum_depth, 64U);
    EXPECT_EQ(admitted_profile.maximum.maximum_generated_events, 512U);
    EXPECT_EQ(owner.active, "deep");
}

TEST(Rules004, G9ExternalReductionRequiresNewActivationAndRetainsAdmittedPolicy) {
    for (const bool deny : {false, true}) {
        SCOPED_TRACE(deny);
        Owner owner;
        auto admitted_profile = profile;
        admitted_profile.maximum = {64, 512};
        auto reduced = profile;
        reduced.maximum = {40, 512};
        owner.define(
            "C", {41, 3},
            {named_rule("R-rule", "seed",
                        {delta("plus5", "5"), emit("emit-child", "leaf"), delta("plus7", "7")})});
        ASSERT_EQ(owner.activate("C", admitted_profile), PolicyCheck::supported);
        const auto r = root();
        const auto fresh = root("R-new", "CR-new");
        const auto action = key("R-rule", "emit-child", 1);
        const auto proposed = child(r.event, "E1", NamedAction{action});
        owner.keep(r.event, "seed");
        owner.keep(fresh.event, "seed");
        owner.keep(proposed, "leaf");

        {
            const auto initial = owner.open(r, admitted_profile);
            ASSERT_TRUE(std::holds_alternative<OpenedChain>(initial));
            EXPECT_EQ(std::get<OpenedChain>(initial).committed_children, 0U);
        }
        const ModeledHistory admitted{r,  "admit-R", admitted_profile, {{r.event, 0, 0}}, {},
                                      {}, {}};
        EXPECT_EQ(std::get<OpenRefusal>(owner.open(fresh, reduced)),
                  OpenRefusal::needs_reactivation);
        EXPECT_EQ(std::get<OpenRefusal>(open_chain(fresh, reduced, Unavailable{}, owner.view())),
                  OpenRefusal::history_unavailable);
        auto conflicting_origin = r;
        conflicting_origin.normalized_occurrence = fresh.normalized_occurrence;
        EXPECT_EQ(
            std::get<OpenRefusal>(open_chain(conflicting_origin, reduced, admitted, owner.view())),
            OpenRefusal::conflict);
        EXPECT_TRUE(owner.mapped.empty());
        EXPECT_TRUE(owner.handled.empty());

        auto retained = std::get<OpenedChain>(open_chain(r, reduced, admitted, owner.view()));
        EXPECT_EQ(retained.committed_children, 0U);
        EXPECT_FALSE(retained.cutoff);
        const auto pub = owner.run(retained.chain, r.event, "pass-R",
                                   {{"emit-child", {proposed, "leaf"}}}, deny ? "emit-child" : "");
        ASSERT_EQ(pub.pass->rules.size(), 1U);
        ASSERT_EQ(pub.pass->rules.front().accepted.size(), 1U);
        expect_time(pub.pass->rules.front().accepted.front(), key("R-rule", "plus5"), 5, 5);
        EXPECT_EQ(owner.handled, (std::vector{key("R-rule", "plus5")}));
        if (deny) {
            expect_use_stop(pub.pass->rules.front(), 1);
            EXPECT_TRUE(owner.offered.empty());
            EXPECT_TRUE(pub.named_suppressions.empty());
        } else {
            expect_owner_stop(pub.pass->rules.front(), 1, "retained_policy_exceeds_profile");
            EXPECT_EQ(owner.offered, (std::vector{action}));
            ASSERT_EQ(pub.named_suppressions.size(), 1U);
            EXPECT_EQ(pub.named_suppressions.front().action, action);
            EXPECT_TRUE(std::holds_alternative<RetainedPolicyExceedsProfile>(
                pub.named_suppressions.front().stop));
        }
        EXPECT_EQ(owner.time, 5);
        EXPECT_TRUE(pub.children.empty());
        EXPECT_FALSE(pub.cutoff);
        EXPECT_TRUE(owner.notices.empty());
        EXPECT_TRUE(std::holds_alternative<NotRecorded>(lookup_child(retained.chain, proposed)));
        EXPECT_EQ(owner.configurations.at("C").data().generation.maximum_depth, 41U);
        EXPECT_EQ(owner.configurations.at("C").data().generation.maximum_generated_events, 3U);
    }
}

TEST(Rules004, InvalidProfilePrecedesSelectionAndCapacityIsAnOwnerHold) {
    Owner owner;
    const auto &configuration = owner.define("C", {}, {});
    owner.active = "C";
    const auto r = root();
    owner.keep(r.event, "seed");

    auto invalid = profile;
    invalid.maximum.maximum_generated_events = 255;
    EXPECT_EQ(check_generation_policy(configuration, invalid), PolicyCheck::invalid_profile);
    EXPECT_EQ(std::get<OpenRefusal>(open_chain(r, invalid, Unavailable{}, owner.view())),
              OpenRefusal::invalid_profile);
    invalid = profile;
    invalid.active_roots = 0;
    EXPECT_EQ(check_generation_policy(configuration, invalid), PolicyCheck::invalid_profile);
    invalid = profile;
    invalid.pending_events = 0;
    EXPECT_EQ(check_generation_policy(configuration, invalid), PolicyCheck::invalid_profile);
    invalid = profile;
    invalid.owned_bytes = 0;
    EXPECT_EQ(check_generation_policy(configuration, invalid), PolicyCheck::invalid_profile);

    owner.other.active_roots = profile.active_roots;
    EXPECT_EQ(std::get<OpenRefusal>(owner.open(r)), OpenRefusal::capacity_hold);
    owner.other = {};
    owner.other.pending_events = profile.pending_events;
    EXPECT_EQ(std::get<OpenRefusal>(owner.open(r)), OpenRefusal::capacity_hold);
    owner.other = {};

    auto tight = profile;
    tight.owned_bytes = 1;
    const ModeledHistory retained{r, "admit-R", profile, {{r.event, 0, 0}}, {}, {}, {}};
    EXPECT_EQ(std::get<OpenRefusal>(open_chain(r, tight, retained, owner.view())),
              OpenRefusal::capacity_hold);
    const auto reopened = std::get<OpenedChain>(open_chain(r, profile, retained, owner.view()));
    EXPECT_EQ(reopened.committed_children, 0U);
    EXPECT_FALSE(reopened.cutoff);
    EXPECT_TRUE(owner.mapped.empty());
    EXPECT_TRUE(owner.notices.empty());
}

TEST(Rules004, PendingCeilingCountsProvisionalSlotsWithoutGenerationCutoff) {
    Owner owner;
    owner.define("C", {}, {named_rule("F", "seed", {emit("one", "leaf"), emit("two", "leaf")})});
    owner.active = "C";
    const auto r = root();
    const auto first_action = key("F", "one");
    const auto second_action = key("F", "two", 1);
    const auto c1 = child(r.event, "E1", NamedAction{first_action});
    const auto c2 = child(r.event, "E2", NamedAction{second_action});
    owner.keep(r.event, "seed");
    owner.keep(c1, "leaf");
    owner.keep(c2, "leaf");
    auto tight = profile;
    tight.pending_events = 2;
    auto opened = std::get<OpenedChain>(owner.open(r, tight));
    auto unit = std::get<Unit>(begin_event(opened.chain, r.event, "pass-R"));
    const auto plan = plan_event(unit, owner.mapper());
    std::vector<ChildDecision> decisions;
    const auto pass = consume_rule_actions(
        plan, owner.checks(""),
        [&](const RuleEventLineage &, const RuleActionKey &action, const RuleBoundAction &bound,
            std::string_view) -> RuleOwnerResult {
            const auto &proposed = action == first_action ? c1 : c2;
            decisions.push_back(
                offer_named_child(unit, proposed, action, std::get<RuleNamedEvent>(bound.payload)));
            if (const auto *accepted = std::get_if<Reserved>(&decisions.back())) {
                RuleOwnerResult result = RuleOwnerAcceptance{lineage(proposed)};
                EXPECT_EQ(confirm_named_child(unit, accepted->event, NamedOutcome{result}),
                          NamedConfirmation::verified);
                return result;
            }
            return RuleOwnerRefusal{"fixture-cannot-hold"};
        });
    ASSERT_TRUE(std::holds_alternative<RuleStagedPass>(pass));
    ASSERT_EQ(decisions.size(), 2U);
    EXPECT_EQ(std::get<Reserved>(decisions.front()).event, (AdmittedEvent{c1, 1, 1}));
    EXPECT_TRUE(std::holds_alternative<CapacityHold>(decisions.back()));
    EXPECT_TRUE(std::holds_alternative<RolledBack>(
        finish_unit(std::move(unit), std::get<RuleStagedPass>(pass), RuleCompletion::rolled_back)));
    EXPECT_TRUE(std::holds_alternative<NotRecorded>(lookup_child(opened.chain, c1)));
    EXPECT_TRUE(std::holds_alternative<NotRecorded>(lookup_child(opened.chain, c2)));
    EXPECT_TRUE(owner.notices.empty());
}

} // namespace
