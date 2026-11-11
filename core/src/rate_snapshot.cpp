// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <numeric>
#include <subathon/core/rate_snapshot.hpp>

namespace subathon::core {
namespace {
using boost::multiprecision::cpp_int;
using Error = RateSnapshotError;
constexpr std::size_t pair_bits = 8192;
constexpr std::size_t temporary_bits = 16384;

[[noreturn]] void fail(Error error) { throw RateSnapshotFailure(error); }

void add_bounded(std::size_t &total, std::size_t amount, std::size_t ceiling) {
    if (amount > ceiling || total > ceiling - amount)
        fail(Error::resource_limit);
    total += amount;
}

void validate_options(const RateSnapshotLimits &limits) {
    constexpr RateSnapshotLimits maximum;
    const auto check = [](std::size_t value, std::size_t ceiling) {
        if (value == 0 || value > ceiling)
            fail(Error::invalid_options);
    };
    check(limits.currency_bytes, maximum.currency_bytes);
    check(limits.reference_bytes, maximum.reference_bytes);
    check(limits.raw_time_bytes, maximum.raw_time_bytes);
    check(limits.reason_bytes, maximum.reason_bytes);
    check(limits.observations, maximum.observations);
    check(limits.quotes_per_observation, maximum.quotes_per_observation);
    check(limits.total_quotes, maximum.total_quotes);
    check(limits.overrides, maximum.overrides);
    check(limits.constraints, maximum.constraints);
    check(limits.currencies, maximum.currencies);
    check(limits.items, maximum.items);
    check(limits.owned_bytes, maximum.owned_bytes);
    check(limits.potential_component_bits, maximum.potential_component_bits);
}

// This is the contract's semantic byte/item metric, not an allocator budget.
struct Measure {
    const RateSnapshotLimits &limits;
    std::size_t bytes{};
    std::size_t items{};

    void add_bytes(std::size_t count) { add_bounded(bytes, count, limits.owned_bytes); }
    void collection(std::size_t count) { add_bounded(items, count, limits.items); }
    void text(std::string_view value, std::size_t ceiling) {
        if (value.size() > ceiling)
            fail(Error::resource_limit);
        add_bytes(value.size());
    }
    void optional_text(const std::optional<std::string> &value, std::size_t ceiling) {
        if (value)
            text(*value, ceiling);
    }
    void time(const RateTimeEvidence &value) {
        optional_text(value.original_text, limits.raw_time_bytes);
        if (value.utc_ms)
            add_bytes(8);
    }
    void context(const RateObservationContext &value) {
        text(value.observation_ref, limits.reference_bytes);
        text(value.provider_ref, limits.reference_bytes);
        text(value.configuration_ref, limits.reference_bytes);
        text(value.request_ref, limits.reference_bytes);
        text(value.source_evidence_ref, limits.reference_bytes);
        optional_text(value.provider_snapshot_ref, limits.reference_bytes);
        if (value.fetched_at_utc_ms)
            add_bytes(8);
        time(value.source_rate_time);
        time(value.publication_time);
    }
    void observation(const FetchedRateObservationInput &value) {
        context(value.context);
        for (const auto &quote : value.quotes) {
            text(quote.from_currency, limits.currency_bytes);
            text(quote.to_currency, limits.currency_bytes);
            text(quote.original_decimal, maximum_decimal_bytes);
        }
    }
    void exact(const Exact &value) {
        // Each serialized component comes from an already bounded parse/ratio.
        add_bytes(boost::multiprecision::numerator(value).str().size());
        add_bytes(boost::multiprecision::denominator(value).str().size());
    }
};

void quote_count(const FetchedRateObservationInput &input, Measure &measure, std::size_t &total) {
    if (input.quotes.size() > measure.limits.quotes_per_observation)
        fail(Error::resource_limit);
    add_bounded(total, input.quotes.size(), measure.limits.total_quotes);
    measure.collection(input.quotes.size());
}

Measure preflight(const FetchedRateObservationInput &input, const RateSnapshotLimits &limits) {
    Measure result{limits};
    std::size_t quotes = 0;
    quote_count(input, result, quotes);
    result.observation(input);
    return result;
}

// All recursive vector lengths precede every string/number scan. These passes do
// not copy input, parse a number, sort, or allocate according to an input count.
Measure preflight(const EffectiveRateSnapshotInput &input, const RateSnapshotLimits &limits,
                  std::optional<std::pair<std::string_view, std::string_view>> query) {
    Measure result{limits};
    if (input.sources.size() > limits.observations)
        fail(Error::resource_limit);
    result.collection(input.sources.size());
    std::size_t quotes = 0;
    for (const auto &source : input.sources)
        quote_count(source, result, quotes);
    if (input.overrides.size() > limits.overrides)
        fail(Error::resource_limit);
    result.collection(input.overrides.size());
    if (input.constraints.size() > limits.constraints)
        fail(Error::resource_limit);
    result.collection(input.constraints.size());
    result.collection(input.components.size());
    std::size_t members = 0;
    for (const auto &component : input.components) {
        add_bounded(members, component.members.size(), limits.currencies);
        result.collection(component.members.size());
    }

    result.text(input.snapshot_ref, limits.reference_bytes);
    result.optional_text(input.parent_snapshot_ref, limits.reference_bytes);
    if (input.constructed_at_utc_ms)
        result.add_bytes(8);
    for (const auto &source : input.sources)
        result.observation(source);
    for (const auto &manual : input.overrides) {
        result.text(manual.override_ref, limits.reference_bytes);
        result.add_bytes(1);
        result.text(manual.from_currency, limits.currency_bytes);
        result.text(manual.to_currency, limits.currency_bytes);
        result.text(manual.original_decimal, maximum_decimal_bytes);
        result.text(manual.decision_ref, limits.reference_bytes);
        result.optional_text(manual.actor_ref, limits.reference_bytes);
        result.text(manual.reason, limits.reason_bytes);
        if (manual.recorded_at_utc_ms)
            result.add_bytes(8);
    }
    for (const auto &constraint : input.constraints) {
        result.add_bytes(1);
        if (const auto *fetched = std::get_if<FetchedRateConstraint>(&constraint)) {
            result.text(fetched->observation_ref, limits.reference_bytes);
            result.add_bytes(8);
        } else if (const auto *manual = std::get_if<ManualRateConstraint>(&constraint)) {
            result.text(manual->override_ref, limits.reference_bytes);
        }
        // A valueless variant has no child and is rejected in lexical phase 3.
    }
    for (const auto &component : input.components) {
        for (const auto &member : component.members) {
            result.text(member.currency, limits.currency_bytes);
            result.text(member.numerator, 1234);
            result.text(member.denominator, 1234);
        }
    }
    if (query && (query->first.size() > limits.currency_bytes ||
                  query->second.size() > limits.currency_bytes))
        fail(Error::resource_limit);
    return result;
}

void reference(std::string_view value) {
    if (value.empty())
        fail(Error::invalid_reference);
}
void reference(const std::optional<std::string> &value) {
    if (value)
        reference(std::string_view(*value));
}
void currency(std::string_view value) {
    if (value.empty())
        fail(Error::invalid_currency);
}

ExactDecimal decimal(std::string_view text, bool positive) {
    // Distinguish parser syntax/resources from the rate domain, without messages.
    auto parsed = [&] {
        try {
            return parse_decimal(text);
        } catch (const std::length_error &) {
            fail(Error::resource_limit);
        } catch (const std::invalid_argument &) {
            fail(Error::invalid_decimal);
        }
    }();
    if (positive && parsed.value() <= 0)
        fail(Error::nonpositive_rate);
    return parsed;
}

std::size_t bits(const cpp_int &value) {
    return value == 0 ? 0 : static_cast<std::size_t>(boost::multiprecision::msb(value)) + 1;
}

cpp_int positive_integer(std::string_view text, std::size_t ceiling) {
    if (text.empty() || text.front() < '1' || text.front() > '9' ||
        !std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; }))
        fail(Error::invalid_potential);
    cpp_int result = 0;
    for (const char digit : text) {
        // Lexical preflight bounds construction to 1234 digits (at most 4100
        // bits). Stop as soon as the positive prefix exceeds its selected bound.
        result *= 10;
        result += static_cast<unsigned>(digit - '0');
        if (bits(result) > ceiling)
            fail(Error::resource_limit);
    }
    return result;
}

cpp_int gcd(cpp_int left, cpp_int right) {
    while (right != 0) {
        cpp_int remainder = left % right;
        left = std::move(right);
        right = std::move(remainder);
    }
    return left;
}

Exact potential(const RatePotentialInput &value, const RateSnapshotLimits &limits) {
    auto numerator = positive_integer(value.numerator, limits.potential_component_bits);
    auto denominator = positive_integer(value.denominator, limits.potential_component_bits);
    if (gcd(numerator, denominator) != 1)
        fail(Error::invalid_potential);
    Exact result(numerator);
    result /= denominator;
    return result;
}

void lexical(const FetchedRateObservationInput &input) {
    const auto &context = input.context;
    reference(std::string_view(context.observation_ref));
    reference(std::string_view(context.provider_ref));
    reference(std::string_view(context.configuration_ref));
    reference(std::string_view(context.request_ref));
    reference(std::string_view(context.source_evidence_ref));
    reference(context.provider_snapshot_ref);
    for (const auto &quote : input.quotes) {
        currency(quote.from_currency);
        currency(quote.to_currency);
        static_cast<void>(decimal(quote.original_decimal, true));
    }
}

void lexical(const EffectiveRateSnapshotInput &input, const RateSnapshotLimits &limits,
             std::optional<std::pair<std::string_view, std::string_view>> query) {
    reference(std::string_view(input.snapshot_ref));
    reference(input.parent_snapshot_ref);
    for (const auto &source : input.sources)
        lexical(source);
    for (const auto &manual : input.overrides) {
        reference(std::string_view(manual.override_ref));
        if (manual.kind != RateOverrideKind::temporary && manual.kind != RateOverrideKind::pinned)
            fail(Error::invalid_override);
        currency(manual.from_currency);
        currency(manual.to_currency);
        if (manual.from_currency == manual.to_currency)
            fail(Error::invalid_override);
        static_cast<void>(decimal(manual.original_decimal, true));
        reference(std::string_view(manual.decision_ref));
        reference(manual.actor_ref);
        if (manual.reason.empty())
            fail(Error::invalid_override);
    }
    for (const auto &constraint : input.constraints) {
        if (const auto *fetched = std::get_if<FetchedRateConstraint>(&constraint))
            reference(std::string_view(fetched->observation_ref));
        else if (const auto *manual = std::get_if<ManualRateConstraint>(&constraint))
            reference(std::string_view(manual->override_ref));
        else
            fail(Error::unresolved_basis);
    }
    for (const auto &component : input.components) {
        for (const auto &member : component.members) {
            currency(member.currency);
            static_cast<void>(potential(member, limits));
        }
    }
    if (query) {
        currency(query->first);
        currency(query->second);
    }
}

bool less_bytes(std::string_view left, std::string_view right) {
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end(),
                                        [](unsigned char a, unsigned char b) { return a < b; });
}

void quote_duplicates(const FetchedRateObservationInput &input) {
    std::vector<const RateQuoteInput *> quotes;
    quotes.reserve(input.quotes.size());
    for (const auto &quote : input.quotes)
        quotes.push_back(&quote);
    std::sort(quotes.begin(), quotes.end(), [](const auto *left, const auto *right) {
        if (left->from_currency != right->from_currency)
            return less_bytes(left->from_currency, right->from_currency);
        return less_bytes(left->to_currency, right->to_currency);
    });
    for (std::size_t index = 1; index < quotes.size(); ++index) {
        if (quotes[index - 1]->from_currency == quotes[index]->from_currency &&
            quotes[index - 1]->to_currency == quotes[index]->to_currency)
            fail(Error::duplicate_quote);
    }
}

bool constraint_less(const RateConstraint *left, const RateConstraint *right) {
    if (left->index() != right->index())
        return left->index() < right->index();
    if (const auto *a = std::get_if<FetchedRateConstraint>(left)) {
        const auto &b = std::get<FetchedRateConstraint>(*right);
        if (a->observation_ref != b.observation_ref)
            return less_bytes(a->observation_ref, b.observation_ref);
        return a->quote_ordinal < b.quote_ordinal;
    }
    return less_bytes(std::get<ManualRateConstraint>(*left).override_ref,
                      std::get<ManualRateConstraint>(*right).override_ref);
}

cpp_int product(const cpp_int &left, const cpp_int &right, std::size_t ceiling) {
    const auto left_bits = bits(left);
    const auto right_bits = bits(right);
    if (left_bits > ceiling || right_bits > ceiling - left_bits)
        fail(Error::resource_limit);
    cpp_int result = left * right;
    if (bits(result) > ceiling)
        fail(Error::resource_limit);
    return result;
}

Exact ratio(const RatePotentialInput &from, const RatePotentialInput &to,
            const RateSnapshotLimits &limits) {
    const auto from_weight = potential(from, limits);
    const auto to_weight = potential(to, limits);
    const auto numerator = product(boost::multiprecision::numerator(to_weight),
                                   boost::multiprecision::denominator(from_weight), pair_bits);
    const auto denominator = product(boost::multiprecision::denominator(to_weight),
                                     boost::multiprecision::numerator(from_weight), pair_bits);
    Exact result(numerator);
    result /= denominator;
    return result;
}

bool equal_rate(const Exact &left, const Exact &right) {
    const auto a = product(boost::multiprecision::numerator(left),
                           boost::multiprecision::denominator(right), temporary_bits);
    const auto b = product(boost::multiprecision::numerator(right),
                           boost::multiprecision::denominator(left), temporary_bits);
    return a == b;
}

// All scratch owns only indices/references, never a speculative snapshot or
// all-pairs matrix. The fixed maxima keep even simultaneous scratch well below
// 65536 records: 16 sources, 1024 overrides, 4096 constraints/edges, 256 members
// and parents, 1040 usage flags, plus at most 4096 temporary quote references.
struct SnapshotValidation {
    const EffectiveRateSnapshotInput &input;
    const RateSnapshotLimits &limits;
    Measure measure;
    std::vector<const FetchedRateObservationInput *> sources;
    std::vector<const ManualRateOverrideInput *> overrides;
    std::vector<const RateConstraint *> constraints;
    struct Member {
        const RatePotentialInput *input;
        std::size_t component;
    };
    std::vector<Member> members;

    SnapshotValidation(const EffectiveRateSnapshotInput &value, const RateSnapshotLimits &options,
                       std::optional<std::pair<std::string_view, std::string_view>> query)
        : input(value), limits(options), measure(preflight(value, options, query)) {
        lexical(input, limits, query);
        duplicates_and_order();
        shape();
        edges();
        project();
    }

    void duplicates_and_order() {
        // Identity namespaces are separate, and their category precedes quotes.
        for (std::size_t index = 0; index < input.sources.size(); ++index) {
            for (std::size_t prior = 0; prior < index; ++prior)
                if (input.sources[prior].context.observation_ref ==
                    input.sources[index].context.observation_ref)
                    fail(Error::duplicate_identity);
        }
        for (std::size_t index = 0; index < input.overrides.size(); ++index) {
            for (std::size_t prior = 0; prior < index; ++prior)
                if (input.overrides[prior].override_ref == input.overrides[index].override_ref)
                    fail(Error::duplicate_identity);
        }
        for (const auto &source : input.sources)
            quote_duplicates(source);
        constraints.reserve(input.constraints.size());
        for (const auto &constraint : input.constraints)
            constraints.push_back(&constraint);
        std::sort(constraints.begin(), constraints.end(), constraint_less);
        for (std::size_t index = 1; index < constraints.size(); ++index) {
            if (!constraint_less(constraints[index - 1], constraints[index]))
                fail(Error::duplicate_constraint);
        }
        sources.reserve(input.sources.size());
        for (const auto &source : input.sources)
            sources.push_back(&source);
        std::sort(sources.begin(), sources.end(), [](const auto *left, const auto *right) {
            return less_bytes(left->context.observation_ref, right->context.observation_ref);
        });
        overrides.reserve(input.overrides.size());
        for (const auto &manual : input.overrides)
            overrides.push_back(&manual);
        std::sort(overrides.begin(), overrides.end(), [](const auto *left, const auto *right) {
            return less_bytes(left->override_ref, right->override_ref);
        });
    }

    std::size_t member_index(std::string_view key) const {
        for (std::size_t index = 0; index < members.size(); ++index)
            if (members[index].input->currency == key)
                return index;
        return members.size();
    }

    void shape() {
        members.reserve(limits.currencies);
        std::string_view previous_anchor;
        for (std::size_t index = 0; index < input.components.size(); ++index) {
            const auto &component = input.components[index];
            if (component.members.empty())
                fail(Error::noncanonical_component);
            const auto &anchor = component.members.front();
            if ((index != 0 && !less_bytes(previous_anchor, anchor.currency)) ||
                anchor.numerator != "1" || anchor.denominator != "1")
                fail(Error::noncanonical_component);
            previous_anchor = anchor.currency;
            std::string_view previous_member;
            for (std::size_t position = 0; position < component.members.size(); ++position) {
                const auto &member = component.members[position];
                if ((position != 0 && !less_bytes(previous_member, member.currency)) ||
                    member_index(member.currency) != members.size())
                    fail(Error::noncanonical_component);
                members.push_back({&member, index});
                previous_member = member.currency;
            }
        }
    }

    void edges() const {
        struct Edge {
            std::size_t from;
            std::size_t to;
            std::string_view decimal;
        };
        std::vector<Edge> resolved;
        resolved.reserve(constraints.size());
        std::vector<bool> used_sources(sources.size());
        std::vector<bool> used_overrides(overrides.size());
        // Resolve ALL references before any edge equality check.
        for (const auto *constraint : constraints) {
            std::string_view from, to, text;
            if (const auto *fetched = std::get_if<FetchedRateConstraint>(constraint)) {
                const auto found = std::find_if(sources.begin(), sources.end(), [&](const auto *s) {
                    return s->context.observation_ref == fetched->observation_ref;
                });
                if (found == sources.end() || fetched->quote_ordinal == 0 ||
                    fetched->quote_ordinal > (*found)->quotes.size())
                    fail(Error::unresolved_basis);
                const auto &quote =
                    (*found)->quotes[static_cast<std::size_t>(fetched->quote_ordinal - 1)];
                from = quote.from_currency;
                to = quote.to_currency;
                text = quote.original_decimal;
                used_sources[static_cast<std::size_t>(found - sources.begin())] = true;
            } else {
                const auto &manual = std::get<ManualRateConstraint>(*constraint);
                const auto found =
                    std::find_if(overrides.begin(), overrides.end(), [&](const auto *m) {
                        return m->override_ref == manual.override_ref;
                    });
                if (found == overrides.end())
                    fail(Error::unresolved_basis);
                from = (*found)->from_currency;
                to = (*found)->to_currency;
                text = (*found)->original_decimal;
                used_overrides[static_cast<std::size_t>(found - overrides.begin())] = true;
            }
            const auto from_index = member_index(from);
            const auto to_index = member_index(to);
            if (from == to || from_index == members.size() || to_index == members.size() ||
                members[from_index].component != members[to_index].component)
                fail(Error::unresolved_basis);
            resolved.push_back({from_index, to_index, text});
        }
        for (const auto &edge : resolved) {
            const auto expected = ratio(*members[edge.from].input, *members[edge.to].input, limits);
            const auto supplied = decimal(edge.decimal, true);
            if (!equal_rate(expected, supplied.value()))
                fail(Error::contradictory_constraint);
        }
        if (std::find(used_overrides.begin(), used_overrides.end(), false) !=
                used_overrides.end() ||
            std::find(used_sources.begin(), used_sources.end(), false) != used_sources.end())
            fail(Error::unresolved_basis);

        std::vector<std::size_t> parents(members.size());
        std::iota(parents.begin(), parents.end(), std::size_t(0));
        const auto root = [&](std::size_t index) {
            while (parents[index] != index) {
                parents[index] = parents[parents[index]];
                index = parents[index];
            }
            return index;
        };
        for (const auto &edge : resolved)
            parents[root(edge.to)] = root(edge.from);
        std::size_t anchor = 0;
        for (std::size_t index = 0; index < members.size(); ++index) {
            if (index == 0 || members[index].component != members[index - 1].component)
                anchor = index;
            if (root(index) != root(anchor))
                fail(Error::unbacked_component);
        }
    }

    void project() {
        // Canonical potential strings have exactly the same bytes/items as
        // their Exact output. Only parsed decimals add fields to the raw metric.
        for (const auto *source : sources)
            for (const auto &quote : source->quotes)
                measure.exact(decimal(quote.original_decimal, true).value());
        for (const auto *manual : overrides)
            measure.exact(decimal(manual->original_decimal, true).value());
    }
};
} // namespace

RateSnapshotFailure::RateSnapshotFailure(RateSnapshotError code)
    : std::invalid_argument("Invalid rate snapshot input"), code_(code) {}

RateSnapshotNotImplemented::RateSnapshotNotImplemented()
    : std::logic_error("Rate snapshots are intentionally unimplemented; review tests first") {}

RateMoney make_rate_money(const ReceiptMoney &input, const RateSnapshotLimits &limits) {
    validate_options(limits);
    Measure measure{limits};
    measure.text(input.original_decimal, maximum_decimal_bytes);
    measure.text(input.currency, limits.currency_bytes);
    auto parsed = decimal(input.original_decimal, false);
    currency(input.currency);
    measure.exact(parsed.value());
    return RateMoney({input, parsed.value()});
}

FetchedRateObservation make_fetched_rate_observation(const FetchedRateObservationInput &input,
                                                     const RateSnapshotLimits &limits) {
    validate_options(limits);
    auto measure = preflight(input, limits);
    lexical(input);
    quote_duplicates(input);
    for (const auto &quote : input.quotes)
        measure.exact(decimal(quote.original_decimal, true).value());
    FetchedRateObservationData output{input.context, {}};
    output.quotes.reserve(input.quotes.size());
    for (const auto &quote : input.quotes)
        output.quotes.push_back(
            {quote.from_currency, quote.to_currency, decimal(quote.original_decimal, true)});
    return FetchedRateObservation(std::move(output));
}

EffectiveRateSnapshot make_effective_rate_snapshot(const EffectiveRateSnapshotInput &input,
                                                   const RateSnapshotLimits &limits) {
    validate_options(limits);
    const SnapshotValidation validated(input, limits, std::nullopt);
    EffectiveRateSnapshotData output{
        input.snapshot_ref, input.parent_snapshot_ref, input.constructed_at_utc_ms, {}, {}, {}, {}};
    output.sources.reserve(validated.sources.size());
    for (const auto *source : validated.sources)
        output.sources.push_back(make_fetched_rate_observation(*source, limits));
    output.overrides.reserve(validated.overrides.size());
    for (const auto *manual : validated.overrides)
        output.overrides.push_back({manual->override_ref, manual->kind, manual->from_currency,
                                    manual->to_currency, decimal(manual->original_decimal, true),
                                    manual->decision_ref, manual->actor_ref, manual->reason,
                                    manual->recorded_at_utc_ms});
    output.constraints.reserve(validated.constraints.size());
    for (const auto *constraint : validated.constraints)
        output.constraints.push_back(*constraint);
    output.components.reserve(input.components.size());
    for (const auto &component : input.components) {
        auto &owned = output.components.emplace_back();
        owned.members.reserve(component.members.size());
        for (const auto &member : component.members)
            owned.members.push_back({member.currency, potential(member, limits)});
    }
    return EffectiveRateSnapshot(std::move(output));
}

RatePairResult quote_effective_rate(const EffectiveRateSnapshotInput &input,
                                    std::string_view from_currency, std::string_view to_currency,
                                    const RateSnapshotLimits &limits) {
    validate_options(limits);
    const SnapshotValidation validated(input, limits, std::pair{from_currency, to_currency});
    Measure measure{limits};
    measure.add_bytes(1);
    if (from_currency == to_currency) {
        measure.exact(Exact(1));
        return RatePairResult({RatePairKind::identity, Exact(1), std::nullopt});
    }
    const auto from = validated.member_index(from_currency);
    const auto to = validated.member_index(to_currency);
    if (from == validated.members.size() || to == validated.members.size() ||
        validated.members[from].component != validated.members[to].component)
        return RatePairResult({RatePairKind::missing, std::nullopt, std::nullopt});
    auto value = ratio(*validated.members[from].input, *validated.members[to].input, limits);
    measure.exact(value);
    measure.text(input.snapshot_ref, limits.reference_bytes);
    measure.text(from_currency, limits.currency_bytes);
    measure.text(to_currency, limits.currency_bytes);
    return RatePairResult({RatePairKind::available, std::move(value),
                           RatePairProvenance{input.snapshot_ref, std::string(from_currency),
                                              std::string(to_currency)}});
}
} // namespace subathon::core
