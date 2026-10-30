# EPIC-019 — Desktop-only scene editor, themes and media assets

Module: Presentation. Kind: epic. Specification: v0.45; §11.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `SCENETEST-001` → [SCENE-001](EPIC-019/SCENE-001.md) | Versioned scene document drafts and server asset references |
| `SCENETEST-002` → [SCENE-002](EPIC-019/SCENE-002.md) | Native canvas selection geometry grouping and layers |
| `SCENETEST-003` → [SCENE-003](EPIC-019/SCENE-003.md) | Binding visibility and lifecycle preview editors |
| `SCENETEST-004` → [SCENE-004](EPIC-019/SCENE-004.md) | Undo redo and scene draft edit transactions |
| `SCENETEST-005` → [SCENE-005](EPIC-019/SCENE-005.md) | Asset upload budgets restricted SVG and font validation |
| `SCENETEST-006` → [SCENE-006](EPIC-019/SCENE-006.md) | Portable themes and asset font import export |
| `SCENETEST-007` → [SCENE-007](EPIC-019/SCENE-007.md) | Permissioned live publishing restore and compatibility checks |

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
