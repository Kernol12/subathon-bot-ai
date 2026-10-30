# EPIC-027 — Release license/source manifests, diagnostics and documentation

Module: Release. Kind: epic. Specification: v0.45; §1, §16, §19.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `RELEASETEST-001` → [RELEASE-001](EPIC-027/RELEASE-001.md) | Dependency font license and corresponding-source manifests |
| `RELEASETEST-002` → [RELEASE-002](EPIC-027/RELEASE-002.md) | Operational log retention and redacted diagnostic export |
| [RELEASE-003](EPIC-027/RELEASE-003.md) | Write operator setup recovery and client workflow guides |
| [RELEASE-004](EPIC-027/RELEASE-004.md) | Review final artifacts notices diagnostics and release checklist |

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
