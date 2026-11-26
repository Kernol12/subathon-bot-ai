// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/connector_intake_start.hpp>

namespace subathon::server::connector_intake {

// Stage-one declarations for the trusted history boundary. Candidate DTOs
// are checked separately from observations supplied by the registered adapter.
struct AdmittedHistoryRequest {
    HistoryRequest request;
    HistoryPurpose purpose;
    QByteArray cursor_contract, intent;
    std::optional<QByteArray> gap;
    qint64 operation_revision, gap_revision;
};

enum class Membership { member, outside, unknown };
struct HistoryEvidence {
    QList<Membership> membership;
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
};
using CutWitness = std::function<bool(quint64, const CutObservation &)>;

struct RetainedPageRequest;

struct HistoryProfile {
    Scope scope;
    QByteArray revision, cursor_contract, generation;
    ConnectorHistoryAdapter &adapter;
    std::function<std::optional<HistoryEvidence>(const PageInvocation &)> validate;
    std::function<std::optional<Page>(const RetainedPageRequest &)> reconstruct{};
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
    QList<Membership> membership;
    QList<std::optional<VerifiedActivityLink>> activities;
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
using EnvelopeWitness = std::function<std::optional<QByteArray>(const QList<ObservedCut> &)>;
struct HandoffOutcome {
    IntakeStatus status{IntakeStatus::not_implemented};
    std::optional<SqliteWorkResult> work;
};
[[nodiscard]] HandoffOutcome
commit_history_handoff(SqliteWriter &, const OwnerInputs &, const QList<const HistoryProfile *> &,
                       const AdmittedHandoff &, const QList<CutObservation> &,
                       const EnvelopeWitness &, const std::function<bool()> &before_commit = {});

[[nodiscard]] IntakeStatus open_history_import(SqliteWriter &, const OwnerInputs &,
                                               const HistoryProfile &, const AdmittedHistoryCut &,
                                               const CutObservation &, const CutWitness &);
[[nodiscard]] PageQualification qualify_history_page(SqliteWriter &, const OwnerInputs &,
                                                     const HistoryProfile &,
                                                     const AdmittedHistoryRequest &, const Page &);
[[nodiscard]] PageQualification reconstruct_history_page(SqliteWriter &, const OwnerInputs &,
                                                         const HistoryProfile &,
                                                         const RetainedPageRequest &);
[[nodiscard]] PageOutcome bind_history_page(SqliteWriter &, const OwnerInputs &,
                                            const QualifiedPage &);
[[nodiscard]] PageOutcome finalize_history_page(SqliteWriter &, const OwnerInputs &,
                                                const QualifiedPage &);
} // namespace subathon::server::connector_intake
