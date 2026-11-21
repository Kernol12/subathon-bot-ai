// SPDX-License-Identifier: GPL-3.0-or-later
#include "modifier_stacking_internal.hpp"

#include <array>
#include <subathon/server/modifier_stacking.hpp>

namespace subathon::server {
namespace {
using namespace core;
using namespace core::modifier_detail;

void profile_input(Budget &budget, const ModifierTraceProfile &profile) {
    budget.refs({profile.producer_ref, profile.profile_ref});
}

void context_input(Budget &budget, const ModifierStackCapture &capture) {
    const auto &arrival = capture.arrival.get();
    budget.refs({capture.original_arrival_ref, arrival.connection_ref, arrival.configuration_ref,
                 arrival.modifier_context_ref});
    budget.input.add(10); // Timestamp and the two optional bindings.
    if (capture.admitted_profile)
        profile_input(budget, *capture.admitted_profile);
    if (capture.recalculation) {
        const auto &recalculation = *capture.recalculation;
        const auto &decision = recalculation.decision.get();
        budget.refs({recalculation.plan_ref, decision.actor_ref, decision.decision_ref});
        budget.value(decision.reason);
        budget.input.add(9);
        require(recalculation.revision > 1 && decision.authority == OverrideAuthority::authorized &&
                !decision.reason.empty());
    }
    if (capture.context) {
        const auto &context = *capture.context;
        budget.refs({context.document_ref, context.configuration_ref, context.policy_ref});
        profile_input(budget, context.profile);
        budget.input.add(1);
        if (context.recalculation) {
            const auto &binding = *context.recalculation;
            budget.refs({binding.plan_ref, binding.original_arrival_ref, binding.decision_ref});
            budget.input.add(8);
        }
    }
}

void validate_context(Budget &budget, const ModifierStackCapture &capture) {
    const auto &arrival = capture.arrival.get();
    if (capture.recalculation)
        require(!budget.equal(capture.recalculation->decision.get().decision_ref,
                              arrival.modifier_context_ref));

    require(capture.context != nullptr, Failure::unresolved);
    const auto &context = *capture.context;
    if (capture.recalculation) {
        const auto &recalculation = *capture.recalculation;
        const auto &decision_ref = recalculation.decision.get().decision_ref;
        require(context.recalculation.has_value());
        const auto &binding = *context.recalculation;
        require(budget.equal(context.document_ref, decision_ref) &&
                budget.equal(binding.decision_ref, decision_ref) &&
                budget.equal(binding.plan_ref, recalculation.plan_ref) &&
                binding.revision == recalculation.revision &&
                budget.equal(binding.original_arrival_ref, capture.original_arrival_ref));
    } else {
        require(!context.recalculation &&
                budget.equal(context.document_ref, arrival.modifier_context_ref) &&
                budget.equal(context.configuration_ref, arrival.configuration_ref));
    }
    if (capture.admitted_profile)
        require(budget.equal(context.profile, *capture.admitted_profile));
    if (capture.policy)
        require(budget.equal(context.policy_ref, capture.policy->policy_ref) &&
                budget.equal(context.configuration_ref, capture.policy->configuration_ref));

    require(capture.admitted_profile && capture.policy && capture.eligible, Failure::unresolved);
}

void definition_input(Budget &budget, const ModifierDefinition &definition) {
    // These are source rows, including unmatched qualifiers. They are bounded
    // by the byte/work envelope, independently of the qualified occurrence count.
    budget.refs({definition.definition_ref, definition.revision_ref});
    budget.input.add(17);
    for (const auto &row : definition.targets) {
        budget.work();
        target_input(budget, {row.kind, row.target_ref});
        budget.input.add(1);
        if (row.qualifier_ref)
            budget.refs({*row.qualifier_ref});
        factor_input(budget, row.factor_decimal);
    }
}

void revisions_input(Budget &budget, std::span<const SelectedModifierRevision> revisions) {
    budget.input.add(8);
    for (const auto &selected : revisions) {
        budget.work();
        budget.refs({selected.instance_ref, selected.instance_revision_ref,
                     selected.definition_revision_ref});
        budget.input.add(2);
        if (selected.instance) {
            const auto &instance = *selected.instance;
            budget.refs({instance.instance_ref, instance.revision_ref, instance.source.source_ref});
            budget.input.add(11);
            if (instance.parent_revision_ref)
                budget.refs({*instance.parent_revision_ref});
            if (instance.trigger) {
                budget.refs({instance.trigger->event_ref});
                budget.input.add(1);
            }
            definition_input(budget, instance.captured_definition);
        }
        if (selected.definition &&
            (!selected.instance || selected.definition != &selected.instance->captured_definition))
            definition_input(budget, *selected.definition);
    }
}

bool same_definition(Budget &budget, const ModifierDefinition &a, const ModifierDefinition &b) {
    if (&a == &b)
        return true;
    if (!budget.equal(a.definition_ref, b.definition_ref) ||
        !budget.equal(a.revision_ref, b.revision_ref) || a.duration_ms != b.duration_ms ||
        a.reactivation != b.reactivation || a.targets.size() != b.targets.size())
        return false;
    for (std::size_t i = 0; i < a.targets.size(); ++i) {
        if (!budget.equal(a.targets[i], b.targets[i]))
            return false;
    }
    return true;
}

void validate_revisions(Budget &budget, std::span<const SelectedModifierRevision> revisions) {
    Index<std::string_view, bool> instances{MeteredLess{&budget}};
    Index<std::string_view, bool> instance_revisions{MeteredLess{&budget}};
    Index<std::string_view, const ModifierDefinition *> definitions{MeteredLess{&budget}};
    bool missing = false;
    for (const auto &selected : revisions) {
        budget.work();
        require(instances.try_emplace(selected.instance_ref, true).second);
        require(instance_revisions.try_emplace(selected.instance_revision_ref, true).second);
        const auto *instance = selected.instance;
        const auto *definition = selected.definition;
        if (instance)
            require(budget.equal(selected.instance_ref, instance->instance_ref) &&
                    budget.equal(selected.instance_revision_ref, instance->revision_ref) &&
                    budget.equal(selected.definition_revision_ref,
                                 instance->captured_definition.revision_ref));
        if (definition)
            require(budget.equal(selected.definition_revision_ref, definition->revision_ref));
        if (instance && definition)
            require(same_definition(budget, instance->captured_definition, *definition));

        // A single retained definition revision cannot supply different contents
        // through different selected lineage reads.
        const auto *known = definition ? definition
                            : instance ? &instance->captured_definition
                                       : nullptr;
        if (known) {
            const auto [found, inserted] = definitions.try_emplace(known->revision_ref, known);
            require(inserted || same_definition(budget, *found->second, *known));
        }
        missing |= !instance || !definition;
    }
    require(!missing, Failure::unresolved);
}

std::vector<Operand> project(Budget &budget, std::span<const SelectedModifierRevision> revisions,
                             std::span<const std::string_view> matches,
                             std::size_t selection_rows_left) {
    Index<std::string_view, bool> qualifiers{MeteredLess{&budget}};
    for (const auto match : matches) {
        budget.work();
        qualifiers.try_emplace(match, true);
    }

    // #215 parses the factor before testing the qualifier. Walk the retained
    // definition itself so filtered and equal rows keep their source ordinals.
    std::vector<Operand> operands;
    auto ordinal_slots = std::min(budget.occurrences, selection_rows_left);
    for (const auto &selected : revisions) {
        std::size_t ordinal = 0;
        for (const auto &row : selected.definition->targets) {
            ++ordinal;
            auto parsed = factor(budget, row.factor_decimal);
            if (row.qualifier_ref && !qualifiers.contains(*row.qualifier_ref))
                continue;
            debit(ordinal_slots, 1);
            operands.push_back({{selected.instance_ref, selected.instance_revision_ref,
                                 selected.definition_revision_ref, ordinal},
                                {row.kind, row.target_ref},
                                row.factor_decimal,
                                std::move(parsed)});
        }
    }
    return operands;
}

using SelectionFields = std::array<std::string_view, 7>;

SelectionFields fields(const ModifierSelection &row) {
    return {row.definition_version_ref,
            row.instance_revision_ref,
            row.target_ref,
            row.group_ref,
            row.group_operator,
            row.target_operator,
            row.original_factor_decimal};
}

void recorded_input(Budget &budget, const ModifierStackCapture &capture) {
    const auto &rows = capture.recorded_selection;
    fit(capture.recorded_links.size(), budget.occurrences);
    require(rows ? rows->size() == capture.recorded_links.size() : capture.recorded_links.empty());
    budget.input.add(17);
    if (!rows)
        return;
    fit(rows->size(), budget.occurrences);
    for (const auto &row : *rows) {
        budget.work();
        const auto [definition, instance, target, group, group_op, target_op, factor] = fields(row);
        budget.refs({definition, instance, target, group, group_op, target_op});
        factor_input(budget, factor);
    }
    for (const auto &link : capture.recorded_links) {
        budget.work();
        require(link.plan_row_ordinal != 0 && link.plan_row_ordinal <= rows->size());
        key_input(budget, view(link.occurrence));
        budget.input.add(8);
    }
}

std::string_view operator_name(Op operation) {
    switch (operation) {
    case Op::multiply:
        return "multiply";
    case Op::additive:
        return "additive";
    case Op::highest:
        return "highest";
    case Op::lowest:
        return "lowest";
    case Op::priority:
        return "priority";
    }
    throw Failure::invalid;
}

struct SelectionDraft {
    Key key;
    SelectionFields fields;
};

std::vector<SelectionDraft> selection_drafts(Budget &budget, std::span<const TargetDraft> targets,
                                             const ModifierStackCapture &capture,
                                             std::size_t count) {
    std::vector<SelectionDraft> selection;
    selection.reserve(count);
    for (const auto &target : targets) {
        for (const auto &group : target.groups) {
            for (const auto &member : group.members) {
                budget.work();
                const auto &key = member.operand->key;
                selection.push_back(
                    {key,
                     {std::get<2>(key), std::get<1>(key), std::get<1>(target.target),
                      group.policy->group_ref, operator_name(group.reduction.operation),
                      operator_name(target.reduction.operation), member.operand->spelling}});
            }
        }
    }
    if (capture.recorded_selection) {
        const auto rows = *capture.recorded_selection;
        require(rows.size() == selection.size());
        Index<Key, const SelectionDraft *> unused{MeteredLess{&budget}};
        for (const auto &row : selection)
            unused.try_emplace(row.key, &row);
        std::vector<SelectionDraft> recorded(rows.size());
        for (const auto &link : capture.recorded_links) {
            budget.work();
            const auto index = link.plan_row_ordinal - 1;
            const auto found = unused.find(view(link.occurrence));
            require(found != unused.end() && recorded[index].fields[0].empty());
            require(budget.equal(found->second->fields, fields(rows[index])));
            recorded[index] = *found->second;
            unused.erase(found);
        }
        selection = std::move(recorded);
    }

    budget.output.add(16);
    for (const auto &row : selection) {
        for (const auto field : std::span{row.fields}.first<6>()) {
            fit(field.size(), budget.ref_bytes);
            budget.output.text(field);
        }
        budget.output.text(row.fields.back());
        budget.output.add(16); // Plan row and original target ordinals in the owned link.
        budget.output.text(std::get<0>(row.key));
        budget.output.text(std::get<1>(row.key));
        budget.output.text(std::get<2>(row.key));
    }
    return selection;
}

void base_input(Budget &budget, const ModifierBaseEffect &base) {
    // The upstream action owner supplies these fields. Preserve their bytes
    // and ordinal as provided, subject to the plan's field size limits.
    budget.input.add(9);
    budget.refs(
        {base.action_ref, base.action_version_ref, base.rule_ref, base.reversal_behavior_ref},
        false);
    target_input(budget, view(base.target));
    exact_shape(base.exact_units);
}

struct EffectDraft {
    Exact units;
    std::optional<std::int64_t> rounded;
};

std::vector<EffectDraft> scale(Budget &budget, std::span<const ModifierBaseEffect> bases,
                               std::span<const TargetDraft> targets) {
    Index<Target, const TargetDraft *> traces{MeteredLess{&budget}};
    for (const auto &target : targets)
        traces.try_emplace(target.target, &target);
    std::vector<EffectDraft> effects;
    effects.reserve(bases.size());
    budget.output.add(8);
    for (const auto &base : bases) {
        budget.work();
        const auto &trace = *traces.at(view(base.target));
        budget.output.add(trace.trace_bytes);
        auto exact = multiply(budget, base.exact_units, trace.reduction.factor);
        std::optional<std::int64_t> rounded;
        if (base.target.kind != ModifierTargetKind::counter) {
            budget.work(6); // #171 sign, quotient, remainder, tie and range checks.
            rounded = round_effect_units(exact);
        }
        budget.output.add(11 + (rounded ? 8 : 0));
        for (const auto *ref : {&base.action_ref, &base.action_version_ref, &base.rule_ref,
                                &base.target.ref, &base.reversal_behavior_ref})
            budget.output.text(*ref);
        exact_bytes(budget, budget.output, base.exact_units);
        exact_bytes(budget, budget.output, exact);
        effects.push_back({std::move(exact), rounded});
    }
    return effects;
}

ModifierStackProposal own_proposal(const CapturedModifierContext &context,
                                   std::span<const SelectionDraft> selection,
                                   std::span<const TargetDraft> traces,
                                   std::span<const ModifierBaseEffect> bases,
                                   std::vector<EffectDraft> effects) {
    ModifierStackProposal proposal{
        context.document_ref, context.profile, {}, {}, own_traces(traces, context.policy_ref), {}};
    proposal.selection.reserve(selection.size());
    proposal.links.reserve(selection.size());
    proposal.effects.reserve(bases.size());
    for (const auto &row : selection) {
        const auto [definition, instance, target, group, group_op, target_op, factor] = row.fields;
        proposal.selection.push_back({std::string(definition), std::string(instance),
                                      std::string(target), std::string(group),
                                      std::string(group_op), std::string(target_op),
                                      std::string(factor)});
        proposal.links.push_back({proposal.selection.size(), own_key(row.key)});
    }
    for (std::size_t i = 0; i < bases.size(); ++i)
        proposal.effects.push_back({bases[i], std::move(effects[i].units), effects[i].rounded});
    return proposal;
}
} // namespace

ModifierStackEvaluation evaluate_captured_modifiers(const ModifierStackCapture &capture,
                                                    std::span<const ModifierBaseEffect> bases,
                                                    const ModifierStackLimits &limits) {
    try {
        Budget budget(limits);
        fit(bases.size(), budget.requests);
        fit(bases.size(), budget.rows);
        context_input(budget, capture);
        if (capture.policy)
            policy_input(budget, *capture.policy);
        if (capture.eligible)
            revisions_input(budget, *capture.eligible);
        recorded_input(budget, capture);
        budget.input.add(16);
        for (const auto match : capture.matched_qualifiers) {
            budget.work();
            budget.refs({match});
        }
        for (const auto &base : bases) {
            budget.work();
            base_input(budget, base);
        }
        validate_context(budget, capture);
        validate_revisions(budget, *capture.eligible);
        for (const auto &base : bases)
            exact_input(budget, base.exact_units);

        const auto &context = *capture.context;
        budget.output.text(context.document_ref);
        budget.output.text(context.profile.producer_ref);
        budget.output.text(context.profile.profile_ref);
        const auto operands = project(budget, *capture.eligible, capture.matched_qualifiers,
                                      budget.rows - bases.size());
        std::vector<Target> requested;
        requested.reserve(bases.size());
        for (const auto &base : bases)
            requested.push_back(view(base.target));
        auto traces = assign(budget, operands, *capture.policy, requested);
        const auto selection = selection_drafts(budget, traces, capture, operands.size());
        reduce_traces(budget, traces, capture.policy->policy_ref);
        auto effects = scale(budget, bases, traces);
        return own_proposal(context, selection, traces, bases, std::move(effects));
    } catch (Failure failure) {
        return failure;
    } catch (const std::length_error &) {
        return Failure::resource_limit;
    } catch (const std::invalid_argument &) {
        return Failure::invalid;
    } catch (const std::overflow_error &) {
        return Failure::numeric_overflow;
    }
}
} // namespace subathon::server
