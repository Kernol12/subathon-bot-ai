// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <functional>
#include <subathon/core/application_plan.hpp>
#include <subathon/core/modifier_stacking.hpp>

namespace subathon::server {
// Supplied by the exact selected-revision reader. An absent revision or definition
// leaves the selection unresolved. The assertions identify the selected revision even when its read
// is missing.
struct SelectedModifierRevision {
    std::string_view instance_ref;
    std::string_view instance_revision_ref;
    std::string_view definition_revision_ref;
    const core::ModifierInstance *instance;
    const core::ModifierDefinition *definition;
};

struct ModifierTraceProfile {
    std::string producer_ref;
    std::string profile_ref;
    bool operator==(const ModifierTraceProfile &) const = default;
};

struct ModifierRecalculationBinding {
    std::string plan_ref;
    std::uint64_t revision;
    std::string original_arrival_ref;
    std::string decision_ref;
    bool operator==(const ModifierRecalculationBinding &) const = default;
};

struct CapturedModifierContext {
    std::string document_ref;
    std::string configuration_ref; // Typed link from retained context to configuration.
    std::string policy_ref;
    ModifierTraceProfile profile;
    std::optional<ModifierRecalculationBinding> recalculation;
};

struct ModifierTraceLink {
    std::size_t plan_row_ordinal; // One-based row in this revision's ordered selection.
    core::ModifierOccurrenceKey occurrence;
    bool operator==(const ModifierTraceLink &) const = default;
};

struct ModifierRecalculation {
    std::string_view plan_ref;
    std::uint64_t revision;
    std::reference_wrapper<const core::PlanOverrideDecision> decision;
};

struct ModifierStackCapture {
    std::string_view original_arrival_ref;
    std::reference_wrapper<const core::ReceiptArrival> arrival;
    // For a recalculation the reader looks up the separate document under decision_ref.
    std::optional<ModifierRecalculation> recalculation;
    const CapturedModifierContext *context;
    const ModifierTraceProfile *admitted_profile;
    const core::ModifierStackPolicy *policy;
    // A present empty span is a resolved empty eligible selection.
    std::optional<std::span<const SelectedModifierRevision>> eligible;
    std::span<const std::string_view> matched_qualifiers;
    // When recorded rows are used as input for a new calculation, validate them
    // and their source links against the selected policy and exact occurrences.
    std::optional<std::span<const core::ModifierSelection>> recorded_selection;
    std::span<const ModifierTraceLink> recorded_links;
};

struct ModifierBaseEffect {
    std::size_t action_ordinal;
    std::string action_ref;
    std::string action_version_ref;
    std::string rule_ref;
    core::ModifierTargetKey target;
    bool requires_historical_valuation;
    core::Exact exact_units;
    std::string reversal_behavior_ref;
    bool operator==(const ModifierBaseEffect &) const = default;
};

struct ModifierScaledEffect {
    ModifierBaseEffect base;
    core::Exact exact_units;
    // Populated using #171 only for time and score. A count remains an exact ratio.
    std::optional<std::int64_t> rounded_units;
};

struct ModifierStackProposal {
    std::string context_ref;
    ModifierTraceProfile profile;
    std::vector<core::ModifierSelection> selection;
    std::vector<ModifierTraceLink> links;
    std::vector<core::ModifierTargetTrace> targets;
    std::vector<ModifierScaledEffect> effects;
};

class ModifierHandoffNotImplemented final : public std::logic_error {
  public:
    ModifierHandoffNotImplemented()
        : std::logic_error(
              "Captured modifier handoff is intentionally unimplemented; review tests first") {}
};

using ModifierStackEvaluation = std::variant<core::ModifierStackFailure, ModifierStackProposal>;

// Proposal seam for the general evaluator. Validate exact identity, context/profile, captured
// policy, all source factors before filtering, and original ordinals before combination.
// A recalculation requires explicitly authorized authority and a matching binding on the supplied
// context document. A non-authorizing decision or a missing or contradictory binding on that
// document is invalid. A missing admitted profile is unresolved.
// Apply the tightest supplied limits to the whole proposal, including each stored trace copy.
// Failure carries no partial proposal. Producers own eligibility, archival reads and decoder
// admission. The plan/application owners own publication, count finalization and actuals.
[[nodiscard]] ModifierStackEvaluation
evaluate_captured_modifiers(const ModifierStackCapture &capture,
                            std::span<const ModifierBaseEffect> bases,
                            const core::ModifierStackLimits &limits);
} // namespace subathon::server
