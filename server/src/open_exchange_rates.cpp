// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/open_exchange_rates.hpp>

namespace subathon::server::open_exchange_rates {

// Stage 1 setup hook. Exercise selection through admission and owner-view assertions.
void Adapter::select() {}

std::variant<Started, Refusal> Adapter::start(const Request &) {
    throw NotImplemented("Open Exchange Rates request admission");
}

Completion Adapter::finish(Reply) {
    throw NotImplemented("Open Exchange Rates response normalization and owner acceptance");
}

void Adapter::observe_exhaustion() {
    throw NotImplemented("Open Exchange Rates owner exhaustion evidence");
}

OwnerView Adapter::view(const Clocks &) const {
    throw NotImplemented("Open Exchange Rates owner view");
}

namespace private_transport {

std::optional<Handoff> make_handoff(const Admission &, const Credential &) {
    throw NotImplemented("Open Exchange Rates private credential handoff");
}

Reply sanitize_reply(const Handoff &, RawFrame) {
    throw NotImplemented("Open Exchange Rates private response sanitization");
}

} // namespace private_transport
} // namespace subathon::server::open_exchange_rates
