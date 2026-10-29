// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/time_credit.hpp>
#include <type_traits>
static_assert(sizeof(subathon::core::Milliseconds) == 8);
static_assert(std::is_signed_v<subathon::core::Milliseconds>);
int main() {
    // Infrastructure-only check: the public boundary is usable. Does not claim
    // that any credit behavior works, and stays valid after implementation.
    const subathon::core::TimeCreditInput input{0, 0, std::nullopt};
    return input.remaining_ms == 0 && !input.maximum_remaining_ms.has_value() ? 0 : 1;
}
