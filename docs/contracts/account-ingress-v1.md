# Account ingress — v1, partial author checkpoint revision 1

SESSIONCONTRACT-005; specification v0.45 §§2/10/16/17, SERVER-01,
AUTH-12/15, API-01 and TEST-04/05. Identity `subathon.account-ingress.v1`.
This is a partial DOCUMENT proposal for different actual review. SESSIONCONTRACT-005
is NOT complete: proxy semantics/solution and any necessary explicit contract
revision or prerequisite split remain open. No patch, dependency, listener, proxy
configuration or supported client is admitted by writing it.

Import [account transport v1](session-transport-v1.md) §§1–5, blob
`2c33c2bbe9052455107c3056783d6bb605a5fef2`; [supplier v1](session-resolution-v1.md),
blob `c5f5a6e71664042e18475068d3caf577b0c8d009`;
[reference HTTP v1](reference-http-v1.md), blob
`54f1b65a3000a8945513fa5158d0a22e50712d99`; and
[common wire v1](wire-primitives-v1.md), blob
`11eeb5f422a174e042143df6b476ae1d717eeacf`, unchanged.
The supplier prepares a digest; ingress never grants identity or permission.
Reference HTTP's smaller 4096-byte external/semantic request limits remain exact.
This proposal adds no endpoint, account-token syntax, public authorization service,
provider behavior, persistent queue or authoritative writer.

## 1. Verified starting point and proposed correction

Selected source remains Drogon 1.9.14
`3aae769cc405cf5abb9d477675c6430924f02c7c` and bundled Trantor 1.5.29
`78e480f5e138acdc05f76c55a2716718cd846364`, as in
[setup](../../tools/setup_drogon.py) and
[dependency evidence](../evidence/spike-001-dependencies.md). Author evidence
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

**Proposed selected-stack correction:** retain Drogon/Trantor/OpenSSL and add an
explicit, listener-bound strict HTTP/1.1 ingress path inside the selected source.
Use a reviewed downstream patch set against these exact commits, with original
licenses, immutable patch/build manifests and a distinct installed artifact identity.
Do not edit the shared cache or label a patched binary as pristine 1.9.14. No new
HTTP library is justified merely to replace this small parser boundary; replacing
it would require a separate compatibility/security/dependency decision.

The patch has three explicit integration points:

1. At `TcpConnectionImpl::readCallback`/TLS plaintext production, admit storage
   before reading/growing as §3 specifies. Ordinary `MsgBuffer::readFd`, its spill
   buffer and growing `recvBuffer_` are bypassed for this listener profile. This
   setting is fixed when the connection is created, before the first read.
2. In `HttpRequestParser`, before `setPath`, `addHeader`, request-body reserve,
   temporary-file creation, `Expect` processing or generic request logging, parse
   the bounded original head into the fixed owner below. Validate complete lines,
   syntax, multiplicity and framing using that owner. Never call original
   `addHeader` on Cookie, Authorization, Origin, marker or arbitrary raw fields.
3. At `HttpServer::onMessage`, dispatch one admitted owner to the trusted route
   binding, bypassing its request-pool/pipeline accumulation for this profile.
   A private `AccountIngressSource` borrows this owner during one synchronous
   transport call. No source can be constructed from a public `HttpRequestPtr`,
   request attributes, booleans, client metadata or a decoded map.

Only a minimal nonsecret routing/framing projection may enter an ordinary Drogon
request: a trusted matched route identity and validated method/content length.
Raw target, query, headers and body stay in the bounded owner; account parsing,
JSON parsing and request errors use their owning contracts rather than Drogon's
convenience deserializers. No substitute credential enters cookies_/headers_.
Other endpoint families require their own finite profiles; do not route provider
webhooks, assets or WebSockets through this account JSON profile by accident.

This is a real correction proposal, not an available public parser hook. Its
compatibility review must cover route dispatch, keep-alive, early error output,
TLS metadata, shutdown/cancellation, content decoding and unchanged bridge tests.
Compile/link version strings alone cannot identify the admitted patched artifact.

## 2. Exact wire profile and failure order

This first proposed account path accepts HTTPS HTTP/1.1 only. Advertise only
`http/1.1` by ALPN; reject a selected different protocol before HTTP parsing.
No cleartext upgrade, h2c, prior-knowledge HTTP/2, HTTP/3, CONNECT, STARTTLS,
absolute-form/authority-form target, obs-fold or trailer is admitted. An absent
ALPN may use strictly parsed HTTP/1.1. Do not reinterpret an HTTP/2 preface as an
ordinary request. Public plaintext listeners do not process credentials or redirect
credential requests; they close. Backend plaintext is a different configured peer
profile (§4), never evidence of direct HTTPS.

HTTP/2 is **unavailable in this profile**, including at a terminating proxy.
Transport v1's allowance for split Cookie fields remains unchanged for a future
actual HTTP/2 implementation. Such an implementation must preserve decoded
occurrences, reject invalid/duplicate pseudo-headers and connection-specific fields,
count encoded HEADERS/CONTINUATION/DATA separately, cap HPACK table/work/expansion,
and retain every cookie pair when RFC9113's `; ` combination is used. Advertising
SETTINGS_MAX_HEADER_LIST_SIZE or flattening a decoded header map proves none of
those bounds. There is no silent downgrade after partial HTTP/2 processing.
[RFC9113 field validation and Cookie rules](https://www.rfc-editor.org/rfc/rfc9113.html#section-8.2).
Browser/native HTTP/1.1 compatibility still needs actual supported-client evidence;
protocol refusal does not complete or reduce the agreed browser/WebSocket scope.

Parse octets, not Unicode whitespace. Require exact CRLF, one SP between request
line components, method ASCII token length 1..16, target 1..2048 octets beginning
`/`, exact `HTTP/1.1`, and no control/space/fragment in target. Owning routes reject
queries/encoded aliases where required. Require one Host field with a syntactically
valid authority, with only outer SP/HTAB removed. Names are nonempty ASCII HTTP
tokens, no whitespace before colon. Values reject NUL/DEL/CR/LF and controls other
than HTAB. Empty values remain present. Preserve original name/value occurrences,
field order and all value bytes except permitted outer SP/HTAB; do not lowercase
values, percent-decode, unfold, comma-combine, choose first/last or trim `isspace`.
Colonless nonempty lines and whitespace-prefixed lines fail rather than ending a
head. The only header terminator is the empty CRLF line.

For this account JSON profile, transfer coding is refused: ANY Transfer-Encoding,
Trailer, Upgrade or Expect field is 400/invalid_primitive and close. Require at most
one Content-Length with nonempty ASCII digits only, checked accumulation, no sign,
comma/list or embedded whitespace; zero and leading zeros are legal. Repeated
Content-Length rejects even when equal; Content-Length plus Transfer-Encoding
rejects before any body read/100 response. Absent Content-Length means zero body.
Route-level content-type/body rules still apply: a body-required route with no
body fails its own contract. Content-Encoding is absent or the owning route's
single `identity`; no request decompression. A future chunked path needs separate
reviewed framing/chunk/extension/trailer/wire-work bounds; the existing permissive
chunk parser is not that implementation. This is an explicit proposed framing
restriction for review, not an assertion that the reference contract requires
chunked support or a restriction on webhook transport.
[RFC9112 parsing, whitespace and message framing](https://www.rfc-editor.org/rfc/rfc9112.html#section-6.3).

Consume exactly the validated body length. Before an earlier request is dispatched,
bytes beyond it may remain in the one bounded receive quantum, but are not parsed
as a second request. Pause reads while one request is pending; resume only after
its raw owner is erased and normal response ownership permits it. A later pipelined
request is independently admitted under the then-current policy. Excess pending
bytes, unsupported framing, early EOF, timeout, cancellation or an unsafe parser
state closes and wipes; it never triggers reparsing/fallback or partial dispatch.
No response bytes are emitted from an unverified request fragment.

Early producer failures use fixed nonsecret classifications. Oversize head/count
or field → bounded 431/resource_limit then close when safely possible; oversized
body → 413/resource_limit; malformed/unsupported framing → 400/invalid_primitive
then close; oversized target/request line → 414/resource_limit then close.
Framing/desynchronization, TLS and inability-to-form-envelope may simply close.
Use transport §4's bounded no-store/CommonFailure envelope when one can safely be
formed; do not invoke the stock direct error writer or echo a parser exception.
At the complete borrowed semantic boundary, transport §3's ordering/status table
is unchanged. Outer producer failures precede that helper; no promise to discover
an unseen later overage after an earlier invalid wire line.

## 3. Finite allocation and owner lifetime

These are proposed frozen engineering ceilings, not measurements or global memory
promises. All arithmetic, container max_size, terminator and representation checks
precede allocation/read/copy. Invalid trusted configuration fails readiness with
503/unavailable; allocation failure closes/unavailable without an input diagnostic.
Every route profile must be frozen before listening. A route cannot enlarge limits
from a request header. Reference HTTP has its exact smaller caps at both external
and backend semantic boundaries; a 4097-byte original reference request cannot
become admissible because a proxy shortened it.

| Owner/stage | Proposed finite bound and order |
| --- | --- |
| Direct/proxy-backend socket receive | One preallocated 16384-byte slab per admitted connection; each syscall receives only remaining writable space. No spill buffer, geometric growth, reallocation or plaintext duplication during compaction. Wipe consumed regions before reuse, and entire slab on error/destruction. Pause/close rather than growing. |
| HTTP head parser | One fixed 34816-byte arena: complete request line <=2048 including CRLF, complete field section <=32768 including final CRLF. One fixed array of 64 offset/length records. Per raw name+value <=8192; raw OWS/separators still count in the complete raw head. Check length while scanning even when CRLF is already present. Check the next occurrence before recording/copying it. |
| Semantic head | Imported H=2+sum(name+value+4)<=32768, field_count<=64, name+value<=8192, cookie_pairs<=64. Count every occurrence and unrelated pair; at most fixed counters plus imported helper's sixteen views. A smaller route profile is checked against the original raw bytes and the complete semantic projection before dispatch. |
| Body | One route-owned buffer projected from the checked Content-Length before any copy; general account JSON ceiling 65536 bytes, reference ceiling 4096 with its complete request <=4096. Count complete raw line+head+body <=100352 in the general profile, without enlarging either component. No temp file, JSON DOM or decompression before the owning route's pre-scan. |
| TLS provider adapter | Separate fixed ciphertext input slab 18432 bytes and fixed plaintext output slab 16384 bytes; consume/decrypt in bounded quanta into the socket/HTTP consumer, not a grow-until-WANT_READ loop. Check capacity before BIO feed or SSL_read and dispatch/drain before another quantum. Pending encrypted output has a separate 65536-byte cap before growth; pause/close on full. No application plaintext is placed in an unconstrained BIO or framework buffer. |
| Secret-bearing application ownership | At most one receive plaintext slab, one head arena and one body owner coexist: 16384+34816+65536=116736 bytes per general request. TLS output plaintext adds 16384 →133120. These ceilings include arbitrary bytes, not just the selected token. Supplier's fixed 124-byte application scratch and one 32-byte digest are additional; native scratch/fixed field records are separate. No extra full head/body copy, request serialization or owning header map is allowed. |
| Framework pending work | One active raw account request per connection, no raw owner in a request pool, pipeline vector, Qt task, retry cache or completion. API-001 must admit finite connection counts and finite active/queued command bytes before ownership; its actual reviewed budgets are not supplied here. Reference HTTP's existing <=64 queue/1048576-byte aggregate/per-item16384 caps remain unchanged. |

These bounds require actual source changes; existing MsgBuffer/TLS behavior does
not satisfy them. The TLS slabs bound adapter-owned byte buffers, **not** all
OpenSSL allocations, certificate chains, handshake state or process RSS. A selected
OpenSSL version/configuration/native review must separately freeze handshake byte/
time/certificate-chain/session-cache/work limits and account for internal record,
BIO and output ownership before claiming a supported direct TLS path. If that
cannot be established, the path remains unavailable; do not invent a number from
an SSL_read result or silently borrow the general host memory limit as proof.
Operating-system socket buffers/ciphertext, TLS implementation scratch/registers,
allocator overhead and other tenants are additional external owners.

The head owner is bound to one connection/request and immutable through the
synchronous source/transport call; returned string_views cannot outlive it. After
transport returns, move only digest+transport kind+owned policy reference to the
queue, wipe raw header arena/consumed receive/TLS plaintext immediately, and retain
only the route's independently bounded noncredential request body until its own
parse completes. A malformed body can contain a token; erase that body on every
success/error/cancel/destruction path after route handoff. Use selected libsodium
secure erasure, including fixed stack scratch, before release, pool reuse or
compaction; `clear`, `resize`, destruction and ordinary memset do not prove it.
The outgoing credential owner/emitter remains SESSION-002/003, not supplied here.

Owned memory and timing invariants need source audit plus allocation/copy/wipe
fault injection. No assertion of heap-wide, kernel, swap, registers, debugger,
crash-dump or third-party TLS-buffer zeroization follows. Deployment owns dump/
ptrace/swap policy and platform validation; accepted supplier owner limits remain
unchanged. [Libsodium secure memory](https://doc.libsodium.org/memory_management).

## 4. TLS, authority and proxy trust

The configured origin is trusted installation data: ASCII canonical HTTPS origin,
no path/userinfo/query/fragment/trailing slash; canonical DNS ASCII host, bracketed
IPv6 where needed, effective port443 omitted. Validate it once before publishing
policy, with a reviewed URL/authority canonicalizer. Request Host/Origin/forwarded
headers cannot configure it. Raw Origin must still equal that serialization exactly
under transport v1, including case/port spelling rules; Host is parsed as authority
and compared by canonical host/effective port, never suffix/substring.
Reject missing/repeated Host, userinfo, invalid bracket/port, forbidden control,
ambiguous IPv6 and absolute request targets before account parsing. Trusted route
selection happens only after authority admission. Explicit aliases need a new
configured policy; they are not automatically same origin.

**Direct listener:** derive secure transport, negotiated protocol and (where used)
SNI from the actual listener/TLS session, never HttpRequest attributes or a forwarded
header. Require successful server-side TLS >=1.2 with certificate/private-key
configuration for the canonical host and no 0-RTT protected processing. Check
external authority against that same listener installation; SNI alone grants no
HTTP authority. The TLS/library/configuration path must be admitted by actual tests
and native review. Loopback plaintext with `X-Forwarded-Proto: https` is not direct
TLS. The direct path is a component/deployment option, not a replacement for
required bundled Caddy, alternate-proxy packaging or its certificate ownership.

**Terminating Caddy or alternative proxy:** backend is bound to a private network
endpoint with an explicit single-hop peer allowlist and firewall/container rules.
Prefer a private Unix socket with verified process/permission ownership when the
selected stack actually supports it; do not claim that capability here. Otherwise
use dedicated loopback/private TCP and authenticated backend TLS with a configured
proxy client certificate identity; IP trust alone cannot distinguish other local
processes. Backend TLS authenticates the peer, not the original public request.
No public bind, broad private_ranges trust, unreviewed multi-hop chain, PROXY
protocol or client-selected proxy mode is accepted. Each rootless/rootful/native
network layout requires actual reachability and spoofing evidence.

The edge must validate canonical external HTTPS authority before forwarding; strip
ALL client instances of Forwarded, X-Forwarded-*, X-Real-IP and reserved Subathon
proxy metadata; emit exactly one configured original scheme/authority value and
its admitted edge-policy identity. Backend requires the exact authenticated peer,
known edge-policy revision, strict one-field syntax and exact configured authority.
The exact metadata representation is unresolved; the following is a proposed trust
requirement, not a finalized proxy wire API. Missing, repeated, malformed, unexpected
or stale trusted metadata →503/unavailable, never
fallback to Host or a different trust mode. Untrusted peers carrying these fields
cannot supply AccountIngressSource at all. Configure backend Host explicitly and
validate it separately; upstream TLS SNI/Host need not equal the public authority.
The edge/backend binding must meet the original-request/limit interpretation
resolved by the open audit below; a secret shared metadata header alone is not
peer authentication.

Caddy's [reverse_proxy documentation](https://caddyserver.com/docs/caddyfile/directives/reverse_proxy#headers)
describes forwarding controls and upstream protocol choices; its
[server options](https://caddyserver.com/docs/caddyfile/options#servers) expose
protocols, max_header_size, timeouts, strict_sni_host and logging controls. These
are configuration ingredients, not proof of original occurrence/byte preservation.
The [access log documentation](https://caddyserver.com/docs/caddyfile/directives/log)
describes default sensitive-header redaction. That does not exclude secret URLs,
bodies, custom headers, error/runtime/debug logs or earlier parser transformations.
No Caddy version/Go toolchain is pinned by the existing setup or this proposal.
The inspected v2.10.2 release page is background, not a proposed production pin.

The **proxy completion question is open**, rather than answered by a new proxy
subsystem in this checkpoint. Transport v1 asks for original decoded occurrences
before lossy framework maps. This may mean the actual application listener's
complete ingress; reference HTTP additionally fixes original **external** wire and
semantic request bounds. Distinguish those requirements explicitly. A backend
cannot reconstruct original external field count/raw bytes from a proxy-selected
map, and a backend-only allocation cap cannot bound proxy allocations. Neither
conclusion alone establishes that a custom edge parser/envelope is necessary.

A different bounded source/contract audit must compare: (a) unchanged accepted
semantics with actual pinned proxy/parser evidence, and (b) an explicit minimal
contract revision, if justified by specification-based security reasoning. A
revision could define exactly which HTTP-compliant proxy transformations preserve
authentication ambiguity and which counts/limits belong at the edge versus the
application. It must account for repeated/combined Authorization/Origin/marker,
all Cookie pairs, obs-fold, original external reference bounds, proxy allocations,
pre-log safety, trust and revision changes. It cannot silently replace an exact
external4096 limit with a backend4096 limit or choose a convenient first/last value.
No such revision is accepted here and no stock proxy preservation is claimed.

If actual selected proxy source cannot satisfy the retained requirements and a
separately reviewed explicit revision is insufficient, record the resulting
substantial prerequisite in explicit contract/test/implementation tasks before
expansion. A new Go/Caddy parser, custom original-request handoff or proxy runtime
would need its own actual wire/lifetime/native/security/configuration review; do
not select or implement one just to close this document.

Bundled Caddy and existing alternate-proxy support remain intended release scope.
Until actual source/version/configuration and real-stack evidence establish an
admitted terminating path, it is unavailable; a successful direct harness cannot
complete SESSIONCONTRACT-005 or close the release gate. There is no promise that
an arbitrary stock proxy supports it or that adding a Caddyfile alone solves it.

## 5. Policy production, current execution and closed CORS

Propose one immutable `IngressPolicySnapshot` published by the installation's
trusted configuration owner. It contains installation binding, canonical origin,
route/protocol profile limits, direct/proxy mode, exact peer/certificate identities,
edge artifact/config revision (once its contract is finalized), parser/patch artifact
identity, logging rules and
one nonempty bounded policy revision. These are owned trusted fields, never request
flags. Validate the complete snapshot before listening; unknown artifacts, unset
limits, missing native TLS bounds or unattested edge path keep readiness closed.
AccountIngressSource holds one strong snapshot reference and one raw owner for its
synchronous call. It cannot mix origin from revision1 with proxy trust from2.

For a concrete revision source, propose a trusted persisted unsigned64 installation
boot epoch, advanced/acknowledged by the authoritative configuration writer before
enabling listeners, plus process-local unsigned64 policy generation starting1.
Serialize `i1.<boot_epoch>.<generation>` as decimal without leading zeros, at most
44 ASCII bytes. Never reuse an epoch or wrap either counter; storage/commit unknown
or exhausted values keep ingress unavailable. This needs an explicit production
configuration port; it is not an extra database implementation here. If an accepted
configuration producer already supplies an equally nonreused revision, reuse it
with a reviewed mapping rather than inventing a second writer.

Configuration changes first suspend new admission, then enter the authoritative
queue as a policy-change operation. Construct/validate a replacement and publish
its new generation on that same serialization boundary before resuming admission.
In-flight requests keep their original snapshot: their prepared revision compared
at execution differs and returns503/unavailable with **no session lookup, protected
effect or private result disclosure**. Raw input was already erased; do not retry
by rebinding a digest to the new revision, rewrite a queued reference or return
cached protected data. The client must make a fresh actual request under the new
policy, then supplier resolution/current permission runs normally. A no-op change
may keep the revision only when every trust/normalization/log/limit behavior is
identical. Canonical-origin, proxy/TLS identity, parser artifact, protocol, limits,
route binding or secret-log policy changes always advance it.

External proxy/config/TLS changes cannot silently retain old admission. Deployment
must coordinate revision/readiness, attest exact active edge and backend manifests,
and drain/refuse old channels before resuming. A local config-file hash or HTTP
header does not prove the proxy actually loaded it. Uncoordinated reload, uncertain
proxy state, policy-source failure or missing snapshot means503/unavailable or close.
API-001 owns actual startup/shutdown/queue ordering/backpressure and publication;
SESSION-003 owns full current-execution/route integration. No generic runtime
reload service is implemented by this document.

Closed CORS applies before protected work and on every response/error path:
no allow-origin/credentials/headers permission, no reflection, no wildcard or
same-site-subdomain allowance. Route OPTIONS must be handled by the owned closed
policy before Drogon's default `handleHttpOptions`; unknown OPTIONS likewise
receives no permission. Keep transport's mandatory browser marker on GET/HEAD and
unsafe requests, unsafe exact Origin, and forbidden Origin/marker on desktop
Bearer unchanged. Disable stock session cookies, generic controller implicit
OPTIONS and CORS middleware on the account family. Browser preflight refusal is
not a substitute for server-side Origin/marker validation on an actual request.

## 6. Secret/log boundary

No raw head, URL/query, request body, token, cookie, arbitrary header, JSON parser
error text, trusted proxy metadata or native exception is a diagnostic value. Remove
the selected Cookie TRACE statement in the admitted artifact and bypass/replace
Expect/path logs; review all reachable parser, TLS, routing, error, cancellation
and response paths. Disable AccessLogger and request/body dumping for this listener
rather than relying on a format blacklist. No debug/trace/packet capture facility
may be enabled on an admitted production path if it can record plaintext. Caddy
access logs, runtime debug/trace, reverse-proxy retry errors and exports have the
same rule; `log_credentials` must never be enabled. A configured severity floor
alone is not an erasure/security boundary.

Allow only a fixed event code, bounded trusted route identity/status/count and an
independently admitted nonsecret correlation reference. Never derive correlation,
metrics labels, filenames or failure text from credentials or malformed input.
No retries/queues retain raw requests. Synthetic observation harnesses may compare
predeclared sentinel bytes in disposable private captures; they must not publish
request dumps as CI artifacts. A leak finding records only case ID, sink and
byte-offset/count, with the capture erased after attribution. Default tests never
use live credentials, provider traffic or supporter data.

## 7. Literal wire oracles and future real-stack baseline

Each `\r\n` below denotes the two actual wire bytes, not four backslash characters.
Use disposable `timer.example`, canonical origin `https://timer.example`, revision
`i1.1.1`, trusted TLS fixture and protected GET route `/api/reference/v1/timer`.
The token literal is exactly64 `0` characters; it is synthetic and never issued.
Complete header blocks below end with one additional empty `\r\n` line. No parser
or implementation produced these expected outcomes.

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

The `A:`/`B:`/`C:`/`D:` labels are explanatory and are NOT transmitted.
Wire lengths A237, B241, C199 and D277 were independently counted, including
every CRLF and Host. All fit the reference4096-byte bound.
A →401/not_authorized without
crypto/session resolution; B/C →403/not_authorized without crypto/resolution;
D →401/not_authorized without first/last cookie selection. Case-insensitive field
names do not change multiplicity. Later unsupported method/route rules cannot
turn any case into authorized work.

Use A with its second Authorization removed as valid desktop control: one digestZ
plus original policy reference; no identity until queue-time supplier resolution.
Use D with its second Cookie removed as browser safe-read control with marker1 and
no Origin. These controls must reach the real source; a suite in which every request
is503/closed does not prove duplicates were handled.

| ID | Literal replacement/addition to a control, or exact bounded construction | Expected oracle |
| --- | --- | --- |
| I01 | One `Authorization: Bearer 0000000000000000000000000000000000000000000000000000000000000000, Bearer 0000000000000000000000000000000000000000000000000000000000000000` | 401; combined value is not alternatives. |
| I02 | `Authorization:` empty; or empty plus valid account Cookie | 401, no cookie fallback. |
| I03 | One Cookie value `__Host-subathon_session=0000000000000000000000000000000000000000000000000000000000000000; __Host-subathon_session=0000000000000000000000000000000000000000000000000000000000000000` | 401, even identical pairs in one field. |
| I04 | Cookie field1 `a=1`; field2 canonical account pair; field3 `a=2` | Browser control succeeds transport; every pair/field counted, unrelated duplicates legal. |
| I05 | `Cookie:` empty; `Cookie: a=1;;b=2`; `Cookie: __Host-subathon_session="0000000000000000000000000000000000000000000000000000000000000000"` | Empty/empty-pair →400; quoted account token →401. |
| I06 | Browser control plus `Origin:` empty, `Origin: null`, `Origin: https://timer.example, https://timer.example`, or marker `X-Subathon-CSRF: 1, 1` | 403; empty/combined fields remain present. |
| I07 | Browser control plus `Authorization: Basic synthetic`; desktop control plus `Origin: https://timer.example` or marker1 | Mixed credentials401; desktop Origin/marker403. |
| I08 | `Authorization : Bearer synthetic`; name `Bad[Name`; value containing literal NUL/DEL; `\r\n X-Subathon-CSRF: 1`; bare LF; colonless `secret-canary` line | Producer400/close; no map/log/dispatch. |
| I09 | Generic head with exactly65 separate `X: a\r\n` fields; or one name/value total8193; incomplete field past32768 section bytes; same oversized field already followed by CRLF | 431/close before excess record/copy/growth, including completed-line case. |
| I10 | Reference valid desktop control padded by one unrelated `X-Pad` field to exact complete4096 bytes, then4097 | Desktop control149 bytes; X-Pad value3938 gives exact4096, name+value3943. First admitted; one extra padding byte gives431/resource_limit or safe close before dispatch. |
| I11 | Two `Content-Length: 0`; `Content-Length: 0, 0`; `Content-Length: +0`; `Content-Length: 1x`; length beyond UINT64_MAX; CL0+`Transfer-Encoding: chunked`; TE alone; `Expect: synthetic-secret-E1` | 400/close, no interim100/body reserve/raw warning. |
| I12 | Length4 with exactly `null` body on body-forbidden GET; length4 with only `nul` then EOF; valid body plus another complete request in same receive quantum | Route400 for GET; short body closes with0dispatch; pipelined later request separately admitted once earlier owner is erased. |
| I13 | Plain client carries `X-Forwarded-Proto: https`, `Forwarded: proto=https;host=timer.example`; Host `timer.example.evil`, `timer.example:444`, repeated Host, `timer.example@evil`, absolute `https://evil/...` target | No valid ingress; close or unavailable; malformed head400. No trust from spoofed forwarding/SNI. |
| I14 | Authenticated backend peer but missing/duplicate/stale trusted edge metadata; public/untrusted peer supplies exact valid-looking metadata; TLS required proxy certificate absent | Unavailable/close,0protected dispatch. No Host-based fallback. |
| I15 | HTTP/2 ALPN offer only, h2c upgrade, literal prior-knowledge `PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n` | No account ingress. Offer with h1+h2 must select h1 only; no accidental h2/H3 claim. |
| I16 | Browser unsafe request lacks Origin; cross-origin OPTIONS requesting `X-Subathon-CSRF`; unknown-route OPTIONS; parser-limit/error response | No protected processing, no access-control allow permission on any response. |
| I17 | Prepare valid digest under `i1.1.1`, pause before queue execution, publish `i1.1.2` with changed origin/trust/limit/log artifact | 503/unavailable,0supplier calls/effects/private disclosure; fresh valid request under2 may prepare normally. Never rebind the old digest. |
| I18 | Policy source throws/is missing; invalid positive limits; boot/generation exhaustion; TLS/native/edge artifact not admitted; source owner expired | Unavailable/close; never fabricated secure/lossless facts. |
| I19 | Valid synthetic token, then failures containing distinct canaries in Cookie, Authorization, Expect, path/query, arbitrary header, body, proxy metadata; enable actual library TRACE in isolated harness | No canary bytes/fragments in ANY production sink; source hooks observe original Cookie TRACE and raw Expect/path sites cannot receive raw values. A late redaction-only pass fails. |
| I20 | Fault at each admitted allocation/BIO feed/head/body copy, cancel/timeout/close/shutdown with partial and complete head | No raw buffer growth above cap, no retained view/Qt token, all owned secret regions wiped before release/reuse, no raw error text,0partial protected dispatch. |
| I21 | Reference control with `Content-Encoding: gzip` or two Content-Encoding fields | 415/invalid_primitive before transport preparation, no decompression/lookup/effect. |

SESSIONTEST-010 must turn this map into separately reviewed, specification-linked
real-stack tests and literal independent bytes/digest/size oracles, then establish
its own intended behavioral red. Do not run historical supplier/spike red again.
The future harness must compile/run the actual admitted source and API-001 lifecycle,
listen on disposable loopback TLS with disposable test CA/certificates, and send
raw octets through sockets/TLS rather than a client library which might normalize
duplicate fields. Parameterize every byte-split position around CRLF, colon, token,
body and limit boundary; include several requests in one read, slow partial input,
read pause/resume and disconnect/shutdown. No live internet/provider/auth endpoint.

For proxy acceptance run those SAME external wire oracles through the exact pinned
Caddy/Go/patch/config image and each claimed alternative; inspect its first-parser transformation/bounds facts and backend independently.
Finalize proxy oracles after the open contract audit; do not fabricate an envelope. Observe actual client certificate/peer,
ALPN, external scheme/authority and original byte/count facts. Include untrusted
network path and spoofed headers, wrong upstream/public Host/SNI and reload races.
A direct-only pass, fake AccountIngressSource, proxy-selected map or backend-only
packet capture cannot establish original external multiplicity/bounds.

Capture all framework/application/proxy stdout/stderr/log/export/error sinks from
startup through shutdown, with bounded disposable capture and overflow=FAIL. Search
exact full canaries and predeclared distinctive fragments/encoded forms; establish
the scanner's positive control by injecting a sentinel into an isolated harness
sink, never by permitting a production leak. Source audit must enumerate unobserved
sinks and copy/ownership sites; absence in logs is not proof of erasure. Allocation
instrumentation must fail/observe calls BEFORE growth, including completed
oversized lines and TLS production; RSS sampling is insufficient. Fixture identity,
actual binary/library/patch/config manifests and platform versions belong in the
receipt. Intended red means reached ingress behavior violates a reviewed oracle;
missing dependency, compilation failure, disabled test or all-unavailable control
is not red. Independent test review precedes that run; separate code/resource/
secret/configuration review and unchanged regression CI precede a pass claim.

## 8. Partial handoff and proposed prerequisite decisions

This checkpoint supplies source findings, a direct-listener correction proposal,
finite application/adapter bounds and proposed raw-wire oracles. It does **not**
finalize the proxy solution, native TLS internal bounds, exact production policy
port or an admitted patch/build identity. It is not ready to establish the complete
SESSIONTEST-010 baseline; independent source/contract audit and resolution of the
proxy question remain required. Leave SESSIONCONTRACT-005 unfinished/blocked if
those requirements need new prerequisite tasks.

The receive/TLS allocation-and-erasure correction is a substantial reusable
selected-runtime prerequisite candidate. The proxy audit may identify another.
Before implementation, the integrator decides the focused task/PR decomposition,
creates explicit contract/test/implementation records and dependencies, and agrees
shared selected-source/patch/build/deployment ownership. Do not expand SESSION-010
into a new networking runtime or proxy subsystem without that split. A smaller
verified correction or explicit separately reviewed requirement revision should be
considered before adding machinery. Preserve source evidence and original oracles
when revising; document specification reasoning and compatibility consequences.

Each eventual executable slice follows contract/examples → independent test review
→ intended real-stack red → implementation → different source/native review →
selected CI. Reuse existing selected tools where sufficient; compare requirements,
source pins/options/licenses and compatibility before changing an artifact.
SESSION-010 later supplies actual admitted ingress/source only. API-001 retains
real bridge/startup/shutdown/queue/backpressure and current policy serialization;
SESSION-003 retains complete parsing/current authorization/egress/route/CORS/client
integration. Direct/proxy TLS, native secret handling, Linux deployment modes,
Windows/Linux clients, browser cookies/HTTP/WS and release/security gates remain
explicit and unverified here. No storage-contract work, all-seven-integration or
three-exchange-provider requirement is removed. The author note records only
read-only source/document work; no test, red, implemented or supported capability
follows from this partial proposal.
