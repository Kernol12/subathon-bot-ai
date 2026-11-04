// SPDX-License-Identifier: GPL-3.0-or-later
// Standalone std/Boost foundation for the exact-value boundary, not permission behavior.
// No project headers, fixtures, helpers, GTest, or runtime work before main.
#include <boost/integer/common_factor_rt.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/version.hpp>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <limits>
#include <string>

int main() {
    static_assert(BOOST_VERSION == 108300, "This foundation requires selected Boost 1.83.0");
    using boost::multiprecision::cpp_int;
    using boost::multiprecision::cpp_rational;
    using boost::multiprecision::denominator;
    using boost::multiprecision::numerator;

    unsigned failures = 0;
    unsigned checks = 0;
    const auto check = [&](bool condition, const char *label) {
        ++checks;
        if (!condition) {
            ++failures;
            std::fprintf(stderr, "FAIL: %s\n", label);
        }
    };

    try {
        const cpp_int signed_minimum(std::numeric_limits<std::int64_t>::min());
        check(signed_minimum == cpp_int("-9223372036854775808"), "signed64 minimum conversion");
        // Negate after widening; native signed abs/negation would overflow at this input.
        const cpp_int magnitude = -signed_minimum;
        check(magnitude == cpp_int("9223372036854775808"), "signed64 minimum exact magnitude");

        const cpp_int unsigned_zero(std::uint64_t{0});
        const cpp_int unsigned_maximum(std::numeric_limits<std::uint64_t>::max());
        const cpp_int two_to_64("18446744073709551616");
        check(unsigned_zero == 0, "unsigned64 zero conversion");
        check(unsigned_maximum == cpp_int("18446744073709551615"), "unsigned64 maximum conversion");
        check(unsigned_maximum + 1 == two_to_64, "unsigned64 maximum plus one stays exact");
        check((cpp_int(1) << 64) == two_to_64, "literal 2^64 agrees with exact shift");

        // Positive denominators: -3/4 < -5/7, and 0/3 > -1/7.
        check(cpp_int(-3) * 7 == -21, "negative ratio left cross-product");
        check(cpp_int(-5) * 4 == -20, "negative ratio right cross-product");
        check(cpp_int(-3) * 7 < cpp_int(-5) * 4, "negative ratio cross-product ordering");
        check(cpp_int(0) * 7 == 0, "zero cross-product remains zero");
        check(cpp_int(0) * 7 > cpp_int(-1) * 3, "zero versus negative cross-product ordering");

        check(boost::integer::gcd(cpp_int(54), cpp_int(24)) == 6, "literal small gcd");
        check(boost::integer::gcd(cpp_int(0), cpp_int(24)) == 24, "zero-operand gcd");
        const cpp_rational reduced = cpp_rational(42) / 56;
        const cpp_rational negative = cpp_rational(-6) / 8;
        const cpp_rational zero(0);
        check(numerator(reduced) == 3 && denominator(reduced) == 4,
              "SDK reduced positive rational");
        check(numerator(negative) == -3 && denominator(negative) == 4,
              "SDK reduced negative rational with positive denominator");
        check(numerator(zero) == 0 && denominator(zero) == 1, "SDK canonical zero rational");

        // These decimal powers are constructed with std::string, not an SDK output oracle.
        // Independently: 10^5000 has 5001 digits/16610 bits; 10^10000 has 10001/33220.
        const std::string long_decimal = "1" + std::string(5000, '0');
        const cpp_int long_integer(long_decimal);
        check(long_decimal.size() == 5001, "bounded long-decimal input size");
        check(boost::multiprecision::msb(long_integer) == 16609U,
              "long decimal independent magnitude-bit count exceeds 4096");
        check(long_integer.str() == long_decimal, "SDK decimal parse/format roundtrip only");
        const cpp_int squared = long_integer * long_integer;
        check(boost::multiprecision::msb(squared) == 33219U,
              "long product independent magnitude-bit count");
        check(squared.str() == "1" + std::string(10000, '0'), "literal decimal-power product");
    } catch (const std::exception &) {
        std::fputs("FAIL: unexpected SDK/standard-library exception\n", stderr);
        return 2;
    } catch (...) {
        std::fputs("FAIL: unexpected exception\n", stderr);
        return 2;
    }

    std::printf("%u std/Boost primitive checks, %u failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
