// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <memory>
#include <subathon/server/bounded_http.hpp>

namespace subathon::server::account_capture {
using bounded_http::Done;
using bounded_http::Error;
using bounded_http::NI;
using bounded_http::Result;
using bounded_http::U64;
inline constexpr std::string_view abi = "subathon.ingress-capture.v1";

// Native facts are observations, never installation policy or proof of HTTPS.
// Hashes identify the selected artifact/profile; only the native issuer supplies them.
struct NativeConnectionFacts {
    std::array<std::byte, 32> artifact_sha256, profile_sha256;
    U64 listener, connection, request_sequence;
    std::array<char, 46> peer_address;
    std::uint16_t peer_port;
};
struct CaptureView {
    bounded_http::RequestView request;
    const NativeConnectionFacts &native;
};

// One privately issued actual request; never constructed from a DTO/Request attribute.
// Borrowed bytes last until the enclosing synchronous transport call returns.
// Moves transfer a charge; they do not prolong that call or permit another request.
class CaptureLease {
  public:
    CaptureLease(const CaptureLease &) = delete;
    CaptureLease &operator=(const CaptureLease &) = delete;
    CaptureLease(CaptureLease &&) noexcept;
    CaptureLease &operator=(CaptureLease &&) noexcept;
    ~CaptureLease();
    Result<Done> with_capture(void *, void (*)(void *, CaptureView)) const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
    explicit CaptureLease(std::unique_ptr<State>) noexcept;
    friend struct NativeIssuer;
};

struct Callbacks {
    void *owner;
    // IO0 only, synchronous; callback may visit once, but never queue raw bytes.
    void (*capture)(void *, CaptureLease);
};

// Installs only on the actual cold foundation, before listeners/reads, once.
// Owns the secure RawHooks installation; an existing foreign hook is a conflict.
// Validates the actual artifact/ABI/profile binding, not a client-supplied hash.
// Does not add routes, implement AccountIngressSource, or configure TLS/proxy trust.
Result<Done> install(bounded_http::Foundation &, Callbacks) noexcept;
} // namespace subathon::server::account_capture
