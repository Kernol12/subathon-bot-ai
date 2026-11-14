// SPDX-License-Identifier: GPL-3.0-or-later
#include "permission_engine.hpp"
#include "permission_scratch.hpp"
#include <new>
#include <subathon/core/delegated_permissions.hpp>
#include <subathon/core/permission_graph_validation.hpp>
#include <utility>

namespace subathon::core {
namespace {
using namespace permission_engine;
using Outcome = PermissionOutcome;
const std::vector<std::string> no_roles;
const PermissionRequest no_request;
template <class R, class K>
const typename R::value_type *find(const R &r, const std::string &key, K k) {
    for (const auto &v : r)
        if (k(v) == key)
            return &v;
    return nullptr;
}
const auto slot = [](const auto &v) -> const std::string & { return v.slot; };
const auto name = [](const auto &v) -> const std::string & { return v; };
const auto opkey = [](const auto &v) -> const std::string & { return v.operation; };
template <class R, class K> bool unique(const R &r, K k) {
    for (std::size_t i = 0; i < r.size(); ++i)
        for (std::size_t j = 0; j < i; ++j)
            if (k(r[i]) == k(r[j]))
                return false;
    return true;
}
Policy policy(const PermissionPolicyFacts &p) { return {p.state, p.assigned_roles, p.entries}; }

// Recursive semantic accounting borrows all strings and containers, including unused graph rows.
void charge(Count &c, const PermissionRational &v) { c.number(v); }
void charge(Count &c, const std::string &v) { c.ref(v); }
template <class T> void charge(Count &c, const std::optional<T> &v) {
    c.add_bytes(1);
    if (v)
        charge(c, *v);
}
template <class T> void charge(Count &c, const std::vector<T> &v);
void charge(Count &c, const PermissionResourceConstraint &v) {
    c.ref(v.slot);
    c.add_bytes(1);
    charge(c, v.allowed);
}
void charge(Count &c, const PermissionQuantityConstraint &v) {
    c.ref(v.slot);
    c.ref(v.unit);
    c.add_bytes(1);
    charge(c, v.lower);
    charge(c, v.upper);
}
void charge(Count &c, const PermissionQuantityDefinition &v) {
    c.ref(v.slot);
    c.ref(v.unit);
    c.add_bytes(2);
    charge(c, v.minimum);
    charge(c, v.maximum);
}
void charge(Count &c, const PermissionOperationDefinition &v) {
    c.ref(v.operation);
    c.add_bytes(1);
    charge(c, v.resource_slots);
    charge(c, v.quantity_slots);
    charge(c, v.helper_target_slot);
}
void charge(Count &c, const PermissionCatalog &v) {
    c.add_bytes(1);
    c.ref(v.revision, v.state == PermissionFactsState::complete);
    charge(c, v.operations);
}
void charge(Count &c, const PermissionOwnerFacts &v) { c.owner(v); }
void charge(Count &c, const PermissionPolicyEntry &v) {
    c.add_bytes(2);
    c.ref(v.source);
    charge(c, v.role);
    c.ref(v.operation);
    charge(c, v.grant);
    charge(c, v.resources);
    charge(c, v.quantities);
}
void charge(Count &c, const PermissionPolicyFacts &v) {
    c.add_bytes(1);
    charge(c, v.assigned_roles);
    charge(c, v.entries);
}
void charge(Count &c, const DelegatedPermissionGrant &v) {
    c.ref(v.operation);
    charge(c, v.resources);
    charge(c, v.quantities);
}
void charge(Count &c, const PermissionRequest &v) { c.request(v); }
void charge(Count &c, const PermissionEditableRole &v) {
    c.ref(v.role);
    charge(c, v.entries);
}
void charge(Count &c, const PermissionEditableAccount &v) {
    c.ref(v.account);
    charge(c, v.assigned_roles);
    charge(c, v.individual_entries);
}
void charge(Count &c, const PermissionEditableState &v) {
    c.add_bytes(1);
    charge(c, v.roles);
    charge(c, v.accounts);
}
void charge(Count &c, const PermissionEditProbe &v) {
    c.ref(v.account);
    charge(c, v.request);
}
template <class T> void charge(Count &c, const std::vector<T> &v) {
    c.vector_size<T>(v.size());
    for (const auto &x : v)
        charge(c, x);
}
void charge(Count &c, const PermissionDelegationInput &v) {
    c.ref(v.actor);
    c.ref(v.snapshot);
    charge(c, v.catalog);
    charge(c, v.owner);
    charge(c, v.delegation);
    c.add_bytes(1);
    c.ref(v.target);
    charge(c, v.proposed);
}
void charge(Count &c, const PermissionAutomationInput &v) {
    c.ref(v.author);
    c.ref(v.snapshot);
    charge(c, v.catalog);
    charge(c, v.owner);
    charge(c, v.use);
    charge(c, v.delegation);
    charge(c, v.actions);
}
void charge(Count &c, const PermissionEditInput &v) {
    c.ref(v.actor);
    c.ref(v.snapshot);
    charge(c, v.catalog);
    charge(c, v.owner);
    charge(c, v.delegation);
    charge(c, v.management);
    charge(c, v.before);
    charge(c, v.after);
    charge(c, v.probes);
}
Count counter(const EffectivePermissionLimits &l, bool result = false) {
    return {0,
            0,
            result ? l.result_owned_bytes : l.input_owned_bytes,
            result ? l.result_items : l.input_items,
            l.reference_bytes,
            l.component_bytes,
            result ? Error::result_limit : Error::input_limit};
}
void limits_check(const EffectivePermissionLimits &l) {
    validate_limits(l);
    const auto max = static_cast<std::uint64_t>(std::min<std::uintmax_t>(SIZE_MAX, PTRDIFF_MAX));
    auto n = l.input_items;
    if (n > max / n)
        fail(Error::invalid_limits);
    n *= n;
    if (n > max / n || l.input_items > (max - 16 * l.component_bytes) / 128)
        fail(Error::invalid_limits);
}

template <class T> bool tag_valid(T v, T last) {
    return static_cast<unsigned>(v) <= static_cast<unsigned>(last);
}

// The pool contains only borrowed tuple/range addresses; no policy or request copies.
struct Tuple {
    const std::string &operation;
    const std::vector<PermissionResourceConstraint> &resources;
    const std::vector<PermissionQuantityConstraint> &quantities;
};
Tuple tuple(const DelegatedPermissionGrant &v) { return {v.operation, v.resources, v.quantities}; }
Tuple tuple(const PermissionPolicyEntry &v) { return {v.operation, v.resources, v.quantities}; }
struct Pool {
    ScratchVector<Tuple> tuples;
    const PermissionCatalog &catalog;
    const PermissionEditInput *edit = nullptr;
    template <class R> void add(const R &rows) {
        for (const auto &r : rows)
            tuples.push_back(tuple(r));
    }
};
void tuple_tags(const Tuple &t) {
    for (const auto &r : t.resources)
        if (!tag_valid(r.mode, PermissionResourceMode::allowed_set))
            fail(Error::invalid_policy);
    for (const auto &q : t.quantities)
        if (!tag_valid(q.mode, PermissionQuantityMode::magnitude_at_most))
            fail(Error::invalid_policy);
}
void tuple_shape(const Tuple &t, const PermissionCatalog &catalog) {
    const auto *op = find(catalog.operations, t.operation, opkey);
    if (!op || t.resources.size() != op->resource_slots.size() ||
        t.quantities.size() != op->quantity_slots.size() || !unique(t.resources, slot) ||
        !unique(t.quantities, slot))
        fail(Error::invalid_policy);
    for (const auto &r : t.resources) {
        if (!find(op->resource_slots, r.slot, name) ||
            !tag_valid(r.mode, PermissionResourceMode::allowed_set) ||
            (r.mode == PermissionResourceMode::any ? !r.allowed.empty() : r.allowed.empty()) ||
            !unique(r.allowed, name))
            fail(Error::invalid_policy);
    }
    for (const auto &q : t.quantities) {
        const auto *d = find(op->quantity_slots, q.slot, slot);
        if (!d || d->unit != q.unit ||
            !tag_valid(q.mode, PermissionQuantityMode::magnitude_at_most) ||
            !(d->allowed_modes & (1U << static_cast<unsigned>(q.mode))) ||
            q.lower.has_value() != (q.mode == PermissionQuantityMode::closed_interval) ||
            q.upper.has_value() != (q.mode != PermissionQuantityMode::any))
            fail(Error::invalid_policy);
    }
}
// Local tag helper keeps invalid enums out of arithmetic and switches.
void tuple_numbers(const Tuple &t) {
    for (const auto &q : t.quantities) {
        if (q.lower)
            validate_number(*q.lower);
        if (q.upper)
            validate_number(*q.upper);
    }
}
void tuple_domains(const Tuple &t, const PermissionCatalog &cat) {
    const auto &op = *find(cat.operations, t.operation, opkey);
    for (const auto &q : t.quantities) {
        const auto &d = *find(op.quantity_slots, q.slot, slot);
        if ((d.kind == PermissionNumberKind::integer &&
             ((q.lower && q.lower->denominator != "1") ||
              (q.upper && q.upper->denominator != "1"))) ||
            (q.lower && compare(*q.lower, *q.upper) > 0) ||
            (q.mode == PermissionQuantityMode::magnitude_at_most && sign(*q.upper) < 0))
            fail(Error::invalid_policy);
    }
}
struct Endpoint {
    const PermissionRational *value = nullptr;
    bool negative = false;
};
int cmp(Endpoint a, Endpoint b) {
    return compare_signed(*a.value, a.negative, *b.value, b.negative);
}
struct Cell {
    Endpoint lower, upper;
    bool point = false;
};
// A dimension retains boundaries, not a Cartesian product or representative request.
struct Dimension {
    const std::string *key;
    const PermissionQuantityDefinition *quantity = nullptr;
    ScratchVector<const std::string *> resources;
    ScratchVector<Endpoint> endpoints;
    std::size_t cursor = 0;
    std::size_t count() const { return quantity ? 2 * endpoints.size() + 1 : resources.size() + 1; }
    Cell cell() const {
        if (cursor % 2)
            return {endpoints[cursor / 2], endpoints[cursor / 2], true};
        return {cursor ? endpoints[cursor / 2 - 1] : Endpoint{},
                cursor / 2 < endpoints.size() ? endpoints[cursor / 2] : Endpoint{}, false};
    }
};
bool fits(const PermissionQuantityConstraint &q, const Cell &c) {
    auto lower = [&](Endpoint e) { return c.lower.value && cmp(c.lower, e) >= 0; };
    auto upper = [&](Endpoint e) { return c.upper.value && cmp(c.upper, e) <= 0; };
    switch (q.mode) {
    case PermissionQuantityMode::any:
        return true;
    case PermissionQuantityMode::at_most:
        return upper({&*q.upper, false});
    case PermissionQuantityMode::closed_interval:
        return lower({&*q.lower, false}) && upper({&*q.upper, false});
    case PermissionQuantityMode::magnitude_at_most:
        return lower({&*q.upper, true}) && upper({&*q.upper, false});
    }
    return false;
}
bool valid_cell(const Dimension &d) {
    if (!d.quantity)
        return true;
    const auto c = d.cell();
    const auto &q = *d.quantity;
    if (q.minimum && (!c.lower.value || cmp(c.lower, {&*q.minimum, false}) < 0))
        return false;
    if (q.maximum && (!c.upper.value || cmp(c.upper, {&*q.maximum, false}) > 0))
        return false;
    if (q.kind == PermissionNumberKind::rational)
        return true;
    if (c.point)
        return c.lower.value->denominator == "1";
    return !c.lower.value || !c.upper.value ||
           integer_gap(*c.lower.value, c.lower.negative, *c.upper.value, c.upper.negative);
}
bool resource_fit(const PermissionResourceConstraint &r, const Dimension &d) {
    return r.mode == PermissionResourceMode::any ||
           (d.cursor < d.resources.size() && resource_contains(r, *d.resources[d.cursor]));
}
bool tuple_fit(const Tuple &t, std::span<const Dimension> dims, bool resources_only = false) {
    for (const auto &d : dims) {
        if (d.quantity) {
            if (!resources_only && !fits(*find(t.quantities, *d.key, slot), d.cell()))
                return false;
        } else if (!resource_fit(*find(t.resources, *d.key, slot), d))
            return false;
    }
    return true;
}
// Scratch audit (payload records, allocator metadata excluded): each graph row
// supports <=80 bytes of simultaneous tuple/change/index/binding storage. Each
// catalog dimension supports <=88 bytes of traversal state plus <=32 bytes of
// domain endpoints; tuple constraints add <=32 endpoint bytes, explicit resource
// members <=8 bytes, and concrete bindings <=16 bytes. Simultaneous changed-role
// and membership-role lists use <=16 bytes per role item; account lists use <=8
// bytes per account item. Helper-owner splitting adds one pointer
// per helper resource dimension (within that dimension's remaining allowance).
// Exact reserves avoid replacement capacity; ScratchScope enforces 128*actual I
// before every scratch allocation, including any unexpected transient capacity.
// Partitions are shared across tasks; only mixed-radix cursors change. Cell
// products/sums are checked before traversal. Summed row-slot scans are <=I^2
// per cell, so admitted <=L^2 cells stay within L^4 comparisons. Global identity
// scans and all account membership scans amortize over actual rows/memberships.
struct Kernel {
    const Pool &pool;
    const std::string &actor;
    const PermissionOwnerFacts &owner;
    std::uint64_t work = 0, ceiling;
    template <class F> void requests(F f) const {
        if (pool.edit) {
            f(pool.edit->management);
            for (const auto &p : pool.edit->probes)
                f(p.request);
        }
    }
    ScratchVector<Dimension> partitions;
    Kernel(const Pool &p, const std::string &a, const PermissionOwnerFacts &o, std::uint64_t w,
           std::uint64_t c)
        : pool(p), actor(a), owner(o), work(w), ceiling(c) {
        std::size_t count = 0;
        for (const auto &op : pool.catalog.operations)
            count += op.resource_slots.size() + op.quantity_slots.size();
        partitions.reserve(count);
        for (const auto &op : pool.catalog.operations)
            build_dimensions(op);
    }
    void build_dimensions(const PermissionOperationDefinition &op) {
        auto &ds = partitions;
        for (const auto &key : op.resource_slots) {
            Dimension d{&key, nullptr, {}, {}, 0};
            std::size_t capacity = 0;
            for (const auto &t : pool.tuples)
                if (t.operation == op.operation)
                    for (const auto &r : t.resources)
                        if (r.slot == key)
                            capacity += r.allowed.size();
            if (op.helper_target_slot && *op.helper_target_slot == key && owner.account)
                ++capacity;
            requests([&](const auto &request) {
                if (request.operation == op.operation)
                    for (const auto &r : request.resources)
                        if (r.slot == key)
                            ++capacity;
            });
            d.resources.reserve(capacity);
            requests([&](const auto &request) {
                if (request.operation == op.operation)
                    for (const auto &r : request.resources)
                        if (r.slot == key)
                            d.resources.push_back(&r.resource);
            });
            for (const auto &t : pool.tuples)
                if (t.operation == op.operation)
                    for (const auto &r : t.resources)
                        if (r.slot == key)
                            for (const auto &v : r.allowed)
                                d.resources.push_back(&v);
            if (op.helper_target_slot && *op.helper_target_slot == key && owner.account)
                d.resources.push_back(&*owner.account);
            std::sort(d.resources.begin(), d.resources.end(),
                      [](auto a, auto b) { return byte_less(*a, *b); });
            d.resources.erase(std::unique(d.resources.begin(), d.resources.end(),
                                          [](auto a, auto b) { return *a == *b; }),
                              d.resources.end());
            ds.push_back(std::move(d));
        }
        for (const auto &q : op.quantity_slots) {
            Dimension d{&q.slot, &q, {}, {}, 0};
            std::size_t capacity = q.minimum.has_value() + q.maximum.has_value();
            for (const auto &t : pool.tuples)
                if (t.operation == op.operation)
                    for (const auto &r : t.quantities)
                        if (r.slot == q.slot)
                            capacity += r.lower.has_value() + r.upper.has_value() +
                                        (r.mode == PermissionQuantityMode::magnitude_at_most);
            requests([&](const auto &request) {
                if (request.operation == op.operation)
                    for (const auto &r : request.quantities)
                        if (r.slot == q.slot)
                            ++capacity;
            });
            d.endpoints.reserve(capacity);
            requests([&](const auto &request) {
                if (request.operation == op.operation)
                    for (const auto &r : request.quantities)
                        if (r.slot == q.slot)
                            d.endpoints.push_back({&r.value, false});
            });
            if (q.minimum)
                d.endpoints.push_back({&*q.minimum, false});
            if (q.maximum)
                d.endpoints.push_back({&*q.maximum, false});
            for (const auto &t : pool.tuples)
                if (t.operation == op.operation)
                    for (const auto &r : t.quantities)
                        if (r.slot == q.slot) {
                            if (r.lower)
                                d.endpoints.push_back({&*r.lower, false});
                            if (r.upper) {
                                d.endpoints.push_back({&*r.upper, false});
                                if (r.mode == PermissionQuantityMode::magnitude_at_most)
                                    d.endpoints.push_back({&*r.upper, true});
                            }
                        }
            std::sort(d.endpoints.begin(), d.endpoints.end(),
                      [](auto a, auto b) { return cmp(a, b) < 0; });
            d.endpoints.erase(std::unique(d.endpoints.begin(), d.endpoints.end(),
                                          [](auto a, auto b) { return cmp(a, b) == 0; }),
                              d.endpoints.end());
            ds.push_back(std::move(d));
        }
    }
    std::span<Dimension> dimensions(const PermissionOperationDefinition &op) {
        std::size_t offset = 0;
        for (const auto &v : pool.catalog.operations) {
            const auto n = v.resource_slots.size() + v.quantity_slots.size();
            if (&v == &op)
                return std::span<Dimension>(partitions).subspan(offset, n);
            offset += n;
        }
        fail(Error::invalid_catalog);
    }
    bool permitted(const Policy &p, const PermissionOperationDefinition &op,
                   std::span<const Dimension> ds, bool grant, const std::string &who) const {
        const bool protected_op =
            op.classification == PermissionOperationClass::protected_access_management ||
            op.classification == PermissionOperationClass::protected_recovery;
        if (protected_op && grant)
            return false; // N1: common to every GRANT path.
        if (protected_op && owner.account && *owner.account == who)
            return true;
        if (op.classification == PermissionOperationClass::helper_password_reset && owner.account) {
            for (const auto &d : ds)
                if (!d.quantity && *d.key == *op.helper_target_slot &&
                    d.cursor < d.resources.size() && *d.resources[d.cursor] == *owner.account)
                    return false;
        }
        bool individual = false, ia = false, ra = false, rd = false;
        for (const auto &r : p.entries)
            if (r.operation == op.operation) {
                const auto t = tuple(r);
                const bool scoped = tuple_fit(t, ds, true), complete = scoped && tuple_fit(t, ds);
                if (r.origin == PermissionOrigin::individual) {
                    individual |= scoped;
                    ia |= complete && r.effect == PermissionEffect::allow;
                } else {
                    ra |= complete && r.effect == PermissionEffect::allow;
                    rd |= complete && r.effect == PermissionEffect::deny;
                }
            }
        return individual ? ia : ra && !rd;
    }
    template <class Select, class Visit>
    void traverse(const PermissionOperationDefinition &op, Select select, Visit visit,
                  bool counting) {
        auto ds = dimensions(op);
        std::uint64_t product = 1;
        // Project the product from independent nonempty dimension counts BEFORE enumeration.
        for (auto &d : ds) {
            std::uint64_t n = 0;
            for (d.cursor = 0; d.cursor < d.count(); ++d.cursor)
                if (valid_cell(d) && select(d))
                    ++n;
            d.cursor = 0;
            if (n && product > ceiling / n)
                fail(Error::input_limit);
            product *= n;
        }
        if (counting) {
            if (product > ceiling - work)
                fail(Error::input_limit);
            work += product;
            return;
        }
        if (!product)
            return;
        // Odometer wraps rescan skipped dimension cells. Across admitted products
        // this adds at most L^2 * O(I) cell selections, still within L^4 work.
        auto advance = [&](Dimension &d) {
            while (d.cursor < d.count() && (!valid_cell(d) || !select(d)))
                ++d.cursor;
        };
        for (auto &d : ds)
            advance(d);
        for (std::uint64_t i = 0; i < product; ++i) {
            visit(ds);
            for (std::size_t j = ds.size(); j > 0; --j) {
                auto &d = ds[j - 1];
                ++d.cursor;
                advance(d);
                if (d.cursor < d.count())
                    break;
                d.cursor = 0;
                advance(d);
            }
        }
    }
    Outcome containment(const Tuple &t, const Policy &p, bool grant, bool counting) {
        const auto &op = *find(pool.catalog.operations, t.operation, opkey);
        // Protected GRANTs are refused as whole proposals, even when catalog
        // intersection leaves no cells. Traversal still charges the same work.
        bool ok = !(grant &&
                    (op.classification == PermissionOperationClass::protected_access_management ||
                     op.classification == PermissionOperationClass::protected_recovery));
        // Owner-target proposals are refused in FULL even when only part intersects owner.
        if (grant && op.classification == PermissionOperationClass::helper_password_reset &&
            owner.account &&
            resource_contains(*find(t.resources, *op.helper_target_slot, slot), *owner.account))
            ok = false;
        auto select = [&](const Dimension &d) {
            return d.quantity ? fits(*find(t.quantities, *d.key, slot), d.cell())
                              : resource_fit(*find(t.resources, *d.key, slot), d);
        };
        traverse(
            op, select, [&](const auto &ds) { ok &= permitted(p, op, ds, grant, actor); },
            counting);
        return ok ? Outcome::permit : Outcome::deny;
    }
};
Input borrowed(const std::string &actor, const std::string &snapshot, const PermissionCatalog &cat,
               const PermissionOwnerFacts &owner, Policy p,
               const PermissionRequest &r = no_request) {
    return {actor, snapshot, cat, owner, p, r};
}
// Delegation's explicit unavailable-state rule also accepts a retained, inert
// supplied payload (the reviewed unknown-delegation oracle). USE keeps the
// effective evaluator's strict unknown-state payload validation.
void policy_tags(const Input &in, bool delegation = false) {
    const auto state = delegation && in.policy.state == PermissionFactsState::unknown
                           ? PermissionFactsState::complete
                           : in.policy.state;
    const Input view{in.actor,
                     in.snapshot,
                     in.catalog,
                     in.owner,
                     {state, in.policy.assigned_roles, in.policy.entries},
                     in.request};
    validate_policy_tags(view.policy);
}
void validate_common(const Input &in, bool empty) {
    if (empty)
        fail(Error::invalid_reference);
    if (in.catalog.state == PermissionFactsState::unknown ||
        in.owner.state == PermissionOwnerState::unknown)
        fail(Error::facts_unavailable);
    validate_catalog_shape(in.catalog);
}
void validate_policy(const Input &in) { validate_policy_shape(in); }
void numeric(const Input &in) {
    for (const auto &o : in.catalog.operations)
        for (const auto &q : o.quantity_slots) {
            if (q.minimum)
                validate_number(*q.minimum);
            if (q.maximum)
                validate_number(*q.maximum);
        }
    for (const auto &r : in.policy.entries)
        tuple_numbers(tuple(r));
    for (const auto &q : in.request.quantities)
        validate_number(q.value);
}
void domains(const Input &in) {
    // The domain validator scans the full catalog and policy even with an empty request.
    const PermissionOperationDefinition empty;
    const auto *op = find(in.catalog.operations, in.request.operation, opkey);
    validate_domains(in, op ? *op : empty);
}
template <class F> auto allocation_guard(F f) {
    try {
        return f();
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_failure);
    } catch (const std::length_error &) {
        fail(Error::allocation_failure);
    }
}

PermissionDelegationDecision delegation_impl(const PermissionDelegationInput &in,
                                             const EffectivePermissionLimits &l) {
    limits_check(l);
    auto c = counter(l);
    charge(c, in);
    ScratchScope scratch(static_cast<std::size_t>(c.items));
    const auto b = borrowed(in.actor, in.snapshot, in.catalog, in.owner, policy(in.delegation));
    validate_catalog_tags(in.catalog);
    validate_owner_tags(in.owner);
    policy_tags(b, true);
    for (const auto &t : in.proposed)
        tuple_tags(tuple(t));
    if (!tag_valid(in.context, PermissionDelegationContext::automation_authorization))
        fail(Error::invalid_policy);
    validate_common(b, c.empty_reference);
    validate_policy(b);
    for (const auto &t : in.proposed)
        tuple_shape(tuple(t), in.catalog);
    validate_individual_scopes(b.policy);
    numeric(b);
    for (const auto &t : in.proposed)
        tuple_numbers(tuple(t));
    domains(b);
    for (const auto &t : in.proposed)
        tuple_domains(tuple(t), in.catalog);
    if (in.delegation.state == PermissionFactsState::unknown)
        fail(Error::facts_unavailable);
    Pool pool{{}, in.catalog};
    pool.tuples.reserve(in.delegation.entries.size() + in.proposed.size());
    pool.add(in.delegation.entries);
    pool.add(in.proposed);
    Kernel k{pool, in.actor, in.owner, 0, l.input_items * l.input_items};
    for (const auto &t : in.proposed)
        (void)k.containment(tuple(t), b.policy, true, true);
    auto outc = counter(l, true);
    outc.ref(in.actor);
    outc.ref(in.snapshot);
    outc.ref(in.catalog.revision);
    outc.ref(in.target);
    outc.add_bytes(1 + in.proposed.size());
    outc.vector_size<Outcome>(in.proposed.size());
    PermissionDelegationDecision out{in.actor,  in.snapshot, in.catalog.revision,
                                     in.target, {},          Outcome::permit};
    out.grants.reserve(in.proposed.size());
    for (const auto &t : in.proposed) {
        const auto r = k.containment(tuple(t), b.policy, true, false);
        out.grants.push_back(r);
        if (r == Outcome::deny)
            out.outcome = r;
    }
    return out;
}
PermissionAutomationDecision automation_impl(const PermissionAutomationInput &in,
                                             const EffectivePermissionLimits &l) {
    limits_check(l);
    auto c = counter(l);
    charge(c, in);
    ScratchScope scratch(static_cast<std::size_t>(c.items));
    const auto use = borrowed(in.author, in.snapshot, in.catalog, in.owner, policy(in.use));
    const auto grant =
        borrowed(in.author, in.snapshot, in.catalog, in.owner, policy(in.delegation));
    validate_catalog_tags(in.catalog);
    validate_owner_tags(in.owner);
    policy_tags(use);
    policy_tags(grant, true);
    for (const auto &t : in.actions)
        tuple_tags(tuple(t));
    validate_common(use, c.empty_reference);
    validate_policy(use);
    validate_policy(grant);
    for (const auto &t : in.actions)
        tuple_shape(tuple(t), in.catalog);
    validate_individual_scopes(use.policy);
    validate_individual_scopes(grant.policy);
    numeric(use);
    numeric(grant);
    for (const auto &t : in.actions)
        tuple_numbers(tuple(t));
    domains(use);
    domains(grant);
    for (const auto &t : in.actions)
        tuple_domains(tuple(t), in.catalog);
    if (in.use.state == PermissionFactsState::unknown ||
        in.delegation.state == PermissionFactsState::unknown)
        fail(Error::facts_unavailable);
    Pool pool{{}, in.catalog};
    pool.tuples.reserve(in.use.entries.size() + in.delegation.entries.size() + in.actions.size());
    pool.add(in.use.entries);
    pool.add(in.delegation.entries);
    pool.add(in.actions);
    Kernel k{pool, in.author, in.owner, 0, l.input_items * l.input_items};
    for (const auto &t : in.actions) {
        (void)k.containment(tuple(t), use.policy, false, true);
        (void)k.containment(tuple(t), grant.policy, true, true);
    }
    auto outc = counter(l, true);
    outc.ref(in.author);
    outc.ref(in.snapshot);
    outc.ref(in.catalog.revision);
    outc.vector_size<PermissionAutomationActionDecision>(in.actions.size());
    outc.add_bytes(3 * in.actions.size());
    PermissionAutomationDecision out{in.author, in.snapshot, in.catalog.revision, {}};
    out.actions.reserve(in.actions.size());
    for (const auto &t : in.actions) {
        const auto u = k.containment(tuple(t), use.policy, false, false),
                   g = k.containment(tuple(t), grant.policy, true, false);
        out.actions.push_back({u, g,
                               u == Outcome::permit && g == Outcome::permit
                                   ? PermissionAutomationDisposition::authorized
                                   : PermissionAutomationDisposition::hold});
    }
    return out;
}
const auto account_key = [](const auto &a) -> const std::string & { return a.account; };
const auto role_key = [](const auto &a) -> const std::string & { return a.role; };
const PermissionEditableAccount *account(const PermissionEditableState &s, const std::string &a) {
    return find(s.accounts, a, account_key);
}
struct GraphIndex {
    const PermissionEditableState &state;
    struct Row {
        const PermissionPolicyEntry *entry;
        const PermissionEditableAccount *account;
    };
    ScratchVector<Row> rows;
    explicit GraphIndex(const PermissionEditableState &s) : state(s) {
        std::size_t n = 0;
        for (const auto &r : s.roles)
            n += r.entries.size();
        for (const auto &a : s.accounts)
            n += a.individual_entries.size();
        rows.reserve(n);
        for (const auto &r : s.roles)
            for (const auto &e : r.entries)
                rows.push_back({&e, nullptr});
        for (const auto &a : s.accounts)
            for (const auto &e : a.individual_entries)
                rows.push_back({&e, &a});
        std::sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) {
            const auto &x = *a.entry, &y = *b.entry;
            if (x.origin != y.origin)
                return x.origin < y.origin;
            if (x.source != y.source)
                return byte_less(x.source, y.source);
            if (x.grant != y.grant)
                return optional_less(x.grant, y.grant);
            return optional_less(x.role, y.role);
        });
    }
};
ScratchVector<const PermissionPolicyEntry *> bind(const GraphIndex &s,
                                                  const PermissionEditableAccount *a) {
    ScratchVector<const PermissionPolicyEntry *> rows;
    if (!a)
        return rows;
    auto includes = [&](const GraphIndex::Row &r) {
        return r.account ? r.account == a : bool(find(a->assigned_roles, *r.entry->role, name));
    };
    std::size_t n = 0;
    for (const auto &r : s.rows)
        if (includes(r))
            ++n;
    rows.reserve(n);
    for (const auto &r : s.rows)
        if (includes(r))
            rows.push_back(r.entry);
    return rows;
}
Policy bound_policy(const PermissionEditableAccount *a,
                    const ScratchVector<const PermissionPolicyEntry *> &rows) {
    return {PermissionFactsState::complete, a ? a->assigned_roles : no_roles,
            Rows{std::span<const PermissionPolicyEntry *const>(rows)}};
}
bool row_identity(const PermissionPolicyEntry &a, const PermissionPolicyEntry &b) {
    return a.origin == b.origin && a.source == b.source && a.grant == b.grant && a.role == b.role;
}
bool number_equal(const std::optional<PermissionRational> &a,
                  const std::optional<PermissionRational> &b) {
    return a.has_value() == b.has_value() &&
           (!a || (a->numerator == b->numerator && a->denominator == b->denominator));
}
bool row_equal(const PermissionPolicyEntry &a, const PermissionPolicyEntry &b) {
    if (!row_identity(a, b) || a.operation != b.operation || a.effect != b.effect ||
        a.resources.size() != b.resources.size() || a.quantities.size() != b.quantities.size())
        return false;
    for (const auto &r : a.resources) {
        const auto *s = find(b.resources, r.slot, slot);
        if (!s || s->mode != r.mode || s->allowed.size() != r.allowed.size())
            return false;
        for (const auto &v : r.allowed)
            if (!find(s->allowed, v, name))
                return false;
    }
    for (const auto &q : a.quantities) {
        const auto *s = find(b.quantities, q.slot, slot);
        if (!s || q.unit != s->unit || q.mode != s->mode || !number_equal(q.lower, s->lower) ||
            !number_equal(q.upper, s->upper))
            return false;
    }
    return true;
}
bool account_equal(const PermissionEditableAccount &a, const PermissionEditableAccount &b) {
    if (a.assigned_roles.size() != b.assigned_roles.size() ||
        a.individual_entries.size() != b.individual_entries.size())
        return false;
    for (const auto &r : a.assigned_roles)
        if (!find(b.assigned_roles, r, name))
            return false;
    for (const auto &r : a.individual_entries) {
        bool found = false;
        for (const auto &s : b.individual_entries)
            found |= row_equal(r, s);
        if (!found)
            return false;
    }
    return true;
}
void graph_tags(const PermissionEditableState &s) {
    if (!tag_valid(s.state, PermissionFactsState::complete) ||
        (s.state == PermissionFactsState::unknown && (!s.roles.empty() || !s.accounts.empty())))
        fail(Error::invalid_policy);
    for (const auto &r : s.roles)
        validate_policy_tags({PermissionFactsState::complete, no_roles, Rows{r.entries}});
    for (const auto &a : s.accounts)
        validate_policy_tags(
            {PermissionFactsState::complete, a.assigned_roles, Rows{a.individual_entries}});
}
void graph_shape(const PermissionEditableState &s, const PermissionCatalog &cat) {
    if (!tag_valid(s.state, PermissionFactsState::complete) ||
        (s.state == PermissionFactsState::unknown && (!s.roles.empty() || !s.accounts.empty())))
        fail(Error::invalid_policy);
    if (!unique(s.roles, role_key) || !unique(s.accounts, account_key))
        fail(Error::invalid_policy);
    for (const auto &r : s.roles)
        for (const auto &e : r.entries) {
            if (e.origin != PermissionOrigin::role || !e.role || *e.role != r.role)
                fail(Error::invalid_policy);
            tuple_shape(tuple(e), cat);
        }
    for (const auto &a : s.accounts) {
        if (!unique(a.assigned_roles, name))
            fail(Error::invalid_policy);
        for (const auto &r : a.assigned_roles)
            if (!find(s.roles, r, role_key))
                fail(Error::invalid_policy);
        for (const auto &e : a.individual_entries) {
            if (e.origin != PermissionOrigin::individual)
                fail(Error::invalid_policy);
            tuple_shape(tuple(e), cat);
        }
    }
}
static_assert(sizeof(Dimension) <= 88);
static_assert(sizeof(Tuple) <= 24);
static_assert(sizeof(Endpoint) <= 16);

struct Changed {
    const PermissionPolicyEntry *row;
    PermissionEditSide side;
};
static_assert(sizeof(Changed) <= 16);
PermissionEditDecision edit_impl(const PermissionEditInput &in,
                                 const EffectivePermissionLimits &l) {
    limits_check(l);
    auto c = counter(l);
    charge(c, in);
    ScratchScope scratch(static_cast<std::size_t>(c.items));
    const auto delegation =
        borrowed(in.actor, in.snapshot, in.catalog, in.owner, policy(in.delegation));
    validate_catalog_tags(in.catalog);
    validate_owner_tags(in.owner);
    policy_tags(delegation, true);
    for (const auto *s : {&in.before, &in.after})
        graph_tags(*s);
    validate_common(delegation, c.empty_reference);
    validate_request_shape(find(in.catalog.operations, in.management.operation, opkey),
                           in.management);
    for (const auto &p : in.probes) {
        validate_request_shape(find(in.catalog.operations, p.request.operation, opkey), p.request);
    }
    graph_shape(in.before, in.catalog);
    graph_shape(in.after, in.catalog);
    if (in.before.state == PermissionFactsState::complete && !account(in.before, in.actor))
        fail(Error::invalid_policy);
    for (const auto &p : in.probes)
        if (in.before.state == PermissionFactsState::complete &&
            in.after.state == PermissionFactsState::complete && !account(in.before, p.account) &&
            !account(in.after, p.account))
            fail(Error::invalid_policy);
    validate_policy(delegation);
    for (const auto *s : {&in.before, &in.after}) {
        // Validate unassigned role definitions too, without manufacturing a policy copy.
        for (const auto &r : s->roles) {
            validate_tags(borrowed(in.actor, in.snapshot, in.catalog, in.owner,
                                   {PermissionFactsState::complete, no_roles, Rows{r.entries}}),
                          false);
            validate_rows_shape(in.catalog, Rows{r.entries});
        }
        for (const auto &a : s->accounts) {
            const auto b =
                borrowed(a.account, in.snapshot, in.catalog, in.owner,
                         {PermissionFactsState::complete, no_roles, Rows{a.individual_entries}});
            validate_tags(b, false);
            validate_rows_shape(in.catalog, Rows{a.individual_entries});
        }
        struct IndexedRow {
            const PermissionPolicyEntry *row;
            const PermissionEditableAccount *account;
        };
        ScratchVector<IndexedRow> all;
        std::size_t n = 0;
        for (const auto &r : s->roles)
            n += r.entries.size();
        for (const auto &a : s->accounts)
            n += a.individual_entries.size();
        all.reserve(n);
        for (const auto &r : s->roles)
            for (const auto &e : r.entries)
                all.push_back({&e, nullptr});
        for (const auto &a : s->accounts)
            for (const auto &e : a.individual_entries)
                all.push_back({&e, &a});
        std::sort(all.begin(), all.end(), [](const auto &a, const auto &b) {
            if (a.row->source != b.row->source)
                return byte_less(a.row->source, b.row->source);
            return optional_less(a.row->grant, b.row->grant);
        });
        // Sorted source groups make all account bindings a total O(I^2) scan.
        // No role-row pair scan is repeated for every member of a shared role.
        for (const auto &a : s->accounts) {
            const PermissionPolicyEntry *previous = nullptr;
            for (const auto &v : all) {
                const auto &e = *v.row;
                if (v.account ? v.account != &a : !find(a.assigned_roles, *e.role, name))
                    continue;
                if (previous && previous->source == e.source &&
                    (previous->origin != e.origin || previous->role != e.role ||
                     previous->operation != e.operation || previous->effect != e.effect ||
                     e.effect == PermissionEffect::deny || previous->grant == e.grant))
                    fail(Error::invalid_policy);
                previous = &e;
            }
        }
    }
    // Stage 5 follows shape validation on BOTH snapshots.
    validate_individual_scopes(delegation.policy);
    for (const auto *s : {&in.before, &in.after})
        for (const auto &a : s->accounts)
            validate_individual_scopes(
                {PermissionFactsState::complete, no_roles, Rows{a.individual_entries}});

    numeric(delegation);
    for (const auto *s : {&in.before, &in.after}) {
        for (const auto &r : s->roles)
            for (const auto &e : r.entries)
                tuple_numbers(tuple(e));
        for (const auto &a : s->accounts)
            for (const auto &e : a.individual_entries)
                tuple_numbers(tuple(e));
    }
    for (const auto &q : in.management.quantities)
        validate_number(q.value);
    for (const auto &p : in.probes)
        for (const auto &q : p.request.quantities)
            validate_number(q.value);
    const auto domain_base = borrowed(in.actor, in.snapshot, in.catalog, in.owner,
                                      {PermissionFactsState::complete, no_roles, Rows{}});
    domains(domain_base);
    domains(
        borrowed(in.actor, in.snapshot, in.catalog, in.owner, domain_base.policy, in.management));
    for (const auto &p : in.probes)
        domains(
            borrowed(p.account, in.snapshot, in.catalog, in.owner, domain_base.policy, p.request));
    domains(delegation);
    for (const auto *s : {&in.before, &in.after}) {
        for (const auto &r : s->roles)
            for (const auto &e : r.entries)
                tuple_domains(tuple(e), in.catalog);
        for (const auto &a : s->accounts)
            for (const auto &e : a.individual_entries)
                tuple_domains(tuple(e), in.catalog);
    }
    const GraphIndex before_index(in.before), after_index(in.after);
    auto management_rows = bind(before_index, account(in.before, in.actor));
    const auto management =
        borrowed(in.actor, in.snapshot, in.catalog, in.owner,
                 bound_policy(account(in.before, in.actor), management_rows), in.management);
    domains(management);
    for (const auto &p : in.probes) {
        auto rows = bind(before_index, account(in.before, p.account));
        domains(borrowed(p.account, in.snapshot, in.catalog, in.owner,
                         bound_policy(account(in.before, p.account), rows), p.request));
    }
    if (in.delegation.state == PermissionFactsState::unknown ||
        in.before.state == PermissionFactsState::unknown ||
        in.after.state == PermissionFactsState::unknown)
        fail(Error::facts_unavailable);
    Pool pool{{}, in.catalog, &in};
    std::size_t tuple_count = in.delegation.entries.size();
    for (const auto *s : {&in.before, &in.after}) {
        for (const auto &r : s->roles)
            tuple_count += r.entries.size();
        for (const auto &a : s->accounts)
            tuple_count += a.individual_entries.size();
    }
    pool.tuples.reserve(tuple_count);
    pool.add(in.delegation.entries);
    for (const auto *s : {&in.before, &in.after}) {
        for (const auto &r : s->roles)
            pool.add(r.entries);
        for (const auto &a : s->accounts)
            pool.add(a.individual_entries);
    }
    ScratchVector<Changed> changed;
    changed.reserve(tuple_count);
    ScratchVector<const std::string *> changed_roles;
    changed_roles.reserve(in.before.roles.size() + in.after.roles.size());
    auto add = [&](const PermissionPolicyEntry &r, PermissionEditSide side) {
        for (const auto &v : changed)
            if (v.side == side && row_equal(*v.row, r))
                return;
        changed.push_back({&r, side});
    };
    for (const auto side : {PermissionEditSide::before, PermissionEditSide::after}) {
        const auto &s = side == PermissionEditSide::before ? in.before : in.after;
        const auto &other = side == PermissionEditSide::before ? in.after : in.before;
        for (const auto &r : s.roles) {
            const auto *o = find(other.roles, r.role, role_key);
            bool different = !o;
            for (const auto &e : r.entries) {
                bool same = false;
                if (o)
                    for (const auto &v : o->entries)
                        same |= row_equal(e, v);
                if (!same) {
                    add(e, side);
                    different = true;
                }
            }
            if (o)
                for (const auto &v : o->entries) {
                    bool same = false;
                    for (const auto &e : r.entries)
                        same |= row_equal(e, v);
                    different |= !same;
                }
            if (different) {
                bool seen = false;
                for (auto p : changed_roles)
                    seen |= *p == r.role;
                if (!seen)
                    changed_roles.push_back(&r.role);
            }
        }
        // Collect changed memberships before expanding rows: many accounts may
        // gain the same role, but its row/side closure is identical for all of them.
        ScratchVector<const PermissionEditableRole *> membership_roles;
        membership_roles.reserve(s.roles.size());
        for (const auto &a : s.accounts) {
            const auto *o = account(other, a.account);
            for (const auto &e : a.individual_entries) {
                bool same = false;
                if (o)
                    for (const auto &v : o->individual_entries)
                        same |= row_equal(e, v);
                if (!same)
                    add(e, side);
            }
            for (const auto &r : a.assigned_roles)
                if (!o || !find(o->assigned_roles, r, name)) {
                    const auto *role = find(s.roles, r, role_key);
                    if (std::find(membership_roles.begin(), membership_roles.end(), role) ==
                        membership_roles.end())
                        membership_roles.push_back(role);
                }
        }
        for (const auto *role : membership_roles)
            for (const auto &e : role->entries)
                add(e, side);
    }
    std::sort(changed.begin(), changed.end(), [](const auto &a, const auto &b) {
        const auto &x = *a.row, &y = *b.row;
        if (x.source != y.source)
            return byte_less(x.source, y.source);
        if (x.grant != y.grant)
            return optional_less(x.grant, y.grant);
        if (x.role != y.role)
            return optional_less(x.role, y.role);
        return a.side < b.side;
    });
    ScratchVector<const std::string *> affected;
    affected.reserve(in.before.accounts.size() + in.after.accounts.size());
    for (const auto *s : {&in.before, &in.after})
        for (const auto &a : s->accounts) {
            const auto *b = account(in.before, a.account), *e = account(in.after, a.account);
            bool impact = !b || !e || (b && e && !account_equal(*b, *e));
            for (auto r : changed_roles)
                impact |= (b && find(b->assigned_roles, *r, name)) ||
                          (e && find(e->assigned_roles, *r, name));
            if (impact) {
                bool seen = false;
                for (auto p : affected)
                    seen |= *p == a.account;
                if (!seen)
                    affected.push_back(&a.account);
            }
        }
    std::sort(affected.begin(), affected.end(), [](auto a, auto b) { return byte_less(*a, *b); });
    Kernel k{pool, in.actor, in.owner, 0, l.input_items * l.input_items};
    // A concrete management/probe traversal is one cell, independently of dimensionality.
    if (in.probes.size() > (k.ceiling - 1) / 2)
        fail(Error::input_limit);
    k.work = 1 + 2 * in.probes.size();
    for (const auto &v : changed)
        (void)k.containment(tuple(*v.row), delegation.policy, true, true);
    auto exposure = [&](const std::string &who, bool counting) {
        auto br = bind(before_index, account(in.before, who)),
             ar = bind(after_index, account(in.after, who));
        const auto bp = bound_policy(account(in.before, who), br),
                   ap = bound_policy(account(in.after, who), ar);
        bool ok = true;
        for (const auto &op : in.catalog.operations)
            k.traverse(
                op, [](const auto &) { return true; },
                [&](const auto &ds) {
                    if (k.permitted(ap, op, ds, false, who) &&
                        !k.permitted(bp, op, ds, false, who) &&
                        !k.permitted(delegation.policy, op, ds, true, in.actor))
                        ok = false;
                },
                counting);
        return ok ? Outcome::permit : Outcome::deny;
    };
    for (auto a : affected)
        (void)exposure(*a, true);
    const auto mr =
        resolve(management, *find(in.catalog.operations, in.management.operation, opkey));
    auto outc = counter(l, true);
    outc.ref(in.actor);
    outc.ref(in.snapshot);
    outc.ref(in.catalog.revision);
    outc.add_bytes(1);
    project_result(management, mr, outc);
    outc.vector_size<PermissionChangedCapabilityDecision>(changed.size());
    for (const auto &v : changed) {
        outc.ref(v.row->source);
        outc.optional_ref(v.row->grant);
        outc.optional_ref(v.row->role);
        outc.add_bytes(2);
    }
    outc.vector_size<PermissionAffectedAccountDecision>(affected.size());
    for (auto a : affected) {
        outc.ref(*a);
        outc.add_bytes(1);
    }
    outc.vector_size<PermissionEditProbeDecision>(in.probes.size());
    for (const auto &p : in.probes)
        for (const auto *s : {&in.before, &in.after}) {
            auto rows = bind(s == &in.before ? before_index : after_index, account(*s, p.account));
            const auto b = borrowed(p.account, in.snapshot, in.catalog, in.owner,
                                    bound_policy(account(*s, p.account), rows), p.request);
            project_result(b, resolve(b, *find(in.catalog.operations, p.request.operation, opkey)),
                           outc);
        }
    PermissionEditDecision out;
    out.actor = in.actor;
    out.snapshot = in.snapshot;
    out.catalog_revision = in.catalog.revision;
    out.outcome = mr.outcome;
    out.management = materialize(management, mr);
    out.changed.reserve(changed.size());
    for (const auto &v : changed) {
        const auto r = k.containment(tuple(*v.row), delegation.policy, true, false);
        out.changed.push_back({v.row->source, v.row->grant, v.row->role, v.side, r});
        if (r == Outcome::deny)
            out.outcome = r;
    }
    out.affected.reserve(affected.size());
    for (auto a : affected) {
        const auto r = exposure(*a, false);
        out.affected.push_back({*a, r});
        if (r == Outcome::deny)
            out.outcome = r;
    }
    out.previews.reserve(in.probes.size());
    for (const auto &p : in.probes) {
        PermissionEditProbeDecision preview;
        for (const auto *s : {&in.before, &in.after}) {
            auto rows = bind(s == &in.before ? before_index : after_index, account(*s, p.account));
            const auto b = borrowed(p.account, in.snapshot, in.catalog, in.owner,
                                    bound_policy(account(*s, p.account), rows), p.request);
            auto data = materialize(
                b, resolve(b, *find(in.catalog.operations, p.request.operation, opkey)));
            if (s == &in.before)
                preview.before = std::move(data);
            else
                preview.after = std::move(data);
        }
        out.previews.push_back(std::move(preview));
    }
    return out;
}
} // namespace
PermissionDelegationDecision evaluate_permission_delegation(const PermissionDelegationInput &in,
                                                            const EffectivePermissionLimits &l) {
    return allocation_guard([&] { return delegation_impl(in, l); });
}
PermissionEditDecision preview_permission_edit(const PermissionEditInput &in,
                                               const EffectivePermissionLimits &l) {
    return allocation_guard([&] { return edit_impl(in, l); });
}
PermissionEditDecision revalidate_permission_edit(const PermissionEditInput &in,
                                                  const EffectivePermissionLimits &l) {
    return allocation_guard([&] { return edit_impl(in, l); });
}
PermissionAutomationDecision authorize_permission_automation(const PermissionAutomationInput &in,
                                                             const EffectivePermissionLimits &l) {
    return allocation_guard([&] { return automation_impl(in, l); });
}
PermissionAutomationDecision revalidate_permission_automation(const PermissionAutomationInput &in,
                                                              const EffectivePermissionLimits &l) {
    return allocation_guard([&] { return automation_impl(in, l); });
}
namespace internal {
void validate_permission_graph_catalog(const PermissionCatalog &cat) {
    validate_catalog_tags(cat);
    if (cat.state != PermissionFactsState::complete)
        fail(Error::facts_unavailable);
    validate_catalog_shape(cat);
    const std::string empty;
    const PermissionOwnerFacts owner{PermissionOwnerState::absent, std::nullopt};
    const auto in =
        borrowed(empty, empty, cat, owner, {PermissionFactsState::complete, no_roles, Rows{}});
    numeric(in);
    domains(in);
}
void validate_permission_graph(const PermissionCatalog &cat, const PermissionEditableState &s) {
    validate_permission_graph_catalog(cat);
    graph_tags(s);
    if (s.state != PermissionFactsState::complete)
        fail(Error::facts_unavailable);
    graph_shape(s, cat);
    const std::string empty;
    const PermissionOwnerFacts owner{PermissionOwnerState::absent, std::nullopt};
    auto rows_valid = [&](const std::vector<PermissionPolicyEntry> &rows) {
        validate_rows_shape(cat, Rows{rows});
        const auto in = borrowed(empty, empty, cat, owner,
                                 {PermissionFactsState::complete, no_roles, Rows{rows}});
        numeric(in);
        domains(in);
    };
    for (const auto &r : s.roles)
        rows_valid(r.entries);
    for (const auto &a : s.accounts) {
        rows_valid(a.individual_entries);
        validate_individual_scopes(
            {PermissionFactsState::complete, no_roles, Rows{a.individual_entries}});
        // Check provenance across the complete bound policy without expanding
        // memberships into owned policy rows. Unassigned roles were checked above.
        auto each = [&](auto f) {
            for (const auto &e : a.individual_entries)
                f(e);
            for (const auto &r : s.roles)
                if (find(a.assigned_roles, r.role, name))
                    for (const auto &e : r.entries)
                        f(e);
        };
        each([&](const auto &x) {
            each([&](const auto &y) {
                if (&x == &y || x.source != y.source)
                    return;
                if (x.origin != y.origin || x.role != y.role || x.operation != y.operation ||
                    x.effect != y.effect || x.effect == PermissionEffect::deny ||
                    x.grant == y.grant)
                    fail(Error::invalid_policy);
            });
        });
    }
}
bool permission_graph_entry_equal(const PermissionPolicyEntry &a, const PermissionPolicyEntry &b) {
    return row_equal(a, b);
}
} // namespace internal
} // namespace subathon::core
