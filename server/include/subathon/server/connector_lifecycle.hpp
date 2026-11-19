// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <subathon/core/contribution.hpp>
#include <subathon/core/receipt.hpp>
#include <vector>

namespace subathon::server {
// API-005 proposal v1. Internal adapter/server boundary.
enum class ConnectorProvider {
    twitch,
    betterplace,
    patreon,
    streamlabs,
    streamelements,
    kofi,
    tipeeestream
};

// Release catalog membership conveys no provider capability or permission.
[[nodiscard]] std::vector<ConnectorProvider> connector_providers();

enum class ConnectorSupport { unknown, unsupported, supported };
enum class ConnectorEventField {
    source_timestamp,
    actor_id,
    actor_display_name,
    money,
    payment_evidence,
    delivery_id,
    event_id,
    activity_id,
    payment_id,
    refund_target
};
enum class ConnectorFeature {
    history,
    // Historical retrieval and replay of a connection gap are separate capabilities.
    gap_replay,
    refunds,
    application_registration,
    api_key_setup,
    webhook_setup,
    browser_link,
    public_client_setup,
    send_chat
};

struct ConnectorEventCapability {
    ConnectorSupport support{ConnectorSupport::unknown};
    std::map<ConnectorEventField, ConnectorSupport> fields;
};

struct ConnectorCapabilities {
    std::map<std::string, ConnectorEventCapability, std::less<>> events;
    std::map<ConnectorFeature, ConnectorSupport> features;

    // An omitted declaration is unknown. Preserve explicit unsupported entries.
    // Supported fields may still be absent in an individual observation.
    [[nodiscard]] ConnectorSupport event_support(std::string_view event) const;
    [[nodiscard]] ConnectorSupport field_support(std::string_view event,
                                                 ConnectorEventField field) const;
    [[nodiscard]] ConnectorSupport feature_support(ConnectorFeature feature) const;
};

enum class ConnectorInstallation { unknown, missing, validated, not_required };
enum class ConnectorConsent { unknown, missing, granted, not_required };
enum class ConnectorConnection { unknown, disconnected, connecting, connected };
enum class ConnectorHealth { unknown, setup_required, disconnected, connecting, connected };
enum class ConnectorGap { unknown, open, unrecoverable, recovered };
enum class ConnectorLink { unknown, unsupported, needs_setup, available };

struct ConnectorReport {
    ConnectorCapabilities capabilities;
    ConnectorInstallation installation{ConnectorInstallation::unknown};
    ConnectorConsent account_consent{ConnectorConsent::unknown};
    ConnectorConnection connection{ConnectorConnection::unknown};
    // The adapter reports recovery separately. Connected status never resolves a gap.
    ConnectorGap gap{ConnectorGap::unknown};
};

class ConnectorAdapter {
  public:
    virtual ~ConnectorAdapter() = default;
    // Read an existing report for this installation/source and selected account.
    // Reading status does not start transport, authorize accounts or send actions.
    [[nodiscard]] virtual ConnectorReport report() const = 0;
};

struct ConnectorSetupContext {
    // Optional exact, displayable callback URL from the streamer-hosted server's
    // configuration for this setup. Excludes authorization codes, keys and tokens.
    std::optional<std::string> installation_callback_url;
};

// Public-facing setup metadata has typed guidance and the installation callback only.
// Provider secrets, tokens and raw adapter diagnostics stay server-side.
struct ConnectorSetupGuidance {
    ConnectorInstallation installation;
    ConnectorConsent account_consent;
    ConnectorSupport application_registration;
    ConnectorSupport api_key_setup;
    ConnectorSupport webhook_setup;
    std::optional<std::string> callback_url;
    ConnectorLink browser_link;
};

struct ConnectorStatus {
    ConnectorSetupGuidance setup;
    ConnectorHealth health;
    ConnectorGap gap;
};

// Missing installation setup takes precedence over reported transport health. A browser
// link requires declared browser support and validated or unnecessary installation setup.
// Unknown browser support or unknown required setup stays unknown. A supported public
// client is a separate capability. Reported account consent is independent of both.
// Echo the configured callback exactly when present. The installation owns browser
// callbacks and exchanges directly with the provider. Project-operated authorization
// brokers, relays and token services are outside this boundary. This projection
// grants no application permission.
[[nodiscard]] ConnectorStatus read_connector_status(const ConnectorAdapter &adapter,
                                                    const ConnectorSetupContext &context);

enum class ConnectorIdentifier { delivery, event, activity, payment, refund_target };

// Adapter-normalized facts for intake, with any missing source facts still absent.
struct ConnectorEventObservation {
    core::ReceiptFacts facts;
    core::PaymentEvidence evidence{core::PaymentEvidence::unknown};
    std::optional<core::ReceiptLinkReference> delivery;
    std::optional<core::ReceiptLinkReference> event;
    std::optional<core::ReceiptLinkReference> refund_target;
};

struct ScopedConnectorIdentifier {
    core::ReceiptOrigin origin;
    core::ReceiptLinkReference reference;
    bool operator==(const ScopedConnectorIdentifier &) const = default;
};

struct ConnectorEvent {
    core::ReceiptOrigin origin;
    core::ReceiptArrival arrival;
    core::ReceiptFacts facts;
    core::PaymentEvidence evidence;
    std::map<ConnectorIdentifier, ScopedConnectorIdentifier> identifiers;
};

// Add the trusted source scope and server arrival to an observation. Retain exact facts,
// evidence and distinct reported identifier roles. A reported reference by itself
// cannot establish stable receipt identity, verified linkage, eligibility or a reversal.
[[nodiscard]] ConnectorEvent describe_connector_event(const core::ReceiptOrigin &origin,
                                                      const core::ReceiptArrival &arrival,
                                                      const ConnectorEventObservation &observation);
} // namespace subathon::server
