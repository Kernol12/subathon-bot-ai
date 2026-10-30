# EPIC-007 — Session/credential encryption, keys and remembered login

Module: Identity. Kind: epic. Specification: v0.45; §10, §16.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `SESSIONTEST-001` → [SESSION-001](EPIC-007/SESSION-001.md) | Password hashing and parameter upgrades |
| `SESSIONTEST-002` → [SESSION-002](EPIC-007/SESSION-002.md) | Opaque sessions expiry revocation and freshness |
| `SESSIONTEST-003` → [SESSION-003](EPIC-007/SESSION-003.md) | Browser cookie CSRF and desktop header transport |
| `SESSIONTEST-004` → [SESSION-004](EPIC-007/SESSION-004.md) | Windows DPAPI remembered-login storage |
| `SESSIONTEST-005` → [SESSION-005](EPIC-007/SESSION-005.md) | Linux QtKeychain remembered-login fallback |
| `SESSIONTEST-006` → [SESSION-006](EPIC-007/SESSION-006.md) | Encrypted provider credentials and protected key provisioning |
| `SESSIONTEST-007` → [SESSION-007](EPIC-007/SESSION-007.md) | Crash-safe credential encryption-key rotation |

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
