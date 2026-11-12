# Isolated internal API host listeners — proposal v1

APICONTRACT-004; identity `subathon.api-host-listeners.v1`. Documentation only,
pending independent review. Authority: specification v0.45 §§2/10/16/17/18,
SERVER-01, API-01, DEP-02/05. Author base `8ca723e`, parent main `bd99912`.

Import [host lifecycle](api-host-lifecycle-v1.md), ALL HL01–17, the complete
[consumer boundary](api-host-lifecycle-boundary-v1.md) and its writer, source,
generation, transaction-finalization, publication, clock and byte/slot accounting
unchanged. This proposal supersedes ONLY the consumer's single internal endpoint
assumption. API-012 source, pins, patch and all44 tests stay frozen, including
`test_marked_listener_refuses_complete_http_before_ordinary_dispatch`.
[API-001 review B1](https://github.com/Kernol12/subathon-bot-ai/pull/121#issuecomment-6816497937)
exposed the incompatible same-port HTTP/WS baseline. Approval here supplies no
runtime capability; the additional source gaps in §4 remain prerequisites.

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
};
struct ListenerEndpoints {
    std::array<char, 46> address{};  // canonical numeric bind address, NUL terminated
    std::uint16_t port = 0, http_port = 0; // ACTUAL nonzero bound ports
};
using ListenersObserver = std::function<void(const ListenerEndpoints&)>;
// Existing one-argument calls remain valid; no observer is required in production.
Result<std::unique_ptr<Transport>> make_drogon_transport(
    DrogonOptions, ListenersObserver = {});
```

Preflight the complete address before framework construction: 1–45 ASCII bytes,
numeric IPv4 or IPv6, no hostname, brackets, zone ID, embedded NUL or whitespace.
Canonicalize without DNS. Explicit wildcard `0.0.0.0`/`::` binds are allowed;
the default stays loopback. Both endpoints use that same address/family. A wildcard
observation is a bind address, never a client destination or public-origin claim.
Deployment must separately restrict exposure; no forwarded header changes it.

Port0 asks the OS for an ephemeral endpoint, independently for each listener;
it never disables either. Nonzero ports must differ. Duplicate fixed tuples and
invalid addresses fail factory validation as `invalid_options`, before any bind.
Register fixed endpoints before ephemeral ones (HTTP first when tied), retaining
their role/order mapping; an ephemeral bind cannot knowingly consume the other
requested fixed port. Do not preselect/release a free port as readiness, enable
port reuse to share the endpoints, or retry a failed bind in process.

Any failure prevents combined readiness. Selected Trantor bind/listen failure
calls `exit(1)`: either occupied fixed port, including failure after the other
bind, is process-terminal HL11, with no graceful-barrier/destructor claim. The
parent reaps it and only a NEW child retries. Other startup failures/early stop
retain the one-attempt rule and real cleanup obligations. Invalid start cannot
make a later host start eligible in that process.

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
this barrier. Retain callback dependencies and check stop/failure at every stage;
late markers cannot revive admission. The owner emits one combined `listener_ready`
and startup success only after this actual pair and its current lifecycle gates.
Storage activation remains the separate unchanged real inspection/read/COMMIT chain.

Deliver the endpoint observer at most once on Qt, before startup-success delivery,
only for that acknowledged pair and before serving notification. It receives one
fixed-size nonsecret value, borrows no transport/source and grants no authority.
An observer exception fails startup with ordinary cleanup; it never escapes an
event loop. Absence is allowed; no production setter/query can manufacture the
observation. After stop an old copied observation is historical data only.
N may serialize exactly one `listeners` record, e.g.
`{"event":"listeners","address":"127.0.0.1","port":41001,"http_port":41002}`;
cap the record at256 bytes inside the EXISTING event/1MiB cumulative pipe budget.
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
literal paths, no pattern endpoint). They keep
their existing RouteIds, lowercase-hex `X-Host-Correlation` (1–256 decoded bytes),
opaque body and every existing success/refusal/exception assertion. WS remains
GET Upgrade `/api-host/ws`, with unchanged binary correlation/RouteId/body records.
No credential/account authority or public production endpoint is created here.

Install a synchronous `registerSyncAdvice` allowlist, before routing, decompression
and built-in OPTIONS handling. On ordinary HTTP allow only those exact POST paths,
matching BOTH `getOriginalPath()` and `path()`, with empty `query()`, no Upgrade,
Content-Encoding or Expect, and bounded framing/body/correlation. Do not invoke
multipart/upload/form/JSON decoders. Other complete parsed requests, including
GET/HEAD/OPTIONS/PUT/DELETE/PATCH, `/`, files, unknown paths and escaped aliases,
receive one empty400 response then close, at most1024 serialized bytes. No redirect,
index, Allow list, reflected input or framework error page. When the fixed reserve
is unavailable, close without a reply. Parser failures retain their bounded SDK
status/EOF handling; they cannot reach a command or expose an input-derived body.
Native Upgrade advice allows only the existing WS binding and native profile.

Also select `setFileTypes({})`, `setHomePage("")`,
`setImplicitPageEnable(false)`, no static locations, and a fixed empty400
`setDefaultHandler`; a default handler alone DOES NOT disable static lookup.
Before run, transport owns one private absolute scratch root (0700, bounded path,
no untrusted symlinks), selects `setDocumentRoot(root/empty)` and
`setUploadPath(root/upload)`, and removes it only after actual joins. The document
root is initially empty; no path may name configuration, database, keys or assets.
Select `enableGzip(false)`, `enableBrotli(false)`, `enableDateHeader(false)`,
`enableServerHeader(false)`, `disableSession()`, `enableRequestStream(false)` and
`enableCompressedRequest(false)`, no plugins,
framework DB/Redis clients, dynamic views, access/file logger or config loading.
Use `setLogLevel(trantor::Logger::kFatal)`; no request/credential logging.
Future asset delivery, public JSON, webhooks, auth, TLS and Caddy/same-public-origin
routing require their owning reviewed profiles; they are not deleted from release.

Set `setClientMaxBodySize(envelope_bytes)` AND
`setClientMaxMemoryBodySize(envelope_bytes)` to avoid intentional body spill files.
`setPipeliningRequestsNumber(per_peer + 1)` uses checked addition/narrowing and is
a post-parse backstop (the SDK rejects
at `>=`; zero means unlimited), NOT a preallocation guarantee. Preserve sequential
keep-alive diagnostics; no one-request-then-close replacement of positive behavior.
Second/subsequent requests while a response is outstanding must reserve the shared
per-peer/global association, callback and full response-wire capacity before
retention, or bounded refusal/close. No application response may grow a framework
queue after its response lease has been released without a separate charge.

## 4. Resource accounting and explicit absent prerequisite

The consumer's reference bridge charge17006592, source storage, fixed tables and
native-profile budgets stay EXACTLY unchanged; two listeners do not double them.
One serialization scratch of `response_bytes + 1` (1048577 reference bytes for
WS correlation) is an ADDITIONAL fixed transport allocation, not a permanent
reservation taken from the response pool. Checked sizing must accept the coherent
case `response_bytes == response_capacity`. HTTP framing/rendered copies and TCP
output are additional while live; charge allocated capacities, not just lengths.

The required ordinary ingress profile below is a TARGET for a separately split
prerequisite, not an assertion that candidate4 implements it. Before API-001 can
claim this surface safe, that prerequisite must supply reviewed mechanisms and
independent tests/red/implementation for ALL pre-retention bounds:

| Ordinary HTTP resource | Required finite reference ceiling |
| --- | --- |
| Shared admitted peers / associations per peer | Existing128 /8 across BOTH listeners; lower valid host profiles retained |
| Request-line bytes / total header wire bytes / header fields | 1024 /16384 /64, checked before path/header-map construction; duplicates count |
| One body usable capacity / materialized request/callback records per peer | Existing envelope1048576 / at most per-peer8, including parsed batches, pipeline and reusable pools |
| Raw receive capacity per ordinary peer | `envelope_bytes + 16384` =1064960, including read slack/rounding; no excess allocation then check |
| Outstanding ordinary response wire capacity per peer | `response_bytes + 1024` =1049600, including framing and fixed refusal reserve; reserve before render/append |
| Incomplete request / response-drain deadline | 5000ms from first byte / existing handoff5000ms, absolute in SAME elapsed domain, checked on each IO continuation; trickle never resets |

At reference128 ordinary peers these ceiling products are136314880 raw bytes,
134217728 body bytes and134348800 wire-output bytes, separate from the bridge and
native pools; actual simultaneous copies, request/header objects and allocator
overhead require an explicit finite resource receipt. The existing1024-byte
per-peer control slot is NOT charged twice or available for ordinary data. Where
response capacity is already committed, omit a refusal and close. Configure finite
framework connection limits as a backstop, but count transient rejected allocations
separately. Kernel memory/backlogs, TLS, allocator/RSS and scheduling remain distinct.

Read-only source observations from the coordinator-selected
`drogon-1.9.14-api012-candidate4-source` (not loaded or executed):

| Actual mechanism | Limit/conflict to resolve before claiming the target |
| --- | --- |
| Trantor `TcpConnectionImpl::readCallback`, `MsgBuffer::readFd` | Ordinary connections bypass native `read`; stock readv uses8192-byte stack scratch plus writable raw capacity, then grows. No application pre-read capacity setter was found. Count that scratch separately; native bounds cannot cover it. |
| Drogon `HttpRequestParser::parseRequest` | Body setters check advertised/cumulative body before reserve/append, but header fields accumulate without a total/count bound. The64KiB line/header check runs only when CRLF is absent. Sync advice is AFTER parsing; it cannot supply the target pre-map/request-line bound. |
| `HttpServer::onMessage/onRequests`, parser pools/pipeline | The ordinary loop builds the complete request vector before its pipelining check and before advice; released requests enter a reusable pool. A pipelining setter or later close cannot retroactively bound these allocations or repeated parser-generated100/error output. |
| `HttpServer::sendResponses`, `HttpResponseImpl`, Trantor `sendInLoop` | Render/merge/output copies need separate reservations. The TCP high-water callback runs AFTER append, so it is not a capacity-enforcement hook. A host response lease alone does not bound all framework output. |
| `HttpServer::onConnection`, `TcpServer::newConnection`, `HttpConnectionLimit` | TCP connection/context/parser and server-set retention precede advice/host callback. The global limit compares the OLD fetch-add value with `> max`, admitting max+1; do not claim an exact128 preallocation cap from `setMaxConnectionNum(128)`. Neither callback tags the listener before these allocations. |
| `HttpAppFrameworkImpl::run`, `ListenerManager::DrogonFileLocker` | Run unconditionally creates256 upload scratch directories. Select a private absolute upload root under owned process-local scratch (0700, no attacker-controlled symlinks), remove after joins; no upload files authorized. With reusePort disabled, the port probe opens absolute `/tmp/drogon.lock` using O_TRUNC/O_CREAT, without a relocation/safe-open hook. A private working directory does not contain that path or fix its safety. |

**Additional prerequisite required:** bounded ordinary Drogon ingress/egress and
framework connection/scratch ownership, including the absolute lock-file conflict.
Coordinator must split it and reconcile its solution with the frozen SDK constraint
before implementation. This document authorizes NO SDK/pin/API-012 change,
alternative parser/server/proxy, exception to the bounds, or deployment-only fix.
The available route/settings mechanisms resolve reachability/isolation, not these
missing resource guarantees. Do not implement unsafe ordinary exposure and call
the new assertions supported; APITEST-015 may author the declared missing-behavior
oracles, with execution/red gated on a working independently reviewed foundation.

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
state and handler quiescence before closing/reusing wake descriptors.

These are independent literal ORACLES, never executed results or new test counts.
APITEST-015 must preserve all75 Qt selectors/18 native methods, all HL01–17 and
their assertions/literals. Any added methods/rows/children require exact declared
registrations and reviewed harness budgets; no removals/weakening are authorized.
The revised endpoint selection is a disclosed baseline correction; historical
PR112 source8fa8810/red dd0f625/reviews retain their original meaning.

| ID / stimulus | Required oracle and old/new mapping |
| --- | --- |
| L01: address127.0.0.1, port41001, http_port41002, both free | One actual pair event with those values, one combined ready, one IO loop; POST diagnostic on41002 returns HTTP200/body `diag`; WS Upgrade on41001 returns101. With real activation, client frame `82 86 01 02 03 04 40 03 67 65 75 63` carries `A\x01data`; normal committed result `done` yields server `82 05 41 64 6f 6e 65` (`Adone`), preserving the original SQL/publication assertions. |
| L02: port0/http_port0; separately one fixed41001 and one0 | Observer reports two distinct ACTUAL nonzero ports; fixed value stays41001. Never connect to0 or assume adjacent ports. Default/old aggregate `{address,41001}` preserves WS41001, selects an actual ephemeral HTTP endpoint. |
| L03: duplicate41001/41001; address `localhost`, empty, or embedded NUL | `invalid_options` before bind/framework; no listeners/ready event. No same-process start retry entitlement. |
| L04: exact existing diagnostic request `POST /api-host/diagnostic HTTP/1.1\r\nHost: localhost\r\nX-Host-Correlation: 44\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n` | Send to observed HTTP endpoint in diagnostics-before-activation (fresh/future), all six source-refusal subcases and fresh HL11 retry. Preserve HTTP200/`diag`, inactive/no-BEGIN/no-clock and all SQL facts. All existing Upgrade/frame helpers keep the WS endpoint. |
| L05: that POST to native endpoint; valid existing WS Upgrade to ordinary endpoint | Each fixed `HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n` (66 bytes), then EOF within original bounds; zero ordinary diagnostic/domain calls and no101/stock WS on HTTP. Separate fresh peers still serve their correct positive routes. |
| L06: files containing literal `PRIVATE-SENTINEL` at private root/index.html, secret.txt and upload scratch; GET `/`, `/index.html`, `/secret.txt`, `/uploads/tmp/00/secret.txt`, `/../secret.txt`, `/%73ecret.txt`; HEAD same, OPTIONS `*`, PUT diagnostic | No sentinel, file lookup/body/index/redirect/Allow/default page or command; complete parsed refusals400/empty/close ≤1024 bytes. Verify with real files and positive HTTP diagnostic, not only absence assertions. |
| L07: parent holds HTTP41002; separately WS41001, with other fixed port free | Each fresh child fails exit1, zero combined ready/serving; no graceful-cleanup claim. Release held port; NEW child binds both and diagnostic200 succeeds on HTTP plus101 on WS. Keep original HL11 WS occupied-port assertions; add the HTTP variant. |
| L08: stop before start, between actual endpoint creation/listen acknowledgment, or before late Qt ready | No transient serving or successful endpoint-pair notification after stop; one cleanup/fence, both listeners gone, no second start. Synchronization observes real stages, never sets readiness. |
| L09: original HL12 peers, plus ordinary idle and partial POST | Move partial POST to HTTP; partial Upgrade/frame/uncooperative WS stay native. Both listener kinds close by original stop bound. Stop1000/drain5000 means6000ms even after repeat5999; writer/network/joins precede stopped. Nonreturning-body/watchdog child remains process-terminal only. |
| L10: shared peer limit2; one HTTP and one native peer occupy it; third otherwise valid connection | Refuse/close third before host peer/command retention, neither listener gets a second quota. Release one once, correct new peer eligible; source review separately covers framework/transient costs. Associations global2 similarly count HTTP diagnostic plus WS work; third refuses, no body/BEGIN, unchanged true outcomes. |
| L11: reduced body4; advertised5; header target fields64 then65; total16384 then16385; pipeline per-peer2 then3 while completions held | Exact bounds eligible if all other gates pass; one-over refuses BEFORE the affected retention, with no domain work for refused request and bounded error/output/cleanup. Current SDK cannot satisfy the header/batch oracle: §4 prerequisite is explicit, no post-parse check counts as success. |
| L12: response_bytes4/capacity4; native body result `done` plus one correlation byte; HTTP reply `diag` | Coherent startup accepted; fixed WS serialization scratch5 is separately accounted. Body4 eligible, all wire/header/framework copies additionally reserved; occupied output refuses/closes before append, releases once through removal. Existing response_cap3 result `done` still refuses `resource_limit` (5), never changed to unavailable (4). |
| L13: queued-before-signal HTTP diagnostic; separately throwing HTTP diagnostic plus native credential request | Route HTTP stimulus to observed `http_port` in the two existing methods. Retain zero queued diagnostic entry/handoff after signal; retain diagnostic_exception, absent `diag`, absent `Cdone`, unavailable4 and actual no-BEGIN writer results. Endpoint correction does not change failure phase or transaction truth. |

Contract approval precedes APITEST-015's separate authored-header/fixture/test review
and valid red, then implementation/review/admission/acceptance. Proxy/TLS/forwarded
header security, session UTC/auth, asset delivery, Linux/container/Windows/OBS/UI
and full release gates remain separate. All seven integrations, three FX providers,
English/German, appearance modes and both renderers remain required.
