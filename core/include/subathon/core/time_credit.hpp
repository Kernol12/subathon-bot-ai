// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
namespace subathon::core {
// Contract revision 1: docs/contracts/time-credit-v0.md; review gate remains separate.
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
// Plans nonnegative main-timer credit after eligibility/debt/deadline handling.
// Throws std::invalid_argument for negatives or remaining time above a supplied cap;
// throws std::overflow_error if the final remaining time is not representable.
// The application must hold overflow for review without committing any effects.
[[nodiscard]] TimeCreditResult calculate_time_credit(const TimeCreditInput& input);
} // namespace subathon::core
