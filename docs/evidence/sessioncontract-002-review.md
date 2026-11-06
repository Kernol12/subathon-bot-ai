# Independent review — SESSIONCONTRACT-002 document

Reviewer `/root/session_plan_review`, different from author
`/root/session_supplier_review` and coordinator `/root`, 6 November2026.
**APPROVED for the corrected transport document and author evidence only.**
No header, schema, producer, endpoint, test baseline, runtime authentication,
framework capability or later CI/merge is approved by this verdict.

Read actual initial candidate `3f1699a2f0adfc1a577dd7088b1012d5b7c03139`, then
actual correction `6cd4a1565cc504e5e227deea3bc15335ac362977`, on NEW isolated branch
`task/SESSIONCONTRACT-002-document-review`, worktree
`/tmp/subathon-sessioncontract002-review`. Only this review note changes here.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| [Transport document](../contracts/session-transport-v1.md) | 2c33c2bbe9052455107c3056783d6bb605a5fef2 |
| [Author evidence](sessioncontract-002-author.md) | 33252c9f9c066f1ba5b1a74c86fe602b20abe95d |
| [Accepted supplier](../contracts/session-resolution-v1.md), unchanged | c5f5a6e71664042e18475068d3caf577b0c8d009 |

Authority: root instructions, task002 brief, specification v0.45 §§10/16/17/18,
AUTH-05/12, API-01/02/03, DEP-05/SERVER-01, accepted supplier and common wire
primitives. Read both complete authored files, supplier imports and current task
graph/briefs, full failure priority, bounds, literals and source evidence.

## Findings resolved on the actual correction

1. Initial wording allowed re-admission after an ingress-policy revision changed,
   while PreparedTransportSession retained only digest/mode/revision and all raw
   credential/header evidence was discarded. The corrected document now requires
   SAME revision at execution, otherwise503/unavailable and a fresh actual client
   request. It explicitly forbids reconstructing Origin/marker/authority/multiplicity
   from the prepared digest. Both interface description and execution/error table
   agree; no impossible replay or stale admission certificate remains.
2. Initial mandatory-custom-header wording could be imported to a browser WebSocket
   handshake, whose standard constructor exposes URL/protocols rather than arbitrary
   request headers. The corrected scope explicitly covers JSON HTTP/sign-in only,
   preserves full browser/native WebSocket scope, and leaves handshake/cookie/Origin
   and message-time authority as actual CONTRACT-007/API-003 gates. It authorizes
   neither browser WebSocket support nor a marker-free HTTP fallback.

I inspected the entire6cd4a15 diff, not a promised future correction. No remaining
blocking document/security/scope defect was found. These are document clarifications,
not changes to an executable test baseline, assertions or product implementation.

## Authentication, origin and lifecycle audit

Supplier token encoding/hash, clocks, expiry/freshness, epochs, current resolution,
rotation and commit uncertainty remain unchanged imports. Viewer/overlay/provider/
invitation/recovery/transfer credentials remain distinct. Actual route purpose is
trusted host binding, never a browser flag. The browser pre-sign-in result confers
no identity; a canonical stale cookie can coexist with independent new sign-in,
without refreshing/adopting its old principal or revoking unrelated sessions.

All Cookie fields are fully parsed before selection; duplicate account cookies,
repeated Authorization and mixed account-cookie/Bearer forms fail without fallback.
Cookie names and token bytes remain exact, including complete lengths. Unrelated
quoted cookie values and embedded equals follow the stated cookie grammar; account
token values remain unquoted64lowerhex. Header names and Bearer scheme are ASCII
case-insensitive, while its accepted whitespace/token grammar is explicit.

Every browser JSON API request needs the custom marker, including protected GET/
HEAD and sign-in/logout/rotation. Unsafe methods additionally require exact Origin;
any supplied Origin must match even on safe methods. Safe requests with absent
Origin still need the marker/closed CORS and cannot mutate state. Null/aliases/
different ports/same-site suffixes and untrusted forwarded metadata do not weaken
the binding. The document does not treat missing Origin, a spoofable User-Agent or
SameSite alone as authentication/CSRF proof. Desktop Bearer has its separate exact
form; browser Origin/marker cannot silently convert a request to that mode.

Proxy peer/TLS/authority/canonical-origin evidence comes from a real ingress owner,
not request-supplied trust booleans. No public authorization broker is introduced.
Digest-only queue admission grants no session or permission. SAME-policy admission,
supplier resolution, route freshness and current permission/revision/disclosure
checks precede protected execution within the authoritative serialized scope.
Acknowledged revocation/rotation invalidates the old digest; an open page, session
reference or deduplicated result cannot bypass current disclosure checks.

Issuance/deletion preserve Secure/HttpOnly/SameSite=Lax, host-only Path=/ and no
Domain. Normal browser retention and remembered Max-Age are separate from recorded
server expiry. Cookie deletion does not revoke a server row; logout must commit
authorized revocation first. Unknown commit/clock/publication never publishes a
guessed credential or claims rollback/success. Tokens stay out of browser JSON/
HTML/URLs/local storage and logs; separate native response/secure-persistence gates
remain. The prefix does not claim port isolation or verified browser behavior.

## Priority, bounds and public failures

All trusted limit/purpose checks precede the single actual ingress read. Complete
field counts/lengths/header-byte preflight precede character scans. Then field/
cookie syntax and first-excess-pair limits, duplicate/mixed/required-credential
shape, Origin/CSRF, and complete result projection precede token crypto. The stated
left-to-right cookie syntax versus later pair-overage priority is explicit rather
than falsely described as a whole-cookie counting pass before syntax. Unsupported
browser methods are unsafe here and still need their owning route refusal.

All policy references fit both their field limit and the complete one-string
result ceiling before ownership. Borrowed header/pair scans have fixed counters
and at most16 additional views; no owning header/cookie collections or queued
token views are allowed. Supplier scratch/native obligations stay separate. The
digest32, discriminant and object overhead are explicitly additional fixed storage;
result_bytes does not pretend to count them as string payload. Earlier producer
wire/decoded allocation and finite host queue admission are actual separate gates.
Complete auth emitter header/body/framing limits sum to3328 variable bytes before
growth, with separate budgets for native success/other producer-added data.

401 collapses missing/malformed/duplicate/mixed/expired/revoked credentials with a
fixed Bearer challenge; it does not expose account/session existence reasons.
Origin/CSRF and current permission denials use403; the freshness hint is emitted
only after actual valid current-session resolution. Trusted producer/crypto/stored
inability and changed policy remain honest503. Transport resource431 and syntax400
are separate from stored-fact inability. CommonFailure keeps its closed bounded
code/correlation shape, no-store and nonsecret content. Unknown writer outcome is
not transformed into auth failure, confirmed rollback or write success.

## Primary sources and actual framework evidence

Independently opened/read the supporting primary sources. These support the
protocol/security reasoning, not a claim our future adapters or platforms pass it:

- [RFC6265](https://www.rfc-editor.org/rfc/rfc6265.html): cookie grammar, replacement/
  deletion scope and duplicate-order concerns.
- [RFC9110](https://www.rfc-editor.org/rfc/rfc9110.html): case-insensitive field/
  authentication-scheme names and mandatory challenge on401.
- [RFC6750§2.1](https://www.rfc-editor.org/rfc/rfc6750.html#section-2.1): Bearer header
  syntax; the contract explicitly does not import OAuth issuance/query alternatives.
- [OWASP CSRF guidance](https://cheatsheetseries.owasp.org/cheatsheets/Cross-Site_Request_Forgery_Prevention_Cheat_Sheet.html):
  custom-header/CORS defense, safe-method constraints, origin/proxy checks and
  limitations of SameSite; this local profile is stricter in requiring its marker
  on all protected JSON requests.
- [WHATWG Fetch](https://fetch.spec.whatwg.org/): non-safelisted custom headers,
  Origin behavior, preflight/credentials and same-origin rules.
- [HTTPWG6265bis](https://httpwg.org/http-extensions/draft-ietf-httpbis-rfc6265bis.html):
  actual2November2026 working draft, explicitly labeled work in progress, host-prefix/
  SameSite semantics and lack of port isolation.
- [WHATWG WebSockets constructor](https://websockets.spec.whatwg.org/#dom-websocket-websocket):
  independently checked the interface restriction underlying the scope finding.

Independent local read-only source inspection confirms clean Drogon HEAD
`3aae769cc405cf5abb9d477675c6430924f02c7c`. HttpRequestParser.cc215 calls addHeader;
HttpRequestImpl.cc443–552 lowercases/trims,466 logs the complete Cookie value at
TRACE,486/508 overwrites same-name cookies and552 inserts ordinary fields into a
map without retaining repeated occurrences. The authored exact-commit source links
therefore support its stated limitation. Parsed maps cannot establish this
contract's multiplicity/lexical proof. A real pre-loss ingress solution and secret
log suppression before framework execution remain unimplemented gates, with no
pin change/parser patch/proxy support prospectively approved. No exploit or live
request was executed by this review.

## Observed checks and remaining gates

Independent standard-Python byte/hash arithmetic, without project code, asserted
cookie name23, pair88, browser header120, plusOrigin151, desktop90, normal cookie128,
remembered1001ms→2seconds/value139 and imported zero-token BLAKE2b-256 digest.
Full-U64 ceil division was checked without addition overflow, and2048+1024+256=3328.
All passed; these are independent literal derivations, not product behavior tests.

On actual corrected6cd4a15: `python3 tools/check_project.py` PASS423records/
122decisions/local links/presets/currentHTML; `python3 tools/progress.py render --check`
PASS; `git diff --check df1f1d1..6cd4a15` PASS. Git-object assertions confirm exact
accepted supplier blob and only the two authorized authored documents changed.
No build, C++ session/parser call, native crypto foundation, red replay, credential
issuance, network request to the application or live-provider action ran here.

Existing document/tooling CI is appropriate; no executable suite or protection/pin
change is introduced. Root still owns task completion metadata, actual latest-head
CI/rules/reviews/discussions and normal merge. Actual ingress/proxy/TLS/CORS/parser/
secret-lifetime/native scratch/clock/current-session/queue/permission producers,
reviewed tests/intended reds/implementation, full WebSockets, desktop persistence,
platform/recovery/live/release evidence remain open. All seven integrations, three
FX providers and full agreed product scope remain required.
