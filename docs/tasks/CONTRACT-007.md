# CONTRACT-007 — Persistent WebSocket synchronization contract

Parent contract gate: [CONTRACT-003](CONTRACT-003.md). Kind: contract.
Authority: specification v0.45 API-01/02/03, §§2/10/11.
Status/owner/dependencies: progress/state.json.

## Scope and acceptance

Define separately versioned hello/subscription/snapshot/update/resync/error schemas
using reviewed common primitives and HTTP resource/auth boundaries. Specify the
authoritative snapshot cut, ordered updates, gaps/reconnects, current authorization,
revocation/private projections, bounded slow subscribers, shutdown and both client/
server close behavior. Include independent multi-message traces, stale/gapped cursor
and reset examples, revocation without private leakage, pending update/close races,
and version/capability failures. Commands must use the authoritative Qt queue.

SPIKE-001 is a disposable one-command/server-close experiment, not evidence of
persistent synchronization or production reuse. Actual framework behavior remains
API-001/003 validation work. Contract schemas cannot certify that implementation,
message durability, privacy enforcement or transport performance. Preserve all such
test-first/live/platform gates. Split further before assignment if necessary.

## Ownership and handoff

Author owns assigned schemas/websocket/ files, contract/examples and evidence.
Root coordinates shared schemas/imports/index, state/HTML/build/CI/pins/maps. Read
root/schema AGENTS. Review actual artifacts before clients/generated code; record
real validator/consistency/CI outcomes and remaining framework tests. No connections,
live provider requests or production code is authorized by this brief. Follow
[ADR-0004](../decisions/0004-wire-contract-prerequisites.md) and [workflow](README.md).
