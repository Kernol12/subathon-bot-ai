// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_financial_refunds.hpp>

namespace subathon::server {

struct LiveFinancialRefundFacts {
    ApplicationClockObservation clock;
    core::TimerConfig timer;
    std::string pause_recovery_revision_ref;
    bool paused{};
    bool recovery_pending{};
    // An engaged facts value declares the timer settings, including no cap or deadline.
    std::optional<std::int64_t> main_cap_ms;
    std::optional<core::RefundDeficitPolicy> deficit_policy;
    MilestoneTrackKey track;
    std::optional<MilestoneRefundPolicy> milestone_refund_policy;
};

class LiveFinancialRefundInputs {
  public:
    virtual ~LiveFinancialRefundInputs() = default;
    // Lazily select current input facts on the held writer head. Historical replay
    // does not acquire a fresh observation. Absence means the current facts are unavailable.
    [[nodiscard]] virtual std::optional<LiveFinancialRefundFacts>
    select(const SqliteWorkSession &, const ApplicationStateSnapshot &) = 0;
};

class LiveFinancialRefundNotImplemented final : public std::logic_error {
  public:
    explicit LiveFinancialRefundNotImplemented(const char *entry) : std::logic_error(entry) {}
};

[[nodiscard]] FinancialWorkReply stage_live_financial_refund(const SqliteWorkSession &,
                                                             const FinancialRefundCommand &,
                                                             const FinancialEntitlement &,
                                                             FinancialRefundAuthority &,
                                                             LiveFinancialRefundInputs &);
[[nodiscard]] FinancialWorkReply
stage_live_financial_timer_resolution(const SqliteWorkSession &, const FinancialTimerCommand &,
                                      FinancialRefundAuthority &, LiveFinancialRefundInputs &);

} // namespace subathon::server
