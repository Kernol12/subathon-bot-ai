# Bounded selected HTTP foundation — v1, correction R3

APICONTRACT-005; identity `subathon.bounded-http-foundation.v1`. Normative
**proposal**, not implemented or admitted. R2 addressed initial-review findings
CI005-B1–B6 and A1–A8. Independent Codex verification retained B1's native-wrapper
lifetime gap and B6's read-witness gap. R3 addresses those and its thread/backpressure
advisories; separate re-verification and Claude closing remain required. Author base
`e0d81ead9fdba8d130fb8ad52269a9cb995be4e7`; original source was integrated as
`275f48a8c1b391772a5153165bb6289cc95dd9eb`. Authority is specification v 0.45,
§2, SERVER-01, API-01, AUTH-12 and §§17–18. Import the complete
[listener contract](api-host-listeners-v1.md),
[host lifecycle](api-host-lifecycle-v1.md),
[consumer boundary](api-host-lifecycle-boundary-v1.md),
[native profile](native-websocket-profile-v1.md),
[account ingress](account-ingress-v1.md),
[reference HTTP](reference-http-v1.md) and
[ADR-0011](../decisions/0011-ingress-boundaries.md), including accepted
SESSIONCONTRACT-005/006 closures. This proposal freezes the generic producer
before APITEST-016 authors tests; it implements no account/authentication feature.

## 1. Selected implementation boundary

Keep one process/framework lifetime, two internal plaintext listeners, one IO
thread (IO0), the same host authority and one actual shared
`native_ws::ElapsedSource`. Extend the existing Drogon parser/state machine and
Trantor channel/connection; no alternate parser/library/server/proxy/auth service.
Exactly one start attempt is permitted. Historical API-012 candidate4 source,
public native header, patches, manifest/pins/artifact and all 44 compatibility
methods remain byte-identical. A separately admitted additive successor is needed.
Legacy unselected listeners retain candidate4 behavior; the same 44 methods must
pass unchanged against the successor with fresh consumer admission.

Source observations below use the parent's read-only
`/home/archnoob/ProgrammingProjects/SubathonProject/subathon-bot-ai/build/deps/`
`drogon-1.9.14-api012-candidate4-source` and matching `-sdk/include`.
Drogon is `3aae769cc405cf5abb9d477675c6430924f02c7c`, Trantor is
`78e480f5e138acdc05f76c55a2716718cd846364`, followed by the exact
[historical patches](../../dependencies/native-websocket-profile/manifest.json).
Reading these sources is neither loaded-artifact identity nor runtime proof.
The original manifest's candidate status is historical; API-012's admission is
preserved. New filenames/ABI markers below are proposed, with no existing hashes.

| Actual source producer | Required successor-only seam |
| --- | --- |
| Drogon `lib/inc/drogon/HttpAppFramework.h`, `lib/src/HttpAppFrameworkImpl.{h,cc}`, `ListenerManager.{h,cc}`, `HttpServer.{h,cc}` | Carry a private SDK producer binding and explicit role through both ListenerInfo construction paths. Register the same two addListener instances, nativeProfile=true only for native. Reject TLS, extra IO loops, missing installed profile, duplicate foundation/role or mismatched actual elapsed source before bind. Never classify a connection by a header/path. |
| Trantor `trantor/net/inner/Acceptor.{h,cc}::readCallback`, `trantor/net/TcpServer.{h,cc}::newConnection` | Claim transient FD token T before accept, then role/peer capacity before any connection/channel/socket/name/parser construction. Refused accepted FDs use stack RAII only. Replace bounded-mode connSet with a fixed peer registry. |
| `trantor/net/inner/TcpConnectionImpl.{h,cc}` constructor/readCallback/connectEstablished/handleClose/connectDestroyed and `trantor/utils/MsgBuffer.{h,cc}` | Construct fixed receive storage directly, not a 2048-byte vector followed by replacement. Choose bounded pre-Upgrade read after native Session::read returns false. Add pre-read guard, fixed index/compaction/wipe methods and real channel suspension. Preserve established native reads. |
| Drogon `lib/src/HttpRequestParser.{h,cc}::reset/processRequestLine/parseRequest`, `HttpServer::onMessage/onRequests` | Add bounded sink operations to this same parser. Scan/preflight before setPath/setQuery/addHeader/body reserve. Bounded onConnection bypasses stock HttpConnectionLimit and its per-IP map/fetch-add off-by-one; the private P registry is its sole SDK connection quota. Ordinary bounded requests construct no HttpRequestImpl/map/pool/batch/pipeline entry. Stop before constructing a second request. |
| `HttpRequestImpl.{h,cc}`, `HttpControllersRouter.cc`, `HttpControllerBinder.cc`, `WebSocketConnectionImpl.{h,cc}` | Native only: construct the finite sanitized projection in §6 after validation. Reuse selected synchronous handshake, beginUpgrade/bind/initial and native framing. No generic filters, regex routing, JSON, sessions, async controller or retained projected request. |
| `HttpResponseImpl.{h,cc}::renderToBuffer`, `HttpServer::sendResponse/sendResponses` and parser/refusal/error branches | Add a fixed-destination renderer for §5; no stock render/cache/merge/default/error/100 fallback. Reservation precedes any response construction/render. |
| Trantor `TcpConnectionImpl::send*/sendInLoop/writeCallback`, `BufferNode` | Reference one charged wire slot with one fixed node. No append/list-node/string/copy/stream/file/async send; guard every bounded-connection send entry before copying. High-water advice remains post-append and is not enforcement. |
| `HttpRequestParser` construction, `TcpConnectionImpl::name_`, `TcpServer::stop/handleCloseInLoop`, `EventLoop`, `EpollPoller` | Eliminate eager deque/send-buffer allocation on the bounded branch; fixed name formatting, deferred-removal records and owner mailbox replace allocating concatenation, connSet copy and queueInLoop closures. Fixed poller/event/active-channel storage and a fixed debug registry replace vector doubling/map insertion. |
| First-party `server/src/native_websocket_profile.cpp::Context/ProfileFactory/Context::retire`, in a successor build copy only | Export immutable SDK-native limits and actual retained source identity/sample interface from the installed factory. At actual terminal+detach+zero leases+zero work retirement, latch the registered peer's retirement bit. Never signal retirement at disconnected advice alone. Original source/header remain unchanged; apply a separately reviewed pinned build-copy patch, compiled instead of the original, never alongside it. |

The corrections are one coherent producer, not new prerequisite projects. Their
feasibility and exact allocation sites still need source/ABI review and tests.

## 2. SDK direction and first-party API

The candidate4 direction is retained: SDK-defined abstract interfaces implemented
by first-party code. Proposed SDK headers are
`trantor/net/BoundedIo.h` and `drogon/BoundedHttp.h`. They include only standard
and SDK headers; neither DSO includes or links first-party Foundation, Result,
ElapsedSource or callback types. The following private ABI shapes are normative;
SDK pointer cookies are usable only by their installed producer/connection.

```cpp
namespace trantor::bounded_io {
using U64 = std::uint64_t;
enum class Role : std::uint8_t { http, native };
struct Sample { bool available; U64 ns; };
struct ReadWitness { U64 head, tail, writable, offered;
                     std::int64_t returned; std::int32_t error;
                     std::uint8_t iovcnt; };
class Transport { // SDK implementation; IO0 only, actual accepted connection
public:
    virtual ~Transport() = default;
    virtual void reading(bool) noexcept = 0;
    virtual bool wire(std::span<const std::byte>, void* release_cookie) noexcept = 0;
    virtual void close() noexcept = 0;
};
class PeerStorage { // first-party implementation; SDK holds through removal
public:
    virtual ~PeerStorage() = default;
    virtual std::span<std::byte> receive_with_prefix() noexcept = 0;
    virtual void attached(Transport&) noexcept = 0;
    virtual void read_witness(ReadWitness) noexcept = 0;
    virtual void wire_released(void* release_cookie) noexcept = 0;
    virtual void socket_removed() noexcept = 0;
    virtual void native_retired() noexcept = 0; // latch/wake only, any thread
    virtual void sdk_objects_released() noexcept = 0; // actual SDK deletion fence
};
class Producer { // first-party implementation, installed before run
public:
    virtual ~Producer() = default;
    virtual Sample sample() noexcept = 0;
    virtual bool transient(bool acquire) noexcept = 0;
    virtual PeerStorage* acquire(Role, Sample accepted) noexcept = 0;
    virtual bool before_read(PeerStorage&, ReadWitness) noexcept = 0;
    virtual void owner_turn() noexcept = 0;
};
}
namespace drogon::bounded_http_sdk {
using U64 = std::uint64_t;
struct Field { std::uint32_t name, name_size, value, value_size; };
struct Head { std::span<const std::byte> raw;
              std::string_view method, target;
              std::span<const Field> fields; bool syntax_bad;
              U64 raw_bytes, field_count, semantic_bytes, max_field_bytes, line_bytes; };
struct Decision { std::uint16_t status; U64 body_bytes; }; // status=0 admits
class Sink { // first-party implementation; existing parser invokes on IO0
public:
    virtual ~Sink() = default;
    virtual Decision head(trantor::bounded_io::PeerStorage&, Head) noexcept = 0;
    virtual bool body(trantor::bounded_io::PeerStorage&,
                      std::span<const std::byte>) noexcept = 0;
    virtual void complete(trantor::bounded_io::PeerStorage&) noexcept = 0;
};
struct Endpoint { std::array<char,46> address; std::uint16_t port; };
class Runtime { // SDK implementation, private installation, one per process
public:
    static bool install(trantor::bounded_io::Producer&, Sink&, U64 peers) noexcept;
    static bool listener(HttpAppFramework&, Endpoint,
                         trantor::bounded_io::Role) noexcept;
    // Private fixture instrumentation; absent from the NI target's dependencies.
    static bool arm_first_wire() noexcept;
    static bool release_wire() noexcept;
};
}
```

Head.fields offsets refer to Head.raw (not a copied arena), after bounded scan;
invalid records carry syntax_bad and never enter a request callback. The raw
span is at most 40961 bytes at an overage decision; field_count/S may exceed their
ceilings but fields contains at most 64 offset records. Counters do not authorize
copying an excess record. The first-party
sink performs §4's priority checks, then copies into its fixed arena/body slots.
SDK Transport::wire borrows a reserved immutable span until wire_released, once;
a cookie comes from an actual fixed slot and cannot manufacture response authority.
The SDK enforces one node, closed-state checks and no bypass send. Constructor,
actual socket, parser and callback ownership remain in their source producers;
first-party acquire claims their slots before SDK construction, and attached
privately binds the actual Transport before issuing a Peer.

The successor native ABI additionally defines SDK-only NativeLimits containing
all 17 immutable native Limits fields (connections, incomplete_upgrades, frame,
message, frames/message, callback leases, per-peer/global callback bytes, raw,
assembly, application/control output, frames/turn and four durations). Its private
NativeRegistration interface supplies `const NativeLimits& limits() const`,
`const void* source_identity() const`, `Sample sample() noexcept` and
`bool attach_retirement(Session&, PeerStorage&) noexcept`. The installed real ProfileFactory implements
and privately registers this once alongside its actual Session factory; no public DTO setter
installs facts. Foundation's adapter compares source_identity to its retained
source.get(), checks limits/coherence, and samples that SAME retained source.
The SDK does not cast/dereference the opaque identity or call first-party symbols.

attach_retirement validates the actual Session belongs to that installed Context
before retaining the PeerStorage binding, otherwise false/close. If the real factory returns no Session, no native owner was attached to this
peer; normal socket cleanup needs no manufactured retirement event. Native
transient-factory costs remain under the frozen one-per-IO allowance. The binding is
held until native_retired is consumed, not a disconnected hint. No observer can
register a different Session/context by matching DTOs.

Context::retire currently tests terminal, detached_native, leases and work under
its mutex before clearing the owner slot. The build-copy patch latches a
pre-registered PeerStorage::native_retired atomic bit at that exact point, before clearing the
binding; the notification holds an in-flight binding pin until its last core
access, and peer reuse/stop_drained waits for that pin to leave; no user callback runs under the native mutex. A foreign-thread last lease
release can latch/wake only. IO0 later consumes the bit and releases native
membership. No unchanged API-012 snapshot/data value is reinterpreted as a permit.

Proposed first-party header `server/include/subathon/server/bounded_http.hpp`,
namespace `subathon::server::bounded_http`, uses standard C++20 and the unchanged
public native header; it does not require new SDK headers to compile. Include array/span/string_view/
variant/memory/cstdint and forward-declare drogon::HttpAppFramework; NI definitions
need no new SDK type or symbol.

```cpp
using U64 = std::uint64_t;
using Tick = native_ws::ElapsedInstant;
struct NI {}; struct Done {};
enum class Error { invalid_config, arithmetic_overflow, allocation_failure,
    unavailable, wrong_thread, reentrant, revoked, resource_limit, expired,
    elapsed_unavailable, stopping, invalid_state, invalid_size, already_replied,
    callback_failed, incomplete_shutdown, wipe_failed };
template<class T> using Result = std::variant<T, Error, NI>;
enum class Role : std::uint8_t { http, native };
enum class Pause : std::uint8_t { admitted, suspended };
enum class End : std::uint8_t { drained, disconnected, cancelled, expired,
    stopped, elapsed_failed, callback_failed, wipe_failed };
struct Limits {
    U64 peers, http_peers, native_peers, input_slots, output_slots;
    U64 body_bytes, reply_bytes;
    U64 accept_ms, idle_ms, request_ms, output_ms, tick_ms;
};
struct Route { std::string_view method, target;
    U64 body_ceiling, header_semantic_ceiling, total_semantic_ceiling; };
// Zero semantic ceiling disables that optional preflight; never disables B.
struct Field { std::uint32_t name, name_size, value, value_size; };
struct RequestView { std::uint32_t route_index;
    std::string_view method, escaped_target;
    std::span<const Field> fields;
    std::span<const std::byte> arena, body; };
struct ReplySize { std::uint16_t status; U64 bytes; bool close; };
class Peer; class Request; class Reply; class Wake;
// Each is move-only, with private issued constructor and unique_ptr<State>.
// Every State has class-specific operator new/delete using its reserved slot.
class Peer {
public:
    Peer(Peer&&) noexcept; Peer& operator=(Peer&&) noexcept; ~Peer();
    Result<Done> context(void*) noexcept; // once, during connected only
    Result<Wake> wake() noexcept;        // issued at most once
private: struct State; std::unique_ptr<State> state_;
    explicit Peer(std::unique_ptr<State>) noexcept;
    friend struct Factory;
};
class Wake {
public:
    Wake(Wake&&) noexcept; Wake& operator=(Wake&&) noexcept; ~Wake();
private: struct State; std::unique_ptr<State> state_;
    explicit Wake(std::unique_ptr<State>) noexcept;
    friend struct Factory;
};
class Request {
public:
    Request(Request&&) noexcept; Request& operator=(Request&&) noexcept; ~Request();
    Result<Done> visit(void*, void (*)(void*, RequestView)) const noexcept;
    Result<Reply> reserve_reply(ReplySize) noexcept;
    void release_raw() noexcept;
private: struct State; std::unique_ptr<State> state_;
    explicit Request(std::unique_ptr<State>) noexcept;
    friend struct Factory;
};
class Reply {
public:
    Reply(Reply&&) noexcept; Reply& operator=(Reply&&) noexcept; ~Reply();
    Result<Done> fill(void*, U64 (*)(void*, std::span<std::byte>)) noexcept;
    Result<Done> commit() noexcept;
    void cancel() noexcept;
private: struct State; std::unique_ptr<State> state_;
    explicit Reply(std::unique_ptr<State>) noexcept;
    friend struct Factory;
};
struct NativeBinding { std::string_view target;
    void (*opened)(void*, void* peer_context, const native_ws::Connection&);
    void (*message)(void*, void* peer_context, const native_ws::Connection&);
    void (*closed)(void*, void* peer_context, const native_ws::Connection&); };
struct ReadSample { // at most 64 bytes on the admitted ABI; no payload
    U64 ordinal, bytes_read, head, tail, writable, offered;
    std::int64_t returned; std::int32_t error; std::uint8_t iovcnt;
};
struct Callbacks {
    void* owner;
    void (*connected)(void*, Peer);
    void (*request)(void*, void* peer_context, Request);
    void (*ready)(void*, void* peer_context); // actual IO0 wake callback
    void (*ended)(void*, void* peer_context, End);
    void (*removed)(void*, void* peer_context);
    void (*stop_drained)(void*); // actual network barrier, before quit
    void (*read_observed)(void*, void* peer_context, ReadSample); // optional, IO0
};
struct Counts { U64 peers, http_peers, native_peers, retiring_peers, handles;
    U64 input_slots, output_slots, input_capacity, output_capacity, control_capacity;
    U64 metadata_charge, pending_wakes, transient_fds, constructed, native_retiring;
    U64 sdk_retained_peers; };
struct Storage { U64 payload, external, reserved, mapped, tail, total; };
Result<Storage> project_storage(Limits) noexcept; // pure sizes, no authority
Result<Done> check_read_window(U64 writable) noexcept; // actual adapter guard
struct Observation { Counts current, peak;
    U64 reserved_storage, mapped_storage, reads, one_iovec_reads, bytes_read, requests;
    U64 stock_maps, stock_batches, stock_output_appends, accept_attempts;
    U64 last_read_head, last_read_tail, last_read_writable, last_read_offered;
    std::int64_t last_read_returned; std::int32_t last_read_error;
    U64 read_min_writable, max_iovcnt, read_guard_pauses;
    U64 raw_releases, wire_releases, native_retires, generation_exhaustions;
    U64 native_handoff_capacity, native_handoff_accepted, native_projected_fields;
    U64 native_projected_cookies, held_application_wire; };
enum class Intent : std::uint8_t { owner_ready, admit, suspend, close };
enum class Post : std::uint8_t { queued, coalesced, wake_uncertain };
class Foundation {
public:
    Result<Done> admission(const Peer&, Pause) noexcept; // IO0
    Result<Done> close(const Peer&) noexcept;            // IO0
    Result<Observation> inspect() const noexcept;        // IO0
    Result<Post> post(const Wake&, Intent) noexcept;    // ordinary threads
    Result<Post> stop(Tick first_absolute_deadline) noexcept; // ordinary threads
    Result<Done> finish() noexcept; // creating thread, after framework/IO joins
    ~Foundation();
private: struct State; std::unique_ptr<State> state_;
    explicit Foundation(std::unique_ptr<State>) noexcept;
    friend struct Factory;
};
struct Endpoints { std::array<char,46> address; std::uint16_t http_port, port; };
Result<std::unique_ptr<Foundation>> create(Limits, std::span<const Route>,
    NativeBinding, std::shared_ptr<const native_ws::ElapsedSource>, Callbacks) noexcept;
Result<Done> register_listeners(Foundation&, drogon::HttpAppFramework&, Endpoints) noexcept;
```

All four handle classes explicitly delete copy construction/assignment. Their
private Factory friendship and move constructors issue handles only from actual
producer callbacks. Moves transfer the same State, context and charge; no public
index/context/Connection value can construct, find or revive a handle.
create and register_listeners are FIRST-PARTY. Their successor adapter is
`server/src/bounded_http_sdk_adapter.cpp`; it implements Producer/Sink; create calls
Runtime::install before listener/run configuration, register_listeners calls
Runtime::listener, and ListenerInfo stores the private producer binding in ListenerInfo.
The SDK never defines a competing first-party register_listeners symbol.
Actual source has `HttpAppFrameworkImpl::initFirst_` initializing the main
EventLoop BEFORE main. A runtime pre-bind setting cannot retroactively bound
that constructor. The successor therefore reserves the SDK-only global metadata
quota G=65536 before this initializer constructs its fixed framework bootstrap
objects; first-party global allocations later use the remaining SAME quota.
This is the G in §4, counted once, not an extra mapped array or a post-allocation
claim. The correction adds a nonallocating bootstrap quota guard before the
existing initializer's allocations and inventories its managers, main EventLoop,
MpscQueue sentinel, timer/poller/channel and standard-object requests. If those
source-proven rounded requests cannot fit G, SDK admission fails. Keep the
pre-main main-loop initialization/ownership required by legacy callers. At install,
require no user routes/listeners/queued work/run yet; adopt that already charged
main loop and fix its existing 16 event records, rather than pretending it can be
constructed later with 8. IO0's future constructor receives P+8 fixed records.
Framework run retains the parent's main-loop ownership transfer and exactly one
IO thread. No pre-main allocator/whole-process-heap guarantee is inferred for
other DSOs or unowned singletons.
Producer::before_read calls the actual check_read_window guard after state/time/
compaction checks; PeerStorage records its before-read indices and syscall result.
The pure function returns Done for 8192..65536 and resource_limit otherwise; it
neither reads nor grants a connection permit.

create, hook installation, finish and Foundation destruction use the creating
thread (fixture main or future Qt owner). Listener registration runs on the
framework-main thread before run. Socket/visit/fill/commit/admission/close and all
owner callbacks run on IO0. Cross-thread Result-returning operations other than
post/stop return wrong_thread without touching native state. release_raw and cancel
are IO0 operations; wrong-thread calls terminate before native access, since their
void signatures cannot report refusal. Moves may run on any ordinary thread with
exclusive access to both handles: they transfer State and only destroy the old
destination through the any-thread drop path below. They perform no IO operation.
Callbacks never pump events,
reenter dispatch or retain views. One owner callback executes at once; it may
invoke one synchronous visit/fill visitor, which cannot recursively visit/fill.
Exceptions are caught and mapped to callback_failed/close. No SQL/Qt host/current
session/generation setter is needed by this fixture.

### Correlation, handles and callback order

connected receives the sole Peer, initially suspended. During connected only,
context sets one non-null owner-supplied pointer and wake issues at most one Wake.
Both are optional; null context is valid and is returned unchanged on every
request, ready, ended, removed and native opened/message/closed callback. The
owner may use one preallocated context object per peer to store its Peer, current
Request/Reply, bridge association and Wake. Context is DATA, never transport or
host authority. Any retained owner context/result has its own finite caller
table/pool (fixture budgets or the unchanged host bridge), not uncharged lambda
captures or a claimed retroactive allocator bound by this interface. ready uses
that context to find the owner-held handles; it borrows no second Peer facade
whose C++ object address would need tracking across moves. The owner keeps it alive through removed and all in-flight posts;
callbacks carry no raw host ticket or secret view. Native callbacks obtain their
context from the actual SDK connection's private peer binding, never a public
Connection lookup or request header. message must claim an existing native
Profile lease before retention. Native closed is not native retirement.

Per peer there is exactly one 128-byte State slot for each of Peer, Request,
Reply and Wake (512 bytes, inside its 16384-byte metadata allowance). One current
request uses its slots; ended does not permit the next request until raw release,
wire terminal, current Request/Reply handles destroyed and admission permits it.
Wake/Peer may persist across sequential requests. Even inert retained handles
pin this peer's slot; it cannot be reused for another connection. A checked
internal peer incarnation and separate request sequence each start at 1 and never
wrap: incarnation advances before peer-slot reuse, sequence before a new request.
Exhaustion closes admission instead of reusing a value; observers may report it, but cannot supply it as authority.

State::operator new uses a thread-local slot-allocation descriptor set only by
Factory after reservation; it selects that exact aligned slot and throws
bad_alloc on wrong size/missing descriptor, caught as allocation_failure, never
falls back. operator delete returns it once, holding a core/delete-inflight pin.
It decrements the handle/delete-inflight counts only after State destruction and
slot return, as its last core access; finish acquires that fence before freeing
metadata. First-party Core must remain owned through every delete operation. No general new fallback. State destructors may run
on any ordinary thread: they only latch an atomic drop/cancel flag, latch the drop and wake the owner; they never touch socket/parser/callback
code. Live operations/moves have exclusive ownership; concurrent use/destruction
of the SAME C++ handle object is unsupported. Inert destruction after IO join
only returns metadata under the core lock; it neither posts to a closed FD nor
calls the owner. The core remains alive until all handle States and post calls
leave. Before finish, the caller must join external posters or destroy their Wake
handles, then destroy inert handles. finish reports incomplete_shutdown otherwise.

Dropping Peer closes. Dropping Request before commit cancels/closes; after commit
it releases raw and only drops the handle, preserving committed output. Dropping
an uncommitted Reply cancels/closes; dropping a committed Reply releases only its
handle. cancel explicitly closes/cancels even after commit; it cannot retract
kernel bytes. release_raw invalidates visits, not reply authority. ended fires
once for an issued Request, after raw revocation and reply terminal; notifications
are deferred until the enclosing owner callback/visitor returns. The first
terminal owner decision fixes End: successful local wire release=drained; peer
EOF/error=disconnected; explicit/drop cancel=cancelled; deadline=expired;
stop=stopped; elapsed failure=elapsed_failed; callback exception=callback_failed;
wipe failure=wipe_failed. Later events do not relabel it. ended precedes removed.
removed occurs after socket/channel/removal callbacks, actual native retirement
AND the separate SDK-object deletion fence below,
but retained inert handles still pin the slot afterward. Non-request native peers
have removed but no invented ended callback.

### Owning SDK objects, strong and weak native references

native_ws::Connection is an owning shared_ptr, distinct from the four new handle
States and native message leases. The selected WebSocketConnectionImpl strongly
owns TcpConnectionImpl; its shutdown/destructor can touch EventLoop even after
disconnect. Context::retire does not prove either SDK object's final destruction.
An owner may retain the actual Connection after native closed/retirement, but
the peer's SDK metadata and slot remain charged and unavailable for reuse.

For opted-in peers ONLY, change the SDK construction sites to separate pre-reserved
object storage and shared_ptr control blocks for TcpConnectionImpl and, when used,
WebSocketConnectionImpl. Do not use an ordinary make_shared deleter. Each real
shared_ptr has an SDK-private custom deleter and control-block allocator bound to
that peer's fixed deletion fence; the unchanged public Connection alias remains.
At most two objects, two control blocks and two 128-byte deletion records exist
per peer, all inside its existing 16384-byte metadata allowance. Selected source/
ABI proof must account every simultaneous object/control request there. Failure
before construction releases only actually reserved parts; no heap fallback.

The last strong release on ANY ordinary thread only latches that object's one
pre-reserved destruction record and wakes IO0 through the nonallocating cleanup
path. That path remains enabled during stop; it is not a new public admission
intent. IO0 destroys the real object after active callbacks/channel removal, while
its loop is still alive. No deletion closure or queue node is allocated. Object
destruction, deleter return and control-block storage return have separate pins.
The control-block allocator's deallocate records return intent; storage is actually
reusable only after object destruction and all deleter/allocator accesses finish.
Retained weak_ptr copies also keep that control block and the peer slot charged;
a final weak release on another thread only latches/wakes the same cleanup path.
There is no use_count-based inference or promise that a disconnected facade is inert.

Close cleanup clears the SDK's connection/parser/controller callbacks, native
binding and internal strong/weak references in their actual source owners before
dropping them. Break the WebSocket→TCP/parser ownership cycle explicitly on this
branch. Retire notifications remain independent and must release their own
binding pins; they cannot release SDK object/control-block storage. Once BOTH
objects, their actual control blocks and every destructor/deleter/allocator pin
are gone (one object on HTTP peers), the SDK fence invokes sdk_objects_released
once. This is a private actual-construction binding, never a caller-supplied ID,
test flag or writable snapshot. IO0 consumes the final notification only after
its sender's last fence access; sdk_retained_peers falls then, never at closed.

removed, peer-slot reuse and stop_drained require that fence as well as socket
removal/native retirement. Foundation and the owner context remain alive until
it completes; IO0 is not joined first. Owners should release their Connection
and weak copies during closed or independently before stop drains. An owner that
keeps one prevents clean drain/join/finish; it does not acquire a resettable
deadline or permission to destroy the loop. After the stop deadline, sockets can
be closed but retained facades still cause honest incomplete cleanup. The parent
supervisor may terminate; no graceful completion is fabricated. This changes only
the opt-in successor; historical native source/header/artifact/native44 stay intact.

### Actual read observations

After EVERY actual syscall result, PeerStorage::read_witness updates the peer's
checked ordinal and cumulative positive bytes, then synchronously invokes optional
read_observed on IO0 BEFORE parsing, compaction or another read can overwrite the
witness. ReadSample copies the actual pre-read indices/offered length, iovcnt and
signed return/errno; bytes_read is that peer's cumulative positive total including
this result. EAGAIN/EINTR/EOF are events too, not invented positive reads. Ordinal
overflow closes rather than wrapping. last_read_* remains a convenience snapshot.

There is one stack sample and no queue/history allocation or coalescing. Delivery
of a configured read callback is once per result; a throw closes with callback_failed.
The observation callback may inspect/copy the sample but cannot reenter transport
operations, pump events or retain borrowed payloads (none are supplied). No callback
occurs after removed. With a null callback only internal counters are maintained.
Caller-owned histories must have their own finite reservation: F07 uses two fixed
64-byte first-positive latches (128 bytes in its fixture context), never a growing
vector or log per read. Each latch is write-once; later samples only update bounded
counters, so splitting cannot overwrite it. Invalid/missing expected latches are
fixture failures, not skips or product-red claims. No witness setter grants IO.

### Cross-thread intents and real stop drain

At startup reserve one fixed mailbox record per peer (128 bytes in its metadata),
one global stop record and one Linux nonblocking eventfd plus Channel. No
per-post allocation, lambda, queueInLoop or payload capture. The record holds
owner_ready, close and an optional desired admission state. Under its bounded
core mutex, close absorbs other intents; admission is last-write-wins; repeated
owner_ready coalesces. IO0 swaps the record into a fixed executing record before
ready, so a post during ready can leave one additional pending notification.
There is one pending record plus one executing record per peer, both precharged;
a live issued Wake cannot encounter a full mailbox. A full PEER table instead
refuses issuance/construction before accept retention; a bridge payload queue
being full is handled by that bridge, not an unbounded mailbox fallback.

post validates the Wake's private live core/peer-incarnation binding before storing
anything. revoked/stopping/unavailable means no new intent committed. queued or
coalesced means it was recorded, NOT that ready ran. After committing the record,
write eventfd once; EAGAIN means an existing wake is pending and counts as success;
EINTR or another write error returns wake_uncertain with the intent STILL recorded.
Never roll it back, replay a response, or claim that a thrown enqueue recovered.
No stock throwing queue post is used. The fixed scan wake also observes mailbox
records, so later owner turns may consume an uncertain intent; the caller must
rely on actual ready/ended/removed observations, not the post result, for delivery.
Lock failure before commit returns unavailable with no write; a post does no user
callback while holding the lock. FD closure is gated under that same lifetime
lock after posters quiesce; post after that gate returns revoked without borrowing
an FD. This is ordinary-thread synchronization, not a signal-handler API.

IO0 eventfd readiness/scan invokes Producer::owner_turn, then ready(owner, context) for owner_ready. That callback takes the future host's already bounded
ResponseLease/result from the context's separately owned bridge, rechecks its
actual authority, and calls reserve_reply/fill/commit on IO0. No payload travels
in the foundation intent record. Empty/revoked bridge work causes no new response.
Generic fixtures use a finite context/result slot with the same wake callback,
without host flags. Output slot exhaustion returns resource_limit before fill;
the owner closes or waits under the original request deadline, with no reply queue.

stop first records and preserves ONE absolute deadline, closes admission for all
later posts, and wakes IO0. Later stop calls (earlier or later arguments) coalesce
without changing it. Already committed nonsecret output may drain until the
original output/stop bound; uncommitted requests/replies cancel. IO0 removes both
acceptors, then actual peers/nodes/callbacks, and awaits native retirement AND the
SDK-object/control-block deletion fences. When
there is no network owner, executing/pending callback or removal work (apart from
the notification itself), stop_drained fires once on IO0. Inert handles may still
remain; they do not grant network access. Only AFTER this notification may the
owner initiate app().quit(), join framework-main/IO cleanup, dispose remaining
inert handles/posters and call finish. Early quit force-closes in candidate4 and
is NOT a drain proof. Startup failure before run uses the same cleanup gate unless
the process took the parent's terminal bind/listen path.

finish acknowledges only actual joins, zero issued handles/post calls and no
quarantined storage; otherwise incomplete_shutdown/wipe_failed. Cold/NI Foundation
can destruct inertly. A live production Foundation destructor without successful
finish calls std::terminate (process-terminal, no graceful cleanup claim), rather
than free dependencies reachable by a stuck callback. A nonreturning trusted
callback cannot be preempted; supervisors retain their separate terminal policy.

## 3. Fixed storage and complete admission

Freeze one listener profile with uniform input capacity B and output wire
capacity W=E+128; there are no partitioned pools. Route.body_ceiling ≤ B is a
semantic refusal threshold, not a separate storage partition. At most eight
compiled ordinary routes (method token ≤ 16, target ≤ 2048) and one literal native
target ≤ 128 are copied once into fixed startup storage. No regex/decoded alias,
query repair or general router. Route index is the declaration's zero-based
position and is exposed only on admitted RequestView. Current host composition
uses its four exact POST paths; the generic fixture's separate routes are §7.

Ordinary peers reserve one I slot, even for body length zero, before head scanning
or enabling a first read. If unavailable suspend under accept/idle deadline; a
round-robin release wake retries before reading. Native pre-Upgrade uses its own
fixed head region and native U reservation; no ordinary input/reply slot.

Each peer has 65536 receive payload bytes plus the existing 8-byte prefix. Add a
non-owning fixed-region mode to MsgBuffer; its existing indices/readFd are used,
but no append/ensureWritable/retrieve path can grow, resize, copy into a vector or
fall back to stock storage. Before EVERY read, recheck state/time, process eligible
pending bytes, compact if needed, then require writable ≥ 8192. This selects actual
readFd iovcnt=1; its 8192-byte automatic stack array receives no payload and causes
no spill copy. Offered length is exactly the writable space ( ≤ 65536). No 1 MiB raw
buffer. A guard pause disables the real channel until identifiable progress or
expiry. EAGAIN/EWOULDBLOCK/EINTR preserve indices/anchors; EOF cancels incomplete
input with zero dispatch. Other errors/exception cancel and remove.

Retain one bounded raw head in receive until the complete head has been scanned;
40960 bytes fit. During scanning do not call setMethod/setPath/setQuery/addHeader,
copy semantic fields, allocate body or emit output. Find line endings once with
a fixed cursor. Bare LF endings can delimit a scan line but latch syntax_bad;
only CRLF is valid syntax. An empty scan line terminates the head. The scanner
still counts malformed lines/fields; a colonless line is one invalid record with
its whole content as candidate name and empty value, never end-of-head. All offset
arithmetic is checked before slicing. Completed oversized lines in one read have
exactly the same checks as split lines. No additional raw head copy is made.

Common bounds: method 1–16 token bytes; target 1–2048 bytes; complete request line
 ≤ 2048 including method/spaces/version/CRLF; raw head ≤ 40960 including all raw OWS,
separators/CRLF and final empty line; ≤ 64 field records; each trimmed candidate
name+value ≤ 8192; `S = 2 + sum(name_size+value_size+4) ≤32768`. Trim only outer
SP/HTAB; preserve all occurrences, original name bytes and empty values. The
separate semantic arena is 32768 bytes; records are 64×16=1024 bytes; method/target
slots are 16/2048. Check all limits BEFORE those copies. Case-insensitive ASCII
comparison of names detects framing/cardinality without a map or lowercased copy.

### Deterministic head decision and future account priority

For identical bytes received before the same deadline, status is independent of
TCP splitting. Within raw head budget, finish the bounded head scan and select
ONE decision below before any projection/body copy/output. Do not emit 414 upon
seeing an unfinished long line and later discover a higher-priority 431. A head
which crosses 40960 before termination always gets 431; no further head bytes are
needed to choose it. Head EOF/expiry without termination closes without a status.

1. Raw head/field count/per-field/S envelope overage → 431. If no such overage,
   line or target length overage → 414; method length 17 or empty → 400. These
   checks do not authorize any truncated/prefix request. Bare LF doesn't evade
   counting, and malformed records cannot turn an overage into early 400.
2. Safe framing/occurrence admission: CL must be one nonempty decimal digit value,
   checked U64 then size_t; leading zeros and absence (zero) are allowed. Duplicate
   CL, duplicate X-Host-Correlation, CL+TE, any Transfer-Encoding/TE/Trailer/Expect
   (including empty/100-continue), invalid CL or Connection nomination → 400.
   Ordinary Upgrade/Proxy-Connection → 400. Never a chunk decoder/decompressor/100.
   Native checks in §6 are part of this admission, before body allocation.
3. Apply the declared route's optional HEADER semantic ceiling to
   `method_size+target_size+12+S` → 431. Then CL greater than its body ceiling
   (or B for a route miss) → 413. Then optional complete semantic ceiling applied
   to that header projection+CL → 431. Check these sums, not their wrapped values.
4. Validate remaining line/field/Host syntax: exact HTTP/1.1, origin-form target,
   no fragment/space/control, HTTP token names, no whitespace before colon,
   obs-fold, colonless record, NUL/DEL/control (except value HTAB) → 400.
   Require exactly one syntactically valid Host, no userinfo, malformed brackets,
   zone ID, ambiguous port or whitespace → 400. HTTP/1.0 or any other version → 400.
5. Internal route/method miss, nonempty query (including an empty '?' marker),
   escaped alias or unknown syntactically valid method → 400. No body is read or
   retained for any refused route; the connection closes after fixed refusal.
   A known route consumes exactly CL, then dispatches once. Missing bytes followed
   by EOF have zero dispatch; extra bytes stay pending for the next request.

This generic internal policy does not replace future account transport syntax,
Cookie grammar, route 404/405, representation 415/406/400, auth 401/403, TLS/authority,
canonical origin or CommonFailure envelopes. The optional ceilings are a concrete
preflight seam; SESSION-011 must bind its trusted account profile before parsing,
then replace ONLY phase 4/5 with its accepted syntax/application decisions. It must
preserve reference HTTP header 431 → body 413 → aggregate 431 → syntax 400 ordering,
framing/occurrence admission first, and exact Cookie/credential failure order.
Account preflight/capture MUST precede any stock log/map. The generic fixture's
account-style budget probe is not an enabled account profile or credential source.
Reference body ownership remains ≤ 4096 AND complete semantic request ≤ 4096; no
1 MiB generic limit silently enlarges that. Real account capability waits for 011.

### Body, coalescing and raw hooks

After head admission, copy body incrementally from receive into one reserved I
slot, at most min(remaining_CL,readable); wipe consumed regions before reuse. No
whole-body scratch/string/temp file or public partial-body callback. After one
complete request, stop before reset/batching/construction of its successor and
disable reads. Later coalesced bytes remain unparsed in the same bounded receive
owner. Only after raw release, wire terminal, current Request/Reply handles gone,
callback exit, live admission and a fresh deadline check may one continuation
reset the parser for the successor. Process pending bytes before enabling reads.
Repeated resume schedules at most one continuation. Suspension does not stop the
original request/output/idle clocks. Stop/cancel/timeout wipes pending bytes.
This is APITEST-016 `second_coalesced_request_after_release`; APITEST-015 separately
owns host-admission composition. Generic tests contain no host fence/setter.

Compaction memmoves the surviving suffix to prefix 8, then wipes obsolete source
ranges EXCLUDING the surviving destination, before resetting indices. No borrowed
visit survives compaction. The future raw hook API is first-party:

```cpp
enum class RawKind { receive, fields, method, target, body };
enum class RawUse { current, pending, owner_teardown };
struct RawTag { void* peer_context; U64 sequence; RawKind kind; RawUse use; };
struct RawHooks { void* owner;
    bool (*wipe)(void*, RawTag, std::span<std::byte>) noexcept;
    void (*revoked)(void*, void* peer_context, U64 sequence, End) noexcept; };
Result<Done> install_raw_hooks(Foundation&, RawHooks) noexcept;
```

Installation is once, on the creating thread before listener registration, with
non-null functions/owner, otherwise invalid_config/invalid_state. Tags come from
the actual private peer/request: sequence is the admitted raw sequence; pending
unattributed suffix uses 0. Malformed head teardown uses its current sequence,
never client data as identity. Hooks may neither reenter nor retain spans.
Revoked runs once before raw scope erasure, after the last visitor returns; wipe
runs on every consumed/compacted/released/terminal mapped region. Default clear
returns true but claims no cryptographic erasure. False wipe marks storage
quarantined, closes the peer, refuses reuse and makes finish return wipe_failed;
its capacity stays charged until process termination. No false successful erasure.

These hooks cover ONLY mapped receive/fields/method/target/body, not heap-backed
native std::string projection or account output/crypto scratch. The native
projection is never an account route and receives no raw account fields. SESSION-011
must install/audit secure erasure and all additional capture owners under its real
artifact capability; installing a callback alone cannot mint that capability.

## 4. Exact resources and fairness

A1 corrects the initial three-copy output design before any authored baseline:
one W slot receives header+direct body fill and one fixed network node references
it. There is no E body copy, second render/TCP W copy or merged/cache buffer.
Uniform capacities, including unused pool slots, are reserved before allocation.

| Owner | Finite reservation and release |
| --- | --- |
| Every peer until actual removal AND last retained handle/post |65544 receive +32768 arena +2064 method/target +1024 records +1024 foundation control =102424 fixed mapped bytes; additional 8192 external native-projection character allowance, unused on HTTP. |
| Per-peer metadata |16384 bytes, includes connection/channel/socket/parser/control blocks, fixed name, four 128-byte handle States, pending/executing 128-byte mailbox records, removal record, up to two SDK object/control-block owners with two 128-byte deletion records and their fence, one native wrapper/projection when applicable, counters/deadlines, and 128 bytes for its fixed poller record. Retained native strong/weak references pin this charge too. |
| Global metadata |65536 bytes: two listener/acceptor bindings/idle FDs, fixed route table (eight targets ≤ 2048 and one native target ≤ 128), source/profile refs, mutex/eventfd/scan timer, free/role/cursor tables, global stop/drain record, and 24 fixed infrastructure poller records×128 (main 16 plus IO infrastructure 8). Peer membership itself is in per-peer metadata. |
| Input pool |I slots of B bytes and 128 metadata each, one per current ordinary request, including zero body; acquire before head scan/read. Move transfers the same slot; no input copy API. |
| Output pool |O slots of W=E+128 bytes and 128 metadata each (fixed node/bookkeeping), one per peer. Reserve whole W+128 before fill/render; node retains W until wire_released after actual drain/teardown. |
| Transient FD |One shared stack RAII token T=1 for both acceptors, no connection metadata/payload. Reserved before accept; always closed/transferred and released before another accept attempt. |

Metadata allocation requests are rounded up to 64 including alignment/control
block; before every construction, reserve the source-proven request sizes against
these whole preclaimed allowances. Use fixed typed slots for first-party records
and bounded source-reviewed allocations for unavoidable SDK std objects. No
allocator fallback, allocate-then-check, unknown capacity or replacement growth. State destruction/deletion are noexcept;
an unexpected lock/structural failure there terminates, rather than free a still
reachable owner or claim successful cleanup.
Selected sizeof/alignof/stdlib proof must fit the allowances; otherwise admission
blocks that build rather than silently changing numbers. Extra allocator heap
bookkeeping/usable slack, thread stacks, kernel FD/socket/backlog memory, TLS and
OS latency remain separate; no whole-process heap/RSS/hard callback bound.

The following source allocations are explicitly removed or charged:

- HttpRequestParser's eager std::deque pipeline storage becomes an optional legacy
  holder, never constructed in bounded mode; request/response batch vectors,
  request pool and default send MsgBuffer remain absent/empty. Do not construct
  an unused stock deque/2048-byte send owner then declare zero entries bounded.
- TcpConnectionImpl fixed-mode receive has no vector; pending TLS/list/output
  containers are absent on this plaintext branch. Fixed endpoint/name formatting
  replaces `local.toIpPort()+"--"+peer.toIpPort()` and its temporary strings.
  Numeric endpoint scratch and its terminators fit the preclaimed metadata.
- SDK removal uses one pre-reserved record per peer, run after the active channel
  dispatch turn, not queueInLoop's allocating closure/node. The same rule covers
  connectEstablished and fixed owner continuations; startup registrations are
  finite global records. MpscQueue allocates a sentinel on construction and a
  BufferNode per enqueue in LockFreeQueue.h: bootstrap sentinels are charged in G;
  bounded lifecycle/main↔IO markers use fixed records instead of those enqueues. No per-request EventLoop::funcs_ enqueue occurs here.
- At profile construction reserve IO poller events/active/debug storage for P+8
  channels and framework-main storage for 16, once. Each 128-byte record covers a
  16-byte event cell,8-byte active pointer and fixed debug/FD metadata; typed
  layouts must fit. Never double vectors on full epoll return; process the fixed
  returned batch, then poll again. Debug channel maps become fixed records too.
- Disable stock ordinary kickoff entirely before connection construction; no
  TimingWheel or extendLife bucket insertion on opted-in peers. One separate
  Linux periodic timerfd/Channel scans existing records; no repeating TimerQueue
  expired vector, per-tick registration or per-read timing-wheel allocation.
  Native C/U/message/timer/queue resources remain separately charged by frozen
  native ownership; this does not claim their allocations disappeared.

Projection is native-only after all checks. Copy only the configured literal path,
Host, Connection=`Upgrade`, Upgrade=`websocket`, Sec-WebSocket-Key and
Sec-WebSocket-Version=`13`. Project no Cookie/auth/query/other input field. Native
Host ≤ 256, target ≤ 128, key is canonical base64 encoding of 16 bytes (24 ASCII bytes).
For an initially empty string assigned once, charge
`round_up_pow2(max(32,n+1))` including terminator; source admission proves the
selected stdlib's allocation/capacity never exceeds it. Path 256 + Host name 32 /
value 512 + four other name/value pairs 8×32 =1056 character bytes. Remaining 7136
of 8192 covers simultaneously alive path-lowercase, matched path, appended key/GUID,
accept-key/response and other actual selected handshake temporaries, each
precharged. Request map reserves five entries; response map three: at most 64
buckets/eight nodes combined and 2048 rounded metadata bytes inside 16384. Enumerate
actual stdlib bucket/node/copy requests before admission; no unknown libc policy
or post-construction capacity measurement supplies the proof. Native heap
projection isn't an account capture/secure-wipe surface.

Reference profile: P=128 peers, HTTP quota=80, native quota=48, I=8, O=8,
B=E=1048576, W=1048704. Native immutable C=32/U=16 remain additional phase limits.
Tiny profile: P=4, HTTP=2/native=2, I=O=2, B=E=4, W=132, native C=1/U=1.
The output-pressure child changes ONLY tiny O to 1; it has a spare HTTP request
with which to observe a real output-pool refusal.
Smaller P2 profile: P=2, HTTP=1/native=1, I=O=1, B=E=4, W=132, native C=1/U=1.
For tiny/P2 use native frame/message/assembly 64, frames/message 16, callback leases 2,
per-peer/global callback bytes 128/256, raw 256, application output 256, control 254,
frames/turn 8, upgrade/frame/message/close 50/50/100/50 ms. Reference keeps all native
reference limits unchanged. Foundation accept/idle/request/output=5000 ms and
scan tick 10 ms reference; all four durations 50 ms and tick 1 ms for tiny/P2.

Trusted configuration must have positive finite counts/capacities/durations,
HTTP+native=P, I/O ≤ P, every route body ceiling ≤ B and native C ≤ native quota ≤ C+U
(checked from the installed immutable native limits, not a DTO supplied to create).
P2 can occupy one native phase at once; it need not reach C and U simultaneously.
The existing valid E==host response_capacity case remains valid; native frame
sizes and B/E are distinct. Source/ABI limits, representable products/sums/page
rounding and size_t/ssize_t/epoll int narrowing are all checked before run/bind.
There are no partitioning fields or implied partitioned/account pools.

Both listeners share the same P table. Never lend role quota: HTTP cannot consume
native's 48 reference slots. If a role becomes full after successful promotion,
disable its acceptor immediately until a slot actually retires. A connect in the
kernel backlog may succeed while full, but no application accept/read is promised;
it can wait or be refused by the kernel. A previously dispatched stale readiness
callback performs NO accept if full. T reaches 1 only for an actual allowed attempt;
it releases on accept EAGAIN/error or successful ownership transfer. If admission
closes between reservation and construction (e.g. stop), close that one accepted
FD with zero connection/parser allocations. Do not assert a third accepted FD
when the role was already known full.

Each owner wake processes at most 8 accepts per role and 8 peer continuations per
role, alternating HTTP/native and advancing fixed round-robin cursors. Each
HTTP/pre-Upgrade turn scans/copies ≤ 8192 bytes, issues at most one read, and each
output turn sends ≤ 8192 bytes. Established native keeps its frozen frames/turn and
raw limits; it is offered bounded round-robin opportunities, not redefined byte
limits. Reserved capacity and bounded opportunities hold when IO runs/callbacks
return. Finite HTTP deadlines do not prove kernel fairness, service when all of
one role's quota is occupied, OS scheduling or arbitrary callback completion.

Let X be the external metadata/projection allowance (not header semantic S):

`L = P*102424 + I*B + O*(E+128)` fixed mapped payload.

`X = P*(8192+16384) + 65536 + (I+O)*128` external requested-capacity allowances.

project_storage returns this exact scalar plan without allocating, sampling,
registering or granting native authority. It validates positive scalar limits and
HTTP+native=P/I,O≤P, then computes the expressions in F11's order. create reuses
that SAME function before platform/SDK coherence/narrowing and allocation; this
makes the arithmetic oracle a real producer seam, not an invented isolated product.
After scalar/plan and duration conversion, create checks P+8 against epoll
INT_MAX first, then payload/object size_t/ssize_t/ptrdiff limits, then installed
native coherence/source availability. No constructor/listener registration occurs
before these checks; the SDK bootstrap allowance already exists independently.

Reserve `A=L+X` before construction. Map only L, NOT another copy of X.
Page count is `L/4096 + (L%4096 != 0)` followed by checked pages*4096, not L+4095.
Peer byte stride is eight-aligned; I/O slots are byte storage, typed metadata
aligns separately. Native and host payload/backing/source pools are additional.

| Profile | L | X | A before page tail | Mapped L / tail | Total including tail |
| --- | ---: | ---: | ---: | --- | ---: |
| Reference |29888512|3213312|33101824|29888512 /0|33101824|
| Tiny |409968|164352|574320|413696 /3728|578048|
| P2 |204984|114944|319928|208896 /3912|323840|
| Tiny with O=1 |409836|164224|574060|413696 /3860|577920|

Observation reports A,
mapped L, assigned capacities and per-slot live/peak/release counts separately;
none measures RSS or untouched pages. A move adds zero; each separately retained
host/bridge copy is charged by its own pool.

### Exact consumer control-ledger qualification (CI005-B2)

The foundation SOLELY owns its per-peer 1024 mapped control array, used for fixed
HTTP failures and native 101; it is never lent/returned/aliased to host code. Host
HTTP refusal closes, as in the scoped API001 source; no new rich HTTP failure body
is promised. Parent positive 200/diag, fixed SDK400 and native/CommonFailure outcomes
stay unchanged. The consumer boundary's 17006592 ledger, INCLUDING its separate
131072-byte host control arrays, is RETAINED and charged separately in this
successor composition. Those are distinct physical arrays, unused for host HTTP
refusal, still available only to their original host/native bridge owner. Thus
foundation reference control 131072 PLUS host control 131072 is 262144 physical
bytes, already contained respectively in L and 17006592. This explicitly qualifies
the earlier “no double billing/no additional host 1024” wording; no array is removed,
no transfer API is inferred, and no historical assertion/receipt is rewritten.
WS serialization scratch, native protocol 254 reserve and other parent allocations
remain additional, with their existing owners.

## 5. Replies and deadlines

Ordinary ReplySize accepts status 200 only and bytes ≤ E. Serialize exactly
`HTTP/1.1 200 OK\r\nContent-Length: ` + canonical length +
`\r\nConnection: keep-alive\r\n\r\n` then body; close uses `close`.
Effective close is request Connection:close OR ReplySize.close. The request cannot
be kept alive by a false reply flag. On local drain of a close response, close
and discard/wipe pending successor bytes. No Content-Type/cookie/date/server/
compression/CORS/default page. Header cap 128 exceeds the maximum U64 header 81.

Only SUCCESSFUL reserve_reply consumes its response right. reserve_reply atomically reserves one W slot plus 128 node metadata and the
once-only response right before rendering the header. Body fill borrows precisely
the declared body subspan AFTER that header, directly in W. fill's returned count
must equal declared bytes; otherwise invalid_size/cancel/close. The span itself
prevents no malicious C++ write; trusted callback memory safety is a precondition.
Second reserve_reply → already_replied; second fill or commit before fill → invalid_state
with no visitor/network call or new allocation. A normal first fill is required,
including bytes 0. commit rechecks original request deadline/time/stop, freezes
W and queues its one referencing fixed node; no body/render/TCP copy. At equality
commit expires, no node/send. A duplicate commit is invalid_state. Wire capacity
remains charged after committed handle destruction until actual wire_released.
A user-space drain is not peer receipt and authorizes no replay.

Fixed control literals: `HTTP/1.1 <status> <reason>\r\nContent-Length: 0\r\nConnection: close\r\n\r\n`.
400 Bad Request /413 Payload Too Large /414 URI Too Long /431 Request Header Fields
Too Large are 66/72/67/86 bytes. No 100/405/417/reflected body or formattedString
fallback. Reserve the 1024 control slot before constructing failure/101. If held,
close without output. Never append another response after any bytes of an earlier
one; close instead. Cancel an unstarted application node before control, releasing
it once; there is no second reply queue. Native 101/control retains its slot until
actual node release. Its fixed node drains BEFORE established native output may
write, retaining candidate4 guard/order semantics.

Owner entry and commit guards first consume any recorded stop, without invoking
nested callbacks; a pending stop prevents a new commit even inside an already
executing callback. create preflights source availability on its creating thread; IO0 guards keep
their own previous sample, without comparing readings from different threads.
All guards sample the same retained source object, per-owner nonregression and
checked unsigned 64 ns conversion/addition; equality expires. A native source query
is not trusted UTC. Stock ordinary timing-wheel kickoff is DISABLED, not an earlier
clock/backstop. The periodic timerfd only wakes to sample/recheck this shared
source; it supplies no alternative elapsed observation.

| Phase | Absolute anchor / reference duration / tiny and P2 duration |
| --- | --- |
| Accepted/no request bytes | Sample while T held, before construction; +5000/+50 ms. Never restart at connected. Native pre-Upgrade also keeps acceptance+immutable native upgrade_ms until establishment. |
| Between HTTP requests | Later of raw release and actual previous wire terminal; +5000/+50 ms. Retained inert request handles or suspended admission cannot restart it; pending eligible successor starts in that turn. |
| Active HTTP request | First positive read observation for current request, or eligible attribution of a buffered successor; +5000/+50 ms, through parsing/body/suspension/reply commit. No trickle reset. |
| Output/control drain | First accepted commit/control decision; +5000/+50 ms. Successful commit ends request deadline; output/first-stop constraints remain. No progress/handle-drop reset. |
| Stop | First stop argument is preserved, not minimized or reset by another call. Actual IO consumption immediately fences work; post/stop submission is no receipt of that fence. Native first-Close bound also remains independently binding. |

Failure to sample, regression or deadline overflow tears down affected IO owners;
no UTC fallback or native syscall/SQL/outcome invention. Arbitrary callback/kernel/
OS latency has no hard deadline. Close after malformed/time/input failure never
fabricates a request callback. Exactly one pending continuation checks all guards
before more parse/read/write work.

## 6. Native pre-Upgrade and imported listener policy

Opted-in native pre-Upgrade uses the common scanner/fixed receive/admission;
Session::read false must NOT fall through stock read/parser. After common
preflight, require exact GET/native target/no query, one Host ≤ 256, one canonical
24-byte base64 key for 16 decoded bytes, version 13, one Upgrade:websocket and a
valid Connection token list containing upgrade. Any Protocol or Extensions
occurrence (even empty), Proxy-Connection or nonzero CL → 400. Missing/duplicate/
invalid Host/key/version/Upgrade/Connection → 400, except Host length 257 → 431.
CL absent/0/00 is eligible. Cookie/Authorization/other unrelated fields count
and remain raw but are never projected; no authenticated Upgrade is supplied.
Ordinary Upgrade and a complete non-Upgrade native HTTP request still receive
exact 66-byte 400 then close, zero ordinary/native message dispatch.

Validate before constructing the sanitized request/response. Use the existing
selected synchronous router/binder handshake and beginUpgrade/bind on IO0, with
fixed native callbacks and no projected-request escape. Invoke bind before 101;
resource/bind refusal safely closes with no 101. Nonempty read-ahead's ASSIGNED
MsgBuffer capacity 65544 plus R=min(native raw,4096) must fit native raw. Reference
1048590 permits 65544+4096=69640. Tiny/P2 raw 256 has R = 256, so nonempty fixed read-ahead
cannot promote: safe EOF, no 101. Empty read-ahead detaches the fixed MsgBuffer view
before bind, capacity 0, preserving the original empty-only small-profile behavior.
Do not free/reuse peer backing until full peer retirement. On success retain the
existing initial consume/release ordering and continuation/resource refusals;
report native assigned handoff capacity separately from whole reserved backing.
After establishment HTTP accept/idle/request guards cease; the frozen native
frame/message/Close/idle/lease producer continues. Native C/U owner and shared P
slot stay charged until actual Context::retire plus socket/handle cleanup.

Import, do not reopen, the parent numeric-address/data-endpoint observation,
public enableReusePort(true) probe bypass plus pre-listen SO_REUSEPORT clear and
readback 0/SO_REUSEADDR = 1, bounded private scratch parent/child/canaries, disabled
static/default/upload/plugins/logging/decompression and one fixed shared stop.
The bounded route table enforces the same four internal POST paths; stock sync
advice remains defense in depth elsewhere, not a pre-map guarantee. Source
exit(1) is NOT kernel/teardown proof. APITEST-015 retains both plain occupied-port
oracles plus CC-A1 SO_REUSEPORT occupier; exact exit 1 remains UNVERIFIED. CC-A4
terminal scratch cleanup/dedicated parent/PrivateTmp/restart belongs to
DEPLOYMENT-007/008, not a generic SDK file manager. No TLS/proxy/public auth behavior
is admitted here.

## 7. Literal oracle families and future handoff

All bytes/expected values below are independently specified, not implementation
output. `\r\n` denotes actual CRLF; ASCII repetitions are literal. A response
mentioned as “400” etc is the exact §5 literal then close. EOF is no reply.
Each positive uses current time/admission/capacity and valid Host:x unless changed.

The fixture route table is EXACT and ordered:0 POST /f (body ceiling B),1 GET /f
(B),2 GET `/` followed by 2032 `a` (B),3 POST /p (body ceiling 4096,
header/total ceilings 4096). Index 3 is present only in a separate B=8192 budget-probe
profile; main tiny/reference tables have indices 0–2 with zero optional ceilings.
Native target is /foundation/ws. Known generic methods are exactly configured
method strings, not the stock method enum: a separate method-bound child replaces
index 1 with `ABCDEFGHIJKLMNOP /f` (16-byte token) for its positive, without adding
a route or changing the 2048-byte complete-line limit. Its 17-byte method is 400.
Host production still has its four literal POST paths, never these fixture paths.

| Family | Literal stimulus / observable expected result |
| --- | --- |
| F01 basic | `POST /f HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\ndata` is 52 bytes (18+9+19+2+4). One route 0 callback/body data. Reply diag is 66 bytes (17+19+24+2+4). Tiny W = 132+node 128=260 reserved before fill, direct body fill at header offset 62; no extra body/wire copy. |
| F02 lines/methods | `GET `+`/`+2032 a+` HTTP/1.1\r\nHost: x\r\n\r\n` has complete line 2048 → route 2/200;2033 a gives 2049 → 414. GET /f gives route 1/200. Method-bound 16 token → its route 1/200;17 → 400. Unconfigured token FROB → 400, no body retained. Identical complete bytes/splits choose same status. |
| F03 raw head | Base `GET /f HTTP/1.1\r\nHost: x\r\n`26 bytes; eight `X: `+4092 SP+`a\r\n`4098 each; `Y: `+8134 SP+`a\r\n`8140; final CRLF gives 40952. Add 8 SP to Y → 40960/route 1/200; add 9 → 40961/431. Ten fields, S=2+9+8×6+6=65. One-read and every CR/LF split have same result. |
| F04 semantic records | GET /f plus Host and 63 `X:\r\n` → 64 fields/200, one more → 431. `X: `+8191 a gives name+value 8192/200;8192 a → 431. Host plus three X values 8191 and one Y value 8164 gives S=2+9+3×8196+8169=32768/200; one additional Y byte → 431. Raw head remains below 40960. |
| F05 framing/syntax | F01 CL = 5 under B = 4 → 413 before body copy; CL = 4 positive. CL `+4`, `4x`, empty, `18446744073709551616`, duplicate 0/00, CL+TE → 400. CL = 0/0000 with empty body → 200. Duplicate X-Host-Correlation 44/44, any TE/Trailer/Expect(empty/100-continue) → 400, no 100 prefix. Host absent/duplicate/space-invalid, HTTP/1.0/2.0, obs-fold/colonless/whitespace-before-colon/NUL/bare-LF head → 400 after bounded scan. Route /missing, GET-only /f sent as PUT, /f? or /%66 → 400 and no advertised body read. |
| F05P priority probe | Separate B = 8192 route 3: `POST /p HTTP/1.1\r\nHost: x\r\nContent-Length: 4097\r\nBad Header: x\r\n\r\n` → 413 despite malformed field token, identically split or coalesced; S=48 and header projection=4+2+12+48=66, complete 4163. Add `X-Pad: `+4022 a+CRLF: extra 4031 gives header 4097 → 431 even with CL = 4097. Without X-Pad and CL = 4096, header 66+body 4096=4162 → 431. CL = 1 gives header 63+body 1=64 → 400 for the same bad field. With a well-formed `Bad-Header: x`, CL = 1 and appended literal body `a`, the 64-byte projection is admitted/route 3. All split variants preserve these results. Refusals have no route callback/auth/Cookie/state lookup; the well-formed control has one route 3 callback. |
| F06 coalescing | Two F01 requests concatenated 104 bytes, one write. Hold first raw/Reply/current handles or suspend → requests 1, batches 0. Release raw and actual first wire, drop old Request/Reply, still suspended → 1. Admit once → route 0 request 2 once, pending consumed before read; repeat resume adds no wake. Owns generic second_coalesced_request_after_release, not host composition. |
| F07 read witnesses | Configure read_observed with two write-once 64-byte latches. Send only first 32 bytes of F01; callback latches the first positive read at head=8/tail=8/writable=65536/offered=65536/iovcnt=1. Wait for its cumulative bytes_read=32 while head incomplete, then send remaining 20. Latch the first positive sample whose prior cumulative bytes were 32: head=8/tail=40/writable=65504/offered=65504/iovcnt=1. Permitted 16+16 then 10+10 splitting preserves BOTH latches before subsequent samples overwrite last_read_*. EAGAIN/EINTR do not consume a positive latch. Total 52 gives one dispatch. Missing/contradictory latches or context overflow are fixture failures; no assumed packetization. Signed return/errno comes from the syscall. check_read_window(8192) → Done;8191 → resource_limit/no syscall, using the SAME adapter guard. Every actual positive read has writable ≥8192 and max_iovcnt1. Full 65536 coalesced receive while predecessor owns the request stays suspended/unparsed, no extra read/growth. Reference CL=1048576 streams to one B slot with raw=65544, no second body owner. |
| F08 output ownership | Two tiny output slots cost 2 × (W132 + metadata128) = 520. Use the separate tiny O=1 child and the finite actual-node hold gate below; withholding client reads alone is NOT proof of user-space retention. First committed node holds 260, then drop Reply and raw-released Request: observed slot1/capacity132 remains. A second HTTP request reserve → resource_limit before fill, without consuming its response right. Release gate and observe actual wire_released/node+metadata return → 0; second may reserve260 under its original deadline. Moves add0. Also require an ungated real-loopback66-byte positive control. The gate tests SDK ownership under a deliberate hold, not kernel EAGAIN or peer receipt. Partial emitted reply then error → EOF, no second head; busy control → EOF. |
| F09 admission/fairness | Reference HTTP peer 80 fills its quota and disables HTTP accept; peer 81 can sit in kernel backlog but accept_attempts/constructed do not increase. Native reserved 48 remain eligible subject to C = 32 / U = 16. P2 one HTTP+one native fills 2, no third accepted FD claim. An allowed attempt raises transient_fds 1 then returns 0 on accept/EAGAIN/stop-before-construction; zero refused construction. Actual removed plus last inert handle release frees quota once; disconnected alone doesn't. Observe per-role opportunities/caps, no kernel no-starvation claim. |
| F10 time/stop | Accept 1000 ms+5000 → 6000, idle 5999 eligible/6000 close; first request read 1100 → 6100 despite bytes 6099; commit 2000 → output 7000 despite progress 6999; raw+wire terminal 3000 → idle 8000. Tiny/P2 add 50. stop argument 6000 called at 1000 fixes 6000; repeat at 5999 with arguments 6500 OR 5500 keeps 6000. Equality expires. The creating-thread preflight 100/IO0 first 99 aren't falsely globally ordered; same owner's 100 → 99 ns closes with elapsed_failed. Native established idle has no HTTP expiry. stop_drained precedes quit/join/finish and has zero network counts. |
| F11 configuration/arithmetic | project_storage uses checked W=E+128, P*102424, I*B, O*W, L sums, P*24576, I+O and 128 product, X sums, A=L+X, page count/multiply, then total=mapped+X, in that order. With B = E = 4, I = O = 1, P = 145249953336295 (HTTP = P−1, native = 1), L = 14877081220516679216 and X = 3569662853192851712 individually fit, A = 18446744073709530928 fits; next P = 145249953336296 gives L = 14877081220516781640 and X = 3569662853192876288 individually fit but A = 18446744073709657928 → arithmetic_overflow at L+X. project_storage's first case returns its literal plan; actual create then refuses that huge otherwise numeric plan as invalid_config at epoll int narrowing, before registration. E=18446744073709551615 → arithmetic_overflow at E+128. Real create duration 18446744073709 ms converts 18446744073709000000 ns; next ms → arithmetic_overflow. Size preflight precedes platform/native profile coherence checks. Zero/incoherent scalar counts → invalid_config; allocation failure → allocation_failure. Actual source U64_MAX ns plus 1 ms → elapsed_unavailable/close, no wrap. |
| F12 once/drop/fault | Second reserve → already_replied; second fill/commit-before-fill/duplicate commit → invalid_state without extra callback/send. Wrong returned fill count → invalid_size then cancelled/close. Commit at exact request deadline → expired/no wire. Drop before commit → cancelled, after commit retains W until real release. EOF with 3/4 body → zero request callback; callback throw → callback_failed/close. End once before removed; native retirement delayed by a real retained lease pins peer even after closed. Inert custom handles after IO join block finish until any-thread destruction; no closed-FD wake. Result-returning wrong-thread/reentrant/revoked access refuses without underlying work. Wrong-thread void release_raw/cancel terminates its disposable child before native access; exclusive off-thread moves remain allowed. |
| F13 native | Literal canonical Upgrade and all native/refusal/read-ahead cases are enumerated immediately below; observe callback context, control release and native retirement, not a fabricated map DTO. |

For F08 only, the reviewed fixture observer header
`server/include/subathon/server/testing/bounded_http_observers.hpp` declares
`testing::Access::arm_first_wire(Foundation&)` (creating thread, cold, once) and
`testing::Access::release_wire(Foundation&)` (IO0), both Result<Done>/noexcept.
The optional one-shot gate holds the first actual application fixed wire node
after commit and BEFORE its first socket write. It neither fabricates a node nor
replaces Transport::wire; controls/101 are never gated. One 128-byte global record
inside G65536 holds its state; inspect exposes held_application_wire=0/1. The fixture
uses existing post(owner_ready) to release on IO0, with no new payload queue. Wrong
thread/state returns the corresponding error. Expiry, cancellation and stop close
and release the real node once regardless of the gate; the gate cannot extend a
deadline. Unavailable gate/witness/positive control is fixture failure, not red.
Its instrumentation and actual node position require independent test/source and
artifact admission; no production/public host option or capability setter is added.
The first-party test adapter calls the SDK-only Runtime arm/release functions,
which validate the actual installed cold/IO binding. NI Access definitions call
no new SDK symbol. The SDK owns the gate and actual fixed node; the fixture cannot
set wire_released or an observed capacity directly.

Native lifetime cases additionally retain one real Connection from opened, with
NO message lease, across closed and actual native retirement. Drop all four new
handles: sdk_retained_peers remains1, removed stays0, role quota is not reused and
stop_drained does not fire even after socket removal/deadline. Releasing the last
Connection on an ordinary non-IO thread queues bounded SDK destruction; observe
the real destructors/control-block return on IO0, then removed once and drain
before quit/join/finish. Repeat while retaining only a weak Connection: object
destruction may complete but the control-block fence still pins the peer until
the final weak reset. No use_count, native closed, zero leases or public snapshot
can substitute for actual final disposal. A retained message-lease case remains
separate; neither assertion replaces the other.

Native positive request is exactly:

`GET /foundation/ws HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n`.

The complete 101 (129 bytes; request 153 bytes) is exactly
`HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n\r\n`.
Use fixed-destination case/order, disabling stock extra headers. Empty read-ahead
reports capacity 0/accepted=1 and succeeds both native reference and tiny/P2; opened carries the connected context.
Host made of labels 63 a,63 b,63 c,58 d (three dots), followed by`:12345`, is 256
bytes and succeeds; last label 59 gives 257 → 431. Missing/duplicate/invalid Host → 400.
Key 23/25 bytes, noncanonical base64, version 12/absent/repeated, Protocol or Extensions
present(empty included), nonzero CL and invalid Upgrade/Connection → 400/no 101.
CL = 0/00 succeeds. Cookie:`synthetic=1` does not change 101: native_projected_fields=5 and
native_projected_cookies=0, with no Cookie addHeader/log call; it makes no account-log/erasure claim. Append masked Binary
`82 81 01 02 03 04 40` in the SAME actually observed receive as the Upgrade:
reference reports native_handoff_capacity=65544, accepted=1 and one
message 41/native lease; tiny/P2 reports capacity 65544/accepted=0 and closes with
no 101 because 65544+256 exceeds 256. Separate write alone is not proof of read-ahead;
assert the actual SDK witness/assigned handoff observation. The fixture may
preload the literal pair behind a bounded connected-callback gate, then release
it without event pumping. Permit at most three fresh peers to obtain the required
actual coalesced witness; absence is a fixture failure, not a product red, skip
or claim of kernel packetization. Each previous peer must actually retire before
the next; no in-process profile/framework restart. No ordinary request
callback, stock WS fallback or owner-context manufacture in any native case.

## 8. Baseline, admission and scope handoff

Future ownership remains bounded:

| Owner | Planned paths and gates |
| --- | --- |
| APITEST-016 after contract closing | `server/include/subathon/server/bounded_http.hpp`, `server/src/bounded_http.cpp` NI-only declaration/stub packet, including the private `server/include/subathon/server/testing/bounded_http_observers.hpp` and NI Access functions; `tests/bounded_http_foundation/bounded_http_probe.cpp`, `tests/bounded_http_foundation/test_bounded_http.py`. Independent literal/fixture/stub review before behavioral red; finite child/peer/pipe/log/event/maps/watchdog/reap budgets fixed before execution. No API001/Qt/SQL dependency. |
| API-016 fresh implementation | Those first-party producers plus `server/src/bounded_http_sdk_adapter.cpp` and the exact §1 source seams, including the new native build copy. Preserve reviewed tests, source/resource and separate code/artifact/native review. |
| Coordinator | New `dependencies/bounded-http-foundation/{manifest.json,0003-trantor-bounded-http.patch,0004-drogon-bounded-http.patch,0005-native-source-binding.patch,README.md}`, SDK prefix/provenance, build-copy recipe, dependency/CMake/CI/acceptance/license integration. Original native files/input hashes remain unchanged. Names are proposed, not actual artifact identity. |
| SESSION-011 | Actual successor rebase, account-specific lossless capture ABI, secure erasure/pre-log suppression and accepted account priorities. SESSION010 retains later source/policy integration. No API001 dependency for generic/capture fixture proof. |
| APITEST-015/API001 | Host dual-listener/current-source/bridge/shared-stop composition; all 75 Qt selectors,18 native methods, HL01–17/L01–13, actual SQL/authority/cleanup gates and original red history stay binding. |

The baseline first-party header uses only unchanged candidate4/public native types.
NI implementations of create/register_listeners/post/stop/hooks/project_storage/
check_read_window and other declared
functions are in bounded_http.cpp; they contain NO references to new SDK symbols,
no registration/parser fix and no competing SDK namespace definitions. Do not
compile bounded_http_sdk_adapter.cpp or a new vendor interface implementation in
that NI target. Therefore unchanged admitted candidate4 can compile/link the
baseline; a real legacy listener positive control runs in its OWN disposable control
child. A separate feature child, before listener configuration/run, requires
Foundation availability and fails explicitly at NI.
That is missing-producer behavior red, not a missing header/symbol/setup failure.
F-family bodies behind this first missing boundary remain unexecuted until the
producer exists; receipts must name that limit rather than claim their individual
wire behaviors observed red. No skip/NI-success/all-refusal control is allowed.

The later SDK types/functions exist ONLY in the separately reviewed successor;
the adapter implements them and replaces the first-party NI definitions in a
separate selected target. Vendor SDK interfaces cannot collide with the baseline
first-party names. The source/ABI/stub/test/admission packet must be approved
before any executable stage. Independent implementation/source/resource review
and all 44 unchanged compatibility follow passing assertions.

Preserve selected Release/shared/C++20/OpenSSL sixteen-option recipe; linking
OpenSSL admits no TLS runtime. New manifest binds exact old upstream/patch inputs,
ordered NEW patches, original/derived first-party source hashes, ABI marker
`DROGON_SUBATHON_BOUNDED_HTTP_V1`, SDK-only interfaces, compiler/stdlib/ABI, all
actual allocations/rounding and installed inventory. Never compile original and
derived native first-party sources together. MIT/BSD, orm_lib/COPYING,
GPL-3.0-or-later correction provenance and actual transitive notices remain.
Each new foundation/native 44 consumer needs passive complete ELF/interpreter/
DT_NEEDED/VERNEED/symbol/RTTI/ABI closure before loading, scrubbed controlled
identity after release, complete actual loaded maps at initialization/serving/
cleanup and unchanged seal throughout. No old loaded receipt admits a new binary.

Only authored design/source reading/literal arithmetic and document validation
are evidence here. No header/stub/test/source/patch/build/SDK/native execution is
authored or run. Independent different Codex verification then fresh Claude closing
must approve this correction before test authorship. API001 draftPR121 source70ea261
has scoped partial approvals only; current main host stubs are unimplemented.
All SQL/CI/provider/platform/security/deployment/release gates, seven integrations,
three FX providers, English/German, themes, separate renderers and full desktop
editor remain in complete v 0.45 scope.
