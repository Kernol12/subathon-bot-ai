// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/application_service.hpp>

namespace subathon::server {
std::optional<RetainedDocument>
ApplicationReferenceReadView::find_retained(const RetainedKey &) const {
    throw ApplicationReferenceReadNotImplemented{};
}
OriginalApplicationService::OriginalApplicationService(SqliteWriter &writer,
                                                       ApplicationServicePorts &ports,
                                                       ApplicationServiceLimits limits)
    : writer_(writer), ports_(ports), limits_(std::move(limits)) {}
ApplicationServicePreparedWork
OriginalApplicationService::make_work(const OriginalApplicationCommand &, Completion) const {
    throw OriginalApplicationServiceNotImplemented{};
}
ApplicationServiceAdmission OriginalApplicationService::enqueue(const OriginalApplicationCommand &,
                                                                Completion) {
    throw OriginalApplicationServiceNotImplemented{};
}
} // namespace subathon::server
