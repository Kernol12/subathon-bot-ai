// SPDX-License-Identifier: GPL-3.0-or-later
// Independent primitive foundation: no project header, fixture, symbol or library.
// Compile only until this exact program receives different review.
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>

namespace {
unsigned facts = 0;
void require(bool condition, const char *label) {
    if (!condition)
        throw std::runtime_error(label);
    ++facts;
}
} // namespace

int main() {
    using boost::multiprecision::cpp_int;
    using boost::multiprecision::cpp_rational;
    try {
        require(sizeof(std::int64_t) == 8, "signed64 width");
        require(std::numeric_limits<std::int64_t>::min() == -9'223'372'036'854'775'807LL - 1,
                "signed minimum literal");
        require(std::numeric_limits<std::int64_t>::max() == 9'223'372'036'854'775'807LL,
                "signed maximum literal");
        require(std::numeric_limits<std::uint64_t>::max() == 18'446'744'073'709'551'615ULL,
                "unsigned maximum literal");
        const cpp_int low(std::numeric_limits<std::int64_t>::min());
        const cpp_int high(std::numeric_limits<std::int64_t>::max());
        const cpp_int distance = high - low;
        require(low == cpp_int("-9223372036854775808"), "widen signed minimum before negation");
        require(distance == cpp_int("18446744073709551615"), "widen before subtraction");
        require(-low == cpp_int("9223372036854775808"), "positive minimum magnitude");
        require(distance.convert_to<std::uint64_t>() == 18'446'744'073'709'551'615ULL,
                "full unsigned conversion");
        require(distance.convert_to<std::string>() == "18446744073709551615",
                "maximum lossless decimal spelling");
        require(std::gcd(9'223'372'036'854'775'808ULL, 18'446'744'073'709'551'615ULL) == 1ULL,
                "full-width unsigned gcd");
        const cpp_rational full = cpp_rational(std::uint64_t{9'223'372'036'854'775'808ULL}) /
                                  std::uint64_t{18'446'744'073'709'551'615ULL};
        require(boost::multiprecision::numerator(full) == cpp_int("9223372036854775808"),
                "full-width reduced numerator");
        require(boost::multiprecision::denominator(full) == cpp_int("18446744073709551615"),
                "full-width reduced denominator");
        const cpp_rational third = cpp_rational(1) / 3;
        require(boost::multiprecision::numerator(third) == 1, "literal third numerator");
        require(boost::multiprecision::denominator(third) == 3, "literal third denominator");
        const cpp_rational tenth = cpp_rational(100'000'000) / 1'000'000'000;
        require(boost::multiprecision::numerator(tenth) == 1, "reduce bounded numerator");
        require(boost::multiprecision::denominator(tenth) == 10, "reduce bounded denominator");
        const cpp_rational zero = cpp_rational(0) / 7;
        require(boost::multiprecision::numerator(zero) == 0 &&
                    boost::multiprecision::denominator(zero) == 1,
                "canonical zero fraction");
        require(cpp_int(1'000'000'000) - 800'000'000 == 200'000'000, "literal deficit distance");
        require(cpp_int(2'000'000'000) - 800'000'000 == 1'200'000'000,
                "literal remaining distance");
        require(3 + 8 == 11, "input empty byte fact");
        require(3 + 8 + 16 + 1 + 8 == 36, "result empty byte fact");
        require(11 + 18 + 21 == 50, "input interval byte fact");
        require(1 + 17 + 9 + 9 + 8 + 8 + 18 == 70, "projection interval byte fact");
        require(11 + 21 + 16 + 70 == 118, "result interval byte fact");
        require(2 + 1 == 3 && 1 + 1 == 2, "independent input/output item facts");
        std::cout << "PASS: " << facts
                  << " independent std/Boost literal facts; no project linked\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
