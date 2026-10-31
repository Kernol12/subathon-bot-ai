// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/application_plan.hpp>

#include <limits>
#include <utility>

namespace subathon::core {
namespace {
void validate_decision(const PlanOverrideDecision &decision) {
    // This records an upstream permission result; it does not authenticate the actor.
    if (decision.authority != OverrideAuthority::authorized || decision.actor_ref.empty() ||
        decision.reason.empty() || decision.decision_ref.empty()) {
        throw std::invalid_argument("Plan override requires authorized, complete audit evidence");
    }
}

void validate_capture(const PlanCapture &capture) {
    const auto &event = capture.event;
    if (event.event_ref.empty() || event.root_event_ref.empty() || event.receipt_id.value == 0) {
        throw std::invalid_argument("Plan requires event/root references and a receipt ID");
    }
    const auto &valuation = capture.valuation;
    switch (valuation.state) {
    case PlanValuationState::resolved:
        if (!valuation.historical || !event.facts.money ||
            valuation.historical->original_money != *event.facts.money) {
            throw std::invalid_argument(
                "Resolved valuation must retain the event's original money");
        }
        break;
    case PlanValuationState::not_applicable:
    case PlanValuationState::missing_historical:
        if (valuation.historical) {
            throw std::invalid_argument(
                "Unresolved or inapplicable valuation cannot carry a quote");
        }
        break;
    default:
        throw std::invalid_argument("Unknown plan valuation state");
    }
    if (valuation.state == PlanValuationState::missing_historical) {
        for (const auto &request : capture.requested_effects) {
            if (request.requires_historical_valuation) {
                throw std::invalid_argument("Requested effect requires a historical valuation");
            }
        }
    }
}

// Validate caller-owned histories before relying on their head or original event.
void validate_history(const ApplicationPlan &plan) {
    if (plan.plan_ref.empty() || plan.revisions.empty()) {
        throw std::invalid_argument("Plan requires a reference and a nonempty history");
    }
    const auto &first = plan.revisions.front();
    if (first.revision != 1 || first.parent_revision || first.override_decision) {
        throw std::invalid_argument(
            "Initial plan revision must be unaudited revision 1 without parent");
    }
    const CapturedPlanRevision *previous = nullptr;
    for (const auto &revision : plan.revisions) {
        validate_capture(revision.captured);
        if (revision.captured.event != first.captured.event) {
            throw std::invalid_argument("Plan history cannot rewrite the original event");
        }
        if (previous) {
            if (previous->revision == std::numeric_limits<std::uint64_t>::max() ||
                revision.revision != previous->revision + 1 ||
                revision.parent_revision != previous->revision || !revision.override_decision) {
                throw std::invalid_argument("Plan history requires contiguous parented revisions");
            }
            validate_decision(*revision.override_decision);
        }
        previous = &revision;
    }
}

CapturedPlanRevision resolve_revision(CapturedPlanRevision revision, RulePlanSource &rules,
                                      ModifierPlanSource &modifiers) {
    revision.rules = rules.capture(revision.captured.event);
    revision.modifiers = modifiers.capture(revision.captured.event);
    return revision;
}
} // namespace

ApplicationPlan capture_application_plan(std::string plan_ref, const PlanCapture &capture,
                                         RulePlanSource &rules, ModifierPlanSource &modifiers) {
    if (plan_ref.empty()) {
        throw std::invalid_argument("Plan reference must not be empty");
    }
    validate_capture(capture);
    CapturedPlanRevision initial{1, std::nullopt, capture, {}, {}, std::nullopt};
    ApplicationPlan plan{std::move(plan_ref), {}};
    plan.revisions.push_back(resolve_revision(std::move(initial), rules, modifiers));
    return plan;
}

ApplicationPlan revise_application_plan(const ApplicationPlan &original,
                                        std::uint64_t expected_revision,
                                        const PlanCapture &replacement,
                                        const PlanOverrideDecision &decision, RulePlanSource &rules,
                                        ModifierPlanSource &modifiers) {
    validate_history(original);
    const auto head = original.revisions.back().revision;
    if (expected_revision != head) {
        throw std::runtime_error("Plan override expected revision is stale");
    }
    validate_decision(decision);
    validate_capture(replacement);
    if (replacement.event != original.revisions.front().captured.event) {
        throw std::invalid_argument("Plan override cannot rewrite the original event");
    }
    if (head == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Plan revision number exhausted");
    }
    auto revised = original;
    CapturedPlanRevision next{head + 1, head, replacement, {}, {}, decision};
    revised.revisions.push_back(resolve_revision(std::move(next), rules, modifiers));
    return revised;
}

PlanPreviewValidity check_plan_preview(const ApplicationPlan &plan, const PlanPreview &preview,
                                       const std::string &current_state_revision) {
    validate_history(plan);
    if (preview.authoritative_state_revision.empty() || current_state_revision.empty()) {
        throw std::invalid_argument("Plan preview requires nonempty state revision tokens");
    }
    if (plan.revisions.back().revision != preview.plan_revision) {
        return PlanPreviewValidity::stale_plan;
    }
    if (preview.authoritative_state_revision != current_state_revision) {
        return PlanPreviewValidity::stale_state;
    }
    return PlanPreviewValidity::current;
}
} // namespace subathon::core
