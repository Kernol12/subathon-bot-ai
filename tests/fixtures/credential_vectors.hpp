// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <subathon/server/credential_storage.hpp>

namespace credential_vectors {
// Public synthetic bytes. SC01 derives from draft-irtf-cfrg-xchacha-03 A.1
// and independent Poly1305 arithmetic in the approved Stage 1 handoff.
template <std::size_t N> consteval auto hex(const char (&digits)[N]) {
    static_assert(N % 2 == 1);
    const auto nibble = [](char digit) { return digit >= 'a' ? digit - 'a' + 10 : digit - '0'; };
    std::array<unsigned char, N / 2> result{};
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] =
            static_cast<unsigned char>(16 * nibble(digits[2 * i]) + nibble(digits[2 * i + 1]));
    return result;
}

inline constexpr auto key = hex("808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f");
inline constexpr auto id = hex("101112131415161718191a1b1c1d1e1f");
inline constexpr auto nonce = hex("404142434445464748494a4b4c4d4e4f5051525354555657");
inline constexpr auto second_nonce = hex("606162636465666768696a6b6c6d6e6f7071727374757677");
inline constexpr auto third_nonce = hex("a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7");
inline constexpr auto plaintext = hex("00ff4100800a7f");
inline constexpr auto scope = hex("00024900010002ff430200014100027400");
inline constexpr auto key_file =
    hex("534b5031101112131415161718191a1b1c1d1e1f"
        "808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f");
inline constexpr auto record = hex("53435231101112131415161718191a1b1c1d1e1f"
                                   "404142434445464748494a4b4c4d4e4f5051525354555657"
                                   "00000007f1f332f4dbfa8ba60c190191b0c392d44387f2d9757bab");

inline constexpr subathon::server::CredentialScope exact{
    std::string_view("I\0", 2), subathon::server::ConnectorProvider::twitch,
    std::string_view("\xff"
                     "C",
                     2),
    subathon::server::AccountSubject{"A"}, std::string_view("t\0", 2)};

} // namespace credential_vectors
