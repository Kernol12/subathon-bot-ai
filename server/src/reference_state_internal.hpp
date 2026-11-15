// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "application_state_codec_internal.hpp"
#include "reference_runtime_internal.hpp"
#include <bit>
namespace subathon::server::reference_runtime_detail {
constexpr std::string_view state_format = "subathon.internal.application-state.v1";
struct StateReader {
    std::string_view bytes;
    const ReferenceAdapterLimits &l;
    std::size_t pos = 0;
    std::string_view take(std::size_t n) {
        require(n <= bytes.size() - pos, Error::invalid_document);
        auto s = bytes.substr(pos, n);
        pos += n;
        return s;
    }
    unsigned u8(unsigned max = 255) {
        auto n = static_cast<unsigned char>(take(1)[0]);
        require(n <= max, Error::invalid_document);
        return n;
    }
    std::uint64_t u64() {
        std::uint64_t n = 0;
        for (unsigned char x : take(8))
            n = (n << 8) | x;
        return n;
    }
    std::int64_t i64() { return std::bit_cast<std::int64_t>(u64()); }
    std::size_t count(std::size_t cap) {
        std::uint32_t n = 0;
        for (unsigned char x : take(4))
            n = (n << 8) | x;
        require(n <= cap, Error::result_limit);
        return n;
    }
    std::string_view ref() {
        auto s = take(count(UINT32_MAX));
        require(s.size() <= l.reference_bytes, Error::result_limit);
        require(!s.empty(), Error::invalid_document);
        return s;
    }
    std::string reference() { return std::string(ref()); }
    template <class F> auto opt(F f) -> std::optional<decltype(f())> {
        if (!u8(1))
            return {};
        return f();
    }
    void end() { require(pos == bytes.size(), Error::invalid_document); }
};
inline core::TimerState timer(StateReader &r) {
    return {core::TimerLifecycle(r.u8(2)), core::CountdownPhase(r.u8(1)), r.i64(), r.i64(),
            bool(r.u8(1))};
}
inline void snapshot_input(const ApplicationStateSnapshot &s, const ReferenceAdapterLimits &l) {
    valid_limits(l);
    auto p = measure(l);
    p(s);
    require(unsigned(s.timer.lifecycle) <= 2 && unsigned(s.timer.phase) <= 1, Error::invalid_tag);
    require(!s.token.empty() && !s.configuration_ref.empty() && !s.envelope.format_ref.empty(),
            Error::invalid_reference);
    for (const auto &b : s.balances)
        require(unsigned(b.kind) >= 2 && unsigned(b.kind) <= 3, Error::invalid_tag);
    for (const auto &b : s.balances)
        require(!b.target_ref.empty(), Error::invalid_reference);
    require(!s.envelope.document.isNull(), Error::invalid_shape);
    require(s.timer.main_remaining_ms >= 0 && s.timer.grace_remaining_ms >= 0 &&
                s.time_debt_ms >= 0,
            Error::invalid_value);
}
inline void scan_state(const ApplicationStateSnapshot &s, const ReferenceAdapterLimits &l,
                       bool admit_decoded_result = true) {
    require(s.envelope.format_ref == state_format, Error::unsupported_format);
    StateReader r{{s.envelope.document.constData(), std::size_t(s.envelope.document.size())}, l};
    require(r.take(4) == "SAS1", Error::invalid_document);
    auto config = r.ref();
    auto basis = r.opt([&] { return r.ref(); });
    auto lineage = r.ref(), observation = r.ref();
    auto utc = r.i64();
    auto n = r.count(l.value_items);
    std::int64_t elapsed = 0, final = utc;
    for (std::size_t i = 0; i < n; ++i) {
        auto e = r.i64();
        final = r.i64();
        r.u8(1);
        require(e >= 0 && e <= INT64_MAX - elapsed, Error::invalid_document);
        elapsed += e;
    }
    require(!n || final == utc, Error::invalid_document);
    require(basis || n == 0, Error::invalid_document);
    auto policy = r.ref();
    auto t = timer(r);
    auto debt = r.i64();
    require(t.main_remaining_ms >= 0 && t.grace_remaining_ms >= 0 && debt >= 0,
            Error::invalid_document);
    auto count = r.count(l.value_items);
    require(config == s.configuration_ref &&
                std::tie(t.lifecycle, t.phase, t.main_remaining_ms, t.grace_remaining_ms,
                         t.awaiting_confirmation) ==
                    std::tie(s.timer.lifecycle, s.timer.phase, s.timer.main_remaining_ms,
                             s.timer.grace_remaining_ms, s.timer.awaiting_confirmation) &&
                debt == s.time_debt_ms && count == s.balances.size(),
            Error::invalid_binding);
    for (std::size_t i = 0; i < count; ++i) {
        auto kind = r.u8(3);
        require(kind >= 2, Error::invalid_document);
        auto target = r.ref();
        auto units = r.i64();
        const auto &b = s.balances[i];
        require(unsigned(b.kind) == kind && target == b.target_ref && units == b.units,
                Error::invalid_binding);
        if (i)
            require(unsigned(s.balances[i - 1].kind) < kind ||
                        (s.balances[i - 1].kind == b.kind &&
                         bundle_detail::raw_less(s.balances[i - 1].target_ref, b.target_ref)),
                    Error::invalid_document);
    }
    r.end();
    if (!admit_decoded_result)
        return; // Encoding returns the separately admitted snapshot, not a decoded wrapper.
    auto out = measure(l, Error::result_limit);
    out(s);
    out.p.fixed(1);
    if (basis)
        out(*basis);
    out(lineage, observation, config, policy, basis ? *basis : std::string_view(s.token), utc);
    require(n <= out.p.item_cap - out.p.items, Error::result_limit);
    out.p.items += n;
    out.p.fixed(n * 17);
    out(policy);
}
struct StateWriter {
    std::uint64_t cap, size = 0;
    QByteArray *out = nullptr;
    void raw(std::string_view s) {
        require(s.size() <= cap - size, Error::result_limit);
        size += s.size();
        if (out)
            out->append(s.data(), qsizetype(s.size()));
    }
    void u8(unsigned n) {
        char c = char(n);
        raw({&c, 1});
    }
    void u32(std::size_t n) {
        require(n <= UINT32_MAX, Error::result_limit);
        for (int i = 3; i >= 0; --i)
            u8(unsigned((n >> (i * 8)) & 255));
    }
    void u64(std::uint64_t n) {
        for (int i = 7; i >= 0; --i)
            u8(unsigned((n >> (i * 8)) & 255));
    }
    void i64(std::int64_t n) { u64(std::bit_cast<std::uint64_t>(n)); }
    void b(std::string_view s) {
        u32(s.size());
        raw(s);
    }
    template <class T, class F> void vec(const std::vector<T> &v, F f) {
        u32(v.size());
        for (const auto &x : v)
            f(x);
    }
};
} // namespace subathon::server::reference_runtime_detail
