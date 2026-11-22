// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/core/modifier_clock.hpp>
#include <subathon/server/contribution_evidence.hpp>
#include <subathon/server/modifier_stacking.hpp>
#include <subathon/server/sqlite_application_bundle.hpp>
#include <subathon/server/sqlite_modifier_activations.hpp>
#include <subathon/server/sqlite_retained_context.hpp>

namespace subathon::server {

struct IssuedModifierEvent {
    std::string event_ref;
    std::string root_ref;
    std::optional<std::string> parent_ref;
};

struct ModifierArrivalInstance {
    ModifierInstanceKey key;
    std::string definition_revision_ref;
    std::int64_t remaining_ms;
    bool operator==(const ModifierArrivalInstance &) const = default;
};

// The triggering view and its cut come from the same ordered activation operation.
struct ModifierTriggerCut {
    core::ModifierTriggeringContribution trigger;
    core::ModifierClockObservation observation;
    std::uint64_t turn;
    core::ModifierActivationProposal activation;
};

struct QualifiedModifierCut {
    core::ModifierClockObservation observation;
    std::uint64_t turn;
    ModifierTraceProfile profile;
    core::ModifierStackPolicy policy;
    std::vector<std::string> matched_qualifiers;
    std::vector<ModifierArrivalInstance> eligible;
    std::optional<ModifierTriggerCut> trigger;
};

struct ModifierArrivalInput {
    core::ReceiptIdentity identity;
    core::ReceiptArrival arrival;
    IssuedModifierEvent event;
    std::optional<QualifiedModifierCut> cut;
};

struct ModifierDecisionMode {
    std::string plan_ref;
    std::uint64_t revision;
    core::PlanOverrideDecision decision;
};

struct PreparedModifierArrival {
    RetainedDocument retained;
};

enum class ModifierCaptureRefusal {
    unresolved,
    invalid,
    resource_limit,
    denied,
    stale_head,
    conflict,
    append_refused
};

using ModifierArrivalPreparation = std::variant<PreparedModifierArrival, ModifierCaptureRefusal>;
using LocalModifierRecordShape = std::optional<ModifierCaptureRefusal>;

struct SelectedModifierCapture {
    core::PlanEvent event;
    CapturedModifierContext context;
    core::ModifierStackPolicy policy;
    std::vector<std::string> matched_qualifiers;

    struct Revision {
        ModifierArrivalInstance observed;
        core::ModifierInstance exact_instance;
        core::ModifierDefinition exact_definition;
    };
    std::vector<Revision> eligible;
    std::optional<std::vector<core::ModifierSelection>> recorded_selection;
    std::vector<ModifierTraceLink> recorded_links;
};

// The normal selected reader owns the SQLite reads. A supplied exact resolver
// models unavailable history in the caller's admitted read view.
using ModifierExactInstanceRead = std::function<std::optional<SqliteModifierInstanceRevision>(
    const SqliteWorkSession &, std::string_view)>;
using ModifierCaptureSelection = std::variant<SelectedModifierCapture, ModifierCaptureRefusal>;

struct ModifierBindingName {
    std::string configuration_ref;
    std::string definition_ref;
    std::string revision_ref;
    std::string name;
};

struct ModifierBindingProjection {
    std::optional<std::string> name;
    core::Exact factor;
    std::string factor_decimal;
    std::optional<std::int64_t> remaining_ms;
    std::optional<bool> active;
};

[[nodiscard]] ModifierArrivalPreparation
prepare_modifier_arrival(const ModifierArrivalInput &, const core::ModifierStackLimits &);
[[nodiscard]] RetainDisposition retain_modifier_arrival(const SqliteWorkSession &,
                                                        const PreparedModifierArrival &);
[[nodiscard]] ModifierCaptureSelection select_modifier_capture(
    const SqliteWorkSession &, const ContributionReceipt &selected_original,
    const IssuedModifierEvent &, std::optional<core::ContributionId>,
    const std::optional<ModifierDecisionMode> &, const ModifierTraceProfile &,
    const core::ModifierStackLimits &, const ModifierExactInstanceRead & = {},
    std::optional<std::span<const core::ModifierSelection>> recorded_selection = {});

// The separately qualified cut belongs to the override configuration. The
// original event and arrival in next_revision belong to the selected receipt.
using ModifierRecalculationResult = std::optional<ModifierCaptureRefusal>;
// Stage 1 can model one synchronous call to the owning append after context
// staging. This borrowed dependency uses the same active session. Empty uses
// the normal plan owner.
using ModifierPlanAppend = std::function<SqlitePlanAppendDisposition(
    const SqliteWorkSession &, const ApplicationPlanKey &, const core::CapturedPlanRevision &)>;
[[nodiscard]] ModifierRecalculationResult
retain_modifier_recalculation(const SqliteWorkSession &, const ApplicationPlanKey &expected_head,
                              const core::PlanOverrideDecision &admitted_authority,
                              const std::optional<QualifiedModifierCut> &,
                              std::span<const ModifierBaseEffect>,
                              const core::CapturedPlanRevision &next_revision,
                              const core::ModifierStackLimits &, const ModifierPlanAppend & = {});

// Structural admission of an already selected bounded local retained record.
[[nodiscard]] LocalModifierRecordShape
inspect_modifier_record_shape(const RetainedDocument &, bool expect_recalculation,
                              const core::ModifierStackLimits &);

[[nodiscard]] ModifierBindingProjection project_modifier_binding(
    const std::string &configuration_ref, const core::ModifierDefinition &exact_definition,
    std::size_t target_ordinal, const core::ModifierClockValue &qualified_clock_view,
    const std::optional<ModifierBindingName> &qualified_name);

} // namespace subathon::server
