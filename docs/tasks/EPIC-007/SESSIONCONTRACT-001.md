# SESSIONCONTRACT-001 — Session resolution and credential transport contract

Parent: [EPIC-007](../EPIC-007.md). Kind: DOCUMENT contract.
State/owner/dependencies: progress/state.json. Authority: specification v0.45
§§10/17/18, AUTH-05/AUTH-12, API-01/02/03 and existing approved
[wire primitives](../../contracts/wire-primitives-v1.md).

Finalize and independently review the missing concrete supplier document/API that
SESSION-002/003 and CONTRACT-013 must share. This is a contract task, not a session
implementation, actual C++ header, final HTTP package or authentication capability.
Import existing password/permission/wire contracts without treating their data or
pure calculations as current authenticated facts.

## Required bounded scope

- Owned current-session/account/installation resolution API, producer ownership,
  complete/missing/unavailable/revoked/expired facts, authoritative execution-time
  recheck, truthful failure/disclosure and fresh-auth semantics. Identify actual
  account/storage/clock producers and retain their later implementation gates.
- Exact token encoding/hash/rotation/issuance/renewal/revocation/import boundaries,
  without raw token storage/audit/URL/browser-local-storage leaks. Preserve random
  256-bit opacity, server-side hashes only, owner-configurable normal24h/remembered
  30day defaults and five-minute freshness; expiry does not terminate the subathon.
- Concrete browser Secure/HttpOnly/SameSite=Lax cookie parameters, issuance/deletion,
  same-origin/Origin/CSRF trust and binding rules, and desktop authorization-header
  syntax. Define duplicate/mixed credentials, malformed input, proxies/trusted
  origins, stale pages/logout, cross-origin reads/writes and secret-redacted failures.
  Keep viewer/overlay credentials separate; no anonymous/public-client shortcut.
- Finite borrowed transport/pre-scan/secret/result/scratch bounds and exact error
  priority with independently derived literal success/boundary/refusal examples.
  Make cookie/header parsing evidence requirements explicit rather than inventing
  a transport/framework capability that later adapters cannot demonstrate.
- Exact HTTP-authentication status/error import needed by CONTRACT-013. Shared
  negotiation cannot decide authentication. No pending endpoint/capability is
  advertised; the complete HTTP/WS/scene and runtime session gates remain intact.

Routine engineering choices must remain within the agreed specification and be
explicitly proposed/reviewed. Unresolved product or provider/framework questions
must be recorded, never converted into supported behavior. If producer and transport
contracts exceed a focused review, split the document task and preserve the
appropriate prerequisites before authoring or declaring either part done.

Root owns shared schema imports, migration allocation, build/CI/pins/state/HTML.
Assigned author owns only approved new contract document(s), examples and evidence;
separate reviewer examines the actual authored revision and imports/oracles.
No production/schema/test implementation or native red is authorized by this task.

## Completion and handoff

Record exact supplier/consumer revisions, literal oracle derivation, independent
review, remaining producer/runtime/platform gates and actual documentation checks.
Add executable document/schema validation to CI only when it exists and is relevant;
never claim validator/runtime success without running it. SESSIONTEST-002/003 still
need their own reviewed tests, minimal interfaces/stubs and genuine behavioral red
before SESSION-002/003 code. APITEST-010/API-010 retain actual SESSION-003 and all
other runtime prerequisites. No release requirement is removed by this document.
