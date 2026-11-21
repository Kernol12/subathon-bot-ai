// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/frankfurter_rates.hpp>

#include <QUtf8StringView>
#include <QUuid>

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <map>
#include <new>
#include <set>
#include <string_view>
#include <subathon/core/valuation_snapshot_codec.hpp>

namespace subathon::server {
namespace {
using namespace std::chrono;
using Failure = FrankfurterFailureKind;
using Refusal = FrankfurterLatestRefusal;
using Wait = FrankfurterWaitState;
using namespace std::literals;

constexpr auto provider = "Frankfurter/ECB"sv;
constexpr auto base = "EUR"sv;
constexpr std::size_t body_limit = 256 * 1024;
constexpr std::size_t header_limit = 16 * 1024;
constexpr std::size_t currency_limit = 256;
constexpr std::size_t row_limit = 256;
constexpr std::size_t date_limit = 16;
constexpr unsigned depth_limit = 16;
constexpr std::size_t hint_limit = 256;
constexpr std::size_t logical_limit = 24'576;
constexpr core::RateSnapshotLimits model_limits{.owned_bytes = logical_limit};

FrankfurterFailure failure(Failure kind, std::optional<int> status = {},
                           std::optional<FrankfurterRequest> admitted = {}) {
    return {kind, status, std::move(admitted), {Wait::unknown, {}, {}, {}, {}}};
}

bool digit(char c) { return c >= '0' && c <= '9'; }

bool currency_code(std::string_view text) {
    return text.size() == 3 &&
           std::ranges::all_of(text, [](char c) { return c >= 'A' && c <= 'Z'; });
}

std::optional<unsigned> unsigned_number(std::string_view text, std::size_t width) {
    if (text.size() != width || !std::ranges::all_of(text, digit))
        return {};
    unsigned number{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), number);
    return result.ec == std::errc{} ? std::optional{number} : std::nullopt;
}

std::optional<year_month_day> iso_date(std::string_view text) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-')
        return {};
    const auto y = unsigned_number(text.substr(0, 4), 4);
    const auto m = unsigned_number(text.substr(5, 2), 2);
    const auto d = unsigned_number(text.substr(8, 2), 2);
    if (!y || *y == 0 || !m || !d)
        return {};
    const auto date = year{static_cast<int>(*y)} / *m / *d;
    return date.ok() ? std::optional{date} : std::nullopt;
}

bool charge(std::size_t &used, std::size_t amount, std::size_t limit) {
    if (amount > limit - used)
        return false;
    used += amount;
    return true;
}

bool headers_fit(const FrankfurterHeaders &headers, std::size_t limit) {
    std::size_t used = 0;
    if (!charge(used, 2, limit)) // Final CRLF.
        return false;
    for (const auto &[name, value] : headers) {
        if (!charge(used, name.size(), limit) || !charge(used, value.size(), limit) ||
            !charge(used, 4, limit)) // ": " and CRLF.
            return false;
    }
    return true;
}

Failure transport_failure(FrankfurterTransportFailure signal) {
    switch (signal) {
    case FrankfurterTransportFailure::timeout:
        return Failure::timeout;
    case FrankfurterTransportFailure::cancelled:
        return Failure::cancelled;
    case FrankfurterTransportFailure::transport:
        return Failure::transport;
    case FrankfurterTransportFailure::framing:
        return Failure::framing;
    case FrankfurterTransportFailure::resource_limit:
        return Failure::resource_limit;
    }
    return Failure::transport;
}

Failure http_failure(int status) {
    switch (status) {
    case 400:
    case 422:
        return Failure::rejected_request;
    case 404:
        return Failure::missing_rates;
    case 429:
        return Failure::limited;
    case 503:
        return Failure::unavailable;
    default:
        return Failure::http_failure;
    }
}

bool ascii_equal_folded(std::string_view left, std::string_view right) {
    const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
    return std::ranges::equal(left, right, [&](char a, char b) { return lower(a) == lower(b); });
}

std::string_view trim_ows(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos)
        return {};
    return value.substr(first, value.find_last_not_of(" \t") - first + 1);
}

template <std::size_t Size>
std::optional<unsigned> token_index(const std::array<std::string_view, Size> &choices,
                                    std::string_view text) {
    const auto found = std::ranges::find(choices, text);
    if (found == choices.end())
        return {};
    return static_cast<unsigned>(found - choices.begin());
}

std::optional<seconds> http_time(std::string_view text) {
    if (text.size() != 8 || text[2] != ':' || text[5] != ':')
        return {};
    const auto h = unsigned_number(text.substr(0, 2), 2);
    const auto m = unsigned_number(text.substr(3, 2), 2);
    const auto s = unsigned_number(text.substr(6, 2), 2);
    if (!h || *h > 23 || !m || *m > 59 || !s || *s > 60)
        return {};
    if (*s == 60 && (*h != 23 || *m != 59))
        return {};
    return hours{*h} + minutes{*m} + seconds{*s};
}

struct HttpDate {
    unsigned weekday;
    unsigned day;
    unsigned month;
    unsigned year;
    seconds since_midnight;
    bool short_year;
};

std::optional<HttpDate> parse_http_date(std::string_view text) {
    constexpr std::array weekdays{"Sun"sv, "Mon"sv, "Tue"sv, "Wed"sv, "Thu"sv, "Fri"sv, "Sat"sv};
    constexpr std::array weekdays_long{"Sunday"sv,   "Monday"sv, "Tuesday"sv, "Wednesday"sv,
                                       "Thursday"sv, "Friday"sv, "Saturday"sv};
    constexpr std::array months{"Jan"sv, "Feb"sv, "Mar"sv, "Apr"sv, "May"sv, "Jun"sv,
                                "Jul"sv, "Aug"sv, "Sep"sv, "Oct"sv, "Nov"sv, "Dec"sv};
    std::optional<unsigned> weekday, day, month, year;
    std::optional<seconds> time;
    bool short_year = false;
    if (text.size() == 29 && text.substr(3, 2) == ", " && text[7] == ' ' && text[11] == ' ' &&
        text[16] == ' ' && text.substr(25) == " GMT") {
        weekday = token_index(weekdays, text.substr(0, 3));
        day = unsigned_number(text.substr(5, 2), 2);
        month = token_index(months, text.substr(8, 3));
        year = unsigned_number(text.substr(12, 4), 4);
        time = http_time(text.substr(17, 8));
    } else if (text.size() == 24 && text[3] == ' ' && text[7] == ' ' && text[10] == ' ' &&
               text[19] == ' ') {
        // The obsolete asctime form uses a space before a single-digit day.
        weekday = token_index(weekdays, text.substr(0, 3));
        month = token_index(months, text.substr(4, 3));
        day = text[8] == ' ' ? unsigned_number(text.substr(9, 1), 1)
                             : unsigned_number(text.substr(8, 2), 2);
        time = http_time(text.substr(11, 8));
        year = unsigned_number(text.substr(20, 4), 4);
    } else {
        // The obsolete RFC 850 form needs a trustworthy reference year.
        const auto comma = text.find(',');
        if (comma == std::string_view::npos)
            return {};
        weekday = token_index(weekdays_long, text.substr(0, comma));
        text.remove_prefix(comma + 1);
        if (text.size() != 23 || text[0] != ' ' || text[3] != '-' || text[7] != '-' ||
            text[10] != ' ' || text.substr(19) != " GMT")
            return {};
        day = unsigned_number(text.substr(1, 2), 2);
        month = token_index(months, text.substr(4, 3));
        year = unsigned_number(text.substr(8, 2), 2);
        time = http_time(text.substr(11, 8));
        short_year = true;
    }
    if (!weekday || !day || !month || !year || !time || (!short_year && *year == 0))
        return {};
    const auto sample_year = std::chrono::year{static_cast<int>(*year) + (short_year ? 2000 : 0)};
    if (!(sample_year / (*month + 1) / *day).ok())
        return {};
    return HttpDate{*weekday, *day, *month + 1, *year, *time, short_year};
}

FrankfurterWait date_wait(const HttpDate &hint, std::string_view text,
                          std::optional<std::int64_t> receipt_utc) {
    FrankfurterWait wait{Wait::indeterminate, {}, {}, {}, std::string{text}};
    int full_year = static_cast<int>(hint.year);
    if (hint.short_year) {
        if (!receipt_utc)
            return wait;
        const sys_time<milliseconds> receipt{milliseconds{*receipt_utc}};
        const auto day = floor<days>(receipt);
        if (day < sys_days{year{1} / January / 1} || day >= sys_days{year{10000} / January / 1})
            return wait;
        const year_month_day receipt_date{day};
        const int receipt_year = static_cast<int>(receipt_date.year());
        const int limit_year = receipt_year + 50;
        full_year += limit_year / 100 * 100;
        const auto limit_date = year{limit_year} / receipt_date.month() / receipt_date.day();
        const auto limit = sys_days{limit_date} + (receipt - day);
        const auto proposed = year{full_year} / hint.month / hint.day;
        if (sys_days{proposed} + hint.since_midnight > limit)
            full_year -= 100;
        if (full_year < 1 || full_year > 9999)
            return wait;
    }
    const auto date = year{full_year} / hint.month / hint.day;
    if (!date.ok() || weekday{sys_days{date}}.c_encoding() != hint.weekday)
        return {Wait::unknown, {}, {}, {}, {}};
    // POSIX milliseconds cannot distinguish the leap second from the next midnight.
    if (hint.since_midnight == days{1})
        return wait;
    wait.deadline_utc_ms =
        (sys_days{date} + hint.since_midnight).time_since_epoch() / milliseconds{1};
    if (receipt_utc)
        wait.state = Wait::known;
    return wait;
}

FrankfurterWait retry_after(const FrankfurterHttpFrame &frame) {
    const FrankfurterWait unknown{Wait::unknown, {}, {}, {}, {}};
    std::optional<std::string_view> field;
    for (const auto &[name, value] : frame.headers) {
        if (ascii_equal_folded(name, "retry-after")) {
            if (field)
                return unknown;
            field = trim_ows(value);
        }
    }
    if (!field || field->empty())
        return unknown;

    if (std::ranges::all_of(*field, digit)) {
        const auto first = field->find_first_not_of('0');
        const auto significant = first == std::string_view::npos ? "0"sv : field->substr(first);
        if (significant.size() > hint_limit)
            return {Wait::indeterminate, {}, {}, {}, "at least 257 significant decimal digits"};

        std::int64_t seconds_value{};
        const auto parsed = std::from_chars(significant.data(),
                                            significant.data() + significant.size(), seconds_value);
        constexpr std::int64_t units = seconds{1} / milliseconds{1};
        if (parsed.ec != std::errc{} ||
            seconds_value > std::numeric_limits<std::int64_t>::max() / units)
            return {Wait::indeterminate, {}, {}, {}, std::string{significant}};

        const auto tick = frame.receipt_monotonic_ms;
        return {tick || seconds_value == 0 ? Wait::known : Wait::indeterminate,
                seconds_value * units,
                tick,
                {},
                std::string{significant}};
    }
    if (const auto date = parse_http_date(*field))
        return date_wait(*date, *field, frame.receipt_utc_ms);
    return unknown;
}

// The syntax pass borrows the body and retains only fixed-size row metadata.
// In particular, scanning a JSON number never converts or rewrites its token.
struct JsonString {
    std::array<char, 10> bytes{};
    std::size_t size{};
    bool fits{true};

    void append(unsigned c) {
        if (c > 127 || size == bytes.size())
            fits = false;
        else if (fits)
            bytes[size++] = static_cast<char>(c);
    }
    std::string_view view() const {
        return fits ? std::string_view{bytes.data(), size} : std::string_view{};
    }
};

struct Row {
    JsonString date, base, quote;
    std::string_view rate;
    std::size_t group_index{};
    unsigned seen{};
    bool valid{true};

    void field(unsigned index) {
        const unsigned bit = 1u << index;
        if (seen & bit)
            valid = false;
        seen |= bit;
    }
};

struct Rows {
    std::array<Row, row_limit> values{};
    std::array<std::size_t, date_limit> group_first_rows{};
    std::size_t size{};
    std::size_t groups{};
};

class RateJson {
  public:
    explicit RateJson(std::string_view text) : remaining_(text) {}

    Rows read() {
        Rows rows;
        expect('[');
        if (!take(']')) {
            do {
                skip_space();
                if (front() != '{')
                    throw Failure::invalid_document;
                if (rows.size == rows.values.size())
                    throw Failure::resource_limit;
                auto &row = rows.values[rows.size];
                container('{', '}', 2, &row);
                record_group(rows, row);
                ++rows.size;
            } while (take(','));
            expect(']');
        }
        skip_space();
        if (!remaining_.empty())
            throw Failure::invalid_document;
        return rows;
    }

  private:
    std::string_view remaining_;

    static void record_group(Rows &rows, Row &row) {
        const auto date = row.date.view();
        if (!iso_date(date))
            return;
        for (std::size_t i = 0; i < rows.groups; ++i) {
            if (rows.values[rows.group_first_rows[i]].date.view() == date) {
                row.group_index = i;
                return;
            }
        }
        if (rows.groups == rows.group_first_rows.size())
            throw Failure::resource_limit;
        row.group_index = rows.groups;
        rows.group_first_rows[rows.groups++] = rows.size;
    }

    char front() const { return remaining_.empty() ? '\0' : remaining_.front(); }

    void skip_space() {
        while (front() == ' ' || front() == '\t' || front() == '\r' || front() == '\n')
            remaining_.remove_prefix(1);
    }

    bool take(char token) {
        skip_space();
        if (front() != token)
            return false;
        remaining_.remove_prefix(1);
        return true;
    }

    void expect(char token) {
        if (!take(token))
            throw Failure::invalid_document;
    }

    unsigned escape() {
        if (remaining_.empty())
            throw Failure::invalid_document;
        const char c = front();
        remaining_.remove_prefix(1);
        switch (c) {
        case '"':
        case '\\':
        case '/':
            return static_cast<unsigned>(c);
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case 'u': {
            if (remaining_.size() < 4)
                throw Failure::invalid_document;
            const auto hex = remaining_.substr(0, 4);
            unsigned unit{};
            const auto result = std::from_chars(hex.data(), hex.data() + hex.size(), unit, 16);
            if (result.ec != std::errc{} || result.ptr != hex.data() + hex.size())
                throw Failure::invalid_document;
            remaining_.remove_prefix(4);
            return unit;
        }
        default:
            throw Failure::invalid_document;
        }
    }

    JsonString string() {
        expect('"');
        const auto start = remaining_;
        JsonString decoded;
        while (!remaining_.empty()) {
            const auto c = static_cast<unsigned char>(front());
            remaining_.remove_prefix(1);
            if (c == '"') {
                const auto size = static_cast<qsizetype>(start.size() - remaining_.size() - 1);
                if (!QUtf8StringView{start.data(), size}.isValidUtf8())
                    throw Failure::invalid_document;
                return decoded;
            }
            if (c < 32)
                throw Failure::invalid_document;
            decoded.append(c == '\\' ? escape() : c);
        }
        throw Failure::invalid_document;
    }

    void digits() {
        if (!digit(front()))
            throw Failure::invalid_document;
        do {
            remaining_.remove_prefix(1);
        } while (digit(front()));
    }

    std::string_view number() {
        const auto start = remaining_;
        if (front() == '-')
            remaining_.remove_prefix(1);
        if (front() == '0')
            remaining_.remove_prefix(1);
        else
            digits();
        if (front() == '.') {
            remaining_.remove_prefix(1);
            digits();
        }
        if (front() == 'e' || front() == 'E') {
            remaining_.remove_prefix(1);
            if (front() == '+' || front() == '-')
                remaining_.remove_prefix(1);
            digits();
        }
        return start.substr(0, start.size() - remaining_.size());
    }

    void literal(std::string_view token) {
        if (!remaining_.starts_with(token))
            throw Failure::invalid_document;
        remaining_.remove_prefix(token.size());
    }

    void value(unsigned parent_depth) {
        skip_space();
        switch (front()) {
        case '{':
            container('{', '}', parent_depth + 1);
            return;
        case '[':
            container('[', ']', parent_depth + 1);
            return;
        case '"':
            string();
            return;
        case 't':
            literal("true");
            return;
        case 'f':
            literal("false");
            return;
        case 'n':
            literal("null");
            return;
        default:
            number();
        }
    }

    void member(Row &row, const JsonString &name, unsigned depth) {
        constexpr std::array keys{"date"sv, "base"sv, "quote"sv, "rate"sv};
        const auto index = token_index(keys, name.view());
        if (!index) {
            value(depth);
            return;
        }
        row.field(*index);
        skip_space();
        if (*index == 3 && (digit(front()) || front() == '-')) {
            row.rate = number();
        } else if (*index < 3 && front() == '"') {
            auto *destination = std::array{&row.date, &row.base, &row.quote}[*index];
            *destination = string();
        } else {
            row.valid = false;
            value(depth);
        }
    }

    void container(char open, char close, unsigned depth, Row *row = nullptr) {
        if (depth > depth_limit)
            throw Failure::resource_limit;
        expect(open);
        if (take(close))
            return;
        do {
            if (open == '{') {
                const auto name = string();
                expect(':');
                if (row) {
                    member(*row, name, depth);
                    continue;
                }
            }
            value(depth);
        } while (take(','));
        expect(close);
    }
};

struct ObservationBudget {
    std::size_t bytes{};
    std::size_t items{};

    void add_bytes(std::size_t count) {
        if (!charge(bytes, count, logical_limit))
            throw Failure::resource_limit;
    }

    void add_item() {
        if (!charge(items, 1, model_limits.items))
            throw Failure::resource_limit;
    }
};

FrankfurterFetch read_rates(const FrankfurterRequest &request, const FrankfurterHttpFrame &frame) {
    const auto rows = RateJson{frame.decoded_body}.read();
    const auto populated = std::span{rows.values}.first(rows.size);
    if (request.quotes.size() > currency_limit)
        throw Failure::resource_limit;
    std::array<bool, row_limit> seen{};
    ObservationBudget raw;
    std::size_t parsed_digits = 0;
    std::array<bool, date_limit> charged_groups{};

    // RSV1 charges the original bytes in both projections, then adds exact
    // numerator and denominator digits only to the parsed projection.
    constexpr std::size_t uuid_bytes = 36; // QUuid::WithoutBraces, including separators.
    for (const auto &row : populated) {
        const auto date = row.date.view();
        const auto quote = row.quote.view();
        const auto requested = std::ranges::find(request.quotes, quote);
        if (!row.valid || row.seen != 15 || !iso_date(date) || row.base.view() != base ||
            requested == request.quotes.end() || !currency_code(quote))
            throw Failure::invalid_row;
        const auto index = static_cast<std::size_t>(requested - request.quotes.begin());
        if (seen.at(index))
            throw Failure::invalid_row;
        seen[index] = true;

        if (!charged_groups[row.group_index]) {
            raw.add_item();
            raw.add_bytes(uuid_bytes + provider.size() + date.size() +
                          (frame.receipt_utc_ms ? 8u : 0u));
            raw.add_bytes(request.configuration_ref.size());
            raw.add_bytes(request.request_ref.size());
            raw.add_bytes(request.evidence_ref.size());
            charged_groups[row.group_index] = true;
        }
        raw.add_item();
        raw.add_bytes(base.size() + quote.size());
        raw.add_bytes(row.rate.size());
        const auto rate = core::parse_positive_rate(row.rate);
        if (quote == base && rate.value() != 1)
            throw Failure::invalid_row;
        if (!charge(parsed_digits, boost::multiprecision::numerator(rate.value()).str().size(),
                    logical_limit) ||
            !charge(parsed_digits, boost::multiprecision::denominator(rate.value()).str().size(),
                    logical_limit) ||
            parsed_digits > logical_limit - raw.bytes)
            throw Failure::resource_limit;
    }

    FrankfurterFetch fetch{request, {}, {}, {}};
    std::array<std::vector<core::RateQuoteInput>, date_limit> quotes_by_date;
    for (const auto &row : populated) {
        const auto date = row.date.view();
        const auto quote = row.quote.view();
        quotes_by_date[row.group_index].push_back(
            {std::string{base}, std::string{quote}, std::string{row.rate}});
        if (request.historical_date) {
            fetch.historical_matches.push_back({std::string{quote}, *request.historical_date,
                                                std::string{date},
                                                *request.historical_date == date});
        }
    }
    for (std::size_t i = 0; i < request.quotes.size(); ++i) {
        if (!seen.at(i))
            fetch.omitted_quotes.push_back(request.quotes[i]);
    }
    fetch.observations.reserve(rows.groups);
    for (std::size_t i = 0; i < rows.groups; ++i) {
        const auto date = rows.values[rows.group_first_rows[i]].date.view();
        const core::FetchedRateObservationInput input{
            {QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString(),
             std::string{provider},
             request.configuration_ref,
             request.request_ref,
             request.evidence_ref,
             {},
             frame.receipt_utc_ms,
             {std::string{date}, {}},
             {}},
            std::move(quotes_by_date[i])};
        fetch.observations.push_back(core::make_fetched_rate_observation(input, model_limits));
    }
    return fetch;
}

bool same_request(const FrankfurterRequest &left, const FrankfurterRequest &right) {
    return left.configuration_ref == right.configuration_ref &&
           left.request_ref == right.request_ref && left.evidence_ref == right.evidence_ref &&
           left.base == right.base && left.quotes == right.quotes &&
           left.historical_date == right.historical_date;
}

bool matches_admission(const core::RateObservationContext &context,
                       const FrankfurterRequest &request) {
    const auto &date = context.source_rate_time.original_text;
    return context.provider_ref == provider &&
           context.configuration_ref == request.configuration_ref &&
           context.request_ref == request.request_ref &&
           context.source_evidence_ref == request.evidence_ref && !context.provider_snapshot_ref &&
           !context.source_rate_time.utc_ms && !context.publication_time.original_text &&
           !context.publication_time.utc_ms && date && iso_date(*date).has_value();
}

bool non_self(const core::RateQuote &quote) { return quote.from_currency != quote.to_currency; }

using Pair = std::pair<std::string_view, std::string_view>;

core::FetchedRateObservationInput source_input(const core::FetchedRateObservation &observation) {
    const auto &data = observation.data();
    core::FetchedRateObservationInput input{data.context, {}};
    input.quotes.reserve(data.quotes.size());
    for (const auto &quote : data.quotes)
        input.quotes.push_back({quote.from_currency, quote.to_currency, quote.rate.original()});
    return input;
}

std::optional<std::int64_t> difference(std::optional<std::int64_t> end,
                                       std::optional<std::int64_t> start) {
    using Limits = std::numeric_limits<std::int64_t>;
    if (!end || !start || (*start > 0 && *end < Limits::min() + *start) ||
        (*start < 0 && *end > Limits::max() + *start))
        return {};
    return *end - *start;
}

std::optional<std::int64_t> age(std::optional<std::int64_t> now,
                                std::optional<std::int64_t> recorded) {
    const auto elapsed = difference(now, recorded);
    return elapsed && *elapsed >= 0 ? elapsed : std::nullopt;
}

FrankfurterWarning rate_date_warning(const std::optional<std::string> &date,
                                     std::optional<std::int64_t> now, milliseconds threshold) {
    const auto parsed = date ? iso_date(*date) : std::nullopt;
    if (!parsed || threshold.count() < 0)
        return FrankfurterWarning::unknown;
    const auto following_midnight =
        (sys_days{*parsed} + days{1}).time_since_epoch() / milliseconds{1};
    const auto elapsed = difference(now, following_midnight);
    if (!elapsed)
        return FrankfurterWarning::unknown;
    return *elapsed > threshold.count() ? FrankfurterWarning::due : FrankfurterWarning::clear;
}
} // namespace

FrankfurterRequestResult make_frankfurter_request(const FrankfurterRequest &request) try {
    if (request.base != base || request.configuration_ref.empty() || request.request_ref.empty() ||
        request.evidence_ref.empty() ||
        (request.historical_date && !iso_date(*request.historical_date)))
        return failure(Failure::invalid_request);
    if (request.configuration_ref.size() > model_limits.reference_bytes ||
        request.request_ref.size() > model_limits.reference_bytes ||
        request.evidence_ref.size() > model_limits.reference_bytes)
        return failure(Failure::resource_limit);

    std::set<std::string_view> selector;
    for (const auto &code : request.quotes) {
        if (!currency_code(code))
            return failure(Failure::invalid_request);
        selector.insert(code);
        if (selector.size() + !selector.contains(base) > currency_limit)
            return failure(Failure::resource_limit);
    }
    if (selector.size() <= static_cast<std::size_t>(selector.contains(base)))
        return failure(Failure::invalid_request);

    FrankfurterRequest admitted{
        request.configuration_ref, request.request_ref, request.evidence_ref, request.base, {},
        request.historical_date};
    admitted.quotes.assign(selector.begin(), selector.end());
    std::string url{"https://api.frankfurter.dev/v2/rates?providers=ecb&base=EUR&quotes="};
    url += admitted.quotes.front();
    for (const auto &quote : std::span{admitted.quotes}.subspan(1)) {
        url += ',';
        url += quote;
    }
    if (admitted.historical_date)
        url += "&date=" + *admitted.historical_date;
    return FrankfurterHttpRequest{
        std::move(admitted), "GET",      std::move(url), {{"Accept", "application/json"}},
        body_limit,          body_limit, header_limit,   seconds{30}};
} catch (const std::bad_alloc &) {
    return failure(Failure::resource_limit);
}

FrankfurterFetchResult normalize_frankfurter_response(const FrankfurterHttpRequest &request,
                                                      const FrankfurterHttpResult &response) try {
    if (const auto *signal = std::get_if<FrankfurterTransportFailure>(&response))
        return failure(transport_failure(*signal), {}, request.admitted);
    const auto &frame = std::get<FrankfurterHttpFrame>(response);
    if (frame.wire_body_bytes > std::min(request.max_wire_body_bytes, body_limit) ||
        frame.decoded_body.size() > std::min(request.max_decoded_body_bytes, body_limit) ||
        !headers_fit(frame.headers, std::min(request.max_header_bytes, header_limit)))
        return failure(Failure::resource_limit, {}, request.admitted);
    if (frame.status != 200) {
        auto result = failure(http_failure(frame.status), frame.status, request.admitted);
        if (result.kind == Failure::limited || result.kind == Failure::unavailable)
            result.wait = retry_after(frame);
        return result;
    }
    try {
        return read_rates(request.admitted, frame);
    } catch (Failure kind) {
        return failure(kind, frame.status, request.admitted);
    } catch (const core::RateSnapshotFailure &error) {
        return failure(error.code() == core::RateSnapshotError::resource_limit
                           ? Failure::resource_limit
                           : Failure::invalid_row,
                       frame.status, request.admitted);
    } catch (const std::length_error &) {
        return failure(Failure::resource_limit, frame.status, request.admitted);
    } catch (const std::invalid_argument &) {
        return failure(Failure::invalid_row, frame.status, request.admitted);
    }
} catch (const std::bad_alloc &) {
    return failure(Failure::resource_limit);
}

FrankfurterFetchResult FrankfurterAdapter::fetch(const FrankfurterRequest &request) const try {
    const auto admitted = make_frankfurter_request(request);
    if (const auto *refused = std::get_if<FrankfurterFailure>(&admitted))
        return *refused;
    const auto &http = std::get<FrankfurterHttpRequest>(admitted);
    return normalize_frankfurter_response(http, port_.exchange(http));
} catch (const std::bad_alloc &) {
    return failure(Failure::resource_limit);
} catch (const std::length_error &) {
    return failure(Failure::resource_limit);
}

ConnectorReport FrankfurterAdapter::report() const {
    using Feature = ConnectorFeature;
    ConnectorReport report;
    report.installation = ConnectorInstallation::not_required;
    report.account_consent = ConnectorConsent::not_required;
    for (const auto feature : {Feature::application_registration, Feature::api_key_setup,
                               Feature::webhook_setup, Feature::browser_link})
        report.capabilities.features.emplace(feature, ConnectorSupport::unsupported);
    return report;
}

FrankfurterLatestResult propose_frankfurter_latest(const FrankfurterFetch &fetch,
                                                   const FrankfurterRequest &current_admission,
                                                   const core::EffectiveRateSnapshot &current,
                                                   const RateSourceBasis &previous,
                                                   const std::string &candidate_ref) try {
    if (fetch.admitted.historical_date)
        return Refusal::historical;
    if (!same_request(fetch.admitted, current_admission))
        return Refusal::superseded;

    const core::FetchedRateObservation *selected = nullptr;
    for (const auto &observation : fetch.observations) {
        const auto &source = observation.data();
        if (!matches_admission(source.context, current_admission))
            return Refusal::superseded;
        if (std::ranges::any_of(source.quotes, non_self) &&
            (!selected || source.context.source_rate_time.original_text >
                              selected->data().context.source_rate_time.original_text))
            selected = &observation;
    }
    if (!selected)
        return Refusal::no_usable_quotes;

    const auto &fresh = selected->data();
    std::set<Pair> fresh_pairs;
    std::vector<core::RateOverrideFetchedEdge> edges;
    for (std::size_t i = 0; i < fresh.quotes.size(); ++i) {
        const auto &quote = fresh.quotes[i];
        if (!non_self(quote))
            continue;
        const auto ordinal = static_cast<std::uint64_t>(i + 1);
        fresh_pairs.emplace(quote.from_currency, quote.to_currency);
        edges.push_back({*selected, ordinal});
    }

    std::map<Pair, RateSourceEdge> carried;
    for (const auto &edge : previous.edges) {
        const auto &source = previous.observations.at(edge.observation_index);
        if (edge.quote_ordinal == 0 || edge.quote_ordinal > source.quotes.size())
            throw core::RateSnapshotFailure(core::RateSnapshotError::unresolved_basis);
        if (source.context.observation_ref == fresh.context.observation_ref)
            throw core::RateSnapshotFailure(core::RateSnapshotError::duplicate_identity);
        const auto &quote = source.quotes[static_cast<std::size_t>(edge.quote_ordinal - 1)];
        if (quote.from_currency == quote.to_currency)
            continue;
        const Pair pair{quote.from_currency, quote.to_currency};
        if (fresh_pairs.contains(pair)) {
            if (source.context.provider_ref == provider) {
                const auto &old_date = source.context.source_rate_time.original_text;
                if (!old_date || !iso_date(*old_date))
                    throw core::RateSnapshotFailure(core::RateSnapshotError::unresolved_basis);
                if (*old_date > *fresh.context.source_rate_time.original_text)
                    return Refusal::regression;
            }
        } else if (!carried.emplace(pair, edge).second) {
            throw core::RateSnapshotFailure(core::RateSnapshotError::duplicate_quote);
        }
    }

    // Build one ordered sequence for the update and for later override edits.
    // Rebuild only referenced old observations, keeping their full original rows.
    std::map<std::size_t, core::FetchedRateObservation> prior_observations;
    for (const auto &entry : carried) {
        const auto &edge = entry.second;
        const auto source_index = edge.observation_index;
        auto found = prior_observations.find(source_index);
        if (found == prior_observations.end()) {
            found =
                prior_observations
                    .emplace(source_index, core::make_fetched_rate_observation(
                                               previous.observations[source_index], model_limits))
                    .first;
        }
        edges.push_back({found->second, edge.quote_ordinal});
    }

    RateSourceBasis basis{previous.policy_ref, {}, {}};
    std::map<std::string_view, std::uint32_t> observation_indices;
    for (const auto &edge : edges) {
        const auto &observation = edge.observation.get();
        const auto [entry, inserted] =
            observation_indices.try_emplace(observation.data().context.observation_ref,
                                            static_cast<std::uint32_t>(basis.observations.size()));
        if (inserted) {
            if (basis.observations.size() == model_limits.observations)
                return Refusal::resource_limit;
            basis.observations.push_back(source_input(observation));
        }
        basis.edges.push_back({entry->second, edge.quote_ordinal});
    }

    auto candidate = core::propose_rate_update(current, selected, edges, candidate_ref);
    static_cast<void>(
        core::encode_valuation_snapshot(core::make_effective_rate_snapshot(*candidate)));

    FrankfurterLatestProposal result{*selected, std::move(basis), {}, {}, std::move(*candidate)};
    for (const auto &quote : current_admission.quotes) {
        if (quote == current_admission.base)
            continue;
        const Pair pair{current_admission.base, quote};
        if (!fresh_pairs.contains(pair))
            (carried.contains(pair) ? result.retained_quotes : result.missing_quotes)
                .push_back(quote);
    }
    return result;
} catch (const core::RateSnapshotFailure &error) {
    if (error.code() == core::RateSnapshotError::resource_limit)
        return Refusal::resource_limit;
    throw;
} catch (const core::ExactValuationFailure &error) {
    if (error.code() == core::ExactValuationError::resource_limit ||
        error.code() == core::ExactValuationError::allocation_failure)
        return Refusal::resource_limit;
    throw;
} catch (const std::bad_alloc &) {
    return Refusal::resource_limit;
} catch (const std::length_error &) {
    return Refusal::resource_limit;
}

FrankfurterFreshness assess_frankfurter_freshness(const FrankfurterFreshnessInput &input) {
    FrankfurterFreshness result{input.history, {}, {}, FrankfurterWarning::unknown, {}};
    if (input.history == FrankfurterHistory::never_succeeded) {
        result.fetch_warning = FrankfurterWarning::due;
    } else if (input.history == FrankfurterHistory::accepted) {
        result.last_successful_fetch_utc_ms = input.last_successful_fetch_utc_ms;
        result.fetch_age_ms = age(input.now_utc_ms, result.last_successful_fetch_utc_ms);
        if (result.fetch_age_ms && input.fetch_warning_after.count() >= 0)
            result.fetch_warning = *result.fetch_age_ms >= input.fetch_warning_after.count()
                                       ? FrankfurterWarning::due
                                       : FrankfurterWarning::clear;
    }
    result.observations.reserve(input.active_observations.size());
    for (const auto &observation : input.active_observations) {
        const auto &context = observation.data().context;
        const auto &date = context.source_rate_time.original_text;
        result.observations.push_back(
            {context.observation_ref,
             context.fetched_at_utc_ms,
             age(input.now_utc_ms, context.fetched_at_utc_ms),
             date,
             rate_date_warning(date, input.now_utc_ms, input.rate_date_warning_after),
             {}});
    }
    return result;
}

} // namespace subathon::server
