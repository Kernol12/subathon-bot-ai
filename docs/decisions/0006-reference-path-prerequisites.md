# ADR-0006 — Restore §17's early reference path

Historical decision text below retains its original tracker references.
See its [pre-cutover source](https://github.com/Kernol12/subathon-bot-ai/blob/b80e94f514caf33cad8abde8d00fa744b565768a/docs/decisions/0006-reference-path-prerequisites.md) and current [workflow](../AGENT_WORKFLOW.md).

Date: 2026-11-05. Status: proposed; actual approval is recorded in progress/state.json
and the independent review evidence. Task: [CONTRACTPLAN-003](../tasks/CONTRACTPLAN-003.md).
Authority: specification v0.45 §17 and §§2–4/6/10/18, API-01/02/03, NUM-04.

## Problem and decision

At main436ed9a, API-008 and APITEST-008 require API-003 → API-002 → CONTRACT-006
→ CONTRACT-008 → CONTRACT-010/012. This drags complete scene schemas, milestone
presentation and modifier timelines into the first synthetic contribution path.
Other broad dependencies require all connector lifecycle metadata, every ordered
rule action and periodic checkpoint/recovery. Those are release requirements but
are not prerequisites for evaluating one captured rule and reading a committed
snapshot. Simply dropping them would also drop important guarantees.

Use a bounded independently reviewed reference profile, retaining production
HTTP/thread/session/access guarantees. A local JSON function alone cannot finish
API-008. The new [profile](../contracts/reference-path-v1.md) supplies logical
invariants and hand-derived examples. Its concrete HTTP/schema and rule/adapter
contracts require separate completion before executable baselines. No scene types
or unfinished endpoints may appear in the reference package or advertised support.
CONTRACT-006 remains the COMPLETE HTTP package and CONTRACT-003 the complete
HTTP/WS/scene integration audit; neither becomes done from the reference package.

## Required replacements

Replace the same broad edges in BOTH APITEST-008 and API-008:

| Previous prerequisite | Reviewed replacement supplying the required guarantee |
| --- | --- |
| API-003 (complete persistent WS via API-002/HTTP006/scene008) | CONTRACT-013 → APITEST-010 → API-010: real bounded HTTP negotiation and authorized committed timer snapshot; retain API-001, SESSION-003, ACCESS-002, STORAGE-002 |
| API-005 (complete connector lifecycle) | CONTRACT-014 → APITEST-011 → API-011: synthetic normalized identity/evidence, receipt durability, contribution linkage and immutable arrival capture through existing ACCT/storage services; no provider capability inferred |
| RULES-003 (complete planner/delegated actions) | CONTRACT-014 → RULESTEST-006 → RULES-006: one immutable captured rule, exact time request and execution authority; retain ACCESS-003 and hence ACCESS-002 |
| STORAGE-003 (all checkpoints/restart/recovery) | API-011 with STORAGE-002 and ACCT-004: persist actual timer/effects/claim/result/audit atomically with every application; no restart/recovery capability |

Both retain ACCT-004 and CORE-002, and explicitly require CORE-005/STORAGE-002,
CONTRACT-013/014, API-010/011, RULES-006 and this reviewed plan. API-008 retains
APITEST-008. New baseline/implementation pairs retain their own independent
review/red gates. Each baseline depends on the real inputs listed in state;
no planned helper is treated as reviewed or implemented because it was added.

Add API-003, API-005, RULES-003 and STORAGE-003 explicitly to BOTH APITEST-009
and API-009. Their original edges remain, so advanced integration cannot silently
forget the requirements. EPIC-008 additionally requires CONTRACT-013/014 and both
API010/011 baseline/implementation pairs; EPIC-003 requires RULESTEST006/RULES006.
CONTRACT-006 and CONTRACT-003 add CONTRACT-013 as required reviewed import/integration.
No unrelated task's dependencies or existing baseline edges change.

The resulting early chain is:

1. CONTRACTPLAN-003 reviewed/integrated → CONTRACT-013 (bounded complete HTTP
   package) and CONTRACT-014 (one-rule/current-adapter contract), independently
   reviewed with concrete signatures/errors/resource bounds and examples.
2. API-001 bridge + SESSION-003 (including SESSION-001/002) + ACCESS-002 →
   APITEST-010 reviewed/red → API-010 accepted/reviewed. No anonymous shortcut.
3. ACCESS-003 → RULESTEST-006 reviewed/red → RULES-006 accepted/reviewed.
4. Existing durable intake/linkage/capture/storage + ACCT-004 + CORE-002/005 +
   RULES-006/ACCESS-003 → APITEST-011 reviewed/red → API-011 accepted/reviewed.
5. All above → APITEST-008 real HTTP/real SQLite end-to-end tests independently
   reviewed → observed intended red → API-008 implementation, code review and
   passing relevant acceptance/CI. A green component suite is not this baseline.

## Discovered blockers and sequencing

Main436ed9a has implemented ACCT-004 and SQLite storage, but its tests supply
synthetic access, policy, clock, action and reference ports. Actual producers are
absent. API-001, SESSION-001/002/003, ACCESS-002/003, rule evaluation and bounded
HTTP/schema also remain planned. These are concrete work prerequisites, not owner
permission questions and not a reason to invent an allow-all port or authorization
service. Existing ACCESS002 and SESSION001 reviewed red baselines may be used by
their implementation tasks; do not repeat them or claim those features pass.

CONTRACT-013/014 can be specified next without finishing scenes. APITEST-008
cannot yet be claimed under the repository's dependency-ready rule; implementation
cannot precede the actual replacement guarantees and its independently reviewed
red baseline. Record that explicitly instead of marking API-008 ready. If a
contract exposes a new unresolved behavior, record the blocker and split only its
necessary prerequisite. No unrelated features belong in this change.

## Preserved deferred release scope

| Deferred requirement | Required completion owner/gate retained |
| --- | --- |
| Complete OpenAPI endpoint families, matching web assets/desktop compatibility | CONTRACT-006/003, API-002/004 and consumer baselines; reference import cannot satisfy the complete package |
| Persistent WS, sequencing/resync/close/revocation/slow clients | CONTRACT-007, API-003 and API-009; SPIKE-001's Close finding stays open |
| Scene model/bindings/milestone presentation/modifier timelines/assets/schema | CONTRACT-008/009–012 and SCENE/RENDER epics; all previous edges retained |
| Full rule conditions/priority/exclusions/generated chains/action categories | RULES-001–005/003 and API-009; one-rule profile refuses other configuration |
| Paused review, modifiers, milestones, reversals and delivery jobs | Existing ACCT-005–007, MODIFIER/MILESTONE, API-009 and domain/release epics |
| Periodic checkpoints, restart/downtime/recovery/backup/upgrades | CORE-004/006/007, STORAGE-003–009, API-009 and operational/release epics |
| Seven integrations, three FX providers, clients/renderers, English/German/themes, platforms/OBS/packages/security/live evidence | All existing owner tasks and aggregate release gates, unchanged |

No specification requirement or observed behavior is changed. Reference simulations
use disposable state and synthetic adapters; never feed live accounting or infer
provider facts. Existing exact accounting/rounding, original context, cap-limited
actual effects, duplicate identity, permission and transaction boundaries remain.
All changes remain proposed until actual independent planning/profile review;
concrete contract review, test-first, separate code review, latest-head CI and PR
rules govern every later behavior.
