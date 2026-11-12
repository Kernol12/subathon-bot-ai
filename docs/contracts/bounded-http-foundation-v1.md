# Bounded selected HTTP foundation — v1

APICONTRACT-005; proposed identity `subathon.bounded-http-foundation.v1`.
Normative design for independent review, **not implemented or artifact-admitted**.
Author checkout starts at `d5d19fc8017adf9fa966daa01d8f092fe69a6c7d`, after the
recorded normal PR122 merge `c9fd637`. Authority: specification v0.45 §2,
SERVER-01, API-01, AUTH-12, §§17–18. Import the COMPLETE
[listener contract](api-host-listeners-v1.md),
[host lifecycle](api-host-lifecycle-v1.md),
[consumer boundary](api-host-lifecycle-boundary-v1.md),
[native profile](native-websocket-profile-v1.md),
[account ingress](account-ingress-v1.md) and
[ADR-0011](../decisions/0011-ingress-boundaries.md). Account transport/reference
R2, SESSIONCONTRACT-005/006 closures and their failure priorities remain binding.
This document freezes the generic producer before APITEST-016 authors its baseline.

## 1. Selection and source correction

Use the existing Drogon parser/state machine and Trantor connection/channel in ONE
framework lifetime, TWO internal plaintext listeners, exactly ONE IO thread and
ONE shared `native_ws::ElapsedSource`. No replacement parser/server, proxy, auth
service, TLS admission or second host is selected. The old API-012 candidate4
artifact, two patches, manifest/pins, public native header and all44 methods remain
immutable. A distinct opt-in successor carries these additions; legacy unselected
listeners retain candidate4 behavior, including its marked-listener HTTP refusal.
All44 must also pass unchanged when rebuilt/admitted against the successor.

Read-only source basis was the parent's
`/home/archnoob/ProgrammingProjects/SubathonProject/subathon-bot-ai/build/deps/`
`drogon-1.9.14-api012-candidate4-source` and matching `-sdk/include`:
Drogon `3aae769cc405cf5abb9d477675c6430924f02c7c`, Trantor
`78e480f5e138acdc05f76c55a2716718cd846364`, followed by the exact patches in
[the historical manifest](../../dependencies/native-websocket-profile/manifest.json).
These observations are neither clean-upstream nor installed-binary/runtime proof.
The manifest's original candidate status is historical; API-012 remains admitted
history. No successor hash, installed capability or admission receipt exists yet.

The following are required PATCH sites, not claimed upstream hooks. Paths are
relative to those selected source roots. The bounded branch is selected from the
owning `ListenerInfo`/`HttpServer`/`TcpServer`, before constructing a connection.
It is never selected by Host, Upgrade, a path, a DTO string or connection advice.

| Actual producer/site | Required additive correction |
| --- | --- |
| Drogon `lib/inc/drogon/HttpAppFramework.h`, `lib/src/HttpAppFrameworkImpl.{h,cc}`, `ListenerManager.{h,cc}`, `HttpServer.{h,cc}` | Append a private bounded-listener registration seam, carrying an installed profile and role through BOTH listener-construction paths. Keep existing `addListener(..., nativeProfile)` ABI declarations available. Reject TLS, missing native profile for native role, multiple IO loops, mismatched elapsed source or duplicate foundation installation before run/bind. |
| Trantor `trantor/net/inner/Acceptor.{h,cc}::readCallback`, `trantor/net/TcpServer.{h,cc}::newConnection` | Acquire shared role/peer and FD reservations before accept/construction; rejected accepted FDs get only stack RAII/close, no connection/parser/name/map. Admit before `make_shared<TcpConnectionImpl>`, channel/socket/name and `connSet_.insert`. No public after-accept callback can substitute. |
| `trantor/net/inner/TcpConnectionImpl.{h,cc}` constructor, `readCallback`, `connectEstablished`, `handleClose`, `connectDestroyed`; `trantor/utils/MsgBuffer.{h,cc}` | Construct the fixed receive owner directly, with no initial2048 vector followed by replacement. Add fixed-storage/index/wipe operations and actual channel disable/enable. Check before every `readFd`; never enter grow/overflow-copy branches. Preserve the native established read path. |
| Drogon `lib/src/HttpRequestParser.{h,cc}::reset/processRequestLine/parseRequest` | Extend the SAME state machine with a bounded sink: scan method/line/fields before setPath/setQuery/addHeader or body allocation. In bounded mode reset creates no stock request/pool entry. Store validated offsets/bytes in fixed storage, parse checked CL from occurrences, stream body into its reserved route slot; do not enter chunk/Expect branches. |
| `HttpRequestImpl.{h,cc}::addHeader/reserveBodySize/appendToBody`, `HttpServer::onMessage/onRequests` | Ordinary bounded requests use the private view/lease below; ZERO stock header/cookie maps, path decoding, request clones, request/response batch vectors or pipeline deque entries. A fixed compiled route table consumes exact escaped targets. Native bounded Upgrade uses ONLY the sanitized projection below. Unselected framework paths are unchanged. |
| `lib/src/HttpControllersRouter.cc` WebSocket routing, `HttpControllerBinder.cc` handshake, `WebSocketConnectionImpl.{h,cc}` | Keep existing handshake/framing implementation. Add one bounded synchronous binding for the compiled native route, using a sanitized HttpRequestImpl projection, existing beginNativeProfileUpgrade/bindNativeProfile and existing native initial handoff; no alternate WS parser/handshake implementation. Account capture is not a WS binding. |
| `HttpResponseImpl.{h,cc}::renderToBuffer`, `HttpServer::sendResponse/sendResponses` and parser-error/marked-refusal branches | Add fixed-destination rendering for the finite response shape in §5, with reservation BEFORE construction/render. Bounded mode cannot fall through stock error/100/default/render/cache/merge output. Native101 projection uses the separately reserved control destination. |
| Trantor `TcpConnectionImpl::send* / sendInLoop / writeCallback`, `BufferNode` | Add a fixed node referencing the charged wire slot; no std::string/MsgBuffer append, BufferNode growth, stream/file/async path or off-thread send-copy. Guard ALL send entry points on bounded connections before a copy. High-water advice is after append and supplies no enforcement. |
| First-party `server/src/native_websocket_profile.cpp::Context/ProfileFactory` in the SUCCESSOR build copy only | Add a private installed-context source-identity query/registration issued by the real factory and revoked for new admission when its owner is closed. Existing public native header/semantics and original tracked source/manifest input remain unchanged. Apply a separately pinned narrow first-party patch to a new build-local copy; compile that copy instead of (never alongside) the original in successor consumers. No old hash/receipt describes that copy. |
| `TcpServer::handleCloseInLoop/stop`, EventLoop owner queues/timer wake | Release only after connSet erase, channel removal, connectDestroyed and last callback/node release; use fixed peer-index iteration instead of stop's allocating connSet copy. One coalesced bounded mailbox/wake and scan timer replace per-request queued closures. |

This is one coherent foundation correction, not a series of new prerequisite
projects. It does not assert that stock APIs already supply these guarantees.

## 2. Private C++20 boundary

Proposed first-party header `server/include/subathon/server/bounded_http.hpp`,
namespace `subathon::server::bounded_http`. Supporting declarations include
`<array>`, `<span>`, `<memory>`, `<variant>`, `<cstdint>`, `<string_view>` and native profile types.
All methods below are `noexcept`; implementations catch allocation/user callback
exceptions and close the affected owner. `NI` exists ONLY in explicit baseline
stubs; no caller treats it as success.

```cpp
using U64 = std::uint64_t;
using Tick = native_ws::ElapsedInstant;
struct NI {};
enum class Error { invalid_config, not_owned, wrong_thread, reentrant, revoked,
    resource_limit, allocation_failure, expired, elapsed_unavailable, stopping };
struct Done {};
template<class T> using Result = std::variant<T, Error, NI>;
enum class Role : std::uint8_t { http, native };
enum class Pause : std::uint8_t { admitted, suspended };
enum class End : std::uint8_t { drained, disconnected, cancelled, expired };
struct Limits {
    U64 peers, http_peers, native_peers, input_slots, output_slots;
    U64 body_bytes, reply_bytes; // per route profile; §4 defines partitioning
    U64 accept_ms, idle_ms, request_ms, output_ms, tick_ms;
};
struct Field { std::uint32_t name, name_size, value, value_size; }; // 16 bytes
struct RequestView {
    std::string_view method, escaped_target; // never URL-repaired
    std::span<const Field> fields;
    std::span<const std::byte> arena, body;
};
struct Route { std::string_view method, escaped_target; U64 body_bytes; };
struct NativeBinding {
    std::string_view escaped_target; // at most128 bytes, one compiled binding
    void* owner;
    void (*opened)(void*, const native_ws::Connection&);
    void (*message)(void*, const native_ws::Connection&); // claim Profile lease
    void (*closed)(void*, const native_ws::Connection&);
};
struct ReplySize { std::uint16_t status; U64 bytes; bool close; };
struct Counts {
    U64 peers, http_peers, native_peers, input_slots, output_slots;
    U64 receive_bytes, field_bytes, body_bytes, reply_bytes, wire_bytes;
    U64 metadata_bytes, control_bytes, callbacks, queued_wakes;
};
struct Observation { Counts current, peak; U64 reserved_backing, mapped_backing;
    U64 reads, one_iovec_reads, requests;
    U64 suspended_reads, stock_maps, stock_batches, stock_output_appends; };
class Peer; class Request; class Reply; class Foundation;
// Move-only opaque handles: private native-issued constructors, no shared_ptr
// escape, equality/token setter, raw socket, host ticket or public ID lookup.
class Peer {
public:
    Peer(Peer&&) noexcept; Peer& operator=(Peer&&) noexcept; ~Peer();
    Peer(const Peer&) = delete; Peer& operator=(const Peer&) = delete;
private: struct State; std::unique_ptr<State> state_;
    friend class Foundation;
};
class Request {
public:
    Request(Request&&) noexcept; Request& operator=(Request&&) noexcept; ~Request();
    Request(const Request&) = delete; Request& operator=(const Request&) = delete;
    Result<Done> visit(void*, void (*)(void*, RequestView)) const noexcept;
    Result<Reply> reserve_reply(ReplySize) noexcept;
    void release_raw() noexcept;
private: struct State; std::unique_ptr<State> state_;
    friend class Foundation;
};
class Reply {
public:
    Reply(Reply&&) noexcept; Reply& operator=(Reply&&) noexcept; ~Reply();
    Reply(const Reply&) = delete; Reply& operator=(const Reply&) = delete;
    Result<Done> fill(void*, void (*)(void*, std::span<std::byte>)) noexcept;
    Result<Done> commit() noexcept;
    void cancel() noexcept;
private: struct State; std::unique_ptr<State> state_;
    friend class Request;
};
struct Callbacks {
    void* owner;
    void (*connected)(void*, Peer);          // one move-only observation/handle
    void (*request)(void*, Request);        // only completed admitted requests
    void (*ended)(void*, const Peer&, const Request&, End); // once/request
    void (*removed)(void*, const Peer&);                  // once/actual removal
};
class Foundation {
public:
    Result<Done> admission(const Peer&, Pause) noexcept;
    Result<Done> close(const Peer&) noexcept;
    Result<Observation> inspect() const noexcept;
    Result<Done> stop(Tick absolute_deadline) noexcept;
    // Creating caller thread, AFTER framework return and IO cleanup/join.
    Result<Done> finish() noexcept;
    ~Foundation();
private: struct State; std::unique_ptr<State> state_;
    friend struct Factory;
};
Result<std::unique_ptr<Foundation>> create(
    Limits, std::span<const Route>, NativeBinding,
    std::shared_ptr<const native_ws::ElapsedSource>, Callbacks) noexcept;
```

Route declarations are startup DATA copied into the fixed route table, at most8
ordinary exact method/escaped-target pairs; route.body_bytes≤B. NativeBinding
borrows its owner through finish and forwards existing native profile events on
IO0; message callbacks must claim the existing Profile lease before any retention.
It grants no authenticated Upgrade. Existing native controller compatibility is
retained on unselected listeners. The private SDK registration is proposed as:

```cpp
namespace drogon::bounded_http {
struct NumericEndpoint { std::array<char,46> address; std::uint16_t port; };
subathon::server::bounded_http::Result<subathon::server::bounded_http::Done>
add_listener(HttpAppFramework&, subathon::server::bounded_http::Foundation&,
             NumericEndpoint, subathon::server::bounded_http::Role) noexcept;
}
```

It delegates to the owning addListener construction, with nativeProfile=true ONLY
for Role::native; it carries the private installed foundation binding in
ListenerInfo. Exactly one registration per role is allowed, before run; numeric
address and port rules import the listener owner. A caller can configure a role,
but cannot tag an existing connection or mint a Peer/capture capability. The
bridge verifies the installed native profile's retained source pointer privately
through the successor-only Context/ProfileFactory seam in §1; this query is NOT
in the old SDK. Its result borrows the actual immutable installed context, carries
no setter and cannot substitute an arbitrary source/capability string. The
original public native header stays unchanged.

`create` validates/reserves fixed tables/backing before construction, installs ONCE
before run and retains the actual source. A separate private SDK bridge binds
`Foundation` to the two ListenerInfo instances and the installed native profile;
no public string names an admitted capability. Runtime/ELF identity is admission
input, not a caller-set profile field. Native capture later binds to the actual
parser/connection/request sequence/source identity inside this bridge. None of
these data observations supplies host serving, account authority or current UTC.

Create/finish/destruction belong to the creating coordinator thread (fixture
main, or Qt owner in the host). It joins the distinct framework-main thread after
run returns; framework cleanup owns joining IO0. The framework-main thread may
register the preinstalled listener bindings before run. All handle operations,
admission/close/inspect and callbacks run on IO0. Stop is thread-safe, posting
only the once-coalesced fixed mailbox intent; it grants no new admission.
Only the owner's fixed mailbox
may carry `stop`/admission/close/reply intents from another thread, using internally
issued revocable handles and fixed pre-reserved records; payloads/views never
cross that mailbox. The eventual Qt adapter owns its separately charged bridge
copies and posts only those finite intents. This API needs no Qt host, writer,
session, generation or fake readiness flag in the real-stack fixture.

One owner callback executes at a time; no event pumping, recursive visit/dispatch or
retention of borrowed views. Callback function pointers and the single owner are
registered once, remain alive through finish, and have no arbitrary captured
lambda storage. Request callbacks may retain the move-only handle on IO0; its
body/raw data remain charged. Each new peer starts suspended; connected must
explicitly admit transport parsing, without manufacturing host authority. Dropping
its sole Peer handle closes it. Dropping a current Request cancels it/its reply
and closes unless already terminal; release_raw alone does not finish a request
awaiting reply. Successful reserve_reply transfers one response right, once only,
without releasing raw data; Request can later release raw and remain an inert
binding through the response's terminal notification. `visit`/`fill` each borrow only for that synchronous
call; reentry, wrong-thread, revoked, exceptions or late commit refuse. A throw
revokes the request/reply and closes, not a reflected500. `fill` runs at most once
with exactly declared bytes, including an empty span; commit requires a normal
fill and performs a fresh deadline check. A handle destructor cancels/releases
once; State uses the pre-reserved finite metadata slots, never unaccounted heap allocation.
An owner callback may synchronously call visit/fill (accessor depth at most1);
these visitors cannot recursively visit/fill or invoke another owner callback.
Ended/removed notifications arising inside an accessor are latched and delivered
only after its enclosing owner callback returns, from the same fixed record.

`release_raw` invalidates all future visits, releases body/capture only after the
current visit returns and cannot release reply/network ownership. The ended and
removed callbacks run against the same registered owner; to disambiguate peers,
their signatures borrow `const Peer&` (the original handle may have moved);
ended also borrows `const Request&`. These revocable borrows
are valid only within the callback and cannot resurrect a visit. No payload or
public ID supplies their binding. Once
ended fires, any retained Request is inert. IO detaches/revokes handles before
finish; inert handle State/metadata must outlive retained handles (charged until
the last handle is destroyed). Finish requires zero issued live handles; an
uncooperative owner cannot obtain a false cleanup acknowledgment.

## 3. Parse, receive and admission rules

The common fixed head envelope counts original octets BEFORE retention/projection:
method1..16; escaped target1..2048; complete request line≤2048 INCLUDING method,
spaces, HTTP/1.1 and CRLF; head≤40960 INCLUDING that line, every field's original
OWS/colon/CRLF and terminating CRLF. At most64 fields, each trimmed semantic
name+value≤8192; `H = 2 + sum(name_size + value_size + 4) ≤ 32768`.
The arena capacity is32768, even though H also counts separators. Records are
64×16=1024 bytes, with checked 32-bit offsets. Method/target have separate fixed
16/2048 slots. Before storing each field check the full count, H, raw length and
arena end; empty values still occupy records. Every declared target length is
prevalidated before the route-table copy; no route/query decoder is invoked. CRLF search success does NOT bypass
length checks. On incomplete lines, reject as soon as no legal completion can fit;
a trailing CR may be pending only if its LF would fit. Repeated scans track a
fixed cursor, not a growing line string or quadratic restart scan.

CRLF only; no obs-fold, whitespace before colon, colonless nonempty field or
NUL/DEL/control except value HTAB. Method must be a token; target origin-form,
no fragment/space/control; no URI decoding/cleaning. Only exact HTTP/1.1. The
existing method switch is bounded before invocation; unknown syntactically valid
methods are refused400 on the internal route profile, never an unbounded method
string. Host occurs exactly once and has valid bounded authority syntax. Host
canonical trust matching, all credential/Origin/CORS and application415/406
priorities remain with account ingress/transport; this foundation invents none.

Validate occurrences BEFORE maps, body reserve, application callback or output.
All duplicate CL (even0/00), duplicate X-Host-Correlation, TE, Trailer, Expect,
Transfer-Encoding and CL+TE refuse400/close. Single CL is nonempty decimal digits,
checked U64 then size_t then route limit, permitting leading zeros; absent means0.
No chunk decoder, decompression, interim100 or input-derived early error body.
Ordinary HTTP also refuses Upgrade and Proxy-Connection; Connection uses the
accepted whole-list close/keep-alive policy. Native role alone permits its WS
Upgrade/Connection grammar after the same raw bounds. Account subprofiles continue
to refuse Upgrade/Proxy-Connection and apply ALL accepted account policies.
Duplicate auth/Origin/Cookie fields are NOT collapsed or treated as generic400;
SESSION-011 later owns their lossless capture and exact semantic decisions.

For independently complete heads, first structural/size envelope failures431
(line/target overrun414), then malformed/ambiguous framing400, then excessive
valid advertised body413; no callback or100. On incremental input an already
terminal violation need not wait for unseen bytes. Within the account semantic
phase retain the accepted complete reference projection4096 and all existing
431/413/400/415/406/auth priorities; a generic1MiB route must never widen that
account bound. Generic envelope admission does not claim account acceptance.

For ordinary HTTP acquire an I slot before parser reset or enabling the first
read of a new request, including a zero-body request; if unavailable, suspend
without parsing and keep the accept/idle deadline. The round-robin release wake
offers freed slots to eligible peers, at most one request per peer. Native
pre-Upgrade uses its per-peer head/projection region and existing U owner, no
ordinary input slot; its handshake requires body length0. This prevents one role
from exhausting the other's body-slot admission.

Each pre-Upgrade/ordinary peer has ONE MsgBuffer payload65536 plus its existing
8-byte prefix. The successor adds a non-owning fixed-region storage mode (no
vector allocation for that mode) to this existing MsgBuffer, using its indices
and readFd. Neither append nor ensureWritable nor retrieveAll may allocate,
resize, swap into a growing buffer or silently abandon the wiping seam. Before
EVERY syscall, on IO0: recheck time/state, process eligible pending bytes, compact
if necessary, then require writable≥8192. Thus actual readFd selects iovcnt1;
its8192-byte automatic array still exists on the IO stack but contains no read
payload, causes no spill copy and is not an extra per-peer heap allocation.
The entire writable interval, at most65536, is the read bound. No 1MiB raw owner.

Compaction uses memmove of the unconsumed suffix to prefix8, then the registered
wipe operation on consumed/obsolete ranges EXCLUDING the surviving destination;
reset indices only afterward. Before compaction borrowers must be out of visit.
Each consumed range, discarded invalid head/body and terminal entire payload has
one explicit wipe seam. The private bridge's exact future hook shape is:

```cpp
enum class RawKind { receive, fields, method, target, body, projection };
struct RawStorageHooks {
    void* owner;
    void (*wipe)(void*, RawKind, std::span<std::byte>) noexcept;
    void (*revoke)(void*) noexcept;
};
```

Install once before account-path admission through the selected artifact's
private capture bridge, never an HttpRequest attribute/public identity setter.
Revoke before any scope exit clears/wipes/reuses storage, including exception,
cancel, timeout and disconnect. Hook failure cannot continue admission. The
default foundation clears its fixed storage; this is
NOT the account secure-erasure claim. SESSION-011 must install selected secure
wipe before account admission, cover arena/method/target/body/projection as well
as receive, and prove no unhooked growing/unwiped path or stock pool survives.
No callback can retain a secret view through compaction/release.

Body bytes stream from this buffer into ONE separately reserved route slot after
complete head/framing admission. Copy only min(remaining_CL,readable); decrement
checked remaining; no temporary whole-body MsgBuffer/string/spill file. Partial
body callbacks are private fixed-sink writes, not route dispatch. If a request
is larger than receive capacity, consume/copy/wipe/refill within the same slot.
If writable<8192 and parsing cannot progress, disable channel reading and wait
only on an identified owner release/admission transition, otherwise close at the
original deadline; never repeatedly read/poll/grow. EAGAIN/EWOULDBLOCK/EINTR retain
indices/reservations and do not reset anchors. EOF cancels incomplete input with
zero dispatch; EOF after completion revokes undrained reply authority and tears
down. Other errors/exceptions cancel and remove, with all owners still charged.

There is ONE current request per connection through raw release AND response
terminal ownership. `onMessage` stops before parser reset, batching or construction
of a second request. Later coalesced bytes remain unparsed in receive storage;
when the first completes, reading is disabled even if spare capacity remains.
Only after raw released, ended reply (drain/cancel), callback exit, live peer and
admission=admitted may a single coalesced continuation parse the next head. The
continuation checks the same time/source/fence BEFORE reset/slot acquisition and
processes pending bytes BEFORE enabling reads. External suspension is level state;
repeated resume schedules at most one wake and cannot reopen a closing peer.
Cancellation/timeout/stop while suspended wipes pending bytes and removes it.
No generic idle timer protects a deliberately suspended active request: its
request/output anchors continue. This is APITEST-016's
`second_coalesced_request_after_release`; APITEST-015 separately proves the
host-admission composition, including release while host admission stays closed.

## 4. Finite reservations and fairness

The following are engineering requested-capacity charges, not measured
malloc/RSS. Fixed byte payloads use disjoint mapped regions and fixed slots,
without a variable-hole allocator. SDK metadata/native projection use the
separate finite allocation allowance below: reserve the WHOLE allowance before
construction, then each operation must fit its precomputed remaining subcharge.
Startup/source admission checks actual sizeof/alignof, selected stdlib object,
bucket/string rounding and simultaneously alive construction scratch. An unknown
allocation path blocks that build/profile; never allocate then see if it fits.
Charge each metadata allocation request rounded up to64 (including alignment
padding/control block), so its finite requested size is covered before new.
Per-peer/global16384/65536 and handle128 allowances include this rounding; they
are quotas, not claims that those are measured SDK sizeof values. Native projection
string character allowances instead use the power-of-two rule below.

Map only the fixed payload regions; do NOT also map the external metadata/string
allowances and hide a second physical allocation behind them. Payload regions
have exact capacities and no terminators; peer stride102424 is a multiple of8,
so the uint32 records align without gaps. The one mapping's page tail is charged
separately. Stacks, allocator bookkeeping/extra usable heap, kernel queues/FD
tables, TLS, framework singletons and OS scheduling remain separate admission
facts; there is no whole-process heap or hard syscall-latency guarantee.

| Fixed owner / reservation BEFORE use | Charge / maximum simultaneously alive |
| --- | --- |
| Every peer, HTTP or native, from preconstruction through actual removal |65544 receive +32768 arena +2064 method/target +1024 records +1024 control =102424 fixed mapped bytes. Additionally reserve8192 external native projection character bytes/peer (unused on HTTP). |
| Peer metadata allowance |16384 per peer, includes connection/channel/socket/context/parser, one request/reply handle binding, timers/deadlines/counters, native WebSocket wrapper when used, four128-byte mail records, at most one executing and one pending continuation, one removal record and fixed server membership entry. All allocation sites must preclaim slots; object/control-block sizes and any SDK fixed strings count here. |
| Global metadata |65536: two listener/acceptor bindings/idle FDs, profile/source references, one fixed route table (≤8 entries, each≤2048 target bytes), peer free/role tables, round-robin cursors, one scan timer, one mailbox wake plus executing predecessor. Require the actual layout to fit; framework singleton infrastructure outside these objects is separately inventoried. |
| Input slots |I slots, each B bytes and128 metadata; one per current ordinary request including zero-body requests, reserved BEFORE reset/head parse. No input copy API. |
| Reply slots |O slots, each E bytes body, W=E+128 bytes render, W bytes TCP pending copy, plus128 metadata. Full `E+2W` reserved atomically before fill/render, even for a short reply. One per peer; no detached second reply slot. |
| Control |ONE1024-byte slot already in each peer, no additional host1024 charge. It is either reserved for one HTTP failure/native101 or lent to the host's existing control owner, never both. Host must return it before a foundation control reply; otherwise close. No normal reply uses it. |
| Callback/queue cardinality |One request/fill/visit executing in IO0, one request per peer, at most P connected +P request +O reply handle states, fixed four mailbox records/peer; repeated intents coalesce. No std::function payload capture or per-tick registration. |

SDK objects with dynamic internals must either use the fixed region or have a
source-proven finite requested-capacity allowance BEFORE construction. For example,
bounded ordinary mode constructs NO HttpRequestImpl/maps/body strings and NO
stock BufferNode/list node. Replace connSet on the bounded branch with the fixed
peer-index registry; never charge an unlimited unordered set by live length.
Bound connection endpoint/name strings before construction (numeric addresses,
fixed formats only); reserve their selected ABI's capacity/terminator and control
blocks within16384, or use fixed arrays internally. Poller registration and fixed
loop queue entries also require preclaimed finite slots, including removal entries.
There is no exception fallback to an ordinary allocator/growing path.

The one native Upgrade projection is the only bounded-mode stock header map.
After complete raw validation, project ONLY configured literal path plus Host,
Connection=`Upgrade`, Upgrade=`websocket`, Sec-WebSocket-Key and
Sec-WebSocket-Version=`13`; no Cookie/auth/query/other raw fields enter it.
Require one bounded key24 ASCII bytes, Host≤256, and exact configured native path≤128;
unsupported extra WS negotiation is refused, not silently claimed. For the internal
host use `/api-host/ws`. The foundation fixture uses `/foundation/ws`. Handshake
uses the existing selected router/binder through a bounded synchronous entry, without general filters/static/session/
JSON/async controller operations. Its finite synchronous callback table forwards
native message/close events through existing native profile leases, not retained
HTTP requests. The projected request is destroyed before raw release/promotion
cleanup; a controller cannot retain it.

Preclaim all projected string capacities before set/insert: for each empty string
assigned once, charge `round_up_pow2(max(32,n+1))`, including terminator, and
source-review the SELECTED stdlib's construction/assignment capacity never exceeds
that allowance. Five header pairs plus path use at most8192 character bytes:
path256 + Host name32/value512 + other four name/value pairs8×32 =1056;
remaining7136 covers temporary lowercasing/key/accept/handshake strings and their
simultaneously alive copies. Enumerate those actual copies in the patch review;
they may not exceed8192 in total. Request map is pre-reserved for5 entries and handshake response map for3;
at most64 buckets and8 nodes combined, with at most2048 requested metadata bytes
INCLUDING allocator
rebind/control blocks, preclaimed from16384 before map construction. Source/ABI
admission must prove bucket rounding, node and string bounds; an unknown stdlib
is unavailable, never validated afterward by capacity inspection. No generic
all-header map projection is permitted. SESSION-011 account path has NO such
projection and must prove all secret copies/wipe seams separately. Native
projection is not lossless account capture or an authenticated Upgrade capability.

Reference: P128, HTTP quota80, native quota48 (native C32+U16), I8, O8,
B=E1048576, W1048704. Per-role ceilings sum to P and cannot be borrowed: HTTP
cannot occupy native's48 slots, native cannot occupy HTTP's80. All peers still
count in the SAME host P table and native C/U limits; no new128 allowance per
listener. IO0 claims the chosen role before constructing any peer object. A
single preallocated transient FD slot T1 is shared by both acceptors (not per
listener). Reserve it BEFORE accept; acquire a role/peer slot before construction;
otherwise close the FD without HTTP output/allocation, then release T. Exception
paths close it/release exactly once. EMFILE's existing idle-FD recovery also uses
T and cannot create an extra connection; two acceptor idle descriptors are fixed
startup infrastructure, not peers. Backlog/kernel memory is outside this claim.
A role-full acceptor is disabled until that role loses an owner; failed accepts
and refused descriptors have a bounded turn quota, not an infinite drain loop.

At each IO wake process at most8 accept attempts per role and at most8 peer
continuations per role in alternating HTTP/native order, advancing a round-robin
peer cursor; each peer turn scans/copies at most8192 input bytes and issues at
most one read syscall. Schedule only one shared continuation wake if work remains.
Output does at most8192 bytes/peer/turn, same alternation; control precedes that
peer's application output only before any bytes of a response have been emitted.
Never interleave a new error head into an already partially emitted response.
This provides finite work and reserved opportunities/capacity to both roles when
the loop runs and callbacks return. It cannot guarantee service under kernel
accept starvation, malicious peers holding ALL of one role's quota, a blocked
trusted callback or scheduler starvation. Native established idle policy remains
unchanged; finite HTTP deadlines alone are no no-starvation proof.

Default-constructed MsgBuffers in parser send/receive helpers must use the
fixed/empty constructor on this branch; suppress request pool, batch vector and
send buffer allocations rather than charging an unused stock2048 owner. The
selected parser/connection wrapper, shared control blocks, handshake response
object, function wrappers and their fixed metadata are all included in16384.
Native profile's OWN C/U/context/message resources remain separate and unchanged.

A tiny VALID composition is P4, HTTP2/native2, native C1/U1, I2/O2,
B=E4, W132. An even smaller P2/HTTP1/native1/I1/O1 profile is also valid; native
established/incomplete then share its ONE transport slot. General
coherence is P>0, each role>0, sum=P, I/O>0 and≤P, B/E>0, native quota≤C+U and
native C≤native quota; both roles need not simultaneously reach C and U. Reference
native48 can reach32+16; P2 can reach one native phase at a time. Explicitly set role
quotas with a changed host peer limit; don't reject the existing valid
response_bytes==response_capacity case or conflate B/E with native message bounds.
The new transport pools are ADDITIONAL to unchanged host17006592, native backing,
WS serialization scratch and source/storage allocations; no double billing of
host control or moving one reservation into another pool without actual transfer.
An installation may partition input/reply pools by finite trusted route profiles;
apply the same sums per partition. SESSION-011 reference uses B4096 AND complete
semantic4096 (never the generic1MiB admission), its existing64/32768/16/2048 limits
and separate secure metadata. This is a future account-layer profile, not enabled
by this contract.

At page4096 use two distinct checked quantities:

`L = P*102424 + I*B + O*(E+2*(E+128))` (fixed payload mapping).

`H = P*(8192+16384) + 65536 + (I+O)*128` (external allocation allowances).

`A=L+H` is the engineering reservation total before page tail; the total bound
on requested storage is `ceil_page(L)+H`, not a second mapping of H. Compute pages
as `L/4096 + (L%4096 != 0)` and checked pages*4096; don't evaluate an overflowing
L+4095. Check every sum/product/size_t/ssize_t narrowing before mapping or new.
No resource gate relies on currently untouched pages or RSS.

| Independently specified profile | L | H | A | Mapped payload / tail | Total including tail |
| --- | ---: | ---: | ---: | --- | ---: |
| Reference above |46666752|3213312|49880064|46669824 /3072|49883136|
| Tiny P4 above |410240|164352|574592|413696 /3456|578048|
| Smaller P2 above |205120|114944|320064|208896 /3776|323840|

These include unused slots and capacity slack, not observed allocation sizes.
Allocation failure fails startup without serving. Observation.reserved_backing is
A, mapped_backing is ceil_page(L); their difference is not live payload. Report
H and tail separately in admission evidence, and actual metadata/string request
subcharges against H. A move adds0; a forbidden Request/Reply copy has no API.
A bridge copy outside this boundary is charged additionally by its owning pool.

## 5. Response and network ownership

Ordinary replies accept status200 only in this generic interface; profile/control
failures use the fixed literals below. Owning future public routes require their
reviewed finite status/header profile before extending it. Body is opaque bytes,
no Content-Type/cookie/date/server/compression/redirect/CORS/default page. Serialize
exactly `HTTP/1.1 200 OK\r\nContent-Length: ` + canonical decimal length +
`\r\nConnection: keep-alive\r\n\r\n`, then body; close=true uses `close`.
128 header bytes covers all U64 decimal lengths; E still must fit size_t/ssize_t
and selected limits. Render fills ONE W slot without terminator/growth. A fixed
TCP node copies to ONE other W slot before returning render ownership. Reserve
both and E while all three may coexist; no third merged/cached/off-thread copy.
The TCP slot remains charged until its node is actually destroyed after final
user-space drain or teardown, even when the host's local-handoff lease has ended.
A successful send/empty node proves no peer receipt. Stop cannot create an
uncharged reply queue or append new app data after a revoked host handoff.

Control literals are fixed empty responses with Content-Length0/Connection close:
400 `Bad Request`, 413 `Payload Too Large`, 414 `URI Too Long`, 431
`Request Header Fields Too Large`. Wire lengths are66/72/67/86 respectively.
No arbitrary error string, rich body,100/417/405/default output or reflected field.
If control slot is already held/unavailable, close without a reply. If ordinary
output has started, close without another response. If it has not started, cancel
its reserved reply before control; no second retained output queue. Force-close
on control write failure, output deadline, time failure or stop deadline. A
control response, including pre-Upgrade400, owns its control slot through actual
node removal. Native101 uses the same control reserve, with the existing fixed
accept-key handshake and total rendered≤1024; reserve before constructing its
projected response and account all transient handshake copies in §4. Never use
unbounded `utils::formattedString` parser output or stock append on this path.

Parser/error/refusal/route default are all fenced inside the bounded branch.
Import the listener contract's four exact HTTP POST paths/default allowlist,
static/homepage/upload/plugin/log/decompression disabling and private scratch
policy; the bounded route table replaces late stock routing for this branch,
while the existing global sync advice remains defense in depth for other paths.
The public enableReusePort(true) probe bypass AND actual pre-listen clear/readback
SO_REUSEPORT0/SO_REUSEADDR1 remain the listener owner's choice, not a second socket
policy here. Canonical numeric address/data-only actual endpoints, private bounded
scratch parent/child and one shared stop remain unchanged. Parent kernel/listen
exit1 oracles are UNVERIFIED; reading source is no kernel/exit proof. APITEST-015
retains the SO_REUSEPORT-occupying-listener variant CC-A1 without weakening its
plain occupied-port assertions. Terminal scratch cleanup/dedicated owned parent,
PrivateTmp and restart concerns CC-A4 belong to DEPLOYMENT-007/008, not an SDK
file manager or a graceful-cleanup claim after exit1.

## 6. Elapsed anchors, native transition and teardown

All owners retain the EXACT SAME source object supplied to host/native profile;
identity comes from installation, not equal observations or a public identity
string. Per IO owner nonregression, checked U64 nanosecond conversion/addition,
expiry at equality and unavailable/regressing fail-close import host §4. Timers
only wake. Disable ordinary stock idle kickoff for opted-in connections; its
Trantor-clock timing wheel may at most be an earlier safety backstop, never proof
of shared elapsed deadlines. Use one fixed scan wake (reference≤10ms, tiny≤1ms)
that rechecks each live deadline; timer lateness does not extend an anchor.

| Phase | Absolute anchor / reference duration / tiny duration |
| --- | --- |
| Accepted, no request bytes | Accepted FD promotion observation,5000ms /50ms. Include preconstruction time by sampling while T is held; never reset at connection callback. Native also keeps an outer bound anchored to this SAME acceptance sample plus its configured native upgrade_ms, continuing until establishment; the frozen native timer remains an independent backstop within that phase. |
| Between completed HTTP requests | Actual predecessor reply terminal AND raw release (later of the two),5000ms /50ms. If bytes are already pending, immediately start next eligible request at that transition, or retain suspension under this idle deadline. Resume doesn't restart it. |
| Incomplete/active HTTP request | First positive read observation for the admitted request,5000ms /50ms (or the current attribution turn for an already-buffered successor). Includes parsing, body, suspended request and waiting for reply commit; no reset on trickle/head completion/body chunks. Already-coalesced successor starts only when predecessor ownership/admission allows attribution. |
| Output drain | First accepted reply commit/control failure decision,5000ms /50ms. Request deadline ceases only at successful commit; retain earlier host handoff/stop limits when composed. No reset on EAGAIN/write progress or last host lease release. |
| Stop | First absolute stop deadline supplied by owner, host reference drain5000ms; subsequent calls can only keep/minimize it. All acceptors/ordinary/native peers share it; native Close still uses its independent original1000ms. |

Accept/idle expire by close with no manufactured request/output. Incomplete or
suspended request expiry closes with zero new dispatch; after dispatch it revokes
response authority, cannot undo domain work. An elapsed failure tears down that
owner on its next IO turn; the independent native owner applies its own checks.
No hard deadline for a kernel call, synchronous trusted callback, SQL or OS
scheduler is promised. finish refuses while any actual cleanup/handle remains.

Native **pre-Upgrade changes** only on the opted-in listener: it uses these raw
bounds/fixed receive/preconstruction/output control guarantees rather than
falling through unbounded stock HTTP. Existing native `Session::read` returns
false before establishment: TcpConnectionImpl must then choose this bounded
read branch, never stock fallback. On a complete valid native Upgrade stop reads,
validate role/route, project as §4, invoke existing beginUpgrade/bind on IO0,
serialize101 into charged control and call the existing initial read-ahead handoff.
A non-Upgrade complete native request still gets the exact66-byte400 and close,
zero ordinary dispatch. An ordinary Upgrade still gets that refusal, no stock WS.

At bind the native algorithm checks existing MsgBuffer capacity PLUS
`R=min(raw_capacity_bytes,4096)` against raw_capacity_bytes. Account the fixed
65544 region while nonempty. Empty read-ahead releases its MsgBuffer view/storage
before bind; this MUST NOT free/reuse the peer backing prematurely. For nonempty
read-ahead reference native raw1048590 fits65544+4096=69640; preserve the original
native check, initial consume/release order, and its continuation/resource refusals.
Small native raw≤4096 has no spare handoff capacity with R=raw: as historically,
only empty read-ahead can promote; coalesced data may refuse. Do not silently raise
raw_capacity_bytes, change API-012 snapshots or require65544 for an empty-handoff
tiny profile. Source review must distinguish mapped reserved peer backing from
native-reported ASSIGNED handoff capacity; no double assignment after release.
Established reads/frame/control/message/Close continue in the frozen native
producer; no HTTP idle/request timers apply after successful transition. Shared P
slot remains held alongside native C owner until BOTH actual removals/leases end.

Stop closes both acceptors on IO0, latches admission, revokes pending request/reply
work, drains already committed nonsecret output until the earliest applicable
absolute bound, then removes sockets/channels/nodes. Pending wakes hold only
reserved peer slots; after revocation they perform cleanup only. Erase server
membership before connectDestroyed, retain removal reservation through that
queued callback and its destructor; disconnected advice alone releases nothing
still referenced. Scan timer/mailbox registrations are invalidated and actually
removed or retained as bounded inert context through framework exit. One IO join
and actual framework-main return, zero callbacks/nodes/handles and destroyed
acceptors precede finish's cleanup acknowledgment. The caller retains owner/source
until it receives that acknowledgment and destroys Foundation on its owner thread.
No callback retains raw host tickets, borrowed secret views or destroyed host
pointers. A stuck callback gives incomplete cleanup, never permission to destroy
its dependencies or claim stopped. One process startup only; no reset hook.

## 7. Independent literal oracles and handoff

Octet notation below uses actual CRLF for `\r\n`; repetitions specify literal
ASCII bytes, no client library normalization. These are planned expected values,
not implementation-generated fixtures or executed results.

| Family | Independently derived stimulus and oracle |
| --- | --- |
| F01 positive wire | `POST /f HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\ndata` is52 bytes:18 line+9 Host+19 CL+2 end+4 body. Tiny fixture compiled route `/f`, callback once/body `data`. Reply `diag` is66 bytes:17 status+19 CL+24 Connection+2 end+4 body. HTTP200/length4; wire budgets W132, payload4 and all three output capacities268 reserved BEFORE fill. |
| F02 line/target | `GET ` + `/` followed by2032 `a` + ` HTTP/1.1\r\n` is2048 bytes:4+2033+11. Complete exact eligible by raw-line bound; one extra `a` gives2049→414 before setPath. Also split before final CR/LF and send complete oversize in ONE read. A method of16 token bytes fits method slot but a method17 refuses before storage; an otherwise allowed method is needed for a positive route control. |
| F03 fields/raw head | Base `GET /f HTTP/1.1\r\nHost: x\r\n` is26 bytes. Add `X: `+4092 SP+`a\r\n` EIGHT times, then `Y: `+8134 SP+`a\r\n`, then final CRLF:26+8×4098+8140+2=40952. Add8 SP to Y→40960 eligible, add9→40961→431. Fields10; semantic H=2+9+8×6+6=65, so raw bound is isolated. Complete one-read and every CRLF boundary have same oracle. |
| F04 count/semantic | Host plus63 occurrences `X:\r\n` gives64 fields, one more→431 before record65. Per-field `X: `+8191 `a` gives name+value8192 exact,8192 `a`→8193 refuses. For aggregate H32768 use Host (9), three `X` values8191 (3×8196), and `Y` value8164 (8169):2+9+24588+8169=32768; next value byte→32769 refuses before copy. Raw total remains below40960. |
| F05 framing | F01 CL changed to5 under tiny B4→413 before body reservation/copy; CL4 exact. CL `+4`, `4x`, empty,18446744073709551616, duplicate0/00 and any CL+TE→400; zero/0000 positive with empty body. Duplicate X-Host-Correlation44/44→400 before map. Any TE/Trailer/Expect including empty/100-continue→400, exactly66 bytes, NO100 prefix. Colonless, obs-fold, LF-only, whitespace-before-colon and NUL→400/close, zero map/route calls. |
| F06 one current | Send two literal F01 requests concatenated104 bytes in ONE write. Hold Request1/raw or Reply1 or admission suspended: requests=1, stock_batches=0, no parser reset/request2. Release raw1 and drain reply1 while still suspended→still1. Resume once→request2 once, pending bytes processed before read. Duplicate resume adds no wake. This is APITEST-016 `second_coalesced_request_after_release`; APITEST-015 owns the corresponding Host fence. |
| F07 fixed receive | At indices head8/tail57352 writable8192: next read permitted with iovcnt1, max8192. Tail57353 leaves8191: no syscall until eligible compaction/consume; never iovcnt2/spill. Full65536 coalesced payload has zero writable; pending suffix preserved only in that region. Trickle/EAGAIN/EOF/throw/cancel/remove/suspend/resume cannot increase65544 or dispatch incomplete body. Stream generic body1048576 through repeated reads; peak raw65544, body1048576, one body slot, no second whole-body scratch. |
| F08 output/resources | Tiny P4 reply body4: E4+2W264=268 bytes, metadata128, total396; two slots792. A next request cannot acquire a third slot even if logical bodies are empty. Release one actual TCP node plus metadata→396, then new output eligible. Host local handoff alone leaves396 charged if its network owner survives. A move adds0. Partial emitted response followed by failure→EOF, never a second400 head. Full/held control→EOF, no allocated fallback. |
| F09 peers/fairness | Reference HTTP80 peers occupy only80/P128; HTTP81 refuses before TcpConnection construction; native still has48 transport slots subject to C32/U16. Tiny HTTP1/native1 total2; third FD gets T1 then closes, zero parser/context/map; peak constructed2, transient FD1. Releasing HTTP advice alone does not free its slot until connSet/channel/callback removal. HTTP flood cannot consume native reserved slot or unbounded IO-turn callbacks. No stronger kernel fairness oracle. |
| F10 time | Accept1000ms/reference5000→6000ms, no bytes:5999 eligible,6000 close. Request first byte1100→6100 regardless of more bytes6099. Commit at2000→output7000; progress6999 doesn't extend. Reply terminal/raw release3000→idle8000; repeated resume7999 doesn't extend. Tiny anchors add50. Repeat stop5999 cannot change stop6000. Native establishes idle→no HTTP idle expiry. All sample same object; per-owner100→99ns closes, independent other-owner99 isn't falsely ordered. |
| F11 arithmetic | U64 max18446744073709551615ns +1ms fails checked addition;18446744073709ms converts to18446744073709000000ns, next ms overflows. For an isolated checked product,166763796138981×110616=18446744073709522296 fits U64; one more unit produces18446744073709632912 and must refuse BEFORE multiplication. Other profile terms can overflow sooner, so this is an isolated checked-arithmetic oracle, not an admissible peer configuration. Zero limits, role sum overflow and SIZE_MAX/ssize_t narrowing fail before mapping/listen. Page example A4096→4096, A4097→8192; U64_MAX ceil-page multiplication fails rather than wrap. |
| F12 cleanup/identity | EOF with3 of4 body bytes→0callback; callback throws→no reply, all owned slots released after actual teardown. Held inert handle prevents finish acknowledgment until destroyed. Wrong-thread/reentrant/revoked visit/commit refuses with no underlying read/copy. Bad successor seal/ABI/loaded map refuses BEFORE bodies, which is admission failure, not intended red. Old API012 receipt cannot admit new DSO. |

APITEST-016 independently reviews these literal derivations before using them;
expectations must never be learned from the implementation. Account A–D/I01–24 remain SESSIONTEST-011's exact policies/oracles,
including native149/237/241/199/277 controls, complete4096/4097 and erasure/log
canaries. Foundation success supplies no account capture/policy proof.

Planned owning paths and gates are finite:

| Owner | Future paths / required proof |
| --- | --- |
| APITEST-016 author after this contract's closing approval | `server/include/subathon/server/bounded_http.hpp`, `server/src/bounded_http.cpp` explicit NI declarations/stubs only; `tests/bounded_http_foundation/bounded_http_probe.cpp`, `tests/bounded_http_foundation/test_bounded_http.py`. Small actual-framework child/loopback fixture with one profile/start per child, finite command/event/log/maps/peer/watchdog/reap budgets fixed in test review. No API-001/Qt/SQL dependency. |
| API-016 fresh implementation | Same first-party producer plus the EXACT selected patch sites in §1, including the successor-only native first-party build copy; original native source stays byte-identical. Preserve reviewed literal tests. Native message callbacks still claim existing Profile leases; no app echo becomes a protocol requirement. Generic snapshot/fault instrumentation must observe reservation-before-construction/read/render/append/release, not infer it from RSS. |
| Coordinator | Proposed NEW `dependencies/bounded-http-foundation/{manifest.json,0003-trantor-bounded-http.patch,0004-drogon-bounded-http.patch,0005-native-source-binding.patch,README.md}`; new SDK prefix/provenance; `cmake/DrogonDependency.cmake`, dedicated fixture CMake/preset/acceptance registration, `tools/native_websocket_ci.py` or separate owned admission recipe, selected CI and license notices. Keep historical native files byte-identical. Names are PROPOSED identity, not existing files/hashes. |
| SESSIONTEST-011/SESSION-011 | Rebase onto actual admitted API016 source/seal; own occurrence capture ABI, secure wiping, pre-log suppression and accepted account subprofiles, independently of API001. SESSION010 owns source/policy binding later. |
| APITEST-015/API-001 | Host two-endpoint composition, all75 Qt/18 native/HL01–17 and listener L01–13, real lifecycle/current-source/SQL/bridge/fixed shared-stop gates. Generic fixture cannot certify host admission or revise old outcomes. |

Test-first ordering needs a compiled meaningful missing-behavior red. After
contract approval, minimal NI stubs can link alongside the admitted historical
framework and expose `NI` to explicit required-positive assertions, with working
real sockets/legacy positive control. They perform no foundation work; failure
must identify the required unimplemented producer, not accept NI or fake output.
Where a test needs later SDK counters/hooks, keep that feature's intended-red
assertion at the first missing producer boundary until the separately reviewed
SDK seam can actually compile. Minimal declarations/NI vendor seams, if necessary,
need source/ABI/admission review before use, contain no parser/transport fix and
belong to the disclosed baseline setup packet. Missing API, build/import failure,
absent native fixture, skipped test or all-refusal control is NEVER red. Independent
test/fixture/stub review precedes execution; independent source/resource/code and
artifact/runtime review follow implementation, with all44 unchanged compatibility.

Keep the selected Release/shared/C++20/OpenSSL16-option recipe from candidate4;
no TLS runtime is enabled by linking OpenSSL. Upstream MIT/BSD, orm_lib/COPYING,
GPL-3.0-or-later correction provenance and actual direct/transitive notices remain.
The new manifest binds old exact inputs plus ordered NEW vendor/first-party patches, original and
derived first-party bytes (no mixed original/derived duplicate symbols), ABI marker `DROGON_SUBATHON_BOUNDED_HTTP_V1`, Trantor additions, source options
and exact installed inventory. Actual hashes/build IDs are generated only from the
reviewed build, not invented here. New consumer gates follow existing recipes:
passive complete ELF/interpreter/DT_NEEDED/symbol-version/RTTI/ABI closure before
loading, scrubbed controlled identity only after release, actual complete loaded
maps at initialization/serving/cleanup, unchanged seal throughout, then bodies.
The native44 successor consumer and foundation fixture EACH require admission;
old candidate4 load receipts cannot be reused after a binary change.
In particular the selected source's stock projections, callback allocator sizes,
loop/poller cleanup and bounded rendering need source/ABI proof against these
fixed allowances before admission; this contract does not infer that proof from
syntax, a getVersion string or successful configuration.

Authored design/source observations/literal arithmetic are the only evidence
level of this packet. No headers, patches, SDKs, tests, native processes or network
writes are authored/executed here. Independent Claude initial, Codex fixes and
separate Codex verification, then Claude closing must freeze this packet before
test authorship. Source/API feasibility, metadata/layout/stdlib rounding and
real selected behavior remain those reviews' and API016's gates, never assumed
capabilities. API001 draftPR121 source70ea261 has scoped approval only; current
main host stubs remain unimplemented. All SQL, CI, platform/provider/deployment/
release gates, seven integrations, three FX providers, English/German, themes,
separate renderers and desktop full editor remain in complete v0.45 scope.
