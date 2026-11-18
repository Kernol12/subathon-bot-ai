// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <memory>
#include <subathon/server/api_host.hpp>

namespace subathon::server::session_clock::detail {
struct BindingState;
struct ScopeState;
// PRIVATE product integration contract (C9-2 option A); no public Host operation.
// Stage2 definitions live in api_host.cpp, where Engine/Record/handle Impl exist.
// Only this friend can compare SqliteWorkContext::state_ identity with the context
// captured at actual Engine::forward body entry. Nonexpired identity is necessary
// but NEVER sufficient without current Engine/Record/body scope membership.
class HostBindingAccess final {
  public:
    // Owner-only, once per Host LIFETIME, including before start. Engine retains
    // exactly one strong BindingState slot and a never-reset claimed bit. A second
    // HostClock is inert: no source read, second monitor or descriptor creation.
    // Destroying/replacing a wrapper cannot reset the first monitor/continuity floor.
    static bool attach(api_host::Host &, const std::shared_ptr<BindingState> &);
    // These queries are owner-only AFTER immutable owner/lifetime/stop checks.
    // Body context must match THIS Engine's current Record + unit + live writer
    // weak control block. Completion handle must match THIS Record/Engine, actual
    // last accepted unit (pending==0), normal committed/none/value result and no
    // withheld/terminal/stop condition. No supplied result, ID or readiness is used.
    // Called ONLY by Engine's real wrappers: Record* is a borrowed identity (not
    // authority), owned by Engine for this scope. Implementations recheck active
    // Record/unit membership; completion derives pending/withheld from Engine,
    // never a supplied last/ready flag. Result is the real wrapper's writer result.
    static void enter_body(BindingState &, const void *record, std::uint64_t unit,
                           const SqliteWorkContext &);
    static void enter_completion(BindingState &, const void *record, std::uint64_t unit,
                                 const SqliteWorkResult &);
    static void leave_scope(BindingState &) noexcept;
    static void revoke(BindingState &) noexcept;
    static std::weak_ptr<ScopeState> body(const BindingState &, const SqliteWorkContext &);
    static std::weak_ptr<ScopeState> completion(const BindingState &,
                                                const api_host::CommandHandle &);
};
// Stage2 shared private layout/operations belong in src/session_clock_host_internal.hpp.
// Engine owns only ONE current ScopeState (body OR completion), never a history or
// secondary queue. Bound clocks retain weak tokens, with immutable owner identity
// and atomic revocation checked BEFORE accessing Engine/writer/source, including
// foreign calls and calls after Host destruction. Reentrancy is refused before read.
// Engine::forward wrappers open the scope immediately before body/done, revoke on
// BOTH normal and exceptional exit, and never pump events. Stop, end/finish and
// Host::~Host revoke before cleanup; host destruction also invalidates BindingState.
// BindingState owns one lazy production Monitor/source through Host lifetime; all
// old scopes remain revoked. Clock shutdown revokes first, then closes its one fd.
// No SQL query, transaction, writer protocol, public Host API, observer event,
// queue/deadline/capture limit, generation or publication stage is introduced.
} // namespace subathon::server::session_clock::detail
