// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <boost/multiprecision/cpp_int.hpp>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace subathon::core {
using Exact = boost::multiprecision::cpp_rational;

// Local resource policy: docs/contracts/exact-decimal-v1.md, revision 1.
inline constexpr std::size_t maximum_decimal_bytes = 512;
inline constexpr std::size_t maximum_decimal_digits = 256;
inline constexpr unsigned maximum_decimal_exponent = 256;
inline constexpr unsigned maximum_finalizer_component_bits = 4096;

// Owns source spelling and its exact value; only the parser constructs a pair.
class ExactDecimal final {
  public:
    [[nodiscard]] const std::string &original() const noexcept { return original_; }
    [[nodiscard]] const Exact &value() const noexcept { return value_; }

  private:
    ExactDecimal(std::string original, Exact value)
        : original_(std::move(original)), value_(std::move(value)) {}
    friend ExactDecimal parse_decimal(std::string_view);
    std::string original_;
    Exact value_;
};

class ExactNumericNotImplemented final : public std::logic_error {
  public:
    ExactNumericNotImplemented()
        : std::logic_error(
              "Exact numeric behavior is intentionally unimplemented; review tests first") {}
};

// Parse a bounded JSON-number spelling, preserving source precision and spelling.
// Throws invalid_argument for syntax/domain and length_error for resource limits.
[[nodiscard]] ExactDecimal parse_decimal(std::string_view original);
[[nodiscard]] ExactDecimal parse_positive_rate(std::string_view original);
[[nodiscard]] ExactDecimal parse_nonnegative_factor(std::string_view original);

// Input is exact, already-scaled milliseconds or micro-points. Round half-to-even,
// then check int64 range; oversized components throw length_error before rounding.
// Caller owns input review and atomic application when conversion throws.
[[nodiscard]] std::int64_t round_effect_units(const Exact &value);
} // namespace subathon::core
