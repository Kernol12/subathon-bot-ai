// SPDX-License-Identifier: GPL-3.0-or-later
#include "account_capture_erasure.hpp"
#include <optional>
#include <sodium.h>

namespace subathon::server::account_capture::detail {
namespace {
std::optional<testing::Region> region(bounded_http::RawKind kind, std::size_t size) noexcept {
    using bounded_http::RawKind;
    using testing::Region;
    switch (kind) {
    case RawKind::receive:
        return Region::receive;
    case RawKind::fields:
        // Actual Foundation whole-slot guarantee; no guessed partial attribution.
        if (size == 1024)
            return Region::field_records;
        if (size == 32768)
            return Region::field_arena;
        return std::nullopt;
    case RawKind::method:
        return Region::method;
    case RawKind::target:
        return Region::target;
    case RawKind::body:
        return Region::body;
    }
    return std::nullopt;
}
} // namespace
Result<Done> SecureErasure::install(testing::ErasureObserver observer) noexcept {
    if (!observer.owner || !observer.visit)
        return Error::invalid_config;
    if (observer_.visit)
        return Error::invalid_state;
    observer_ = observer;
    return Done{};
}
bool SecureErasure::erase(bounded_http::RawTag tag, std::span<std::byte> bytes) noexcept {
    const auto selected = region(tag.kind, bytes.size());
    if (!observer_.visit) {
        // Disabled observation: no scan, callback, counter update or allocation.
        sodium_memzero(bytes.data(), bytes.size());
        return selected.has_value();
    }
    if (!selected || calls_ == 8192) {
        // Never emit an invalid attribution or reused/out-of-range ordinal.
        // Exhaustion/invalid span still MUST execute the selected secure erase.
        sodium_memzero(bytes.data(), bytes.size());
        return false;
    }
    const testing::ErasureEvent before{tag, ++calls_, *selected, testing::Phase::before_sodium};
    const bool accepted_before = observer_.visit(observer_.owner, before, bytes);
    sodium_memzero(bytes.data(), bytes.size());
    auto after = before;
    after.phase = testing::Phase::after_sodium;
    const bool accepted_after = observer_.visit(observer_.owner, after, bytes);
    // False cannot skip the erase or the after callback. The owning RawHooks
    // result must propagate this failure to Foundation quarantine/close/finish.
    return accepted_before && accepted_after;
}
} // namespace subathon::server::account_capture::detail
