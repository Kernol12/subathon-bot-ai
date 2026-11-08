// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>

namespace drogon {
class WebSocketConnection;
}
namespace subathon::server::native_ws {
using Connection = std::shared_ptr<drogon::WebSocketConnection>;

enum class Backend { stock_upstream, admitted_corrected };
Backend linked_backend() noexcept;
struct Unimplemented {};
enum class Failure {
    null_connection,
    not_owned,
    wrong_owner,
    closed,
    resource_limit,
    allocation_failure,
    allocation_bound_unavailable,
    outside_delivery,
    delivery_mismatch,
    delivery_claimed,
    elapsed_unavailable,
    elapsed_regression,
    deadline_expired,
    invalid_input
};
struct Unavailable {
    Failure reason;
};
template <class T> using Result = std::variant<T, Unimplemented, Unavailable>;

struct Limits {
    std::uint64_t connections, incomplete_upgrades;
    std::uint64_t frame_bytes, message_bytes, frames_per_message;
    std::uint64_t callback_leases_per_connection;
    std::uint64_t callback_bytes_per_connection, callback_bytes_global;
    std::uint64_t raw_capacity_bytes, assembly_capacity_bytes;
    std::uint64_t application_output_bytes, control_output_bytes;
    std::uint64_t frames_per_turn;
    std::uint64_t upgrade_ms, frame_ms, message_ms, close_ms;
};
struct ElapsedInstant {
    std::uint64_t nanoseconds;
};
struct ElapsedUnavailable {};
class ElapsedSource {
  public:
    virtual ~ElapsedSource() = default;
    virtual std::variant<ElapsedInstant, ElapsedUnavailable> sample() const noexcept = 0;
};
enum class ConfigurationFailure {
    zero_limit,
    inconsistent_limits,
    arithmetic_overflow,
    unsupported_platform,
    missing_elapsed_source,
    already_installed,
    native_started
};
struct InvalidConfiguration {
    ConfigurationFailure reason;
};

enum class MessageKind { text, binary };
struct MessageView {
    MessageKind kind;
    std::span<const std::byte> bytes;
};
struct LeaseResult;
class MessageLease {
  public:
    MessageLease(MessageLease &&) noexcept;
    MessageLease &operator=(MessageLease &&) noexcept;
    MessageLease(const MessageLease &) = delete;
    MessageLease &operator=(const MessageLease &) = delete;
    ~MessageLease();
    std::optional<MessageView> view() const noexcept;
    std::uint64_t reserved_payload_bytes() const noexcept;
    LeaseResult try_copy() const noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
    explicit MessageLease(std::unique_ptr<State>) noexcept;
    friend class Profile;
};
struct LeaseResult {
    std::variant<MessageLease, Unimplemented, Unavailable> value;
};

struct NoCloseFrame {};
struct CloseWithoutStatus {};
struct CloseWithStatus {
    std::uint16_t code;
    std::array<std::byte, 123> reason;
    std::uint8_t reason_size;
};
using PeerClose = std::variant<NoCloseFrame, CloseWithoutStatus, CloseWithStatus>;
using LocalClose = std::variant<CloseWithoutStatus, CloseWithStatus>;
struct ByteCounters {
    std::uint64_t raw_capacity, assembly_capacity, fixed_native_storage;
    std::uint64_t application_output, control_output;
    std::uint64_t callback_payload_connection, callback_payload_global;
};
struct GlobalSnapshot {
    std::uint64_t owners_live_or_retiring, owners_peak;
    std::uint64_t upgrade_owners, upgrade_owners_peak;
    std::uint64_t callback_leases, callback_leases_peak;
    std::uint64_t callback_payload_bytes, callback_payload_bytes_peak;
};
struct Snapshot {
    ByteCounters current, peak;
    std::uint64_t callback_leases_connection, callback_leases_global;
    std::uint64_t owners_live_or_retiring, owners_peak;
    std::uint64_t upgrade_owners, upgrade_owners_peak;
    std::uint64_t callback_leases_connection_peak, callback_leases_global_peak;
    std::uint64_t ws_bytes_read, ws_bytes_consumed, frames_consumed;
    std::uint64_t frames_in_last_turn;
    bool continuation_pending, close_sent, transport_closed;
    PeerClose first_peer_close;
    std::optional<ElapsedInstant> frame_deadline, message_deadline, close_deadline;
};
struct Admitted {};
enum class CloseDisposition { started, already_closing };
struct Rechecked {};
class Profile {
  public:
    Profile(const Profile &) = delete;
    Profile &operator=(const Profile &) = delete;
    ~Profile();
    LeaseResult accept_callback(const Connection &) noexcept;
    Result<Admitted> try_send(const Connection &, MessageKind, std::span<const std::byte>) noexcept;
    Result<CloseDisposition> start_close(const Connection &, const LocalClose &) noexcept;
    Result<Rechecked> recheck_deadlines(const Connection &) noexcept;
    Result<Snapshot> inspect(const Connection &) const noexcept;
    Result<GlobalSnapshot> inspect_global() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
    explicit Profile(std::unique_ptr<State>) noexcept;
    friend struct ProfileFactory;
};
using CreateResult =
    std::variant<std::unique_ptr<Profile>, InvalidConfiguration, Unimplemented, Unavailable>;
CreateResult create_profile(const Limits &, std::shared_ptr<const ElapsedSource>) noexcept;
} // namespace subathon::server::native_ws
