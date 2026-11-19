// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/connector_lifecycle.hpp>
#include <subathon/server/testing/connector_lifecycle_not_implemented.hpp>

namespace subathon::server {
std::vector<ConnectorProvider> connector_providers() {
    throw ConnectorLifecycleNotImplemented("connector_providers");
}

ConnectorSupport ConnectorCapabilities::event_support(std::string_view) const {
    throw ConnectorLifecycleNotImplemented("ConnectorCapabilities::event_support");
}

ConnectorSupport ConnectorCapabilities::field_support(std::string_view, ConnectorEventField) const {
    throw ConnectorLifecycleNotImplemented("ConnectorCapabilities::field_support");
}

ConnectorSupport ConnectorCapabilities::feature_support(ConnectorFeature) const {
    throw ConnectorLifecycleNotImplemented("ConnectorCapabilities::feature_support");
}

ConnectorStatus read_connector_status(const ConnectorAdapter &, const ConnectorSetupContext &) {
    throw ConnectorLifecycleNotImplemented("read_connector_status");
}

ConnectorEvent describe_connector_event(const core::ReceiptOrigin &, const core::ReceiptArrival &,
                                        const ConnectorEventObservation &) {
    throw ConnectorLifecycleNotImplemented("describe_connector_event");
}
} // namespace subathon::server
