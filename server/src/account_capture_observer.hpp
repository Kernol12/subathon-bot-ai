// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// PRIVATE first-party test seam. Never installed/exported as a public SDK header.
#include <subathon/server/account_capture.hpp>
namespace subathon::server::account_capture::testing {
enum class Region : std::uint8_t { receive, field_records, field_arena, method, target, body };
enum class Phase : std::uint8_t { before_sodium, after_sodium };
struct ErasureEvent {
    bounded_http::RawTag tag; // actual private owner; pending has sequence 0
    // One successful observer registration / Foundation lifetime; cumulative over
    // ALL peers, requests, regions and uses. No per-request reset or wrap.
    U64 call; // 1..8192, same actual call on before/after
    // RawTag alone says fields: derive records/arena from Foundation's guaranteed
    // whole spans (1024/32768). Unknown lengths must wipe then fail, never guess.
    Region region;
    Phase phase;
};
struct ErasureObserver {
    void *owner;
    // IO0, noexcept, synchronous borrow only. False/overflow is wipe_failed:
    // quarantine/close, never successful reuse. No payload/pointer retention.
    bool (*visit)(void *, ErasureEvent, std::span<const std::byte>) noexcept;
};
// Creator thread, cold, once, before listener registration, either side of account
// install. Null owner/visit => invalid_config; late/repeated => invalid_state;
// foreign thread => wrong_thread. No allocation or generic RawHooks replacement.
// No callback/extra scan/storage when uninstalled; existing wipe behavior unchanged.
// Observer lifetime extends through actual cleanup/join; no callbacks after finish.
// No ordinal is maintained while uninstalled. Beyond 8192 admitted pairs, the
// actual sodium wipe still runs, but no invalid/reused ordinal is emitted: return
// false through the owned RawHooks boundary and quarantine/refuse clean finish.
// This Foundation-lifetime exhaustion path requires source review; current native
// tests exercise smaller caller capacity failure, not 8193 real ordinal pairs.
Result<Done> install_erasure_observer(bounded_http::Foundation &, ErasureObserver) noexcept;
} // namespace subathon::server::account_capture::testing
