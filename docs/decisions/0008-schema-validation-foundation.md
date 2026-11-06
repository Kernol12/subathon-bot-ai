# ADR-0008 — Validate schema packages through an explicit foundation

Status: proposed DOCUMENT sequencing repair; different actual review required.
Authority: specification v0.45 §§17/18, API-01/02/03 and TEST-04.

## Evidence and problem

CONTRACT013 requires an actual OpenAPI3.1 package, complete references and independently
reviewed literal examples. With session supplier/transport documents accepted, its
remaining current producers belong to later runtime gates; they do not prevent
schema authoring. However schemas/ currently contains only README/instructions, no
common executable schemas or selected/pinned standards validator. check_project.py
checks repository links/progress/presets, not OpenAPI/JSONSchema or example instances.
Read-only preparation found no validator requirements/lock/harness; ambient PyYAML
is not selected validation tooling. No validator version, execution or success follows.

Claiming schema acceptance from JSON syntax/link checks would miss real structure,
reference and instance defects. Silently adding executable wrapper behavior to a
'document-only' acceptance would evade its independent test-first/CI gates.

## Decision and exact additive graph

Keep CONTRACT013 as the existing two-route schema/package task. Add separate
SCHEMATEST001 developer-tool baseline and SCHEMATOOL001 implementation, linked from
one bounded brief. Use reviewed existing standards libraries, not a new engine;
actual versions/pins/contracts/examples/tests/red belong to the baseline. The
wrapper must validate offline structure/dialect/local refs/instances with explicit
format policy, bounded input/path resolution and truthful diagnostics. Existing
runtime decoder/auth/proxy/WebSocket/framework gates remain untouched.

CONTRACTPLAN005 depends on integrated CONTRACT005. SCHEMATEST001 depends on that
plan and CONTRACT005; SCHEMATOOL001 depends on its reviewed/red baseline.
CONTRACT013 retains every prerequisite and additionally requires SCHEMATOOL001.
From actualmain4e85241's423IDs/1624edges, delta is three records/five edges,
result426/1629. No old ID/edge/status/review/assertion or requirement is removed.
New records are standalone foundations like existing contract prerequisites;
epic work-package counts are unchanged. Plan completion does not complete either
new foundation or CONTRACT013. This avoids tool→actual-package→tool cycles.

## Acceptance and consequences

Different actual plan review verifies graph/scope preservation. The future developer
baseline requires real selected-library green foundations and observed explicit
wrapper red after test review, then separate code review/passing checks/ordinary CI
for implementation. Libraries, licenses, dependency locks/hashes, CLI/diagnostics,
limits and owning-example formats remain unselected until actual baseline review.
No schema/interface/stub/test/code/dependency/CI/protection change is made here.

Actual schema validation is required before CONTRACT013 completion; raw HTTP duplicate
keys/credential transport/current authority still need their own implementation
baselines and cannot be certified by this tool. All seven integrations, threeFX,
renderers/locales/themes, platform/OBS/recovery/security/package/license/live/full
release remain required. Existing consistency/tooling CI covers this plan; new
executable CI is mandatory when the later stable wrapper exists. Actual protected-
check configuration must be assessed then, never inferred from this document.
