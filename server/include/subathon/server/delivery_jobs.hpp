// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/server/sqlite_application_bundle.hpp>
#include <utility>
#include <vector>

namespace subathon::server {

// Internal delivery boundary, proposed revision 1. A legacy bundle envelope has
// no delivery authority. This marker binds an opaque captured action document to
// typed capture data in the same transaction as the first application.
inline constexpr auto delivery_job_format = "subathon.internal.delivery-job.v1";

enum class DeliveryJobState {
    queued = 0,
    sending = 1,
    delivered = 2,
    failed = 3,
    outcome_unknown = 4
};
enum class DeliverySenderSelection { configured_default = 0, explicit_identity = 1 };

struct DeliveryJobKey {
    ApplicationOperationKey operation;
    std::string job_ref;
};

struct DeliveryCapture {
    std::string job_ref;
    std::string route_ref; // Stable external connection/destination, without credentials.
    DeliverySenderSelection selection{DeliverySenderSelection::configured_default};
    std::string sender_ref; // Resolved identity, including when the default was selected.
    std::string action_ref;
    std::string action_version_ref;
};

struct DeliveryJob {
    DeliveryJobKey key;
    DeliveryCapture capture;
    QByteArray action_document;
    DeliveryJobState state{DeliveryJobState::queued};
    std::int64_t revision{0}; // Starts at 1. Each accepted transition adds one, without wrap.
    std::optional<std::int64_t> attempt; // The revision at which this attempt entered sending.
    std::optional<std::string> evidence_ref;
};

// After selected-row validation, absence wins, then expected revision mismatch,
// then ineligible state. For active sending, an attempt mismatch is stale.
enum class DeliveryDisposition { changed, missing, stale, refused };
struct DeliveryJobResult {
    DeliveryDisposition disposition{DeliveryDisposition::refused};
    std::optional<DeliveryJob> job; // Present only on changed.
};

enum class DeliveryEvidence { delivered = 0, known_undelivered = 1, indeterminate = 2 };

// Both ports are trusted, bounded, synchronous decisions for the exact job
// supplied. They run on the owner thread and do not send, pump events, or call
// the writer. The caller owns real current authority and policy provenance.
class DeliveryDispatchAuthority {
  public:
    virtual ~DeliveryDispatchAuthority() = default;
    [[nodiscard]] virtual bool allows(const DeliveryJob &job) = 0;
};

class DeliveryRetryPolicy {
  public:
    virtual ~DeliveryRetryPolicy() = default;
    // Called only for failed jobs with retained known-undelivered evidence.
    // Return a retained policy decision reference, or absence to refuse.
    [[nodiscard]] virtual std::optional<std::string> allows(const DeliveryJob &job) = 0;
};

class DeliveryJobsNotImplemented final : public std::runtime_error {
  public:
    DeliveryJobsNotImplemented() : std::runtime_error("MILESTONE-003 delivery jobs NI") {}
};

class SqliteDeliveryJobStore final {
  public:
    explicit SqliteDeliveryJobStore(SqliteWorkSession session) : session_(std::move(session)) {}

    // Validate exact one-to-one captures for all marked jobs before SQL.
    // Uses the existing bundle stage on this session. Only a newly staged bundle
    // inserts queued rows. An existing operation with differing or missing capture
    // is a conflict and never backfills. Unmarked jobs are never eligible.
    // All successes are provisional. A storage failure after writes unwinds the
    // whole caller unit through the existing session, including bundle writes.
    [[nodiscard]] ApplicationBundleStageResult
    stage(const PreparedApplicationBundle &bundle,
          const std::vector<DeliveryCapture> &captures) const;

    // Selected operation-scoped job only. Unenrolled legacy rows are absent.
    // Malformed selected data fails closed. Refs are bounded nonempty byte
    // strings (at most 4096 bytes); payloads retain the bundle/writer limits.
    [[nodiscard]] std::optional<DeliveryJob> lookup(const DeliveryJobKey &key) const;

    // Exact expected revision and attempt required for an active sending job.
    // Immutable evidence and transition are written together. Unknown is held.
    [[nodiscard]] DeliveryJobResult finish(const DeliveryJobKey &key,
                                           std::int64_t expected_revision, std::int64_t attempt,
                                           DeliveryEvidence evidence,
                                           const std::string &evidence_ref) const;

    // Startup owner calls after the prior process is gone and before outbound
    // dispatch begins. A persisted sending row becomes outcome_unknown even if
    // the prior process might never have reached the adapter. No sending occurs.
    [[nodiscard]] DeliveryJobResult recover(const DeliveryJobKey &key,
                                            std::int64_t expected_revision) const;

    // Only failed with known non-delivery plus a current policy decision may
    // return to queued. The original capture and previous attempt stay retained.
    [[nodiscard]] DeliveryJobResult retry(const DeliveryJobKey &key, std::int64_t expected_revision,
                                          DeliveryRetryPolicy &policy) const;

  private:
    SqliteWorkSession session_;
};

class DeliveryAdapter {
  public:
    virtual ~DeliveryAdapter() = default;
    // Start once with owned committed data, on the owner thread. Return promptly.
    // Later evidence re-enters through SqliteWorkSession via finish().
    virtual void start(DeliveryJob claim) = 0;
};

struct DeliveryDispatchCompletion {
    SqliteWorkResult work;
    std::optional<DeliveryJobResult> result;
    bool adapter_threw{false}; // A thrown start does not establish non-delivery.
};

// Trusted entry for a single already-selected job. Reject invalid input before
// admission. On ordinary committed completion of queued -> sending, call the
// adapter once and then complete. No adapter call on denial, stale/corrupt data,
// rejected work, known rollback, unknown commit or abnormal completion. State is
// already durable when start is called. No reentrant dispatch from either port.
using DeliveryDispatchCallback = std::function<void(DeliveryDispatchCompletion)>;
[[nodiscard]] StorageError
dispatch_delivery_job(SqliteWriter &writer, DeliveryJobKey key, std::int64_t expected_revision,
                      std::shared_ptr<DeliveryDispatchAuthority> authority,
                      std::shared_ptr<DeliveryAdapter> adapter,
                      DeliveryDispatchCallback completion);

} // namespace subathon::server
