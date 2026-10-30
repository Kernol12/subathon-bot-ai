# EPIC-006 — Invites, permissions, local/Twitch identities and chat roles

Module: Identity. Kind: epic. Specification: v0.45; §10.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `ACCESSTEST-001` → [ACCESS-001](EPIC-006/ACCESS-001.md) | Initial owner setup and invitation registration |
| `ACCESSTEST-002` → [ACCESS-002](EPIC-006/ACCESS-002.md) | Effective permissions and complete-grant constraints |
| `ACCESSTEST-003` → [ACCESS-003](EPIC-006/ACCESS-003.md) | Delegation and authorized automation capabilities |
| `ACCESSTEST-004` → [ACCESS-004](EPIC-006/ACCESS-004.md) | Account login and optional Twitch identity linking |
| `ACCESSTEST-005` → [ACCESS-005](EPIC-006/ACCESS-005.md) | Recovery codes and administrative recovery |
| `ACCESSTEST-006` → [ACCESS-006](EPIC-006/ACCESS-006.md) | Two-party ownership transfer and fresh authentication |
| `ACCESSTEST-007` → [ACCESS-007](EPIC-006/ACCESS-007.md) | Chat command categories arguments and cooldowns |

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
