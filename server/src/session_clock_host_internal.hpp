// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <functional>
#include <subathon/server/detail/session_clock_host.hpp>
#include <subathon/server/session_clock.hpp>
#include <thread>

namespace subathon::server::session_clock::detail {
enum class ScopeKind { body, completion };
struct ScopeState {
    const std::thread::id owner = std::this_thread::get_id();
    std::atomic<bool> live{true};
    ScopeKind kind;
    const void *record;
    std::uint64_t unit;
    std::weak_ptr<subathon::server::detail::SqliteWorkState> work;
    ScopeState(ScopeKind k, const void *r, std::uint64_t u) : kind(k), record(r), unit(u) {}
};
// Engine owns this single slot until destruction. No strong reference back to
// Engine, no history and no per-pending-unit allocation. Scope tokens are weak.
struct BindingState {
    const std::thread::id owner = std::this_thread::get_id();
    std::atomic<bool> alive{true};
    bool sampling = false;
    std::shared_ptr<CurrentSessionClock> source;
    std::shared_ptr<ScopeState> scope;
    std::function<bool(const void *, std::uint64_t, ScopeKind)> eligible;
};
} // namespace subathon::server::session_clock::detail
