// SPDX-License-Identifier: GPL-3.0-or-later
#include "session_clock_host_internal.hpp"
#include <subathon/server/testing/session_clock_host.hpp>
#include <utility>

namespace subathon::server::session_clock {
namespace {
class UnavailableClock final : public CurrentSessionClock {
  public:
    SessionClockResolution sample_current() override { return SessionSourceUnavailable{}; }
};
std::shared_ptr<CurrentSessionClock> unavailable_clock() noexcept {
    static UnavailableClock value;
    // Non-null alias with no control block/allocation on refusal.
    return {std::shared_ptr<CurrentSessionClock>{}, &value};
}
class BoundClock final : public CurrentSessionClock {
  public:
    BoundClock(std::weak_ptr<detail::BindingState> state, std::weak_ptr<detail::ScopeState> scope)
        : state_(std::move(state)), scope_(std::move(scope)) {}
    SessionClockResolution sample_current() override {
        auto scope = scope_.lock();
        if (!scope || scope->owner != std::this_thread::get_id() || !scope->live.load())
            return SessionSourceUnavailable{};
        auto state = state_.lock();
        if (!state || !state->alive.load() || state->sampling || state->scope != scope)
            return SessionSourceUnavailable{};
        try {
            if (!state->eligible || !state->eligible(scope->record, scope->unit, scope->kind))
                return SessionSourceUnavailable{};
            state->sampling = true;
            struct Release {
                bool &value;
                ~Release() { value = false; }
            } release{state->sampling};
            if (!state->source)
                state->source = std::make_shared<Monitor>(linux_observations());
            const auto source = state->source; // keep dependency alive through synchronous sample
            auto result = source->sample_current();
            if (!state->alive.load() || !scope->live.load() || state->scope != scope ||
                !state->eligible(scope->record, scope->unit, scope->kind))
                return SessionSourceUnavailable{};
            return result;
        } catch (...) {
            return SessionSourceUnavailable{};
        }
    }

  private:
    std::weak_ptr<detail::BindingState> state_;
    std::weak_ptr<detail::ScopeState> scope_;
};
std::shared_ptr<CurrentSessionClock> bind(const std::shared_ptr<detail::BindingState> &state,
                                          std::weak_ptr<detail::ScopeState> scope) {
    if (scope.expired())
        return unavailable_clock();
    return std::make_shared<BoundClock>(state, std::move(scope));
}
} // namespace
struct HostClock::Impl {
    std::weak_ptr<detail::BindingState> state;
};
HostClock::HostClock(api_host::Host &host) : HostClock(host, {}) {}
HostClock::HostClock(api_host::Host &host, std::shared_ptr<CurrentSessionClock> source)
    : impl_(std::make_unique<Impl>()) {
    auto state = std::make_shared<detail::BindingState>();
    state->source = std::move(source);
    if (detail::HostBindingAccess::attach(host, state))
        impl_->state = state;
}
HostClock::~HostClock() = default;
std::shared_ptr<CurrentSessionClock> HostClock::bind(const SqliteWorkContext &context) {
    auto state = impl_->state.lock();
    if (!state || state->owner != std::this_thread::get_id() || !state->alive.load() ||
        state->sampling)
        return unavailable_clock();
    try {
        return session_clock::bind(state, detail::HostBindingAccess::body(*state, context));
    } catch (...) {
        return unavailable_clock();
    }
}
std::shared_ptr<CurrentSessionClock>
HostClock::bind_completion(const api_host::CommandHandle &command) {
    auto state = impl_->state.lock();
    if (!state || state->owner != std::this_thread::get_id() || !state->alive.load() ||
        state->sampling)
        return unavailable_clock();
    try {
        return session_clock::bind(state, detail::HostBindingAccess::completion(*state, command));
    } catch (...) {
        return unavailable_clock();
    }
}
} // namespace subathon::server::session_clock
namespace subathon::server::session_clock::testing {
std::unique_ptr<HostClock> HostClockAccess::create(api_host::Host &host,
                                                   std::shared_ptr<CurrentSessionClock> source) {
    return std::unique_ptr<HostClock>(new HostClock(host, std::move(source)));
}
} // namespace subathon::server::session_clock::testing
