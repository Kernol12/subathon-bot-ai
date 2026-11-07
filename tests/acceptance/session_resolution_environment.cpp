// SPDX-License-Identifier: GPL-3.0-or-later
// Standalone selected SDK foundation; no project headers, fixtures, wrappers or symbols.
// Expected bytes are independent supplier-v1 section-5 hashlib literals.
#include <sodium.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace {
bool require(bool condition, const char *label) {
    if (!condition) {
        std::fprintf(stderr, "Session SDK foundation failed: %s\n", label);
    }
    return condition;
}

// Fixed application byte storage only; this is not the native stack/work bound.
struct Scratch {
    std::array<unsigned char, 32> raw{};
    std::array<unsigned char, 60> framed{};
    std::array<unsigned char, 32> digest{};
    ~Scratch() {
        sodium_memzero(raw.data(), raw.size());
        sodium_memzero(framed.data(), framed.size());
        sodium_memzero(digest.data(), digest.size());
    }
};
constexpr std::array<std::array<unsigned char, 32>, 3> expected{
    {{0xa2, 0x26, 0xd5, 0x44, 0xff, 0x92, 0xf3, 0x24, 0x7d, 0xc7, 0x84,
      0xe9, 0x49, 0xc8, 0x46, 0x99, 0x9f, 0xde, 0x80, 0x1e, 0x45, 0x81,
      0xc6, 0xbb, 0x21, 0x9f, 0xde, 0x07, 0x05, 0x91, 0xd3, 0xea},
     {0xe2, 0x9b, 0xe8, 0xcd, 0x64, 0xa1, 0xfd, 0xea, 0x5c, 0x6f, 0xb3,
      0x5c, 0x35, 0x70, 0xa3, 0x22, 0xa6, 0x5e, 0x79, 0x3d, 0x85, 0x19,
      0x8c, 0x87, 0xa5, 0x66, 0x49, 0x0b, 0xe0, 0xab, 0x9b, 0x66},
     {0xce, 0xf0, 0x51, 0xcb, 0xdd, 0x51, 0x57, 0x1a, 0xc9, 0x0e, 0xbe,
      0xed, 0x6f, 0x59, 0x4e, 0xd4, 0xbd, 0x56, 0xa4, 0xdd, 0xed, 0x25,
      0x9c, 0xd9, 0xdc, 0xcb, 0x4b, 0x59, 0x3b, 0x2e, 0xcb, 0xe9}}};
} // namespace

int main() {
    if (!require(std::string_view(SODIUM_VERSION_STRING) == "1.0.22" &&
                     SODIUM_LIBRARY_VERSION_MAJOR == 26 && SODIUM_LIBRARY_VERSION_MINOR == 4 &&
                     crypto_generichash_BYTES_MIN <= 32 && crypto_generichash_BYTES_MAX >= 32 &&
                     sizeof(crypto_generichash_state) == 384,
                 "selected header identity and fixed profile")) {
        return 1;
    }
    const int initialized = sodium_init();
    if (!require(initialized == 0 || initialized == 1, "initialization readiness") ||
        !require(sodium_init() == 1, "repeated initialization readiness") ||
        !require(std::strcmp(sodium_version_string(), "1.0.22") == 0 &&
                     sodium_library_version_major() == 26 && sodium_library_version_minor() == 4 &&
                     std::strcmp(crypto_generichash_primitive(), "blake2b") == 0 &&
                     crypto_generichash_statebytes() == 384,
                 "selected native runtime and primitive identity")) {
        return 1;
    }
    Scratch scratch;
    constexpr char prefix[] = "subathon.account-session.v1";
    static_assert(sizeof(prefix) == 28);
    for (std::size_t row = 0; row < expected.size(); ++row) {
        for (std::size_t index = 0; index < scratch.raw.size(); ++index) {
            scratch.raw[index] = row == 0 ? 0 : row == 1 ? static_cast<unsigned char>(index) : 0xff;
        }
        std::memcpy(scratch.framed.data(), prefix, sizeof(prefix));
        std::copy(scratch.raw.begin(), scratch.raw.end(), scratch.framed.begin() + 28);
        if (!require(crypto_generichash(scratch.digest.data(), scratch.digest.size(),
                                        scratch.framed.data(), scratch.framed.size(), nullptr,
                                        0) == 0,
                     "real unkeyed single-part native hash") ||
            !require(scratch.digest == expected[row], "independent exact framed digest vector") ||
            !require(sodium_memcmp(scratch.digest.data(), expected[row].data(), 32) == 0,
                     "fixed32 equality")) {
            return 1;
        }
        scratch.digest[31] ^= 1;
        if (!require(sodium_memcmp(scratch.digest.data(), expected[row].data(), 32) != 0,
                     "fixed32 inequality includes final byte")) {
            return 1;
        }
        sodium_memzero(scratch.raw.data(), scratch.raw.size());
        sodium_memzero(scratch.framed.data(), scratch.framed.size());
        if (!require(std::all_of(scratch.raw.begin(), scratch.raw.end(),
                                 [](auto b) { return b == 0; }) &&
                         std::all_of(scratch.framed.begin(), scratch.framed.end(),
                                     [](auto b) { return b == 0; }),
                     "selected secure clearing of raw and framed buffers")) {
            return 1;
        }
    }
    std::puts("PASS: selected session SDK identity, independent framed BLAKE2b vectors, fixed32 "
              "comparison and secure clearing");
    return 0;
}
