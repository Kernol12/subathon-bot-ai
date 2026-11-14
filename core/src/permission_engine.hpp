// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <limits>
#include <span>
#include <subathon/core/effective_permissions.hpp>
namespace subathon::core::permission_engine {
using Error = EffectivePermissionError;
[[noreturn]] void fail(Error);
bool byte_less(const std::string &, const std::string &);
bool optional_less(const std::optional<std::string> &, const std::optional<std::string> &);
// A range borrows either original contiguous rows or a bounded index of row pointers.
struct Rows {
    using value_type = PermissionPolicyEntry;
    std::span<const PermissionPolicyEntry> direct;
    std::span<const PermissionPolicyEntry *const> indirect;
    Rows() = default;
    Rows(const std::vector<PermissionPolicyEntry> &v) : direct(v) {}
    Rows(std::span<const PermissionPolicyEntry *const> v) : indirect(v) {}
    std::size_t size() const { return direct.size() + indirect.size(); }
    bool empty() const { return size() == 0; }
    const value_type &operator[](std::size_t i) const {
        return direct.empty() ? *indirect[i] : direct[i];
    }
    struct Iterator {
        const Rows *rows;
        std::size_t i;
        const value_type &operator*() const { return (*rows)[i]; }
        Iterator &operator++() {
            ++i;
            return *this;
        }
        bool operator==(const Iterator &) const = default;
    };
    Iterator begin() const { return {this, 0}; }
    Iterator end() const { return {this, size()}; }
};
struct Policy {
    PermissionFactsState state;
    const std::vector<std::string> &assigned_roles;
    Rows entries;
};
struct Input {
    const std::string &actor;
    const std::string &snapshot;
    const PermissionCatalog &catalog;
    const PermissionOwnerFacts &owner;
    Policy policy;
    const PermissionRequest &request;
};
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
struct Resolution {
    PermissionOutcome outcome = PermissionOutcome::deny;
    PermissionReason reason = PermissionReason::default_deny;
    bool individual = false;
    bool protected_source = false;
};
void validate_limits(const EffectivePermissionLimits &);
bool preflight_input(const Input &, const EffectivePermissionLimits &);
void validate_catalog_tags(const PermissionCatalog &);
void validate_owner_tags(const PermissionOwnerFacts &);
void validate_policy_tags(const Policy &);
void validate_tags(const Input &, bool);
void validate_catalog_shape(const PermissionCatalog &);
void validate_request_shape(const PermissionOperationDefinition *, const PermissionRequest &);
void validate_rows_shape(const PermissionCatalog &, Rows);
void validate_policy_shape(const Input &);
void validate_individual_scopes(const Policy &);
void validate_number(const PermissionRational &);
void validate_domains(const Input &, const PermissionOperationDefinition &);
int compare(const PermissionRational &, const PermissionRational &, bool magnitude = false);
int sign(const PermissionRational &);
bool resource_contains(const PermissionResourceConstraint &, const std::string &);
Resolution resolve(const Input &, const PermissionOperationDefinition &);
void project_result(const Input &, const Resolution &, Count &);
EffectivePermissionDecisionData materialize(const Input &, const Resolution &);
// Signed borrowed endpoint comparison and exact integer open-gap nonemptiness.
int compare_signed(const PermissionRational &, bool, const PermissionRational &, bool);
bool integer_gap(const PermissionRational &, bool, const PermissionRational &, bool);
} // namespace subathon::core::permission_engine
