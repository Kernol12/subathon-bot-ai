// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <subathon/core/rate_snapshot.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace subathon::server::exchange_rate_api {

enum class Profile { open, free, pro, business, volume, unknown };
enum class Operation { latest, history, quota };
enum class RequestOrigin { automatic, manual };
enum class ScopeKind { open_usage, account_quota };

struct Scope {
    std::string installation_ref;
    ScopeKind kind;
    std::string scope_ref;
    bool operator==(const Scope &) const = default;
};

struct SelectionBinding {
    Profile profile;
    Scope scope;
    std::string configuration_ref;
    std::string selection_revision;
    std::optional<std::string> credential_revision;
    bool operator==(const SelectionBinding &) const = default;
};

struct Credential {
    SelectionBinding binding;
    std::string private_segment; // Raw secret bytes, supplied only by the trusted port.
};

enum class AuthorizedChange {
    unchanged,
    plan_entitlement,
    credential_entitlement,
    corrected_credential
};
enum class ScopeContinuity { qualified, unknown };

struct SelectionUpdate {
    SelectionBinding selection;
    std::optional<Credential> credential;
    std::optional<std::uint64_t> reference_allowance;
    AuthorizedChange change;
    ScopeContinuity continuity;
};

// Supplied by the trusted installation owner. Request DTOs carry no credentials.
class SelectionPort {
  public:
    virtual ~SelectionPort() = default;
    [[nodiscard]] virtual SelectionUpdate selected_setup() const = 0;
};

struct Elapsed {
    std::string clock_epoch;
    std::int64_t tick_ms;
    bool operator==(const Elapsed &) const = default;
};

struct Clocks {
    // UTC qualifies calendar hints. Only ticks in the same elapsed epoch prove duration.
    std::optional<std::int64_t> utc_ms;
    std::optional<Elapsed> elapsed;
};

struct Request {
    Operation operation;
    std::string request_ref;
    std::string evidence_ref;
    RequestOrigin origin;
    std::string base{};
    std::vector<std::string> quotes{};
    std::optional<std::string> history_date{};
    bool operator==(const Request &) const = default;
};

struct Admission {
    SelectionBinding selection;
    Request request;
    bool recovery_probe;
    bool operator==(const Admission &) const = default;
};

enum class DispatchKind { not_sent, sent, possibly_sent };
enum class SendTimeKind { exact, upper_bound };

struct SendTime {
    SendTimeKind kind;
    Elapsed elapsed;
    bool operator==(const SendTime &) const = default;
};

struct Dispatch {
    DispatchKind kind;
    std::optional<SendTime> time{};
    bool operator==(const Dispatch &) const = default;
};

struct HttpRequest {
    Admission admission;
    std::string method;
    std::string private_url;
    bool follow_redirects;
    std::size_t max_header_bytes;
    std::size_t max_wire_body_bytes;
    std::size_t max_decoded_body_bytes;
};

using Headers = std::vector<std::pair<std::string, std::string>>;

struct HttpReply {
    int status;
    Headers headers;
    std::string decoded_body;
    std::size_t wire_body_bytes;
    Clocks receipt;
};

enum class TransportFailure { timeout, cancelled, transport, framing, resource_limit };
struct FailedTransport {
    TransportFailure kind;
    Clocks receipt;
};

using Reply = std::variant<HttpReply, FailedTransport>;

class HttpPort {
  public:
    virtual ~HttpPort() = default;
    // One handoff for an admitted GET. Enforce byte ceilings before owning a reply.
    // A completion can arrive after this evidence, including after a selection change.
    [[nodiscard]] virtual Dispatch send(const HttpRequest &) = 0;
};

enum class SelectionStatus {
    selected,
    unknown_profile,
    reference_not_allowed,
    invalid_credential_binding,
    not_implemented
};
struct SelectedProfile {
    SelectionBinding binding;
    std::chrono::minutes nominal_refresh;
    bool latest;
    bool history;
    bool quota;
};

enum class FailureKind {
    not_implemented,
    unsupported_operation,
    invalid_selectors,
    coalesced,
    replay,
    authentication_hold,
    quota_hold,
    operation_unavailable,
    retry_wait,
    rate_limited,
    request_domain,
    invalid_document,
    resource_limit,
    failed_sync,
    superseded
};

enum class RetryEvidence { absent, invalid, delay, utc_deadline };
struct RetryAfter {
    RetryEvidence kind;
    std::optional<std::int64_t> delay_ms{};
    std::optional<std::int64_t> deadline_utc_ms{};
};
struct Failure {
    FailureKind kind;
    std::optional<int> status{};
    std::optional<std::string> safe_code{};
    RetryAfter retry_after{RetryEvidence::absent};
};

struct UnqualifiedEol {
    std::string member;
    std::string token;
    bool operator==(const UnqualifiedEol &) const = default;
};
struct UpdateTime {
    std::optional<std::int64_t> unix_seconds;
    std::optional<std::string> utc_text;
    std::optional<std::int64_t> parsed_text_utc_ms;
    std::optional<std::int64_t> qualified_utc_ms;
    bool disagrees;
};
struct UpdateEvidence {
    UpdateTime last;
    UpdateTime next;
    std::vector<UnqualifiedEol> eol;
};

struct OpenDisclosure {
    std::string attribution_text;
    std::string attribution_url;
    bool redistribution_restricted;
};
enum class RateUse { latest_candidate, history_only };
struct Rate {
    core::FetchedRateObservation observation;
    std::vector<std::string> missing_quotes;
    UpdateEvidence update;
    std::optional<std::string> matched_history_date;
    std::optional<OpenDisclosure> open_disclosure;
    RateUse use;
};

enum class RenewalState { absent, invalid, valid };
struct RenewalDay {
    RenewalState state;
    std::optional<unsigned> day;
    bool operator==(const RenewalDay &) const = default;
};
struct Quota {
    std::uint64_t plan_quota;
    std::uint64_t requests_remaining;
    RenewalDay renewal_day;
    Clocks receipt;
};

struct Usage {
    std::uint64_t total;
    std::uint64_t possibly_sent;
    bool operator==(const Usage &) const = default;
};
struct QuotaView {
    std::optional<Quota> sample;
    std::optional<std::int64_t> sample_age_ms;
    bool provider_may_lag;
    std::optional<Usage> local_total; // Absent when the owner's count continuity is unknown.
    std::optional<Usage> local_since_report;
    std::optional<std::uint64_t> reference_allowance;
    bool quota_held;
};

struct Started {
    Admission admission{};
    Dispatch dispatch{DispatchKind::not_sent};

    Started() = default;
    Started(Admission admitted, Dispatch dispatch_evidence)
        : admission(std::move(admitted)), dispatch(std::move(dispatch_evidence)) {}

  private:
    friend class Adapter;
    // Only start() can issue the reservation that authorizes a completion.
    std::shared_ptr<const Admission> reservation_;
};
using StartResult = std::variant<Started, Failure>;
struct Completion {
    Admission admission;
    Dispatch dispatch;
    std::variant<Rate, Quota, Failure> outcome;
};

// The serialized caller owns one admission/dispatch window and quota publication path.
// Rate results remain proposals for the separate snapshot owner.
class Adapter {
  public:
    Adapter(SelectionPort &selection, HttpPort &http);
    ~Adapter();
    Adapter(const Adapter &) = delete;
    Adapter &operator=(const Adapter &) = delete;

    [[nodiscard]] SelectionStatus select();
    [[nodiscard]] SelectionStatus edit_reference_allowance(std::optional<std::uint64_t>);
    [[nodiscard]] std::optional<SelectedProfile> current() const;
    [[nodiscard]] StartResult start(const Request &, const Clocks &);
    [[nodiscard]] Completion finish(const Started &, Reply);
    [[nodiscard]] QuotaView quota_view(const Scope &, const Clocks &) const;

  private:
    SelectionPort &selection_;
    HttpPort &http_;
    struct State;
    std::unique_ptr<State> state_;
};

} // namespace subathon::server::exchange_rate_api
