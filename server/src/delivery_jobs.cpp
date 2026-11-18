// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/delivery_jobs.hpp>

namespace subathon::server {

ApplicationBundleStageResult
SqliteDeliveryJobStore::stage(const PreparedApplicationBundle &,
                              const std::vector<DeliveryCapture> &) const {
    throw DeliveryJobsNotImplemented("stage");
}

std::optional<DeliveryJob> SqliteDeliveryJobStore::lookup(const DeliveryJobKey &) const {
    throw DeliveryJobsNotImplemented("lookup");
}

DeliveryJobResult SqliteDeliveryJobStore::finish(const DeliveryJobKey &, std::int64_t, std::int64_t,
                                                 DeliveryEvidence, const std::string &) const {
    throw DeliveryJobsNotImplemented("finish");
}

StrandedDeliveryPage SqliteDeliveryJobStore::stranded(const std::optional<DeliveryJobKey> &,
                                                      std::size_t) const {
    throw DeliveryJobsNotImplemented("stranded");
}

DeliveryJobResult SqliteDeliveryJobStore::recover(const DeliveryJobKey &, std::int64_t) const {
    throw DeliveryJobsNotImplemented("recover");
}

DeliveryJobResult SqliteDeliveryJobStore::retry(const DeliveryJobKey &, std::int64_t,
                                                DeliveryRetryPolicy &) const {
    throw DeliveryJobsNotImplemented("retry");
}

StorageError dispatch_delivery_job(SqliteWriter &, DeliveryJobKey, std::int64_t,
                                   std::shared_ptr<DeliveryDispatchAuthority>,
                                   std::shared_ptr<DeliveryAdapter>, DeliveryDispatchCallback) {
    throw DeliveryJobsNotImplemented("dispatch_delivery_job");
}

} // namespace subathon::server
