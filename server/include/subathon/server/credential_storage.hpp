// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <subathon/server/connector_lifecycle.hpp>
#include <variant>

namespace subathon::server {
class SqliteWorkSession;
struct SqliteWorkResult;

struct InstallationSubject {};
struct AccountSubject {
    std::string_view account_ref;
};
struct CredentialScope {
    std::string_view installation_ref;
    ConnectorProvider provider;
    std::string_view connector_ref;
    std::variant<InstallationSubject, AccountSubject> subject;
    std::string_view slot_ref;
};

struct PrivateKeyPath {
    std::string_view bytes;
};

enum class CredentialError {
    feature_not_implemented,
    invalid_input,
    resource_limit,
    key_file_rejected,
    key_unavailable,
    invalid_key_file,
    invalid_record,
    authentication_failed,
    crypto_unavailable
};

namespace credential_detail {
struct ValueAccess;
}

class CredentialKey final {
  public:
    ~CredentialKey();
    CredentialKey(CredentialKey &&) noexcept;
    CredentialKey &operator=(CredentialKey &&) noexcept;

  private:
    struct Storage;
    explicit CredentialKey(std::unique_ptr<Storage>) noexcept;
    std::unique_ptr<Storage> storage_;

    friend class CredentialRepository;
    friend std::variant<CredentialKey, CredentialError> provision(PrivateKeyPath);
};

class CredentialValue final {
  public:
    ~CredentialValue();
    CredentialValue(CredentialValue &&) noexcept;
    CredentialValue &operator=(CredentialValue &&) noexcept;

    // Only trusted server consumers may borrow this view while the owner is live.
    [[nodiscard]] std::span<const unsigned char> bytes() const noexcept;

  private:
    struct Storage;
    explicit CredentialValue(std::unique_ptr<Storage>) noexcept;
    std::unique_ptr<Storage> storage_;

    friend struct credential_detail::ValueAccess;
};

struct CredentialMissing {};
struct CredentialStaged {};
using CredentialProvisionResult = std::variant<CredentialKey, CredentialError>;
using CredentialLoadResult = std::variant<CredentialValue, CredentialMissing, CredentialError>;
using CredentialPutResult = std::variant<CredentialStaged, CredentialError>;

[[nodiscard]] CredentialProvisionResult provision(PrivateKeyPath path);

// Borrows an active caller-owned writer session and a live provisioned key.
// Local refusals are typed here. SqliteWorkFailure propagates unchanged.
class CredentialRepository final {
  public:
    CredentialRepository(const SqliteWorkSession &session, const CredentialKey &key) noexcept;
    [[nodiscard]] CredentialLoadResult load(const CredentialScope &scope) const;
    [[nodiscard]] CredentialPutResult stage_put(const CredentialScope &scope,
                                                std::span<const unsigned char> bytes) const;

  private:
    const SqliteWorkSession &session_;
    const CredentialKey &key_;
};

// The caller's private work slot is consumed exactly once at writer completion.
[[nodiscard]] std::optional<CredentialValue>
finish_credential_load(std::optional<CredentialLoadResult> slot,
                       const SqliteWorkResult &completion);
} // namespace subathon::server
