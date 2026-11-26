// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/connector_intake_start.hpp>

namespace subathon::server::connector_intake {

// Trusted history boundary on the serialized owner queue. Candidate DTOs
// are checked separately from observations supplied by the registered adapter.
struct AdmittedHistoryRequest {
    HistoryRequest request;
    HistoryPurpose purpose;
    QByteArray cursor_contract, intent;
    std::optional<QByteArray> gap;
    qint64 operation_revision, gap_revision;
    QByteArray registry_revision;
};

struct HistoryPageCandidate {
    Page page;
    HistoryPurpose purpose;
    std::optional<QByteArray> gap;
};

enum class Membership { member, outside, unknown };
enum class Overlap { unresolved, canonical_identity, verified_activity, disjoint };
struct SlotEvidence {
    Membership membership;
    Overlap overlap;
    std::optional<core::ReceiptIdentity> receipt_identity{};
    std::optional<core::ReceiptIdentity> canonical_identity{};
    std::optional<VerifiedActivityLink> activity{};
    QByteArray disjointness{};
    bool operator==(const SlotEvidence &) const = default;
};
struct HistoryEvidence {
    QList<SlotEvidence> slot_evidence;
    bool order_proved;
    std::optional<RangeProof> range;
    QList<QuarantineEntry> quarantine{};
};

struct PageInvocation {
    quint64 identity;
    const ConnectorHistoryAdapter &adapter;
    const AdmittedHistoryRequest &binding;
    const Page &observed;
};

struct AdmittedHistoryCut {
    CutRequest request;
    OperationIntent intent;
    QByteArray cursor_contract;
    Fence predecessor;
};
using CutWitness = std::function<bool(quint64, const CutObservation &)>;

struct GapInvocation {
    quint64 identity;
    const StartupProfile &profile;
    const GapOpening &binding;
};
struct GapEvidence {
    QByteArray observation;
    std::optional<qint64> observed_utc_ms;
};
using GapProducer = std::function<std::optional<GapEvidence>(const GapInvocation &)>;

struct RetainedPageRequest;
struct RetainedHistoryPage {
    Page page;
    HistoryEvidence evidence;
};

struct HistoryProfile {
    StartupProfileRegistry &registry;
    std::shared_ptr<const StartupProfile> snapshot;
    std::function<std::optional<HistoryEvidence>(const PageInvocation &)> validate;
    std::function<std::optional<RetainedHistoryPage>(const RetainedPageRequest &)> reconstruct{};
    mutable quint64 page_invocations{}, cut_invocations{}, gap_invocations{};
};

struct RetainedPageRequest {
    AdmittedHistoryRequest original;
    AdmittedHistoryRequest current;
    QByteArray page, reconnect;
};

class QualifiedPage;
struct PageQualification {
    IntakeStatus status{IntakeStatus::not_implemented};
    std::shared_ptr<const QualifiedPage> qualified;
    std::optional<HistoryFailure> adapter_failure;
    std::optional<HistoryStatus> storage_status;
    QList<SlotEvidence> slot_evidence;
};

struct PageOutcome {
    IntakeStatus status{IntakeStatus::not_implemented};
    std::optional<HistoryStatus> storage_status;
    std::optional<qint64> prefix;
};

struct AdmittedHandoff {
    SourceChange change;
    QList<CutRequest> selected;
    QList<QByteArray> prior_cuts;
};
struct ObservedCut {
    quint64 invocation;
    const ConnectorHistoryAdapter *adapter;
    CutObservation observation;
};
struct OldEnvelope {
    ObservedCut source;
    QByteArray eligible_upper;
    Fence through;
    bool includes_pending_and_outstanding;
    quint64 attempt;
    std::optional<QByteArray> legacy_eligible_first{};
};
struct NewSide {
    ObservedCut source;
    QByteArray exclusive_lower, first;
    quint64 attempt;
};
using OldEnvelopeWitness = std::function<std::optional<OldEnvelope>(const ObservedCut &)>;
using NewSideWitness = std::function<std::optional<NewSide>(const ObservedCut &)>;
using EnvelopeWitness =
    std::function<std::optional<QByteArray>(const OldEnvelope &, const std::optional<NewSide> &)>;
struct HandoffValidators {
    OldEnvelopeWitness old_side;
    NewSideWitness new_side;
    EnvelopeWitness relation;
};
struct HandoffOutcome {
    IntakeStatus status{IntakeStatus::not_implemented};
    std::optional<SqliteWorkResult> work;
};
[[nodiscard]] HandoffOutcome
commit_history_handoff(SqliteWriter &, const OwnerInputs &, const QList<const HistoryProfile *> &,
                       const AdmittedHandoff &, const QList<CutObservation> &,
                       const HandoffValidators &, const std::function<bool()> &before_commit = {});

[[nodiscard]] IntakeStatus open_history_gap(SqliteWriter &, const OwnerInputs &,
                                            const HistoryProfile &, const GapOpening &,
                                            const OpenGap &, const GapProducer &);
[[nodiscard]] IntakeStatus begin_history_catch_up(SqliteWriter &, const OwnerInputs &,
                                                  const HistoryProfile &,
                                                  const AdmittedHistoryCut &,
                                                  const CutObservation &, const CutWitness &);
[[nodiscard]] IntakeStatus open_history_import(SqliteWriter &, const OwnerInputs &,
                                               const HistoryProfile &, const AdmittedHistoryCut &,
                                               const HistoryRequest &, const CutObservation &,
                                               const CutWitness &);
[[nodiscard]] PageQualification qualify_history_page(SqliteWriter &, const OwnerInputs &,
                                                     const HistoryProfile &,
                                                     const AdmittedHistoryRequest &,
                                                     const HistoryPageCandidate &);
[[nodiscard]] PageQualification reconstruct_history_page(SqliteWriter &, const OwnerInputs &,
                                                         const HistoryProfile &,
                                                         const RetainedPageRequest &);
[[nodiscard]] PageOutcome bind_history_page(SqliteWriter &, const OwnerInputs &,
                                            const QualifiedPage &);
[[nodiscard]] PageOutcome finalize_history_page(SqliteWriter &, const OwnerInputs &,
                                                const QualifiedPage &);
} // namespace subathon::server::connector_intake
