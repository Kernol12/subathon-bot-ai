# SESSION-009 — Actual current session-clock producer

Parent: [EPIC-007](../EPIC-007.md). Paired baseline: SESSIONTEST-009.
Kinds: test_baseline / implementation. Specification v0.45 §10/§16,
AUTH-05/09/12, DB-01, SERVER-01, TEST-04/05.
Dependencies/status: progress/state.json. Contract: independently accepted
SESSIONCONTRACT-004; no clock mechanism is finalized by ADR-0010.

Own only the actual host session-clock producer and its reviewed continuity/trust
state integration, with focused server/test paths agreed in the claim. Read scoped
instructions. The integrator owns shared schemas/migrations/build/CI/state. Reuse
the existing writer and actual physical store where required by the contract;
neither a new writer nor hidden timer/recovery subsystem is permitted.

SESSIONTEST-009 derives independently reviewed deterministic clock/fence cases and
actual selected-platform observation tests as needed by the accepted mechanism.
Cover actual producer wiring and finite calls/lifetime, trustworthy samples and
signed extremes, sampling failure, rollback/discontinuity, startup/restart uncertainty,
recovery/restore refusal and explicitly allowed trust transitions. Keep fake-time
policy scenarios distinct from actual native-source evidence. Verify execution and
publication scope and that refusal cannot produce freshness/expiry extension.
Working dependencies, separate test approval and an executed intended behavior red
are required before implementation; old supplier-clock fakes are not this baseline.

SESSION-009 must pass the reviewed relevant suites and existing affected regressions,
obtain separate source/platform-trust/resource/configuration review and actual selected
CI. Demonstrate the real producer binding, not only deterministic policy helpers.
State which OS/runtime observations actually ran and which supported-platform gates
remain. Preserve original assertions/receipts and normal latest-head merge gates.

This producer does not implement timer countdown/checkpoint/downtime, accepted login,
recovery offers, staged restore or installation-epoch rotation protocols. ACCESS-005
and STORAGE-007 must test their actual use of its refusal/fence boundary. Record
exact revisions/commands/environment/results and those remaining gates in a handoff.


The accepted clock contract §2 assigns actual host/queue lifecycle to API-001. Both
SESSIONTEST-009 and SESSION-009 therefore require that real host explicitly. A
supplied readiness fixture combined with real native calls remains simulated host
evidence; it cannot complete this brief's actual producer-binding acceptance. The
clock owns its adapter/continuity/facade binding, not a parallel generic host issuer.
