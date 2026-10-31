# Architecture and dependency boundaries

The specification is authoritative. This map describes intended modules, not built features.

| Path | Owns | May depend on / may not do |
| --- | --- | --- |
| core | Exact domain values, calculations, event/effect contracts, clock abstractions | C++ standard library and selected exact arithmetic; no UI, network or provider SDK |
| server | Qt application services, authoritative queue, persistence, permissions, API bridge | Core and explicit adapters; all durable state follows one writer path |
| integrations | Provider-specific auth, transport, validation and capability descriptors | Intake contracts; cannot write timer/database state directly |
| desktop | Qt Quick/QML management/editor/native renderer | Versioned server API and shared schemas; no service credentials/authoritative state |
| web | React management/viewer and independent TypeScript overlay | Versioned server API/schemas; no direct DB access or full scene editor |
| schemas | OpenAPI 3.1, WS/scene/normalized-event contracts | Shared ownership; exact numeric JSON strings and compatibility policy |
| tests | Acceptance oracles, fakes, fixtures, reference scenes, platform/live evidence | Public interfaces; explicit suite ownership |
| deploy | Docker Compose, Podman Quadlet, native/systemd/Caddy and packaging | Agreed x86-64 support; no project-operated authorization service |

## Thread and transaction boundary

Drogon handles public transport and bridges commands to the authoritative Qt queue.
The application validates permissions/revisions and commits receipts, effects/state
and delivery jobs through the controlled Qt SQL/SQLite writer. External delivery happens
through persisted jobs; uncertain outcomes are not blindly retried. Clients render
server state and never own the authoritative timer or accounting.

## Contracts before dependent implementation

The first small C++ time-credit boundary is revision 1 in docs/contracts/time-credit-v0.md;
its reviewed original red baseline is recorded in
[evidence](evidence/time-credit-baseline-handoff.md). The bounded numeric helper is
implemented; [IMPL-001 evidence](evidence/impl-001-handoff.md) records passing
acceptance/regression checks and separate code review. It does not apply contributions
or commit effects.
CORE-001 also implements [exact decimal/final conversion revision1](contracts/exact-decimal-v1.md)
as an explicit core target;32 reviewed cases plus one regression pass with separate
code review and observed numeric CI. It supplies no application transaction,
aggregate policy or provider interpretation; see [handoff](evidence/core-001-handoff.md).
The internal [record, clock and writer boundary contract](contracts/event-state-boundaries-v1.md)
(revision 1, CONTRACT-002) separates durable intake from atomic application and
post-commit delivery. It defines shared invariants and specification-derived examples;
concrete C++/SQL layouts, bounded domain baselines and behavior remain downstream work.
No OpenAPI, normalized-event wire schema, effect ledger schema or complete timer
schema is provided as final. CONTRACT-003 defines transport contracts before clients;
implementation still requires each slice's reviewed tests and observed intended red.
Keep UI, provider payloads, normalized events, contributions, plans, effects and delivery
jobs distinct. Request/actual credit, arrival context, originals and reversals must retain
provenance. Two scene renderers share data/behavior contracts, not a renderer implementation.

The [identified receipt service](contracts/receipt-intake-v1.md) implements one
repository-port transaction with immutable originals and traceable retry/conflict
attempts. Fifteen reviewed cases pass in a primitive-store simulation with separate
code review/automatic CI; [handoff](evidence/acct-001-handoff.md). This is no SQL/Qt
writer, actual-provider or contribution-application verification.

## Early feasibility work

Check Drogon/Qt lifecycle/queue/shutdown, Socket.IO protocol compatibility, Twitch
independent authorization coverage and Linux/Windows OBS/media behavior early. Isolated
spikes are allowed before production implementation; document results and unresolved
capabilities, and discard or explicitly review spike code before using it as production.


SPIKE-001 now has [bounded Linux loopback evidence](evidence/spike-001-bridge-validation.md)
for pinned Drogon/Trantor, real HTTP/WebSocket queue/writer operations and cooperative
shutdown. This approves no production reuse. The observed WebSocket close behavior
and remaining lifecycle cases are carried into APITEST-001/API-001 and
APITEST-003/API-003; persistent connections and platform/release support remain open.

The [Main/Grace reducer](contracts/timer-lifecycle-v1.md) implements pure countdown
proposals and ordered transitions with39 reviewed passing cases, separate code
review and automatic CI; [handoff](evidence/core-002-handoff.md). UTC and elapsed
inputs remain separate, and accepted actual credit can resume confirmation wait.
The caller still owns segmentation, pause reasons, permissions, actual-credit
calculation and atomic commit; no Qt timer/queue, durable state or recovery is supplied.
