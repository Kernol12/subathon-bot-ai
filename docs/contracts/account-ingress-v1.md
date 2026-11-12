# Account ingress — v1, document proposal revision 2

SESSIONCONTRACT-005; specification v0.45 §§2/10/16/17, SERVER-01,
AUTH-12/15, API-01 and TEST-04/05. Identity `subathon.account-ingress.v1`.
This is the complete proposed solution document for **different actual review**,
not an admitted patch, configuration, endpoint or supported-client announcement.
The original partial author checkpoint `4b0178cc89b340dcb6f80e6eb80bab8329ca0198`
and its source receipts remain historical; this revision replaces its affected
engineering choices after actual accepted SESSIONCONTRACT-006/ADR-0011.

Frozen imports:

| Accepted document | Git blob |
| --- | --- |
| [Account transport v1 R2](session-transport-v1.md) §§1–5 | `0d0cbde9450abc5d7f99b7a97b03cfe41f385890` |
| [Reference HTTP v1 R2](reference-http-v1.md) | `fe69e3c3b06fbe63d1d2beaea458cbacf905032f` |
| [Account supplier v1](session-resolution-v1.md) | `c5f5a6e71664042e18475068d3caf577b0c8d009` |
| [Common wire v1](wire-primitives-v1.md) | `11eeb5f422a174e042143df6b476ae1d717eeacf` |
| [Current session clock v1](session-clock-v1.md) | `946bd12b94d3a83dcd4c86c787bb257d9472e022` |
| [Physical account/session v1](sqlite-account-sessions-v1.md) | `8dc7ba9fce0e40bcfa02897c6d88946849cbd1b1` |

All imports are unchanged. The clock/store remain separate current execution
producers; an ingress digest, policy generation, TLS connection or stored session
reference grants no identity, time trust, permission, transaction or publication.
This document introduces no public auth service or raw-public-head envelope.

The authored [foundation v1](bounded-http-foundation-v1.md) supplies the proposed
generic fixed receive, bounded sink and wipe-installation seams for this rebase.
Its ordinary sink bypasses stock maps; its sanitized native Upgrade projection
is a separate listener path and is NEVER an account capture. SESSION-011 retains
this document's exact policies/limits, secure erasure and pre-log obligations,
and must bind its capture ABI to the actual admitted successor/connection.

## 1. Verified starting point and proposed correction

Historical authenticated starting source is Drogon 1.9.14
`3aae769cc405cf5abb9d477675c6430924f02c7c` and bundled Trantor 1.5.29
`78e480f5e138acdc05f76c55a2716718cd846364`, as in
[setup](../../tools/setup_drogon.py) and
[dependency evidence](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/spike-001-dependencies.md). Author evidence
records clean source identity and byte-for-byte official-download comparison.
The installed binary was NOT invoked or authenticated by this document.

| Actual immutable source observation | Consequence |
| --- | --- |
| [HttpRequestParser.cc](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/src/HttpRequestParser.cc), `kExpectHeaders` calls `addHeader` before application advice. A colonless line terminates headers. The 64 KiB line check occurs only when CRLF is absent. Framing subsequently reads a selected map value. | A filter/AOP/controller is too late. Existing line/body settings establish neither complete original-field bounds nor strict framing. |
| [HttpRequestImpl.cc](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/src/HttpRequestImpl.cc), `addHeader` trims using `isspace`, logs the complete Cookie at TRACE, overwrites cookie names and emplaces other fields in a map. | Capture/check before this call; do not pass account header input through it and later reconstruct a list. |
| Parser `Expect` failure logs its supplied value at WARN. [HttpServer.cc](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/src/HttpServer.cc) logs paths at TRACE; parser errors write directly without pre-sending advice; default OPTIONS emits reflected allow-origin/allow-headers. | Remove raw-input diagnostics in the admitted path, own early errors and override OPTIONS before default CORS. A late response scrubber is insufficient. |
| [AccessLogger.cc](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/src/AccessLogger.cc) supports URL/query/header/cookie formatters; `HttpRequestImpl::parseJson` logs parser error strings. | No generic access logger, request serialization or framework JSON parser on this path. A secret placed in an invalid field/body remains secret. |
| Trantor [MsgBuffer.cc](https://github.com/an-tao/trantor/blob/78e480f5e138acdc05f76c55a2716718cd846364/trantor/utils/MsgBuffer.cc) grows/copies buffers; `readFd` uses an 8192-byte stack overflow buffer; retrieval resets indices/resizes without wiping. | Later semantic limits cannot bound earlier allocations or erase released copies. |
| [TcpConnectionImpl.cc](https://github.com/an-tao/trantor/blob/78e480f5e138acdc05f76c55a2716718cd846364/trantor/net/inner/TcpConnectionImpl.cc) reads before its receive callback. [OpenSSLProvider.cc](https://github.com/an-tao/trantor/blob/78e480f5e138acdc05f76c55a2716718cd846364/trantor/net/inner/tlsprovider/OpenSSLProvider.cc) feeds a memory BIO and grows plaintext storage before event dispatch. | An `onMessage` size check alone is not a preallocation limit for either plain or TLS ingress. The TLS single-read constant is not a total bound. |

Execution now requires the separately reviewed/admitted additive candidate4
successor from [APICONTRACT-005](../tasks/EPIC-008/APICONTRACT-005.md) →
[APITEST-016 / API-016](../tasks/EPIC-008/API-016.md), preserving the historical
API-012 artifact/patch/pins and all44 methods unchanged. The table above remains
pristine-source history, NOT the future execution selection. Generic per-listener
pre-map sizing/request admission, fixed receive/suspension, output reservation
and transient connection ownership belong to API-016 and its independent native
fixture; that foundation has NO API-001 dependency. Both SESSIONTEST-011 and
SESSION-011 depend on API-016. Native pre-Upgrade effects must be explicitly
reviewed per listener against unchanged44 compatibility, never assumed isolated.

The account-specific correction belongs to [SESSION-011](../tasks/EPIC-007/SESSION-011.md),
explicitly REBASED onto that admitted successor with reviewed narrow patches,
preserved licenses and a distinct patch/build/capture-ABI identity. It still owns
every occurrence capture, raw-secret erasure and pre-log removal obligation below;
API-016 supplies NONE of those account guarantees. All accepted account sizes,
credential/framing policies, lease lifetimes and literal outcomes remain unchanged.
Do not edit a shared cache or label a patched library as pristine1.9.14.
The [accepted006 evidence](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/sessioncontract-006-handoff.md)
also supplies the separately authenticated26-file Caddy/Go candidate proof and its
corrected different review0990c608. Those are source/engineering facts, not loaded
artifact admission. No new parser library or TLS/proxy runtime is selected here.

## 2. Native pre-map/pre-log capture and source handoff

Propose one listener-specific `subathon.ingress-capture.v1` internal capability in
the selected native artifact. It is not an existing upstream hook. The artifact
manifest fixes ABI version, exact Drogon/Trantor commits/patch hashes/options,
listener profile and permitted consumer build. Fail readiness on mismatch; a
getVersion string or arbitrary shared HttpRequest attributes cannot establish it.
Root owns the actual headers/build/pins;011 authors/reviews its concrete declaration
and real-stack baseline before implementation. The following operations/ownership
are the contract, not a separately implemented public SDK:

| Private capability | Required behavior |
| --- | --- |
| `CaptureLease` | Move-only, one actual connection/request; only the patched parser can construct it. Owns fixed occurrence records and semantic byte arena. Bound to the native I/O owner thread; never a Qt queued/public request attribute. |
| `with_capture(lease, callback)` | One synchronous, nonreentrant visit while the lease is live. Borrow method, exact escaped target, body view, all header name/value occurrences and actual native connection/profile facts. No callback retention/async visit. The complete view stays immutable and valid until the enclosing transport call returns, not merely until read_current returns. |
| `NativeConnectionFacts` | Native-owned listener/connection identity, actual peer endpoint, request sequence, admitted artifact/profile identity and direct-TLS facts if that path is separately admitted. These are captured from the actual connection, never parsed header flags. |
| `RequestIngressScope` | SESSION-010 privately binds one lease/actual installation/immutable policy snapshot/trusted route purpose. Implements the UNCHANGED AccountIngressSource::read_current API. Validate limits and actual native/policy trust before exposing views; returns unavailable on mismatch, expired owner, wrong thread, exception or unadmitted path. |
| Scope completion | Move only the unchanged prepared digest/mode/owned policy reference (or sign-in guard) onward. Revoke borrowed access, erase owned raw secret regions and release the lease on every exit. A retained C++ source reference cannot extend scope or prepare another request. |

At `HttpRequestParser::processRequestLine`, scan the original bounded line before
`setPath`/`setQuery`; preserve escaped target and an empty query marker. Bind a
trusted profile from an exact route table, never from client purpose/User-Agent.
At `kExpectHeaders`, scan/capture every field BEFORE `addHeader`, framing selection,
Expect processing or body reserve. Capture ALL names/values, including empty,
unrelated and repeated occurrences; trim only outer SP/HTAB and preserve per-name
relative order. Do not call original addHeader for any raw account-family field.
No cookie/header map, field selection, percent decoder or cookie jar is the source.

At head completion validate framing against the occurrence list, project the whole
route request, then admit any body copy. Never ask a selected Content-Length map
value to establish framing. At HttpServer dispatch, give the private lease to the
SESSION-010 handler before path/request logging, generic filters/controllers or
serializer access. Only a trusted nonsecret route identity and validated framing
projection may enter ordinary Drogon routing objects. Raw headers/target/body remain
in their bounded owner and are parsed by the owning HTTP/transport contracts.
Replace/bypass the raw Cookie TRACE, Expect WARN, path TRACE, JSON-error and stock
OPTIONS/error-output paths for this profile. Capturing duplicates and then allowing
the original Cookie logger to run is still failure.

One complete raw account request is active per connection. Do not accumulate raw
leases in Drogon's request pool/pipeline vector or accept an unbounded response
queue. Parse a later pipelined request only when its predecessor's raw scope has
ended and API-016's transport admission gate permits it; API-001 additionally
gates work when later composed with the host. Already-read later bytes stay
unparsed in the bounded native receive owner. Generic keep-alive/read suspension,
continuation and transport shutdown are API-016 foundation responsibilities,
independently testable without API-001; SESSION-011 supplies account lease erasure
on those paths. Unknown routes receive the owning bounded failure/close without granting a request-selected
transport purpose. Other endpoint families retain their separately owned profiles.

## 3. Native framing, allocation and secret-copy bounds

R2 permits enumerated normalization at the EXTERNAL edge, not general normalization
inside the app source. Proposed native H1 profile is deliberately stricter: CRLF
only, no obs-fold, exact HTTP/1.1 request line, method token1..16, raw escaped target
1..2048, complete request line<=2048 including CRLF, origin-form only, no fragment,
space/control or URI cleaning. Require one valid Host authority. Header names are
ASCII HTTP tokens, no whitespace before colon; reject NUL/DEL/CR/LF and other
controls except value HTAB. An empty line alone terminates headers. A colonless
nonempty line is malformed, not end-of-head. Empty field values remain present.
OWS/name-case tolerance and duplicate auth/Origin/marker/Cookie outcomes remain
transport semantics, never a parser first/last-value shortcut.

The native producer refuses any Transfer-Encoding, Upgrade, Trailer, TE,
Proxy-Connection or Expect occurrence before body parsing/reserve/100 response;
Connection, if present, must use transportR2's whole-list close/keep-alive grammar.
It may refuse duplicate Content-Length more strictly than the edge; proposal here
refuses ALL repeated CL at the native listener. Single CL is nonempty digits only,
checked without signs/lists/partial parse; zero/leading zeros are legal. CL+TE and
unsupported transfer coding never become an admitted body. No chunk decoder,
trailer, request decompression, HTTP1.0, h2c, CONNECT or upgrade is supplied by this
account profile. Reference content encoding/media/body/query/method rules retain
their own exact phase and415/406/400 outcomes. Edge-normalized equal CL reaches the
native listener as one agreed field or an allowed omitted zero length.

Freeze budgets BEFORE accepting the connection or copying fields. Validate trusted
sizes, checked sums/terminators, SIZE_MAX/max_size and profile/body representability.
Invalid trusted limits → unavailable; malformed/oversized input → owning bounded
failure or close. No prefix acceptance, temp file or unbounded DOM/request clone.
The concrete reference profile uses its exact4096 semantic request/body ceiling;
other session/login routes require their separately reviewed finite profile before
enabling them. This does not invent a generic65536-byte login body limit.

| Native/application owner | Proposed ceiling and admission order |
| --- | --- |
| Plain backend receive storage | One65536-byte MsgBuffer payload, initialized before reads, plus its existing8-byte prefix. Before EACH read compact/wipe consumed regions and require writable>=8192. In the selected readFd this selects the ONE iovec branch, avoiding the8192 spill buffer and append/growth path. Never read/grow if the precondition fails: process pending bounded bytes, suspend the channel, or close. No ensureWritable reallocation or ordinary unwiped compaction in this mode. |
| Raw parser work | Complete native request-line+field-section<=40960 bytes, including raw OWS, separators and every occurrence; check while scanning even when a complete oversized line arrived at once. At most one current incomplete head. The receive quantum is separate: up to65536 coalesced bytes may include later requests and are not all attributed to the current head. |
| Captured semantic fields | One fixed32768-byte arena and64 fixed offset/length records, no owning strings/maps per field. Validate field_count<=64, semantic name+value<=8192 and H=2+sum(name+value+4)<=32768 before recording/copying. Reference uses smaller field/header4096 and complete method+target+12+H+body<=4096 checks before its semantic capture/body growth. Every incoming metadata/Via occurrence counts. |
| Method/target views | Copy only validated method/target into fixed16/2048-byte slots, or use pinned lease-owned slices with the same bounds. No URL parsing allocation before raw length checks. These fixed slots/records are additional to the semantic field arena; the complete reference projection counts their actual bytes. |
| Body | One route-owned, projected bounded byte owner; reference<=4096 AND complete semantic request<=4096. Consume exact checked CL, no subsequent bytes as body. Native receive and route body ownership may coexist; body parsing scratch belongs to referenceHTTP, not this capture. |
| One reference connection/request peak at these boundaries |65536 receive+8prefix+32768 field arena+16method+2048target+4096body=104472 bytes, plus exactly64 fixed records/lease facts. It is a conservative native capacity sum, not referenceHTTP's116992 semantic application-payload sum and not whole-process RSS. No additional full head/body copy or raw request clone. |
| Transport preparation | Imported at-most16 borrowed views, fixed counters, supplier124-byte application scratch, one32-byte digest and one owned policy string<=4096. Preproject the complete policy result before copy/crypto; retain no raw token/header view in Qt work. Native crypto scratch remains separate. |

The generic receive mode is now an explicit API-016 prerequisite using existing
Trantor storage/read/channel ownership, not an upstream setting or account-only
011 correction. API-016 must enforce its capacity/pre-read invariant on coalesced
input, EAGAIN, EOF, exceptions, suspension/resumption and destruction; its tests/
source review prove no growing path. SESSION-011 additionally proves every account
wipe/compaction obligation in the unchanged table, with no unwiped account path.
Generic read suspension/admission uses the reviewed transport interface and fixture
without an API-001 evidence dependency; no second I/O loop or blocking thread.
The substantial generic foundation is already split; further expansion still
requires integrator scope/review before code.

When route header checks and the synchronous transport call have BOTH finished,
wipe the token-bearing field arena and consumed receive ranges before release/reuse.
Wipe method/target/body owners as soon as their owning route scan/parse has finished,
and always before raw scope release; malformed input can carry secrets anywhere. Preserve any later pipelined bytes only inside
the same bounded native owner until independently admitted; wipe them on closure.
Use selected libsodium erasure for project-owned regions/scratch, including error,
timeout/cancellation and request-pool destruction. Ordinary clear/resize/free and
memset do not prove erasure. Borrowers never clone the head or construct diagnostic
strings from it. The reference decoder/secret response emitter retain their own
accepted copy/lifetime budgets; no raw request is persisted or replayed. Only the
route's already reviewed bounded parsed DTO may cross its queue boundary; any
separately owned typed authentication-secret command needs its own accepted owner,
not a retained HTTP body/view. Fixed native capacities and referenceHTTP semantic
payload budgets overlap in actual request bytes and must not be falsely presented
as disjoint totals or inferred heap measurements.

Direct TLS is OPTIONAL. Existing Trantor/OpenSSL plaintext/BIO growth is not proved
bounded by the above plain receive mode. Enabling it requires a separately admitted
native allocation/copy/handshake/certificate/session/output/time manifest and actual
TLS producer tests under011/deployment ownership; use no invented fixed TLS slabs.
Without that evidence this path is unavailable, while the required bundled proxy
path may use an isolated plain backend. Edge Go/TLS/maps/copies, native TLS scratch,
allocator overhead, kernel/swap/registers/dumps and concurrency have separate actual
owners/limits. No global zeroization or exact native heap/CPU promise follows.
[Accepted secure-erasure basis](https://doc.libsodium.org/memory_management).

## 4. Concrete edge, authority and logging solution

Use accepted transportR2 §§1.1–1.3 in full: candidate Caddy2.10.2 commit
`551f793700fe1550845c824470b623fd1aa03d36`/Go1.25.0 commit
`6e676ab2b809d46623acb5988248d95d1eb7939c`. These are inspected candidate source
identities, not approved dependency binaries/configuration. [Bundled007](../tasks/EPIC-024/DEPLOYMENT-007.md)
and [alternative008](../tasks/EPIC-024/DEPLOYMENT-008.md) own actual artifact/config/
platform admission and their own reviewed tests/red. Default Caddy AND an existing
proxy alternative remain release requirements; direct TLS does not replace either.

The concrete candidate guards execute as ordered JSON terminal routes BEFORE any
header mutation/reverse_proxy: retained TransferEncoding any-value refusal; ANY
Upgrade/Trailer/TE/Proxy-Connection/Expect; ALL comma-joined Connection values against
the anchored close/keep-alive allowlist; HTTPS/H1.1, original origin-form URI and
original RequestURI==URL.RequestURI roundtrip; ASCII/non-zone authority and complete
canonical host/effective-port comparison. Import the exact CEL/refusal fragments
and parser417/505/501 exceptions from R2 rather than inventing a different matcher.
An any-one-value Connection allowlist is unsafe. A request which nominates Cookie,
Authorization, Origin, marker, Host, negotiation, conditional, correlation or
forwarding fields is refused before Caddy can remove them.

Only R2's named transformations are allowed: bounded legacy whitespace/fold/line
parsing at this edge; agreed equal-CL framing normalization; name casing/different-
name ordering; explicitly enumerated User-Agent/Pragma/hop metadata; overwritten
forwarding metadata; and addition of one `Via: 1.1 Caddy`. Preserve ALL security
occurrences and Cookie pairs. Count ALL fields actually arriving at the app,
including empty/unrelated fields, incoming Via and Caddy's added Via16 semantic
bytes. No raw-head protocol, authority rewrite, query stripping or silent extra
normalization. A changed module/configuration/version requires renewed admission.
The alternative must document its own exact equivalent or stricter admissible
profile with supported-client evidence; unknown transformations are unavailable.

Edge input/storage is independent of app semantic4096: candidate head ceiling40960
including Go slop/prior buffering; body admission4096 plus documented excess/cleanup
quanta and timeouts. Import R2's N528386 decoded cleanup and conservative raw-body
upper bound2165866504; neither is retained storage or an accepted body size. Go
reader/upstream buffers/response quantum and TLS/native/concurrency/process limits
remain separately admitted. Fixed parser417/505/501 and other early nonreflecting
stock refusals are external transport failures with zero backend dispatch, not
application CommonFailure/auth outcomes. Application oversize/framing failures use
referenceHTTP/transport's exact431/413/400 ordering/envelope or safe close.

Canonical origin comes from the immutable trusted installation policy, not Host,
Origin, forwarded fields or SNI. Validate ASCII DNS/IPv6 canonical authority,
effective port and HTTPS scheme before listening. Native Host must occur exactly
once, parse without userinfo/control/ambiguous brackets/port and match that canonical
authority; reference raw Origin still compares exact canonical serialized bytes.
SNI/certificate matching alone is not HTTP authority. No suffix/subdomain/portless
matcher substitutes for full comparison. Exact escaped target and empty query
remain visible for reference's independently enforced path/query rules.

For proxy mode bind a private loopback/Unix/private network endpoint with verified
ownership/firewall/container isolation to the explicit configured peer/channel.
An admitted private TCP backend is sufficient where untrusted processes cannot
reach/impersonate it; do not infer isolation merely from127.0.0.1. Unix or authenticated
backend TLS may be chosen by actual deployment where supported/needed, not mandated
as a new protocol. At connection admission bind the real peer identity to that
policy; an untrusted peer never receives a proxy-mode source. Caddy overwrites/removes
hostile Forwarded/X-Forwarded-* and other untrusted forwarding metadata, retaining
canonical Host and emitting its one configured HTTPS/authority facts. The backend
requires exact configured single-hop fields/cardinality/value and admitted peer;
missing/duplicate/stale/unprovable facts →503/unavailable or close, no fallback to
request Host as trust policy. Via/client IP never proves TLS or authority.
Optional direct mode instead uses actual completed TLS session/listener facts;
forwarding strings do not turn plain HTTP into HTTPS.

Before listening, enforce R2's complete candidate log suppression: root Caddy sink
writer discard, ONLY default logger discard/NopCore, no access log/custom core/
exporter/trace/telemetry/profiling/request placeholders, and stdout/stderr discarded
from process launch, including pre-config/panic paths. Audit Go/TLS/third-party
sinks and environment. This covers pre-handler request clones/lazy error/default5xx
paths; disabling access logs or relying on Cookie redaction/severity is insufficient.
The native patch independently eliminates/bypasses raw-input log statements and
AccessLogger/JSON diagnostic serialization BEFORE input can reach them. Do not log
URL/query/body/headers/forwarding fields/native errors or input-derived labels even
when malformed. Allow only fixed event codes/status/counts/trusted route identity
and an independently admitted nonsecret correlation reference. Source audit and
canary/fault tests must cover startup, parser errors, matcher failures, upstream
down, reload/shutdown, export/debug attempts and responses. No memory erasure of
Go/TLS/process copies is inferred from absent logs; their actual owners remain
separate gates, with no heap-wide assurance.

## 5. One immutable policy and current execution

SESSION-010's host-owned policy producer supplies one immutable snapshot containing
installation binding/canonical origin, exact route purpose/limits, native artifact/
profile identity, direct/proxy mode, configured peer isolation/metadata rules,
admitted edge artifact/configuration/log profile and generation reference.
No client flag, trusted=true field or public constructor can supply these facts.
Reference fields/snapshot storage are bounded before publication: origin<=1024,
policy reference<=4096, finite configured route/peer manifests and fixed native IDs;
actual manifest collection counts belong to API-001/deployment admission, not an
unbounded per-request copy. Each request holds one shared immutable snapshot and
one capture lease; it borrows snapshot strings during the synchronous call.

Use a process/host-owner-lifetime unsigned64 generation starting1, serialized as
`g` plus canonical decimal (<=21 bytes). Increment for ANY effective origin, trust,
limits, route-purpose, parser/proxy artifact, protocol or log-policy change; refuse
on exhaustion. Same generation means the same immutable snapshot in that live
owner. No durable boot epoch, historical global uniqueness or cross-process ABA
promise is required: prepared requests/scopes/queues are NEVER persisted/restored
or transferred to a new owner. API-001 stops old admission and cancels/drains every
old callback/work before replacing the owner; a stale source/scope is unavailable.
The original process/installation lifecycle cannot be extended by holding a pointer.
A whole-host rollback/compromise is not solved by a policy string; accepted clock/
restore trust limitations remain separate.

Capture the snapshot at request-start admission and keep it through parse/prepare.
On change, suspend new admission, validate replacement, publish its generation on
the authoritative queue, coordinate native/edge configuration readiness and drain/
close incompatible old channels, then resume. Existing scopes retain OLD immutable
facts rather than mixing old origin/new peer data. At authoritative execution,
compare the prepared reference against SAME current owner/policy BEFORE supplier
lookup, protected effect or private disclosure. Mismatch →503/unavailable and
requires a FRESH actual client request. Never mutate the queued reference, rebind a
digest to new policy or replay a raw request. An unchanged pointer/string from a
prior scope is not proof of current readiness.

External edge reloads must be coordinated with that producer: exact active config/
artifact/isolation must be known before readiness resumes. File hashes/forwarded
strings alone do not prove the edge loaded a configuration. Unknown/failed/partial
reload or policy source → unavailable/close. API-001 owns serialization/startup/
shutdown/backpressure; SESSION-010 supplies the real source binding; SESSION-003
owns full route/execution integration. Current supplier resolution then uses its
unchanged original digest, actual installation/current facts and accepted current
clock in the same serialized execution. Storage transactions/publication and
accepted login/permission remain their own producers, not ingress booleans.

Closed CORS applies on every route/error/OPTIONS: no allow-origin/credentials/
headers permission, reflection or implicit stock CORS. Bind owned OPTIONS refusal
before Drogon's default handleHttpOptions; no unknown-route preflight permission.
Keep transport's marker on browser GET/HEAD and unsafe requests, required exact
unsafe Origin, and forbidden Origin/marker on desktop Bearer unchanged. No cookie
fallback from an empty/invalid Authorization. Browser hiding a response is not a
server-side CSRF check. SESSION-003 retains actual browser cookie/egress/platform
and complete WebSocket authorization work; H1-only JSON admission removes none.

## 6. Literal oracles and reproducible future red/pass

A–D are unchanged independently hand-authored native-listener heads below. Each
`\r\n` denotes actual two-byte CRLF; display newlines/labels are not transmitted.
Configured origin=`https://timer.example`, policy=`g1`, trusted admitted peer/TLS
fixture, protected GET `/api/reference/v1/timer`. Token is exactly64 ASCII zeros,
synthetic and never issued. Each block ends with an empty CRLF line.

```text
A: GET /api/reference/v1/timer HTTP/1.1\r\n
Host: timer.example\r\n
Authorization: Bearer 0000000000000000000000000000000000000000000000000000000000000000\r\n
Authorization: Bearer 0000000000000000000000000000000000000000000000000000000000000000\r\n
\r\n

B: GET /api/reference/v1/timer HTTP/1.1\r\n
Host: timer.example\r\n
Cookie: __Host-subathon_session=0000000000000000000000000000000000000000000000000000000000000000\r\n
Origin: https://timer.example\r\n
oRiGiN: https://timer.example\r\n
X-Subathon-CSRF: 1\r\n
\r\n

C: GET /api/reference/v1/timer HTTP/1.1\r\n
Host: timer.example\r\n
Cookie: __Host-subathon_session=0000000000000000000000000000000000000000000000000000000000000000\r\n
X-Subathon-CSRF: 1\r\n
x-subathon-csrf: 1\r\n
\r\n

D: GET /api/reference/v1/timer HTTP/1.1\r\n
Host: timer.example\r\n
Cookie: __Host-subathon_session=0000000000000000000000000000000000000000000000000000000000000000\r\n
Cookie: __Host-subathon_session=0000000000000000000000000000000000000000000000000000000000000000\r\n
X-Subathon-CSRF: 1\r\n
\r\n
```

Native wire counts remain A237/B241/C199/D277. A/D →401/not_authorized;
B/C →403/not_authorized, all without crypto/current session lookup. Remove A's
second Authorization for desktop control (native149 bytes), or D's second Cookie
for browser control. A valid control must actually expose the source and prepare
digestZ=`a226d544ff92f3247dc784e949c846999fde801e4581c6bb219fde070591d3ea`
plus ORIGINAL generation; it is not authenticated until current supplier resolution.
At Caddy ingress, count its actual forwarded metadata/Via separately below.

The original partial I01–21 receipts remain in4b0178cc; this table is the explicit
reassessment. No future implementation generated an expected result.

| ID | Literal variation/control | Required revised oracle |
| --- | --- | --- |
| I01–02 | `Authorization: Bearer Z, Bearer Z`; `Authorization:` empty; empty Authorization plus canonical account Cookie |401 before Origin/crypto/lookup; Z denotes the complete64-zero literal above, no fallback. |
| I03–04 | Same account pair twice in ONE Cookie joined `; `; or twice across Cookie fields; compare `Cookie: a=1`, canonical account Cookie, `Cookie: a=2` | Duplicate account401, including identical values. Unrelated duplicates preserve/count every field/pair and permit browser transport control. |
| I05 | Empty Cookie; `Cookie: a=1;;b=2`; quoted account value |400/400/401 respectively; no ignored empty pairs or quote repair. |
| I06–07 | Repeated/empty Origin; `Origin: null`; comma-combined Origin/marker; browser Cookie plus `Authorization: Basic synthetic`; desktop Bearer plus Origin/marker | Origin/marker403; mixed credentials401 FIRST. Empty occurrences remain present. |
| I08 R2 | Native obs-fold, bare LF, colonless line, whitespace-before-colon, NUL/DEL | Native400/close before map/log. At candidate EDGE only, R2's legacy folding/LF/OWS normalization is permitted: folded `Bearer\r\n Z` may prepare; token-internal fold401; duplicate/empty security occurrences never disappear. |
| I09 |65 fields; semantic name+value8193; raw head40961, either partial or already complete; reference projection4097 |431/close at relevant producer/semantic bound before excess copy; no completed-line bypass or fitting prefix. |
| I10 R2 | External desktop149-byte control plus `X-Pad: `+3938 SP+`a\r\n` |4097 external bytes is NOT automatic failure. Normalized159 plus forwarding87+Via16 gives candidate backend262 and may admit. With same exact complete field set, X-Pad3835 `a` bytes gives app4096;3836 gives4097→431. |
| I11 R2 | Native repeated CL0; candidate edge repeated trimmed CL0 vs0/00 or0/1; `Content-Length: +0`, `1x`, UINT64 overflow; any CL+TE/chunked | Native duplicate CL400/close; EDGE identical CL may normalize, unequal/malformed refuses. EVERY transfer-coded edge request refuses before backend, using retained TransferEncoding. Unsupported TE may produce fixed501; no framing fallback. |
| I12 | GET CL4 body `null`; CL4 but `nul` then EOF; valid request plus second request in same receive quantum | Route400/no account lookup; incomplete closes0dispatch; later request separately scoped/admitted, no pooled raw duplicate. |
| I13 | Spoofed Forwarded/X-Forwarded-* on untrusted peer; Host `timer.example.evil`, `timer.example:444`, duplicate Host, userinfo/zone/invalid brackets; absolute URI or literal fragment/non-roundtripping target | No trusted source; exact owning400/refusal/503/close,0protected effect. No suffix/SNI/forwarding fallback. Edge original URI roundtrip prevents repaired authority/fragment forwarding. |
| I14 | Actual configured proxy with missing/duplicate/unprovable scheme/host facts; wrong peer/channel; unexpected active artifact/config/reload |503/unavailable or close,0lookup/effect; valid actual isolated-peer control required. No mandatory mTLS/envelope oracle. |
| I15 R2 | h2-only ALPN; h2c/prior-knowledge `PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n`; H3; ASCII HTTP/3.0 request line | No admitted JSON request. Candidate uses H1 only; Go unsupported ASCII version505/close has0backend dispatch and fixed nonreflecting body, not HTTP3 capability. |
| I16 | Unsafe browser request without Origin; cross-origin/unknown-route OPTIONS; parser-limit/error paths | No protected processing or access-control allow permission on ANY path; no stock reflected CORS. |
| I17 | Prepare underg1, publish changed origin/trust/limit/artifact/log policyg2 before execution |503/unavailable,0supplier calls/effects/private disclosure. A FRESH actual request underg2 may prepare; old digest/reference never rebound. |
| I18 | Expired/reentrant/wrong-thread native lease/source, bad ABI/artifact/profile, missing policy owner, invalid limits, exhausted generation, unadmitted direct TLS | Unavailable/close; never manufactured secure/lossless facts. |
| I19 | Distinct canaries in Cookie/Authorization/Expect/path/query/arbitrary header/body/forwarded fields; actual TRACE/debug/matcher error/upstream-down/export/reload/shutdown | No canary or encoded fragment in reachable persistent/diagnostic sinks. Pre-log source audit proves raw Cookie/Expect/path sites bypassed/removed; late redaction alone fails. |
| I20 | Allocation/copy/receive-compaction failure, cancellation/timeout/EOF/shutdown before and during scope; later pipelined bytes present | Native caps/pre-read8192 invariant hold; no spill/growth/unwiped released owner, retained view/raw Qt token or partial protected dispatch. Pending bytes stay bounded and wipe on close. No global/TLS-heap erasure claim. |
| I21 | Reference gzip or repeated Content-Encoding |415/invalid_primitive before transport preparation; no decompression. |
| I22 | `Connection: keep-alive` then `cOnNeCtIoN: cOoKiE`; `Connection: Authorization, Origin, X-Subathon-CSRF`; individually nominate Host/CL/Accept/conditional/correlation/forwarding | Fixed edge400/close,0backend dispatch. ALL joined occurrences checked; allowed absent/empty/close/keep-alive lists have real positive controls. |
| I23 | Native control plus incoming `Via: 1.0 edge-a` and `vIa: 1.1 edge-b` through candidate | Both values preserved in order, added `Via: 1.1 Caddy`; count all3=50 semantic bytes. No Via trust/discount. With forwarding fields and base149, complete projection286 without X-Pad; with normalized X-Pad:a,296. |
| I24 | `Expect: synthetic-secret` vs empty/100-continue; malformed/overflow head above conservative40960 at EDGE | Nonempty unsupported Expect fixed Go417/close before Caddy; empty/100-continue that reaches Caddy fixed400. Edge oversize parser431/refusal/close as R2; no backend/echo/CORS/auth lookup; finite cleanup still observed. |

Exact forwarding vector in I10/I23: X-Forwarded-For=`127.0.0.1`,
X-Forwarded-Proto=`https`, X-Forwarded-Host=`timer.example` adds87 semantic bytes.
Caddy Via adds16. Every additional configured field requires independent recounting;
actual production peer addresses are not assumed to be these fixture bytes.

SESSIONTEST-011 owns real selected-native capture/ABI/log/allocation/lifetime tests;
SESSIONTEST-010 owns actual source/policy/transport binding on API-001;
DEPLOYMENTTEST-007/008 own actual external edge/alternative normalization/trust/log
profiles. Each authors independent literal oracles/harness, obtains different test
review, proves mandatory valid controls on a working selected stack, then observes
its own intended BEHAVIOR red before implementation. A fake source, static audit,
setup/config parse/compile failure or all-unavailable control is not that red.
Do not replay old supplier/spike reds or alter their assertions.

Use disposable local sockets/CA/state and synthetic secrets, no provider traffic.
Send literal raw octets rather than a client library which may merge/repair fields;
cover every split around CRLF/colon/token/limit/body, coalesced/pipelined requests,
read suspension, slow input, disconnect and shutdown. At the edge include all R2
Connection/TE/metadata/URL/417/505/cleanup controls and inspect the native capture
independently; backend tests cannot prove edge behavior. Record exact artifact/
patch/ABI/config/toolchain/TLS/platform and actual peer facts. No claim of original
external4096 or H2 encoded4096 remains after R2.

Capture all relevant production sink boundaries from launch to shutdown using
bounded disposable test observation (overflow=FAIL), search full/fragment/encoded
canaries, and establish a scanner positive control in an isolated harness sink.
Do not publish plaintext captures as CI artifacts; report only case/sink/count/
offset and erase temporary captures after attribution. Instrument BEFORE copies/
allocations/releases and audit source sites; RSS and output absence do not measure
preallocation or zeroization. Independent source/resource/secret/configuration
review and actual selected CI must follow passing unchanged tests.

## 7. Explicit implementation and release gates

This document's complete-solution review is separate from006's boundary/source
review. Native011 owns actual narrow patch/build/capture ABI/lifetime and native
finite owners;010 owns actual AccountIngressSource/current-policy binding;007/008
own loaded proxy profiles and resource/log/authority/peer evidence; API001 owns
production lifecycle/serialization/backpressure; SESSION003 owns complete routes/
current authorization/egress/closed CORS/browser/native behavior. Their reviewed
baseline/red/implementation/code review/CI gates remain required before enabling
any supported path. Unknown TLS/native/concurrency/config/sinks are unavailable,
not omitted capability details. No additional reusable runtime is implemented or
implicitly approved; split a genuinely necessary expansion before code.

Current clock/physical session store, accepted login/permission/restore, full
WebSocket/cookie/Windows/Linux client/platform/security/deployment/package gates
remain with their owners. All seven integrations/three FX providers/full release
scope remains. The author evidence records document/source/literal/tooling checks
only, never runtime acceptance, feature red or completion of dependent behaviors.
