// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <span>
#include <subathon/server/credential_storage.hpp>
#include <variant>
#include <vector>

namespace subathon::server::credential_detail {
using RecordBytes = std::vector<unsigned char>;
using EncodedBytes = std::variant<RecordBytes, CredentialError>;
using KeyIdView = std::span<const unsigned char, 16>;
using KeyView = std::span<const unsigned char, 32>;

[[nodiscard]] EncodedBytes encode_scope(const CredentialScope &scope);

// The repository supplies validated canonical scope and protected key/input views.
// Sealing owns the nonce draw. Opening validates framing before key selection
// and releases an owned protected value only after authentication.
[[nodiscard]] EncodedBytes seal_record(std::span<const unsigned char> scope,
                                       std::span<const unsigned char> plaintext, KeyIdView id,
                                       KeyView key);
[[nodiscard]] std::variant<CredentialValue, CredentialError>
open_record(std::span<const unsigned char> scope, std::span<const unsigned char> record,
            KeyIdView id, KeyView key);
} // namespace subathon::server::credential_detail
