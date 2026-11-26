// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <subathon/core/contribution.hpp>

namespace subathon::server::streamelements_astro {
enum class SourceTimeQuality { absent, reliable };
enum class Moderation { unknown, allowed, pending, rejected };
enum class Status {
    admitted,
    retry,
    conflict,
    held,
    recorded,
    pending_review,
    coverage_gap,
    source_binding_failure,
    ignored
};
enum class Gap {
    missing_tip_identity,
    missing_transaction,
    invalid_amount,
    invalid_currency,
    retired_delivery,
    reconnect_interruption
};

struct AcknowledgedRoute {
    std::string_view canonical_room;
    std::uint64_t generation;
    std::string_view installation;
    std::string_view stream;
};

struct Attempt {
    std::string installation;
    std::string stream;
    subathon::core::ReceiptInput receipt;
    subathon::core::PaymentEvidence evidence;
    subathon::core::EvidencePolicy policy;
    Moderation moderation;
    SourceTimeQuality source_time_quality;
    std::optional<std::string> reported_provider;
    std::optional<std::string> delivery_id;
};

struct NumericOverflow {
    std::string installation;
    std::string stream;
    subathon::core::ReceiptArrival arrival;
    subathon::core::ReceiptIdentity identity;
    subathon::core::ReceiptLinkReference payment_reference;
    std::string original_decimal;
    std::string currency;
};

class CoverageJournal {
  public:
    virtual ~CoverageJournal() = default;
    // The scoped gap owner obtains the last confirmed point from its own
    // durable frontier. The adapter supplies only the interruption trigger.
    virtual void record(Gap gap) = 0;
};

class HoldOwner {
  public:
    virtual ~HoldOwner() = default;
    virtual Status submit(Attempt attempt) = 0;
    virtual Status retain_overflow(NumericOverflow overflow) = 0;
};

// A caller supplies an acknowledged route and its scoped owners. The offline
// adapter does not open sockets, access credentials, or write durable state.
Status ingest(std::string_view frame, std::uint64_t generation, const AcknowledgedRoute &route,
              const subathon::core::ReceiptArrival &arrival, subathon::core::EvidencePolicy policy,
              HoldOwner &owner, CoverageJournal &journal);

enum class TransportStatus {
    awaiting_ack,
    acknowledged,
    source_binding_failure,
    retryable,
    retired_delivery,
    current_delivery,
    ordinary_reconnect,
    ignored,
    subscription_failed,
};
enum class TransportEvent {
    welcome,
    subscribe_success,
    authorization_deadline,
    rate_limit,
    graceful_reconnect,
    replacement_welcome,
    reconnect_rejected,
    delivery,
    binary_frame,
    subscribe_refused,
    acknowledgement_timeout,
    connection_rate_limit,
    draining,
    unexpected_disconnect,
};

struct TransportInput {
    TransportEvent event;
    std::uint64_t generation;
    std::string_view nonce{};
    std::string_view topic{};
    std::string_view room{};
    std::optional<std::uint32_t> retry_after_seconds{};
};

struct SubscriptionRequest {
    std::string topic;
    std::string token_type;
    std::string room;
    std::string nonce;
};

struct TransportState {
    std::string configured_room;
    std::optional<std::string> canonical_room;
    std::optional<std::uint64_t> active_generation;
    std::optional<std::uint64_t> replacement_generation;
    std::optional<std::string> pending_nonce;
    bool tips_read{};
    bool interruption_open{};
    std::optional<std::uint64_t> pending_generation;
    std::uint64_t nonce_counter{};
};

struct TransportResult {
    TransportStatus status;
    std::optional<SubscriptionRequest> subscribe;
    std::optional<std::uint32_t> retry_after_seconds{};
};

// Advance an already classified transport event. Credentials and reconnect
// tokens remain with the transport owner.
TransportResult advance(TransportState &state, TransportInput input, CoverageJournal &journal);
} // namespace subathon::server::streamelements_astro
