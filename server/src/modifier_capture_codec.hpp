// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/modifier_capture.hpp>

namespace subathon::server::modifier_capture_detail {

struct DecisionBinding {
    core::ReceiptId receipt;
    core::ContributionId contribution;
    ModifierDecisionMode mode;
};

struct CapturedCut {
    // MCA1 retains the qualified observation reference, UTC and turn.
    std::string observation_ref;
    std::int64_t utc;
    std::uint64_t turn;
    ModifierTraceProfile profile;
    core::ModifierStackPolicy policy;
    std::vector<std::string> matched_qualifiers;
    std::vector<ModifierArrivalInstance> eligible;
};

struct DecodedCapture {
    core::ReceiptIdentity identity;
    core::ReceiptArrival arrival;
    IssuedModifierEvent event;
    std::optional<DecisionBinding> decision;
    std::optional<CapturedCut> cut;
};

// The input values are borrowed until the full size and shape pass succeeds.
RetainedDocument encode(const core::ReceiptIdentity &, const core::ReceiptArrival &,
                        const IssuedModifierEvent &, const QualifiedModifierCut *,
                        const core::PlanEvent *original, const ApplicationPlanKey *successor,
                        const core::PlanOverrideDecision *, core::ModifierStackLimits &);
DecodedCapture decode(const RetainedDocument &, bool expect_recalculation,
                      core::ModifierStackLimits &);

} // namespace subathon::server::modifier_capture_detail
