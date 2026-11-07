# Reference JSON HTTP — v1, revision 1

CONTRACT-013; authority: specification v0.45 §§2/3/10/17, API-01/02/03,
NUM-04 and [reference path R1](reference-path-v1.md). Identity
`subathon.reference-http.v1`. The concrete package is
[manifest.json](../../schemas/http/reference-v1/manifest.json), with exactly one
[OpenAPI 3.1.1 document](../../schemas/http/reference-v1/api.json). This is an
contract with separate actual review and observed package validation recorded in
[the task handoff](../evidence/contract-013-handoff.md); it supplies no HTTP
implementation, supported server version or capability.

Import [common wire v1](wire-primitives-v1.md),
[current account supplier v1](session-resolution-v1.md) and
[account transport v1](session-transport-v1.md) without changing their semantics.
Package-local primitive schemas are bounded reference field profiles of those
logical imports; no common concrete schema exists to import. Their limits do not
narrow domain storage, rational, receipt, rule or general wire profiles. No scene,
WebSocket, provider, login, mutation or contribution-submission schema is imported.

## 1. Routes, compatibility and exact HTTP envelope

| Method and exact external path | Access and result |
| --- | --- |
| POST `/api/reference/v1/negotiate` | Public, nonsecret negotiation; never authenticates, creates a session, or inspects timer/account existence |
| GET `/api/reference/v1/timer` | Current account and granular timer-read permission; coherent committed timer projection |

Paths are case-sensitive and have no trailing slash, percent-encoded alias or
generic-v1 alias. Only these two routes belong to this package. Unknown paths
(including `/api/reference/v2/timer`, scene and login paths) have no reference
handler and return 404/invalid_primitive under this listener's bounded failure
envelope. This does not prohibit separately owned release endpoint families.
Unsupported methods on the two paths return 405/invalid_primitive with `Allow:
POST` or `Allow: GET`, respectively. HEAD and OPTIONS are not implicit handlers;
cross-origin preflight receives no CORS permission. No redirect is used.

The actual external origin is the installation's canonical HTTPS origin, admitted
under the imported ingress policy, including trusted proxy/TLS and exact authority
checks. This package does not supply a new trusted ingress adapter. Raw HTTP
framing, conflicting Content-Length/Transfer-Encoding, invalid pseudo-headers,
request smuggling and unprovable/lost header multiplicity are rejected or closed
by the listener before a reference request exists. No reconstructed framework map
counts as lossless ingress. The selected Drogon limitation and its existing
API-001/SESSION-003 runtime gates in the transport import remain unresolved here.

Both paths forbid any query component, including an empty `?`, fragment, body
credential, version/capability header override or request-selected auth purpose.
No `If-*`, Range or other conditional header changes this operation: conditionals
and range headers are rejected as 400/invalid_primitive, so success is always a
complete 200 body, never 206/304. Ordinary transport headers (Host/authority,
User-Agent, connection management) are allowed within the complete finite budget;
there is no assumption that a browser can send an otherwise closed header set.

POST has one nonempty complete UTF-8 JSON object body. Exactly one Content-Type
field is required, with case-insensitive media type `application/json`, surrounding
HTTP OWS allowed and no parameters; all other/missing/duplicate content types are
415/invalid_primitive. GET has zero body octets and no Content-Type; any body or
Content-Type is 400/invalid_primitive. Content-Encoding is absent or exactly one
case-insensitive `identity`; compressed or repeated encodings are
415/invalid_primitive. Accept is absent or exactly one OWS-trimmed
`application/json` or `*/*` (media type case-insensitive), otherwise
406/invalid_primitive. This intentionally small content-negotiation profile has no
q-values, lists or alternate representation. Content-Length and transfer framing
must describe the actual complete body; no trusting a declared size to bound reads.

`X-Subathon-Correlation-Id` is optional, at most one field. Its OWS-trimmed value
is a nonempty common CorrelationReference: even lowercase hex, 1..256 decoded
bytes (2..512 characters). Duplicate/invalid correlation is 400/invalid_primitive
with null correlation. Once safely admitted, success echoes exactly that value in
the same response header and failures place it in `correlation_id`; absent input
omits the success header and uses null. Earlier failures may use null. Never echo
arbitrary input, create a correlation from credentials, or accept it as authority.

Every response uses `Cache-Control: no-store`, `Content-Type: application/json`
and `X-Content-Type-Options: nosniff`; no Set-Cookie, credential-bearing Location,
access-control allow header, ETag or Last-Modified is produced. The emitter uses
uncompressed UTF-8 JSON, no BOM and no extra wrapper. Optional Date and actual
framing headers count in the response budget. No implicit framework error page,
debug text, account detail or request echo is allowed. Failure to safely construct
the bounded envelope closes the connection; it never fabricates a success/failure
body or claims a confirmed rollback.

## 2. Closed JSON representations and negotiation

All object fields below are REQUIRED; every object rejects additional members.
Null is allowed only where stated. Grammar applies to the decoded string value;
ordinary JSON escapes/whitespace do not change identifier/byte equality. Decoder
acceptance additionally requires section 4's raw-input checks, which a schema over
an already decoded value cannot perform.

| Representation | Exact fields and meaning |
| --- | --- |
| NegotiateRequest | `supported_versions`: 1..16 unique common ProtocolIdentifiers; `required_capabilities`: 0..16 unique common CapabilityIdentifiers |
| NegotiateResponse | `selected_version`: literal `reference.v1`; `capabilities`: unique array with 0..1 entry, the only possible entry `reference.timer-snapshot.v1` |
| TimerSnapshot | `profile_version`: `reference.v1`; `state_revision`: nonempty OpaqueBytes, 1..4096 decoded bytes; `lifecycle`: `active`; `main_ms`, `grace_ms`: nonnegative Signed64 strings; `debt_ms`: string `0`; `maximum_main_ms`: nonnegative Signed64 string or explicit null; `observed_utc_ms`: full Signed64 string; `paused`: false |
| CommonFailure | `code`: the unchanged ten-value common failure enum; `correlation_id`: CorrelationReference or explicit null; at most 1024 complete UTF-8 body bytes |

Protocol/capability identifiers are ASCII `[a-z][a-z0-9_.-]{0,63}`, exact and
case-sensitive. All exact integers are JSON strings with canonical integer grammar
and exact signed64 range; JSON numbers, booleans, `-0`, leading zeros, plus signs,
fractions, exponents, whitespace and out-of-range strings fail. The package regexes
encode the exact endpoints without floating-point conversion and use a complete
end-of-string assertion (also rejecting a final newline). Opaque fields require
even lowercase hexadecimal; `00` is one byte and differs from absence/empty/null.
The nullable cap's null means no cap; it never means unknown. No omission is filled
with zero, a current timestamp, an invented active state or a manufactured token.

The trusted enabled version preference list for THIS package is either
`[reference.v1]` or empty. Select its first mutual entry; unknown valid offered
versions are skipped. No intersection returns 409/incompatible_version BEFORE
checking required capabilities. For the selected version, test every requested
capability against its actual public enabled set; any missing/unknown capability
returns 422/unsupported_capability. Neither error accesses protected state. Both
failures carry fixed header `X-Subathon-Upgrade: compatible-client-or-server`:
clients localize the code and instruct use of a mutually supported client/server
release; no guessed version range, URL or upgrade success is returned.

The only target capability is `reference.timer-snapshot.v1`, advertised only after
its implementation acceptance and while actually enabled. An incomplete server
omits it or declines `reference.v1`; this document is no availability claim.
An empty capability response is valid. No generic/scene/WS/provider capability is
inferred, and incompatible clients send no protected operation. Compatible clients
hide unavailable features. Negotiation is stateless and grants no lease, principal
or permission. Every GET independently binds the explicit path version and current
capability/authority; there is no negotiation token to bypass auth. A capability
disabled after negotiation prevents disclosure (503/unavailable after access).
HTTP006/API-004 retain full client combinations/matching web assets/upgrade UI.

## 3. Public versus protected access and committed observation

Public POST uses the same real external HTTPS/authority, lossless header and log
admission, but NEVER `prepare_account_transport(protected_access)` or a sign-in
guard. Credentials, if incidentally sent, are not adopted, resolved or echoed.
All fields/cookies are still completely syntax-scanned and bounded. No CSRF marker
or Origin is required for this nonsecret, stateless request; if Origin is present
it must occur once and exactly match the configured origin, otherwise
403/not_authorized. This path grants no CORS access. An incidental marker does not
change its purpose; it must be at most one OWS-trimmed `1` if present, otherwise
403/not_authorized. These are public-route rules, not a management-route fallback.

GET uses the unchanged transport import's `protected_access` purpose. Desktop
uses exactly one Authorization Bearer credential (64 lowercase hex); browser uses
exactly one unquoted `__Host-subathon_session` cookie and exactly one
`X-Subathon-CSRF: 1`. Browser GET may omit Origin, but a present Origin must occur
once and exactly match the configured origin. Desktop Bearer forbids Origin and
the browser marker. Mixed/duplicate credentials never fall back. The complete
imported Cookie syntax, unrelated-pair accounting, OWS, field multiplicity and
exact priority apply, not just this summary. The OpenAPI OR security alternatives
describe credential locations; they cannot express the mandatory browser marker,
mutual exclusion, token grammar or current authorization by themselves.

At execution on the authoritative Qt queue, compare the prepared ingress-policy
revision with the SAME current policy; a change is 503/unavailable and needs a new
actual client request. Resolve the original digest with actual current installation,
session/account epochs and trustworthy current clock. Timer read requires
`require_fresh=false`; an otherwise valid old-but-unexpired session may read.
This route never emits `X-Subathon-Reauthenticate`. Current granular ordinary
operation `timer.snapshot.read` binds the single actual installation timer in
resource slot `timer`, with no quantity slots; the trusted current permission
catalog/policy and ACCESS-002 evaluator must establish this exact definition and
binding. No owner/page/loopback/test-mode bypass or client-supplied actor/resource
exists. Missing/unknown authority denies. An earlier resolution or still-open page
grants no later disclosure; revocation ordered first denies the queued request.

Check current disclosure BEFORE protected state/readiness/existence lookup. Use
one coherent serialized committed observation and its current authorized config,
actual revision and original observed UTC annotation. No staged/private writer
state, request-time timestamp substitution, unsupported pause/Grace/deadline/debt/
recovery configuration or missing annotation becomes a normal snapshot. Readiness
must be established by the reference-path and owning recovery gates. Public HTTP
maps known unsupported/not_ready, absent/malformed committed facts, unknown policy,
read/producer inability and unknown commit admission state to 503/unavailable;
no private cause is disclosed. This is a read and has no operation ID, write,
application acknowledgement, NUM-04 durable hold, audit/job creation or retry award.
It neither advances countdown nor performs a timer transition merely to answer GET.
An independently authorized read after acknowledged commit may return that commit;
RP06/07 never permit publishing tentative work or manufacturing application success.

## 4. Finite ownership and first-failure order

These are fixed profile limits, not request-controlled options. Validate all
trusted sizes/representability before work; invalid trusted configuration is
503/unavailable. Do checked size/terminator/sum arithmetic before allocations and
growth; resource failure cannot accept a prefix or create partial disclosure.

| Owned boundary | Ceiling and counting rule |
| --- | --- |
| Listener request admission | 4096 octets for complete external HTTP/1.1 request (request line, every header/separator, transfer framing and body). For HTTP/2, at most 4096 request-specific encoded HEADERS/CONTINUATION+DATA payload octets and separately 4096 decoded semantic request octets; protocol/frame/HPACK connection storage has an additional listener-owned finite budget, never claimed covered here |
| Semantic request projection | `method_bytes + target_bytes + 12 + H + body_bytes <= 4096`, where `H=2+sum(name_bytes+value_bytes+4)` over EVERY decoded header occurrence; HTTP/2 pseudo-header/authority equivalents count once in this projection as request line/Host rather than vanishing. This separate bound applies even through a proxy |
| Imported transport limits | field_count=64; field_bytes=4096 for each name+value; header_bytes=4096; cookie_pairs=64; method<=16; configured_origin<=1024; policy_reference_bytes=4096; result_bytes=4096. All other supplier limits/scratch remain unchanged |
| JSON request scan | body<=4096; container depth<=4 (root container depth1); <=128 values (containers and member/array values, not member names); each raw quoted token<=1024 bytes including quotes/escapes; decoded string<=512 bytes, member name<=64; each array<=16; each object<=10 members |
| JSON scratch and decoded request | scan uses fixed depth4 frames/counters plus at most 32 identifier slices and 10 key slices per frame; no owning parse tree before admission. At most 4096 additional scratch bytes and 4096 decoded request string/key payload bytes, counted per occurrence; <=128 fixed value records. No generic unbounded DOM or second full request copy |
| Queued HTTP work | <=64 accepted reference requests; complete queued variable payload<=1048576 bytes and per-request<=16384 before enqueue. Count digest, policy/correlation/config/request occurrences under actual ownership; fixed records/queue nodes separately finite. Queue refusal is 503/unavailable; no unbounded pending fallback |
| Snapshot producer/result | Validate selected source lengths before copying; one result has <=16384 string/key payload bytes and <=128 fixed value records, depth<=4. Revision decoded<=4096/hex<=8192, numeric string<=20; one snapshot result, no full extra result copy |
| Response publication | complete success body<=65536 UTF-8 bytes, complete failure body<=1024; complete generated headers<=2048 bytes using H above; status-line/framing<=256 bytes. Success wire projection<=67840, failure<=3328 before growth, including optional correlation and fixed challenge/Allow/upgrade headers |

The success bound is deliberately above the largest allowed snapshot; it does not
enlarge any field. Producer/result projection occurs BEFORE hex/JSON expansion or
copy; stream to one bounded final buffer after projection. Release/move result
ownership instead of retaining duplicate full strings. Worst variable ownership
per active reference request at these application boundaries is **116992 bytes**:
4096 raw request +4096 scan scratch +4096 decoded input +4096 prepared policy result
+16384 selected snapshot source +16384 result +67840 publication.
Each simultaneous occurrence counts, including source/result and JSON escaping;
fixed C++/framework/native objects, imported session-source/result payloads, crypto
scratch, TLS/HPACK/proxy buffers and connection admission have their OWN reviewed
finite budgets. This is no global process-memory/CPU guarantee. Host integration
must establish those earlier real budgets before enabling the profile; unknown
producer/connection bounds return unavailable or close, never retroactively count
an already allocated request. No new reusable subsystem is implemented by this doc.

Stop at the first applicable phase; do not reorder to disclose protected state:

1. Actual listener framing/TLS/authority/lossless/log admission and trusted-limit
   validity. Unknown ingress =>503/unavailable or safe close. Wire/body overflow
   found before complete headers =>413/resource_limit if a safe response exists;
   complete header/count/field/projection overage =>431/resource_limit. If both are
   known at admission, header overage wins. Never read past a bound to seek an error.
2. Complete header/count/length preflight, then generic field names/values and
   complete Cookie syntax/pair counts, in original field/pair order. Imported
   transport syntax =>400/invalid_primitive; resource=>431/resource_limit. Preserve
   the import's rule that a malformed earlier Cookie pair is not skipped to find
   a later pair overage. No crypto/account/state lookup.
3. Route/method =>404/405; forbidden query/conditionals =>400; request representation
   =>415/406/400 as section1; correlation shape =>400 (null correlation).
   These public framing errors disclose no account/timer state.
4. POST only: pre-materialization JSON scan. Whole-body bound=>413/resource_limit;
   depth/value/token/collection/decoded/scratch projection=>413/resource_limit at
   first excess. At the same next input offset size admission precedes decoding.
   Invalid UTF-8/BOM/lone surrogate=>400/invalid_encoding; other JSON syntax,
   duplicate decoded object key, NaN/Infinity or second/trailing value=>400/invalid_json.
   Do not materialize a duplicate-key map. Then closed schema/primitive failure
   (including unique arrays, missing/unknown keys/range)=>400/invalid_primitive.
   POST public Origin/optional-marker checks follow, then version selection before
   capabilities (409/422); only then project a 200 negotiation response.
5. GET only: imported credential multiplicity/form, Origin/CSRF and token preparation
   in EXACT transport section3 order (steps4–6), then queue admission. Resource
   preflight/projection=>431/resource_limit; credential problems=>401/not_authorized
   with `WWW-Authenticate: Bearer realm="subathon"`; Origin/CSRF=>403/not_authorized;
   ingress/crypto/internal inability=>503/unavailable. Earlier HTTP phases are not
   permission to change this imported relative order.
6. GET execution: current ingress-policy revision, current session resolution, current
   permission, then enabled capability/readiness/committed observation. Missing/
   revoked/expired/invalid-input session=>401 with the SAME challenge. Valid session
   with denied or unknown permission=>403/not_authorized. Invalid stored facts,
   supplier resource/unavailable, unknown clock/policy/readiness, queue/read/result
   inability=>503/unavailable. No timer/existence lookup before disclosure permission.
   A session/source outage is not a fabricated 401. This nonsensitive read does not
   request freshness; the imported 403 reauthentication rule remains for other
   routes only. Actual coherent normal read completion and bounded output precede200.

The CommonFailure enum is imported whole, but `conflict`/`outcome_unknown` are not
new responses of these stateless/read-only routes. There is no successful partial
body; after publication begins, failure closes rather than appending another JSON
value. Never infer rollback or replay rights from disconnect/503. All errors use
the closed body and fixed headers above, no raw exception message or Retry-After.

## 5. Explicit inventory and independent literal oracles

Every positive data instance is an external `examples/*.json` file, bound exactly
once in the manifest to its actual component schema. There are no inline OAS
examples/defaults, hidden example documents or remote refs. The package contains
only `api.json`, `manifest.json` and those declared example files. All `$ref`s are
local component pointers, including unused definitions, so complete typed closure
must pass. The author evidence records the concrete inventory/count/checks; actual
schema-package validation, different review and APITEST-010 remain separate gates.
`timer-no-cap-zero` and the numeric/byte boundary snapshots are schema-only values:
in particular Active/Main0 may be unsupported by the ready reference producer.
Their schema validity does not assert that a real coherent ready source may emit
them. Current cap/state/configuration relationships remain source invariants; the
schema never supplies missing facts or proves readiness from field shape.

The JSON files are hand-selected oracle data, not production encoder output. They
cover RP01's exact projection, explicit no-cap/zero fields, signed MIN/MAX observed
UTC, signed64 maximum nonnegative fields, 4096 decoded revision bytes, 256-byte
correlation, identifier length64/array count16, unknown valid offers/capabilities,
empty capability response and each emitted failure code. A structurally valid
unknown offer is a positive SCHEMA example with a failing negotiation oracle.
Negative/raw/semantic cases below are deliberately NOT manifest examples: the
validator admits only instances that satisfy their actual schema, not expected
HTTP-failure flags. Later tests must independently exercise these literal cases.

| ID | Literal input/condition (otherwise admitted request) | Required result |
| --- | --- | --- |
| H01 / RP13 | offer `["future.v9","reference.v1"]`, required `[]`, actual timer capability enabled | 200, selected `reference.v1`, capabilities `["reference.timer-snapshot.v1"]` |
| H02 / RP13 | offer `["future.v9"]`, required `["scene.v1"]` | 409/incompatible_version, upgrade header, no auth/state lookup; version failure wins |
| H03 / RP13 | offer `["reference.v1"]`, required `["scene.v1"]` | 422/unsupported_capability, upgrade header; no scene/WS advertisement or route |
| H04 / RP13 | offer reference, required `[]`, timer unimplemented | Only200 with empty capabilities or409 if reference version disabled; never advertise pending timer support |
| H05 / RP13 | GET anonymous or actual revoked/expired session | 401/not_authorized, challenge, no state/existence disclosure |
| H06 / RP08/13 | valid session but read permission revoked while queued, or unknown permission | 403/not_authorized; no committed snapshot/private readiness lookup |
| H07 | valid read session older than300000ms but unexpired | May200 with current permission/readiness; no freshness challenge |
| H08 | browser cookie without marker; or Bearer with Origin | 403/not_authorized before crypto/session lookup |
| H09 | duplicate identical Authorization/account cookie; or mixed forms | 401/not_authorized before Origin/token/lookup; never select first/last |
| H10 | malformed Cookie pair before excess65th pair | 400/invalid_primitive; if65th well-formed pair is reached first,431/resource_limit |
| H11 | changed queued ingress policy revision; unavailable current source; crypto failure | 503/unavailable, no fabricated auth failure/snapshot |
| H12 / RP12 | unsupported state/config or missing revision/observation; capability disabled after negotiate | After current read authorization503/unavailable, no invented zeros/Active/current timestamp |
| H13 / RP04/06/07 | known rollback / commit unknown / acknowledged commit then work throws | GET may independently read prior acknowledged committed state / no tentative read while admission unknown / later coherent authorized committed state; never application success inferred from GET |
| H14 | body `{"supported_versions":["reference.v1"],"required_capabilities":[],"supported_versions":["future.v9"]}` | 400/invalid_json; same for escaped duplicate `"\u0073upported_versions"`; no map overwrite |
| H15 | POST body starts UTF-8 BOM; bytes C0 AF in string; string `"\ud800"` | 400/invalid_encoding, no replacement/normalization |
| H16 | POST truncated JSON, `NaN`, `Infinity`, two root values or trailing junk | 400/invalid_json; structural whitespace after one value is allowed |
| H17 | missing required_capabilities, extra member, null array, duplicate array identifiers, empty supported_versions or JSON-number identifier | 400/invalid_primitive; unknown valid identifiers alone are not malformed |
| H18 / RP14 | complete request4097 bytes; header projection4097; nesting5; values129; raw token1025; array17; object11 members | 413/resource_limit for wire/JSON/body;431 for header projection; never truncate. A bound crossed before later malformed input wins |
| H19 | unsupported method plus wrong media type; query plus malformed body; absent GET credentials plus malformed Accept | 405 /400 /406 respectively; no protected lookup |
| H20 | `observed_utc_ms` `"-9223372036854775808"` or `"9223372036854775807"`; main/grace/cap `"9223372036854775807"` | Exact schema endpoints; source must separately establish real coherent supported state |
| H21 | numeric strings `"-9223372036854775809"`, `"9223372036854775808"`, `"-0"`, `"01"`, `"1e3"`, `"1\n"` or integer JSON number | Schema rejects; invalid incoming primitive400, invalid selected stored/output fact503; no float/coercion/NUM-04 hold created by decoder |
| H22 | revision `"00"`; empty, odd `"0"`, uppercase `"AA"`,8194-character hex revision | First valid one-byte token; remaining reject. Revision8192 hex characters is valid; reference is opaque, not a parsed integer |
| H23 | required maximum_main_ms omitted versus null; debt_ms `"1"`; paused true; lifecycle grace | Omission and unsupported literals reject; null explicitly means no cap; never default missing fields |
| H24 | identifier64 bytes versus65; arrays16 versus17; escaped identifier whose decoded value equals an earlier item | First within schema/profile;65-byte identifier is primitive400 if its raw token fits,17th array resource413, escaped duplicate primitive400 |
| H25 | invalid/duplicate correlation alongside missing credentials | 400 with null correlation before authentication; valid512-char correlation echoed/admitted, malformed input never echoed |
| H26 | GET body/Content-Type; POST missing/wrong/duplicate Content-Type; content compression; Accept q/list | 400 /415 /415 /406; no alternate decoder/default body |

These examples do not demonstrate a running server, secure framework/proxy ingress,
browser/native behavior, actual current permission/SQLite reads or provider support.
No substantial new implementation prerequisite is introduced: existing ingress,
session/access/queue/readiness and runtime test-first tasks must establish the real
producers/bounds before enabling this profile. Full HTTP006/CONTRACT003, all seven
integrations/three FX providers and the complete release remain in scope.
