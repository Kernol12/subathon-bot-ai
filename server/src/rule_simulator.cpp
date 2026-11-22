// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/rule_simulator.hpp>

#include "rule_simulator_budget.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <string_view>

namespace subathon::server::rule_simulator {
namespace {
using namespace core;
using namespace std::chrono_literals;

template <class Range, class Predicate>
const typename Range::value_type *find(const Range &range, Predicate predicate) {
    const auto found = std::ranges::find_if(range, predicate);
    return found == range.end() ? nullptr : std::addressof(*found);
}

struct Refusal {
    Completion completion;
};

bool uncertain(RuleCompletion completion) {
    return completion == RuleCompletion::outcome_unknown ||
           completion == RuleCompletion::abnormal_completion;
}

std::optional<std::int64_t> age(std::int64_t observed, std::int64_t published) {
    if (observed < published)
        return {};
    const auto difference =
        static_cast<std::uint64_t>(observed) - static_cast<std::uint64_t>(published);
    if (difference > std::numeric_limits<std::int64_t>::max())
        return {};
    return static_cast<std::int64_t>(difference);
}

std::optional<QuoteTrace> inspect_quote(const EventCapture &capture) {
    if (!capture.valuation.historical)
        return {};
    const auto &quote = *capture.valuation.historical;
    auto rate = parse_positive_rate(quote.effective_rate_decimal).value();
    if (const auto *exact = find(capture.explicit_valuations, [&](const auto &row) {
            return row.resolution_ref == quote.resolution_ref &&
                   row.event_ref == capture.binding.event &&
                   row.original.value == quote.original_money &&
                   row.comparison_currency == quote.comparison_currency;
        })) {
        // begin_event validates this retained canonical rational before inspection.
        rate = Exact{boost::multiprecision::cpp_int{exact->numerator}} /
               boost::multiprecision::cpp_int{exact->denominator};
    }
    QuoteTrace trace{
        quote, rate, make_rate_money(quote.original_money).data().value * rate, {}, {}};
    if (!capture.quote_observation_utc_ms || !capture.rates || !quote.source_ref)
        return trace;
    const auto &snapshot = capture.rates->data();
    if (snapshot.snapshot_ref != quote.snapshot_ref)
        return trace;
    const auto *source = find(snapshot.sources, [&](const auto &row) {
        return row.data().context.observation_ref == *quote.source_ref;
    });
    if (source && source->data().context.publication_time.utc_ms) {
        trace.age_ms =
            age(*capture.quote_observation_utc_ms, *source->data().context.publication_time.utc_ms);
        if (trace.age_ms && capture.quote_age_threshold_ms)
            trace.stale = *trace.age_ms > *capture.quote_age_threshold_ms;
    }
    return trace;
}

PermissionRational rational(const Exact &value) {
    return {numerator(value).str(), denominator(value).str()};
}

struct DeltaLane {
    RequestedUnit unit;
    ModifierTargetKind modifier;
    const char *permission_unit;
};

DeltaLane lane(RuleDeltaUnit unit) {
    switch (unit) {
    case RuleDeltaUnit::milliseconds:
        return {RequestedUnit::milliseconds, ModifierTargetKind::added_time, "milliseconds"};
    case RuleDeltaUnit::micro_points:
        return {RequestedUnit::micro_points, ModifierTargetKind::milestone_score, "micro_points"};
    case RuleDeltaUnit::whole_count:
        return {RequestedUnit::whole_count, ModifierTargetKind::counter, "whole_count"};
    }
    throw Refusal{Completion::invalid_input};
}

// The scenario models an ordinary author grant over this exact bound tuple.
RuleActionAuthorityFacts authority(std::string_view author, const PermissionRequest &request,
                                   const std::string &revision) {
    std::vector<std::string> resource_slots;
    std::vector<PermissionResourceConstraint> resources;
    for (const auto &resource : request.resources) {
        resource_slots.push_back(resource.slot);
        resources.push_back(
            {resource.slot, PermissionResourceMode::allowed_set, {resource.resource}});
    }
    std::vector<PermissionQuantityDefinition> slots;
    std::vector<PermissionQuantityConstraint> quantities;
    if (!request.quantities.empty()) {
        const auto &quantity = request.quantities.front();
        slots.push_back({quantity.slot, quantity.unit, PermissionNumberKind::rational, {}, {}, 15});
        quantities.push_back({quantity.slot, quantity.unit, PermissionQuantityMode::closed_interval,
                              quantity.value, quantity.value});
    }
    const PermissionPolicyEntry allow{PermissionOrigin::individual,
                                      std::string(author),
                                      {},
                                      request.operation,
                                      PermissionEffect::allow,
                                      "simulated-grant",
                                      std::move(resources),
                                      std::move(quantities)};
    return {revision,
            {PermissionFactsState::complete,
             "simulated-catalog",
             {{request.operation,
               PermissionOperationClass::ordinary,
               std::move(resource_slots),
               std::move(slots),
               {}}}},
            {PermissionOwnerState::present, "simulated-owner"},
            {PermissionFactsState::complete, {}, {allow}},
            {PermissionFactsState::complete, {}, {allow}}};
}

struct PlanRules final : RulePlanSource {
    std::vector<RuleSelection> rows;
    std::vector<RuleSelection> capture(const PlanEvent &) override { return std::move(rows); }
};
struct PlanModifiers final : ModifierPlanSource {
    std::vector<ModifierSelection> rows;
    std::vector<ModifierSelection> capture(const PlanEvent &) override { return std::move(rows); }
};

const char *disposition_name(RuleSelectionDisposition value) {
    switch (value) {
    case RuleSelectionDisposition::selected:
        return "selected";
    case RuleSelectionDisposition::disabled:
        return "disabled";
    case RuleSelectionDisposition::condition_no_match:
        return "condition_no_match";
    case RuleSelectionDisposition::condition_unknown:
        return "condition_unknown";
    case RuleSelectionDisposition::excluded:
        return "excluded";
    case RuleSelectionDisposition::exclusive_group:
        return "exclusive_group";
    }
    throw std::logic_error("Unexpected selection disposition");
}

ModifierStackEvaluation evaluate(const EventCapture &event, const SelectedModifiers &selected,
                                 std::span<const ModifierBaseEffect> bases,
                                 const ModifierStackLimits &limits) {
    std::vector<SelectedModifierRevision> revisions;
    if (selected.eligible) {
        revisions.reserve(selected.eligible->size());
        for (const auto &instance : *selected.eligible)
            revisions.push_back({instance.instance_ref, instance.revision_ref,
                                 instance.captured_definition.revision_ref, &instance,
                                 &instance.captured_definition});
    }
    std::vector<std::string_view> qualifiers(selected.matched_qualifiers.begin(),
                                             selected.matched_qualifiers.end());
    return evaluate_captured_modifiers(
        {event.binding.event,
         std::cref(event.receipt.arrival),
         {},
         &selected.context,
         &selected.admitted_profile,
         &selected.policy,
         selected.eligible ? std::optional{std::span<const SelectedModifierRevision>{revisions}}
                           : std::nullopt,
         qualifiers,
         {},
         {}},
        bases, limits);
}

bool valid_queue(const RuleDeliveryObservation &queue) {
    return !queue.job_ref.empty() &&
           queue.job_ref.size() <= EffectivePermissionLimits{}.reference_bytes &&
           queue.state == RuleDeliveryState::queued && queue.revision == 1 && !queue.attempt &&
           !queue.evidence_ref;
}

struct ReportPort final : ConnectorAdapter {
    explicit ReportPort(const ConnectorReport &report) : selected(report) {}
    const ConnectorReport &selected;
    ConnectorReport report() const override { return selected; }
};

struct StagedEffects {
    ScenarioState state;
    bool changed{};
    std::vector<CommittedMilestoneGroup> groups;
    std::optional<MilestoneProgressionData> progression;
};

struct PendingEffects {
    chain::UnitIdentity identity;
    StagedEffects proposal;
    std::string basis;
};

struct ChainState {
    chain::OpenedChain opened;
    std::vector<chain::AdmittedEvent> events;
    std::size_t pending;
    std::size_t bytes;
    std::optional<chain::UnitIdentity> blocked;
    std::optional<PendingEffects> effects;
};

struct OccurrenceUnit {
    ChainState &context;
    chain::Unit unit;
    const MilestoneCapture &capture;
};

struct CreditedTime {
    chain::ChainKey chain;
    std::string event;
    RuleActionKey action;
    std::string effect;
    RuleTimeObservation actual;
    std::optional<RefundAmounts> last_refund;
    std::optional<RefundTimeCredit> prior;
};

struct ReleaseInterrupted {};
struct ReleaseAttempt {
    std::string turn;
    MilestoneOccurrenceKey occurrence;
};

class Run final : public MilestoneGroupRelease {
    struct TimelineStep {
        Run &owner;
        const OwnerTurn &turn;
        TurnTrace &trace;
        std::optional<OccurrenceUnit> unit;
        bool skipped{};
    };

  public:
    Run(const ScenarioInputs &input, const Limits &limits, Budget budget, SimulationResult &out)
        : input_(input), limits_(limits), budget_(budget), out_(out), state_(input.initial),
          timeline_(std::make_unique<MilestoneTimeline>(state_.timeline, *this)),
          countdown_paused_(!state_.pause.reasons.empty()) {
        timeline_->set_countdown_paused(countdown_paused_, 0ms);
    }

    void run() {
        for (const auto &turn : input_.turns) {
            TurnTrace trace{};
            trace.ref = turn.ref;
            std::visit([&](const auto &value) { process(turn, value, trace); }, turn.input);
            trace.state = state_;
            trace.presentation = timeline_->presentation();
            trace.countdown_paused = countdown_paused_;
            trace.queue_paused = queue_paused_;
            budget_.entry(trace);
            // A resource stop publishes none of this turn's private mutations.
            // Every port and all scratch state die with this Run.
            out_.trace.push_back(std::move(trace));
            out_.final_state = state_;
        }
        if (out_.completion == Completion::complete &&
            (release_attempt_ ||
             std::ranges::any_of(chains_, [](const auto &c) { return c.blocked.has_value(); })))
            out_.completion = Completion::unit_hold;
    }

  private:
    void classify(Completion result) {
        if (out_.completion == Completion::complete)
            out_.completion = result;
    }

    const EventCapture *capture(const chain::EventBinding &binding) const {
        return find(input_.captures, [&](const auto &value) { return value.binding == binding; });
    }

    ChainState *chain_state(const chain::ChainKey &key) {
        const auto row = std::ranges::find_if(
            chains_, [&](const auto &entry) { return entry.events.front().binding.chain == key; });
        return row == chains_.end() ? nullptr : &*row;
    }

    chain::OwnerView owner() {
        return {[this](const chain::EventBinding &binding) -> std::optional<chain::CaptureView> {
                    ++out_.owner_callbacks;
                    const auto *event = capture(binding);
                    if (!event)
                        return {};
                    return chain::CaptureView{
                        std::cref(event->configuration),
                        {binding.event, event->receipt.arrival.configuration_ref, event->trigger,
                         event->facts, event->rates ? &*event->rates : nullptr,
                         event->rates ? std::optional{event->rates->data().snapshot_ref}
                                      : std::nullopt},
                        event->authors,
                        event->explicit_valuations};
                },
                [this](const chain::ChainKey &key) {
                    ++out_.owner_callbacks;
                    chain::CapacityUse use{};
                    for (const auto &context : chains_) {
                        if (context.events.front().binding.chain == key)
                            continue;
                        ++use.active_roots;
                        use.pending_events += static_cast<std::uint32_t>(std::min(
                            context.pending, static_cast<std::size_t>(limits_.chain.pending_events -
                                                                      use.pending_events)));
                        // The modeled owner charges its retained evidence for other roots.
                        use.owned_bytes +=
                            std::min(context.bytes, limits_.chain.owned_bytes - use.owned_bytes);
                    }
                    return use;
                }};
    }

    void describe(const ChainState &context, TurnTrace &trace) {
        trace.committed_children = context.opened.committed_children;
        trace.retained_cutoff = context.opened.cutoff;
    }

    ChainState *open(const RootSelection &selection, TurnTrace &trace) {
        const auto &binding = selection.root.event;
        const auto *history = std::get_if<chain::ModeledHistory>(&selection.selection);
        auto opened =
            chain::open_chain(selection.root, limits_.chain, selection.selection, owner());
        if (const auto *refusal = std::get_if<chain::OpenRefusal>(&opened)) {
            trace.open_refusal = *refusal;
            switch (*refusal) {
            case chain::OpenRefusal::history_unavailable:
                classify(Completion::unavailable);
                break;
            case chain::OpenRefusal::invalid_profile:
                classify(Completion::invalid_input);
                break;
            case chain::OpenRefusal::conflict:
                classify(Completion::conflict);
                break;
            case chain::OpenRefusal::needs_reactivation:
            case chain::OpenRefusal::capacity_hold:
                throw BudgetExceeded{};
            }
            return nullptr;
        }

        Budget retained{limits_.owned_bytes, limits_.rows};
        retained.take(sizeof(ChainState));
        retained.entry(selection);
        if (const auto *root = capture(binding))
            retained.entry(*root);
        std::size_t pending = 1;
        if (history) {
            pending = history->events.size();
            for (const auto &publication : history->publications) {
                if (publication.pass)
                    --pending;
            }
            for (const auto &event : std::span{history->events}.subspan(1)) {
                if (const auto *selected = capture(event.binding))
                    retained.entry(*selected);
            }
        }
        budget_.charge(limits_.owned_bytes - retained.bytes, limits_.rows - retained.rows);
        std::vector<chain::AdmittedEvent> events;
        if (history)
            events = history->events;
        else
            events.push_back({binding, 0, 0});
        chains_.push_back({std::get<chain::OpenedChain>(std::move(opened)),
                           std::move(events),
                           pending,
                           limits_.owned_bytes - retained.bytes,
                           {},
                           {}});
        auto *existing = &chains_.back();
        if (history && history->blocked)
            existing->blocked = history->blocked->identity;
        describe(*existing, trace);
        return existing;
    }

    ChainState *select_chain(const chain::ChainKey &key, TurnTrace &trace) {
        if (auto *context = chain_state(key)) {
            describe(*context, trace);
            return context;
        }
        if (restart_selection_ && restart_selection_->root.event.chain == key) {
            if (std::holds_alternative<chain::VerifiedNew>(restart_selection_->selection)) {
                trace.open_refusal = chain::OpenRefusal::history_unavailable;
                classify(Completion::unavailable);
                return nullptr;
            }
            return open(*restart_selection_, trace);
        }
        if (const auto *selection =
                find(input_.roots, [&](const auto &root) { return root.root.event.chain == key; }))
            return open(*selection, trace);
        trace.open_refusal = chain::OpenRefusal::history_unavailable;
        classify(Completion::unavailable);
        return nullptr;
    }

    void report_begin(chain::BeginDisposition result, TurnTrace &trace) {
        trace.begin_disposition = result;
        switch (result) {
        case chain::BeginDisposition::conflict:
            classify(Completion::conflict);
            break;
        case chain::BeginDisposition::not_admitted:
        case chain::BeginDisposition::history_unavailable:
            classify(Completion::unavailable);
            break;
        case chain::BeginDisposition::capacity_hold:
            throw BudgetExceeded{};
        case chain::BeginDisposition::duplicate_event:
        case chain::BeginDisposition::in_flight_hold:
            break;
        }
    }

    void publish(ChainState &context, chain::Publication publication, TurnTrace &trace) {
        Budget retained{limits_.owned_bytes - context.bytes, limits_.rows};
        retained.entry(publication);
        for (const auto &child : publication.children) {
            retained.entry(child);
            if (const auto *selected = capture(child.binding))
                retained.entry(*selected);
        }
        budget_.charge(limits_.owned_bytes - context.bytes - retained.bytes,
                       limits_.rows - retained.rows);
        context.bytes = limits_.owned_bytes - retained.bytes;
        if (publication.pass)
            --context.pending;
        context.pending += publication.children.size();
        context.opened.committed_children +=
            static_cast<std::uint32_t>(publication.children.size());
        context.events.insert(context.events.end(), publication.children.begin(),
                              publication.children.end());
        if (publication.cutoff) {
            context.opened.cutoff = publication.cutoff;
            const auto &key = publication.identity.chain;
            // Length-prefix the components so distinct chain refs cannot alias.
            const auto identity =
                std::to_string(key.installation.size()) + ":" + key.installation + key.root;
            trace.cutoff_audit_ref = "simulated-cutoff-audit:" + identity;
            trace.cutoff_notice_ref = "simulated-cutoff-notice:" + identity;
        }
        describe(context, trace);
        trace.publication.emplace(std::move(publication));
    }

    void commit_effects(const OwnerTurn &turn, const chain::Publication &publication,
                        StagedEffects &&staged, TurnTrace &trace) {
        if (staged.changed) {
            staged.state.state_revision = turn.ref;
            state_ = std::move(staged.state);
        }
        if (!staged.groups.empty()) {
            pending_groups_.insert(pending_groups_.end(), staged.groups.begin(),
                                   staged.groups.end());
            timeline_->accept_committed(std::move(staged.groups), turn.sample.elapsed);
        }
        trace.progression = std::move(staged.progression);
        for (const auto &rule : publication.pass->rules) {
            for (const auto &accepted : rule.accepted) {
                auto it = std::ranges::find(trace.actions, accepted.key,
                                            [](const ActionTrace &row) { return row.intent.key; });
                if (it != trace.actions.end())
                    it->accepted = accepted.observed;
                if (const auto *job = std::get_if<RuleDeliveryObservation>(&accepted.observed)) {
                    deliveries_.push_back({*job, *job});
                    trace.delivery = deliveries_.back();
                }
                if (const auto *effect = std::get_if<RuleEffectObservation>(&accepted.observed)) {
                    const auto *time = std::get_if<RuleTimeObservation>(&effect->change);
                    if (time && time->requested_ms >= 0)
                        credits_.push_back({publication.identity.chain,
                                            publication.identity.anchor,
                                            accepted.key,
                                            effect->effect_ref,
                                            *time,
                                            {},
                                            {}});
                }
            }
        }
    }

    void process(const OwnerTurn &, const EventTurn &, TurnTrace &);
    void process(const OwnerTurn &, const TimeCut &, TurnTrace &);
    void process(const OwnerTurn &, const PauseControl &, TurnTrace &);
    void process(const OwnerTurn &, const RefundTurn &, TurnTrace &);
    void process(const OwnerTurn &, const TimelineTurn &, TurnTrace &);
    void process(const OwnerTurn &, const GroupReadback &, TurnTrace &);
    void process(const OwnerTurn &, const ConnectorReport &, TurnTrace &);
    void process(const OwnerTurn &, const DeliveryTurn &, TurnTrace &);
    void process(const OwnerTurn &, const UnitReconciliation &, TurnTrace &);
    void process(const OwnerTurn &, const RestartTurn &, TurnTrace &);

    RuleActionBinding bind(const EventTurn &, const RuleActionKey &, const RuleActionPayload &);
    void resolve_requests(const EventCapture &, const EventTurn &, TurnTrace &);
    void append_progression(const chain::EventBinding &, StagedEffects &);
    RuleOwnerResult propose(const OwnerTurn &, const EventTurn &, const EventCapture &,
                            const RuleActionKey &, const RuleBoundAction &, ActionTrace &,
                            StagedEffects &, std::vector<std::string> &);
    bool apply_timer_cut(const OwnerTurn &, const TimeCut &, TurnTrace &);
    void advance_timer_to(const OwnerTurn &, TurnTrace &);
    void project_cut(const OwnerTurn &, const TimerState &, const TimerClockStep &,
                     const TimerAdvance &);
    bool state_held() const {
        return std::ranges::any_of(chains_, [](const auto &context) {
            return context.effects && context.effects->proposal.changed;
        });
    }
    bool reserved_job(std::string_view ref) const;

    std::optional<MilestoneHeadDisposition>
    occurrence_disposition(const MilestoneOccurrenceKey &) const override;
    bool review_held(const MilestoneCompletion &) const override;
    void release(CommittedMilestoneGroup group) override;
    const MilestoneCapture *milestone(const MilestoneOccurrenceKey &) const;
    std::optional<OccurrenceUnit> begin_occurrence(const std::string &unit,
                                                   const MilestoneOccurrenceKey &, TurnTrace &);
    void project_occurrence(const std::string &unit, OccurrenceUnit &, chain::OccurrenceState,
                            TurnTrace &);

    const ScenarioInputs &input_;
    const Limits &limits_;
    Budget budget_;
    SimulationResult &out_;
    ScenarioState state_;
    std::vector<ChainState> chains_;
    const RootSelection *restart_selection_{};
    std::vector<CreditedTime> credits_;
    std::vector<RuleDeliveryTrace> deliveries_;
    std::vector<CommittedMilestoneGroup> pending_groups_;
    // Independent status and timeline observations do not consume timer elapsed.
    const OwnerTurn *timer_turn_{};
    ClockSample timer_sample_{};
    TimelineStep *timeline_step_{};
    std::optional<ReleaseAttempt> release_attempt_;
    std::unique_ptr<MilestoneTimeline> timeline_;
    bool countdown_paused_{};
    bool queue_paused_{};
};

RuleActionBinding Run::bind(const EventTurn &event, const RuleActionKey &key,
                            const RuleActionPayload &payload) {
    if (const auto *delta = std::get_if<RuleDeltaAction>(&payload)) {
        // Resolved quantities are installed after selection and modifier evaluation.
        return RuleBoundAction{{"delta",
                                {{"target", delta->target_ref}},
                                {{"amount", lane(delta->unit).permission_unit, {}}}},
                               payload};
    }
    if (const auto *chat = std::get_if<RuleChatAction>(&payload)) {
        if (!event.chat || event.chat->action != key || event.chat->sender.empty() ||
            chat->sender == RuleChatSender::configured_default)
            return RuleActionRefusal::unresolved_input;
        return RuleBoundAction{
            {"chat", {{"route", chat->route_ref}, {"sender", event.chat->sender}}, {}}, payload};
    }
    if (const auto *named = std::get_if<RuleNamedEvent>(&payload))
        return RuleBoundAction{{"emit", {{"name", named->event_name}}, {}}, payload};
    return RuleActionRefusal::capability_unavailable;
}

void Run::resolve_requests(const EventCapture &event, const EventTurn &input, TurnTrace &trace) {
    auto &selection = *trace.selection;
    const auto &rules = event.configuration.data().rules;
    PlanRules rule_source;
    PlanModifiers modifier_source;
    PlanCapture plan{event.receipt, event.eligibility, event.valuation, {}};
    if (!input.prepared) {
        for (std::size_t i = 0; i < rules.size(); ++i) {
            const auto disposition = selection.selection.decisions[i].disposition;
            rule_source.rows.push_back({rules[i].rule_ref, rules[i].version_ref,
                                        disposition == RuleSelectionDisposition::selected,
                                        disposition_name(disposition)});
        }
    }

    std::vector<ModifierBaseEffect> bases;
    std::size_t ordinal = 0;
    for (const auto &rule : selection.selected) {
        for (const auto &intent : rule.intents) {
            ++ordinal;
            const auto *delta = std::get_if<RuleDeltaAction>(&intent.captured_payload);
            const auto *fixed = delta ? std::get_if<RuleFixedDelta>(&delta->delta) : nullptr;
            if (!input.prepared && fixed)
                bases.push_back({ordinal,
                                 intent.key.action_ref,
                                 intent.key.action_version,
                                 intent.key.rule_ref,
                                 {lane(delta->unit).modifier, delta->target_ref},
                                 false,
                                 parse_decimal(fixed->original_decimal).value(),
                                 "reverse-actual"});
        }
    }

    const ModifierStackProposal *scaled = nullptr;
    if (!input.prepared && event.modifiers) {
        auto limits = limits_.modifiers;
        limits.output_bytes = std::min(limits.output_bytes, budget_.bytes);
        limits.materialized_rows = std::min(limits.materialized_rows, budget_.rows);
        trace.modifiers = evaluate(event, *event.modifiers, bases, limits);
        if (const auto *failure = std::get_if<ModifierStackFailure>(&*trace.modifiers)) {
            if (*failure == ModifierStackFailure::resource_limit)
                throw BudgetExceeded{};
            if (*failure == ModifierStackFailure::numeric_overflow)
                trace.pass_failure = RulePassFailure::numeric_hold;
        } else {
            scaled = &std::get<ModifierStackProposal>(*trace.modifiers);
            modifier_source.rows = scaled->selection;
        }
    }

    const auto *prepared =
        input.prepared ? &trace.plan->inspected.captured.requested_effects : nullptr;
    ordinal = 0;
    auto base = bases.begin();
    auto effect =
        scaled ? scaled->effects.begin() : std::vector<ModifierScaledEffect>::const_iterator{};
    for (auto &rule : selection.selected) {
        for (auto &intent : rule.intents) {
            ++ordinal;
            const auto *delta = std::get_if<RuleDeltaAction>(&intent.captured_payload);
            if (!delta)
                continue;
            auto *bound = std::get_if<RuleBoundAction>(&intent.binding);
            const auto target = lane(delta->unit);
            std::optional<RequestedEffect> request;
            if (prepared) {
                const auto matches = [&](const auto &row) {
                    return row.action_ref == intent.key.action_ref &&
                           row.action_version_ref == intent.key.action_version &&
                           row.rule_ref == intent.key.rule_ref && row.unit == target.unit &&
                           row.target_ref == delta->target_ref;
                };
                if (std::ranges::count_if(*prepared, matches) > 1)
                    throw Refusal{Completion::conflict};
                if (const auto *selected = find(*prepared, matches))
                    request = *selected;
            } else if (base != bases.end() && base->action_ordinal == ordinal) {
                const auto &basis = *base++;
                if (!event.modifiers || scaled) {
                    const auto &exact = scaled ? effect->exact_units : basis.exact_units;
                    std::int64_t units{};
                    try {
                        if (target.unit == RequestedUnit::whole_count && denominator(exact) != 1) {
                            intent.binding = RuleActionRefusal::unresolved_input;
                            if (scaled)
                                ++effect;
                            continue;
                        }
                        units = scaled && effect->rounded_units ? *effect->rounded_units
                                                                : round_effect_units(exact);
                    } catch (const std::overflow_error &) {
                        trace.pass_failure = RulePassFailure::numeric_hold;
                    }
                    if (!trace.pass_failure)
                        request = RequestedEffect{basis.action_ref,
                                                  basis.action_version_ref,
                                                  basis.rule_ref,
                                                  basis.target.ref,
                                                  target.unit,
                                                  false,
                                                  exact,
                                                  units,
                                                  "captured fixed request",
                                                  basis.reversal_behavior_ref};
                    if (scaled)
                        ++effect;
                }
            }
            if (!bound || !request) {
                intent.binding = RuleActionRefusal::unresolved_input;
                continue;
            }
            bound->request.quantities.front().value = rational(Exact{request->requested_units});
            if (!prepared)
                plan.requested_effects.push_back(std::move(*request));
        }
    }
    if (!prepared) {
        const auto selected =
            capture_application_plan(event.binding.capture, plan, rule_source, modifier_source);
        trace.plan = PlanTrace{{}, selected.revisions.front(), 1, plan.requested_effects};
    }
}

bool Run::reserved_job(std::string_view ref) const {
    if (find(deliveries_, [&](const auto &job) { return job.queued.job_ref == ref; }))
        return true;
    // A held event may have queued a job. Its identity stays reserved until
    // complete committed readback or known rollback.
    for (const auto &turn : input_.turns) {
        if (const auto *event = std::get_if<EventTurn>(&turn.input);
            event && event->chat && event->chat->queue_acceptance &&
            event->chat->queue_acceptance->job_ref == ref &&
            std::ranges::any_of(chains_, [&](const auto &context) {
                return context.effects && context.effects->identity.unit == turn.ref;
            }))
            return true;
    }
    return false;
}

void Run::append_progression(const chain::EventBinding &event, StagedEffects &staged) {
    auto &track = *staged.state.milestone_track;
    track.score_micro_points = staged.state.score_micro_points;
    auto limits = limits_.milestones;
    limits.owned_bytes = std::min(limits.owned_bytes, budget_.bytes);
    limits.items = std::min(limits.items, budget_.rows);
    auto proposal = advance_milestone_progression(track, limits);
    const auto &result = proposal.data();
    for (auto i = result.newly_completed_begin; i < result.completed_prefix.size(); ++i) {
        const auto &achievement = result.completed_prefix[i];
        const auto *selected = find(input_.milestone_captures, [&](const auto &row) {
            return row.released_event.chain == event.chain &&
                   row.released_event.parent == event.event && row.occurrence.ordinal == i;
        });
        if (selected) {
            const auto *retained = capture(selected->released_event);
            if (!retained)
                throw Refusal{Completion::unavailable};
            const auto *fact = find(retained->facts, [&](const auto &row) {
                const auto *text = std::get_if<RuleText>(&row);
                return text && text->field == RuleTextField::milestone_ref &&
                       text->value == achievement.milestone_ref;
            });
            if (!fact)
                throw Refusal{Completion::conflict};
            staged.groups.push_back({achievement,
                                     selected->actions,
                                     selected->timing,
                                     MilestoneOccurrenceKey{selected->occurrence.occurrence},
                                     {}});
        } else {
            // Keep the achievement and the ordered head. A missing selection
            // cannot authorize a release or fabricate a child binding.
            staged.groups.push_back({achievement, {}, {}, {}, {}});
        }
    }
    if (staged.progression) {
        const auto begin = staged.progression->newly_completed_begin;
        staged.progression = result;
        staged.progression->newly_completed_begin = begin;
        staged.progression->newly_completed_count = result.completed_prefix.size() - begin;
    } else {
        staged.progression = result;
    }
    track.completed_prefix = result.completed_prefix;
}

RuleOwnerResult Run::propose(const OwnerTurn &turn, const EventTurn &event,
                             const EventCapture &captured, const RuleActionKey &key,
                             const RuleBoundAction &bound, ActionTrace &trace,
                             StagedEffects &staged, std::vector<std::string> &jobs) {
    if (const auto *chat = std::get_if<RuleChatAction>(&bound.payload)) {
        trace.proposed = RuleProjectedDelivery{chat->route_ref, event.chat->sender};
        const auto &selected = event.chat->queue_acceptance;
        if (!selected || !valid_queue(*selected) || reserved_job(selected->job_ref) ||
            std::ranges::find(jobs, selected->job_ref) != jobs.end())
            return RuleOwnerRefusal{"queue_acceptance_unavailable"};
        jobs.push_back(selected->job_ref);
        return RuleOwnerAcceptance{*selected};
    }

    const auto &delta = std::get<RuleDeltaAction>(bound.payload);
    if (state_held() || state_.pause.reasons.contains(PauseReason::RecoveryReview))
        return RuleOwnerRefusal{"simulated_effect_owner_held"};
    const auto &eligibility =
        event.prepared
            ? event.prepared->selected_history.revisions.back().captured.contribution_state
            : captured.eligibility;
    if (!assess_contribution_eligibility(eligibility).blockers.empty())
        return RuleOwnerRefusal{"contribution_ineligible"};

    const auto ref = find(event.effect_refs, [&](const auto &row) { return row.first == key; });
    const auto effect_ref = ref ? ref->second : key.action_ref;
    const auto requested = parse_decimal(bound.request.quantities.front().value.numerator).value();
    const auto units = round_effect_units(requested);
    auto &state = staged.state;

    if (delta.unit == RuleDeltaUnit::milliseconds) {
        if (state.timer.lifecycle != TimerLifecycle::Active ||
            (state.timer_config.hard_end_utc_ms &&
             turn.sample.utc_ms >= *state.timer_config.hard_end_utc_ms))
            return RuleOwnerRefusal{"timer_not_eligible"};
        if (units < 0)
            return RuleOwnerRefusal{"signed_timer_control_unavailable"};
        const auto addition =
            plan_time_addition({state.timer.main_remaining_ms, state.time_debt_ms},
                               {effect_ref, units}, state.main_cap_ms);
        auto timer = apply_applicable_time_credit(state.timer, addition.main_added_ms);
        const RuleTimeObservation observed{units, addition.main_added_ms, -addition.debt_repaid_ms,
                                           addition.cap_limited_ms, timer.main_remaining_ms};
        state.timer = timer;
        state.time_debt_ms = addition.balance_after.time_debt_ms;
        staged.changed = true;
        trace.proposed = observed;
        return RuleOwnerAcceptance{RuleEffectObservation{effect_ref, observed}};
    }

    if (delta.unit == RuleDeltaUnit::whole_count)
        return RuleOwnerRefusal{"count_owner_unavailable"};
    state.score_micro_points = round_effect_units(Exact{state.score_micro_points} + units);
    if (state.milestone_track)
        append_progression(event.event, staged);
    staged.changed = true;
    const RuleQuantityObservation observed{"micro_points", rational(requested),
                                           rational(requested)};
    trace.proposed = observed;
    return RuleOwnerAcceptance{RuleEffectObservation{effect_ref, observed}};
}

void Run::process(const OwnerTurn &turn, const EventTurn &event, TurnTrace &trace) {
    auto *context = select_chain(event.event.chain, trace);
    if (!context)
        return;
    auto begun = chain::begin_event(context->opened.chain, event.event, turn.ref);
    if (const auto *refused = std::get_if<chain::BeginDisposition>(&begun)) {
        report_begin(*refused, trace);
        return;
    }
    auto &unit = std::get<chain::Unit>(begun);
    advance_timer_to(turn, trace);
    const auto *selected = capture(event.event); // A successful begin required this exact binding.
    trace.admitted =
        *find(context->events, [&](const auto &row) { return row.binding == event.event; });
    trace.quote = inspect_quote(*selected);

    bool stale = false;
    if (event.prepared) {
        const auto &prepared = *event.prepared;
        const auto &history = prepared.selected_history;
        const auto validity = check_plan_preview(history, prepared.preview, state_.state_revision);
        const auto *inspected = find(history.revisions, [&](const auto &row) {
            return row.revision == prepared.preview.plan_revision;
        });
        if (!inspected)
            throw Refusal{Completion::unavailable};
        if (inspected->captured.event != selected->receipt)
            throw Refusal{Completion::conflict};
        trace.plan = PlanTrace{validity, *inspected, history.revisions.back().revision,
                               history.revisions.back().captured.requested_effects};
        stale = validity != PlanPreviewValidity::current;
    }

    trace.configuration.emplace(selected->configuration);
    trace.selection =
        chain::plan_event(unit, [&](const auto &key, const auto &payload, const auto &) {
            return bind(event, key, payload);
        });
    resolve_requests(*selected, event, trace);
    for (const auto &rule : trace.selection->selected) {
        for (const auto &intent : rule.intents) {
            ActionTrace action{};
            action.intent = intent;
            if (const auto *chat = std::get_if<RuleChatAction>(&intent.captured_payload);
                chat && event.chat && event.chat->action == intent.key)
                action.message = MessageIntent{chat->route_ref, event.chat->sender,
                                               chat->template_version_ref, event.chat->text};
            trace.actions.push_back(std::move(action));
        }
    }
    if (stale || trace.pass_failure) {
        static_cast<void>(chain::finish_unit(std::move(unit), {}, RuleCompletion::rolled_back));
        return;
    }

    StagedEffects staged{state_, false, {}, {}};
    std::vector<std::string> jobs;
    const std::string snapshot = state_.state_revision;
    ActionTrace *checking{};
    const RuleActionChecks checks{
        [&](std::string_view author, const RuleActionKey &key, const RuleBoundAction &bound) {
            ++out_.owner_callbacks;
            checking = &*std::ranges::find(trace.actions, key,
                                           [](const auto &row) { return row.intent.key; });
            return authority(author, bound.request, snapshot);
        },
        [&](const RuleBoundAction &, std::string_view checked) {
            ++out_.owner_callbacks;
            // #208 has established both author checks before reaching execution.
            checking->gates = {{Gate::use, RuleGate::permit}, {Gate::grant, RuleGate::permit}};
            const auto gate = checked != snapshot ? RuleGate::unavailable
                              : checking->message ? event.chat->execution
                                                  : RuleGate::permit;
            checking->gates.push_back({Gate::execution, gate});
            return gate;
        },
        [&](const RuleBoundAction &bound) {
            ++out_.owner_callbacks;
            const auto *delta = std::get_if<RuleDeltaAction>(&bound.payload);
            const bool valid =
                !delta ||
                ((delta->unit != RuleDeltaUnit::milliseconds || delta->target_ref == "Main") &&
                 (delta->unit != RuleDeltaUnit::micro_points || !state_.milestone_track ||
                  delta->target_ref == state_.milestone_track->track_ref));
            checking->gates.push_back({Gate::target, valid ? RuleGate::permit : RuleGate::deny});
            return valid;
        },
        [&](const RuleBoundAction &) {
            ++out_.owner_callbacks;
            checking->gates.push_back({Gate::capability, RuleGate::permit});
            return RuleCapability::supported;
        }};

    const auto named_binding = [&](const RuleActionKey &key) {
        return find(event.named, [&](const auto &binding) {
            const auto *cause =
                binding.cause ? std::get_if<chain::NamedAction>(&*binding.cause) : nullptr;
            return cause && cause->key == key;
        });
    };
    const auto submit = chain::chain_action_submit(
        unit, [&](const RuleActionKey &key, const RuleNamedEvent &) { return *named_binding(key); },
        [&](const chain::AdmittedEvent &child, const RuleNamedEvent &,
            std::string_view checked) -> chain::NamedOutcome {
            if (checked != snapshot)
                return chain::OutcomeUnknown{};
            return RuleOwnerAcceptance{RuleEventLineage{
                child.binding.event, child.binding.parent, child.binding.chain.root,
                std::get<chain::NamedAction>(*child.binding.cause).key}};
        },
        [&](const RuleEventLineage &, const RuleActionKey &key, const RuleBoundAction &bound,
            std::string_view checked) -> RuleOwnerResult {
            if (checked != snapshot)
                return RuleOwnerRefusal{"snapshot_changed"};
            try {
                return propose(turn, event, *selected, key, bound, *checking, staged, jobs);
            } catch (const std::overflow_error &) {
                return RulePassFailure::numeric_hold;
            }
        });

    auto pass = consume_rule_actions(
        *trace.selection, checks,
        [&](const RuleEventLineage &lineage, const RuleActionKey &key, const RuleBoundAction &bound,
            std::string_view checked) -> RuleOwnerResult {
            ++out_.owner_callbacks;
            checking->submitted = true;
            if (const auto *named = std::get_if<RuleNamedEvent>(&bound.payload)) {
                checking->proposed = RuleProjectedChild{named->event_name};
                if (!named_binding(key))
                    return RuleOwnerRefusal{"named_capture_unavailable"};
            }
            return submit(lineage, key, bound, checked);
        });

    std::optional<RuleStagedPass> report;
    if (auto *complete = std::get_if<RuleStagedPass>(&pass)) {
        for (const auto &rule : complete->rules) {
            if (!rule.stop)
                continue;
            const auto *selected_rule = find(trace.selection->selected, [&](const auto &row) {
                return row.captured_index == rule.captured_index;
            });
            const auto &key = selected_rule->intents[rule.stop->position].key;
            auto &action = *std::ranges::find(trace.actions, key,
                                              [](const auto &row) { return row.intent.key; });
            action.stop = rule.stop;
        }
        report.emplace(std::move(*complete));
    } else {
        trace.pass_failure = std::get<RulePassFailure>(pass);
    }

    trace.completion = trace.pass_failure ? RuleCompletion::rolled_back : event.completion;
    // Bound the represented evidence before asking the private chain to acknowledge.
    auto budget = budget_;
    budget.entry(trace);
    if (report)
        budget.entry(*report);
    budget.entry(staged.state);
    budget.entry(staged.groups);
    auto outcome = chain::finish_unit(std::move(unit), report, *trace.completion);
    if (auto *publication = std::get_if<chain::Publication>(&outcome)) {
        commit_effects(turn, *publication, std::move(staged), trace);
        publish(*context, std::move(*publication), trace);
    } else if (std::holds_alternative<chain::Unresolved>(outcome) ||
               std::holds_alternative<chain::Conflict>(outcome)) {
        chain::UnitIdentity identity{turn.ref, event.event.chain, event.event.event,
                                     chain::UnitPurpose::event_pass};
        context->blocked = identity;
        context->effects = PendingEffects{std::move(identity), std::move(staged), snapshot};
        if (std::holds_alternative<chain::Conflict>(outcome))
            classify(Completion::conflict);
    } else if (*trace.completion == RuleCompletion::committed) {
        throw BudgetExceeded{};
    }
}

void Run::project_cut(const OwnerTurn &turn, const TimerState &before, const TimerClockStep &step,
                      const TimerAdvance &advance) {
    if (!state_.current_modifier)
        return;
    const auto &prior_ref = timer_turn_ ? timer_turn_->ref : input_.initial.state_revision;
    state_.current_modifier =
        project_modifier_cut({"simulated-timer", prior_ref, timer_sample_.utc_ms},
                             {"simulated-timer", turn.ref, step.utc_now_ms}, before, step, advance,
                             state_.current_modifier)
            .head;
}

bool Run::apply_timer_cut(const OwnerTurn &turn, const TimeCut &cut, TurnTrace &trace) {
    if (state_held()) {
        trace.clock_held = true;
        return false;
    }
    const auto elapsed = (turn.sample.elapsed - timer_sample_.elapsed).count();
    const auto before = state_.timer;
    auto running = state_.pause.reasons.empty() ? elapsed : 0;
    if (cut.signals) {
        if (cut.signals->elapsed_ms != elapsed)
            throw Refusal{Completion::conflict};

        auto observation = *cut.signals;
        const auto observe = [&](std::int64_t ticks) {
            observation.elapsed_ms = ticks;
            return observe_pause_signals(state_.pause, state_.pause_config, observation);
        };
        const bool initially_paused = !observe(0).reasons.empty();
        auto after = observe(elapsed);
        running = initially_paused ? 0 : elapsed;
        if (!initially_paused && !after.reasons.empty()) {
            // For a constant observation the first added pause starts at its
            // threshold, after the preceding countdown span already elapsed.
            std::int64_t first = 0;
            std::int64_t last = elapsed;
            while (last - first > 1) {
                const auto mid = first + (last - first) / 2;
                if (observe(mid).reasons.empty())
                    first = mid;
                else
                    last = mid;
            }
            running = last;
        }
        state_.pause = std::move(after);
    }

    const auto advance = [&](const TimerState &state, const TimerClockStep &step) {
        return state.lifecycle == TimerLifecycle::Ready
                   ? TimerAdvance{state, {}, 0}
                   : advance_timer(state, state_.timer_config, step);
    };
    const TimerClockStep step{running == 0 ? elapsed : running, turn.sample.utc_ms, running == 0};
    trace.timer_cut = advance(before, step);
    project_cut(turn, before, step, *trace.timer_cut);
    if (running > 0 && running < elapsed && state_.current_modifier) {
        // The explicit cut supplies the pause boundary's elapsed distance.
        // Keep the single observed UTC throughout the split clock projection.
        const TimerClockStep paused{elapsed - running, turn.sample.utc_ms, true};
        const auto rest = advance(trace.timer_cut->state, paused);
        state_.current_modifier =
            project_modifier_cut({"simulated-timer", turn.ref + ":progression", turn.sample.utc_ms},
                                 {"simulated-timer", turn.ref, turn.sample.utc_ms},
                                 trace.timer_cut->state, paused, rest, state_.current_modifier)
                .head;
        trace.timer_cut->state = rest.state;
        trace.timer_cut->transitions.insert(trace.timer_cut->transitions.end(),
                                            rest.transitions.begin(), rest.transitions.end());
    }
    state_.timer = trace.timer_cut->state;
    state_.state_revision = turn.ref;
    timer_sample_ = turn.sample;
    timer_turn_ = &turn;
    return true;
}

void Run::advance_timer_to(const OwnerTurn &turn, TurnTrace &trace) {
    if (!timer_turn_) {
        // Before a countdown cut is qualified, the initial state is the
        // baseline observed at the first applying timer owner turn.
        timer_sample_ = turn.sample;
        timer_turn_ = &turn;
        return;
    }
    if (turn.sample.elapsed > timer_sample_.elapsed)
        apply_timer_cut(turn, TimeCut{}, trace);
}

void Run::process(const OwnerTurn &turn, const TimeCut &cut, TurnTrace &trace) {
    if (!apply_timer_cut(turn, cut, trace))
        return;
    // An explicit timer cut observes the timer's pause state for the timeline.
    countdown_paused_ = !state_.pause.reasons.empty();
    timeline_->set_countdown_paused(countdown_paused_, turn.sample.elapsed);
}

void Run::process(const OwnerTurn &turn, const PauseControl &control, TurnTrace &trace) {
    if (state_held()) {
        trace.clock_held = true;
        return;
    }
    advance_timer_to(turn, trace);
    state_.pause = set_pause_reason(state_.pause, control.reason, control.active);
    state_.state_revision = turn.ref;
    countdown_paused_ = !state_.pause.reasons.empty();
    timeline_->set_countdown_paused(countdown_paused_, turn.sample.elapsed);
}

void Run::process(const OwnerTurn &turn, const RefundTurn &refund, TurnTrace &trace) {
    auto *context = select_chain(refund.event.chain, trace);
    if (!context)
        return;
    auto begun = chain::begin_event(context->opened.chain, refund.event, turn.ref);
    if (const auto *refused = std::get_if<chain::BeginDisposition>(&begun)) {
        report_begin(*refused, trace);
        return;
    }
    auto &unit = std::get<chain::Unit>(begun);
    const auto rollback = [&] {
        trace.completion = RuleCompletion::rolled_back;
        static_cast<void>(chain::finish_unit(std::move(unit), {}, RuleCompletion::rolled_back));
    };
    const auto &retained = *capture(refund.event);
    trace.refund_receipt = retained.receipt;
    const auto credited = std::ranges::find_if(credits_, [&](const auto &credit) {
        return credit.chain.installation == refund.event.chain.installation &&
               credit.event == refund.original_event && credit.action == refund.original_action;
    });
    if (credited == credits_.end()) {
        classify(Completion::unavailable);
        rollback();
        return;
    }
    const auto *original = find(input_.captures, [&](const auto &event) {
        return event.binding.chain == credited->chain && event.binding.event == credited->event;
    });
    if (!original || !original->receipt.facts.money || !retained.receipt.facts.money ||
        !original->receipt.facts.payment_reference) {
        classify(Completion::unavailable);
        rollback();
        return;
    }
    const auto &original_money = *original->receipt.facts.money;
    const auto &refund_money = *retained.receipt.facts.money;
    if (retained.trigger != RuleTriggerKind::refund ||
        original->receipt.identity.origin != retained.receipt.identity.origin ||
        original->receipt.facts.payment_reference != retained.receipt.facts.payment_reference ||
        original_money.currency != refund_money.currency ||
        make_rate_money(original_money).data().value != refund.amounts.original ||
        make_rate_money(refund_money).data().value != -refund.amounts.incoming ||
        (credited->last_refund &&
         refund.amounts.previously_accepted !=
             credited->last_refund->previously_accepted + credited->last_refund->incoming) ||
        (credited->prior &&
         (refund.previous.main_ms != credited->prior->main_ms ||
          refund.previous.debt_repayment_ms != credited->prior->debt_repayment_ms))) {
        classify(Completion::conflict);
        rollback();
        return;
    }
    if (state_held() || !assess_contribution_eligibility(retained.eligibility).blockers.empty() ||
        refund.selected_current_timer.lifecycle != TimerLifecycle::Active ||
        (state_.timer_config.hard_end_utc_ms &&
         turn.sample.utc_ms >= *state_.timer_config.hard_end_utc_ms)) {
        rollback();
        return;
    }

    advance_timer_to(turn, trace);
    try {
        trace.refund = calculate_refund_time_entitlement(
            refund.amounts, {credited->actual.main_change_ms, -credited->actual.debt_change_ms},
            refund.previous);
        auto timer = refund.selected_current_timer;
        auto reversal = plan_time_reversal(
            {timer.main_remaining_ms, state_.time_debt_ms},
            {refund.event.event, credited->effect, trace.refund->requested_main_adjustment_ms},
            refund.policy);
        if (reversal.disposition == TimeReversalDisposition::Applied) {
            reversal.balance_after.time_debt_ms =
                round_effect_units(Exact{reversal.balance_after.time_debt_ms} +
                                   trace.refund->requested_debt_restoration_ms);
        }

        // The refund owner has its own selected credit basis. Record its
        // event-pass identity through the ordinary chain acknowledgement.
        const auto plan = chain::plan_event(unit, {});
        const auto report = std::get<RuleStagedPass>(consume_rule_actions(plan, {}, {}));
        auto outcome = chain::finish_unit(std::move(unit), report, RuleCompletion::committed);
        if (auto *publication = std::get_if<chain::Publication>(&outcome)) {
            trace.completion = RuleCompletion::committed;
            trace.reversal = reversal;
            credited->last_refund = refund.amounts;
            credited->prior = trace.refund->cumulative;
            if (reversal.disposition == TimeReversalDisposition::Applied) {
                timer.main_remaining_ms = reversal.balance_after.main_remaining_ms;
                state_.timer = timer;
                state_.time_debt_ms = reversal.balance_after.time_debt_ms;
                state_.state_revision = turn.ref;
            }
            publish(*context, std::move(*publication), trace);
        } else if (std::holds_alternative<chain::RolledBack>(outcome)) {
            throw BudgetExceeded{};
        } else {
            classify(Completion::conflict);
        }
    } catch (const std::overflow_error &) {
        trace.pass_failure = RulePassFailure::numeric_hold;
        rollback();
    }
}

const MilestoneCapture *Run::milestone(const MilestoneOccurrenceKey &key) const {
    return find(input_.milestone_captures,
                [&](const auto &row) { return row.occurrence.occurrence == key.ref; });
}

bool Run::review_held(const MilestoneCompletion &) const {
    ++out_.owner_callbacks;
    // Legacy achievement rows have no selected legacy release evidence in this seam.
    return true;
}

std::optional<MilestoneHeadDisposition>
Run::occurrence_disposition(const MilestoneOccurrenceKey &key) const {
    ++out_.owner_callbacks;
    if (!timeline_step_)
        return {};
    auto &step = *timeline_step_;
    const auto &input = std::get<TimelineTurn>(step.turn.input);
    if (!input.head || input.head->head != key) {
        if (!step.trace.group_head)
            step.trace.group_head = GroupObservation{key, {}, RuleCompletion::outcome_unknown};
        return {};
    }
    step.trace.group_head = input.head;
    const auto disposition = input.head->disposition;
    if (disposition == MilestoneHeadDisposition::ready ||
        disposition == MilestoneHeadDisposition::skipped) {
        auto unit = step.owner.begin_occurrence(step.turn.ref, key, step.trace);
        if (!unit) {
            step.trace.group_head->disposition = MilestoneHeadDisposition::held;
            return MilestoneHeadDisposition::held;
        }
        step.unit.emplace(std::move(*unit));
        step.skipped = disposition == MilestoneHeadDisposition::skipped;
    }
    return disposition;
}

void Run::release(CommittedMilestoneGroup group) {
    ++out_.owner_callbacks;
    const auto &step = *timeline_step_;
    const auto completion = step.trace.group_head->release_completion;
    step.trace.releases.push_back({*group.occurrence, completion});
    if (completion != RuleCompletion::committed) {
        release_attempt_ = ReleaseAttempt{step.turn.ref, *group.occurrence};
        throw ReleaseInterrupted{};
    }
}

std::optional<OccurrenceUnit> Run::begin_occurrence(const std::string &unit_ref,
                                                    const MilestoneOccurrenceKey &key,
                                                    TurnTrace &trace) {
    const auto *group = milestone(key);
    if (!group) {
        classify(Completion::unavailable);
        return {};
    }
    const auto &child = group->released_event;
    auto *context = select_chain(child.chain, trace);
    if (!context)
        return {};
    auto begun = chain::begin_occurrences(context->opened.chain, *child.parent, unit_ref);
    if (const auto *refused = std::get_if<chain::BeginDisposition>(&begun)) {
        report_begin(*refused, trace);
        return {};
    }
    return OccurrenceUnit{*context, std::get<chain::Unit>(std::move(begun)), *group};
}

void Run::project_occurrence(const std::string &unit_ref, OccurrenceUnit &selected,
                             chain::OccurrenceState observed, TurnTrace &trace) {
    auto &[context, unit, group] = selected;
    const auto &child = group.released_event;
    const auto decision = chain::offer_occurrence_child(
        unit, group.occurrence,
        observed == chain::OccurrenceState::released ? std::optional{child} : std::nullopt,
        observed);
    if (std::holds_alternative<chain::Duplicate>(decision)) {
        static_cast<void>(chain::finish_unit(std::move(unit), {}, RuleCompletion::rolled_back));
        return;
    }
    if (std::holds_alternative<chain::CapacityHold>(decision)) {
        static_cast<void>(chain::finish_unit(std::move(unit), {}, RuleCompletion::rolled_back));
        throw BudgetExceeded{};
    }
    auto outcome = chain::finish_unit(std::move(unit), {}, RuleCompletion::committed);
    if (auto *publication = std::get_if<chain::Publication>(&outcome))
        publish(context, std::move(*publication), trace);
    else if (std::holds_alternative<chain::RolledBack>(outcome))
        throw BudgetExceeded{};
    else {
        context.blocked = chain::UnitIdentity{unit_ref, child.chain, *child.parent,
                                              chain::UnitPurpose::released_occurrences};
        if (std::holds_alternative<chain::Conflict>(outcome))
            classify(Completion::conflict);
    }
}

void Run::process(const OwnerTurn &turn, const TimelineTurn &input, TurnTrace &trace) {
    if (!pending_groups_.empty() && pending_groups_.front().occurrence) {
        if (const auto *group = milestone(*pending_groups_.front().occurrence)) {
            if (auto *context = chain_state(group->released_event.chain))
                describe(*context, trace);
        }
    }

    TimelineStep step{*this, turn, trace, {}, false};
    timeline_step_ = &step;
    try {
        switch (input.operation) {
        case TimelineOperation::sample:
            timeline_->advance(turn.sample.elapsed);
            break;
        case TimelineOperation::countdown_pause:
        case TimelineOperation::countdown_resume:
            // A timeline control supplies an explicit gate observation. The next
            // explicit timer cut, pause control, or recovery can update it.
            countdown_paused_ = input.operation == TimelineOperation::countdown_pause;
            timeline_->set_countdown_paused(countdown_paused_, turn.sample.elapsed);
            break;
        case TimelineOperation::queue_pause:
        case TimelineOperation::queue_resume:
            queue_paused_ = input.operation == TimelineOperation::queue_pause;
            timeline_->set_queue_paused(queue_paused_, turn.sample.elapsed);
            break;
        default:
            throw Refusal{Completion::invalid_input};
        }
    } catch (const ReleaseInterrupted &) {
        // The shipped timeline keeps the head and latches the attempted release.
    }
    timeline_step_ = nullptr;
    if (!step.unit)
        return;
    if (!step.skipped && (trace.releases.empty() ||
                          trace.releases.front().completion != RuleCompletion::committed)) {
        // The occurrence unit only prepared a projection. The separate group
        // release is the uncertain operation, and leaves this unit unexecuted.
        static_cast<void>(
            chain::finish_unit(std::move(step.unit->unit), {}, RuleCompletion::rolled_back));
        return;
    }
    pending_groups_.erase(pending_groups_.begin());
    project_occurrence(
        turn.ref, *step.unit,
        step.skipped ? chain::OccurrenceState::skipped : chain::OccurrenceState::released, trace);
}

bool same_group(const CommittedMilestoneGroup &a, const CommittedMilestoneGroup &b) {
    const auto fields = [](const MilestoneCompletion &completion) {
        return std::tie(completion.milestone_ref, completion.definition_revision_ref,
                        completion.target_micro_points, completion.score_micro_points,
                        completion.completion_context_ref);
    };
    return fields(a.achievement) == fields(b.achievement) && a.actions == b.actions &&
           a.timing.presentation == b.timing.presentation && a.timing.gap == b.timing.gap &&
           a.occurrence == b.occurrence && !a.edit_membership && !b.edit_membership;
}

void Run::process(const OwnerTurn &turn, const GroupReadback &readback, TurnTrace &trace) {
    if (!release_attempt_ || release_attempt_->turn != readback.attempt_turn ||
        release_attempt_->occurrence != readback.attempted) {
        classify(Completion::conflict);
        return;
    }
    if (uncertain(readback.completion))
        return;
    const bool released = readback.completion == RuleCompletion::committed;
    const auto suffix = std::span{pending_groups_}.subspan(released ? 1 : 0);
    if (!std::ranges::equal(readback.committed_pending_suffix, suffix, same_group)) {
        classify(Completion::conflict);
        return;
    }
    std::optional<OccurrenceUnit> confirmed;
    if (released) {
        auto unit = begin_occurrence(readback.attempt_turn, readback.attempted, trace);
        if (!unit)
            return;
        confirmed.emplace(std::move(*unit));
    }
    auto fresh = std::make_unique<MilestoneTimeline>(state_.timeline, *this);
    fresh->restore_committed(readback.committed_pending_suffix);
    fresh->set_countdown_paused(countdown_paused_, turn.sample.elapsed);
    fresh->set_queue_paused(queue_paused_, turn.sample.elapsed);
    timeline_ = std::move(fresh);
    if (released) {
        pending_groups_.erase(pending_groups_.begin());
        project_occurrence(readback.attempt_turn, *confirmed, chain::OccurrenceState::released,
                           trace);
    }
    release_attempt_.reset();
    trace.completion = readback.completion;
}

void Run::process(const OwnerTurn &, const ConnectorReport &report, TurnTrace &trace) {
    ++out_.owner_callbacks;
    trace.connector = read_connector_status(ReportPort{report}, {});
}

void Run::process(const OwnerTurn &, const DeliveryTurn &delivery, TurnTrace &trace) {
    const auto job = std::ranges::find_if(deliveries_, [&](const auto &entry) {
        return entry.queued.job_ref == delivery.selected_job;
    });
    if (job == deliveries_.end())
        return;
    ++out_.owner_callbacks;
    *job = observe_rule_delivery(*job, delivery.observation);
    trace.delivery = *job;
}

void Run::process(const OwnerTurn &turn, const UnitReconciliation &readback, TurnTrace &trace) {
    auto *context = select_chain(readback.chain, trace);
    if (!context)
        return;
    if (context->blocked != readback.unit || readback.chain != readback.unit.chain) {
        classify(Completion::conflict);
        return;
    }
    const bool commits = std::holds_alternative<chain::ModeledHistory>(readback.selected);
    if (commits && readback.unit.purpose == chain::UnitPurpose::event_pass &&
        (!context->effects || (context->effects->proposal.changed &&
                               context->effects->basis != state_.state_revision))) {
        // A modeled restart can retain a blocked identity without its private
        // effect proposal. Chain evidence alone cannot reconstruct that state.
        classify(Completion::unavailable);
        return;
    }

    ++out_.owner_callbacks;
    auto result =
        chain::reconcile_unit(context->opened.chain, readback.unit.unit, readback.selected);
    if (auto *publication = std::get_if<chain::Publication>(&result)) {
        trace.completion = RuleCompletion::committed;
        if (context->effects)
            commit_effects(turn, *publication, std::move(context->effects->proposal), trace);
        publish(*context, std::move(*publication), trace);
    } else if (std::holds_alternative<chain::RolledBack>(result)) {
        trace.completion = RuleCompletion::rolled_back;
    } else {
        if (std::holds_alternative<chain::Conflict>(result))
            classify(Completion::conflict);
        return;
    }
    context->blocked.reset();
    context->effects.reset();
}

void Run::process(const OwnerTurn &turn, const RestartTurn &restart, TurnTrace &trace) {
    if (release_attempt_ || std::ranges::any_of(chains_, [](const auto &context) {
            return context.blocked.has_value();
        })) {
        trace.clock_held = true;
        return;
    }
    RecoveryScheduleFacts schedule;
    if (const auto *empty = std::get_if<EmptyRecoverySchedule>(&restart.schedule))
        schedule = RecoveryScheduleCoverage{empty->facts_ref, empty->delivery_revision, {}, {}, {}};
    else
        schedule = std::get<RecoveryScheduleUnavailable>(restart.schedule);

    const DowntimeRecoveryInput input{restart.basis,       restart.policy, state_.timer,
                                      state_.timer_config, state_.pause,   restart.checkpoint,
                                      restart.restart,     restart.gap,    std::move(schedule)};
    auto result = preview_downtime_recovery(input);
    if (const auto *error = std::get_if<RecoveryPreviewRefusal>(&result)) {
        if (*error == RecoveryPreviewRefusal::InputLimit)
            throw BudgetExceeded{};
        classify(*error == RecoveryPreviewRefusal::ReviewAlreadyPending
                     ? Completion::unit_hold
                     : Completion::invalid_input);
        return;
    }
    auto preview = std::get<DowntimeRecoveryPreview>(std::move(result));
    const auto &chosen = preview.review_hold                              ? *preview.review_hold
                         : restart.policy == DowntimePolicy::CountElapsed ? *preview.count_elapsed
                                                                          : preview.freeze;
    if (state_.current_modifier)
        state_.current_modifier =
            project_modifier_restart(input, preview, *state_.current_modifier).value;
    state_.timer = chosen.timer;
    state_.pause = chosen.pause;
    state_.state_revision = turn.ref;
    timer_sample_ = turn.sample;
    timer_turn_ = &turn;
    countdown_paused_ = !state_.pause.reasons.empty();
    timeline_->set_countdown_paused(countdown_paused_, turn.sample.elapsed);
    trace.recovery = std::move(preview);

    // A later event uses the restart selection even if an older root was opened.
    restart_selection_ = &restart.retained_chain;
    const auto key = restart_selection_->root.event.chain;
    std::erase_if(chains_,
                  [&](const auto &context) { return context.events.front().binding.chain == key; });
    if (std::holds_alternative<chain::VerifiedNew>(restart_selection_->selection)) {
        trace.open_refusal = chain::OpenRefusal::history_unavailable;
        classify(Completion::unavailable);
        return;
    }
    static_cast<void>(open(*restart_selection_, trace));
}

bool valid_completion(RuleCompletion completion) {
    return completion == RuleCompletion::committed || completion == RuleCompletion::rolled_back ||
           uncertain(completion);
}

void validate(const ScenarioInputs &inputs) {
    const auto valid_timing = [](const MilestoneTimingOverride &timing) {
        return (!timing.presentation || *timing.presentation > 0ms) &&
               (!timing.gap || *timing.gap >= 0ms);
    };
    const auto &initial = inputs.initial;
    if (initial.state_revision.empty() || initial.time_debt_ms < 0 ||
        (initial.main_cap_ms && *initial.main_cap_ms < 0) || initial.timer.main_remaining_ms < 0 ||
        initial.timer.grace_remaining_ms < 0 || initial.timer_config.grace_duration_ms < 0 ||
        initial.timeline.presentation <= 0ms || initial.timeline.gap < 0ms)
        throw Refusal{Completion::invalid_input};
    if (initial.milestone_track &&
        initial.milestone_track->score_micro_points != initial.score_micro_points)
        throw Refusal{Completion::conflict};

    std::chrono::milliseconds elapsed{};
    for (const auto &turn : inputs.turns) {
        if (turn.ref.empty() || turn.sample.elapsed < elapsed || turn.ref == initial.state_revision)
            throw Refusal{Completion::invalid_input};
        elapsed = turn.sample.elapsed;
        if (const auto *event = std::get_if<EventTurn>(&turn.input)) {
            if (!valid_completion(event->completion))
                throw Refusal{Completion::invalid_input};
            for (const auto &[key, effect] : event->effect_refs) {
                if (effect.empty() || effect.size() > EffectivePermissionLimits{}.reference_bytes ||
                    std::ranges::count(event->effect_refs, key,
                                       [](const auto &row) { return row.first; }) != 1)
                    throw Refusal{Completion::conflict};
            }
            for (const auto &child : event->named) {
                const auto *cause =
                    child.cause ? std::get_if<chain::NamedAction>(&*child.cause) : nullptr;
                if (!cause ||
                    std::ranges::count(event->named, child.cause, &chain::EventBinding::cause) != 1)
                    throw Refusal{Completion::conflict};
            }
        }
        if (const auto *timeline = std::get_if<TimelineTurn>(&turn.input);
            timeline && timeline->head && !valid_completion(timeline->head->release_completion))
            throw Refusal{Completion::invalid_input};
        if (const auto *readback = std::get_if<GroupReadback>(&turn.input);
            readback && !valid_completion(readback->completion))
            throw Refusal{Completion::invalid_input};
        if (std::ranges::count(inputs.turns, turn.ref, &OwnerTurn::ref) != 1)
            throw Refusal{Completion::conflict};
    }

    for (const auto &root : inputs.roots) {
        const auto &key = root.root.event.chain;
        if (std::ranges::count_if(inputs.roots, [&](const auto &other) {
                return other.root.event.chain == key ||
                       (other.root.event.chain.installation == key.installation &&
                        other.root.normalized_occurrence == root.root.normalized_occurrence);
            }) != 1)
            throw Refusal{Completion::conflict};
    }

    for (const auto &capture : inputs.captures) {
        const auto &binding = capture.binding;
        const auto &receipt = capture.receipt;
        if (binding.event != receipt.event_ref || binding.parent != receipt.parent_event_ref ||
            binding.chain.root != receipt.root_event_ref ||
            capture.configuration.data().configuration_ref != receipt.arrival.configuration_ref ||
            capture.eligibility.conditions.valuation.original_money != receipt.facts.money ||
            (capture.valuation.historical &&
             (receipt.facts.money != capture.valuation.historical->original_money ||
              capture.eligibility.conditions.valuation.resolution_ref !=
                  capture.valuation.historical->resolution_ref ||
              (capture.rates &&
               capture.rates->data().snapshot_ref != capture.valuation.historical->snapshot_ref))))
            throw Refusal{Completion::conflict};
        if (capture.quote_age_threshold_ms && *capture.quote_age_threshold_ms < 0)
            throw Refusal{Completion::invalid_input};
        if (std::ranges::count_if(inputs.captures, [&](const auto &other) {
                return binding.chain.installation == other.binding.chain.installation &&
                       (binding.event == other.binding.event ||
                        binding.capture == other.binding.capture ||
                        receipt.identity == other.receipt.identity);
            }) != 1)
            throw Refusal{Completion::conflict};
        if (std::ranges::any_of(inputs.roots, [&](const auto &root) {
                return root.root.event.chain.installation != binding.chain.installation;
            }))
            throw Refusal{Completion::conflict};
    }

    for (const auto &group : inputs.milestone_captures) {
        const auto &event = group.released_event;
        if (!valid_timing(group.timing))
            throw Refusal{Completion::invalid_input};
        if (!event.parent || event.cause != chain::ChainCause{group.occurrence} ||
            group.occurrence.occurrence.empty() ||
            std::ranges::count_if(inputs.milestone_captures, [&](const auto &other) {
                return other.occurrence.occurrence == group.occurrence.occurrence ||
                       (other.released_event.chain == event.chain &&
                        other.released_event.parent == event.parent &&
                        other.occurrence.ordinal == group.occurrence.ordinal);
            }) != 1)
            throw Refusal{Completion::conflict};
    }
}
} // namespace

SimulationResult inspect_simulation(const Scenario &scenario, const Limits &limits) {
    const auto *replay = std::get_if<CapturedReplay>(&scenario.input);
    const auto &inputs = replay ? replay->inputs : std::get<Hypothetical>(scenario.input).inputs;
    SimulationResult result{scenario.ref,
                            replay ? Mode::captured_replay : Mode::hypothetical,
                            true,
                            Completion::complete,
                            {},
                            inputs.initial,
                            0};
    try {
        const auto &capture_ref =
            replay ? replay->capture : std::get<Hypothetical>(scenario.input).capture;
        if (scenario.version != 1 || scenario.ref.empty() || capture_ref.empty() ||
            (replay && replay->attempted_manual_modifiers) || !limits.turns ||
            !limits.owned_bytes || !limits.rows)
            throw Refusal{Completion::invalid_input};
        if (inputs.turns.size() > limits.turns)
            throw BudgetExceeded{};
        Budget budget{limits.owned_bytes, limits.rows};
        budget.entry(scenario);
        budget.entry(result.scenario);
        budget.entry(result.final_state);
        validate(inputs);
        if (replay) {
            switch (replay->selected) {
            case CaptureSelection::exact:
                break;
            case CaptureSelection::unavailable:
                throw Refusal{Completion::unavailable};
            case CaptureSelection::conflict:
                throw Refusal{Completion::conflict};
            default:
                throw Refusal{Completion::invalid_input};
            }
        }
        Run{inputs, limits, budget, result}.run();
    } catch (const BudgetExceeded &) {
        result.completion = Completion::resource_hold;
    } catch (const Refusal &refusal) {
        result.completion = refusal.completion;
    } catch (const MilestoneProgressionFailure &failure) {
        result.completion = failure.code() == MilestoneProgressionError::resource_limit
                                ? Completion::resource_hold
                                : Completion::invalid_input;
    } catch (const RuleDefinitionFailure &failure) {
        result.completion = failure.code() == RuleDefinitionError::resource_limit
                                ? Completion::resource_hold
                                : Completion::invalid_input;
    } catch (const std::length_error &) {
        result.completion = Completion::resource_hold;
    } catch (const std::invalid_argument &) {
        result.completion = Completion::invalid_input;
    }
    return result;
}
} // namespace subathon::server::rule_simulator
