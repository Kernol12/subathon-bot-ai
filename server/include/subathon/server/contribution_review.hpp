// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/application_service.hpp>
#include <subathon/server/milestone_group_decisions.hpp>
#include <subathon/server/sqlite_migrations.hpp>
#include <subathon/server/testing/contribution_review_test_access.hpp>

namespace subathon::server {

enum class ReviewVerb { accept, reject, apply_accepted, continue_timer, reject_held, retain_held };
enum class ReviewStatus {
    accepted_pending,
    applied,
    rejected,
    timer_resolved,
    retained,
    ineligible,
    stale,
    denied
};
enum class ReviewDisposition {
    committed,
    existing,
    write_failed,
    commit_uncertain,
    committed_result_unavailable,
    unattempted
};
enum class ReviewRefusal {
    invalid_selection,
    denied,
    identity_conflict,
    batch_too_large,
    unavailable
};
enum class ReviewStop {
    write_failed,
    commit_uncertain,
    committed_result_unavailable,
    row_budget_exceeded,
    denied,
    unavailable,
    admission_refused
};

struct ReviewSelector {
    core::ContributionId cid{0};
    core::ReceiptId receipt{0};
    bool operator==(const ReviewSelector &) const = default;
};
struct ReviewTimerSelector {
    ApplicationHoldVersionLocator locator;
    std::int64_t ordinal;
    std::int64_t signed_request;
};
struct ReviewDeficitChoice {
    std::int64_t ordinal;
    std::int64_t signed_request;
    core::RefundDeficitPolicy policy;
};
struct ReviewEntryIntent {
    ReviewSelector selector;
    ReviewVerb verb{ReviewVerb::accept};
    PausedAcceptanceMode mode{PausedAcceptanceMode::normal};
    std::optional<std::vector<MilestoneAcceptanceChoice>> milestones;
    std::optional<std::string> pending_decision_ref;
    std::optional<ReviewTimerSelector> timer;
    std::optional<std::vector<ReviewDeficitChoice>> deficits;
    std::vector<ApplicationHoldResolutionEntry> ordinary_closures;
    std::optional<core::CapturedPlanRevision> revision; // eligible proposal, bound to its parent
};
struct ReviewPreviewRequest {
    ApplicationPrincipalBinding principal;
    std::vector<ReviewEntryIntent> entries;
};
struct ReviewEntry {
    ReviewEntryIntent intent;
    QByteArray basis;
    std::string decision_ref;
    std::string reason;
};
struct ReviewCommand {
    ApplicationOperationKey key;
    ApplicationPrincipalBinding principal;
    std::string actor;
    std::vector<ReviewEntry> entries;
};
struct ReviewPreviewFacts {
    ReviewSelector selector; // echoes the selection; never establishes a verified anchor
    QByteArray basis;
    bool may_decide{};
    bool may_apply{};
    std::vector<core::ContributionBlocker> blockers;
    std::vector<ApplicationEffect> prospective_effects;
    std::vector<MilestoneOccurrenceKey> occurrences;
    std::optional<ReviewTimerSelector> reached_deficit;
};
struct ReviewPreviewRow {
    int ordinal;
    std::optional<ReviewPreviewFacts> facts; // absent for redacted/denied
};
struct ReviewOutcomeFacts {
    std::optional<ReviewSelector> verified;
    std::optional<ApplicationOperationKey> bundle;
    std::vector<core::ContributionBlocker> blockers;
    std::optional<ReviewTimerSelector> timer;
};
struct ReviewOutcome {
    ReviewStatus status;
    std::optional<ReviewOutcomeFacts> facts;
};
struct ReviewRowResult {
    int ordinal;
    std::optional<ReviewDisposition> disposition; // absent for a scope-redacted outcome
    std::optional<ReviewOutcome> outcome;
};
struct ReviewReply {
    std::vector<ReviewRowResult> rows;
    std::optional<ReviewStop> stop;
};

// Session material is separate from the immutable intent. These synthetic or
// trusted admission inputs are transient and never part of the journal.
enum class ReviewAuthorityUse { session, batch_key, execute_row, disclose_row };
struct ReviewAuthorityQuery {
    ReviewAuthorityUse use;
    ApplicationPrincipalBinding principal;
    std::string_view session_proof;
    std::string_view actor;
    const ApplicationOperationKey *key;
    const ReviewEntryIntent *entry;
    const ReviewOutcomeFacts *committed;
};
struct ReviewPolicyFacts {
    std::string revision;
    std::optional<std::int64_t> cap;
    core::TimerConfig timer;
    core::TimerClockStep clock;
    std::optional<std::vector<core::RequestedUnit>> finished_siblings;
    std::vector<core::MilestoneTarget> milestones;
    std::vector<ApplicationJobIntent> ordinary_alerts;
    std::vector<ApplicationJobIntent> internal_jobs;
    std::vector<ApplicationSignedCorrectionPolicy> signed_original;
    bool may_apply_through_financial_hold{};
};
class ContributionReviewFacts {
  public:
    virtual ~ContributionReviewFacts() = default;
    virtual AccessDecision check(const ApplicationReferenceReadView &,
                                 const ReviewAuthorityQuery &) = 0;
    virtual std::optional<ReviewPolicyFacts> policy(const ApplicationReferenceReadView &,
                                                    const ReviewSelector &) = 0;
};

class ContributionReviewNotImplemented final : public std::logic_error {
  public:
    explicit ContributionReviewNotImplemented(const char *entry)
        : std::logic_error(std::string("ACCT-005 NI: ") + entry) {}
};

[[nodiscard]] MigrationManifest contribution_review_repository_manifest();

class ContributionReview final {
  public:
    using PreviewResult = std::variant<ReviewRefusal, std::vector<ReviewPreviewRow>>;
    using Result = std::variant<ReviewRefusal, ReviewReply>;

    ContributionReview(SqliteWriter &writer, ContributionReviewFacts &facts,
                       testing::ContributionReviewObserver observe = {})
        : writer_(writer), facts_(facts), observe_(std::move(observe)) {}

    void preview(ReviewPreviewRequest request, std::string session_proof,
                 std::function<void(PreviewResult)> done);
    void execute(ReviewCommand command, std::string session_proof,
                 std::function<void(Result)> done);
    void lookup(ApplicationOperationKey key, ApplicationPrincipalBinding principal,
                std::string session_proof, std::function<void(Result)> done);

  private:
    SqliteWriter &writer_;
    ContributionReviewFacts &facts_;
    testing::ContributionReviewObserver observe_;
};

} // namespace subathon::server
