// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/account_capture.hpp>
namespace subathon::server::account_capture {
// Explicit missing producer. No NativeIssuer is defined and no State is allocated.
// This file calls no SDK symbol, registers no callback and installs no wipe hook.
struct CaptureLease::State {};
CaptureLease::CaptureLease(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
CaptureLease::CaptureLease(CaptureLease &&) noexcept = default;
CaptureLease &CaptureLease::operator=(CaptureLease &&) noexcept = default;
CaptureLease::~CaptureLease() = default;
Result<Done> CaptureLease::with_capture(void *, void (*)(void *, CaptureView)) const noexcept {
    return NI{};
}
Result<Done> install(bounded_http::Foundation &, Callbacks) noexcept { return NI{}; }
} // namespace subathon::server::account_capture
