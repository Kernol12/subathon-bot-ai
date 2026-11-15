// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/reference_application.hpp>
namespace subathon::server {
ReferenceAdmissionResolution admit_reference_configuration(const std::string &,
                                                           const ReferenceConfigurationResolution &,
                                                           const ReferenceAdapterLimits &) {
    throw ReferenceApplicationNotImplemented{};
}
ReferenceRuleResolution evaluate_reference_rule(const ReferenceArrivalCapture &,
                                                const ReferenceConfigurationResolution &,
                                                const ReferenceAdapterLimits &) {
    throw ReferenceApplicationNotImplemented{};
}
AccessDecision check_reference_execution(const ApplicationReferenceReadView &,
                                         const OriginalApplicationCommand &,
                                         ReferenceConfigurationSource &, ReferenceAuthoritySource &,
                                         const ReferenceAdapterLimits &,
                                         const ReferenceAuthorityLimits &) {
    throw ReferenceApplicationNotImplemented{};
}
} // namespace subathon::server
