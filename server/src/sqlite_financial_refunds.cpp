// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_financial_refunds.hpp>

namespace subathon::server {
std::optional<FinancialPaymentHistory>
SqliteFinancialRefundStore::history(const FinancialPaymentKey &, qsizetype) const {
    throw SqliteFinancialRefundNotImplemented("read payment history");
}

std::optional<FinancialRefundRecord>
SqliteFinancialRefundStore::lookup(const ApplicationOperationKey &) const {
    throw SqliteFinancialRefundNotImplemented("read accepted refund closure");
}

std::optional<FinancialTimerResolution>
SqliteFinancialRefundStore::lookup_resolution(const ApplicationOperationKey &) const {
    throw SqliteFinancialRefundNotImplemented("read financial timer resolution closure");
}

std::vector<FinancialTimerHold>
SqliteFinancialRefundStore::active(const ApplicationHoldSubject &) const {
    throw SqliteFinancialRefundNotImplemented("read active financial timer holds");
}

FinancialWorkReply stage_financial_refund(const SqliteWorkSession &, const FinancialRefundCommand &,
                                          const FinancialEntitlement &, FinancialRefundAuthority &,
                                          FinancialRefundEffectPort &) {
    throw SqliteFinancialRefundNotImplemented("stage financial refund");
}

FinancialWorkReply stage_financial_timer_resolution(const SqliteWorkSession &,
                                                    const FinancialTimerCommand &,
                                                    FinancialRefundAuthority &,
                                                    FinancialRefundEffectPort &) {
    throw SqliteFinancialRefundNotImplemented("stage financial timer resolution");
}

FinancialWorkReply stage_financial_aware_linked_resolution(const SqliteWorkSession &,
                                                           const ApplicationHoldResolutionBatch &) {
    throw SqliteFinancialRefundNotImplemented("stage financial-aware linked resolution");
}

FinancialWorkReply stage_financial_aware_nonfinancial_reversal(const SqliteWorkSession &,
                                                               const PreparedApplicationBundle &) {
    throw SqliteFinancialRefundNotImplemented("stage financial-aware nonfinancial reversal");
}

std::optional<FinancialWorkReply> acknowledged_financial_reply(const SqliteWorkResult &) {
    throw SqliteFinancialRefundNotImplemented("classify acknowledged financial reply");
}
} // namespace subathon::server
