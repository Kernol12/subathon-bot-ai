# EPIC-020 — Independent native/browser renderers and OBS outputs

Module: Presentation. Kind: epic. Specification: v0.45; §11.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| [RENDER-001](EPIC-020/RENDER-001.md) | Author shared scene reference fixtures and conformance oracles |
| `RENDERTEST-002` → [RENDER-002](EPIC-020/RENDER-002.md) | Native QML scene elements and synchronized bindings |
| `RENDERTEST-003` → [RENDER-003](EPIC-020/RENDER-003.md) | Independent browser scene renderer and state projection |
| `RENDERTEST-004` → [RENDER-004](EPIC-020/RENDER-004.md) | Elapsed-time animations media and font profiles |
| `RENDERTEST-005` → [RENDER-005](EPIC-020/RENDER-005.md) | Revocable overlay credentials and live scene revisions |
| [RENDER-090](EPIC-020/RENDER-090.md) | Verify native OBS capture on KDE Wayland |
| [RENDER-091](EPIC-020/RENDER-091.md) | Verify native and browser OBS outputs on Windows |
| [RENDER-092](EPIC-020/RENDER-092.md) | Verify Linux OBS browser-source output and media |

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
