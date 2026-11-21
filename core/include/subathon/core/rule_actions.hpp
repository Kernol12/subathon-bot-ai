// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <subathon/core/delegated_permissions.hpp>
#include <subathon/core/rule_selection.hpp>
#include <utility>

namespace subathon::core {

// RULES-003 revision 2. All ports are supplied by the owning application.
// Core plans and reports observations without opening a transaction or starting delivery.
struct RuleActionKey {
    std::string rule_ref;
    std::string rule_version;
    std::string action_ref;
    std::string action_version;
    std::size_t position; // Zero-based within the captured rule.
    bool operator==(const RuleActionKey &) const = default;
};

struct RuleEventLineage {
    std::string event_ref;
    std::optional<std::string> parent_ref;
    std::string root_ref;
    std::optional<RuleActionKey> cause;
    bool operator==(const RuleEventLineage &) const = default;
};

enum class RuleActionRefusal {
    invalid_binding,
    unresolved_input,
    authorization_hold,
    authority_unavailable,
    invalid_target,
    unsupported_capability,
    capability_unavailable,
    handler_refusal
};

struct RuleBoundAction {
    PermissionRequest request; // One exact operation, tuple and amount from the owning mapper.
    RuleActionPayload payload; // The mapped typed request, including immutable version references.
};
using RuleActionBinding = std::variant<RuleBoundAction, RuleActionRefusal>;

struct RuleActionIntent {
    RuleActionKey key;
    RuleActionPayload captured_payload;
    RuleActionBinding binding;
};
struct RuleSelectedActions {
    std::size_t captured_index;
    std::string original_author;
    std::vector<RuleActionIntent> intents;
};
struct RuleActionPlan {
    RuleEventLineage event;
    RuleSelection selection;
    std::vector<RuleSelectedActions> selected;
};
using RuleActionMapper = std::function<RuleActionBinding(
    const RuleActionKey &, const RuleActionPayload &, const RuleCapturedInput &)>;

// Author identities are supplied in retained configuration order. Selection errors
// propagate with no partially mapped result. Mapping resolves immutable inputs and
// the minimum concrete binding before any current or protected query.
[[nodiscard]] RuleActionPlan plan_rule_actions(const RuleConfiguration &, const RuleCapturedInput &,
                                               const RuleEventLineage &,
                                               std::span<const std::string> original_authors,
                                               std::span<const RuleExplicitValuation>,
                                               const RuleActionMapper &);

struct RuleActionAuthorityFacts {
    std::string snapshot;
    PermissionCatalog catalog;
    PermissionOwnerFacts owner;
    PermissionPolicyFacts use;
    PermissionPolicyFacts grant;
};
enum class RuleGate { permit, deny, unavailable };
enum class RuleCapability { supported, unsupported, unavailable };

struct RuleActionChecks {
    std::function<std::optional<RuleActionAuthorityFacts>(
        std::string_view author, const RuleActionKey &, const RuleBoundAction &)>
        authority;
    // The owner supplies the separate execution-subject or sender gate where applicable.
    std::function<RuleGate(const RuleBoundAction &, std::string_view snapshot)> execution;
    std::function<bool(const RuleBoundAction &)> valid_target;
    std::function<RuleCapability(const RuleBoundAction &)> capability;
};

enum class RuleAuthorityChannel { use, grant, execution };
struct RuleStopDetail {
    std::optional<PermissionAutomationActionDecision> author_decision;
    std::optional<RuleAuthorityChannel> channel;
    std::optional<PermissionRational> limit;
    std::optional<EffectivePermissionError> authority_error;
    std::optional<std::string> opaque_reason;
};

struct RuleStop {
    std::size_t position;
    RuleActionRefusal refusal;
    std::optional<RuleStopDetail> detail;
};
enum class RuleStopDisclosure { none, stop_only, detail };
// The reader supplies current disclosure from the owning boundary.
[[nodiscard]] std::optional<RuleStop> disclose_rule_stop(const RuleStop &, RuleStopDisclosure);

struct RuleTimeObservation {
    std::int64_t requested_ms;
    std::int64_t main_change_ms;
    std::int64_t debt_change_ms;
    std::int64_t cap_loss_ms;
    std::int64_t main_after_ms;
    bool operator==(const RuleTimeObservation &) const = default;
};
struct RuleQuantityObservation {
    std::string unit;
    PermissionRational requested;
    PermissionRational actual;
};
struct RuleEffectObservation {
    std::string effect_ref;
    std::variant<std::monostate, RuleTimeObservation, RuleQuantityObservation> change;
};
struct RuleProjectedDelivery {
    std::string route_ref;
    std::string sender_ref;
};
struct RuleProjectedChild {
    std::string name;
};
using RuleProjection = std::variant<std::monostate, RuleTimeObservation, RuleQuantityObservation,
                                    RuleProjectedDelivery, RuleProjectedChild>;
struct RuleSimulation {
    std::vector<std::pair<RuleActionKey, RuleProjection>> projected;
    std::vector<std::pair<std::size_t, RuleStop>> stops; // Indexed by captured rule index.
};
using RuleActionProjector = std::function<RuleProjection(const RuleBoundAction &)>;
[[nodiscard]] RuleSimulation simulate_rule_actions(const RuleActionPlan &, const RuleActionChecks &,
                                                   const RuleActionProjector &);

enum class RuleDeliveryState { queued, sending, delivered, failed, outcome_unknown };
struct RuleDeliveryObservation {
    std::string job_ref;
    RuleDeliveryState state;
    std::int64_t revision;
    std::optional<std::int64_t> attempt;
    std::optional<std::string> evidence_ref;
    bool operator==(const RuleDeliveryObservation &) const = default;
};
struct RuleDeliveryTrace {
    RuleDeliveryObservation queued;
    RuleDeliveryObservation latest;
};
// Report supplied observations from the delivery owner. A declined start supplies
// no new observation. Queue acceptance and later observation remain distinct.
[[nodiscard]] RuleDeliveryTrace
observe_rule_delivery(const RuleDeliveryTrace &, const std::optional<RuleDeliveryObservation> &);

using RuleOwnerObservation =
    std::variant<std::monostate, RuleEffectObservation, RuleDeliveryObservation, RuleEventLineage>;
struct RuleOwnerAcceptance {
    RuleOwnerObservation observed;
};
struct RuleOwnerRefusal {
    std::string opaque_reason;
};
enum class RulePassFailure { numeric_hold };
using RuleOwnerResult = std::variant<RuleOwnerAcceptance, RuleOwnerRefusal, RulePassFailure>;
// The owner must recheck currentness as part of accepting the exact request in
// its own write scope. A snapshot reference is an input to that check.
using RuleActionSubmit =
    std::function<RuleOwnerResult(const RuleEventLineage &, const RuleActionKey &,
                                  const RuleBoundAction &, std::string_view checked_snapshot)>;

struct RuleAcceptedAction {
    RuleActionKey key;
    RuleOwnerObservation observed;
};
struct RuleConsumedActions {
    std::size_t captured_index;
    std::vector<RuleAcceptedAction> accepted;
    std::optional<RuleStop> stop;
};
struct RuleStagedPass {
    std::vector<RuleConsumedActions> rules;
};
using RulePass = std::variant<RuleStagedPass, RulePassFailure>;
[[nodiscard]] RulePass consume_rule_actions(const RuleActionPlan &, const RuleActionChecks &,
                                            const RuleActionSubmit &);
struct RuleCommittedPass {
    std::vector<RuleConsumedActions> rules;
};
// A failure from the transaction owner prevents publication even if a prefix
// was staged. Numeric holds from consume have no staged pass to publish.
enum class RuleCompletion { committed, rolled_back, outcome_unknown, abnormal_completion };
[[nodiscard]] std::optional<RuleCommittedPass> publish_rule_pass(const RuleStagedPass &,
                                                                 RuleCompletion);

class RuleActionsNotImplemented final : public std::logic_error {
  public:
    RuleActionsNotImplemented()
        : std::logic_error("RULES-003 behavior is intentionally unimplemented") {}
};

} // namespace subathon::core
