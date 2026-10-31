// SPDX-License-Identifier: GPL-3.0-or-later
// Contract revision 1 checks normalized rational components, independent of raw storage.
#include <gtest/gtest.h>
#include <subathon/core/exact_decimal.hpp>

namespace {
using namespace subathon::core;
using boost::multiprecision::cpp_int;

TEST(ExactDecimalRegression, ZeroUsesNormalizedComponentsAfterExactDivision) {
    // Mathematically zero is 0/1 regardless of an intermediate denominator's size.
    const Exact zero = Exact(0) / (cpp_int(1) << 4096);
    EXPECT_EQ(round_effect_units(zero), 0);
}
} // namespace
