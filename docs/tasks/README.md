# Executable backlog beneath the release epics

The specification v0.45 remains authoritative. All seven integrations, all three
exchange providers and every agreed platform, renderer, account/security and operations
requirement remain in release scope. This decomposition defines planned work only;
it neither finalizes contracts nor claims a reviewed test or supported provider capability.

## Read and claim a child

1. Start with FIRST_AGENT_PROMPT.md and the existing initial core baseline sequence.
   Do not implement product behavior while its reviewed/red gate remains unfinished.
2. Choose a dependency-ready child from progress/state.json. Read its brief, the
   referenced specification decisions and scoped AGENTS.md. Create its task branch
   from current main and record scope, contract revision and shared-file ownership.
3. For a production slice, first complete its `*TEST-NNN` record (kind test_baseline):
   bounded contract/examples → test authoring → separate test review → intended red.
   The paired implementation record cannot start until that baseline is done. Both
   records link one brief so the oracle and implementation scope stay together.
   Minimal public interfaces, build harnesses, fakes and explicit unimplemented stubs
   are allowed for this stage; missing dependencies or compilation errors are not red.
4. Implement the paired bounded behavior, keep assertions intact, obtain separate
   code review and pass its relevant acceptance/regression checks. Keep unimplemented
   behavior visible in the full acceptance lane; green scaffold CI is never product proof.
5. Verify live providers/platforms only in their separate verification tasks. Default
   fixtures are synthetic/redacted, clocks controlled and adapters fake. Missing accounts,
   hardware or capability evidence stays blocked/not_run, never fabricated support.
6. Update evidence/acceptance map, progress JSON and generated HTML; open a focused PR
   following AGENT_WORKFLOW.md. Independent reviewers must be different agents/sessions/
   people from the author. Do not invent review identities to satisfy validation.

New decomposition records are planned, not authored/reviewed; consult state/evidence
for slices completed since the original decomposition. Their
specific acceptance/failure examples are in the linked brief. Contract ambiguities must
be resolved in that baseline before code, with explicit versioned examples and separate
review. If a slice still spans independent contracts or cannot fit a focused review,
split it further before claiming production work; preserve its original requirement scope.

## Dependency and completion model

The existing progress schema/tooling is unchanged. Epic `depends_on` lists aggregate
required child completion, including test, implementation and live/platform gates.
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
| [EPIC-002 — Contribution ledger, review, partial/full reversals and deduplication](EPIC-002.md) | 7 | 14 |
| [EPIC-003 — Rule builder, exclusions, generated events and simulator](EPIC-003.md) | 5 | 10 |
| [EPIC-004 — Timed modifiers, stacking and arrival-time context](EPIC-004.md) | 6 | 12 |
| [EPIC-005 — Sequential milestones and persisted action delivery](EPIC-005.md) | 8 | 16 |
| [EPIC-006 — Invites, permissions, local/Twitch identities and chat roles](EPIC-006.md) | 7 | 14 |
| [EPIC-007 — Session/credential encryption, keys and remembered login](EPIC-007.md) | 7 | 14 |
| [EPIC-008 — Qt/Drogon API server, durable intake and live snapshots](EPIC-008.md) | 9 | 18 |
| [EPIC-009 — Qt SQL migrations, backups, recovery and rollback](EPIC-009.md) | 17 | 32 |
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

Total: 188 work packages, 345 child records
(157 test baselines plus their paired
implementations and distinct feasibility/verification/documentation tasks).
The legacy cap-credit task IMPL-001 remains required under EPIC-001 in addition to these.
Task counts are not completion percentages.

See [decision ownership](DECISION_OWNERS.md) for navigation from every specification
decision to planned children and/or existing foundation tasks. That index is planning
coverage; tests/acceptance-map.json remains the test/evidence map and is not marked
verified by this decomposition.
