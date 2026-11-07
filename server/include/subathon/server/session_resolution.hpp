// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

namespace subathon::server {

using AccountSessionDigest = std::array<std::uint8_t, 32>;

enum class SessionFailure : std::uint8_t {
    invalid_limits,
    resource_limit,
    invalid_input,
    unavailable,
    invalid_stored_data,
    missing,
    revoked,
    expired,
    fresh_authentication_required
};
enum class SessionRecordState : std::uint8_t { active, revoked };

struct SessionLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t owned_bytes = 65536;
};
struct SessionLookupRequest {
    std::string installation_ref;
    AccountSessionDigest digest;
};
struct SessionRecordFacts {
    std::string installation_ref, account_ref, session_ref;
    std::string installation_epoch_ref, account_epoch_ref, record_revision_ref;
    AccountSessionDigest digest;
    SessionRecordState state;
    std::int64_t lifetime_started_utc_ms, authenticated_utc_ms, expires_utc_ms;
};
struct CurrentAccountFacts {
    std::string installation_ref, account_ref, session_epoch_ref;
    bool application_access_enabled;
};
struct CurrentInstallationFacts {
    std::string installation_ref, session_epoch_ref;
};
struct CompleteSessionFacts {
    SessionRecordFacts session;
    CurrentAccountFacts account;
    CurrentInstallationFacts installation;
    std::string snapshot_ref;
};
struct SessionMissing {};
struct SessionSourceUnavailable {};
struct SessionSourceInvalidData {};
struct SessionSourceResourceLimit {};
using SessionLookup = std::variant<CompleteSessionFacts, SessionMissing, SessionSourceUnavailable,
                                   SessionSourceInvalidData, SessionSourceResourceLimit>;

// A bounded, consistent current snapshot inside the caller's active serialized scope.
class CurrentSessionSource {
  public:
    virtual ~CurrentSessionSource() = default;
    virtual SessionLookup load_current(const SessionLookupRequest &, const SessionLimits &) = 0;
};

struct SessionClockSample {
    std::int64_t now_utc_ms;
};
using SessionClockResolution = std::variant<SessionClockSample, SessionSourceUnavailable>;
class CurrentSessionClock {
  public:
    virtual ~CurrentSessionClock() = default;
    virtual SessionClockResolution sample_current() = 0;
};

struct ResolvedAccountSession {
    std::string installation_ref, account_ref, session_ref;
    std::string record_revision_ref, snapshot_ref;
    std::int64_t authenticated_utc_ms, expires_utc_ms;
    bool fresh;
};
using PreparedAccountSession = std::variant<AccountSessionDigest, SessionFailure>;
using AccountSessionResolution = std::variant<ResolvedAccountSession, SessionFailure>;

// Borrow the transport secret synchronously; callers erase their original after handoff.
[[nodiscard]] PreparedAccountSession prepare_account_session(std::string_view token_hex);
// A resolved value is owned metadata, never authority for a later execution scope.
[[nodiscard]] AccountSessionResolution resolve_current_account_session(const SessionLookupRequest &,
                                                                       bool require_fresh,
                                                                       CurrentSessionSource &,
                                                                       CurrentSessionClock &,
                                                                       const SessionLimits & = {});

} // namespace subathon::server
