// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/connector_lifecycle.hpp>

namespace subathon::server {
namespace {
ConnectorLink browser_link(ConnectorSupport support, ConnectorInstallation installation) {
    if (support == ConnectorSupport::unsupported) {
        return ConnectorLink::unsupported;
    }
    if (support != ConnectorSupport::supported) {
        return ConnectorLink::unknown;
    }

    switch (installation) {
    case ConnectorInstallation::missing:
        return ConnectorLink::needs_setup;
    case ConnectorInstallation::validated:
    case ConnectorInstallation::not_required:
        return ConnectorLink::available;
    case ConnectorInstallation::unknown:
        return ConnectorLink::unknown;
    }
    return ConnectorLink::unknown;
}

ConnectorHealth connector_health(const ConnectorReport &report) {
    if (report.installation == ConnectorInstallation::missing) {
        return ConnectorHealth::setup_required;
    }

    switch (report.connection) {
    case ConnectorConnection::disconnected:
        return ConnectorHealth::disconnected;
    case ConnectorConnection::connecting:
        return ConnectorHealth::connecting;
    case ConnectorConnection::connected:
        return ConnectorHealth::connected;
    case ConnectorConnection::unknown:
        return ConnectorHealth::unknown;
    }
    return ConnectorHealth::unknown;
}
} // namespace

std::vector<ConnectorProvider> connector_providers() {
    using Provider = ConnectorProvider;
    return {Provider::twitch,      Provider::betterplace,    Provider::patreon,
            Provider::streamlabs,  Provider::streamelements, Provider::kofi,
            Provider::tipeeestream};
}

ConnectorSupport ConnectorCapabilities::event_support(std::string_view event) const {
    const auto entry = events.find(event);
    return entry == events.end() ? ConnectorSupport::unknown : entry->second.support;
}

ConnectorSupport ConnectorCapabilities::field_support(std::string_view event,
                                                      ConnectorEventField field) const {
    const auto entry = events.find(event);
    if (entry == events.end()) {
        return ConnectorSupport::unknown;
    }
    const auto &fields = entry->second.fields;
    const auto declaration = fields.find(field);
    return declaration == fields.end() ? ConnectorSupport::unknown : declaration->second;
}

ConnectorSupport ConnectorCapabilities::feature_support(ConnectorFeature feature) const {
    const auto declaration = features.find(feature);
    return declaration == features.end() ? ConnectorSupport::unknown : declaration->second;
}

ConnectorStatus read_connector_status(const ConnectorAdapter &adapter,
                                      const ConnectorSetupContext &context) {
    const auto report = adapter.report();
    const auto &capabilities = report.capabilities;
    return {
        .setup =
            {
                .installation = report.installation,
                .account_consent = report.account_consent,
                .application_registration =
                    capabilities.feature_support(ConnectorFeature::application_registration),
                .api_key_setup = capabilities.feature_support(ConnectorFeature::api_key_setup),
                .webhook_setup = capabilities.feature_support(ConnectorFeature::webhook_setup),
                .callback_url = context.installation_callback_url,
                .browser_link =
                    browser_link(capabilities.feature_support(ConnectorFeature::browser_link),
                                 report.installation),
            },
        .health = connector_health(report),
        .gap = report.gap,
    };
}

ConnectorEvent describe_connector_event(const core::ReceiptOrigin &origin,
                                        const core::ReceiptArrival &arrival,
                                        const ConnectorEventObservation &observation) {
    ConnectorEvent event{origin, arrival, observation.facts, observation.evidence, {}};
    const auto add_identifier = [&](ConnectorIdentifier role,
                                    const std::optional<core::ReceiptLinkReference> &reference) {
        if (reference) {
            event.identifiers.emplace(role, ScopedConnectorIdentifier{origin, *reference});
        }
    };
    add_identifier(ConnectorIdentifier::delivery, observation.delivery);
    add_identifier(ConnectorIdentifier::event, observation.event);
    add_identifier(ConnectorIdentifier::activity, observation.facts.activity_reference);
    add_identifier(ConnectorIdentifier::payment, observation.facts.payment_reference);
    add_identifier(ConnectorIdentifier::refund_target, observation.refund_target);
    return event;
}
} // namespace subathon::server
