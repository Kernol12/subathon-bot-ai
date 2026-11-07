# ADR-0010 — Separate executable session prerequisites from full integration

Status: accepted planning decomposition after independent reviewea27001e;
[review and publication gates](../evidence/contractplan-006-handoff.md).
Authority: specification v0.45 §§10/16/17, AUTH-01/05/11/12/15, DB-01,
SERVER-01, DEV-02, TEST-04/05. This is a planning decision, not a new API contract.

## Evidence and boundary

Accepted [supplier v1](../contracts/session-resolution-v1.md) §2 concretizes only
`prepare_account_session` and `resolve_current_account_session`. Its §4 preserves
issuance, replacement, reauthentication, inspection and revocation obligations,
but supplies no mutation/inspection API, physical account/credential/session/epoch
store, snapshot producer, accepted-login authority or trustworthy runtime clock.
The implemented password primitive neither accepts a login nor writes credentials.
The existing writer/application stores do not supply those missing producers.

Accepted [transport v1](../contracts/session-transport-v1.md) requires actual
lossless ingress and secret handling. Its pinned-source inspection identifies
duplicate header/cookie loss and a Cookie TRACE statement in selected Drogon;
ordinary decoded maps cannot establish the accepted transport contract. A fake
source, application-only log filter or declared TLS boolean establishes none of
the required runtime facts. A solution is not selected by this decision.

Assigning all of this to SESSION-002/003 now would hide several substantial
contract, SQL, runtime and verification efforts in one implementation task. Making
SESSION-002 depend on ACCESS-004 would instead cycle: ACCESS-004 already depends
on SESSION-002. These are prerequisite gaps, not grounds to reduce guarantees.

## Decision

Add the following bounded work. Detailed dependencies and exact additive edges are
in the [author packet](../evidence/contractplan-006-author.md); progress/state.json
remains authoritative after coordinator integration.

| New task(s) | Deliverable and boundary |
| --- | --- |
| SESSIONTEST-008 / SESSION-008 | Reviewed/red baseline then prepare/resolve component against unchanged supplier v1 §2/3. Fakes establish component behavior only. |
| SESSIONCONTRACT-003 | Concrete internal lifecycle mutation/inspection contract, importing supplier §4 and transport publication obligations unchanged. No implementation. |
| STORAGECONTRACT-002 | Concrete account/local-credential/session/epoch storage and current-snapshot contract on the existing writer, including exact schema/API proposal and bounds for later review. |
| STORAGETEST-020 / STORAGE-020 | Reviewed/red real-SQL baseline then that store and actual current-session/password-credential readers. No login decision or token publication. |
| SESSIONCONTRACT-004 | Concrete trustworthy session-clock producer contract, including discontinuity/recovery refusal and execution/publication sampling. |
| SESSIONTEST-009 / SESSION-009 | Reviewed/red baseline then actual clock producer under that contract. No timer countdown/downtime policy implementation. |
| ACCESSCONTRACT-001 | Concrete actual accepted-local-login authority, current credential/account/installation checks and conditional rehash boundary. |
| ACCESSTEST-008 / ACCESS-008 | Reviewed/red baseline then local-login acceptance and real password-source/accepted-attempt integration. No session issue service, Twitch linking or hosted broker. |
| SESSIONCONTRACT-005 | Reviewed concrete lossless/log-safe selected-framework ingress solution, with explicit upstream/dependency/proxy trust and deployment implications if needed. |
| SESSIONTEST-010 / SESSION-010 | Reviewed/red real-ingress baseline then selected solution and real ingress source; no complete transport route/egress or browser support claim. |

The first executable baseline is SESSIONTEST-008: its accepted contract and current
component prerequisites already exist once this plan is integrated. New contract
tasks may proceed separately. Each implementation waits for its own reviewed tests
and observed intended behavioral red. Document approval, a missing dependency,
compilation failure, a skipped case or another component's old red is insufficient.

SESSIONTEST-002/SESSION-002 retain their IDs, kinds, original edges and full
service scope: actual accepted-login issuance, random token generation, atomic
hash-only storage, absolute lifetimes, rotation/reauthentication, individual/self/
authorized-administrative inspection and revocation, epoch invalidation, truthful
commit/unknown/publication and queued-use ordering. Their newly required suppliers
do not finish this service. Real current authority and permission checks must be
bound at execution; raw references or the pure permission evaluator alone cannot
authorize a mutation or disclose a session list.

SESSIONTEST-003/SESSION-003 retain the full browser/native transport integration:
accepted credential syntax, origin/CSRF/closed CORS, current queue-time resolution,
cookie/native secret publication and deletion, response limits/errors, actual TLS/
proxy/log handling, and supported client behavior. SESSION-010 supplies only real
ingress evidence. API-001 still owns production bridge/startup/shutdown; API-002/
HTTP domain owners and API-010 still own their routes/schemas/current protected
execution. WebSocket API-003 retains its distinct handshake/message-time gates.

## Ownership and cycle avoidance

STORAGE-020 owns only the physical facts needed by these session/local-login
boundaries and their bounded current readers/conditional persistence primitives.
Existing account setup/invitation ACCESS-001 consumes this store for those shared
facts; add it as a prerequisite of both ACCESSTEST-001 and ACCESS-001. Setup codes,
invitation policy/roles, recovery offers and provider identities remain their existing
owners. Schema/migration order, shared build/CI/pins and progress are integrator-owned.
Do not create another writer or change an existing migration prefix silently.

ACCESS-008 supplies upstream *local* acceptance without requiring SESSION-002,
ACCESS-004, a provider callback or any preexisting account session. ACCESS-004 and
ACCESSTEST-004 retain all original local/Twitch login/linking and owner-fallback
requirements and now additionally consume ACCESS-008. Actual verified Twitch
authentication/reauthentication must reach the lifecycle authority boundary under
ACCESS-004's own reviewed baseline; no fake or claimed provider fact is support.
This allocation preserves the full local-plus-Twitch release requirement and
avoids a reverse dependency from SESSION-002 to ACCESS-004.

Clock refusal under unresolved recovery does not implement the restore/recovery
workflow. ACCESS-005 still owns account recovery and same-commit account-epoch
invalidation; STORAGE-007 still owns restored-installation epoch replacement before
access. Add explicit store/clock prerequisites to their existing baseline and
implementation records. Normal unchanged startup must preserve valid epochs.
These later workflows must test their actual fence transitions, not inherit a
general recovery claim from SESSION-009.

The physical/account authority and ingress solution contracts are intentionally
new document work. They must identify any requirement conflict or additional
substantial reusable subsystem before implementation, with a further focused
split if necessary. This plan does not approve a parser patch, proxy guarantee,
new crypto/protocol/library/pin, clock heuristic, credential proof format, SQL
schema, C++ header or test source.

## Preservation and acceptance

Only add nodes and edges; preserve every existing ID, dependency edge, task kind,
valid completed state, original reviewed assertion/receipt and release gate. No new
child depends on its epic or on downstream SESSION-002/003. Aggregate new children
under EPIC-006/007/009 as applicable. The coordinator must check the actual final
graph for absent IDs, lost edges, cycles, state validity, brief links and current
rendered progress, then obtain a different review of the written packet and graph.

New executable suites acquire reviewed finite harness/CI ownership when authored;
this document-only PR enables no runtime suite. Stable passing component suites
continue unchanged. Each new implementation needs separate code/configuration
review, relevant passing acceptance/regression and actual selected CI before normal
merge. Supported browser/DPAPI/QtKeychain/framework/OS/package evidence remains
explicitly separate from simulation or Linux component evidence. Full account/
Twitch, all seven integrations, three FX providers, both renderers, localization/
themes, backups/recovery and every other release requirement remain in scope.
