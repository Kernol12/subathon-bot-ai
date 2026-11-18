// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
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

// Internal delivery boundary, proposed revision 1, review round 1.
// This marker binds a captured action document to typed capture data in the
// first application transaction. A legacy envelope carries no delivery authority.
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

struct StrandedDeliveryAttempt {
    DeliveryJobKey key;
    std::int64_t sending_revision{0};
};

struct StrandedDeliveryPage {
    std::vector<StrandedDeliveryAttempt> attempts;
    std::optional<DeliveryJobKey> next_after;
};

inline constexpr std::size_t stranded_delivery_page_limit = 8;

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
// the writer. The caller must prove the decision from current authoritative
// sender/permission or policy state and bind it to the exact job and attempt
// where applicable. An owner-thread snapshot may be used only if its
// freshness is established in this turn and it cannot change before start.
// A caller without that provenance must deny. The port conveys no SQL access.
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
    explicit DeliveryJobsNotImplemented(const char *entry)
        : std::runtime_error(std::string("MILESTONE-003 delivery jobs NI: ") + entry) {}
};

class SqliteDeliveryJobStore final {
  public:
    explicit SqliteDeliveryJobStore(SqliteWorkSession session) : session_(std::move(session)) {}

    // Validate exact one-to-one captures for all marked jobs before SQL.
    // Missing/extraneous/duplicate captures, invalid sender selection and any
    // empty or over-4096-byte capture ref throw SqliteBundleInputFailure
    // (invalid_job), without SQL, writes or ending the caller session.
    // Uses the existing bundle stage on this session. Only a newly staged bundle
    // inserts queued rows. An existing operation with differing or missing
    // capture returns operation_conflict with no existing payload or backfill.
    // Unmarked jobs are never eligible.
    // All successes are provisional. A storage failure after writes unwinds the
    // whole caller unit through the existing session, including bundle writes.
    [[nodiscard]] ApplicationBundleStageResult
    stage(const PreparedApplicationBundle &bundle,
          const std::vector<DeliveryCapture> &captures) const;

    // Selected operation-scoped job only. Missing enrollment is absent.
    // A selected enrollment with missing/contradictory marker, latest event,
    // immediate predecessor or attempt link fails with SqliteBundleStorageFailure
    // (invalid_stored_data) after known whole-unit rollback. This also applies
    // to mutation/discovery reads. Unknown or protocol failures retain their
    // WorkFailure category. This is bounded local validation of the selected
    // state, not a full historical audit. Refs are nonempty, at most 4096 bytes.
    [[nodiscard]] std::optional<DeliveryJob> lookup(const DeliveryJobKey &key) const;

    // Exact expected revision and attempt required for an active sending job.
    // Immutable evidence and transition are written together. Unknown is held.
    [[nodiscard]] DeliveryJobResult finish(const DeliveryJobKey &key,
                                           std::int64_t expected_revision, std::int64_t attempt,
                                           DeliveryEvidence evidence,
                                           const std::string &evidence_ref) const;

    // Ordinary startup discovery. Scan the capture PK in BLOB tuple order
    // strictly after the optional last-examined key. Examine at most limit
    // captures with a bounded indexed latest-event probe per key. Return only
    // latest sending keys/revisions in traversal order, without action bodies.
    // A full page returns its last examined key even if no attempts matched.
    // A short page returns no continuation. Exact-full exhaustion may require
    // one empty page. Invalid cursor/limit throw SqliteBundleInputFailure
    // (invalid_key/invalid_options) before SQL. Limit is 1..8. The result
    // remains provisional until the caller's normal committed completion.
    [[nodiscard]] StrandedDeliveryPage stranded(const std::optional<DeliveryJobKey> &after,
                                                std::size_t limit) const;

    // Startup owner must establish exclusive store ownership after the old
    // process and its delivery workers are gone. Keep capture membership stable
    // and dispatch, finish and retry quiescent throughout the sweep. Use the
    // discovered key and revision. Recheck latest state before recording outcome_unknown
    // and retaining the attempt. This pair provides no epoch or process fence.
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
    // A thrown start leaves changed/committed sending intact. Dispatch catches
    // the exception and calls completion once with adapter_threw set.
    bool adapter_threw{false};
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
