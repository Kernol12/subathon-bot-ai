// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

namespace {
using namespace rules_004;

TEST(Rules004, G1CycleStopsAtDepth32) {
    Owner owner;
    const auto &configuration = owner.define("C", {},
                                             {named_rule("P", "ping", {emit("to-pong", "pong")}),
                                              named_rule("Q", "pong", {emit("to-ping", "ping")})});
    EXPECT_EQ(configuration.data().generation.maximum_depth, 32U);
    EXPECT_EQ(configuration.data().generation.maximum_generated_events, 256U);
    owner.active = "C";
    const auto binding = root();
    owner.keep(binding.event, "ping");
    auto opened = std::get<OpenedChain>(owner.open(binding));
    EXPECT_EQ(opened.committed_children, 0U);
    EXPECT_FALSE(opened.cutoff);

    auto parent = binding.event;
    for (std::uint32_t n = 1; n <= 32; ++n) {
        SCOPED_TRACE(n);
        const auto action = n % 2 == 1 ? key("P", "to-pong") : key("Q", "to-ping");
        const std::string name = n % 2 == 1 ? "pong" : "ping";
        const auto next = child(parent, "E" + std::to_string(n), NamedAction{action});
        owner.keep(next, name);
        const auto result = owner.run(opened.chain, parent, "pass-" + parent.event,
                                      {{action.action_ref, {next, name}}});
        EXPECT_EQ(result.children, (std::vector{AdmittedEvent{next, n, n}}));
        ASSERT_EQ(result.pass->rules.size(), 1U);
        ASSERT_EQ(result.pass->rules.front().accepted.size(), 1U);
        EXPECT_EQ(result.pass->rules.front().accepted.front().key, action);
        EXPECT_EQ(std::get<RuleEventLineage>(result.pass->rules.front().accepted.front().observed),
                  lineage(next));
        EXPECT_FALSE(result.pass->rules.front().stop);
        EXPECT_EQ(result.cutoff.has_value(), n == 32);
        parent = next;
    }

    const auto final_action = key("P", "to-pong");
    const auto e33 = child(parent, "E33", NamedAction{final_action});
    owner.keep(e33, "pong");
    const auto result = owner.run(opened.chain, parent, "pass-E32", {{"to-pong", {e33, "pong"}}});
    ASSERT_EQ(result.pass->rules.size(), 1U);
    expect_owner_stop(result.pass->rules.front(), 0, "generation_cutoff");
    const Cutoff expected{
        {"I", "R"},
        "CR",
        {{{"I", "R"}, "E32", "E31", "CE32", NamedAction{key("Q", "to-ping")}}, 32, 32},
        {32, 256},
        CutoffReason::depth};
    expect_suppression(result, final_action, expected);
    ASSERT_EQ(owner.audits.size(), 1U);
    expect_cutoff(owner.audits.front(), expected);
    EXPECT_EQ(owner.notices, (std::vector{std::pair{ChainKey{"I", "R"}, 32U}}));
    EXPECT_EQ(owner.handled.size(), 32U);
    EXPECT_EQ(owner.queued.size(), 32U);
    EXPECT_TRUE(std::holds_alternative<NotRecorded>(lookup_child(opened.chain, e33)));
}

TEST(Rules004, G2FanOutStopsAtCount256WithNoLeafMatches) {
    Owner owner;
    const auto binding = root();
    std::vector<RuleActionDefinition> actions;
    std::map<std::string, Candidate> candidates;
    std::vector<AdmittedEvent> expected;
    for (std::uint32_t n = 1; n <= 257; ++n) {
        auto id = "A" + std::to_string(n);
        actions.push_back(emit(id, "leaf"));
        auto next = child(binding.event, "C" + std::to_string(n), NamedAction{key("F", id, n - 1)});
        candidates.emplace(id, Candidate{next, "leaf"});
        if (n <= 256)
            expected.push_back({next, 1, n});
    }
    owner.define("C", {}, {named_rule("F", "seed", std::move(actions))});
    owner.active = "C";
    owner.keep(binding.event, "seed");
    for (const auto &[_, candidate] : candidates)
        owner.keep(candidate.binding, "leaf");
    auto opened = std::get<OpenedChain>(owner.open(binding));
    const auto pass = owner.run(opened.chain, binding.event, "pass-R", candidates);
    EXPECT_EQ(pass.children, expected);
    EXPECT_EQ(owner.queued, expected);
    ASSERT_EQ(pass.pass->rules.size(), 1U);
    const auto &rule = pass.pass->rules.front();
    ASSERT_EQ(rule.accepted.size(), 256U);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(rule.accepted[i].key, key("F", "A" + std::to_string(i + 1), i));
        EXPECT_EQ(std::get<RuleEventLineage>(rule.accepted[i].observed),
                  lineage(expected[i].binding));
    }
    expect_owner_stop(rule, 256, "generation_cutoff");
    const Cutoff expected_stop{
        binding.event.chain, "CR", expected.back(), {32, 256}, CutoffReason::count};
    ASSERT_TRUE(pass.cutoff);
    expect_cutoff(*pass.cutoff, expected_stop);
    ASSERT_EQ(pass.named_suppressions.size(), 1U);
    EXPECT_EQ(pass.named_suppressions.front().action, key("F", "A257", 256));
    expect_cutoff(std::get<Cutoff>(pass.named_suppressions.front().stop), expected_stop);
    EXPECT_EQ(owner.handled.size(), 256U);
    EXPECT_TRUE(std::holds_alternative<NotRecorded>(
        lookup_child(opened.chain, candidates.at("A257").binding)));

    const auto mapped = owner.mapped.size();
    for (const auto &leaf : expected) {
        const auto result = owner.run(opened.chain, leaf.binding, "pass-" + leaf.binding.event);
        EXPECT_TRUE(result.pass->rules.empty());
        EXPECT_TRUE(result.children.empty());
        EXPECT_FALSE(result.cutoff);
    }
    EXPECT_EQ(owner.mapped.size(), mapped);
    EXPECT_EQ(owner.queued, expected);
    EXPECT_EQ(owner.audits.size(), 1U);
    EXPECT_EQ(owner.notices, (std::vector{std::pair{ChainKey{"I", "R"}, 256U}}));
}

struct CapturedPolicy {
    Owner owner;
    const RootBinding binding = root();
    const EventBinding e1 = child(binding.event, "E1", NamedAction{key("S", "emit-step")});
    const EventBinding e2 = child(e1, "E2", NamedAction{key("T", "emit-end")});
    const EventBinding e3 = child(e2, "E3", NamedAction{key("U", "emit-over")});
    const Cutoff stop{{"I", "R"}, "CR", {e2, 2, 2}, {2, 2}, CutoffReason::both};

    OpenedChain start() {
        owner.define("C1", {2, 2}, {named_rule("S", "seed", {emit("emit-step", "step")})});
        owner.define("C2", {4, 8},
                     {named_rule("T", "step", {emit("emit-end", "end")}),
                      named_rule("U", "end", {emit("emit-over", "over")})});
        owner.define("C3", {1, 1}, {});
        EXPECT_EQ(owner.activate("C1", profile), PolicyCheck::supported);
        owner.keep(binding.event, "seed");
        auto opened = std::get<OpenedChain>(owner.open(binding));
        EXPECT_EQ(opened.committed_children, 0U);
        EXPECT_EQ(owner.activate("C2", profile), PolicyCheck::supported);
        owner.keep(e1, "step");
        owner.keep(e2, "end");
        return opened;
    }

    void first(Chain &chain, bool replay = false) {
        auto unit = std::get<Unit>(begin_event(chain, binding.event, "pass-R"));
        if (replay) {
            EXPECT_EQ(std::get<BeginDisposition>(begin_event(chain, binding.event, "repeat")),
                      BeginDisposition::in_flight_hold);
            EXPECT_TRUE(owner.mapped.empty());
        }
        const auto plan = plan_event(unit, owner.mapper());
        EXPECT_EQ(plan.selection.configuration_ref, "C1");
        EXPECT_EQ(plan.event, lineage(binding.event));
        const auto staged = std::get<RuleStagedPass>(
            owner.consume(unit, binding.event, plan, {{"emit-step", {e1, "step"}}}));
        auto pub =
            std::get<Publication>(finish_unit(std::move(unit), staged, RuleCompletion::committed));
        EXPECT_EQ(pub.identity, event_unit("pass-R", binding.event));
        EXPECT_EQ(pub.children, (std::vector{AdmittedEvent{e1, 1, 1}}));
        EXPECT_FALSE(pub.cutoff);
        owner.publish(pub);
    }

    void second(Chain &chain) {
        const auto pub = owner.run(chain, e1, "pass-E1", {{"emit-end", {e2, "end"}}});
        EXPECT_EQ(pub.children, (std::vector{AdmittedEvent{e2, 2, 2}}));
        ASSERT_TRUE(pub.cutoff);
        expect_cutoff(*pub.cutoff, stop);
    }

    void third(Chain &chain) {
        EXPECT_EQ(owner.activate("C3", profile), PolicyCheck::supported);
        owner.keep(e3, "over");
        const auto pub = owner.run(chain, e2, "pass-E2", {{"emit-over", {e3, "over"}}});
        ASSERT_EQ(pub.pass->rules.size(), 1U);
        EXPECT_EQ(pub.pass->rules.front().captured_index, 1U);
        expect_owner_stop(pub.pass->rules.front(), 0, "generation_cutoff");
        expect_suppression(pub, key("U", "emit-over"), stop);
        EXPECT_EQ(owner.mapped, (std::vector{key("S", "emit-step"), key("T", "emit-end"),
                                             key("U", "emit-over")}));
        EXPECT_EQ(owner.handled, (std::vector{key("S", "emit-step"), key("T", "emit-end")}));
    }

    // The modeled readback is specified independently of the candidate's publications.
    ModeledHistory readback() const {
        RuleStopDetail detail{};
        detail.opaque_reason = "generation_cutoff";
        const RuleCommittedPass r{{{0, {{key("S", "emit-step"), lineage(e1)}}, {}}}};
        const RuleCommittedPass first_child{{{0, {{key("T", "emit-end"), lineage(e2)}}, {}}}};
        const RuleCommittedPass last_child{
            {{1, {}, RuleStop{0, RuleActionRefusal::handler_refusal, detail}}}};
        return {
            binding,
            "admit-R",
            profile,
            {{binding.event, 0, 0}, {e1, 1, 1}, {e2, 2, 2}},
            {{event_unit("pass-R", binding.event), r, {{e1, 1, 1}}, {}, {}, {}},
             {event_unit("pass-E1", e1), first_child, {{e2, 2, 2}}, {}, {}, stop},
             {event_unit("pass-E2", e2), last_child, {}, {}, {{key("U", "emit-over"), stop}}, {}}},
            stop,
            {}};
    }
};

TEST(Rules004, G3RootLimitsAndChildCapturesStayImmutable) {
    CapturedPolicy f;
    auto opened = f.start();
    f.first(opened.chain);
    f.second(opened.chain);
    f.third(opened.chain);
    EXPECT_EQ(f.owner.audits.size(), 1U);
    EXPECT_EQ(f.owner.notices, (std::vector{std::pair{ChainKey{"I", "R"}, 2U}}));
    EXPECT_EQ(f.owner.queued, (std::vector{AdmittedEvent{f.e1, 1, 1}, AdmittedEvent{f.e2, 2, 2}}));
}

TEST(Rules004, G4ReplayChecksExactBindingBeforeCutoffAndPlanning) {
    CapturedPolicy f;
    auto opened = f.start();
    f.first(opened.chain, true);
    const auto expect_replayed_root = [&] {
        const auto calls = f.owner.calls;
        const auto mapped = f.owner.mapped;
        EXPECT_EQ(std::get<BeginDisposition>(begin_event(opened.chain, f.binding.event, "repeat")),
                  BeginDisposition::duplicate_event);
        EXPECT_EQ(f.owner.calls, calls);
        EXPECT_EQ(f.owner.mapped, mapped);
    };
    expect_replayed_root();
    f.second(opened.chain);
    expect_replayed_root();
    expect_duplicate(opened.chain, {f.e1, 1, 1});
    expect_duplicate(opened.chain, {f.e2, 2, 2});

    auto changed_capture = f.e1;
    changed_capture.capture = "CE1-other";
    f.owner.keep(changed_capture, "step");
    auto changed_parent = f.e1;
    changed_parent.parent = f.e2.event;
    auto changed_cause = f.e1;
    auto &action = std::get<NamedAction>(*changed_cause.cause).key;
    action.action_version = "v2";
    auto changed_child = f.e1;
    changed_child.event = "fresh";
    changed_child.capture = "CF";
    f.owner.keep(changed_child, "step");
    for (const auto &changed : {changed_capture, changed_parent, changed_cause, changed_child})
        EXPECT_TRUE(std::holds_alternative<Conflict>(lookup_child(opened.chain, changed)));
    for (const auto &changed : {changed_capture, changed_parent, changed_cause})
        EXPECT_EQ(std::get<BeginDisposition>(begin_event(opened.chain, changed, "changed")),
                  BeginDisposition::conflict);
    EXPECT_EQ(f.owner.queued, (std::vector{AdmittedEvent{f.e1, 1, 1}, AdmittedEvent{f.e2, 2, 2}}));
    EXPECT_EQ(f.owner.handled, (std::vector{key("S", "emit-step"), key("T", "emit-end")}));
    EXPECT_EQ(f.owner.notices.size(), 1U);
}

TEST(Rules004, G10RetainedHistoryRestoresExactCutoffAndFinishedClaims) {
    CapturedPolicy f;
    {
        auto opened = f.start();
        f.first(opened.chain);
        f.second(opened.chain);
        f.third(opened.chain);
    }
    const auto expected = f.readback();
    auto restored = std::get<OpenedChain>(open_chain(f.binding, profile, expected, f.owner.view()));
    EXPECT_EQ(restored.committed_children, 2U);
    ASSERT_TRUE(restored.cutoff);
    expect_cutoff(*restored.cutoff, f.stop);
    const auto mapped = f.owner.mapped;
    const auto calls = f.owner.calls;
    EXPECT_EQ(std::get<BeginDisposition>(begin_event(restored.chain, f.e2, "replayed-E2")),
              BeginDisposition::duplicate_event);
    EXPECT_EQ(f.owner.mapped, mapped);
    EXPECT_EQ(f.owner.calls, calls);
    EXPECT_EQ(f.owner.notices.size(), 1U);
    EXPECT_EQ(f.owner.queued.size(), 2U);
    EXPECT_EQ(std::get<OpenRefusal>(open_chain(f.binding, profile, Unavailable{}, f.owner.view())),
              OpenRefusal::history_unavailable);

    auto corrupt = expected;
    corrupt.events[2].ordinal = 3;
    EXPECT_EQ(std::get<OpenRefusal>(open_chain(f.binding, profile, corrupt, f.owner.view())),
              OpenRefusal::conflict);
    corrupt = expected;
    corrupt.cutoff.reset();
    EXPECT_EQ(std::get<OpenRefusal>(open_chain(f.binding, profile, corrupt, f.owner.view())),
              OpenRefusal::conflict);
    corrupt = expected;
    corrupt.publications.back().identity.unit = "pass-E1";
    EXPECT_EQ(std::get<OpenRefusal>(open_chain(f.binding, profile, corrupt, f.owner.view())),
              OpenRefusal::conflict);

    corrupt = expected;
    corrupt.events[2].depth = 1;
    EXPECT_EQ(std::get<OpenRefusal>(open_chain(f.binding, profile, corrupt, f.owner.view())),
              OpenRefusal::conflict);
    corrupt = expected;
    std::swap(corrupt.events[1], corrupt.events[2]);
    corrupt.events[1].ordinal = 1;
    corrupt.events[2].ordinal = 2;
    EXPECT_EQ(std::get<OpenRefusal>(open_chain(f.binding, profile, corrupt, f.owner.view())),
              OpenRefusal::conflict);
    corrupt = expected;
    corrupt.cutoff->last = expected.events[1];
    corrupt.publications[1].cutoff = corrupt.cutoff;
    corrupt.publications[2].named_suppressions[0].stop = *corrupt.cutoff;
    EXPECT_EQ(std::get<OpenRefusal>(open_chain(f.binding, profile, corrupt, f.owner.view())),
              OpenRefusal::conflict);
    corrupt = expected;
    corrupt.cutoff->reason = CutoffReason::depth;
    corrupt.publications[1].cutoff = corrupt.cutoff;
    corrupt.publications[2].named_suppressions[0].stop = *corrupt.cutoff;
    EXPECT_EQ(std::get<OpenRefusal>(open_chain(f.binding, profile, corrupt, f.owner.view())),
              OpenRefusal::conflict);
}

} // namespace
