// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <subathon/server/password_hashing.hpp>
#include <subathon/server/session_lifecycle_values.hpp>
#include <subathon/server/sqlite_migrations.hpp>
#include <subathon/server/sqlite_writer.hpp>
#include <variant>

namespace subathon::server {
namespace testing {
class SqliteAccountSessionTestAccess;
}
// Internal physical boundary: docs/contracts/sqlite-account-sessions-v1.md.
enum class AccountSessionReadFailure : std::uint8_t {
    missing,
    invalid_data,
    unavailable,
    resource_limit
};
enum class AccountSessionInputError : std::uint8_t {
    invalid_limits,
    resource_limit,
    invalid_input
};
template <class T> using AccountSessionRead = std::variant<T, AccountSessionReadFailure>;
struct AccountSessionStoreLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t selected_bytes = 1048576;
    std::uint64_t mutation_bytes = 1048576;
    std::uint64_t page_rows = 64;
};
struct StoredInstallationFacts {
    CurrentInstallationFacts current;
    std::string revision_ref;
};
struct StoredAccountFacts {
    CurrentAccountFacts current;
    std::string revision_ref;
};
struct StoredLocalCredential {
    std::string installation_ref;
    PasswordCredential credential;
};
struct CompleteLocalCredentialFacts {
    StoredLocalCredential local;
    StoredAccountFacts account;
    StoredInstallationFacts installation;
    std::string snapshot_ref;
};
struct AccountSessionReadScope {
    std::array<std::uint8_t, 32> read_scope_ref;
};
class AccountSessionInputFailure final : public std::runtime_error {
  public:
    explicit AccountSessionInputFailure(AccountSessionInputError);
    [[nodiscard]] AccountSessionInputError code() const noexcept;

  private:
    AccountSessionInputError code_;
};
class AccountSessionStorageFailure final : public std::runtime_error {
  public:
    explicit AccountSessionStorageFailure(AccountSessionReadFailure);
    [[nodiscard]] AccountSessionReadFailure code() const noexcept;

  private:
    AccountSessionReadFailure code_;
};
// Deliberate test-first marker, never a successful storage result.
class AccountSessionStorageNotImplemented final : public std::logic_error {
  public:
    AccountSessionStorageNotImplemented();
};
// Additive catalog head; implementation must reuse the actual compiled prefix1–7.
[[nodiscard]] MigrationManifest account_session_repository_manifest();

// Borrows a revocable caller ticket; never begins, commits, rolls back or opens SQL.
class SqliteAccountSessionUnit final : public CurrentSessionSource {
  public:
    SqliteAccountSessionUnit(const SqliteWorkSession &, AccountSessionReadScope,
                             AccountSessionStoreLimits = {});
    ~SqliteAccountSessionUnit() override;
    SqliteAccountSessionUnit(const SqliteAccountSessionUnit &) = delete;
    SqliteAccountSessionUnit &operator=(const SqliteAccountSessionUnit &) = delete;
    SessionLookup load_current(const SessionLookupRequest &, const SessionLimits &) override;
    AccountSessionRead<StoredInstallationFacts> read_installation(std::string_view);
    AccountSessionRead<StoredAccountFacts> read_account(std::string_view, std::string_view);
    AccountSessionRead<CompleteLocalCredentialFacts> read_local_credential(std::string_view,
                                                                           std::string_view);
    AccountSessionRead<CompleteSessionFacts> read_selected(std::string_view, std::string_view,
                                                           std::string_view);
    AccountSessionRead<LifecycleCurrentSession> read_lifecycle_actor(const SessionLookupRequest &,
                                                                     const SessionLimits &);
    AccountSessionRead<LifecyclePageFacts> read_page(std::string_view, std::string_view,
                                                     const std::optional<std::string> &,
                                                     std::uint64_t);
    LifecycleWriteResult insert_installation(const StoredInstallationFacts &);
    LifecycleWriteResult replace_installation_epoch(const StoredInstallationFacts &,
                                                    std::string_view, std::string_view);
    LifecycleWriteResult insert_account(const StoredAccountFacts &);
    LifecycleWriteResult replace_account(const StoredAccountFacts &, const StoredAccountFacts &);
    LifecycleWriteResult replace_local_credential(const std::optional<StoredLocalCredential> &,
                                                  const StoredLocalCredential &,
                                                  const StoredAccountFacts &,
                                                  const StoredInstallationFacts &);
    LifecycleWriteResult replace_password_upgrade(const PasswordReplacement &, std::string_view,
                                                  const CompleteLocalCredentialFacts &);
    LifecycleWriteResult insert_session(const SessionRecordFacts &, SessionRetention,
                                        const AcceptedSessionAuthentication &);
    LifecycleWriteResult replace_session(const SessionRecordFacts &, const SessionRecordFacts &,
                                         SessionRetention, const AcceptedSessionAuthentication *);
    LifecycleWriteResult revoke_session(const SessionRecordFacts &, std::string_view);

  private:
    SqliteWorkSession session_;
    AccountSessionReadScope scope_;
    AccountSessionStoreLimits limits_;
    std::uint64_t read_ordinal_{0};
    bool read_ordinal_exhausted_{false};
    friend class testing::SqliteAccountSessionTestAccess;
};
} // namespace subathon::server
