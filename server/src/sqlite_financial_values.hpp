// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_milestone_detail.hpp"
#include <QDataStream>
#include <QIODevice>
#include <subathon/server/sqlite_application_plans.hpp>
#include <subathon/server/sqlite_financial_refunds.hpp>

namespace subathon::server::financial_sql {
using milestone_sql::blob;
using milestone_sql::corrupt;
using milestone_sql::integer;
using milestone_sql::reference;
constexpr qsizetype document_limit = 262144;
constexpr qsizetype component_limit = 16384;
constexpr qint64 collection_limit = 1024;

// Versioned private binary records. Each length is checked before allocation;
// the existing canonical exact-component codec owns rational normalization.
class Document {
  public:
    Document() : stream_(&data_, QIODevice::WriteOnly) { number(1); }
    explicit Document(const QByteArray &data) : data_(data), stream_(data_), reading_(true) {
        if (data.size() > document_limit || number() != 1)
            corrupt();
    }
    qint64 number(qint64 value = 0) {
        if (reading_)
            stream_ >> value;
        else
            stream_ << value;
        check();
        return value;
    }
    qint64 range(qint64 value, qint64 end) {
        value = number(value);
        if (value < 0 || value > end)
            corrupt();
        return value;
    }
    QByteArray bytes(QByteArray value = {}, qsizetype maximum = document_limit) {
        const auto size = number(value.size());
        if (size < 0 || size > maximum || size > document_limit - stream_.device()->pos())
            corrupt();
        if (reading_)
            value = QByteArray(size, Qt::Uninitialized);
        if ((reading_ ? stream_.readRawData(value.data(), size)
                      : stream_.writeRawData(value.constData(), size)) != size)
            corrupt();
        return value;
    }
    std::string string(const std::string &value = {}, qsizetype bound = 4096) {
        if (reading_)
            return bytes({}, bound).toStdString();
        if (value.size() > size_t(bound))
            corrupt();
        return bytes(QByteArray(value.data(), qsizetype(value.size())), bound).toStdString();
    }
    std::optional<std::string> optional(const std::optional<std::string> &value = {},
                                        qsizetype bound = 4096) {
        if (range(value.has_value(), 1) == 0)
            return {};
        return string(value.value_or(""), bound);
    }
    core::Exact exact(const core::Exact &value = 0) {
        if (reading_) {
            const auto n = bytes({}, component_limit);
            const auto d = bytes({}, component_limit);
            return decode_plan_exact({n, d}, component_limit);
        }
        const auto encoded = encode_plan_exact(value, component_limit);
        bytes(encoded.numerator, component_limit);
        bytes(encoded.denominator, component_limit);
        return value;
    }
    ApplicationOperationKey operation(const ApplicationOperationKey &key = {}) {
        return {string(key.operation_scope), string(key.operation_ref)};
    }
    ApplicationPlanKey plan(const ApplicationPlanKey &key = {}) {
        return {string(key.plan_ref), unsigned_number(key.revision)};
    }
    std::uint64_t unsigned_number(std::uint64_t value = 0) {
        if (reading_)
            return decode_plan_revision_uint64(bytes({}, 8));
        bytes(encode_plan_revision_uint64(value), 8);
        return value;
    }
    core::ReceiptLinkReference link(const core::ReceiptLinkReference &key = {}) {
        return {string(key.identifier_namespace), string(key.opaque_id)};
    }
    FinancialPaymentKey payment(const FinancialPaymentKey &key = {}) {
        return {{string(key.origin.provider), string(key.origin.source_account)},
                link(key.payment),
                {unsigned_number(key.support_receipt.value)}};
    }
    ApplicationPrincipalBinding principal(const ApplicationPrincipalBinding &value = {}) {
        return {ApplicationPrincipalKind(range(int(value.kind), 1)), string(value.installation_ref),
                string(value.principal_ref)};
    }
    ApplicationAudit audit(const ApplicationAudit &value = {}) {
        return {optional(value.actor_ref),
                optional(value.decision_ref),
                optional(value.reason, document_limit),
                {string(value.envelope.format_ref), bytes(value.envelope.document)}};
    }
    ApplicationHoldVersionLocator locator(const ApplicationHoldVersionLocator &value = {}) {
        return {string(value.hold_ref), string(value.version_ref)};
    }
    GroupedRefundEntitlement time(const GroupedRefundEntitlement &value = {}) {
        return {{operation(value.original.operation), number(value.original.request_ordinal),
                 ApplicationComponentKind(range(int(value.original.component_kind), 5))},
                number(value.request_ordinal),
                number(value.expected_basis_position),
                {number(value.previous.main), number(value.previous.restoration)},
                {number(value.cumulative.main), number(value.cumulative.restoration)},
                exact(value.exact_cumulative),
                exact(value.residual),
                number(value.main_request),
                number(value.restoration_request)};
    }
    RefundTimerOutcome timer(const RefundTimerOutcome &value = {}) {
        return {RefundTimerDisposition(range(int(value.disposition), 2)),
                {number(value.actual.main_removed), number(value.actual.restored),
                 number(value.actual.deficit_debt), number(value.actual.unapplied_main),
                 number(value.actual.unapplied_restoration)}};
    }
    std::vector<core::TimerTransition>
    transitions(const std::vector<core::TimerTransition> &value = {}) {
        const auto count = range(qint64(value.size()), collection_limit);
        std::vector<core::TimerTransition> result;
        for (qint64 i = 0; i < count; ++i)
            result.push_back(core::TimerTransition(range(reading_ ? 0 : int(value[size_t(i)]), 5)));
        return result;
    }
    QByteArray finish() {
        check();
        if ((reading_ && !stream_.atEnd()) || data_.size() > document_limit)
            corrupt();
        return data_;
    }

  private:
    void check() {
        if (stream_.status() != QDataStream::Ok)
            corrupt();
    }
    QByteArray data_;
    QDataStream stream_;
    bool reading_{false};
};

inline QByteArray payment_key(const FinancialPaymentKey &key) {
    Document doc;
    doc.string(key.origin.provider);
    doc.string(key.origin.source_account);
    doc.link(key.payment);
    return doc.finish();
}
inline QByteArray economic_key(const VerifiedRefundCorrelation &correlation) {
    Document doc;
    doc.string(correlation.payment.origin.provider);
    doc.string(correlation.payment.origin.source_account);
    doc.link(correlation.refund);
    return doc.finish();
}
inline QList<QVariant> operation_values(const ApplicationOperationKey &key) {
    return {blob(key.operation_scope), blob(key.operation_ref)};
}

} // namespace subathon::server::financial_sql
