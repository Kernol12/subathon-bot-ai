// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "credential_internal.hpp"

#include <QByteArray>

namespace subathon::server::credential_detail {
// Shared exact-row operations for the connector and FX families.
using SelectedRecord = std::variant<QByteArray, CredentialMissing, CredentialError>;

SelectedRecord select_record(const SqliteWorkSession &, const QByteArray &scope);
QByteArray blob(std::span<const unsigned char>);
std::span<const unsigned char> byte_view(const QByteArray &);
void store_record(const SqliteWorkSession &, const QByteArray &scope, const QByteArray *prior,
                  const QByteArray &record);
void erase_record(const SqliteWorkSession &, const QByteArray &scope, const QByteArray &prior);

// Keep the provisioned key and its raw bytes inside the credential repository.
struct RepositoryAccess {
    static bool usable(const CredentialKey *) noexcept;
    static CredentialLoadResult open(const CredentialKey &, std::span<const unsigned char> scope,
                                     const QByteArray &record);
    static EncodedBytes seal_guarded(const CredentialKey &, std::span<const unsigned char> scope,
                                     const GuardedBytes &);
};
} // namespace subathon::server::credential_detail
