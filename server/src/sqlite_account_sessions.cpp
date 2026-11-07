// SPDX-License-Identifier: GPL-3.0-or-later
// Test-first declarations/markers only. No SQL, codecs, crypto or persistence behavior.
#include <subathon/server/sqlite_account_sessions.hpp>

namespace subathon::server {
AccountSessionInputFailure::AccountSessionInputFailure(AccountSessionInputError code)
    : std::runtime_error("Account session storage input rejected"), code_(code) {}
AccountSessionInputError AccountSessionInputFailure::code() const noexcept { return code_; }
AccountSessionStorageFailure::AccountSessionStorageFailure(AccountSessionReadFailure code)
    : std::runtime_error("Account session storage operation failed"), code_(code) {}
AccountSessionReadFailure AccountSessionStorageFailure::code() const noexcept { return code_; }
AccountSessionStorageNotImplemented::AccountSessionStorageNotImplemented()
    : std::logic_error("STORAGE-020 account-session storage not implemented") {}
MigrationManifest account_session_repository_manifest() {
    throw AccountSessionStorageNotImplemented();
}
SqliteAccountSessionUnit::SqliteAccountSessionUnit(const SqliteWorkSession &,
                                                   AccountSessionReadScope,
                                                   AccountSessionStoreLimits) {
    throw AccountSessionStorageNotImplemented();
}
SqliteAccountSessionUnit::~SqliteAccountSessionUnit() = default;
SessionLookup SqliteAccountSessionUnit::load_current(const SessionLookupRequest &,
                                                     const SessionLimits &) {
    throw AccountSessionStorageNotImplemented();
}
AccountSessionRead<StoredInstallationFacts>
SqliteAccountSessionUnit::read_installation(std::string_view) {
    throw AccountSessionStorageNotImplemented();
}
AccountSessionRead<StoredAccountFacts> SqliteAccountSessionUnit::read_account(std::string_view,
                                                                              std::string_view) {
    throw AccountSessionStorageNotImplemented();
}
AccountSessionRead<CompleteLocalCredentialFacts>
SqliteAccountSessionUnit::read_local_credential(std::string_view, std::string_view) {
    throw AccountSessionStorageNotImplemented();
}
AccountSessionRead<CompleteSessionFacts>
SqliteAccountSessionUnit::read_selected(std::string_view, std::string_view, std::string_view) {
    throw AccountSessionStorageNotImplemented();
}
AccountSessionRead<LifecycleCurrentSession>
SqliteAccountSessionUnit::read_lifecycle_actor(const SessionLookupRequest &,
                                               const SessionLimits &) {
    throw AccountSessionStorageNotImplemented();
}
AccountSessionRead<LifecyclePageFacts>
SqliteAccountSessionUnit::read_page(std::string_view, std::string_view,
                                    const std::optional<std::string> &, std::uint64_t) {
    throw AccountSessionStorageNotImplemented();
}
LifecycleWriteResult
SqliteAccountSessionUnit::insert_installation(const StoredInstallationFacts &) {
    throw AccountSessionStorageNotImplemented();
}
LifecycleWriteResult
SqliteAccountSessionUnit::replace_installation_epoch(const StoredInstallationFacts &,
                                                     std::string_view, std::string_view) {
    throw AccountSessionStorageNotImplemented();
}
LifecycleWriteResult SqliteAccountSessionUnit::insert_account(const StoredAccountFacts &) {
    throw AccountSessionStorageNotImplemented();
}
LifecycleWriteResult SqliteAccountSessionUnit::replace_account(const StoredAccountFacts &,
                                                               const StoredAccountFacts &) {
    throw AccountSessionStorageNotImplemented();
}
LifecycleWriteResult SqliteAccountSessionUnit::replace_local_credential(
    const std::optional<StoredLocalCredential> &, const StoredLocalCredential &,
    const StoredAccountFacts &, const StoredInstallationFacts &) {
    throw AccountSessionStorageNotImplemented();
}
LifecycleWriteResult
SqliteAccountSessionUnit::replace_password_upgrade(const PasswordReplacement &, std::string_view,
                                                   const CompleteLocalCredentialFacts &) {
    throw AccountSessionStorageNotImplemented();
}
LifecycleWriteResult
SqliteAccountSessionUnit::insert_session(const SessionRecordFacts &, SessionRetention,
                                         const AcceptedSessionAuthentication &) {
    throw AccountSessionStorageNotImplemented();
}
LifecycleWriteResult
SqliteAccountSessionUnit::replace_session(const SessionRecordFacts &, const SessionRecordFacts &,
                                          SessionRetention, const AcceptedSessionAuthentication *) {
    throw AccountSessionStorageNotImplemented();
}
LifecycleWriteResult SqliteAccountSessionUnit::revoke_session(const SessionRecordFacts &,
                                                              std::string_view) {
    throw AccountSessionStorageNotImplemented();
}
} // namespace subathon::server
