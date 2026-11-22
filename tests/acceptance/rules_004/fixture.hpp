// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <gtest/gtest.h>
#include <map>
#include <subathon/core/rule_chain.hpp>

namespace rules_004 {
using namespace subathon::core;
using namespace subathon::core::rule_chain;

inline constexpr ChainProfile profile{{64, 1024}, 4, 1024, 64 * 1024 * 1024};

inline RuleActionKey key(std::string rule, std::string action, std::size_t position = 0) {
    return {std::move(rule), "v1", std::move(action), "v1", position};
}

inline RuleActionDefinition emit(std::string id, std::string name) {
    return {std::move(id), "v1", RuleNamedEvent{std::move(name), {}}};
}

inline RuleActionDefinition delta(std::string id, std::string amount,
                                  RuleDeltaUnit unit = RuleDeltaUnit::milliseconds) {
    return {std::move(id), "v1", RuleDeltaAction{unit, "A", RuleFixedDelta{std::move(amount)}}};
}

inline RuleDefinition named_rule(std::string ref, std::string name,
                                 std::vector<RuleActionDefinition> actions) {
    return {
        .rule_ref = ref,
        .version_ref = "v1",
        .name = std::move(ref),
        .description = {},
        .exclusive_group_ref = {},
        .excluded_rule_refs = {},
        .common_condition = {},
        .triggers = {{RuleTriggerKind::named_event,
                      {RulePredicate{RuleComparison<RuleText, RuleEquality>{
                          {RuleTextField::event_name, std::move(name)}, RuleEquality::equal}}}}},
        .actions = std::move(actions)};
}

inline RootBinding root(std::string id = "R", std::string capture = "CR") {
    return {{{"I", id}, id, {}, std::move(capture), {}}, "normalized-" + id};
}

inline EventBinding child(const EventBinding &parent, std::string id, ChainCause cause,
                          std::string capture = {}) {
    if (capture.empty())
        capture = "C" + id;
    return {parent.chain, std::move(id), parent.event, std::move(capture), std::move(cause)};
}

inline RuleEventLineage lineage(const EventBinding &event) {
    std::optional<RuleActionKey> cause;
    if (event.cause) {
        if (const auto *named = std::get_if<NamedAction>(&*event.cause))
            cause = named->key;
    }
    return {event.event, event.parent, event.chain.root, std::move(cause)};
}

inline UnitIdentity event_unit(std::string id, const EventBinding &event) {
    return {std::move(id), event.chain, event.event, UnitPurpose::event_pass};
}

inline void expect_cutoff(const Cutoff &actual, const Cutoff &expected) {
    EXPECT_EQ(actual.chain, expected.chain);
    EXPECT_EQ(actual.root_capture, expected.root_capture);
    EXPECT_EQ(actual.last, expected.last);
    EXPECT_EQ(actual.limits.maximum_depth, expected.limits.maximum_depth);
    EXPECT_EQ(actual.limits.maximum_generated_events, expected.limits.maximum_generated_events);
    EXPECT_EQ(actual.reason, expected.reason);
}

inline void expect_owner_stop(const RuleConsumedActions &actions, std::size_t position,
                              std::string_view reason) {
    ASSERT_TRUE(actions.stop);
    EXPECT_EQ(actions.stop->position, position);
    EXPECT_EQ(actions.stop->refusal, RuleActionRefusal::handler_refusal);
    ASSERT_TRUE(actions.stop->detail);
    EXPECT_EQ(actions.stop->detail->opaque_reason, reason);
}

inline void expect_use_stop(const RuleConsumedActions &actions, std::size_t position) {
    ASSERT_TRUE(actions.stop);
    EXPECT_EQ(actions.stop->position, position);
    EXPECT_EQ(actions.stop->refusal, RuleActionRefusal::authorization_hold);
    ASSERT_TRUE(actions.stop->detail);
    EXPECT_EQ(actions.stop->detail->channel, RuleAuthorityChannel::use);
}

inline RuleEffectObservation time_effect(std::string ref, std::int64_t amount, std::int64_t after) {
    return {std::move(ref), RuleTimeObservation{amount, amount, 0, 0, after}};
}

inline void expect_time(const RuleAcceptedAction &action, const RuleActionKey &expected_key,
                        std::int64_t amount, std::int64_t after) {
    EXPECT_EQ(action.key, expected_key);
    const auto &observed = std::get<RuleEffectObservation>(action.observed);
    EXPECT_EQ(observed.effect_ref, expected_key.action_ref);
    EXPECT_EQ(std::get<RuleTimeObservation>(observed.change),
              (RuleTimeObservation{amount, amount, 0, 0, after}));
}

inline RuleActionAuthorityFacts authority(const PermissionRequest &request, bool deny_use) {
    const auto &resource = request.resources.at(0);
    std::vector<PermissionQuantityConstraint> quantities;
    std::vector<PermissionQuantityDefinition> slots;
    if (!request.quantities.empty()) {
        const auto &quantity = request.quantities.front();
        quantities.push_back({quantity.slot, quantity.unit, PermissionQuantityMode::closed_interval,
                              PermissionRational{"0", "1"}, PermissionRational{"25", "1"}});
        slots.push_back({quantity.slot, quantity.unit, PermissionNumberKind::rational, {}, {}, 15});
    }
    const PermissionPolicyEntry allow{
        PermissionOrigin::individual,
        "actor",
        {},
        request.operation,
        PermissionEffect::allow,
        "permit",
        {{resource.slot, PermissionResourceMode::allowed_set, {resource.resource}}},
        std::move(quantities)};
    return {"current",
            {PermissionFactsState::complete,
             "catalog",
             {{request.operation,
               PermissionOperationClass::ordinary,
               {resource.slot},
               std::move(slots),
               {}}}},
            {PermissionOwnerState::present, "owner"},
            {PermissionFactsState::complete,
             {},
             deny_use ? std::vector<PermissionPolicyEntry>{} : std::vector{allow}},
            {PermissionFactsState::complete, {}, {allow}}};
}

enum class NamedFault { none, unknown, wrong_parent, refused };

struct Candidate {
    EventBinding binding;
    std::string name;
};

struct Capture {
    std::string event;
    const RuleConfiguration &configuration;
    RuleTriggerKind kind;
    std::vector<RuleCapturedFact> facts;
    std::vector<std::string> authors;
};

struct Owner {
    std::map<std::string, RuleConfiguration> configurations;
    std::map<std::string, Capture> captures;
    std::string active;
    CapacityUse other{};
    std::size_t capture_reads = 0;
    std::size_t capacity_reads = 0;
    std::vector<std::string> calls;
    std::vector<RuleActionKey> mapped;
    std::vector<RuleActionKey> offered;
    std::vector<RuleActionKey> handled;
    std::vector<AdmittedEvent> queued;
    std::vector<Cutoff> audits;
    std::vector<std::pair<ChainKey, std::uint32_t>> notices;
    std::int64_t time = 0;
    std::int64_t score = 0;

    const RuleConfiguration &define(std::string id, RuleGenerationLimits limits,
                                    std::vector<RuleDefinition> rules) {
        auto value = validate_rule_configuration({id, limits, std::move(rules)});
        static_cast<void>(validate_rule_selection(value));
        return configurations.emplace(std::move(id), std::move(value)).first->second;
    }

    PolicyCheck activate(const std::string &id, const ChainProfile &runtime) {
        const auto result = check_generation_policy(configurations.at(id), runtime);
        if (result == PolicyCheck::supported)
            active = id;
        return result;
    }

    void keep(const EventBinding &event, std::string value,
              RuleTriggerKind kind = RuleTriggerKind::named_event) {
        const auto &configuration = configurations.at(active);
        const auto field = kind == RuleTriggerKind::named_event ? RuleTextField::event_name
                                                                : RuleTextField::milestone_ref;
        const auto [_, inserted] = captures.emplace(
            event.capture,
            Capture{event.event,
                    configuration,
                    kind,
                    {RuleText{field, std::move(value)}},
                    std::vector<std::string>(configuration.data().rules.size(), "author")});
        if (!inserted)
            throw std::logic_error("fixture reused an immutable capture");
    }

    OwnerView view() {
        return {[this](const EventBinding &event) -> std::optional<CaptureView> {
                    ++capture_reads;
                    const auto selected = captures.find(event.capture);
                    if (selected == captures.end())
                        return std::nullopt;
                    const auto &capture = selected->second;
                    return CaptureView{capture.configuration,
                                       {capture.event,
                                        capture.configuration.data().configuration_ref,
                                        capture.kind,
                                        capture.facts,
                                        nullptr,
                                        {}},
                                       capture.authors,
                                       {}};
                },
                [this](const ChainKey &) {
                    ++capacity_reads;
                    return other;
                }};
    }

    OpenResult open(const RootBinding &binding, const ChainProfile &runtime = profile) {
        return open_chain(binding, runtime, VerifiedNew{"admit-" + binding.event.event}, view());
    }

    RuleActionMapper mapper() {
        return [this](const RuleActionKey &action, const RuleActionPayload &payload,
                      const RuleCapturedInput &) -> RuleActionBinding {
            mapped.push_back(action);
            if (const auto *named = std::get_if<RuleNamedEvent>(&payload))
                return RuleBoundAction{{"event.emit", {{"name", named->event_name}}, {}}, payload};
            const auto &change = std::get<RuleDeltaAction>(payload);
            const auto &fixed = std::get<RuleFixedDelta>(change.delta);
            const bool timer = change.unit == RuleDeltaUnit::milliseconds;
            return RuleBoundAction{
                {timer ? "timer.adjust" : "score.adjust",
                 {{timer ? "timer" : "track", change.target_ref}},
                 {{"amount", timer ? "ms" : "micro_points", {fixed.original_decimal, "1"}}}},
                payload};
        };
    }

    RuleActionChecks checks(const std::string &deny) {
        return {
            [this, deny](std::string_view author, const RuleActionKey &action,
                         const RuleBoundAction &bound) -> std::optional<RuleActionAuthorityFacts> {
                EXPECT_EQ(author, "author");
                calls.push_back("authority:" + action.action_ref);
                return authority(bound.request, action.action_ref == deny);
            },
            [this](const RuleBoundAction &bound, std::string_view snapshot) {
                EXPECT_EQ(snapshot, "current");
                calls.push_back("execution:" + bound.request.operation);
                return RuleGate::permit;
            },
            [this](const RuleBoundAction &bound) {
                calls.push_back("target:" + bound.request.operation);
                return true;
            },
            [this](const RuleBoundAction &bound) {
                calls.push_back("capability:" + bound.request.operation);
                return RuleCapability::supported;
            }};
    }

    RulePass consume(Unit &unit, const EventBinding &event, const RuleActionPlan &plan,
                     const std::map<std::string, Candidate> &candidates = {}, std::string deny = {},
                     NamedFault fault = NamedFault::none, std::string numeric_hold = {}) {
        auto staged_time = time;
        auto submit = chain_action_submit(
            unit,
            [&](const RuleActionKey &action, const RuleNamedEvent &bound) {
                offered.push_back(action);
                calls.push_back("offer:" + action.action_ref);
                const auto &candidate = candidates.at(action.action_ref);
                EXPECT_EQ(bound.event_name, candidate.name);
                EXPECT_FALSE(bound.payload_version_ref);
                EXPECT_EQ(candidate.binding.cause, ChainCause{NamedAction{action}});
                return candidate.binding;
            },
            [&](const AdmittedEvent &reserved, const RuleNamedEvent &bound,
                std::string_view snapshot) -> NamedOutcome {
                const auto &action = std::get<NamedAction>(reserved.binding.cause.value()).key;
                const auto &candidate = candidates.at(action.action_ref);
                EXPECT_EQ(snapshot, "current");
                EXPECT_EQ(reserved.binding, candidate.binding);
                EXPECT_EQ(bound.event_name, candidate.name);
                EXPECT_FALSE(bound.payload_version_ref);
                handled.push_back(action);
                if (fault == NamedFault::unknown)
                    return OutcomeUnknown{};
                if (fault == NamedFault::refused)
                    return RuleOwnerResult{RuleOwnerRefusal{"owner_declined"}};
                auto observed = lineage(candidate.binding);
                if (fault == NamedFault::wrong_parent)
                    observed.parent_ref = event.chain.root;
                return RuleOwnerResult{RuleOwnerAcceptance{observed}};
            },
            [&](const RuleEventLineage &seen, const RuleActionKey &action,
                const RuleBoundAction &bound, std::string_view snapshot) -> RuleOwnerResult {
                EXPECT_EQ(seen, lineage(event));
                EXPECT_EQ(snapshot, "current");
                handled.push_back(action);
                if (action.action_ref == numeric_hold)
                    return RulePassFailure::numeric_hold;
                const auto &change = std::get<RuleDeltaAction>(bound.payload);
                const auto amount =
                    std::stoll(std::get<RuleFixedDelta>(change.delta).original_decimal);
                if (change.unit == RuleDeltaUnit::micro_points)
                    return RuleOwnerAcceptance{RuleEffectObservation{
                        action.action_ref, RuleQuantityObservation{"micro_points",
                                                                   {std::to_string(amount), "1"},
                                                                   {std::to_string(amount), "1"}}}};
                staged_time += amount;
                return RuleOwnerAcceptance{time_effect(action.action_ref, amount, staged_time)};
            });
        return consume_rule_actions(plan, checks(deny), submit);
    }

    // Consume only new publications. Reconciliation readback is inspected by the
    // caller without submitting its previously committed publication again.
    void publish(const Publication &publication) {
        queued.insert(queued.end(), publication.children.begin(), publication.children.end());
        if (publication.cutoff) {
            audits.push_back(*publication.cutoff);
            notices.emplace_back(publication.cutoff->chain, publication.cutoff->last.ordinal);
        }
        if (!publication.pass)
            return;
        for (const auto &rule : publication.pass->rules) {
            for (const auto &action : rule.accepted) {
                const auto *effect = std::get_if<RuleEffectObservation>(&action.observed);
                if (!effect)
                    continue;
                if (const auto *timer = std::get_if<RuleTimeObservation>(&effect->change))
                    time = timer->main_after_ms;
                if (const auto *quantity = std::get_if<RuleQuantityObservation>(&effect->change))
                    score += std::stoll(quantity->actual.numerator);
            }
        }
    }

    Publication run(Chain &chain, const EventBinding &event, std::string id,
                    const std::map<std::string, Candidate> &candidates = {},
                    std::string deny = {}) {
        auto unit = std::get<Unit>(begin_event(chain, event, id));
        const auto plan = plan_event(unit, mapper());
        EXPECT_EQ(plan.event, lineage(event));
        const auto staged = std::get<RuleStagedPass>(consume(unit, event, plan, candidates, deny));
        auto result =
            std::get<Publication>(finish_unit(std::move(unit), staged, RuleCompletion::committed));
        EXPECT_EQ(result.identity, event_unit(id, event));
        if (!result.pass)
            throw std::logic_error("committed event unit omitted its rule pass");
        publish(result);
        return result;
    }
};

inline void expect_duplicate(const Chain &chain, const AdmittedEvent &expected) {
    const auto found = lookup_child(chain, expected.binding);
    ASSERT_TRUE(std::holds_alternative<Duplicate>(found));
    EXPECT_EQ(std::get<Duplicate>(found).event, expected);
}

inline void expect_suppression(const Publication &passed, const RuleActionKey &action,
                               const Cutoff &cutoff) {
    ASSERT_EQ(passed.named_suppressions.size(), 1U);
    EXPECT_EQ(passed.named_suppressions.front().action, action);
    expect_cutoff(std::get<Cutoff>(passed.named_suppressions.front().stop), cutoff);
    EXPECT_FALSE(passed.cutoff);
    EXPECT_TRUE(passed.children.empty());
}
} // namespace rules_004
