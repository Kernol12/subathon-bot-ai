// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/core/rate_snapshot.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace subathon::server::open_exchange_rates {

enum class Profile { unknown, free, developer, enterprise, unlimited };
enum class Operation { latest, historical_day, usage };
enum class IntentKind { automatic, manual, requalification };
enum class WaitEligibility { eligible, waiting, unknown };
enum class Dispatch { not_sent, sent, possibly_sent };

struct Elapsed {
    std::string timeline_ref;
    std::int64_t tick_ms;
    bool operator==(const Elapsed &) const = default;
};
struct Clocks {
    std::optional<std::int64_t> utc_ms;
    std::optional<Elapsed> elapsed;
    bool operator==(const Clocks &) const = default;
};
struct Selection {
    std::string installation_ref;
    std::string quota_scope_ref;
    std::string configuration_ref;
    std::string credential_revision;
    Profile profile;
    bool operator==(const Selection &) const = default;
};
struct FeatureQualification {
    Selection selection;
    bool alternative_base;
    bool historical_symbols;
};
struct SelectedSetup {
    Selection selection;
    bool profile_qualified;
    std::optional<FeatureQualification> features;
    std::optional<std::int64_t> display_allowance;
    bool ledger_continuity_qualified;
};
struct Request {
    Operation operation;
    std::string request_ref;
    std::string evidence_ref;
    std::optional<std::string> observation_ref;
    std::optional<std::string> base;
    std::vector<std::string> quotes;
    std::optional<std::string> utc_day;
    bool operator==(const Request &) const = default;
};

enum class HoldKind { quota, credential, provider_access, route_denial };
struct HoldIdentity {
    HoldKind kind;
    std::string installation_ref;
    std::string quota_scope_ref;
    std::optional<Operation> denied_operation;
    std::uint64_t generation;
    bool operator==(const HoldIdentity &) const = default;
};
enum class ProbeCause {
    corrected_credential,
    restored_access,
    restored_route,
    changed_entitlement
};
struct ProbeAuthority {
    HoldIdentity hold;
    Selection selection;
    ProbeCause cause;
    std::string evidence_ref;
};
struct OwnerIntent {
    Selection selection;
    IntentKind kind;
    bool route_authorized;
    bool due;
    Clocks now;
    std::optional<std::string> qualified_utc_day;
    std::chrono::milliseconds deadline;
    std::vector<ProbeAuthority> authorities;
    WaitEligibility applicable_owner_waits;
};
struct ExhaustionEvidence {
    Selection selection;
    std::string evidence_ref;
};

// Supplies qualified facts from the serialized owner, including the authority for each hold.
class OwnerPort {
  public:
    virtual ~OwnerPort() = default;
    [[nodiscard]] virtual SelectedSetup selected_setup() const = 0;
    [[nodiscard]] virtual OwnerIntent intent_for(const Request &) const = 0;
    [[nodiscard]] virtual std::optional<ExhaustionEvidence> exhaustion_evidence() const = 0;
};

using Headers = std::vector<std::pair<std::string, std::string>>;
struct AdmissionIdentity {
    Selection selection;
    std::string request_ref;
    std::uint64_t reservation_order;
    bool operator==(const AdmissionIdentity &) const = default;
};
struct Admission {
    AdmissionIdentity identity;
    Request canonical_request;
    std::string method;
    std::string url;
    std::string credential_header_name;
    bool follow_redirects;
    std::size_t max_header_bytes;
    std::size_t max_wire_bytes;
    std::size_t max_decoded_bytes;
    std::chrono::milliseconds deadline;
};
struct Started {
    Admission admission;
    bool follower;
};
enum class EchoMatch { match, mismatch, unknown };
struct EchoEvidence {
    AdmissionIdentity identity;
    EchoMatch match;
};
// The original sizes describe the received frame before removing the echoed credential.
struct SanitizedFrame {
    int status;
    Headers headers;
    std::string body;
    std::size_t wire_bytes;
    std::size_t decoded_bytes;
    Clocks receipt;
    std::optional<EchoEvidence> echo;
};
enum class FailureKind {
    invalid_selection,
    invalid_request,
    forbidden_feature,
    request_conflict,
    not_due,
    not_authorized,
    retry_wait,
    quota_hold,
    credential_hold,
    access_hold,
    route_hold,
    credential_rejected,
    credential_echo_mismatch,
    credential_echo_unverified,
    provider_access_restricted,
    route_denied,
    rate_limited,
    timeout,
    transport_failure,
    invalid_document,
    history_unverified,
    missing_rates,
    resource_limit,
    http_failure
};
enum class TransportFailureKind { timeout, transport, framing, resource_limit, invalid_document };
struct TransportFailure {
    TransportFailureKind kind;
    Clocks receipt;
};
struct Reply {
    AdmissionIdentity identity;
    std::variant<SanitizedFrame, TransportFailure> payload;
    Dispatch dispatch;
};

namespace private_transport {

// These DTOs stay within the synthetic credential/transport boundary.
struct Credential {
    Selection selection;
    std::string app_id;
};
struct Handoff {
    Admission admission;
    std::pair<std::string, std::string> authorization;
};
struct RawFrame {
    int status;
    Headers headers;
    std::string body;
    std::size_t wire_bytes;
    std::size_t decoded_bytes;
    Clocks receipt;
};

[[nodiscard]] std::optional<Handoff> make_handoff(const Admission &, const Credential &);
[[nodiscard]] Reply sanitize_reply(const Handoff &, RawFrame);

} // namespace private_transport

// Only the private port has access to the selected App ID. Completion returns through Reply.
class PrivateTransportPort {
  public:
    virtual ~PrivateTransportPort() = default;
    [[nodiscard]] virtual bool has_qualified_credential(const Selection &) const = 0;
    virtual void send(const Admission &) = 0;
};

struct Rates {
    Operation operation;
    core::FetchedRateObservation observation;
    std::vector<std::string> missing_quotes;
};
enum class AccountState { unknown, active, restricted };
struct FiniteQuota {
    std::int64_t total;
    std::int64_t remaining;
    bool operator==(const FiniteQuota &) const = default;
};
struct UnlimitedQuota {};
using Quota = std::variant<std::monostate, FiniteQuota, UnlimitedQuota>;
enum class UsageIssue { invalid_quota, invalid_metadata };
struct UsageSample {
    Selection selection;
    AccountState account;
    std::optional<std::string> plan_label;
    std::optional<std::string> quota_banner;
    std::optional<std::int64_t> refresh_interval_ms;
    std::optional<bool> base;
    std::optional<bool> symbols;
    Quota monthly_quota;
    std::optional<std::int64_t> requests;
    std::optional<std::int64_t> days_elapsed;
    std::optional<std::int64_t> days_remaining;
    std::optional<core::ExactDecimal> daily_average;
    Clocks receipt;
    std::vector<UsageIssue> issues;
};
struct RetryEvidence {
    std::optional<std::int64_t> relative_delay_ms;
    std::optional<Elapsed> receipt_elapsed;
    std::optional<std::int64_t> absolute_deadline_utc_ms;
};
struct Refusal {
    FailureKind kind;
    std::optional<int> http_status;
    RetryEvidence retry;
};
using Outcome = std::variant<Rates, UsageSample, Refusal>;
struct Completion {
    AdmissionIdentity identity;
    Outcome outcome;
    Dispatch dispatch;
};
struct Counts {
    std::optional<std::int64_t> http;
    std::optional<std::int64_t> metered;
    std::optional<std::int64_t> uncertain_http;
    bool operator==(const Counts &) const = default;
};
using UsageOutcome = std::variant<UsageSample, Refusal>;
struct OwnerView {
    std::optional<UsageOutcome> latest_usage_outcome;
    std::optional<UsageSample> last_usable_quota;
    std::optional<std::int64_t> quota_age_ms;
    std::optional<std::int64_t> quota_reset_utc_ms;
    Counts total;
    Counts since_sample;
    std::optional<std::int64_t> display_allowance;
    std::vector<HoldIdentity> holds;
    std::optional<core::FetchedRateObservation> latest;
    std::optional<std::int64_t> next_latest_utc_ms;
};

// Models the server owner's serialized in-memory path with supplied authority and transport.
// Real selection, credentials, scheduling persistence and keyed composition belong to #775
// or their owning services.
class Adapter {
  public:
    Adapter(OwnerPort &owner, PrivateTransportPort &transport)
        : owner_(owner), transport_(transport) {}

    void select();
    [[nodiscard]] std::variant<Started, Refusal> start(const Request &);
    [[nodiscard]] Completion finish(Reply);
    void observe_exhaustion();
    [[nodiscard]] OwnerView view(const Clocks &) const;

  private:
    OwnerPort &owner_;
    PrivateTransportPort &transport_;
};

class NotImplemented final : public std::logic_error {
  public:
    explicit NotImplemented(const char *operation) : std::logic_error(operation) {}
};

} // namespace subathon::server::open_exchange_rates
