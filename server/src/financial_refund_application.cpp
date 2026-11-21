// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/financial_refund_application.hpp>

namespace subathon::server {

FinancialWorkReply stage_live_financial_refund(const SqliteWorkSession &,
                                               const FinancialRefundCommand &,
                                               const FinancialEntitlement &,
                                               FinancialRefundAuthority &,
                                               LiveFinancialRefundInputs &) {
    throw LiveFinancialRefundNotImplemented("stage_live_financial_refund");
}

FinancialWorkReply stage_live_financial_timer_resolution(const SqliteWorkSession &,
                                                         const FinancialTimerCommand &,
                                                         FinancialRefundAuthority &,
                                                         LiveFinancialRefundInputs &) {
    throw LiveFinancialRefundNotImplemented("stage_live_financial_timer_resolution");
}

} // namespace subathon::server
