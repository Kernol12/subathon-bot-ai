// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "credential_record.hpp"

#include <cstddef>
#include <cstring>
#include <sodium.h>
#include <span>

namespace subathon::server::credential_detail {
inline constexpr std::size_t max_plaintext_size = 65536;
inline constexpr std::size_t min_scope_size = 11;
inline constexpr std::size_t max_scope_size = 16394;
inline constexpr std::size_t header_size = 48;
inline constexpr std::size_t envelope_overhead =
    header_size + crypto_aead_xchacha20poly1305_ietf_ABYTES;

[[nodiscard]] EncodedBytes encode_put_scope(const CredentialScope &scope, std::size_t value_size);

[[nodiscard]] inline bool sodium_ready() noexcept {
    const int initialized = sodium_init();
    const auto *version = sodium_version_string();
    return (initialized == 0 || initialized == 1) && version &&
           std::strcmp(version, SODIUM_VERSION_STRING) == 0 &&
           sodium_library_version_major() == SODIUM_LIBRARY_VERSION_MAJOR &&
           sodium_library_version_minor() == SODIUM_LIBRARY_VERSION_MINOR;
}

// Call only after sodium readiness. Expose writable bytes only after checked
// locking, since sodium_malloc's internal attempt to lock can fail silently.
class GuardedBytes final {
  public:
    explicit GuardedBytes(std::size_t size) noexcept
        : size_(size), data_(static_cast<unsigned char *>(sodium_malloc(size))) {
        if (data_ && sodium_mlock(data_, size_) != 0) {
            sodium_free(data_);
            data_ = nullptr;
        }
    }

    GuardedBytes(const GuardedBytes &) = delete;
    GuardedBytes &operator=(const GuardedBytes &) = delete;
    ~GuardedBytes() {
        if (data_) {
            sodium_memzero(data_, size_);
            sodium_free(data_);
        }
    }

    [[nodiscard]] std::span<unsigned char> bytes() noexcept {
        return data_ ? std::span{data_, size_} : std::span<unsigned char>{};
    }
    [[nodiscard]] std::span<const unsigned char> bytes() const noexcept {
        return data_ ? std::span<const unsigned char>{data_, size_}
                     : std::span<const unsigned char>{};
    }

  private:
    std::size_t size_;
    unsigned char *data_;
};
} // namespace subathon::server::credential_detail
