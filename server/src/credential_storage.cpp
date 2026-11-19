// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/credential_storage.hpp>

#include <subathon/server/sqlite_writer.hpp>

namespace subathon::server {
struct CredentialKey::Storage {};

CredentialKey::CredentialKey(std::unique_ptr<Storage> storage) noexcept
    : storage_(std::move(storage)) {}
CredentialKey::~CredentialKey() = default;
CredentialKey::CredentialKey(CredentialKey &&) noexcept = default;
CredentialKey &CredentialKey::operator=(CredentialKey &&) noexcept = default;

CredentialProvisionResult provision(PrivateKeyPath) {
    return CredentialError::feature_not_implemented; // NI: protected Linux key provisioning
}

CredentialRepository::CredentialRepository(const SqliteWorkSession &session,
                                           const CredentialKey &key) noexcept
    : session_(session), key_(key) {}

CredentialLoadResult CredentialRepository::load(const CredentialScope &) const {
    return CredentialError::feature_not_implemented; // NI: exact bounded writer selection
}

CredentialPutResult CredentialRepository::stage_put(const CredentialScope &,
                                                    std::span<const unsigned char>) const {
    return CredentialError::feature_not_implemented; // NI: authenticated staging
}

std::optional<CredentialValue> finish_credential_load(std::optional<CredentialLoadResult>,
                                                      const SqliteWorkResult &) {
    return std::nullopt; // NI: gated typed completion delivery
}
} // namespace subathon::server
