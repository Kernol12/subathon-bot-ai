// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/core/application_plan.hpp>
#include <subathon/server/sqlite_connector_intake.hpp>

#include <QDate>
#include <variant>

namespace subathon::server::connector_intake {

enum class CatchUpMode { review, automatic, record_without_credit };
enum class SourceTimeQuality { reliable, unreliable, absent };
enum class CutPurpose { first_connect, import_upper, gap_upper, new_only_handoff, legacy_adoption };
enum class Authorization { allowed, denied, unavailable };
enum class Action { baseline, import, gap, catch_up, live, page, inspect, resume };
enum class IntakeStatus {
    not_implemented,
    committed,
    exact_replay,
    baseline_only,
    pending_review,
    record_only,
    pending_original,
    linked,
    held,
    applied,
    denied,
    unavailable,
    invalid_input,
    stale,
    conflict,
    busy,
    unproved,
    unsupported,
    invalid_import_date,
    limit_exceeded,
    quarantine_full
};

struct Selection {
    Scope scope;
    Fence fence;
    QByteArray profile;
};

struct CutRequest {
    Selection source;
    CutPurpose purpose;
    std::optional<QByteArray> prior_frontier;
};

struct CutObservation {
    CutRequest request;
    QualifiedCut cut;
    Endpoint boundary;
};

enum class HistoryFailure { unknown_capability, unsupported, temporarily_unavailable, unproved };
using CutResult = std::variant<CutObservation, HistoryFailure>;

struct BeginHistory {
    QByteArray key;
    HistoryPurpose purpose;
    std::optional<QByteArray> gap;
    std::optional<QDate> local_date;
    QByteArray zone_id, zone_basis;
};

struct Item {
    QByteArray key, role, identifier_namespace;
    std::optional<QByteArray> identifier;
    core::ReceiptFacts facts;
    QByteArray position, membership_proof, disjointness_proof;
    std::optional<VerifiedActivityLink> activity;
    SourceTimeQuality source_time;
    core::PaymentEvidence evidence;
    ContributionObservationKind kind{ContributionObservationKind::support};
    QByteArray award_class{"individual"};
    std::optional<QByteArray> aggregate_metric{};
    QByteArray normalized;
};

struct HistoryRequest {
    Selection source;
    QByteArray operation;
    qint64 ordinal;
    std::optional<QByteArray> continuation, frontier;
    HistoryInterval interval;
    QualifiedCut upper;
    std::optional<qint64> resolved_start_ms;
};

struct RangeProof {
    Coverage meaning;
    QByteArray lower, upper, proof, loss_reason;
};

struct Page {
    HistoryRequest request;
    QList<Item> items;
    std::optional<QByteArray> next;
    std::optional<RangeProof> coverage;
};

class ConnectorHistoryAdapter {
  public:
    virtual ~ConnectorHistoryAdapter() = default;
    virtual CutResult observe_cut(const CutRequest &) = 0;
    virtual std::variant<Page, HistoryFailure> fetch_page(const HistoryRequest &) = 0;
};

struct Baseline {
    CutPurpose purpose;
    QList<PolicyMember> members;
    CatchUpMode mode;
    std::optional<Selection> previous;
};

struct OpenGap {
    QByteArray key;
    std::optional<qint64> observed_utc_ms;
};

struct Inspect {
    std::optional<QByteArray> intake_key;
};

struct IntakeProjection {
    core::ContributionId contribution;
    core::ReceiptId original;
    core::PaymentEvidence evidence;
    SourceTimeQuality source_time;
    std::optional<qint64> source_utc_ms;
    qint64 arrival_utc_ms, credited_time_ms, credited_score, credited_progression;
    bool authoritative_verified_record_only;
};

struct IntakeResult {
    IntakeStatus status{IntakeStatus::not_implemented};
    std::optional<AttemptLocator> attempt{};
    std::optional<core::ContributionId> contribution{};
    std::optional<qint64> resolved_utc_start{};
    std::optional<IntakeProjection> projection{};
    std::optional<core::ReceiptMoney> aggregate_delta{};
};

using Command = std::variant<Baseline, OpenGap, BeginHistory, Item, Page, Inspect>;

// Resumption selects an exact intake key. It cannot select a receipt, plan or
// claim independently. A recovery request reacquires current authority.
enum class Phase { receipt, evidence, capture, application, reconcile };
struct PhaseRequest {
    Selection source;
    QByteArray key;
    Phase phase;
    std::optional<Item> received{};
    std::optional<PageSlot> history{};
};

using CurrentAuthority = std::function<Authorization(Action, const Scope &)>;

struct CapturedSupport {
    core::ReceiptArrival arrival;
    core::ContributionConditions conditions;
    core::PlanValuation valuation;
    std::vector<core::RuleSelection> rules;
    std::vector<core::ModifierSelection> modifiers;
    std::vector<core::RequestedEffect> requests;
};

struct ZoneTransition {
    qint64 utc_ms;
    int offset_after_seconds;
};

struct ZoneBasis {
    int initial_offset_seconds;
    QList<ZoneTransition> transitions;
};

struct OwnerInputs {
    CurrentAuthority current_authority;
    ConnectorHistoryAdapter &history;
    std::function<CapturedSupport(const Item &)> capture;
    std::function<std::optional<ZoneBasis>(const QByteArray &, const QByteArray &, QDate)> zone;
};

// Stage 1 seams. The owner queue supplies current producers and selects each
// separate writer unit. Results from an active unit are tentative until normal
// writer completion. No phase publishes, recovers authority, or advances a
// suffix following an uncertain or abnormal completion.
[[nodiscard]] inline IntakeResult submit(SqliteWriter &, const OwnerInputs &, const Selection &,
                                         const Command &) {
    return {};
}
// A fixture may veto the owning transaction immediately before COMMIT.
[[nodiscard]] inline IntakeResult run_phase(const SqliteWorkContext &context, const OwnerInputs &,
                                            const PhaseRequest &,
                                            const std::function<bool()> & = {}) {
    context.begin().rollback();
    return {};
}

} // namespace subathon::server::connector_intake
