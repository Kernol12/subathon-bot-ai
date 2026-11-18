// SPDX-License-Identifier: GPL-3.0-or-later
#include "secret_scanner.hpp"
#include <gtest/gtest.h>
namespace account_capture_test {
TEST(AccountCaptureScanner, RawFragmentAndEachLiteralEncodingAcrossEveryChunkBoundary) {
    // Positive controls are local scanner inputs, NOT a fabricated production log.
    for (const auto needle : scanner_needles) {
        for (std::size_t split = 0; split <= needle.size(); ++split) {
            SecretScanner scanner;
            scanner.feed("safe:");
            scanner.feed(needle.substr(0, split));
            scanner.feed(needle.substr(split));
            EXPECT_FALSE(scanner.clean());
            EXPECT_GE(scanner.hits(), 1U);
            EXPECT_EQ(scanner.first_offset(), 5U);
            EXPECT_FALSE(scanner.overflow());
        }
    }
}
TEST(AccountCaptureScanner, SingleByteChunksCannotHideCanary) {
    SecretScanner scanner;
    for (const char &c : scanner_needles[0])
        scanner.feed({&c, 1});
    EXPECT_FALSE(scanner.clean());
    EXPECT_GE(scanner.hits(), 1U);
}
TEST(AccountCaptureScanner, BenignResponseControlHasNoHits) {
    SecretScanner scanner;
    scanner.feed("HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
    EXPECT_TRUE(scanner.clean());
    EXPECT_EQ(scanner.hits(), 0U);
    EXPECT_GT(scanner.bytes(), 0U);
}
TEST(AccountCaptureScanner, CapacityEqualityThenOneByteOverIsFailure) {
    SecretScanner scanner;
    constexpr std::array<char, 8192> zeros{};
    for (unsigned i = 0; i < 256; ++i)
        scanner.feed({zeros.data(), zeros.size()});
    EXPECT_EQ(scanner.bytes(), 2097152U);
    EXPECT_TRUE(scanner.clean());
    scanner.feed("x");
    EXPECT_TRUE(scanner.overflow());
    EXPECT_FALSE(scanner.clean());
}
} // namespace account_capture_test
