// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <bit>
#include <initializer_list>
#include <limits>
#include <map>
#include <subathon/core/modifier_stacking.hpp>
#include <tuple>
#include <type_traits>

namespace subathon::core::modifier_detail {
using Failure = ModifierStackFailure;
using Op = ModifierStackOperator;
using Integer = boost::multiprecision::cpp_int;

static_assert(2 * modifier_component_bits + 1 == modifier_temporary_bits);

inline constexpr ModifierStackLimits defaults{};

inline void require(bool condition, Failure failure = Failure::invalid) {
    if (!condition)
        throw failure;
}

inline void fit(std::size_t size, std::size_t limit) {
    require(size <= limit, Failure::resource_limit);
}

inline void debit(std::size_t &remaining, std::size_t amount) {
    fit(amount, remaining);
    remaining -= amount;
}

// Logical byte charge. Strings and sequences have a length field; numerics use
// decimal component text and fixed-width lengths. No allocator size enters the budget.
class Bytes {
  public:
    explicit Bytes(std::size_t limit) : remaining_(limit), limit_(limit) {}
    void add(std::size_t size) { debit(remaining_, size); }
    void text(std::string_view value) {
        add(8);
        add(value.size());
    }
    [[nodiscard]] std::size_t used() const { return limit_ - remaining_; }

  private:
    std::size_t remaining_;
    std::size_t limit_;
};

struct Budget {
    explicit Budget(const ModifierStackLimits &limits)
        : input(limits.input_bytes), output(limits.output_bytes), work_left(limits.operations),
          occurrences(std::min(limits.occurrences, defaults.occurrences)),
          policy_entries(std::min(limits.policy_entries, defaults.policy_entries)),
          targets(std::min(limits.targets, defaults.targets)),
          requests(std::min(limits.requests, defaults.requests)),
          rows(std::min(limits.materialized_rows, defaults.materialized_rows)),
          ref_bytes(std::min(limits.reference_bytes, defaults.reference_bytes)),
          value_bytes(std::min(limits.value_bytes, defaults.value_bytes)),
          component_bytes(std::min(limits.exact_component_bytes, defaults.exact_component_bytes)) {}

    void work(std::size_t amount = 1) { debit(work_left, amount); }
    template <class T, class U> bool equal(const T &left, const U &right) {
        work();
        return left == right;
    }
    void refs(std::initializer_list<std::string_view> values, bool required = true) {
        for (const auto value : values) {
            fit(value.size(), ref_bytes);
            require(!required || !value.empty());
            input.text(value);
        }
    }
    void value(std::string_view text) {
        fit(text.size(), value_bytes);
        input.text(text);
    }

    Bytes input;
    Bytes output;
    std::size_t work_left;
    std::size_t occurrences;
    std::size_t policy_entries;
    std::size_t targets;
    std::size_t requests;
    std::size_t rows;
    std::size_t ref_bytes;
    std::size_t value_bytes;
    std::size_t component_bytes;
};

struct MeteredLess {
    Budget *budget;
    template <class T> bool operator()(const T &left, const T &right) const {
        budget->work();
        return left < right;
    }
};

template <class Key, class Value> using Index = std::map<Key, Value, MeteredLess>;

using Target = std::tuple<ModifierTargetKind, std::string_view>;
using Key = std::tuple<std::string_view, std::string_view, std::string_view, std::size_t>;

inline Target view(const ModifierTargetKey &target) { return {target.kind, target.ref}; }
inline Key view(const ModifierOccurrenceKey &key) {
    return {key.instance_ref, key.instance_revision_ref, key.definition_revision_ref,
            key.target_ordinal};
}

inline void target_input(Budget &budget, Target target) {
    const auto [kind, ref] = target;
    require(kind == ModifierTargetKind::added_time || kind == ModifierTargetKind::milestone_score ||
            kind == ModifierTargetKind::counter);
    budget.refs({ref});
    budget.input.add(1);
}

inline void key_input(Budget &budget, Key key) {
    const auto [instance, revision, definition, ordinal] = key;
    budget.refs({instance, revision, definition});
    require(ordinal != 0);
    budget.input.add(8);
}

inline void factor_input(Budget &budget, std::string_view text) {
    budget.value(text);
    fit(text.size(), maximum_decimal_bytes);
}

// Borrow components before testing size. The multiprecision free functions
// numerator/denominator return owning copies even for a caller's oversized value.
template <class Backend> const auto &numerator_backend(const Backend &backend) {
    if constexpr (requires { backend.num(); })
        return backend.num();
    else
        return backend.data().numerator().backend();
}

template <class Backend> const auto &denominator_backend(const Backend &backend) {
    if constexpr (requires { backend.denom(); })
        return backend.denom();
    else
        return backend.data().denominator().backend();
}

template <class Component> void component_shape(const Component &component) {
    using Limb = std::remove_cvref_t<decltype(*component.limbs())>;
    constexpr unsigned width = std::numeric_limits<Limb>::digits;
    require(component.size() != 0);
    fit(component.size(), (modifier_component_bits + width - 1) / width);
    const auto high = component.limbs()[component.size() - 1];
    require(high != 0 || (component.size() == 1 && !component.sign()));
    const auto bits = (component.size() - 1) * width + std::bit_width(high);
    fit(bits, modifier_component_bits);
}

inline void exact_shape(const Exact &value) {
    component_shape(numerator_backend(value.backend()));
    const auto &denominator = denominator_backend(value.backend());
    component_shape(denominator);
    require(!denominator.sign() && denominator.limbs()[denominator.size() - 1] != 0);
}

inline std::size_t component_size(Budget &budget, const Integer &value) {
    budget.work();
    if (value == 0) {
        fit(1, budget.component_bytes);
        return 1;
    }
    const bool negative = value < 0;
    const Integer magnitude = negative ? Integer(-value) : value;
    // For 1..4096 bits, this lower bound on decimal digits is at most one off.
    const auto bits_minus_one = static_cast<unsigned>(boost::multiprecision::msb(magnitude));
    const auto lower = bits_minus_one * 1233U / 4096U + 1U;
    fit(lower + negative, budget.component_bytes);
    const Integer boundary = boost::multiprecision::pow(Integer(10), lower);
    const auto size = lower + (magnitude >= boundary) + negative;
    fit(size, budget.component_bytes);
    return size;
}

inline void exact_bytes(Budget &budget, Bytes &bytes, const Exact &value) {
    // All callers have already bounded the components.
    bytes.add(16);
    bytes.add(component_size(budget, boost::multiprecision::numerator(value)));
    bytes.add(component_size(budget, boost::multiprecision::denominator(value)));
}

inline Integer gcd(Budget &budget, Integer first, Integer second) {
    if (first < 0)
        first = -first;
    // Euclidean remainders only shrink the temporary components. Debit
    // each step so worst-case inputs also respect the caller's work limit.
    while (second != 0) {
        budget.work();
        Integer remainder = first % second;
        first = std::move(second);
        second = std::move(remainder);
    }
    return first;
}

inline void exact_input(Budget &budget, const Exact &value) {
    exact_bytes(budget, budget.input, value);
    require(gcd(budget, boost::multiprecision::numerator(value),
                boost::multiprecision::denominator(value)) == 1);
}

inline Exact factor(Budget &budget, std::string_view text) {
    budget.work(text.size());
    budget.work();
    return parse_nonnegative_factor(text).value();
}

inline Exact normalized(Budget &budget, const Integer &n, const Integer &d) {
    // Keep the potentially larger intermediates as integers. Verify the
    // normalized pair before constructing an Exact value.
    const Integer divisor = gcd(budget, n, d);
    budget.work(2); // The two exact quotients.
    const Integer numerator = n / divisor;
    const Integer denominator = d / divisor;
    component_shape(numerator.backend());
    component_shape(denominator.backend());
    budget.work(); // Exact's canonical constructor.
    return Exact(numerator, denominator);
}

inline Exact multiply(Budget &budget, const Exact &left, const Exact &right) {
    // Two products of at most 4096-bit components. Constructing a canonical
    // rational from them cannot increase either component.
    budget.work(2);
    return normalized(
        budget, boost::multiprecision::numerator(left) * boost::multiprecision::numerator(right),
        boost::multiprecision::denominator(left) * boost::multiprecision::denominator(right));
}

struct Operand {
    Key key;
    Target target;
    std::string_view spelling;
    Exact factor;
};

struct MemberDraft {
    const Operand *operand;
    std::optional<ModifierPriority> priority;
    bool winner = false;
};

struct GroupDraft {
    const ModifierGroupPolicy *policy;
    ModifierReduction reduction;
    std::vector<MemberDraft> members;
    bool winner = false;
};

struct TargetDraft {
    Target target;
    ModifierReduction reduction;
    std::vector<GroupDraft> groups;
    std::size_t trace_bytes = 0;
};

void policy_input(Budget &budget, const ModifierStackPolicy &policy);
std::vector<TargetDraft> assign(Budget &budget, std::span<const Operand> occurrences,
                                const ModifierStackPolicy &policy,
                                std::span<const Target> requested);
void reduce_traces(Budget &budget, std::span<TargetDraft> drafts, std::string_view policy_ref);
std::vector<ModifierTargetTrace> own_traces(std::span<const TargetDraft> drafts,
                                            std::string_view policy_ref);
ModifierOccurrenceKey own_key(Key key);
} // namespace subathon::core::modifier_detail
