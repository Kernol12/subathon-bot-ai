// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/application_service.hpp>
#include <subathon/server/milestone_group_decisions.hpp>

namespace subathon::server {

// Stage 1 boundary for sqlite-refund-entitlements-v1. These owned facts neither
// authorize a command nor prove the selected receipt, plan or effect exists.
struct FinancialPaymentKey {
    core::ReceiptOrigin origin;
    core::ReceiptLinkReference payment;
    core::ReceiptId support_receipt;
    bool operator==(const FinancialPaymentKey &) const = default;
};

struct VerifiedRefundCorrelation {
    FinancialPaymentKey payment;
    core::ReceiptLinkReference refund;
    std::string evidence_ref;
    core::ContributionId contribution;
    ApplicationOperationKey original;
    ApplicationPlanKey original_plan;
};

struct RefundTimeAllocation {
    std::int64_t main;
    std::int64_t restoration;
    bool operator==(const RefundTimeAllocation &) const = default;
};

struct GroupedRefundEntitlement {
    ApplicationOriginalComponent original;
    std::int64_t request_ordinal;
    std::int64_t expected_basis_position;
    RefundTimeAllocation previous;
    RefundTimeAllocation cumulative;
    core::Exact exact_cumulative;
    core::Exact residual;
    std::int64_t main_request;
    std::int64_t restoration_request;
};

struct FinancialEntitlement {
    core::Exact previous_money;
    core::Exact accepted_money;
    core::Exact fraction;
    bool full;
    std::optional<GroupedRefundEntitlement> time;
    std::vector<ApplicationReversalLink> components;
};

struct FinancialRefundCommand {
    ApplicationOperationKey operation;
    core::ReceiptId receipt;
    std::optional<VerifiedRefundCorrelation> correlation;
    ApplicationPlanKey financial_plan;
    std::int64_t expected_payment_position;
    std::string expected_state_token;
    core::ContributionState expected_contribution;
    ApplicationPrincipalBinding principal;
    ApplicationAudit audit;
};

struct FinancialTimerCommand {
    ApplicationOperationKey operation;
    FinancialPaymentKey payment;
    ApplicationHoldVersionLocator origin;
    std::vector<ApplicationHoldVersionLocator> expected_active;
    std::string expected_state_token;
    std::optional<core::RefundDeficitPolicy> override_policy;
    ApplicationPrincipalBinding principal;
    ApplicationAudit audit;
};

enum class RefundTimerDisposition { applied, held, finished };
enum class FinancialTimerHoldReason { deficit, prior_request };

struct RefundTimerActual {
    std::int64_t main_removed{};
    std::int64_t restored{};
    std::int64_t deficit_debt{};
    std::int64_t unapplied_main{};
    std::int64_t unapplied_restoration{};
    bool operator==(const RefundTimerActual &) const = default;
};

struct RefundTimerOutcome {
    RefundTimerDisposition disposition;
    RefundTimerActual actual;
    bool operator==(const RefundTimerOutcome &) const = default;
};

struct FinancialTimerHold {
    ApplicationHoldVersionLocator locator;
    core::ContributionId subject;
    FinancialPaymentKey payment;
    std::int64_t payment_position;
    ApplicationOperationKey refund_operation;
    GroupedRefundEntitlement time;
    FinancialTimerHoldReason reason;
};

struct FinancialRefundRecord {
    FinancialRefundCommand command;
    core::ReceiptRecord refund_receipt;
    core::ReceiptRecord support_receipt;
    core::CapturedPlanRevision original_capture;
    core::CapturedPlanRevision financial_capture;
    std::int64_t payment_position;
    FinancialEntitlement entitlement;
    RecordedApplicationBundle application;
    std::optional<RefundTimerOutcome> timer;
    std::optional<FinancialTimerHold> hold;
    std::vector<MilestoneRefundObservation> milestone_observations;
};

struct FinancialTimerResolution {
    FinancialTimerCommand command;
    FinancialTimerHold held;
    ApplicationHoldVersion closed;
    ApplicationStateSnapshot before;
    ApplicationStateSnapshot after;
    RefundTimerOutcome timer;
    std::vector<core::TimerTransition> transitions;
    ApplicationAudit audit;
};

struct FinancialPaymentHistory {
    std::int64_t position;
    core::Exact accepted_money;
    std::vector<FinancialRefundRecord> refunds;
    std::vector<FinancialTimerResolution> resolutions;
};

enum class FinancialDisposition {
    applied,
    existing,
    refused,
    denied_execution,
    denied_disclosure,
    active_hold,
    stale_payment,
    stale_basis,
    stale_state,
    exceeds_original,
    identity_conflict,
    invalid_financial_input,
    basis_conflict,
    prerequisite,
    out_of_order,
    stale_hold,
    deficit,
    resolved,
    ordinary_resolved
};

struct FinancialWorkReply {
    FinancialDisposition disposition;
    std::variant<std::monostate, FinancialRefundRecord, FinancialTimerResolution,
                 ApplicationHoldMutationOutcome, RecordedApplicationBundle>
        record;
};

// Synchronous on the owner and bound to the caller's one active session. The
// effect port supplies only started observations and actual timer effect proposals;
// financial eligibility, classification, validation and writes belong to #199.
struct FinancialEffectObservation {
    ApplicationClockObservation clock;
    core::TimerConfig config;
    core::TimerState observed_timer;
    std::vector<core::TimerTransition> transitions;
    core::RefundDeficitPolicy deficit_policy;
    std::optional<std::int64_t> current_cap;
    std::string current_rule_version;
    std::string current_rate_spelling;
};

struct FinancialMilestoneTarget {
    MilestoneOccurrenceKey occurrence;
    std::int64_t target;
    MilestoneRefundPolicy policy;
};

class FinancialRefundEffectPort {
  public:
    virtual ~FinancialRefundEffectPort() = default;
    [[nodiscard]] virtual FinancialEffectObservation observe(const SqliteWorkSession &,
                                                             const ApplicationStateSnapshot &) = 0;
    [[nodiscard]] virtual RefundTimerActual timer_effect(const SqliteWorkSession &,
                                                         const GroupedRefundEntitlement &,
                                                         const FinancialEffectObservation &,
                                                         core::RefundDeficitPolicy) = 0;
    [[nodiscard]] virtual std::vector<FinancialMilestoneTarget>
    milestones(const SqliteWorkSession &) = 0;
};

enum class FinancialAuthorityUse { accept, continue_timer, disclose, override_timer };
class FinancialRefundAuthority {
  public:
    virtual ~FinancialRefundAuthority() = default;
    [[nodiscard]] virtual bool allows(const SqliteWorkSession &, FinancialAuthorityUse,
                                      const ApplicationPrincipalBinding &,
                                      const ApplicationOperationKey &,
                                      const FinancialPaymentKey &) = 0;
};

class SqliteFinancialRefundNotImplemented final : public std::logic_error {
  public:
    explicit SqliteFinancialRefundNotImplemented(const char *entry)
        : std::logic_error(std::string("ACCT-006 writer/service NI: ") + entry) {}
};

// Provisional selected committed/transaction-visible records; no global scan
// or migration authority. The owner controls whether a selected result may be disclosed.
class SqliteFinancialRefundStore final {
  public:
    explicit SqliteFinancialRefundStore(SqliteWorkSession session) : session_(std::move(session)) {}
    // The read limit bounds materialization of the selected payment's history.
    // It does not constrain what a payment can retain.
    [[nodiscard]] std::optional<FinancialPaymentHistory> history(const FinancialPaymentKey &,
                                                                 qsizetype max_entries) const;
    [[nodiscard]] std::optional<FinancialRefundRecord>
    lookup(const ApplicationOperationKey &) const;
    [[nodiscard]] std::optional<FinancialTimerResolution>
    lookup_resolution(const ApplicationOperationKey &) const;
    [[nodiscard]] std::vector<FinancialTimerHold> active(const ApplicationHoldSubject &) const;

  private:
    SqliteWorkSession session_;
};

// The service owns revalidation and composition on the supplied session. The
// work owner commits once and passes only normal acknowledged typed work to publication.
[[nodiscard]] FinancialWorkReply stage_financial_refund(const SqliteWorkSession &,
                                                        const FinancialRefundCommand &,
                                                        const FinancialEntitlement &,
                                                        FinancialRefundAuthority &,
                                                        FinancialRefundEffectPort &);
[[nodiscard]] FinancialWorkReply stage_financial_timer_resolution(const SqliteWorkSession &,
                                                                  const FinancialTimerCommand &,
                                                                  FinancialRefundAuthority &,
                                                                  FinancialRefundEffectPort &);

// The linked operation may already be present in the same session. The existing
// selected bundle/origin rules apply alongside the complete mixed active-set guard.
[[nodiscard]] FinancialWorkReply
stage_financial_aware_linked_resolution(const SqliteWorkSession &,
                                        const ApplicationHoldResolutionBatch &);
[[nodiscard]] FinancialWorkReply
stage_financial_aware_nonfinancial_reversal(const SqliteWorkSession &,
                                            const PreparedApplicationBundle &);
[[nodiscard]] std::optional<FinancialWorkReply>
acknowledged_financial_reply(const SqliteWorkResult &);
} // namespace subathon::server

Q_DECLARE_METATYPE(subathon::server::FinancialWorkReply)
