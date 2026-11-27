// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// SESSION-010 S3 (#893) candidate: binds the #591 capture lane to the reference
// GET path. Bodies are INERT for the tests stage; the implementation stage moves
// this interface to a private server header and aliases it here (S1/S2 precedent).
#include "session_policy.hpp"

#include <array>
#include <memory>
#include <optional>
#include <span>
#include <subathon/server/account_capture.hpp>
#include <variant>

namespace subathon::server::session_ingress_candidate {
namespace bh = bounded_http;
namespace ac = account_capture;

// Reference preflight/primitive/media refusals checked BEFORE credential
// preparation. API-010 owns their public codes (400 and 415); this is private.
enum class ReferenceRefusal : std::uint8_t { invalid_primitive, unsupported_media };
using Outcome = std::variant<AccountTransportResult, ReferenceRefusal>;

// Private same-request provenance. Only the binding issues it; a public request,
// DTO or test cannot construct one that a later serialized check would accept.
// The peer port is ephemeral evidence, never a trust key. `fields` and `projection`
// are bounded nonsecret counts of the scoped request (field occurrences and
// method+target+12+H), carried for evidence reports only.
struct Provenance {
    session_policy::Pin pin; // the snapshot pinned at this request's start fence
    bh::U64 listener, connection, sequence;
    std::array<char, 46> peer_address;
    std::uint16_t peer_port;
    bh::U64 fields, projection;
};
struct Handoff {
    Outcome outcome;
    Provenance provenance;
};

class Binding {
  public:
    // Cold only, on the creator thread before bh::register_listeners. Installs the
    // #591 account capture and a private request-start fence on the same Foundation.
    // `routes` is the exact table given to bh::create; the native route index is
    // interpreted only in it. Returns null if either installation is refused.
    static std::unique_ptr<Binding> install(bh::Foundation &foundation,
                                            std::span<const bh::Route> routes) {
        (void)routes; // INERT: capture installed with a callback that visits nothing
        std::unique_ptr<Binding> binding(new Binding);
        if (!std::holds_alternative<ac::Done>(
                ac::install(foundation, {binding.get(), [](void *, ac::CaptureLease) {}})))
            return {};
        return binding;
    }
    // Any thread. Request starts after return pin `pin`; a request already past its
    // start fence keeps the snapshot it pinned. A null pin closes admission.
    void publish(session_policy::Pin pin) noexcept { (void)pin; }
    // Any thread. No new request start pins a snapshot until the next publish; one
    // that reaches its start fence is refused unavailable or closed safely.
    void suspend() noexcept {}
    // IO0, inside the ordinary request callback, after any reply commit. Retires the
    // raw request, obtains the synchronous cleanup disposition for this live
    // peer/sequence, revokes the scope and returns that request's own handoff. Empty
    // when no scope matched, cleanup failed or the request was cancelled/stopped.
    std::optional<Handoff> release(bh::Request &request) noexcept {
        request.release_raw(); // INERT: retire raw only; no disposition, no handoff
        return {};
    }

    Binding(const Binding &) = delete;
    Binding &operator=(const Binding &) = delete;

  private:
    Binding() = default;
};
} // namespace subathon::server::session_ingress_candidate
