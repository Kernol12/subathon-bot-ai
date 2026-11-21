// SPDX-License-Identifier: GPL-3.0-or-later
// RULES-003 revision 2 (#208). The section names and literal expectations follow
// the frozen examples; ports below supply synthetic observations only.
#include <gtest/gtest.h>
#include <subathon/core/rule_actions.hpp>

#include <array>
#include <map>
#include <utility>

namespace {
using namespace subathon::core;
using Refusal = RuleActionRefusal;
using Disposition = RuleSelectionDisposition;
using State = PermissionFactsState;
using Outcome = PermissionOutcome;
using Kind = RuleTriggerKind;

RuleDefinition rule(std::string ref, std::int64_t priority,
                    std::vector<RuleActionDefinition> actions = {}) {
    auto name = ref;
    return {.rule_ref = std::move(ref),
            .version_ref = "v1",
            .name = std::move(name),
            .description = {},
            .priority = priority,
            .exclusive_group_ref = {},
            .excluded_rule_refs = {},
            .common_condition = {},
            .triggers = {{Kind::named_event, {}}},
            .actions = std::move(actions)};
}

RuleConditionNode event_name_is(std::string name) {
    return RulePredicate{RuleComparison<RuleText, RuleEquality>{
        {RuleTextField::event_name, std::move(name)}, RuleEquality::equal}};
}

RuleConditionNode main_is(std::int64_t main) {
    return RulePredicate{RuleComparison<RuleInteger, RuleOrder>{
        {RuleIntegerField::main_remaining_ms, main}, RuleOrder::equal}};
}

RuleActionDefinition credit(std::string id, std::string amount) {
    return {std::move(id), "v1",
            RuleDeltaAction{RuleDeltaUnit::milliseconds, "A", RuleFixedDelta{std::move(amount)}}};
}

RuleActionDefinition emit(std::string id, std::string name) {
    return {std::move(id), "v1", RuleNamedEvent{std::move(name), {}}};
}

PermissionRequest timer(std::string amount, std::string denominator = "1") {
    return {"timer.adjust",
            {{"timer", "A"}},
            {{"amount", "ms", {std::move(amount), std::move(denominator)}}}};
}

PermissionRequest named(std::string name) {
    return {"event.emit", {{"name", std::move(name)}}, {}};
}

PermissionRequest chat(std::string sender) {
    return {"chat.send", {{"sender", std::move(sender)}}, {}};
}

PermissionPolicyEntry allow(std::string source, std::string operation, std::string slot,
                            std::string resource,
                            std::vector<PermissionQuantityConstraint> quantities = {}) {
    return {PermissionOrigin::individual,
            std::move(source),
            {},
            std::move(operation),
            PermissionEffect::allow,
            "grant",
            {{std::move(slot), PermissionResourceMode::allowed_set, {std::move(resource)}}},
            std::move(quantities)};
}

PermissionPolicyEntry limit(std::string source, std::string target, std::string maximum) {
    return allow(std::move(source), "timer.adjust", "timer", std::move(target),
                 {{"amount", "ms", PermissionQuantityMode::closed_interval,
                   PermissionRational{"0", "1"}, PermissionRational{std::move(maximum), "1"}}});
}

RuleActionAuthorityFacts facts(std::string snapshot, std::vector<PermissionPolicyEntry> use,
                               std::vector<PermissionPolicyEntry> grant) {
    return {std::move(snapshot),
            {State::complete,
             "catalog-1",
             {{"timer.adjust",
               PermissionOperationClass::ordinary,
               {"timer"},
               {{"amount", "ms", PermissionNumberKind::rational, {}, {}, 15}},
               {}},
              {"event.emit", PermissionOperationClass::ordinary, {"name"}, {}, {}},
              {"chat.send", PermissionOperationClass::ordinary, {"sender"}, {}, {}}}},
            {PermissionOwnerState::present, "owner"},
            {State::complete, {}, std::move(use)},
            {State::complete, {}, std::move(grant)}};
}

RuleActionAuthorityFacts timer_facts(std::string snapshot, std::string grant_max = "300000") {
    return facts(std::move(snapshot), {limit("timer-use", "A", "300000")},
                 {limit("timer-grant", "A", std::move(grant_max))});
}

RuleActionAuthorityFacts event_facts(std::string name) {
    return facts("event-current", {allow("event-use", "event.emit", "name", name)},
                 {allow("event-grant", "event.emit", "name", name)});
}

struct AuthorityCall {
    std::string author;
    RuleActionKey key;
    PermissionRequest request;
};

struct Submission {
    RuleEventLineage event;
    RuleActionKey key;
    RuleBoundAction bound;
    std::string snapshot;
};

struct Ports {
    std::map<std::string, PermissionRequest> mapped;
    std::map<std::string, Refusal> mapping_failures;
    std::map<std::string, RuleActionAuthorityFacts> current;
    std::map<std::string, RuleOwnerResult> responses;
    std::vector<std::pair<std::string, RuleActionKey>> mappings;
    std::vector<AuthorityCall> authority_calls;
    std::vector<Submission> submissions;
    std::vector<std::string> calls;
    RuleGate execution = RuleGate::permit;
    bool target_valid = true;
    RuleCapability readiness = RuleCapability::supported;
    std::optional<std::string> owner_snapshot;

    RuleActionMapper mapper() {
        return [this](const RuleActionKey &key, const RuleActionPayload &payload,
                      const RuleCapturedInput &capture) -> RuleActionBinding {
            mappings.emplace_back(capture.event_ref, key);
            if (const auto failure = mapping_failures.find(key.action_ref);
                failure != mapping_failures.end()) {
                return failure->second;
            }
            return RuleBoundAction{mapped.at(key.action_ref), payload};
        };
    }

    RuleActionChecks checks() {
        return {[this](std::string_view author, const RuleActionKey &key,
                       const RuleBoundAction &bound) -> std::optional<RuleActionAuthorityFacts> {
                    calls.push_back("authority:" + key.action_ref);
                    authority_calls.push_back({std::string(author), key, bound.request});
                    const auto entry = current.find(key.action_ref);
                    if (entry == current.end()) {
                        return std::nullopt;
                    }
                    return entry->second;
                },
                [this](const RuleBoundAction &bound, std::string_view snapshot) {
                    calls.push_back("execution:" + bound.request.operation + ":" +
                                    std::string(snapshot));
                    return bound.request.operation == "timer.adjust" ? execution : RuleGate::permit;
                },
                [this](const RuleBoundAction &bound) {
                    calls.push_back("target:" + bound.request.operation);
                    return bound.request.operation != "timer.adjust" || target_valid;
                },
                [this](const RuleBoundAction &bound) {
                    calls.push_back("capability:" + bound.request.operation);
                    return bound.request.operation == "timer.adjust" ? readiness
                                                                     : RuleCapability::supported;
                }};
    }

    RuleActionSubmit submit() {
        return [this](const RuleEventLineage &event, const RuleActionKey &key,
                      const RuleBoundAction &bound, std::string_view snapshot) -> RuleOwnerResult {
            calls.push_back("submit:" + key.action_ref);
            submissions.push_back({event, key, bound, std::string(snapshot)});
            if (bound.request.operation == "timer.adjust" && owner_snapshot &&
                *owner_snapshot != snapshot) {
                return RuleOwnerRefusal{"owner-stale-authority"};
            }
            return responses.at(key.action_ref);
        };
    }
};

RuleActionPlan plan(Ports &ports, const RuleConfiguration &configuration, RuleEventLineage event,
                    std::span<const RuleCapturedFact> captured = {},
                    std::span<const RuleExplicitValuation> valuations = {},
                    std::span<const std::string> authors = {}, Kind kind = Kind::named_event) {
    const std::vector<std::string> defaults(configuration.data().rules.size(), "admin");
    const RuleCapturedInput input{
        event.event_ref, configuration.data().configuration_ref, kind, captured, nullptr, {}};
    return plan_rule_actions(configuration, input, event, authors.empty() ? defaults : authors,
                             valuations, ports.mapper());
}

void expect_timer(const PermissionRequest &request, std::string_view amount,
                  std::string_view denominator = "1") {
    EXPECT_EQ(request.operation, "timer.adjust");
    ASSERT_EQ(request.resources.size(), 1U);
    EXPECT_EQ(request.resources[0].slot, "timer");
    EXPECT_EQ(request.resources[0].resource, "A");
    ASSERT_EQ(request.quantities.size(), 1U);
    const auto &quantity = request.quantities[0];
    EXPECT_EQ(quantity.slot, "amount");
    EXPECT_EQ(quantity.unit, "ms");
    EXPECT_EQ(quantity.value.numerator, amount);
    EXPECT_EQ(quantity.value.denominator, denominator);
}

void expect_single_resource(const PermissionRequest &request, std::string_view operation,
                            std::string_view slot, std::string_view resource) {
    EXPECT_EQ(request.operation, operation);
    ASSERT_EQ(request.resources.size(), 1U);
    EXPECT_EQ(request.resources[0].slot, slot);
    EXPECT_EQ(request.resources[0].resource, resource);
    EXPECT_TRUE(request.quantities.empty());
}

void expect_stop(const RuleStop &stop, std::size_t position, Refusal refusal) {
    EXPECT_EQ(stop.position, position);
    EXPECT_EQ(stop.refusal, refusal);
    const auto bounded = disclose_rule_stop(stop, RuleStopDisclosure::stop_only);
    ASSERT_TRUE(bounded);
    EXPECT_EQ(bounded->position, position);
    EXPECT_EQ(bounded->refusal, refusal);
    EXPECT_FALSE(bounded->detail);
    EXPECT_FALSE(disclose_rule_stop(stop, RuleStopDisclosure::none));
}

void expect_denial(const RuleStop &stop, RuleAuthorityChannel channel, Outcome use, Outcome grant,
                   std::optional<std::string_view> limit = {}) {
    const auto detailed = disclose_rule_stop(stop, RuleStopDisclosure::detail);
    ASSERT_TRUE(detailed);
    EXPECT_EQ(detailed->position, stop.position);
    EXPECT_EQ(detailed->refusal, Refusal::authorization_hold);
    ASSERT_TRUE(detailed->detail);
    const auto &detail = *detailed->detail;
    EXPECT_EQ(detail.channel, channel);
    ASSERT_TRUE(detail.author_decision);
    EXPECT_EQ(detail.author_decision->use, use);
    EXPECT_EQ(detail.author_decision->grant, grant);
    EXPECT_EQ(detail.author_decision->disposition, PermissionAutomationDisposition::hold);
    EXPECT_FALSE(detail.authority_error);
    EXPECT_EQ(detail.limit.has_value(), limit.has_value());
    if (detail.limit && limit) {
        EXPECT_EQ(detail.limit->numerator, *limit);
        EXPECT_EQ(detail.limit->denominator, "1");
    }
}

constexpr RuleTimeObservation capped_credit{10000, 2000, 0, 8000, 60000};

TEST(RuleActions, RetainedExclusionCappedPrefixRevokedBonusAndObservedLaterEvent) {
    auto a =
        rule("A", 30,
             {credit("credit", "10000"), credit("bonus", "300000"), emit("suppressed", "skipped")});
    auto b = rule("B", 20, {emit("excluded", "excluded")});
    auto c = rule("C", 10, {emit("follow", "ready")});
    auto d = rule("D", 0);
    a.excluded_rule_refs = {"B"};
    b.excluded_rule_refs = {"C"};
    for (auto *definition : {&a, &b, &c}) {
        definition->triggers[0].condition = {event_name_is("start")};
    }
    d.common_condition = {main_is(60000)};
    const auto retained = validate_rule_configuration({"cfg-v1", {}, {a, b, c, d}});
    const RuleEventLineage root{"root", {}, "root", {}};
    const std::array<RuleCapturedFact, 2> original_context{
        RuleText{RuleTextField::event_name, "start"},
        RuleInteger{RuleIntegerField::main_remaining_ms, 58000}};
    const RuleActionKey credit_key{"A", "v1", "credit", "v1", 0};
    const RuleActionKey bonus_key{"A", "v1", "bonus", "v1", 1};
    const RuleActionKey suppressed_key{"A", "v1", "suppressed", "v1", 2};
    const RuleActionKey follow_key{"C", "v1", "follow", "v1", 0};
    const RuleEventLineage child{"child", "root", "root", follow_key};

    Ports ports;
    ports.mapped = {{"credit", timer("10000")},
                    {"bonus", timer("300000")},
                    {"suppressed", named("skipped")},
                    {"follow", named("ready")}};
    ports.current = {{"credit", timer_facts("preview-1")},
                     {"bonus", timer_facts("preview-1")},
                     {"suppressed", event_facts("skipped")},
                     {"follow", event_facts("ready")}};
    const auto plan_v1 = plan(ports, retained, root, original_context);
    const std::vector<RuleSelectionDecision> original_decisions{
        {Disposition::selected, {}},
        {Disposition::excluded, 0},
        {Disposition::selected, {}},
        {Disposition::condition_no_match, {}}};
    EXPECT_EQ(plan_v1.event, root);
    EXPECT_EQ(plan_v1.selection.event_ref, "root");
    EXPECT_EQ(plan_v1.selection.configuration_ref, "cfg-v1");
    EXPECT_EQ(plan_v1.selection.decisions, original_decisions);
    EXPECT_EQ(plan_v1.selection.selected_rule_indices, (std::vector<std::size_t>{0, 2}));
    EXPECT_TRUE(plan_v1.selection.priority_ties.empty());
    ASSERT_EQ(plan_v1.selected.size(), 2U);
    EXPECT_EQ(plan_v1.selected[0].original_author, "admin");
    ASSERT_EQ(plan_v1.selected[0].intents.size(), 3U);
    ASSERT_EQ(plan_v1.selected[1].intents.size(), 1U);
    EXPECT_EQ(ports.mappings,
              (std::vector<std::pair<std::string, RuleActionKey>>{{"root", credit_key},
                                                                  {"root", bonus_key},
                                                                  {"root", suppressed_key},
                                                                  {"root", follow_key}}));
    EXPECT_EQ(plan_v1.selected[0].intents[2].key, suppressed_key);
    EXPECT_TRUE(ports.calls.empty());

    // CS01 scaled to ms: Main 58000, debt 0, cap 60000. Request 10000
    // fills 2000 headroom, losing 8000 to the cap. The fake supplies the oracle.
    unsigned timer_projections = 0;
    const auto preview = simulate_rule_actions(
        plan_v1, ports.checks(), [&](const RuleBoundAction &bound) -> RuleProjection {
            if (bound.request.operation == "timer.adjust") {
                if (++timer_projections == 1) {
                    return capped_credit;
                }
                return RuleTimeObservation{300000, 0, 0, 300000, 60000};
            }
            return RuleProjectedChild{std::get<RuleNamedEvent>(bound.payload).event_name};
        });
    EXPECT_TRUE(preview.stops.empty());
    ASSERT_EQ(preview.projected.size(), 4U);
    EXPECT_EQ(preview.projected[0].first, credit_key);
    EXPECT_EQ(std::get<RuleTimeObservation>(preview.projected[0].second), capped_credit);
    EXPECT_EQ(preview.projected[1].first, bonus_key);
    EXPECT_EQ(std::get<RuleTimeObservation>(preview.projected[1].second).main_change_ms, 0);
    EXPECT_EQ(preview.projected[2].first, suppressed_key);
    EXPECT_EQ(std::get<RuleProjectedChild>(preview.projected[2].second).name, "skipped");
    EXPECT_EQ(preview.projected[3].first, follow_key);
    EXPECT_TRUE(ports.submissions.empty());

    const auto current_configuration = validate_rule_configuration({"cfg-v2", {}, {b, c, d}});
    ports.calls.clear();
    ports.authority_calls.clear();
    ports.current["credit"] = timer_facts("current-credit");
    ports.current["bonus"] = timer_facts("current-bonus", "299999");
    ports.current["bonus"].grant.entries.push_back(limit("timer-B-grant", "B", "600000"));
    ports.responses = {
        {"credit", RuleOwnerAcceptance{RuleEffectObservation{"effect-A", capped_credit}}},
        {"follow", RuleOwnerAcceptance{child}}};
    const auto consumed = consume_rule_actions(plan_v1, ports.checks(), ports.submit());
    ASSERT_TRUE(std::holds_alternative<RuleStagedPass>(consumed));
    const auto committed =
        publish_rule_pass(std::get<RuleStagedPass>(consumed), RuleCompletion::committed);
    ASSERT_TRUE(committed);
    ASSERT_EQ(committed->rules.size(), 2U);
    const auto &first = committed->rules[0];
    EXPECT_EQ(first.captured_index, 0U);
    ASSERT_EQ(first.accepted.size(), 1U);
    EXPECT_EQ(first.accepted[0].key, credit_key);
    const auto &effect = std::get<RuleEffectObservation>(first.accepted[0].observed);
    EXPECT_EQ(effect.effect_ref, "effect-A");
    EXPECT_EQ(std::get<RuleTimeObservation>(effect.change), capped_credit);
    ASSERT_TRUE(first.stop);
    expect_stop(*first.stop, 1, Refusal::authorization_hold);
    expect_denial(*first.stop, RuleAuthorityChannel::grant, Outcome::permit, Outcome::deny,
                  "299999");
    ASSERT_EQ(committed->rules[1].accepted.size(), 1U);
    EXPECT_EQ(committed->rules[1].captured_index, 2U);
    EXPECT_FALSE(committed->rules[1].stop);
    EXPECT_EQ(committed->rules[1].accepted[0].key, follow_key);
    EXPECT_EQ(std::get<RuleEventLineage>(committed->rules[1].accepted[0].observed), child);
    EXPECT_EQ(ports.calls,
              (std::vector<std::string>{"authority:credit", "execution:timer.adjust:current-credit",
                                        "target:timer.adjust", "capability:timer.adjust",
                                        "submit:credit", "authority:bonus", "authority:follow",
                                        "execution:event.emit:event-current", "target:event.emit",
                                        "capability:event.emit", "submit:follow"}));
    ASSERT_EQ(ports.authority_calls.size(), 3U);
    EXPECT_EQ(ports.authority_calls[0].author, "admin");
    EXPECT_EQ(ports.authority_calls[1].author, "admin");
    EXPECT_EQ(ports.authority_calls[2].author, "admin");
    expect_timer(ports.authority_calls[0].request, "10000");
    expect_timer(ports.authority_calls[1].request, "300000");
    ASSERT_EQ(ports.submissions.size(), 2U);
    EXPECT_EQ(ports.submissions[0].snapshot, "current-credit");
    expect_timer(ports.submissions[0].bound.request, "10000");
    EXPECT_EQ(ports.submissions[1].event, root);
    EXPECT_EQ(ports.submissions[1].key, follow_key);
    expect_single_resource(ports.submissions[1].bound.request, "event.emit", "name", "ready");

    // Resolution after the pass supplies no new admitted root or request.
    ports.current["bonus"] = timer_facts("after-hold");
    EXPECT_EQ(first.accepted.size(), 1U);
    EXPECT_EQ(first.stop->position, 1U);

    const std::array<RuleCapturedFact, 2> later_context{
        RuleText{RuleTextField::event_name, "ready"},
        RuleInteger{RuleIntegerField::main_remaining_ms, 60000}};
    const auto later = plan(ports, current_configuration, child, later_context);
    EXPECT_EQ(later.event, child);
    EXPECT_EQ(later.selection.event_ref, "child");
    EXPECT_EQ(later.selection.configuration_ref, "cfg-v2");
    EXPECT_EQ(later.selection.decisions,
              (std::vector<RuleSelectionDecision>{{Disposition::condition_no_match, {}},
                                                  {Disposition::condition_no_match, {}},
                                                  {Disposition::selected, {}}}));
    EXPECT_EQ(later.selection.selected_rule_indices, (std::vector<std::size_t>{2}));
    ASSERT_EQ(later.selected.size(), 1U);
    EXPECT_EQ(later.selected[0].captured_index, 2U);
    EXPECT_TRUE(later.selected[0].intents.empty());
    EXPECT_EQ(plan_v1.selection.decisions, original_decisions);
    EXPECT_EQ(plan_v1.selection.selected_rule_indices, (std::vector<std::size_t>{0, 2}));
}

enum class Fault {
    denied,
    unknown_use,
    no_lookup,
    invalid_policy,
    invalid_binding,
    missing_input,
    execution_denied,
    invalid_target,
    unavailable_handler,
    unsupported_handler,
    owner_refused,
    stale_at_owner
};

struct FaultRow {
    const char *name;
    Fault fault;
    Refusal refusal;
    std::vector<std::string> first_rule_calls;
};

class RuleActionFault : public testing::TestWithParam<FaultRow> {};

TEST_P(RuleActionFault, StopsThisRuleOnlyAtTheFirstRefusal) {
    const auto &row = GetParam();
    auto first = rule("first", 20, {credit("first-credit", "1000"), credit("suffix", "1")});
    first.exclusive_group_ref = "timer-group";
    auto loser = rule("loser", 20, {emit("loser-event", "never")});
    loser.exclusive_group_ref = "timer-group";
    const auto sibling = rule("sibling", 10, {emit("next", "independent")});
    const auto retained = validate_rule_configuration({"fault-cfg", {}, {sibling, first, loser}});
    const RuleEventLineage root{"fault-root", {}, "fault-root", {}};
    const RuleActionKey next_key{"sibling", "v1", "next", "v1", 0};
    const RuleEventLineage child{"fault-child", "fault-root", "fault-root", next_key};
    const std::array<std::string, 3> original_authors{"second-author", "admin", "admin"};
    Ports ports;
    ports.mapped = {
        {"first-credit", timer("1000")}, {"suffix", timer("1")}, {"next", named("independent")}};
    ports.current = {{"first-credit", timer_facts("fault-current")},
                     {"next", event_facts("independent")}};
    ports.responses = {{"next", RuleOwnerAcceptance{child}}};
    auto &current = ports.current.at("first-credit");
    switch (row.fault) {
    case Fault::denied:
        current.grant.entries.clear();
        ports.target_valid = false;
        ports.readiness = RuleCapability::unavailable;
        break;
    case Fault::unknown_use:
        current.use = {};
        break;
    case Fault::no_lookup:
        ports.current.erase("first-credit");
        break;
    case Fault::invalid_policy:
        // Nonempty role rows with an individual source are rejected by #249.
        current.use.entries[0].role = "contradictory-role";
        break;
    case Fault::invalid_binding:
    case Fault::missing_input:
        ports.mapping_failures.emplace("first-credit", row.refusal);
        break;
    case Fault::execution_denied:
        ports.execution = RuleGate::deny;
        break;
    case Fault::invalid_target:
        ports.target_valid = false;
        ports.readiness = RuleCapability::unavailable;
        break;
    case Fault::unavailable_handler:
        ports.readiness = RuleCapability::unavailable;
        break;
    case Fault::unsupported_handler:
        ports.readiness = RuleCapability::unsupported;
        break;
    case Fault::owner_refused:
        ports.responses.emplace("first-credit", RuleOwnerRefusal{"owner-private-42"});
        break;
    case Fault::stale_at_owner:
        ports.owner_snapshot = "revoked-after-check";
        ports.responses.emplace("first-credit", RuleOwnerAcceptance{});
        break;
    }
    const auto planned = plan(ports, retained, root, {}, {}, original_authors);
    ASSERT_EQ(planned.selected.size(), 2U);
    EXPECT_EQ(planned.selection.selected_rule_indices, (std::vector<std::size_t>{1, 0}));
    EXPECT_EQ(planned.selection.priority_ties, (std::vector<RulePriorityTie>{{{1, 2}}}));
    EXPECT_EQ(planned.selection.decisions,
              (std::vector<RuleSelectionDecision>{{Disposition::selected, {}},
                                                  {Disposition::selected, {}},
                                                  {Disposition::exclusive_group, 1}}));
    EXPECT_EQ(planned.selected[0].original_author, "admin");
    EXPECT_EQ(planned.selected[1].original_author, "second-author");
    ASSERT_EQ(planned.selected[0].intents.size(), 2U);
    EXPECT_EQ(planned.selected[0].intents[1].key,
              (RuleActionKey{"first", "v1", "suffix", "v1", 1}));
    EXPECT_TRUE(ports.calls.empty());

    const auto result = consume_rule_actions(planned, ports.checks(), ports.submit());
    ASSERT_TRUE(std::holds_alternative<RuleStagedPass>(result));
    const auto committed =
        publish_rule_pass(std::get<RuleStagedPass>(result), RuleCompletion::committed);
    ASSERT_TRUE(committed);
    ASSERT_EQ(committed->rules.size(), 2U);
    const auto &stopped = committed->rules[0];
    EXPECT_EQ(stopped.captured_index, 1U);
    EXPECT_TRUE(stopped.accepted.empty());
    ASSERT_TRUE(stopped.stop);
    expect_stop(*stopped.stop, 0, row.refusal);

    if (row.fault == Fault::denied) {
        expect_denial(*stopped.stop, RuleAuthorityChannel::grant, Outcome::permit, Outcome::deny);
    }
    if (row.refusal == Refusal::authority_unavailable) {
        const auto detailed = disclose_rule_stop(*stopped.stop, RuleStopDisclosure::detail);
        ASSERT_TRUE(detailed);
        EXPECT_EQ(detailed->refusal, Refusal::authority_unavailable);
        if (row.fault == Fault::unknown_use) {
            ASSERT_TRUE(detailed->detail);
            EXPECT_EQ(detailed->detail->channel, RuleAuthorityChannel::use);
            EXPECT_EQ(detailed->detail->authority_error,
                      EffectivePermissionError::facts_unavailable);
        }
        if (detailed->detail) {
            EXPECT_FALSE(detailed->detail->author_decision);
            EXPECT_FALSE(detailed->detail->limit);
        }
    }
    if (row.refusal == Refusal::handler_refusal) {
        const auto detailed = disclose_rule_stop(*stopped.stop, RuleStopDisclosure::detail);
        ASSERT_TRUE(detailed && detailed->detail);
        EXPECT_EQ(detailed->detail->opaque_reason,
                  row.fault == Fault::owner_refused ? "owner-private-42" : "owner-stale-authority");
    }
    EXPECT_EQ(committed->rules[1].captured_index, 0U);
    EXPECT_FALSE(committed->rules[1].stop);
    ASSERT_EQ(committed->rules[1].accepted.size(), 1U);
    EXPECT_EQ(committed->rules[1].accepted[0].key, next_key);
    EXPECT_EQ(std::get<RuleEventLineage>(committed->rules[1].accepted[0].observed), child);

    auto expected = row.first_rule_calls;
    expected.insert(expected.end(), {"authority:next", "execution:event.emit:event-current",
                                     "target:event.emit", "capability:event.emit", "submit:next"});
    EXPECT_EQ(ports.calls, expected);
    ASSERT_FALSE(ports.authority_calls.empty());
    if (row.fault == Fault::invalid_binding || row.fault == Fault::missing_input) {
        EXPECT_EQ(ports.authority_calls.size(), 1U);
    } else {
        ASSERT_EQ(ports.authority_calls.size(), 2U);
        EXPECT_EQ(ports.authority_calls.front().author, "admin");
        expect_timer(ports.authority_calls.front().request, "1000");
    }
    EXPECT_EQ(ports.authority_calls.back().author, "second-author");
    expect_single_resource(ports.authority_calls.back().request, "event.emit", "name",
                           "independent");
    ASSERT_EQ(ports.submissions.size(), row.refusal == Refusal::handler_refusal ? 2U : 1U);
    if (row.refusal == Refusal::handler_refusal) {
        EXPECT_EQ(ports.submissions.front().snapshot, "fault-current");
        expect_timer(ports.submissions.front().bound.request, "1000");
    }
    EXPECT_EQ(ports.submissions.back().key, next_key);
    EXPECT_EQ(ports.submissions.back().event, root);
}

INSTANTIATE_TEST_SUITE_P(
    CurrentFacts, RuleActionFault,
    testing::Values(
        FaultRow{"NoGrant", Fault::denied, Refusal::authorization_hold, {"authority:first-credit"}},
        FaultRow{"UnknownUse",
                 Fault::unknown_use,
                 Refusal::authority_unavailable,
                 {"authority:first-credit"}},
        FaultRow{"FailedLookup",
                 Fault::no_lookup,
                 Refusal::authority_unavailable,
                 {"authority:first-credit"}},
        FaultRow{"EvaluatorError",
                 Fault::invalid_policy,
                 Refusal::authority_unavailable,
                 {"authority:first-credit"}},
        FaultRow{"ContradictoryBinding", Fault::invalid_binding, Refusal::invalid_binding, {}},
        FaultRow{"UnresolvedImmutableInput", Fault::missing_input, Refusal::unresolved_input, {}},
        FaultRow{"ExecutionSubjectDenied",
                 Fault::execution_denied,
                 Refusal::authorization_hold,
                 {"authority:first-credit", "execution:timer.adjust:fault-current"}},
        FaultRow{"InvalidTarget",
                 Fault::invalid_target,
                 Refusal::invalid_target,
                 {"authority:first-credit", "execution:timer.adjust:fault-current",
                  "target:timer.adjust"}},
        FaultRow{"UnavailableHandler",
                 Fault::unavailable_handler,
                 Refusal::capability_unavailable,
                 {"authority:first-credit", "execution:timer.adjust:fault-current",
                  "target:timer.adjust", "capability:timer.adjust"}},
        FaultRow{"UnsupportedHandler",
                 Fault::unsupported_handler,
                 Refusal::unsupported_capability,
                 {"authority:first-credit", "execution:timer.adjust:fault-current",
                  "target:timer.adjust", "capability:timer.adjust"}},
        FaultRow{"OrdinaryOwnerRefusal",
                 Fault::owner_refused,
                 Refusal::handler_refusal,
                 {"authority:first-credit", "execution:timer.adjust:fault-current",
                  "target:timer.adjust", "capability:timer.adjust", "submit:first-credit"}},
        FaultRow{"StaleAtOwnerAcceptance",
                 Fault::stale_at_owner,
                 Refusal::handler_refusal,
                 {"authority:first-credit", "execution:timer.adjust:fault-current",
                  "target:timer.adjust", "capability:timer.adjust", "submit:first-credit"}}),
    [](const testing::TestParamInfo<FaultRow> &row) { return row.param.name; });

RuleActionAuthorityFacts chat_facts(std::string snapshot, bool broadcaster_grant) {
    auto out = facts(std::move(snapshot),
                     {allow("bot-use", "chat.send", "sender", "bot"),
                      allow("broadcaster-use", "chat.send", "sender", "broadcaster-A")},
                     {allow("bot-grant", "chat.send", "sender", "bot")});
    if (broadcaster_grant) {
        out.grant.entries.push_back(
            allow("broadcaster-grant", "chat.send", "sender", "broadcaster-A"));
    }
    return out;
}

constexpr std::string_view route = "test/channel-A";
const RuleActionKey announce_key{"chat-rule", "v1", "announce", "v7", 0};

RuleActionPlan chat_plan(Ports &ports, RuleChatSender selection) {
    const auto retained = validate_rule_configuration(
        {"chat-cfg",
         {},
         {rule("chat-rule", 0,
               {{"announce", "v7",
                 RuleChatAction{std::string(route), selection, "template-v1", {}}}})}});
    ports.mapped.emplace("announce", chat("broadcaster-A"));
    return plan(ports, retained, {"chat-root", {}, "chat-root", {}});
}

void expect_chat_request(const RuleBoundAction &bound, RuleChatSender selection) {
    expect_single_resource(bound.request, "chat.send", "sender", "broadcaster-A");
    const auto &message = std::get<RuleChatAction>(bound.payload);
    EXPECT_EQ(message.route_ref, route);
    EXPECT_EQ(message.sender, selection);
    EXPECT_EQ(message.template_version_ref, "template-v1"); // N2: mapped request, not job capture.
    EXPECT_FALSE(message.fallback_policy_version_ref);
}

TEST(RuleActions, PreviewBroadcasterPermitCannotQueueAfterGrantRevocation) {
    Ports ports;
    const auto planned = chat_plan(ports, RuleChatSender::broadcaster);
    ASSERT_EQ(planned.selected.size(), 1U);
    ASSERT_EQ(planned.selected[0].intents.size(), 1U);
    const auto &intent = planned.selected[0].intents[0];
    EXPECT_EQ(intent.key, announce_key);
    expect_chat_request(std::get<RuleBoundAction>(intent.binding), RuleChatSender::broadcaster);

    ports.current.emplace("announce", chat_facts("chat-preview", true));
    unsigned projection_calls = 0;
    const auto preview = simulate_rule_actions(
        planned, ports.checks(), [&](const RuleBoundAction &bound) -> RuleProjection {
            ++projection_calls;
            expect_chat_request(bound, RuleChatSender::broadcaster);
            return RuleProjectedDelivery{std::string(route), "broadcaster-A"};
        });
    EXPECT_TRUE(preview.stops.empty());
    ASSERT_EQ(preview.projected.size(), 1U);
    EXPECT_EQ(preview.projected[0].first, announce_key);
    const auto &proposal = std::get<RuleProjectedDelivery>(preview.projected[0].second);
    EXPECT_EQ(proposal.route_ref, route);
    EXPECT_EQ(proposal.sender_ref, "broadcaster-A");
    EXPECT_EQ(projection_calls, 1U);
    EXPECT_TRUE(ports.submissions.empty());

    ports.current["announce"] = chat_facts("chat-current", false);
    ports.calls.clear();
    ports.authority_calls.clear();
    const auto result = consume_rule_actions(planned, ports.checks(), ports.submit());
    const auto committed =
        publish_rule_pass(std::get<RuleStagedPass>(result), RuleCompletion::committed);
    ASSERT_TRUE(committed);
    ASSERT_EQ(committed->rules.size(), 1U);
    ASSERT_TRUE(committed->rules[0].stop);
    expect_stop(*committed->rules[0].stop, 0, Refusal::authorization_hold);
    expect_denial(*committed->rules[0].stop, RuleAuthorityChannel::grant, Outcome::permit,
                  Outcome::deny);
    EXPECT_TRUE(committed->rules[0].accepted.empty());
    EXPECT_TRUE(ports.submissions.empty());
    EXPECT_EQ(ports.calls, (std::vector<std::string>{"authority:announce"}));
    ASSERT_EQ(ports.authority_calls.size(), 1U);
    EXPECT_EQ(ports.authority_calls[0].author, "admin");
    expect_single_resource(ports.authority_calls[0].request, "chat.send", "sender",
                           "broadcaster-A");
}

TEST(RuleActions, QueueAcceptanceRetainsExactSenderAcrossSeparateDeliveryObservations) {
    Ports ports;
    const auto planned = chat_plan(ports, RuleChatSender::broadcaster);
    ports.current.emplace("announce", chat_facts("chat-current", true));
    const RuleDeliveryObservation queued{"job", RuleDeliveryState::queued, 1, {}, {}};
    const RuleDeliveryObservation sending{"job", RuleDeliveryState::sending, 2, 2, {}};
    const RuleDeliveryObservation interrupted{"job", RuleDeliveryState::outcome_unknown, 3, 2,
                                              "interrupted-sending"};
    ports.responses.emplace("announce", RuleOwnerAcceptance{queued});

    const auto pass = consume_rule_actions(planned, ports.checks(), ports.submit());
    const auto committed =
        publish_rule_pass(std::get<RuleStagedPass>(pass), RuleCompletion::committed);
    ASSERT_TRUE(committed);
    ASSERT_EQ(committed->rules.size(), 1U);
    ASSERT_EQ(committed->rules[0].accepted.size(), 1U);
    EXPECT_FALSE(committed->rules[0].stop);
    const auto &accepted = committed->rules[0].accepted[0];
    EXPECT_EQ(accepted.key, announce_key);
    const auto &observed_queue = std::get<RuleDeliveryObservation>(accepted.observed);
    EXPECT_EQ(observed_queue, queued);
    ASSERT_EQ(ports.submissions.size(), 1U);
    expect_chat_request(ports.submissions[0].bound, RuleChatSender::broadcaster);
    EXPECT_EQ(ports.submissions[0].snapshot, "chat-current");
    EXPECT_EQ(ports.calls, (std::vector<std::string>{
                               "authority:announce", "execution:chat.send:chat-current",
                               "target:chat.send", "capability:chat.send", "submit:announce"}));

    // Delivery owner fixture from the shipped revision-1/2/3 control. Its fresh
    // start gate can refuse despite the earlier queue acceptance. The adapter is
    // a counter on the fake owner's normal committed claim only.
    unsigned fresh_gate_checks = 0;
    unsigned starts = 0;
    const auto claim = [&](bool current_permit) -> std::optional<RuleDeliveryObservation> {
        ++fresh_gate_checks;
        if (!current_permit) {
            return {};
        }
        ++starts;
        return sending;
    };
    EXPECT_EQ(starts, 0U);
    EXPECT_EQ(fresh_gate_checks, 0U);
    const RuleDeliveryTrace initial{observed_queue, observed_queue};
    const auto denied = observe_rule_delivery(initial, claim(false));
    EXPECT_EQ(denied.queued, queued);
    EXPECT_EQ(denied.latest, queued);
    EXPECT_EQ(starts, 0U);
    EXPECT_EQ(fresh_gate_checks, 1U);
    const auto claimed = observe_rule_delivery(denied, claim(true));
    EXPECT_EQ(claimed.queued, queued);
    EXPECT_EQ(claimed.latest, sending);
    EXPECT_EQ(starts, 1U);
    EXPECT_EQ(fresh_gate_checks, 2U);
    const auto recovered = observe_rule_delivery(claimed, interrupted);
    EXPECT_EQ(recovered.queued, queued);
    EXPECT_EQ(recovered.latest, interrupted);
    EXPECT_EQ(starts, 1U);
    EXPECT_EQ(fresh_gate_checks, 2U);
    EXPECT_EQ(std::get<RuleDeliveryObservation>(accepted.observed), queued);
    EXPECT_EQ(ports.authority_calls.size(), 1U);
    EXPECT_EQ(ports.submissions.size(), 1U);
}

TEST(RuleActions, ConfiguredDefaultBindsAConcreteSenderBeforeCurrentAuthorization) {
    Ports ports;
    const auto planned = chat_plan(ports, RuleChatSender::configured_default);
    ports.current.emplace("announce", chat_facts("default-current", false));

    const auto result = consume_rule_actions(planned, ports.checks(), ports.submit());
    const auto &staged = std::get<RuleStagedPass>(result);
    ASSERT_EQ(staged.rules.size(), 1U);
    ASSERT_TRUE(staged.rules[0].stop);
    EXPECT_EQ(staged.rules[0].stop->refusal, Refusal::authorization_hold);
    ASSERT_EQ(ports.authority_calls.size(), 1U);
    expect_single_resource(ports.authority_calls[0].request, "chat.send", "sender",
                           "broadcaster-A");
    EXPECT_TRUE(ports.submissions.empty());
}

TEST(RuleActions, ExplicitValuationIsBoundToCaptureAndUnknownDoesNotRun) {
    auto valued = rule("valued", 10, {emit("valued-action", "valued")});
    valued.triggers = {{Kind::payment_received,
                        {RulePredicate{RuleComparison<RuleMoney, RuleOrder>{
                            {RuleMoneyField::source_amount, {"9", "EUR"}}, RuleOrder::equal}}}}};
    auto independent = rule("independent", 0, {emit("independent-action", "independent")});
    independent.triggers = {{Kind::payment_received, {}}};
    const auto retained = validate_rule_configuration({"fx-cfg", {}, {valued, independent}});
    const std::array<RuleCapturedFact, 1> captured{
        RuleMoney{RuleMoneyField::source_amount, {"12.000", "USD"}}};
    const RuleEventLineage event{"fx-event", {}, "fx-event", {}};
    Ports ports;
    ports.mapped = {{"valued-action", named("valued")},
                    {"independent-action", named("independent")}};
    const auto unknown = plan(ports, retained, event, captured, {}, {}, Kind::payment_received);
    EXPECT_EQ(unknown.selection.decisions,
              (std::vector<RuleSelectionDecision>{{Disposition::condition_unknown, {}},
                                                  {Disposition::selected, {}}}));
    EXPECT_EQ(unknown.selection.selected_rule_indices, (std::vector<std::size_t>{1}));
    ASSERT_EQ(ports.mappings.size(), 1U);
    EXPECT_EQ(ports.mappings[0].second.action_ref, "independent-action");

    ports.mappings.clear();
    std::array valuations{RuleExplicitValuation{"resolution",
                                                "fx-event",
                                                {RuleMoneyField::source_amount, {"12.000", "USD"}},
                                                "EUR",
                                                "3",
                                                "4"}};
    // 12 USD * 3/4 = 9 EUR exactly, without an arrival snapshot.
    const auto known =
        plan(ports, retained, event, captured, valuations, {}, Kind::payment_received);
    EXPECT_EQ(known.selection.selected_rule_indices, (std::vector<std::size_t>{0, 1}));
    ASSERT_EQ(ports.mappings.size(), 2U);
    EXPECT_EQ(ports.mappings[0].second.action_ref, "valued-action");

    ports.mappings.clear();
    valuations[0].event_ref = "another-event";
    try {
        (void)plan(ports, retained, event, captured, valuations, {}, Kind::payment_received);
        ADD_FAILURE() << "A rejected valuation must not yield a partial plan";
    } catch (const RuleDefinitionFailure &failure) {
        EXPECT_EQ(failure.code(), RuleDefinitionError::invalid_valuation);
    }
    EXPECT_TRUE(ports.mappings.empty());
    EXPECT_TRUE(ports.calls.empty());
}

TEST(RuleActions, RejectedSelectionDoesNotMapIndependentRule) {
    auto a = rule("A", 1, {credit("a", "1")});
    auto b = rule("B", 0, {credit("b", "2")});
    b.excluded_rule_refs = {"A"};
    const auto retained = validate_rule_configuration({"invalid-cfg", {}, {a, b}});
    Ports ports;
    ports.mapped = {{"a", timer("1")}, {"b", timer("2")}};
    try {
        (void)plan(ports, retained, {"root", {}, "root", {}});
        ADD_FAILURE() << "Invalid backward exclusion must reject the entire plan";
    } catch (const RuleDefinitionFailure &failure) {
        EXPECT_EQ(failure.code(), RuleDefinitionError::invalid_exclusion);
    }
    EXPECT_TRUE(ports.mappings.empty());
    EXPECT_TRUE(ports.calls.empty());
}

TEST(RuleActions, RetainsTypedActionsAndImmutableReferencesInUserOrder) {
    const std::vector<RuleActionDefinition> actions{
        {"z-score", "sv1",
         RuleDeltaAction{RuleDeltaUnit::micro_points, "score", RuleFixedDelta{"-1.25"}}},
        {"a-counter", "cv1",
         RuleDeltaAction{RuleDeltaUnit::whole_count, "counter", RuleFixedDelta{"3"}}},
        {"computed", "av3",
         RuleDeltaAction{RuleDeltaUnit::milliseconds, "A", RuleCalculation{"calc-v1"}}},
        {"include", "v1", RuleActivateModifier{"boost-v1", ModifierTriggerTiming::include_trigger}},
        {"subsequent", "v1",
         RuleActivateModifier{"boost-v1", ModifierTriggerTiming::subsequent_only}},
        {"extend", "v1", RuleExtendModifier{"modifier-7", 2000}},
        {"terminate", "v1", RuleTerminateModifier{"modifier-7"}},
        {"pause", "v1", RuleTimerControl{"A", "pause-v2"}},
        {"message", "v1",
         RuleChatAction{"test/channel-A", RuleChatSender::bot, "template-v1", "policy-v1"}},
        {"show", "v1", RuleAnnouncement{"presentation-v1"}},
        {"animate", "v1", RuleAnimation{"animation-v1"}},
        {"generate", "v1", RuleNamedEvent{"generated", "payload-v1"}},
        {"connector", "v1", RuleConnectorAction{"connection", "effect", "parameters-v1"}},
        {"unknown-connector", "v1",
         RuleConnectorAction{"other-connection", "effect", "parameters-v1"}},
        {"missing-calc", "v1",
         RuleDeltaAction{RuleDeltaUnit::milliseconds, "A", RuleCalculation{"calc-v0"}}},
    };
    Ports ports;
    ports.mapped = {
        {"z-score",
         {"score.adjust", {{"score", "score"}}, {{"amount", "micro_points", {"-5", "4"}}}}},
        {"a-counter",
         {"counter.adjust", {{"counter", "counter"}}, {{"amount", "count", {"3", "1"}}}}},
        {"computed", timer("15", "2")},
        {"include", {"modifier.activate", {{"definition", "boost-v1"}}, {}}},
        {"subsequent", {"modifier.activate", {{"definition", "boost-v1"}}, {}}},
        {"extend",
         {"modifier.extend", {{"modifier", "modifier-7"}}, {{"duration", "ms", {"2000", "1"}}}}},
        {"terminate", {"modifier.terminate", {{"modifier", "modifier-7"}}, {}}},
        {"pause", {"timer.pause", {{"timer", "A"}}, {}}},
        {"message", chat("bot")},
        {"show", {"announcement.show", {{"presentation", "presentation-v1"}}, {}}},
        {"animate", {"animation.play", {{"animation", "animation-v1"}}, {}}},
        {"generate", named("generated")},
        {"connector", {"connector.effect", {{"connection", "connection"}}, {}}},
    };
    ports.mapping_failures = {{"unknown-connector", Refusal::capability_unavailable},
                              {"missing-calc", Refusal::unresolved_input}};
    const auto planned = [&] {
        const std::array<std::string, 1> authors{"admin"};
        auto def = rule("authored", 10, actions);
        def.version_ref = "rule-v3";
        const auto retained = validate_rule_configuration({"types-v1", {}, {def}});
        return plan(ports, retained, {"typed-root", {}, "typed-root", {}}, {}, {}, authors);
    }(); // Captured configuration and author inputs may leave scope.
    auto edited = rule("authored", 10, actions);
    edited.version_ref = "rule-v4";
    std::swap(edited.actions[0], edited.actions[1]);
    edited.actions[2].version_ref = "av4";
    std::get<RuleCalculation>(std::get<RuleDeltaAction>(edited.actions[2].payload).delta)
        .calculation_version_ref = "calc-v2";
    std::get<RuleConnectorAction>(edited.actions[12].payload).parameters_version_ref =
        "parameters-v2";
    edited.actions[12].version_ref = "v2";
    const auto current_config = validate_rule_configuration({"types-v2", {}, {edited}});
    EXPECT_EQ(current_config.data().rules[0].version_ref, "rule-v4");

    ASSERT_EQ(planned.selected.size(), 1U);
    const auto &selected = planned.selected[0];
    EXPECT_EQ(planned.selection.configuration_ref, "types-v1");
    EXPECT_EQ(selected.original_author, "admin");
    const auto &intents = selected.intents;
    ASSERT_EQ(intents.size(), actions.size());
    ASSERT_EQ(ports.mappings.size(), actions.size());
    for (std::size_t pos = 0; pos < actions.size(); ++pos) {
        SCOPED_TRACE(actions[pos].action_ref);
        const RuleActionKey key{"authored", "rule-v3", actions[pos].action_ref,
                                actions[pos].version_ref, pos};
        EXPECT_EQ(intents[pos].key, key);
        EXPECT_EQ(intents[pos].captured_payload.index(), actions[pos].payload.index());
        EXPECT_EQ(ports.mappings[pos].first, "typed-root");
        EXPECT_EQ(ports.mappings[pos].second, key);
    }
    const auto &score = std::get<RuleDeltaAction>(intents[0].captured_payload);
    EXPECT_EQ(score.unit, RuleDeltaUnit::micro_points);
    EXPECT_EQ(score.target_ref, "score");
    EXPECT_EQ(std::get<RuleFixedDelta>(score.delta).original_decimal, "-1.25");
    const auto &counter = std::get<RuleDeltaAction>(intents[1].captured_payload);
    EXPECT_EQ(counter.unit, RuleDeltaUnit::whole_count);
    EXPECT_EQ(counter.target_ref, "counter");
    EXPECT_EQ(std::get<RuleFixedDelta>(counter.delta).original_decimal, "3");
    const auto &computed = std::get<RuleBoundAction>(intents[2].binding);
    EXPECT_EQ(std::get<RuleCalculation>(std::get<RuleDeltaAction>(computed.payload).delta)
                  .calculation_version_ref,
              "calc-v1");
    expect_timer(computed.request, "15", "2");
    const auto &include = std::get<RuleActivateModifier>(intents[3].captured_payload);
    EXPECT_EQ(include.definition_version_ref, "boost-v1");
    EXPECT_EQ(include.timing, ModifierTriggerTiming::include_trigger);
    EXPECT_EQ(std::get<RuleActivateModifier>(intents[4].captured_payload).timing,
              ModifierTriggerTiming::subsequent_only);
    const auto &extend = std::get<RuleExtendModifier>(intents[5].captured_payload);
    EXPECT_EQ(extend.modifier_ref, "modifier-7");
    EXPECT_EQ(extend.duration_ms, 2000);
    EXPECT_EQ(std::get<RuleTerminateModifier>(intents[6].captured_payload).modifier_ref,
              "modifier-7");
    EXPECT_EQ(std::get<RuleTimerControl>(intents[7].captured_payload).command_version_ref,
              "pause-v2");
    const auto &message = std::get<RuleChatAction>(intents[8].captured_payload);
    EXPECT_EQ(message.template_version_ref, "template-v1");
    EXPECT_EQ(message.fallback_policy_version_ref, "policy-v1");
    EXPECT_EQ(std::get<RuleAnnouncement>(intents[9].captured_payload).presentation_version_ref,
              "presentation-v1");
    EXPECT_EQ(std::get<RuleAnimation>(intents[10].captured_payload).animation_version_ref,
              "animation-v1");
    EXPECT_EQ(std::get<RuleNamedEvent>(intents[11].captured_payload).payload_version_ref,
              "payload-v1");
    const auto &connector = std::get<RuleBoundAction>(intents[12].binding);
    EXPECT_EQ(std::get<RuleConnectorAction>(connector.payload).parameters_version_ref,
              "parameters-v1");
    expect_single_resource(connector.request, "connector.effect", "connection", "connection");
    EXPECT_EQ(std::get<Refusal>(intents[13].binding), Refusal::capability_unavailable);
    EXPECT_EQ(std::get<Refusal>(intents[14].binding), Refusal::unresolved_input);
    EXPECT_TRUE(ports.calls.empty());
}

// Direct entry fixtures isolate later seams so their Stage 1 failures do not
// depend on a working planner. The integrated examples above exercise composition.
const RuleActionKey seed_credit{"credit-rule", "v1", "seed-credit", "v1", 0};
const RuleActionKey seed_next{"credit-rule", "v1", "seed-next", "v1", 1};
const RuleActionKey seed_event{"event-rule", "v1", "seed-event", "v1", 0};

RuleActionPlan preplanned() {
    const auto first = credit("seed-credit", "10000").payload;
    const auto next = credit("seed-next", "1000").payload;
    const auto event = emit("seed-event", "accepted-request").payload;
    return {
        {"root", {}, "root", {}},
        {"cfg", "root", {{Disposition::selected, {}}, {Disposition::selected, {}}}, {0, 1}, {}},
        {{0,
          "admin",
          {{seed_credit, first, RuleBoundAction{timer("10000"), first}},
           {seed_next, next, RuleBoundAction{timer("1000"), next}}}},
         {1, "admin", {{seed_event, event, RuleBoundAction{named("accepted-request"), event}}}}}};
}

TEST(RuleActions, SimulationStopsAtMissingFactsWithOnlyProjectedPrefixAndSibling) {
    Ports ports;
    ports.current = {{"seed-credit", timer_facts("preview")},
                     {"seed-event", event_facts("accepted-request")}};
    const auto simulation = simulate_rule_actions(
        preplanned(), ports.checks(), [](const RuleBoundAction &action) -> RuleProjection {
            if (action.request.operation == "timer.adjust") {
                return capped_credit;
            }
            return RuleProjectedChild{std::get<RuleNamedEvent>(action.payload).event_name};
        });
    ASSERT_EQ(simulation.projected.size(), 2U);
    EXPECT_EQ(simulation.projected[0].first, seed_credit);
    EXPECT_EQ(std::get<RuleTimeObservation>(simulation.projected[0].second), capped_credit);
    EXPECT_EQ(simulation.projected[1].first, seed_event);
    EXPECT_EQ(std::get<RuleProjectedChild>(simulation.projected[1].second).name,
              "accepted-request");
    ASSERT_EQ(simulation.stops.size(), 1U);
    EXPECT_EQ(simulation.stops[0].first, 0U);
    EXPECT_EQ(simulation.stops[0].second.position, 1U);
    EXPECT_EQ(simulation.stops[0].second.refusal, Refusal::authority_unavailable);
    EXPECT_TRUE(ports.submissions.empty());
}

TEST(RuleActions, NumericHoldAbortsPassAndDoesNotContinueWithNextRule) {
    Ports ports;
    ports.current = {{"seed-credit", timer_facts("at-credit")},
                     {"seed-next", timer_facts("at-hold")}};
    ports.responses = {
        {"seed-credit", RuleOwnerAcceptance{RuleEffectObservation{"staged", capped_credit}}},
        {"seed-next", RulePassFailure::numeric_hold}};
    const auto result = consume_rule_actions(preplanned(), ports.checks(), ports.submit());
    EXPECT_EQ(std::get<RulePassFailure>(result), RulePassFailure::numeric_hold);
    ASSERT_EQ(ports.submissions.size(), 2U);
    EXPECT_EQ(ports.submissions[0].key, seed_credit);
    EXPECT_EQ(ports.submissions[1].key, seed_next);
    ASSERT_EQ(ports.authority_calls.size(), 2U);
    EXPECT_EQ(ports.authority_calls.back().key, seed_next);
}

TEST(RuleActions, AcceptedNamedRequestWithoutOwnerObservationDoesNotEmit) {
    Ports ports;
    ports.current = {{"seed-event", event_facts("accepted-request")}};
    ports.responses = {{"seed-event", RuleOwnerAcceptance{}}};
    const auto result = consume_rule_actions(preplanned(), ports.checks(), ports.submit());
    const auto &staged = std::get<RuleStagedPass>(result);
    ASSERT_EQ(staged.rules.size(), 2U);
    ASSERT_EQ(staged.rules[1].accepted.size(), 1U);
    EXPECT_EQ(staged.rules[1].accepted[0].key, seed_event);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(staged.rules[1].accepted[0].observed));
    ASSERT_EQ(ports.submissions.size(), 1U);
    EXPECT_EQ(ports.submissions[0].event, (RuleEventLineage{"root", {}, "root", {}}));
    EXPECT_EQ(ports.submissions[0].key, seed_event);
}

class UnpublishedRulePass : public testing::TestWithParam<RuleCompletion> {};
TEST_P(UnpublishedRulePass, OwnerFailureKeepsTheStagedPrefixUnpublished) {
    const RuleStagedPass staged{{{0,
                                  {{seed_credit, RuleEffectObservation{"staged", capped_credit}}},
                                  RuleStop{1, Refusal::handler_refusal, {}}}}};
    EXPECT_FALSE(publish_rule_pass(staged, GetParam()));
}
INSTANTIATE_TEST_SUITE_P(OwnerCompletion, UnpublishedRulePass,
                         testing::Values(RuleCompletion::rolled_back,
                                         RuleCompletion::outcome_unknown,
                                         RuleCompletion::abnormal_completion));

TEST(RuleActions, CurrentReaderLosesAndRegainsProtectedStopDetail) {
    const RuleStop stop{
        1, Refusal::authorization_hold,
        RuleStopDetail{PermissionAutomationActionDecision{Outcome::permit, Outcome::deny,
                                                          PermissionAutomationDisposition::hold},
                       RuleAuthorityChannel::grant,
                       PermissionRational{"299999", "1"},
                       {},
                       {}}};
    expect_denial(stop, RuleAuthorityChannel::grant, Outcome::permit, Outcome::deny, "299999");
    expect_stop(stop, 1, Refusal::authorization_hold);
    expect_denial(stop, RuleAuthorityChannel::grant, Outcome::permit, Outcome::deny, "299999");
}

TEST(RuleActions, AnObservedTerminalDeliveryDoesNotReplaceQueueAcceptance) {
    const RuleDeliveryObservation queued{"job", RuleDeliveryState::queued, 1, {}, {}};
    const RuleDeliveryObservation sending{"job", RuleDeliveryState::sending, 2, 2, {}};
    const RuleDeliveryObservation delivered{"job", RuleDeliveryState::delivered, 3, 2, "report-A"};
    const auto observed = observe_rule_delivery({queued, sending}, delivered);
    EXPECT_EQ(observed.queued, queued);
    EXPECT_EQ(observed.latest, delivered);
}

} // namespace
