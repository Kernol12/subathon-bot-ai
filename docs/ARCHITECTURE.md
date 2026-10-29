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

The first small C++ time-credit boundary is a DRAFT in docs/contracts/time-credit-v0.md.
No OpenAPI, normalized-event, effect ledger or complete timer schema is falsely provided
as final. CONTRACT-002/003 develop these with tests before dependent agents build them.
Keep UI, provider payloads, normalized events, contributions, plans, effects and delivery
jobs distinct. Request/actual credit, arrival context, originals and reversals must retain
provenance. Two scene renderers share data/behavior contracts, not a renderer implementation.

## Early feasibility work

Check Drogon/Qt lifecycle/queue/shutdown, Socket.IO protocol compatibility, Twitch
independent authorization coverage and Linux/Windows OBS/media behavior early. Isolated
spikes are allowed before production implementation; document results and unresolved
capabilities, and discard or explicitly review spike code before using it as production.
