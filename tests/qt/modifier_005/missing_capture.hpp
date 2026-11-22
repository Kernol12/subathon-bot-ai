// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stdexcept>
#include <subathon/server/modifier_capture.hpp>

// Stage 1 only. These definitions mark the exact unimplemented entry and never
// return a successful value, domain refusal or transaction outcome.
namespace subathon::server {
namespace modifier_005_baseline {
[[noreturn]] inline void missing(const char *entry) {
    throw std::logic_error(std::string("MODIFIER-005 NI: ") + entry);
}
} // namespace modifier_005_baseline

inline ModifierArrivalPreparation prepare_modifier_arrival(const ModifierArrivalInput &,
                                                           const core::ModifierStackLimits &) {
    modifier_005_baseline::missing("prepare_modifier_arrival");
}
inline RetainDisposition retain_modifier_arrival(const SqliteWorkSession &,
                                                 const PreparedModifierArrival &) {
    modifier_005_baseline::missing("retain_modifier_arrival");
}
inline ModifierCaptureSelection
select_modifier_capture(const SqliteWorkSession &, const ContributionReceipt &,
                        const IssuedModifierEvent &, std::optional<core::ContributionId>,
                        const std::optional<ModifierDecisionMode> &, const ModifierTraceProfile &,
                        const core::ModifierStackLimits &, const ModifierExactInstanceRead &,
                        std::optional<std::span<const core::ModifierSelection>>) {
    modifier_005_baseline::missing("select_modifier_capture");
}
inline ModifierRecalculationResult retain_modifier_recalculation(
    const SqliteWorkSession &, const ApplicationPlanKey &, const core::PlanOverrideDecision &,
    const std::optional<QualifiedModifierCut> &, std::span<const ModifierBaseEffect>,
    const core::CapturedPlanRevision &, const core::ModifierStackLimits &) {
    modifier_005_baseline::missing("retain_modifier_recalculation");
}
inline LocalModifierRecordShape inspect_modifier_record_shape(const RetainedDocument &, bool,
                                                              const core::ModifierStackLimits &) {
    modifier_005_baseline::missing("inspect_modifier_record_shape");
}
inline ModifierBindingProjection
project_modifier_binding(const std::string &, const core::ModifierDefinition &, std::size_t,
                         const core::ModifierClockValue &,
                         const std::optional<ModifierBindingName> &) {
    modifier_005_baseline::missing("project_modifier_binding");
}
} // namespace subathon::server
