// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/application_service.hpp>
namespace subathon::server::state_codec {
// One SAS1 grammar. Writers first run in sizing mode; readers scan before copying.
template <class W>
void write(W &w, const ApplicationStateSnapshot &s, const std::optional<std::string_view> &basis,
           const ApplicationClockObservation &c, std::string_view policy) {
    w.raw("SAS1");
    w.b(s.configuration_ref);
    w.u8(bool(basis));
    if (basis)
        w.b(*basis);
    w.b(c.clock_lineage_ref);
    w.b(c.observation_ref);
    w.i64(c.observed_utc_ms);
    w.vec(c.segments, [&](const auto &x) {
        w.i64(x.elapsed_ms);
        w.i64(x.utc_now_ms);
        w.u8(x.paused);
    });
    w.b(policy);
    w.u8(unsigned(s.timer.lifecycle));
    w.u8(unsigned(s.timer.phase));
    w.i64(s.timer.main_remaining_ms);
    w.i64(s.timer.grace_remaining_ms);
    w.u8(s.timer.awaiting_confirmation);
    w.i64(s.time_debt_ms);
    w.vec(s.balances, [&](const auto &x) {
        w.u8(unsigned(x.kind));
        w.b(x.target_ref);
        w.i64(x.units);
    });
}
struct Meaning {
    std::string configuration;
    core::TimerState timer;
    std::int64_t debt{0};
    std::vector<ApplicationBalance> balances;
    std::optional<std::string> basis;
    ApplicationClockObservation clock;
    std::string policy;
};
// Called only after a caller-specific, allocation-free whole-document scan.
template <class R, class Clock, class Timer>
Meaning read(R &r, Clock clock, Timer timer, std::size_t balance_limit) {
    Meaning m;
    m.configuration = r.reference();
    m.basis = r.opt([&] { return r.reference(); });
    m.clock = clock(r);
    m.policy = r.reference();
    m.timer = timer(r);
    m.debt = r.i64();
    auto n = r.count(balance_limit);
    for (std::size_t i = 0; i < n; ++i)
        m.balances.push_back({ApplicationBalanceKind(r.u8(3)), r.reference(), r.i64()});
    r.end();
    return m;
}
} // namespace subathon::server::state_codec
