# EPIC-025 — Windows portable ZIP, Linux archives and Arch package

Module: Distribution. Kind: epic. Specification: v0.45; §16.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `PACKAGETEST-001` → [PACKAGE-001](EPIC-025/PACKAGE-001.md) | Pinned native build and runtime dependency manifests |
| `PACKAGETEST-002` → [PACKAGE-002](EPIC-025/PACKAGE-002.md) | Windows installation-free portable ZIP |
| `PACKAGETEST-003` → [PACKAGE-003](EPIC-025/PACKAGE-003.md) | Bundled Linux desktop archive and relocation policy |
| `PACKAGETEST-004` → [PACKAGE-004](EPIC-025/PACKAGE-004.md) | Arch PKGBUILD against system dependencies |
| [PACKAGE-005](EPIC-025/PACKAGE-005.md) | Verify clean Windows portable startup media and moved login |
| [PACKAGE-006](EPIC-025/PACKAGE-006.md) | Verify Linux archives Arch package and secure keyrings |

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
