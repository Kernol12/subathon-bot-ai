// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

namespace {
using namespace rules_004;

struct Transaction {
    Owner owner;
    const RootBinding binding = root();
    const EventBinding e1 = child(binding.event, "E1", NamedAction{key("A", "emit-b")});
    const EventBinding e2 = child(e1, "E2", NamedAction{key("B", "emit-c", 1)});
    const UnitIdentity attempt = event_unit("pass-E1", e1);
    const Cutoff stop{{"I", "R"}, "CR", {e2, 2, 2}, {3, 2}, CutoffReason::count};

    OpenedChain start(bool select_later_rule = false) {
        auto rules = std::vector{
            named_rule("A", "a", {emit("emit-b", "b")}),
            named_rule("B", "b", {delta("plus5", "5"), emit("emit-c", "c"), delta("zero", "0")})};
        if (select_later_rule)
            rules.push_back(named_rule("Later", "b", {delta("later", "7")}));
        owner.define("C", {3, 2}, std::move(rules));
        owner.active = "C";
        owner.keep(binding.event, "a");
        owner.keep(e1, "b");
        owner.keep(e2, "c");
        auto opened = std::get<OpenedChain>(owner.open(binding));
        const auto first =
            owner.run(opened.chain, binding.event, "pass-R", {{"emit-b", {e1, "b"}}});
        EXPECT_EQ(first.children, (std::vector{AdmittedEvent{e1, 1, 1}}));
        EXPECT_FALSE(first.cutoff);
        return opened;
    }

    RulePass stage(Unit &unit, NamedFault fault = NamedFault::none, std::string numeric = {}) {
        const auto plan = plan_event(unit, owner.mapper());
        EXPECT_EQ(plan.event, lineage(e1));
        return owner.consume(unit, e1, plan, {{"emit-c", {e2, "c"}}}, {}, fault, numeric);
    }

    void expect_second(const Publication &publication, const std::string &unit = "pass-E1") const {
        EXPECT_EQ(publication.identity, event_unit(unit, e1));
        EXPECT_EQ(publication.children, (std::vector{AdmittedEvent{e2, 2, 2}}));
        ASSERT_TRUE(publication.cutoff);
        expect_cutoff(*publication.cutoff, stop);
        ASSERT_TRUE(publication.pass);
        ASSERT_EQ(publication.pass->rules.size(), 1U);
        const auto &rule = publication.pass->rules.front();
        EXPECT_EQ(rule.captured_index, 1U);
        EXPECT_FALSE(rule.stop);
        ASSERT_EQ(rule.accepted.size(), 3U);
        expect_time(rule.accepted[0], key("B", "plus5"), 5, 5);
        EXPECT_EQ(rule.accepted[1].key, key("B", "emit-c", 1));
        EXPECT_EQ(std::get<RuleEventLineage>(rule.accepted[1].observed), lineage(e2));
        expect_time(rule.accepted[2], key("B", "zero", 2), 0, 5);
    }

    ModeledHistory committed_readback() const {
        const RuleCommittedPass root_pass{{{0, {{key("A", "emit-b"), lineage(e1)}}, {}}}};
        const RuleCommittedPass child_pass{{{1,
                                             {{key("B", "plus5"), time_effect("plus5", 5, 5)},
                                              {key("B", "emit-c", 1), lineage(e2)},
                                              {key("B", "zero", 2), time_effect("zero", 0, 5)}},
                                             {}}}};
        return {binding,
                "admit-R",
                profile,
                {{binding.event, 0, 0}, {e1, 1, 1}, {e2, 2, 2}},
                {{event_unit("pass-R", binding.event), root_pass, {{e1, 1, 1}}, {}, {}, {}},
                 {attempt, child_pass, {{e2, 2, 2}}, {}, {}, stop}},
                stop,
                {}};
    }

    void expect_prefix() const {
        EXPECT_EQ(owner.time, 0);
        EXPECT_EQ(owner.queued, (std::vector{AdmittedEvent{e1, 1, 1}}));
        EXPECT_TRUE(owner.audits.empty());
        EXPECT_TRUE(owner.notices.empty());
    }

    void retry(Chain &chain) {
        expect_prefix();
        EXPECT_TRUE(std::holds_alternative<NotRecorded>(lookup_child(chain, e2)));
        const auto fresh = owner.run(chain, e1, "retry-E1", {{"emit-c", {e2, "c"}}});
        expect_second(fresh, "retry-E1");
        EXPECT_EQ(owner.time, 5);
        EXPECT_EQ(owner.notices, (std::vector{std::pair{ChainKey{"I", "R"}, 2U}}));
    }
};

TEST(Rules004, G6CommitFinalizesEffectsChildChargeAndCutoffTogether) {
    Transaction f;
    auto opened = f.start();
    auto unit = std::get<Unit>(begin_event(opened.chain, f.e1, f.attempt.unit));
    const auto staged = std::get<RuleStagedPass>(f.stage(unit));
    f.expect_prefix();
    const auto pub =
        std::get<Publication>(finish_unit(std::move(unit), staged, RuleCompletion::committed));
    f.expect_second(pub);
    f.owner.publish(pub);
    EXPECT_EQ(f.owner.time, 5);
    EXPECT_EQ(f.owner.queued, (std::vector{AdmittedEvent{f.e1, 1, 1}, AdmittedEvent{f.e2, 2, 2}}));
    EXPECT_EQ(f.owner.audits.size(), 1U);
    EXPECT_EQ(f.owner.notices, (std::vector{std::pair{ChainKey{"I", "R"}, 2U}}));
}

TEST(Rules004, G6NumericHoldAndRollbackPreserveOnlyEarlierCommit) {
    for (const bool numeric_hold : {false, true}) {
        SCOPED_TRACE(numeric_hold);
        Transaction f;
        auto opened = f.start();
        auto unit = std::get<Unit>(begin_event(opened.chain, f.e1, f.attempt.unit));
        auto pass = f.stage(unit, NamedFault::none, numeric_hold ? "zero" : "");
        EXPECT_EQ(f.owner.handled, (std::vector{key("A", "emit-b"), key("B", "plus5"),
                                                key("B", "emit-c", 1), key("B", "zero", 2)}));
        std::optional<RuleStagedPass> staged;
        if (numeric_hold)
            EXPECT_EQ(std::get<RulePassFailure>(pass), RulePassFailure::numeric_hold);
        else
            staged = std::get<RuleStagedPass>(std::move(pass));
        EXPECT_TRUE(std::holds_alternative<RolledBack>(
            finish_unit(std::move(unit), staged, RuleCompletion::rolled_back)));
        f.retry(opened.chain);
    }
}

TEST(Rules004, G6UncertainCompletionWaitsForExactOwnerReadback) {
    for (const auto completion :
         {RuleCompletion::outcome_unknown, RuleCompletion::abnormal_completion}) {
        for (const bool committed : {false, true}) {
            SCOPED_TRACE(static_cast<int>(completion));
            SCOPED_TRACE(committed);
            Transaction f;
            auto opened = f.start();
            auto unit = std::get<Unit>(begin_event(opened.chain, f.e1, f.attempt.unit));
            const auto staged = std::get<RuleStagedPass>(f.stage(unit));
            EXPECT_TRUE(std::holds_alternative<Unresolved>(
                finish_unit(std::move(unit), staged, completion)));
            f.expect_prefix();
            const auto before = f.owner.handled;
            EXPECT_TRUE(std::holds_alternative<Unresolved>(
                reconcile_unit(opened.chain, f.attempt.unit, Unavailable{})));
            EXPECT_EQ(std::get<BeginDisposition>(begin_event(opened.chain, f.e1, "repeat")),
                      BeginDisposition::in_flight_hold);
            EXPECT_EQ(f.owner.handled, before);

            if (committed) {
                const auto readback = f.committed_readback();
                const auto &owner_outcome = readback.publications.back();
                // The owner has published by the time its committed state is selected.
                f.owner.publish(owner_outcome);
                const auto pub =
                    std::get<Publication>(reconcile_unit(opened.chain, f.attempt.unit, readback));
                f.expect_second(pub);
                expect_duplicate(opened.chain, {f.e2, 2, 2});
                EXPECT_EQ(std::get<BeginDisposition>(begin_event(opened.chain, f.e1, "repeat")),
                          BeginDisposition::duplicate_event);
                EXPECT_EQ(f.owner.handled, before);
                EXPECT_EQ(f.owner.time, 5);
                EXPECT_EQ(f.owner.queued.size(), 2U);
                EXPECT_EQ(f.owner.notices.size(), 1U);
            } else {
                EXPECT_TRUE(std::holds_alternative<RolledBack>(
                    reconcile_unit(opened.chain, f.attempt.unit, f.attempt)));
                f.retry(opened.chain);
            }
        }
    }
}

TEST(Rules004, G6bNamedFaultLatchesDespiteOrdinaryCommittedReport) {
    for (const auto fault : {NamedFault::unknown, NamedFault::wrong_parent}) {
        const bool unknown = fault == NamedFault::unknown;
        SCOPED_TRACE(unknown);
        Transaction f;
        auto opened = f.start();
        auto unit = std::get<Unit>(begin_event(opened.chain, f.e1, f.attempt.unit));
        const auto staged = std::get<RuleStagedPass>(f.stage(unit, fault));
        ASSERT_EQ(staged.rules.size(), 1U);
        const auto &rule = staged.rules.front();
        ASSERT_EQ(rule.accepted.size(), 1U);
        expect_time(rule.accepted.front(), key("B", "plus5"), 5, 5);
        expect_owner_stop(rule, 1,
                          unknown ? "generated_event_outcome_unknown"
                                  : "invalid_generated_event_observation");
        EXPECT_EQ(f.owner.handled,
                  (std::vector{key("A", "emit-b"), key("B", "plus5"), key("B", "emit-c", 1)}));
        const auto result = finish_unit(std::move(unit), staged, RuleCompletion::committed);
        if (unknown)
            EXPECT_TRUE(std::holds_alternative<Unresolved>(result));
        else
            EXPECT_TRUE(std::holds_alternative<Conflict>(result));
        f.expect_prefix();
        EXPECT_EQ(std::get<BeginDisposition>(begin_event(opened.chain, f.e1, "repeat")),
                  BeginDisposition::in_flight_hold);
        EXPECT_TRUE(std::holds_alternative<RolledBack>(
            reconcile_unit(opened.chain, f.attempt.unit, f.attempt)));
        f.retry(opened.chain);
    }
}

TEST(Rules004, KnownNamedRefusalReleasesProspectiveCutoff) {
    Transaction f;
    auto opened = f.start();
    auto unit = std::get<Unit>(begin_event(opened.chain, f.e1, f.attempt.unit));
    const auto staged = std::get<RuleStagedPass>(f.stage(unit, NamedFault::refused));
    const auto pub =
        std::get<Publication>(finish_unit(std::move(unit), staged, RuleCompletion::committed));
    ASSERT_TRUE(pub.pass);
    ASSERT_EQ(pub.pass->rules.size(), 1U);
    ASSERT_EQ(pub.pass->rules.front().accepted.size(), 1U);
    expect_owner_stop(pub.pass->rules.front(), 1, "owner_declined");
    EXPECT_TRUE(pub.children.empty());
    EXPECT_TRUE(pub.named_suppressions.empty());
    EXPECT_FALSE(pub.cutoff);
    f.owner.publish(pub);
    EXPECT_EQ(f.owner.time, 5);
    EXPECT_EQ(f.owner.queued, (std::vector{AdmittedEvent{f.e1, 1, 1}}));
    EXPECT_TRUE(f.owner.notices.empty());
    EXPECT_TRUE(std::holds_alternative<NotRecorded>(lookup_child(opened.chain, f.e2)));
}

TEST(Rules004, LatchedNamedOutcomeRefusesLaterSelectedRuleWithoutCallingItsOwner) {
    Transaction f;
    auto opened = f.start(true);
    auto unit = std::get<Unit>(begin_event(opened.chain, f.e1, f.attempt.unit));
    const auto staged = std::get<RuleStagedPass>(f.stage(unit, NamedFault::unknown));
    ASSERT_EQ(staged.rules.size(), 2U);
    expect_owner_stop(staged.rules[0], 1, "generated_event_outcome_unknown");
    EXPECT_EQ(staged.rules[1].captured_index, 2U);
    EXPECT_TRUE(staged.rules[1].accepted.empty());
    expect_owner_stop(staged.rules[1], 0, "generated_event_unit_held");
    EXPECT_EQ(f.owner.handled,
              (std::vector{key("A", "emit-b"), key("B", "plus5"), key("B", "emit-c", 1)}));
    EXPECT_TRUE(std::holds_alternative<Unresolved>(
        finish_unit(std::move(unit), staged, RuleCompletion::committed)));
    f.expect_prefix();
    EXPECT_TRUE(std::holds_alternative<RolledBack>(
        reconcile_unit(opened.chain, f.attempt.unit, f.attempt)));
    f.expect_prefix();
}

struct Siblings {
    Owner owner;
    const RootBinding binding = root();
    const EventBinding a = child(binding.event, "A", NamedAction{key("Seed", "emit-a")});
    const EventBinding b = child(binding.event, "B", NamedAction{key("Seed", "emit-b", 1)});
    const EventBinding a2 = child(a, "A2", NamedAction{key("A", "emit-a2")});
    const Cutoff stop{{"I", "R"}, "CR", {a2, 2, 3}, {2, 8}, CutoffReason::depth};

    OpenedChain start(std::vector<RuleDefinition> b_rules) {
        auto other =
            std::vector{named_rule("Seed", "seed", {emit("emit-a", "a"), emit("emit-b", "b")}),
                        named_rule("A", "a", {emit("emit-a2", "a2")})};
        other.insert(other.end(), b_rules.begin(), b_rules.end());
        owner.define("C", {2, 8}, std::move(other));
        owner.active = "C";
        owner.keep(binding.event, "seed");
        owner.keep(a, "a");
        owner.keep(b, "b");
        owner.keep(a2, "a2");
        auto opened = std::get<OpenedChain>(owner.open(binding));
        const auto initial = owner.run(opened.chain, binding.event, "pass-R",
                                       {{"emit-a", {a, "a"}}, {"emit-b", {b, "b"}}});
        EXPECT_EQ(initial.children, (std::vector{AdmittedEvent{a, 1, 1}, AdmittedEvent{b, 1, 2}}));
        EXPECT_FALSE(initial.cutoff);
        const auto next = owner.run(opened.chain, a, "pass-A", {{"emit-a2", {a2, "a2"}}});
        EXPECT_EQ(next.children, (std::vector{AdmittedEvent{a2, 2, 3}}));
        EXPECT_TRUE(next.cutoff);
        if (next.cutoff)
            expect_cutoff(*next.cutoff, stop);
        return opened;
    }
};

TEST(Rules004, G7WholeRootCutoffSuppressesShallowerSiblingAndOtherRootContinues) {
    Siblings f;
    auto opened = f.start({named_rule("B", "b", {emit("emit-b2", "b2")})});
    const auto b2 = child(f.b, "B2", NamedAction{key("B", "emit-b2")});
    f.owner.keep(b2, "b2");
    const auto sibling = f.owner.run(opened.chain, f.b, "pass-B", {{"emit-b2", {b2, "b2"}}});
    expect_suppression(sibling, key("B", "emit-b2"), f.stop);
    expect_owner_stop(sibling.pass->rules.at(0), 0, "generation_cutoff");

    f.owner.define("CZ", {}, {named_rule("Z", "z", {emit("emit-z1", "z1")})});
    f.owner.active = "CZ";
    const auto z = root("Z", "CZ-root");
    const auto z1 = child(z.event, "Z1", NamedAction{key("Z", "emit-z1")});
    f.owner.keep(z.event, "z");
    f.owner.keep(z1, "z1");
    auto independent = std::get<OpenedChain>(f.owner.open(z));
    EXPECT_EQ(independent.committed_children, 0U);
    const auto pass = f.owner.run(independent.chain, z.event, "pass-Z", {{"emit-z1", {z1, "z1"}}});
    EXPECT_EQ(pass.children, (std::vector{AdmittedEvent{z1, 1, 1}}));
    EXPECT_FALSE(pass.cutoff);
    const auto final_child = f.owner.run(opened.chain, f.a2, "pass-A2");
    EXPECT_TRUE(final_child.pass->rules.empty());
    EXPECT_FALSE(final_child.cutoff);
    EXPECT_EQ(f.owner.notices, (std::vector{std::pair{ChainKey{"I", "R"}, 3U}}));
    EXPECT_EQ(f.owner.queued, (std::vector{AdmittedEvent{f.a, 1, 1}, AdmittedEvent{f.b, 1, 2},
                                           AdmittedEvent{f.a2, 2, 3}, AdmittedEvent{z1, 1, 1}}));
}

TEST(Rules004, G8EarlierUseDenialWinsAndOrdinaryRefusalPreservesOtherRules) {
    for (const bool deny : {false, true}) {
        SCOPED_TRACE(deny);
        Siblings f;
        auto opened =
            f.start({named_rule("B", "b",
                                {delta("plus5", "5"), emit("emit-b2", "b2"), delta("plus7", "7")}),
                     named_rule("Other", "b", {delta("plus11", "11")})});
        const auto action = key("B", "emit-b2", 1);
        const auto b2 = child(f.b, "B2", NamedAction{action});
        f.owner.keep(b2, "b2");
        f.owner.calls.clear();
        const auto pass = f.owner.run(opened.chain, f.b, "pass-B", {{"emit-b2", {b2, "b2"}}},
                                      deny ? "emit-b2" : "");
        ASSERT_EQ(pass.pass->rules.size(), 2U);
        const auto &b = pass.pass->rules[0];
        const auto &other = pass.pass->rules[1];
        EXPECT_EQ(b.captured_index, 2U);
        EXPECT_EQ(other.captured_index, 3U);
        ASSERT_EQ(b.accepted.size(), 1U);
        ASSERT_EQ(other.accepted.size(), 1U);
        expect_time(b.accepted.front(), key("B", "plus5"), 5, 5);
        expect_time(other.accepted.front(), key("Other", "plus11"), 11, 16);
        EXPECT_FALSE(other.stop);
        auto expected_calls = std::vector<std::string>{
            "authority:plus5", "execution:timer.adjust", "target:timer.adjust",
            "capability:timer.adjust", "authority:emit-b2"};
        if (deny) {
            expect_use_stop(b, 1);
            EXPECT_TRUE(pass.named_suppressions.empty());
        } else {
            expect_owner_stop(b, 1, "generation_cutoff");
            expect_suppression(pass, action, f.stop);
            expected_calls.insert(expected_calls.end(),
                                  {"execution:event.emit", "target:event.emit",
                                   "capability:event.emit", "offer:emit-b2"});
        }
        expected_calls.insert(expected_calls.end(),
                              {"authority:plus11", "execution:timer.adjust", "target:timer.adjust",
                               "capability:timer.adjust"});
        EXPECT_EQ(f.owner.calls, expected_calls);
        EXPECT_EQ(f.owner.handled,
                  (std::vector{key("Seed", "emit-a"), key("Seed", "emit-b", 1), key("A", "emit-a2"),
                               key("B", "plus5"), key("Other", "plus11")}));
        EXPECT_EQ(f.owner.time, 16);
        EXPECT_TRUE(pass.children.empty());
        EXPECT_FALSE(pass.cutoff);
        EXPECT_EQ(f.owner.queued.size(), 3U);
        EXPECT_EQ(f.owner.notices.size(), 1U);
    }
}

} // namespace
