// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <string_view>
#include <subathon/server/exchange_rate_api.hpp>

namespace subathon::server::exchange_rate_api::detail {
using namespace std::chrono;
using namespace std::literals;

template <typename T> std::optional<T> integer(std::string_view text) {
    if (text.empty())
        return {};
    T value{};
    const auto end = text.data() + text.size();
    const auto parsed = std::from_chars(text.data(), end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end)
        return {};
    return value;
}

inline std::optional<unsigned> digits_at(std::string_view text) {
    if (!std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; }))
        return {};
    return integer<unsigned>(text);
}

inline std::optional<year_month_day> iso_date(std::string_view text) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-')
        return {};
    const auto y = digits_at(text.substr(0, 4));
    const auto m = digits_at(text.substr(5, 2));
    const auto d = digits_at(text.substr(8, 2));
    if (!y || *y == 0 || !m || !d)
        return {};
    const auto date = year{static_cast<int>(*y)} / *m / *d;
    return date.ok() ? std::optional{date} : std::nullopt;
}

inline std::optional<std::int64_t> milliseconds_from_seconds(std::int64_t seconds_value) {
    constexpr auto low = std::numeric_limits<std::int64_t>::min();
    constexpr auto high = std::numeric_limits<std::int64_t>::max();
    if (seconds_value < low / 1000 || seconds_value > high / 1000)
        return {};
    return seconds_value * 1000;
}

inline constexpr std::array weekdays{"Sun"sv, "Mon"sv, "Tue"sv, "Wed"sv, "Thu"sv, "Fri"sv, "Sat"sv};
inline constexpr std::array long_weekdays{"Sunday"sv,   "Monday"sv, "Tuesday"sv, "Wednesday"sv,
                                          "Thursday"sv, "Friday"sv, "Saturday"sv};
inline constexpr std::array months_of_year{"Jan"sv, "Feb"sv, "Mar"sv, "Apr"sv, "May"sv, "Jun"sv,
                                           "Jul"sv, "Aug"sv, "Sep"sv, "Oct"sv, "Nov"sv, "Dec"sv};

template <std::size_t Size>
std::optional<unsigned> token_index(const std::array<std::string_view, Size> &choices,
                                    std::string_view text) {
    const auto found = std::ranges::find(choices, text);
    if (found == choices.end())
        return {};
    return static_cast<unsigned>(found - choices.begin());
}

inline std::optional<seconds> time_of_day(std::string_view text) {
    if (text.size() != 8 || text[2] != ':' || text[5] != ':')
        return {};
    const auto h = digits_at(text.substr(0, 2));
    const auto m = digits_at(text.substr(3, 2));
    const auto s = digits_at(text.substr(6, 2));
    // POSIX milliseconds have no distinct representation for a leap second.
    if (!h || *h > 23 || !m || *m > 59 || !s || *s > 59)
        return {};
    return hours{*h} + minutes{*m} + seconds{*s};
}

inline bool expanded_date_shape(std::string_view text) {
    return text.size() >= 26 && text.substr(3, 2) == ", " && text[7] == ' ' && text[11] == ' ' &&
           text[16] == ' ' && text[25] == ' ';
}

struct DateParts {
    std::optional<unsigned> weekday;
    std::optional<unsigned> day;
    std::optional<unsigned> month_index;
    std::optional<unsigned> year;
    std::optional<seconds> time;
};

inline DateParts expanded_date(std::string_view text) {
    return {token_index(weekdays, text.substr(0, 3)), digits_at(text.substr(5, 2)),
            token_index(months_of_year, text.substr(8, 3)), digits_at(text.substr(12, 4)),
            time_of_day(text.substr(17, 8))};
}

inline std::optional<std::int64_t> date_ms(const DateParts &parts, minutes offset = 0min) {
    if (!parts.weekday || !parts.day || !parts.month_index || !parts.year || !parts.time ||
        *parts.year == 0 || *parts.year > 9999)
        return {};
    const auto date = year{static_cast<int>(*parts.year)} / (*parts.month_index + 1) / *parts.day;
    if (!date.ok() || weekday{sys_days{date}}.c_encoding() != parts.weekday)
        return {};
    return (sys_days{date} + *parts.time - offset).time_since_epoch() / 1ms;
}

inline std::optional<std::int64_t> update_text_ms(std::string_view text) {
    if (!expanded_date_shape(text))
        return {};
    const auto zone = text.substr(26);
    if (zone == "GMT" || zone == "UTC")
        return date_ms(expanded_date(text));
    if (zone.size() != 5 || (zone[0] != '+' && zone[0] != '-'))
        return {};
    const auto h = digits_at(zone.substr(1, 2));
    const auto m = digits_at(zone.substr(3, 2));
    if (!h || *h > 23 || !m || *m > 59 || zone == "-0000")
        return {};
    const auto offset = hours{*h} + minutes{*m};
    return date_ms(expanded_date(text), zone[0] == '-' ? -offset : offset);
}

inline std::optional<std::int64_t> http_date_ms(std::string_view text,
                                                std::optional<std::int64_t> receipt_utc) {
    if (expanded_date_shape(text) && text.substr(26) == "GMT")
        return date_ms(expanded_date(text));
    if (text.size() == 24 && text[3] == ' ' && text[7] == ' ' && text[10] == ' ' &&
        text[19] == ' ') {
        // Obsolete asctime HTTP-date.
        return date_ms({token_index(weekdays, text.substr(0, 3)),
                        digits_at(text[8] == ' ' ? text.substr(9, 1) : text.substr(8, 2)),
                        token_index(months_of_year, text.substr(4, 3)),
                        digits_at(text.substr(20, 4)), time_of_day(text.substr(11, 8))});
    }

    // An obsolete RFC 850 date needs qualified UTC to resolve its two-digit year.
    const auto comma = text.find(',');
    if (comma == std::string_view::npos || !receipt_utc)
        return {};
    const auto named_day = token_index(long_weekdays, text.substr(0, comma));
    text.remove_prefix(comma + 1);
    if (text.size() != 23 || text[0] != ' ' || text[3] != '-' || text[7] != '-' ||
        text[10] != ' ' || text.substr(19) != " GMT")
        return {};
    DateParts parts{named_day, digits_at(text.substr(1, 2)),
                    token_index(months_of_year, text.substr(4, 3)), digits_at(text.substr(8, 2)),
                    time_of_day(text.substr(11, 8))};
    const sys_time<milliseconds> receipt{milliseconds{*receipt_utc}};
    const auto receipt_day = floor<days>(receipt);
    if (!parts.day || !parts.month_index || !parts.year || !parts.time ||
        receipt_day < sys_days{year{1} / January / 1} ||
        receipt_day >= sys_days{year{10000} / January / 1})
        return {};
    const year_month_day receipt_date{receipt_day};
    const auto future_date = receipt_date + years{50};
    const auto cutoff = sys_days{future_date} + (receipt - receipt_day);
    const auto cutoff_year = static_cast<int>(future_date.year());
    auto full_year = cutoff_year / 100 * 100 + static_cast<int>(*parts.year);
    const auto proposed = year{full_year} / (*parts.month_index + 1) / *parts.day;
    if (sys_days{proposed} + *parts.time > cutoff)
        full_year -= 100;
    if (full_year < 1)
        return {};
    parts.year = static_cast<unsigned>(full_year);
    return date_ms(parts);
}

inline std::optional<std::int64_t> worldwide_end_after(std::int64_t anchor, unsigned hint) {
    // A Gregorian 400-year cycle has a fixed length. Map even extreme trusted
    // clock readings into one cycle before using chrono's bounded year type.
    constexpr auto cycle = days{146097} / 1ms;
    auto mapped_ms = anchor % cycle;
    if (mapped_ms < 0)
        mapped_ms += cycle;
    const sys_time<milliseconds> mapped{milliseconds{mapped_ms}};
    // Before noon, yesterday can still be the first occurrence to end worldwide.
    const year_month_day yesterday{floor<days>(mapped) - days{1}};
    auto month = yesterday.year() / yesterday.month();
    for (;; month += months{1}) {
        const auto date = month / hint;
        if (!date.ok())
            continue;
        const auto end = sys_days{date} + days{1} + hours{12};
        if (end <= mapped)
            continue;
        const auto delta = (end - mapped) / 1ms;
        if (anchor > std::numeric_limits<std::int64_t>::max() - delta)
            return {};
        return anchor + delta;
    }
}

// Return the full unsigned distance even when it exceeds a signed millisecond count.
inline std::optional<std::uint64_t> elapsed_between(const std::optional<Elapsed> &start,
                                                    const std::optional<Elapsed> &end) {
    if (!start || !end || start->clock_epoch.empty() || start->clock_epoch != end->clock_epoch ||
        end->tick_ms < start->tick_ms)
        return {};
    return static_cast<std::uint64_t>(end->tick_ms) - static_cast<std::uint64_t>(start->tick_ms);
}

} // namespace subathon::server::exchange_rate_api::detail
