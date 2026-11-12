# WORKFLOW-006 — Artifact-aware workflow and results-only records

Kind: implementation. Specification v0.45 §§17–18. Depends on WORKFLOW-005.
Contract: bounded-coordination-v2 §§1–2,4–5. Before tests, author/review the exact
record schema, completion/invalidation rules, compact projection and migration.

Coordinator owns schema, guidance/AGENTS/PR template, task/progress/publication and
shared integration. Test author owns focused additive workflow/schema cases; fresh
implementer owns tools/workflow.py and tools/progress.py integration. Existing
schema dependencies/pins and historical exemption/evidence records remain unchanged.

Add optional first-class required admission/results gates and artifact-manifest
currency, with one review-order validator and strict schema/semantic agreement.
Implement explicitly eligible ONE-independent-Claude results-only review, preserving
full substantive cycles and all old explicit task gates. Make declared pending
results block completion. Activate the approved forward policy in guidance only
with this reviewed implementation. No opaque parallel source of current gate truth.

Independent cases include stale operator digest despite unchanged source, preserved
rejected closing history, author self-review, pending results, results-only eligibility
refusal on changed code/recipe/oracle, unknown fields, and receipt-only metadata that
does not change packet identity. Keep historical supplemental evidence without
inventing migration approvals; define explicit compatibility in the reviewed contract.

Use fresh author/test/implementation sessions and full substantive cycles, intended
assertion red, real schema/tooling/scaffold checks and protected latest-head publication.
Document one record authority, honest proof limits and exact review transitions.
