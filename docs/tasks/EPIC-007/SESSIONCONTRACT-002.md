# SESSIONCONTRACT-002 — Browser and desktop credential transport contract

Parent: [EPIC-007](../EPIC-007.md). Kind: DOCUMENT contract.
State/owner/dependencies: progress/state.json. Authority: specification v0.45
§§10/17/18, AUTH-05/AUTH-12, API-01/02/03, approved wire primitives and the integrated
[current-session supplier](SESSIONCONTRACT-001.md). This is the transport portion
split from the original combined document before authoring; no scope was removed.

Finalize and independently review the exact credential transport contract consumed
by SESSION-003 and CONTRACT-013. This task supplies no HTTP/WS implementation,
authentication producer, final package/schema or supported framework capability.

## Required bounded scope

- Concrete browser Secure/HttpOnly/SameSite=Lax cookie parameters, issuance/deletion,
  same-origin/Origin/CSRF trust and binding rules, desktop authorization-header
  syntax, token lifecycle import and clock/freshness/error handling from the reviewed
  supplier. Preserve all agreed token secrecy, lifetime and expiry semantics.
- Exact duplicate/mixed credential, malformed input, proxy/trusted-origin, stale page/
  logout and cross-origin read/write behavior with secret-redacted failures. Viewer/
  overlay credentials remain separate; no anonymous/public-client shortcut.
- Finite borrowed transport pre-scan/secret/result/scratch bounds, complete error
  priority and independently derived literal success/boundary/refusal examples.
  Define cookie/header parsing evidence needed from adapters; unknown framework
  behavior is not a supported capability. Verify external claims using primary sources.
- Exact HTTP authentication status/error import for CONTRACT-013. Shared negotiation
  cannot decide authentication. Actual queue-time recheck and production producers
  remain required; no pending endpoint/capability is advertised.

Root owns shared imports/schema/build/CI/pins/state/HTML. Assigned author owns only
approved session-transport-v1.md, examples and author evidence. Separate reviewer
must examine actual authored revision and imports/oracles. No production code,
schema allocation, tests/stubs/native red or live credential operation is authorized.
Split further before authoring if focused review is not practical.

## Completion and handoff

Record exact integrated supplier/consumer revisions, literal oracle derivation,
independent review and actual documentation checks. Retain account/session/storage/
clock/transport/queue implementation gates and all platform/live/release obligations.
SESSIONTEST-002/003 and CONTRACT-013 require BOTH document tasks; no partial
resolution-only completion releases consumers. Executable document validation is
added to CI only if it exists and is relevant. Later tests retain their independent
review and intended behavior-red gates. No product requirement is removed.
