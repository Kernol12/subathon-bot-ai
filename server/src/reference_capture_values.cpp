// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/reference_capture_values.hpp>

namespace subathon::server {
ApplicationEnvelope encode_reference_configuration(const ReferenceConfiguration &,
                                                   const ReferenceAdapterLimits &) {
    throw ReferenceCaptureValuesNotImplemented{};
}
ReferenceConfiguration decode_reference_configuration(const ApplicationEnvelope &,
                                                      const ReferenceAdapterLimits &) {
    throw ReferenceCaptureValuesNotImplemented{};
}
ApplicationEnvelope encode_reference_arrival(const ReferenceArrivalCapture &,
                                             const ReferenceAdapterLimits &) {
    throw ReferenceCaptureValuesNotImplemented{};
}
ReferenceArrivalCapture decode_reference_arrival(const ApplicationEnvelope &,
                                                 const ReferenceAdapterLimits &) {
    throw ReferenceCaptureValuesNotImplemented{};
}
ApplicationEnvelope encode_reference_execution(const ReferenceExecutionBinding &,
                                               const ReferenceAdapterLimits &) {
    throw ReferenceCaptureValuesNotImplemented{};
}
ReferenceExecutionBinding decode_reference_execution(const ApplicationEnvelope &,
                                                     const ReferenceAdapterLimits &) {
    throw ReferenceCaptureValuesNotImplemented{};
}
ApplicationEnvelope encode_reference_initial_capture(const ReferenceInitialCapture &,
                                                     const ReferenceAdapterLimits &) {
    throw ReferenceCaptureValuesNotImplemented{};
}
ReferenceInitialCapture decode_reference_initial_capture(const ApplicationEnvelope &,
                                                         const ReferenceAdapterLimits &) {
    throw ReferenceCaptureValuesNotImplemented{};
}
} // namespace subathon::server
