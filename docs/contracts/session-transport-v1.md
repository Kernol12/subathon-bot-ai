# Account credential transport — document proposal v1

SESSIONCONTRACT-002; specification v0.45 §§10/16/17/18, AUTH-05/12,
API-01/02/03, DEP-05 and SERVER-01. Identity `subathon.account-session-transport.v1`.
Base: integrated supplier main0605948, task claimdf1f1d1. This is a proposed internal
transport contract, not a C++ header, HTTP schema, implemented endpoint or capability.
Different actual review precedes acceptance and later independently reviewed tests/red.

Import accepted [supplier v1](session-resolution-v1.md), exact blob
`c5f5a6e71664042e18475068d3caf577b0c8d009`, unchanged:32 random bytes,
64lowerhex representation, framed BLAKE2b lookup, server hashes only,24h/30day
configurable defaults, absolute expiry, five-minute freshness, current execution
resolution, epochs and truthful commit/cleanup uncertainty. Import
[common failures](wire-primitives-v1.md) unchanged. No provider OAuth, viewer/overlay,
setup/invitation/recovery/transfer token is accepted as an account credential.
CONTRACT-013 still owns actual routes/schema/negotiation/domain results; public
negotiation grants no account access. No login or other route is registered here.

This profile covers JSON HTTP request/response and the browser sign-in guard only.
The browser WebSocket constructor cannot supply this profile's arbitrary custom
header. Its handshake/cookie/Origin authentication and later message-time current
authority remain explicitly unfinalized CONTRACT-007/API-003 owning gates; neither
this profile nor HTTP013 establishes browser WebSocket support. No marker-free
HTTP fallback is permitted. Full browser/native WebSocket scope remains required.
[WebSocket constructor standard](https://websockets.spec.whatwg.org/#dom-websocket-websocket).

## 1. Actual ingress and bounded internal interface

An application-owned ingress adapter must expose the ORIGINAL decoded field
multiplicity and complete values before lossy cookie/header maps, trimming beyond
HTTP outer OWS, percent decoding or credential parsing. Field names compare ASCII
case-insensitively; cookie names/values remain exact. HTTP/2 split Cookie fields may
be visited as consecutive fields with an implicit semicolon separator; every pair
and its multiplicity remain visible. Repeated Authorization, Origin or CSRF fields
are never combined into a selected first/last value. A comma-combined Authorization
value is invalid, not a list of alternatives. Framing/HTTP smuggling rejection and
wire/decompression limits are producer obligations before this borrowed boundary.

Future document API, namespace `subathon::server` (declarations ONLY):

```cpp
enum class AccountTransportPurpose : std::uint8_t {
    protected_access, browser_sign_in_guard
};
enum class AccountTransportKind : std::uint8_t { browser_cookie, desktop_bearer };
enum class AccountTransportFailure : std::uint8_t {
    invalid_limits, resource_limit, invalid_syntax, authentication_required,
    origin_or_csrf_rejected, ingress_unavailable, crypto_unavailable
};
struct AccountHeaderView { std::string_view name, value; };
struct AccountIngressView {
    std::string_view method, configured_origin, policy_revision_ref;
    std::span<const AccountHeaderView> fields;
};
struct AccountIngressUnavailable {};
using AccountIngressRead = std::variant<AccountIngressView, AccountIngressUnavailable>;
struct AccountTransportLimits {
    std::uint64_t field_count = 64, field_bytes = 8192;
    std::uint64_t header_bytes = 32768, cookie_pairs = 64;
    std::uint64_t origin_bytes = 1024, policy_reference_bytes = 4096;
    std::uint64_t result_bytes = 4096;
};
class AccountIngressSource {
  public:
    virtual ~AccountIngressSource() = default;
    virtual AccountIngressRead read_current(const AccountTransportLimits &) = 0;
};
struct PreparedTransportSession {
    AccountSessionDigest digest;
    AccountTransportKind transport;
    std::string policy_revision_ref;
};
struct BrowserSignInGuard { std::string policy_revision_ref; };
using AccountTransportResult = std::variant<PreparedTransportSession,
    BrowserSignInGuard, AccountTransportFailure>;
AccountTransportResult prepare_account_transport(
    AccountIngressSource &, AccountTransportPurpose,
    const AccountTransportLimits & = {});
```

Enums are consecutive from zero in listed order. Purpose is selected by the trusted
actual route binding, never a request flag or client type/User-Agent. The source is
bound to this actual request/installation and keeps every view stable through the
synchronous call. It returns unavailable unless it establishes ALL of:

- Actual external HTTPS, either direct TLS or a request from an explicitly
  configured proxy peer whose original HTTPS/authority metadata is validated under
  that proxy configuration. Client-supplied Forwarded/X-Forwarded-* alone proves
  neither TLS nor authority. Untrusted peers cannot claim the proxy path.
- The external request authority matches the installation's configured canonical
  HTTPS origin. Compare the full scheme/host/effective port, not a suffix, substring,
  request Host chosen as trust policy, or all same-site subdomains. The trusted
  configuration producer supplies canonical ASCII serialized origin (no path,
  userinfo/query/fragment/trailing slash; DNS uses its ASCII form, default443 omitted).
  URL parsing/canonicalization, IPv6/nondefault-port and proxy forwarding need their
  own actual adapter review. Raw Origin acceptance below remains exact bytes.
- Lossless field/pair multiplicity and bounded wire/decoded ingress before framework
  ownership; no silent truncation/selected-map reconstruction. Secret-bearing
  headers and request URLs/bodies cannot enter framework/proxy/app logs or exports.

These are obligations on a real owned adapter, not booleans from an HTTP client.
An unimplemented/unknown producer returns unavailable; a test fake is explicitly
simulated raw input, not evidence of a supported framework. Policy revision is a
nonempty exact opaque reference. Queue execution must compare it with current
ingress policy. A changed revision MUST reject as503/unavailable and require a
fresh actual client request under current policy. This profile does not re-admit
using a prepared digest: it cannot reconstruct erased Origin, marker, authority or
raw field/pair multiplicity. The reference never grants account access.

Before any variable copy, validate positive limits and checked representability
of sizes/sums/terminators in size_t, uint64 and the destination max_size; otherwise
invalid_limits. The producer's earlier wire limits cannot be retroactively supplied
by this helper. header_bytes counts normalized decoded semantic storage:
2+sum(name.size+value.size+4), including every field occurrence. Each name+value
also fits field_bytes. Method length is at most16 ASCII bytes; configured_origin
fits origin_bytes and policy revision fits policy_reference_bytes. Cookie-pair count
includes unrelated pairs across ALL Cookie fields; no truncation at the target pair.
Request/body/JSON and compressed-wire limits remain owning listener/HTTP profiles.

The helper scans borrowed views with fixed counters and at most sixteen additional
string_views (selected token, Origin, marker, Authorization and current pair pieces),
not an owning header/cookie collection. It calls supplier prepare once only after
all transport checks; that call's124-byte application scratch and native scratch
obligation remain unchanged. It retains at most one32-byte digest and exactly one
owned policy-reference result; result_bytes bounds ONLY that string payload.
The32-byte digest, enum/discriminant and string object are additional fixed storage;
BrowserSignInGuard has the same one-string payload and no digest. Project the whole
variable result before its copy. No token or header view enters the Qt queue. No extra full owning output
copy is made during return. Fixed objects/native scratch and producer/framework
buffers remain separately bounded; these limits are not a global RAM/CPU guarantee.
Host queue count/bytes/admission are separately finite before accepting this result.

## 2. Accepted credential forms and browser request binding

Account cookie name is exactly `__Host-subathon_session`, value the supplier's64
unquoted lowerhex characters. Cookie names are case-sensitive. Parse every Cookie
field fully as semicolon-separated name=value pairs; surrounding SP/HTAB at pair
boundaries is allowed, empty pairs are invalid. Names use HTTP token characters;
values use RFC6265 cookie-octet, optionally surrounded by a single pair of quotes.
Quoted OTHER cookies are legal, but the account cookie must be unquoted. No percent
or JSON decoding, escaping, case-folding, suffix repair or hidden name whitespace.
Extra `=` inside an unrelated value is an ordinary cookie-octet. Duplicate account
cookie occurrences fail even if values agree. Duplicate unrelated names do not
select an account identity. Cookie attributes never arrive as proof of their prior
Secure/HttpOnly/SameSite settings; those belong to actual issuance/platform evidence.
[RFC6265 cookie syntax and duplicate-order warning](https://www.rfc-editor.org/rfc/rfc6265.txt).

Desktop form is one Authorization field whose outer SP/HTAB OWS is removed, then
case-insensitive ASCII scheme `Bearer`, one or more ASCII SP, and exactly64
lowerhex token characters with no trailing internal bytes. Emit canonical
`Bearer ` followed by the64 characters. HTAB between scheme/token, extra parameters,
commas, quoted token, alternate schemes and extra token whitespace are invalid.
HTTP field names and auth schemes are case-insensitive; token bytes are not.
[RFC9110 authentication](https://www.rfc-editor.org/rfc/rfc9110.txt),
[RFC6750§2.1 syntax](https://www.rfc-editor.org/rfc/rfc6750.txt).
Only that header syntax is reused: these application sessions are not OAuth access
tokens and no query/body bearer alternatives from RFC6750 are imported.

Presence of ANY Authorization field plus an account cookie fails authentication;
never fall back to the other form, even if one is valid or both tokens agree.
Repeated Authorization fails, including identical values; an empty field is present,
not absent. No credentials means authentication_required for protected_access.
Unrelated/viewer cookies alone grant no account access. A desktop bearer may coexist
with unrelated cookies, but desktop requests must carry neither Origin nor the
browser marker; native clients do not need browser CORS permission. This restriction
does not make client identity trustworthy or replace bearer secrecy.

Browser management API requests require EXACTLY one field
`X-Subathon-CSRF: 1`, including protected GET/HEAD and sign-in/logout/rotation.
Header-name comparison is case-insensitive; outer OWS is ignored and value is
exactly ASCII `1`. This marker is nonsecret, not a session token or separate stored
proof. The defense is the mandatory non-safelisted header plus a CLOSED cross-origin
policy, not its unpredictability. No credentialed browser API route grants CORS
access, reflects origins, enables arbitrary subdomains, or answers cross-origin
preflight with allow-origin/credentials/headers permission. Deny before protected
processing; do not rely solely on the browser hiding a response after an effect.
[OWASP custom-header API CSRF guidance](https://cheatsheetseries.owasp.org/cheatsheets/Cross-Site_Request_Forgery_Prevention_Cheat_Sheet.html#employing-custom-request-headers-for-ajaxapi),
[Fetch CORS and credentials](https://fetch.spec.whatwg.org/).

If Origin is present it must be exactly one value equal to configured_origin;
`null`, comma/list values, aliases, noncanonical spelling and different ports fail.
For browser unsafe methods (every method except GET/HEAD), Origin MUST be present
and match. Same-origin GET/HEAD may omit Origin, but still require the marker and
closed CORS; do not misclassify absent Origin as proof of same origin. No protected
GET/HEAD mutates state. No Referer/Fetch-Metadata/User-Agent fallback weakens these
rules; safe reads do not rely on Referer availability or a spoofable client label.
Fetch Metadata may add earlier rejection in a later owning adapter, never bypass
this contract. Same-site is not same-origin. SameSite=Lax alone is insufficient
CSRF protection, and XSS/client-side CSRF remain separate UI security gates.
[OWASP Origin/target checks](https://cheatsheetseries.owasp.org/cheatsheets/Cross-Site_Request_Forgery_Prevention_Cheat_Sheet.html#using-standard-headers-to-verify-origin),
[Fetch Origin rules](https://fetch.spec.whatwg.org/#origin-header).

browser_sign_in_guard is only for an explicitly bound pre-session browser sign-in
route and requires POST, the browser marker and matching Origin. Authorization
is forbidden in that guard request. Zero or one existing account cookie is allowed;
its canonical shape is checked, but no current identity is adopted from it. An
expired/revoked browser cookie must not prevent a new accepted sign-in. The actual
login service authenticates the new sign-in independently and keeps unrelated
sessions individually revocable; this guard never reauthenticates an old session.
Its BrowserSignInGuard result grants NO session, login, permission or issuance;
the actual login service must establish accepted current authentication. No
request can select this purpose to avoid protected_access. Public negotiation,
static assets, provider callbacks and viewer/overlay routes retain their own
separate profiles; they do not become unprotected management routes.

## 3. Full priority and current-session handoff

Stop at the FIRST applicable phase, without owning token/header input:

1. Validate trusted limits and purpose enum; invalid purpose is invalid_syntax.
   No source call for invalid limits/purpose. Call actual ingress source ONCE;
   unavailable/unknown native/source exception → ingress_unavailable. No invented
   secure/lossless facts or raw diagnostic escape.
2. Check ALL counts, method/origin/revision lengths and checked complete header
   bytes before character scans; any overage → resource_limit. Validate source
   configured origin/revision/method shape; malformed trusted facts →
   ingress_unavailable. Methods are case-sensitive ASCII HTTP tokens.
3. Validate complete field names and values. Reject empty/non-token names,
   CR/LF/NUL/DEL and control bytes except field-value HTAB OWS → invalid_syntax.
   Fully parse all Cookie pairs/counts. Bad cookie syntax → invalid_syntax;
   exceeding cookie_pairs → resource_limit at the first excess pair. An overage
   after an earlier malformed pair is not discovered by skipping that syntax error.
4. Reject repeated Authorization, repeated account cookie or mixed forms →
   authentication_required. For protected_access require one credential; for the
   explicit browser sign-in guard forbid Authorization and require method POST
   (one account cookie may be present, never adopted as authentication).
   Fail this shape as authentication_required. Presence of invalid alternate
   credentials never permits cookie fallback.
5. Check exact Origin and marker multiplicity/value under the selected browser/
   desktop form and sign-in purpose. Any failure → origin_or_csrf_rejected, before
   token crypto, account lookup or side effect. Unknown browser method is unsafe;
   owning HTTP routes separately refuse unsupported methods without executing them.
6. Project complete policy-reference result size before copying or crypto. Overage
   → resource_limit. For browser sign-in, check any account cookie is
   unquoted64lowerhex (otherwise authentication_required), then return only
   BrowserSignInGuard without crypto or current resolution. Otherwise
   validate Bearer structure/account cookie shape, then invoke supplier prepare
   on the selected64-byte token. Invalid shape/length/lexeme, including supplier
   resource_limit from token length, → authentication_required. Supplier runtime/
   crypto unavailable → crypto_unavailable. Success returns digest+mode+policy ref.
   Allocation/unclassified internal failure → ingress_unavailable, never success.

At authoritative execution, require the SAME current ingress-policy revision;
otherwise reject503/unavailable and retry only a fresh actual client request.
A prepared digest alone cannot revalidate discarded request evidence. Then call the accepted
supplier resolve with the installation selected by the actual host, original
digest, real current session source/clock and route's required freshness. Apply
current permissions/revisions/disclosure in that SAME serialized operation. An
earlier guard/prepared digest/resolved principal or still-open page grants no later
authority. Logout/revocation ordered before a queued operation denies it; no
identity refresh by session_ref can make the old digest current after rotation.
Stored/deduplicated outcomes still require current disclosure permission.

## 4. Cookie/secret publication and HTTP error import

Browser issuance uses ONE exact Set-Cookie field after acknowledged session commit:

```text
__Host-subathon_session=<64lowerhex>; Path=/; Secure; HttpOnly; SameSite=Lax
```

No Domain is present. Normal login emits no Max-Age/Expires (browser-session
retention with the independent recorded24h default maximum). Remembered login
adds `; Max-Age=<positive-seconds>` where seconds=ceil(remaining_ms/1000), calculated
without overflowing signed/full-U64 arithmetic from trusted publication-time clock
and recorded expiry; expiry must still be future. Browser retention/clamping/early
eviction is not server authority and cannot extend recorded expiry. Configured
long lifetimes do not promise browser persistence beyond browser limits. If the
clock/commit/publication outcome is unknown, publish no guessed credential. Token
rotation/import preserves the supplier's absolute lifetime/freshness rules.

Deletion emits the same name/host/path and protected attributes, an empty value,
`; Max-Age=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT`. Deleting a cookie alone does
not revoke server access; acknowledged logout first commits current authorized
revocation, then deletes it. Browser/cache/network failures cannot restore or prove
revocation. Do not clear/replace cookies on untrusted-origin failures or manufacture
a successful logout when persistence is unavailable/unknown. Normal logout and
session expiry leave the subathon running under its independent disconnect policy.

Secure/HttpOnly behavior and deletion scope follow
[RFC6265](https://www.rfc-editor.org/rfc/rfc6265.txt). SameSite and host-prefix
semantics are described by the current
[HTTPWG6265bis working draft](https://httpwg.org/http-extensions/draft-ietf-httpbis-rfc6265bis.html)
(work in progress,2 November2026), not misrepresented as guarantees of our clients.
The __Host- prefix does NOT bind a port; exact configured-origin/authority checks
remain necessary. Supported-browser/embedded-framework cookie evidence remains a
later platform gate. TLS/flags do not replace CSRF or server revocation.

Browser responses never place the account token in JSON/HTML/URLs/JS-readable
headers/storage. Native sign-in may publish its token only in the separately
reviewed secret response body after commit, with no browser CORS access; actual
desktop issue-route/body schema remains SESSION002/HTTP006 work. Desktop keeps it
in memory or agreed DPAPI/QtKeychain protected persistence, never a password or
insecure fallback. Neither native nor browser clients automatically forward account
credentials across redirects/origins; re-enter the configured origin explicitly.
Ingress/egress copies, framework buffers, crash dumps and logs need real lifetime/
redaction review. A decoded request map or HttpOnly flag is not proof of that work.

An auth response emitter must preproject its complete generated header block
against2048 bytes (every name/value/separator occurrence, including any token),
its failure body against1024UTF8 bytes, and status-line/framing against256 bytes
before output growth: at most3328 bytes for this auth failure envelope. Native
sign-in success bodies and application/proxy added headers require their owning
additional finite budgets; they cannot silently enlarge this emitter. At most ONE
64-character bearer secret occurs in a browser credential response, only in its
Set-Cookie value. These are variable payload limits, separate from fixed emitter
objects/framework buffers. Transport/producer failure that cannot safely form
this bounded envelope may close without a synthetic response.

All credential/auth/protected responses use `Cache-Control: no-store` and
`Content-Type: application/json` for failure bodies, with no credential-bearing
Location or access-control allow headers. CommonFailure remains the CLOSED
code/correlation_id object, <=1024UTF8 bytes; correlation is bounded/admitted or
null, never echoed from arbitrary malformed input. Exact auth import:

| Internal observation | HTTP status / common code / extra fixed headers |
| --- | --- |
| Resource limit in transport pre-scan/projection | 431 / resource_limit |
| Invalid request/header/cookie syntax | 400 / invalid_primitive |
| Missing, malformed, duplicate or mixed account credential; supplier missing/revoked/expired/invalid_input | 401 / not_authorized / `WWW-Authenticate: Bearer realm="subathon"` |
| Origin/CSRF rejection | 403 / not_authorized |
| Actual valid session, required freshness absent | 403 / not_authorized / `X-Subathon-Reauthenticate: 1` |
| Valid session/current permission denies | 403 / not_authorized |
| Invalid trusted limits, ingress/crypto unavailable, supplier unavailable/invalid_stored_data/resource_limit, changed ingress-policy revision | 503 / unavailable |

Supplier resource_limit refers here to current stored-fact/result inability; the
separate token-lexeme length mapping above is401. No detailed expired/revoked/account
existence/reason field or untrusted library text is exposed. The reauthenticate
header is emitted ONLY after actual current valid-session resolution requiring
freshness, never based on a caller principal. No automatic sign-in redirect or
Retry-After guessed from an unknown outage. Actual writer outcome_unknown stays
the owning operation's honest unknown response/disconnection, not an auth401 or
confirmed rollback. No status mapping in this document may synthesize write success.
HTTP401 requires a challenge and valid-but-insufficient credentials use403 under
[RFC9110§11/§15](https://www.rfc-editor.org/rfc/rfc9110.txt). Native/library details
are redacted. All auth schemas/status imports in CONTRACT013 must reuse this table;
domain request/content/compatibility priorities remain the owning HTTP contract.

## 5. Literal cases, verified framework limits and next gates

Use configured_origin=`https://timer.example`, policy revision=`p`, protected
GET unless varied, account tokenZ=64 ASCII `0`. Header lists below are independent
literal inputs; no production parser/resolver generated these expected outcomes.

| Raw variation (field names case-insensitive) | Exact result before supplier resolution |
| --- | --- |
| Cookie `__Host-subathon_session=Z`, marker `1`, no Origin | browser digestZ + policy `p`; safe missing-Origin permitted only under closed CORS |
| Same plus Origin `https://timer.example` | same browser result |
| Same plus Origin `https://timer.example.evil` or `null` or `https://timer.example:444` | 403/not_authorized; no crypto/source resolution |
| Same POST without Origin | 403/not_authorized |
| Same POST with exact Origin | browser result; later actual current resolution/permission still required |
| Cookie only, no marker | 403/not_authorized |
| Two identical account cookie pairs | 401/not_authorized; no first/last selection |
| Account cookie plus ANY Authorization | 401/not_authorized |
| One `authorization: bEaReR  Z` and no Cookie/Origin/marker | desktop digestZ; scheme case-insensitive and two SP accepted |
| Two identical Authorization fields or comma-combined values | 401/not_authorized |
| Bearer with HTAB separator or64 uppercase `F` token | 401/not_authorized |
| Desktop Bearer plus matching Origin | 403/not_authorized |
| No credentials, marker `1` | 401/not_authorized for protected_access |
| browser_sign_in_guard POST, marker `1`, exact Origin, no account credentials | BrowserSignInGuard only; no authentication/session grant |
| Any complete header bound exceeded plus otherwise malformed credential | 431/resource_limit first |
| Source cannot prove actual lossless ingress/TLS/log policy | 503/unavailable; no fabricated valid map |
| Previously prepared digestZ after acknowledged revocation | queue-time supplier revoked→401, no protected effect/disclosure |

Literal byte counts: cookie name23 bytes; value23+1+64=88. Cookie field name6,
browser marker field name15/value1: semantic header bytes
2+(6+88+4)+(15+1+4)=120. Origin header name6 and origin21 add31 →151.
One Authorization name13/value71 (`Bearer `+64) costs2+13+71+4=90.
The normal Set-Cookie VALUE is128 bytes (88+8+8+10+14); remembered remaining
1001ms adds `; Max-Age=2` (11bytes), yielding139. These boundaries and expected
digests import supplierZ literal `a226d544ff92f3247dc784e949c846999fde801e4581c6bb219fde070591d3ea`.
The one-byte policy result fits result_bytes1; result_bytes is a positive limit,
so0 is invalid_limits. A two-byte revision under result_bytes1 is431 before crypto.

Concrete limitation discovered by READ-ONLY source inspection: selected Drogon
1.9.14 commit3aae769cc405cf5abb9d477675c6430924f02c7c (tools/setup_drogon.py)
exposes maps through getHeader/headers/cookies. Its parser lowercases/trims fields,
overwrites duplicate cookie names in cookies_, and uses headers_.emplace for other
headers, losing multiplicity. It also has a LOG_TRACE statement containing the
whole Cookie value. Sources: exact
[HttpRequestImpl.cc](https://raw.githubusercontent.com/drogonframework/drogon/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/src/HttpRequestImpl.cc)
and [HttpRequest.h](https://raw.githubusercontent.com/drogonframework/drogon/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/HttpRequest.h).
This is source evidence, not a live exploit or a validated runtime adapter. These
maps cannot by themselves implement this contract. A later reviewed/tested ingress
solution must retain/reject multiplicity before loss and prevent secret logging
before the statement can execute; disabling only application-level logs is insufficient.
No dependency/pin/parser patch or proxy guarantee is approved here.

Actual request framing/HTTP2 combination, Caddy/alternative proxy trust/settings,
wire/decoded preallocation bounds, lossless parser hooks/correction, trace suppression,
TLS/origin canonicalization, browser CORS/cookies and native header/persistence need
their own literal tests, separate review, intended red and implementation evidence.
Keep SESSIONTEST002/003, API001/CONTRACT013/API010 runtime gates, supported platforms,
all seven integrations/three FX providers and full release scope. No source-owned
placeholder, public auth service, supported endpoint or unverified capability follows
from accepting this document.
