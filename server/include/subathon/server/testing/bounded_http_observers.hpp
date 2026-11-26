// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/bounded_http.hpp>
namespace trantor::bounded_io {
struct ListenerObservation; // installed SDK type; never included from this header
}
namespace subathon::server::bounded_http::testing {
// Private fixture instrumentation. Never a public host option or an observation setter.
struct Access {
    static Result<Done> arm_first_wire(Foundation &) noexcept; // creating thread, cold, once
    static Result<Done> release_wire(Foundation &) noexcept;   // IO0; actual node only
    // Creating thread, cold, once, before registration is sealed (R4 correction 19).
    // The fixture owns the record and keeps it alive through the joins.
    static Result<Done> arm_listener_observer(Foundation &,
                                              trantor::bounded_io::ListenerObservation &) noexcept;
    // Any thread: total peer records that Foundation::post has visited since create.
    // A static work count for API-033, kept apart from latency; it changes no post
    // outcome or ordering and is only read here.
    static Result<U64> post_peer_visits(Foundation &) noexcept;
    // Any thread: the Foundation's own count of peers with any pending or executing
    // intent (API-033), for comparison with inspect's independent scan.
    static Result<U64> pending_peers(Foundation &) noexcept;
};
} // namespace subathon::server::bounded_http::testing
