// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/password_hashing.hpp>

#include <utility>

namespace subathon::server {

PasswordHashingFailure::PasswordHashingFailure(PasswordHashingError code)
    : std::runtime_error("Password hashing operation failed"), code_(code) {}

PasswordHashingError PasswordHashingFailure::code() const noexcept { return code_; }

PasswordHashingNotImplemented::PasswordHashingNotImplemented()
    : std::logic_error("Password hashing is intentionally unimplemented; review tests first") {}

PasswordVerifiedSource::PasswordVerifiedSource(const PasswordCredential &credential,
                                               std::string_view login_attempt_ref,
                                               const PasswordLoginSource &source) noexcept
    : credential_(&credential), login_attempt_ref_(login_attempt_ref), source_(&source) {}

const PasswordCredential &PasswordVerifiedSource::credential() const noexcept {
    return *credential_;
}

std::string_view PasswordVerifiedSource::login_attempt_ref() const noexcept {
    return login_attempt_ref_;
}

bool PasswordVerifiedSource::belongs_to(const PasswordLoginSource &source) const noexcept {
    return source_ == &source;
}

PasswordUpgradeResult::PasswordUpgradeResult(PasswordUpgradeData data) : data_(std::move(data)) {}

const PasswordUpgradeData &PasswordUpgradeResult::data() const noexcept { return data_; }

std::string create_password_hash(PasswordBytes, const PasswordHashingOptions &) {
    throw PasswordHashingNotImplemented{};
}

PasswordVerification verify_password(PasswordBytes, std::string_view,
                                     const PasswordHashingOptions &) {
    throw PasswordHashingNotImplemented{};
}

PasswordUpgradeResult propose_password_upgrade(PasswordBytes, std::string_view,
                                               PasswordLoginSource &,
                                               const PasswordHashingOptions &) {
    throw PasswordHashingNotImplemented{};
}

} // namespace subathon::server
