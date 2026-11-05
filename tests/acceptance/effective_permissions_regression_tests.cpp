// SPDX-License-Identifier: GPL-3.0-or-later
// Isolated executable: the allocation hook is armed only around one direct evaluator call.
#include <subathon/core/effective_permissions.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <gtest/gtest.h>
#include <limits>
#include <new>
#include <optional>
#include <string>
#include <utility>

namespace allocation_fault {
struct State {
    bool armed = false;
    bool fired = false;
    std::optional<std::size_t> before_failure;
    std::size_t observed = 0;
};
constinit thread_local State state;

void before_allocation() {
    if (!state.armed)
        return;
    ++state.observed;
    if (!state.fired && state.before_failure) {
        if (*state.before_failure == 0) {
            state.fired = true;
            throw std::bad_alloc{};
        }
        --*state.before_failure;
    }
}

class Scope final {
  public:
    explicit Scope(std::optional<std::size_t> before_failure) noexcept {
        state = {true, false, before_failure, 0};
    }
    ~Scope() { state.armed = false; }
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
};
} // namespace allocation_fault

void *operator new(std::size_t bytes) {
    allocation_fault::before_allocation();
    if (void *result = std::malloc(bytes == 0 ? 1 : bytes))
        return result;
    throw std::bad_alloc{};
}
void *operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void *pointer) noexcept { std::free(pointer); }
void operator delete[](void *pointer) noexcept { std::free(pointer); }
void operator delete(void *pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void *pointer, std::size_t) noexcept { std::free(pointer); }

void *operator new(std::size_t bytes, std::align_val_t requested_alignment) {
    allocation_fault::before_allocation();
    const auto alignment = static_cast<std::size_t>(requested_alignment);
    if (bytes == 0)
        bytes = 1;
    const auto remainder = bytes % alignment;
    if (remainder != 0) {
        const auto padding = alignment - remainder;
        if (bytes > std::numeric_limits<std::size_t>::max() - padding)
            throw std::bad_alloc{};
        bytes += padding;
    }
    if (void *result = std::aligned_alloc(alignment, bytes))
        return result;
    throw std::bad_alloc{};
}
void *operator new[](std::size_t bytes, std::align_val_t alignment) {
    return ::operator new(bytes, alignment);
}
void operator delete(void *pointer, std::align_val_t) noexcept { std::free(pointer); }
void operator delete[](void *pointer, std::align_val_t) noexcept { std::free(pointer); }
void operator delete(void *pointer, std::size_t, std::align_val_t) noexcept { std::free(pointer); }
void operator delete[](void *pointer, std::size_t, std::align_val_t) noexcept {
    std::free(pointer);
}

void *operator new(std::size_t bytes, const std::nothrow_t &) noexcept {
    try {
        return ::operator new(bytes);
    } catch (...) {
        return nullptr;
    }
}
void *operator new[](std::size_t bytes, const std::nothrow_t &) noexcept {
    return ::operator new(bytes, std::nothrow);
}
void operator delete(void *pointer, const std::nothrow_t &) noexcept { std::free(pointer); }
void operator delete[](void *pointer, const std::nothrow_t &) noexcept { std::free(pointer); }
void *operator new(std::size_t bytes, std::align_val_t alignment, const std::nothrow_t &) noexcept {
    try {
        return ::operator new(bytes, alignment);
    } catch (...) {
        return nullptr;
    }
}
void *operator new[](std::size_t bytes, std::align_val_t alignment,
                     const std::nothrow_t &) noexcept {
    return ::operator new(bytes, alignment, std::nothrow);
}
void operator delete(void *pointer, std::align_val_t, const std::nothrow_t &) noexcept {
    std::free(pointer);
}
void operator delete[](void *pointer, std::align_val_t, const std::nothrow_t &) noexcept {
    std::free(pointer);
}

namespace {
using namespace subathon::core;

// N=10^80. These raw spellings are inputs, never values produced by the evaluator.
const std::string n = "1" + std::string(80, '0');
const std::string n_plus_one = "1" + std::string(79, '0') + "1";
const std::string n_plus_two = "1" + std::string(79, '0') + "2";
const std::string n_plus_three = "1" + std::string(79, '0') + "3";
const std::string n_minus_one(80, '9');

EffectivePermissionInput input(PermissionRational requested, PermissionRational upper) {
    return {"Actor",
            "Snapshot",
            {PermissionFactsState::complete,
             "Catalog",
             {{"O",
               PermissionOperationClass::ordinary,
               {},
               {{"quantity", "ms", PermissionNumberKind::rational, {}, {}, 15}},
               {}}}},
            {PermissionOwnerState::absent, {}},
            {PermissionFactsState::complete,
             {},
             {{PermissionOrigin::individual,
               "Source",
               {},
               "O",
               PermissionEffect::allow,
               "Grant",
               {},
               {{"quantity", "ms", PermissionQuantityMode::at_most, {}, std::move(upper)}}}}},
            {"O", {}, {{"quantity", "ms", std::move(requested)}}}};
}

TEST(EffectivePermissionRegression, UnequalLongRatiosAndCancellationStayExact) {
    struct Row {
        const char *description;
        PermissionRational requested;
        PermissionRational bound;
        bool permit;
    };
    // (N+1)^2 - N(N+2) = 1, hence (N+1)/N > (N+2)/(N+1).
    // No common numerator/denominator factors cancel in that comparison.
    // Equal positive numerator: smaller positive denominator is larger.
    // Equal denominator: larger numerator is larger. Negation reverses each order.
    const Row rows[]{
        {"full crossproducts differ by one", {n_plus_one, n}, {n_plus_two, n_plus_one}, false},
        {"full crossproducts reversed", {n_plus_two, n_plus_one}, {n_plus_one, n}, true},
        {"common numerator cancels", {n_plus_one, n_minus_one}, {n_plus_one, n_plus_three}, false},
        {"common numerator reversed", {n_plus_one, n_plus_three}, {n_plus_one, n_minus_one}, true},
        {"common denominator cancels", {n_plus_one, n}, {n_plus_three, n}, true},
        {"common denominator reversed", {n_plus_three, n}, {n_plus_one, n}, false},
        {"negative full products reverse",
         {"-" + n_plus_one, n},
         {"-" + n_plus_two, n_plus_one},
         true},
        {"negative common numerator reverses",
         {"-" + n_plus_one, n_plus_three},
         {"-" + n_plus_one, n_minus_one},
         false},
        {"negative common denominator reverses",
         {"-" + n_plus_three, n},
         {"-" + n_plus_one, n},
         true},
    };
    for (const auto &row : rows) {
        SCOPED_TRACE(row.description);
        const auto result = evaluate_effective_permission(input(row.requested, row.bound));
        const auto &data = result.data();
        EXPECT_EQ(data.outcome, row.permit ? PermissionOutcome::permit : PermissionOutcome::deny);
        EXPECT_EQ(data.reason, row.permit ? PermissionReason::individual_allow
                                          : PermissionReason::individual_limit);
        EXPECT_EQ(data.witness_row, row.permit ? std::optional<std::uint64_t>{0} : std::nullopt);
        ASSERT_EQ(data.sources.size(), 1U);
        EXPECT_EQ(data.sources[0].match,
                  row.permit ? PermissionMatch::complete : PermissionMatch::quantity_mismatch);
        EXPECT_EQ(data.sources[0].disposition, row.permit
                                                   ? PermissionSourceDisposition::permit_witness
                                                   : PermissionSourceDisposition::quantity_limit);
        ASSERT_EQ(data.request.quantities.size(), 1U);
        EXPECT_EQ(data.request.quantities[0].value.numerator, row.requested.numerator);
        EXPECT_EQ(data.request.quantities[0].value.denominator, row.requested.denominator);
    }
}

enum class CallResult { returned, typed_failure, raw_bad_alloc, other_exception };
struct Observed {
    CallResult result = CallResult::other_exception;
    std::optional<EffectivePermissionError> error;
    bool injected = false;
    std::size_t allocations = 0;
    std::optional<EffectivePermissionDecision> decision;
};

// Fixture/limits exist before arming. Nothing inside this scope calls GTest/logging,
// constructs a diagnostic, or invokes an external allocator other than the evaluator.
Observed call_with_failure(const EffectivePermissionInput &in,
                           const EffectivePermissionLimits &limits,
                           std::optional<std::size_t> allocation_index) {
    Observed observed;
    {
        allocation_fault::Scope fault(allocation_index);
        try {
            observed.decision.emplace(evaluate_effective_permission(in, limits));
            observed.result = CallResult::returned;
        } catch (const EffectivePermissionFailure &failure) {
            observed.result = CallResult::typed_failure;
            observed.error = failure.code();
        } catch (const std::bad_alloc &) {
            observed.result = CallResult::raw_bad_alloc;
        } catch (...) {
            observed.result = CallResult::other_exception;
        }
        observed.injected = allocation_fault::state.fired;
        observed.allocations = allocation_fault::state.observed;
    }
    return observed;
}

TEST(EffectivePermissionRegression, AllocationFailuresTranslateAndPreflightStaysAllocationFree) {
    // A strict permit with unequal long fractions requires numeric work; long owned
    // context/source fields also force real output allocation on ordinary C++ libraries.
    auto fixture = input({n_plus_two, n_plus_one}, {n_plus_one, n});
    fixture.actor = std::string(128, 'a');
    fixture.snapshot = std::string(128, 'p');
    fixture.catalog.revision = std::string(128, 'c');
    fixture.policy.entries[0].source = std::string(128, 's');
    fixture.policy.entries[0].grant = std::string(128, 'g');
    const auto expected_actor = fixture.actor;
    const auto expected_snapshot = fixture.snapshot;
    const auto expected_catalog = fixture.catalog.revision;
    const auto expected_source = fixture.policy.entries[0].source;
    const auto expected_grant = fixture.policy.entries[0].grant;
    const EffectivePermissionLimits limits;

    auto invalid = limits;
    invalid.result_items = 0;
    const auto invalid_result = call_with_failure(fixture, invalid, 0);
    EXPECT_FALSE(invalid_result.injected);
    EXPECT_EQ(invalid_result.allocations, 0U);
    EXPECT_EQ(invalid_result.result, CallResult::typed_failure);
    EXPECT_EQ(invalid_result.error, EffectivePermissionError::invalid_limits);
    auto oversized = limits;
    oversized.reference_bytes = 1;
    const auto oversized_result = call_with_failure(fixture, oversized, 0);
    EXPECT_FALSE(oversized_result.injected);
    EXPECT_EQ(oversized_result.allocations, 0U);
    EXPECT_EQ(oversized_result.result, CallResult::typed_failure);
    EXPECT_EQ(oversized_result.error, EffectivePermissionError::input_limit);

    // Observe only private allocator fault points, never derive a permission oracle.
    // No fixed count restricts a conforming finite allocation strategy.
    const auto unfailed = call_with_failure(fixture, limits, std::nullopt);
    ASSERT_EQ(unfailed.result, CallResult::returned);
    ASSERT_TRUE(unfailed.decision);
    EXPECT_FALSE(unfailed.injected);
    ASSERT_GT(unfailed.allocations, 0U);
    for (std::size_t index = 0; index < unfailed.allocations; ++index) {
        const auto observed = call_with_failure(fixture, limits, index);
        SCOPED_TRACE(index); // Hook is already disarmed before any assertion/trace.
        EXPECT_TRUE(observed.injected);
        EXPECT_EQ(observed.result, CallResult::typed_failure);
        EXPECT_EQ(observed.error, EffectivePermissionError::allocation_failure);
        EXPECT_FALSE(observed.decision);
    }
    const auto complete = *unfailed.decision; // Caller copy occurs with the hook disarmed.
    fixture.actor.clear();
    fixture.snapshot.clear();
    fixture.catalog.operations.clear();
    fixture.catalog.revision.clear();
    fixture.policy.entries.clear();
    fixture.request.quantities.clear();
    const auto &data = complete.data();
    EXPECT_EQ(data.actor, expected_actor);
    EXPECT_EQ(data.snapshot, expected_snapshot);
    EXPECT_EQ(data.catalog_revision, expected_catalog);
    EXPECT_EQ(data.outcome, PermissionOutcome::permit);
    EXPECT_EQ(data.reason, PermissionReason::individual_allow);
    EXPECT_EQ(data.witness_row, std::optional<std::uint64_t>{0});
    ASSERT_EQ(data.sources.size(), 1U);
    EXPECT_EQ(data.sources[0].source, expected_source);
    EXPECT_EQ(data.sources[0].grant, expected_grant);
    EXPECT_EQ(data.sources[0].match, PermissionMatch::complete);
    EXPECT_EQ(data.sources[0].disposition, PermissionSourceDisposition::permit_witness);
    EXPECT_TRUE(data.sources[0].failed_resources.empty());
    EXPECT_TRUE(data.sources[0].failed_quantities.empty());
    ASSERT_EQ(data.request.quantities.size(), 1U);
    EXPECT_EQ(data.request.quantities[0].value.numerator, n_plus_two);
    EXPECT_EQ(data.request.quantities[0].value.denominator, n_plus_one);
}
} // namespace
