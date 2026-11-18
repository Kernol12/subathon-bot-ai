// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "oracles.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace account_capture_test {
// Fixed fixture construction only, with capacity checked BEFORE copying/repeating.
// No SUT parser or computed expected status/size is used to derive an oracle.
class Literal {
    std::array<char, 65536> bytes_{};
    std::size_t used_ = 0;

  public:
    void add(std::string_view s) {
        if (s.size() > bytes_.size() - used_)
            throw std::runtime_error("FIXTURE: literal bound");
        std::copy(s.begin(), s.end(), bytes_.begin() + used_);
        used_ += s.size();
    }
    void repeat(char c, std::size_t n) {
        if (n > bytes_.size() - used_)
            throw std::runtime_error("FIXTURE: literal bound");
        std::fill_n(bytes_.begin() + used_, n, c);
        used_ += n;
    }
    std::string_view view() const { return {bytes_.data(), used_}; }
};
} // namespace account_capture_test
