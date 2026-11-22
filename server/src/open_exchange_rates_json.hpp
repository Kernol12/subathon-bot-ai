// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <subathon/server/open_exchange_rates.hpp>

namespace subathon::server::open_exchange_rates::detail {

inline constexpr std::size_t header_limit = 16 * 1024;
inline constexpr std::size_t body_limit = 256 * 1024;
inline constexpr std::size_t rate_map_limit = 256;

inline bool digit(char c) { return c >= '0' && c <= '9'; }

inline bool currency_code(std::string_view code) {
    return code.size() == 3 &&
           std::ranges::all_of(code, [](char c) { return c >= 'A' && c <= 'Z'; });
}

enum class JsonKind { object, array, string, number, literal };

enum class Projection { root, rates, data, plan, features, usage, ignored };

struct JsonField;
using JsonFields = std::map<std::string, JsonField, std::less<>>;
struct JsonField {
    JsonKind kind;
    std::string_view token;
    std::string text{};
    std::unique_ptr<JsonFields> children{};
    std::string_view member{}; // From the opening key quote through the end of its value.
};

// Derived from the sibling provider's scanner. Check uniqueness and UTF-8 throughout,
// preserving lexemes and owning only the fields in the normalized document's closure.
class ReplyJson {
  public:
    explicit ReplyJson(std::string_view body, std::string_view sent_id = {},
                       std::string_view echoed_id = {})
        : remaining_(body), sent_id_(sent_id), echoed_id_(echoed_id) {}

    JsonFields read() {
        JsonFields members;
        object(1, Projection::root, &members);
        skip_space();
        require(remaining_.empty());
        return members;
    }

  private:
    std::string_view remaining_;
    std::string_view sent_id_;
    std::string_view echoed_id_;

    void check_reflection(std::string_view text) const {
        require((sent_id_.empty() || text.find(sent_id_) == std::string_view::npos) &&
                (echoed_id_.empty() || text.find(echoed_id_) == std::string_view::npos));
    }

    static void require(bool condition) {
        if (!condition)
            throw FailureKind::invalid_document;
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

    unsigned byte() {
        require(!remaining_.empty());
        const auto c = static_cast<unsigned char>(front());
        remaining_.remove_prefix(1);
        return c;
    }

    unsigned unicode_unit() {
        unsigned unit = 0;
        for (unsigned i = 0; i < 4; ++i) {
            const auto c = byte();
            unit *= 16;
            if (c >= '0' && c <= '9')
                unit += c - '0';
            else if (c >= 'a' && c <= 'f')
                unit += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                unit += c - 'A' + 10;
            else
                throw FailureKind::invalid_document;
        }
        return unit;
    }

    unsigned escape() {
        switch (byte()) {
        case '"':
            return '"';
        case '\\':
            return '\\';
        case '/':
            return '/';
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
            const auto first = unicode_unit();
            if (first < 0xd800 || first > 0xdfff)
                return first;
            require(first <= 0xdbff && byte() == '\\' && byte() == 'u');
            const auto second = unicode_unit();
            require(second >= 0xdc00 && second <= 0xdfff);
            return 0x10000 + ((first - 0xd800) << 10) + (second - 0xdc00);
        }
        default:
            throw FailureKind::invalid_document;
        }
    }

    // Return a Unicode scalar, rejecting overlong encodings and surrogates.
    unsigned utf8(unsigned lead) {
        require(lead >= 0xc2 && lead <= 0xf4);
        const unsigned tails = lead < 0xe0 ? 1 : lead < 0xf0 ? 2 : 3;
        unsigned scalar = lead & (0x7fu >> (tails + 1));
        for (unsigned i = 0; i < tails; ++i) {
            const auto c = byte();
            require((c & 0xc0) == 0x80);
            scalar = (scalar << 6) | (c & 0x3f);
        }
        constexpr unsigned minimum[]{0, 0x80, 0x800, 0x10000};
        require(scalar >= minimum[tails] && scalar <= 0x10ffff &&
                (scalar < 0xd800 || scalar > 0xdfff));
        return scalar;
    }

    static void append_utf8(std::string &out, unsigned scalar) {
        if (scalar < 0x80) {
            out.push_back(static_cast<char>(scalar));
            return;
        }
        const unsigned tails = scalar < 0x800 ? 1 : scalar < 0x10000 ? 2 : 3;
        constexpr unsigned prefix[]{0, 0xc0, 0xe0, 0xf0};
        out.push_back(static_cast<char>(prefix[tails] | (scalar >> (6 * tails))));
        for (unsigned i = tails; i > 0; --i)
            out.push_back(static_cast<char>(0x80 | ((scalar >> (6 * (i - 1))) & 0x3f)));
    }

    std::string string(bool capture) {
        require(take('"'));
        std::string result;
        for (auto c = byte(); c != '"'; c = byte()) {
            require(c >= 0x20);
            if (c == '\\')
                c = escape();
            else if (c >= 0x80)
                c = utf8(c);
            if (capture)
                append_utf8(result, c);
        }
        check_reflection(result);
        return result;
    }

    void digits() {
        require(digit(front()));
        do {
            remaining_.remove_prefix(1);
        } while (digit(front()));
    }

    void number() {
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
    }

    void literal(std::string_view token) {
        require(remaining_.starts_with(token));
        remaining_.remove_prefix(token.size());
    }

    static void check_depth(unsigned depth) {
        if (depth > 16)
            throw FailureKind::resource_limit;
    }

    void array(unsigned depth) {
        check_depth(depth);
        require(take('['));
        if (!take(']')) {
            do {
                value(depth);
            } while (take(','));
            require(take(']'));
        }
    }

    static Projection child_projection(Projection parent, std::string_view name) {
        using enum Projection;
        if (parent == root && name == "rates")
            return rates;
        if (parent == root && name == "data")
            return data;
        if (parent == data && name == "plan")
            return plan;
        if (parent == data && name == "usage")
            return usage;
        if (parent == plan && name == "features")
            return features;
        return ignored;
    }

    static bool retained(Projection projection, std::string_view name) {
        switch (projection) {
        case Projection::root:
            return name == "base" || name == "rates" || name == "timestamp" || name == "status" ||
                   name == "error" || name == "message" || name == "data";
        case Projection::data:
            return name == "status" || name == "app_id" || name == "plan" || name == "usage";
        case Projection::plan:
            return name == "name" || name == "quota" || name == "features" ||
                   name == "update_frequency";
        case Projection::features:
            return name == "base" || name == "symbols";
        case Projection::usage:
            return name == "requests_quota" || name == "requests_remaining" || name == "requests" ||
                   name == "days_elapsed" || name == "days_remaining" || name == "daily_average";
        case Projection::rates:
            return true;
        case Projection::ignored:
            return false;
        }
        return false;
    }

    void object(unsigned depth, Projection projection, JsonFields *fields) {
        check_depth(depth);
        require(take('{'));
        if (take('}'))
            return;
        std::set<std::string, std::less<>> ignored_names;
        do {
            skip_space();
            const auto start = remaining_;
            auto name = string(true);
            require(take(':'));
            if (!fields || !retained(projection, name)) {
                require(ignored_names.insert(std::move(name)).second);
                value(depth);
                continue;
            }
            require(!fields->contains(name));
            if (projection == Projection::rates && fields->size() == rate_map_limit)
                throw FailureKind::resource_limit;
            auto field = value(depth, true, child_projection(projection, name));
            field.member = start.substr(0, start.size() - remaining_.size());
            fields->emplace(std::move(name), std::move(field));
        } while (take(','));
        require(take('}'));
    }

    JsonField value(unsigned depth, bool capture = false,
                    Projection projection = Projection::ignored) {
        skip_space();
        const auto start = remaining_;
        JsonField field{JsonKind::literal, {}};
        switch (front()) {
        case '{':
            field.kind = JsonKind::object;
            if (projection != Projection::ignored)
                field.children = std::make_unique<JsonFields>();
            object(depth + 1, projection, field.children.get());
            break;
        case '[':
            field.kind = JsonKind::array;
            array(depth + 1);
            break;
        case '"': {
            field.kind = JsonKind::string;
            auto decoded = string(capture || !sent_id_.empty() || !echoed_id_.empty());
            if (capture)
                field.text = std::move(decoded);
            break;
        }
        case 't':
            literal("true");
            break;
        case 'f':
            literal("false");
            break;
        case 'n':
            literal("null");
            break;
        default:
            field.kind = JsonKind::number;
            number();
            break;
        }
        field.token = start.substr(0, start.size() - remaining_.size());
        if (field.kind == JsonKind::number)
            check_reflection(field.token);
        return field;
    }
};

} // namespace subathon::server::open_exchange_rates::detail
