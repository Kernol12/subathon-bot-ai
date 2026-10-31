// SPDX-License-Identifier: GPL-3.0-or-later
// Specification v0.45 NUM-01/02/04; docs/contracts/exact-decimal-v1.md revision 1.
// Expected values below are literal fractions/integers, not production calculations.
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <string_view>
#include <subathon/core/exact_decimal.hpp>

namespace {
using namespace subathon::core;
using boost::multiprecision::cpp_int;
constexpr auto maximum_units = std::numeric_limits<std::int64_t>::max();
constexpr auto minimum_units = std::numeric_limits<std::int64_t>::min();

TEST(ExactDecimalAcceptance, PreservesSourcePrecisionAndTrailingZeros) {
    const auto result = parse_decimal("1.2300");
    EXPECT_EQ(result.original(), "1.2300");
    EXPECT_EQ(result.value(), Exact(123) / 100);
}

TEST(ExactDecimalAcceptance, PreservesExponentSpellingAndExactValue) {
    const auto result = parse_decimal("1.2300E-02");
    EXPECT_EQ(result.original(), "1.2300E-02");
    EXPECT_EQ(result.value(), Exact(123) / 10'000);
    EXPECT_EQ(parse_decimal("-25e+2").value(), -2500);
}

TEST(ExactDecimalAcceptance, RetainsDigitsBeyondBinaryFloatingPointPrecision) {
    const auto result = parse_decimal("0.10000000000000000001");
    EXPECT_EQ(result.original(), "0.10000000000000000001");
    EXPECT_EQ(result.value(),
              Exact(cpp_int("10000000000000000001")) / cpp_int("100000000000000000000"));
}

TEST(ExactDecimalAcceptance, OwnsItsSourceAfterCallerBufferChanges) {
    std::string source = "-42.00";
    const auto result = parse_decimal(source);
    source.assign("unrelated");
    EXPECT_EQ(result.original(), "-42.00");
    EXPECT_EQ(result.value(), -42);
}

TEST(ExactDecimalAcceptance, SignedZeroRetainsOriginalSpelling) {
    const auto result = parse_decimal("-0.000e+03");
    EXPECT_EQ(result.original(), "-0.000e+03");
    EXPECT_EQ(result.value(), 0);
}

TEST(ExactDecimalAcceptance, RejectsNonJsonNumberGrammar) {
    for (const std::string_view input :
         {"", "+1", "01", "-01", ".1", "1.", "1e", "1e+", "1e-", "--1", "1e1.0", " 1", "1 ", "1,2",
          "1_0", "0x1", "1/2", "\t1"}) {
        SCOPED_TRACE(input);
        EXPECT_THROW(static_cast<void>(parse_decimal(input)), std::invalid_argument);
    }
}

TEST(ExactDecimalAcceptance, RejectsNonFiniteSpellingsAcrossNumericEntryPoints) {
    for (const std::string_view input : {"NaN", "nan", "Infinity", "inf", "-Infinity"}) {
        SCOPED_TRACE(input);
        EXPECT_THROW(static_cast<void>(parse_decimal(input)), std::invalid_argument);
        EXPECT_THROW(static_cast<void>(parse_positive_rate(input)), std::invalid_argument);
        EXPECT_THROW(static_cast<void>(parse_nonnegative_factor(input)), std::invalid_argument);
    }
}

TEST(ExactDecimalAcceptance, RejectsEmbeddedNulInsteadOfParsingOnlyAPrefix) {
    const std::string source("1\0.25", 5);
    EXPECT_THROW(static_cast<void>(parse_decimal(source)), std::invalid_argument);
}

TEST(ExactDecimalAcceptance, AcceptsTheByteLimitWithLeadingExponentZeros) {
    // 2-byte prefix plus 510 exponent digits: exponent value zero, 512 bytes total.
    const std::string source = "1e" + std::string(510, '0');
    ASSERT_EQ(source.size(), 512U);
    const auto result = parse_decimal(source);
    EXPECT_EQ(result.original(), source);
    EXPECT_EQ(result.value(), 1);
}

TEST(ExactDecimalAcceptance, RejectsOneByteBeyondTheLimitBeforeSyntax) {
    EXPECT_THROW(static_cast<void>(parse_decimal("1e" + std::string(511, '0'))), std::length_error);
    EXPECT_THROW(static_cast<void>(parse_decimal(std::string(513, '?'))), std::length_error);
}

TEST(ExactDecimalAcceptance, AcceptsExactly256MantissaDigits) {
    const std::string source(256, '9');
    const auto result = parse_decimal(source);
    EXPECT_EQ(result.original(), source);
    // A sequence of 256 nines is exactly 10^256 - 1.
    EXPECT_EQ(result.value(), Exact(boost::multiprecision::pow(cpp_int(10), 256) - 1));
}

TEST(ExactDecimalAcceptance, RejectsOneMantissaDigitBeyondTheLimit) {
    EXPECT_THROW(static_cast<void>(parse_decimal("1" + std::string(256, '0'))), std::length_error);
    // All mantissa digits count, including zero before the decimal point.
    EXPECT_THROW(static_cast<void>(parse_decimal("0." + std::string(256, '0'))), std::length_error);
}

TEST(ExactDecimalAcceptance, AcceptsBothExponentMagnitudeBoundaries) {
    const cpp_int power = boost::multiprecision::pow(cpp_int(10), 256);
    EXPECT_EQ(parse_decimal("1e256").value(), Exact(power));
    EXPECT_EQ(parse_decimal("1e-256").value(), Exact(1) / power);
}

TEST(ExactDecimalAcceptance, RejectsExcessExponentEvenForZero) {
    for (const std::string_view input :
         {"1e257", "1e-257", "0e257", "-0e-257", "1e999999999999999999999"}) {
        SCOPED_TRACE(input);
        EXPECT_THROW(static_cast<void>(parse_decimal(input)), std::length_error);
    }
}

TEST(ExactDecimalAcceptance, PositiveRatesPreserveExactOriginalValue) {
    const auto result = parse_positive_rate("0.5000");
    EXPECT_EQ(result.original(), "0.5000");
    EXPECT_EQ(result.value(), Exact(1) / 2);
    EXPECT_EQ(parse_positive_rate("2e-2").value(), Exact(1) / 50);
}

TEST(ExactDecimalAcceptance, RatesRejectZeroSignedZeroAndNegativeValues) {
    for (const std::string_view input : {"0", "-0.00", "0e3", "-1", "-0.1"}) {
        SCOPED_TRACE(input);
        EXPECT_THROW(static_cast<void>(parse_positive_rate(input)), std::invalid_argument);
    }
}

TEST(ExactDecimalAcceptance, FactorsPermitZeroAndPositiveFractions) {
    const auto zero = parse_nonnegative_factor("-0.00");
    EXPECT_EQ(zero.original(), "-0.00");
    EXPECT_EQ(zero.value(), 0);
    EXPECT_EQ(parse_nonnegative_factor("0.5").value(), Exact(1) / 2);
    EXPECT_EQ(parse_nonnegative_factor("2").value(), 2);
}

TEST(ExactDecimalAcceptance, FactorsRejectNegativeValues) {
    EXPECT_THROW(static_cast<void>(parse_nonnegative_factor("-0.0001")), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(parse_nonnegative_factor("-2")), std::invalid_argument);
}

TEST(ExactDecimalAcceptance, ValidatorsRetainParserSyntaxAndResourceFailures) {
    EXPECT_THROW(static_cast<void>(parse_positive_rate("+1")), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(parse_nonnegative_factor("1.")), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(parse_positive_rate("0e257")), std::length_error);
    EXPECT_THROW(static_cast<void>(parse_nonnegative_factor("-1e-257")), std::length_error);
}

TEST(ExactDecimalAcceptance, PositiveHalfTiesRoundToEven) {
    EXPECT_EQ(round_effect_units(Exact(1) / 2), 0);
    EXPECT_EQ(round_effect_units(Exact(3) / 2), 2);
    EXPECT_EQ(round_effect_units(Exact(5) / 2), 2);
    EXPECT_EQ(round_effect_units(Exact(7) / 2), 4);
}

TEST(ExactDecimalAcceptance, NegativeHalfTiesRoundToEven) {
    EXPECT_EQ(round_effect_units(Exact(-1) / 2), 0);
    EXPECT_EQ(round_effect_units(Exact(-3) / 2), -2);
    EXPECT_EQ(round_effect_units(Exact(-5) / 2), -2);
    EXPECT_EQ(round_effect_units(Exact(-7) / 2), -4);
}

TEST(ExactDecimalAcceptance, BothSidesOfHalfTiesRemainDistinct) {
    EXPECT_EQ(round_effect_units(Exact(499) / 1000), 0);
    EXPECT_EQ(round_effect_units(Exact(501) / 1000), 1);
    EXPECT_EQ(round_effect_units(Exact(-499) / 1000), 0);
    EXPECT_EQ(round_effect_units(Exact(-501) / 1000), -1);
}

TEST(ExactDecimalAcceptance, IntegralValuesAndSigned64EndpointsAreUnchanged) {
    EXPECT_EQ(round_effect_units(Exact(0)), 0);
    EXPECT_EQ(round_effect_units(Exact(-17)), -17);
    EXPECT_EQ(round_effect_units(Exact(maximum_units)), maximum_units);
    EXPECT_EQ(round_effect_units(Exact(minimum_units)), minimum_units);
}

TEST(ExactDecimalAcceptance, RangeCheckOccursAfterRoundingAtSigned64Boundaries) {
    EXPECT_EQ(round_effect_units(Exact(maximum_units) + Exact(49) / 100), maximum_units);
    EXPECT_EQ(round_effect_units(Exact(minimum_units) - Exact(1) / 2), minimum_units);
}

TEST(ExactDecimalAcceptance, RoundedResultsBeyondSigned64RangeRequireReview) {
    // INT64_MAX is odd, so the upper half tie rounds to the out-of-range even integer.
    EXPECT_THROW(static_cast<void>(round_effect_units(Exact(maximum_units) + Exact(1) / 2)),
                 std::overflow_error);
    EXPECT_THROW(static_cast<void>(round_effect_units(Exact(minimum_units) - Exact(5001) / 10'000)),
                 std::overflow_error);
    EXPECT_THROW(static_cast<void>(round_effect_units(Exact(maximum_units) + 1)),
                 std::overflow_error);
    EXPECT_THROW(static_cast<void>(round_effect_units(Exact(minimum_units) - 1)),
                 std::overflow_error);
}

TEST(ExactDecimalAcceptance, TimeEffectsScaleExactlyBeforeFinalMilliseconds) {
    // 0.0005 and 0.0015 seconds become exact 0.5 and 1.5 milliseconds.
    EXPECT_EQ(round_effect_units((Exact(5) / 10'000) * 1000), 0);
    EXPECT_EQ(round_effect_units((Exact(15) / 10'000) * 1000), 2);
}

TEST(ExactDecimalAcceptance, ScoreEffectsScaleExactlyBeforeFinalMicroPoints) {
    // 0.0000005 and 0.0000015 points become exact 0.5 and 1.5 micro-points.
    EXPECT_EQ(round_effect_units((Exact(5) / 10'000'000) * 1'000'000), 0);
    EXPECT_EQ(round_effect_units((Exact(15) / 10'000'000) * 1'000'000), 2);
}

TEST(ExactDecimalAcceptance, IntermediateFractionsAreNotRounded) {
    // Rounding 1/3 early would lose the eventual whole unit.
    const Exact third = Exact(1) / 3;
    EXPECT_EQ(round_effect_units(third * 3), 1);
    EXPECT_EQ(round_effect_units((Exact(7) / 3) * 3), 7);
}

TEST(ExactDecimalAcceptance, FinalizerAccepts4096BitNormalizedComponents) {
    const cpp_int power = cpp_int(1) << 4095;
    // Coprime numerator and denominator both have exactly 4096 bits, ratio > 1/2.
    EXPECT_EQ(round_effect_units(Exact(power) / (power + 1)), 1);
    EXPECT_EQ(round_effect_units(Exact(1) / power), 0);
}

TEST(ExactDecimalAcceptance, FinalizerRejects4097BitNumeratorBeforeRangeConversion) {
    const cpp_int too_large = cpp_int(1) << 4096;
    EXPECT_THROW(static_cast<void>(round_effect_units(Exact(too_large))), std::length_error);
    EXPECT_THROW(static_cast<void>(round_effect_units(Exact(-too_large))), std::length_error);
}

TEST(ExactDecimalAcceptance, FinalizerRejects4097BitDenominatorBeforeRounding) {
    const cpp_int too_large = cpp_int(1) << 4096;
    EXPECT_THROW(static_cast<void>(round_effect_units(Exact(1) / too_large)), std::length_error);
    EXPECT_THROW(static_cast<void>(round_effect_units(Exact(-1) / too_large)), std::length_error);
}
} // namespace
