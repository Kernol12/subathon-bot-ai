// SPDX-License-Identifier: GPL-3.0-or-later
// Third-party dependency foundation only. No project headers or feature linkage.
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/version.hpp>
#include <iostream>

int main() {
    using boost::multiprecision::cpp_int;
    using boost::multiprecision::cpp_rational;
    const cpp_int boundary = cpp_int(1) << 4095;
    const cpp_int over = cpp_int(1) << 4096;
    const cpp_int maximum = over - 1;
    const cpp_int product = maximum * maximum;
    const cpp_int temporary = product * product;
    const cpp_int p3 = boost::multiprecision::pow(cpp_int(3), 2584);
    const cpp_int p5 = boost::multiprecision::pow(cpp_int(5), 1764);
    const cpp_int pair = p3 * p5;
    const bool facts[] = {
        cpp_rational(6) / 5 == cpp_rational(12) / 10,
        cpp_rational(1) / (cpp_rational(6) / 5) == cpp_rational(5) / 6,
        (cpp_rational(1) / 2) / (cpp_rational(5) / 6) == cpp_rational(3) / 5,
        (cpp_rational(2469) / 2000) * (cpp_rational(6) / 5) == cpp_rational(7407) / 5000,
        boost::multiprecision::msb(boundary) + 1 == 4096,
        boost::multiprecision::msb(over) + 1 == 4097,
        over.str().size() == 1234,
        boost::multiprecision::msb(product) + 1 == 8192,
        boost::multiprecision::msb(temporary) + 1 == 16384,
        boost::multiprecision::msb(p3) + 1 == 4096,
        boost::multiprecision::msb(p5) + 1 == 4096,
        boost::multiprecision::msb(pair) + 1 == 8192,
        numerator(cpp_rational(pair) / p3) == p5,
        denominator(cpp_rational(pair) / p3) == 1,
    };
    for (unsigned index = 0; index < sizeof(facts) / sizeof(facts[0]); ++index) {
        if (!facts[index]) {
            std::cerr << "FAIL rate snapshot Boost foundation fact " << index << '\n';
            return 1;
        }
    }
    std::cout << "PASS rate snapshot Boost foundation: 14 facts, Boost " << BOOST_LIB_VERSION
              << '\n';
    return 0;
}
