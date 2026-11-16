// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/bounded_http.hpp>
#include <subathon/server/testing/bounded_http_observers.hpp>
namespace subathon::server::bounded_http {
// Explicit missing-producer baseline. No Factory definition: no handle can be issued.
// Empty private States only complete unique_ptr destruction; they are never allocated.
struct Peer::State {};
Peer::Peer(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
Peer::~Peer() = default;
Peer::Peer(Peer &&) noexcept = default;
Peer &Peer::operator=(Peer &&) noexcept = default;
struct Wake::State {};
Wake::Wake(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
Wake::~Wake() = default;
Wake::Wake(Wake &&) noexcept = default;
Wake &Wake::operator=(Wake &&) noexcept = default;
struct Request::State {};
Request::Request(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
Request::~Request() = default;
Request::Request(Request &&) noexcept = default;
Request &Request::operator=(Request &&) noexcept = default;
struct Reply::State {};
Reply::Reply(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
Reply::~Reply() = default;
Reply::Reply(Reply &&) noexcept = default;
Reply &Reply::operator=(Reply &&) noexcept = default;
struct Foundation::State {};
Foundation::Foundation(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
Foundation::~Foundation() = default;
Result<Done> Peer::context(void *) noexcept { return NI{}; }
Result<Wake> Peer::wake() noexcept { return NI{}; }
Result<Done> Request::visit(void *, void (*)(void *, RequestView)) const noexcept { return NI{}; }
Result<Reply> Request::reserve_reply(ReplySize) noexcept { return NI{}; }
Result<Done> Reply::fill(void *, U64 (*)(void *, std::span<std::byte>)) noexcept { return NI{}; }
Result<Done> Reply::commit() noexcept { return NI{}; }
Result<Done> Foundation::admission(const Peer &, Pause) noexcept { return NI{}; }
Result<Done> Foundation::close(const Peer &) noexcept { return NI{}; }
Result<Observation> Foundation::inspect() const noexcept { return NI{}; }
Result<Post> Foundation::post(const Wake &, Intent) noexcept { return NI{}; }
Result<Post> Foundation::stop(Tick) noexcept { return NI{}; }
Result<Done> Foundation::finish() noexcept { return NI{}; }
Result<Storage> project_storage(Limits) noexcept { return NI{}; }
Result<Done> check_read_window(U64) noexcept { return NI{}; }
Result<Done> register_listeners(Foundation &, drogon::HttpAppFramework &, Endpoints) noexcept {
    return NI{};
}
Result<Done> install_raw_hooks(Foundation &, RawHooks) noexcept { return NI{}; }
Result<Done> testing::Access::arm_first_wire(Foundation &) noexcept { return NI{}; }
Result<Done> testing::Access::release_wire(Foundation &) noexcept { return NI{}; }
void Request::release_raw() noexcept {} // No issued State or native owner exists in NI.
void Reply::cancel() noexcept {}
Result<std::unique_ptr<Foundation>> create(Limits, std::span<const Route>, NativeBinding,
                                           std::shared_ptr<const native_ws::ElapsedSource>,
                                           Callbacks) noexcept {
    return NI{};
}
} // namespace subathon::server::bounded_http
