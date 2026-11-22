// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <map>
#include <numeric>
#include <string_view>
#include <subathon/core/rule_selection.hpp>
#include <utility>

namespace subathon::core {
namespace {

using Disposition = RuleSelectionDisposition;

struct RankedRules {
    std::vector<std::size_t> order;
    std::vector<std::size_t> position;
    std::vector<std::vector<std::size_t>> exclusions;
};

RankedRules rank_and_validate_edges(const RuleConfiguration &configuration) {
    // The immutable configuration has already passed structural validation and budget checks.
    // All indices below are bounded by its rule count and aggregate exclusion count.
    const auto &rules = configuration.data().rules;
    RankedRules ranked;
    ranked.order.resize(rules.size());
    ranked.position.resize(rules.size());
    ranked.exclusions.resize(rules.size());

    std::iota(ranked.order.begin(), ranked.order.end(), std::size_t{0});
    std::ranges::sort(ranked.order, [&](std::size_t left, std::size_t right) {
        const auto left_priority = rules[left].priority;
        const auto right_priority = rules[right].priority;
        return left_priority != right_priority ? left_priority > right_priority : left < right;
    });

    std::map<std::string_view, std::size_t> indices_by_ref;
    for (std::size_t position = 0; position < ranked.order.size(); ++position) {
        const auto index = ranked.order[position];
        ranked.position[index] = position;
        indices_by_ref.emplace(rules[index].rule_ref, index);
    }

    for (const auto source : ranked.order) {
        auto &targets = ranked.exclusions[source];
        targets.reserve(rules[source].excluded_rule_refs.size());
        for (const auto &ref : rules[source].excluded_rule_refs) {
            const auto target = indices_by_ref.find(ref);
            if (target == indices_by_ref.end() ||
                ranked.position[target->second] <= ranked.position[source])
                throw RuleDefinitionFailure(RuleDefinitionError::invalid_exclusion);
            targets.push_back(target->second);
        }
        std::ranges::sort(targets);
        const auto duplicates = std::ranges::unique(targets);
        targets.erase(duplicates.begin(), duplicates.end());
    }
    return ranked;
}

std::vector<RulePriorityTie> priority_ties(const std::vector<RuleDefinition> &rules,
                                           const std::vector<std::size_t> &order) {
    using GroupPriority = std::pair<std::string_view, std::int64_t>;
    std::map<GroupPriority, std::size_t> bucket_indices;
    std::vector<RulePriorityTie> buckets;
    for (const auto index : order) {
        const auto &rule = rules[index];
        if (!rule.enabled || !rule.exclusive_group_ref)
            continue;

        const auto [bucket, inserted] = bucket_indices.try_emplace(
            GroupPriority{*rule.exclusive_group_ref, rule.priority}, buckets.size());
        if (inserted)
            buckets.emplace_back();
        buckets[bucket->second].rule_indices.push_back(index);
    }

    // First encounter in rank order determines warning order, even if another tie fills sooner.
    std::erase_if(buckets, [](const auto &bucket) { return bucket.rule_indices.size() < 2; });
    return buckets;
}

} // namespace

std::vector<RulePriorityTie> validate_rule_selection(const RuleConfiguration &configuration) {
    const auto ranked = rank_and_validate_edges(configuration);
    return priority_ties(configuration.data().rules, ranked.order);
}

RuleSelectionResult select_rules(const RuleConfiguration &configuration,
                                 const RuleCapturedInput &input,
                                 std::span<const RuleExplicitValuation> valuations) {
    const auto ranked = rank_and_validate_edges(configuration);
    const auto &data = configuration.data();
    const auto truths = evaluate_rule_conditions(data, input, valuations);
    RuleSelectionResult result{
        .configuration_ref = data.configuration_ref,
        .event_ref = input.event_ref,
        .decisions = std::vector<RuleSelectionDecision>(data.rules.size()),
        .selected_rule_indices = {},
        .priority_ties = priority_ties(data.rules, ranked.order),
    };

    std::map<std::string_view, std::size_t> group_winners;
    std::vector<std::optional<std::size_t>> exclusion_blockers(data.rules.size());
    for (const auto index : ranked.order) {
        auto &decision = result.decisions[index];
        const auto truth = truths[index];
        if (!truth) {
            decision = {Disposition::disabled, {}};
            continue;
        }
        if (*truth != RuleTruth::match) {
            decision = {*truth == RuleTruth::unknown ? Disposition::condition_unknown
                                                     : Disposition::condition_no_match,
                        {}};
            continue;
        }

        const auto &group = data.rules[index].exclusive_group_ref;
        const auto group_winner = group ? group_winners.find(*group) : group_winners.end();
        const auto exclusion = exclusion_blockers[index];
        if (exclusion && (group_winner == group_winners.end() ||
                          ranked.position[*exclusion] <= ranked.position[group_winner->second])) {
            decision = {Disposition::excluded, exclusion};
            continue;
        }
        if (group_winner != group_winners.end()) {
            decision = {Disposition::exclusive_group, group_winner->second};
            continue;
        }

        decision = {Disposition::selected, {}};
        result.selected_rule_indices.push_back(index);
        if (group)
            group_winners.emplace(*group, index);
        // The first selected source for an edge is its highest-ranked blocker.
        for (const auto target : ranked.exclusions[index]) {
            if (!exclusion_blockers[target])
                exclusion_blockers[target] = index;
        }
    }
    return result;
}

} // namespace subathon::core
