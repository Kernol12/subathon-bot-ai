// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/native_websocket_profile.hpp>

#include <utility>

namespace subathon::server::native_ws {

// Linkable baseline only. No native profile/context or ownership is manufactured.
struct MessageLease::State {};
struct Profile::State {};

Backend linked_backend() noexcept { return Backend::stock_upstream; }

MessageLease::MessageLease(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
MessageLease::MessageLease(MessageLease &&) noexcept = default;
MessageLease &MessageLease::operator=(MessageLease &&) noexcept = default;
MessageLease::~MessageLease() = default;

std::optional<MessageView> MessageLease::view() const noexcept { return std::nullopt; }
std::uint64_t MessageLease::reserved_payload_bytes() const noexcept { return 0; }
LeaseResult MessageLease::try_copy() const noexcept { return {Unimplemented{}}; }
void MessageLease::reset() noexcept { state_.reset(); }

Profile::Profile(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
Profile::~Profile() = default;

LeaseResult Profile::accept_callback(const Connection &) noexcept { return {Unimplemented{}}; }

Result<Admitted> Profile::try_send(const Connection &, MessageKind,
                                   std::span<const std::byte>) noexcept {
    return Unimplemented{};
}

Result<CloseDisposition> Profile::start_close(const Connection &, const LocalClose &) noexcept {
    return Unimplemented{};
}

Result<Rechecked> Profile::recheck_deadlines(const Connection &) noexcept {
    return Unimplemented{};
}
Result<Snapshot> Profile::inspect(const Connection &) const noexcept { return Unimplemented{}; }
Result<GlobalSnapshot> Profile::inspect_global() const noexcept { return Unimplemented{}; }

CreateResult create_profile(const Limits &, std::shared_ptr<const ElapsedSource>) noexcept {
    return Unimplemented{};
}

} // namespace subathon::server::native_ws
