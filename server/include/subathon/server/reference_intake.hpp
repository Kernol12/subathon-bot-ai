// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/reference_runtime.hpp>

namespace subathon::server {
// API-011 public contract revision 3. Trusted internal queue composition only.
// No public HTTP mutation, amount/current facts/server-arrival input or owner flag.
struct ReferenceConnectorRegistration {
    std::string connection_ref;
    core::ReceiptOrigin origin;
    std::string identifier_namespace;
};
struct ReferenceInitialRequest {
    SessionLookupRequest owner_session;
    std::string configuration_ref, state_token, clock_lineage_ref, observation_ref;
    core::TimerState timer;
};
struct ReferenceInitialCompletion {
    SqliteWorkResult work;
    std::optional<ReferenceClockAnchor> anchor;
    std::optional<ReferenceUnavailable> refusal;
};
struct ReferenceReceiptCompletion {
    SqliteWorkResult work;
    std::optional<core::ReceiptIntakeResult> receipt;
    std::optional<ReferenceArrivalCapture> original;
    std::optional<ReferenceUnavailable> refusal;
};
struct ReferenceEvidenceCompletion {
    SqliteWorkResult work;
    std::optional<ContributionLinkResult> evidence;
    std::optional<ReferenceUnavailable> refusal;
};
struct ReferenceCaptureCompletion {
    SqliteWorkResult work;
    std::optional<ReferenceInitialCapture> capture;
    std::optional<ReferenceUnavailable> refusal;
};
struct ReferenceSnapshot {
    ReferenceCommittedState committed;
    std::optional<std::int64_t> maximum_main_ms;
};
struct ReferenceSnapshotCompletion {
    SqliteWorkResult work;
    std::optional<ReferenceSnapshot> snapshot;
    std::optional<ReferenceUnavailable> refusal;
};
// Matching prepared body/completion pair only. No completion can turn rolled-back,
// unknown or committed-body-error into normal typed data. Work carries ORIGINAL
// writer result/error; exceptions remain original (no storage-error relabelling).
struct ReferencePreparedWork {
    SqliteWriter::Work body;
    SqliteWriter::WorkCompletion completion;
};
// Structural constructors only. make_* preflight complete arguments+dependencies'
// selected profiles BEFORE copy/admission. Closures own bounded DATA, borrow the
// owner (must outlive pairs). Actual queue admission/error belongs to SqliteWriter.
// Each body BEGINs ONCE, never nested work/raw connection. Every returned work
// value remains tentative until actual committed/none/present NORMAL completion.
// On refusal after staging, whole cleanup required; unknown cleanup outranks local
// failure. Do not double-commit adopted receipt/evidence sessions. No pipeline-wide
// atomicity: earlier acknowledged units remain durable on later failure.
// initialize: actual same-session freshly resolved stored owner_session, trusted
// CurrentSessionClock and complete current graph owner/access.manage USE required;
// initialization forces require_fresh=true at actual session read (age<=300000ms);
// snapshot forces require_fresh=false, still checking live session/expiry/revocation.
// never trust raw account/owner boolean. Complete config/registry must be supported.
// fresh empty state namespace ONLY, no existing-head reset even identical startup.
// Actual fresh SAS1 absent basis, timer Active/Main positive/debt0/empty balances,
// actual separately sampled live UTC+monotonic. Initial SAS1 has zero segments,
// observed UTC equals that sample UTC; policy is admitted config.policy_revision_ref. Caller refs
// are bounded installation startup allocations, not external authority. Real retain_initial_state
// commit and normal completion BEFORE anchor/readiness. Not Start, reopen or recovery.
// receipt/evidence/capture require this process's actual acknowledged live anchor
// through borrowed live_runtime.current plus coherent current head, before new
// durable work; absent/reopened journal is
// not_ready, never reconstituted from state bytes. Startup returns the sole new
// live anchor; owner constructs runtime with that output AFTER normal completion.
// Intake borrows that runtime through later anchor advances, never a stale copy.
// receipt: validate connection registration against complete current config; actual
// queue clock supplies arrival UTC. retain original SRC1/SRA1 + adopt receipt service
// ONE commit. Fixed support/schema1, unknown provider/payment/activity remain absent.
// Payload is audit, not parsed amount. Retry new arrival retained, original receipt
// context/facts/config returned; conflict cannot create a new award or capability.
// allocation domains: context RCT1, event REV1, execution REX1, plan RPL1,
// each four ASCII bytes+32 entropy bytes. ONE draw per context/event pair shares
// its suffix; REV1 uniqueness is bound to successful RCT1 namespace retention,
// not an event annotation scan or probability. SRX uses retained context conflict;
// plan uses actual plan-header conflict. Any collision refuses the whole current
// unit, cleans staged rows, NEVER retries/draws a replacement. Earlier units stay.
// evidence: reread actual ORIGINAL receipt+retained context/config in same session;
// explicit nonfinancial not_applicable support only, adopted evidence ONE commit.
// capture: find first indexed source binding BEFORE new refs/plan/current-rule work;
// absent -> plan1+SRX1+index ONE caller transaction, noninserted conflict removes ALL
// attempted staged rows. Original index closure wins on retries/new operation keys.
// Indexed corrupt/dangling rows never absence; preserve ReferenceCaptureStorageFailure.
// application uses existing OriginalApplicationService as distinct FOURTH unit,
// actual five ports, reservation sampled immediately before execution. No second
// timer/state write here. Root composition owns FIFO wiring between these units.
// snapshot: read coherent head+SAS1+exact complete configuration; freshly resolve
// actual stored session and current ordinary timer.snapshot.read USE for timer T
// from actual complete graph/catalog/account facts, no owner/execute shortcut.
// Timer scope comes from current supported configuration. Auth before
// protected state lookup; committed normal read only, no journal fabrication.
// Resource rules: structural B/I in configuration header for ALL input/output/
// intermediate fields, presence bytes and recursive vectors; function/callback/
// dependency/limits objects uncharged. SessionLookupRequest B=installation+32/I0;
// live_runtime is a borrowed dependency, not payload/authority from input;
// registration B=connection+provider+account+namespace/I0; initialize input sums
// all request fields/timer; receipt input=config_ref+synthetic input; evidence=
// receipt ID8; capture=receipt ID8+CID8; snapshot=session lookup. make_* owns bounded
// input before admission. Local prepared-pair slots <=pending_reservations;
// overflow throws ReferenceInputFailure(input_limit) before copy/payload ownership,
// sampling or entropy. Shared pair lease releases exactly once on completion or
// abandonment of BOTH closures (copies retain lease). Writer queue admission is
// exclusively SqliteWriter enqueue/error, not predicted by make_* with no writer
// dependency; writer.pending_batches remains its independent limit.
// Qt QVariant returned work values count HELD typed semantic payload, not ABI.
// Successful snapshot body holds ReferenceSnapshot in work.value (no hidden
// additional envelope); its duplicate output is charged AGAIN in completion.
// Completion includes actual work (outcome1,error1,optional1+held value), optional
// outputs/refusals (1+body each). Nested SQL/store/service profiles retain all
// independent rows, bound bytes, component, document, vector and result ceilings.
// Whole new envelope includes format bytes; ALL codec intermediates independently
// fit values.document_bytes/value_owned_bytes/value_items BEFORE allocation/hash.
// Options positive/representable/container n+1 -> complete B/I/fields -> tags ->
// refs -> shapes -> numbers -> bindings -> affinity/capability -> actual auth ->
// profile -> output projection -> allocation; no copy/SQL/core work before pure
// preflight. Own allocation -> ReferenceInputFailure(allocation_failure); ORIGINAL
// SQL/capability/adopted-service failures preserved including lifetimes/cleanup.
// Profile/auth/conflict refusals explicitly roll back: ORIGINAL work.error is
// not_committed and work.value absent (writer-transactions-v1 finish rules).
// Only the bounded refusal diagnostic may be retained by the matching pair and
// released after KNOWN rollback; anchor/receipt/evidence/capture/snapshot stay absent.
// Unknown cleanup or body exception suppresses this diagnostic as well; never
// rewrite writer error/outcome or synthesize a normal result. Reasons:
// denied reference.access-denied.v1; existing head reference.initial-conflict.v1;
// context/CID conflict reference.collision.v1; runtime reference.runtime-unavailable.v1;
// missing/unknown config reference.configuration-unavailable.v1; known unsupported
// config reference.configuration-unsupported.v1. Malformed source facts remain
// typed failures; retained SRC1 contradiction is ReferenceExecutionFailure(invalid_data).
// Project the COMPLETE prospective success completion BEFORE COMMIT. On a local
// adapter completion overage, explicitly roll back: the ORIGINAL writer result is
// rolled_back/not_committed, NOT a fabricated sticky WorkError::result_limit.
// No work value or normal typed output survives. The matching pair may retain ONLY
// bounded ReferenceUnavailable{not_ready,"reference.result-limit.v1"} diagnosis,
// subject to the SAME complete diagnostic-completion and field ceilings. If it
// cannot fit, omit the diagnosis, never the original work outcome/error. Unknown
// cleanup/body error suppresses diagnosis as above. Actual writer SQL-result-limit
// failures keep their original tag; never issue dummy SQL to manufacture that tag.
// SQL source audit must prove selected bounded queries, cumulative budgets/no
// reset, old error truth and original transaction ownership. No partial publication.
class ReferenceIntake final {
  public:
    ReferenceIntake(ReferenceConfigurationSource &, ReferenceClockSampler &, ReferenceEntropy &,
                    CurrentSessionClock &, ReferenceConnectorRegistration,
                    ReferenceApplicationAdapters *live_runtime = nullptr,
                    ReferenceRuntimeLimits = {});
    [[nodiscard]] ReferencePreparedWork
    make_initial_work(const ReferenceInitialRequest &,
                      std::function<void(ReferenceInitialCompletion)>) const;
    [[nodiscard]] ReferencePreparedWork
    make_receipt_work(const std::string &configuration_ref, const ReferenceSyntheticInput &,
                      std::function<void(ReferenceReceiptCompletion)>) const;
    [[nodiscard]] ReferencePreparedWork
        make_evidence_work(core::ReceiptId, std::function<void(ReferenceEvidenceCompletion)>) const;
    [[nodiscard]] ReferencePreparedWork
        make_capture_work(core::ReceiptId, core::ContributionId,
                          std::function<void(ReferenceCaptureCompletion)>) const;
    [[nodiscard]] ReferencePreparedWork
    make_snapshot_work(const SessionLookupRequest &,
                       std::function<void(ReferenceSnapshotCompletion)>) const;

  private:
    ReferenceConfigurationSource &configuration_;
    ReferenceClockSampler &clock_;
    ReferenceEntropy &entropy_;
    CurrentSessionClock &sessions_;
    ReferenceConnectorRegistration connector_;
    ReferenceApplicationAdapters *runtime_;
    ReferenceRuntimeLimits limits_;
};
} // namespace subathon::server
