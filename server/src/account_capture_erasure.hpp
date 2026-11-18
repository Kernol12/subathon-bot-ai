// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "account_capture_observer.hpp"

namespace subathon::server::account_capture::detail {
// Private Foundation-owned metadata, reserved with Core before any reads.
// No pointer/span to raw bytes survives erase(). Not a generic SDK observer.
class SecureErasure {
    testing::ErasureObserver observer_{};
    U64 calls_ = 0;

  public:
    // The actual Foundation installer checks creator thread and cold phase first.
    Result<Done> install(testing::ErasureObserver) noexcept;
    bool erase(bounded_http::RawTag, std::span<std::byte>) noexcept;
};
static_assert(sizeof(SecureErasure) <= 32);
} // namespace subathon::server::account_capture::detail
