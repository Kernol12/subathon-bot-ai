// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <limits>
#include <subathon/core/time_credit.hpp>
namespace subathon::core {
TimeCreditResult calculate_time_credit(const TimeCreditInput &input) {
    if (input.remaining_ms < 0 || input.requested_ms < 0 ||
        (input.maximum_remaining_ms &&
         (*input.maximum_remaining_ms < 0 || input.remaining_ms > *input.maximum_remaining_ms))) {
        throw std::invalid_argument(
            "Time credit requires nonnegative inputs and remaining time within the cap");
    }

    auto applied_ms = input.requested_ms;
    if (input.maximum_remaining_ms) {
        // Validation makes headroom representable; limit the actual effect before adding.
        applied_ms = std::min(applied_ms, *input.maximum_remaining_ms - input.remaining_ms);
    }
    if (applied_ms > std::numeric_limits<Milliseconds>::max() - input.remaining_ms) {
        throw std::overflow_error("Time credit exceeds the remaining-time range");
    }
    return {input.requested_ms, applied_ms, input.remaining_ms + applied_ms};
}
} // namespace subathon::core
