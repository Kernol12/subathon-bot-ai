// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
namespace subathon::core {
// Draft boundary: review docs/contracts/time-credit-v0.md before implementation.
using Milliseconds = std::int64_t;
struct TimeCreditInput {
    Milliseconds remaining_ms;
    Milliseconds requested_ms;
    std::optional<Milliseconds> maximum_remaining_ms;
};
struct TimeCreditResult {
    Milliseconds requested_ms;
    Milliseconds applied_ms;
    Milliseconds remaining_after_ms;
};
class FeatureNotImplemented final : public std::logic_error {
public:
    FeatureNotImplemented() : std::logic_error("Time credit is intentionally unimplemented; review tests first") {}
};
[[nodiscard]] TimeCreditResult calculate_time_credit(const TimeCreditInput& input);
} // namespace subathon::core
