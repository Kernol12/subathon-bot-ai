// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/effective_permissions.hpp>

#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstddef>
#include <limits>
#include <new>
#include <string_view>
#include <utility>

namespace subathon::core {
namespace {
using Error = EffectivePermissionError;
using Big = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<>,
                                          boost::multiprecision::et_off>;

[[noreturn]] void fail(Error code) { throw EffectivePermissionFailure{code}; }

// The byte order is explicit even on targets where plain char is signed.
bool byte_less(const std::string &a, const std::string &b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                        [](unsigned char x, unsigned char y) { return x < y; });
}

bool optional_less(const std::optional<std::string> &a, const std::optional<std::string> &b) {
    if (a.has_value() != b.has_value())
        return !a;
    return a && byte_less(*a, *b);
}

template <class Range, class Key>
const typename Range::value_type *find_key(const Range &range, const std::string &key, Key get) {
    const auto found = std::find_if(range.begin(), range.end(),
                                    [&](const auto &value) { return get(value) == key; });
    return found == range.end() ? nullptr : &*found;
}

template <class Range, class Key> bool unique_keys(const Range &range, Key get) {
    for (std::size_t i = 0; i < range.size(); ++i)
        for (std::size_t j = 0; j < i; ++j)
            if (get(range[i]) == get(range[j]))
                return false;
    return true;
}

const auto slot_key = [](const auto &value) -> const std::string & { return value.slot; };
const auto string_key = [](const std::string &value) -> const std::string & { return value; };
const auto operation_key = [](const auto &value) -> const std::string & { return value.operation; };

void validate_limits(const EffectivePermissionLimits &limits) {
    const auto max = static_cast<std::uint64_t>(std::min<std::uintmax_t>(
        std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::ptrdiff_t>::max()));
    for (const auto value :
         {limits.reference_bytes, limits.component_bytes, limits.input_items,
          limits.input_owned_bytes, limits.result_items, limits.result_owned_bytes})
        if (!value || value > max)
            fail(Error::invalid_limits);
    if (limits.component_bytes > (max - 1) / 8 || limits.component_bytes > max / 16 ||
        limits.input_items > (max - 16) ||
        limits.input_items > (max - 16 * limits.component_bytes) / 8)
        fail(Error::invalid_limits);
}

// Only borrows caller containers. No indices, copied strings or output DTOs are allocated.
struct Count {
    std::uint64_t bytes = 0;
    std::uint64_t items = 0;
    std::uint64_t byte_limit;
    std::uint64_t item_limit;
    std::uint64_t reference_limit;
    std::uint64_t component_limit;
    Error error;
    bool empty_reference = false;

    void add_bytes(std::uint64_t n) {
        if (n > byte_limit - bytes)
            fail(error);
        bytes += n;
    }
    void add_items(std::uint64_t n) {
        if (n > item_limit - items)
            fail(error);
        items += n;
    }
    void ref(const std::string &value, bool required = true) {
        if (value.size() > reference_limit || value.size() > std::string{}.max_size())
            fail(error);
        add_bytes(value.size());
        empty_reference |= required && value.empty();
    }
    void component(const std::string &value) {
        if (value.size() > component_limit || value.size() > std::string{}.max_size())
            fail(error);
        add_bytes(value.size());
    }
    void number(const PermissionRational &value) {
        component(value.numerator);
        component(value.denominator);
    }
    void optional_ref(const std::optional<std::string> &value) {
        add_bytes(1);
        if (value)
            ref(*value);
    }
    void optional_number(const std::optional<PermissionRational> &value) {
        add_bytes(1);
        if (value)
            number(*value);
    }
    template <class T> void vector_size(std::size_t n) {
        if (n > std::vector<T>{}.max_size())
            fail(error);
        add_items(n);
    }
    void owner(const PermissionOwnerFacts &value) {
        add_bytes(1);
        optional_ref(value.account);
    }
    void request(const PermissionRequest &value) {
        ref(value.operation);
        vector_size<PermissionResourceBinding>(value.resources.size());
        for (const auto &binding : value.resources) {
            ref(binding.slot);
            ref(binding.resource);
        }
        vector_size<PermissionQuantityBinding>(value.quantities.size());
        for (const auto &binding : value.quantities) {
            ref(binding.slot);
            ref(binding.unit);
            number(binding.value);
        }
    }
};

bool preflight_input(const EffectivePermissionInput &in, const EffectivePermissionLimits &limits) {
    Count count{0,
                0,
                limits.input_owned_bytes,
                limits.input_items,
                limits.reference_bytes,
                limits.component_bytes,
                Error::input_limit};
    count.ref(in.actor);
    count.ref(in.snapshot);
    count.add_bytes(1);
    count.ref(in.catalog.revision, in.catalog.state == PermissionFactsState::complete);
    count.vector_size<PermissionOperationDefinition>(in.catalog.operations.size());
    for (const auto &operation : in.catalog.operations) {
        count.ref(operation.operation);
        count.add_bytes(1);
        count.vector_size<std::string>(operation.resource_slots.size());
        for (const auto &slot : operation.resource_slots)
            count.ref(slot);
        count.vector_size<PermissionQuantityDefinition>(operation.quantity_slots.size());
        for (const auto &slot : operation.quantity_slots) {
            count.ref(slot.slot);
            count.ref(slot.unit);
            count.add_bytes(2); // number kind and admitted-mode mask
            count.optional_number(slot.minimum);
            count.optional_number(slot.maximum);
        }
        count.optional_ref(operation.helper_target_slot);
    }
    count.owner(in.owner);
    count.add_bytes(1);
    count.vector_size<std::string>(in.policy.assigned_roles.size());
    for (const auto &role : in.policy.assigned_roles)
        count.ref(role);
    count.vector_size<PermissionPolicyEntry>(in.policy.entries.size());
    for (const auto &entry : in.policy.entries) {
        count.add_bytes(2); // origin and effect
        count.ref(entry.source);
        count.optional_ref(entry.role);
        count.ref(entry.operation);
        count.optional_ref(entry.grant);
        count.vector_size<PermissionResourceConstraint>(entry.resources.size());
        for (const auto &constraint : entry.resources) {
            count.ref(constraint.slot);
            count.add_bytes(1);
            count.vector_size<std::string>(constraint.allowed.size());
            for (const auto &member : constraint.allowed)
                count.ref(member);
        }
        count.vector_size<PermissionQuantityConstraint>(entry.quantities.size());
        for (const auto &constraint : entry.quantities) {
            count.ref(constraint.slot);
            count.ref(constraint.unit);
            count.add_bytes(1);
            count.optional_number(constraint.lower);
            count.optional_number(constraint.upper);
        }
    }
    count.request(in.request);
    return count.empty_reference;
}

template <class T> bool tag_valid(T value, T last) {
    return static_cast<std::uint8_t>(value) <= static_cast<std::uint8_t>(last);
}

// Entire categories are checked before the next category, preserving global error priority.
void validate_tags(const EffectivePermissionInput &in, bool empty_reference) {
    if (!tag_valid(in.catalog.state, PermissionFactsState::complete) ||
        (in.catalog.state == PermissionFactsState::unknown &&
         (!in.catalog.revision.empty() || !in.catalog.operations.empty())))
        fail(Error::invalid_catalog);
    for (const auto &operation : in.catalog.operations) {
        if (!tag_valid(operation.classification, PermissionOperationClass::helper_password_reset))
            fail(Error::invalid_catalog);
        for (const auto &quantity : operation.quantity_slots)
            if (!tag_valid(quantity.kind, PermissionNumberKind::integer))
                fail(Error::invalid_catalog);
    }
    if (!tag_valid(in.owner.state, PermissionOwnerState::present) ||
        ((in.owner.state == PermissionOwnerState::present) != in.owner.account.has_value()))
        fail(Error::invalid_facts);
    if (!tag_valid(in.policy.state, PermissionFactsState::complete) ||
        (in.policy.state == PermissionFactsState::unknown &&
         (!in.policy.assigned_roles.empty() || !in.policy.entries.empty())))
        fail(Error::invalid_policy);
    for (const auto &entry : in.policy.entries) {
        if (!tag_valid(entry.origin, PermissionOrigin::role) ||
            !tag_valid(entry.effect, PermissionEffect::deny))
            fail(Error::invalid_policy);
        for (const auto &constraint : entry.resources)
            if (!tag_valid(constraint.mode, PermissionResourceMode::allowed_set))
                fail(Error::invalid_policy);
        for (const auto &constraint : entry.quantities)
            if (!tag_valid(constraint.mode, PermissionQuantityMode::magnitude_at_most))
                fail(Error::invalid_policy);
    }
    if (empty_reference)
        fail(Error::invalid_reference);
}

void validate_catalog_shape(const PermissionCatalog &catalog) {
    if (!unique_keys(catalog.operations, operation_key))
        fail(Error::invalid_catalog);
    for (const auto &operation : catalog.operations) {
        if (!unique_keys(operation.resource_slots, string_key) ||
            !unique_keys(operation.quantity_slots, slot_key))
            fail(Error::invalid_catalog);
        for (const auto &quantity : operation.quantity_slots)
            if (quantity.allowed_modes == 0 || quantity.allowed_modes > 15)
                fail(Error::invalid_catalog);
        const bool helper =
            operation.classification == PermissionOperationClass::helper_password_reset;
        if (helper != operation.helper_target_slot.has_value() ||
            (helper &&
             !find_key(operation.resource_slots, *operation.helper_target_slot, string_key)))
            fail(Error::invalid_catalog);
    }
}

template <class A, class B> bool same_slots(const A &declared, const B &supplied, auto key) {
    if (declared.size() != supplied.size() || !unique_keys(supplied, slot_key))
        return false;
    for (const auto &item : supplied)
        if (!find_key(declared, item.slot, key))
            return false;
    return true;
}

void validate_request_shape(const PermissionOperationDefinition *definition,
                            const PermissionRequest &request) {
    if (!definition || !same_slots(definition->resource_slots, request.resources, string_key) ||
        !same_slots(definition->quantity_slots, request.quantities, slot_key))
        fail(Error::invalid_request);
    for (const auto &binding : request.quantities)
        if (find_key(definition->quantity_slots, binding.slot, slot_key)->unit != binding.unit)
            fail(Error::invalid_request);
}

void validate_policy_shape(const EffectivePermissionInput &in) {
    if (!unique_keys(in.policy.assigned_roles, string_key))
        fail(Error::invalid_policy);
    for (const auto &entry : in.policy.entries) {
        const auto *operation = find_key(in.catalog.operations, entry.operation, operation_key);
        if (!operation || (entry.origin == PermissionOrigin::role) != entry.role.has_value() ||
            (entry.role && !find_key(in.policy.assigned_roles, *entry.role, string_key)) ||
            (entry.effect == PermissionEffect::allow) != entry.grant.has_value() ||
            !same_slots(operation->resource_slots, entry.resources, string_key) ||
            !same_slots(operation->quantity_slots, entry.quantities, slot_key))
            fail(Error::invalid_policy);
        for (const auto &constraint : entry.resources) {
            if ((constraint.mode == PermissionResourceMode::any && !constraint.allowed.empty()) ||
                (constraint.mode == PermissionResourceMode::allowed_set &&
                 constraint.allowed.empty()) ||
                !unique_keys(constraint.allowed, string_key))
                fail(Error::invalid_policy);
        }
        for (const auto &constraint : entry.quantities) {
            const auto *quantity = find_key(operation->quantity_slots, constraint.slot, slot_key);
            if (quantity->unit != constraint.unit ||
                !(quantity->allowed_modes & (1U << static_cast<unsigned>(constraint.mode))))
                fail(Error::invalid_policy);
            const bool lower = constraint.mode == PermissionQuantityMode::closed_interval;
            const bool upper = constraint.mode != PermissionQuantityMode::any;
            if (constraint.lower.has_value() != lower || constraint.upper.has_value() != upper)
                fail(Error::invalid_policy);
        }
    }
    for (std::size_t i = 0; i < in.policy.entries.size(); ++i)
        for (std::size_t j = 0; j < i; ++j) {
            const auto &a = in.policy.entries[i];
            const auto &b = in.policy.entries[j];
            if (a.source != b.source)
                continue;
            if (a.origin != b.origin || a.role != b.role || a.operation != b.operation ||
                a.effect != b.effect || a.effect == PermissionEffect::deny || a.grant == b.grant)
                fail(Error::invalid_policy);
        }
}

bool resource_contains(const PermissionResourceConstraint &constraint,
                       const std::string &resource) {
    return constraint.mode == PermissionResourceMode::any ||
           find_key(constraint.allowed, resource, string_key);
}

void validate_individual_scopes(const PermissionPolicyFacts &policy) {
    for (std::size_t i = 0; i < policy.entries.size(); ++i) {
        const auto &a = policy.entries[i];
        if (a.origin != PermissionOrigin::individual)
            continue;
        for (std::size_t j = 0; j < i; ++j) {
            const auto &b = policy.entries[j];
            if (b.origin != PermissionOrigin::individual || a.operation != b.operation ||
                a.effect == b.effect)
                continue;
            bool overlap = true;
            for (const auto &scope : a.resources) {
                const auto &other = *find_key(b.resources, scope.slot, slot_key);
                if (scope.mode == PermissionResourceMode::any ||
                    other.mode == PermissionResourceMode::any)
                    continue;
                if (!std::any_of(
                        scope.allowed.begin(), scope.allowed.end(),
                        [&](const auto &resource) { return resource_contains(other, resource); })) {
                    overlap = false;
                    break;
                }
            }
            if (overlap)
                fail(Error::contradictory_individual);
        }
    }
}

// Numeric work never uses Boost's general multiply/divide expression algorithms.
// ET is off; scalar parse, shifts, subtraction and addition do not own hidden Big values.
// The checked actual component lengths prove the bounds BEFORE these operations.
std::uint64_t magnitude_bound(const PermissionRational &a, const PermissionRational *b = nullptr) {
    std::uint64_t digits = std::max(a.numerator.size(), a.denominator.size());
    if (b)
        digits =
            std::max<std::uint64_t>(digits, std::max(b->numerator.size(), b->denominator.size()));
    // Stage0's 8D+1 representability and stage1's component ceiling already prove this.
    return 4 * digits;
}

Big parse_magnitude(std::string_view text) {
    if (!text.empty() && text.front() == '-')
        text.remove_prefix(1);
    Big value = 0;
    for (const char digit : text) {
        value *= 10;
        value += static_cast<unsigned>(digit - '0');
    }
    return value;
}

// At each subtract-and-shift step the sum of operand bit lengths decreases.
// Two G-bit positive operands therefore take at most 2G+1 such steps.
Big gcd(Big a, Big b, std::uint64_t bound) {
    if (a == 0)
        return b;
    if (b == 0)
        return a;
    std::uint64_t common = 0;
    std::uint64_t remaining = 2 * bound + 1;
    while (!boost::multiprecision::bit_test(a, 0) && !boost::multiprecision::bit_test(b, 0)) {
        a >>= 1;
        b >>= 1;
        ++common;
    }
    while (!boost::multiprecision::bit_test(a, 0))
        a >>= 1;
    while (b != 0) {
        if (remaining == 0)
            fail(Error::allocation_failure); // unreachable for valid bounded integers
        --remaining;
        while (!boost::multiprecision::bit_test(b, 0))
            b >>= 1;
        if (a > b)
            a.swap(b);
        b -= a;
    }
    while (common != 0) {
        --common;
        a <<= 1;
    }
    return a;
}

// Exact unsigned long division, at most G shifts/steps, three owned work integers.
// The shifted divisor uses at most G+1 bits, within preflighted 2G. There is no quotient
// or general-division backend temporary; the original value is moved into remainder.
void divide_exact(Big &value, const Big &divisor, std::uint64_t bound) {
    if (divisor == 1)
        return;
    Big remainder = std::move(value);
    Big shifted = divisor;
    Big quotient = 0;
    std::uint64_t shifts = 0;
    while (shifted <= remainder) {
        if (shifts == bound)
            fail(Error::allocation_failure);
        shifted <<= 1; // at most G+1 bits, within the comparison's preflighted 2G capacity
        ++shifts;
    }
    while (shifts != 0) {
        shifted >>= 1;
        quotient <<= 1;
        if (remainder >= shifted) {
            remainder -= shifted;
            quotient += 1;
        }
        --shifts;
    }
    value = std::move(quotient);
}

// Shift/add product: <=G iterations, two work integers, every sum <2^(2G).
// This avoids Karatsuba recursion/scratch and keeps the simultaneous Big count explicit.
Big product(const Big &a, const Big &b, std::uint64_t bound) {
    Big result = 0;
    Big shifted = a;
    Big bits = b;
    std::uint64_t remaining = bound;
    while (bits != 0) {
        if (remaining == 0)
            fail(Error::allocation_failure);
        --remaining;
        if (boost::multiprecision::bit_test(bits, 0))
            result += shifted;
        bits >>= 1;
        if (bits != 0)
            shifted <<= 1;
    }
    return result;
}

bool canonical_integer(std::string_view text, bool signed_value) {
    if (text.empty())
        return false;
    if (signed_value && text.front() == '-') {
        text.remove_prefix(1);
        if (text.empty() || text.front() == '0')
            return false;
    }
    if (text == "0")
        return signed_value;
    if (text.front() < '1' || text.front() > '9')
        return false;
    return std::all_of(text.begin(), text.end(),
                       [](unsigned char c) { return c >= '0' && c <= '9'; });
}

void validate_number(const PermissionRational &value) {
    if (!canonical_integer(value.numerator, true) || !canonical_integer(value.denominator, false) ||
        (value.numerator == "0" && value.denominator != "1"))
        fail(Error::invalid_number);
    if (value.denominator == "1")
        return;
    const auto bound = magnitude_bound(value);
    if (gcd(parse_magnitude(value.numerator), parse_magnitude(value.denominator), bound) != 1)
        fail(Error::invalid_number);
}

int sign(const PermissionRational &number) {
    return number.numerator == "0" ? 0 : (number.numerator.front() == '-' ? -1 : 1);
}

int compare(const PermissionRational &a, const PermissionRational &b, bool magnitude = false) {
    const int as = magnitude ? (sign(a) != 0) : sign(a);
    const int bs = magnitude ? (sign(b) != 0) : sign(b);
    if (as != bs)
        return as < bs ? -1 : 1;
    if (as == 0)
        return 0;
    std::string_view an = a.numerator;
    std::string_view bn = b.numerator;
    if (an.front() == '-')
        an.remove_prefix(1);
    if (bn.front() == '-')
        bn.remove_prefix(1);
    if (an == bn && a.denominator == b.denominator)
        return 0;
    const auto bound = magnitude_bound(a, &b);
    Big na = parse_magnitude(an);
    Big da = parse_magnitude(a.denominator);
    Big nb = parse_magnitude(bn);
    Big db = parse_magnitude(b.denominator);
    {
        Big common = gcd(na, nb, bound);
        divide_exact(na, common, bound);
        divide_exact(nb, common, bound);
    }
    {
        Big common = gcd(da, db, bound);
        divide_exact(da, common, bound);
        divide_exact(db, common, bound);
    }
    const Big left = product(na, db, bound);
    const Big right = product(nb, da, bound);
    const int result = left < right ? -1 : (left > right ? 1 : 0);
    return as < 0 ? -result : result;
}

template <class F> void every_number(const EffectivePermissionInput &in, F apply) {
    for (const auto &operation : in.catalog.operations)
        for (const auto &quantity : operation.quantity_slots) {
            if (quantity.minimum)
                apply(*quantity.minimum);
            if (quantity.maximum)
                apply(*quantity.maximum);
        }
    for (const auto &binding : in.request.quantities)
        apply(binding.value);
    for (const auto &entry : in.policy.entries)
        for (const auto &constraint : entry.quantities) {
            if (constraint.lower)
                apply(*constraint.lower);
            if (constraint.upper)
                apply(*constraint.upper);
        }
}

bool kind_fits(const PermissionRational &value, PermissionNumberKind kind) {
    return kind != PermissionNumberKind::integer || value.denominator == "1";
}

void validate_domains(const EffectivePermissionInput &in,
                      const PermissionOperationDefinition &requested) {
    for (const auto &operation : in.catalog.operations)
        for (const auto &quantity : operation.quantity_slots) {
            if ((quantity.minimum && !kind_fits(*quantity.minimum, quantity.kind)) ||
                (quantity.maximum && !kind_fits(*quantity.maximum, quantity.kind)) ||
                (quantity.minimum && quantity.maximum &&
                 compare(*quantity.minimum, *quantity.maximum) > 0))
                fail(Error::invalid_catalog);
        }
    for (const auto &binding : in.request.quantities) {
        const auto &quantity = *find_key(requested.quantity_slots, binding.slot, slot_key);
        if (!kind_fits(binding.value, quantity.kind) ||
            (quantity.minimum && compare(binding.value, *quantity.minimum) < 0) ||
            (quantity.maximum && compare(binding.value, *quantity.maximum) > 0))
            fail(Error::invalid_request);
    }
    for (const auto &entry : in.policy.entries) {
        const auto &operation = *find_key(in.catalog.operations, entry.operation, operation_key);
        for (const auto &constraint : entry.quantities) {
            const auto &quantity = *find_key(operation.quantity_slots, constraint.slot, slot_key);
            if ((constraint.lower && !kind_fits(*constraint.lower, quantity.kind)) ||
                (constraint.upper && !kind_fits(*constraint.upper, quantity.kind)) ||
                (constraint.mode == PermissionQuantityMode::closed_interval &&
                 compare(*constraint.lower, *constraint.upper) > 0) ||
                (constraint.mode == PermissionQuantityMode::magnitude_at_most &&
                 sign(*constraint.upper) < 0))
                fail(Error::invalid_policy);
        }
    }
}

bool quantity_fits(const PermissionQuantityConstraint &constraint,
                   const PermissionRational &value) {
    switch (constraint.mode) {
    case PermissionQuantityMode::any:
        return true;
    case PermissionQuantityMode::at_most:
        return compare(value, *constraint.upper) <= 0;
    case PermissionQuantityMode::closed_interval:
        return compare(value, *constraint.lower) >= 0 && compare(value, *constraint.upper) <= 0;
    case PermissionQuantityMode::magnitude_at_most:
        return compare(value, *constraint.upper, true) <= 0;
    }
    return false; // tag validation precedes matching
}

// Callbacks receive only borrowed failed keys. The same complete scan projects the
// result before materialization and later fills the preflighted failed-key lists.
template <class ResourceFailure, class QuantityFailure>
PermissionMatch match(const PermissionPolicyEntry &entry, const PermissionRequest &request,
                      ResourceFailure resource_failure, QuantityFailure quantity_failure) {
    if (entry.operation != request.operation)
        return PermissionMatch::other_operation;
    bool resources_fit = true;
    for (const auto &constraint : entry.resources) {
        const auto &binding = *find_key(request.resources, constraint.slot, slot_key);
        if (!resource_contains(constraint, binding.resource)) {
            resources_fit = false;
            resource_failure(constraint.slot);
        }
    }
    if (!resources_fit)
        return PermissionMatch::resource_mismatch;
    bool quantities_fit = true;
    for (const auto &constraint : entry.quantities) {
        const auto &binding = *find_key(request.quantities, constraint.slot, slot_key);
        if (!quantity_fits(constraint, binding.value)) {
            quantities_fit = false;
            quantity_failure(constraint.slot);
        }
    }
    return quantities_fit ? PermissionMatch::complete : PermissionMatch::quantity_mismatch;
}

PermissionMatch match(const PermissionPolicyEntry &entry, const PermissionRequest &request) {
    const auto ignore = [](const std::string &) {};
    return match(entry, request, ignore, ignore);
}

struct Resolution {
    PermissionOutcome outcome = PermissionOutcome::deny;
    PermissionReason reason = PermissionReason::default_deny;
    bool individual = false;
    bool protected_source = false;
};

Resolution resolve(const EffectivePermissionInput &in,
                   const PermissionOperationDefinition &operation) {
    Resolution result;
    const bool owner = in.owner.account && *in.owner.account == in.actor;
    if (owner &&
        (operation.classification == PermissionOperationClass::protected_access_management ||
         operation.classification == PermissionOperationClass::protected_recovery)) {
        result.outcome = PermissionOutcome::permit;
        result.reason = PermissionReason::protected_owner;
        result.protected_source = true;
        return result;
    }
    if (operation.classification == PermissionOperationClass::helper_password_reset &&
        in.owner.account &&
        find_key(in.request.resources, *operation.helper_target_slot, slot_key)->resource ==
            *in.owner.account) {
        result.reason = PermissionReason::protected_owner_target;
        result.protected_source = true;
        return result;
    }
    bool individual_allow = false, individual_deny = false;
    bool role_allow = false, role_deny = false;
    for (const auto &entry : in.policy.entries) {
        const auto matching = match(entry, in.request);
        const bool scoped =
            matching == PermissionMatch::complete || matching == PermissionMatch::quantity_mismatch;
        if (entry.origin == PermissionOrigin::individual) {
            result.individual |= scoped;
            individual_allow |=
                matching == PermissionMatch::complete && entry.effect == PermissionEffect::allow;
            individual_deny |=
                matching == PermissionMatch::complete && entry.effect == PermissionEffect::deny;
        } else {
            role_allow |=
                matching == PermissionMatch::complete && entry.effect == PermissionEffect::allow;
            role_deny |=
                matching == PermissionMatch::complete && entry.effect == PermissionEffect::deny;
        }
    }
    if (result.individual) {
        result.outcome = individual_allow ? PermissionOutcome::permit : PermissionOutcome::deny;
        result.reason = individual_allow  ? PermissionReason::individual_allow
                        : individual_deny ? PermissionReason::individual_deny
                                          : PermissionReason::individual_limit;
    } else {
        result.outcome =
            role_allow && !role_deny ? PermissionOutcome::permit : PermissionOutcome::deny;
        result.reason = role_deny    ? PermissionReason::role_deny
                        : role_allow ? PermissionReason::role_allow
                                     : PermissionReason::default_deny;
    }
    return result;
}

void preflight_result(const EffectivePermissionInput &in, const EffectivePermissionLimits &limits,
                      const Resolution &resolution) {
    Count count{0,
                0,
                limits.result_owned_bytes,
                limits.result_items,
                limits.reference_bytes,
                limits.component_bytes,
                Error::result_limit};
    count.ref(in.actor);
    count.ref(in.snapshot);
    count.ref(in.catalog.revision);
    count.owner(in.owner);
    count.request(in.request);
    count.add_bytes(3 + (resolution.outcome == PermissionOutcome::permit ? 8 : 0));
    const auto rows = in.policy.entries.size();
    if (resolution.protected_source && rows == std::numeric_limits<std::size_t>::max())
        fail(Error::result_limit);
    count.vector_size<PermissionSourceExplanation>(rows + (resolution.protected_source ? 1 : 0));
    if (resolution.protected_source) {
        count.add_bytes(6); // kind, two absent optionals, effect, match, disposition
        count.ref(*in.owner.account);
        count.ref(in.request.operation);
    }
    for (const auto &entry : in.policy.entries) {
        count.add_bytes(4); // kind, effect, match, disposition
        count.ref(entry.source);
        count.optional_ref(entry.grant);
        count.optional_ref(entry.role);
        count.ref(entry.operation);
        std::size_t resources = 0, quantities = 0;
        const auto resource_failure = [&](const std::string &key) {
            ++resources;
            count.ref(key);
        };
        const auto quantity_failure = [&](const std::string &key) {
            ++quantities;
            count.ref(key);
        };
        (void)match(entry, in.request, resource_failure, quantity_failure);
        count.vector_size<std::string>(resources);
        count.vector_size<std::string>(quantities);
    }
}

PermissionSourceDisposition disposition(const PermissionPolicyEntry &entry,
                                        PermissionMatch matching, const Resolution &resolution) {
    if (matching == PermissionMatch::other_operation ||
        matching == PermissionMatch::resource_mismatch)
        return PermissionSourceDisposition::not_applicable;
    if (resolution.protected_source ||
        (resolution.individual && entry.origin == PermissionOrigin::role))
        return PermissionSourceDisposition::suppressed;
    if (matching == PermissionMatch::quantity_mismatch)
        return PermissionSourceDisposition::quantity_limit;
    if (entry.effect == PermissionEffect::deny)
        return PermissionSourceDisposition::decisive_deny;
    if (resolution.reason == PermissionReason::role_deny)
        return PermissionSourceDisposition::suppressed;
    return PermissionSourceDisposition::fitting_allow;
}

bool source_less(const PermissionSourceExplanation &a, const PermissionSourceExplanation &b) {
    if (a.kind != b.kind)
        return a.kind < b.kind;
    if (a.source != b.source)
        return byte_less(a.source, b.source);
    if (a.grant != b.grant)
        return optional_less(a.grant, b.grant);
    return optional_less(a.role, b.role);
}

EffectivePermissionDecisionData materialize(const EffectivePermissionInput &in,
                                            const Resolution &resolution) {
    // This first owning DTO is constructed only after the whole result projection.
    EffectivePermissionDecisionData out;
    out.actor = in.actor;
    out.snapshot = in.snapshot;
    out.catalog_revision = in.catalog.revision;
    out.owner = in.owner;
    out.request = in.request;
    out.outcome = resolution.outcome;
    out.reason = resolution.reason;
    const auto by_slot = [](const auto &a, const auto &b) { return byte_less(a.slot, b.slot); };
    std::sort(out.request.resources.begin(), out.request.resources.end(), by_slot);
    std::sort(out.request.quantities.begin(), out.request.quantities.end(), by_slot);
    out.sources.reserve(in.policy.entries.size() + (resolution.protected_source ? 1 : 0));
    if (resolution.protected_source) {
        const bool permit = resolution.outcome == PermissionOutcome::permit;
        out.sources.push_back({PermissionSourceKind::protected_owner,
                               *in.owner.account,
                               {},
                               {},
                               in.request.operation,
                               permit ? PermissionEffect::allow : PermissionEffect::deny,
                               PermissionMatch::complete,
                               permit ? PermissionSourceDisposition::fitting_allow
                                      : PermissionSourceDisposition::decisive_deny,
                               {},
                               {}});
    }
    for (const auto &entry : in.policy.entries) {
        PermissionSourceExplanation row;
        row.kind = entry.origin == PermissionOrigin::individual ? PermissionSourceKind::individual
                                                                : PermissionSourceKind::role;
        row.source = entry.source;
        row.grant = entry.grant;
        row.role = entry.role;
        row.operation = entry.operation;
        row.effect = entry.effect;
        row.match = match(
            entry, in.request, [&](const std::string &key) { row.failed_resources.push_back(key); },
            [&](const std::string &key) { row.failed_quantities.push_back(key); });
        row.disposition = disposition(entry, row.match, resolution);
        std::sort(row.failed_resources.begin(), row.failed_resources.end(), byte_less);
        std::sort(row.failed_quantities.begin(), row.failed_quantities.end(), byte_less);
        out.sources.push_back(std::move(row));
    }
    std::sort(out.sources.begin(), out.sources.end(), source_less);
    if (resolution.outcome == PermissionOutcome::permit)
        for (std::size_t i = 0; i < out.sources.size(); ++i)
            if (out.sources[i].disposition == PermissionSourceDisposition::fitting_allow) {
                out.sources[i].disposition = PermissionSourceDisposition::permit_witness;
                out.witness_row = i;
                break;
            }
    return out;
}
} // namespace

EffectivePermissionDecision::EffectivePermissionDecision(EffectivePermissionDecisionData data)
    : data_(std::move(data)) {}

const EffectivePermissionDecisionData &EffectivePermissionDecision::data() const noexcept {
    return data_;
}

EffectivePermissionFailure::EffectivePermissionFailure(EffectivePermissionError code) noexcept
    : code_(code) {}

EffectivePermissionError EffectivePermissionFailure::code() const noexcept { return code_; }

const char *EffectivePermissionFailure::what() const noexcept {
    return "Effective permission evaluation failed";
}

EffectivePermissionsNotImplemented::EffectivePermissionsNotImplemented()
    : std::logic_error(
          "Effective permission behavior is intentionally unimplemented; review tests first") {}

EffectivePermissionDecision evaluate_effective_permission(const EffectivePermissionInput &in,
                                                          const EffectivePermissionLimits &limits) {
    try {
        validate_limits(limits);
        const bool empty_reference = preflight_input(in, limits);
        validate_tags(in, empty_reference);
        if (in.catalog.state == PermissionFactsState::unknown ||
            in.owner.state == PermissionOwnerState::unknown)
            fail(Error::facts_unavailable);
        validate_catalog_shape(in.catalog);
        const auto *operation =
            find_key(in.catalog.operations, in.request.operation, operation_key);
        validate_request_shape(operation, in.request);
        validate_policy_shape(in);
        validate_individual_scopes(in.policy);
        every_number(in, validate_number);
        validate_domains(in, *operation);
        const bool protected_owner =
            in.owner.account && *in.owner.account == in.actor &&
            (operation->classification == PermissionOperationClass::protected_access_management ||
             operation->classification == PermissionOperationClass::protected_recovery);
        if (in.policy.state == PermissionFactsState::unknown && !protected_owner)
            fail(Error::facts_unavailable);
        const auto resolution = resolve(in, *operation);
        preflight_result(in, limits, resolution);
        return EffectivePermissionDecision{materialize(in, resolution)};
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_failure);
    } catch (const std::length_error &) {
        fail(Error::allocation_failure);
    }
}
} // namespace subathon::core
