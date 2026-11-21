// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_financial_values.hpp"

#include <subathon/server/financial_refund_application.hpp>

namespace subathon::server::financial_live {

// This is the observation portion of a private v2 journal. The selected state
// and all transitions are derived from it, both before writing and on reread.
struct Evidence {
    ApplicationClockObservation clock;
    core::TimerConfig config;
    std::string pause_recovery_revision;
    bool paused{};
    core::RefundDeficitPolicy policy{};
    std::optional<std::int64_t> cap;
    std::size_t observation_transitions{};
};

struct Observation {
    Evidence evidence;
    core::TimerState timer;
    std::vector<core::TimerTransition> transitions;
    MilestoneTrackKey track;
    std::optional<MilestoneRefundPolicy> milestone_policy;
};

std::optional<Observation> observe(const SqliteWorkSession &, const ApplicationStateSnapshot &,
                                   LiveFinancialRefundInputs &);
RefundTimerActual timer_effect(const GroupedRefundEntitlement &, const core::TimerState &,
                               std::int64_t debt, core::RefundDeficitPolicy);
std::optional<std::vector<FinancialMilestoneTarget>>
milestone_crossings(const SqliteWorkSession &, const Observation &,
                    const ApplicationPrincipalBinding &, const std::string &configuration,
                    std::int64_t before_score, std::int64_t after_score);

void bind_envelope(ApplicationStateSnapshot &, const Evidence &);
void append_effect_transitions(ApplicationStateSnapshot &, const RefundTimerActual &,
                               const Evidence &, std::vector<core::TimerTransition> &);
void write_evidence(financial_sql::Document &, const Evidence &);
std::optional<Evidence> read_evidence(financial_sql::Document &);
void validate(const Evidence &, const ApplicationStateSnapshot &before,
              const ApplicationStateSnapshot &advanced, const ApplicationStateSnapshot &after,
              const RefundTimerActual &, const std::vector<core::TimerTransition> &,
              bool balances_may_change = true);

} // namespace subathon::server::financial_live
