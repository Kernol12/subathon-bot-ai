# EPIC-022 — Viewer access, milestones, supporter privacy and statistics

Module: Web. Kind: epic. Specification: v0.45; §13.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `VIEWERTEST-001` → [VIEWER-001](EPIC-022/VIEWER-001.md) | Viewer access modes and separately scoped links |
| `VIEWERTEST-002` → [VIEWER-002](EPIC-022/VIEWER-002.md) | Source-respecting supporter privacy projections |
| `VIEWERTEST-003` → [VIEWER-003](EPIC-022/VIEWER-003.md) | Exact metrics and historical refund aggregation |
| `VIEWERTEST-004` → [VIEWER-004](EPIC-022/VIEWER-004.md) | Graph bucketing archive corrections and export query contract |
| `VIEWERTEST-005` → [VIEWER-005](EPIC-022/VIEWER-005.md) | React viewer page milestone sections and recent support |
| `VIEWERTEST-006` → [VIEWER-006](EPIC-022/VIEWER-006.md) | Permissioned CSV and JSON export without secrets |
| `VIEWERTEST-007` → [VIEWER-007](EPIC-022/VIEWER-007.md) | Retention deletion and archive access safeguards |

## Completion and integration

The epic depends on every listed baseline/implementation/verification child plus its
retained foundation prerequisites in progress/state.json. Marking it done requires all
of those dependencies done and a closure evidence note. Planned tasks are not authored
tests, reviewed contracts, implementations or verified capabilities.

Claim a dependency-ready child on its own branch, read its brief and scoped instructions,
and record shared-file ownership. Keep separate independent test/code review roles and
the existing [test strategy](../TEST_STRATEGY.md). No assertion weakening or simulated
provider/platform support is permitted. Missing live/platform evidence stays an explicit
gate; it does not remove a release requirement. Integrate focused PRs, update task evidence
and regenerate PROGRESS.html before handoff.
