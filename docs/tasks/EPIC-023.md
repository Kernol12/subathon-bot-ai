# EPIC-023 — English/German catalogs and Light/Dark/System

Module: Localization. Kind: epic. Specification: v0.45; §12.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `LOCALETEST-001` → [LOCALE-001](EPIC-023/LOCALE-001.md) | Shared translation catalog schema and build generator |
| `LOCALETEST-002` → [LOCALE-002](EPIC-023/LOCALE-002.md) | Native runtime language locale and saved preferences |
| `LOCALETEST-003` → [LOCALE-003](EPIC-023/LOCALE-003.md) | Web management and viewer runtime translation |
| `LOCALETEST-004` → [LOCALE-004](EPIC-023/LOCALE-004.md) | Independent Light Dark and System appearance |
| [LOCALE-090](EPIC-023/LOCALE-090.md) | Verify English German and theme layouts on native and browser UIs |

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
