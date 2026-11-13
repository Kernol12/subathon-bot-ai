# Executable backlog beneath the release epics

The specification v0.45 remains authoritative. All seven integrations, all three
exchange providers and every agreed platform, renderer, account/security and operations
requirement remain in release scope. This decomposition defines planned work only;
it neither finalizes contracts nor claims a reviewed test or supported provider capability.

## Read and claim a child

Read [the agent workflow](../AGENT_WORKFLOW.md), select a dependency-ready child from
`progress/state.json`, and follow its brief plus scoped `AGENTS.md`. The workflow covers
task branches, test-first gates, independent review, evidence, PRs and completion.
Follow the current priority in [AGENTS.md](../../AGENTS.md); the completed EPIC-028
cleanup preserves reviewed-baseline and full-release-scope rules.

New decomposition records are planned, not authored/reviewed; consult state/evidence
for slices completed since the original decomposition. Their
specific acceptance/failure examples are in the linked brief. Contract ambiguities must
be resolved in that baseline before code, with explicit versioned examples and separate
review. If a slice still spans independent contracts or cannot fit a focused review,
split it further before claiming production work; preserve its original requirement scope.

## Dependency and completion model

The progress schema/tooling validates task kinds, local or HTTPS evidence, and separate
approved reviews for completed cleanup tasks. Epic `depends_on` lists aggregate required
child completion, including test, implementation and live/platform gates.
A child never depends on its own epic. Each child's notes identify its parent for
searching in PROGRESS.html. All statuses/dependencies live in progress/state.json;
the tables here are navigation, not a second status database.

Existing foundation/contract/spike IDs and the six draft credit cases are preserved.
CONTRACT-002/003 remain foundation checkpoints; child baselines refine only their
bounded domain/API/scene portions before implementation. Epic-026/027 retain original
release prerequisites, but early evidence authoring, diagnostics and license work can
start via their own narrow dependencies rather than waiting for an entire release epic.
No existing verification result was upgraded by this planning change.

## Shared ownership boundaries

- CORE owns exact arithmetic/clock reducers; ACCT owns receipts, eligibility, plans,
  actual effects and reversals; API composes them through the authoritative Qt queue.
- STORAGE owns SQL/migrations/checkpoints/snapshots/encrypted backups/restore/upgrades.
  SESSION owns account sessions, credential encryption and credential-key rotation;
  STORAGE owns separate backup-recipient keys/rotation.
- MILESTONE owns progression plus generic durable jobs/action-group timeline. Provider
  adapters own supported outbound delivery; uncertain outcomes require review.
- API-005/006 own shared connector capabilities, setup, source selection and intake
  policies. Each of seven connectors owns its actual transport/evidence/correlation.
  RATES owns all three provider adapters and shared exact cache/schedule/override policy.
- SCENE owns the declarative document/assets/editor/publication; RENDER owns independent
  native/browser outputs, reference conformance and actual OBS checks. Browser management
  never gains a full scene editor. DESKTOP/WEBAPP own operations UIs; VIEWER owns read-only
  privacy/statistics/exports; LOCALE owns shared catalogs and individual appearance.
- DEPLOYMENT owns server image/proxy/Docker/Podman/native archive. PACKAGE owns desktop
  artifacts/build manifests. VALIDATE consolidates integration/platform/load evidence;
  RELEASE audits notices/corresponding source, diagnostics, docs and complete readiness.

## Epic index

| Epic | Work packages | Child records |
| --- | --- | --- |
| [EPIC-001 — Core test plan: exact arithmetic and full timer lifecycle](EPIC-001.md) | 8 | 16 |
| [EPIC-002 — Contribution ledger, review, partial/full reversals and deduplication](EPIC-002.md) | 8 | 15 |
| [EPIC-003 — Rule builder, exclusions, generated events and simulator](EPIC-003.md) | 5 | 10 |
| [EPIC-004 — Timed modifiers, stacking and arrival-time context](EPIC-004.md) | 6 | 12 |
| [EPIC-005 — Sequential milestones and persisted action delivery](EPIC-005.md) | 8 | 16 |
| [EPIC-006 — Invites, permissions, local/Twitch identities and chat roles](EPIC-006.md) | 7 | 14 |
| [EPIC-007 — Session/credential encryption, keys and remembered login](EPIC-007.md) | 9 | 16 |
| [EPIC-008 — Qt/Drogon API server, durable intake and live snapshots](EPIC-008.md) | 15 | 27 |
| [EPIC-009 — Qt SQL migrations, backups, recovery and rollback](EPIC-009.md) | 21 | 39 |
| [EPIC-010 — Twitch API, EventSub and both chat identities](EPIC-010.md) | 6 | 11 |
| [EPIC-011 — betterplace public integration and aggregate visibility](EPIC-011.md) | 4 | 7 |
| [EPIC-012 — Patreon v2 payment evidence, webhooks and reconciliation](EPIC-012.md) | 4 | 7 |
| [EPIC-013 — Streamlabs socket, verified payments and history](EPIC-013.md) | 4 | 7 |
| [EPIC-014 — StreamElements Astro tips and recovery limits](EPIC-014.md) | 5 | 8 |
| [EPIC-015 — Ko-fi payment webhooks and deduplication](EPIC-015.md) | 4 | 6 |
| [EPIC-016 — TipeeeStream socket/history and evidence](EPIC-016.md) | 5 | 8 |
| [EPIC-017 — Frankfurter, ExchangeRate-API and Open Exchange Rates](EPIC-017.md) | 9 | 17 |
| [EPIC-018 — Qt Quick desktop management and permissions](EPIC-018.md) | 10 | 20 |
| [EPIC-019 — Desktop-only scene editor, themes and media assets](EPIC-019.md) | 7 | 14 |
| [EPIC-020 — Independent native/browser renderers and OBS outputs](EPIC-020.md) | 8 | 12 |
| [EPIC-021 — React/TypeScript/Vite management foundation and workflows](EPIC-021.md) | 11 | 22 |
| [EPIC-022 — Viewer access, milestones, supporter privacy and statistics](EPIC-022.md) | 7 | 14 |
| [EPIC-023 — English/German catalogs and Light/Dark/System](EPIC-023.md) | 5 | 9 |
| [EPIC-024 — Docker Compose, Podman Quadlet, native Linux and Caddy](EPIC-024.md) | 9 | 15 |
| [EPIC-025 — Windows portable ZIP, Linux archives and Arch package](EPIC-025.md) | 6 | 10 |
| [EPIC-026 — Target platform/browser/OBS matrix and performance evidence](EPIC-026.md) | 6 | 6 |
| [EPIC-027 — Release license/source manifests, diagnostics and documentation](EPIC-027.md) | 4 | 6 |
| [EPIC-028 — Simplify agent workflow and focus test coverage](EPIC-028.md) | 13 | 13 |

Total: 208 work packages, 368 child records
(test baselines, their paired implementations, and distinct feasibility,
verification and documentation tasks).
The legacy cap-credit task IMPL-001 remains required under EPIC-001 in addition to these.
Task counts are not completion percentages.

See [decision ownership](DECISION_OWNERS.md) for navigation from every specification
decision to planned children and/or existing foundation tasks. That index is planning
coverage; tests/acceptance-map.json remains the test/evidence map and is not marked
verified by this decomposition.

The [application-service prerequisite split](../decisions/0003-application-service-prerequisites.md)
adds a documentation-only logical contract plus separate durable-hold and early-
claim baseline/store prerequisites
before the original service baseline; no original release requirement is removed.


The [reviewed wire-contract prerequisite split](../decisions/0004-wire-contract-prerequisites.md)
separates common representations, HTTP, persistent WebSocket and scene contracts.
CONTRACT-003 retains their complete integration gate; helper sequencing changes do
not finalize a schema or certify any client, provider or renderer.

The [scene-contract prerequisite proposal](../decisions/0005-scene-contract-prerequisites.md)
retains CONTRACT-008 as the full integration gate while separating the scene model,
reviewed owning-domain bindings/timelines, assets/packages and complete schemas.
No schema/renderer/provider/platform capability follows from these planning records.


[CONTRACTPLAN-003](CONTRACTPLAN-003.md) / [ADR-0006](../decisions/0006-reference-path-prerequisites.md)
realigns §17's early reference path with bounded HTTP013, rule/adapter014 and
separately baseline-gated API010/011/RULES006. Full scene, WS, connector lifecycle,
planner, modifier/milestone and checkpoint/recovery/release gates remain required.

[CONTRACTPLAN-004](CONTRACTPLAN-004.md) and
[ADR-0007](../decisions/0007-session-contract-prerequisite.md) expose the missing
[SESSIONCONTRACT-001](EPIC-007/SESSIONCONTRACT-001.md) supplier and
[SESSIONCONTRACT-002](EPIC-007/SESSIONCONTRACT-002.md) transport documents before session
baselines and the reference HTTP schema; runtime authentication gates remain.

[CONTRACTPLAN-005](CONTRACTPLAN-005.md) /
[ADR-0008](../decisions/0008-schema-validation-foundation.md) add standalone
SCHEMATEST-001/SCHEMATOOL-001 foundations before actual CONTRACT013 validation.
They are not extra epic work packages or evidence of a selected/working validator.

## Coordination tooling

- [WORKFLOW-001 — T3 coordination and derived task context](WORKFLOW-001.md).
- [WORKFLOW-002 — Coordinator-selected reviews with Codex correction cycles](WORKFLOW-002.md).

[Exact retained valuation prerequisite](EPIC-017/RATECONTRACT-001.md) separates
the logical representation/provenance/compatibility contract before RATESTEST-009.
[RATECONTRACT-002](EPIC-017/RATECONTRACT-002.md) separately gates the concrete
API/codec/additive-storage contract. These are additional foundation records,
not extra epic work packages. No existing
release requirement, baseline assertion or shipped migration is removed.

The [controlled parent rebuild prerequisite](EPIC-009/STORAGECONTRACT-003.md)
separates migration mechanism contract, [real-SQL baseline](EPIC-009/STORAGETEST-021.md)
and [implementation](EPIC-009/STORAGE-021.md) before exact valuation storage tests.
Existing writer PRAGMA admission and plan CHECK/FK constraints require explicit
reviewed migration support; the planning split supplies no runtime capability.

[CI-001 — Reliable Ubuntu package acquisition](CI-001.md) is an owner-authorized
CI-only repair; publication receipts are recorded in state and PR119. Product work
remains bounded by the current owner instruction.

[WORKFLOW-003 — Structured coordination and publication](WORKFLOW-003.md) adds
forward review records, derived stage handoffs/metrics and saved-PR reconciliation
while preserving historical evidence and the existing review cycle.

The owner-authorized bounded coordination program starts with
[WORKFLOW-004 — Forward policy and task split](WORKFLOW-004.md), followed by
[WORKFLOW-005 — Compact context/publication checks](WORKFLOW-005.md),
[WORKFLOW-006 — Artifact-aware records](WORKFLOW-006.md),
[WORKFLOW-007 — Reusable execution runner](WORKFLOW-007.md), and
[CI-002 — Setup diagnostics/merge enforcement](CI-002.md).
The forward record/results policy activates with reviewed WORKFLOW-006 tooling;
non-record practices/private-evidence guidance applies at WORKFLOW-004 merge.
Existing product work stays stopped. Consult state and the latest checkpoint for
actual gates and authorization.

## GitHub task migration

[EPIC-029](EPIC-029.md) preserves the complete migration scope. The explicitly
owner-authorized sequence is [MIGRATE-001](EPIC-029/MIGRATE-001.md) →
[MIGRATE-002](EPIC-029/MIGRATE-002.md) → [MIGRATE-003](EPIC-029/MIGRATE-003.md) →
[MIGRATE-004](EPIC-029/MIGRATE-004.md) → [MIGRATE-006](EPIC-029/MIGRATE-006.md) →
[MIGRATE-005](EPIC-029/MIGRATE-005.md). Read the
[migration contract](../contracts/github-task-migration-v1.md) and its
[consumer inventory](../contracts/github-task-migration-inventory-v1.md).
Only 001 is claimed here; independent contract review is pending. Until 004's
reviewed cutover merge, state remains authoritative. WORKFLOW-006/007 and product
work remain stopped; migration permits only WF006 storage/dependency rescope.
