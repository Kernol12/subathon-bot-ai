// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/application_plan.hpp>

namespace subathon::core {
ApplicationPlan capture_application_plan(std::string, const PlanCapture &, RulePlanSource &,
                                         ModifierPlanSource &) {
    throw ApplicationPlanNotImplemented{};
}
ApplicationPlan revise_application_plan(const ApplicationPlan &, std::uint64_t, const PlanCapture &,
                                        const PlanOverrideDecision &, RulePlanSource &,
                                        ModifierPlanSource &) {
    throw ApplicationPlanNotImplemented{};
}
PlanPreviewValidity check_plan_preview(const ApplicationPlan &, const PlanPreview &,
                                       const std::string &) {
    throw ApplicationPlanNotImplemented{};
}
} // namespace subathon::core
