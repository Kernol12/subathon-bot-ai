// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "permission_engine.hpp"
#include <memory>

namespace subathon::core::permission_engine {
// Per-call accounting, with nested scopes restored and no shared cross-thread state.
// All scratch vectors use exact reserve; the allocator also checks transient
// replacement storage before allocation. DTO/result allocations use normal owners.
struct ScratchBudget {
    std::size_t limit;
    std::size_t used = 0;
};
inline thread_local ScratchBudget *current_scratch = nullptr;
struct ScratchScope {
    ScratchBudget budget;
    ScratchBudget *previous;
    explicit ScratchScope(std::size_t input_items)
        : budget{128 * input_items}, previous(current_scratch) {
        current_scratch = &budget;
    }
    ~ScratchScope() { current_scratch = previous; }
    ScratchScope(const ScratchScope &) = delete;
    ScratchScope &operator=(const ScratchScope &) = delete;
};
template <class T> struct ScratchAllocator {
    using value_type = T;
    ScratchBudget *budget = current_scratch;
    ScratchAllocator() = default;
    template <class U> ScratchAllocator(const ScratchAllocator<U> &other) : budget(other.budget) {}
    T *allocate(std::size_t n) {
        if (!budget || n > (budget->limit - budget->used) / sizeof(T))
            fail(Error::input_limit);
        auto *p = std::allocator<T>{}.allocate(n);
        budget->used += n * sizeof(T);
        return p;
    }
    void deallocate(T *p, std::size_t n) noexcept {
        budget->used -= n * sizeof(T);
        std::allocator<T>{}.deallocate(p, n);
    }
    template <class U> bool operator==(const ScratchAllocator<U> &other) const {
        return budget == other.budget;
    }
};
template <class T> using ScratchVector = std::vector<T, ScratchAllocator<T>>;
} // namespace subathon::core::permission_engine
