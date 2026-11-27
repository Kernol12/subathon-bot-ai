// SPDX-License-Identifier: GPL-3.0-or-later
#include "session_transport.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>

namespace subathon::server {
namespace {
using Failure = AccountTransportFailure;
using Kind = AccountTransportKind;
using Purpose = AccountTransportPurpose;
using View = std::string_view;

bool token_byte(unsigned char c) {
    constexpr View punctuation = "!#$%&'*+-.^`_|~";
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           punctuation.find(static_cast<char>(c)) != View::npos;
}

bool http_token(View value) { return !value.empty() && std::ranges::all_of(value, token_byte); }

bool equal_ascii(View value, View lowercase) {
    return std::ranges::equal(value, lowercase, [](unsigned char a, unsigned char b) {
        return (a >= 'A' && a <= 'Z' ? a + ('a' - 'A') : a) == b;
    });
}

View trim_ows(View value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == View::npos)
        return {};
    return value.substr(first, value.find_last_not_of(" \t") - first + 1);
}

bool field_value(View value) {
    return std::ranges::all_of(
        value, [](unsigned char c) { return c == '\t' || (c >= ' ' && c != 0x7f); });
}

bool canonical_origin(View origin) {
    constexpr View prefix = "https://";
    if (!origin.starts_with(prefix))
        return false;
    origin.remove_prefix(prefix.size());
    // S1 admits the canonical DNS form. Other authority forms need adapter review.
    if (origin.empty() || origin.size() > 253 || origin.back() == '.')
        return false;
    while (!origin.empty()) {
        const auto dot = origin.find('.');
        const auto label = origin.substr(0, dot);
        if (label.empty() || label.size() > 63 || label.front() == '-' || label.back() == '-' ||
            !std::ranges::all_of(label, [](unsigned char c) {
                return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
            }))
            return false;
        if (dot == View::npos)
            return true;
        origin.remove_prefix(dot + 1);
    }
    return false;
}

bool valid_limits(const AccountTransportLimits &limits) {
    const std::array values{limits.field_count,  limits.field_bytes,  limits.header_bytes,
                            limits.cookie_pairs, limits.origin_bytes, limits.policy_reference_bytes,
                            limits.result_bytes};
    constexpr auto span_max = std::numeric_limits<std::size_t>::max() - 1;
    return std::ranges::all_of(values, [](auto value) { return value > 0 && value <= span_max; }) &&
           limits.policy_reference_bytes <= std::string{}.max_size() &&
           limits.result_bytes <= std::string{}.max_size();
}

bool spend(std::uint64_t &remaining, std::uint64_t size) {
    if (size > remaining)
        return false;
    remaining -= size;
    return true;
}

bool within_limits(const AccountIngressView &input, const AccountTransportLimits &limits) {
    if (input.fields.size() > limits.field_count || input.method.size() > 16 ||
        input.configured_origin.size() > limits.origin_bytes ||
        input.policy_revision_ref.size() > limits.policy_reference_bytes)
        return false;
    auto remaining = limits.header_bytes;
    if (!spend(remaining, 2))
        return false;
    for (const auto &field : input.fields) {
        auto per_field = limits.field_bytes;
        if (!spend(per_field, field.name.size()) || !spend(per_field, field.value.size()) ||
            !spend(remaining, field.name.size()) || !spend(remaining, field.value.size()) ||
            !spend(remaining, 4))
            return false;
    }
    return true;
}

bool cookie_octet(unsigned char c) {
    return c == 0x21 || (c >= 0x23 && c <= 0x2b) || (c >= 0x2d && c <= 0x3a) ||
           (c >= 0x3c && c <= 0x5b) || (c >= 0x5d && c <= 0x7e);
}

struct Occurrences {
    View authorization, account, origin, marker;
    std::size_t authorizations = 0, accounts = 0, origins = 0, markers = 0;
};

std::optional<Failure> scan_cookie(View value, std::uint64_t &remaining, Occurrences &seen) {
    for (;;) {
        const auto separator = value.find(';');
        const auto pair = trim_ows(value.substr(0, separator));
        const auto equals = pair.find('=');
        if (equals == View::npos || !http_token(pair.substr(0, equals)))
            return Failure::invalid_syntax;
        const auto raw = pair.substr(equals + 1);
        auto contents = raw;
        if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"')
            contents = raw.substr(1, raw.size() - 2);
        if (!std::ranges::all_of(contents, cookie_octet))
            return Failure::invalid_syntax;
        if (!spend(remaining, 1))
            return Failure::resource_limit;
        if (pair.substr(0, equals) == "__Host-subathon_session") {
            ++seen.accounts;
            seen.account = raw;
        }
        if (separator == View::npos)
            return {};
        value.remove_prefix(separator + 1);
    }
}

std::optional<Failure> collect(const AccountIngressView &input, std::uint64_t pairs,
                               Occurrences &seen) {
    for (const auto &field : input.fields) {
        if (!http_token(field.name) || !field_value(field.value))
            return Failure::invalid_syntax;
        if (equal_ascii(field.name, "cookie")) {
            if (auto error = scan_cookie(field.value, pairs, seen))
                return error;
        } else if (equal_ascii(field.name, "authorization")) {
            ++seen.authorizations;
            seen.authorization = field.value;
        } else if (equal_ascii(field.name, "origin")) {
            ++seen.origins;
            seen.origin = field.value;
        } else if (equal_ascii(field.name, "x-subathon-csrf")) {
            ++seen.markers;
            seen.marker = field.value;
        }
    }
    return {};
}

bool lowerhex(View value) {
    return value.size() == 64 && std::ranges::all_of(value, [](unsigned char c) {
               return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
           });
}

View bearer_token(View field) {
    field = trim_ows(field);
    if (field.size() < 7 || !equal_ascii(field.substr(0, 6), "bearer") || field[6] != ' ')
        return {};
    const auto first = field.find_first_not_of(' ', 6);
    return first == View::npos ? View{} : field.substr(first);
}

AccountTransportResult prepare(AccountIngressSource &source, Purpose purpose,
                               const AccountTransportLimits &limits,
                               testing::TransportSupplier supplier, void *state) {
    if (!valid_limits(limits))
        return Failure::invalid_limits;
    if (purpose != Purpose::protected_access && purpose != Purpose::browser_sign_in_guard)
        return Failure::invalid_syntax;

    try {
        const auto read = source.read_current(limits);
        const auto *input = std::get_if<AccountIngressView>(&read);
        if (!input)
            return Failure::ingress_unavailable;
        if (!within_limits(*input, limits))
            return Failure::resource_limit;
        if (!http_token(input->method) || !canonical_origin(input->configured_origin) ||
            input->policy_revision_ref.empty())
            return Failure::ingress_unavailable;

        Occurrences seen;
        if (const auto error = collect(*input, limits.cookie_pairs, seen))
            return *error;
        const bool sign_in = purpose == Purpose::browser_sign_in_guard;
        const bool desktop = seen.authorizations != 0;
        if (seen.authorizations > 1 || seen.accounts > 1 || (desktop && seen.accounts) ||
            (sign_in ? desktop || input->method != "POST" : !desktop && !seen.accounts))
            return Failure::authentication_required;

        const bool safe = input->method == "GET" || input->method == "HEAD";
        if (desktop
                ? seen.origins || seen.markers
                : seen.markers != 1 || trim_ows(seen.marker) != "1" || seen.origins > 1 ||
                      (seen.origins ? seen.origin != input->configured_origin : !safe || sign_in))
            return Failure::origin_or_csrf_rejected;
        if (input->policy_revision_ref.size() > limits.result_bytes)
            return Failure::resource_limit;

        const auto token = desktop ? bearer_token(seen.authorization) : seen.account;
        if ((!sign_in || seen.accounts) && !lowerhex(token))
            return Failure::authentication_required;
        if (sign_in)
            return BrowserSignInGuard{std::string(input->policy_revision_ref)};
        const auto result = supplier(state, token);
        if (const auto *digest = std::get_if<AccountSessionDigest>(&result))
            return PreparedTransportSession{*digest,
                                            desktop ? Kind::desktop_bearer : Kind::browser_cookie,
                                            std::string(input->policy_revision_ref)};
        switch (std::get<SessionFailure>(result)) {
        case SessionFailure::unavailable:
            return Failure::crypto_unavailable;
        case SessionFailure::resource_limit:
        case SessionFailure::invalid_input:
            return Failure::authentication_required;
        default:
            return Failure::ingress_unavailable;
        }
    } catch (...) {
        return Failure::ingress_unavailable;
    }
}

PreparedAccountSession real_supplier(void *, View token) { return prepare_account_session(token); }
} // namespace

AccountTransportResult prepare_account_transport(AccountIngressSource &source, Purpose purpose,
                                                 const AccountTransportLimits &limits) {
    return prepare(source, purpose, limits, real_supplier, nullptr);
}

AccountTransportResult testing::prepare_transport(AccountIngressSource &source, Purpose purpose,
                                                  const AccountTransportLimits &limits,
                                                  TransportSupplier supplier, void *state) {
    if (!supplier)
        return Failure::ingress_unavailable;
    return prepare(source, purpose, limits, supplier, state);
}
} // namespace subathon::server
