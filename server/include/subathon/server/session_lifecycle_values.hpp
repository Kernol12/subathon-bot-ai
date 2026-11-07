// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <subathon/server/session_resolution.hpp>
#include <vector>

namespace subathon::server {
// Value declarations imported unchanged from session-lifecycle-v1 §§1–2.
// These facts confer no authentication, permission or attempt reservation.
enum class SessionRetention : std::uint8_t { normal, remembered };
enum class AcceptedSignInMethod : std::uint8_t { local_password, verified_twitch };
enum class LifecycleWriteResult : std::uint8_t { staged, stale, collision };
struct LifecycleCurrentSession {
    CompleteSessionFacts current;
    SessionRetention retention;
};
struct AcceptedSessionAuthentication {
    std::string installation_ref, account_ref, attempt_ref;
    std::string installation_epoch_ref, account_epoch_ref, account_revision_ref;
    std::string authenticator_revision_ref, acceptance_revision_ref;
    AcceptedSignInMethod method;
    std::int64_t accepted_utc_ms;
};
struct LifecyclePageFacts {
    CurrentAccountFacts account;
    CurrentInstallationFacts installation;
    std::string snapshot_ref;
    std::vector<SessionRecordFacts> sessions;
};
} // namespace subathon::server
