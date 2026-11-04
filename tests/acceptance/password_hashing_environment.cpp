// SPDX-License-Identifier: GPL-3.0-or-later
// Standalone selected-SDK foundation: no project headers, fixtures, helpers or symbols.
// H0: libsodium 1.0.22 test/default/pwhash_argon2id.c, tv3[2] and published .exp.
// H1: hashcat v7.0.0 src/modules/module_34000.c, ST_PASS and ST_HASH (MIT data).
#include <sodium.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace {
constexpr char h0[] = "$argon2id$v=19$m=4882,t=2,p=1$bA81arsiXysd3WbTRzmEOw$"
                      "Nm8QBM+7RH1DXo9rvp5cwKEOOOfD2g6JuxlXihoNcpE";
constexpr char h1[] = "$argon2id$v=19$m=65536,t=3,p=1$FBMjI4RJBhIykCgol1KEJA$"
                      "2ky5GAdhT1kH4kIgPN/oERE3Taiy43vNN70a3HpiKQU";
constexpr std::array<unsigned char, 16> h0_salt{0x6c, 0x0f, 0x35, 0x6a, 0xbb, 0x22, 0x5f, 0x2b,
                                                0x1d, 0xdd, 0x66, 0xd3, 0x47, 0x39, 0x84, 0x3b};
constexpr std::array<unsigned char, 32> h0_digest{
    0x36, 0x6f, 0x10, 0x04, 0xcf, 0xbb, 0x44, 0x7d, 0x43, 0x5e, 0x8f, 0x6b, 0xbe, 0x9e, 0x5c, 0xc0,
    0xa1, 0x0e, 0x38, 0xe7, 0xc3, 0xda, 0x0e, 0x89, 0xbb, 0x19, 0x57, 0x8a, 0x1a, 0x0d, 0x72, 0x91};

bool require(bool condition, const char *label) {
    if (!condition) {
        std::fprintf(stderr, "Password SDK foundation failed: %s\n", label);
    }
    return condition;
}

// Constructed only inside main after initialization; cleanup also covers early returns.
struct ProtectedBytes {
    void *data{};
    std::size_t size{};
    bool locked{};
    ~ProtectedBytes() {
        if (data) {
            sodium_memzero(data, size);
            if (locked) {
                (void)sodium_munlock(data, size);
            }
            sodium_free(data);
        }
    }
};

bool canonical_field(std::string_view field, std::size_t decoded_size) {
    std::array<unsigned char, 32> decoded{};
    std::array<char, 45> encoded{};
    std::size_t actual{};
    const char *end{};
    if (sodium_base642bin(decoded.data(), decoded.size(), field.data(), field.size(), nullptr,
                          &actual, &end, sodium_base64_VARIANT_ORIGINAL_NO_PADDING) != 0 ||
        actual != decoded_size || end != field.data() + field.size()) {
        return false;
    }
    if (!sodium_bin2base64(encoded.data(), encoded.size(), decoded.data(), actual,
                           sodium_base64_VARIANT_ORIGINAL_NO_PADDING)) {
        return false;
    }
    const auto encoded_end = std::find(encoded.begin(), encoded.end(), '\0');
    return encoded_end != encoded.end() &&
           field == std::string_view(encoded.data(),
                                     static_cast<std::size_t>(encoded_end - encoded.begin()));
}

bool generated_profile(const std::array<char, 128> &record) {
    const auto terminator = std::find(record.begin(), record.end(), '\0');
    if (terminator == record.end()) {
        return false;
    }
    const std::string_view text(record.data(),
                                static_cast<std::size_t>(terminator - record.begin()));
    constexpr std::string_view prefix = "$argon2id$v=19$m=65536,t=3,p=1$";
    if (!text.starts_with(prefix)) {
        return false;
    }
    const auto separator = text.find('$', prefix.size());
    if (separator == std::string_view::npos) {
        return false;
    }
    const auto salt = text.substr(prefix.size(), separator - prefix.size());
    const auto digest = text.substr(separator + 1);
    return salt.size() == 22 && digest.size() == 43 && canonical_field(salt, 16) &&
           canonical_field(digest, 32);
}
} // namespace

int main() {
    if (!require(std::string_view(SODIUM_VERSION_STRING) == "1.0.22" &&
                     SODIUM_LIBRARY_VERSION_MAJOR == 26 && SODIUM_LIBRARY_VERSION_MINOR == 4 &&
                     crypto_pwhash_argon2id_ALG_ARGON2ID13 == 2 &&
                     crypto_pwhash_argon2id_SALTBYTES == 16 &&
                     crypto_pwhash_argon2id_STRBYTES == 128,
                 "selected header identity and constants")) {
        return 1;
    }
    const int initialized = sodium_init();
    if (!require(initialized == 0 || initialized == 1, "initialization readiness") ||
        !require(sodium_init() == 1, "repeated initialization readiness") ||
        !require(std::strcmp(sodium_version_string(), "1.0.22") == 0 &&
                     sodium_library_version_major() == 26 && sodium_library_version_minor() == 4,
                 "selected native runtime identity")) {
        return 1;
    }

    ProtectedBytes empty{sodium_malloc(1), 1, false};
    if (!require(empty.data != nullptr, "one-byte guarded empty allocation")) {
        return 1;
    }
    if (!require(sodium_mlock(empty.data, empty.size) == 0, "explicit empty-buffer lock")) {
        return 1;
    }
    empty.locked = true;
    *static_cast<char *>(empty.data) = '\0';
    const auto *empty_password = static_cast<const char *>(empty.data);
    if (!require(crypto_pwhash_argon2id_str_verify(h0, empty_password, 0) == 0,
                 "published H0 empty-password verification") ||
        !require(crypto_pwhash_argon2id_str_verify(h0, empty_password, 1) == -1,
                 "one NUL is not the published empty input") ||
        !require(sizeof(h0) - 1 == 96 && 4882U * 1024U == 4999168U,
                 "literal record and memory sizes")) {
        return 1;
    }
    std::array<unsigned char, 32> derived{};
    if (!require(crypto_pwhash_argon2id(derived.data(), derived.size(), empty_password, 0,
                                        h0_salt.data(), 2, 4999168,
                                        crypto_pwhash_argon2id_ALG_ARGON2ID13) == 0 &&
                     derived == h0_digest,
                 "raw explicit-algorithm H0 digest equals independently decoded published bytes") ||
        !require(crypto_pwhash_argon2id_str_verify(h1, "hashcat", 7) == 0,
                 "published hashcat profile verification") ||
        !require(crypto_pwhash_argon2id_str_verify(h1, "hashcaT", 7) == -1,
                 "different public hashcat input unsuccessful")) {
        return 1;
    }
    sodium_memzero(derived.data(), derived.size());

    // A real SDK round trip is a byte/profile capability check, not an independent hash oracle.
    constexpr std::array<unsigned char, 5> raw{0x61, 0x00, 0x62, 0xc3, 0xa9};
    ProtectedBytes bytes{sodium_malloc(raw.size()), raw.size(), false};
    if (!require(bytes.data != nullptr, "raw-byte guarded allocation") ||
        !require(sodium_mlock(bytes.data, bytes.size) == 0, "explicit raw-byte buffer lock")) {
        return 1;
    }
    bytes.locked = true;
    std::memcpy(bytes.data, raw.data(), raw.size());
    std::array<char, 128> generated{};
    const auto *password = static_cast<const char *>(bytes.data);
    if (!require(crypto_pwhash_argon2id_str(generated.data(), password, raw.size(), 3, 67108864) ==
                     0,
                 "real custom application profile creation") ||
        !require(generated_profile(generated), "canonical selected generated profile") ||
        !require(crypto_pwhash_argon2id_str_verify(generated.data(), password, raw.size()) == 0,
                 "NUL and non-ASCII explicit-length round trip") ||
        !require(crypto_pwhash_argon2id_str_verify(generated.data(), password, 1) == -1,
                 "truncated public byte input unsuccessful")) {
        return 1;
    }
    sodium_memzero(bytes.data, bytes.size);
    const auto *cleared = static_cast<const unsigned char *>(bytes.data);
    if (!require(std::all_of(cleared, cleared + bytes.size,
                             [](unsigned char byte) { return byte == 0; }),
                 "guarded buffer clearing") ||
        !require(sodium_munlock(bytes.data, bytes.size) == 0, "explicit raw-byte unlock")) {
        return 1;
    }
    bytes.locked = false;
    sodium_memzero(generated.data(), generated.size());
    std::puts("PASS: selected password SDK identity, published vectors, guarded memory and "
              "byte/profile facts");
    return 0;
}
