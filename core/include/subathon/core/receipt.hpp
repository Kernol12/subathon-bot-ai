// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace subathon::core {
// Already-validated stable source identity, never a transport connection instance.
struct ReceiptOrigin {
    std::string provider;
    std::string source_account;
    bool operator==(const ReceiptOrigin &) const = default;
};

// The namespace must have documented delivery/event identity semantics at the source.
struct ReceiptIdentity {
    ReceiptOrigin origin;
    std::string identifier_namespace;
    std::string opaque_id;
    bool operator==(const ReceiptIdentity &) const = default;
};

struct ReceiptId {
    std::uint64_t value;
    bool operator==(const ReceiptId &) const = default;
};

// Linkage evidence has its own namespace; it does not replace receipt identity.
struct ReceiptLinkReference {
    std::string identifier_namespace;
    std::string opaque_id;
    bool operator==(const ReceiptLinkReference &) const = default;
};

struct ReceiptMoney {
    std::string original_decimal;
    std::string currency;
    bool operator==(const ReceiptMoney &) const = default;
};

// A bounded local intake projection, not the complete normalized-event wire schema.
struct ReceiptFacts {
    std::string event_type;
    std::uint32_t schema_version;
    std::optional<std::int64_t> source_timestamp_utc_ms;
    std::optional<std::string> actor_id;
    std::optional<std::string> actor_display_name;
    std::optional<ReceiptMoney> money;
    std::optional<ReceiptLinkReference> payment_reference;
    std::optional<ReceiptLinkReference> activity_reference;
    bool operator==(const ReceiptFacts &) const = default;
};

struct ReceiptArrival {
    std::int64_t server_timestamp_utc_ms;
    std::string connection_ref;
    // References must resolve to retained versioned context, not mutable current settings.
    std::string configuration_ref;
    std::string modifier_context_ref;
    bool operator==(const ReceiptArrival &) const = default;
};

enum class ReceiptDisposition { created, retry, conflict };

struct ReceiptAttempt {
    ReceiptArrival arrival;
    ReceiptFacts observed_facts;
    ReceiptDisposition disposition;
    bool operator==(const ReceiptAttempt &) const = default;
};

struct ReceiptRecord {
    ReceiptId id;
    ReceiptIdentity identity;
    ReceiptFacts original_facts;
    ReceiptArrival original_arrival;
    std::vector<ReceiptAttempt> attempts;
    bool operator==(const ReceiptRecord &) const = default;
};

struct ReceiptInput {
    ReceiptIdentity identity;
    ReceiptFacts facts;
    ReceiptArrival arrival;
};

struct ReceiptIntakeResult {
    ReceiptId receipt_id;
    ReceiptDisposition disposition;
};
} // namespace subathon::core
