// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/open_exchange_rates.hpp>

#include "exchange_rate_api_time.hpp"
#include "open_exchange_rates_json.hpp"

#include <array>
#include <limits>
#include <ranges>
#include <stdexcept>

namespace subathon::server::open_exchange_rates {
namespace {
using namespace detail;
using namespace std::literals;
using exchange_rate_api::detail::http_date_ms;
using exchange_rate_api::detail::integer;
using exchange_rate_api::detail::iso_date;
using exchange_rate_api::detail::milliseconds_from_seconds;

constexpr auto origin = "https://openexchangerates.org"sv;
constexpr auto authorization = "Authorization"sv;
constexpr auto token_prefix = "Token "sv;
constexpr auto max_count = std::numeric_limits<std::int64_t>::max();

bool valid_ref(std::string_view ref) {
    return !ref.empty() && ref.size() <= core::RateSnapshotLimits{}.reference_bytes &&
           ref.find('\0') == std::string_view::npos;
}

bool valid_selection(const Selection &selection) {
    return valid_ref(selection.installation_ref) && valid_ref(selection.quota_scope_ref) &&
           valid_ref(selection.configuration_ref) && valid_ref(selection.credential_revision);
}

bool paid(Profile profile) {
    return profile == Profile::developer || profile == Profile::enterprise ||
           profile == Profile::unlimited;
}

bool valid_request(const Request &request) {
    if (!valid_ref(request.request_ref) || !valid_ref(request.evidence_ref))
        return false;
    switch (request.operation) {
    case Operation::usage:
        return !request.observation_ref && !request.base && request.quotes.empty() &&
               !request.utc_day;
    case Operation::historical_day:
        if (!request.utc_day || !iso_date(*request.utc_day))
            return false;
        break;
    case Operation::latest:
        if (request.utc_day)
            return false;
        break;
    default:
        return false;
    }
    return request.observation_ref && valid_ref(*request.observation_ref) && request.base &&
           currency_code(*request.base) && !request.quotes.empty() &&
           request.quotes.size() <= 255 &&
           std::ranges::all_of(request.quotes, [&](std::string_view quote) {
               return currency_code(quote) && quote != *request.base;
           });
}

std::string url_for(const Request &request, bool send_symbols) {
    std::string url{origin};
    if (request.operation == Operation::usage)
        return url + "/api/usage.json";
    if (request.operation == Operation::latest)
        url += "/api/latest.json";
    else
        url += "/api/historical/" + *request.utc_day + ".json";
    const bool send_base = *request.base != "USD";
    if (send_base)
        url += "?base=" + *request.base;
    if (send_symbols) {
        url += send_base ? "&symbols=" : "?symbols=";
        url += request.quotes.front();
        for (const auto &quote : request.quotes | std::views::drop(1))
            url += ',' + quote;
    }
    return url;
}

bool strictly_sorted(const std::vector<std::string> &values) {
    return std::ranges::is_sorted(values) && std::ranges::adjacent_find(values) == values.end();
}

bool valid_app_id(std::string_view id) {
    return id.size() == 32 && std::ranges::all_of(id, [](char c) {
               return digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
           });
}

bool headers_fit(const Headers &headers, std::size_t limit) {
    if (limit < 2)
        return false;
    auto remaining = limit - 2; // Final CRLF.
    for (const auto &[name, value] : headers) {
        if (remaining < 4 || name.size() > remaining - 4 ||
            value.size() > remaining - 4 - name.size())
            return false;
        remaining -= 4 + name.size() + value.size();
    }
    return true;
}

bool frame_fits(const Admission &admission, const Headers &headers, std::size_t wire,
                std::size_t decoded, std::size_t body) {
    const auto decoded_limit = std::min(admission.max_decoded_bytes, body_limit);
    return wire <= std::min(admission.max_wire_bytes, body_limit) && decoded <= decoded_limit &&
           body <= decoded_limit &&
           headers_fit(headers, std::min(admission.max_header_bytes, header_limit));
}

bool equal_header(std::string_view left, std::string_view right) {
    return std::ranges::equal(left, right, [](char a, char b) {
        return (a >= 'A' && a <= 'Z' ? a + ('a' - 'A') : a) == b;
    });
}

bool has_elapsed(const std::optional<Elapsed> &elapsed) {
    return elapsed && valid_ref(elapsed->timeline_ref);
}

std::optional<std::uint64_t> elapsed_since(std::optional<Elapsed> const &from,
                                           std::optional<Elapsed> const &to) {
    if (!has_elapsed(from) || !has_elapsed(to) || from->timeline_ref != to->timeline_ref ||
        to->tick_ms < from->tick_ms)
        return {};
    return static_cast<std::uint64_t>(to->tick_ms) - static_cast<std::uint64_t>(from->tick_ms);
}

std::optional<std::uint64_t> utc_since(std::optional<std::int64_t> from,
                                       std::optional<std::int64_t> to) {
    if (!from || !to || *to < *from)
        return {};
    return static_cast<std::uint64_t>(*to) - static_cast<std::uint64_t>(*from);
}

RetryEvidence retry_after(const SanitizedFrame &frame) {
    std::optional<std::string_view> field;
    for (const auto &[name, value] : frame.headers) {
        if (!equal_header(name, "retry-after"))
            continue;
        if (field)
            return {};
        const auto first = value.find_first_not_of(" \t");
        field =
            first == std::string::npos
                ? ""sv
                : std::string_view(value).substr(first, value.find_last_not_of(" \t") - first + 1);
    }
    if (!field || field->empty())
        return {};
    if (std::ranges::all_of(*field, digit)) {
        const auto seconds = integer<std::int64_t>(*field);
        if (seconds && (*seconds == 0 || has_elapsed(frame.receipt.elapsed))) {
            if (const auto ms = milliseconds_from_seconds(*seconds))
                return {*ms,
                        has_elapsed(frame.receipt.elapsed) ? frame.receipt.elapsed : std::nullopt,
                        {}};
        }
        return {};
    }
    if (frame.receipt.utc_ms)
        return {{}, {}, http_date_ms(*field, frame.receipt.utc_ms)};
    return {};
}

Refusal refuse(FailureKind kind, std::optional<int> status = {}, RetryEvidence retry = {}) {
    return {kind, status, std::move(retry)};
}

const JsonField *field(const JsonFields &fields, std::string_view name) {
    const auto it = fields.find(name);
    return it == fields.end() ? nullptr : &it->second;
}

const JsonFields &members(const JsonField *value) {
    static const JsonFields empty;
    return value && value->children ? *value->children : empty;
}

std::optional<std::string_view> string_value(const JsonField *value) {
    if (value && value->kind == JsonKind::string)
        return value->text;
    return {};
}

std::optional<bool> boolean_value(const JsonField *value) {
    if (value && value->kind == JsonKind::literal) {
        if (value->token == "true")
            return true;
        if (value->token == "false")
            return false;
    }
    return {};
}

std::optional<std::int64_t> integer_value(const JsonField *value, bool allow_unlimited = false) {
    if (!value || value->kind != JsonKind::number)
        return {};
    if (allow_unlimited && value->token == "-1")
        return -1;
    if (!std::ranges::all_of(value->token, digit))
        return {};
    return integer<std::int64_t>(value->token);
}

std::optional<int> safe_status(int status) {
    if (status >= 100 && status <= 599)
        return status;
    return {};
}

FailureKind classify_http(int status, const JsonFields &root) {
    using enum FailureKind;
    const auto *envelope_status = field(root, "status");
    if (envelope_status && integer_value(envelope_status) != status)
        return http_failure;

    const auto code = boolean_value(field(root, "error")) == true
                          ? string_value(field(root, "message"))
                          : std::nullopt;
    if (code == "invalid_app_id" || code == "missing_app_id")
        return status == 401 ? credential_rejected : http_failure;
    if (code == "access_restricted")
        return status == 403 || status == 429 ? provider_access_restricted : http_failure;
    if (code == "not_allowed")
        return status == 429 ? route_denied : http_failure;
    if (status == 401)
        return credential_rejected;
    return status == 429 ? rate_limited : http_failure;
}

Outcome normalize_rates(const Admission &admission, const JsonFields &root, const Clocks &receipt) {
    using enum FailureKind;
    const auto &request = admission.canonical_request;
    const auto *rate_map = field(root, "rates");
    if (string_value(field(root, "base")) != *request.base || !rate_map || !rate_map->children)
        return refuse(invalid_document, 200);
    core::RateTimeEvidence times;
    if (const auto *timestamp = field(root, "timestamp")) {
        const auto seconds = integer_value(timestamp);
        const auto ms = seconds ? milliseconds_from_seconds(*seconds) : std::nullopt;
        if (!ms)
            return refuse(invalid_document, 200);
        times = {std::string(timestamp->token), ms};
    }
    if (request.operation == Operation::historical_day) {
        const auto requested = std::chrono::sys_days{*iso_date(*request.utc_day)};
        const auto day_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(requested.time_since_epoch())
                .count();
        if (!times.utc_ms || *times.utc_ms < day_ms || *times.utc_ms >= day_ms + 86'400'000)
            return refuse(history_unverified, 200);
    }

    core::FetchedRateObservationInput input{{*request.observation_ref,
                                             "open_exchange_rates",
                                             admission.identity.selection.configuration_ref,
                                             request.request_ref,
                                             request.evidence_ref,
                                             {},
                                             receipt.utc_ms,
                                             times,
                                             times},
                                            {}};
    std::vector<std::string> missing;
    if (const auto *identity = field(*rate_map->children, *request.base)) {
        if (identity->kind != JsonKind::number ||
            core::parse_positive_rate(identity->token).value() != 1)
            return refuse(invalid_document, 200);
    }
    for (const auto &quote : request.quotes) {
        const auto *rate = field(*rate_map->children, quote);
        if (!rate)
            missing.push_back(quote);
        else if (rate->kind != JsonKind::number)
            return refuse(invalid_document, 200);
        else
            input.quotes.push_back({*request.base, quote, std::string(rate->token)});
    }
    if (input.quotes.empty())
        return refuse(missing_rates, 200);
    return Rates{request.operation, core::make_fetched_rate_observation(input), std::move(missing)};
}

Outcome normalize_usage(const Admission &admission, const JsonFields &root,
                        const SanitizedFrame &frame) {
    using enum FailureKind;
    const auto *data = field(root, "data");
    if (integer_value(field(root, "status")) != 200 || !data || !data->children)
        return refuse(invalid_document, 200);
    if (!frame.echo || !frame.echo->verified_for(admission.identity) ||
        frame.echo->match == EchoMatch::unknown)
        return refuse(credential_echo_unverified, 200);
    if (frame.echo->match != EchoMatch::match)
        return refuse(credential_echo_mismatch, 200);

    const auto &body = *data->children;
    if (field(body, "app_id"))
        return refuse(invalid_document, 200);
    const auto status = string_value(field(body, "status"));
    UsageSample sample{};
    sample.selection = admission.identity.selection;
    sample.account = status == "active"              ? AccountState::active
                     : status == "access_restricted" ? AccountState::restricted
                                                     : AccountState::unknown;
    sample.receipt = frame.receipt;
    const auto *plan_field = field(body, "plan");
    const auto &plan = members(plan_field);
    const auto *features_field = field(plan, "features");
    const auto &features = members(features_field);
    const auto *usage_field = field(body, "usage");
    const auto &usage = members(usage_field);

    // Optional fields can fail independently without invalidating the envelope.
    const auto not_object = [](const JsonField *value) { return value && !value->children; };
    bool invalid_metadata =
        not_object(plan_field) || not_object(features_field) || not_object(usage_field);
    const auto display = [&](std::string_view key) -> std::optional<std::string> {
        const auto *value = field(plan, key);
        const auto text = string_value(value);
        if (text && text->size() <= 128)
            return std::string(*text);
        invalid_metadata |= value != nullptr;
        return {};
    };
    const auto feature = [&](std::string_view key) {
        const auto *value = field(features, key);
        const auto flag = boolean_value(value);
        invalid_metadata |= value && !flag.has_value();
        return flag;
    };
    const auto count = [&](std::string_view key, std::int64_t max) -> std::optional<std::int64_t> {
        const auto *value = field(usage, key);
        const auto amount = integer_value(value);
        if (amount && *amount <= max)
            return amount;
        invalid_metadata |= value != nullptr;
        return {};
    };
    sample.plan_label = display("name");
    sample.quota_banner = display("quota");
    if (const auto *value = field(plan, "update_frequency")) {
        if (string_value(value) == "30-minute")
            sample.refresh_interval_ms = 1'800'000;
        else
            invalid_metadata = true;
    }
    sample.base = feature("base");
    sample.symbols = feature("symbols");

    const auto *quota = field(usage, "requests_quota");
    const auto *remaining = field(usage, "requests_remaining");
    const auto total_number = integer_value(quota, true);
    const auto remaining_number = integer_value(remaining, true);
    if (total_number == -1 && remaining_number == -1)
        sample.monthly_quota = UnlimitedQuota{};
    else if (total_number && *total_number >= 0 && remaining_number && *remaining_number >= 0 &&
             *remaining_number <= *total_number)
        sample.monthly_quota = FiniteQuota{*total_number, *remaining_number};
    else if (quota || remaining)
        sample.issues.push_back(UsageIssue::invalid_quota);

    sample.requests = count("requests", max_count);
    sample.days_elapsed = count("days_elapsed", 31);
    sample.days_remaining = count("days_remaining", 31);
    if (const auto *average = field(usage, "daily_average")) {
        if (average->kind == JsonKind::number) {
            try {
                sample.daily_average = core::parse_nonnegative_factor(average->token);
            } catch (const std::invalid_argument &) {
                invalid_metadata = true;
            } catch (const std::length_error &) {
                invalid_metadata = true;
            }
        } else {
            invalid_metadata = true;
        }
    }
    if (invalid_metadata)
        sample.issues.push_back(UsageIssue::invalid_metadata);
    return sample;
}

Outcome normalize(const Admission &admission, const SanitizedFrame &frame) {
    using enum FailureKind;
    const std::optional<int> status = safe_status(frame.status);
    if (!frame_fits(admission, frame.headers, frame.wire_bytes, frame.decoded_bytes,
                    frame.body.size()))
        return refuse(resource_limit, status);
    if (!status || frame.body.size() > frame.decoded_bytes)
        return refuse(invalid_document, status);
    const auto retry = retry_after(frame);
    try {
        const auto root =
            frame.status != 200 && frame.body.find_first_not_of(" \r\n\t") == std::string::npos
                ? JsonFields{}
                : ReplyJson(frame.body).read();
        if (frame.status != 200)
            return refuse(classify_http(frame.status, root), status, retry);
        if (const auto *reported = field(root, "status");
            reported && integer_value(reported) != frame.status)
            return refuse(http_failure, status, retry);
        if (const auto *error = field(root, "error")) {
            const auto flag = boolean_value(error);
            if (flag != false)
                return refuse(flag ? http_failure : invalid_document, status, retry);
        }
        auto outcome = admission.canonical_request.operation == Operation::usage
                           ? normalize_usage(admission, root, frame)
                           : normalize_rates(admission, root, frame.receipt);
        if (auto *failure = std::get_if<Refusal>(&outcome))
            failure->retry = retry;
        return outcome;
    } catch (FailureKind kind) {
        if (frame.status != 200 && kind == invalid_document)
            return refuse(classify_http(frame.status, {}), status, retry);
        return refuse(kind, status, retry);
    } catch (const core::RateSnapshotFailure &failure) {
        return refuse(failure.code() == core::RateSnapshotError::resource_limit ? resource_limit
                                                                                : invalid_document,
                      status, retry);
    } catch (const std::length_error &) {
        return refuse(resource_limit, status, retry);
    } catch (const std::invalid_argument &) {
        return refuse(invalid_document, status, retry);
    }
}

FailureKind transport_failure(TransportFailureKind kind) {
    switch (kind) {
    case TransportFailureKind::timeout:
        return FailureKind::timeout;
    case TransportFailureKind::transport:
    case TransportFailureKind::framing:
        return FailureKind::transport_failure;
    case TransportFailureKind::resource_limit:
        return FailureKind::resource_limit;
    case TransportFailureKind::invalid_document:
        return FailureKind::invalid_document;
    }
    return FailureKind::transport_failure;
}

} // namespace

bool EchoEvidence::verified_for(const AdmissionIdentity &admission) const {
    return proof_ && proof_->first == admission && proof_->first == identity &&
           proof_->second == match;
}

namespace private_transport {

std::optional<Handoff> make_handoff(const Admission &admission, const Credential &credential) {
    const auto &request = admission.canonical_request;
    if (credential.selection != admission.identity.selection || !valid_app_id(credential.app_id) ||
        !valid_selection(admission.identity.selection) || !valid_request(request) ||
        request.request_ref != admission.identity.request_ref ||
        !admission.identity.reservation_order || !strictly_sorted(request.quotes) ||
        admission.method != "GET" || admission.credential_header_name != authorization ||
        admission.follow_redirects || admission.max_header_bytes != header_limit ||
        admission.max_wire_bytes != body_limit || admission.max_decoded_bytes != body_limit ||
        admission.deadline.count() <= 0)
        return {};
    const auto profile = admission.identity.selection.profile;
    if (request.operation != Operation::usage && (!(profile == Profile::free || paid(profile)) ||
                                                  (*request.base != "USD" && !paid(profile))))
        return {};
    const bool can_filter = request.operation == Operation::latest ||
                            (request.operation == Operation::historical_day && paid(profile));
    if (admission.url != url_for(request, false) &&
        !(can_filter && admission.url == url_for(request, true)))
        return {};
    return Handoff{admission,
                   {std::string(authorization), std::string(token_prefix) + credential.app_id}};
}

Reply sanitize_reply(const Handoff &handoff, RawFrame raw) {
    const auto &admission = handoff.admission;
    const auto failure = [&](TransportFailureKind kind) -> Reply {
        return {admission.identity, TransportFailure{kind, std::move(raw.receipt)}, Dispatch::sent};
    };
    if (!frame_fits(admission, raw.headers, raw.wire_bytes, raw.decoded_bytes, raw.body.size()))
        return failure(TransportFailureKind::resource_limit);
    if (raw.body.size() != raw.decoded_bytes || !safe_status(raw.status))
        return failure(TransportFailureKind::framing);

    const std::string_view token = handoff.authorization.second;
    if (handoff.authorization.first != authorization || !token.starts_with(token_prefix) ||
        !valid_app_id(token.substr(token_prefix.size())))
        return failure(TransportFailureKind::framing);
    const auto sent_id = token.substr(token_prefix.size());
    const auto reflected = [&](std::string_view text, std::string_view echoed_id) {
        return text.find(sent_id) != std::string_view::npos ||
               (!echoed_id.empty() && text.find(echoed_id) != std::string_view::npos);
    };

    std::optional<EchoEvidence> echo;
    std::string echoed_id;
    try {
        if (admission.canonical_request.operation == Operation::usage)
            echo = EchoEvidence{admission.identity, EchoMatch::unknown};
        if (raw.status == 200 || raw.body.find_first_not_of(" \r\n\t") != std::string::npos) {
            if (echo) {
                // Validate the complete original document, including escaped duplicate keys,
                // before determining the echo fact or modifying any bytes.
                const auto original = ReplyJson(raw.body).read();
                const auto &data = members(field(original, "data"));
                if (const auto *value = field(data, "app_id")) {
                    if (const auto id = string_value(value); id && valid_app_id(*id)) {
                        echoed_id = *id;
                        echo->match = *id == sent_id ? EchoMatch::match : EchoMatch::mismatch;
                    }
                    auto first = static_cast<std::size_t>(value->member.data() - raw.body.data());
                    auto end = first + value->member.size();
                    const auto next = raw.body.find_first_not_of(" \r\n\t", end);
                    if (next != std::string::npos && raw.body[next] == ',') {
                        end = next + 1;
                    } else if (first != 0) {
                        const auto previous = raw.body.find_last_not_of(" \r\n\t", first - 1);
                        if (previous != std::string::npos && raw.body[previous] == ',')
                            first = previous;
                    }
                    raw.body.erase(first, end - first);
                }
            }
            // Check decoded strings and member names, as well as numeric lexemes, for reflections
            // outside the echo field. A reflected credential invalidates the sanitized frame.
            if (reflected(raw.body, echoed_id))
                return failure(TransportFailureKind::invalid_document);
            static_cast<void>(ReplyJson(raw.body, sent_id, echoed_id).read());
        }
    } catch (FailureKind kind) {
        if (kind == FailureKind::resource_limit)
            return failure(TransportFailureKind::resource_limit);
        if (raw.status == 200)
            return failure(TransportFailureKind::invalid_document);
        // An invalid error document supplies no business code or echo. Keep only HTTP evidence.
        raw.body.clear();
        echo.reset();
    }

    // Retain only the headers consumed as time evidence, without changing duplicate semantics.
    std::erase_if(raw.headers, [](const auto &header) {
        return !equal_header(header.first, "date") && !equal_header(header.first, "retry-after");
    });
    if (std::ranges::any_of(
            raw.headers, [&](const auto &header) { return reflected(header.second, echoed_id); }))
        return failure(TransportFailureKind::invalid_document);
    if (echo)
        echo->proof_.emplace(echo->identity, echo->match);
    return {admission.identity,
            SanitizedFrame{raw.status, std::move(raw.headers), std::move(raw.body), raw.wire_bytes,
                           raw.decoded_bytes, std::move(raw.receipt), std::move(echo)},
            Dispatch::sent};
}

} // namespace private_transport

namespace {
using Order = std::uint64_t;
using ScopeKey = std::pair<std::string, std::string>;

ScopeKey scope_key(const Selection &selection) {
    return {selection.installation_ref, selection.quota_scope_ref};
}

struct Probe {
    HoldIdentity hold;
    ProbeCause cause;
    std::string evidence_ref;
    bool operator==(const Probe &) const = default;
};

struct Hold {
    HoldIdentity identity;
    Order entered_after;
    std::string rejected_credential;
    std::set<std::string, std::less<>> spent_evidence;
    std::optional<std::string> probed_credential;
};

struct LocalCounts {
    std::int64_t http{};
    std::int64_t metered{};
    std::int64_t uncertain{};
};

struct Wait {
    Order failure_order;
    Clocks receipt;
    std::uint64_t delay_ms;
    std::optional<std::int64_t> absolute_deadline;

    bool ready(const OwnerIntent &intent) const {
        const auto passed = has_elapsed(receipt.elapsed)
                                ? elapsed_since(receipt.elapsed, intent.now.elapsed)
                                : utc_since(receipt.utc_ms, intent.now.utc_ms);
        const bool relative_ready =
            delay_ms == 0 ||
            (passed ? *passed >= delay_ms : intent.qualified_local_retry_order == failure_order);
        return relative_ready && (!absolute_deadline || (intent.now.utc_ms && receipt.utc_ms &&
                                                         *intent.now.utc_ms >= *receipt.utc_ms &&
                                                         *intent.now.utc_ms >= *absolute_deadline));
    }
};

struct Lane {
    std::optional<Wait> wait;
    std::vector<Wait> provider_waits;
    std::size_t retry_step{};
    Order last_result{};

    bool ready(const OwnerIntent &intent) const {
        return (!wait || wait->ready(intent)) &&
               std::ranges::all_of(provider_waits, [&](const Wait &provider_wait) {
                   return provider_wait.ready(intent);
               });
    }

    void retry(Order order, const Clocks &receipt) {
        constexpr std::array<std::uint64_t, 4> delays{60'000, 300'000, 900'000, 3'600'000};
        wait = Wait{order, receipt, delays[retry_step], {}};
        retry_step = std::min(retry_step + 1, delays.size() - 1);
    }

    void observe_provider_wait(Order order, const Clocks &receipt, const RetryEvidence &retry) {
        if (retry.relative_delay_ms || retry.absolute_deadline_utc_ms)
            provider_waits.push_back(
                {order,
                 {receipt.utc_ms, retry.receipt_elapsed},
                 static_cast<std::uint64_t>(retry.relative_delay_ms.value_or(0)),
                 retry.absolute_deadline_utc_ms});
    }

    void succeeded() {
        wait.reset();
        retry_step = 0;
    }
};

struct RefClaim {
    Order reservation;
    std::string evidence;
    std::optional<std::string> observation;
};

struct ScopeState {
    bool ledger_known{true};
    LocalCounts totals;
    std::optional<LocalCounts> at_sample;
    std::optional<UsageSample> usable_quota;
    std::vector<Hold> holds;
    std::set<std::string, std::less<>> exhaustion_refs;
    std::map<std::string, RefClaim, std::less<>> requests;
    std::set<std::string, std::less<>> evidence_refs;
    std::set<std::string, std::less<>> observation_refs;
    std::array<Lane, 3> lanes;
    std::optional<core::FetchedRateObservation> latest;
    std::optional<std::int64_t> last_latest_fetch;
    std::optional<std::int64_t> next_latest;

    Lane &lane(Operation operation) { return lanes[static_cast<std::size_t>(operation)]; }

    void count(Dispatch dispatch, Operation operation) {
        if (!ledger_known || dispatch == Dispatch::not_sent)
            return;
        if (totals.http == max_count) {
            ledger_known = false;
            return;
        }
        ++totals.http;
        if (operation != Operation::usage)
            ++totals.metered;
        if (dispatch == Dispatch::possibly_sent)
            ++totals.uncertain;
    }

    void claim(const Request &request, Order reservation) {
        requests.emplace(request.request_ref,
                         RefClaim{reservation, request.evidence_ref, request.observation_ref});
        evidence_refs.insert(request.evidence_ref);
        if (request.observation_ref)
            observation_refs.insert(*request.observation_ref);
    }
};

struct Pending {
    Admission admission;
    Order selection_epoch;
    IntentKind intent;
    std::vector<Probe> probes;
};

bool same_operation(const Request &left, const Request &right) {
    return left.operation == right.operation && left.base == right.base &&
           left.quotes == right.quotes && left.utc_day == right.utc_day;
}

bool applicable(const Hold &hold, Operation operation) {
    if (hold.identity.kind == HoldKind::quota)
        return operation != Operation::usage;
    return !hold.identity.denied_operation || hold.identity.denied_operation == operation;
}

FailureKind hold_failure(HoldKind kind) {
    switch (kind) {
    case HoldKind::quota:
        return FailureKind::quota_hold;
    case HoldKind::credential:
        return FailureKind::credential_hold;
    case HoldKind::provider_access:
        return FailureKind::access_hold;
    case HoldKind::route_denial:
        return FailureKind::route_hold;
    }
    return FailureKind::invalid_selection;
}

bool qualifies(const ProbeAuthority &authority, const Hold &hold, const Selection &selection,
               Operation operation) {
    if (authority.hold != hold.identity || authority.selection != selection ||
        !valid_ref(authority.evidence_ref))
        return false;
    using enum ProbeCause;
    switch (hold.identity.kind) {
    case HoldKind::quota:
        return false;
    case HoldKind::credential:
        return operation == Operation::usage &&
               ((authority.cause == corrected_credential &&
                 selection.credential_revision != hold.rejected_credential) ||
                (authority.cause == restored_credential &&
                 selection.credential_revision == hold.probed_credential));
    case HoldKind::provider_access:
        return operation == Operation::usage &&
               (authority.cause == restored_access || authority.cause == changed_entitlement);
    case HoldKind::route_denial:
        return authority.cause == restored_route || authority.cause == changed_entitlement;
    }
    return false;
}

std::optional<std::int64_t> sample_age(const Clocks &receipt, const Clocks &now) {
    auto age = elapsed_since(receipt.elapsed, now.elapsed);
    if (!age)
        age = utc_since(receipt.utc_ms, now.utc_ms);
    if (age && *age <= static_cast<std::uint64_t>(max_count))
        return static_cast<std::int64_t>(*age);
    return {};
}

bool preferred_latest(const core::FetchedRateObservation &candidate,
                      const std::optional<core::FetchedRateObservation> &previous) {
    if (!previous)
        return true;
    const auto old_time = previous->data().context.source_rate_time.utc_ms;
    const auto new_time = candidate.data().context.source_rate_time.utc_ms;
    return !old_time || (new_time && *new_time >= *old_time);
}

void accept_latest(ScopeState &scope, const Rates &rates, IntentKind intent) {
    const auto fetched = rates.observation.data().context.fetched_at_utc_ms;
    scope.last_latest_fetch = fetched;
    if (preferred_latest(rates.observation, scope.latest))
        scope.latest = rates.observation;
    // An accepted fetch qualifies scheduling independently of observation replacement.
    constexpr std::int64_t interval = 43'200'000; // 12 hours.
    if (!fetched)
        return;
    if (!scope.next_latest) {
        if (*fetched <= max_count - interval)
            scope.next_latest = *fetched + interval;
        return;
    }
    if (intent != IntentKind::automatic || *fetched < *scope.next_latest)
        return;
    // Advance from the original schedule, even when an automatic success was late.
    const auto late = *utc_since(scope.next_latest, fetched);
    const auto until_next = interval - static_cast<std::int64_t>(late % interval);
    if (*fetched <= max_count - until_next)
        scope.next_latest = *fetched + until_next;
    else
        scope.next_latest.reset();
}

} // namespace

struct Adapter::State {
    std::optional<SelectedSetup> setup;
    Order selection_epoch{};
    Order last_reservation{};
    Order last_hold{};
    std::map<ScopeKey, ScopeState> scopes;
    std::map<Order, Pending> pending;

    // Current evidence is fenced by the installed selection. Usable quota and local
    // accounting have a different lifetime and live with the trusted quota scope.
    std::optional<Completion> current;
    std::optional<UsageOutcome> latest_usage;
    std::optional<bool> reported_base;
    std::optional<bool> reported_symbols;

    void hold(ScopeState &scope, const Selection &selection, HoldKind kind,
              std::optional<Operation> denied = {}) {
        const auto existing = std::ranges::find_if(scope.holds, [&](const auto &held) {
            return held.identity.kind == kind && held.identity.denied_operation == denied;
        });
        if (existing != scope.holds.end()) {
            if (kind == HoldKind::credential)
                existing->rejected_credential = selection.credential_revision;
            if (kind == HoldKind::quota)
                existing->entered_after = last_reservation;
            return;
        }
        if (last_hold == std::numeric_limits<Order>::max())
            throw std::overflow_error("Open Exchange Rates hold sequence exhausted");
        scope.holds.push_back(
            {{kind, selection.installation_ref, selection.quota_scope_ref, denied, ++last_hold},
             last_reservation,
             kind == HoldKind::credential ? selection.credential_revision : "",
             {},
             {}});
    }

    void accept_usage(ScopeState &scope, const Pending &request, const UsageSample &sample) {
        latest_usage = sample;
        // An unavailable value is visible in this report but does not retract a
        // previous explicit feature claim for the same selected revision.
        if (sample.base)
            reported_base = sample.base;
        if (sample.symbols)
            reported_symbols = sample.symbols;
        if (sample.account == AccountState::restricted)
            hold(scope, sample.selection, HoldKind::provider_access);
        if (!std::holds_alternative<std::monostate>(sample.monthly_quota)) {
            scope.usable_quota = sample;
            scope.at_sample = scope.totals;
        }
        const auto *finite = std::get_if<FiniteQuota>(&sample.monthly_quota);
        if (finite && finite->remaining == 0) {
            hold(scope, sample.selection, HoldKind::quota);
        } else if (sample.account == AccountState::active &&
                   (finite || std::holds_alternative<UnlimitedQuota>(sample.monthly_quota))) {
            std::erase_if(scope.holds, [&](const Hold &held) {
                return held.identity.kind == HoldKind::quota &&
                       request.admission.identity.reservation_order > held.entered_after;
            });
        }
    }

    void accept(ScopeState &scope, const Pending &request, const Completion &result,
                const Clocks &receipt) {
        const auto order = result.identity.reservation_order;
        const auto operation = request.admission.canonical_request.operation;
        auto &lane = scope.lane(operation);
        if (order <= lane.last_result)
            return;
        lane.last_result = order;
        current = result;

        if (const auto *failure = std::get_if<Refusal>(&result.outcome)) {
            if (operation == Operation::usage)
                latest_usage = *failure;
            lane.observe_provider_wait(order, receipt, failure->retry);
            switch (failure->kind) {
            case FailureKind::credential_rejected:
                hold(scope, result.identity.selection, HoldKind::credential);
                break;
            case FailureKind::provider_access_restricted:
                hold(scope, result.identity.selection, HoldKind::provider_access);
                break;
            case FailureKind::route_denied:
                hold(scope, result.identity.selection, HoldKind::route_denial, operation);
                break;
            default:
                lane.retry(order, receipt);
                break;
            }
            return;
        }

        lane.succeeded();
        const auto *usage = std::get_if<UsageSample>(&result.outcome);
        if (usage)
            accept_usage(scope, request, *usage);
        else if (operation == Operation::latest)
            accept_latest(scope, std::get<Rates>(result.outcome), request.intent);

        if (!usage || usage->account == AccountState::active) {
            std::erase_if(scope.holds, [&](const Hold &held) {
                return std::ranges::any_of(request.probes, [&](const Probe &probe) {
                    return held.identity == probe.hold;
                });
            });
        }
    }
};

Adapter::Adapter(OwnerPort &owner, PrivateTransportPort &transport)
    : owner_(owner), transport_(transport), state_(std::make_unique<State>()) {}

Adapter::~Adapter() = default;

void Adapter::select() {
    auto setup = owner_.selected_setup();
    const bool changed = !state_->setup || state_->setup->selection != setup.selection;
    const bool same_schedule_scope =
        state_->setup && scope_key(state_->setup->selection) == scope_key(setup.selection) &&
        state_->setup->selection.profile == setup.selection.profile &&
        state_->setup->selection.credential_revision == setup.selection.credential_revision;
    if (changed) {
        ++state_->selection_epoch;
        state_->current.reset();
        state_->latest_usage.reset();
        state_->reported_base.reset();
        state_->reported_symbols.reset();
    }
    if (!valid_selection(setup.selection)) {
        state_->setup.reset();
        return;
    }
    auto &scope = state_->scopes[scope_key(setup.selection)];
    scope.ledger_known &= setup.ledger_continuity_qualified;
    if (changed && !same_schedule_scope)
        scope.next_latest.reset();
    state_->setup = std::move(setup);
}

std::variant<Started, Refusal> Adapter::start(const Request &request) {
    using enum FailureKind;
    if (!state_->setup || owner_.selected_setup().selection != state_->setup->selection)
        return refuse(invalid_selection);
    const auto &setup = *state_->setup;
    const auto &selection = setup.selection;
    const auto intent = owner_.intent_for(request);
    if (intent.selection != selection || !transport_.has_qualified_credential(selection))
        return refuse(invalid_selection);
    if (!valid_request(request) || intent.deadline.count() <= 0)
        return refuse(invalid_request);
    auto canonical = request;
    std::ranges::sort(canonical.quotes);
    if (std::ranges::adjacent_find(canonical.quotes) != canonical.quotes.end())
        return refuse(invalid_request);

    const auto operation = canonical.operation;
    const auto &features = setup.features;
    const bool qualified_paid_features =
        paid(selection.profile) && features && features->selection == selection;
    bool send_symbols = false;
    if (operation != Operation::usage) {
        if (!setup.profile_qualified ||
            !(selection.profile == Profile::free || paid(selection.profile)))
            return refuse(forbidden_feature);
        if (*canonical.base != "USD" && (!qualified_paid_features || !features->alternative_base ||
                                         state_->reported_base == false))
            return refuse(forbidden_feature);
        if (operation == Operation::historical_day &&
            (!intent.qualified_utc_day || !iso_date(*intent.qualified_utc_day) ||
             *canonical.utc_day < "1999-01-01" || *canonical.utc_day >= *intent.qualified_utc_day))
            return refuse(forbidden_feature);
        send_symbols = state_->reported_symbols != false &&
                       (operation == Operation::latest ||
                        (qualified_paid_features && features->historical_symbols));
    }

    auto &scope = state_->scopes.at(scope_key(selection));
    std::vector<Probe> probes;
    std::optional<FailureKind> spent;
    for (const auto &hold : scope.holds) {
        if (!applicable(hold, operation))
            continue;
        if (intent.kind != IntentKind::requalification)
            return refuse(hold_failure(hold.identity.kind));
        const auto authority = std::ranges::find_if(intent.authorities, [&](const auto &entry) {
            return qualifies(entry, hold, selection, operation);
        });
        if (authority == intent.authorities.end())
            return refuse(hold_failure(hold.identity.kind));
        if (hold.spent_evidence.contains(authority->evidence_ref))
            spent = hold_failure(hold.identity.kind);
        probes.push_back({hold.identity, authority->cause, authority->evidence_ref});
    }
    auto &lane = scope.lane(operation);
    if (intent.applicable_owner_waits != WaitEligibility::eligible || !lane.ready(intent))
        return refuse(retry_wait);
    if (!intent.route_authorized || (intent.kind == IntentKind::requalification && probes.empty()))
        return refuse(not_authorized);
    if (intent.kind == IntentKind::automatic && !intent.due)
        return refuse(not_due);

    const auto same_pending = [&](const Pending &pending) {
        return pending.selection_epoch == state_->selection_epoch &&
               same_operation(pending.admission.canonical_request, canonical);
    };
    const auto active = std::ranges::find_if(
        state_->pending, [&](const auto &entry) { return same_pending(entry.second); });
    const auto known = scope.requests.find(canonical.request_ref);
    if (known != scope.requests.end()) {
        const auto &claim = known->second;
        if (active == state_->pending.end() || active->first != claim.reservation ||
            claim.evidence != canonical.evidence_ref ||
            claim.observation != canonical.observation_ref)
            return refuse(request_conflict);
    } else if (scope.evidence_refs.contains(canonical.evidence_ref) ||
               (canonical.observation_ref &&
                scope.observation_refs.contains(*canonical.observation_ref))) {
        return refuse(request_conflict);
    }
    if (active != state_->pending.end()) {
        if (active->second.probes != probes)
            return refuse(spent.value_or(request_conflict));
        if (known == scope.requests.end())
            scope.claim(canonical, active->first);
        return Started{active->second.admission, true};
    }
    if (spent)
        return refuse(*spent);
    if (state_->last_reservation == std::numeric_limits<Order>::max())
        return refuse(resource_limit);

    const auto reservation = ++state_->last_reservation;
    Admission admission{{selection, canonical.request_ref, reservation},
                        std::move(canonical),
                        "GET",
                        {},
                        std::string(authorization),
                        false,
                        header_limit,
                        body_limit,
                        body_limit,
                        intent.deadline};
    admission.url = url_for(admission.canonical_request, send_symbols);
    scope.claim(admission.canonical_request, reservation);
    for (const auto &probe : probes) {
        auto hold = std::ranges::find(scope.holds, probe.hold, &Hold::identity);
        hold->spent_evidence.insert(probe.evidence_ref);
        if (hold->identity.kind == HoldKind::credential)
            hold->probed_credential = selection.credential_revision;
    }
    state_->pending.emplace(
        reservation, Pending{admission, state_->selection_epoch, intent.kind, std::move(probes)});
    // Retire provider waits only when a new authorized reservation passes every wait.
    lane.provider_waits.clear();
    try {
        transport_.send(admission);
    } catch (...) {
        // A port exception cannot establish that the request stayed local.
        if (state_->pending.contains(reservation)) {
            auto result =
                finish({admission.identity, TransportFailure{TransportFailureKind::transport, {}},
                        Dispatch::possibly_sent});
            return std::get<Refusal>(std::move(result.outcome));
        }
    }
    return Started{std::move(admission), false};
}

Completion Adapter::finish(Reply reply) {
    const auto found = state_->pending.find(reply.identity.reservation_order);
    if (found == state_->pending.end() || found->second.admission.identity != reply.identity)
        return {std::move(reply.identity), refuse(FailureKind::request_conflict),
                Dispatch::not_sent};

    auto request = std::move(found->second);
    state_->pending.erase(found);
    auto &scope = state_->scopes.at(scope_key(request.admission.identity.selection));
    const auto *frame = std::get_if<SanitizedFrame>(&reply.payload);
    const auto dispatch = frame ? Dispatch::sent : reply.dispatch;
    scope.count(dispatch, request.admission.canonical_request.operation);
    const auto &receipt = std::visit(
        [](const auto &value) -> const Clocks & { return value.receipt; }, reply.payload);
    Completion result{
        std::move(reply.identity),
        frame ? normalize(request.admission, *frame)
              : Outcome{refuse(transport_failure(std::get<TransportFailure>(reply.payload).kind))},
        dispatch};
    if (state_->setup && request.selection_epoch == state_->selection_epoch &&
        owner_.selected_setup().selection == result.identity.selection)
        state_->accept(scope, request, result, receipt);
    return result;
}

void Adapter::observe_exhaustion() {
    if (!state_->setup || owner_.selected_setup().selection != state_->setup->selection)
        return;
    const auto evidence = owner_.exhaustion_evidence();
    if (!evidence || evidence->selection != state_->setup->selection ||
        !valid_ref(evidence->evidence_ref))
        return;
    auto &scope = state_->scopes.at(scope_key(evidence->selection));
    if (scope.exhaustion_refs.insert(evidence->evidence_ref).second)
        state_->hold(scope, evidence->selection, HoldKind::quota);
}

OwnerView Adapter::view(const Clocks &now) const {
    OwnerView view{};
    if (!state_->setup)
        return view;
    const auto &setup = *state_->setup;
    const auto &scope = state_->scopes.at(scope_key(setup.selection));
    view.latest_usage_outcome = state_->latest_usage;
    view.last_usable_quota = scope.usable_quota;
    if (scope.usable_quota)
        view.quota_age_ms = sample_age(scope.usable_quota->receipt, now);
    if (scope.ledger_known) {
        view.total = {scope.totals.http, scope.totals.metered, scope.totals.uncertain};
        if (scope.at_sample) {
            const auto &anchor = *scope.at_sample;
            view.since_sample = {scope.totals.http - anchor.http,
                                 scope.totals.metered - anchor.metered,
                                 scope.totals.uncertain - anchor.uncertain};
        }
    }
    if (setup.profile_qualified && paid(setup.selection.profile) && setup.display_allowance &&
        *setup.display_allowance >= 0)
        view.display_allowance = setup.display_allowance;
    for (const auto &hold : scope.holds)
        view.holds.push_back(hold.identity);
    view.latest = scope.latest;
    view.next_latest_utc_ms = scope.next_latest;
    view.last_successful_latest_fetch_utc_ms = scope.last_latest_fetch;
    view.current_result = state_->current;
    return view;
}

} // namespace subathon::server::open_exchange_rates
