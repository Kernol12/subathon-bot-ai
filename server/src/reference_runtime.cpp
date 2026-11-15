// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/reference_runtime.hpp>
#include <subathon/server/testing/reference_not_implemented.hpp>
namespace subathon::server {
ReferenceClockSampleResult SystemReferenceClockSampler::sample() {
    throw ReferenceNotImplemented("API-011 NI: clock.sample");
}
ReferenceEntropyRead OsReferenceEntropy::read32() {
    throw ReferenceNotImplemented("API-011 NI: entropy.read32");
}
ReferenceCommittedState decode_reference_state(const ApplicationStateSnapshot &,
                                               const ReferenceAdapterLimits &) {
    throw ReferenceNotImplemented("API-011 NI: state.decode");
}
ApplicationStateSnapshot encode_reference_state(const ReferenceCommittedState &,
                                                const ReferenceAdapterLimits &) {
    throw ReferenceNotImplemented("API-011 NI: state.encode");
}
ReferenceApplicationAdapters::ReferenceApplicationAdapters(
    ReferenceConfigurationSource &configuration, ReferenceAuthoritySource &authority,
    ReferenceClockSampler &clock, ReferenceEntropy &entropy,
    std::optional<ReferenceClockAnchor> anchor, ReferenceRuntimeLimits limits)
    : configuration_(configuration), authority_(authority), clock_(clock), entropy_(entropy),
      anchor_(std::move(anchor)), limits_(limits) {}
std::variant<ReferenceCommittedState, ReferenceUnavailable>
ReferenceApplicationAdapters::current(const ApplicationStateSnapshot &) const {
    throw ReferenceNotImplemented("API-011 NI: runtime.current");
}
std::optional<ReferenceUnavailable>
ReferenceApplicationAdapters::reserve(const ReferenceCommittedState &,
                                      const OriginalApplicationCommand &, ReferenceZeroStep) {
    throw ReferenceNotImplemented("API-011 NI: runtime.reserve");
}
void ReferenceApplicationAdapters::settle(const ApplicationServiceCompletion &,
                                          const std::optional<ReferenceCommittedState> &) {
    throw ReferenceNotImplemented("API-011 NI: runtime.settle");
}
void ReferenceApplicationAdapters::reconcile(const ReferenceCommittedState &) {
    throw ReferenceNotImplemented("API-011 NI: runtime.reconcile");
}
AccessDecision ReferenceApplicationAdapters::check(const ApplicationReferenceReadView &,
                                                   const OriginalApplicationCommand &) {
    throw ReferenceNotImplemented("API-011 NI: access.check");
}
bool ReferenceApplicationAdapters::disclose_application(const ApplicationReferenceReadView &,
                                                        const OriginalApplicationCommand &,
                                                        const RecordedApplicationBundle &) {
    throw ReferenceNotImplemented("API-011 NI: access.disclose_application");
}
bool ReferenceApplicationAdapters::disclose_holds(const ApplicationReferenceReadView &,
                                                  const OriginalApplicationCommand &,
                                                  const ApplicationHoldActiveSet &) {
    throw ReferenceNotImplemented("API-011 NI: access.disclose_holds");
}
PolicyResolution ReferenceApplicationAdapters::resolve(const ApplicationReferenceReadView &,
                                                       const OriginalApplicationCommand &,
                                                       const ApplicationBundleInputs &,
                                                       const AccessDecision &) {
    throw ReferenceNotImplemented("API-011 NI: policy.resolve");
}
ClockResolution ReferenceApplicationAdapters::observe(const ApplicationReferenceReadView &,
                                                      const OriginalApplicationCommand &,
                                                      const ApplicationBundleInputs &,
                                                      const BoundApplicationPolicy &) {
    throw ReferenceNotImplemented("API-011 NI: clock.observe");
}
ActionResolution ReferenceApplicationAdapters::prepare(const ApplicationReferenceReadView &,
                                                       const OriginalApplicationCommand &,
                                                       const ApplicationBundleInputs &,
                                                       const BoundApplicationPolicy &,
                                                       const ApplicationCalculatedProposal &) {
    throw ReferenceNotImplemented("API-011 NI: actions.prepare");
}
ReferenceResolution ReferenceApplicationAdapters::issue(ApplicationReferencePurpose,
                                                        const ApplicationReferenceBinding &) {
    throw ReferenceNotImplemented("API-011 NI: references.issue");
}
} // namespace subathon::server
