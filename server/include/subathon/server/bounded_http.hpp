// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <subathon/server/native_websocket_profile.hpp>
#include <variant>
namespace drogon {
class HttpAppFramework;
}
namespace subathon::server::bounded_http {
// APICONTRACT-005 R3 shapes only. NI source provides no transport producer.
using U64 = std::uint64_t;
using Tick = native_ws::ElapsedInstant;
struct NI {};
struct Done {};
enum class Error {
    invalid_config,
    arithmetic_overflow,
    allocation_failure,
    unavailable,
    wrong_thread,
    reentrant,
    revoked,
    resource_limit,
    expired,
    elapsed_unavailable,
    stopping,
    invalid_state,
    invalid_size,
    already_replied,
    callback_failed,
    incomplete_shutdown,
    wipe_failed
};
template <class T> using Result = std::variant<T, Error, NI>;
enum class Role : std::uint8_t { http, native };
enum class Pause : std::uint8_t { admitted, suspended };
enum class End : std::uint8_t {
    drained,
    disconnected,
    cancelled,
    expired,
    stopped,
    elapsed_failed,
    callback_failed,
    wipe_failed
};
struct Limits {
    U64 peers, http_peers, native_peers, input_slots, output_slots;
    U64 body_bytes, reply_bytes;
    U64 accept_ms, idle_ms, request_ms, output_ms, tick_ms;
};
struct Route {
    std::string_view method, target;
    U64 body_ceiling, header_semantic_ceiling, total_semantic_ceiling;
};
// Zero semantic ceiling disables that optional preflight; never disables B.
struct Field {
    std::uint32_t name, name_size, value, value_size;
};
struct RequestView {
    std::uint32_t route_index;
    std::string_view method, escaped_target;
    std::span<const Field> fields;
    std::span<const std::byte> arena, body;
};
struct ReplySize {
    std::uint16_t status;
    U64 bytes;
    bool close;
};
class Peer;
class Request;
class Reply;
class Wake;
// Each is move-only, with private issued constructor and unique_ptr<State>.
// Every State has class-specific operator new/delete using its reserved slot.
class Peer {
  public:
    Peer(const Peer &) = delete;
    Peer &operator=(const Peer &) = delete;
    Peer(Peer &&) noexcept;
    Peer &operator=(Peer &&) noexcept;
    ~Peer();
    Result<Done> context(void *) noexcept; // once, during connected only
    Result<Wake> wake() noexcept;          // issued at most once
  private:
    struct State;
    std::unique_ptr<State> state_;
    explicit Peer(std::unique_ptr<State>) noexcept;
    friend struct Factory;
};
class Wake {
  public:
    Wake(const Wake &) = delete;
    Wake &operator=(const Wake &) = delete;
    Wake(Wake &&) noexcept;
    Wake &operator=(Wake &&) noexcept;
    ~Wake();

  private:
    struct State;
    std::unique_ptr<State> state_;
    explicit Wake(std::unique_ptr<State>) noexcept;
    friend struct Factory;
};
class Request {
  public:
    Request(const Request &) = delete;
    Request &operator=(const Request &) = delete;
    Request(Request &&) noexcept;
    Request &operator=(Request &&) noexcept;
    ~Request();
    Result<Done> visit(void *, void (*)(void *, RequestView)) const noexcept;
    Result<Reply> reserve_reply(ReplySize) noexcept;
    void release_raw() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
    explicit Request(std::unique_ptr<State>) noexcept;
    friend struct Factory;
};
class Reply {
  public:
    Reply(const Reply &) = delete;
    Reply &operator=(const Reply &) = delete;
    Reply(Reply &&) noexcept;
    Reply &operator=(Reply &&) noexcept;
    ~Reply();
    Result<Done> fill(void *, U64 (*)(void *, std::span<std::byte>)) noexcept;
    Result<Done> commit() noexcept;
    void cancel() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
    explicit Reply(std::unique_ptr<State>) noexcept;
    friend struct Factory;
};
struct NativeBinding {
    std::string_view target;
    void (*opened)(void *, void *peer_context, const native_ws::Connection &);
    void (*message)(void *, void *peer_context, const native_ws::Connection &);
    void (*closed)(void *, void *peer_context, const native_ws::Connection &);
};
struct ReadSample { // at most 64 bytes on the admitted ABI; no payload
    U64 ordinal, bytes_read, head, tail, writable, offered;
    std::int64_t returned;
    std::int32_t error;
    std::uint8_t iovcnt;
};
struct Callbacks {
    void *owner;
    void (*connected)(void *, Peer);
    void (*request)(void *, void *peer_context, Request);
    void (*ready)(void *, void *peer_context); // actual IO0 wake callback
    void (*ended)(void *, void *peer_context, End);
    void (*removed)(void *, void *peer_context);
    void (*stop_drained)(void *); // actual network barrier, before quit
    void (*read_observed)(void *, void *peer_context, ReadSample); // optional, IO0
};
struct Counts {
    U64 peers, http_peers, native_peers, retiring_peers, handles;
    U64 input_slots, output_slots, input_capacity, output_capacity, control_capacity;
    U64 metadata_charge, pending_wakes, transient_fds, constructed, native_retiring;
    U64 sdk_retained_peers;
};
struct Storage {
    U64 payload, external, reserved, mapped, tail, total;
};
// Pure helpers may run on any ordinary thread; neither grants transport authority.
Result<Storage> project_storage(Limits) noexcept;
Result<Done> check_read_window(U64 writable) noexcept; // actual adapter guard
struct Observation {
    Counts current, peak;
    U64 reserved_storage, mapped_storage, reads, one_iovec_reads, bytes_read, requests;
    U64 stock_maps, stock_batches, stock_output_appends, accept_attempts;
    U64 last_read_head, last_read_tail, last_read_writable, last_read_offered;
    std::int64_t last_read_returned;
    std::int32_t last_read_error;
    U64 read_min_writable, max_iovcnt, read_guard_pauses;
    U64 raw_releases, wire_releases, native_retires, generation_exhaustions;
    U64 native_handoff_capacity, native_handoff_accepted, native_projected_fields;
    U64 native_projected_cookies, held_application_wire;
};
enum class Intent : std::uint8_t { owner_ready, admit, suspend, close };
enum class Post : std::uint8_t { queued, coalesced, wake_uncertain };
class Foundation {
  public:
    Foundation(const Foundation &) = delete;
    Foundation &operator=(const Foundation &) = delete;
    Result<Done> admission(const Peer &, Pause) noexcept;     // IO0
    Result<Done> close(const Peer &) noexcept;                // IO0
    Result<Observation> inspect() const noexcept;             // IO0
    Result<Post> post(const Wake &, Intent) noexcept;         // ordinary threads
    Result<Post> stop(Tick first_absolute_deadline) noexcept; // ordinary threads
    Result<Done> finish() noexcept; // creating thread, after framework/IO joins
    ~Foundation();

  private:
    struct State;
    std::unique_ptr<State> state_;
    explicit Foundation(std::unique_ptr<State>) noexcept;
    friend struct Factory;
};
struct Endpoints {
    std::array<char, 46> address;
    std::uint16_t http_port, port;
};
Result<std::unique_ptr<Foundation>> create(Limits, std::span<const Route>, NativeBinding,
                                           std::shared_ptr<const native_ws::ElapsedSource>,
                                           Callbacks) noexcept;
Result<Done> register_listeners(Foundation &, drogon::HttpAppFramework &, Endpoints) noexcept;
enum class RawKind { receive, fields, method, target, body };
enum class RawUse { current, pending, owner_teardown };
struct RawTag {
    void *peer_context;
    U64 sequence;
    RawKind kind;
    RawUse use;
};
struct RawHooks {
    void *owner;
    bool (*wipe)(void *, RawTag, std::span<std::byte>) noexcept;
    void (*revoked)(void *, void *peer_context, U64 sequence, End) noexcept;
};
Result<Done> install_raw_hooks(Foundation &, RawHooks) noexcept;
} // namespace subathon::server::bounded_http
