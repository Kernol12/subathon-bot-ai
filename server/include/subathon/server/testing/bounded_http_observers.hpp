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
};
} // namespace subathon::server::bounded_http::testing
