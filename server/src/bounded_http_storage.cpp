// SPDX-License-Identifier: GPL-3.0-or-later
#include <limits>
#include <subathon/server/bounded_http.hpp>

namespace subathon::server::bounded_http {
namespace {
// These operations never publish a wrapped intermediate to the storage plan.
bool add(U64 left, U64 right, U64 &out) noexcept {
    if (right > std::numeric_limits<U64>::max() - left)
        return false;
    out = left + right;
    return true;
}
bool multiply(U64 left, U64 right, U64 &out) noexcept {
    if (left != 0 && right > std::numeric_limits<U64>::max() / left)
        return false;
    out = left * right;
    return true;
}
} // namespace

// Pure preflight for the successor allocator and the F11 arithmetic seam.
// Keep the evaluation order specified by R3: scalar checks, L, X, A, page rounding.
Result<Storage> project_storage(Limits limits) noexcept {
    if (!limits.peers || !limits.http_peers || !limits.native_peers || !limits.input_slots ||
        !limits.output_slots || !limits.body_bytes || !limits.reply_bytes || !limits.accept_ms ||
        !limits.idle_ms || !limits.request_ms || !limits.output_ms || !limits.tick_ms ||
        limits.http_peers > limits.peers ||
        limits.native_peers != limits.peers - limits.http_peers ||
        limits.input_slots > limits.peers || limits.output_slots > limits.peers)
        return Error::invalid_config;

    Storage plan{};
    U64 wire = 0, peers = 0, inputs = 0, outputs = 0, slots = 0, slot_metadata = 0;
    if (!add(limits.reply_bytes, 128, wire) || !multiply(limits.peers, 102424, peers) ||
        !multiply(limits.input_slots, limits.body_bytes, inputs) ||
        !multiply(limits.output_slots, wire, outputs) || !add(peers, inputs, plan.payload) ||
        !add(plan.payload, outputs, plan.payload) || !multiply(limits.peers, 24576, peers) ||
        !add(limits.input_slots, limits.output_slots, slots) ||
        !multiply(slots, 128, slot_metadata) || !add(peers, 65536, plan.external) ||
        !add(plan.external, slot_metadata, plan.external) ||
        !add(plan.payload, plan.external, plan.reserved))
        return Error::arithmetic_overflow;

    // Avoid L+4095: that would reject otherwise representable page counts early.
    U64 pages = plan.payload / 4096;
    if (!add(pages, plan.payload % 4096 != 0, pages) || !multiply(pages, 4096, plan.mapped) ||
        !add(plan.mapped, plan.external, plan.total))
        return Error::arithmetic_overflow;
    plan.tail = plan.mapped - plan.payload;
    return plan;
}

// The SDK adapter must call this immediately before each bounded receive syscall.
Result<Done> check_read_window(U64 writable) noexcept {
    if (writable < 8192 || writable > 65536)
        return Error::resource_limit;
    return Done{};
}
} // namespace subathon::server::bounded_http
