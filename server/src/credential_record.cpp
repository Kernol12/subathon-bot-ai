// SPDX-License-Identifier: GPL-3.0-or-later
#include "credential_record.hpp"

namespace subathon::server {
struct CredentialValue::Storage {};

CredentialValue::CredentialValue(std::unique_ptr<Storage> storage) noexcept
    : storage_(std::move(storage)) {}
CredentialValue::~CredentialValue() = default;
CredentialValue::CredentialValue(CredentialValue &&) noexcept = default;
CredentialValue &CredentialValue::operator=(CredentialValue &&) noexcept = default;

std::span<const unsigned char> CredentialValue::bytes() const noexcept { return {}; }

namespace credential_detail {
EncodedBytes encode_scope(const CredentialScope &) {
    return CredentialError::feature_not_implemented; // NI: canonical scope framing
}

EncodedBytes seal_record(std::span<const unsigned char>, std::span<const unsigned char>, KeyIdView,
                         KeyView) {
    return CredentialError::feature_not_implemented; // NI: authenticated record sealing
}

std::variant<CredentialValue, CredentialError>
open_record(std::span<const unsigned char>, std::span<const unsigned char>, KeyIdView, KeyView) {
    return CredentialError::feature_not_implemented; // NI: record preflight and authenticated
                                                     // opening
}
} // namespace credential_detail
} // namespace subathon::server
