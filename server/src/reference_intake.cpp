// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/reference_intake.hpp>
#include <subathon/server/testing/reference_not_implemented.hpp>
namespace subathon::server {
ReferenceIntake::ReferenceIntake(ReferenceConfigurationSource &configuration,
                                 ReferenceClockSampler &clock, ReferenceEntropy &entropy,
                                 CurrentSessionClock &sessions,
                                 ReferenceConnectorRegistration connector,
                                 ReferenceApplicationAdapters *runtime,
                                 ReferenceRuntimeLimits limits)
    : configuration_(configuration), clock_(clock), entropy_(entropy), sessions_(sessions),
      connector_(std::move(connector)), runtime_(runtime), limits_(limits) {}
ReferencePreparedWork
ReferenceIntake::make_initial_work(const ReferenceInitialRequest &,
                                   std::function<void(ReferenceInitialCompletion)>) const {
    throw ReferenceNotImplemented("API-011 NI: intake.initial");
}
ReferencePreparedWork
ReferenceIntake::make_receipt_work(const std::string &, const ReferenceSyntheticInput &,
                                   std::function<void(ReferenceReceiptCompletion)>) const {
    throw ReferenceNotImplemented("API-011 NI: intake.receipt");
}
ReferencePreparedWork
ReferenceIntake::make_evidence_work(core::ReceiptId,
                                    std::function<void(ReferenceEvidenceCompletion)>) const {
    throw ReferenceNotImplemented("API-011 NI: intake.evidence");
}
ReferencePreparedWork
ReferenceIntake::make_capture_work(core::ReceiptId, core::ContributionId,
                                   std::function<void(ReferenceCaptureCompletion)>) const {
    throw ReferenceNotImplemented("API-011 NI: intake.capture");
}
ReferencePreparedWork
ReferenceIntake::make_snapshot_work(const SessionLookupRequest &,
                                    std::function<void(ReferenceSnapshotCompletion)>) const {
    throw ReferenceNotImplemented("API-011 NI: intake.snapshot");
}
} // namespace subathon::server
