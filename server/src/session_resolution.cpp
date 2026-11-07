// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/session_resolution.hpp>

#include <sodium.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>

namespace subathon::server {
namespace {

struct TokenScratch final {
    std::array<unsigned char, 32> raw{};
    std::array<unsigned char, 60> frame{};

    ~TokenScratch() {
        sodium_memzero(raw.data(), raw.size());
        sodium_memzero(frame.data(), frame.size());
    }
};

bool canonical_hex(unsigned char byte) noexcept {
    return (byte >= '0' && byte <= '9') || (byte >= 'a' && byte <= 'f');
}

unsigned char hex_value(unsigned char byte) noexcept {
    return static_cast<unsigned char>(byte <= '9' ? byte - '0' : byte - 'a' + 10);
}

bool valid_limits(const SessionLimits &limits) noexcept {
    constexpr auto size_max = std::numeric_limits<std::size_t>::max();
    constexpr auto peak_max =
        std::min(std::numeric_limits<std::uint64_t>::max(), static_cast<std::uint64_t>(size_max)) /
        3;
    // Only individual references occupy strings; owned_bytes bounds each aggregate.
    return limits.reference_bytes > 0 && limits.owned_bytes > 0 &&
           limits.reference_bytes < size_max &&
           limits.reference_bytes <= std::string{}.max_size() && limits.owned_bytes <= peak_max;
}

// Borrow every occurrence, including equal strings, and admit the whole value before growth.
bool references_fit(std::span<const std::string_view> references,
                    const SessionLimits &limits) noexcept {
    std::uint64_t total = 0;
    for (const auto &reference : references) {
        const auto size = static_cast<std::uint64_t>(reference.size());
        if (size > limits.reference_bytes || size > limits.owned_bytes - total) {
            return false;
        }
        total += size;
    }
    return true;
}

} // namespace

PreparedAccountSession prepare_account_session(std::string_view token_hex) {
    if (token_hex.size() > 64) {
        return SessionFailure::resource_limit;
    }
    if (token_hex.size() < 64) {
        return SessionFailure::invalid_input;
    }

    PreparedAccountSession result{SessionFailure::unavailable};
    {
        TokenScratch scratch;
        // Stage the two text halves in existing scratch: read each input byte once,
        // validate the whole input, then decode without another secret-text buffer.
        for (std::size_t at = 0; at < 64; ++at) {
            const auto byte = static_cast<unsigned char>(token_hex[at]);
            if (!canonical_hex(byte)) {
                return SessionFailure::invalid_input;
            }
            if (at < 32) {
                scratch.raw[at] = byte;
            } else {
                scratch.frame[at - 32] = byte;
            }
        }

        // Forward decoding safely consumes the first text half in place.
        for (std::size_t at = 0; at < 16; ++at) {
            scratch.raw[at] = static_cast<unsigned char>((hex_value(scratch.raw[at * 2]) << 4) |
                                                         hex_value(scratch.raw[at * 2 + 1]));
        }
        for (std::size_t at = 0; at < 16; ++at) {
            scratch.raw[at + 16] = static_cast<unsigned char>(
                (hex_value(scratch.frame[at * 2]) << 4) | hex_value(scratch.frame[at * 2 + 1]));
        }
        static constexpr char prefix[] = "subathon.account-session.v1";
        static_assert(sizeof(prefix) == 28);
        std::memcpy(scratch.frame.data(), prefix, sizeof(prefix));
        std::copy(scratch.raw.begin(), scratch.raw.end(), scratch.frame.begin() + sizeof(prefix));

        const int initialized = sodium_init();
        if (initialized != 0 && initialized != 1) {
            return SessionFailure::unavailable;
        }
        auto &digest = result.emplace<AccountSessionDigest>();
        if (crypto_generichash(digest.data(), digest.size(), scratch.frame.data(),
                               scratch.frame.size(), nullptr, 0) != 0) {
            return SessionFailure::unavailable;
        }
    }
    // End the 92-byte scratch lifetime before a possibly non-elided digest handoff.
    return result;
}

AccountSessionResolution resolve_current_account_session(const SessionLookupRequest &request,
                                                         bool require_fresh,
                                                         CurrentSessionSource &source,
                                                         CurrentSessionClock &clock,
                                                         const SessionLimits &limits) {
    try {
        if (!valid_limits(limits)) {
            return SessionFailure::invalid_limits;
        }
        {
            const std::array<std::string_view, 1> references{request.installation_ref};
            if (!references_fit(references, limits)) {
                return SessionFailure::resource_limit;
            }
        }
        if (request.installation_ref.empty()) {
            return SessionFailure::invalid_input;
        }

        // The producer hands off one owned snapshot; validation makes no owning copy.
        const SessionLookup lookup = source.load_current(request, limits);
        if (std::holds_alternative<SessionMissing>(lookup)) {
            return SessionFailure::missing;
        }
        if (std::holds_alternative<SessionSourceInvalidData>(lookup)) {
            return SessionFailure::invalid_stored_data;
        }
        if (std::holds_alternative<SessionSourceResourceLimit>(lookup)) {
            return SessionFailure::resource_limit;
        }
        const auto *facts = std::get_if<CompleteSessionFacts>(&lookup);
        if (!facts) {
            return SessionFailure::unavailable;
        }
        const auto &session = facts->session;
        {
            const std::array<std::string_view, 12> references{session.installation_ref,
                                                              session.account_ref,
                                                              session.session_ref,
                                                              session.installation_epoch_ref,
                                                              session.account_epoch_ref,
                                                              session.record_revision_ref,
                                                              facts->account.installation_ref,
                                                              facts->account.account_ref,
                                                              facts->account.session_epoch_ref,
                                                              facts->installation.installation_ref,
                                                              facts->installation.session_epoch_ref,
                                                              facts->snapshot_ref};
            if (!references_fit(references, limits)) {
                return SessionFailure::resource_limit;
            }
            if (std::any_of(references.begin(), references.end(),
                            [](const auto &reference) { return reference.empty(); })) {
                return SessionFailure::invalid_stored_data;
            }
        }
        if ((session.state != SessionRecordState::active &&
             session.state != SessionRecordState::revoked) ||
            session.lifetime_started_utc_ms > session.authenticated_utc_ms ||
            session.authenticated_utc_ms >= session.expires_utc_ms ||
            session.installation_ref != request.installation_ref ||
            facts->account.installation_ref != request.installation_ref ||
            facts->installation.installation_ref != request.installation_ref ||
            session.account_ref != facts->account.account_ref ||
            sodium_memcmp(session.digest.data(), request.digest.data(), request.digest.size()) !=
                0) {
            return SessionFailure::invalid_stored_data;
        }
        if (session.state == SessionRecordState::revoked ||
            !facts->account.application_access_enabled ||
            session.account_epoch_ref != facts->account.session_epoch_ref ||
            session.installation_epoch_ref != facts->installation.session_epoch_ref) {
            return SessionFailure::revoked;
        }

        const auto clock_resolution = clock.sample_current();
        const auto *sample = std::get_if<SessionClockSample>(&clock_resolution);
        if (!sample || sample->now_utc_ms < session.authenticated_utc_ms ||
            sample->now_utc_ms < session.lifetime_started_utc_ms) {
            return SessionFailure::unavailable;
        }
        if (sample->now_utc_ms >= session.expires_utc_ms) {
            return SessionFailure::expired;
        }
        // After the ordered comparison, modulo unsigned subtraction is the exact
        // nonnegative full-domain age, including INT64_MIN through INT64_MAX.
        const auto age = static_cast<std::uint64_t>(sample->now_utc_ms) -
                         static_cast<std::uint64_t>(session.authenticated_utc_ms);
        const bool fresh = age <= 300000;
        if (require_fresh && !fresh) {
            return SessionFailure::fresh_authentication_required;
        }
        {
            const std::array<std::string_view, 5> references{
                session.installation_ref, session.account_ref, session.session_ref,
                session.record_revision_ref, facts->snapshot_ref};
            if (!references_fit(references, limits)) {
                return SessionFailure::resource_limit;
            }
        }

        AccountSessionResolution result{std::in_place_type<ResolvedAccountSession>};
        auto &resolved = std::get<ResolvedAccountSession>(result);
        resolved.installation_ref = session.installation_ref;
        resolved.account_ref = session.account_ref;
        resolved.session_ref = session.session_ref;
        resolved.record_revision_ref = session.record_revision_ref;
        resolved.snapshot_ref = facts->snapshot_ref;
        resolved.authenticated_utc_ms = session.authenticated_utc_ms;
        resolved.expires_utc_ms = session.expires_utc_ms;
        resolved.fresh = fresh;
        return result;
    } catch (...) {
        // Unclassified producer, clock and allocation failures do not diagnose data.
        return SessionFailure::unavailable;
    }
}

} // namespace subathon::server
