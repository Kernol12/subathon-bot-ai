# SESSIONCONTRACT-001 — Current-session supplier contract

Parent: [EPIC-007](../EPIC-007.md). Kind: DOCUMENT contract.
State/owner/dependencies: progress/state.json. Authority: specification v0.45
§§10/17/18, AUTH-05/AUTH-12, API-01/02/03 and existing approved
[wire primitives](../../contracts/wire-primitives-v1.md).

Finalize and independently review the missing current-session supplier document/API
shared by SESSION-002/003 and CONTRACT-013. This is a document, not an actual C++
header, implementation, HTTP package or authentication capability. Password and
permission components do not supply current authenticated facts.

The original combined assignment was split before content authoring because producer
and transport require separate focused review. This task retains current-session
resolution and credential lifecycle; [SESSIONCONTRACT-002](SESSIONCONTRACT-002.md)
retains every browser/desktop credential transport requirement. Existing consumers
require both documents. No release or runtime requirement is removed.

## Required bounded scope

- Owned current-session/account/installation resolution API, producer ownership,
  complete/missing/unavailable/revoked/expired facts, authoritative execution-time
  recheck, truthful failure/disclosure and fresh-auth semantics. Identify actual
  account/storage/clock producers and retain later implementation gates.
- Exact token encoding/hash/rotation/issuance/renewal/revocation/import boundaries,
  without raw token storage/audit/URL/browser-local-storage leaks. Preserve random
  256-bit opacity, server-side hashes only, owner-configurable normal24h/remembered
  30day defaults and five-minute freshness; expiry does not terminate the subathon.
- Finite borrowed secret/pre-scan/result/scratch bounds, exact error priority and
  independently derived literal success/boundary/refusal examples. Document the
  supplier facts needed by the separate transport contract without finalizing cookie,
  header, Origin, CSRF or HTTP status behavior in this task.
- Explicitly distinguish actual selected-library guarantees from proposed producer
  contracts and unknown/unimplemented framework behavior. Verify external technical
  and security claims with primary sources. No pending capability is advertised.

Routine engineering choices must remain within the agreed specification and receive
actual independent review. Record unresolved product/provider/framework questions;
do not convert them into supported behavior. If this bounded supplier contract still
exceeds focused review, split further before authoring.

Root owns shared schema imports, migration allocation, build/CI/pins/state/HTML.
Assigned author owns only session-resolution-v1.md, literal examples and author
evidence; separate reviewer examines the actual revision and imports/oracles.
No production/schema/test implementation or native red is authorized by this task.

## Completion and handoff

Record exact supplier/consumer revisions, literal oracle derivation, independent
review, remaining producer/runtime/platform gates and actual document checks.
Executable validation belongs in CI only when it exists and is relevant; no invented
validator/runtime proof. SESSIONCONTRACT-002 must subsequently finalize transport.
SESSIONTEST-002/003 still require both documents, their own reviewed tests/minimal
interfaces/stubs and actual behavior red before production code. CONTRACT-013 keeps
both supplier prerequisites; APITEST-010/API-010 retain actual SESSION-003 and all
other runtime gates. All agreed product and release requirements remain.
