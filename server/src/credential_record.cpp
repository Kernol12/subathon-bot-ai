// SPDX-License-Identifier: GPL-3.0-or-later
#include "credential_internal.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <new>
#include <utility>

namespace subathon::server {
struct CredentialValue::Storage {
    explicit Storage(std::size_t size) noexcept : payload(size) {}
    credential_detail::GuardedBytes payload;
};

CredentialValue::CredentialValue(std::unique_ptr<Storage> storage) noexcept
    : storage_(std::move(storage)) {}
CredentialValue::~CredentialValue() = default;
CredentialValue::CredentialValue(CredentialValue &&) noexcept = default;
CredentialValue &CredentialValue::operator=(CredentialValue &&) noexcept = default;

std::span<const unsigned char> CredentialValue::bytes() const noexcept {
    return storage_ ? std::as_const(storage_->payload).bytes() : std::span<const unsigned char>{};
}

namespace credential_detail {
namespace {
constexpr std::size_t max_ref_size = 4096;
constexpr std::array<unsigned char, 4> magic{'S', 'C', 'R', '1'};
constexpr std::size_t id_offset = magic.size();
constexpr std::size_t nonce_offset = id_offset + KeyIdView::extent;
constexpr std::size_t nonce_size = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
constexpr std::size_t length_offset = nonce_offset + nonce_size;
constexpr char aad_domain[] = "subathon.provider-credential.v1"; // Include the terminating NUL.
static_assert(nonce_size == 24 && length_offset + 4 == header_size &&
              KeyView::extent == crypto_aead_xchacha20poly1305_ietf_KEYBYTES &&
              envelope_overhead == 64);

unsigned char provider_tag(ConnectorProvider provider) noexcept {
    switch (provider) {
    case ConnectorProvider::twitch:
        return 1;
    case ConnectorProvider::betterplace:
        return 2;
    case ConnectorProvider::patreon:
        return 3;
    case ConnectorProvider::streamlabs:
        return 4;
    case ConnectorProvider::streamelements:
        return 5;
    case ConnectorProvider::kofi:
        return 6;
    case ConnectorProvider::tipeeestream:
        return 7;
    }
    return 0;
}

void append_ref(RecordBytes &out, std::string_view ref) {
    out.push_back(static_cast<unsigned char>(ref.size() >> 8));
    out.push_back(static_cast<unsigned char>(ref.size()));
    out.insert(out.end(), ref.begin(), ref.end());
}

RecordBytes associated_data(std::span<const unsigned char, header_size> header,
                            std::span<const unsigned char> scope) {
    RecordBytes aad;
    aad.reserve(sizeof(aad_domain) + header.size() + scope.size());
    aad.insert(aad.end(), std::begin(aad_domain), std::end(aad_domain));
    aad.insert(aad.end(), header.begin(), header.end());
    aad.insert(aad.end(), scope.begin(), scope.end());
    return aad;
}
} // namespace

struct ValueAccess {
    static std::variant<CredentialValue, CredentialError>
    decrypt(std::span<const unsigned char> ciphertext, std::span<const unsigned char> aad,
            std::span<const unsigned char, nonce_size> nonce, KeyView key, std::size_t size) {
        auto storage = std::unique_ptr<CredentialValue::Storage>(
            new (std::nothrow) CredentialValue::Storage(size));
        if (!storage || storage->payload.bytes().empty())
            return CredentialError::crypto_unavailable;

        unsigned long long written{};
        if (crypto_aead_xchacha20poly1305_ietf_decrypt(
                storage->payload.bytes().data(), &written, nullptr, ciphertext.data(),
                ciphertext.size(), aad.data(), aad.size(), nonce.data(), key.data()) != 0)
            return CredentialError::authentication_failed;
        if (written != size)
            return CredentialError::crypto_unavailable;
        return CredentialValue(std::move(storage));
    }
};

namespace {
EncodedBytes encode_scope_and_value_shape(const CredentialScope &scope, bool has_value) {
    const auto *account = std::get_if<AccountSubject>(&scope.subject);
    const auto account_ref = account ? account->account_ref : std::string_view{};
    if (std::max({scope.installation_ref.size(), scope.connector_ref.size(), account_ref.size(),
                  scope.slot_ref.size()}) > max_ref_size)
        return CredentialError::resource_limit;

    const auto provider = provider_tag(scope.provider);
    if (!has_value || !provider || scope.subject.valueless_by_exception() ||
        scope.installation_ref.empty() || scope.connector_ref.empty() || scope.slot_ref.empty() ||
        (account && account_ref.empty()))
        return CredentialError::invalid_input;

    RecordBytes out;
    const std::size_t framing_size = account ? 10 : 8;
    out.reserve(framing_size + scope.installation_ref.size() + scope.connector_ref.size() +
                scope.slot_ref.size() + account_ref.size());
    append_ref(out, scope.installation_ref);
    out.push_back(provider);
    append_ref(out, scope.connector_ref);
    out.push_back(account ? 2 : 1);
    if (account)
        append_ref(out, account_ref);
    append_ref(out, scope.slot_ref);
    return out;
}

} // namespace

EncodedBytes encode_scope(const CredentialScope &scope) {
    return encode_scope_and_value_shape(scope, true);
}

EncodedBytes encode_put_scope(const CredentialScope &scope, std::size_t value_size) {
    if (value_size > max_plaintext_size)
        return CredentialError::resource_limit;
    return encode_scope_and_value_shape(scope, value_size != 0);
}

EncodedBytes seal_record(std::span<const unsigned char> scope,
                         std::span<const unsigned char> plaintext, KeyIdView id, KeyView key) {
    if (scope.size() > max_scope_size || plaintext.size() > max_plaintext_size)
        return CredentialError::resource_limit;
    if (scope.size() < min_scope_size || plaintext.empty())
        return CredentialError::invalid_input;
    if (!sodium_ready())
        return CredentialError::crypto_unavailable;

    GuardedBytes input(plaintext.size());
    if (input.bytes().empty())
        return CredentialError::crypto_unavailable;
    std::ranges::copy(plaintext, input.bytes().begin());

    RecordBytes out(envelope_overhead + plaintext.size());
    auto header = std::span(out).first<header_size>();
    std::ranges::copy(magic, header.begin());
    std::ranges::copy(id, header.subspan<id_offset, KeyIdView::extent>().begin());
    const auto nonce = header.subspan<nonce_offset, nonce_size>();
    randombytes_buf(nonce.data(), nonce.size());

    const auto length = header.subspan<length_offset, 4>();
    for (std::size_t i = 0; i < length.size(); ++i)
        length[i] = static_cast<unsigned char>(plaintext.size() >> (8 * (length.size() - 1 - i)));

    const auto aad = associated_data(header, scope);
    unsigned long long written{};
    if (crypto_aead_xchacha20poly1305_ietf_encrypt(
            out.data() + header.size(), &written, input.bytes().data(), input.bytes().size(),
            aad.data(), aad.size(), nullptr, nonce.data(), key.data()) != 0 ||
        written != plaintext.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES)
        return CredentialError::crypto_unavailable;
    return out;
}

std::variant<CredentialValue, CredentialError> open_record(std::span<const unsigned char> scope,
                                                           std::span<const unsigned char> record,
                                                           KeyIdView id, KeyView key) {
    if (scope.size() > max_scope_size)
        return CredentialError::resource_limit;
    if (scope.size() < min_scope_size)
        return CredentialError::invalid_input;
    if (record.size() <= envelope_overhead ||
        record.size() > envelope_overhead + max_plaintext_size)
        return CredentialError::invalid_record;

    const auto header = record.first<header_size>();
    if (!std::ranges::equal(header.first<magic.size()>(), magic))
        return CredentialError::invalid_record;
    std::uint32_t size{};
    for (const auto byte : header.subspan<length_offset, 4>())
        size = (size << 8) | byte;
    if (size != record.size() - envelope_overhead)
        return CredentialError::invalid_record;
    if (!std::ranges::equal(header.subspan<id_offset, KeyIdView::extent>(), id))
        return CredentialError::key_unavailable;
    if (!sodium_ready())
        return CredentialError::crypto_unavailable;

    const auto aad = associated_data(header, scope);
    return ValueAccess::decrypt(record.subspan<header_size>(), aad,
                                header.subspan<nonce_offset, nonce_size>(), key, size);
}
} // namespace credential_detail
} // namespace subathon::server
