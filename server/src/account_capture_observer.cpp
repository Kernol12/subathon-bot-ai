// SPDX-License-Identifier: GPL-3.0-or-later
#include "account_capture_observer.hpp"
namespace subathon::server::account_capture::testing {
// Explicit NI: installs nothing, holds no observer/payload and calls no SDK/sodium.
// Future dispatch belongs only around the account-owned actual sodium_memzero call.
Result<Done> install_erasure_observer(bounded_http::Foundation &, ErasureObserver) noexcept {
    return NI{};
}
} // namespace subathon::server::account_capture::testing
