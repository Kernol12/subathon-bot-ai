# EPIC-010 — Twitch API, EventSub and both chat identities

Module: Integrations. Kind: epic. Specification: v0.45; §10, §14.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `TWITCHTEST-001` → [TWITCH-001](EPIC-010/TWITCH-001.md) | Installation authorization broadcaster bot and user grants |
| `TWITCHTEST-002` → [TWITCH-002](EPIC-010/TWITCH-002.md) | EventSub subscriptions gifts and Bits normalization |
| `TWITCHTEST-003` → [TWITCH-003](EPIC-010/TWITCH-003.md) | Chat ingress and stream-state signals |
| `TWITCHTEST-004` → [TWITCH-004](EPIC-010/TWITCH-004.md) | Explicit bot and broadcaster message delivery |
| `TWITCHTEST-005` → [TWITCH-005](EPIC-010/TWITCH-005.md) | Connection loss capability health and unrecoverable gaps |
| [TWITCH-090](EPIC-010/TWITCH-090.md) | Verify Twitch grants events senders and gap limitations live |

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
