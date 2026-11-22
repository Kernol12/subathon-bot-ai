// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/exchange_rate_api.hpp>

#include "exchange_rate_api_json.hpp"
#include "exchange_rate_api_time.hpp"

#include <array>
#include <bitset>
#include <charconv>
#include <limits>
#include <tuple>

namespace subathon::server::exchange_rate_api {
namespace {
using namespace detail;
using enum FailureKind;
constexpr core::RateSnapshotLimits model_limits;
constexpr auto keyed_host = "v6.exchangerate-api.com/v6/"sv;

struct ProfileCapabilities {
    minutes refresh;
    bool keyed;
    bool paid;
};

std::optional<ProfileCapabilities> capabilities(Profile profile) {
    switch (profile) {
    case Profile::open:
        return ProfileCapabilities{24h, false, false};
    case Profile::free:
        return ProfileCapabilities{24h, true, false};
    case Profile::pro:
        return ProfileCapabilities{60min, true, true};
    case Profile::business:
    case Profile::volume:
        return ProfileCapabilities{5min, true, true};
    case Profile::unknown:
        break;
    }
    return {};
}

bool supports(const SelectedProfile &profile, Operation operation) {
    switch (operation) {
    case Operation::latest:
        return profile.latest;
    case Operation::history:
        return profile.history;
    case Operation::quota:
        return profile.quota;
    }
    return false;
}

bool reference(std::string_view value) {
    return !value.empty() && value.size() <= model_limits.reference_bytes &&
           value.find('\0') == std::string_view::npos;
}

std::string encoded_segment(std::string_view secret) {
    std::string result;
    constexpr auto hex = "0123456789ABCDEF"sv;
    for (const unsigned char c : secret) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || digit(static_cast<char>(c)) ||
            c == '-' || c == '_' || c == '~') {
            result.push_back(static_cast<char>(c));
        } else {
            result += '%';
            result += hex[c >> 4];
            result += hex[c & 0xf];
        }
    }
    return result;
}

bool valid_selectors(const Request &request, std::optional<year_month_day> date) {
    if (!reference(request.request_ref) || !reference(request.evidence_ref))
        return false;
    if (request.operation == Operation::quota)
        return request.base.empty() && request.quotes.empty() && !request.history_date;
    if (request.operation == Operation::history ? !date : request.history_date.has_value())
        return false;

    constexpr std::size_t code_count = 26 * 26 * 26;
    if (!currency_code(request.base) || request.quotes.empty() ||
        request.quotes.size() >= code_count)
        return false;

    const auto index = [](std::string_view code) {
        return static_cast<std::size_t>((code[0] - 'A') * 676 + (code[1] - 'A') * 26 + code[2] -
                                        'A');
    };
    std::bitset<code_count> seen;
    seen.set(index(request.base));
    for (const auto &quote : request.quotes) {
        if (!currency_code(quote) || seen.test(index(quote)))
            return false;
        seen.set(index(quote));
    }
    return true;
}

std::string target(const Admission &admission, std::string_view key,
                   std::optional<year_month_day> date) {
    const auto &request = admission.request;
    if (admission.selection.profile == Profile::open)
        return "https://open.er-api.com/v6/latest/" + request.base;
    const auto root = "https://" + std::string(keyed_host) + std::string(key) + '/';
    switch (request.operation) {
    case Operation::latest:
        return root + "latest/" + request.base;
    case Operation::history:
        return root + "history/" + request.base + '/' + std::to_string(int(date->year())) + '/' +
               std::to_string(unsigned(date->month())) + '/' +
               std::to_string(unsigned(date->day()));
    case Operation::quota:
        return root + "quota";
    }
    return {};
}

bool headers_fit(const Headers &headers) {
    auto available = header_limit - 2; // Final CRLF.
    for (const auto &[name, value] : headers) {
        if (available < 4 || name.size() > available - 4 ||
            value.size() > available - 4 - name.size())
            return false;
        available -= 4 + name.size() + value.size(); // ": " and CRLF.
    }
    return true;
}

bool ascii_equal_folded(std::string_view left, std::string_view right) {
    const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
    return std::ranges::equal(left, right, [&](char a, char b) { return lower(a) == lower(b); });
}

RetryAfter read_retry_after(const HttpReply &reply) {
    std::optional<std::string_view> field;
    for (const auto &[name, value] : reply.headers) {
        if (!ascii_equal_folded(name, "retry-after"))
            continue;
        if (field)
            return {RetryEvidence::invalid};
        const auto start = value.find_first_not_of(" \t");
        field =
            start == std::string::npos
                ? ""sv
                : std::string_view(value).substr(start, value.find_last_not_of(" \t") - start + 1);
    }
    if (!field)
        return {RetryEvidence::absent};
    if (!field->empty() && std::ranges::all_of(*field, digit)) {
        const auto seconds_value = integer<std::int64_t>(*field);
        if (seconds_value) {
            if (const auto delay = milliseconds_from_seconds(*seconds_value))
                return {RetryEvidence::delay, delay};
        }
    } else if (const auto deadline = http_date_ms(*field, reply.receipt.utc_ms)) {
        return {RetryEvidence::utc_deadline, {}, deadline};
    }
    return {RetryEvidence::invalid};
}

const JsonField *find(const JsonFields &fields, std::string_view name) {
    const auto it = fields.find(name);
    return it == fields.end() ? nullptr : &it->second;
}

std::optional<std::string_view> text(const JsonField *field) {
    if (!field || field->kind != JsonKind::string)
        return {};
    return field->text;
}

template <typename T> std::optional<T> number(const JsonField *field) {
    if (!field || field->kind != JsonKind::number)
        return {};
    return integer<T>(field->token);
}

Quota read_quota(const JsonFields &fields, const Clocks &receipt) {
    const auto total = number<std::uint64_t>(find(fields, "plan_quota"));
    const auto remaining = number<std::uint64_t>(find(fields, "requests_remaining"));
    if (!total || !remaining || *remaining > *total)
        throw invalid_document;
    RenewalDay renewal{RenewalState::absent, {}};
    if (const auto *field = find(fields, "refresh_day_of_month")) {
        const auto day = number<unsigned>(field);
        renewal = day && *day >= 1 && *day <= 31 ? RenewalDay{RenewalState::valid, day}
                                                 : RenewalDay{RenewalState::invalid, {}};
    }
    return {*total, *remaining, renewal, receipt};
}

// Only explicitly named, bounded metadata may leave the parser. An echoed key
// or private target is discarded even when it was placed in a known text field.
struct PrivateKey {
    std::string raw;
    std::string encoded;

    bool echoes_key(std::string_view value) const {
        if (raw.empty())
            return false;
        if (value.find(raw) != std::string_view::npos)
            return true;
        if (value.find('%') == std::string_view::npos)
            return false;

        // Escapes in echoed metadata can use either case and can mix with literal bytes.
        std::string decoded;
        decoded.reserve(value.size());
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (value[i] == '%' && value.size() - i >= 3) {
                unsigned byte{};
                const auto *first = value.data() + i + 1;
                const auto *last = first + 2;
                const auto result = std::from_chars(first, last, byte, 16);
                if (result.ec == std::errc{} && result.ptr == last) {
                    decoded.push_back(static_cast<char>(byte));
                    i += 2;
                    continue;
                }
            }
            decoded.push_back(value[i]);
        }
        return decoded.find(raw) != std::string::npos ||
               decoded.find(keyed_host) != std::string::npos;
    }

    bool public_text(std::string_view value) const {
        return value.size() <= model_limits.raw_time_bytes &&
               std::ranges::all_of(value, [](unsigned char c) { return c >= 0x20 && c < 0x7f; }) &&
               value.find("://") == std::string_view::npos &&
               value.find(keyed_host) == std::string_view::npos && !echoes_key(value);
    }
};

UpdateTime read_update_time(const JsonFields &fields, std::string_view unix_name,
                            std::string_view text_name, const PrivateKey &key) {
    UpdateTime result{};
    if (const auto *field = find(fields, unix_name)) {
        result.unix_seconds = number<std::int64_t>(field);
        if (!result.unix_seconds)
            throw invalid_document;
        result.qualified_utc_ms = milliseconds_from_seconds(*result.unix_seconds);
        if (!result.qualified_utc_ms)
            throw invalid_document;
    }
    if (const auto reported = text(find(fields, text_name));
        reported && key.public_text(*reported)) {
        result.utc_text = std::string(*reported);
        result.parsed_text_utc_ms = update_text_ms(*reported);
        result.disagrees = result.qualified_utc_ms && result.parsed_text_utc_ms &&
                           result.qualified_utc_ms != result.parsed_text_utc_ms;
    }
    return result;
}

Rate read_rate(const Admission &admission, const Document &document, const Clocks &receipt,
               const PrivateKey &key) {
    const auto &request = admission.request;
    const auto &fields = document.members;
    const bool open = admission.selection.profile == Profile::open;
    const bool historical = request.operation == Operation::history;
    const auto *map_field = find(fields, open ? "rates" : "conversion_rates");
    if (text(find(fields, "base_code")) != request.base || !map_field ||
        map_field->kind != JsonKind::object || find(fields, open ? "conversion_rates" : "rates"))
        throw invalid_document;

    UpdateEvidence update{};
    std::optional<std::string> matched_date;
    if (historical) {
        const auto date = iso_date(*request.history_date).value();
        if (number<int>(find(fields, "year")) != int(date.year()) ||
            number<unsigned>(find(fields, "month")) != unsigned(date.month()) ||
            number<unsigned>(find(fields, "day")) != unsigned(date.day()))
            throw invalid_document;
        matched_date = request.history_date;
    } else {
        update.last =
            read_update_time(fields, "time_last_update_unix", "time_last_update_utc", key);
        update.next =
            read_update_time(fields, "time_next_update_unix", "time_next_update_utc", key);
        for (const auto name : {"time_eol_unix"sv, "time_eol"sv}) {
            if (const auto *field = find(fields, name)) {
                // Preserve the descriptor even when its value is unsafe to retain.
                const bool scalar =
                    field->kind != JsonKind::array && field->kind != JsonKind::object;
                const bool safe = scalar && key.public_text(field->token) &&
                                  (field->kind != JsonKind::string || key.public_text(field->text));
                update.eol.push_back({std::string(name), safe ? std::string(field->token) : ""});
            }
        }
    }

    const auto &rates = open ? document.rates : document.conversion_rates;
    core::FetchedRateObservationInput input{
        {request.evidence_ref,
         "ExchangeRate-API",
         admission.selection.configuration_ref,
         request.request_ref,
         request.evidence_ref,
         {},
         receipt.utc_ms,
         {historical ? matched_date : update.last.utc_text, update.last.qualified_utc_ms},
         {}},
        {}};
    std::vector<std::string> missing;
    for (const auto &quote : request.quotes) {
        const auto *field = find(rates, quote);
        if (!field) {
            missing.push_back(quote);
            continue;
        }
        if (field->kind != JsonKind::number)
            throw invalid_document;
        if (field->token.size() > core::maximum_decimal_bytes)
            throw resource_limit;
        input.quotes.push_back({request.base, quote, std::string(field->token)});
    }
    if (input.quotes.empty())
        throw invalid_document;

    std::optional<OpenDisclosure> disclosure;
    if (open)
        disclosure =
            OpenDisclosure{"Rates By Exchange Rate API", "https://www.exchangerate-api.com", true};
    return {core::make_fetched_rate_observation(input),
            std::move(missing),
            std::move(update),
            std::move(matched_date),
            std::move(disclosure),
            historical ? RateUse::history_only : RateUse::latest_candidate};
}

FailureKind provider_disposition(std::string_view code) {
    if (code == "invalid-key" || code == "inactive-account")
        return authentication_hold;
    if (code == "quota-reached")
        return quota_hold;
    if (code == "plan-upgrade-required")
        return operation_unavailable;
    if (code == "unsupported-code" || code == "no-data-available" || code == "malformed-request")
        return request_domain;
    return failed_sync;
}

using Outcome = std::variant<Rate, Quota, Failure>;

Outcome read_reply(const Admission &admission, const Reply &reply, const PrivateKey &key) {
    const auto *frame = std::get_if<HttpReply>(&reply);
    if (!frame) {
        return Failure{std::get<FailedTransport>(reply).kind == TransportFailure::resource_limit
                           ? resource_limit
                           : failed_sync};
    }

    const bool bounded_headers = headers_fit(frame->headers);
    Failure failure{failed_sync,
                    frame->status,
                    {},
                    bounded_headers ? read_retry_after(*frame)
                                    : RetryAfter{RetryEvidence::invalid}};
    if (!bounded_headers || frame->wire_body_bytes > body_limit ||
        frame->decoded_body.size() > body_limit) {
        failure.kind = resource_limit;
        return failure;
    }

    const bool success_status = frame->status >= 200 && frame->status < 300;
    try {
        const auto document = ReplyJson(frame->decoded_body).read();
        const auto &fields = document.members;
        const auto result = text(find(fields, "result"));
        if (result == "error") {
            const auto code = text(find(fields, "error-type"));
            if (!code || code->empty() || code->size() > 64)
                throw invalid_document;
            failure.kind = provider_disposition(*code);
            if (admission.selection.profile == Profile::open &&
                (failure.kind == quota_hold || failure.kind == authentication_hold))
                failure.kind = failed_sync; // Open has no credential or account quota scope.
            if (failure.kind != failed_sync && key.public_text(*code))
                failure.safe_code = std::string(*code);
        } else if (success_status) {
            if (result != "success" || find(fields, "error-type"))
                throw invalid_document;
            if (admission.request.operation == Operation::quota)
                return read_quota(fields, frame->receipt);
            return read_rate(admission, document, frame->receipt, key);
        }
    } catch (FailureKind kind) {
        if (kind == resource_limit || success_status)
            failure.kind = kind;
    } catch (const core::RateSnapshotFailure &error) {
        failure.kind = error.code() == core::RateSnapshotError::resource_limit ? resource_limit
                                                                               : invalid_document;
    }
    if (frame->status == 429 && failure.kind == failed_sync)
        failure.kind = rate_limited;
    return failure;
}

bool same_job(const Request &a, const Request &b) {
    return a.operation == b.operation && a.base == b.base && a.quotes == b.quotes &&
           a.history_date == b.history_date;
}

struct ScopeOrder {
    bool operator()(const Scope &a, const Scope &b) const {
        return std::tie(a.installation_ref, a.kind, a.scope_ref) <
               std::tie(b.installation_ref, b.kind, b.scope_ref);
    }
};

struct HintGate {
    std::optional<unsigned> day;
    bool anchored{};
    std::optional<std::int64_t> threshold;
    std::optional<std::int64_t> previous_threshold;

    void observe(std::optional<std::int64_t> utc) {
        if (day && utc && !anchored) {
            anchored = true;
            threshold = worldwide_end_after(*utc, *day);
            if (threshold && previous_threshold)
                threshold = std::max(*threshold, *previous_threshold);
        }
    }

    void set(std::optional<unsigned> replacement, std::optional<std::int64_t> utc) {
        if (day != replacement) {
            previous_threshold =
                replacement ? (threshold ? threshold : previous_threshold) : std::nullopt;
            threshold.reset();
            anchored = false;
            day = replacement;
        }
        observe(utc);
    }

    bool ready(std::optional<std::int64_t> utc) const {
        return threshold && utc && *utc >= *threshold;
    }
};

struct RetryWait {
    std::optional<Elapsed> since;
    std::uint64_t delay{};
    std::optional<std::int64_t> utc_deadline;

    bool ready(const Clocks &now) {
        if (delay == 0)
            return true;
        // A full wait from a later qualified clock reading is conservative.
        // The probe's separate rolling gate never re-anchors across epochs.
        if (now.elapsed && !now.elapsed->clock_epoch.empty() &&
            (!since || since->clock_epoch.empty() ||
             since->clock_epoch != now.elapsed->clock_epoch))
            since = now.elapsed;
        return elapsed_between(since, now.elapsed) >= delay &&
               (!utc_deadline || (now.utc_ms && now.utc_ms >= utc_deadline));
    }

    // A concurrent failure cannot discard a longer wait from an earlier completion.
    void extend(const Clocks &receipt, std::uint64_t duration,
                std::optional<std::int64_t> deadline) {
        utc_deadline = std::max(utc_deadline, deadline);
        if (delay == 0) {
            since = receipt.elapsed;
            delay = duration;
        } else if (const auto passed = elapsed_between(since, receipt.elapsed)) {
            since = receipt.elapsed;
            delay = std::max(duration, *passed >= delay ? 0 : delay - *passed);
        } else if (const auto backwards = elapsed_between(receipt.elapsed, since)) {
            delay = std::max(delay, *backwards >= duration ? 0 : duration - *backwards);
        } else {
            since.reset(); // An interval across missing or incompatible epochs is unprovable.
            delay = std::max(delay, duration);
        }
    }
};

struct ScopeState {
    std::shared_ptr<const SelectionBinding> last_selection;
    bool known_usage{true};
    Usage local{};
    Usage report_boundary{};
    std::optional<Quota> report;
    std::optional<std::uint64_t> reference_allowance;
    std::optional<std::string> authentication_stop;
    std::set<Operation> plan_stops;
    bool quota_held{};
    HintGate hint;
    bool used_probe_slot{};
    std::optional<Elapsed> last_probe_send;
    std::shared_ptr<const Admission> pending_probe;
    RetryWait wait;
    std::size_t retry_step{};

    void hold_quota(const Clocks &receipt) {
        quota_held = true;
        hint.set(report ? report->renewal_day.day : std::nullopt, receipt.utc_ms);
    }

    void accept(const Quota &sample, bool probe) {
        report = sample;
        report_boundary = local;
        if (sample.requests_remaining == 0)
            hold_quota(sample.receipt);
        else if (probe) {
            quota_held = false;
            hint = {};
        } else if (quota_held) {
            // A still-current report can refresh display evidence. Only a qualified probe clears
            // the hold, and HintGate preserves the existing eligibility floor.
            hint.set(sample.renewal_day.day, sample.receipt.utc_ms);
        }
    }

    void retry(const Clocks &receipt, const Failure &failure, Profile profile) {
        constexpr std::array ladder{1min, 5min, 15min, 60min};
        auto duration = ladder[retry_step] / 1ms;
        retry_step = std::min(retry_step + 1, ladder.size() - 1);
        if (profile == Profile::open && failure.status == 429)
            duration = std::max(duration, 20min / 1ms);
        if (failure.retry_after.delay_ms)
            duration = std::max(duration, *failure.retry_after.delay_ms);
        wait.extend(receipt, static_cast<std::uint64_t>(duration),
                    failure.retry_after.deadline_utc_ms);
    }
};

struct Pending {
    std::shared_ptr<const Admission> admission;
    std::shared_ptr<const SelectionBinding> selection;
    std::optional<Elapsed> admitted_at;
    Dispatch dispatch{DispatchKind::not_sent};
    std::uint64_t local_ordinal{};
};

void record_dispatch(ScopeState &scope, Pending &pending) {
    if (scope.local.total == std::numeric_limits<std::uint64_t>::max()) {
        scope.known_usage = false;
    } else {
        pending.local_ordinal = ++scope.local.total;
        if (pending.dispatch.kind == DispatchKind::possibly_sent)
            ++scope.local.possibly_sent;
    }
    if (pending.admission->recovery_probe) {
        scope.used_probe_slot = true;
        scope.last_probe_send =
            pending.dispatch.time ? std::optional{pending.dispatch.time->elapsed} : std::nullopt;
    }
}

void confirm_send(ScopeState &scope, Pending &pending, const Clocks &receipt) {
    const auto previous = pending.dispatch.kind;
    pending.dispatch.kind = DispatchKind::sent;
    if (!pending.dispatch.time && elapsed_between(pending.admitted_at, receipt.elapsed))
        pending.dispatch.time = SendTime{SendTimeKind::upper_bound, *receipt.elapsed};

    if (previous == DispatchKind::not_sent) {
        record_dispatch(scope, pending);
    } else if (previous == DispatchKind::possibly_sent && pending.local_ordinal != 0) {
        --scope.local.possibly_sent;
        if (pending.local_ordinal <= scope.report_boundary.total)
            --scope.report_boundary.possibly_sent;
    }
    if (pending.admission->recovery_probe && pending.dispatch.time)
        scope.last_probe_send = pending.dispatch.time->elapsed;
}

bool actual_entitlement(const SelectionUpdate &update, const SelectionBinding &previous) {
    if (update.change == AuthorizedChange::plan_entitlement) {
        // The trusted owner can report a real change within the same documented profile.
        // A replay of the same binding cannot authorize another recovery opportunity.
        return update.selection.selection_revision != previous.selection_revision ||
               update.selection.configuration_ref != previous.configuration_ref;
    }
    return update.change == AuthorizedChange::credential_entitlement &&
           update.selection.credential_revision != previous.credential_revision;
}

} // namespace

struct Adapter::State {
    std::optional<SelectedProfile> selected;
    std::shared_ptr<const SelectionBinding> selection;
    PrivateKey key;
    bool requalification{};
    std::map<Scope, ScopeState, ScopeOrder> scopes;
    std::map<std::string, Pending, std::less<>> pending;
    std::set<std::string, std::less<>> completed;
};

Adapter::Adapter(SelectionPort &selection, HttpPort &http)
    : selection_(selection), http_(http), state_(std::make_unique<State>()) {}
Adapter::~Adapter() = default;

SelectionStatus Adapter::select() {
    auto update = selection_.selected_setup();
    const auto &binding = update.selection;
    const auto profile = capabilities(binding.profile);
    if (!profile)
        return SelectionStatus::unknown_profile;
    if (update.reference_allowance && !profile->paid)
        return SelectionStatus::reference_not_allowed;
    if (!reference(binding.scope.installation_ref) || !reference(binding.scope.scope_ref) ||
        !reference(binding.configuration_ref) || !reference(binding.selection_revision))
        return SelectionStatus::invalid_credential_binding;

    PrivateKey key;
    if (profile->keyed) {
        if (binding.scope.kind != ScopeKind::account_quota || !binding.credential_revision ||
            !reference(*binding.credential_revision) || !update.credential ||
            update.credential->binding != binding || update.credential->private_segment.empty())
            return SelectionStatus::invalid_credential_binding;
        key.raw = std::move(update.credential->private_segment);
        if (state_->selection && state_->selection->scope == binding.scope &&
            state_->selection->credential_revision == binding.credential_revision &&
            state_->key.raw != key.raw)
            return SelectionStatus::invalid_credential_binding;
        key.encoded = encoded_segment(key.raw);
    } else if (binding.scope.kind != ScopeKind::open_usage || binding.credential_revision ||
               update.credential) {
        return SelectionStatus::invalid_credential_binding;
    }

    auto &scope = state_->scopes[binding.scope];
    const bool lost_continuity =
        scope.known_usage && update.continuity != ScopeContinuity::qualified;
    if (lost_continuity)
        scope.known_usage = false;
    const bool entitlement =
        scope.last_selection && actual_entitlement(update, *scope.last_selection);
    // A different profile can be selected without evidence that authorizes plan-stop recovery.
    if (entitlement)
        scope.plan_stops.clear();
    if (scope.authentication_stop && update.change == AuthorizedChange::corrected_credential &&
        binding.credential_revision != scope.authentication_stop)
        scope.authentication_stop.reset();

    if (!state_->selection || *state_->selection != binding || lost_continuity) {
        state_->selection = std::make_shared<const SelectionBinding>(binding);
        state_->requalification = entitlement && scope.quota_held && scope.known_usage;
    }
    scope.last_selection = state_->selection;
    scope.reference_allowance = update.reference_allowance;
    state_->key = std::move(key);
    state_->selected =
        SelectedProfile{binding, profile->refresh, true, profile->paid, profile->keyed};
    return SelectionStatus::selected;
}

SelectionStatus Adapter::edit_reference_allowance(std::optional<std::uint64_t> allowance) {
    if (!state_->selected)
        return SelectionStatus::unknown_profile;
    if (allowance && !state_->selected->history)
        return SelectionStatus::reference_not_allowed;
    state_->scopes.at(state_->selection->scope).reference_allowance = allowance;
    return SelectionStatus::selected;
}

std::optional<SelectedProfile> Adapter::current() const { return state_->selected; }

StartResult Adapter::start(const Request &request, const Clocks &now) {
    if (!state_->selected)
        return Failure{unsupported_operation};
    const auto &selection = *state_->selection;
    auto &scope = state_->scopes.at(selection.scope);
    scope.hint.observe(now.utc_ms);

    if (state_->completed.contains(request.request_ref) ||
        state_->pending.contains(request.request_ref))
        return Failure{replay};
    if (!supports(*state_->selected, request.operation))
        return Failure{unsupported_operation};
    const auto date = request.history_date ? iso_date(*request.history_date) : std::nullopt;
    if (!valid_selectors(request, date))
        return Failure{invalid_selectors};

    if (scope.authentication_stop)
        return Failure{authentication_hold};
    if (scope.plan_stops.contains(request.operation))
        return Failure{operation_unavailable};
    if (selection.scope.kind == ScopeKind::account_quota && !scope.known_usage)
        return Failure{quota_hold};

    const bool probe = scope.quota_held;
    if (probe && (request.operation != Operation::quota || scope.pending_probe || !now.elapsed ||
                  now.elapsed->clock_epoch.empty() ||
                  (!state_->requalification && !scope.hint.ready(now.utc_ms)) ||
                  (scope.used_probe_slot &&
                   elapsed_between(scope.last_probe_send, now.elapsed) < std::uint64_t{24h / 1ms})))
        return Failure{quota_hold};
    if (!scope.wait.ready(now))
        return Failure{retry_wait};
    for (const auto &[identity, pending] : state_->pending) {
        if (pending.selection == state_->selection && same_job(pending.admission->request, request))
            return Failure{coalesced};
    }

    auto admitted = std::make_shared<const Admission>(Admission{selection, request, probe});
    const HttpRequest outgoing{
        *admitted,  "GET",     target(*admitted, state_->key.encoded, date), false, header_limit,
        body_limit, body_limit};
    auto &pending =
        state_->pending
            .try_emplace(request.request_ref, Pending{admitted, state_->selection, now.elapsed})
            .first->second;
    if (probe)
        scope.pending_probe = admitted;
    scope.wait = {};
    // An exception after handoff cannot prove that the request stayed local.
    try {
        pending.dispatch = http_.send(outgoing);
    } catch (...) {
        pending.dispatch = {DispatchKind::possibly_sent};
    }
    if (pending.dispatch.kind == DispatchKind::not_sent ||
        (pending.dispatch.time &&
         (pending.dispatch.time->elapsed.clock_epoch.empty() ||
          (probe && !elapsed_between(now.elapsed, pending.dispatch.time->elapsed)) ||
          (pending.dispatch.kind == DispatchKind::possibly_sent &&
           pending.dispatch.time->kind != SendTimeKind::upper_bound))))
        pending.dispatch.time.reset();

    if (pending.dispatch.kind != DispatchKind::not_sent) {
        record_dispatch(scope, pending);
        if (probe)
            state_->requalification = false;
    }

    Started result{*admitted, pending.dispatch};
    result.reservation_ = std::move(admitted);
    return result;
}

Completion Adapter::finish(const Started &started, Reply reply) {
    if (!started.reservation_)
        return {Admission{}, {DispatchKind::not_sent}, Failure{replay}};
    const auto it = state_->pending.find(started.reservation_->request.request_ref);
    if (it == state_->pending.end() || it->second.admission != started.reservation_ ||
        *it->second.admission != started.admission || it->second.dispatch != started.dispatch)
        return {Admission{}, {DispatchKind::not_sent}, Failure{replay}};

    state_->completed.insert(it->first);
    auto pending = std::move(it->second);
    state_->pending.erase(it);
    const auto &admission = *pending.admission;
    auto &scope = state_->scopes.at(admission.selection.scope);
    const auto &receipt =
        std::visit([](const auto &result) -> const Clocks & { return result.receipt; }, reply);
    const bool current = pending.selection == state_->selection;

    if (std::holds_alternative<HttpReply>(reply)) {
        confirm_send(scope, pending, receipt);
        if (current && admission.recovery_probe)
            state_->requalification = false;
    }
    if (admission.recovery_probe)
        scope.pending_probe.reset();
    if (!current)
        return {admission, pending.dispatch, Failure{superseded}};

    auto outcome = read_reply(admission, reply, state_->key);
    if (const auto *sample = std::get_if<Quota>(&outcome)) {
        scope.accept(*sample, admission.recovery_probe);
        scope.retry_step = 0;
    } else if (const auto *failure = std::get_if<Failure>(&outcome)) {
        switch (failure->kind) {
        case authentication_hold:
            scope.authentication_stop = admission.selection.credential_revision;
            break;
        case operation_unavailable:
            scope.plan_stops.insert(admission.request.operation);
            break;
        case quota_hold:
            scope.hold_quota(receipt);
            break;
        case request_domain:
            break; // This request's selectors cannot change the scope-wide retry ladder.
        default:
            scope.retry(receipt, *failure, admission.selection.profile);
            break;
        }
    } else {
        scope.retry_step = 0;
    }
    scope.hint.observe(receipt.utc_ms);
    return {admission, pending.dispatch, std::move(outcome)};
}

QuotaView Adapter::quota_view(const Scope &binding, const Clocks &now) const {
    const auto it = state_->scopes.find(binding);
    if (it == state_->scopes.end())
        return {};
    const auto &scope = it->second;
    QuotaView view{scope.report,    {}, scope.report.has_value(), {}, {}, scope.reference_allowance,
                   scope.quota_held};
    if (scope.known_usage)
        view.local_total = scope.local;
    if (scope.report) {
        const auto age = elapsed_between(scope.report->receipt.elapsed, now.elapsed);
        if (age && *age <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
            view.sample_age_ms = static_cast<std::int64_t>(*age);
        if (scope.known_usage) {
            view.local_since_report =
                Usage{scope.local.total - scope.report_boundary.total,
                      scope.local.possibly_sent - scope.report_boundary.possibly_sent};
        }
    }
    return view;
}

} // namespace subathon::server::exchange_rate_api
