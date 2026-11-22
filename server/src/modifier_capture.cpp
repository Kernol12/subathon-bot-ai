// SPDX-License-Identifier: GPL-3.0-or-later
#include "modifier_capture_codec.hpp"
#include "modifier_stacking_internal.hpp"
#include "sqlite_application_bundle_access.hpp"

namespace subathon::server {
namespace {
using namespace core;
namespace stack = core::modifier_detail;
namespace codec = modifier_capture_detail;
using Refusal = ModifierCaptureRefusal;

void require_capture(bool condition, Refusal refusal = Refusal::invalid) {
    if (!condition)
        throw refusal;
}

Refusal stack_refusal(ModifierStackFailure failure) {
    switch (failure) {
    case ModifierStackFailure::unresolved:
        return Refusal::unresolved;
    case ModifierStackFailure::resource_limit:
        return Refusal::resource_limit;
    case ModifierStackFailure::invalid:
    case ModifierStackFailure::numeric_overflow:
        return Refusal::invalid;
    }
    return Refusal::invalid;
}

Refusal selected_refusal(selected_record_read::Failure failure) {
    switch (failure) {
    case selected_record_read::Failure::missing:
        return Refusal::unresolved;
    case selected_record_read::Failure::invalid_data:
        return Refusal::invalid;
    case selected_record_read::Failure::resource_limit:
        return Refusal::resource_limit;
    }
    return Refusal::invalid;
}

void consume(ModifierStackLimits &limits, const stack::Budget &budget) {
    limits.input_bytes -= budget.input.used();
    limits.output_bytes -= budget.output.used();
    limits.operations = budget.work_left;
}

void trigger_instance_input(const ModifierInstance &instance, stack::Budget &budget) {
    budget.refs({instance.instance_ref, instance.revision_ref, instance.source.source_ref,
                 instance.captured_definition.definition_ref,
                 instance.captured_definition.revision_ref});
    if (instance.parent_revision_ref)
        budget.refs({*instance.parent_revision_ref});
    if (instance.trigger)
        budget.refs({instance.trigger->event_ref});
    for (const auto &target : instance.captured_definition.targets) {
        budget.work();
        stack::target_input(budget, {target.kind, target.target_ref});
        if (target.qualifier_ref)
            budget.refs({*target.qualifier_ref});
        stack::factor_input(budget, target.factor_decimal);
    }
}

void validate_cut(const QualifiedModifierCut &cut, const IssuedModifierEvent &event,
                  ModifierStackLimits &limits) {
    stack::Budget budget(limits);
    budget.refs({cut.observation.timer_lineage, cut.observation.ref});
    if (cut.trigger) {
        const auto &trigger = *cut.trigger;
        const auto &activation = trigger.activation;
        budget.refs({trigger.trigger.event_ref, trigger.observation.timer_lineage,
                     trigger.observation.ref});
        using Timing = ModifierTriggerTiming;
        const auto timing = trigger.trigger.timing;
        require_capture(trigger.trigger.event_ref == event.event_ref &&
                        trigger.observation == cut.observation && trigger.turn == cut.turn &&
                        (timing == Timing::include_trigger || timing == Timing::subsequent_only));
        const auto &appended = activation.revision_to_append;
        const auto &selected = activation.for_trigger;
        if (appended) {
            trigger_instance_input(*appended, budget);
            require_capture(appended->trigger == trigger.trigger && appended->remaining_ms > 0);
        }
        if (activation.superseded_instance_ref)
            budget.refs({*activation.superseded_instance_ref});
        if (selected) {
            trigger_instance_input(*selected, budget);
            require_capture(selected->remaining_ms > 0);
        }
        if (timing == Timing::include_trigger) {
            require_capture(selected.has_value());
            if (appended)
                require_capture(*selected == *appended);
        } else if (appended) {
            if (appended->parent_revision_ref)
                require_capture(selected && selected->instance_ref == appended->instance_ref &&
                                selected->revision_ref == *appended->parent_revision_ref);
            else if (activation.superseded_instance_ref)
                require_capture(selected &&
                                selected->instance_ref == *activation.superseded_instance_ref);
            else
                require_capture(!selected);
        }

        stack::fit(cut.eligible.size(), budget.occurrences);
        bool found = false;
        for (const auto &entry : cut.eligible) {
            budget.work();
            budget.refs(
                {entry.key.instance_ref, entry.key.revision_ref, entry.definition_revision_ref});
            if (selected && entry.key.instance_ref == selected->instance_ref) {
                require_capture(!found && entry.key.revision_ref == selected->revision_ref &&
                                entry.definition_revision_ref ==
                                    selected->captured_definition.revision_ref &&
                                entry.remaining_ms == selected->remaining_ms);
                found = true;
            } else {
                require_capture((!appended || entry.key.instance_ref != appended->instance_ref) &&
                                (!activation.superseded_instance_ref ||
                                 entry.key.instance_ref != *activation.superseded_instance_ref));
            }
        }
        require_capture(found == selected.has_value());
    }
    consume(limits, budget);
}

void event_input(const IssuedModifierEvent &event, stack::Budget &budget) {
    budget.refs({event.event_ref, event.root_ref});
    if (event.parent_ref)
        budget.refs({*event.parent_ref});
}

void receipt_input(ReceiptId id, const ReceiptIdentity &identity, const ReceiptArrival &arrival,
                   const ReceiptFacts &facts, stack::Budget &budget) {
    require_capture(id.value != 0);
    budget.refs({identity.origin.provider, identity.origin.source_account,
                 identity.identifier_namespace, identity.opaque_id, arrival.configuration_ref,
                 arrival.modifier_context_ref});
    budget.refs({arrival.connection_ref}, false);
    budget.input.add(32); // Fixed receipt and arrival fields, including optional markers.
    budget.value(facts.event_type);
    for (const auto *value : {&facts.actor_id, &facts.actor_display_name})
        if (*value)
            budget.value(**value);
    if (facts.money) {
        budget.value(facts.money->original_decimal);
        budget.value(facts.money->currency);
    }
    for (const auto *link : {&facts.payment_reference, &facts.activity_reference})
        if (*link)
            budget.refs({(*link)->identifier_namespace, (*link)->opaque_id});
}

void decision_input(const std::string &plan_ref, const PlanOverrideDecision &decision,
                    stack::Budget &budget) {
    budget.refs({plan_ref, decision.decision_ref, decision.actor_ref});
    budget.value(decision.reason);
    require_capture(
        !decision.reason.empty() &&
        QByteArrayView(decision.reason.data(), qsizetype(decision.reason.size())).isValidUtf8());
}

void require_anchor(Access &reader, ReceiptId receipt, const std::optional<ContributionId> &cid) {
    if (!cid)
        return;
    const auto contribution = reader.contribution_by_id(*cid);
    require_capture(contribution.has_value(), Refusal::unresolved);
    require_capture(contribution->original_receipt_id == receipt);
}

RetainedKey arrival_key(const std::string &ref) { return {RetainedKind::arrival_context, ref}; }

void revision_definition_output(stack::Budget &budget, const ModifierDefinition &definition) {
    budget.output.text(definition.definition_ref);
    budget.output.text(definition.revision_ref);
    budget.output.add(17);
    for (const auto &target : definition.targets) {
        budget.work();
        budget.output.add(2);
        budget.output.text(target.target_ref);
        if (target.qualifier_ref)
            budget.output.text(*target.qualifier_ref);
        budget.output.text(target.factor_decimal);
    }
}

void revision_output(stack::Budget &budget, const ModifierInstance &instance,
                     const ModifierDefinition &definition) {
    budget.work();
    budget.output.add(11);
    for (const auto *ref :
         {&instance.instance_ref, &instance.revision_ref, &instance.source.source_ref})
        budget.output.text(*ref);
    if (instance.parent_revision_ref)
        budget.output.text(*instance.parent_revision_ref);
    if (instance.trigger)
        budget.output.text(instance.trigger->event_ref);
    revision_definition_output(budget, instance.captured_definition);
    revision_definition_output(budget, definition);
}

// Only exact revision reads qualify. Lifetime is a property of today's lineage;
// the captured remaining and eligibility belong to the recorded observation.
std::vector<SelectedModifierCapture::Revision>
read_history(const SqliteWorkSession &session, std::vector<ModifierArrivalInstance> observations,
             const ModifierExactInstanceRead &resolver, ModifierStackLimits &limits) {
    stack::Budget budget(limits);
    stack::fit(observations.size(), budget.rows);
    std::vector<SelectedModifierCapture::Revision> revisions;
    revisions.reserve(observations.size());
    const SqliteModifierActivationStore store(session);
    bool missing = false;
    for (auto &observed : observations) {
        budget.work(2);
        auto instance = resolver ? resolver(session, observed.key.revision_ref)
                                 : store.instance_revision(observed.key.revision_ref);
        if (instance) {
            const auto &exact = instance->instance;
            require_capture(exact.instance_ref == observed.key.instance_ref &&
                            exact.revision_ref == observed.key.revision_ref &&
                            exact.captured_definition.revision_ref ==
                                observed.definition_revision_ref &&
                            observed.remaining_ms <= exact.remaining_ms);
        }
        auto definition = store.definition_revision(observed.definition_revision_ref);
        if (definition)
            require_capture(definition->definition.revision_ref ==
                            observed.definition_revision_ref);
        if (!instance || !definition) {
            missing = true;
            continue;
        }
        revision_output(budget, instance->instance, definition->definition);
        revisions.push_back({std::move(observed), std::move(instance->instance),
                             std::move(definition->definition)});
    }
    require_capture(!missing, Refusal::unresolved);
    consume(limits, budget);
    return revisions;
}

ModifierStackProposal
calculate(const PlanEvent &event, const CapturedModifierContext &context,
          const ModifierStackPolicy &policy, const std::vector<std::string> &qualifiers,
          const std::vector<SelectedModifierCapture::Revision> &revisions,
          const ModifierTraceProfile &profile, const std::optional<ModifierRecalculation> &mode,
          std::span<const ModifierBaseEffect> bases, const ModifierStackLimits &limits) {
    std::vector<SelectedModifierRevision> eligible;
    eligible.reserve(revisions.size());
    for (const auto &r : revisions)
        eligible.push_back({r.observed.key.instance_ref, r.observed.key.revision_ref,
                            r.observed.definition_revision_ref, &r.exact_instance,
                            &r.exact_definition});
    const std::vector<std::string_view> matches(qualifiers.begin(), qualifiers.end());
    auto result = evaluate_captured_modifiers({event.arrival.modifier_context_ref,
                                               event.arrival,
                                               mode,
                                               &context,
                                               &profile,
                                               &policy,
                                               std::span(eligible),
                                               matches,
                                               {},
                                               {}},
                                              bases, limits);
    if (const auto *failure = std::get_if<ModifierStackFailure>(&result))
        throw stack_refusal(*failure);
    return std::get<ModifierStackProposal>(std::move(result));
}

void selection_input(std::span<const ModifierSelection> rows, stack::Budget &budget) {
    stack::fit(rows.size(), budget.occurrences);
    for (const auto &row : rows) {
        budget.work();
        budget.refs({row.definition_version_ref, row.instance_revision_ref, row.target_ref,
                     row.group_ref, row.group_operator, row.target_operator});
        stack::factor_input(budget, row.original_factor_decimal);
    }
}

RequestedUnit request_unit(ModifierTargetKind kind) {
    switch (kind) {
    case ModifierTargetKind::added_time:
        return RequestedUnit::milliseconds;
    case ModifierTargetKind::milestone_score:
        return RequestedUnit::micro_points;
    case ModifierTargetKind::counter:
        return RequestedUnit::whole_count;
    }
    throw Refusal::invalid;
}

void request_input(const CapturedPlanRevision &next, std::span<const ModifierBaseEffect> bases,
                   stack::Budget &budget) {
    const auto &requests = next.captured.requested_effects;
    stack::fit(requests.size(), budget.requests);
    stack::fit(next.rules.size(), std::min(budget.rows, std::size_t{1024}));
    require_capture(requests.size() == bases.size());
    for (const auto &rule : next.rules) {
        budget.work();
        budget.refs({rule.rule_ref, rule.version_ref}, false);
        budget.value(rule.explanation);
    }
    const auto &valuation = next.captured.valuation;
    require_capture((valuation.state == PlanValuationState::resolved) ==
                    valuation.historical.has_value());
    if (valuation.historical) {
        const auto &quote = *valuation.historical;
        budget.refs({quote.resolution_ref, quote.snapshot_ref});
        budget.value(quote.original_money.original_decimal);
        budget.value(quote.original_money.currency);
        budget.value(quote.comparison_currency);
        budget.value(quote.effective_rate_decimal);
        if (quote.fetched_rate_decimal)
            budget.value(*quote.fetched_rate_decimal);
        for (const auto *ref : {&quote.source_ref, &quote.manual_decision_ref})
            if (*ref)
                budget.refs({**ref});
        require_capture(quote.original_money == next.captured.event.facts.money);
    }
    const auto &conditions = next.captured.contribution_state.conditions;
    if (conditions.valuation.original_money) {
        budget.value(conditions.valuation.original_money->original_decimal);
        budget.value(conditions.valuation.original_money->currency);
    }
    if (conditions.valuation.resolution_ref)
        budget.refs({*conditions.valuation.resolution_ref});
    budget.refs({conditions.evidence_admission.policy_ref}, false);
    if (const auto &review = conditions.evidence_admission.review_decision)
        budget.refs({review->decision_ref, review->actor_ref});
    for (const auto &request : requests) {
        budget.work();
        budget.refs({request.action_ref, request.action_version_ref, request.rule_ref,
                     request.target_ref, request.reversal_behavior_ref},
                    false);
        budget.value(request.calculation_explanation);
        stack::exact_shape(request.exact_requested_units);
        stack::exact_input(budget, request.exact_requested_units);
        if (request.requires_historical_valuation)
            require_capture(valuation.state == PlanValuationState::resolved && valuation.historical,
                            Refusal::unresolved);
    }
}

void match_requests(std::span<const RequestedEffect> requests, const ModifierStackProposal &p) {
    require_capture(requests.size() == p.effects.size());
    for (std::size_t i = 0; i < requests.size(); ++i) {
        const auto &request = requests[i];
        const auto &effect = p.effects[i];
        const auto &base = effect.base;
        require_capture(
            request.action_ref == base.action_ref &&
            request.action_version_ref == base.action_version_ref &&
            request.rule_ref == base.rule_ref && request.target_ref == base.target.ref &&
            request.unit == request_unit(base.target.kind) &&
            request.requires_historical_valuation == base.requires_historical_valuation &&
            request.reversal_behavior_ref == base.reversal_behavior_ref &&
            request.exact_requested_units == effect.exact_units &&
            (!effect.rounded_units || request.requested_units == *effect.rounded_units));
    }
}

// Cleanup is idempotent if the port already rolled back. Preserve the original
// exception unless rollback itself leaves the work outcome uncertain.
void rollback_after_exception(const SqliteWorkSession &session) {
    if (const auto error = session.cleanup(); error != WorkError::none)
        throw SqliteWorkFailure(error);
}
} // namespace

ModifierArrivalPreparation prepare_modifier_arrival(const ModifierArrivalInput &input,
                                                    const ModifierStackLimits &limits) {
    try {
        auto remaining = limits;
        if (input.cut) {
            require_capture(input.cut->observation.utc_ms ==
                                input.arrival.server_timestamp_utc_ms &&
                            input.cut->policy.configuration_ref == input.arrival.configuration_ref);
            validate_cut(*input.cut, input.event, remaining);
        }
        return PreparedModifierArrival{codec::encode(input.identity, input.arrival, input.event,
                                                     input.cut ? &*input.cut : nullptr, nullptr,
                                                     nullptr, nullptr, remaining)};
    } catch (Refusal refusal) {
        return refusal;
    } catch (ModifierStackFailure failure) {
        return stack_refusal(failure);
    }
}

RetainDisposition retain_modifier_arrival(const SqliteWorkSession &session,
                                          const PreparedModifierArrival &value) {
    const ModifierStackLimits owner_limits{262144, 262144, 65536};
    if (inspect_modifier_record_shape(value.retained, false, owner_limits))
        throw RetentionFailure(RetentionError::invalid_document);
    return SqliteRetainedContextStore(session).retain(value.retained);
}

LocalModifierRecordShape inspect_modifier_record_shape(const RetainedDocument &record,
                                                       bool expect_recalculation,
                                                       const ModifierStackLimits &limits) {
    try {
        auto remaining = limits;
        (void)codec::decode(record, expect_recalculation, remaining);
        return {};
    } catch (Refusal refusal) {
        return refusal;
    }
}

ModifierCaptureSelection
select_modifier_capture(const SqliteWorkSession &session, const ContributionReceipt &original,
                        const IssuedModifierEvent &issued, std::optional<ContributionId> cid,
                        const std::optional<ModifierDecisionMode> &mode,
                        const ModifierTraceProfile &profile, const ModifierStackLimits &limits,
                        const ModifierExactInstanceRead &resolver,
                        std::optional<std::span<const ModifierSelection>> recorded) {
    try {
        auto remaining = limits;
        stack::Budget input(remaining);
        receipt_input(original.id, original.identity, original.arrival, original.facts, input);
        event_input(issued, input);
        require_capture(!profile.producer_ref.empty() && !profile.profile_ref.empty(),
                        Refusal::unresolved);
        input.refs({profile.producer_ref, profile.profile_ref});
        if (mode) {
            require_capture(cid && mode->revision > 1 &&
                            mode->decision.authority == OverrideAuthority::authorized &&
                            mode->decision.decision_ref != original.arrival.modifier_context_ref);
            decision_input(mode->plan_ref, mode->decision, input);
        }
        if (recorded)
            selection_input(*recorded, input);
        input.output.add(input.input.used());
        consume(remaining, input);

        const auto &key =
            mode ? mode->decision.decision_ref : original.arrival.modifier_context_ref;
        const auto retained = SqliteRetainedContextStore(session).find(arrival_key(key));
        require_capture(retained.has_value(), Refusal::unresolved);
        auto decoded = codec::decode(*retained, mode.has_value(), remaining);
        const auto &arrival = decoded.arrival;
        require_capture(
            decoded.identity == original.identity && decoded.event.event_ref == issued.event_ref &&
            decoded.event.root_ref == issued.root_ref &&
            decoded.event.parent_ref == issued.parent_ref &&
            arrival.modifier_context_ref == original.arrival.modifier_context_ref &&
            arrival.connection_ref == original.arrival.connection_ref &&
            arrival.server_timestamp_utc_ms == original.arrival.server_timestamp_utc_ms &&
            (mode || arrival.configuration_ref == original.arrival.configuration_ref));

        // Use the existing strict readers for the immutable receipt and original
        // contribution anchor, without adopting or finalizing the caller's work.
        Access reader(session, {}, Access::ReadPolicy::selected);
        reader.immutable_reads_ = true;
        const auto actual = reader.original(original.id);
        require_capture(actual.has_value(), Refusal::unresolved);
        require_capture(*actual == original);
        require_anchor(reader, original.id, cid);

        PlanEvent event{std::move(decoded.event.event_ref),
                        std::move(decoded.event.parent_ref),
                        std::move(decoded.event.root_ref),
                        original.id,
                        original.identity,
                        original.facts,
                        original.arrival,
                        cid};
        std::optional<ModifierRecalculation> recalculation;
        std::optional<ModifierRecalculationBinding> binding;
        std::optional<SqlitePlanRevisionSnapshot> persisted;
        if (mode) {
            const auto &audit = *decoded.decision;
            require_capture(audit.receipt == original.id && audit.contribution == *cid &&
                            audit.mode.plan_ref == mode->plan_ref &&
                            audit.mode.revision == mode->revision &&
                            audit.mode.decision == mode->decision);
            persisted =
                SqliteApplicationPlanStore(session).find_revision(mode->plan_ref, mode->revision);
            require_capture(persisted.has_value(), Refusal::unresolved);
            const auto &revision = persisted->revision;
            require_capture(revision.captured.event == event &&
                            revision.override_decision == mode->decision);
            const auto rows = std::span<const ModifierSelection>(revision.modifiers);
            if (recorded)
                require_capture(std::ranges::equal(*recorded, rows));
            else {
                stack::Budget row_input(remaining);
                selection_input(rows, row_input);
                consume(remaining, row_input);
            }
            recorded = rows;
            recalculation = ModifierRecalculation{mode->plan_ref, mode->revision, mode->decision};
            binding = ModifierRecalculationBinding{mode->plan_ref, mode->revision,
                                                   original.arrival.modifier_context_ref, key};
        }

        require_capture(decoded.cut.has_value(), Refusal::unresolved);
        auto &cut = *decoded.cut;
        require_capture(cut.profile == profile, Refusal::unresolved);
        CapturedModifierContext context{key, cut.policy.configuration_ref, cut.policy.policy_ref,
                                        std::move(cut.profile), std::move(binding)};
        auto revisions = read_history(session, std::move(cut.eligible), resolver, remaining);
        auto proposal = calculate(event, context, cut.policy, cut.matched_qualifiers, revisions,
                                  profile, recalculation, {}, remaining);

        // The calculator emits links from complete policy order after qualifier
        // filtering. Equal serialized rows can therefore retain different ordinals.
        std::optional<std::vector<ModifierSelection>> rows;
        std::vector<ModifierTraceLink> links;
        if (recorded) {
            require_capture(std::ranges::equal(*recorded, proposal.selection));
            rows = std::move(proposal.selection);
            links = std::move(proposal.links);
        }
        return SelectedModifierCapture{std::move(event),      std::move(context),
                                       std::move(cut.policy), std::move(cut.matched_qualifiers),
                                       std::move(revisions),  std::move(rows),
                                       std::move(links)};
    } catch (Refusal refusal) {
        return refusal;
    } catch (ModifierStackFailure failure) {
        return stack_refusal(failure);
    } catch (selected_record_read::Failure failure) {
        return selected_refusal(failure);
    }
}

ModifierRecalculationResult retain_modifier_recalculation(
    const SqliteWorkSession &session, const ApplicationPlanKey &expected,
    const PlanOverrideDecision &authority, const std::optional<QualifiedModifierCut> &cut,
    std::span<const ModifierBaseEffect> bases, const CapturedPlanRevision &next,
    const ModifierStackLimits &limits, const ModifierPlanAppend &append) {
    RetainedDocument document;
    try {
        require_capture(authority.authority == OverrideAuthority::authorized, Refusal::denied);
        require_capture(cut.has_value(), Refusal::unresolved);
        auto remaining = limits;
        stack::Budget input(remaining);
        const auto &event = next.captured.event;
        decision_input(expected.plan_ref, authority, input);
        require_capture(expected.revision > 0 &&
                        expected.revision < std::numeric_limits<std::uint64_t>::max() &&
                        next.revision == expected.revision + 1 &&
                        next.parent_revision == expected.revision &&
                        next.override_decision == authority && event.contribution_id);
        receipt_input(event.receipt_id, event.identity, event.arrival, event.facts, input);
        input.refs({event.event_ref, event.root_event_ref});
        if (event.parent_event_ref)
            input.refs({*event.parent_event_ref});
        selection_input(next.modifiers, input);
        request_input(next, bases, input);
        consume(remaining, input);

        Access reader(session, {}, Access::ReadPolicy::selected);
        reader.immutable_reads_ = true;
        const auto head = reader.header(expected.plan_ref);
        require_capture(head && head->head == expected.revision, Refusal::stale_head);
        require_capture(head->event == event);
        require_anchor(reader, event.receipt_id, event.contribution_id);

        const IssuedModifierEvent issued{event.event_ref, event.root_event_ref,
                                         event.parent_event_ref};
        validate_cut(*cut, issued, remaining);
        const ApplicationPlanKey successor{expected.plan_ref, next.revision};
        document = codec::encode(event.identity, event.arrival, issued, &*cut, &event, &successor,
                                 &authority, remaining);
        const auto &policy = cut->policy;
        const auto &key = authority.decision_ref;
        const CapturedModifierContext context{
            key, policy.configuration_ref, policy.policy_ref, cut->profile,
            ModifierRecalculationBinding{expected.plan_ref, next.revision,
                                         event.arrival.modifier_context_ref, key}};
        auto revisions = read_history(session, cut->eligible, {}, remaining);
        const auto proposal = calculate(
            event, context, policy, cut->matched_qualifiers, revisions, cut->profile,
            ModifierRecalculation{expected.plan_ref, next.revision, authority}, bases, remaining);
        require_capture(proposal.selection == next.modifiers);
        match_requests(next.captured.requested_effects, proposal);
        require_capture(!SqliteRetainedContextStore(session).find(document.key), Refusal::conflict);
    } catch (Refusal refusal) {
        return refusal;
    } catch (ModifierStackFailure failure) {
        return stack_refusal(failure);
    } catch (selected_record_read::Failure failure) {
        return selected_refusal(failure);
    }

    const auto retained = SqliteRetainedContextStore(session).retain(document);
    if (retained == RetainDisposition::missing_reference)
        return Refusal::unresolved;
    if (retained != RetainDisposition::inserted)
        return Refusal::conflict;

    SqlitePlanAppendDisposition disposition;
    try {
        disposition = append ? append(session, expected, next)
                             : SqliteApplicationPlanStore(session).append(expected.plan_ref,
                                                                          expected.revision, next);
        if (append && disposition == SqlitePlanAppendDisposition::appended) {
            const auto written =
                SqliteApplicationPlanStore(session).find_revision(expected.plan_ref, next.revision);
            if (!written || written->revision != next)
                disposition = SqlitePlanAppendDisposition::revision_conflict;
        }
    } catch (const SqlitePlanInputFailure &) {
        session.rollback();
        return Refusal::append_refused;
    } catch (...) {
        rollback_after_exception(session);
        throw;
    }
    if (disposition != SqlitePlanAppendDisposition::appended) {
        session.rollback();
        return Refusal::append_refused;
    }
    return {};
}

ModifierBindingProjection project_modifier_binding(const std::string &configuration_ref,
                                                   const ModifierDefinition &definition,
                                                   std::size_t ordinal,
                                                   const ModifierClockValue &clock,
                                                   const std::optional<ModifierBindingName> &name) {
    if (ordinal == 0 || ordinal > definition.targets.size() ||
        clock.revision.captured_definition != definition ||
        (clock.remaining_ms && *clock.remaining_ms < 0))
        throw std::invalid_argument("Invalid exact modifier display binding");

    const auto &spelling = definition.targets[ordinal - 1].factor_decimal;
    ModifierBindingProjection projected{
        {}, parse_nonnegative_factor(spelling).value(), spelling, clock.remaining_ms, {}};
    if (clock.remaining_ms)
        projected.active = *clock.remaining_ms > 0;
    if (name && name->configuration_ref == configuration_ref &&
        name->definition_ref == definition.definition_ref &&
        name->revision_ref == definition.revision_ref)
        projected.name = name->name;
    return projected;
}

} // namespace subathon::server
