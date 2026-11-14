# CONTRACT-005 — Common wire primitives and compatibility

Parent contract gate: [CONTRACT-003](CONTRACT-003.md). Kind: contract.
Authority: specification v0.45 API-01/02/03, NUM-04, §§2/10 and CONTRACT-002.


## Scope and acceptance

Start with a bounded versioned document and independently specified valid/invalid
examples. Define exact decimal/accounting, signed/unsigned64 and explicit rational
numerator/denominator string representations without float conversion. Distinguish
opaque-byte references from Unicode text, absence/unknown/null from known zero, and
validation from authority. Define common correlation/revision/version/capability
primitives, bounded decoding and compatibility errors. Explain compatible features,
unavailable capabilities and incompatible operations before performing them.

A capability does not grant permission or prove a provider's support. Keep secrets
and private supporter fields out of generic errors. Do not choose unresolved domain
policy, invent timestamps/provider metadata, finalize every endpoint or introduce a
project-hosted authorization service. Synthetic examples include numeric extremes,
malformed/noncanonical inputs, unknown fields/versions and private error handling.

One reviewed definition feeds later HTTP/WS/scene contracts. Root coordinates shared
schema location/imports before schemas or generated code exist. Contract review
precedes clients; downstream feature tests/red/code/review and platform/live gates
remain required. Split further if an actual claim exceeds focused review scope.

## Ownership and handoff

Claim a dedicated branch from current main, read root/schema AGENTS, and record the
actual contract revision/examples/author/reviewer. Initial author owns a new common
contract and evidence only. Root owns state/HTML, shared schemas/imports, build/CI,
pins and maps. No production or schema implementation is authorized by this brief.
Run project consistency/current HTML/whitespace checks and assess CI for any later
executable additions. Follow [ADR-0004](../decisions/0004-wire-contract-prerequisites.md)
and [the task workflow](README.md).
