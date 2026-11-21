// SPDX-License-Identifier: GPL-3.0-or-later
#include "modifier_stacking_internal.hpp"

namespace subathon::core::modifier_detail {
namespace {
Op operation(std::optional<Op> op) {
    const auto effective = op.value_or(Op::multiply);
    switch (effective) {
    case Op::multiply:
    case Op::additive:
    case Op::highest:
    case Op::lowest:
    case Op::priority:
        return effective;
    }
    throw Failure::invalid;
}

void priority_input(Budget &budget, const std::optional<ModifierPriority> &priority) {
    budget.input.add(priority ? 17 : 1);
}

template <class Candidates>
void validate_priorities(Budget &budget, Op op, const Candidates &candidates) {
    if (op != Op::priority)
        return;
    Index<std::pair<std::int64_t, std::size_t>, bool> orders{MeteredLess{&budget}};
    for (const auto &candidate : candidates) {
        budget.work();
        const auto &priority = candidate.priority;
        require(priority.has_value());
        require(orders.try_emplace({priority->rank, priority->order}, true).second);
    }
}

Exact add_bonus(Budget &budget, const Exact &running, const Exact &factor) {
    using boost::multiprecision::denominator;
    using boost::multiprecision::numerator;
    const auto d = denominator(factor);
    // The nonnegative factor guarantees |n - d| fits 4096 bits. Each
    // cross product fits 8192 bits and their signed sum fits 8193 bits.
    budget.work(5);
    const Integer bonus = numerator(factor) - d;
    const auto running_denominator = denominator(running);
    return normalized(budget, numerator(running) * d + bonus * running_denominator,
                      running_denominator * d);
}

int compare(Budget &budget, const Exact &left, const Exact &right) {
    budget.work(3);
    const Integer l =
        boost::multiprecision::numerator(left) * boost::multiprecision::denominator(right);
    const Integer r =
        boost::multiprecision::numerator(right) * boost::multiprecision::denominator(left);
    return l.compare(r);
}

const auto &priority_of(const MemberDraft &member) { return member.priority; }
const auto &priority_of(const GroupDraft &group) { return group.policy->priority; }

template <class Candidates, class Factor>
ModifierReduction reduce(Budget &budget, Op op, Candidates &candidates, Factor factor_of) {
    Exact raw = 1;
    if (candidates.empty())
        return {op, raw, raw};

    if (op == Op::multiply || op == Op::additive) {
        for (auto &candidate : candidates) {
            const auto &factor = factor_of(candidate);
            raw =
                op == Op::multiply ? multiply(budget, raw, factor) : add_bonus(budget, raw, factor);
            candidate.winner = true;
        }
    } else {
        auto *winner = &candidates.front();
        for (auto &candidate : candidates) {
            if (op == Op::priority) {
                budget.work();
                const auto &a = *priority_of(candidate);
                const auto &b = *priority_of(*winner);
                if (a.rank > b.rank || (a.rank == b.rank && a.order < b.order))
                    winner = &candidate;
            } else {
                const auto order = compare(budget, factor_of(candidate), factor_of(*winner));
                if ((op == Op::highest && order > 0) || (op == Op::lowest && order < 0))
                    winner = &candidate;
            }
        }
        raw = factor_of(*winner);
        if (op == Op::priority) {
            winner->winner = true;
        } else {
            for (auto &candidate : candidates) {
                // Canonical pairs compare equal exactly when both components match.
                budget.work(2);
                const auto &factor = factor_of(candidate);
                candidate.winner =
                    numerator_backend(factor.backend()).compare(numerator_backend(raw.backend())) ==
                        0 &&
                    denominator_backend(factor.backend())
                            .compare(denominator_backend(raw.backend())) == 0;
            }
        }
    }
    budget.work();
    Exact clamped = numerator_backend(raw.backend()).sign() ? Exact(0) : raw;
    return {op, std::move(raw), std::move(clamped)};
}

// Use the same trace charge for the owned target trace and each request that
// retains it. Include the plan-row ordinal carried by each versioned trace link.
std::size_t trace_size(Budget &budget, const TargetDraft &target, std::string_view policy_ref) {
    Bytes bytes(budget.value_bytes);
    bytes.add(10); // Target kind, operation and group-vector length.
    bytes.text(std::get<1>(target.target));
    bytes.text(policy_ref);
    exact_bytes(budget, bytes, target.reduction.raw);
    exact_bytes(budget, bytes, target.reduction.factor);
    for (const auto &group : target.groups) {
        bytes.add(10 + (group.policy->priority ? 17 : 1));
        bytes.text(group.policy->group_ref);
        exact_bytes(budget, bytes, group.reduction.raw);
        exact_bytes(budget, bytes, group.reduction.factor);
        for (const auto &member : group.members) {
            const auto &key = member.operand->key;
            bytes.add(18 + (member.priority ? 17 : 1));
            bytes.text(std::get<0>(key));
            bytes.text(std::get<1>(key));
            bytes.text(std::get<2>(key));
            bytes.text(std::get<1>(member.operand->target));
            bytes.text(member.operand->spelling);
            exact_bytes(budget, bytes, member.operand->factor);
        }
    }
    return bytes.used();
}

ModifierTargetKey own_target(Target key) {
    return {std::get<0>(key), std::string(std::get<1>(key))};
}
} // namespace

void policy_input(Budget &budget, const ModifierStackPolicy &policy) {
    fit(policy.targets.size(), budget.targets);
    auto entries_left = budget.policy_entries;
    debit(entries_left, policy.targets.size());
    budget.refs({policy.policy_ref, policy.configuration_ref});
    budget.input.add(8);
    for (const auto &target : policy.targets) {
        budget.work();
        debit(entries_left, target.groups.size());
        target_input(budget, view(target.target));
        (void)operation(target.operation);
        budget.input.add(10);
        for (const auto &group : target.groups) {
            budget.work();
            debit(entries_left, group.members.size());
            budget.refs({group.group_ref});
            (void)operation(group.operation);
            budget.input.add(10);
            priority_input(budget, group.priority);
            for (const auto &member : group.members) {
                budget.work();
                key_input(budget, view(member.occurrence));
                priority_input(budget, member.priority);
            }
        }
    }
}

std::vector<TargetDraft> assign(Budget &budget, std::span<const Operand> occurrences,
                                const ModifierStackPolicy &policy,
                                std::span<const Target> requested) {
    Index<Key, const Operand *> unassigned{MeteredLess{&budget}};
    for (const auto &occurrence : occurrences) {
        budget.work();
        require(unassigned.try_emplace(occurrence.key, &occurrence).second);
    }

    Index<Target, bool> wanted{MeteredLess{&budget}};
    for (const auto &target : requested) {
        budget.work();
        wanted.try_emplace(target, true);
    }
    Index<Target, bool> seen_targets{MeteredLess{&budget}};
    Index<Key, bool> seen_members{MeteredLess{&budget}};
    std::vector<TargetDraft> drafts;
    auto target_slots = budget.targets;
    for (const auto &target : policy.targets) {
        const auto key = view(target.target);
        require(seen_targets.try_emplace(key, true).second);
        TargetDraft draft{key, {operation(target.operation), {}, {}}, {}};
        Index<std::string_view, bool> seen_groups{MeteredLess{&budget}};
        Index<std::pair<std::int64_t, std::size_t>, bool> group_orders{MeteredLess{&budget}};
        for (const auto &group : target.groups) {
            budget.work();
            require(seen_groups.try_emplace(group.group_ref, true).second);
            GroupDraft selected{&group, {operation(group.operation), {}, {}}, {}};
            for (const auto &member : group.members) {
                budget.work();
                const auto member_key = view(member.occurrence);
                require(seen_members.try_emplace(member_key, true).second);
                const auto found = unassigned.find(member_key);
                if (found == unassigned.end())
                    continue;
                require(budget.equal(found->second->target, key));
                selected.members.push_back({found->second, member.priority});
                unassigned.erase(found);
            }
            if (selected.members.empty())
                continue;
            validate_priorities(budget, selected.reduction.operation, selected.members);
            if (draft.reduction.operation == Op::priority) {
                budget.work();
                require(group.priority.has_value());
                require(
                    group_orders.try_emplace({group.priority->rank, group.priority->order}, true)
                        .second);
            }
            draft.groups.push_back(std::move(selected));
        }
        if (wanted.erase(key) != 0 || !draft.groups.empty()) {
            debit(target_slots, 1);
            drafts.push_back(std::move(draft));
        }
    }
    require(unassigned.empty());
    for (const auto &target : requested) {
        if (wanted.erase(target) != 0) {
            debit(target_slots, 1);
            drafts.push_back({target, {Op::multiply, {}, {}}, {}});
        }
    }

    return drafts;
}

void reduce_traces(Budget &budget, std::span<TargetDraft> drafts, std::string_view policy_ref) {
    budget.output.add(8);
    for (auto &target : drafts) {
        for (auto &group : target.groups)
            group.reduction = reduce(
                budget, group.reduction.operation, group.members,
                [](const MemberDraft &member) -> const Exact & { return member.operand->factor; });
        target.reduction =
            reduce(budget, target.reduction.operation, target.groups,
                   [](const GroupDraft &group) -> const Exact & { return group.reduction.factor; });
        target.trace_bytes = trace_size(budget, target, policy_ref);
        budget.output.add(target.trace_bytes);
    }
}

ModifierOccurrenceKey own_key(Key key) {
    const auto [instance, revision, definition, ordinal] = key;
    return {std::string(instance), std::string(revision), std::string(definition), ordinal};
}

std::vector<ModifierTargetTrace> own_traces(std::span<const TargetDraft> drafts,
                                            std::string_view policy_ref) {
    std::vector<ModifierTargetTrace> traces;
    traces.reserve(drafts.size());
    for (const auto &draft : drafts) {
        ModifierTargetTrace target{
            own_target(draft.target), std::string(policy_ref), draft.reduction, {}};
        target.groups.reserve(draft.groups.size());
        for (const auto &group : draft.groups) {
            ModifierGroupTrace trace{
                group.policy->group_ref, group.policy->priority, group.reduction, {}, group.winner};
            trace.members.reserve(group.members.size());
            for (const auto &member : group.members) {
                const auto &operand = *member.operand;
                trace.members.push_back({{own_key(operand.key), own_target(operand.target),
                                          std::string(operand.spelling), operand.factor},
                                         member.priority,
                                         member.winner});
            }
            target.groups.push_back(std::move(trace));
        }
        traces.push_back(std::move(target));
    }
    return traces;
}
} // namespace subathon::core::modifier_detail

namespace subathon::core {
ModifierCombination combine_modifiers(std::span<const QualifiedModifierOccurrence> occurrences,
                                      const ModifierStackPolicy &policy,
                                      std::span<const ModifierTargetKey> targets,
                                      const ModifierStackLimits &limits) {
    using namespace modifier_detail;
    try {
        Budget budget(limits);
        fit(occurrences.size(), budget.occurrences);
        fit(targets.size(), budget.targets);
        policy_input(budget, policy);
        budget.input.add(16);
        for (const auto &occurrence : occurrences) {
            budget.work();
            key_input(budget, view(occurrence.key));
            target_input(budget, view(occurrence.target));
            factor_input(budget, occurrence.original_factor_decimal);
            exact_shape(occurrence.factor);
        }
        for (const auto &target : targets) {
            budget.work();
            target_input(budget, view(target));
        }
        for (const auto &occurrence : occurrences)
            exact_input(budget, occurrence.factor);

        std::vector<Operand> operands;
        operands.reserve(occurrences.size());
        for (const auto &occurrence : occurrences) {
            auto parsed = factor(budget, occurrence.original_factor_decimal);
            require(budget.equal(parsed, occurrence.factor));
            operands.push_back({view(occurrence.key), view(occurrence.target),
                                occurrence.original_factor_decimal, std::move(parsed)});
        }
        std::vector<Target> requested;
        requested.reserve(targets.size());
        for (const auto &target : targets)
            requested.push_back(view(target));
        auto drafts = assign(budget, operands, policy, requested);
        reduce_traces(budget, drafts, policy.policy_ref);
        return own_traces(drafts, policy.policy_ref);
    } catch (Failure failure) {
        return failure;
    } catch (const std::length_error &) {
        return Failure::resource_limit;
    } catch (const std::invalid_argument &) {
        return Failure::invalid;
    }
}
} // namespace subathon::core
