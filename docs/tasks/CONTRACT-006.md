# CONTRACT-006 — HTTP/OpenAPI 3.1 contract

Parent contract gate: [CONTRACT-003](CONTRACT-003.md). Kind: contract.
Authority: specification v0.45 API-01/02/03, §§2/10/11.
Status/owner/dependencies: progress/state.json.

## Scope and acceptance

Define versioned HTTP request/result/error packages in source-controlled OpenAPI3.1,
using reviewed CONTRACT-005 primitives and referenced CONTRACT-008 scene schemas.
Document operation/correlation IDs, retries, stale revisions and current execution-
time authorization; safe account/viewer credential transport and disclosure; resource/
asset references; capability handshake and matching server web assets. Independently
specify valid/invalid examples, compatibility errors and schema-reference resolution.
Map every advertised endpoint family to its owning task and required domain contract.

Do not invent unresolved command behavior or claim a schema grants authority,
durability, idempotency, provider support or client conformance. Split endpoint
families into focused contract tasks before assignment if their actual reviewed
scope requires it; preserve them under the parent completion gate. Domain behaviors
and protected operations still require their own reviewed tests/red/implementation.

## Ownership and handoff

Author owns assigned schemas/http/ files, HTTP contract and evidence; root coordinates
common imports/schema index, state/HTML/build/CI/pins/maps. Read root/schema AGENTS.
Review actual schemas and independent examples before generated clients/product use.
Record version pins, applicable schema validation commands and observed CI; adding
an executable validator needs focused reviewed checks and CI. No live requests or
source generation is authorized by this planning brief. See
[ADR-0004](../decisions/0004-wire-contract-prerequisites.md) and [workflow](README.md).
