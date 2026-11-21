// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <subathon/server/connector_lifecycle.hpp>
#include <subathon/server/sqlite_rate_overrides.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace subathon::server {

struct FrankfurterRequest {
    std::string configuration_ref;
    std::string request_ref;
    std::string evidence_ref;
    std::string base;
    std::vector<std::string> quotes;
    // Absent selects latest; present selects this exact historical date.
    std::optional<std::string> historical_date;
};

using FrankfurterHeaders = std::vector<std::pair<std::string, std::string>>;

struct FrankfurterHttpRequest {
    FrankfurterRequest admitted;
    std::string method;
    std::string url;
    FrankfurterHeaders headers;
    std::size_t max_wire_body_bytes;
    std::size_t max_decoded_body_bytes;
    std::size_t max_header_bytes;
    std::chrono::milliseconds deadline;
};

struct FrankfurterHttpFrame {
    int status;
    FrankfurterHeaders headers;
    std::string decoded_body;
    std::size_t wire_body_bytes;
    // Both values belong to the receipt, in distinct clock domains.
    std::optional<std::int64_t> receipt_utc_ms;
    std::optional<std::int64_t> receipt_monotonic_ms;
};

enum class FrankfurterTransportFailure { timeout, cancelled, transport, framing, resource_limit };
using FrankfurterHttpResult = std::variant<FrankfurterHttpFrame, FrankfurterTransportFailure>;

class FrankfurterHttpPort {
  public:
    virtual ~FrankfurterHttpPort() = default;
    // Enforce receive limits while forming a complete owned result. One call dispatches one GET.
    [[nodiscard]] virtual FrankfurterHttpResult exchange(const FrankfurterHttpRequest &) = 0;
};

enum class FrankfurterFailureKind {
    invalid_request,
    resource_limit,
    timeout,
    cancelled,
    transport,
    framing,
    rejected_request,
    missing_rates,
    limited,
    unavailable,
    http_failure,
    invalid_document,
    invalid_row
};

enum class FrankfurterWaitState { unknown, known, indeterminate };

struct FrankfurterWait {
    FrankfurterWaitState state;
    // A delay is evaluated against elapsed time, without adding to the receipt tick.
    std::optional<std::int64_t> delay_ms;
    std::optional<std::int64_t> receipt_monotonic_ms;
    std::optional<std::int64_t> deadline_utc_ms;
    std::optional<std::string> retained_hint;
};

struct FrankfurterFailure {
    FrankfurterFailureKind kind;
    std::optional<int> status;
    std::optional<FrankfurterRequest> admitted;
    FrankfurterWait wait;
};

struct FrankfurterHistoricalMatch {
    std::string quote;
    std::string requested_date;
    std::string returned_date;
    bool matches;
};

struct FrankfurterFetch {
    FrankfurterRequest admitted;
    std::vector<core::FetchedRateObservation> observations;
    std::vector<std::string> omitted_quotes;
    std::vector<FrankfurterHistoricalMatch> historical_matches;
};

using FrankfurterRequestResult = std::variant<FrankfurterHttpRequest, FrankfurterFailure>;
using FrankfurterFetchResult = std::variant<FrankfurterFetch, FrankfurterFailure>;

[[nodiscard]] FrankfurterRequestResult make_frankfurter_request(const FrankfurterRequest &);
[[nodiscard]] FrankfurterFetchResult normalize_frankfurter_response(const FrankfurterHttpRequest &,
                                                                    const FrankfurterHttpResult &);

class FrankfurterAdapter final : public ConnectorAdapter {
  public:
    explicit FrankfurterAdapter(FrankfurterHttpPort &port) : port_(port) {}
    [[nodiscard]] FrankfurterFetchResult fetch(const FrankfurterRequest &) const;
    [[nodiscard]] ConnectorReport report() const override;

  private:
    FrankfurterHttpPort &port_;
};

enum class FrankfurterLatestRefusal {
    historical,
    superseded,
    no_usable_quotes,
    regression,
    resource_limit
};

struct FrankfurterLatestProposal {
    core::FetchedRateObservation selected_observation;
    RateSourceBasis fetched_basis;
    std::vector<std::string> retained_quotes;
    std::vector<std::string> missing_quotes;
    core::EffectiveRateSnapshotInput candidate;
};

using FrankfurterLatestResult = std::variant<FrankfurterLatestProposal, FrankfurterLatestRefusal>;

// The owner supplies current admission and accepted provider-layer facts. A result is a proposal.
[[nodiscard]] FrankfurterLatestResult
propose_frankfurter_latest(const FrankfurterFetch &, const FrankfurterRequest &current_admission,
                           const core::EffectiveRateSnapshot &current, const RateSourceBasis &,
                           const std::string &candidate_ref);

enum class FrankfurterHistory { unknown, never_succeeded, accepted };
enum class FrankfurterWarning { unknown, clear, due };

struct FrankfurterFreshnessInput {
    FrankfurterHistory history;
    std::optional<std::int64_t> last_successful_fetch_utc_ms;
    std::span<const core::FetchedRateObservation> active_observations;
    std::optional<std::int64_t> now_utc_ms;
    std::chrono::milliseconds fetch_warning_after{std::chrono::hours{24}};
    std::chrono::milliseconds rate_date_warning_after{std::chrono::hours{72}};
};

struct FrankfurterObservationFreshness {
    std::string observation_ref;
    std::optional<std::int64_t> fetched_at_utc_ms;
    std::optional<std::int64_t> fetch_age_ms;
    std::optional<std::string> rate_date;
    FrankfurterWarning rate_date_warning;
    std::optional<std::int64_t> publication_age_ms;
};

struct FrankfurterFreshness {
    FrankfurterHistory history;
    std::optional<std::int64_t> last_successful_fetch_utc_ms;
    std::optional<std::int64_t> fetch_age_ms;
    FrankfurterWarning fetch_warning;
    std::vector<FrankfurterObservationFreshness> observations;
};

[[nodiscard]] FrankfurterFreshness assess_frankfurter_freshness(const FrankfurterFreshnessInput &);

class FrankfurterNotImplemented final : public std::logic_error {
  public:
    explicit FrankfurterNotImplemented(const char *operation) : std::logic_error(operation) {}
};

} // namespace subathon::server
