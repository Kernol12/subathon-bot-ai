// SPDX-License-Identifier: GPL-3.0-or-later
#include "rule_chain_evidence.hpp"

namespace subathon::core::rule_chain {
namespace {

using evidence::Bytes;
using evidence::exact;

constexpr RuleGenerationLimits default_limits{};
constexpr EffectivePermissionLimits reference_limits{};
constexpr auto capacity_reason = "generation_capacity_hold";
constexpr auto cutoff_reason = "generation_cutoff";
constexpr auto policy_reason = "retained_policy_exceeds_profile";

bool valid_ref(std::string_view value) {
    return !value.empty() && value.size() <= reference_limits.reference_bytes;
}

bool valid(const RuleActionKey &key) {
    return valid_ref(key.rule_ref) && valid_ref(key.rule_version) && valid_ref(key.action_ref) &&
           valid_ref(key.action_version);
}

bool valid(const MilestoneOccurrence &occurrence) {
    return valid_ref(occurrence.source) && valid_ref(occurrence.occurrence);
}

bool valid(const EventBinding &event) {
    if (!valid_ref(event.chain.installation) || !valid_ref(event.chain.root) ||
        !valid_ref(event.event) || !valid_ref(event.capture))
        return false;
    if (!event.parent)
        return event.chain.root == event.event && !event.cause;
    return valid_ref(*event.parent) && *event.parent != event.event &&
           event.event != event.chain.root && event.cause &&
           !event.cause->valueless_by_exception() &&
           std::visit(
               [](const auto &cause) {
                   if constexpr (std::is_same_v<std::decay_t<decltype(cause)>, NamedAction>)
                       return valid(cause.key);
                   else
                       return valid(cause);
               },
               *event.cause);
}

bool fits(RuleGenerationLimits limits, RuleGenerationLimits maximum) {
    return limits.maximum_depth <= maximum.maximum_depth &&
           limits.maximum_generated_events <= maximum.maximum_generated_events;
}

bool valid(const ChainProfile &profile) {
    return fits(default_limits, profile.maximum) && profile.active_roots > 0 &&
           profile.pending_events > 0 && profile.owned_bytes > 0;
}

RuleEventLineage lineage(const EventBinding &event) {
    const auto *named = event.cause ? std::get_if<NamedAction>(&*event.cause) : nullptr;
    return {event.event, event.parent, event.chain.root,
            named ? std::optional{named->key} : std::nullopt};
}

bool matches(const RuleEventLineage &seen, const EventBinding &event) {
    const auto *named = event.cause ? std::get_if<NamedAction>(&*event.cause) : nullptr;
    return seen.event_ref == event.event && seen.parent_ref == event.parent &&
           seen.root_ref == event.chain.root &&
           (named ? seen.cause && *seen.cause == named->key : !seen.cause);
}

bool same_occurrence(const MilestoneOccurrence &a, const MilestoneOccurrence &b) {
    return a.source == b.source && a.occurrence == b.occurrence;
}

bool same_child_claim(const EventBinding &a, const EventBinding &b) {
    if (a.event == b.event || a.capture == b.capture)
        return true;
    if (!a.cause || !b.cause)
        return false;
    if (const auto *named = std::get_if<NamedAction>(&*a.cause)) {
        const auto *other = std::get_if<NamedAction>(&*b.cause);
        return other && a.parent == b.parent && *other == *named;
    }
    const auto *ours = std::get_if<MilestoneOccurrence>(&*a.cause);
    const auto *other = std::get_if<MilestoneOccurrence>(&*b.cause);
    return ours && other && same_occurrence(*ours, *other);
}

template <class Cause> bool has_cause(const EventBinding &event, const Cause &cause) {
    const auto *binding = event.cause ? std::get_if<Cause>(&*event.cause) : nullptr;
    return binding && *binding == cause;
}

const AdmittedEvent *find_event(std::span<const AdmittedEvent> events, std::string_view id) {
    const auto found = std::ranges::find_if(
        events, [&](const AdmittedEvent &entry) { return entry.binding.event == id; });
    return found == events.end() ? nullptr : &*found;
}

bool acknowledged(std::span<const Publication> publications, std::string_view id) {
    return std::ranges::any_of(publications, [&](const Publication &pub) {
        return pub.pass && pub.identity.anchor == id;
    });
}

const OccurrenceDisposition *find_occurrence(std::span<const OccurrenceDisposition> dispositions,
                                             const MilestoneOccurrence &occurrence) {
    const auto found = std::ranges::find_if(dispositions, [&](const OccurrenceDisposition &entry) {
        return same_occurrence(entry.occurrence, occurrence);
    });
    return found == dispositions.end() ? nullptr : &*found;
}

std::optional<CutoffReason> reached(std::uint32_t depth, std::uint32_t ordinal,
                                    RuleGenerationLimits limits) {
    if (depth == limits.maximum_depth)
        return ordinal == limits.maximum_generated_events ? CutoffReason::both
                                                          : CutoffReason::depth;
    if (ordinal == limits.maximum_generated_events)
        return CutoffReason::count;
    return {};
}

bool exact_cutoff(const Cutoff &cutoff, const AdmittedEvent &last, const RootBinding &root,
                  RuleGenerationLimits limits, CutoffReason reason) {
    return cutoff.chain == root.event.chain && cutoff.root_capture == root.event.capture &&
           cutoff.last == last && exact(cutoff.limits, limits) && cutoff.reason == reason;
}

enum class EvidenceIssue { conflict, unavailable, capacity };
using SelectedCapture = std::variant<CaptureView, EvidenceIssue>;

SelectedCapture capture_event(const OwnerView &owner, const EventBinding &event) {
    auto capture = owner.capture ? owner.capture(event) : std::nullopt;
    if (!capture)
        return EvidenceIssue::unavailable;
    const auto &config = capture->configuration.get().data();
    if (capture->input.event_ref != event.event ||
        capture->input.configuration_ref != config.configuration_ref ||
        capture->authors.size() != config.rules.size() ||
        !std::ranges::all_of(capture->authors, valid_ref))
        return EvidenceIssue::conflict;
    return std::move(*capture);
}

const RuleText *text_fact(const CaptureView &capture, RuleTriggerKind kind, RuleTextField field) {
    if (capture.input.kind != kind)
        return nullptr;
    const RuleText *found = nullptr;
    for (const auto &fact : capture.input.facts) {
        if (const auto *text = std::get_if<RuleText>(&fact); text && text->field == field) {
            if (found)
                return nullptr;
            found = text;
        }
    }
    return found;
}

bool typed_child(const CaptureView &capture, const EventBinding &child,
                 const RuleNamedEvent *named) {
    if (std::holds_alternative<NamedAction>(*child.cause)) {
        const auto *name =
            text_fact(capture, RuleTriggerKind::named_event, RuleTextField::event_name);
        return name && !name->value.empty() && (!named || named->event_name == name->value);
    }
    const auto *milestone =
        text_fact(capture, RuleTriggerKind::milestone_completion, RuleTextField::milestone_ref);
    return milestone && valid_ref(milestone->value);
}

bool charge_capture(Bytes &bytes, const CaptureView &capture) {
    return bytes.entries(capture, capture.configuration.get().data());
}

RuleSelectionResult select_capture(const CaptureView &capture) {
    return select_rules(capture.configuration, capture.input, capture.valuations);
}

bool key_at(const RuleDefinition &rule, std::size_t position, const RuleActionKey &key) {
    return position < rule.actions.size() && key.position == position &&
           key.rule_ref == rule.rule_ref && key.rule_version == rule.version_ref &&
           key.action_ref == rule.actions[position].action_ref &&
           key.action_version == rule.actions[position].version_ref;
}

const RuleActionDefinition *selected_action(const CaptureView &capture,
                                            std::span<const std::size_t> selected,
                                            const RuleActionKey &key) {
    const auto &rules = capture.configuration.get().data().rules;
    for (auto index : selected) {
        const auto &rule = rules[index];
        if (key_at(rule, key.position, key))
            return &rule.actions[key.position];
    }
    return nullptr;
}

const char *stop_reason(const GenerationStop &stop) {
    return std::holds_alternative<Cutoff>(stop) ? cutoff_reason : policy_reason;
}

bool valid_stop(const GenerationStop &stop, const Cutoff *cutoff) {
    if (const auto *record = std::get_if<Cutoff>(&stop))
        return cutoff && exact(*record, *cutoff);
    return !cutoff && std::holds_alternative<RetainedPolicyExceedsProfile>(stop);
}

// Check a #208 receipt against its exact selected-rule order and child edges.
// The owner supplies the actual effects and current gate outcomes.
bool valid_pass(const Publication &pub, const CaptureView &capture,
                std::span<const std::size_t> selected, const Cutoff *cutoff) {
    if (!pub.pass || pub.pass->rules.size() != selected.size() || !pub.occurrences.empty())
        return false;
    const auto &rules = capture.configuration.get().data().rules;
    std::size_t child_index = 0;
    std::size_t suppression_index = 0;
    for (std::size_t i = 0; i < selected.size(); ++i) {
        const auto &report = pub.pass->rules[i];
        const auto &rule = rules[selected[i]];
        if (report.captured_index != selected[i] || report.accepted.size() > rule.actions.size())
            return false;
        for (std::size_t position = 0; position < report.accepted.size(); ++position) {
            const auto &accepted = report.accepted[position];
            if (!key_at(rule, position, accepted.key))
                return false;
            const auto *child = std::get_if<RuleEventLineage>(&accepted.observed);
            if (std::holds_alternative<RuleNamedEvent>(rule.actions[position].payload)) {
                if (!child || child_index == pub.children.size() || child->cause != accepted.key ||
                    !matches(*child, pub.children[child_index].binding))
                    return false;
                ++child_index;
            } else if (child || accepted.observed.valueless_by_exception()) {
                return false;
            }
        }
        if (!report.stop) {
            if (report.accepted.size() != rule.actions.size())
                return false;
            continue;
        }
        const auto &stop = *report.stop;
        if (stop.position != report.accepted.size() || stop.position >= rule.actions.size())
            return false;
        const auto *reason =
            stop.detail && stop.detail->opaque_reason ? &*stop.detail->opaque_reason : nullptr;
        const bool named =
            std::holds_alternative<RuleNamedEvent>(rule.actions[stop.position].payload);
        if (!named || stop.refusal != RuleActionRefusal::handler_refusal || !reason)
            continue;
        if (suppression_index == pub.named_suppressions.size())
            continue;
        const auto &suppression = pub.named_suppressions[suppression_index];
        // An opaque reason alone cannot establish a typed chain stop.
        if (!key_at(rule, stop.position, suppression.action))
            continue;
        ++suppression_index;
        // A staged cutoff is available only after acceptance of its final child.
        const auto *stop_cutoff =
            pub.cutoff && child_index < pub.children.size() ? nullptr : cutoff;
        if (*reason != stop_reason(suppression.stop) ||
            !valid_stop(suppression.stop, stop_cutoff) ||
            (!pub.children.empty() &&
             std::holds_alternative<RetainedPolicyExceedsProfile>(suppression.stop)))
            return false;
    }
    return child_index == pub.children.size() && suppression_index == pub.named_suppressions.size();
}

struct Work {
    Publication draft;
    std::optional<CaptureView> capture;
    std::vector<std::size_t> selected;
    std::optional<BlockReason> block;
    bool finished = false;
    bool rollback_only = false;
    bool awaiting_named = false;
    bool restored = false;
    std::size_t child_capture_bytes = 0;
    std::size_t reserved_capture_bytes = 0;
};

struct Context {
    ModeledHistory history;
    ChainProfile runtime;
    RuleGenerationLimits limits;
    OwnerView owner;
    // Bytes for the committed view and its capture backing, excluding active work.
    std::size_t retained_bytes;
    std::size_t pending;
    std::shared_ptr<Work> current;
};

std::span<const Publication> publications(const Context &chain) {
    return chain.history.publications;
}

const Cutoff *current_cutoff(const Context &chain, const Work &work) {
    return work.draft.cutoff      ? &*work.draft.cutoff
           : chain.history.cutoff ? &*chain.history.cutoff
                                  : nullptr;
}

std::optional<Bytes> capacity(const ChainProfile &profile, CapacityUse other, std::size_t pending) {
    if (other.active_roots >= profile.active_roots ||
        other.pending_events > profile.pending_events ||
        pending > profile.pending_events - other.pending_events ||
        other.owned_bytes > profile.owned_bytes)
        return {};
    return Bytes{profile.owned_bytes - other.owned_bytes};
}

CapacityUse other_use(const Context &chain) {
    return chain.owner.other_usage(chain.history.root.event.chain);
}

// Include the second representation of each child in the retained event list.
// The selected indices and optional capture are allocated only for this unit.
bool charge_work(Bytes &budget, const Work &work) {
    if (!budget.take(sizeof(Work)) || !budget.payloads(work.draft, work.selected) ||
        (work.capture &&
         !budget.payloads(work.capture->input.event_ref, work.capture->input.configuration_ref,
                          work.capture->input.arrival_snapshot_ref)) ||
        !budget.take(work.child_capture_bytes))
        return false;
    return std::ranges::all_of(work.draft.children,
                               [&](const AdmittedEvent &event) { return budget.entry(event); });
}

std::optional<Bytes> growth(const Context &chain, const Work &work, std::size_t new_pending = 0) {
    const auto staged = work.draft.children.size();
    if (staged > chain.runtime.pending_events - chain.pending ||
        new_pending > chain.runtime.pending_events - chain.pending - staged)
        return {};
    auto space = capacity(chain.runtime, other_use(chain), chain.pending + staged + new_pending);
    if (!space || !space->take(chain.retained_bytes) || !charge_work(*space, work))
        return {};
    return space;
}

const OccurrenceDisposition *recorded_occurrence(const Context &chain, const Work *work,
                                                 const MilestoneOccurrence &occurrence) {
    if (work) {
        if (const auto *found = find_occurrence(work->draft.occurrences, occurrence))
            return found;
    }
    for (const auto &pub : publications(chain)) {
        if (const auto *found = find_occurrence(pub.occurrences, occurrence))
            return found;
    }
    return nullptr;
}

ChildLookup lookup(const Context &chain, const EventBinding &event) {
    if (!valid(event) || !event.parent || event.chain != chain.history.root.event.chain ||
        !find_event(chain.history.events, *event.parent))
        return Conflict{};
    for (const auto &known : chain.history.events) {
        if (same_child_claim(known.binding, event))
            return known.binding == event ? ChildLookup{Duplicate{known}} : ChildLookup{Conflict{}};
    }
    if (const auto *occurrence = std::get_if<MilestoneOccurrence>(&*event.cause)) {
        if (const auto *known = recorded_occurrence(chain, nullptr, *occurrence);
            known && known->suppressed_binding != event)
            return Conflict{};
    }
    for (const auto &pub : publications(chain)) {
        if (std::ranges::any_of(pub.occurrences, [&](const OccurrenceDisposition &record) {
                return record.suppressed_binding && *record.suppressed_binding != event &&
                       same_child_claim(*record.suppressed_binding, event);
            }))
            return Conflict{};
    }
    if (chain.current && chain.current->block)
        return Unavailable{};
    return NotRecorded{};
}

bool valid_occurrences(const ModeledHistory &history, std::size_t publication_index,
                       const Cutoff *cutoff) {
    const auto &pub = history.publications[publication_index];
    if (pub.pass || !pub.named_suppressions.empty())
        return false;
    std::size_t child_index = 0;
    for (std::size_t i = 0; i < pub.occurrences.size(); ++i) {
        const auto &record = pub.occurrences[i];
        if (!valid(record.occurrence) || record.parent != pub.identity.anchor ||
            find_occurrence(std::span{pub.occurrences}.first(i), record.occurrence))
            return false;
        for (const auto &previous : std::span{history.publications}.first(publication_index)) {
            if (find_occurrence(previous.occurrences, record.occurrence))
                return false;
        }
        if (const auto *child = std::get_if<AdmittedEvent>(&record.disposition)) {
            if (record.suppressed_binding || child_index == pub.children.size() ||
                *child != pub.children[child_index++] ||
                !has_cause(child->binding, record.occurrence))
                return false;
        } else if (const auto *suppressed =
                       std::get_if<GenerationSuppressed>(&record.disposition)) {
            const auto *stop_cutoff =
                pub.cutoff && child_index < pub.children.size() ? nullptr : cutoff;
            const auto &event = record.suppressed_binding;
            if (!event || !valid(*event) || event->chain != pub.identity.chain ||
                event->parent != record.parent || !has_cause(*event, record.occurrence) ||
                !valid_stop(suppressed->stop, stop_cutoff) ||
                (!pub.children.empty() &&
                 std::holds_alternative<RetainedPolicyExceedsProfile>(suppressed->stop)) ||
                std::ranges::any_of(history.events, [&](const AdmittedEvent &entry) {
                    return same_child_claim(entry.binding, *event);
                }))
                return false;
            const auto aliases_release = [&](const OccurrenceDisposition &prior) {
                return prior.suppressed_binding &&
                       same_child_claim(*prior.suppressed_binding, *event);
            };
            if (std::ranges::any_of(std::span{pub.occurrences}.first(i), aliases_release) ||
                std::ranges::any_of(std::span{history.publications}.first(publication_index),
                                    [&](const Publication &previous) {
                                        return std::ranges::any_of(previous.occurrences,
                                                                   aliases_release);
                                    }))
                return false;
        } else if (!std::holds_alternative<Skipped>(record.disposition) ||
                   record.suppressed_binding) {
            return false;
        }
    }
    return child_index == pub.children.size();
}

using CheckedHistory = std::variant<EvidenceIssue, std::pair<std::size_t, std::size_t>>;

// Replay publication order over the supplied history. Nothing is installed
// until the exact root, every edge and every finished claim has been checked.
CheckedHistory check_history(const ModeledHistory &history, const OwnerView &owner,
                             const ChainProfile &runtime, RuleGenerationLimits limits,
                             const CaptureView &root_capture) {
    const auto &root = history.root;
    const auto events = std::span{history.events};
    const auto pubs = std::span{history.publications};
    if (!valid_ref(history.admission) || !valid(history.admitted_profile) ||
        !fits(limits, history.admitted_profile.maximum) || events.empty() ||
        events.front().binding != root.event || events.front().depth != 0 ||
        events.front().ordinal != 0 || events.size() - 1 > limits.maximum_generated_events)
        return EvidenceIssue::conflict;

    const auto other = owner.other_usage(root.event.chain);
    auto space = capacity(runtime, other, 0);
    if (!space || !space->take(sizeof(Context)) || !space->payloads(history) ||
        !charge_capture(*space, root_capture))
        return EvidenceIssue::capacity;

    std::size_t admitted = 1;
    std::size_t passes = 0;
    const Cutoff *cutoff = nullptr;
    for (std::size_t i = 0; i < pubs.size(); ++i) {
        const auto &pub = pubs[i];
        const auto &id = pub.identity;
        const auto prefix = pubs.first(i);
        const auto *parent = find_event(events.first(admitted), id.anchor);
        if (!parent || !valid_ref(id.unit) || id.chain != root.event.chain ||
            std::ranges::any_of(prefix, [&](const Publication &previous) {
                return previous.identity.unit == id.unit;
            }))
            return EvidenceIssue::conflict;
        const auto *previous_cutoff = cutoff;
        std::optional<CaptureView> parent_capture;
        if (id.purpose == UnitPurpose::event_pass) {
            if (acknowledged(prefix, id.anchor))
                return EvidenceIssue::conflict;
            auto selected = capture_event(owner, parent->binding);
            if (const auto *issue = std::get_if<EvidenceIssue>(&selected))
                return *issue;
            parent_capture.emplace(std::get<CaptureView>(std::move(selected)));
            ++passes;
        } else if (id.purpose != UnitPurpose::released_occurrences ||
                   !acknowledged(prefix, id.anchor)) {
            return EvidenceIssue::conflict;
        }
        for (const auto &child : pub.children) {
            const auto &event = child.binding;
            if (cutoff || admitted == events.size() || child != events[admitted] || !valid(event) ||
                event.chain != root.event.chain || event.parent != id.anchor ||
                parent->depth >= limits.maximum_depth || child.depth != parent->depth + 1 ||
                child.ordinal != admitted ||
                std::ranges::any_of(events.first(admitted), [&](const AdmittedEvent &entry) {
                    return same_child_claim(entry.binding, event);
                }))
                return EvidenceIssue::conflict;
            auto selected = capture_event(owner, event);
            if (const auto *issue = std::get_if<EvidenceIssue>(&selected))
                return *issue;
            const auto &capture = std::get<CaptureView>(selected);
            const RuleNamedEvent *named = nullptr;
            if (parent_capture) {
                const auto *cause = std::get_if<NamedAction>(&*event.cause);
                if (!cause)
                    return EvidenceIssue::conflict;
                for (const auto &rule : parent_capture->configuration.get().data().rules) {
                    if (key_at(rule, cause->key.position, cause->key))
                        named =
                            std::get_if<RuleNamedEvent>(&rule.actions[cause->key.position].payload);
                }
                if (!named)
                    return EvidenceIssue::conflict;
            } else if (!std::holds_alternative<MilestoneOccurrence>(*event.cause)) {
                return EvidenceIssue::conflict;
            }
            if (!charge_capture(*space, capture))
                return EvidenceIssue::capacity;
            if (!typed_child(capture, event, named))
                return EvidenceIssue::conflict;
            try {
                static_cast<void>(select_capture(capture));
            } catch (const RuleDefinitionFailure &) {
                return EvidenceIssue::conflict;
            }
            if (auto reason = reached(child.depth, child.ordinal, limits)) {
                if (!pub.cutoff || !exact_cutoff(*pub.cutoff, child, root, limits, *reason))
                    return EvidenceIssue::conflict;
                cutoff = &*pub.cutoff;
            }
            ++admitted;
        }
        if (pub.cutoff.has_value() != (previous_cutoff != cutoff))
            return EvidenceIssue::conflict;
        if (parent_capture) {
            try {
                const auto selected = select_capture(*parent_capture);
                if (!valid_pass(pub, *parent_capture, selected.selected_rule_indices, cutoff))
                    return EvidenceIssue::conflict;
            } catch (const RuleDefinitionFailure &) {
                return EvidenceIssue::conflict;
            }
        } else if (!valid_occurrences(history, i, cutoff)) {
            return EvidenceIssue::conflict;
        }
    }
    if (admitted != events.size() || history.cutoff.has_value() != (cutoff != nullptr) ||
        (cutoff && !exact(*history.cutoff, *cutoff)))
        return EvidenceIssue::conflict;

    auto used = runtime.owned_bytes - other.owned_bytes - space->remaining();
    if (history.blocked) {
        const auto &blocked = *history.blocked;
        const auto &id = blocked.identity;
        const bool source_done = acknowledged(pubs, id.anchor);
        if (id.chain != root.event.chain || !valid_ref(id.unit) || !find_event(events, id.anchor) ||
            std::ranges::any_of(
                pubs, [&](const Publication &pub) { return pub.identity.unit == id.unit; }) ||
            (id.purpose == UnitPurpose::event_pass
                 ? source_done
                 : id.purpose != UnitPurpose::released_occurrences || !source_done) ||
            (blocked.reason != BlockReason::unresolved && blocked.reason != BlockReason::conflict))
            return EvidenceIssue::conflict;
        if (!space->take(sizeof(Work)) || !space->payloads(id))
            return EvidenceIssue::capacity;
    }

    const auto pending = events.size() - passes;
    if (pending > runtime.pending_events - other.pending_events)
        return EvidenceIssue::capacity;
    return std::pair{used, pending};
}

OpenRefusal refusal(EvidenceIssue issue) {
    switch (issue) {
    case EvidenceIssue::capacity:
        return OpenRefusal::capacity_hold;
    case EvidenceIssue::unavailable:
        return OpenRefusal::history_unavailable;
    case EvidenceIssue::conflict:
        return OpenRefusal::conflict;
    }
    return OpenRefusal::conflict;
}

} // namespace

struct Chain::State : Context {};
struct Unit::State {
    std::weak_ptr<Context> chain;
    std::weak_ptr<Work> work;

    ~State() {
        if (auto active = work.lock(); active && !active->finished) {
            active->finished = true;
            if (!active->block)
                active->block = BlockReason::unresolved;
        }
    }
};

struct ChainOperations {
    static Chain open(Context context) {
        Chain chain;
        chain.state_ = std::make_shared<Chain::State>(Chain::State{std::move(context)});
        return chain;
    }

    static Unit begin(Chain &chain, std::shared_ptr<Work> work) {
        Unit unit;
        unit.state_ = std::make_unique<Unit::State>();
        unit.state_->chain = chain.state_;
        unit.state_->work = work;
        chain.state_->current = std::move(work);
        return unit;
    }

    static Context &get(Chain &chain) { return *chain.state_; }
    static const Context &get(const Chain &chain) { return *chain.state_; }

    static auto lock(const Unit &unit) {
        if (!unit.state_)
            return std::pair<std::shared_ptr<Context>, std::shared_ptr<Work>>{};
        return std::pair{unit.state_->chain.lock(), unit.state_->work.lock()};
    }
};

Chain::Chain() = default;
Chain::Chain(Chain &&) noexcept = default;
Chain &Chain::operator=(Chain &&) noexcept = default;
Chain::~Chain() = default;
Unit::Unit() = default;
Unit::Unit(Unit &&) noexcept = default;
Unit &Unit::operator=(Unit &&) noexcept = default;
Unit::~Unit() = default;

PolicyCheck check_generation_policy(const RuleConfiguration &configuration,
                                    const ChainProfile &profile) {
    if (!valid(profile))
        return PolicyCheck::invalid_profile;
    return fits(configuration.data().generation, profile.maximum) ? PolicyCheck::supported
                                                                  : PolicyCheck::exceeds_profile;
}

OpenResult open_chain(const RootBinding &root, const ChainProfile &runtime,
                      const OwnerSelection &selection, OwnerView owner) {
    if (!valid(runtime))
        return OpenRefusal::invalid_profile;
    if (!valid(root.event) || root.event.parent || !valid_ref(root.normalized_occurrence))
        return OpenRefusal::conflict;
    const auto *fresh = std::get_if<VerifiedNew>(&selection);
    const auto *retained = std::get_if<ModeledHistory>(&selection);
    if (!fresh && !retained)
        return OpenRefusal::history_unavailable;
    if ((retained && retained->root != root) || (fresh && !valid_ref(fresh->proposed_admission)))
        return OpenRefusal::conflict;
    if (!owner.capture || !owner.other_usage)
        return OpenRefusal::history_unavailable;
    auto selected = capture_event(owner, root.event);
    if (const auto *issue = std::get_if<EvidenceIssue>(&selected))
        return refusal(*issue);
    const auto &capture = std::get<CaptureView>(selected);
    const auto limits = capture.configuration.get().data().generation;
    if (fresh && !fits(limits, runtime.maximum))
        return OpenRefusal::needs_reactivation;

    if (fresh) {
        const auto other = owner.other_usage(root.event.chain);
        auto space = capacity(runtime, other, 1);
        if (!space || !space->take(sizeof(Context) + sizeof(AdmittedEvent)) ||
            !space->payloads(root, fresh->proposed_admission, root.event) ||
            !charge_capture(*space, capture))
            return OpenRefusal::capacity_hold;
        try {
            static_cast<void>(select_capture(capture));
        } catch (const RuleDefinitionFailure &) {
            return OpenRefusal::conflict;
        }
        ModeledHistory history{
            root, fresh->proposed_admission, runtime, {{root.event, 0, 0}}, {}, {}, {}};
        const auto used = runtime.owned_bytes - other.owned_bytes - space->remaining();
        return OpenedChain{
            ChainOperations::open(
                {std::move(history), runtime, limits, std::move(owner), used, 1, {}}),
            0,
            {}};
    }

    auto checked = check_history(*retained, owner, runtime, limits, capture);
    if (const auto *issue = std::get_if<EvidenceIssue>(&checked))
        return refusal(*issue);
    try {
        static_cast<void>(select_capture(capture));
    } catch (const RuleDefinitionFailure &) {
        return OpenRefusal::conflict;
    }
    const auto [used, pending] = std::get<std::pair<std::size_t, std::size_t>>(checked);
    std::shared_ptr<Work> blocked;
    if (retained->blocked) {
        blocked = std::make_shared<Work>();
        blocked->draft.identity = retained->blocked->identity;
        blocked->block = retained->blocked->reason;
        blocked->finished = true;
        blocked->restored = true;
    }
    auto history = *retained;
    history.blocked.reset();
    return OpenedChain{ChainOperations::open({std::move(history), runtime, limits, std::move(owner),
                                              used, pending, std::move(blocked)}),
                       static_cast<std::uint32_t>(retained->events.size() - 1), retained->cutoff};
}

namespace {

BeginResult begin(Chain &handle, const AdmittedEvent &anchor, std::string id, UnitPurpose purpose) {
    auto &chain = ChainOperations::get(handle);
    const auto prior = publications(chain);
    const bool source_done = acknowledged(prior, anchor.binding.event);
    if (purpose == UnitPurpose::event_pass && source_done)
        return BeginDisposition::duplicate_event;
    if (chain.current)
        return BeginDisposition::in_flight_hold;
    if (purpose == UnitPurpose::released_occurrences && !source_done)
        return BeginDisposition::history_unavailable;
    if (!valid_ref(id) ||
        std::ranges::any_of(prior, [&](const Publication &pub) { return pub.identity.unit == id; }))
        return BeginDisposition::conflict;

    auto space = capacity(chain.runtime, other_use(chain), chain.pending);
    if (!space || !space->take(chain.retained_bytes) || !space->take(sizeof(Work)) ||
        !space->payloads(id, anchor.binding.chain, anchor.binding.event))
        return BeginDisposition::capacity_hold;

    std::optional<CaptureView> capture;
    std::vector<std::size_t> selected;
    if (purpose == UnitPurpose::event_pass) {
        auto resolved = capture_event(chain.owner, anchor.binding);
        if (const auto *issue = std::get_if<EvidenceIssue>(&resolved))
            return *issue == EvidenceIssue::unavailable ? BeginDisposition::history_unavailable
                                                        : BeginDisposition::conflict;
        capture.emplace(std::get<CaptureView>(std::move(resolved)));
        if (!space->payloads(capture->input.event_ref, capture->input.configuration_ref,
                             capture->input.arrival_snapshot_ref) ||
            capture->authors.size() > space->remaining() / sizeof(std::size_t))
            return BeginDisposition::capacity_hold;
        try {
            selected = select_capture(*capture).selected_rule_indices;
        } catch (const RuleDefinitionFailure &) {
            return BeginDisposition::conflict;
        }
    }
    auto work = std::make_shared<Work>();
    work->draft.identity = {std::move(id), anchor.binding.chain, anchor.binding.event, purpose};
    work->capture = std::move(capture);
    work->selected = std::move(selected);
    return ChainOperations::begin(handle, std::move(work));
}

} // namespace

BeginResult begin_event(Chain &handle, const EventBinding &event, std::string unit) {
    const auto &chain = ChainOperations::get(handle);
    if (!valid(event) || event.chain != chain.history.root.event.chain)
        return BeginDisposition::conflict;
    const auto *anchor = find_event(chain.history.events, event.event);
    if (anchor && anchor->binding != event)
        return BeginDisposition::conflict;
    if (anchor)
        return begin(handle, *anchor, std::move(unit), UnitPurpose::event_pass);
    if (event.parent && std::holds_alternative<Conflict>(lookup(chain, event)))
        return BeginDisposition::conflict;
    if (chain.current)
        return BeginDisposition::in_flight_hold;
    return BeginDisposition::not_admitted;
}

BeginResult begin_occurrences(Chain &handle, const std::string &parent, std::string unit) {
    const auto &chain = ChainOperations::get(handle);
    if (chain.current)
        return BeginDisposition::in_flight_hold;
    if (!valid_ref(parent))
        return BeginDisposition::conflict;
    const auto *anchor = find_event(chain.history.events, parent);
    if (!anchor)
        return BeginDisposition::not_admitted;
    return begin(handle, *anchor, std::move(unit), UnitPurpose::released_occurrences);
}

RuleActionPlan plan_event(const Unit &unit, const RuleActionMapper &mapper) {
    const auto [chain, work] = ChainOperations::lock(unit);
    if (!chain || !work || work->finished || work->block || !work->capture)
        throw std::logic_error("rule action planning requires an active event unit");
    const auto &capture = *work->capture;
    const auto &anchor = find_event(chain->history.events, work->draft.identity.anchor)->binding;
    return plan_rule_actions(capture.configuration, capture.input, lineage(anchor), capture.authors,
                             capture.valuations, mapper);
}

ChildLookup lookup_child(const Chain &handle, const EventBinding &child) {
    const auto &chain = ChainOperations::get(handle);
    auto known = lookup(chain, child);
    if (chain.current && (std::holds_alternative<NotRecorded>(known) ||
                          std::holds_alternative<Unavailable>(known))) {
        for (const auto &staged : chain.current->draft.children) {
            if (same_child_claim(staged.binding, child))
                return staged.binding == child ? ChildLookup{Unavailable{}}
                                               : ChildLookup{Conflict{}};
        }
    }
    return known;
}

namespace {

bool child_of(const Work &work, const EventBinding &event) {
    return valid(event) && event.chain == work.draft.identity.chain &&
           event.parent == work.draft.identity.anchor;
}

bool terminal_binding_claimed(const Context &chain, const Work &work, const EventBinding &child) {
    const auto claimed = [&](const auto &records) {
        return std::ranges::any_of(records, [&](const OccurrenceDisposition &record) {
            return record.suppressed_binding && same_child_claim(*record.suppressed_binding, child);
        });
    };
    return claimed(work.draft.occurrences) ||
           std::ranges::any_of(publications(chain),
                               [&](const Publication &pub) { return claimed(pub.occurrences); });
}

bool generation_suppressed(const Context &chain, const Cutoff *cutoff) {
    return cutoff || !fits(chain.limits, chain.runtime.maximum);
}

GenerationStop generation_stop(const Cutoff *cutoff) {
    return cutoff ? GenerationStop{*cutoff} : GenerationStop{RetainedPolicyExceedsProfile{}};
}

ChildDecision reserve_child(const Context &chain, Work &work, const EventBinding &event,
                            const RuleNamedEvent *named) {
    const auto *parent = find_event(chain.history.events, work.draft.identity.anchor);
    const auto count = chain.history.events.size() - 1 + work.draft.children.size();
    if (parent->depth >= chain.limits.maximum_depth ||
        count >= chain.limits.maximum_generated_events)
        return Conflict{}; // A limit without its cutoff is contradictory.

    const auto depth = parent->depth + 1;
    const auto ordinal = static_cast<std::uint32_t>(count + 1);
    const auto reason = reached(depth, ordinal, chain.limits);
    auto space = growth(chain, work, 1);
    if (!space || !space->take(2 * sizeof(AdmittedEvent)) || !space->payloads(event, event))
        return CapacityHold{};
    if (reason &&
        !space->payloads(event, event, event.chain, event.chain, chain.history.root.event.capture,
                         chain.history.root.event.capture))
        return CapacityHold{};
    if (!named &&
        (!space->take(sizeof(OccurrenceDisposition)) ||
         !space->payloads(event, *event.parent, std::get<MilestoneOccurrence>(*event.cause))))
        return CapacityHold{};

    auto selected = capture_event(chain.owner, event);
    if (const auto *issue = std::get_if<EvidenceIssue>(&selected)) {
        if (*issue == EvidenceIssue::unavailable)
            work.block = BlockReason::unresolved;
        return Conflict{};
    }
    const auto &capture = std::get<CaptureView>(selected);
    const auto before = space->remaining();
    if (!charge_capture(*space, capture))
        return CapacityHold{};
    if (!typed_child(capture, event, named))
        return Conflict{};
    try {
        static_cast<void>(select_capture(capture));
    } catch (const RuleDefinitionFailure &) {
        return Conflict{};
    }

    AdmittedEvent candidate{event, depth, ordinal};
    std::optional<Cutoff> cutoff;
    if (reason)
        cutoff.emplace(Cutoff{event.chain, chain.history.root.event.capture, candidate,
                              chain.limits, *reason});
    std::optional<OccurrenceDisposition> release;
    if (!named) {
        release.emplace(OccurrenceDisposition{std::get<MilestoneOccurrence>(*event.cause),
                                              *event.parent, candidate});
        work.draft.occurrences.reserve(work.draft.occurrences.size() + 1);
    }
    work.draft.children.push_back(candidate);
    if (release)
        work.draft.occurrences.push_back(std::move(*release));
    work.draft.cutoff = std::move(cutoff);
    work.reserved_capture_bytes = before - space->remaining();
    work.child_capture_bytes += work.reserved_capture_bytes;
    work.awaiting_named = named != nullptr;
    return Reserved{std::move(candidate)};
}

ChildDecision offer_named(Context &chain, Work &work, const EventBinding &child,
                          const RuleActionKey &action, const RuleNamedEvent &bound) {
    if (work.finished || work.block || !work.capture || !valid(action) || !child_of(work, child))
        return Conflict{};
    const auto *cause = std::get_if<NamedAction>(&*child.cause);
    const auto *selected = selected_action(*work.capture, work.selected, action);
    const auto *named = selected ? std::get_if<RuleNamedEvent>(&selected->payload) : nullptr;
    if (!cause || cause->key != action || !named || !exact(*named, bound))
        return Conflict{};

    const auto known = lookup(chain, child);
    if (const auto *duplicate = std::get_if<Duplicate>(&known))
        return *duplicate;
    if (!std::holds_alternative<NotRecorded>(known) || terminal_binding_claimed(chain, work, child))
        return Conflict{};
    for (const auto &event : work.draft.children) {
        if (same_child_claim(event.binding, child))
            return event.binding == child &&
                           !(work.awaiting_named && &event == &work.draft.children.back())
                       ? ChildDecision{Duplicate{event}}
                       : ChildDecision{Conflict{}};
    }
    if (work.awaiting_named)
        return Conflict{};
    if (work.rollback_only)
        return CapacityHold{};
    const auto *cutoff = current_cutoff(chain, work);
    if (generation_suppressed(chain, cutoff)) {
        auto space = growth(chain, work);
        if (!space || !space->take(sizeof(NamedSuppression)) || !space->payloads(action) ||
            (cutoff && !space->payloads(*cutoff))) {
            work.rollback_only = true;
            return CapacityHold{};
        }
        auto stop = generation_stop(cutoff);
        work.draft.named_suppressions.push_back({action, stop});
        return GenerationSuppressed{std::move(stop)};
    }
    auto decision = reserve_child(chain, work, child, &bound);
    if (std::holds_alternative<CapacityHold>(decision))
        work.rollback_only = true;
    return decision;
}

NamedConfirmation confirm_named(Work &work, const AdmittedEvent &event,
                                const NamedOutcome &outcome) {
    if (work.finished || work.block || !work.capture)
        return NamedConfirmation::invalid_owner_result;
    if (!work.awaiting_named || work.draft.children.back() != event) {
        work.block = BlockReason::conflict;
        return NamedConfirmation::invalid_owner_result;
    }
    const auto *result = std::get_if<RuleOwnerResult>(&outcome);
    if (!result) {
        work.block = BlockReason::unresolved;
        return NamedConfirmation::unresolved;
    }
    if (const auto *accepted = std::get_if<RuleOwnerAcceptance>(result)) {
        const auto *observed = std::get_if<RuleEventLineage>(&accepted->observed);
        if (!observed || !matches(*observed, event.binding)) {
            work.block = BlockReason::conflict;
            return NamedConfirmation::invalid_owner_result;
        }
        work.awaiting_named = false;
        return NamedConfirmation::verified;
    }

    work.awaiting_named = false;
    work.draft.children.pop_back();
    work.draft.cutoff.reset();
    work.child_capture_bytes -= work.reserved_capture_bytes;
    work.reserved_capture_bytes = 0;
    if (std::holds_alternative<RulePassFailure>(*result)) {
        work.rollback_only = true;
        return NamedConfirmation::numeric_hold;
    }
    return NamedConfirmation::known_refusal;
}

} // namespace

ChildDecision offer_named_child(Unit &unit, const EventBinding &child, const RuleActionKey &action,
                                const RuleNamedEvent &bound) {
    const auto [chain, work] = ChainOperations::lock(unit);
    return chain && work ? offer_named(*chain, *work, child, action, bound)
                         : ChildDecision{Conflict{}};
}

NamedConfirmation confirm_named_child(Unit &unit, const AdmittedEvent &event,
                                      const NamedOutcome &outcome) {
    const auto [chain, work] = ChainOperations::lock(unit);
    return chain && work ? confirm_named(*work, event, outcome)
                         : NamedConfirmation::invalid_owner_result;
}

RuleActionSubmit chain_action_submit(Unit &unit, NamedProposal propose, NamedSubmit submit,
                                     RuleActionSubmit other) {
    const auto [context, active] = ChainOperations::lock(unit);
    return [weak_chain = std::weak_ptr{context}, weak_work = std::weak_ptr{active},
            propose = std::move(propose), submit = std::move(submit), other = std::move(other)](
               const RuleEventLineage &event, const RuleActionKey &action,
               const RuleBoundAction &bound, std::string_view snapshot) -> RuleOwnerResult {
        const auto chain = weak_chain.lock();
        const auto work = weak_work.lock();
        if (!chain || !work || work->finished || work->block || !work->capture)
            return RuleOwnerRefusal{"generated_event_unit_held"};
        if (work->rollback_only)
            return RuleOwnerRefusal{capacity_reason};
        const auto &anchor =
            find_event(chain->history.events, work->draft.identity.anchor)->binding;
        if (!matches(event, anchor) || !selected_action(*work->capture, work->selected, action)) {
            work->block = BlockReason::conflict;
            return RuleOwnerRefusal{"invalid_generated_event_binding"};
        }

        const auto *named = std::get_if<RuleNamedEvent>(&bound.payload);
        if (!named) {
            if (!other)
                return RuleOwnerRefusal{"action_owner_unavailable"};
            auto result = other(event, action, bound, snapshot);
            if (std::holds_alternative<RulePassFailure>(result))
                work->rollback_only = true;
            return result;
        }
        if (!propose || !submit)
            return RuleOwnerRefusal{"generation_owner_unavailable"};

        const auto decision = offer_named(*chain, *work, propose(action, *named), action, *named);
        if (const auto *held = std::get_if<GenerationSuppressed>(&decision))
            return RuleOwnerRefusal{stop_reason(held->stop)};
        if (std::holds_alternative<CapacityHold>(decision))
            return RuleOwnerRefusal{capacity_reason};
        if (const auto *duplicate = std::get_if<Duplicate>(&decision))
            return RuleOwnerAcceptance{lineage(duplicate->event.binding)};
        const auto *reserved = std::get_if<Reserved>(&decision);
        if (!reserved) {
            if (work->block == BlockReason::unresolved)
                return RuleOwnerRefusal{"generated_event_capture_unavailable"};
            work->block = BlockReason::conflict;
            return RuleOwnerRefusal{"invalid_generated_event_binding"};
        }

        auto outcome = submit(reserved->event, *named, snapshot);
        switch (confirm_named(*work, reserved->event, outcome)) {
        case NamedConfirmation::verified:
        case NamedConfirmation::known_refusal:
        case NamedConfirmation::numeric_hold:
            return std::get<RuleOwnerResult>(std::move(outcome));
        case NamedConfirmation::unresolved:
            return RuleOwnerRefusal{"generated_event_outcome_unknown"};
        case NamedConfirmation::invalid_owner_result:
            return RuleOwnerRefusal{"invalid_generated_event_observation"};
        }
        throw std::logic_error("unexpected named confirmation");
    };
}

ChildDecision offer_occurrence_child(Unit &unit, const MilestoneOccurrence &occurrence,
                                     const std::optional<EventBinding> &binding,
                                     OccurrenceState state) {
    const auto [chain, work] = ChainOperations::lock(unit);
    if (!chain || !work || work->finished || work->block ||
        work->draft.identity.purpose != UnitPurpose::released_occurrences)
        return Conflict{}; // A wrong-purpose offer leaves the event unit finishable.
    if (!valid(occurrence) || binding.has_value() != (state == OccurrenceState::released) ||
        (binding && (!child_of(*work, *binding) || !has_cause(*binding, occurrence))))
        return Conflict{};

    const auto &parent = work->draft.identity.anchor;
    if (const auto *known = recorded_occurrence(*chain, work.get(), occurrence)) {
        if (known->occurrence != occurrence || known->parent != parent)
            return Conflict{};
        if (state == OccurrenceState::skipped &&
            std::holds_alternative<Skipped>(known->disposition))
            return Skipped{};
        if (binding) {
            if (const auto *event = std::get_if<AdmittedEvent>(&known->disposition);
                event && event->binding == *binding)
                return Duplicate{*event};
            if (known->suppressed_binding == binding)
                return std::get<GenerationSuppressed>(known->disposition);
        }
        return Conflict{};
    }

    if (state == OccurrenceState::awaiting_release)
        return AwaitingRelease{};
    if (state == OccurrenceState::skipped) {
        auto space = growth(*chain, *work);
        if (!space || !space->take(sizeof(OccurrenceDisposition)) ||
            !space->payloads(occurrence, parent))
            return CapacityHold{};
        work->draft.occurrences.push_back({occurrence, parent, Skipped{}});
        return Skipped{};
    }
    if (!binding || !std::holds_alternative<NotRecorded>(lookup(*chain, *binding)) ||
        terminal_binding_claimed(*chain, *work, *binding) ||
        std::ranges::any_of(work->draft.children, [&](const AdmittedEvent &event) {
            return same_child_claim(event.binding, *binding);
        }))
        return Conflict{};

    const auto *cutoff = current_cutoff(*chain, *work);
    if (generation_suppressed(*chain, cutoff)) {
        auto space = growth(*chain, *work);
        if (!space || !space->take(sizeof(OccurrenceDisposition)) ||
            !space->payloads(occurrence, parent, *binding) || (cutoff && !space->payloads(*cutoff)))
            return CapacityHold{};
        GenerationSuppressed result{generation_stop(cutoff)};
        work->draft.occurrences.push_back({occurrence, parent, result, binding});
        return result;
    }
    return reserve_child(*chain, *work, *binding, nullptr);
}

namespace {

UnitResult held(BlockReason reason) {
    return reason == BlockReason::unresolved ? UnitResult{Unresolved{}} : UnitResult{Conflict{}};
}

UnitResult block(Work &work, BlockReason reason) {
    work.block = reason;
    return held(reason);
}

UnitResult rollback(Context &chain) {
    chain.current.reset();
    chain.history.blocked.reset();
    return RolledBack{};
}

UnitResult commit(Context &chain, Work &work, std::size_t bytes) {
    auto &pub = work.draft;
    // Materialize every copy before changing the retained projection. Vector
    // growth and copy failures therefore leave the prior commit untouched.
    Publication retained = pub;
    std::vector<AdmittedEvent> children = pub.children;
    std::optional<Cutoff> cutoff = pub.cutoff;
    auto &history = chain.history;
    history.events.reserve(history.events.size() + children.size());
    history.publications.reserve(history.publications.size() + 1);
    for (auto &child : children)
        history.events.push_back(std::move(child));
    if (cutoff)
        history.cutoff = std::move(cutoff);
    history.publications.push_back(std::move(retained));
    chain.pending += pub.children.size();
    if (pub.pass)
        --chain.pending;
    chain.retained_bytes += bytes;
    chain.current.reset();
    return std::move(pub);
}

// An ordinary uncertain result must equal the prepared publication. A named
// owner's fault can replace its refusal only with exact committed evidence
// for the disputed reservation. The previously observed effects stay exact.
bool readback_matches(const Work &work, const Publication &readback) {
    const auto &draft = work.draft;
    if (work.restored)
        return true; // A complete trusted owner view is the only evidence after modeled restore.
    if (work.rollback_only || draft.identity != readback.identity ||
        !exact(draft.children, readback.children) || !exact(draft.cutoff, readback.cutoff) ||
        !exact(draft.occurrences, readback.occurrences) ||
        !exact(draft.named_suppressions, readback.named_suppressions))
        return false;
    if (!work.awaiting_named)
        return exact(draft.pass, readback.pass);
    if (!draft.pass || !readback.pass || draft.pass->rules.size() != readback.pass->rules.size())
        return false;

    const auto &candidate = draft.children.back().binding;
    const auto &key = std::get<NamedAction>(*candidate.cause).key;
    bool recovered = false;
    for (std::size_t i = 0; i < draft.pass->rules.size(); ++i) {
        const auto &known = draft.pass->rules[i];
        const auto &selected = readback.pass->rules[i];
        if (known.stop && known.stop->position == key.position &&
            known.captured_index == selected.captured_index &&
            selected.accepted.size() == known.accepted.size() + 1 &&
            selected.accepted.back().key == key) {
            const auto *observed =
                std::get_if<RuleEventLineage>(&selected.accepted.back().observed);
            if (recovered || !observed || !matches(*observed, candidate) ||
                !std::ranges::equal(known.accepted,
                                    std::span{selected.accepted}.first(known.accepted.size()),
                                    [](const RuleAcceptedAction &a, const RuleAcceptedAction &b) {
                                        return exact(a, b);
                                    }))
                return false;
            recovered = true;
        } else if (!exact(known, selected)) {
            return false;
        }
    }
    return recovered;
}

} // namespace

UnitResult finish_unit(Unit &&unit, const std::optional<RuleStagedPass> &staged,
                       RuleCompletion completion) try {
    const auto [chain, work] = ChainOperations::lock(unit);
    if (!chain || !work)
        return Conflict{};
    if (work->finished)
        return work->block ? held(*work->block) : UnitResult{Conflict{}};

    work->finished = true;
    const bool uncertain = completion == RuleCompletion::outcome_unknown ||
                           completion == RuleCompletion::abnormal_completion;
    if (!work->block &&
        (completion == RuleCompletion::rolled_back || (work->rollback_only && !uncertain)))
        return rollback(*chain);
    if (work->rollback_only && uncertain && !work->block)
        return block(*work, BlockReason::unresolved);
    // Preflight the report before copying it. Contested reservations stay
    // held even if its report cannot be retained within the available space.
    if (staged && work->capture) {
        auto space = growth(*chain, *work);
        if (!space || !space->payloads(*staged)) {
            if (work->block || uncertain)
                return block(*work, work->block.value_or(BlockReason::unresolved));
            return rollback(*chain);
        }
        work->draft.pass = publish_rule_pass(*staged, RuleCompletion::committed);
    }
    if (work->block)
        return held(*work->block);
    if (work->awaiting_named)
        return block(*work, BlockReason::unresolved);
    if (work->capture.has_value() != work->draft.pass.has_value() || (!work->capture && staged))
        return block(*work, BlockReason::conflict);
    if (work->capture &&
        !valid_pass(work->draft, *work->capture, work->selected, current_cutoff(*chain, *work)))
        return block(*work, BlockReason::conflict);
    if (uncertain)
        return block(*work, BlockReason::unresolved);
    if (completion != RuleCompletion::committed)
        return block(*work, BlockReason::conflict);

    const auto other = other_use(*chain);
    auto space = capacity(chain->runtime, other, chain->pending + work->draft.children.size());
    if (!space || !space->take(chain->retained_bytes))
        return rollback(*chain);
    const auto before = space->remaining();
    if (!space->entry(work->draft) || !space->take(work->child_capture_bytes) ||
        (work->draft.cutoff && !space->payloads(*work->draft.cutoff)) ||
        !std::ranges::all_of(work->draft.children,
                             [&](const AdmittedEvent &event) { return space->entry(event); }))
        return rollback(*chain);
    return commit(*chain, *work, before - space->remaining());
} catch (...) {
    const auto [chain, work] = ChainOperations::lock(unit);
    if (work && !work->block)
        work->block = BlockReason::unresolved;
    throw;
}

UnitResult reconcile_unit(Chain &handle, const std::string &unit, const UnitReadback &readback) {
    auto &chain = ChainOperations::get(handle);
    const auto &work = chain.current;
    if (!work || !work->block || !work->finished || work->draft.identity.unit != unit)
        return Conflict{};
    if (const auto *identity = std::get_if<UnitIdentity>(&readback))
        return *identity == work->draft.identity ? rollback(chain) : UnitResult{Conflict{}};
    const auto *history = std::get_if<ModeledHistory>(&readback);
    if (!history)
        return held(*work->block);
    if (history->root != chain.history.root || history->admission != chain.history.admission ||
        !exact(history->admitted_profile, chain.history.admitted_profile))
        return Conflict{};
    const auto prefix = publications(chain);
    if (history->publications.size() <= prefix.size())
        return held(*work->block);
    if (history->publications.size() - prefix.size() != 1 || history->blocked)
        return Conflict{};

    auto selected = capture_event(chain.owner, history->root.event);
    if (const auto *issue = std::get_if<EvidenceIssue>(&selected))
        return *issue == EvidenceIssue::conflict ? UnitResult{Conflict{}} : held(*work->block);
    const auto &capture = std::get<CaptureView>(selected);
    if (!exact(capture.configuration.get().data().generation, chain.limits))
        return Conflict{};
    const auto checked = check_history(*history, chain.owner, chain.runtime, chain.limits, capture);
    if (const auto *issue = std::get_if<EvidenceIssue>(&checked))
        return *issue == EvidenceIssue::conflict ? UnitResult{Conflict{}} : held(*work->block);
    if (!std::ranges::equal(
            prefix, std::span{history->publications}.first(prefix.size()),
            [](const Publication &a, const Publication &b) { return exact(a, b); }) ||
        history->events.size() < chain.history.events.size() ||
        !std::ranges::equal(chain.history.events,
                            std::span{history->events}.first(chain.history.events.size())))
        return Conflict{};

    const auto &outcome = history->publications.back();
    if (outcome.identity != work->draft.identity || !readback_matches(*work, outcome))
        return Conflict{};
    Publication result = outcome;
    auto committed = *history;
    chain.history = std::move(committed);
    const auto [bytes, pending] = std::get<std::pair<std::size_t, std::size_t>>(checked);
    chain.retained_bytes = bytes;
    chain.pending = pending;
    chain.current.reset();
    return result;
}

} // namespace subathon::core::rule_chain
