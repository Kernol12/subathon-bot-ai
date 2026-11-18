// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/delivery_jobs.hpp>

namespace subathon::server {

ApplicationBundleStageResult
SqliteDeliveryJobStore::stage(const PreparedApplicationBundle &,
                              const std::vector<DeliveryCapture> &) const {
    throw DeliveryJobsNotImplemented();
}

std::optional<DeliveryJob> SqliteDeliveryJobStore::lookup(const DeliveryJobKey &) const {
    throw DeliveryJobsNotImplemented();
}

DeliveryJobResult SqliteDeliveryJobStore::finish(const DeliveryJobKey &, std::int64_t, std::int64_t,
                                                 DeliveryEvidence, const std::string &) const {
    throw DeliveryJobsNotImplemented();
}

DeliveryJobResult SqliteDeliveryJobStore::recover(const DeliveryJobKey &, std::int64_t) const {
    throw DeliveryJobsNotImplemented();
}

DeliveryJobResult SqliteDeliveryJobStore::retry(const DeliveryJobKey &, std::int64_t,
                                                DeliveryRetryPolicy &) const {
    throw DeliveryJobsNotImplemented();
}

StorageError dispatch_delivery_job(SqliteWriter &, DeliveryJobKey, std::int64_t,
                                   std::shared_ptr<DeliveryDispatchAuthority>,
                                   std::shared_ptr<DeliveryAdapter>, DeliveryDispatchCallback) {
    return StorageError::feature_not_implemented;
}

} // namespace subathon::server
