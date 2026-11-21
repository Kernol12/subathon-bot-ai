// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics_private.hpp"

#include <QDateTime>
#include <QTimeZone>
#include <charconv>
#include <ctime>
#include <span>
#include <string_view>

namespace subathon::server::diagnostics::detail {
namespace {
constexpr std::array providers{"twitch",         "betterplace", "patreon",     "streamlabs",
                               "streamelements", "kofi",        "tipeeestream"};
constexpr std::array codes{"host.serving", "connector.report_unavailable",
                           "log.retention_unavailable", "log.capacity_reached"};
constexpr std::array fields{"receipt_disposition", "receipt_schema_version"};

// The accepted vocabulary is ASCII. Decode escapes before checking membership
// or duplicates, with a fixed token buffer and no allocation from input.
class Tokens {
  public:
    explicit Tokens(QByteArrayView bytes)
        : rest_(bytes.data() ? bytes.data() : "", std::size_t(bytes.size())) {}
    bool take(char value) {
        whitespace();
        if (rest_.empty() || rest_.front() != value)
            return false;
        rest_.remove_prefix(1);
        return true;
    }
    bool finished() {
        whitespace();
        return rest_.empty();
    }
    std::optional<std::string_view> string() {
        if (!take('"'))
            return {};
        std::size_t size = 0;
        while (!rest_.empty()) {
            unsigned char ch = static_cast<unsigned char>(rest_.front());
            rest_.remove_prefix(1);
            if (ch == '"')
                return std::string_view(decoded_.data(), size);
            if (ch == '\\') {
                if (rest_.size() < 5 || rest_.front() != 'u')
                    return {}; // No other JSON escape decodes to an allowed character.
                unsigned value = 0;
                for (char digit : rest_.substr(1, 4)) {
                    unsigned nibble;
                    if (digit >= '0' && digit <= '9')
                        nibble = unsigned(digit - '0');
                    else if (digit >= 'a' && digit <= 'f')
                        nibble = unsigned(digit - 'a') + 10;
                    else if (digit >= 'A' && digit <= 'F')
                        nibble = unsigned(digit - 'A') + 10;
                    else
                        return {};
                    value = value * 16 + nibble;
                }
                if (value > 127)
                    return {};
                ch = static_cast<unsigned char>(value);
                rest_.remove_prefix(5);
            }
            if (ch < 32 || ch >= 127 || size == decoded_.size())
                return {};
            decoded_[size++] = char(ch);
        }
        return {};
    }

  private:
    void whitespace() {
        const auto first = rest_.find_first_not_of(" \t\r\n");
        rest_.remove_prefix(first == std::string_view::npos ? rest_.size() : first);
    }
    std::string_view rest_;
    std::array<char, 32> decoded_{};
};

template <typename T> std::optional<T> integer(const std::optional<std::string_view> &token) {
    if (!token || token->empty() || *token == "-0" ||
        (token->size() > 1 && token->front() == '0') ||
        (token->size() > 2 && token->starts_with("-0")))
        return {};
    T value;
    const auto end = token->data() + token->size();
    const auto [ptr, error] = std::from_chars(token->data(), end, value);
    if (error != std::errc{} || ptr != end)
        return {};
    return value;
}
template <std::size_t N>
std::optional<std::size_t> lookup(const std::array<const char *, N> &names,
                                  std::optional<std::string_view> token) {
    if (token)
        for (std::size_t i = 0; i < names.size(); ++i)
            if (*token == names[i])
                return i;
    return {};
}

struct BoundedSelector {
    std::uint64_t receipt = 0;
    std::int64_t attempt = 0;
    std::array<Field, 2> fields{};
    std::size_t count = 0;
};
bool read_fields(Tokens &tokens, BoundedSelector &out) {
    if (!tokens.take('['))
        return false;
    unsigned seen = 0;
    do {
        auto field = lookup(fields, tokens.string());
        if (!field || (seen & (1U << *field)))
            return false;
        seen |= 1U << *field;
        out.fields[out.count++] = static_cast<Field>(*field);
    } while (tokens.take(','));
    return tokens.take(']');
}
bool read_selector(Tokens &tokens, BoundedSelector &out) {
    if (!tokens.take('{'))
        return false;
    unsigned seen = 0;
    do {
        auto key = lookup(std::array{"receipt", "attempt", "fields"}, tokens.string());
        if (!key || (seen & (1U << *key)) || !tokens.take(':'))
            return false;
        seen |= 1U << *key;
        switch (*key) {
        case 0:
            out.receipt = integer<std::uint64_t>(tokens.string()).value_or(0);
            break;
        case 1:
            out.attempt = integer<std::int64_t>(tokens.string()).value_or(0);
            break;
        case 2:
            if (!read_fields(tokens, out))
                return false;
            break;
        }
    } while (tokens.take(','));
    return seen == 7 && out.receipt > 0 && out.attempt > 0 && tokens.take('}');
}
} // namespace

const char *name(ConnectorProvider value) {
    const auto index = static_cast<std::size_t>(value);
    return index < providers.size() ? providers[index] : nullptr;
}

QJsonObject encode(const Row &row) {
    QJsonObject result{{"utc_ms", QString::number(row.utc)},
                       {"code", codes[static_cast<std::size_t>(row.observation.code)]}};
    if (row.observation.provider)
        result.insert("provider", name(*row.observation.provider));
    return result;
}

std::optional<Row> parse_row(QByteArrayView bytes) {
    Tokens tokens(bytes);
    if (bytes.size() > 255 || !tokens.take('{'))
        return {};
    unsigned seen = 0;
    std::optional<Utc> utc;
    std::optional<Code> code;
    std::optional<ConnectorProvider> provider;
    do {
        auto key = lookup(std::array{"utc_ms", "code", "provider"}, tokens.string());
        if (!key || (seen & (1U << *key)) || !tokens.take(':'))
            return {};
        seen |= 1U << *key;
        auto value = tokens.string();
        if (*key == 0)
            utc = integer<Utc>(value);
        if (*key == 1)
            if (const auto found = lookup(codes, value))
                code = static_cast<Code>(*found);
        if (*key == 2)
            if (const auto found = lookup(providers, value))
                provider = static_cast<ConnectorProvider>(*found);
    } while (tokens.take(','));
    if (!utc || *utc < min_utc || *utc > max_utc || !code ||
        (*code == Code::report_unavailable ? seen != 7 || !provider : seen != 3) ||
        !tokens.take('}') || !tokens.finished())
        return {};
    return Row{*utc, {*code, provider}};
}

std::optional<std::vector<Selector>> parse_selectors(QByteArrayView bytes) {
    if (bytes.size() > 4096)
        return {};
    Tokens tokens(bytes);
    if (!tokens.take('['))
        return {};
    std::array<BoundedSelector, 32> entries{};
    std::size_t count = 0;
    if (!tokens.take(']')) {
        do {
            if (count == entries.size() || !read_selector(tokens, entries[count]))
                return {};
            for (std::size_t i = 0; i < count; ++i)
                if (entries[i].receipt == entries[count].receipt &&
                    entries[i].attempt == entries[count].attempt)
                    return {};
            ++count;
        } while (tokens.take(','));
        if (!tokens.take(']'))
            return {};
    }
    if (!tokens.finished())
        return {};
    std::vector<Selector> result;
    result.reserve(count);
    for (const auto &entry : std::span(entries).first(count))
        result.push_back(
            {entry.receipt,
             entry.attempt,
             {entry.fields.begin(), entry.fields.begin() + std::ptrdiff_t(entry.count)}});
    return result;
}

QByteArray date_name(Utc utc) {
    return QDateTime::fromMSecsSinceEpoch(utc, QTimeZone::UTC)
               .date()
               .toString(Qt::ISODate)
               .toLatin1() +
           ".log";
}

std::optional<Utc> system_utc() noexcept {
    timespec now{};
    if (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_nsec < 0 ||
        now.tv_nsec >= 1'000'000'000L || now.tv_sec < min_utc / 1000 || now.tv_sec > max_utc / 1000)
        return {};
    return Utc(now.tv_sec) * 1000 + now.tv_nsec / 1'000'000;
}
} // namespace subathon::server::diagnostics::detail
