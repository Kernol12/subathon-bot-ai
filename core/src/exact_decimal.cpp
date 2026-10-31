// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/exact_decimal.hpp>

namespace subathon::core {
ExactDecimal parse_decimal(std::string_view) { throw ExactNumericNotImplemented{}; }
ExactDecimal parse_positive_rate(std::string_view) { throw ExactNumericNotImplemented{}; }
ExactDecimal parse_nonnegative_factor(std::string_view) { throw ExactNumericNotImplemented{}; }
std::int64_t round_effect_units(const Exact &) { throw ExactNumericNotImplemented{}; }
} // namespace subathon::core
