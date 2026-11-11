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

## Contracts and authority

The [specification](specification.md) defines the full release scope. Read the
selected task brief and its versioned [contracts](contracts) before implementation.
Contract examples, independently reviewed tests and intended red precede behavior.
A proposal or supplied fact never grants execution authority. Real current
account/session/permission/policy/clock producers and the authoritative application
service remain responsible for admission, enforcement, commit and publication.

Keep provider payloads, normalized events, receipts, contributions, captured plans,
actual effects, review holds and persisted delivery jobs distinct. Preserve immutable
arrival context, original/reversal provenance and exact request-versus-actual values.
Use exact checked numeric codecs; do not infer known values from unknown provider
fields. Current caps, debts, refunds and financial allocation follow owning contracts.
Reducers and projectors return proposals; callers own interval segmentation, pause
reasons, policy composition and actual state transitions.

A single controlled Qt SQL writer owns actual transactions and private connections
on its command/state thread. Repository adapters compose on caller-owned sessions;
intermediate returns and active-session reads remain provisional until real commit
and normal work completion. Revoked work capabilities cannot escape their session.
Unknown outcomes or post-commit exceptions cannot synthesize success. Opening a
writer does not authorize migrations: explicit reviewed catalog advancement owns
one whole missing prefix, preserving shipped migration bytes. Backup, update,
restore and recovery authority are separate operational requirements.

Resource bounds and ownership require source review as well as output assertions.
Preflight complete owned inputs and checked sizes before allocation; selected reads
must materialize only their contracted closure. Keep configuration identity retention
separate from semantic validation, and structural codecs separate from authority.

Two scene renderers share declarative data/behavior contracts, not implementation.
The complete editor is desktop-only. English/German translation uses one authoritative
catalog and separate native/browser outputs. Builds verify tracked generated text;
explicit update targets regenerate it, and source text is never a cleanable build
byproduct. Native consumers use declaration metadata and raw QTranslator lookup
before later formatting. Preferences, live switching and platform packaging remain
separate integration gates.

## Contract navigation

| Boundary | Starting contracts |
| --- | --- |
| Intake and atomic application | [Records/clock/writer](contracts/event-state-boundaries-v1.md), [application service](contracts/application-service-api-v1.md) |
| Timer and exact arithmetic | [Exact decimals](contracts/exact-decimal-v1.md), [lifecycle](contracts/timer-lifecycle-v1.md), [pause](contracts/pause-policy-v1.md), [editorial commands](contracts/timer-editorial-commands-api-v1.md) |
| Persistence and captures | [Writer](contracts/sqlite-writer-v1.md), [transactions](contracts/writer-transactions-v1.md), [migration catalog](contracts/migration-catalog-v1.md), [initial captures](contracts/sqlite-initial-captures-v1.md) |
| Access and sessions | [Permissions](contracts/effective-permissions-api-v1.md), [passwords](contracts/password-hashing-api-v1.md), [session facts](contracts/session-resolution-v1.md) |
| Localization and progression | [Translation build](contracts/translation-catalog-api-v1.md), [milestones](contracts/milestone-progression-api-v1.md) |

These are entry points, not an exhaustive contract index or implementation-status
ledger. Follow each brief's complete dependency/contract closure.

## Feasibility and release boundaries

Keep Drogon/Qt lifecycle, queue ownership and shutdown, Socket.IO compatibility,
independent Twitch authorization coverage, and Linux/Windows OBS/media behavior as
explicit integration/platform risks. Isolated spikes are permitted before production;
discard or separately review spike code before production reuse. Loopback transport,
fake-store services, real SQLite components and configured SDK discovery establish
different proof levels. None alone establishes a running host, TLS, live provider,
authoritative clock, delivery, recovery or supported-platform release.

Current outcomes and remaining gates belong in [progress](../progress/state.json)
and the [evidence summary](evidence/README.md). Detailed original component receipts
remain in the [published archive](https://github.com/Kernol12/subathon-bot-ai/tree/evidence-archive-2026-11/docs/evidence).
The [previous architecture ledger](https://github.com/Kernol12/subathon-bot-ai/blob/f2a134e57b9e25d4033b56720b72c08d0ef4df52/docs/ARCHITECTURE.md)
preserves its historical counts and direct receipt links. This document describes
boundaries and navigation; green task counts are not product verification.
