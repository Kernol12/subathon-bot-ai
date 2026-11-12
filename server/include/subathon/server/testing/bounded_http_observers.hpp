// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/bounded_http.hpp>
namespace subathon::server::bounded_http::testing {
// Private fixture instrumentation. Never a public host option or an observation setter.
struct Access {
    static Result<Done> arm_first_wire(Foundation &) noexcept; // creating thread, cold, once
    static Result<Done> release_wire(Foundation &) noexcept;   // IO0; actual node only
};
} // namespace subathon::server::bounded_http::testing
