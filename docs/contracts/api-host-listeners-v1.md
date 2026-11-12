# Isolated internal API host listeners — proposal v1

APICONTRACT-004; identity `subathon.api-host-listeners.v1`. Documentation only,
pending independent review. Authority: specification v0.45 §§2/10/16/17/18,
SERVER-01, API-01, DEP-02/05. Author base `8ca723e`, parent main `bd99912`.

Import [host lifecycle](api-host-lifecycle-v1.md), ALL HL01–17, and the complete
[consumer boundary](api-host-lifecycle-boundary-v1.md). Authority, source,
generation, transaction-finalization, publication, clock and bridge byte/slot
accounting remain binding. The explicit amendments are enumerated in §6; this is
more than a same-port correction. [API-001 review B1](https://github.com/Kernol12/subathon-bot-ai/pull/121#issuecomment-6816497937)
exposed the incompatible baseline; [initial contract review](https://github.com/Kernol12/subathon-bot-ai/pull/122#issuecomment-6817347262)
requires these corrections. Mechanisms below are prospective, never runtime proof.

“Frozen API-012” means its historical candidate4 artifact, patch, pins and all44
methods are IMMUTABLE, including
`test_marked_listener_refuses_complete_http_before_ordinary_dispatch`. Execution
will require a NEW additive candidate4 successor from
[APICONTRACT-005](../tasks/EPIC-008/APICONTRACT-005.md) →
[APITEST-016 / API-016](../tasks/EPIC-008/API-016.md), with its own reviewed patch,
pins, license/build/ABI/artifact identity, all44 unchanged compatibility methods
and fresh new-consumer passive ELF/ABI/controlled identity/loaded-map admission.
This chain is planned, not implemented. API-001 AND APITEST-015 depend on API-016;
so do SESSIONTEST-011/SESSION-011. The foundation has NO API-001 dependency.
It is on the critical path to API-001 and its dependent SESSION-009/010, API-002,
API-013 and DEPLOYMENT-007/008; dependency readiness grants no implementation waiver.

## 1. Two endpoints, one host

Use TWO internal TCP listeners in the SAME Drogon application/framework lifetime,
exactly ONE IO thread, the same Qt owner/writer/Ingress, shared peer and association
tables, and ONE host-owned elapsed source. Register ordinary HTTP with
`addListener(..., nativeProfile=false)` and native WS with `nativeProfile=true`;
both use plaintext internally. No second host, parser, proxy, authority, framework
run or independently constructed native profile is permitted.

Minimal later declaration amendment in `api_host_drogon.hpp`:

```cpp
struct DrogonOptions {
    std::string address = "127.0.0.1"; // both listeners
    std::uint16_t port = 0;           // native WS; preserves existing WS callers
    std::uint16_t http_port = 0;      // ordinary HTTP; append after existing fields
    std::string scratch_parent = "/tmp"; // existing absolute parent; see §3
};
struct ListenerEndpoints {
    std::array<char, 46> address{};  // canonical numeric bind address, NUL terminated
    std::uint16_t port = 0, http_port = 0; // ACTUAL nonzero bound ports
    std::array<char, 534> scratch_root{}; // actual private child, NUL terminated
    // Actual pre-listen readback: bit0 SO_REUSEADDR, bit1 SO_REUSEPORT.
    std::uint8_t native_socket_options = 0, http_socket_options = 0;
};
using ListenersObserver = std::function<void(const ListenerEndpoints&)>;
// Existing one-argument calls remain valid; no observer is required in production.
Result<std::unique_ptr<Transport>> make_drogon_transport(
    DrogonOptions, ListenersObserver = {});
```

Preflight the complete address before framework construction: 1–45 ASCII bytes,
numeric IPv4 or IPv6, no hostname, brackets, zone ID, embedded NUL or whitespace.
Require strict `inet_pton` IPv4/IPv6 syntax, canonicalize with `inet_ntop`, and
pass that canonical text to `addListener`, without DNS. Thus `127.1` refuses
before Drogon can abort. Explicit wildcard `0.0.0.0`/`::` binds are allowed;
the default stays loopback. Both endpoints use that same address/family. A wildcard
observation is a bind address, never a client destination or public-origin claim.
Deployment must separately restrict exposure; no forwarded header changes it.

Port0 asks the OS for an ephemeral endpoint, independently for each listener;
it never disables either. Nonzero ports must differ. Duplicate fixed tuples and
invalid addresses fail factory validation as `invalid_options`, before any bind.
Register fixed endpoints before ephemeral ones (HTTP first when tied), retaining
their role/order mapping; an ephemeral bind cannot knowingly consume the other
requested fixed port. A released free-port candidate is not readiness. Do not share
the endpoints or retry a failed bind in process.

On selected Linux, `HttpServer` uses `TcpServer` defaults SO_REUSEADDR=1 and
SO_REUSEPORT=1; `app().reusePort()` controls the probe, not those socket defaults.
Call PUBLIC `enableReusePort(true)` separately (it returns void), skipping the
probe and `/tmp/drogon.lock`. Install `setBeforeListenSockOptCallback` to clear
SO_REUSEPORT on EACH actual socket; verify SO_REUSEPORT=0 and SO_REUSEADDR=1 before
`listen`; capture those actual readbacks for §2's data observation (each bitset1).
An option failure/readback mismatch is fail-closed process-terminal startup
before listen (`_Exit(1)`), never return-and-listen or a ready result. The selected
`Acceptor` binds first and calls this callback just before `Socket::listen`:
the sequence is available, but its kernel effects are NOT verified here.
APITEST-015 must observe both socket options, no lock-file effects and the original
plain-socket occupied-port failures for BOTH roles; inability to meet those oracles
blocks this profile, not permission to weaken HL11/L07. Lock avoidance alone needs
no SDK correction; the four pre-retention gaps in §4 still require API-016.

Any failure prevents combined readiness. Selected Trantor bind/listen failure
calls `exit(1)`: either occupied fixed port, including failure after the other
bind, is process-terminal HL11, with no graceful-barrier/destructor claim. The
parent reaps it and only a NEW child retries. Other startup failures/early stop
retain the one-attempt rule and real cleanup obligations. Factory `invalid_options`
before any Host exists consumes no nonexistent Host start attempt; Q may validate
such factories without constructing a Host/profile. Once actual Host::start is
attempted, its failure/stop cannot permit another attempt in process. The source
`exit(1)` also runs static destructors while Qt may be active: exact status1 remains
the preserved test oracle, NOT a guarantee derived from reading the call site.
Actual status/teardown observation is required for both roles.

## 2. Actual readiness and connection ownership

`registerBeginningAdvice` precedes listen; it is NOT readiness. On selected Linux
source, `createListeners` creates servers in registration order per IO loop and
`getListeners()` returns their actual bound addresses (including port0 results).
With one IO loop require exactly two, matching their configured roles/address,
distinct nonzero ports and requested fixed ports. An unexpected/missing/extra
endpoint refuses startup; no partial endpoint event or serving admission.

Use the available loop queues: beginning advice queues a framework-main-loop
continuation; the framework then calls `startListening`, queuing both actual
`TcpServer::start` listen operations to IO0. That continuation queues an IO0 marker
AFTER those operations. Only that marker, after both listens return, can acknowledge
the pair to Qt. No timer, successful bind alone, fixture socket or setter supplies
this barrier. The reviewed API-012 fixture already uses this ordering in
`tests/native_websocket_profile/native_ws_probe.cpp:599–660`; it is fixture/source
evidence, not production host proof. Retain callback dependencies and check
stop/failure at every stage;
late markers cannot revive admission. The owner emits one combined `listener_ready`
and startup success only after this actual pair and its current lifecycle gates.
Storage activation remains the separate unchanged real inspection/read/COMMIT chain.

Deliver the endpoint observer at most once on Qt, before startup-success delivery,
only for that acknowledged pair and before serving notification. It receives one
fixed-size nonsecret value, borrows no transport/source and grants no authority.
If the endpoint observer throws, deliver `Failure::unavailable` to the startup
observer on Qt, then follow ordinary failed-start cleanup; no startup success or
serving notification. Observer exceptions never escape an event loop. Absence is
allowed; no production setter/query manufactures the observation. After stop an
old copy is historical data only.
N may serialize exactly one `listeners` record, e.g.
`{"event":"listeners","address":"127.0.0.1","port":41001,"http_port":41002,"scratch_root":"/tmp/api-host-X/subathon-host-ABCDEF","native_socket_options":1,"http_socket_options":1}`.
Its serialized cap is4096 bytes, including JSON escaping (root≤533 UTF-8 bytes,
address≤45); it fits the EXISTING event/1MiB cumulative pipe budget. Reserve the
fixed record before serialization; retain no growing observer history.
The option bitsets come from each real pre-listen callback's `getsockopt` readback,
not configuration intent; N asserts both1. They grant no socket handle or setter.
The driver waits for this real record AND serving, checks fixed-port equality,
and selects `http_port` for HTTP, `port` for WS. It never reports requested port0.

`registerNewConnectionAdvice(remote, local)` has no listener tag or connection
handle. `setConnectionCallback` DOES supply the actual connection and local tuple.
Classify by actual local port/family against the verified endpoints, also matching
the address for a specific bind; a wildcard matches its concrete local addresses
(including IPv4-mapped IPv6 on an actual dual-stack socket). Unknown tuples close.
Use one peer table/cap across both, including idle, partial and refusing peers;
each admitted TCP connection owns one slot/control reserve through actual removal.
Ordinary peers never acquire native upgrade/message slots; native peers retain
all native budgets. Do not classify by client Host/path/Upgrade text or treat an
untagged advice as proof of listener kind. SDK listener flags enforce separation
before application advice: native ordinary HTTP and ordinary WS Upgrade receive
the fixed400/close refusal, never ordinary dispatch or stock WS parsing. Host
callbacks also verify role before Ingress. Refused/unadmitted connections retain
framework costs until removal; the pre-callback gap is NOT covered by host slots.

## 3. Explicit internal HTTP surface

Keep only POST `/api-host/{diagnostic,installation,credential,account}` (the four
literal paths, no pattern endpoint). Keep their existing RouteIds, lowercase-hex
`X-Host-Correlation` (1–256 decoded bytes),
opaque body and every existing success/refusal/exception assertion. WS remains
GET Upgrade `/api-host/ws`, with unchanged binary correlation/RouteId/body records.
No credential/account authority or public production endpoint is created here.

Install a synchronous `registerSyncAdvice` allowlist, before routing, decompression
and built-in OPTIONS handling. Advice is GLOBAL: determine role from the actual
`req->localAddr()` using §2's verified tuple rules before inspecting paths. On
ordinary HTTP allow only those exact POST paths, matching BOTH `getOriginalPath()`
and `path()`, with empty `query()`, no Upgrade,
Content-Encoding, Transfer-Encoding, TE, Trailer or Expect, and bounded framing/
body/correlation. API-016 must enforce the selected profile's framing/occurrence
checks BEFORE map insertion and any100 response: duplicate Content-Length or
X-Host-Correlation cannot be detected from the lossy map, and candidate4 sends
100 Continue before advice. Reject any transfer-coded request (including a retained
TransferEncoding representation), TE/Trailer/Expect occurrence on this internal
HTTP profile; no interim100 is authorized. A late advice check is only defense in
depth. This internal profile does not change account-layer error priorities.
Do not invoke multipart/upload/form/JSON decoders. Other complete parsed requests, including
GET/HEAD/OPTIONS/PUT/DELETE/PATCH, `/`, files, unknown paths and escaped aliases,
receive one empty400 response then close, at most1024 serialized bytes. No redirect,
index, Allow list, reflected input or framework error page. When the fixed reserve
is unavailable, close without a reply. Parser failures retain their bounded SDK
status/EOF handling; they cannot reach a command or expose an input-derived body.
Native Upgrade advice allows only the existing WS binding and native profile.

Also select `setFileTypes({})`, `setHomePage("")`,
`setImplicitPageEnable(false)`, no static locations, and a fixed empty400
`setDefaultHandler`; a default handler alone DOES NOT disable static lookup.
`scratch_parent` selects DATA only: an existing normalized absolute UTF-8 path of
at most512 bytes, no NUL, dot/dotdot components or symlink components. Require an
owner-only parent owned by the service UID; the default `/tmp` instead requires
the trusted root-owned sticky OS-temp directory. N passes its existing private
`fixture_root`. Before run, transport creates exactly ONE unpredictable child
`<parent>/subathon-host-XXXXXX` with `mkdtemp`-equivalent exclusive creation and
mode0700. Child length≤533 bytes; create child/empty and child/upload before
selecting `setDocumentRoot(child/empty)` and `setUploadPath(child/upload)`.
Observer §2 reports this actual child as data only.
No option/observer chooses files or changes readiness/authority.

The scratch subtree has at most260 directories: child, empty, upload, upload/tmp
and the SDK's256 upload/tmp/00–FF leaves. Production creates no files there and
no route may serve it. N may add exactly four16-byte `PRIVATE-SENTINEL` files:
`child/empty/index.html`, `child/empty/secret.txt`, `child/secret.txt` and
`child/upload/tmp/00/secret.txt` (64 bytes total; longest path≤558 bytes).
Transport removes its subtree after actual joins on normal cleanup. For exit1,
option failure or watchdog kill, the N parent/supervisor removes the at-most-one
child under its dedicated parent after reaping; it does not need a ready event.
Supervised production failure cleanup likewise uses a dedicated owned parent;
a terminal default-temp child has no in-process cleanup guarantee. Never scan/
remove unrelated shared-temp entries or create an unbounded subtree.
Select `enableGzip(false)`, `enableBrotli(false)`, `enableDateHeader(false)`,
`enableServerHeader(false)`, `disableSession()`, `enableRequestStream(false)` and
`enableCompressedRequest(false)`, no plugins,
framework DB/Redis clients, dynamic views, access/file logger or config loading.
Use `setLogLevel(trantor::Logger::kFatal)`; no request/credential logging.
Future asset delivery, public JSON, webhooks, auth, TLS and Caddy/same-public-origin
routing require their owning reviewed profiles; they are not deleted from release.

Set `setClientMaxBodySize(envelope_bytes)` AND
`setClientMaxMemoryBodySize(envelope_bytes)` to avoid intentional body spill files.
Keep these existing body setters as backstops; they do not provide the foundation's
pre-retention guarantees. ONE current HTTP request is admitted per connection.
Later pipeline bytes stay UNPARSED inside the bounded receive owner until the
previous request/response ownership is released and the generic admission gate
permits the next. No parsed batch, pooled raw duplicate or second outstanding HTTP
response is authorized. Preserve sequential keep-alive diagnostics and positive
old behavior. Native WS retains its independent existing per-peer associations.

## 4. Resource accounting and allocated foundation

The consumer's reference bridge charge17006592, source storage, fixed tables and
native-profile frame/control budgets remain binding; two listeners do not double them.
One serialization scratch of `response_bytes + 1` (1048577 reference bytes for
WS correlation) is an ADDITIONAL fixed transport allocation, not a permanent
reservation taken from the response pool. Checked sizing must accept the coherent
case `response_bytes == response_capacity`. HTTP framing/rendered copies and TCP
output are additional while live; charge allocated capacities, not just lengths.

APICONTRACT-005 selects the independent exact numeric profile, head/line inclusion,
output/copy/metadata/global arithmetic and concrete deadline durations before its
baseline is authored. Remove the earlier §4 speculative ceilings/totals: no
one-MiB raw body buffer or two-outstanding-request acceptance is selected here.
Align with accepted account ingress: one fixed65536-byte receive payload (+existing
8-byte prefix), at least8192 writable bytes BEFORE each read, one current request;
process pending bytes, suspend, or close rather than grow. A body may stream from
that receive owner into separately bounded route storage. This is a proposed
API-016 mechanism, not an upstream setting or completed account capture/erasure.

Require finite checked reservations BEFORE line/header-map/request construction,
read/copy/growth, callback retention and response render/append, per connection/
association AND globally, including rejected/transient connections, capacity
rounding, fixed scratch and all simultaneous copies. Existing host peer/control/
association/payload slots remain shared; two listeners do not double them. Network
output reservation survives host handoff until actual drain or teardown. Local
handoff expiry is NOT network-output drain or peer receipt. Separate finite accept,
idle-between-requests, absolute incomplete-request and output-drain anchors must
use the SAME host elapsed source. No ordinary HTTP no-byte/trickling/between-request
peer may remain immortal; established idle WS retains its native policy. API-016
owns the concrete anchors/suspension interface and its independent
real-stack fixture; API-001 composes them with host stop. Socket callback timers can
anchor acceptance/idle without inventing a first-byte application hook.

The unchanged `OwnedReply`/`OwnedDomainResult` interface lets trusted dispatchers
allocate before returning. It supplies NO allocator/preallocation enforcement for
that caller-owned result. Their bounded-result precondition and source review
remain required; host checks/reserves its OWN retention/copies before taking them.
Do not claim the frozen interface retroactively bounds dispatcher allocations.
The existing1024-byte per-peer control slot is not charged twice. Framework/kernel/
TLS/allocator overhead is separate from bridge accounting, not an exact RSS claim.

Read-only source observations from the coordinator-selected
`drogon-1.9.14-api012-candidate4-source` (not loaded or executed):

| Actual mechanism | Gap / selected resolution |
| --- | --- |
| Trantor `TcpConnectionImpl::readCallback`, `MsgBuffer::readFd` | Stock readv uses8192-byte stack scratch plus raw writable space, then grows; no public pre-read capacity setter. API-016 supplies fixed receive/suspension. Native pre-Upgrade `read()` can fall through to stock input/parser too: the successor must explicitly bound that phase per listener or prove it unchanged, while preserving all44 native semantics. |
| Drogon `HttpRequestParser::parseRequest` | Body setters check advertised/cumulative body before reserve/append, but header fields accumulate without a total/count bound. The64KiB line/header check runs only when CRLF is absent. Sync advice is AFTER parsing; it cannot supply a pre-map/request-line bound. API-016 owns this correction. |
| `HttpServer::onMessage/onRequests`, parser pools/pipeline | The ordinary loop builds the complete request vector before its pipelining check and before advice; released requests enter a reusable pool. A pipelining setter or later close cannot retroactively bound these allocations or repeated parser-generated100/error output. API-016 owns admission before construction. |
| `HttpServer::sendResponses`, `HttpResponseImpl`, Trantor `sendInLoop` | Render/merge/output copies need separate reservations. The TCP high-water callback runs AFTER append, so it is not a capacity-enforcement hook. A host response lease alone does not bound all framework output; API-016 owns pre-append reservations. |
| `HttpServer::onConnection`, `TcpServer::newConnection`, `HttpConnectionLimit` | TCP connection/context/parser and server-set retention precede advice/host callback. The global limit compares the OLD fetch-add value with `> max`, admitting max+1; do not claim an exact128 preallocation cap from `setMaxConnectionNum(128)`. Neither callback tags the listener before these allocations; API-016 owns finite transient admission/cleanup. |
| `HttpAppFrameworkImpl::run`, `ListenerManager::DrogonFileLocker` | Run creates256 upload scratch leaves, confined by §3. With app reusePort false the probe opens `/tmp/drogon.lock` using O_TRUNC/O_CREAT. §1 selects the existing public probe-bypass/pre-listen option sequence; it does not relocate or fix that unsafe open. |

**Allocated prerequisite:** APICONTRACT-005 → APITEST-016 → API-016 owns the
four absent pre-retention guarantees (headers, request construction, receive and
output), generic connection ownership and transport suspension/admission. No
API-001 host/authority is needed to prove that foundation. SESSION-011 rebases its
account-specific occurrence capture, erasure and pre-log suppression onto the
admitted successor, preserving every accepted account policy/budget and separate
review. Its pristine-source observations remain history, not the selected future
execution artifact. Native pre-Upgrade effects require explicit per-listener review.
APITEST-016 owns the generic preallocation portions of L10–L12; APITEST-015 owns
host endpoint selection/composition and all old HL oracles, itself changes NO SDK,
and awaits API-016. No unsafe partial exposure, alternate parser/server/proxy,
implementation waiver or deployment-only substitute is authorized.

Framework advice/controller/runtime-factory registrations can persist until process
exit. Keep them finite (one route table, one observer/control block, one profile
context/source); unregister where supported, otherwise revoke access to destroyed
owners and retain only accounted inert state. No strong callback→destroyed transport
reference or unbounded registration history. Include two listener/acceptor objects,
their descriptors (including idle /dev/null descriptors), timer wheels, fixed peer/
pending/mail tables and one coalesced wake per owner in source/resource review.
No exact process-memory or hard arbitrary-native-call deadline is claimed.

## 5. Stop and independent baseline correction examples

One first-stop fence/deadline applies to BOTH listeners, ordinary partial/idle/
output peers AND native upgrade/frame/Close peers. Refuse new work immediately;
cancel unentered diagnostics/wrappers, retain true FIFO SQL outcomes. Close WS by
its unchanged1000ms bound; force-close all remaining ordinary/native peers by
the SAME original drain deadline. Observe actual callbacks revoked, sockets AND
both acceptors removed, framework/IO joins and writer connection/completions before
the existing stopped/owner-destruction barrier. `quit()` initiates framework cleanup;
its return alone is not proof that every host dependency/callback was released.
Early stop, partial startup and startup-observer exceptions require the same
barriers unless bind/listen terminated the process. Signal handling retains the
consumer's record/wake-only safety rule, including cross-thread lock-free atomic
state, a handler-entry gate and an in-flight lock-free counter (or separately
reviewed equally safe mechanism) before closing/reusing wake descriptors. Closing
the gate must prevent any new descriptor borrow; handlers count before borrowing,
late entries observe the closed gate without touching the descriptor, and teardown
waits for all permitted borrowers to leave. Keep the bounded handler state alive
through possible late entry. Merely sampling a counter once or masking only the
teardown thread is insufficient; source review must prove this quiescence.

These are independent literal ORACLES, never executed results or new test counts.
APITEST-015 must preserve all75 Qt selectors/18 native methods, all HL01–17 and
their assertions/literals. Any added methods/rows/children require exact declared
registrations and reviewed harness budgets; no removals/weakening are authorized.
The revised endpoint selection is a disclosed baseline correction; historical
PR112 source8fa8810/red dd0f625/reviews retain their original meaning.

| ID / stimulus | Required oracle and old/new mapping |
| --- | --- |
| L01: two free, distinct harness-selected fixed candidate ports P/Q, address127.0.0.1 | One actual pair event reports WS=P, HTTP=Q, one combined ready, one IO loop; POST diagnostic on Q returns HTTP200/body `diag`; WS Upgrade on P returns101. With real activation, client `82 86 01 02 03 04 40 03 67 65 75 63` carries `A\x01data`; normal committed `done` yields server `82 05 41 64 6f 6e 65` (`Adone`), with original SQL/publication assertions. 41001/41002 are illustrative only, never required free ports. |
| L02: port0/http_port0; separately one fixed P and one0 | Observe two distinct ACTUAL nonzero ports; fixed value stays P. Never connect to0 or assume adjacent ports. Old aggregate `{address,P}` preserves WS=P, selects ephemeral HTTP, and defaults scratch_parent. Candidate-port selection does not establish readiness; genuine bind races remain startup failures. |
| L03: duplicate P/P; address `localhost`, `127.1`, empty, or embedded NUL | Factory `invalid_options` before bind/framework; no listeners/ready event and no Host constructed. Q validates that distinction. Separately, an actual failed/stopped Host start attempt cannot restart in process. |
| L04: exact existing diagnostic request `POST /api-host/diagnostic HTTP/1.1\r\nHost: localhost\r\nX-Host-Correlation: 44\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n` | Send to observed HTTP endpoint in diagnostics-before-activation (fresh/future), all six source-refusal subcases and fresh HL11 retry. Preserve HTTP200/`diag`, inactive/no-BEGIN/no-clock and all SQL facts. All existing Upgrade/frame helpers keep the WS endpoint. |
| L05: that POST to native endpoint; valid existing WS Upgrade to ordinary endpoint | Each fixed `HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n` (66 bytes), then EOF within original bounds; zero ordinary diagnostic/domain calls and no101/stock WS on HTTP. Separate fresh peers still serve their correct positive routes. |
| L06: seed the four §3 canaries under the OBSERVED actual child; GET `/`, `/index.html`, `/secret.txt`, `/%73ecret.txt`, `/../secret.txt`, `/../upload/tmp/00/secret.txt`, `/%2e%2e/upload/tmp/00/secret.txt`; HEAD same, OPTIONS `*`, PUT diagnostic | `/index.html` and `/secret.txt` map to genuine docroot files; traversal cases test confinement separately. No sentinel/file body/index/redirect/Allow/default page or command; complete parsed refusals400/empty/close ≤1024 bytes. Keep the positive HTTP diagnostic. Seed through filesystem data only, no host setter; supervise residue cleanup after reap. |
| L07: parent holds a plain listening socket on HTTP=Q; separately WS=P, other distinct candidate free | Each fresh child retains expected exit1, zero combined ready/serving; no graceful-cleanup claim. Release held port; NEW child binds both, diagnostic200 on HTTP plus101 on WS. Assert actual SO_REUSEADDR=1/SO_REUSEPORT=0 on both listeners before listen and no `/tmp/drogon.lock` effects. Source exit(1)/kernel inference alone cannot certify status1 or static-destructor safety. Keep old HL11 WS assertions; add HTTP variant and parent scratch cleanup. |
| L08: stop before start, between actual endpoint creation/listen acknowledgment, or before late Qt ready | No transient serving or successful endpoint-pair notification after stop; one cleanup/fence, both listeners gone, no second start. Synchronization observes real stages, never sets readiness. |
| L09: exactly SIX peers, within existing N cap8 | Retain native idle TCP, move original partial POST to HTTP, retain native partial Upgrade, partial-frame WS and uncooperative WS; add one HTTP idle TCP. Preserve all five original drain/refusal/Close assertions and add HTTP-idle closure. Q fake elapsed: first stop at1000ms, drain5000ms ⇒ deadline6000ms, unchanged by repeat5999ms; Close keeps its independent1000ms bound. N observes bounded real closure and actual writer/network/joins, never injects or claims those exact elapsed instants. Nonreturning-body/watchdog child remains terminal only. |
| L10: shared host peer limit2; one HTTP and one native peer occupy it; third otherwise valid connection | APITEST-015: close third before host peer/command retention, no second quota; release one once, fresh peer eligible. Associations global2 similarly count HTTP diagnostic plus WS work; third refuses, no domain body/BEGIN, true outcomes unchanged. APITEST-016 separately proves finite pre-admission/transient framework costs; host counters alone cannot prove them. |
| L11: reduced allowed body4 versus advertised5; independently selected APICONTRACT-005 exact/one-over header, raw/copy/output and global limits | Exact bounds remain eligible under all other gates; one-over refuses BEFORE affected retention/append, no refused domain dispatch. APITEST-016 owns concrete generic numeric oracles. With two requests coalesced, admit only the first; later bytes stay bounded/unparsed until release, then the second may enter. No two-outstanding-HTTP-request acceptance or speculative header totals. APITEST-015 preserves body4/5 host behavior. |
| L12: response_bytes4/capacity4; native `done` plus correlation; HTTP `diag` | APITEST-015: coherent startup accepted; separate fixed WS scratch5, body4 eligible. Existing response_cap3/`done` still refuses resource_limit5, never unavailable4. APITEST-016: concrete wire/render/output-copy bounds and pre-append refusal/drain release derive independently from APICONTRACT-005; no retrospective charge or host-handoff-as-drain assertion. |
| L13: queued-before-signal HTTP diagnostic; separately throwing HTTP diagnostic plus native credential request | Route HTTP stimulus to observed `http_port` in the two existing methods. Retain zero queued diagnostic entry/handoff after signal; retain diagnostic_exception, absent `diag`, absent `Cdone`, unavailable4 and actual no-BEGIN writer results. Endpoint correction does not change failure phase or transaction truth. |

## 6. Explicit supersessions and retained proof limits

References below identify the ORIGINAL packet, not shifting lines in this file.
`1bed699` is this author's initial packet (byte-identical five-doc import e71402b);
`8fa8810` is the historical APITEST-001 packet. All unlisted authority/domain
oracles stay binding. Contract filenames below are under `docs/contracts`;
“Consumer” means `api-host-lifecycle-boundary-v1.md`, “Initial listeners” means
`api-host-listeners-v1.md`. These amendments need one coherent independent review.

| Original path / reference and wording | Replacement / qualification |
| --- | --- |
| `api-host-listeners-v1.md` at1bed699:7–15 and both lifecycle pointer paragraphs at1bed699:25–30 / boundary:28–34, “ONLY … single internal endpoint” / resources unchanged | This table is the complete amendment scope: topology, buffers, thread ownership, refusal/scratch, projection/observer and signal safety. Existing bridge counts/charges, source finalization, generation, clock, SQL outcomes and HL01–17 stay binding. |
| `server/include/subathon/server/api_host_drogon.hpp` at8fa8810:6–14, one port for HTTP/WS | §1 appends http_port and scratch_parent, adds bounded actual endpoint/scratch/socket-option observation; §2 requires both real listens. APITEST-015 later authors declarations; no header changes here. |
| `api-host-lifecycle-v1.md` at1bed699:215–217, “Network receive/parser/output buffers use the separate native profile” | Native frame/control owners keep native bounds; ordinary HTTP needs API-016, and native pre-Upgrade changes require its explicit per-listener review (§4). Bridge pools alone cap neither. |
| `api-host-lifecycle-boundary-v1.md` at1bed699:352–356, framework “main-loop accept/listen work” | Selected Linux framework main configures/binds/coordinates; its single IO0 performs real listen/accept and peer callbacks. §2's FIFO marker follows both actual listens; all owning-loop barriers remain. |
| Consumer at1bed699 §2 Diagnostic route execution / composition, and initial listeners:121–157 | §3 adds global role-aware allowlist, fixed400/close, no implicit routes/static/upload/log output, early framing refusals from API-016, one current ordinary request and generic suspension. Diagnostic/domain authority separation is unchanged. |
| Initial listeners at1bed699:136–139, undefined root; consumer §2 N disposable paths | §3 defines bounded private parent/child, data-only observation and real docroot/upload canaries, normal cleanup and supervised terminal residue cleanup. No scratch content may be served. |
| Initial listeners at1bed699:56–57,202–208, no port reuse / lock requires SDK | §1 discloses real defaults; public app reuse skips probe, pre-listen callback clears actual SO_REUSEPORT. This is prospective and needs socket-option/occupied-port/no-lock evidence, not another SDK task for the lock alone. |
| Consumer at1bed699:482–501 capacity ledger / initial listeners:163–167 | §4 keeps the ledger unchanged; WS serialization scratch is separately fixed, HTTP wire copies use API-016 reservations through drain. Trusted dispatcher result allocation precedes host ownership under the frozen interface; no new preallocation guarantee for it. |
| Consumer at1bed699:272–273 startup callback exceptions “may finish in failed”; initial listeners:85–90 | §2 endpoint observer runs on Qt before startup delivery; throw ⇒ startup Failure::unavailable, no serving success, actual failed-start barriers. |
| Consumer at8fa8810 §2 signal paragraph (“sig_atomic_t” only); at1bed699:365–382 clarification | §5 requires async-signal-safe lock-free record/wake AND gated/counted quiescence (or separately reviewed equivalent); descriptor closure cannot race any permitted handler borrow. No signal API/authority change. |
| Initial listeners at1bed699:169–212,L10–L12 / account-ingress-v1.md §§1–3 pristine selection and “011/API-001 lifecycle integration” | §4 delegates concrete generic resource/suspension design to APICONTRACT-005 → APITEST-016 → API-016. SESSION-011 explicitly rebases onto its admitted candidate4 successor, retaining account budgets, capture, erasure and pre-log obligations without an API-001 foundation dependency. |
| Lifecycle at1bed699 §1 one-attempt/bind failure, HL11; initial listeners:59–64,L03,L07 | Factory rejection without Host consumes no Host attempt. Actual start remains once; exit1 oracles preserved, but source exit(1) may run static destructors on active Qt, so exact status/cleanup requires actual observation, not inference. |

Contract approval supplies no foundation. APICONTRACT-005 and APITEST-016's reviewed
baseline/admitted behavioral red precede API-016 implementation and independent
acceptance. APITEST-015 then requires that foundation and fresh consumer admission
for its reviewed host baseline/red before API-001 composition implementation.
DEPLOYMENT-007/008 must document/verify BOTH internal upstreams for default Caddy
and alternative proxies under the same public origin; no deployment implemented here.
Proxy/TLS/forwarded-header security, session UTC/auth, asset delivery, Linux/container/
Windows/OBS/UI and full release remain separate gates. All seven integrations,
three FX providers, English/German, appearance modes and both renderers remain required.
