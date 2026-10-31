// SPDX-License-Identifier: GPL-3.0-or-later
#include <limits>
#include <subathon/core/exact_decimal.hpp>

namespace subathon::core {
namespace {
using boost::multiprecision::cpp_int;

constexpr bool is_digit(char character) { return character >= '0' && character <= '9'; }

[[noreturn]] void invalid_decimal() {
    throw std::invalid_argument("Expected a complete ASCII JSON number");
}
} // namespace

ExactDecimal parse_decimal(std::string_view original) {
    if (original.size() > maximum_decimal_bytes) {
        throw std::length_error("Decimal exceeds the input byte limit");
    }

    std::size_t position = 0;
    const bool negative = position < original.size() && original[position] == '-';
    if (negative) {
        ++position;
    }
    const auto integer_begin = position;
    if (position < original.size() && original[position] == '0') {
        ++position;
    } else if (position < original.size() && original[position] >= '1' &&
               original[position] <= '9') {
        do {
            ++position;
        } while (position < original.size() && is_digit(original[position]));
    } else {
        invalid_decimal();
    }
    const auto integer_digits = position - integer_begin;

    std::size_t fractional_digits = 0;
    if (position < original.size() && original[position] == '.') {
        const auto fraction_begin = ++position;
        while (position < original.size() && is_digit(original[position])) {
            ++position;
        }
        fractional_digits = position - fraction_begin;
        if (fractional_digits == 0) {
            invalid_decimal();
        }
    }
    const auto mantissa_end = position;

    unsigned exponent_magnitude = 0;
    bool exponent_negative = false;
    if (position < original.size() && (original[position] == 'e' || original[position] == 'E')) {
        ++position;
        if (position < original.size() &&
            (original[position] == '+' || original[position] == '-')) {
            exponent_negative = original[position] == '-';
            ++position;
        }
        const auto exponent_begin = position;
        while (position < original.size() && is_digit(original[position])) {
            // Cap the scan at a rejection sentinel, but finish validating the grammar.
            if (exponent_magnitude <= maximum_decimal_exponent) {
                const auto digit = static_cast<unsigned>(original[position] - '0');
                exponent_magnitude = exponent_magnitude * 10 + digit;
            }
            ++position;
        }
        if (position == exponent_begin) {
            invalid_decimal();
        }
    }
    if (position != original.size()) {
        invalid_decimal();
    }
    // Internal resource failures follow complete syntax validation, even for zero.
    if (integer_digits + fractional_digits > maximum_decimal_digits ||
        exponent_magnitude > maximum_decimal_exponent) {
        throw std::length_error("Decimal exceeds mantissa or exponent limits");
    }

    cpp_int coefficient = 0;
    for (auto index = integer_begin; index < mantissa_end; ++index) {
        if (original[index] != '.') {
            coefficient *= 10;
            coefficient += static_cast<unsigned>(original[index] - '0');
        }
    }
    if (negative) {
        coefficient = -coefficient;
    }
    const auto exponent = exponent_negative ? -static_cast<int>(exponent_magnitude)
                                            : static_cast<int>(exponent_magnitude);
    const auto scale = exponent - static_cast<int>(fractional_digits);
    const auto power_digits = static_cast<unsigned>(scale < 0 ? -scale : scale);
    const cpp_int decimal_power = boost::multiprecision::pow(cpp_int(10), power_digits);
    Exact value(coefficient);
    if (scale < 0) {
        value /= decimal_power;
    } else {
        value *= decimal_power;
    }
    return ExactDecimal(std::string(original), std::move(value));
}

ExactDecimal parse_positive_rate(std::string_view original) {
    auto result = parse_decimal(original);
    if (result.value() <= 0) {
        throw std::invalid_argument("Exchange rate must be positive");
    }
    return result;
}

ExactDecimal parse_nonnegative_factor(std::string_view original) {
    auto result = parse_decimal(original);
    if (result.value() < 0) {
        throw std::invalid_argument("Modifier factor must be nonnegative");
    }
    return result;
}

std::int64_t round_effect_units(const Exact &value) {
    cpp_int magnitude = boost::multiprecision::numerator(value);
    const cpp_int denominator = boost::multiprecision::denominator(value);
    const bool negative = magnitude < 0;
    if (negative) {
        magnitude = -magnitude;
    }
    if ((magnitude != 0 &&
         boost::multiprecision::msb(magnitude) >= maximum_finalizer_component_bits) ||
        boost::multiprecision::msb(denominator) >= maximum_finalizer_component_bits) {
        throw std::length_error("Effect exceeds normalized component limits");
    }

    cpp_int rounded = magnitude / denominator;
    const cpp_int twice_remainder = (magnitude % denominator) * 2;
    if (twice_remainder > denominator || (twice_remainder == denominator && (rounded & 1) != 0)) {
        ++rounded;
    }
    if (negative) {
        rounded = -rounded;
    }
    if (rounded < std::numeric_limits<std::int64_t>::min() ||
        rounded > std::numeric_limits<std::int64_t>::max()) {
        throw std::overflow_error("Rounded effect exceeds signed 64-bit range");
    }
    return rounded.convert_to<std::int64_t>();
}
} // namespace subathon::core
