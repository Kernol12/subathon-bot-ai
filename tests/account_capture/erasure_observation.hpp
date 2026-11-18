// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "account_capture_observer.hpp" // private include path in root-only build DATA
#include <algorithm>
#include <array>
namespace account_capture_test {
namespace bh = subathon::server::bounded_http;
namespace eo = subathon::server::account_capture::testing;
// Independent oracle, not an eraser/SDK observer implementation. Metadata only;
// no span, pointer-to-payload, digest or raw bytes retained between notifications.
struct ErasureObservation {
    eo::ErasureEvent open{};
    std::size_t size = 0;
    std::array<std::uint16_t, 18> complete{};
    std::array<bool, 18> nonzero{};
    std::uint32_t calls = 0;
    bool in_call = false, failed = false;
    bool reject_before = false, reject_after = false;
    unsigned limit = 8192; // caller metadata capacity, never > ceiling
    static constexpr unsigned ceiling = 8192;
    bool visit(eo::ErasureEvent event, std::span<const std::byte> bytes) noexcept {
        const auto region = static_cast<unsigned>(event.region);
        const auto use = static_cast<unsigned>(event.tag.use);
        constexpr std::array caps{65544U, 1024U, 32768U, 16U, 2048U, 4096U};
        constexpr std::array kinds{bh::RawKind::receive, bh::RawKind::fields, bh::RawKind::fields,
                                   bh::RawKind::method,  bh::RawKind::target, bh::RawKind::body};
        if (failed)
            return false;
        if (region >= 6 || use >= 3 || bytes.empty() || bytes.size() > caps[region] ||
            event.tag.kind != kinds[region] ||
            ((event.tag.use == bh::RawUse::pending) != (event.tag.sequence == 0))) {
            failed = true;
            return false;
        }
        auto index = region * 3 + use;
        if (event.phase == eo::Phase::before_sodium) {
            if (in_call || calls == limit || calls == ceiling || event.call != calls + 1) {
                failed = true;
                return false;
            }
            open = event;
            size = bytes.size();
            in_call = true;
            nonzero[index] = nonzero[index] || std::any_of(bytes.begin(), bytes.end(),
                                                           [](auto b) { return b != std::byte{}; });
        } else if (event.phase == eo::Phase::after_sodium) {
            if (!in_call || event.call != open.call || event.region != open.region ||
                event.tag.kind != open.tag.kind || event.tag.use != open.tag.use ||
                event.tag.sequence != open.tag.sequence ||
                event.tag.peer_context != open.tag.peer_context || bytes.size() != size ||
                !std::all_of(bytes.begin(), bytes.end(), [](auto b) { return b == std::byte{}; })) {
                failed = true;
                return false;
            }
            ++calls;
            ++complete[index];
            in_call = false;
        } else {
            failed = true;
            return false;
        }
        return event.phase == eo::Phase::before_sodium ? !reject_before : !reject_after;
    }
    static bool callback(void *owner, eo::ErasureEvent event,
                         std::span<const std::byte> bytes) noexcept {
        return static_cast<ErasureObservation *>(owner)->visit(event, bytes);
    }
    bool clean() const noexcept { return !failed && !in_call; }
    bool observed(eo::Region r, bh::RawUse use, bool require_nonzero = true) const noexcept {
        const auto index = static_cast<unsigned>(r) * 3 + static_cast<unsigned>(use);
        return complete[index] != 0 && (!require_nonzero || nonzero[index]);
    }
};
static_assert(sizeof(ErasureObservation) <= 192);
} // namespace account_capture_test
