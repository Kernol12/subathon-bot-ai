// SPDX-License-Identifier: GPL-3.0-or-later
#include <map>
#include <subathon/core/rate_override.hpp>

namespace subathon::core {
namespace {
using Error = RateSnapshotError;
constexpr RateSnapshotLimits limits;
constexpr auto pair_bits = 2 * limits.potential_component_bits;

[[noreturn]] void fail(Error error) { throw RateSnapshotFailure(error); }

void add_bounded(std::size_t &total, std::size_t amount, std::size_t ceiling) {
    if (amount > ceiling - total)
        fail(Error::resource_limit);
    total += amount;
}

// Count the proposal DTO as it grows, then continue the same budget for the preview.
// The snapshot factory separately projects the parsed snapshot's ownership.
struct ProposalBudget {
    std::size_t bytes{};
    std::size_t items{};

    void add_bytes(std::size_t count) { add_bounded(bytes, count, limits.owned_bytes); }
    void add_items(std::size_t count) { add_bounded(items, count, limits.items); }

    void text(std::string_view value, std::size_t ceiling) {
        if (value.size() > ceiling)
            fail(Error::resource_limit);
        add_bytes(value.size());
    }

    void optional_ref(const std::optional<std::string> &value) {
        if (value)
            text(*value, limits.reference_bytes);
    }

    void timestamp(const std::optional<std::int64_t> &value) {
        if (value)
            add_bytes(8);
    }

    void time_evidence(const RateTimeEvidence &value) {
        if (value.original_text)
            text(*value.original_text, limits.raw_time_bytes);
        timestamp(value.utc_ms);
    }

    void observation(const FetchedRateObservationData &source) {
        add_items(1 + source.quotes.size());
        const auto &context = source.context;
        text(context.observation_ref, limits.reference_bytes);
        text(context.provider_ref, limits.reference_bytes);
        text(context.configuration_ref, limits.reference_bytes);
        text(context.request_ref, limits.reference_bytes);
        text(context.source_evidence_ref, limits.reference_bytes);
        optional_ref(context.provider_snapshot_ref);
        timestamp(context.fetched_at_utc_ms);
        time_evidence(context.source_rate_time);
        time_evidence(context.publication_time);
        for (const auto &quote : source.quotes) {
            text(quote.from_currency, limits.currency_bytes);
            text(quote.to_currency, limits.currency_bytes);
            text(quote.rate.original(), maximum_decimal_bytes);
        }
    }

    void exact(const std::optional<Exact> &value) {
        if (value) {
            add_bytes(boost::multiprecision::numerator(*value).str().size());
            add_bytes(boost::multiprecision::denominator(*value).str().size());
        }
    }
};

void check_bits(const Exact &value, std::size_t ceiling) {
    if (boost::multiprecision::msb(boost::multiprecision::numerator(value)) >= ceiling ||
        boost::multiprecision::msb(boost::multiprecision::denominator(value)) >= ceiling)
        fail(Error::resource_limit);
}

ExactDecimal parse_edit_rate(std::string_view text) {
    auto parsed = [&] {
        try {
            return parse_decimal(text);
        } catch (const std::length_error &) {
            fail(Error::resource_limit);
        } catch (const std::invalid_argument &) {
            fail(Error::invalid_decimal);
        }
    }();
    if (parsed.value() <= 0)
        fail(Error::nonpositive_rate);
    return parsed;
}

// Keep a spanning forest and weights relative to each component's least bytewise
// currency. Redundant compatible constraints keep their evidence outside the forest.
class RateForest {
    struct Node;
    struct Arc {
        Node *neighbor;
        const Exact *rate;
        bool forward;
    };
    struct Node {
        std::string_view anchor;
        Exact weight{1};
        std::vector<Arc> neighbors;
    };
    std::map<std::string_view, Node> nodes_;

    Node &node(std::string_view key) {
        if (const auto existing = nodes_.find(key); existing != nodes_.end())
            return existing->second;
        if (nodes_.size() == limits.currencies)
            fail(Error::resource_limit);
        return nodes_.try_emplace(key, key, Exact(1), std::vector<Arc>{}).first->second;
    }

    static void rebase(Node &node, const Node *previous, std::string_view anchor, Exact weight) {
        // An intermediate anchor can change when two trees merge. Its relative
        // rates must fit the pair ceiling, while final weights must fit 4096 bits.
        check_bits(weight, pair_bits);
        node.anchor = anchor;
        node.weight = std::move(weight);
        for (const auto &arc : node.neighbors) {
            if (arc.neighbor == previous)
                continue;
            Exact next = node.weight;
            if (arc.forward)
                next *= *arc.rate;
            else
                next /= *arc.rate;
            rebase(*arc.neighbor, &node, anchor, std::move(next));
        }
    }

  public:
    bool accept(std::string_view from_key, std::string_view to_key, const Exact &rate) {
        auto &from = node(from_key);
        auto &to = node(to_key);
        if (from.anchor == to.anchor) {
            // Each weight has at most 8192 bits per component. Compare normalized
            // rationals after one quotient, whose products fit in 16384 bits.
            const Exact implied = to.weight / from.weight;
            return implied == rate;
        }

        // Only the tree losing its anchor needs new weights. Each step uses one
        // bounded source decimal, including when walking an edge in reverse.
        if (from.anchor < to.anchor)
            rebase(to, nullptr, from.anchor, from.weight * rate);
        else
            rebase(from, nullptr, to.anchor, to.weight / rate);
        from.neighbors.push_back({&to, &rate, true});
        to.neighbors.push_back({&from, &rate, false});
        return true;
    }

    std::vector<RateComponentInput> components(ProposalBudget &budget) const {
        std::vector<RateComponentInput> result;
        std::map<std::string_view, std::size_t> component_by_anchor;
        for (const auto &[currency, node] : nodes_) {
            check_bits(node.weight, limits.potential_component_bits);
            if (currency == node.anchor) {
                budget.add_items(1);
                component_by_anchor.emplace(currency, result.size());
                result.emplace_back();
            }
            auto numerator = boost::multiprecision::numerator(node.weight).str();
            auto denominator = boost::multiprecision::denominator(node.weight).str();
            budget.add_items(1);
            budget.text(currency, limits.currency_bytes);
            budget.add_bytes(numerator.size());
            budget.add_bytes(denominator.size());
            result[component_by_anchor.at(node.anchor)].members.push_back(
                {std::string(currency), std::move(numerator), std::move(denominator)});
        }
        return result;
    }
};

class Proposal {
    EffectiveRateSnapshotInput candidate_;
    ProposalBudget budget_;
    RateForest forest_;
    std::map<std::string_view, const FetchedRateObservationData *> selected_sources_;
    std::size_t total_quotes_{};

    void add_constraint(std::string_view ref, std::size_t extra_bytes) {
        if (candidate_.constraints.size() == limits.constraints)
            fail(Error::resource_limit);
        budget_.add_items(1);
        budget_.add_bytes(1 + extra_bytes);
        budget_.text(ref, limits.reference_bytes);
    }

    void keep_source(const FetchedRateObservationData &source) {
        const auto &ref = source.context.observation_ref;
        if (const auto existing = selected_sources_.find(ref);
            existing != selected_sources_.end()) {
            // Resolve every retained ordinal against the observation that supplied it.
            if (existing->second != &source)
                fail(Error::duplicate_identity);
            return;
        }
        if (selected_sources_.size() == limits.observations ||
            source.quotes.size() > limits.quotes_per_observation)
            fail(Error::resource_limit);
        add_bounded(total_quotes_, source.quotes.size(), limits.total_quotes);
        budget_.observation(source);
        auto &owned =
            candidate_.sources.emplace_back(source.context, std::vector<RateQuoteInput>{});
        owned.quotes.reserve(source.quotes.size());
        for (const auto &quote : source.quotes)
            owned.quotes.push_back({quote.from_currency, quote.to_currency, quote.rate.original()});
        selected_sources_.emplace(ref, &source);
    }

  public:
    Proposal(std::string_view current_ref, std::string_view candidate_ref) {
        budget_.text(candidate_ref, limits.reference_bytes);
        budget_.text(current_ref, limits.reference_bytes);
        // Reusing the head identity would leave old previews apparently current.
        if (candidate_ref.empty() || candidate_ref == current_ref)
            fail(Error::invalid_reference);
        candidate_.snapshot_ref = candidate_ref;
        candidate_.parent_snapshot_ref.emplace(current_ref);
    }

    // The validated and input override DTOs share their evidence fields.
    template <typename Manual> void keep_manual(const Manual &manual, const ExactDecimal &rate) {
        if (candidate_.overrides.size() == limits.overrides)
            fail(Error::resource_limit);
        budget_.add_items(1);
        budget_.text(manual.override_ref, limits.reference_bytes);
        budget_.add_bytes(1);
        budget_.text(manual.from_currency, limits.currency_bytes);
        budget_.text(manual.to_currency, limits.currency_bytes);
        budget_.text(rate.original(), maximum_decimal_bytes);
        budget_.text(manual.decision_ref, limits.reference_bytes);
        budget_.optional_ref(manual.actor_ref);
        budget_.text(manual.reason, limits.reason_bytes);
        budget_.timestamp(manual.recorded_at_utc_ms);
        if (manual.override_ref.empty() || manual.decision_ref.empty() ||
            (manual.actor_ref && manual.actor_ref->empty()))
            fail(Error::invalid_reference);
        if (manual.from_currency.empty() || manual.to_currency.empty())
            fail(Error::invalid_currency);
        if ((manual.kind != RateOverrideKind::temporary &&
             manual.kind != RateOverrideKind::pinned) ||
            manual.from_currency == manual.to_currency || manual.reason.empty())
            fail(Error::invalid_override);
        if (!forest_.accept(manual.from_currency, manual.to_currency, rate.value()))
            fail(Error::contradictory_constraint);

        add_constraint(manual.override_ref, 0);
        candidate_.overrides.push_back({manual.override_ref, manual.kind, manual.from_currency,
                                        manual.to_currency, rate.original(), manual.decision_ref,
                                        manual.actor_ref, manual.reason,
                                        manual.recorded_at_utc_ms});
        candidate_.constraints.emplace_back(ManualRateConstraint{manual.override_ref});
    }

    void select_fetched(std::span<const RateOverrideFetchedEdge> edges) {
        for (const auto &edge : edges) {
            const auto &source = edge.observation.get().data();
            if (edge.quote_ordinal == 0 || edge.quote_ordinal > source.quotes.size())
                fail(Error::unresolved_basis);
            const auto &quote = source.quotes[static_cast<std::size_t>(edge.quote_ordinal - 1)];
            if (quote.from_currency == quote.to_currency ||
                !forest_.accept(quote.from_currency, quote.to_currency, quote.rate.value()))
                continue;
            add_constraint(source.context.observation_ref, 8);
            keep_source(source);
            candidate_.constraints.emplace_back(
                FetchedRateConstraint{source.context.observation_ref, edge.quote_ordinal});
        }
        candidate_.components = forest_.components(budget_);
    }

    const EffectiveRateSnapshotInput &candidate() const { return candidate_; }
    EffectiveRateSnapshotInput take_candidate() { return std::move(candidate_); }
    ProposalBudget budget() const { return budget_; }
};

const ManualRateOverrideData *replacement(const EffectiveRateSnapshotData &current,
                                          std::string_view from, std::string_view to,
                                          std::optional<std::string_view> target) {
    for (const auto &active : current.overrides) {
        const bool same_pair = (active.from_currency == from && active.to_currency == to) ||
                               (active.from_currency == to && active.to_currency == from);
        if (same_pair) {
            if (!target)
                fail(Error::invalid_override);
            if (active.override_ref == *target)
                return &active;
        }
    }
    if (target)
        fail(Error::invalid_override);
    return nullptr;
}

// Expiry uses the accepted observation alone. Its full quote list supplies
// reachability even when cache policy selects different edges for the successor.
class FetchedReachability {
    std::map<std::string_view, std::size_t> indices_;
    std::vector<std::size_t> parents_;

    std::size_t index(std::string_view key) {
        const auto [entry, inserted] = indices_.try_emplace(key, parents_.size());
        if (inserted)
            parents_.push_back(entry->second);
        return entry->second;
    }

    std::size_t root(std::size_t member) {
        while (parents_[member] != member) {
            parents_[member] = parents_[parents_[member]];
            member = parents_[member];
        }
        return member;
    }

  public:
    explicit FetchedReachability(const FetchedRateObservation &observation) {
        for (const auto &quote : observation.data().quotes) {
            const auto from = index(quote.from_currency);
            const auto to = index(quote.to_currency);
            parents_[root(from)] = root(to);
        }
    }

    bool connects(std::string_view from, std::string_view to) {
        const auto left = indices_.find(from);
        const auto right = indices_.find(to);
        return left != indices_.end() && right != indices_.end() &&
               root(left->second) == root(right->second);
    }
};

struct PairMember {
    const RateComponent *component{};
    const Exact *weight{};
};
struct PreviewMember {
    PairMember before;
    PairMember after;
};

std::optional<Exact> pair_rate(const PairMember &from, const PairMember &to) {
    if (!from.component || from.component != to.component)
        return std::nullopt;
    return Exact(*to.weight / *from.weight);
}

std::vector<RatePairChange> affected_rates(const EffectiveRateSnapshotData &before,
                                           const EffectiveRateSnapshotData &after,
                                           ProposalBudget &budget) {
    std::map<std::string_view, PreviewMember> members;
    for (const auto &component : before.components)
        for (const auto &member : component.members)
            members[member.currency].before = {&component, &member.weight};
    for (const auto &component : after.components)
        for (const auto &member : component.members)
            members[member.currency].after = {&component, &member.weight};

    std::vector<RatePairChange> affected;
    for (const auto &[from, left] : members) {
        for (const auto &[to, right] : members) {
            if (from == to)
                continue; // The snapshot contract's arithmetic identity never changes.
            auto old_rate = pair_rate(left.before, right.before);
            auto new_rate = pair_rate(left.after, right.after);
            if (old_rate == new_rate)
                continue;
            budget.add_items(1);
            budget.text(from, limits.currency_bytes);
            budget.text(to, limits.currency_bytes);
            budget.exact(old_rate);
            budget.exact(new_rate);
            affected.push_back(
                {std::string(from), std::string(to), std::move(old_rate), std::move(new_rate)});
        }
    }
    return affected;
}

void check_edge_count(std::span<const RateOverrideFetchedEdge> edges) {
    if (edges.size() > limits.total_quotes)
        fail(Error::resource_limit);
}
} // namespace

RateOverridePreview preview_rate_override(const EffectiveRateSnapshot &current,
                                          const ManualRateOverrideInput &edit,
                                          std::span<const RateOverrideFetchedEdge> fetched_edges,
                                          std::string_view candidate_ref,
                                          std::optional<std::string_view> replace_override_ref) {
    check_edge_count(fetched_edges);
    const auto &base = current.data();
    const auto *replaced =
        replacement(base, edit.from_currency, edit.to_currency, replace_override_ref);
    const auto rate = parse_edit_rate(edit.original_decimal);

    Proposal proposal(base.snapshot_ref, candidate_ref);
    for (const auto &active : base.overrides)
        if (&active != replaced)
            proposal.keep_manual(active, active.rate);
    proposal.keep_manual(edit, rate);
    proposal.select_fetched(fetched_edges);
    const auto validated = make_effective_rate_snapshot(proposal.candidate());

    auto budget = proposal.budget();
    std::optional<std::string> replaced_ref;
    if (replaced) {
        budget.text(replaced->override_ref, limits.reference_bytes);
        replaced_ref = replaced->override_ref;
    }
    auto changes = affected_rates(base, validated.data(), budget);
    return {proposal.take_candidate(), std::move(changes), std::move(replaced_ref)};
}

RateOverrideRemovalProposal
preview_rate_override_removal(const EffectiveRateSnapshot &current, const RateOverridePair &pair,
                              std::string_view removed_override_ref,
                              std::span<const RateOverrideFetchedEdge> fetched_edges,
                              std::string_view candidate_ref) {
    check_edge_count(fetched_edges);
    if (pair.low_currency.size() > limits.currency_bytes ||
        pair.high_currency.size() > limits.currency_bytes)
        fail(Error::resource_limit);
    // std::string comparison orders the bytes as unsigned char.
    if (pair.low_currency.empty() || pair.low_currency >= pair.high_currency)
        fail(Error::invalid_currency);
    if (removed_override_ref.size() > limits.reference_bytes)
        fail(Error::resource_limit);
    if (removed_override_ref.empty())
        fail(Error::invalid_reference);

    const auto &base = current.data();
    Proposal proposal(base.snapshot_ref, candidate_ref);
    const auto *removed =
        replacement(base, pair.low_currency, pair.high_currency, removed_override_ref);
    for (const auto &active : base.overrides)
        if (&active != removed)
            proposal.keep_manual(active, active.rate);
    proposal.select_fetched(fetched_edges);
    const auto validated = make_effective_rate_snapshot(proposal.candidate());
    auto budget = proposal.budget();
    budget.text(removed_override_ref, limits.reference_bytes);
    auto changes = affected_rates(base, validated.data(), budget);
    return {proposal.take_candidate(), std::move(changes), std::string(removed_override_ref)};
}

RatePreviewBaseStatus check_rate_preview_base(const EffectiveRateSnapshot &current,
                                              std::string_view preview_base_ref) {
    return current.data().snapshot_ref == preview_base_ref ? RatePreviewBaseStatus::current
                                                           : RatePreviewBaseStatus::stale;
}

std::optional<EffectiveRateSnapshotInput> propose_rate_update(
    const EffectiveRateSnapshot &current, const FetchedRateObservation *accepted_observation,
    std::span<const RateOverrideFetchedEdge> fetched_edges, std::string_view candidate_ref) {
    if (!accepted_observation)
        return std::nullopt;
    check_edge_count(fetched_edges);

    const auto &base = current.data();
    Proposal proposal(base.snapshot_ref, candidate_ref);
    FetchedReachability fetched(*accepted_observation);
    for (const auto &active : base.overrides) {
        if (active.kind == RateOverrideKind::temporary &&
            fetched.connects(active.from_currency, active.to_currency))
            continue;
        proposal.keep_manual(active, active.rate);
    }
    proposal.select_fetched(fetched_edges);
    static_cast<void>(make_effective_rate_snapshot(proposal.candidate()));
    return proposal.take_candidate();
}

} // namespace subathon::core
