// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <string_view>
#include <subathon/server/exchange_rate_api.hpp>

namespace subathon::server::exchange_rate_api::detail {

inline constexpr std::size_t header_limit = 16 * 1024;
inline constexpr std::size_t body_limit = 256 * 1024;
inline constexpr std::size_t rate_map_limit = 256;

inline bool digit(char c) { return c >= '0' && c <= '9'; }

inline bool currency_code(std::string_view code) {
    return code.size() == 3 &&
           std::ranges::all_of(code, [](char c) { return c >= 'A' && c <= 'Z'; });
}

enum class JsonKind { object, array, string, number, literal };

struct JsonField {
    JsonKind kind;
    std::string_view token; // Borrows the reply. Numbers are never converted by the scanner.
    std::string text{};     // Decoded string when this field is selected for capture.
};

using JsonFields = std::map<std::string, JsonField, std::less<>>;

struct Document {
    JsonFields members;
    JsonFields rates;
    JsonFields conversion_rates;
};

// Retain top-level fields and rate map entries only. Scan all nested values and
// check decoded key uniqueness in every object, including ignored objects.
class ReplyJson {
  public:
    explicit ReplyJson(std::string_view body) : remaining_(body) {}

    Document read() {
        Document document;
        object(1, &document.members, &document);
        skip_space();
        require(remaining_.empty());
        return document;
    }

    // Read direct members of an arbitrary object. Leave nested members available
    // as raw tokens without applying a provider-specific rate-map projection.
    JsonFields read_members() {
        JsonFields fields;
        object(1, &fields, nullptr, true);
        skip_space();
        require(remaining_.empty());
        return fields;
    }

  private:
    std::string_view remaining_;

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

    void object(unsigned depth, JsonFields *fields = nullptr, Document *root = nullptr,
                bool plain_members = false) {
        check_depth(depth);
        require(take('{'));
        if (take('}'))
            return;
        std::set<std::string, std::less<>> ignored_names;
        do {
            auto name = string(true);
            require(take(':'));
            if (!fields) {
                require(ignored_names.insert(std::move(name)).second);
                value(depth);
                continue;
            }
            require(!fields->contains(name));
            if (!root && !plain_members) {
                require(currency_code(name));
                if (fields->size() == rate_map_limit)
                    throw FailureKind::resource_limit;
            }
            JsonFields *rate_map = nullptr;
            if (root && name == "rates")
                rate_map = &root->rates;
            else if (root && name == "conversion_rates")
                rate_map = &root->conversion_rates;
            auto field = value(depth, root != nullptr || plain_members, rate_map);
            fields->emplace(std::move(name), std::move(field));
        } while (take(','));
        require(take('}'));
    }

    JsonField value(unsigned depth, bool capture = false, JsonFields *rate_map = nullptr) {
        skip_space();
        const auto start = remaining_;
        JsonField field{JsonKind::literal, {}};
        switch (front()) {
        case '{':
            field.kind = JsonKind::object;
            object(depth + 1, rate_map);
            break;
        case '[':
            field.kind = JsonKind::array;
            array(depth + 1);
            break;
        case '"':
            field.kind = JsonKind::string;
            field.text = string(capture);
            break;
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
        return field;
    }
};

} // namespace subathon::server::exchange_rate_api::detail
