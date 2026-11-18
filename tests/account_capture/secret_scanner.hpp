// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
namespace account_capture_test {
// Pure fixture assertion helper, not a logger/output hook. Fixed literal canaries
// and encodings; no secret strings or payload chunks are retained. Offsets/counts
// only. Source/sink binding and launch-to-shutdown completeness remain PENDING.
inline constexpr std::array<std::string_view, 8> scanner_needles{
    "CNY-s11-7q9",
    "s11-7q9",
    "434e592d7331312d377139",
    "434E592D7331312D377139",
    "%43%4e%59%2d%73%31%31%2d%37%71%39",
    "%43%4E%59%2D%73%31%31%2D%37%71%39",
    "Q05ZLXMxMS03cTk=",
    "CNY\\u002ds11\\u002d7q9"};
class SecretScanner {
    std::array<std::uint8_t, scanner_needles.size()> matched_{};
    std::uint64_t bytes_ = 0, hits_ = 0, first_ = 0;
    bool overflow_ = false;

  public:
    static constexpr std::uint64_t ceiling = 2097152;
    // Prefix automaton by bounded suffix search, retaining only prefix LENGTHS.
    // On mismatch keep the longest suffix that is a prefix of the literal. There
    // are at most36 literal bytes/pattern; no attacker-driven allocation/history.
    void feed(std::string_view chunk) noexcept {
        if (chunk.size() > ceiling - bytes_) {
            overflow_ = true;
            return;
        }
        for (const char c : chunk) {
            ++bytes_;
            for (std::size_t i = 0; i < scanner_needles.size(); ++i) {
                const auto needle = scanner_needles[i];
                const auto old = matched_[i];
                unsigned next = std::min<std::size_t>(old + 1, needle.size());
                for (; next > 0; --next) {
                    if (needle[next - 1] != c)
                        continue;
                    bool equal = true;
                    // Prior matched prefix plus current byte is the entire
                    // candidate; compare its suffix without materializing bytes.
                    for (unsigned k = 0; k + 1 < next; ++k)
                        if (needle[k] != needle[old + 1 - next + k]) {
                            equal = false;
                            break;
                        }
                    if (equal)
                        break;
                }
                matched_[i] = static_cast<std::uint8_t>(next);
                if (next == needle.size()) {
                    if (hits_++ == 0)
                        first_ = bytes_ - needle.size();
                    // Retain full prefix; next iteration includes its suffix.
                }
            }
        }
    }
    bool clean() const noexcept { return !overflow_ && hits_ == 0; }
    bool overflow() const noexcept { return overflow_; }
    std::uint64_t hits() const noexcept { return hits_; }
    std::uint64_t first_offset() const noexcept { return first_; }
    std::uint64_t bytes() const noexcept { return bytes_; }
};
static_assert(sizeof(SecretScanner) <= 64);
} // namespace account_capture_test
