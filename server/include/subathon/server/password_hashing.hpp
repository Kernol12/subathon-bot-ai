// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace subathon::server {

using PasswordBytes = std::span<const std::byte>;

struct PasswordHashingOptions {
    std::uint32_t desired_operations{3};
    std::uint64_t desired_memory_bytes{67108864};
    std::uint64_t admitted_memory_bytes{268435456};
};

enum class PasswordHashingError {
    invalid_options,
    resource_limit,
    invalid_record,
    unsupported_record,
    invalid_account_reference,
    runtime_unavailable,
    secret_memory_unavailable,
    hash_unavailable,
    native_output_invalid,
    allocation_unavailable,
    source_unavailable,
    source_invalid
};

class PasswordHashingFailure final : public std::runtime_error {
  public:
    explicit PasswordHashingFailure(PasswordHashingError code);
    [[nodiscard]] PasswordHashingError code() const noexcept;

  private:
    PasswordHashingError code_;
};

class PasswordHashingNotImplemented final : public std::logic_error {
  public:
    PasswordHashingNotImplemented();
};

enum class PasswordVerification { verified, not_successful };

struct PasswordCredential {
    std::string account_ref;
    std::string revision_ref;
    std::string encoded_verifier;
};

enum class PasswordCredentialState { present, missing, unavailable };

struct PasswordCredentialRead {
    PasswordCredentialState state{PasswordCredentialState::unavailable};
    std::optional<PasswordCredential> credential;
    std::string login_attempt_ref;
};

enum class PasswordLoginCheck { accepted, denied, stale, unavailable };

class PasswordVerifiedSource;

// Trusted, read-only account/login dependency; callbacks must not retain borrowed arguments.
class PasswordLoginSource {
  public:
    PasswordLoginSource(const PasswordLoginSource &) = delete;
    PasswordLoginSource(PasswordLoginSource &&) = delete;
    PasswordLoginSource &operator=(const PasswordLoginSource &) = delete;
    PasswordLoginSource &operator=(PasswordLoginSource &&) = delete;
    virtual ~PasswordLoginSource() = default;
    virtual PasswordCredentialRead read_current(std::string_view account_ref) = 0;
    virtual PasswordLoginCheck
    check_current_successful_login(const PasswordVerifiedSource &verified) = 0;

  protected:
    PasswordLoginSource() = default;
};

struct PasswordReplacement {
    std::string account_ref;
    std::string login_attempt_ref;
    std::string expected_revision_ref;
    std::string expected_encoded_verifier;
    std::string replacement_encoded_verifier;
};

enum class PasswordUpgradeOutcome {
    credential_missing,
    verification_not_successful,
    login_not_accepted,
    stale_source,
    not_needed,
    replacement_ready,
    upgrade_unavailable
};

struct PasswordUpgradeData {
    PasswordUpgradeOutcome outcome{PasswordUpgradeOutcome::credential_missing};
    std::optional<PasswordReplacement> replacement;
    std::optional<PasswordHashingError> unavailable_reason;
};

class PasswordUpgradeResult;

// Password primitives only; callers retain current-login and conditional-store authority.
[[nodiscard]] std::string create_password_hash(PasswordBytes password,
                                               const PasswordHashingOptions &options = {});
[[nodiscard]] PasswordVerification verify_password(PasswordBytes password,
                                                   std::string_view encoded_verifier,
                                                   const PasswordHashingOptions &options = {});
[[nodiscard]] PasswordUpgradeResult
propose_password_upgrade(PasswordBytes password, std::string_view account_ref,
                         PasswordLoginSource &source, const PasswordHashingOptions &options = {});

// Ephemeral source-bound borrow; only the upgrade operation may construct it.
class PasswordVerifiedSource final {
  public:
    PasswordVerifiedSource(const PasswordVerifiedSource &) = delete;
    PasswordVerifiedSource(PasswordVerifiedSource &&) = delete;
    PasswordVerifiedSource &operator=(const PasswordVerifiedSource &) = delete;
    PasswordVerifiedSource &operator=(PasswordVerifiedSource &&) = delete;
    ~PasswordVerifiedSource() = default;

    [[nodiscard]] const PasswordCredential &credential() const noexcept;
    [[nodiscard]] std::string_view login_attempt_ref() const noexcept;
    [[nodiscard]] bool belongs_to(const PasswordLoginSource &source) const noexcept;

  private:
    PasswordVerifiedSource(const PasswordCredential &credential, std::string_view login_attempt_ref,
                           const PasswordLoginSource &source) noexcept;
    friend PasswordUpgradeResult propose_password_upgrade(PasswordBytes, std::string_view,
                                                          PasswordLoginSource &,
                                                          const PasswordHashingOptions &);

    const PasswordCredential *credential_;
    std::string_view login_attempt_ref_;
    const PasswordLoginSource *source_;
};

// Owned proposal only; persistence still requires the account store's conditional checks.
class PasswordUpgradeResult final {
  public:
    PasswordUpgradeResult(const PasswordUpgradeResult &) = default;
    PasswordUpgradeResult(PasswordUpgradeResult &&) = default;
    PasswordUpgradeResult &operator=(const PasswordUpgradeResult &) = default;
    PasswordUpgradeResult &operator=(PasswordUpgradeResult &&) = default;
    ~PasswordUpgradeResult() = default;

    [[nodiscard]] const PasswordUpgradeData &data() const noexcept;

  private:
    explicit PasswordUpgradeResult(PasswordUpgradeData data);
    friend PasswordUpgradeResult propose_password_upgrade(PasswordBytes, std::string_view,
                                                          PasswordLoginSource &,
                                                          const PasswordHashingOptions &);

    PasswordUpgradeData data_;
};

} // namespace subathon::server
