// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/core/contribution.hpp>
#include <subathon/core/exact_decimal.hpp>
#include <vector>

namespace subathon::core {
// Revision 1: docs/contracts/application-plans-v1.md. Owned internal projections.
struct PlanEvent {
    std::string event_ref;
    std::optional<std::string> parent_event_ref;
    std::string root_event_ref;
    ReceiptId receipt_id{0};
    ReceiptIdentity identity;
    ReceiptFacts facts{};
    ReceiptArrival arrival{};
    std::optional<ContributionId> contribution_id;
    bool operator==(const PlanEvent &) const = default;
};

struct HistoricalValuation {
    std::string resolution_ref;
    ReceiptMoney original_money;
    std::string comparison_currency;
    // Retained decimal spellings, including precision; validation is upstream.
    std::string effective_rate_decimal;
    std::optional<std::string> fetched_rate_decimal;
    std::string snapshot_ref;
    std::optional<std::string> source_ref;
    std::optional<std::int64_t> rate_timestamp_utc_ms;
    std::optional<std::int64_t> fetch_timestamp_utc_ms;
    std::optional<std::string> manual_decision_ref;
    bool operator==(const HistoricalValuation &) const = default;
};

enum class PlanValuationState { not_applicable, missing_historical, resolved };
struct PlanValuation {
    PlanValuationState state{PlanValuationState::missing_historical};
    std::optional<HistoricalValuation> historical;
    bool operator==(const PlanValuation &) const = default;
};
struct RuleSelection {
    std::string rule_ref;
    std::string version_ref;
    bool selected{false};
    std::string explanation;
    bool operator==(const RuleSelection &) const = default;
};
struct ModifierSelection {
    std::string definition_version_ref;
    std::string instance_revision_ref;
    std::string target_ref;
    std::string group_ref;
    std::string group_operator;
    std::string target_operator;
    std::string original_factor_decimal;
    bool operator==(const ModifierSelection &) const = default;
};
// These adapters return already resolved, retained arrival-time selections. API-008
// owns matching, exclusion ordering, combination and numeric evaluation.
class RulePlanSource {
  public:
    virtual ~RulePlanSource() = default;
    [[nodiscard]] virtual std::vector<RuleSelection> capture(const PlanEvent &event) = 0;
};
class ModifierPlanSource {
  public:
    virtual ~ModifierPlanSource() = default;
    [[nodiscard]] virtual std::vector<ModifierSelection> capture(const PlanEvent &event) = 0;
};
enum class RequestedUnit { milliseconds, micro_points, whole_count };
struct RequestedEffect {
    std::string action_ref;
    std::string action_version_ref;
    std::string rule_ref;
    std::string target_ref;
    RequestedUnit unit{RequestedUnit::milliseconds};
    bool requires_historical_valuation{true};
    Exact exact_requested_units;
    std::int64_t requested_units{0};
    std::string calculation_explanation;
    std::string reversal_behavior_ref;
    bool operator==(const RequestedEffect &) const = default;
};
struct PlanCapture {
    PlanEvent event;
    ContributionState contribution_state;
    PlanValuation valuation;
    // Already resolved final requests, never actual applied/cap-limited amounts.
    std::vector<RequestedEffect> requested_effects;
    bool operator==(const PlanCapture &) const = default;
};
enum class OverrideAuthority { unknown, denied, authorized };
struct PlanOverrideDecision {
    OverrideAuthority authority{OverrideAuthority::unknown};
    std::string actor_ref;
    std::string reason;
    std::string decision_ref;
    bool operator==(const PlanOverrideDecision &) const = default;
};
struct CapturedPlanRevision {
    std::uint64_t revision{0};
    std::optional<std::uint64_t> parent_revision;
    PlanCapture captured;
    std::vector<RuleSelection> rules;
    std::vector<ModifierSelection> modifiers;
    std::optional<PlanOverrideDecision> override_decision;
    bool operator==(const CapturedPlanRevision &) const = default;
};
struct ApplicationPlan {
    std::string plan_ref;
    std::vector<CapturedPlanRevision> revisions;
    bool operator==(const ApplicationPlan &) const = default;
};
struct PlanPreview {
    std::uint64_t plan_revision{0};
    std::string authoritative_state_revision;
};
enum class PlanPreviewValidity { current, stale_plan, stale_state };
class ApplicationPlanNotImplemented final : public std::logic_error {
  public:
    ApplicationPlanNotImplemented()
        : std::logic_error("Application plan is intentionally unimplemented; review tests first") {}
};
// No storage or effects. The caller persists the returned value through its writer.
[[nodiscard]] ApplicationPlan capture_application_plan(std::string plan_ref,
                                                       const PlanCapture &capture,
                                                       RulePlanSource &rules,
                                                       ModifierPlanSource &modifiers);
[[nodiscard]] ApplicationPlan
revise_application_plan(const ApplicationPlan &original, std::uint64_t expected_revision,
                        const PlanCapture &replacement, const PlanOverrideDecision &decision,
                        RulePlanSource &rules, ModifierPlanSource &modifiers);
[[nodiscard]] PlanPreviewValidity check_plan_preview(const ApplicationPlan &plan,
                                                     const PlanPreview &preview,
                                                     const std::string &current_state_revision);
} // namespace subathon::core
