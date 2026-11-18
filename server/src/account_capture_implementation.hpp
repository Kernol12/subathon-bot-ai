// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Included ONLY after the real Foundation adapter definitions. No installed ABI.
namespace subathon::server::account_capture {
namespace bh = bounded_http;
struct CaptureLease::State : bh::StateRecord {
    bool visited = false, visiting = false;
    State(bh::Core &core, bh::PeerRecord &peer) noexcept
        : StateRecord(core, peer, bh::capture_bit) {}
    static void *operator new(std::size_t size) {
        if (size != sizeof(State))
            throw std::bad_alloc();
        return bh::state_allocate(size, bh::capture_bit);
    }
    static void operator delete(void *) noexcept { bh::state_delete(); }
};
CaptureLease::CaptureLease(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
CaptureLease::CaptureLease(CaptureLease &&) noexcept = default;
CaptureLease &CaptureLease::operator=(CaptureLease &&) noexcept = default;
CaptureLease::~CaptureLease() = default;

Result<Done> CaptureLease::with_capture(void *owner,
                                        void (*visitor)(void *, CaptureView)) const noexcept {
    if (!state_)
        return Error::revoked;
    auto &c = *state_->core;
    // Do not inspect mutable IO-owned peer/visit state from a foreign thread.
    if (!c.is_io())
        return Error::wrong_thread;
    auto &p = *state_->peer;
    if (!p.live || p.incarnation != state_->incarnation || p.sequence != state_->sequence ||
        !p.capture_scope || !p.raw_live || p.removed || p.socket_gone)
        return Error::revoked;
    if (state_->visiting || c.visitors)
        return Error::reentrant;
    if (state_->visited)
        return Error::revoked;
    if (!visitor)
        return Error::invalid_config;
    state_->visited = state_->visiting = true;
    ++c.visitors;
    const bool ok = c.callback([&] {
        visitor(
            owner,
            {{static_cast<std::uint32_t>(p.route),
              {reinterpret_cast<const char *>(p.method()), static_cast<std::size_t>(p.method_size)},
              {reinterpret_cast<const char *>(p.target()), static_cast<std::size_t>(p.target_size)},
              {p.fields(), static_cast<std::size_t>(p.field_count)},
              p.arena().first(static_cast<std::size_t>(p.arena_used)),
              {c.input_data(p.input), static_cast<std::size_t>(p.body_size)}},
             p.capture_facts});
    });
    --c.visitors;
    state_->visiting = false;
    if (!ok) {
        p.capture_failed = true;
        c.terminate(p, bh::End::callback_failed);
        return Error::callback_failed;
    }
    return Done{};
}

struct NativeIssuer {
    // Numeric rendering only: <=15 IPv4 or <=39 IPv6 characters, no libc
    // formatter, locale, allocation, compression, hostname lookup or zone claim.
    static bool format(const bh::io::PeerEndpoint &endpoint, std::array<char, 46> &out) noexcept {
        out.fill(0);
        unsigned used = 0;
        const auto byte = [&](unsigned i) {
            return std::to_integer<unsigned>(endpoint.address[i]);
        };
        if (endpoint.family == 4) {
            for (unsigned i = 0; i != 4; ++i) {
                if (i)
                    out[used++] = '.';
                const auto n = byte(i);
                if (n >= 100)
                    out[used++] = static_cast<char>('0' + n / 100);
                if (n >= 10)
                    out[used++] = static_cast<char>('0' + n / 10 % 10);
                out[used++] = static_cast<char>('0' + n % 10);
            }
            return true;
        }
        if (endpoint.family != 6)
            return false;
        constexpr char digits[] = "0123456789abcdef";
        for (unsigned i = 0; i != 8; ++i) {
            if (i)
                out[used++] = ':';
            const auto word = byte(i * 2) * 256 + byte(i * 2 + 1);
            bool emitted = false;
            for (int shift = 12; shift >= 0; shift -= 4) {
                const auto digit = (word >> shift) & 15;
                if (digit || emitted || shift == 0) {
                    out[used++] = digits[digit];
                    emitted = true;
                }
            }
        }
        return true;
    }
    static bool deliver(bh::Core &c, bh::PeerRecord &p) noexcept {
        static_assert(sizeof(CaptureLease::State) <= 128 && alignof(CaptureLease::State) <= 64);
        if (p.native || p.input == bh::none || !p.raw_live || !p.attached_once || !p.transport ||
            !c.registered || !c.http_listener_id) {
            c.terminate(p, bh::End::callback_failed);
            return false;
        }
        auto &facts = p.capture_facts;
        facts.artifact_sha256 = detail::selected::artifact;
        facts.profile_sha256 = detail::selected::profile;
        // Identifier assigned at actual successful HTTP listener registration.
        // Not a TCP port, client value, globally unique boot ID or policy identity.
        facts.listener = c.http_listener_id;
        facts.request_sequence = p.sequence;
        facts.peer_port = p.endpoint.port;
        if (!format(p.endpoint, facts.peer_address)) {
            c.terminate(p, bh::End::cancelled);
            return false;
        }
        if (p.states.fetch_or(bh::capture_bit, std::memory_order_acq_rel) & bh::capture_bit) {
            c.terminate(p, bh::End::cancelled); // retained older lease pins its finite slot
            return false;
        }
        bh::Descriptor descriptor{&c, &p, bh::capture_bit, p.handles[4]};
        bh::allocation = &descriptor;
        try {
            std::unique_ptr<CaptureLease::State> state(new CaptureLease::State(c, p));
            bh::allocation = nullptr;
            c.handle_count.fetch_add(1, std::memory_order_release);
            c.peaks();
            p.capture_scope = true;
            const bool ok = c.callback([&] {
                c.account_callbacks.capture(c.account_callbacks.owner,
                                            CaptureLease(std::move(state)));
            });
            // Revoke even an unvisited/retained handle BEFORE route dispatch/release.
            p.capture_scope = false;
            if (!ok || p.capture_failed) {
                c.terminate(p, bh::End::callback_failed);
                return false;
            }
            return !p.close_requested;
        } catch (...) {
            bh::allocation = nullptr;
            p.capture_scope = false;
            p.states.fetch_and(~bh::capture_bit, std::memory_order_release);
            c.terminate(p, bh::End::callback_failed);
            return false;
        }
    }
};
namespace {
bool wipe(void *owner, bh::RawTag tag, std::span<std::byte> bytes) noexcept {
    return static_cast<bh::Core *>(owner)->account_erasure.erase(tag, bytes);
}
void revoked(void *, void *, U64, bh::End) noexcept {
    // Core::release_raw cannot run inside capture_scope; deliver has already
    // revoked all leases by clearing that scope before any owner release.
}
} // namespace
Result<Done> install(bh::Foundation &foundation, Callbacks callbacks) noexcept {
    auto &c = bh::Factory::core(foundation);
    if (std::this_thread::get_id() != c.creator)
        return Error::wrong_thread;
    if (!callbacks.owner || !callbacks.capture)
        return Error::invalid_config;
    if (c.registered || c.hook_installed || c.account_callbacks.capture)
        return Error::invalid_state;
    const auto *registration = bh::sdk::Runtime::native_registration();
    if (!registration || registration->source_identity() != c.source.get() ||
        std::string_view(DROGON_SUBATHON_BOUNDED_HTTP_V1) != "subathon.bounded-http-foundation.v1")
        return Error::unavailable;
    // The compiled hash binding is not an authority token: actual DSO identity
    // must also pass independent per-consumer admission before any execution.
    const auto hooks = bh::install_raw_hooks(foundation, {&c, wipe, revoked});
    if (!std::holds_alternative<Done>(hooks))
        return hooks;
    c.account_callbacks = callbacks; // no fallible work after claiming the one slot
    return Done{};
}
} // namespace subathon::server::account_capture

namespace subathon::server::bounded_http {
namespace {
bool capture_request(Core &c, PeerRecord &p) noexcept {
    return account_capture::NativeIssuer::deliver(c, p);
}
} // namespace
} // namespace subathon::server::bounded_http
