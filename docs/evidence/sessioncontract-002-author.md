# SESSIONCONTRACT-002 credential transport author handoff

Author `session_supplier_review`, 6 November2026. This is a DIFFERENT task/role
from my prior supplier001 review; I cannot approve this authored transport.
Worktree `/tmp/subathon-sessioncontract002-author`, branch
`task/SESSIONCONTRACT-002-document-author`, from actual claim
`df1f1d13d917b77494607746893053b813a352cc` after normal supplierPR72 integration
main0605948. Only [transport document](../contracts/session-transport-v1.md) and
this author note are owned/changed. All old review/worktrees remain preserved.

Authority: specification v0.45 §§10/16/17/18, AUTH05/12, API01/02/03,
DEP05/SERVER01. Accepted supplier blob
`c5f5a6e71664042e18475068d3caf577b0c8d009` remains unchanged. No header/schema,
production code, parser patch, dependency, executable test/stub/native red or live
credential action was added. Actual code/framework support is not claimed.

The document proposes one synchronous bounded raw ingress source and two explicit
route-purpose results: prepared digest for later actual current resolution, or a
browser pre-sign-in guard that grants no identity. It defines exact cookie/Bearer,
duplicate/mixed forms, custom-header/Origin/closed-CORS CSRF, proxy/TLS/current-policy
ownership, priority and complete bounds, cookie publication/deletion and fixed
HTTP status/CommonFailure imports. Token/lifetime/hash/freshness/revocation remain
the accepted supplier's responsibilities. HTTP013 routes/schema and actual producer/
queue/permission/clock/current-session/persistence interfaces retain their gates.

Engineering choices for DIFFERENT actual review: host-prefix cookie with no Domain,
Path=/ and agreed Secure/HttpOnly/SameSiteLax; mandatory nonsecret custom header
on all browser API requests plus matching Origin for unsafe methods (safe reads
with absent Origin still require custom header/closed CORS); case-insensitive
Bearer scheme with canonical lowerhex token; no cookie/bearer fallback or selecting
one duplicate; bounded borrowed original-field input and policy-revision handoff;
non-enumerating401/challenge,403CSRF/freshness,503unavailable. Browser pre-sign-in
permits one canonical stale cookie without adopting its identity, avoiding an
expired-cookie dead end; actual accepted login remains independently necessary.

I read the linked primary sources independently through the web tool: RFC6265,
RFC6750§2.1, RFC9110 authentication/field limits, WHATWG Fetch and OWASP CSRF.
The HTTPWG6265bis document dated2November2026 is explicitly WORK IN PROGRESS,
not represented as an approved RFC or proof of supported browsers. Cookie syntax,
SameSite/__Host-, marker/preflight, auth-scheme case and mandatory401challenge
claims link to the supporting source beside the relevant document text. No OAuth
authorization-service flow or RFC6750 URL/body token option is imported.

Actual READ-ONLY framework-source finding: local selected Drogon source at primary
checkout `build/deps/drogon-1.9.14-source` reports exactHEAD
`3aae769cc405cf5abb9d477675c6430924f02c7c`, matching tools/setup_drogon.py. Its
working tree is clean. I read addHeader in HttpRequestImpl.cc: lowercased/trimmed
fields, cookies_ overwrites and headers_.emplace lose multiplicity; LOG_TRACE
contains whole Cookie value. Public HttpRequest header/cookie accessors expose maps.
Exact-commit primary URLs are in the proposal. This is SOURCE evidence only, no
runtime request/log exploit was run. The document refuses to treat those maps as
lossless evidence and requires a later reviewed/tested ingress solution and secret
log suppression before support. No parser/proxy change or workaround is approved.

Independent literal derivation actually run, without project parser/session code:

```python
z = '0' * 64
cookie = '__Host-subathon_session=' + z
normal = cookie + '; Path=/; Secure; HttpOnly; SameSite=Lax'
fields = [('Cookie', cookie), ('X-Subathon-CSRF', '1')]
size = lambda fs: 2 + sum(len(k) + len(v) + 4 for k, v in fs)
assert len('__Host-subathon_session') == 23
assert len(cookie) == 88
assert size(fields) == 120
assert size(fields + [('Origin', 'https://timer.example')]) == 151
assert size([('Authorization', 'Bearer ' + z)]) == 90
assert len(normal) == 128
assert len(normal + '; Max-Age=2') == 139
assert (1001 + 999) // 1000 == 2
```

All assertions PASS. An earlier uncommitted draft had incorrect hand-counts;
the independent audit corrected them before this candidate and no executable
baseline/test was changed. Expected tokenZ digest imports the independently
reviewed supplier literal, not a newly invoked implementation. Other literal
outcomes follow the document's independently authored first-failure priority;
they are not observed product parser/authentication results.

Actual document checks in this isolated tree:
`python3 tools/check_project.py` PASS423records/122decisions/local links/presets/
currentHTML; `python3 tools/progress.py render --check` PASS;
`git diff --check` PASS. No native build/test is applicable to adding this document.
Existing documentation/tooling CI is appropriate; root owns actual publication/
latest-head CI and shared task/HTML/acceptance evidence. A different reviewer must
examine the actual committed candidate and every literal/resource/security boundary
before document acceptance. Concrete ingress/raw multiplicity/trace suppression,
proxy/TLS/CORS/cookie/platform behavior and every later reviewed test/red/runtime
gate remain open. All seven integrations/three FX and full release scope remain.

Actual separate reviewer `session_plan_review` examined3f1699a and requested two
bounded precision corrections before verdict. The corrected document explicitly
rejects a changed queued ingress-policy revision as503/unavailable with a fresh
actual client request required; digest-only prepared state cannot reconstruct
erased Origin/marker/authority/multiplicity. It explicitly scopes the custom-header
profile to JSON HTTP/sign-in, retaining browser WebSocket handshake/message
authentication as unfinalized CONTRACT007/API003 work and preserving full release
scope. Browser WebSocket constructor constraints are linked to its primary standard.
No executable assertion/baseline or production behavior changed; actual corrected
revision still requires separate approval.
