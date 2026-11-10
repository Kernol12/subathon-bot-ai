// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <drogon/WebSocketConnection.h>
#include <limits>
#include <mutex>
#include <new>
#include <subathon/server/native_websocket_profile.hpp>
#include <sys/mman.h>
#include <sys/socket.h>
#include <thread>
#include <trantor/net/EventLoop.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace subathon::server::native_ws {
namespace {
using U = std::uint64_t;
constexpr U maximum = std::numeric_limits<U>::max();
struct Context;
struct Owner;
struct Block {
    U offset{}, size{};
    bool live{};
};
struct Buffer {
    std::shared_ptr<Context> context;
    std::size_t slot{};
    std::byte *data{};
    U capacity{};
    Buffer() = default;
    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;
    Buffer(Buffer &&other) noexcept { swap(other); }
    Buffer &operator=(Buffer &&other) noexcept {
        reset();
        swap(other);
        return *this;
    }
    ~Buffer() { reset(); }
    void swap(Buffer &other) noexcept {
        context.swap(other.context);
        std::swap(slot, other.slot);
        std::swap(data, other.data);
        std::swap(capacity, other.capacity);
    }
    void reset() noexcept;
};

// Fixed, disjoint owner regions followed by equal M-byte assembly/lease slots.
// Offsets and capacities never depend on allocation history. The entire backing
// mapping (including unused regions and page tail) is reserved at startup.
struct Context : std::enable_shared_from_this<Context> {
    const Limits limits;
    const std::shared_ptr<const ElapsedSource> source;
    mutable std::recursive_mutex mutex;
    std::vector<std::shared_ptr<Owner>> owners;
    std::vector<Block> blocks;
    std::vector<unsigned char> regions;
    void *mapping{MAP_FAILED};
    U mapping_size{}, arena_size{};
    GlobalSnapshot global{};
    explicit Context(Limits l, std::shared_ptr<const ElapsedSource> s)
        : limits(l), source(std::move(s)) {}
    ~Context() {
        if (mapping != MAP_FAILED)
            ::munmap(mapping, mapping_size);
    }
    Buffer take(std::size_t slot) noexcept {
        Buffer result;
        auto &b = blocks[slot];
        if (b.live)
            return result;
        b.live = true;
        result.context = shared_from_this();
        result.slot = slot;
        result.capacity = b.size;
        result.data = static_cast<std::byte *>(mapping) + b.offset;
        return result;
    }
    Buffer region(U owner, U kind) noexcept {
        std::lock_guard lock(mutex);
        return take(static_cast<std::size_t>(owner * 3 + kind));
    }
    Buffer message() noexcept {
        std::lock_guard lock(mutex);
        const auto first = blocks.begin() + static_cast<std::ptrdiff_t>(limits.connections * 3);
        const auto free = std::find_if(first, blocks.end(), [](const auto &b) { return !b.live; });
        if (free == blocks.end())
            return {};
        return take(static_cast<std::size_t>(free - blocks.begin()));
    }
    void retire(const std::shared_ptr<Owner> &) noexcept;
};
void Buffer::reset() noexcept {
    if (!context)
        return;
    auto keep = std::move(context);
    std::lock_guard lock(keep->mutex);
    keep->blocks[slot].live = false;
    data = nullptr;
    capacity = 0;
}

bool utf8(const std::byte *p, U size) noexcept {
    U i = 0;
    while (i < size) {
        unsigned a = std::to_integer<unsigned>(p[i++]);
        if (a < 0x80)
            continue;
        unsigned more, scalar;
        if (a >= 0xc2 && a <= 0xdf) {
            more = 1;
            scalar = a & 31;
        } else if (a >= 0xe0 && a <= 0xef) {
            more = 2;
            scalar = a & 15;
        } else if (a >= 0xf0 && a <= 0xf4) {
            more = 3;
            scalar = a & 7;
        } else
            return false;
        if (more > size - i)
            return false;
        unsigned count = more;
        while (count--) {
            unsigned b = std::to_integer<unsigned>(p[i++]);
            if ((b & 0xc0) != 0x80)
                return false;
            scalar = (scalar << 6) | (b & 63);
        }
        if ((more == 1 && scalar < 0x80) || (more == 2 && scalar < 0x800) ||
            (more == 3 && scalar < 0x10000) || scalar > 0x10ffff ||
            (scalar >= 0xd800 && scalar <= 0xdfff))
            return false;
    }
    return true;
}
bool status_code(unsigned code) noexcept {
    return (code >= 1000 && code <= 1003) || (code >= 1007 && code <= 1014) ||
           (code >= 3000 && code <= 4999);
}
constexpr U fixed_scratch = sizeof(std::array<std::byte, 4>) +
                            2 * sizeof(std::array<std::byte, 125>) + sizeof(PeerClose) +
                            3 * sizeof(unsigned);
thread_local Owner *delivery = nullptr;

struct Owner final : drogon::native_profile::Session, std::enable_shared_from_this<Owner> {
    std::shared_ptr<Context> context;
    std::weak_ptr<trantor::TcpConnection> tcp;
    std::weak_ptr<drogon::WebSocketConnection> connection;
    drogon::WebSocketConnection *connection_object{}; // Identity only; never dereferenced.
    trantor::EventLoop *loop;
    std::thread::id thread;
    std::function<void(unsigned)> dispatch;
    Buffer raw, assembly, output, controls;
    U raw_begin{}, raw_end{}, assembly_size{}, message_size{}, message_frames{}, output_size{},
        output_offset{};
    U region_slot{maximum};
    // Streaming Text validation survives payload discard during local closing.
    unsigned text_scalar{}, text_minimum{}, text_remaining{};
    U leases{}, lease_bytes{}, lease_peak{}, bytes_peak{}, work{};
    U ws_read{}, ws_consumed{}, frames{}, last_turn{}, ingress_capacity{};
    ByteCounters peak{};
    U fixed_peak{fixed_scratch};
    bool established{}, upgrade_pending{}, terminal{}, detached_native{}, continuation{}, sent{},
        received{};
    bool error_drain{}, claimed{}, in_dispatch{}, native_callback{};
    unsigned message_opcode{}, opcode{};
    bool active{}, fin{};
    U payload_length{}, payload_offset{};
    std::array<std::byte, 4> mask{};
    std::array<std::byte, 125> control_body{};
    std::array<std::byte, 125> next_pong{};
    U close_size{}, close_offset{}, pong_size{}, pong_offset{};
    U next_pong_size{}; // Encoded length, so an empty pending Pong is distinguishable.
    PeerClose peer{NoCloseFrame{}};
    std::optional<ElapsedInstant> prior, upgrade_deadline, frame_deadline, message_deadline,
        close_deadline;
    trantor::TimerId timer{trantor::InvalidTimerId};
    explicit Owner(std::shared_ptr<Context> c, const trantor::TcpConnectionPtr &t)
        : context(std::move(c)), tcp(t), loop(t->getLoop()), thread(std::this_thread::get_id()) {}
    const Limits &limits() const noexcept { return context->limits; }
    void peaks() noexcept {
        peak.raw_capacity = std::max(peak.raw_capacity, raw.capacity + ingress_capacity);
        peak.assembly_capacity = std::max(peak.assembly_capacity, assembly.capacity);
        peak.application_output = std::max(peak.application_output, output.capacity);
        peak.control_output = std::max(peak.control_output, controls.capacity);
    }
    void force() noexcept {
        if (auto t = tcp.lock())
            t->forceClose();
    }
    std::optional<Failure> guard() noexcept {
        auto value = context->source->sample();
        auto *now = std::get_if<ElapsedInstant>(&value);
        if (!now) {
            force();
            return Failure::elapsed_unavailable;
        }
        if (prior && now->nanoseconds < prior->nanoseconds) {
            force();
            return Failure::elapsed_regression;
        }
        prior = *now;
        for (auto d : {upgrade_deadline, frame_deadline, message_deadline, close_deadline})
            if (d && now->nanoseconds >= d->nanoseconds) {
                force();
                return Failure::deadline_expired;
            }
        return {};
    }
    bool anchor(std::optional<ElapsedInstant> &target, U ms) noexcept {
        if (target)
            return true;
        if (!prior || ms > (maximum - prior->nanoseconds) / 1000000) {
            force();
            return false;
        }
        target = ElapsedInstant{prior->nanoseconds + ms * 1000000};
        return true;
    }
    bool add(U &counter, U n) noexcept {
        if (n > maximum - counter) {
            force();
            return false;
        }
        counter += n;
        return true;
    }
    void tick() noexcept {
        if (!terminal)
            (void)guard();
    }
    void arm() {
        // This wake cannot run between two FIFO callbacks in the same pending batch.
        std::shared_ptr<Queued> record;
        {
            std::lock_guard lock(context->mutex);
            record = std::make_shared<Queued>(shared_from_this());
        }
        timer = loop->runEvery(0.005, [record] { record->owner->tick(); });
    }
    void want_write() noexcept {
        if (auto t = tcp.lock())
            t->requestNativeProfileWrite();
    }
    bool beginUpgrade() noexcept override {
        if (std::this_thread::get_id() != thread || !loop->isInLoopThread()) {
            force();
            return false;
        }
        if (terminal || established || upgrade_pending || guard())
            return false;
        // Leave all HTTP read-ahead in place. Until the synchronous owner-thread
        // handoff, readiness notifications must not read/parse another HTTP chunk.
        upgrade_pending = true;
        return true;
    }
    bool bind(const Connection &c, std::function<void(unsigned)> callback) noexcept override {
        // Never touch the native buffer or elapsed state on an async advice thread.
        if (std::this_thread::get_id() != thread || !loop->isInLoopThread()) {
            force();
            return false;
        }
        if (terminal || established || !upgrade_pending || guard())
            return false;
        try {
            auto transport = tcp.lock();
            if (!transport)
                return false;
            auto *input = transport->getRecvBuffer();
            if (!input->readableBytes())
                input->releaseNativeProfileStorage();
            const U existing = input->nativeProfileCapacity();
            const U raw_size = std::min<U>(4096, limits().raw_capacity_bytes);
            if (existing > limits().raw_capacity_bytes - raw_size)
                return false;
            {
                std::lock_guard lock(context->mutex);
                if (context->global.owners_live_or_retiring >= limits().connections)
                    return false;
                auto slot = std::find(context->regions.begin(), context->regions.end(), 0);
                if (slot == context->regions.end())
                    return false;
                region_slot = static_cast<U>(slot - context->regions.begin());
                *slot = 1; // Held through terminal + leases + queued work retirement.
                ingress_capacity = existing;
                raw = context->region(region_slot, 0);
                controls = context->region(region_slot, 1);
                connection = c;
                connection_object = c.get();
                dispatch = std::move(callback);
                established = true;
                --context->global.upgrade_owners;
                ++context->global.owners_live_or_retiring;
                context->global.owners_peak =
                    std::max(context->global.owners_peak, context->global.owners_live_or_retiring);
                upgrade_deadline.reset();
                peaks();
            }
            transport->establishNativeProfile();
            return true;
        } catch (...) {
            force();
            return false;
        }
    }
    void initial(trantor::MsgBuffer *input) noexcept override {
        if (std::this_thread::get_id() != thread || !loop->isInLoopThread()) {
            force();
            return;
        }
        auto source = tcp.lock();
        if (!source || input != source->getRecvBuffer() || !established || terminal || guard())
            return;
        // Attribute the still-retained Upgrade read allocation during handoff.
        // A reduced profile may refuse read-ahead whose existing capacity cannot fit.
        ingress_capacity = input->nativeProfileCapacity();
        if (ingress_capacity > limits().raw_capacity_bytes - raw.capacity) {
            ingress_capacity = 0;
            input->releaseNativeProfileStorage();
            fail(1009);
            return;
        }
        peaks();
        while (input->readableBytes() && !terminal) {
            if (continuation) {
                fail(1009);
                break;
            }
            if (raw_begin) {
                std::memmove(raw.data, raw.data + raw_begin, raw_end - raw_begin);
                raw_end -= raw_begin;
                raw_begin = 0;
            }
            U n = std::min<U>(input->readableBytes(), raw.capacity - raw_end);
            if (!n) {
                fail(1009);
                break;
            }
            std::memcpy(raw.data + raw_end, input->peek(), n);
            raw_end += n;
            input->retrieve(n);
            if (!add(ws_read, n))
                break;
            turn();
        }
        input->releaseNativeProfileStorage();
        ingress_capacity = 0;
    }
    bool read(int fd) noexcept override {
        if (terminal)
            return true;
        if (guard())
            return true;
        if (!established)
            return upgrade_pending;
        if (continuation)
            return true;
        if (raw_begin) {
            std::memmove(raw.data, raw.data + raw_begin, raw_end - raw_begin);
            raw_end -= raw_begin;
            raw_begin = 0;
        }
        if (raw_end == raw.capacity) {
            fail(1009);
            return true;
        }
        const auto n = ::recv(fd, raw.data + raw_end, raw.capacity - raw_end, 0);
        if (n == 0) {
            force();
            return true;
        }
        if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
                force();
            return true;
        }
        raw_end += static_cast<U>(n);
        if (add(ws_read, n))
            turn();
        return true;
    }
    bool guardWrite() noexcept override { return !terminal && !guard(); }
    bool write_bytes(int fd, const std::byte *p, U size, U &offset) noexcept {
        if (offset == size)
            return true;
        auto n = ::send(fd, p + offset, size - offset, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
                force();
            return false;
        }
        offset += static_cast<U>(n);
        return offset == size;
    }
    void write(int fd) noexcept override {
        if (terminal || guard() || !established)
            return;
        // A single admitted complete data frame precedes control. No encoded
        // copy is ever passed to Trantor's growable MemBufferNode.
        if (output.data) {
            if (!write_bytes(fd, output.data, output_size, output_offset)) {
                want_write();
                return;
            }
            output.reset();
            output_size = output_offset = 0;
        }
        while (pong_size) {
            if (!write_bytes(fd, controls.data + 127, pong_size, pong_offset)) {
                want_write();
                return;
            }
            pong_size = pong_offset = 0;
            if (next_pong_size) {
                // A started frame cannot change. The latest pending Ping uses
                // the same wire slot only after that frame has fully drained.
                auto *out = controls.data + 127;
                out[0] = std::byte{0x8a};
                out[1] = std::byte(next_pong_size - 2);
                if (next_pong_size > 2)
                    std::memcpy(out + 2, next_pong.data(), next_pong_size - 2);
                pong_size = std::exchange(next_pong_size, 0);
            }
        }
        if (close_size) {
            if (!write_bytes(fd, controls.data, close_size, close_offset)) {
                want_write();
                return;
            }
            close_size = close_offset = 0;
        }
        if ((received && sent) || error_drain)
            force();
    }
    void closed() noexcept override {
        std::lock_guard lock(context->mutex);
        terminal = true;
        ingress_capacity = 0;
        if (timer != trantor::InvalidTimerId) {
            loop->invalidateTimer(timer);
            timer = trantor::InvalidTimerId;
        }
        raw.reset();
        assembly.reset();
        output.reset();
        controls.reset();
        raw_begin = raw_end = assembly_size = 0;
        next_pong_size = 0;
        // The dispatch captures only the SDK's weak connection, not a facade or
        // application consumer. Keep its callable intact if close is reentrant.
    }
    void detached() noexcept override {
        {
            std::lock_guard lock(context->mutex);
            detached_native = true;
        }
        // Queue record retirement is accounted separately from socket closure.
        queue_cleanup();
    }
    struct Queued {
        std::shared_ptr<Owner> owner;
        explicit Queued(std::shared_ptr<Owner> p) : owner(std::move(p)) { ++owner->work; }
        ~Queued() {
            auto p = std::move(owner);
            std::lock_guard lock(p->context->mutex);
            --p->work;
            p->context->retire(p);
        }
    };
    void queue_cleanup() noexcept {
        try {
            std::shared_ptr<Queued> record;
            {
                std::lock_guard lock(context->mutex);
                record = std::make_shared<Queued>(shared_from_this());
            }
            loop->queueInLoop([record] {});
        } catch (...) { /* Preserve the owner slot if the native barrier cannot be queued. */
        }
    }
    void queue_continue() noexcept {
        if (continuation || terminal)
            return;
        continuation = true;
        try {
            std::shared_ptr<Queued> record;
            {
                std::lock_guard lock(context->mutex);
                record = std::make_shared<Queued>(shared_from_this());
            }
            loop->queueInLoop([record] {
                auto &p = *record->owner;
                p.continuation = false;
                if (!p.terminal && !p.guard())
                    p.turn();
            });
        } catch (...) {
            continuation = false;
            force();
        }
    }
    void control(unsigned op, const std::byte *p, U n) noexcept {
        if (terminal || !controls.data)
            return;
        if (op == 10) {
            if (received)
                return;
            if (pong_offset) {
                if (n)
                    std::memcpy(next_pong.data(), p, n);
                next_pong_size = n + 2;
                want_write();
                return;
            }
            auto *out = controls.data + 127;
            out[0] = std::byte{0x8a};
            out[1] = std::byte(n);
            if (n)
                std::memcpy(out + 2, p, n);
            pong_size = n + 2;
        } else {
            if (sent)
                return;
            if (!anchor(close_deadline, limits().close_ms))
                return;
            sent = true;
            controls.data[0] = std::byte{0x88};
            controls.data[1] = std::byte(n);
            if (n)
                std::memcpy(controls.data + 2, p, n);
            close_size = n + 2;
        }
        want_write();
    }
    void fail(unsigned code) noexcept {
        assembly.reset();
        assembly_size = 0;
        message_opcode = 0;
        message_deadline.reset();
        frame_deadline.reset();
        error_drain = true;
        std::byte body[]{std::byte(code >> 8), std::byte(code & 255)};
        control(8, body, 2);
        if (!close_size && !output.data)
            force();
    }
    std::optional<Failure> request_close(bool has_status, unsigned code, const std::byte *reason,
                                         U reason_size, CloseDisposition &result) noexcept {
        if (auto error = guard())
            return error;
        if (reason_size > 123 ||
            (has_status && (!status_code(code) || code == 1010 || !utf8(reason, reason_size))))
            return Failure::invalid_input;
        if (sent || received) {
            result = CloseDisposition::already_closing;
            return {};
        }
        if (!anchor(close_deadline, limits().close_ms))
            return Failure::elapsed_unavailable;
        // Retain frame/fragment sequence, aggregate counts, UTF-8 state and all
        // original anchors. Subsequent legal fragments are validated/discarded.
        assembly.reset();
        assembly_size = 0;
        const U n = has_status ? reason_size + 2 : 0;
        sent = true;
        controls.data[0] = std::byte{0x88};
        controls.data[1] = std::byte(n);
        if (has_status) {
            controls.data[2] = std::byte(code >> 8);
            controls.data[3] = std::byte(code & 255);
            if (reason_size)
                std::memcpy(controls.data + 4, reason, reason_size);
        }
        close_size = n + 2;
        close_offset = 0;
        result = CloseDisposition::started;
        want_write();
        return {};
    }
    std::optional<Failure> local_close(const LocalClose &value, CloseDisposition &result) noexcept {
        if (const auto *body = std::get_if<CloseWithStatus>(&value))
            return request_close(true, body->code, body->reason.data(), body->reason_size, result);
        return request_close(false, 0, nullptr, 0, result);
    }
    std::optional<Failure> application_send(unsigned op, const std::byte *p, U n) noexcept {
        if (auto error = guard())
            return error;
        if (sent || received)
            return Failure::closed;
        if (op != 1 && op != 2)
            return Failure::invalid_input;
        const U header = n <= 125 ? 2 : n <= 65535 ? 4 : 10;
        if (n > limits().frame_bytes || n > maximum - header ||
            n + header > limits().application_output_bytes || output.data)
            return Failure::resource_limit;
        if (op == 1 && !utf8(p, n))
            return Failure::invalid_input;
        auto buffer = context->region(region_slot, 2);
        if (!buffer.data)
            return Failure::resource_limit;
        buffer.data[0] = std::byte(0x80 | op);
        if (header == 2)
            buffer.data[1] = std::byte(n);
        else {
            buffer.data[1] = header == 4 ? std::byte{126} : std::byte{127};
            for (U i = 2; i < header; ++i)
                buffer.data[i] = std::byte((n >> (8 * (header - i - 1))) & 255);
        }
        if (n)
            std::memcpy(buffer.data + header, p, n);
        output_size = n + header;
        output_offset = 0;
        output = std::move(buffer);
        peaks();
        want_write();
        return {};
    }
    void send(unsigned op, const char *p, std::size_t n) noexcept override {
        if (std::this_thread::get_id() != thread || terminal || !established)
            return;
        if (op == 1 || op == 2)
            (void)application_send(op, reinterpret_cast<const std::byte *>(p), n);
        else if (op == 8 && n <= 125) {
            if (n == 1)
                return;
            const unsigned code =
                n ? ((static_cast<unsigned char>(p[0]) << 8) | static_cast<unsigned char>(p[1]))
                  : 0;
            CloseDisposition result{};
            (void)request_close(n != 0, code,
                                n ? reinterpret_cast<const std::byte *>(p + 2) : nullptr,
                                n ? n - 2 : 0, result);
        }
        // Legacy Ping/Pong sends cannot replace a response owed to a peer Ping.
    }
    void close(unsigned short code, const std::string &reason) noexcept override {
        if (std::this_thread::get_id() != thread || terminal || !established)
            return;
        CloseDisposition result{};
        (void)request_close(true, code, reinterpret_cast<const std::byte *>(reason.data()),
                            reason.size(), result);
    }
    unsigned octet(U index) const noexcept {
        return std::to_integer<unsigned>(raw.data[raw_begin + index]);
    }
    bool consume(U n) noexcept {
        raw_begin += n;
        return add(ws_consumed, n);
    }
    void invoke(unsigned op) noexcept {
        struct Scope {
            Owner &owner;
            Owner *previous;
            bool previous_callback;
            Scope(Owner &o, unsigned code)
                : owner(o), previous(delivery), previous_callback(o.native_callback) {
                owner.native_callback = true;
                if (code >= 8)
                    delivery = nullptr;
            }
            ~Scope() {
                delivery = previous;
                owner.native_callback = previous_callback;
            }
        } scope(*this, op);
        fixed_peak = std::max<U>(fixed_peak, fixed_scratch + sizeof(std::string));
        try {
            dispatch(op);
        } catch (...) {
            force();
        }
    }
    void deliver() noexcept {
        if (delivery) {
            fail(1009);
            return;
        }
        if (sent || received) {
            assembly.reset();
            assembly_size = 0;
            return;
        }
        if (message_opcode == 1 && !utf8(assembly.data, assembly_size)) {
            fail(1007);
            return;
        }
        struct Scope {
            Owner &owner;
            Owner *previous;
            explicit Scope(Owner &p) : owner(p), previous(delivery) {
                delivery = &owner;
                owner.claimed = false;
                owner.in_dispatch = true;
            }
            ~Scope() {
                owner.in_dispatch = false;
                delivery = previous;
            }
        } scope(*this);
        invoke(message_opcode);
        assembly.reset();
        assembly_size = 0;
    }
    void finish_frame() noexcept {
        if (opcode < 8) {
            if (fin) {
                if (message_opcode == 1 && text_remaining) {
                    fail(1007);
                    return;
                }
                deliver();
                message_size = 0;
                message_opcode = 0;
                message_frames = 0;
                message_deadline.reset();
            }
        } else if (opcode == 9 && !received) {
            control(10, control_body.data(), payload_length);
            if (!terminal)
                invoke(9);
        } else if (opcode == 8) {
            if (payload_length == 1) {
                fail(1002);
                return;
            }
            unsigned code = 0;
            if (payload_length >= 2) {
                code = (std::to_integer<unsigned>(control_body[0]) << 8) |
                       std::to_integer<unsigned>(control_body[1]);
                if (!status_code(code)) {
                    fail(1002);
                    return;
                }
                if (!utf8(control_body.data() + 2, payload_length - 2)) {
                    fail(1007);
                    return;
                }
            }
            if (!received) {
                received = true;
                if (payload_length == 0)
                    peer = CloseWithoutStatus{};
                else {
                    peer.emplace<CloseWithStatus>();
                    auto &body = std::get<CloseWithStatus>(peer);
                    body.code = static_cast<std::uint16_t>(code);
                    body.reason_size = static_cast<std::uint8_t>(payload_length - 2);
                    std::memcpy(body.reason.data(), control_body.data() + 2, body.reason_size);
                }
                assembly.reset();
                assembly_size = message_size = message_frames = 0;
                message_opcode = 0;
                message_deadline.reset();
                if (!anchor(close_deadline, limits().close_ms))
                    return;
                if (!sent)
                    control(8, control_body.data(), code == 1010 ? 0 : payload_length);
                if (!output.data && !close_size)
                    force();
            }
        }
    }
    bool header() noexcept {
        const U available = raw_end - raw_begin;
        if (available < 2)
            return false;
        const unsigned first = octet(0), second = octet(1);
        opcode = first & 15;
        fin = (first & 128) != 0;
        const unsigned short_length = second & 127;
        if ((first & 0x70) || !(second & 128) ||
            (opcode != 0 && opcode != 1 && opcode != 2 && opcode != 8 && opcode != 9 &&
             opcode != 10) ||
            (opcode >= 8 && (!fin || short_length > 125))) {
            fail(1002);
            return false;
        }
        U bytes = short_length <= 125 ? 2 : short_length == 126 ? 4 : 10;
        if (available < bytes)
            return false;
        U length = short_length;
        if (bytes > 2) {
            if (bytes == 10 && (octet(2) & 128)) {
                fail(1002);
                return false;
            }
            length = 0;
            for (U i = 2; i < bytes; ++i)
                length = (length << 8) | octet(i);
            if ((bytes == 4 && length < 126) || (bytes == 10 && length <= 65535)) {
                fail(1002);
                return false;
            }
        }
        if (opcode < 8) {
            if ((opcode == 0 && !message_opcode) || (opcode != 0 && message_opcode)) {
                fail(1002);
                return false;
            }
            if (length > limits().frame_bytes || length > limits().message_bytes - message_size ||
                message_frames >= limits().frames_per_message) {
                fail(1009);
                return false;
            }
        }
        if (available < bytes + 4)
            return false;
        for (U i = 0; i < 4; ++i)
            mask[i] = raw.data[raw_begin + bytes + i];
        if (opcode < 8) {
            if (opcode != 0) {
                message_opcode = opcode;
                message_frames = 0;
                message_size = 0;
                text_scalar = text_minimum = text_remaining = 0;
                if (!sent && !received) {
                    assembly = context->message();
                    if (!assembly.data) {
                        fail(1009);
                        return false;
                    }
                    peaks();
                }
                if (!fin && !anchor(message_deadline, limits().message_ms))
                    return false;
            }
            ++message_frames;
        }
        payload_length = length;
        payload_offset = 0;
        active = true;
        return consume(bytes + 4);
    }
    bool text_byte(unsigned b) noexcept {
        if (text_remaining) {
            if ((b & 0xc0) != 0x80)
                return false;
            text_scalar = (text_scalar << 6) | (b & 63);
            if (--text_remaining == 0 && (text_scalar < text_minimum || text_scalar > 0x10ffff ||
                                          (text_scalar >= 0xd800 && text_scalar <= 0xdfff)))
                return false;
        } else if (b < 0x80) {
            return true;
        } else if (b >= 0xc2 && b <= 0xdf) {
            text_scalar = b & 31;
            text_minimum = 0x80;
            text_remaining = 1;
        } else if (b >= 0xe0 && b <= 0xef) {
            text_scalar = b & 15;
            text_minimum = 0x800;
            text_remaining = 2;
        } else if (b >= 0xf0 && b <= 0xf4) {
            text_scalar = b & 7;
            text_minimum = 0x10000;
            text_remaining = 3;
        } else {
            return false;
        }
        return true;
    }
    void turn() noexcept {
        if (terminal || guard())
            return;
        last_turn = 0;
        while (raw_begin < raw_end && !terminal && !error_drain) {
            if (!anchor(frame_deadline, limits().frame_ms))
                return;
            if (last_turn == limits().frames_per_turn) {
                queue_continue();
                return;
            }
            if (!active && !header())
                break;
            const U n = std::min(raw_end - raw_begin, payload_length - payload_offset);
            for (U i = 0; i < n; ++i) {
                const auto b = raw.data[raw_begin + i] ^ mask[(payload_offset + i) % 4];
                if (opcode < 8 && message_opcode == 1 && !text_byte(std::to_integer<unsigned>(b))) {
                    fail(1007);
                    return;
                }
                if (opcode >= 8)
                    control_body[payload_offset + i] = b;
                else if (!sent && !received)
                    assembly.data[assembly_size + i] = b;
            }
            if (opcode < 8) {
                message_size += n; // Header checked the full frame against M.
                if (!sent && !received)
                    assembly_size += n;
            }
            payload_offset += n;
            if (!consume(n))
                return;
            if (payload_offset != payload_length)
                break;
            active = false;
            frame_deadline.reset();
            if (!add(frames, 1))
                return;
            ++last_turn;
            finish_frame();
        }
        if (raw_begin == raw_end)
            raw_begin = raw_end = 0;
    }
};

void Context::retire(const std::shared_ptr<Owner> &owner) noexcept {
    // All callers hold mutex. No source object survives just for diagnostics.
    if (!owner->terminal || !owner->detached_native || owner->leases || owner->work)
        return;
    for (auto &slot : owners)
        if (slot == owner) {
            if (owner->established) {
                --global.owners_live_or_retiring;
                regions[owner->region_slot] = 0;
            } else
                --global.upgrade_owners;
            slot.reset();
            break;
        }
}

std::optional<Failure> find_owner(const std::shared_ptr<Context> &c, const Connection &connection,
                                  std::shared_ptr<Owner> &result, bool inspect = false) noexcept {
    if (!connection)
        return Failure::null_connection;
    std::lock_guard lock(c->mutex);
    for (const auto &owner : c->owners) {
        if (!owner || !owner->established)
            continue;
        // Compare identity without creating a temporary strong native reference:
        // its last destruction could otherwise invoke shutdown under this mutex.
        if (owner->connection_object == connection.get() &&
            !owner->connection.owner_before(connection) &&
            !connection.owner_before(owner->connection)) {
            result = owner;
            break;
        }
    }
    if (!result)
        return Failure::not_owned;
    if (result->thread != std::this_thread::get_id())
        return Failure::wrong_owner;
    if (!inspect && result->terminal)
        return Failure::closed;
    return {};
}

bool reserve_lease(Owner &p, U n) noexcept {
    auto &g = p.context->global;
    const auto &l = p.limits();
    if (p.terminal || p.leases >= l.callback_leases_per_connection ||
        n > l.callback_bytes_per_connection - p.lease_bytes ||
        n > l.callback_bytes_global - g.callback_payload_bytes)
        return false;
    ++p.leases;
    p.lease_bytes += n;
    ++g.callback_leases;
    g.callback_payload_bytes += n;
    p.lease_peak = std::max(p.lease_peak, p.leases);
    p.bytes_peak = std::max(p.bytes_peak, p.lease_bytes);
    g.callback_leases_peak = std::max(g.callback_leases_peak, g.callback_leases);
    g.callback_payload_bytes_peak =
        std::max(g.callback_payload_bytes_peak, g.callback_payload_bytes);
    return true;
}
void release_lease(const std::shared_ptr<Owner> &p, U n) noexcept {
    std::lock_guard lock(p->context->mutex);
    --p->leases;
    p->lease_bytes -= n;
    --p->context->global.callback_leases;
    p->context->global.callback_payload_bytes -= n;
    p->context->retire(p);
}
} // namespace

struct MessageLease::State {
    std::shared_ptr<Owner> owner;
    Buffer payload;
    U size{};
    MessageKind kind{};
    ~State() {
        if (owner) {
            std::lock_guard lock(owner->context->mutex);
            const auto n = payload.capacity;
            payload.reset();
            release_lease(owner, n);
        }
    }
};
struct Profile::State {
    std::shared_ptr<Context> context;
};
Backend linked_backend() noexcept { return Backend::admitted_corrected; }
MessageLease::MessageLease(std::unique_ptr<State> s) noexcept : state_(std::move(s)) {}
MessageLease::MessageLease(MessageLease &&) noexcept = default;
MessageLease &MessageLease::operator=(MessageLease &&) noexcept = default;
MessageLease::~MessageLease() = default;
std::optional<MessageView> MessageLease::view() const noexcept {
    if (!state_)
        return {};
    return MessageView{state_->kind,
                       {state_->payload.data, static_cast<std::size_t>(state_->size)}};
}
U MessageLease::reserved_payload_bytes() const noexcept {
    return state_ ? state_->payload.capacity : 0;
}
void MessageLease::reset() noexcept { state_.reset(); }
LeaseResult MessageLease::try_copy() const noexcept {
    if (!state_)
        return {Unavailable{Failure::closed}};
    auto owner = state_->owner;
    auto c = owner->context;
    std::lock_guard lock(c->mutex);
    if (owner->terminal)
        return {Unavailable{Failure::closed}};
    const U bound = state_->payload.capacity;
    if (!reserve_lease(*owner, bound))
        return {Unavailable{Failure::resource_limit}};
    try {
        auto copy = std::make_unique<State>();
        copy->payload = c->message();
        if (bound && !copy->payload.data) {
            release_lease(owner, bound);
            return {Unavailable{Failure::resource_limit}};
        }
        copy->owner = owner;
        copy->kind = state_->kind;
        copy->size = state_->size;
        if (copy->size)
            std::memcpy(copy->payload.data, state_->payload.data, copy->size);
        return {MessageLease(std::move(copy))};
    } catch (...) {
        release_lease(owner, bound);
        return {Unavailable{Failure::allocation_failure}};
    }
}
Profile::Profile(std::unique_ptr<State> s) noexcept : state_(std::move(s)) {}
Profile::~Profile() = default;
LeaseResult Profile::accept_callback(const Connection &connection) noexcept {
    std::shared_ptr<Owner> p;
    if (auto e = find_owner(state_->context, connection, p))
        return {Unavailable{*e}};
    if (!delivery)
        return {Unavailable{Failure::outside_delivery}};
    if (delivery != p.get() || !p->in_dispatch)
        return {Unavailable{Failure::delivery_mismatch}};
    if (p->claimed)
        return {Unavailable{Failure::delivery_claimed}};
    if (auto e = p->guard())
        return {Unavailable{*e}};
    if (p->sent || p->received)
        return {Unavailable{Failure::closed}};
    auto c = p->context;
    std::unique_lock lock(c->mutex);
    const U charge = p->assembly.capacity;
    if (!reserve_lease(*p, charge)) {
        lock.unlock();
        p->fail(1009);
        return {Unavailable{Failure::resource_limit}};
    }
    try {
        auto value = std::make_unique<MessageLease::State>();
        value->owner = p;
        value->payload = std::move(p->assembly);
        value->size = p->assembly_size;
        value->kind = p->message_opcode == 1 ? MessageKind::text : MessageKind::binary;
        p->claimed = true;
        return {MessageLease(std::move(value))};
    } catch (...) {
        release_lease(p, charge);
        lock.unlock();
        p->fail(1009);
        return {Unavailable{Failure::allocation_failure}};
    }
}
Result<Admitted> Profile::try_send(const Connection &connection, MessageKind kind,
                                   std::span<const std::byte> bytes) noexcept {
    std::shared_ptr<Owner> p;
    if (auto e = find_owner(state_->context, connection, p))
        return Unavailable{*e};
    const unsigned op = kind == MessageKind::text ? 1 : kind == MessageKind::binary ? 2 : 0;
    if (auto e = p->application_send(op, bytes.data(), bytes.size()))
        return Unavailable{*e};
    return Admitted{};
}
Result<CloseDisposition> Profile::start_close(const Connection &connection,
                                              const LocalClose &body) noexcept {
    std::shared_ptr<Owner> p;
    if (auto e = find_owner(state_->context, connection, p))
        return Unavailable{*e};
    CloseDisposition result{};
    if (auto e = p->local_close(body, result))
        return Unavailable{*e};
    return result;
}
Result<Rechecked> Profile::recheck_deadlines(const Connection &connection) noexcept {
    std::shared_ptr<Owner> p;
    if (auto e = find_owner(state_->context, connection, p))
        return Unavailable{*e};
    if (auto e = p->guard())
        return Unavailable{*e};
    return Rechecked{};
}
Result<Snapshot> Profile::inspect(const Connection &connection) const noexcept {
    std::shared_ptr<Owner> p;
    if (auto e = find_owner(state_->context, connection, p, true))
        return Unavailable{*e};
    std::lock_guard lock(p->context->mutex);
    const auto &g = p->context->global;
    Snapshot s{};
    // Fixed framing/masking/control decoding scratch is not a payload arena extent.
    constexpr U scratch = fixed_scratch;
    s.current = {p->raw.capacity + p->ingress_capacity,
                 p->assembly.capacity,
                 scratch + (p->native_callback ? sizeof(std::string) : 0),
                 p->output.capacity,
                 p->controls.capacity,
                 p->lease_bytes,
                 g.callback_payload_bytes};
    s.peak = p->peak;
    s.peak.fixed_native_storage = p->fixed_peak;
    s.peak.callback_payload_connection = p->bytes_peak;
    s.peak.callback_payload_global = g.callback_payload_bytes_peak;
    s.callback_leases_connection = p->leases;
    s.callback_leases_global = g.callback_leases;
    s.owners_live_or_retiring = g.owners_live_or_retiring;
    s.owners_peak = g.owners_peak;
    s.upgrade_owners = g.upgrade_owners;
    s.upgrade_owners_peak = g.upgrade_owners_peak;
    s.callback_leases_connection_peak = p->lease_peak;
    s.callback_leases_global_peak = g.callback_leases_peak;
    s.ws_bytes_read = p->ws_read;
    s.ws_bytes_consumed = p->ws_consumed;
    s.frames_consumed = p->frames;
    s.frames_in_last_turn = p->last_turn;
    s.continuation_pending = p->continuation;
    s.close_sent = p->sent;
    s.transport_closed = p->terminal;
    s.first_peer_close = p->peer;
    s.frame_deadline = p->frame_deadline;
    s.message_deadline = p->message_deadline;
    s.close_deadline = p->close_deadline;
    return s;
}
Result<GlobalSnapshot> Profile::inspect_global() const noexcept {
    std::lock_guard lock(state_->context->mutex);
    return state_->context->global;
}

struct ProfileFactory {
    static bool installed() noexcept { return bool(drogon::native_profile::Runtime::factory()); }
    static bool started() noexcept {
        return drogon::native_profile::Runtime::started().load(std::memory_order_acquire);
    }
    static void install(drogon::native_profile::Factory factory) {
        drogon::native_profile::Runtime::factory() = std::move(factory);
    }
    static std::unique_ptr<Profile> make(std::shared_ptr<Context> c) {
        auto s = std::make_unique<Profile::State>();
        s->context = std::move(c);
        return std::unique_ptr<Profile>(new Profile(std::move(s)));
    }
};
CreateResult create_profile(const Limits &l, std::shared_ptr<const ElapsedSource> source) noexcept {
    using C = ConfigurationFailure;
    if (ProfileFactory::installed())
        return InvalidConfiguration{C::already_installed};
    if (ProfileFactory::started())
        return InvalidConfiguration{C::native_started};
#if !defined(__linux__) || !defined(__x86_64__)
    return InvalidConfiguration{C::unsupported_platform};
#endif
    if (!source)
        return InvalidConfiguration{C::missing_elapsed_source};
    const U values[]{l.connections,
                     l.incomplete_upgrades,
                     l.frame_bytes,
                     l.message_bytes,
                     l.frames_per_message,
                     l.callback_leases_per_connection,
                     l.callback_bytes_per_connection,
                     l.callback_bytes_global,
                     l.raw_capacity_bytes,
                     l.assembly_capacity_bytes,
                     l.application_output_bytes,
                     l.control_output_bytes,
                     l.frames_per_turn,
                     l.upgrade_ms,
                     l.frame_ms,
                     l.message_ms,
                     l.close_ms};
    for (auto n : values)
        if (!n)
            return InvalidConfiguration{C::zero_limit};
    const U durations[]{l.upgrade_ms, l.frame_ms, l.message_ms, l.close_ms};
    for (auto n : durations)
        if (n > maximum / 1000000)
            return InvalidConfiguration{C::arithmetic_overflow};
    auto add = [](U a, U b, U &result) {
        if (b > maximum - a)
            return false;
        result = a + b;
        return true;
    };
    auto mul = [](U a, U b, U &result) {
        if (b && a > maximum / b)
            return false;
        result = a * b;
        return true;
    };
    U slots, block_count, pool_count, bytes, per_owner, frame_raw, frame_wire, pool_bytes;
    if (!add(l.connections, l.incomplete_upgrades, slots) ||
        !add(l.connections, l.callback_bytes_global / l.message_bytes, pool_count) ||
        !mul(l.connections, 3, block_count) || !add(block_count, pool_count, block_count) ||
        !add(l.frame_bytes, 14, frame_raw) || !add(l.frame_bytes, 10, frame_wire) ||
        !add(std::min<U>(4096, l.raw_capacity_bytes), l.application_output_bytes, per_owner) ||
        !add(per_owner, 254, per_owner) || !mul(per_owner, l.connections, bytes) ||
        !mul(pool_count, l.message_bytes, pool_bytes) || !add(bytes, pool_bytes, bytes) ||
        !mul(l.connections, l.callback_leases_per_connection, per_owner) ||
        slots > static_cast<U>(std::numeric_limits<std::ptrdiff_t>::max()) /
                    sizeof(std::shared_ptr<Owner>) ||
        block_count > static_cast<U>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(Block) ||
        bytes > static_cast<U>(std::numeric_limits<std::ptrdiff_t>::max()))
        return InvalidConfiguration{C::arithmetic_overflow};
    if (l.frame_bytes > l.message_bytes || l.message_bytes > l.assembly_capacity_bytes ||
        l.raw_capacity_bytes < frame_raw || l.application_output_bytes < frame_wire ||
        l.control_output_bytes < 254 || l.callback_bytes_per_connection < l.message_bytes ||
        l.callback_bytes_global < l.callback_bytes_per_connection)
        return InvalidConfiguration{C::inconsistent_limits};
    const long page = ::sysconf(_SC_PAGESIZE);
    if (page <= 0)
        return InvalidConfiguration{C::unsupported_platform};
    U rounded;
    if (!add(bytes, static_cast<U>(page) - 1, rounded))
        return InvalidConfiguration{C::arithmetic_overflow};
    rounded = rounded / static_cast<U>(page) * static_cast<U>(page);
    try {
        auto c = std::make_shared<Context>(l, std::move(source));
        c->owners.resize(slots);
        c->blocks.resize(block_count);
        c->regions.resize(l.connections);
        U offset = 0;
        for (U i = 0; i < l.connections; ++i) {
            for (U kind = 0; kind < 3; ++kind) {
                const U n = kind == 0   ? std::min<U>(4096, l.raw_capacity_bytes)
                            : kind == 1 ? 254
                                        : l.application_output_bytes;
                c->blocks[i * 3 + kind] = {offset, n, false};
                offset += n; // Bounded by the checked total above.
            }
        }
        for (U i = l.connections * 3; i < block_count; ++i) {
            c->blocks[i] = {offset, l.message_bytes, false};
            offset += l.message_bytes;
        }
        c->arena_size = bytes;
        c->mapping_size = rounded;
        c->mapping =
            ::mmap(nullptr, rounded, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (c->mapping == MAP_FAILED)
            return Unavailable{Failure::allocation_failure};
        // Nonallocating placement array-new starts the byte objects' lifetimes;
        // byte alignment needs no per-extent padding or object header.
        ::new (c->mapping) std::byte[static_cast<std::size_t>(bytes)];
        auto profile = ProfileFactory::make(c);
        ProfileFactory::install([c](const trantor::TcpConnectionPtr &tcp)
                                    -> std::shared_ptr<drogon::native_profile::Session> {
            if (!tcp->getLoop()->isInLoopThread() || tcp->isSSLConnection())
                return {};
            try {
                auto p = std::make_shared<Owner>(c, tcp);
                // sample/anchor may force-close; never hold the global lock here.
                if (p->guard() || !p->anchor(p->upgrade_deadline, c->limits.upgrade_ms))
                    return {};
                {
                    std::lock_guard lock(c->mutex);
                    if (c->global.upgrade_owners >= c->limits.incomplete_upgrades)
                        return {};
                    auto slot = std::find(c->owners.begin(), c->owners.end(), nullptr);
                    if (slot == c->owners.end())
                        return {};
                    *slot = p;
                    ++c->global.upgrade_owners;
                    c->global.upgrade_owners_peak =
                        std::max(c->global.upgrade_owners_peak, c->global.upgrade_owners);
                }
                try {
                    p->arm();
                } catch (...) {
                    // Registered owner has no native attachment yet. Remove it
                    // without teardown/callbacks under the registry lock.
                    std::lock_guard lock(c->mutex);
                    for (auto &slot : c->owners)
                        if (slot == p) {
                            slot.reset();
                            --c->global.upgrade_owners;
                            break;
                        }
                    return {};
                }
                return p;
            } catch (...) {
                return {};
            }
        });
        return profile;
    } catch (...) {
        return Unavailable{Failure::allocation_failure};
    }
}
} // namespace subathon::server::native_ws
